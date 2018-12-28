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
/*   MQPUT2 has 1 required input parameter                          */
/*                                                                  */
/*      -f name of the parameter file                               */
/*                                                                  */
/*   Additional parameters can be used to override certain of the   */
/*   parameters in the parameter file, including the message        */
/*   count, queue manager name, queue name and batch size.  The     */
/*   think time parameter can also be overriden for the MQPUTS      */
/*   program.                                                       */
/*                                                                  */
/*   The input parameter file contains all values, including the    */
/*   name of the queue and queue manager to write data to, the      */
/*   total number of messages to write, any MQMD parameters and     */
/*   a list of files which contain the message data.                */
/*                                                                  */
/*   This program can be compiled into two different programs,      */
/*   depending on the setting of the NOTUNE compile option.         */
/*   The normal MQPUT2 program works by periodically observing      */
/*   the depth of a queue and trying to maintain the depth          */
/*   between a low and high water mark.  A sleeptime parameter      */
/*   determines how often the program checks the queue depth.       */
/*                                                                  */
/*   The other version of the program (NOTUNE option selected)      */
/*   does not check the depth of the queue.  It takes a             */
/*   thinktime parameter.  The program will write batchsize         */
/*   messages and then sleep for the thinktime parameter,           */
/*   which is specified in milliseconds.                            */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.01                                                 */
/*                                                                  */
/* 1) fixed bug with groupidx parameter overlaying qname.           */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.1                                                  */
/*                                                                  */
/* 1) Added support for psc and jms folders in RFH2.                */
/* 2) Added support for Sun Solaris.                                */
/* 3) Use keyword rather than positional arguments.                 */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.11                                                 */
/*                                                                  */
/* 1) fixed bug with argument overrides being ignored.              */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.20                                                 */
/*                                                                  */
/* 1) Added support for message groups.                             */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.21                                                 */
/*                                                                  */
/* 1) Added support for thinktime.                                  */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.22                                                 */
/*                                                                  */
/* 1) Formatted start and stop times.                               */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.23                                                 */
/*                                                                  */
/* 1) Added checks for malloc failures and total memory used.       */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.3                                                  */
/*                                                                  */
/* 1) Added support to use MQMDs saved with the data.               */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.31                                                 */
/*                                                                  */
/* 1) Added support for Linux.						                */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.4                                                  */
/*                                                                  */
/* 1) Added support for Usr folder in RFH2. 		                */
/* 2) Added support for folder contents in XML.                     */
/* 3) Split RFH processing into separate module.                    */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.5                                                  */
/*                                                                  */
/* 1) Added support for handling break key. 		                */
/* 2) Made MQPUTS use think time and MQPUT2 ignore think time.      */
/* 3) Added limited support for latency measurements.               */
/* 4) Added support for global override of think time.              */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.6                                                  */
/*                                                                  */
/* 1) Fixed bug where use of rfh=1 or 2 was causing truncation.     */
/* 2) Straightened out routines that read files and handle          */
/*    delimiters.                                   .               */
/* 3) Added routine to release acquired storage before ending.      */
/* 4) Added ignoreMQMD option.                                      */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.6.2                                                */
/*                                                                  */
/* 1) Added verbose argument to suppress file messages.             */
/* 2) Fixed problem with MQINQ for cluster queue (rc=2068).         */
/* 3) Fixed problem with start and stop times when using embedded   */
/*    MQMDs in message files.  The elapsed time of the run is now   */
/*    reported.                                                     */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.6.3                                                */
/*                                                                  */
/* 1) Corrected bug where FEEDBACK parameter not recognized.        */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V2.0                                                  */
/*                                                                  */
/* 1) Major internal restructing to eliminate global variables.     */
/* 2) Support for all fields in MQMD.                               */
/* 3) Changed some counters to 64-bit integers to avoid overflows;  */
/* 4) Support for 64-bit executables on some platforms.             */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V2.1                                                  */
/*                                                                  */
/* 1) Added additional options for timestamp placement for latency  */
/*    measurements.                                                 */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "signal.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef SOLARIS
#include <sys/types.h>
#endif

#ifndef WIN32
void Sleep(int amount)
{
	usleep(amount*1000);
}
#endif

/* includes for MQI */
#include <cmqc.h>

/* definitions of 64-bit values for platform independence */
#include "int64defs.h"

/* common subroutines include */
#include "comsubs.h"
#include "timesubs.h"

/* parameter file processing routines */
#include "parmline.h"
#include "putparms.h"

/* MQ subroutines include */
#include "qsubs.h"
#include "rfhsubs.h"

static char copyright[] = "(C) Copyright IBM Corp, 2001 - 2014";
static char Version[]=\
"@(#)MQPut2 V3.0 - Performance driver test tool  - Jim MacNair ";

