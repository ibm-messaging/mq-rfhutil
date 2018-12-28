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
/*   MQCAPONE has 2 parameters                                      */
/*                                                                  */
/*      - name of the parameters file                               */
/*      - name of the file to store the captured messages           */
/*                                                                  */
/*    if no queue manager is specified, the default queue manager   */
/*    is used.                                                      */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.1                                                  */
/*                                                                  */
/* 1) Added support for Sun Solaris.                                */
/* 2) Use keyword rather than positional arguments.                 */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.3                                                  */
/*                                                                  */
/* 1) Added support for non-destructive gets.                       */
/* 2) Added support for capture of MQMD as well as message data     */
/* 3) Added maximum message length parameter override.              */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.3.1                                                */
/*                                                                  */
/* 1) Fixed bug where maxMsgLen was too large for client            */
/*    connections (2010 reason code on MQGET).                      */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.3.2                                                */
/*                                                                  */
/* 1) Added support for get by msgid, correlid and groupid          */
/* 2) Added check of completion code after MQINQ for max length.    */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.3.3                                                */
/*                                                                  */
/* 1) Added additional data display for 2080 reason code.           */
/* 2) Added timestamp in file name option.                          */
/* 3) Added wait time option.                                       */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V2.0.0                                                */
/*                                                                  */
/* 1) Changed to use a common parameters file.                      */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>

#ifdef SOLARIS
#include <sys/types.h>
#endif

/* includes for MQI */
#include <cmqc.h>

/* common subroutines include */
#include "int64defs.h"
#include "comsubs.h"
#include "timesubs.h"
#include "parmline.h"
#include "putparms.h"
#include "rfhsubs.h"

/* MQ subroutines include */
#include "qsubs.h"

/* global termination switch */
	volatile int	terminate=0;
	volatile int	cancelled=0;

static char Copyright[] = "(C) Copyright IBM Corp, 2001-2014";
static char Version[]=\
"@(#)MQCapone V3.0 - MQ data capture tool  - Jim MacNair";

#ifdef _DEBUG
	#ifdef MQCLIENT
		static char Level[]="mqcapone.c V3.0 Client Debug version ("__DATE__" "__TIME__")";
	#else
		static char Level[]="mqcapone.c V3.0 Debug version ("__DATE__" "__TIME__")";
	#endif
#else
	#ifdef MQCLIENT
		static char Level[]="mqcapone.c V3.0 Client Release version ("__DATE__" "__TIME__")";
	#else
		static char Level[]="mqcapone.c V3.0 Release version ("__DATE__" "__TIME__")";
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
	printf("   %s -f parm_filename <-o data_filename> <-m QMname> <-q queue> <-p>\n", pgmName);
	printf("         -p will purge the queue before capturing any messages\n");
	printf("         -m will override the queue manager and -q will override the queue name\n");
#ifdef MQCLIENT
	printf("         -m can be in the form of ChannelName/TCP/hostname(port)\n");
#endif
}

int main(int argc, char **argv)

