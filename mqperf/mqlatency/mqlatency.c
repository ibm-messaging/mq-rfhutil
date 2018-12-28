/*
Copyright (c) IBM Corporation 2000, 2018
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Contributors:
Jim MacNair - Initial Contribution
*/

/********************************************************************/
/*                                                                  */
/*   MQLatency has 1 required input parameter                       */
/*                                                                  */
/*      -f name of the parameter file                               */
/*                                                                  */
/*   This program will write messages to a queue.  It will then     */
/*   read the reply messages from a different queue.  It will       */
/*   calculate the time from when the request message is written    */
/*   to when the reply message arrives on the reply queue.          */
/*                                                                  */
/*   Additional parameters can be used to override certain of the   */
/*   parameters in the parameter file, including the message        */
/*   count, queue manager name and queue names.  The think time     */
/*   parameter can also be overriden.                               */
/*                                                                  */
/*   The  parameter file contains all values, including the name    */
/*   of the queues and queue manager to write data to, the total    */
/*   number of messages to write, any MQMD parameters and a list    */
/*   of files which contain the message data.                       */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef SOLARIS
#include <unistd.h>
#endif

/* definitions of 64-bit values for platform independence */
#include "int64defs.h"

#ifndef WIN32
void Sleep(int amount)
{
	usleep(amount*1000);
}
#endif

/* includes for MQI */
#include <cmqc.h>

/* includes for common subroutines */
#include "comsubs.h"
#include "timesubs.h"

/* definition of parameters area */
#include "parmline.h"

/* include for MQ user subroutines */
#include "qsubs.h"
#include "rfhsubs.h"

/* parameter file processing routines */
#include "putparms.h"

#define MAX_BATCH_ALLOW		5000

static char copyright[] = "\n(C) Copyright IBM Corp, 2008-2014";
static char Version[]=\
"@(#)MQLatency V3.0 - Latency measurement tool  - Jim MacNair ";

#ifdef _DEBUG
static char Level[]="mqlatency.c V3.0 Debug version ("__DATE__" "__TIME__")";
#else
static char Level[]="mqlatency.c V3.0 Release version ("__DATE__" "__TIME__")";
#endif

	MQHCONN			qm=0;			/* queue manager connection handle */
	MQHOBJ			qReply=0;		/* queue handle used for mqget     */
	MQHOBJ			q=0;			/* queue handle used for mqput     */
	MQHOBJ			Hinq;			/* inquire object handle           */

	/* global termination switch */
	volatile int	terminate=0;
	volatile int	cancelled=0;

/**************************************************************/
/*                                                            */
/* This routine puts a message on the queue.                  */
/*                                                            */
/**************************************************************/

int getMessage(FILEPTR* fptr, int maxWait, PUTPARMS * parms)

{
	MQLONG			compcode=0;
	MQLONG			reason=0;
	unsigned int	msgSize=0;
	MQMD2			mqmd = {MQMD2_DEFAULT};
	MQGMO			gmo = {MQGMO_DEFAULT};
 
	/* set the get message options */
	gmo.Options = MQGMO_WAIT | MQGMO_FAIL_IF_QUIESCING | MQGMO_NO_SYNCPOINT | MQGMO_ACCEPT_TRUNCATED_MSG;
	gmo.Version = MQGMO_VERSION_2;
	gmo.MatchOptions = MQMO_NONE;

	/* check if correlation ids are to be used to read the reply messages */
	if (1 == parms->GetByCorrelId)
	{
		/* set the correlation id match option in the gmo */
		gmo.MatchOptions = MQMO_MATCH_CORREL_ID;

		/* set the correlation id to look for */
		memcpy(mqmd.CorrelId, parms->savedMsgId, MQ_CORREL_ID_LENGTH);
	}

	/* set the maximum wait time to 5 seconds */
	gmo.WaitInterval = maxWait;

	MQGET(qm, qReply, &mqmd, &gmo, 0, NULL, (MQLONG *)&msgSize, &compcode, &reason);

	/* check for truncated message */
	if (MQRC_TRUNCATED_MSG_ACCEPTED == reason)
	{
		/* accept the truncated message since there is no need for the data */
		reason = MQRC_NONE;
		compcode = MQCC_OK;
	}

	/* if no wait is specified this is a request to drain the queue */
	/* therefore suppress a 2033 (no message found) return code */
	if ((0 == maxWait) && (MQRC_NO_MSG_AVAILABLE == reason))
	{
		/* do not treat this case as an error */
		/* just return the completion code so the caller knows there are no more messages in the queue */
		return compcode;
	}

	/* check for errors */
	checkerror("MQGET", compcode, reason, parms->replyQ);

	/* calculate the total number of bytes in the reply, even though it wasn't read */
	parms->totMsgLen += msgSize;

	/* count the number of messages that were read */
	parms->msgsRead++;

	return reason;
}