#ifdef _DEBUG
static char Level[]="mqput2.c V3.0 Debug version ("__DATE__" "__TIME__")";
#else
#ifdef NOTUNE
#ifdef MQCLIENT
static char Level[]="mqputsc.c V3.0 Client version ("__DATE__" "__TIME__")";
#else
static char Level[]="mqputs.c V3.0 Release version ("__DATE__" "__TIME__")";
#endif
#else
#ifdef MQCLIENT
static char Level[]="mqput2c.c V3.0 Client version ("__DATE__" "__TIME__")";
#else
static char Level[]="mqput2.c V3.0 Release version ("__DATE__" "__TIME__")";
#endif
#endif
#endif

	MQHCONN			qm=0;			/* queue manager connection handle */
	MQHOBJ			q=0;			/* queue handle used for mqput     */
#ifndef NOTUNE
	MQHOBJ			Hinq=0;			/* inquire object handle           */
#endif

	/* global termination switch */
	volatile int	terminate=0;
	volatile int	cancelled=0;

/**************************************************************/
/*                                                            */
/* This routine puts a message on the queue.                  */
/*                                                            */
/**************************************************************/

int putMessage(FILEPTR* fptr, 
			   MQCHAR8 *puttime, 
			   int *groupOpen,
			   PUTPARMS *parms)

{
	MQLONG	compcode=0;
	MQLONG	reason=0;
	MQMD2	msgdesc = {MQMD2_DEFAULT};
	MQPMO	mqpmo = {MQPMO_DEFAULT};
	MY_TIME_T	perfCounter;		/* high performance counter to measure latency */

	/* set the get message options */
	if (parms->batchSize > 1)
	{
		/* use syncpoints */
		mqpmo.Options = MQPMO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING;
	}
	else
	{
		/* no synchpoint, each message as a separate UOW */
		mqpmo.Options = MQPMO_NO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING;
	}

	/* check if we are using an mqmd from the file */
	if (fptr->mqmdptr != NULL)
	{
		mqpmo.Options |= MQPMO_SET_ALL_CONTEXT;

		if (1 == fptr->newMsgId)
		{
			mqpmo.Options |= MQPMO_NEW_MSG_ID;
		}

		/* set the MQMD */
		memcpy(&msgdesc, fptr->mqmdptr, sizeof(msgdesc));
	}
	else
	{
		/* use new message id */
		mqpmo.Options |= MQPMO_NEW_MSG_ID;

		if ((1 == fptr->inGroup) || (1 == fptr->lastGroup) || ((1 == fptr->timeStampInGroupId) && (1 == fptr->setTimeStamp)))
		{
			mqpmo.Options |= MQPMO_LOGICAL_ORDER;

			if (1 == fptr->inGroup)
			{
				msgdesc.MsgFlags |= MQMF_MSG_IN_GROUP;
			}

			if ((1 == fptr->lastGroup) || ((1 == fptr->timeStampInGroupId) && (1 == fptr->setTimeStamp)))
			{
				msgdesc.MsgFlags |= MQMF_LAST_MSG_IN_GROUP;
			}
		}

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

		/* check if feedback was specified */
		if (fptr->Feedback != 0)
		{
			/* set the feedback field */
			msgdesc.Feedback = fptr->Feedback;
		}

		/* set the message format in the MQMD that was specified */
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
		if (fptr->ReplyQM[0] != 0)
		{
			memset(msgdesc.ReplyToQMgr, 0, sizeof(msgdesc.ReplyToQMgr));
			memcpy(msgdesc.ReplyToQMgr, fptr->ReplyQM, strlen(fptr->ReplyQM));
		}

		/* check if a reply to queue was specified */
		if (fptr->ReplyQ[0] != 0)
		{
			memset(msgdesc.ReplyToQ, 0, sizeof(msgdesc.ReplyToQ));
			memcpy(msgdesc.ReplyToQ, fptr->ReplyQ, strlen(fptr->ReplyQ));
		}

		/* check if a correl id was specified */
		if (1 == fptr->CorrelIdSet)
		{
			memcpy(msgdesc.CorrelId, fptr->CorrelId, MQ_CORREL_ID_LENGTH);
		}
		else
		{
			memset(msgdesc.CorrelId, 0, MQ_CORREL_ID_LENGTH);
		}

		/* check if a group id was specified */
		if (1 == (*groupOpen))
		{
			memcpy(msgdesc.GroupId, parms->saveGroupId, MQ_GROUP_ID_LENGTH);
		}
		else
		{
			if (1 == fptr->GroupIdSet)
			{
				memcpy(msgdesc.GroupId, fptr->GroupId, MQ_GROUP_ID_LENGTH);
				msgdesc.MsgFlags |= MQMF_LAST_MSG_IN_GROUP | MQMF_MSG_IN_GROUP ;
			}
			else
			{
				memset(msgdesc.GroupId, 0, sizeof(msgdesc.GroupId));
			}
		}

		/* check if an accounting token was specified */
		if (1 == fptr->AcctTokenSet)
		{
			/* set the accounting token value */
			memcpy(msgdesc.AccountingToken, fptr->AccountingToken, MQ_ACCOUNTING_TOKEN_LENGTH);
		}
	}

	/* check if a timestamp for latency measurements is to be inserted */
	if (1 == fptr->setTimeStamp)
	{
		/* get a high resolution time stamp */
		GetTime(&perfCounter);

		/* check if the timer is to be stored in the MQMD accounting token */
		if (fptr->timeStampInAccountingToken)
		{
			/* hide the performance counter in the MQMD accounting token field */
			memcpy(&(msgdesc.AccountingToken), &perfCounter, sizeof(MY_TIME_T));

			/* check if existing MQMDs are being used */
			if (NULL == fptr->mqmdptr)
			{
				/* must use entire MQMD */
				mqpmo.Options |= MQPMO_SET_ALL_CONTEXT;

				/* set the context fields */
				setContext(&msgdesc);
			}
		}
		/* check if the timer is to be stored in the MQMD correlation id */
		else if (fptr->timeStampInCorrelId)
		{
			/* hide the performance counter in the MQMD correlation id field */
			memcpy(&(msgdesc.CorrelId), &perfCounter, sizeof(MY_TIME_T));
		}
		/* check if the timer is to be stored in the MQMD group id */
		else if (fptr->timeStampInGroupId)
		{
			/* hide the performance counter in the MQMD group id field */
			memcpy(&(msgdesc.GroupId), &perfCounter, sizeof(MY_TIME_T));

			/* force on the last in group indicator */
			msgdesc.MsgFlags |= MQMF_LAST_MSG_IN_GROUP;

			/* force the MQMD to be type 2 */
			msgdesc.Version = MQMD_VERSION_2;
		}
		else if (fptr->timeStampUserProp)
		{
			/* hide the performance counter an RFH2 usr folder */
			/* make sure the message starts with an rfh2 header */
			if ((RFH_V2 == fptr->hasRFH) && 
				(memcmp(fptr->dataptr, MQRFH_STRUC_ID, 4) == 0) && 
				(memcmp(fptr->dataptr + MQRFH_STRUC_LENGTH_FIXED_2 + 4, LATENCYHEADER, 9) == 0))
			{
				/* convert the timestamp to hex and insert into usr folder */
				AsciiToHex((unsigned char *)fptr->dataptr + MQRFH_STRUC_LENGTH_FIXED_2 + 13, (unsigned char *)&perfCounter, 8);
			}
		}
		else if ((fptr->length - fptr->rfhlen) > (strlen(parms->qmname) + sizeof(MY_TIME_T) + 1))
		{
			/* insert the performance counter into the first 8 bytes of the message data */
			/* note that this will clobber the first 8 bytes of the message data */
			/* this should only be done if the mqtimes2 program is processing the messages */
			/* and the latency option is selected for mqtimes2 */
			memcpy((char *)fptr->userDataPtr + fptr->timeStampOffset, &perfCounter, sizeof(MY_TIME_T));
			strcpy((char *)fptr->userDataPtr + fptr->timeStampOffset + sizeof(MY_TIME_T), parms->qmname);
		}
		else
		{
			/* warn that data length is insufficient to hold timestamp */
			Log("***** Message length insufficient to hold timestamp information");
		}
	}

	/* write the message to the queue */
	MQPUT(qm, q, &msgdesc, &mqpmo, fptr->length, fptr->dataptr, &compcode, &reason);

	/* check for errors */
	checkerror("MQPUT", compcode, reason, parms->qname);

	if (0 == compcode)
	{
		/* keep track of the number of bytes we have written */
		parms->byteswritten += fptr->length;
	}

	/* check if this message is part of a group */
	if (1 == fptr->inGroup)
	{
		(*groupOpen) = 1;
	}

	/* check if this is the last message in a group */
	if (1 == fptr->lastGroup)
	{
		(*groupOpen) = 0;
	}

	if ((*groupOpen) == 1)
	{
		memset(parms->saveGroupId, 0, sizeof(parms->saveGroupId));
		memcpy(parms->saveGroupId, msgdesc.GroupId, MQ_GROUP_ID_LENGTH);
	}

	memcpy(puttime, msgdesc.PutTime, sizeof(msgdesc.PutTime));

	return compcode;
}