{
	size_t		fDataLen;
	size_t		msgLen;						/* alternate message length used for 64-bit compatibility */
	size_t		memSize;					/* number of bytes to allocate */
	int			msgcount=0;
	int			maxrate=0;
	int			firstsec=0;
	int			secondcount=0;
	int			totalbytes=0;
	int			firstTime=0;
	int			rfhlength=0;
	int			waitCount;					/* count number of seconds this MQGET has waited */
	MQHCONN		qm=0;
	MQHOBJ		q=0;
	MQLONG		compcode;
	MQLONG		reason;
	MQLONG		Select[1];					/* attribute selectors           */
	MQLONG		IAV[1];						/* integer attribute values      */
	MQOD		objdesc = {MQOD_DEFAULT};
	MQMD2		msgdesc = {MQMD_DEFAULT};
	MQLONG		openopt = 0;
	MQGMO		mqgmo = {MQGMO_DEFAULT};
	MQLONG		datalen=0;
	FILE		*outFile;
	char		*msgdata;
	char		*dataptr;
	FILEPTR		*fptr=NULL;
	PUTPARMS	parms;						/* input parameters and global variables */

	/* print the copyright statement */
	Log(Copyright);
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
	printf("done processing command line arguments parms.err=%d\n",parms.err);

	if (parms.err != 0)
	{
		printHelp(argv[0]);
		exit(99);
	}

	if ((0 == parms.parmFilename[0]) || (0 == parms.outputFilename[0]))
	{
		printHelp(argv[0]);
		exit(99);
	}

	/* tell what parameters file we are using */
	printf("Reading parameters from file %s\n", parms.parmFilename);

	/* read the ini file to get the delimiter string */
	/* ignore any data files in the parameters file */
	fptr = processParmFile(parms.parmFilename, &parms, 0);

	/* return if unable to find parameters file */
	if (fptr != NULL)
	{
		/* exit */
		return 98;
	}

	/* check for overrides */
	processOverrides(&parms);

	/* exit if queue name not found */
	if (0 == parms.qname[0])
	{
		Log("***** Queue name not found in parameters file");
		return 2;
	}

	if (RFHSTRIP == parms.striprfh)
	{
		Log("StripRFH option selected - RFH headers will be discarded if present");
	}

	/* tell if we are in browse mode vs normal get mode */
	if (1 == parms.readOnly)
	{
		Log("Readonly option selected - using browse - messages will be left on the queue");
	}

	/* Tell what queue and qm will be used */
	if (0 == parms.qmname[0])
	{
		Log("Reading messages from Queue(%s) on default Qmgr", parms.qname);
	}
	else
	{
		Log("Reading messages from Queue(%s) on Qmgr(%s)", parms.qname, parms.qmname);
	}

	/* check for add time stamp option */
	if (1 == parms.addTimeStamp)
	{
		/* insert a timestamp into the file name */
		appendTimeStamp(&parms);

		/* check for error */
		if (1 == parms.err)
		{
			/* exit */
			return 97;
		}
	}

	/* open output file */
	Log("opening output file %s", parms.outputFilename);
	outFile = fopen(parms.outputFilename, "wb");

	if (NULL == outFile)
	{
		Log("unable to open output file %s for output", parms.outputFilename);
		return 100;
	}

	/* Connect to the queue manager */
#ifdef MQCLIENT
	clientConnect2QM(parms.qmname, &qm, &parms.maxmsglen, &compcode, &reason);
#else
	connect2QM(parms.qmname, &qm, &compcode, &reason);
#endif

	/* check for errors */
	if (compcode != MQCC_OK)
	{
		return 98;
	}

	/* set the queue open options */
	strncpy(objdesc.ObjectName, parms.qname, MQ_Q_NAME_LENGTH);

	/* check if should use non-destructive gets */
	if (0 == parms.readOnly)
	{
		openopt = MQOO_INPUT_SHARED + MQOO_INQUIRE + MQOO_FAIL_IF_QUIESCING;
	}
	else
	{
		openopt = MQOO_BROWSE + MQOO_INQUIRE + MQOO_FAIL_IF_QUIESCING;
	}

	/* open the queue for input */
	Log("opening queue %s for input", parms.qname);
	MQOPEN(qm, &objdesc, openopt, &q, &compcode, &reason);

	/* check for errors */
	checkerror("MQOPEN", compcode, reason, parms.qname);
	if (compcode != MQCC_OK)
	{
		return 97;
	}

	/* check if our maximum message length is too large         */
	/* This is to avoid 2010 return codes on client connections */
	Select[0] = MQIA_MAX_MSG_LENGTH;
	IAV[0]=0;
	MQINQ(qm, q, 1L, Select, 1L, IAV, 0L, NULL, &compcode, &reason);

	if ((0 == compcode) && (IAV[0] < parms.maxmsglen) && (IAV[0] > 0))
	{
		/* change the maximum message length to the maximum allowed */
		parms.maxmsglen = (unsigned int)IAV[0];
	}
	else
	{
		checkerror("MQINQ", compcode, reason, parms.qname);
	}

	/* allocate a buffer for the message */
	memSize = (unsigned int)parms.maxmsglen + 1;
	msgdata = (char *)malloc(memSize);
	memset(msgdata, 0, memSize);

	/* display the maximum message length that can be read */
	Log("Maximum message size that can be read is %d", parms.maxmsglen);

	/* set a termination handler */
	signal(SIGINT, InterruptHandler);

	/* set the get message options */
	mqgmo.Version = MQGMO_VERSION_2;
	mqgmo.Options = MQGMO_WAIT | MQGMO_FAIL_IF_QUIESCING;
	mqgmo.MatchOptions = MQGMO_NONE;

	/* check for non-destructive read */
	if (1 == parms.readOnly)
	{
		mqgmo.Options |= MQGMO_BROWSE_NEXT;
	}

	/* reset the msgid, correlid and groupid */
	memcpy(msgdesc.MsgId, MQMI_NONE, sizeof(msgdesc.MsgId));
	memcpy(msgdesc.CorrelId, MQCI_NONE, sizeof(msgdesc.CorrelId));
	memcpy(msgdesc.GroupId, MQGI_NONE, sizeof(msgdesc.GroupId));

	/* check if a msg id was specified */
	if (1 == parms.msgidSet)
	{
		memcpy(msgdesc.MsgId, parms.msgid, MQ_MSG_ID_LENGTH);
		mqgmo.MatchOptions |= MQMO_MATCH_MSG_ID;
 	}

	/* check if a correl id was specified */
	if (1 == parms.correlidSet)
	{
		memcpy(msgdesc.CorrelId, parms.correlid, MQ_CORREL_ID_LENGTH);
		mqgmo.MatchOptions |= MQMO_MATCH_CORREL_ID;
	}

	/* check if a group id was specified */
	if (1 == parms.groupidSet)
	{
		memcpy(msgdesc.GroupId, parms.groupid, MQ_GROUP_ID_LENGTH);
		mqgmo.MatchOptions |= MQMO_MATCH_GROUP_ID;
	}

	/* set the wait count to zero */
	/* this keeps track of the number of seconds waiting for a message */
	waitCount = 0;

	do {
		/* wait for maximum of one second (1000 ms), checking the terminate switch each second */
		mqgmo.WaitInterval = 1000;

		/* perform the MQGET */
		MQGET(qm, q, &msgdesc, &mqgmo, parms.maxmsglen, msgdata, &datalen, &compcode, &reason);

		/* increment the wait time counter */
		waitCount++;
	} while ((0 == terminate) && (waitCount < parms.waitTime) && (MQRC_NO_MSG_AVAILABLE == reason));

	if ((0 == terminate) && (reason != MQRC_NONE) && (reason != MQRC_NO_MSG_AVAILABLE))
	{
		/* check for errors */
		checkerror("MQGET", compcode, reason, parms.qname);
	}

	if (MQCC_OK == compcode)
	{
		/* count the total number of messages read */
		msgcount++;

		/* point to the data area */
		dataptr = msgdata;

		if (0 == parms.saveMQMD)
		{
			/* check for an MQMDE present */
			if ((datalen >= MQMDE_LENGTH_2) && (memcmp(dataptr, MQMDE_STRUC_ID, sizeof(MQMDE_STRUC_ID) - 1) == 0))
			{
				dataptr += MQMDE_LENGTH_2;
				datalen -= MQMDE_LENGTH_2;
				Log("MQMD extension removed from data");
			}
		}

		/* do we need to check for an RFH at the front of the data? */
		if (RFHSTRIP == parms.striprfh)
		{
			/* check the message format for an RFH */
			if ((memcmp(msgdesc.Format, MQFMT_RF_HEADER, sizeof(MQFMT_RF_HEADER) - 1) == 0) ||
				(memcmp(msgdesc.Format, MQFMT_RF_HEADER_2, sizeof(MQFMT_RF_HEADER_2) - 1) == 0))
			{
				msgLen = (unsigned int)datalen;
				rfhlength = checkRFH(dataptr, msgLen, &msgdesc, &parms);
			}
		}

		/* should we include the MQMD in the file data? */
		if (1 == parms.saveMQMD)
		{
			fwrite(&msgdesc, sizeof(msgdesc), 1, outFile);
		}

		/* calculate the total bytes in the message */
		totalbytes += datalen;

		/* append the data to the file */
		fDataLen = (unsigned int)datalen - (unsigned int)rfhlength;
		fwrite(msgdata + rfhlength, fDataLen, 1, outFile);
	} 
	else if ((compcode == MQCC_WARNING) && (2080 == reason))
	{
		/* provide some additional details about the truncation */
		Log("***** Input buffer too short maxMsgLen=%d but messageLength=%d", parms.maxmsglen, datalen);
	}

	/* close the file */
	fclose(outFile);

	/* close the input queue */
	Log("closing the input queue");
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
	Log("Total messages %d", msgcount);

	/* give the total and average message size */
	if (msgcount > 0)
	{
		Log("total bytes in message %d", totalbytes);
	}

	/******************************************************************/
	/*                                                                */
	/* END OF PROGRAM                                                 */
	/*                                                                */
	/******************************************************************/

	Log("mqcapone program ended");
	return 0;
}