/**************************************************************/
/*                                                            */
/* This routine puts a message on the queue.                  */
/*                                                            */
/**************************************************************/

int putMessage(FILEPTR* fptr, MQCHAR8 *puttime, PUTPARMS * parms)

{
	MQLONG	compcode=0;
	MQLONG	reason=0;
	MQLONG	putLen;
	MQMD2	msgdesc = {MQMD2_DEFAULT};
	MQPMO	mqpmo = {MQPMO_DEFAULT};

	/* check if we are using an mqmd from the file */
	if (fptr->mqmdptr != NULL)
	{
		/* set the get message options */
		/* no synchpoint, each message as a separate UOW */
		mqpmo.Options = MQPMO_NO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING | MQPMO_SET_ALL_CONTEXT;

		if (1 == fptr->newMsgId)
		{
			mqpmo.Options |= MQPMO_NEW_MSG_ID;
		}

		/* set the MQMD */
		memcpy(&msgdesc, fptr->mqmdptr, sizeof(MQMD));
	}
	else
	{
		/* set the get message options */
		/* no synchpoint, each message as a separate UOW */
		mqpmo.Options = MQPMO_NO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING | MQPMO_NEW_MSG_ID;

		/* Indicate V2 of MQMD */
		msgdesc.Version = MQMD_VERSION_2;

		/* set the persistence, etc if specified */
		msgdesc.Persistence = fptr->Persist;
		msgdesc.Encoding = fptr->Encoding;
		msgdesc.CodedCharSetId = fptr->Codepage;

		/* check if message expiry was specified */
		if (fptr->Expiry > 0)
		{
			msgdesc.Expiry = fptr->Expiry;
		}

		/* check if message type was specified */
		if (fptr->Msgtype > 0)
		{
			msgdesc.MsgType = fptr->Msgtype;
		}

		/* check if message priority was specified */
		if (fptr->Priority != MQPRI_PRIORITY_AS_Q_DEF)
		{
			msgdesc.Priority = fptr->Priority;
		}

		/* check if report options were specified */
		if (fptr->Report > 0)
		{
			msgdesc.Report = fptr->Report;
		}

		/* set the message format in the MQMD was specified */
		switch (fptr->hasRFH)
		{
		case RFH_NO:
			{
				if (1 == fptr->FormatSet)
				{
					memcpy(msgdesc.Format, fptr->Format, MQ_FORMAT_LENGTH);
				}

				break;
			}
		case RFH_V1:
			{
				memcpy(msgdesc.Format, MQFMT_RF_HEADER, sizeof(msgdesc.Format));
				break;
			}
		case RFH_V2:
			{
				memcpy(msgdesc.Format, MQFMT_RF_HEADER_2, sizeof(msgdesc.Format));
				break;
			}
		case RFH_XML:
			{
				memcpy(msgdesc.Format, MQFMT_RF_HEADER_2, sizeof(msgdesc.Format));
				break;
			}
		}

		/* check if a reply to queue manager was specified */
		memset(msgdesc.ReplyToQMgr, 0, sizeof(msgdesc.ReplyToQMgr));
		if (fptr->ReplyQM[0] != 0)
		{
			memcpy(msgdesc.ReplyToQMgr, fptr->ReplyQM, strlen(fptr->ReplyQM));
		}

		/* check if a reply to queue was specified */
		memset(msgdesc.ReplyToQ, 0, sizeof(msgdesc.ReplyToQ));
		if (fptr->ReplyQ[0] != 0)
		{
			memcpy(msgdesc.ReplyToQ, fptr->ReplyQ, strlen(fptr->ReplyQ));
		}

		/* check if a correl id was specified */
		if (parms->correlidSet == 1)
		{
			memcpy(msgdesc.CorrelId, fptr->CorrelId, MQ_CORREL_ID_LENGTH);
		}
		else
		{
			memset(msgdesc.CorrelId, 0, MQ_CORREL_ID_LENGTH);
		}

		/* check if an accounting token was specified */
		if (1 == fptr->AcctTokenSet)
		{
			/* set the accounting token value */
			memcpy(msgdesc.AccountingToken, fptr->AccountingToken, MQ_ACCOUNTING_TOKEN_LENGTH);
		}
	}

	/* perform the MQPUT */
	putLen = (MQLONG)fptr->length;
	MQPUT(qm, q, &msgdesc, &mqpmo, putLen, fptr->dataptr, &compcode, &reason);

	/* check for errors */
	checkerror("MQPUT", compcode, reason, parms->qname);

	/* check if correlation ids are to be used to read the reply messages */
	if (1 == parms->GetByCorrelId)
	{
		/* save the message id to match with the expected reply id */
		/* if this option is used the application must set the correlation id in the reply message */
		memcpy(parms->savedMsgId, msgdesc.MsgId, MQ_MSG_ID_LENGTH);
	}

	if (0 == compcode)
	{
		/* keep track of the number of bytes we have written */
		parms->byteswritten += fptr->length;
	}
	else
	{
		/* set the global error switch */
		parms->err = 1;
	}

	memcpy(puttime, msgdesc.PutTime, sizeof(msgdesc.PutTime));

	return compcode;
}