/**************************************************************/
/*                                                            */
/* This routine gets the number of messages on the queue.     */
/*                                                            */
/**************************************************************/

#ifndef NOTUNE
MQLONG openQueueInq(MQLONG openOpt, MQOD objdesc, const char * qmname, const char * qname, PUTPARMS *parms)

{
	MQLONG	compcode;
	MQLONG	reason;
	MQLONG	Select[1];			/* attribute selectors           */
	MQLONG	IAV[1];				/* integer attribute values      */
	MQLONG	compcode2;
	MQLONG	reason2;

	MQOPEN(qm,				/* connection handle              */
		   &objdesc,		/* object descriptor for queue    */
		   openOpt,			/* open options                   */
		   &Hinq,			/* object handle for MQINQ        */
		   &compcode,		/* MQOPEN completion code         */
		   &reason);		/* reason code                    */

	checkerror("MQOPEN(Inq)", compcode, reason, qmname);

	if (MQCC_OK == compcode)
	{
		/* try and get the queue depth */
		/*  this will fail if the queue is really a cluster queue */
		Select[0] = MQIA_CURRENT_Q_DEPTH;
		MQINQ(qm,			/* connection handle                 */
			  Hinq,			/* object handle                     */
			  1L,			/* Selector count                    */
			  Select,		/* Selector array                    */
			  1L,			/* integer attribute count           */
			  IAV,			/* integer attribute array           */
			  0L,			/* character attribute count         */
			  NULL,			/* character attribute array         */
			  /*  note - can use NULL because count is zero      */
			  &compcode2,	/* completion code                   */
			  &reason2);	/* reason code                       */

		/* check for a 2068 return code, indicating that the     */
		/* queue must be open for something besides queue depth. */
		if ((compcode2 != MQCC_OK) && (0 == parms->reopenInq) && (2068 == reason2))
		{
			/* only try this once */
			parms->reopenInq = 1;

			/* close the queue so we can reopen with the browse option added */
			MQCLOSE(qm, &Hinq, MQCO_NONE, &compcode2, &reason2);
			Hinq = 0;

			/* now try to reopen the queue with a browse option as well */
			openOpt |= MQOO_BROWSE;    /* open to inquire attributes     */
			
			/* try the open again */
			compcode = openQueueInq(openOpt, objdesc, qmname, qname, parms);
		}
	}

	/* check for errors */
	checkerror("MQOPEN(Inq)", compcode, reason, qname);

	return compcode;
}

