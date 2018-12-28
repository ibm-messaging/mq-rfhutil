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
/*   MQCAPTURE has 1 required parameter                             */
/*                                                                  */
/*      - name of the parameters file                               */
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
/* 3) Removed spurious remote qm name paramter.                     */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.2                                                  */
/*                                                                  */
/* 1) Added support for groups.                                     */
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
/* 1) Added display of maximum message length.                      */
/* 2) Do not replace maximum message size if inq returns 0.         */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V1.3.3                                                */
/*                                                                  */
/* 1) Added append file option.                                     */
/* 2) Added timestamp in file name option.                          */
/* 3) Added wait time option.                                       */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* Changes in V3.0                                                  */
/*                                                                  */
/* 1) Changed to use common parameters file.                        */
/* 2) Updated to Visual Studio 2010 compiler for Windows            */
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

/* common subroutines includes */
#include "int64defs.h"
#include "comsubs.h"
#include "timesubs.h"
#include "parmline.h"
#include "putparms.h"

/* MQ user subroutines includes */
#include "qsubs.h"
#include "rfhsubs.h"

/* global termination switch */
	volatile int	terminate=0;
	volatile int	cancelled=0;

static char Copyright[] = "(C) Copyright IBM Corp, 2001-2014";
static char Version[]=\
"@(#)MQCapture V3.0 - MQ data capture tool  - Jim MacNair";

#ifdef _DEBUG
	#ifdef MQCLIENT
		static char Level[]="mqcapture.c V3.0 Client Debug version ("__DATE__" "__TIME__")";
	#else
		static char Level[]="mqcapture.c V3.0 Debug version ("__DATE__" "__TIME__")";
	#endif
#else
	#ifdef MQCLIENT
		static char Level[]="mqcapture.c V3.0 Client Release version ("__DATE__" "__TIME__")";
	#else
		static char Level[]="mqcapture.c V3.0 Release version ("__DATE__" "__TIME__")";
	#endif
#endif

void displayOptions(PUTPARMS * parms)

{
	unsigned char	delimInHex[72];

	/* check if we are using logical order */
	if (1 == parms->logicalOrder)
	{
		Log("Logical order option selected");
	}

	if (RFHSTRIP == parms->striprfh)
	{
		Log("StripRFH option selected - RFH headers will be discarded if present");
	}

	/* tell if we are in browse mode vs normal get mode */
	if (1 == parms->readOnly)
	{
		Log("Readonly option selected - using browse - messages will be left on the queue");
	}

	/* tell if we are saving the MQMD with the data */
	if (1 == parms->saveMQMD)
	{
		Log("SaveMQMD option selected - Message descriptors will be saved with the message data");
	}

	/* tell if we are each message in its own file */
	if (1 == parms->indivFiles)
	{
		Log("indivFiles option selected - Each file will be saved in a seperate file");
	}

	/* Tell what queue and qm will be used */
	if (0 == parms->qmname[0])
	{
		Log("Reading messages from Queue(%s) on default Qmgr", parms->qname);
	}
	else
	{
		Log("Reading messages from Queue(%s) on Qmgr(%s)", parms->qname, parms->qmname);
	}

	/* Tell what delimiter we are using */
	if ((0 == parms->delimiterLen) && (0 == parms->indivFiles))
	{
		/* print a warning message */
		Log("***** WARNING - delimiter length is zero");
	}
	else if (0 == parms->indivFiles)
	{
		if (0 == parms->delimiterIsHex)
		{
			/* print the delimiter */
			Log("Delimiter length is %d  value(%s)", parms->delimiterLen, parms->delimiter);
		}
		else
		{
			/* print the delimiter - allow for binary value */
			memset(delimInHex, 0, sizeof(delimInHex));
			AsciiToHex(delimInHex, (unsigned char *)parms->delimiter, parms->iDelimiterLen);
			Log("Delimiter length is %d  value in hex(%s)", parms->iDelimiterLen, delimInHex);
		}
	}
}

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
	printf("   %s -f parm_filename <-o data_filename> <-m QMname> <-q queue> <-g> <-p>\n", pgmName);
	printf("         -p will purge the queue before capturing any messages\n");
	printf("         -g is used with MQ message groups for logical ordering\n");
	printf("         -m will override the queue manager and -q will override the queue name\n");
#ifdef MQCLIENT
	printf("         -m can be in the form of ChannelName/TCP/hostname(port)\n");
#endif
}