void InterruptHandler (int sigVal) 

{ 
	/* force program to end */
	terminate = 1;

	/* indicate user cancelled the program */
	cancelled = 1;
}

/**************************************************************/
/*                                                            */
/* Display command format.                                    */
/*                                                            */
/**************************************************************/

void printHelp(char *pgmName)

{
	printf("format is:\n");
	printf("  %s -f parm_file {-v} {-m QMgr} {-q queue} {-c count} {-r replyQ} {-t thinktime}\n", pgmName);
	printf("   parm_file is the fully qualified name of the parameters file\n");
	printf("   -v verbose\n");
	printf("   -p purge reply queue before starting\n");
	printf("   -i use correlation id to read reply messages\n");
	printf("   Overrides\n");
	printf("   -m name of queue manager\n");
	printf("   -q name of queue\n");
	printf("   -r name of reply queue\n");
	printf("   -c message count\n");
	printf("   -t think time\n");
}

int main(int argc, char **argv)

{
	int64_t		latency10=0;		/* latency < 10 microseconds */
	int64_t		latency100=0;		/* latency > 10 microseconds and < 100 microseconds */
	int64_t		latency500=0;		/* latency > 10 microseconds and < 100 microseconds */
	int64_t		latency1000=0;		/* latency > 100 microseconds and < 1 milliseconds */
	int64_t		latency5000=0;		/* latency > 1 millisecond and < 5 milliseconds */
	int64_t		latency10000=0;		/* latency > 5 milliseconds and < 10 milliseconds */
	int64_t		latency50000=0;		/* latency > 10 milliseconds and < 50 milliseconds */
	int64_t		latency100000=0;	/* latency > 50 milliseconds and < 100 milliseconds */
	int64_t		latency500000=0;	/* latency > 100 milliseconds and < 500 milliseconds */
	int64_t		latency1000000=0;	/* latency > 500 millioseconds and < 1 second */
	int64_t		latency10000000=0;	/* latency > 1 second and < 10 seconds */
	int64_t		latency100000000=0;	/* latency > 10 seconds */
	int64_t		avglatency;				/* average latency */
	int64_t		elapsed=0;
	int64_t		latency=0;
	int64_t		totalLatency=0;			/* total of all latency observed during test - used to calculate average latency */
	int64_t		minLatency=0;			/* minimum latency observed during test */
	int64_t		maxLatency=0;			/* maximum latency observed during test */
	int64_t		tempLatency=0;			/* latency in milliseconds */
	double		avgrate;
	MQLONG		compcode=MQCC_OK;
	MQLONG		reason;
	MQOD		objdesc = {MQOD_DEFAULT};
	MQLONG		openopt = 0;
	MQLONG		maxMsgLen=0;
	int			numWrittenMin=-1;
	int			numWrittenMax=0;
	int64_t		MsgsAtLastInterval=0;
	int			iElapsed=0;
	int			saveGetByCorrelId;
	MQCHAR8		puttime;
	char		formTime[16];
	char		filename[512];
	MY_TIME_T	startTime;
	MY_TIME_T	endTime;
	MY_TIME_T	prevTime;
	MY_TIME_T	afterGet;
	MY_TIME_T	afterPut;
	MY_TIME_T	timeNow;
	time_t		startTOD;
	time_t		endTOD;
	FILEPTR		*fptr=NULL;
	FILEPTR		*fileptr;
	char		avgLatency[16];
	char		minLat[16];
	char		maxLat[16];
	PUTPARMS	parms;

	/* print the copyright statement */
	Log(copyright);
	Log(Level);

	/* initialize the work areas */
	memset(filename, 0, sizeof(filename));
	memset(&parms, 0, sizeof(parms));
	memset(minLat, 0, sizeof(minLat));
	memset(maxLat, 0, sizeof(maxLat));
	memset(avgLatency, 0, sizeof(avgLatency));

	/* initialize the work areas */
	initializeParms(&parms, sizeof(PUTPARMS));

	/* check for too few input parameters */
	if (argc < 2)
	{
		printHelp(argv[0]);
		exit(99);
	}

	/* check for help request */
	if ((argv[1][0] == '?') || (argv[1][1] == '?'))
	{
		printHelp(argv[0]);
		exit(0);
	}

	/* process any command line arguments */
	processArgs(argc, argv, &parms);

	if (parms.err != 0)
	{
		printHelp(argv[0]);
		exit(99);
	}

	/* initialize the queue name and queue manager name */
	memset(parms.qname, ' ', sizeof(parms.qname) - 1);
	memset(parms.replyQ, ' ', sizeof(parms.replyQ) - 1);
	memset(parms.qmname, ' ', sizeof(parms.qmname) - 1);

	/* process the parameters file data */
	fptr = processParmFile(parms.parmFilename, &parms, READDATAFILES);

	/* start with the first message */
	fileptr = fptr;

	/* check for overrides */
	processOverrides(&parms);

	/* check if we found any message data files */
	if (NULL == fptr)
	{
		Log("***** No message data files found - program terminating");
		return 94;
	}

	/* check that the reply to queue name has been specified */
	if ((0 == parms.replyQ[0]) || (' ' == parms.replyQ[0]))
	{
		Log("***** Reply to queue name must be specified (REPLYQ parameter)");
		return 93;
	}

	/* check that the queue name has been specified */
	if ((0 == parms.qname[0]) || (' ' == parms.qname[0]))
	{
		Log("***** queue name must be specified (QNAME parameter or -q on command line)");
		return 93;
	}

	if (parms.err != 0)
	{
		Log("***** Error detected (err=%d) - program terminating", parms.err);
		return parms.err;
	}

	/* tell how many files and messages we found */
	Log("Total files read %d", parms.fileCount);
	Log("Total messages found %d", parms.mesgCount);

	/* explain what parameters are being used */
	Log("\n" FMTI64 " messages to be written to queue %s on queue manager %s", parms.totcount, parms.qname, parms.qmname);

	/* was a delay time between messages specified? */
	if (parms.thinkTime > 0)
	{
		/* display the wait time between messages */
		Log("wait time between messages = %d", parms.thinkTime);
	}

	/* set a termination handler */
	signal(SIGINT, InterruptHandler);

	/* Connect to the queue manager */
#ifdef MQCLIENT
	clientConnect2QM(parms.qmname, &qm, &maxMsgLen, &compcode, &reason);
#else
	connect2QM(parms.qmname, &qm, &compcode, &reason);
#endif

	/* check for errors */
	if (compcode != MQCC_OK)
	{
		return 98;
	}

	/* set the queue open options */
	strcpy(objdesc.ObjectName, parms.qname);
	strcpy(objdesc.ObjectQMgrName, parms.remoteQM);
	openopt = MQOO_OUTPUT + MQOO_FAIL_IF_QUIESCING;

	/* check if we need to set all context */
	if (1 == parms.foundMQMD)
	{
		openopt |= MQOO_SET_ALL_CONTEXT;
	}

	/* open the queue for output */
	Log("opening queue %s for output", parms.qname);
	MQOPEN(qm, &objdesc, openopt, &q, &compcode, &reason);

	/* check for errors */
	checkerror("MQOPEN", compcode, reason, parms.qname);
	if (compcode != MQCC_OK)
	{
		return 97;
	}

	/* set the queue open options */
	strcpy(objdesc.ObjectName, parms.replyQ);
	openopt = MQOO_INPUT_SHARED + MQOO_FAIL_IF_QUIESCING;

	/* open the queue for input */
	Log("opening queue %s for input", parms.replyQ);
	MQOPEN(qm, &objdesc, openopt, &qReply, &compcode, &reason);

	/* check for errors */
	checkerror("MQOPEN2", compcode, reason, parms.replyQ);
	if (compcode != MQCC_OK)
	{
		/* close the output queue */
		MQCLOSE(qm, &q, MQCO_NONE, &compcode, &reason);
		return 96;
	}

	/* should the reply queue be purged? */
	if (1 == parms.purgeQ)
	{
		/* save the get by correlation id option */
		saveGetByCorrelId = parms.GetByCorrelId;

		/* do not use a get by correlation id */
		parms.GetByCorrelId = 0;

		/* purge the reply queue to make sure the times actually reflect the correct messages */
		compcode = MQCC_OK;
		while (MQCC_OK == compcode)
		{
			/* read any spurious reply messages with no wait time */
			/* the reply queue must be empty before the latency measurements begin */
			compcode = getMessage(fptr, 0, &parms);
		}

		/* restore the get by correlation id */
		parms.GetByCorrelId = saveGetByCorrelId;
	}

	/* remember the starting time */
	GetTime(&startTime);
	GetTime(&prevTime);

	/* reset the completion code */
	compcode = MQCC_OK;

	/* loop until all the messages have been written or an error occurs */
	while ((compcode == MQCC_OK) && (0 == terminate) && (0 == parms.err) && (parms.msgwritten < parms.totcount))
	{
		if (0 == parms.msgwritten)
		{
			/* write out the time the first messsage was sent */
			time(&startTOD);
			Log("First message written at %s", ctime(&startTOD));
		}

		/* perform the MQPUT */
		compcode = putMessage(fileptr, &puttime, &parms);

		/* check for errors */
		if (compcode == MQCC_OK)
		{
			/* get the time after the put */
			GetTime(&afterPut);

			/* increment the message count */
			parms.msgwritten++;

			/* read the reply message with a maximum wait - default is 5 seconds */
			/* this can be overridden on the command line using the -w parameter */
			compcode = getMessage(fileptr, parms.maxWaitTime * 1000, &parms);

			if (compcode != MQCC_OK)
			{
				/* let the user know about the error */
				Log("***** Error reading reply message %d", reason);

				/* error reading message - time to exit */
				parms.err = 1;
			}
			else
			{
				/* get the time after the MQGET */
				GetTime(&afterGet);

				/* calculate the latency */
				latency = DiffTime(afterPut, afterGet);
				totalLatency += latency;

				/* check if this is the lowest latency so far */
				if ((0 == minLatency) || (minLatency > latency))
				{
					/* update the minimum latency */
					minLatency = latency;
				}

				/* check if this is the highest latency so far */
				if (latency > maxLatency)
				{
					/* update the minimum latency */
					maxLatency = latency;
				}

				/* keep track of the number of latencies by ranges */
				/* of microseconds, from less than 10 to over 10 seconds */
				if (latency < 10)
				{
					/* increment counter */
					latency10++;
				} 
				else if (latency < 100)
				{
					/* increment counter */
					latency100++;
				} 
				else if (latency < 500)
				{
					/* increment counter */
					latency500++;
				} 
				else if (latency < 1000)
				{
					/* increment counter */
					latency1000++;
				} 
				else if (latency < 5000)
				{
					/* increment counter */
					latency5000++;
				} 
				else if (latency < 10000)
				{
					/* increment counter */
					latency10000++;
				} 
				else if (latency < 50000)
				{
					/* increment counter */
					latency50000++;
				} 
				else if (latency < 100000)
				{
					/* increment counter */
					latency100000++;
				} 
				else if (latency < 500000)
				{
					/* increment counter */
					latency500000++;
				} 
				else if (latency < 1000000)
				{
					/* increment counter */
					latency1000000++;
				} 
				else if (latency < 10000000)
				{
					/* increment counter */
					latency10000000++;
				}
				else
				{
					/* increment counter */
					latency100000000++;
				}

				/* check if we are supposed to report progress */
				if ((parms.reportEvery > 0) && ((parms.msgwritten % parms.reportEvery) == 0))
				{
					/* report the time to put a given number of messages */
					/* calculate the amount of time it took */
					GetTime(&timeNow);
					elapsed = DiffTime(prevTime, timeNow);

					/* update the time for the next interval */
					GetTime(&prevTime);

					/* avoid divide by zero errors */
					if (parms.msgwritten > 0)
					{
						/* calculate the average latency */
						tempLatency = totalLatency / parms.msgwritten;
					}
					else
					{
						/* no messages yet - just set to zero */
						tempLatency = 0;
					}

					/* format the difference as a string (seconds and 6 decimal places) */
					formatTimeDiffSecs(formTime, elapsed);
					formatTimeDiff(avgLatency, tempLatency);
					formatTimeDiff(minLat, minLatency);
					formatTimeDiff(maxLat, maxLatency);

					if (elapsed > 0)
					{
						/* get the message rate */
						avgrate = (double)(((parms.msgwritten - MsgsAtLastInterval) * 1000000) / elapsed);

						/* write out the time it took to write the messages */
						Log(FMTI64 " messages written in %s seconds rate %.2f", parms.msgwritten - MsgsAtLastInterval, formTime, avgrate);
					}
					else
					{
						/* write out the time it took to write the messages without the rate */
						Log(FMTI64 " messages written in %s seconds minimum latency=%s maximum latency=%s", parms.msgwritten - MsgsAtLastInterval, formTime, minLat, maxLat);
					}

					/* remember the count at the beginning of the next interval */
					MsgsAtLastInterval = parms.msgwritten;

					/* display the minimum, maximum and average latencies */
					Log("Min latency = %s  Max latency = %s Average latency = %s", minLat, maxLat, avgLatency);
				}

				/* was a think time specified? */
				if (fileptr->thinkTime > 0)
				{
					/* delay the amount specified by the think time parameter */
					Sleep(fileptr->thinkTime);
				}

				/* move on to the next message file */
				fileptr = (FILEPTR *)fileptr->nextfile;
				if (NULL == fileptr)
				{
					/* go back to the first message data file */
					fileptr = fptr;
				}
			}
		}
	} /* while */

	/* remember the ending time */
	GetTime(&endTime);

	/* issue message if user cancelled the program */
	if (1 == terminate)
	{
		if (1 == cancelled)
		{
			Log("Program cancelled by user after %d messages written", parms.msgwritten);
		}
		else
		{
			Log("Program terminated due to error after %d messages written", parms.msgwritten);
		}
	}

	/* write out the time the first messsage was sent */
	time(&endTOD);
	Log("Last message written at %s", ctime(&endTOD));

	/* dump out the total message count */
	Log("Total messages read %d", parms.msgsRead);
	Log("Total bytes read      %d", parms.totMsgLen);
	Log("Total memory used %d", parms.memUsed);
	Log("\nTotal messages written " FMTI64 " out of " FMTI64, parms.msgwritten, parms.totcount);
	Log("Total bytes written   " FMTI64, parms.byteswritten);

	if (parms.msgwritten > 0)
	{
		/* calculate the total elapsed time */
		elapsed = DiffTime(startTime, endTime);
		formatTimeDiffSecs(formTime, elapsed);
		Log("Total elapsed time in seconds %s", formTime);

		/* calculate the average latency */
		avglatency = totalLatency / parms.msgwritten;
		formatTimeDiff(avgLatency, avglatency);
		formatTimeDiff(minLat, minLatency);
		formatTimeDiff(maxLat, maxLatency);

		/* display the minimum, maximum and average latencies */
		Log("Min latency = %s  Max latency = %s Average latency = %s", minLat, maxLat, avgLatency);
	}

	/* display the counters */
	/* Some of the counters are only displayed if there were actually */
	/* messages that took that amount of time to be processed */
	if (latency10 > 0)
	{
		/* less than 0.01 millsecond */
		Log("Less than 10 Microseconds   " FMTI64, latency10);
	}

	if ((latency10 > 0) || (latency100 > 0))
	{
		/* less than 0.1 millisecond */
		Log("Less than 100 Microseconds  " FMTI64, latency100);
	}

	if ((latency10 > 0) || (latency100 > 0) || (latency500 > 0))
	{
		/* less than 0.5 millisecond */
		Log("Less than 500 Microseconds  " FMTI64, latency500);
	}

	/* always display these counters */
	/* range covers 0.5 millsecond to 10 seconds */
	Log("Less than 1 Millisecond     " FMTI64, latency1000);
	Log("Less than 5 Milliseconds    " FMTI64, latency5000);
	Log("Less than 10 Milliseconds   " FMTI64, latency10000);
	Log("Less than 50 Milliseconds   " FMTI64, latency50000);
	Log("Less than 100 Milliseconds  " FMTI64, latency100000);
	Log("Less than 500 Milliseconds  " FMTI64, latency500000);
	Log("Less than 1 Second          " FMTI64, latency1000000);
	Log("Less than 10 Seconds        " FMTI64, latency10000000);

	/* check if this counter actually has been used */
	if (latency100000000 > 0)
	{
		Log("Over 10 Seconds             " FMTI64, latency100000000);
	}

	/* close the output queue */
	Log("\nclosing the output queue");
	MQCLOSE(qm, &q, MQCO_NONE, &compcode, &reason);

	checkerror("MQCLOSE", compcode, reason, parms.qname);

	/* close the input queue */
	Log("closing the reply queue");
	MQCLOSE(qm, &qReply, MQCO_NONE, &compcode, &reason);

	checkerror("MQCLOSE", compcode, reason, parms.replyQ);

	/* Disconnect from the queue manager */
	Log("disconnecting from the queue manager");
	MQDISC(&qm, &compcode, &reason);

	checkerror("MQDISC", compcode, reason, parms.qmname);

	/* release any storage used for RFH areas */
	releaseRFH(&parms);

	/* release any storage we acquired for files */
	fileptr = fptr;
	while (fileptr != NULL)
	{
		/* do we have any acquired storage associated with this control block */
		if (fileptr->acqStorAddr != NULL)
		{
			/* release the acquired storage */
			free(fileptr->acqStorAddr);
		}

		/* remember the address of the current control block */
		fptr = fileptr;

		/* move on to the next control block */
		fileptr = (FILEPTR *)fileptr->nextfile;

		/* release the FILEPTR control block */
		free(fptr);
	}

	/******************************************************************/
	/*                                                                */
	/* END OF PROGRAM                                                 */
	/*                                                                */
	/******************************************************************/

	Log("MQLatency program ended");

	return(0);
}