MQLONG getQueueDepth(const char * qmname)

{
	MQLONG	numOnQueue=0;		/* Number of messages on Queue   */
	MQLONG	Select[1];			/* attribute selectors           */
	MQLONG	IAV[1];				/* integer attribute values      */
	MQLONG	compcode;
	MQLONG	reason;

	/* get the current queue depth */
	Select[0] = MQIA_CURRENT_Q_DEPTH;
	MQINQ(qm,			/* connection handle                 */
		  Hinq,			/* object handle                     */
		  1L,			/* Selector count                    */
		  Select,		/* Selector array                    */
		  1L,			/* integer attribute count           */
		  IAV,			/* integer attribute array           */
		  0L,			/* character attribute count         */
		  NULL,			/* character attribute array         */
		  /*  note - can use NULL because count is zero      */
		  &compcode,	/* completion code                   */
		  &reason);		/* reason code                       */

	checkerror("MQINQ", compcode, reason, qmname);
	if (MQCC_OK == compcode)
	{
		numOnQueue= IAV[0];   /* currdepth */
	}

	return numOnQueue;
}
#endif

/**************************************************************/
/*                                                            */
/* Check if the sleep time is too much or not enough.         */
/*                                                            */
/**************************************************************/

#ifndef NOTUNE
void adjustSleeptime(const int numOnQueue, const int lastdepth, PUTPARMS * parms)

{
	int	origSleeptime = parms->sleeptime;
	int minAdjCount;

	/* first, check if the queue is drained */
	if (numOnQueue == 0)
	{
		/* queue is empty, we need to cut the sleeptime */
		parms->sleeptime >>= 1;
	}
	else
	{
		/* check if the counts are the same */
		if (numOnQueue == lastdepth)
		{
			/* no messages were processed */
			/* this indicates that we are checking too often */
			/* increase the sleep time by 50% */
			parms->sleeptime += (parms->sleeptime >> 1) + 1;
		}
		else
		{
			/* we don't want to adjust if the rate is high enough */
			/* first, calculate the difference between the desired */
			/* and maximum values */
			minAdjCount = (parms->qmax - parms->qdepth) >> 2;
			if ((lastdepth > numOnQueue) && ((lastdepth - numOnQueue) < minAdjCount))
			{
				/* not enough messages were processed */
				/* increase the sleeptime by a 12% */
				parms->sleeptime += (parms->sleeptime >> 3) + 1;
			}
		}
	}

	/* make sure we are within the allowed limits for sleeptime */
	if (parms->sleeptime < MIN_SLEEP)
	{
		Log(" sleeptime below minimum (%d) - forced to minimum", parms->sleeptime);
		parms->sleeptime = MIN_SLEEP;
	}

	if (parms->sleeptime > MAX_SLEEP)
	{
		Log(" sleeptime above maximum (%d) - forced to maximum", parms->sleeptime);
		parms->sleeptime = MAX_SLEEP;
	}

	/* tell what we did */
	if (origSleeptime != parms->sleeptime)
	{
		Log(" sleeptime changed from %d to %d milliseconds numOnQueue %d lastdepth %d written " FMTI64, 
				origSleeptime, parms->sleeptime, numOnQueue, lastdepth, parms->msgwritten);
	}
}
#endif

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
#ifdef NOTUNE
	printf("  %s -f parm_file {-v} {-m QMgr} {-q queue} {-c count} {-b batchsize} {-p} {-t thinktime}\n", pgmName);
#else
	printf("  %s -f parm_file {-v} {-m QMgr} {-q queue} {-c count} {-b batchsize}\n", pgmName);
#endif
	printf("   parm_file is the fully qualified name of the parameters file\n");
	printf("   -v verbose\n");
	printf("   Overrides\n");
	printf("   -m name of queue manager\n");
	printf("   -q name of queue\n");
	printf("   -c message count\n");
	printf("   -b batch size\n");
#ifdef NOTUNE
	printf("   -t think time\n");
