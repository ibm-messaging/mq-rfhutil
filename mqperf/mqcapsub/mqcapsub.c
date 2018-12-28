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

/**********************************************************************/
/*                                                                    */
/* mqcapsub.cpp : Defines the entry point for the console application.*/
/*                                                                    */
/**********************************************************************/

/********************************************************************/
/*                                                                  */
/*   MQCAPSUB has 2 parameters                                      */
/*                                                                  */
/*      - name of the parameters file                               */
/*      - name of the file to store the captured messages           */
/*                                                                  */
/*    if no queue manager is specified, the default queue manager   */
/*    is used.                                                      */
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

/* parameters file handling */
#include "parmline.h"
#include "putparms.h"

/* MQ subroutines include */
#include "qsubs.h"
#include "rfhsubs.h"

/* global termination switch */
	volatile int	terminate=0;
	volatile int	cancelled=0;

static char Copyright[] = "(C) Copyright IBM Corp, 2010-2014";
static char Version[]=\
"@(#)MQCapSub V3.0 - MQ subscription data capture tool  - Jim MacNair";

#ifdef _DEBUG
	#ifdef MQCLIENT
		static char Level[]="mqcapsub.c V3.0 Client Debug version ("__DATE__" "__TIME__")";
	#else
		static char Level[]="mqcapsub.c V3.0 Debug version ("__DATE__" "__TIME__")";
	#endif
#else
	#ifdef MQCLIENT
		static char Level[]="mqcapsub.c V3.0 Client Release version ("__DATE__" "__TIME__")";
	#else
		static char Level[]="mqcapsub.c V3.0 Release version ("__DATE__" "__TIME__")";
	#endif
#endif

/*                                                            */
/* Process a line in the parameters file.                     */
/*                                                            */
/**************************************************************/

void displayOptions(PUTPARMS * parms)