int main(int argc, char **argv)
{
	int64_t			msgcount=0;
	int64_t			totalbytes=0;
	int64_t			avgbytes;
	size_t			fileLen=0;					/* starting length of output file */
	size_t			fDataLen;
	size_t			msgLen;						/* alternate message length used for 64-bit compatibility */
	size_t			memSize;					/* number of bytes to allocate */
	unsigned int	rfhlength=0;
	int				fileCount=0;				/* count of individual files used */
	int				waitCount;					/* count number of seconds this MQGET has waited */
	MQLONG			qm=0;
	MQLONG			q=0;
	MQLONG			compcode;
	MQLONG			reason;
	MQLONG			Select[1];					/* attribute selectors           */
	MQLONG			IAV[1];						/* integer attribute values      */
	MQLONG			openopt = 0;				/* MQ open options */
	MQLONG			datalen=0;					/* length of the message that was read */
	FILE			*outFile;					/* output file */
	char			*msgdata;					/* pointer to message data */
	MQOD			objdesc = {MQOD_DEFAULT};
	MQMD2			msgdesc = {MQMD_DEFAULT};
	MQGMO			mqgmo = {MQGMO_DEFAULT};	/* get message options structure used in MQGET */
	MQOD			od = {MQOD_DEFAULT};	    /* Object Descriptor             */
	char			newFileName[512];			/* file name to open - needed when more than one message per file */
	FILEPTR			*fptr=NULL;
	PUTPARMS		parms;						/* input parameters and global variables */

	/* print the copyright statement */
	Log(Copyright);
	Log(Level);

	/* initialize the work areas */
	initializeParms(&parms, sizeof(PUTPARMS));

	/* check for too few input parameters */
	if (argc < 3)
	{
		printHelp(argv[0]);
		exit(99);
	}

	/* process any command line arguments */
	processArgs(argc, argv, &parms);

	if (parms.err != 0)
	{
		printHelp(argv[0]);
		exit(99);
	}

	if (0 == parms.parmFilename[0])
	{
		printHelp(argv[0]);
		exit(99);
	}

	/* tell what parameters file we are using */
	printf("Reading parameters from file %s\n", parms.parmFilename);

	/* read the parameters file to get the delimiter string */
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

	/* check for help request */
	if ((parms.outputFilename[0] == '?') || (parms.outputFilename[1] == '?'))
	{
		printHelp(argv[0]);
		exit(0);
	}

	/* display the selected options */
	displayOptions(&parms);

	/* check for add time stamp option without individual files option */
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

	/* check for an append option */
	if (1 == parms.appendFile)
	{
		/* open output file for append */
		Log("appending to output file %s", parms.outputFilename);
		outFile = fopen(parms.outputFilename, "ab");
	}
	else
	{
		/* open output file */
		Log("opening output file %s", parms.outputFilename);
		outFile = fopen(parms.outputFilename, "wb");
	}

	if (NULL == outFile)
	{
		Log("unable to open output file %s for output", parms.outputFilename);
		return 100;
	}

	/* check for an append option */
	if (1 == parms.appendFile)
	{
		/* check if the file is empty or being appended */
		fseek(outFile, 0L, SEEK_END);
		fileLen = ftell(outFile);
	}

	/* Connect to the queue manager */
#ifdef MQCLIENT
	clientConnect2QM(parms.qmname, &qm, &(parms.maxmsglen), &compcode, &reason);
#else
	connect2QM(parms.qmname, &qm, &compcode, &reason);
#endif

	/* check for errors */
	if (compcode != MQCC_OK)
	{
		/* close the output file */
		fclose(outFile);

		/* exit */
		return 98;
	}

	/* set the queue open options */
	strncpy(objdesc.ObjectName, parms.qname, MQ_Q_NAME_LENGTH);

	/* check if should use non-destructive gets */
	if (0 == parms.readOnly)
	{
		openopt = MQOO_INPUT_SHARED + MQOO_FAIL_IF_QUIESCING + MQOO_INQUIRE;
	}
	else
	{
		openopt = MQOO_BROWSE + MQOO_FAIL_IF_QUIESCING + MQOO_INQUIRE;
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

	/* check if the allocation was successful */
	if (NULL == msgdata)
	{
		/* issue error message and exit */
		Log("*****Memory allocation for message data area failed");

		/* force immediate exit */
		terminate = 1;
	}
	else
	{
		/* initialize the message data area */
		memset(msgdata, 0, memSize);
	}

	/* display the maximum message length that can be read */
	Log("Maximum message size that can be read is %d", parms.maxmsglen);

	/* set a termination handler */
	signal(SIGINT, InterruptHandler);

	/* enter get message loop */
	while ((compcode == MQCC_OK) && (0 == terminate) && ((0 == parms.totcount) || (msgcount < parms.totcount)))
	{
		/* set the get message options, including one second wait */
		mqgmo.Options = MQGMO_WAIT | MQGMO_FAIL_IF_QUIESCING;
		mqgmo.WaitInterval = 1000;
		mqgmo.MatchOptions = MQGMO_NONE;

		/* check for logical order */
		if (1 == parms.logicalOrder)
		{
			mqgmo.Options |= MQGMO_LOGICAL_ORDER;
		}

		/* check for non-destructive gets */
		if (1 == parms.readOnly)
		{
			/* use browse */
			mqgmo.Options |= MQGMO_BROWSE_NEXT;
		}

		/* reset the msgid and correlid */
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
			/* perform the MQGET */
			MQGET(qm, q, &msgdesc, &mqgmo, parms.maxmsglen, msgdata, &datalen, &compcode, &reason);

			/* increment the wait time counter */
			waitCount++;
		} while ((0 == terminate) && (waitCount < parms.waitTime) && (MQRC_NO_MSG_AVAILABLE == reason));

	Log("waitCount=%d parms.waitTime=%d", waitCount, parms.waitTime);
		/* check for the end of the queue */
		/* check for errors, except for no more messages in queue */
		if (reason != MQRC_NO_MSG_AVAILABLE)
		{
			checkerror("MQGET", compcode, reason, parms.qname);
		}

		if (compcode == MQCC_OK)
		{
			/* count the total number of messages read */
			msgcount++;

			/* calculate the total bytes in the message */
			totalbytes += datalen;

			/* check if a delimiter should be added to the file */
			if ((0 == fileLen) || (1 == parms.indivFiles))
			{
				fileLen = 1;
			}
			else
			{
				/* insert a delimiter string */
				fwrite(parms.delimiter, parms.delimiterLen, 1, outFile);
			}


			/* check for an RFH at the front of the data? */
			rfhlength = 0;
			if (RFHSTRIP == parms.striprfh)
			{
				/* check the message format for an RFH */
				if ((memcmp(msgdesc.Format, MQFMT_RF_HEADER, sizeof(MQFMT_RF_HEADER) - 1) == 0) ||
					(memcmp(msgdesc.Format, MQFMT_RF_HEADER_2, sizeof(MQFMT_RF_HEADER_2) - 1) == 0))
				{
					/* get the length of the RFH and update the MQMD format, encoding and code page */
					msgLen = (unsigned int)datalen;
					rfhlength = checkRFH(msgdata, msgLen, &msgdesc, &parms);
				}
			}

			/* include the MQMD in the file data? */
			if (1 == parms.saveMQMD)
			{
				/* write the MQMD to the file */
				fwrite(&msgdesc, sizeof(msgdesc), 1, outFile);
			}

			/* append the data to the file */
			fDataLen = (unsigned int)datalen - (unsigned int)rfhlength;
			fwrite(msgdata + rfhlength, fDataLen, 1, outFile);

			/* check if individual files are to be used for each message */
			if (1 == parms.indivFiles)
			{
				/* using one file per message */
				if (outFile != NULL)
				{
					/* close the current file */
					fclose(outFile);
					outFile = NULL;
				}

				/* increment the file counter */
				fileCount++;

				/* build the next file name to use */
				createNextFileName(parms.outputFilename, newFileName, fileCount);

				/* try to open the new output file */
				outFile = fopen(newFileName, "wb");

				/* check if the file open failed */
				if (NULL == outFile)
				{
					/* tell what happened */
					Log("***** Open failed for file %s", newFileName);

					/* break out of the loop */
					compcode = MQCC_FAILED;
				}
			}
		} 
		else if ((compcode == MQCC_WARNING) && (2080 == reason))
		{
			/* provide some additional details about the truncation */
			Log("***** Input buffer too short maxMsgLen=%d but messageLength=%d", parms.maxmsglen, datalen);
		}
	}

	/* close the file */
	fclose(outFile);

	/* files are created before reading messages  */
	/* delete the last file since it was not used */
	if (1 == parms.indivFiles)
	{
		/* delete the last file that was created */
		remove(newFileName);
	}

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
	Log("Total messages " FMTI64, msgcount);

	/* were inidividual files used for each message? */
	if (1 == parms.indivFiles)
	{
		/* display the number of files used */
		Log("Total files written %d", fileCount);
	}

	/* give the total and average message size */
	if (msgcount > 0)
	{
		avgbytes = totalbytes / msgcount;
		Log("total bytes in all messages " FMTI64, totalbytes);
		Log("average message size " FMTI64, avgbytes);
	}

	/* was storage acquired */
	if (msgdata != NULL)
	{
		/* release the storage that was acquired for the message data */
		free(msgdata);
	}

	/******************************************************************/
	/*                                                                */
	/* END OF PROGRAM                                                 */
	/*                                                                */
	/******************************************************************/

	Log("mqcapture program ended");
	return 0;
}