#endif
}

int main(int argc, char **argv)

{
	int64_t		MsgsAtLastInterval=0;
	int64_t		lastInterval;
	int64_t		elapsed=0;
	int			uowcount=0;
	int			groupOpen = 0;
	int			notDone;
	MQLONG		compcode;
	MQLONG		reason;
	MQOD		objdesc = {MQOD_DEFAULT};
	MQLONG		openopt = 0;
	MQLONG		maxMsgLen=0;
	int			numWrittenMin=-1;
	int			numWrittenMax=0;
	int			iElapsed=0;
	time_t		reportTime=0;		
	time_t		prevReportTime=0;		
#ifndef NOTUNE
	int			numOnQueueMin=0;
	int			numOnQueueMax=0;
	int			writeCount;
	int			lastdepth;
	int64_t		saveCount;
	MQLONG		numOnQueue;					/* Number of messages on Queue   */
	MQLONG		O_optionsq;					/* inquire MQOPEN options        */
#endif
	MQCHAR8		puttime;
	char		formTime[16];
	char		tempCount[16];
	char		tempTotal[16];
	char		tempRate[16];
	MY_TIME_T	startTime;
	MY_TIME_T	endTime;
	MY_TIME_T	prevTime;
	MY_TIME_T	newTime;
	time_t		startTOD;
	time_t		endTOD;
	FILEPTR		*fptr=NULL;
	FILEPTR		*fileptr;
	PUTPARMS	parms;

	/* print the copyright statement */
	Log(copyright);
	Log(Level);

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

	/* check for a log file name */
	if (parms.logFileName[0] != 0)
	{
		/* open the log file */
		openLog(parms.logFileName);
	}

	/* process the parameters file data */
	/* process any data files in the parameters file */
	fptr = processParmFile(parms.parmFilename, &parms, 1);

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

	if (parms.err != 0)
	{
		Log("***** Error detected (err=%d) - program terminating", parms.err);
		return parms.err;
	}

	/* check if a queue name or topic was specified */
	if ((0 == parms.qname[0]) && (0 == parms.topicStr[0]) && (0 == parms.topicName[0]))
	{
		/* no queue name or topic */
		Log("***** Queue name or topicStr/topicName required - program terminating");
		return 95;
	}

	/* tell how many files and messages we found */
	Log("Total files read %d", parms.fileCount);
	Log("Total messages found %d", parms.mesgCount);

	/* explain what parameters are being used */
	if (parms.qname[0] != 0)
	{
		Log("\n" FMTI64 " messages to be written to queue %s on queue manager %s", parms.totcount, &(parms.qname), &(parms.qmname));
	}
	else
	{
		Log("\n" FMTI64 " messages to be written to topic %s topic name %s on queue manager %s", parms.totcount, &(parms.topicStr), &(parms.topicName), &(parms.qmname));
	}

	if (1 == parms.setTimeStamp)
	{
		if (1 == parms.timeStampUserProp)
		{
			/* indicate timestamp will be carried as user property */
			Log("Timestamp will be carried in User Properties (rfh2 usr folder)");
		}
		else if (1 == parms.timeStampInAccountingToken)
		{
			/* indicate timestamp will be stored in MQMD */
			Log("Timestamp will be carried in accounting token in MQMD");
		}
		else if (1 == parms.timeStampInCorrelId)
		{
			/* indicate timestamp will be stored in MQMD */
			Log("Timestamp will be carried in correlation ID in MQMD");
		}
		else if (1 == parms.timeStampInGroupId)
		{
			/* indicate timestamp will be stored in MQMD */
			Log("Timestamp will be carried in group ID in MQMD");
		}
		else
		{
			/* indicate that we are adding a timestamp to the message */
			Log("Some data in message will be overlaid with time stamp at offset %d", parms.timeStampOffset);
		}
	}

#ifdef NOTUNE
	Log("thinkTime = %d batchsize = %d", parms.thinkTime, parms.batchSize);
#else
	Log("minimum queue depth %d max %d batchsize %d", parms.qdepth, parms.qmax, parms.batchSize);
	Log("initial sleep time %d tune = %d", parms.sleeptime, parms.tune);
#endif

	/* set a termination handler */
	signal(SIGINT, InterruptHandler);

	/* Connect to the queue manager */
#ifdef MQCLIENT
	clientConnect2QM((char *)&(parms.qmname), &qm, &maxMsgLen, &compcode, &reason);
#else
	connect2QM((char *)&(parms.qmname), &qm, &compcode, &reason);
#endif

	/* check for errors */
	if (compcode != MQCC_OK)
	{
		return 98;
	}

	/* set the queue manager name */
	strcpy(objdesc.ObjectQMgrName, parms.remoteQM);

	/* check for a queue name */
	if (parms.qname[0] != 0)
	{
		/* set the qname in the open descriptor */
		strncpy(objdesc.ObjectName, parms.qname, sizeof(objdesc.ObjectName));

		/* set the queue open options */
		openopt = MQOO_OUTPUT + MQOO_FAIL_IF_QUIESCING;
	}
	else
	{
		/* must be a topic */
		/* set up the object descriptor for the topic */
		objdesc.Version = MQOD_VERSION_4;
		objdesc.ObjectType = MQOT_TOPIC;
		objdesc.ObjectString.VSPtr = (void *)&(parms.topicStr);
		objdesc.ObjectString.VSLength = MQVS_NULL_TERMINATED;
		strncpy(objdesc.ObjectName, parms.topicName, sizeof(objdesc.ObjectName));

		/* set the queue open options */
		openopt = MQOO_OUTPUT + MQOO_FAIL_IF_QUIESCING;
	}

	/* check if we need to set all context */
	if ((1 == parms.foundMQMD) || (1 == parms.timeStampInAccountingToken))
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

#ifdef NOTUNE
	/**************************************************************/
	/*                                                            */
	/*   Check if all the messages have been written              */
	/*                                                            */
	/**************************************************************/

	if (parms.msgwritten < parms.totcount)
	{
		notDone = 1;
	}
	else
	{
		notDone = 0;
	}
#else
	/**************************************************************/
	/*                                                            */
	/*   Open named queue for INQUIRE                             */
	/*                                                            */
	/**************************************************************/

	O_optionsq = MQOO_INQUIRE    /* open to inquire attributes     */
				 + MQOO_FAIL_IF_QUIESCING;

	/* open the queue for inquiry */
	compcode = openQueueInq(O_optionsq, objdesc, parms.qmname, parms.qname, &parms);

	if (compcode != MQCC_OK)
	{
		return 96;
	}

	numOnQueue = getQueueDepth(parms.qmname);

	/**************************************************************/
	/*                                                            */
	/*   Check if all the messages have been written              */
	/*                                                            */
	/**************************************************************/

	if (((parms.msgwritten + numOnQueue) < parms.qmax) && (parms.msgwritten < parms.totcount))
	{
		notDone = 1;
	}
	else
	{
		notDone = 0;
	}
#endif

	/* remember the starting time */
	GetTime(&startTime);
	GetTime(&prevTime);

	/* if monitoring queue depth top up the queue to hold qmax messages */
	/* prime the queue with an initial number of messages               */
	/* After this, we will monitor the queue depth and                  */
	/* try to keep the queue depth at a specified amount                */
	/* if not monitoring queue depth this loop writes all the messages  */
	while ((compcode == MQCC_OK) && (1 == notDone) && (0 == terminate))
	{
		/* perform the MQPUT */
		compcode = putMessage(fileptr,
							  &puttime,
							  &groupOpen,
							  &parms);

		/* check for errors */
		if (MQCC_OK == compcode)
		{
			if (0 == parms.msgwritten)
			{
				/* write out the time the first messsage was sent */
				time(&startTOD);
				LogNoCRLF("First message written at %s", ctime(&startTOD));

				/* write out the time of the first message */
				formatTime(formTime, puttime);
				Log("MQ Timestamp of first message written at %8.8s", formTime);
			}

			/* increment the message count */
			parms.msgwritten++;

			/* remember how many messages are in this uow */
			uowcount++;

			/* check if we need to issue a commit */
			/* if a group is in progress */
			/* make sure to commit only after */
			/* the group is finished */
			if ((parms.batchSize > 1) && (uowcount >= parms.batchSize) && (0 == groupOpen))
			{
				MQCMIT(qm, &compcode, &reason);
				checkerror("MQCMIT", compcode, reason, parms.qname);
				uowcount = 0;
			}

#ifdef NOTUNE
			/* get the current time in seconds since 1970 */
			time(&reportTime);

			/* check if this is the first time through */
			if (0 == prevReportTime)
			{
				prevReportTime = reportTime;
			}

			/* check if we are supposed to report progress */
			if (((parms.reportEvery > 0) && ((parms.msgwritten % parms.reportEvery) == 0)) ||
				((parms.reportEverySecond > 0) && (prevReportTime != reportTime)))
			{
				/* remember this report time in case reporting every second */
				prevReportTime = reportTime;
#else
			/* check if we are supposed to report progress */
			if ((parms.reportEvery > 0) && ((parms.msgwritten % parms.reportEvery) == 0))
			{
#endif
				/* report the time to put a given number of messages */
				/* calculate the amount of time it took in microseconds */
				GetTime(&newTime);
				elapsed = DiffTime(prevTime, newTime);

				/* update the time for the next interval */
				prevTime = newTime;

				/* format the difference as a string (seconds and 6 decimal places) */
				formatTimeDiff(formTime, elapsed);

				if (elapsed > 0)
				{
					/* avoid any 32-bit overflows */
					lastInterval = parms.msgwritten - MsgsAtLastInterval;

					/* get the message rate as a 64-bit integer */
					/* this is a division by the number of microseconds */
					lastInterval = ((lastInterval * 1000000) / elapsed);

					sprintf(tempRate, " rate " FMTI64, lastInterval);
				}
				else
				{
					tempRate[0] = 0;
				}

				/* get the messages that were written in the previous interval */
				/* and the total so far                                        */
				sprintf(tempCount, FMTI64, parms.msgwritten - MsgsAtLastInterval);
				sprintf(tempTotal, FMTI64, parms.msgwritten);
				
				/* write out the time it took to write the messages without the rate */
				Log("%7.7s messages written in %s seconds - total so far %9.9s%s", tempCount, formTime, tempTotal, tempRate);

				/* remember the count at the beginning of the next interval */
				MsgsAtLastInterval = parms.msgwritten;
			}

#ifdef NOTUNE
			/* was a think time specified? */
			if ((fileptr->thinkTime > 0) && ((parms.msgwritten % parms.batchSize) == 0) && (0 == groupOpen))
			{
				if ((parms.batchSize > 1) && (uowcount > 1) && (0 == groupOpen))
				{
					/* commit the messages first before issuing the sleep */
					MQCMIT(qm, &compcode, &reason);
					checkerror("MQCMIT", compcode, reason, parms.qname);
					uowcount = 0;
				}

				/* delay the amount specified by the think time parameter */
				Sleep(fileptr->thinkTime);
			}
#endif

			/* move on to the next message */
			fileptr = (FILEPTR *)fileptr->nextfile;
			if (NULL == fileptr)
			{
				/* go back to the first message data file */
				fileptr = fptr;
			}
		}

#ifdef NOTUNE
		if (parms.msgwritten >= parms.totcount)
		{
			/* end as soon as the group is finished */
			notDone = groupOpen;
		}
#else
		if (((parms.msgwritten + numOnQueue) >= parms.qmax) || (parms.msgwritten >= parms.totcount))
		{
			/* end as soon as the group is finished */
			notDone = groupOpen;
		}
#endif
	} /* while */

	if (uowcount > 0)
	{
		MQCMIT(qm, &compcode, &reason);
		checkerror("MQCMIT", compcode, reason, parms.qname);
		uowcount = 0;
	}

#ifndef NOTUNE
	/* reset the report times */
	reportTime = 0;
	prevReportTime = 0;

	/* give the initial number of messages written */
	if (parms.tune == 1)
	{
		Log("initial number of messages written " FMTI64, parms.msgwritten);
	}

	/* enter message loop */
	if (parms.msgwritten < parms.totcount)
	{
		notDone = 1;
	}
	else
	{
		notDone = 0;
	}

	/* start the main loop */
	while ((compcode == MQCC_OK) && (1 == notDone) && (0 == parms.err) && (0 == terminate))
	{
		/* initialize the last depth variable */
		lastdepth = getQueueDepth(parms.qmname);

		/* issue a wait for sleeptime milliseconds */
		Sleep(parms.sleeptime); /*sleep in millisecs*/

		/* get the current queue depth */
		numOnQueue = getQueueDepth(parms.qmname);

		/* remember the minimum and maximum counts */
		if ((numOnQueueMax == 0) || (numOnQueue < numOnQueueMin))
		{
			numOnQueueMin = numOnQueue;
		}

		if (numOnQueue > numOnQueueMax)
		{
			numOnQueueMax = numOnQueue;
		}

		/* check if the queue became empty */
		if (0 == numOnQueue)
		{
			/* issue error message if we find no messages on queue */
			Log("***** warning - no messages on queue after " FMTI64 " msgs written", parms.msgwritten);
			Log("***** decrease sleeptime parameter from %d", parms.sleeptime);
		}

		/* check if we are tuning the sleeptime parameter */
		if (1 == parms.tune)
		{
			/* give the current message count */
			Log("number on queue %d, lastdepth %d", numOnQueue, lastdepth);

			/* check if we want to adjust the sleep time */
			adjustSleeptime(numOnQueue, lastdepth, &parms);
		}

		/* check if we are below the minimum depth */
		if (numOnQueue < parms.qdepth)
		{
			/* check if we need to update our max and min statistics */
			writeCount = parms.qdepth - numOnQueue;
			if ((-1 == numWrittenMin) || (writeCount < numWrittenMin))
			{
				numWrittenMin = writeCount;
			}

			if (writeCount > numWrittenMax)
			{
				numWrittenMax = writeCount;
			}

			/* remember the number of messages written previously */
			saveCount = parms.msgwritten;

			/* check the depth of the queue */
			while ((MQCC_OK == compcode) && (numOnQueue < parms.qmax) && (parms.msgwritten < parms.totcount) && (0 == terminate))
			{
				/* perform the MQPUT */
				compcode = putMessage(fileptr, 
									  &puttime, 
									  &groupOpen,
									  &parms);

				/* check for errors */
				if (MQCC_OK == compcode)
				{
					if (0 == parms.msgwritten)
					{
						/* write out the time the first messsage was sent */
						time(&startTOD);
						LogNoCRLF("First message written at %s", ctime(&startTOD));

						/* write out the time of the first message */
						formatTime(formTime, puttime);
						Log("MQ Timestamp of first message written at %8.8s", formTime);
					}

					/* get the current time in seconds since 1970 */
					time(&reportTime);

					/* check if this is the first time through */
					if (0 == prevReportTime)
					{
						prevReportTime = reportTime;
					}

					/* check if we are supposed to report progress */
					if (((parms.reportEvery > 0) && ((parms.msgwritten % parms.reportEvery) == 0)) ||
						((parms.reportEverySecond > 0) && (prevReportTime != reportTime)))
					{
						/* remember this report time in case reporting every second */
						prevReportTime = reportTime;

						/* report the time to put a given number of messages */
						/* calculate the amount of time it took */
						GetTime(&newTime);
						elapsed = DiffTime(prevTime, newTime);

						/* update the time for the next interval */
						prevTime = newTime;

						/* format the difference as a string (seconds and 6 decimal places) */
						formatTimeDiff(formTime, elapsed);

						if (elapsed > 0)
						{
							/* get the number of messages and allow for the division by microseconds */
							lastInterval = parms.msgwritten - MsgsAtLastInterval;
							lastInterval *= 1000000;

							/* get the message rate as a 64-bit integer */
							lastInterval = (lastInterval / elapsed);

							/* get the rate as a string */
							sprintf(tempRate, " rate " FMTI64, lastInterval);
						}
						else
						{
							/* no rate */
							tempRate[0] = 0;
						}

						/* get the messages that were written in the previous interval */
						/* and the total so far                                        */
						sprintf(tempCount, FMTI64, parms.msgwritten - MsgsAtLastInterval);
						sprintf(tempTotal, FMTI64, parms.msgwritten);

						/* write out the time it took to write the messages */
						Log("%7.7s messages written in %s seconds - total so far %9.9s%s", tempCount, formTime, tempTotal, tempRate);
				
						/* remember the count at the beginning of the next interval */
						MsgsAtLastInterval = parms.msgwritten;
					}

					/* move on to the next message file */
					fileptr = (FILEPTR *)fileptr->nextfile;
					if (NULL == fileptr)
					{
						/* go back to the first message data file */
						fileptr = fptr;
					}

					/* increment the message count and uow counter */
					parms.msgwritten++;
					numOnQueue++;
					uowcount++;

					/* check if we need to issue a commit */
					if ((parms.batchSize > 1) && (uowcount >= parms.batchSize) && (0 == groupOpen))
					{
						MQCMIT(qm, &compcode, &reason);
						checkerror("MQCMIT", compcode, reason, parms.qname);
						uowcount = 0;
					}
				}
			}

			/* commit the messages we have just written */
			if ((parms.batchSize > 1) && (uowcount > 0))
			{
				MQCMIT(qm, &compcode, &reason);
				checkerror("MQCMIT", compcode, reason, parms.qname);
				uowcount = 0;
			}

			if (1 == parms.tune)
			{
				Log(FMTI64 " messages written to queue",parms.msgwritten - saveCount);
			}
		}

		if (parms.msgwritten >= parms.totcount)
		{
			/* make sure we do not end in the middle of a group */
			notDone = groupOpen;
		}
	}

	/* check if tuning of the sleep time was requested */
	if (1 == parms.tune)
	{
		/* write out the final sleep time value */
		Log("final sleep time value %d", parms.sleeptime);
	}

	/* write out the minimum and maximum number of messages on the queue */
	Log("number on queue after sleep - min %d, max %d", numOnQueueMin, numOnQueueMax);
#endif

	/* remember the ending time */
	GetTime(&endTime);

	/* write out the time the first messsage was sent */
	time(&endTOD);
	LogNoCRLF("Last message written at %s", ctime(&endTOD));

	/* write out the MQ timestamp of the last message */
	formatTime(formTime, puttime);
	Log("MQ timestamp of last message written at %8.8s", formTime);

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
	Log("\nTotal messages written " FMTI64 " out of " FMTI64, parms.msgwritten, parms.totcount);

	if (parms.msgwritten > 0)
	{
		/* calculate the total elapsed time in microseconds */
		elapsed = DiffTime(startTime, endTime);
		formatTimeDiffSecs(formTime, elapsed);
		Log("Total elapsed time in seconds %s", formTime);
	}

	Log("Total bytes written   " FMTI64, parms.byteswritten);
	Log("Total memory used %d", parms.memUsed);
#ifndef NOTUNE
	if (numWrittenMax > 0)
	{
		Log("Messages written in interval  min=%d max=%d", numWrittenMin, numWrittenMax);
	}
#endif

	/* close the input queue */
	Log("\nclosing the queue");
	MQCLOSE(qm, &q, MQCO_NONE, &compcode, &reason);

	checkerror("MQCLOSE", compcode, reason, parms.qname);

#ifndef NOTUNE
	/* close the inquiry queue handle */
	Log("closing the inquiry queue");
	MQCLOSE(qm, &Hinq, MQCO_NONE, &compcode, &reason);

	checkerror("MQCLOSE", compcode, reason, parms.qname);
#endif

	/* Disconnect from the queue manager */
	Log("disconnecting from the queue manager");
	MQDISC(&qm, &compcode, &reason);

	checkerror("MQDISC", compcode, reason, parms.qmname);

	/* check for a log file */
	if (parms.logFileName[0] != 0)
	{
		/* close the log file */
		closeLog();
	}

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

#ifdef NOTUNE
	Log("MQPUTS program ended");
#else
	Log("MQPUT2 program ended");
#endif

	return(0);
}
