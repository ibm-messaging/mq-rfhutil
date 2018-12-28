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
/*   MQTIMES2 supports the following parameters                     */
/*                                                                  */
/*      -c message count (number to read before ending)             */
/*      -q the name of the input queue (required)                   */
/*      -m queue manager name (optional)                            */
/*      -b batch size (number of messages in a unit of work)        */
/*      -p drain queue before starting measurements                 */
/*                                                                  */
/*    if no queue manager is specified, the default queue manager   */
/*    is used.                                                      */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "signal.h"
#include "time.h"

#ifdef WIN32
#include "windows.h"
#endif

/* includes for MQI */
#include <cmqc.h>
#include <cmqxc.h>

#ifdef SOLARIS
#include <sys/types.h>
#endif

#include "int64defs.h"
#include "comsubs.h"
#include "timesubs.h"
#include "parmline.h"
#include "putparms.h"
#include "qsubs.h"
#include "rfhsubs.h"

/* global error switch */
	int		err=0;

/* global termination switch */
	volatile int	terminate=0;
	volatile int	cancelled=0;

static char copyright[] = "(C) Copyright IBM Corp, 2001/2002/2004/2005/2014";
static char Version[]=\
"@(#)MQTimes2 V3.0 - MQ Performance results tool  - Jim MacNair ";

#ifdef _DEBUG
static char Level[]="mqtimes2.c V3.0 Debug version ("__DATE__" "__TIME__")";
#else
#ifdef MQCLIENT
static char Level[]="mqtimes2c.c V3.0 Client version ("__DATE__" "__TIME__")";
#else
static char Level[]="mqtimes2.c V3.0 Release version ("__DATE__" "__TIME__")";
#endif
#endif

void InterruptHandler (int sigVal) 
{ 
	/* force program to end */
	terminate = 1;

	/* indicate user cancelled the program */
	cancelled = 1;
}

void printHelp(char *pgmName)

{
	printf("\nformat is:\n");
	printf("   %s <-c Count> <-q Queue> <-m Queue manager> <-f Parameters file> <-p> <-b nnn>\n", pgmName);
	printf("    Count is the number of messages to read before stopping.\n");
	printf("    Queue is the name of the queue to read messages from.\n");
#ifdef MQCLIENT
	printf("    Queue manager is the name of the queue manager that holds the input queue\n");
	printf("     or the format of an MQSERVER variable - channel name/TCP/hostname(port).\n");
#else
	printf("    Queue manager is the name of the queue manager that holds the input queue.\n");
#endif
	printf("    The -p option will purge the queue before starting the measurement.\n");
	printf("     Any messages in the queue will be discarded.\n");
	printf("    The -b option specifies the number of messages in a single unit of work.\n");
	printf("    If the program must respond to either PAN or NAN report options, a file\n");
	printf("     containing the data to be used for the reply message must be provided\n");
	printf("     and specified in the parameters file.\n");
}