{
	/* check if we are using logical order */
	if (RFHSTRIP == parms->striprfh)
	{
		Log("StripRFH option selected - RFH headers will be discarded if present");
	}

	/* tell if we are saving the MQMD with the data */
	if (1 == parms->saveMQMD)
	{
		Log("SaveMQMD option selected - Message descriptors will be saved with the message data");
	}

	/* Tell what topic and qm will be used */
	if (0 == parms->qmname[0])
	{
		Log("Reading messages from Topic(%s) topic name(%s) on default Qmgr", parms->topicStr, parms->topicName);
	}
	else
	{
		Log("Reading messages from Topic(%s) topic name(%s) on Qmgr(%s)", parms->topicStr, parms->topicName, parms->qmname);
	}

	/* Tell what delimiter we are using */
	if (0 == parms->delimiterLen)
	{
		/* print a warning message */
		Log("***** WARNING - delimiter length is zero");
	}
	else
	{
		/* print the delimiter */
		Log("Delimiter length is %d  value(%s)", parms->delimiterLen, parms->delimiter);
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
	printf("format is:\n");
	printf("   %s -f parm_filename -o data_filename -m QMname -v\n", pgmName);
	printf("         -v verbose (output message for each message received)\n");
	printf("         -m will override the queue manager\n");
#ifdef MQCLIENT
	printf("         -m can be in the form of ChannelName/TCP/hostname(port)\n");
#endif
}

int main(int argc, char* argv[])

{
	int64_t			msgcount=0;					/* number of messages read */
	int64_t			totalbytes=0;				/* total bytes in all messages */
	int64_t			avgbytes;					/* average message size */
	size_t			memSize;					/* number of bytes to allocate */
	size_t			fileLen=0;					/* starting length of output file */
	unsigned int	rfhlength=0;				/* length of RFH header */
	int				fileCount=0;				/* count of individual files used */
	MQHCONN			qm=0;						/* queue manager connection handle */
	MQHOBJ			subQ=MQHO_NONE;				/* subscription queue handle */
	MQHOBJ			hSub;						/* object handle for subscription */
	MQLONG			compcode;					/* MQ completion code */
	MQLONG			reason;						/* MQ reason code */
	MQLONG			msgLen=0;					/* length of message read */
	MQLONG			Select[1];					/* attribute selectors           */
	MQLONG			IAV[1];						/* integer attribute values      */
	MQLONG			openopt = 0;				/* MQ open options */
	MQLONG			Selector;					/* used for MQINQ */
	FILE			*outFile;					/* output file */
	char			*msgdata;					/* pointer to message data */
	MQMD2			mqmd = {MQMD2_DEFAULT};		/* MQMD used for reads */
	MQMD2			newMqmd = {MQMD2_DEFAULT};	/* default MQMD settings */
	MQGMO			gmo = {MQGMO_DEFAULT};		/* get message options structure used in MQGET */
	MQSD			sd={MQSD_DEFAULT};			/* Subscription Descriptor */
	MQOD			od = {MQOD_DEFAULT};	    /* MQ object descriptor used for MQOPEN */
	char			queueName[MQ_Q_NAME_LENGTH+8];	/* work area to retrieve the name of the managed queue */
	char			subName[MQ_SUB_IDENTITY_LENGTH + 8];
	char			fullTopic[MQ_TOPIC_NAME_LENGTH+8];	/* work area to get the full topic name that is returned from MQSUB */
	PUTPARMS		parms;						/* input parameters and global variables */

	/* print the copyright statement */
	Log(Copyright);
	Log(Level);

	/* initialize the work areas */
	initializeParms(&parms, sizeof(PUTPARMS));

	/* make sure the returned queue name is properly initialized */
	memset(queueName, 0, sizeof(queueName));

	/* initialize the subscription name and returned topic name */
	memset(subName, 0, sizeof(subName));
	memset(fullTopic, 0, sizeof(fullTopic));

	/* check for too few input parameters */
	if (argc < 2)
	{
		Log("Too few parameters");
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

	if (0 == parms.parmFilename[0])
	{
		Log("parameters file name is missing");
		printHelp(argv[0]);
		exit(99);
	}

	/* tell what parameters file we are using */
	printf("Reading parameters from file %s\n", parms.parmFilename);

	/* read the parameters file to get the delimiter string */
	processParmFile(parms.parmFilename, &parms, SKIPDATAFILES);

	/* check for overrides */
	processOverrides(&parms);

	/* exit if topic not found */
	if ((0 == parms.topicStr[0]) && (0 == parms.topicName[0]))
	{
		Log("***** Neither Topic string or topic name not found in parameters file");
		return 2;
	}

	/* check for output file */
	if (0 == parms.outputFilename[0])
	{
		Log("***** Output file not found");
		printHelp(argv[0]);
		exit(0);
	}

	/* display the selected options */
	displayOptions(&parms);

	/* Connect to the queue manager */
#ifdef MQCLIENT
	clientConnect2QM(parms.qmname, &qm, &(parms.maxmsglen), &compcode, &reason);
#else
	connect2QM(parms.qmname, &qm, &compcode, &reason);
#endif

	/* check for errors */
	if (compcode != MQCC_OK)
	{
		/* exit if unable to connect */
		return reason;
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

	/* set the topic string */
	sd.ObjectString.VSPtr = (void *)parms.topicStr;
	sd.ObjectString.VSLength = MQVS_NULL_TERMINATED;

	/* set the pub/sub options */
	sd.Options = MQSO_FAIL_IF_QUIESCING | MQSO_CREATE | MQSO_WILDCARD_TOPIC | MQSO_MANAGED;

	/* set up the subscription name */
	sd.SubName.VSPtr = (void *)subName;					   
	sd.SubName.VSLength = MQVS_NULL_TERMINATED;

	/* check for a topic name */
	if (parms.topicName[0] != 0)
	{
		/* set the topic name in the subscription descriptor */
		memcpy(sd.ObjectName, parms.topicName, strlen(parms.topicName));
	}
	
	/* next check for a selector when creating a subscription */
	if (parms.selector[0] |= 0)
	{
		/* set the selection string pointer and indicate it is a null terminated string */
		sd.SelectionString.VSPtr = (void *)parms.selector;
		sd.SelectionString.VSLength = MQVS_NULL_TERMINATED;
	}

	/* set up a buffer to capture the full topic string that was subscribed to */
	memset(fullTopic, 0, sizeof(fullTopic));
	sd.ResObjectString.VSPtr = fullTopic;
	sd.ResObjectString.VSBufSize = sizeof(fullTopic);

	MQSUB(qm, &sd, &subQ, &hSub, &compcode, &reason);

	if (compcode != MQCC_OK)
	{
		/* return with an error message */
		Log("Error %d subscribing to topic %s topic name %s", reason, parms.topicStr, parms.topicName);

		/* disconnect from the queue manager */
		MQDISC(&qm, &compcode, &reason);

		/* close the file */
		fclose(outFile);

		exit(1);
	}

	/* return the full topic name */
	if (sd.ResObjectString.VSPtr != NULL)
	{
		/* check if the string is null terminated */
		if (MQVS_NULL_TERMINATED == sd.ResObjectString.VSLength)
		{
			/* set the full topic name using a strcpy */
			Log("Subscription created to topic %s", (const char *)sd.ResObjectString.VSPtr);
		}
		else if (sd.ResObjectString.VSLength > 0)
		{
			/* length is specified so use memcpy and remember to terminate the string */
			memSize = (unsigned int)sd.ResObjectString.VSLength;
			memcpy(fullTopic, sd.ResObjectString.VSPtr, memSize);
			Log("Subscription created to topic %s", (const char *)fullTopic);
		}
	}

	/* get the name of the managed queue */
	/* do an inquiry for the queue name  */
	Selector = MQCA_Q_NAME;
	memset(queueName, 0, sizeof(queueName));
	MQINQ(qm,						/* connection handle to queue manager */
		  subQ,						/* object handle for q                */
		  1,						/* inquire only one selector          */
		  &Selector,				/* the selector to inquire            */
		  0,						/* no integer attributes needed       */
		  NULL,						/* pointer to the integer result      */
		  sizeof(queueName),		/* inquiring on character value       */
		  queueName,				/* pointer to the character results   */
		  &compcode,				/* completion code                    */
		  &reason);					/* reason code                        */

	/* check if the inq worked */
	if (MQCC_OK == compcode)
	{
		/* return the name of the managed queue */
		Log("Subscription queue name is %s", queueName);
	}

	/* check if our maximum message length is too large         */
	/* This is to avoid 2010 return codes on client connections */
	Select[0] = MQIA_MAX_MSG_LENGTH;
	IAV[0]=0;
	MQINQ(qm, subQ, 1L, Select, 1L, IAV, 0L, NULL, &compcode, &reason);

	if ((0 == compcode) && (IAV[0] < parms.maxmsglen) && (IAV[0] > 0))
	{
		/* change the maximum message length to the maximum allowed */
		parms.maxmsglen = IAV[0];
	}
	else
	{
		checkerror("MQINQ", compcode, reason, queueName);
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

	/* set a termination handler */
	signal(SIGINT, InterruptHandler);

	/* enter get message loop */
	while ((compcode == MQCC_OK) && (0 == terminate) && ((0 == parms.totcount) || (msgcount < parms.totcount)))
	{
		gmo.Options = MQGMO_WAIT | MQGMO_FAIL_IF_QUIESCING | MQGMO_NO_SYNCPOINT | MQGMO_PROPERTIES_COMPATIBILITY;
		gmo.MatchOptions = MQMO_NONE;

		/* reset the MQMD - especially the message id */
		memcpy((void *)&mqmd, (void *)&newMqmd, sizeof(mqmd));

		/* set the wait time in the GMO to one second */
		gmo.WaitInterval = 1000;

		do
		{
			/* Try to get a message into the default buffer */
			MQGET(qm, subQ, &mqmd, &gmo, parms.maxmsglen, msgdata, &msgLen, &compcode, &reason);
		} while ((MQRC_NO_MSG_AVAILABLE == reason) && (0 == terminate));

		/* did the user terminate the program? */
		if (0 == terminate)
		{
			/* check the return code */
			if (MQCC_OK == compcode)
			{
				/* get worked - process the message */
				/* count the total number of messages read */
				msgcount++;

				/* calculate the total bytes in the message */
				totalbytes += msgLen;

				/* check if a delimiter should be added to the file */
				if (0 == fileLen)
				{
					fileLen = 1;
				}
				else
				{
					/* insert a delimiter string */
					fwrite(parms.delimiter, parms.delimiterLen, 1, outFile);
				}

				/* do we need to check for an RFH at the front of the data? */
				rfhlength = 0;
				if (RFHSTRIP == parms.striprfh)
				{
					/* check the message format for an RFH */
					if (memcmp(mqmd.Format, MQFMT_RF_HEADER_2, sizeof(MQFMT_RF_HEADER_2) - 1) == 0)
					{
						/* get the length of the RFH and update the MQMD format, encoding and code page */
						rfhlength = checkRFH(msgdata, msgLen, &mqmd, &parms);
					}
				}

				/* should we include the MQMD in the file data? */
				if (1 == parms.saveMQMD)
				{
					/* write the MQMD to the file */
					fwrite(&mqmd, sizeof(mqmd), 1, outFile);
				}

				/* append the data to the file */
				fwrite(msgdata + rfhlength, msgLen - rfhlength, 1, outFile);

				/* force out the data */
				fflush(outFile);

				/* check if additional output was requested */
				if (1 == parms.verbose)
				{
					/* tell what is happening */
					Log("Message written to data file - length=%d", msgLen);
				}
			}
			else
			{
				/* issue error message */
				checkerror("MQGET", compcode, reason, queueName);
			}
		}
	}

	/* close the file */
	fclose(outFile);

	/* close the subscription */
	MQCLOSE(qm, &hSub, 0, &compcode, &reason);

	/* check if it worked */
	if (compcode != MQCC_OK)
	{
		/* report the error including the completion and reason codes */
		Log("Error closing subscription cc=%d rc=%d", compcode, reason);
	}

	/* close the subscription queue */
	MQCLOSE(qm, &subQ, MQCO_NONE, &compcode, &reason);

	/* check if it worked */
	if (compcode != MQCC_OK)
	{
		/* report the error including the completion and reason codes */
		Log("Error closing publication queue cc=%d rc=%d", compcode, reason);
	}

	/* disconnect from the queue manager */
	MQDISC(&qm, &compcode, &reason);

	/* check if it worked */
	if (compcode != MQCC_OK)
	{
		/* report the error including the completion and reason codes */
		Log("Error disconnecting from queue manager cc=%d rc=%d", compcode, reason);
	}

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

	Log("mqcapsub program ended");

	return 0;
}