int main(int argc, char **argv)
{
	int64_t		msgcount=0;
	int64_t		totcount=0;
	int64_t		totalbytes=0;
	int64_t		avgbytes;
	int64_t		maxrate=0;
	int64_t		firstsec=0;
	int64_t		secondcount=0;
	int64_t		recent10;
	int64_t		recent10sec;
	int64_t		last10[10];			/* Average rate of last 10 intervals */
	int64_t		last10secs[10];		/* time of last 10 intervals */
	int64_t		tempLatency=0;		/* latency in milliseconds */
	int64_t		totalLatency=0;		/* latency in milliseconds */
	int64_t		currLatency=0;		/* last observed latency in milliseconds */
	int64_t		minLatency=0;		/* minimum latency observed */
	int64_t		maxLatency=0;		/* maximum latency observed */
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
	int64_t		latencyCount=0;		/* number of results in totalLatency */
	int64_t		lastLatencyCount=0;
	double		avgrate;
	size_t		mallocSize;
	MQLONG		datalen=0;
	int			minSize;
	int			uow=0;
	int			drainCount=0;
	time_t		firstTime=0;
	time_t		secondTime=0;
	time_t		lastTime=0;
	time_t		prevLastTime=0;
	int			remainingTime=0;
	int			firstInterval=1;	/* first interval indicator to not report recent average */
	int			secs;				/* work variable - number of seconds between first and last interval */
	int			i;
	int			reportCount=0;
	MQLONG		report;				/* MQ report options */
	MQHCONN		qm=0;
	MQHOBJ		q=0;
	MQLONG		compcode;
	MQLONG		reason;
	MQLONG		cc2;
	MQLONG		rc2;
	MQOD		objdesc = {MQOD_DEFAULT};
	MQMD2		msgdesc = {MQMD2_DEFAULT};
	MQLONG		openopt = 0;
	MQGMO		mqgmo = {MQGMO_DEFAULT};
	MQPMO		mqpmo = {MQPMO_DEFAULT};
	MQMD2		replyMsg = {MQMD2_DEFAULT};
	char		*msgdata;
	char		*userPtr;
	char		prevtime[9];
	char		currtime[9];
	MQLONG		prevtime_secs;
	MQLONG		currtime_secs;
	char		*msgPtr;
	char		timeFirst[16];
	char		timeLast[16];
	char		lastLatency[16];
	char		avgLatency[16];
	char		minLat[16];
	char		maxLat[16];
	char		tempCount[16];
	char		tempTotal[16];
	char		tempAvg[32];
	char		msgArea[10240];
	PUTPARMS	parms;				/* command line arguments and parameter file values */

	MY_TIME_T	endTime;			/* high performance counter to measure latency */
	MY_TIME_T	startTime;			/* high performance counter to measure latency */

	/* display the program name and version information */
	Log("%s program start", Level);

	/* print the copyright statement */
	Log(copyright);

	/* initialize the parameters and arguments area */
	initializeParms(&parms, sizeof(PUTPARMS));

	/* initialize the work areas */
	memset(prevtime, 0, sizeof(prevtime));
	memset(currtime, 0, sizeof(currtime));
	memset(msgArea, 0, sizeof(msgArea));

	for (i=0; i<10; i++)
	{
		last10[i] = 0;
		last10secs[i] = 0;
	}

	prevtime_secs = currtime_secs = 0;

	/* check for too few input parameters */
	if (argc < 2)
	{
		printf("Too few arguments\n");
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

	/* check for any parameters file */
	if (parms.parmFilename[0] != 0)
	{
		/* process the parameters file */
		processParmFile(parms.parmFilename, &parms, 0);
	}

	/* process the command line arguments, including any overrides */
	processOverrides(&parms);

	/* are latency measurements enabled? */
	if (1 == parms.setTimeStamp)
	{
		/* check if the timestamp is in the MQMD accounting token field */
		if (1 == parms.timeStampInAccountingToken)
		{
			/* get the start time from the MQMD Accounting Token field */
			Log("Latency measurements using MQMD Accounting Token");
		}
		else if (1 == parms.timeStampInCorrelId)
		{
			/* get the start time from the MQMD Correlation ID field */
			Log("Latency measurements using MQMD Correlation Id");
		}
		else if (1 == parms.timeStampInGroupId)
		{
			/* get the start time from the MQMD Group ID field */
			Log("Latency measurements using MQMD Group Id");
		}
		else if (1 == parms.timeStampUserProp)
		{
			/* get the start time from the RFH2 usr folder */
			Log("Latency measurements using timestamp in RFH2 usr folder");
		}
		else
		{
			/* check if the message is long enough */
			Log("Latency stored at offset %d", parms.timeStampOffset);
		}
	}

	/* set a termination handler */
	signal(SIGINT, InterruptHandler);

	/* allocate a buffer for the message */
	/* do this after the command line arguments are processed */
	mallocSize = (unsigned int)parms.maxmsglen;
	msgdata = (char *)malloc(mallocSize);

	/* check if the allocate worked */
	if (NULL == msgdata)
	{
		/* allocate failed - exit with error */
		Log("Memory allocation failed");
		return 95;
	}

	/* initialize the message data buffer */
	memset(msgdata, 0, mallocSize);

	/* point to the message area */
	msgPtr = msgArea;

	/* Connect to the queue manager */
#ifdef MQCLIENT
	clientConnect2QM(parms.qmname, &qm, &parms.maxmsglen, &compcode, &reason);
#else
	connect2QM(parms.qmname, &qm, &compcode, &reason);
#endif

	/* check for errors */
	checkerror("MQCONN", compcode, reason, parms.qmname);
	if (compcode != MQCC_OK)
	{
		free(msgdata);
		return 98;
	}

	/* set the queue open options */
	strncpy(objdesc.ObjectName, parms.qname, MQ_Q_NAME_LENGTH);
	openopt = MQOO_INPUT_SHARED | MQOO_FAIL_IF_QUIESCING;

	/* open the queue for input */
	Log("opening queue %s for input", parms.qname);
	MQOPEN(qm, &objdesc, openopt, &q, &compcode, &reason);

	/* check for errors */
	checkerror("MQOPEN", compcode, reason, parms.qname);
	if (compcode != MQCC_OK)
	{
		/* release any acquired storage */
		free(msgdata);

		/* disconnect from the queue manager */
		MQDISC(&qm, &compcode, &reason);

		/* exit */
		return 97;
	}

	/* calculate the minimum message size to check for latency calculations */
	minSize = iStrLen(parms.qmname) + (int)sizeof(MY_TIME_T) + 1;

	/* check to see if queue is to be drained before run starts */
	if (parms.drainQ > 0)
	{
		Log("draining queue");
		uow = 0;
		compcode = MQCC_OK;
		while (compcode == MQCC_OK)
		{
			if (parms.batchSize > 1)
			{
				mqgmo.Options = MQGMO_NO_WAIT | MQGMO_FAIL_IF_QUIESCING | MQGMO_SYNCPOINT | MQGMO_ACCEPT_TRUNCATED_MSG;
			}
			else
			{
				mqgmo.Options = MQGMO_NO_WAIT | MQGMO_FAIL_IF_QUIESCING | MQGMO_ACCEPT_TRUNCATED_MSG;
			}

			/* reset the msgid and correlid */
			memcpy(msgdesc.MsgId, MQMI_NONE, sizeof(msgdesc.MsgId));
			memcpy(msgdesc.CorrelId, MQCI_NONE, sizeof(msgdesc.CorrelId));
			memcpy(msgdesc.GroupId, MQGI_NONE, sizeof(msgdesc.GroupId));

			/* perform the MQGET */
			MQGET(qm, q, &msgdesc, &mqgmo, parms.maxmsglen, msgdata, &datalen, &compcode, &reason);
			
			if ((MQCC_WARNING == compcode) && (reason == 2079))
			{
				compcode = MQCC_OK;
				reason = 0;
			}

			/* check if we got a message */
			if (MQCC_OK == compcode)
			{
				uow++;
				drainCount++;

				if ((parms.batchSize > 1) && (uow > parms.batchSize))
				{
					MQCMIT(qm, &compcode, &reason);
					uow = 0;
				}
			}
		}

		if ((parms.batchSize > 1) && (uow > parms.batchSize))
		{
			MQCMIT(qm, &compcode, &reason);
			uow = 0;
		}

		if (drainCount > 0)
		{
			Log("%d messages drained from Q prior to measurement start", drainCount);
		}
	}

	/* tell what we are doing */
	Log("Reading " FMTI64 " messages from %s on %s with max wait time of %d secs\n",
		   parms.totcount, parms.qname, parms.qmname, parms.maxtime);

	/* enter get message loop */
	compcode = MQCC_OK;
	uow = 0;
	while ((MQCC_OK == compcode) && (totcount < parms.totcount) && (0 == terminate))
	{
		/* set the get message options */
		if (parms.batchSize > 1)
		{
			mqgmo.Options = MQGMO_WAIT | MQGMO_FAIL_IF_QUIESCING | MQGMO_SYNCPOINT | MQGMO_ACCEPT_TRUNCATED_MSG;
		}
		else
		{
			mqgmo.Options = MQGMO_WAIT | MQGMO_FAIL_IF_QUIESCING | MQGMO_NO_SYNCPOINT | MQGMO_ACCEPT_TRUNCATED_MSG;
		}

		mqgmo.WaitInterval = parms.maxtime * 1000;
		mqgmo.MatchOptions = MQGMO_NONE;

		/* reset the msgid and correlid */
		memcpy(msgdesc.MsgId, MQMI_NONE, sizeof(msgdesc.MsgId));
		memcpy(msgdesc.CorrelId, MQCI_NONE, sizeof(msgdesc.CorrelId));
		memcpy(msgdesc.GroupId, MQGI_NONE, sizeof(msgdesc.GroupId));

		remainingTime = parms.maxtime;
		do
		{
			/* only wait for 1 second */
			mqgmo.WaitInterval = 1000;
			remainingTime--;

			/* since we have a signal handler installed, we do not want to be in an MQGET for a long time */
			/* perform the MQGET */
			MQGET(qm, q, &msgdesc, &mqgmo, parms.maxmsglen, msgdata, &datalen, &compcode, &reason);

			/* check for time out with unit of work open */
			if ((MQCC_FAILED == compcode) && (2033 == reason) && (parms.batchSize > 1) && (uow > 0))
			{
				/* not busy - avoid long-running unit of work */
				MQCMIT(qm, &cc2, &rc2);
				checkerror("MQCMIT2", cc2, rc2, parms.qmname);
				if (MQCC_OK == cc2)
				{
					uow = 0;
				}
			}
		} while ((remainingTime > 0) && (MQCC_FAILED == compcode) && (2033 == reason) && (0 == terminate));

		/* check for truncated message */
		if ((MQCC_WARNING == compcode) && (reason == 2079))
		{
			/* accept truncated messages */
			compcode = MQCC_OK;
			reason = 0;
		}

		/* check if we got to the end of the queue */
		/* check for errors, except for no more messages in queue */
		if (2033 == reason)
		{
			/* check that the program was not cancelled */
			if (0 == terminate)
			{
				Log("Program timed out before maximum number of messages were read");
			}
		}
		else
		{
			checkerror("MQGET", compcode, reason, parms.qname);
		}

		if (compcode == MQCC_OK)
		{
			/* increase the uow count */
			uow++;

			/* check if an acknowledgement is required */
			report = msgdesc.Report;
			if ((((report & MQRO_PAN) > 0) && (parms.fileDataPAN != NULL)) ||
				(((report & MQRO_NAN) > 0) && (parms.fileDataNAN != NULL)))
			{
				/* either NAN or PAN is set - therefore, we need to reply */
				/* get the reply to Q and QM and send the reply           */
				memcpy(parms.replyQname, msgdesc.ReplyToQ, MQ_Q_NAME_LENGTH);
				memcpy(parms.replyQMname, msgdesc.ReplyToQMgr, MQ_Q_MGR_NAME_LENGTH);
				issueReply(qm, report, &uow, (char *)msgdesc.MsgId, &parms);
			}

			/* check if we are at the maximum batch size */
			if ((parms.batchSize > 1) && (uow > parms.batchSize))
			{
				MQCMIT(qm, &compcode, &reason);
				checkerror("MQCMIT", compcode, reason, parms.qmname);
				uow = 0;
			}

			/* get the message time from the MQMD */
			memcpy(currtime, msgdesc.PutTime, 8);

			/* check if the time is the same or not */
			/* only check down to the seconds position */
			/* if ((prevtime[0] == 0) || (memcmp(prevtime, currtime, 6) == 0))*/
			currtime_secs = ( atol(currtime) / 100 );

			if (0 == prevtime_secs)
			{
				/* capture the time of the first message */
				firstTime = currtime_secs;
			}
			else
			{
				if (0 == secondTime)
				{
					/* capture the second time */
					secondTime = currtime_secs;
				}

				prevLastTime = lastTime;
				lastTime = currtime_secs;
			}

			if ((0 == prevtime_secs) || (currtime_secs <= prevtime_secs ))
			{
				/* same second as last message */
				/* just increase the counters  */
				msgcount++;
			}
			else
			{
				/* time has changed, so report the counts for the previous interval */

				/* is this the first interval? */
				if (0 == firstInterval)
				{
					/* get the totals of the last 10 intervals */
					recent10 = 0;
					recent10sec = 0;
					for (i=9; i>0; i--)
					{
						/* shift all the counts and times one position */
						last10secs[i] = last10secs[i - 1];
						last10[i] = last10[i - 1];

						/* get the total number of messages and seconds */
						recent10 += last10[i - 1];
						recent10sec += last10secs[i-1];
					}

					/* record the number of seconds in this interval */
					if (0 == prevtime_secs)
					{
						/* force the interval to 1 second */
						last10secs[0] = 1;
					}
					else
					{
						/* calculate the interval in seconds */
						last10secs[0] = getSecs(currtime_secs) - getSecs(prevtime_secs);
					}

					/* get the most recent count */
					last10[0] = msgcount;
					recent10 += msgcount;
					recent10sec += last10secs[0];

					avgrate = (double)recent10 / recent10sec;
				}
				else
				{
					/* start reporting the recent average the next time */
					firstInterval = 0;
					avgrate = 0.0;
				}

				/* get the count as a string */
				sprintf(tempCount, FMTI64, msgcount);
				sprintf(tempTotal, FMTI64, totcount);

				/* write out the number of messages in this second */
				if (latencyCount > lastLatencyCount)
				{
					/* only report if it changes */
					lastLatencyCount = latencyCount;

					/* calculate the average latency */
					tempLatency = totalLatency / latencyCount;

					/* get the latencies into printable format */
					formatTimeDiff(lastLatency, currLatency);
					formatTimeDiff(avgLatency, tempLatency);
					formatTimeDiff(minLat, minLatency);
					formatTimeDiff(maxLat, maxLatency);

					/* save the last ten latencies */
					/* display the results */
					sprintf(msgPtr,"%s %7.7s msgs - rec avg = %7.2f total msgs %9.9s Latency last %s avg %s min %s max %s latencyCount " FMTI64 " msgCount " FMTI64, 
							prevtime, tempCount, avgrate, tempTotal, lastLatency, avgLatency, minLat, maxLat, latencyCount, totcount);
				}
				else
				{
					/* check if the average rate is > 0 */
					if (avgrate > 0.0)
					{
						/* get the average rate */
						sprintf(tempAvg, " - recent average %9.2f", avgrate);
					}
					else
					{
						/* just create a zero length string */
						tempAvg[0] = 0;
					}
						
					/* create a message to display */
					sprintf(msgPtr,"%s %7.7s msgs%s total msgs %9.9s", prevtime, tempCount, tempAvg, tempTotal);
				}

				msgPtr += strlen(msgPtr);

				reportCount++;
				if (reportCount >= parms.reportInterval)
				{
					Log("%s", msgArea);
					reportCount = 0;
					msgPtr = msgArea;
					msgArea[0] = 0;
				}

				/* is this the first time through? */
				if (0 == firstsec)
				{
					/* remember count in first second */
					firstsec = msgcount;
				}

				/* keep track of the maximum message rate */
				if (msgcount > maxrate)
				{
					maxrate = msgcount;
				}

				/* reset the messages in second counter, automatically counting the first message */
				msgcount = 1;

				/* count the number of individual seconds with at least one message */
				secondcount++;
			}

			if (currtime_secs > prevtime_secs )
			{
				memcpy(prevtime, currtime, 8);
				prevtime_secs = currtime_secs;
			}

			/* count the total number of messages */
			totcount++;

			/* calculate the total bytes in the message */
			totalbytes += datalen;

			/* check if latencies are to be calculated */
			/* this assumes that the first 8 bytes of the message plus offset countains a performance counter */
			/* a queue manager name is placed after the counter, which must also match */
			/* or that the timestamp is hidden in the MQMD Accounting Token, Correlation ID or Group ID fields */
			/* this requires the same setTimeStamp options have been used with MQPUT2 */
			if (1 == parms.setTimeStamp)
			{
#ifdef WIN32
				/* zero out the time the message was sent to detect if a valid start time was not found */
				startTime = 0;
#else
				startTime.tv_usec = 0;
				startTime.tv_sec = 0;
#endif

				/* get the current time (time message has arrived) */
				GetTime(&endTime);

				/* check if we have an RFH header */
				userPtr = checkForRFH(msgdata, &msgdesc);

				/* check if the timestamp is in the MQMD accounting token field */
				if (1 == parms.timeStampInAccountingToken)
				{
					/* get the start time from the MQMD Accounting Token field */
					memcpy(&startTime, msgdesc.AccountingToken, sizeof(MY_TIME_T));
				}
				else if (1 == parms.timeStampInCorrelId)
				{
					/* get the start time from the MQMD Correlation ID field */
					memcpy(&startTime, msgdesc.CorrelId, sizeof(MY_TIME_T));
				}
				else if (1 == parms.timeStampInGroupId)
				{
					/* get the start time from the MQMD Group ID field */
					memcpy(&startTime, msgdesc.GroupId, sizeof(MY_TIME_T));
				}
				else if (1 == parms.timeStampUserProp)
				{
					/* get the start time from the RFH2 usr folder */
					getRFHUsrTimeStamp(msgdata, datalen, &startTime);
				}
				else
				{
					/* check if the message is long enough */
					if (datalen > minSize + parms.timeStampOffset)
					{
						/* check if the queue manager name matches */
						if (strcmp(userPtr + parms.timeStampOffset + sizeof(MY_TIME_T), parms.qmname) == 0)
						{
							/* get the start time */
							memcpy(&startTime, userPtr + parms.timeStampOffset, sizeof(MY_TIME_T));
						}
					}
					else
					{
						/* not long enough - skip this message */
#ifdef WIN32
						startTime = 0;
#else
						startTime.tv_sec = 0;
						startTime.tv_usec = 0;
#endif
					}
				}

				/* make sure both counters are not zero */
#ifdef WIN32
				if ((endTime != 0) && (startTime != 0))
#else
				if (((endTime.tv_sec != 0) || (endTime.tv_usec != 0)) && ((startTime.tv_sec != 0) || (startTime.tv_usec != 0)))
#endif
				{
					int64_t diff = DiffTime(startTime, endTime);
					if (diff <= 0)
					{
						Log("Invalid latency detected - less than zero - diff %e", diff);
					}
					else
					{
						/* add to the total latency */
						currLatency = diff;
						totalLatency += diff;
						latencyCount++;

						/* check if this is less than the minimum latency */
						if ((diff < minLatency) || (1 == latencyCount))
						{
							minLatency = diff;
						}

						/* check if this is more than the maximum latency */
						if (diff > maxLatency)
						{
							maxLatency = diff;
						}
					
						/* keep track of the number of latencies by ranges */
						/* of microseconds, from less than 10 to over 10 seconds */
						if (diff < 10)
						{
							/* increment counter */
							latency10++;
						} 
						else if (diff < 100)
						{
							/* increment counter */
							latency100++;
						} 
						else if (diff < 500)
						{
							/* increment counter */
							latency500++;
						} 
						else if (diff < 1000)
						{
							/* increment counter */
							latency1000++;
						} 
						else if (diff < 5000)
						{
							/* increment counter */
							latency5000++;
						} 
						else if (diff < 10000)
						{
							/* increment counter */
							latency10000++;
						} 
						else if (diff < 50000)
						{
							/* increment counter */
							latency50000++;
						} 
						else if (diff < 100000)
						{
							/* increment counter */
							latency100000++;
						} 
						else if (diff < 500000)
						{
							/* increment counter */
							latency500000++;
						} 
						else if (diff < 1000000)
						{
							/* increment counter */
							latency1000000++;
						} 
						else if (diff < 10000000)
						{
							/* increment counter */
							latency10000000++;
						}
						else
						{
							/* increment counter */
							latency100000000++;
						}
					}
				}
			}
		}
	}

	/* make sure there was a message in the last interval */
	if (msgcount > 0)
	{
		/* count the last interval */
		secondcount++;
	}

	/* check if we have a uow open */
	if ((parms.batchSize > 1) && (uow > 0))
	{
		MQCMIT(qm, &compcode, &reason);
		checkerror("MQCMIT", compcode, reason, parms.qmname);
	}

	/* dump out the last time interval */
	sprintf(msgArea, "%6.6s " FMTI64 " msgs", prevtime, msgcount);
	Log("%s", msgArea);

	/* keep track of the maximum message rate */
	if (msgcount > maxrate)
	{
		maxrate = msgcount;
	}

	/* close the input queue */
	Log("\nclosing the input queue (%s)", parms.qname);
	MQCLOSE(qm, &q, MQCO_NONE, &compcode, &reason);

	checkerror("MQCLOSE", compcode, reason, parms.qname);

	/* Disconnect from the queue manager */
	Log("disconnecting from the queue manager");
	MQDISC(&qm, &compcode, &reason);

	checkerror("MQDISC", compcode, reason, parms.qmname);

	/* issue message if user cancelled the program */
	if (1 == terminate)
	{
		if (1 == cancelled)
		{
			Log("Program cancelled by user");
		}
		else
		{
			/* error forced termination */
			Log("Program terminated due to error");
		}
	}

	/* dump out the total message count */
	Log("\nTotal messages " FMTI64, totcount);

	/* give the total and average message size */
	if (totcount > 0)
	{
		/* calculate the average bytes per message */
		avgbytes = totalbytes / totcount;
		Log("total bytes in all messages " FMTI64, totalbytes);
		Log("average message size " FMTI64, avgbytes);
	}

	/* indicate the number of seconds with at least one message */
	Log("Total number of seconds with at least one message " FMTI64, secondcount);

	/* write out the average message rate, ignoring the first and last intervals */
	if (secondcount > 2)
	{
		formatTimeSecs(timeLast, lastTime);
		formatTimeSecs(timeFirst, firstTime);
		Log("First time %s Last time %s seconds %d", timeFirst, timeLast, (int)(difftime(lastTime, firstTime)) + 1);
		secs = (int)(difftime(prevLastTime, secondTime)) - 1;

		/* avoid any divide by zeros */
		if (secs > 0)
		{
			avgrate = (float)(totcount - firstsec - msgcount) / secs;
			Log("Average message rate except first and last intervals %7.2f", avgrate);
		}
	}

	/* print out the maximum rate */
	Log("Peak message rate " FMTI64, maxrate);

	/* check if latency numbers were requested */
	if (1 == parms.setTimeStamp)
	{
		if (0 == latencyCount)
		{
			Log("\nLatency was requested but the counter is 0");
		}
		else
		{
			/* calculate the average latency */
			totalLatency = totalLatency / latencyCount;

			/* get the latencies into printable format */
			formatTimeDiff(avgLatency, totalLatency);
			formatTimeDiff(minLat, minLatency);
			formatTimeDiff(maxLat, maxLatency);

			/* display the results */
			Log("\nAverage Latency %s - min %s max %s number  of msgs " FMTI64, avgLatency, minLat, maxLat, latencyCount);

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
		}
	}

	if (parms.fileDataPAN != NULL)
	{
		free(parms.fileDataPAN);
	}

	if (parms.fileDataNAN != NULL)
	{
		free(parms.fileDataNAN);
	}

	Log("\nMQTIMES2 program ended");

	/******************************************************************/
	/*                                                                */
	/* END OF PROGRAM                                                 */
	/*                                                                */
	/******************************************************************/

	return 0;
}
