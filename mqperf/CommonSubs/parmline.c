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
/*   parmline.c - Input parameter file processing subroutines       */
/*                                                                  */
/********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
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

/* RFH processing subroutines include */
#include "rfhsubs.h"

#define MAX_BATCH_ALLOW		5000
#define DEFAULT_DELIMITER	"#@#@#"

/* section headers in parameters file */
#define HEADER		"[HEADER]"
#define FILELIST	"[FILELIST]"
/* common parameters */
#define QMGR				"QMGR"
#define QNAME				"QNAME"
#define	REMOTEQM			"REMOTEQM"
#define REPLYQNAME			"REPLYQNAME"
#define REPLYQMNAME			"REPLYQMNAME"
#define	TOPIC				"TOPIC"
#define	TOPICNAME			"TOPICNAME"
#define MSGCOUNT			"MSGCOUNT"
#define WRITEONCE			"WRITEONCE"
#define REPORTEVERY			"REPORTEVERY"
#define REPORTEVERYSECOND	"REPORTEVERYSECOND"
#define REPORTINTERVAL		"REPORTINTERVAL"
#define QDEPTH				"QDEPTH"
#define QMAX				"QMAX"
#define TUNE				"TUNE"
#define SLEEPTIME			"SLEEPTIME"
#define THINKTIME			"THINKTIME"
#define BATCHSIZE			"BATCHSIZE"
#define MAXTIME				"MAXTIME"
#define DELIMITER			"DELIMITER"
#define DELIMITERX			"DELIMITERX"
#define SILENT				"SILENT"
#define FILEASGROUP			"FILEASGROUP"
/* fields related to latency measurements */
#define SETTIMESTAMP		"SETTIMESTAMP"
#define TIMESTAMPOFFSET		"TIMESTAMPOFFSET"
#define TIMESTAMPACCTTOKEN	"TIMESTAMPACCTTOKEN"
#define TIMESTAMPGROUPID	"TIMESTAMPGROUPID"
#define TIMESTAMPCORRELID	"TIMESTAMPCORRELID"
#define TIMESTAMPUSERPROP	"TIMESTAMPUSERPROP"
/* handling of embedded MQMDs */
/* determine if MQMDs are saved with data by capture programs */
#define IGNOREMQMD			"IGNOREMQMD"
/* fields used by MQTimes2 and MQTimes3 programs */
#define DRAINQ				"DRAINQ"
/* fields used by MQReply program */
#define MAXREPLYTIME		"MAXREPLYTIME"
#define NANREPLYFILE		"NANREPLYFILE"
#define PANREPLYFILE		"PANREPLYFILE"
#define REPLYFILENAME		"REPLYFILENAME"
/* fields used by capture programs */
#define OUTPUTFILENAME		"OUTPUTFILENAME"
#define APPENDFILE			"APPENDFILE"
#define ADDTIMESTAMP		"ADDTIMESTAMP"
#define STRIPRFH			"STRIPRFH"
#define WAITTIME			"WAITTIME"
/* MQPMO options */
#define NEWMSGID			"NEWMSGID"
/* MQGMO options */
#define READONLY			"READONLY"
#define LOGICALORDER		"LOGICALORDER"
/* fields related to MQMD */
#define REPORT				"REPORT"
#define MSGTYPE				"MSGTYPE"
#define EXPIRY				"EXPIRY"
#define FEEDBACK			"FEEDBACK"
#define ENCODING			"ENCODING"
#define CODEPAGE			"CODEPAGE"
#define CCSID				"CCSID"
#define ENCODING			"ENCODING"
#define FORMAT				"FORMAT"
#define PRIORITY			"PRIORITY"
#define PERSIST				"PERSIST"
#define MSGID				"MSGID"
#define MSGIDX				"MSGIDX"
#define CORRELID			"CORRELID"
#define CORRELIDX			"CORRELIDX"
#define GROUPID				"GROUPID"
#define GROUPIDX			"GROUPIDX"
#define REPLYQ				"REPLYQ"
#define REPLYQM				"REPLYQM"
#define USERID				"USERID"
#define ACCOUNTINGTOKEN		"ACCOUNTINGTOKEN"
#define ACCOUNTINGTOKENX	"ACCOUNTINGTOKENX"
#define APPLID				"APPLID"
#define APPLTYPE			"APPLTYPE"
#define APPLNAME			"APPLNAME"
#define	PUTDATE				"PUTDATE"
#define	PUTTIME				"PUTTIME"
#define APPLORIGIN			"APPLORIGIN"
#define	SEQNO				"SEQNO"
#define OFFSET				"OFFSET"
#define MSGFLAGS			"MSGFLAGS"
#define ORIGLENGTH			"ORIGLENGTH"
/* MQMD flags */
#define INGROUP				"INGROUP"
#define LASTGROUP			"LASTGROUP"
#define GETBYCORRELID		"GETBYCORRELID"
#define SAVEMQMD			"SAVEMQMD"
#define INDIVFILES			"INDIVFILES"
/* fields related to RFH headers */
#define RFH					"RFH"
#define RFH_DOMAIN			"RFH_DOMAIN"
#define RFH_MSG_SET			"RFH_MSG_SET"
#define RFH_MSG_TYPE		"RFH_MSG_TYPE"
#define	RFH_MSG_FMT			"RFH_MSG_FMT"
#define	RFH_APP_GROUP		"RFH_APP_GROUP"
#define RFH_FORMAT			"RFH_FORMAT"
#define RFH_CCSID			"RFH_CCSID"
#define	RFH_ENCODING		"RFH_ENCODING"
#define RFH_NAME_CCSID		"RFH_NAME_CCSID"
#define RFH_RESET_MCD		"RESET_MCD"
#define RFH_RESET_JMS		"RESET_JMS"
#define RFH_RESET_PSC		"RESET_PSC"
#define RFH_RESET_PSCR		"RESET_PSCR"
#define RFH_RESET_USR		"RESET_USR"
/* fields used when publishing messages */
#define PUBLEVEL			"PUBLEVEL"
#define RETAINED			"RETAINED"
#define LOCAL				"LOCAL"
#define NOTOWN				"NOTOWN"
#define SUPPRESSREPLY		"SUPPRESSREPLY"
#define WARNNOSUB			"WARNNOSUB"
#define SUBLEVEL			"SUBLEVEL"
/* fields used by MQReply */
#define RESENDRFHMCD		"RESENDRFHMCD"
#define RESENDRFHJMS		"RESENDRFHJMS"
#define RESENDRFHPSC		"RESENDRFHPSC"
#define RESENDRFHPSCR		"RESENDRFHPSCR"
#define RESENDRFHUSR		"RESENDRFHUSR"
#define RESENDRFH			"RESENDRFH"
#define USEINPUTASREPLY		"USEINPUTASREPLY"

#define FOUND_NONE			0
#define FOUND_USR			1
#define FOUND_JMS			2
#define FOUND_PSC			3
#define FOUND_PSCR			4
#define FOUND_MCD			5

void getRFHUsrTimeStamp(char * msg, int msgLen, MY_TIME_T * startTime)

{
#ifdef WIN32
	/* initialize the start time to 0, in case one is not found */
	(*startTime) = 0;
#else
	/* initialize the start time to 0, in case one is not found */
	startTime->tv_sec = 0;
	startTime->tv_usec = 0;
#endif

	/* check if the message is long enough */
	if (msgLen > sizeof(LATENCYHEADER) + MQRFH_STRUC_LENGTH_FIXED_2 + 4)
	{
		/* check for an RFH2 header at the beginning of the data */
		if (memcmp(msg, MQRFH_STRUC_ID, 4) == 0)
		{
			/* check for a usr folder */
			if (memcmp(msg + MQRFH_STRUC_LENGTH_FIXED_2 + 4, LATENCYHEADER, 9) == 0)
			{
				/* Looks like the right format, so convert back to binary */
				HexToAscii(msg + MQRFH_STRUC_LENGTH_FIXED_2 + 13, 16, (char *)startTime);
			}
		}
	}
}

int getArgPointer(int argc,
				  char *argv[],
				  int index,
				  char ** parmData,
				  const char * parmDesc,
				  PUTPARMS *parms)

{
	if (strlen(argv[index]) > 2)
	{
		/* point to the argument data */
		(*parmData) = argv[index] + 2;
	}
	else
	{
		index++;

		if (index < argc)
		{
			/* point to the argument data */
			(*parmData) = argv[index];
		}
		else
		{
			/* issue an error message */
			printf("*****%s parameter is missing value\n", parmDesc);
			
			/* Set the return code to indicate the error */
			parms->err = 95;

			/*point to something valid */
			(*parmData) = argv[index - 1] + 2;
		}
	}


	return index;
}

int processFileArg(int argc,
				   char *argv[], 
				   int index, 
				   size_t * fileSize, 
				   char **fileData,
				   const char * parmDesc,
				   PUTPARMS * parms)

{
	FILE	*inputFile;
	char	*fileName;

	index = getArgPointer(argc, argv, index, &fileName, parmDesc, parms);

	if (index < argc)
	{
		/* process the file */
		/* look for a file name that contains the data */
		inputFile = fopen(fileName, "rb");
		if (NULL == inputFile)
		{
			/* error opening file */
			printf("***** unable to open file %s\n", argv[5]);
		}
		else
		{
			/* read the PAN file data */
			/* get the length of the data */
			fseek(inputFile, 0L, SEEK_END);
			(*fileSize) = ftell(inputFile);

			/* read the data from the beginning of the file */
			fseek(inputFile, 0L, SEEK_SET);
			(*fileData) = (char *)malloc((*fileSize) + 1);

			if ((*fileSize) > 0)
			{
				fread((*fileData), 1, (*fileSize), inputFile);
			}

			(*fileData)[(*fileSize)] = 0;

			/* close the file */
			fclose(inputFile);
		}
	}

	return index;
}

int processNumArg(int argc,
				  char *argv[], 
				  int index, 
				  int * value, 
				  const char * parmDesc,
				  PUTPARMS *parms)

{
	char *argData;

	index = getArgPointer(argc, argv, index, &argData, parmDesc, parms);

	if (index < argc)
	{
		/* convert to an integer */
		(*value) = atoi(argData);
	}

	return index;
}

int processNumArg64(int argc,
					char *argv[], 
					int index, 
					int64_t * value, 
					const char * parmDesc,
					PUTPARMS *parms)

{
	char *argData;

	index = getArgPointer(argc, argv, index, &argData, parmDesc, parms);

	if (index < argc)
	{
		/* convert to an integer */
		(*value) = my_ato64(argData);
	}

	return index;
}

void captureString(char * value, char * arg, const size_t maxlen, PUTPARMS *parms)

{
	if (strlen(arg) < maxlen)
	{
		strcpy(value, arg);
	}
	else
	{
		/* issue an error message */
		printf("*****Value of constant exceeds maximum length\n");

		/* Set the return code based on the message number */
		parms->err = 94;
	}
}

int processIndArg(int argc, 
				  char *argv[], 
				  int index, 
				  char * parm, 
				  size_t parmsize, 
				  const char * parmDesc,
				  PUTPARMS *parms)

{
	char *argData;

	index = getArgPointer(argc, argv, index, &argData, parmDesc, parms);

	if (index < argc)
	{
		/* value is included with parameter */
		captureString(parm, argData, parmsize, parms);
	}

	return index;
}

void processArgs(int argc, char **argv, PUTPARMS *parms)

{
	int		i;
	int		foundit;
	char	ch;

	i = 1;
	while ((i < argc) && (0 == parms->err))
	{
		foundit = 0;
		if ('-' == argv[i][0])
		{
			ch = toupper(argv[i][1]);

			/* check for option */
			if ((0 == foundit) && ('Q' == ch))
			{
				foundit = 1;

				i = processIndArg(argc, 
								  argv, 
								  i,
								  (char *)&(parms->saveQname), 
								  sizeof(parms->saveQname), 
								  "queue name (-q)",
								  parms);
			}

			/* check for option */
			if ((0 == foundit) && ('M' == ch))
			{
				foundit = 1;

				i = processIndArg(argc, 
								  argv, 
								  i,
								  (char *)&(parms->saveQMname), 
								  sizeof(parms->saveQMname), 
								  "queue manager name (-m)",
								  parms);
			}

			/* check for option */
			if ((0 == foundit) && ('F' == ch))
			{
				foundit = 1;

				i = processIndArg(argc, 
								  argv, 
								  i,
								  (char *)&(parms->parmFilename), 
								  sizeof(parms->parmFilename), 
								  "name of parameters file (-f)",
								  parms);
			}

			/* check for option */
			if ((0 == foundit) && ('O' == ch))
			{
				foundit = 1;

				i = processIndArg(argc, 
								  argv, 
								  i,
								  (char *)&(parms->outputFilename), 
								  sizeof(parms->outputFilename), 
								  "name of output file (-o) for captured messages",
								  parms);
			}

			/* check for option */
			if ((0 == foundit) && ('L' == ch))
			{
				foundit = 1;

				i = processIndArg(argc, 
								  argv, 
								  i,
								  (char *)&(parms->logFileName), 
								  sizeof(parms->logFileName), 
								  "name of log file (-l)",
								  parms);
			}

			/* check for option */
			if ((0 == foundit) && ('B' == ch))
			{
				foundit = 1;

				i = processNumArg(argc,
								  argv, 
								  i,
								  &(parms->saveBatchSize), 
								  "Messages within unit of work (-b)",
								  parms);

				if ((parms->saveBatchSize < 1) || (parms->saveBatchSize > MAX_BATCH_ALLOW))
				{
					printf("Invalid batch size (%d)\n", parms->saveBatchSize);
					parms->saveBatchSize = 0;
				}
				else
				{
					printf("Batch size will be set to %d\n", parms->saveBatchSize);
				}
			}

			/* check for option */
			if ((0 == foundit) && ('T' == ch))
			{
				foundit = 1;

				i = processNumArg(argc,
								  argv, 
								  i,
								  &(parms->saveThinkTime), 
								  "Think time in milliseconds (-t)",
								  parms);

				if ((parms->saveThinkTime < 1) || (parms->saveThinkTime > MAX_THINK))
				{
					printf("Invalid think time (%d)\n", parms->saveThinkTime);
					parms->saveThinkTime = 0;
				}
				else
				{
					printf("Think time will be set to %d\n", parms->saveThinkTime);
				}
			}

			/* check for option */
			if ((0 == foundit) && ('C' == ch))
			{
				foundit = 1;

				i = processNumArg64(argc,
									argv, 
									i,
									&(parms->saveTotcount), 
									"Message count (-c)",
									parms);
			}

			/* check for verbose option */
			if ((0 == foundit) && ('V' == ch))
			{
				foundit = 1;
				parms->verbose = 1;
			}

			/* check for purge queue option */
			if ((0 == foundit) && ('P' == ch))
			{
				foundit = 1;
				parms->purgeQ = 1;
				parms->drainQ = 1;
			}

			/* check for logical order option */
			if ((0 == foundit) && ('G' == ch))
			{
				foundit = 1;
				parms->logicalOrder = 1;
			}

			/* check for silent option */
			if ((0 == foundit) && ('S' == ch))
			{
				foundit = 1;
				parms->silent = 1;
			}
		}

		/* did we recognize the parameter? */
		if (0 == foundit)
		{
			/* issue an error message */
			printf("*****Unrecognized command line parameter found %c%c\n", argv[i][0], argv[i][1]);

			/* Set the return code based on the message number */
			parms->err = 93;
		}

		i++;
	}
}

int processFirstUsrLine(char * ptr, PUTPARMS * parms, int readFiles)

{
	int		foundit=FOUND_NONE;

	if (memcmp(ptr, RFH2_USR_BEG, 5) == 0)
	{
		foundit = FOUND_USR;

		if (READDATAFILES == readFiles)
		{
			/* get rid of any previous values */
			resetUSR(parms);

			/* allocate a new area to store the usr folder and append the data */
			appendUSR(ptr, parms);
		}
	}
	else if (memcmp(ptr, RFH2_JMS_BEGIN, 5) == 0)
	{
		foundit = FOUND_JMS;

		if (READDATAFILES == readFiles)
		{
			/* get rid of any previous values */
			resetJMS(parms);

			/* allocate a new area to store the usr folder */
			appendJMS(ptr, parms);
		}
	}
	else if (memcmp(ptr, RFH2_PSC_BEGIN, 5) == 0)
	{
		foundit = FOUND_PSC;

		if (READDATAFILES == readFiles)
		{
			/* get rid of any previous values */
			resetPSC(parms);

			/* allocate a new area to store the usr folder */
			appendPSC(ptr, parms);
		}
	}
	else if (memcmp(ptr, RFH2_PSCR_BEGIN, 6) == 0)
	{
		foundit = FOUND_PSCR;

		if (READDATAFILES == readFiles)
		{
			/* get rid of any previous values */
			resetPSCR(parms);

			/* allocate a new area to store the usr folder */
			appendPSCR(ptr, parms);
		}
	}
	else if (memcmp(ptr, RFH2_MCD_BEGIN, 5) == 0)
	{
		foundit = FOUND_MCD;

		if (READDATAFILES == readFiles)
		{
			/* get rid of any previous values */
			resetMCD(parms);

			/* allocate a new area to store the usr folder */
			appendMCD(ptr, parms);
		}
	}
	else
	{
		printf("*****Error - unrecognized RFH2 Subfolder value %s\n", ptr);
	}

	return foundit;
}

int processUsrLine(char * ptr, int foundit, PUTPARMS * parms, int readFiles)

{
	switch (foundit)
	{
	case FOUND_USR:
		{
			if (memcmp(ptr, RFH2_USR_END, 6) == 0)
			{
				foundit = FOUND_NONE;
			}

			if (READDATAFILES == readFiles)
			{
				appendUSR(ptr, parms);
			}

			break;
		}
	case FOUND_JMS:
		{
			if (memcmp(ptr, RFH2_JMS_END, 6) == 0)
			{
				foundit = FOUND_NONE;
			}

			if (READDATAFILES == readFiles)
			{
				appendJMS(ptr, parms);
			}

			break;
		}
	case FOUND_PSC:
		{
			if (memcmp(ptr, RFH2_PSC_END, 6) == 0)
			{
				foundit = FOUND_NONE;
			}

			if (READDATAFILES == readFiles)
			{
				appendPSC(ptr, parms);
			}

			break;
		}
	case FOUND_PSCR:
		{
			if (memcmp(ptr, RFH2_PSCR_END, 7) == 0)
			{
				foundit = FOUND_NONE;
			}

			if (READDATAFILES == readFiles)
			{
				appendPSCR(ptr, parms);
			}

			break;
		}
	case FOUND_MCD:
		{
			if (memcmp(ptr, RFH2_MCD_END, 6) == 0)
			{
				foundit = FOUND_NONE;
			}

			if (READDATAFILES == readFiles)
			{
				appendMCD(ptr, parms);
			}

			break;
		}
	}

	return foundit;
}

/**************************************************************/
/*                                                            */
/* Process a line in the parameters file.                     */
/*                                                            */
/**************************************************************/

int evaluateParm(char * ptr, char * value, PUTPARMS *parms)

{
	int			foundit=0;
	int			foundPSC=0;
	int			foundJMS=0;
	int			foundMCD=0;
	int			dropMCD=0;
	int			dropJMS=0;
	int			dropPSC=0;
	int			dropPSCR=0;
	int			dropUSR=0;
	int			mType;
	size_t		len;
	char		*valueptr;
	char		tempValue[32];

	/* remove any quotes from the value */
	valueptr = removeQuotes(value);
	len = strlen(valueptr);

	tempValue[0] = 0;
	if (len < sizeof(tempValue))
	{
		/* it will fit in the temporary area */
		strcpy(tempValue, valueptr);

		/* translate to upper case */
		strupper(tempValue);
	}

#ifdef MQCLIENT
	foundit = checkCharParm(ptr, QMGR, (char *)&(parms->qmname), valueptr, NULL, foundit, sizeof(parms->qmname));
#else
	foundit = checkCharParm(ptr, QMGR, (char *)&(parms->qmname), valueptr, NULL, foundit, MQ_Q_MGR_NAME_LENGTH+ 1);
#endif
	foundit = checkCharParm(ptr, QNAME, (char *)&(parms->qname), valueptr, NULL, foundit, MQ_Q_NAME_LENGTH + 1);
	foundit = checkCharParm(ptr, REPLYQNAME, (char *)&(parms->replyQname), valueptr, NULL, foundit, MQ_Q_NAME_LENGTH + 1);
	foundit = checkCharParm(ptr, REPLYQMNAME, (char *)&(parms->replyQMname), valueptr, NULL, foundit, MQ_Q_MGR_NAME_LENGTH + 1);
	foundit = checkCharParm(ptr, REMOTEQM, (char *)&(parms->remoteQM), valueptr, NULL, foundit, MQ_Q_MGR_NAME_LENGTH + 1);
	foundit = checkCharParm(ptr, TOPIC, (char *)&(parms->topicStr), valueptr, NULL, foundit, MQ_TOPIC_STR_LENGTH + 1);
	foundit = checkCharParm(ptr, TOPICNAME, (char *)&(parms->topicName), valueptr, NULL, foundit, MQ_TOPIC_NAME_LENGTH + 1);
	foundit = checkI64Parm(ptr, MSGCOUNT, &(parms->totcount), valueptr, NULL, foundit);
	foundit = checkIntParm(ptr, TUNE, &(parms->tune), valueptr, NULL, foundit);
	foundit = checkIntParm(ptr, REPORT, &(parms->report), valueptr, NULL, foundit);
	foundit = checkIntParm(ptr, MAXTIME, &(parms->maxtime), valueptr, NULL, foundit);
	foundit = checkIntParm(ptr, MAXREPLYTIME, &(parms->maxWaitTime), valueptr, NULL, foundit);
	foundit = checkIntParm(ptr, WAITTIME, &(parms->waitTime), valueptr, NULL, foundit);
	foundit = checkCharParm(ptr, REPLYQM, (parms->replyQM), valueptr, NULL, foundit, MQ_Q_MGR_NAME_LENGTH);
	foundit = checkCharParm(ptr, REPLYQ, (parms->replyQ), valueptr, NULL, foundit, MQ_Q_NAME_LENGTH);
	foundit = checkCharParm(ptr, USERID, (parms->userId), valueptr, NULL, foundit, MQ_USER_ID_LENGTH);
	foundit = checkCharParm(ptr, OUTPUTFILENAME, (parms->outputFilename), valueptr, NULL, foundit, sizeof(parms->outputFilename));
	foundit = checkCharParm(ptr, REPLYFILENAME, (parms->replyFilename), valueptr, NULL, foundit, sizeof(parms->replyFilename));
	foundit = checkYNParm(ptr, WRITEONCE, &(parms->writeOnce), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, IGNOREMQMD, &(parms->ignoreMQMD), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, NEWMSGID, &(parms->newMsgId), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, GETBYCORRELID, &(parms->GetByCorrelId), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, SETTIMESTAMP, &(parms->setTimeStamp), valueptr, NULL, foundit);
	foundit = checkIntParm(ptr, TIMESTAMPOFFSET, &(parms->timeStampOffset), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, TIMESTAMPACCTTOKEN, &(parms->timeStampInAccountingToken), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, TIMESTAMPGROUPID, &(parms->timeStampInGroupId), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, TIMESTAMPCORRELID, &(parms->timeStampInCorrelId), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, TIMESTAMPUSERPROP, &(parms->timeStampUserProp), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, DRAINQ, &(parms->drainQ), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, SILENT, &(parms->silent), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, LOGICALORDER, &(parms->logicalOrder), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, SAVEMQMD, &(parms->saveMQMD), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, READONLY, &(parms->readOnly), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, INDIVFILES, &(parms->indivFiles), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, STRIPRFH, &(parms->striprfh), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, APPENDFILE, &(parms->appendFile), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, ADDTIMESTAMP, &(parms->addTimeStamp), valueptr, NULL, foundit);
	foundit = checkCharParm(ptr, RFH_DOMAIN, (char *)&(parms->rfhdomain), valueptr, &foundMCD, foundit, sizeof(parms->rfhdomain) - 1);
	foundit = checkCharParm(ptr, RFH_MSG_SET, (char *)&(parms->rfhset), valueptr, &foundMCD, foundit, sizeof(parms->rfhset) - 1);
	foundit = checkCharParm(ptr, RFH_MSG_TYPE, (char *)&(parms->rfhtype), valueptr, &foundMCD, foundit, sizeof(parms->rfhtype) - 1);
	foundit = checkCharParm(ptr, RFH_MSG_FMT, (char *)&(parms->rfhfmt), valueptr, &foundMCD, foundit, sizeof(parms->rfhfmt) - 1);
	foundit = checkCharParm(ptr, RFH_APP_GROUP, (char *)&(parms->rfhappgroup), valueptr, NULL, foundit, sizeof(parms->rfhappgroup) - 1);
	foundit = checkCharParm(ptr, RFH_FORMAT, (char *)&(parms->rfhformat), valueptr, NULL, foundit, sizeof(parms->rfhformat) - 1);
	foundit = checkYNParm(ptr, RFH_PSC_LOCAL, &(parms->rfh_psc_local), valueptr, &foundPSC, foundit);
	foundit = checkYNParm(ptr, RFH_PSC_NEWONLY, &(parms->rfh_psc_newonly), valueptr, &foundPSC, foundit);
	foundit = checkYNParm(ptr, RFH_PSC_OTHERONLY, &(parms->rfh_psc_otheronly), valueptr, &foundPSC, foundit);
	foundit = checkYNParm(ptr, RFH_PSC_ONDEMAND, &(parms->rfh_psc_ondemand), valueptr, &foundPSC, foundit);
	foundit = checkYNParm(ptr, RFH_PSC_RETAINPUB, &(parms->rfh_psc_retainpub), valueptr, &foundPSC, foundit);
	foundit = checkYNParm(ptr, RFH_PSC_ISRETAINPUB, &(parms->rfh_psc_isretainpub), valueptr, &foundPSC, foundit);
	foundit = checkYNParm(ptr, RFH_PSC_CORRELID, &(parms->rfh_psc_correlid), valueptr, &foundPSC, foundit);
	foundit = checkYNParm(ptr, RFH_PSC_DEREGALL, &(parms->rfh_psc_deregall), valueptr, &foundPSC, foundit);
	foundit = checkYNParm(ptr, RFH_PSC_INFRETAIN, &(parms->rfh_psc_infretain), valueptr, &foundPSC, foundit);
	foundit = checkCharParm(ptr, RFH_PSC_TOPIC1, (char *)&(parms->rfh_topic1), valueptr, &foundPSC, foundit, sizeof(parms->rfh_topic1) - 1);
	foundit = checkCharParm(ptr, RFH_PSC_TOPIC2, (char *)&(parms->rfh_topic2), valueptr, &foundPSC, foundit, sizeof(parms->rfh_topic2) - 1);
	foundit = checkCharParm(ptr, RFH_PSC_TOPIC3, (char *)&(parms->rfh_topic3), valueptr, &foundPSC, foundit, sizeof(parms->rfh_topic3) - 1);
	foundit = checkCharParm(ptr, RFH_PSC_TOPIC4, (char *)&(parms->rfh_topic4), valueptr, &foundPSC, foundit, sizeof(parms->rfh_topic4) - 1);
	foundit = checkCharParm(ptr, RFH_PSC_SUBPOINT, (char *)&(parms->rfh_subpoint), valueptr, &foundPSC, foundit, sizeof(parms->rfh_subpoint) - 1);
	foundit = checkCharParm(ptr, RFH_PSC_FILTER, (char *)&(parms->rfh_filter), valueptr, &foundPSC, foundit, sizeof(parms->rfh_filter) - 1);
	foundit = checkCharParm(ptr, RFH_PSC_REPLYQM, (char *)&(parms->rfh_PscReplyQM), valueptr, &foundPSC, foundit, MQ_Q_MGR_NAME_LENGTH);
	foundit = checkCharParm(ptr, RFH_PSC_REPLYQ, (char *)&(parms->rfh_PscReplyQ), valueptr, &foundPSC, foundit, MQ_Q_NAME_LENGTH);
	foundit = checkCharParm(ptr, RFH_PSC_PUBTIME,(char *)&(parms->rfh_pubtime), valueptr, &foundPSC, foundit, sizeof(parms->rfh_pubtime) - 1);
	foundit = checkIntParm(ptr, RFH_PSC_SEQNO, &(parms->rfh_psc_seqno), valueptr, &foundPSC, foundit);
	foundit = checkCharParm(ptr, RFH_JMS_DEST, (char *)&(parms->rfh_jms_dest), valueptr, &foundJMS, foundit, sizeof(parms->rfh_jms_dest) - 1);
	foundit = checkCharParm(ptr, RFH_JMS_REPLY, (char *)&(parms->rfh_jms_reply), valueptr, &foundJMS, foundit, sizeof(parms->rfh_jms_reply) - 1);
	foundit = checkCharParm(ptr, RFH_JMS_CORRELID, (char *)&(parms->rfh_jms_correlid), valueptr, &foundJMS, foundit, sizeof(parms->rfh_jms_correlid) - 1);
	foundit = checkCharParm(ptr, RFH_JMS_GROUPID, (char *)&(parms->rfh_jms_groupid), valueptr, &foundJMS, foundit, sizeof(parms->rfh_jms_groupid) - 1);
	foundit = checkCharParm(ptr, RFH_JMS_EXPIRE, (char *)&(parms->rfh_jms_expire), valueptr, &foundJMS, foundit, sizeof(parms->rfh_jms_expire) - 1);
	foundit = checkCharParm(ptr, RFH_JMS_PRIORITY, (char *)&(parms->rfh_jms_priority), valueptr, &foundJMS, foundit, sizeof(parms->rfh_jms_priority) - 1);
	foundit = checkCharParm(ptr, RFH_JMS_DELMODE, (char *)&(parms->rfh_jms_delmode), valueptr, &foundJMS, foundit, sizeof(parms->rfh_jms_delmode) - 1);
	foundit = checkCharParm(ptr, RFH_JMS_SEQ, (char *)&(parms->rfh_jms_seq), valueptr, &foundJMS, foundit, sizeof(parms->rfh_jms_seq) - 1);
	foundit = checkYNParm(ptr, RFH_RESET_MCD, &dropMCD, valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, RFH_RESET_JMS, &dropJMS, valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, RFH_RESET_PSC, &dropPSC, valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, RFH_RESET_PSCR, &dropPSCR, valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, RFH_RESET_USR, &dropUSR, valueptr, NULL, foundit);
	foundit = checkIntParm(ptr, PUBLEVEL, &(parms->pubLevel), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, RETAINED, &(parms->retained), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, LOCAL, &(parms->local), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, NOTOWN, &(parms->notOwn), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, SUPPRESSREPLY, &(parms->suppressReply), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, WARNNOSUB, &(parms->warnNoSub), valueptr, NULL, foundit);
	foundit = checkYNParm(ptr, USEINPUTASREPLY, &(parms->useInputAsReply), valueptr, NULL, foundit);

	if (strcmp(ptr, MSGID) == 0)
	{
		foundit = 1;
		parms->msgidSet = 1;
		editCharParm(MSGID, parms->msgid, valueptr, sizeof(parms->msgid));
	}

	if (strcmp(ptr, MSGIDX) == 0)
	{
		foundit = 1;
		parms->msgidSet = 1;
		editHexParm(MSGIDX, parms->msgid, valueptr, sizeof(parms->msgid));
	}

	if (strcmp(ptr, CORRELID) == 0)
	{
		foundit = 1;
		parms->correlidSet = 1;
		editCharParm(CORRELID, parms->correlid, valueptr, sizeof(parms->correlid));
	}

	if (strcmp(ptr, CORRELIDX) == 0)
	{
		foundit = 1;
		parms->correlidSet = 1;
		editHexParm(CORRELIDX, parms->correlid, valueptr, sizeof(parms->correlid));
	}

	if (strcmp(ptr, GROUPID) == 0)
	{
		foundit = 1;
		parms->groupidSet = 1;
		editCharParm(GROUPID, parms->groupid, valueptr, sizeof(parms->groupid));
	}

	if (strcmp(ptr, GROUPIDX) == 0)
	{
		foundit = 1;
		parms->groupidSet = 1;
		editHexParm(GROUPIDX, parms->groupid, valueptr, sizeof(parms->groupid));
	}

	if (strcmp(ptr, ACCOUNTINGTOKEN) == 0)
	{
		foundit = 1;
		parms->acctTokenSet = 1;
		editCharParm(ACCOUNTINGTOKEN, parms->accountingToken, valueptr, sizeof(parms->accountingToken));
	}

	if (strcmp(ptr, ACCOUNTINGTOKENX) == 0)
	{
		foundit = 1;
		parms->acctTokenSet = 1;
		editHexParm(ACCOUNTINGTOKENX, parms->accountingToken, valueptr, sizeof(parms->accountingToken));
	}

	if (strcmp(ptr, DELIMITER) == 0)
	{
		foundit = 1;

		if (len > sizeof(parms->delimiter))
		{
			len = sizeof(parms->delimiter);
			valueptr[len] = 0;

			/* delimiter is too long */
			printf("***** delimiter too long - value was truncated to %d\n", len);
		}

		strcpy(parms->delimiter, valueptr);
		parms->iDelimiterLen = iStrLen(valueptr);
		parms->delimiterLen = len;
	}

	if (strcmp(ptr, DELIMITERX) == 0)
	{
		foundit = 1;

		if (len > (2 * sizeof(parms->delimiter)))
		{
			/* limit the delimiter to the maximum allowed to prevent overwriting storage */
			len = 2 * sizeof(parms->delimiter);

			/* terminate the input string */
			valueptr[len] = 0;

			/* get the new length as a 32-bit integer */
			parms->iDelimiterLen = (unsigned int)(iStrLen(valueptr) >> 1);

			/* delimiter is too long - issue an error message */
			printf("***** delimiter too long - value was truncated to length %d\n", parms->iDelimiterLen);
		}

		/* make sure the length is an even number of bytes */
		if ((len & 1) > 0)
		{
			/* odd number of bytes, issue error and return */
			printf("***** delimiterx value has odd number of bytes (%d) for hex value - %s\n", 
					len, value);
			printf("***** last character was ignored\n");

			/* truncate the last character */
			len--;
		}

		parms->iDelimiterLen = (iStrLen(valueptr) >> 1);
		parms->delimiterLen = len >> 1;
		parms->delimiterIsHex = 1;							/* delimiter was specified in hex */
		HexToAscii(valueptr, len, parms->delimiter);
	}

	if (strcmp(ptr, PERSIST) == 0)
	{
		foundit = 1;
		parms->persist = atoi(valueptr);
		if ((parms->persist < 0) || (parms->persist > 2))
		{
			printf("***** invalid value for persistence %d *****\n", parms->persist);
			parms->persist = 0;
		}
	}

	if (strcmp(ptr, ENCODING) == 0)
	{
		foundit = 1;
		parms->encoding = atoi(valueptr);
		if (parms->encoding < 0)
		{
			printf("***** invalid value for encoding %d *****\n", parms->encoding);
			parms->encoding = MQENC_NATIVE;
		}
	}

	if ((strcmp(ptr, CODEPAGE) == 0) || (strcmp(ptr, CCSID) == 0))
	{
		foundit = 1;
		parms->codepage = atoi(valueptr);
		if (parms->codepage < 0)
		{
			printf("***** invalid value for codepage %d *****\n", parms->codepage);
			parms->codepage = MQCCSI_Q_MGR;
		}
	}

	if (strcmp(ptr, EXPIRY) == 0)
	{
		foundit = 1;
		parms->expiry = atoi(valueptr);
		if (parms->expiry < MQEI_UNLIMITED)
		{
			printf("***** invalid value for expiration time %d *****\n", parms->expiry);
			parms->expiry = MQEI_UNLIMITED;
		}
	}

	if (strcmp(ptr, FEEDBACK) == 0)
	{
		foundit = 1;
		parms->feedback = atoi(valueptr);
	}

	if (strcmp(ptr, MSGTYPE) == 0)
	{
		foundit = 1;
		mType = atoi(valueptr);
		if ((mType != MQMT_SYSTEM_FIRST) &&
			(mType != MQMT_REQUEST) &&
			(mType != MQMT_REPLY) &&
			(mType != MQMT_DATAGRAM) &&
			(mType != MQMT_REPORT) &&
			(mType != MQMT_MQE_FIELDS_FROM_MQE) &&
			(mType != MQMT_MQE_FIELDS) &&
			((mType < MQMT_APPL_FIRST) || (mType > MQMT_APPL_LAST)))
		{
			printf("***** invalid value for message type %d *****\n", mType);
			parms->msgtype = 0;
		}
		else
		{
			/* valid type */
			parms->msgtype = mType;
		}
	}

	if (strcmp(ptr, PRIORITY) == 0)
	{
		foundit = 1;
		parms->priority = atoi(valueptr);
		if (parms->priority < MQPRI_PRIORITY_AS_Q_DEF)
		{
			printf("***** invalid value for message priority %d *****\n", parms->priority);
			parms->priority = MQPRI_PRIORITY_AS_Q_DEF;
		}
	}

	if (strcmp(ptr, QDEPTH) == 0)
	{
		foundit = 1;
		parms->qdepth = atoi(valueptr);
		if (parms->qdepth < 1) 
		{
			printf("***** invalid value for queue depth %d *****\n", parms->qdepth);
			parms->qdepth = INIT_COUNT;
		}
	}

	if (strcmp(ptr, QMAX) == 0)
	{
		foundit = 1;
		parms->qmax = atoi(valueptr);
		if (parms->qmax <= 0) 
		{
			printf("***** invalid value for maximum queue depth %d *****\n", parms->qmax);
			parms->qmax = 0;
		}

		if (parms->qmax < parms->qdepth) 
		{
			printf("***** maximum queue depth %d cannot be less than minimum qdepth %d *****\n", parms->qmax, parms->qdepth);
			parms->qmax = parms->qdepth;
		}
	}

	if (strcmp(ptr, SLEEPTIME) == 0)
	{
		foundit = 1;
		parms->sleeptime = atoi(valueptr);
		if ((parms->sleeptime < MIN_SLEEP) || (parms->sleeptime > MAX_SLEEP))
		{
			printf("***** invalid value for sleep time %d *****\n", parms->sleeptime);
			parms->sleeptime = DEF_SLEEP;
		}
	}

	if (strcmp(ptr, THINKTIME) == 0)
	{
		foundit = 1;
		parms->thinkTime = atoi(valueptr);
		if ((parms->thinkTime < MIN_THINK) || (parms->thinkTime > MAX_THINK))
		{
			printf("***** invalid value for think time %d *****\n", parms->thinkTime);
			parms->thinkTime = 0;
		}
	}

	if (strcmp(ptr, BATCHSIZE) == 0)
	{
		foundit = 1;
		parms->batchSize = atoi(valueptr);
		if (parms->batchSize < 1) 
		{
			printf("***** invalid value for batch size %d *****\n", parms->batchSize);

			/* reset value to default */
			parms->batchSize = DEF_SYNC;
		}
	}

	if (strcmp(ptr, REPORTEVERY) == 0)
	{
		foundit = 1;
		parms->reportEvery = atoi(valueptr);
		if (parms->reportEvery < 0) 
		{
			printf("***** invalid value for reportEvery %d *****\n", parms->reportEvery);
			parms->reportEvery = 0;
		}
	}

	if (strcmp(ptr, SUBLEVEL) == 0)
	{
		foundit = 1;
		parms->subLevel = atoi(valueptr);

		/* check that the subscription level is valid */
		if ((parms->subLevel < 0) || (parms->subLevel > 9))
		{
			printf("***** invalid subscription level %d *****\n", parms->subLevel);
			parms->subLevel = -1;
		}
	}

	if (strcmp(ptr, REPORTINTERVAL) == 0)
	{
		foundit = 1;
		parms->reportInterval = atoi(valueptr);
		if (parms->reportInterval < 0) 
		{
			printf("***** invalid value for reportInterval %d *****\n", parms->reportInterval);
			parms->reportInterval = 1;
		}
	}

	if (strcmp(ptr, REPORTEVERYSECOND) == 0)
	{
		foundit = 1;
		switch(toupper(valueptr[0]))
		{
		case '1':
		case 'Y':
			{
				parms->reportEverySecond = 1;
				break;
			}
		default:
			{
				parms->reportEverySecond = 0;
				break;
			}
		}
	}

	if (strcmp(ptr, ENCODING) == 0)
	{
		foundit = 1;

		/* set the ccsid value */
		parms->encoding = atoi(valueptr);
	}

	if (strcmp(ptr, FORMAT) == 0)
	{
		foundit = 1;
		parms->formatSet = 1;
		editCharParm(FORMAT, parms->msgformat, valueptr, sizeof(parms->msgformat));
	}

	if (strcmp(ptr, INGROUP) == 0)
	{
		foundit = 1;
		switch(toupper(valueptr[0]))
		{
		case '1':
		case 'Y':
			{
				parms->inGroup = 1;
				break;
			}
		default:
			{
				parms->inGroup = 0;
				break;
			}
		}
	}

	if (strcmp(ptr, LASTGROUP) == 0)
	{
		foundit = 1;
		switch(toupper(valueptr[0]))
		{
		case '1':
		case 'Y':
			{
				parms->lastGroup = 1;
				break;
			}
		default:
			{
				parms->lastGroup = 0;
				break;
			}
		}
	}

	if (strcmp(ptr, FILEASGROUP) == 0)
	{
		foundit = 1;
		switch(toupper(valueptr[0]))
		{
		case '1':
		case 'Y':
			{
				parms->fileAsGroup = 1;
				break;
			}
		default:
			{
				parms->fileAsGroup = 0;
				break;
			}
		}
	}

	if (strcmp(ptr, RESENDRFHMCD) == 0)
	{
		foundit = 1;

		if (('Y' == valueptr[0]) || ('y' == valueptr[0]) || ('1' == valueptr[0]))
		{
			parms->resendRFHmcd = 1;
			parms->resendRFH = 1;
		}
		else
		{
			parms->resendRFHmcd = 0;
		}
	}

	if (strcmp(ptr, RESENDRFHJMS) == 0)
	{
		foundit = 1;

		if (('Y' == valueptr[0]) || ('y' == valueptr[0]) || ('1' == valueptr[0]))
		{
			parms->resendRFHjms = 1;
			parms->resendRFH = 1;
		}
		else
		{
			parms->resendRFHjms = 0;
		}
	}

	if (strcmp(ptr, RESENDRFHPSC) == 0)
	{
		foundit = 1;

		if (('Y' == valueptr[0]) || ('y' == valueptr[0]) || ('1' == valueptr[0]))
		{
			parms->resendRFHpsc = 1;
			parms->resendRFH = 1;
		}
		else
		{
			parms->resendRFHpsc = 0;
		}
	}

	if (strcmp(ptr, RESENDRFHPSCR) == 0)
	{
		foundit = 1;

		if (('Y' == valueptr[0]) || ('y' == valueptr[0]) || ('1' == valueptr[0]))
		{
			parms->resendRFHpscr = 1;
			parms->resendRFH = 1;
		}
		else
		{
			parms->resendRFHpscr = 0;
		}
	}

	if (strcmp(ptr, RESENDRFHUSR) == 0)
	{
		foundit = 1;

		if (('Y' == valueptr[0]) || ('y' == valueptr[0]) || ('1' == valueptr[0]))
		{
			parms->resendRFHusr = 1;
			parms->resendRFH = 1;
		}
		else
		{
			parms->resendRFHusr = 0;
		}
	}

	if (strcmp(ptr, RESENDRFH) == 0)
	{
		foundit = 1;

		if (('Y' == valueptr[0]) || ('y' == valueptr[0]) || ('1' == valueptr[0]))
		{
			parms->resendRFH = 1;
		}
		else
		{
			parms->resendRFH = 0;
		}
	}

	if (strcmp(ptr, RFH) == 0)
	{
		foundit = 1;
		switch (toupper(valueptr[0]))
		{
		case '1':
			{
				parms->rfh = RFH_V1;
				break;
			}
		case '2':
			{
				parms->rfh = RFH_V2;
				break;
			}
		case 'A':
			{
				parms->rfh = RFH_AUTO;
				break;
			}
		case 'N':
			{
				parms->rfh = RFH_NO;
				break;
			}
		case 'V':
			{
				if (valueptr[1] == '1')
				{
					parms->rfh = RFH_V1;
				}
				else
				{
					if (valueptr[1] == '2')
					{
						parms->rfh = RFH_V2;
					}
					else
					{
						printf("***** invalid value for rfh parameter %s *****\n", valueptr);
					}
				}

				break;
			}
		case 'X':
			{
				parms->rfh = RFH_XML;
				break;
			}
		default:
			{
				printf("***** invalid value for rfh parameter %s *****\n", valueptr);
				break;
			}
		}
	}

	if (strcmp(ptr, RFH_CCSID) == 0)
	{
		foundit = 1;
		parms->rfhccsid = atoi(valueptr);
		if (parms->rfhccsid <= 0) 
		{
			printf("***** invalid value for rfh ccsid %d *****\n", parms->rfhccsid);
			parms->rfhccsid = MQCCSI_Q_MGR;
		}
	}

	if (strcmp(ptr, RFH_ENCODING) == 0)
	{
		foundit = 1;
		parms->rfhencoding = atoi(valueptr);
		if (parms->rfhencoding <= 0) 
		{
			printf("***** invalid value for rfh encoding value %d *****\n", parms->rfhencoding);
			parms->rfhencoding = MQENC_NATIVE;
		}
	}

	if (strcmp(ptr, RFH_NAME_CCSID) == 0)
	{
		foundit = 1;
		parms->nameValueCCSID = atoi(valueptr);
		if (parms->nameValueCCSID <= 0) 
		{
			printf("***** invalid value for rfh name value ccsid %d *****\n", parms->nameValueCCSID);
			parms->nameValueCCSID = 0;
		}
	}

	if (strcmp(ptr, RFH_PSC_REQTYPE) == 0)
	{
		foundit = 1;
		foundPSC = 1;
		if ((strcmp(tempValue, "SUBSCRIBE") == 0) || 
			(strcmp(tempValue, "SUB") == 0) ||
			(tempValue[0] == '1'))
		{
			parms->rfh_psc_reqtype = RFH_PSC_SUB;
		}
		else if ((strcmp(tempValue, "UNSUBSCRIBE") == 0) || 
				 (strcmp(tempValue, "UNSUB") == 0) ||
				 (tempValue[0] == '2'))
		{
			parms->rfh_psc_reqtype = RFH_PSC_UNSUB;
		}
		else if ((strcmp(tempValue, "PUBLISH") == 0) || 
				 (strcmp(tempValue, "PUB") == 0) ||
				 (tempValue[0] == '3'))
		{
			parms->rfh_psc_reqtype = RFH_PSC_PUB;
		}
		else if ((strcmp(tempValue, "REQPUB") == 0) || 
				 (tempValue[0] == '4'))
		{
			parms->rfh_psc_reqtype = RFH_PSC_REQPUB;
		}
		else if ((strcmp(tempValue, "DELPUB") == 0) || 
				 (tempValue[0] == '5'))
		{
			parms->rfh_psc_reqtype = RFH_PSC_DELPUB;
		}
		else
		{
			printf("***** invalid value for pub/sub request type %s *****\n", valueptr);
			parms->rfh_psc_reqtype = 0;
		}
	}

	if (strcmp(ptr, RFH_JMS_REQTYPE) == 0)
	{
		foundit = 1;
		foundJMS = 1;
		if ((strcmp(tempValue, "TEXT") == 0) || 
			(tempValue[0] == '1'))
		{
			parms->rfh_jms_reqtype = RFH_JMS_TEXT;
		}
		else if ((strcmp(tempValue, "BYTES") == 0) || 
				 (tempValue[0] == '2'))
		{
			parms->rfh_jms_reqtype = RFH_JMS_BYTES;
		}
		else if ((strcmp(tempValue, "STREAM") == 0) || 
				 (tempValue[0] == '3'))
		{
			parms->rfh_jms_reqtype = RFH_JMS_STREAM;
		}
		else if ((strcmp(tempValue, "OBJECT") == 0) || 
				 (tempValue[0] == '4'))
		{
			parms->rfh_jms_reqtype = RFH_JMS_OBJECT;
		}
		else if ((strcmp(tempValue, "MAP") == 0) || 
				 (tempValue[0] == '5'))
		{
			parms->rfh_jms_reqtype = RFH_JMS_MAP;
		}
		else if ((strcmp(tempValue, "NONE") == 0) || 
				 (tempValue[0] == '6'))
		{
			parms->rfh_jms_reqtype = RFH_JMS_NONE;
		}
		else
		{
			printf("***** invalid value for jms request type %s *****\n", valueptr);
			parms->rfh_jms_reqtype = 0;
		}
	}

	if (1 == dropMCD)
	{
		resetMCD(parms);
	}

	if (1 == dropJMS)
	{
		resetJMS(parms);
	}

	if (1 == dropPSC)
	{
		resetPSC(parms);
	}

	if (1 == dropPSCR)
	{
		resetPSCR(parms);
	}

	if (1 == dropUSR)
	{
		resetUSR(parms);
	}

	/* check if we need to rebuild any of the rfh areas */
	if (1 == foundMCD)
	{
		/* throw away the old MCD folder and build a new one */
		rebuildMCD(parms);
	}

	if (1 == foundPSC)
	{
		/* throw away the old PSC folder and build a new one */
		rebuildPSC(parms);
	}

	if (1 == foundJMS)
	{
		/* throw away the old JMS folder and build a new one */
		rebuildJMS(parms);
	}

	return foundit;
}

/**************************************************************/
/*                                                            */
/* Find the parameter and the value.                          */
/*                                                            */
/**************************************************************/

int processParmLine(char * ptr, PUTPARMS *parms)

{
	int		foundit=0;
	int		foundfiles=0;
	char *	valueptr;
	char *	blankptr;

	/* find the equal equal sign */
	valueptr = (char *) strchr(ptr, '=');

	/* did we find one? */
	if (valueptr != NULL)
	{
		/* break the line into name and value parts */
		valueptr[0] = 0;
		valueptr++;

		/* find the beginning of the value */
		valueptr = skipBlanks(valueptr);
	}

	/* strip trailing blanks and translate the name to upper case */
	blankptr = findBlank(ptr);
	blankptr[0] = 0;
	strupper(ptr);

	/* is this a name value pair? */
	if (valueptr != NULL)
	{
		/* check that we have a value */
		if (valueptr[0] > 0)
		{
			foundit = evaluateParm(ptr, valueptr, parms);

			if (foundit == 0)
			{
				printf("***** unrecognized parameter %s, value %s\n", ptr, valueptr);
			}
		}
		else
		{
			printf("***** missing or blank value for parameter %s\n", ptr);
		}
	}
	else
	{
		/* check for the end of the parameters and beginning of */
		/* the list of message data files */
		if (strcmp(ptr, FILELIST) == 0)
		{
			/* remember we are in a file section */
			foundfiles = 1;
			createRFH(parms);
		}

		/* check for message parameters section header */
		if (strcmp(ptr, HEADER) == 0)
		{
			/* remember we are in a header section */
			foundfiles = 0;
		}
	}

	return foundfiles;
}

void initializeParms(PUTPARMS *parms, size_t parmSize)

{
	/* initialize the work areas */
	memset(parms, 0, parmSize);

	/* set defaults for the delimiter value and length */
	strcpy(parms->delimiter, DEFAULT_DELIMITER);
	parms->delimiterLen = sizeof(DEFAULT_DELIMITER) - 1;
	parms->iDelimiterLen = strlen(DEFAULT_DELIMITER);

	/* initialize non-zero parameters */
	parms->codepage = MQCCSI_Q_MGR;
	parms->encoding = MQENC_NATIVE;
	parms->ignoreMQMD=MQMD_NO;
	parms->expiry=MQEI_UNLIMITED;
	parms->priority = MQPRI_PRIORITY_AS_Q_DEF;
	parms->sleeptime = 10;
	parms->rfh=RFH_AUTO;
	parms->rfhccsid=MQCCSI_Q_MGR;
	parms->rfhencoding=MQENC_NATIVE;
	parms->maxWaitTime=5;
	parms->maxtime = TIME_OUT_DEF;
	parms->waitTime = TIME_OUT_DEF;
	parms->maxmsglen = MAX_MESSAGE_LENGTH;
	parms->reportInterval = 1;
	parms->batchSize = DEF_SYNC;
	parms->subLevel = -1;
}

void processOverrides(PUTPARMS *parms)

{
	/* check for overrides */
	if (parms->saveQMname[0] != 0)
	{
		strcpy(parms->qmname, parms->saveQMname);
	}

	/* check for overrides */
	if (parms->saveQname[0] != 0)
	{
		strcpy(parms->qname, parms->saveQname);
	}

	/* check for overrides */
	if (parms->saveTotcount > 0)
	{
		parms->totcount = parms->saveTotcount;
	}

	/* check for overrides */
	if (parms->saveBatchSize > 0)
	{
		parms->batchSize = parms->saveBatchSize;
	}

	/* check if the write once parameter was found */
	if (1 == parms->writeOnce)
	{
		printf("Write once switch set - count set to %d\n", parms->mesgCount);
		parms->totcount = parms->mesgCount;
	}

	/* set the default queue depth if none was specified */
	if (parms->qdepth == 0)
	{
		if (parms->qmax == 0)
		{
			parms->qdepth = INIT_COUNT;
		}
		else
		{
			parms->qdepth = (parms->qmax / 2) + 1;
		}
	}

	/* set the maximum depth if not specified */
	if (parms->qmax == 0)
	{
		/* set the maximum depth to twice the minimum */
		parms->qmax = parms->qdepth + 16;
	}

	/* allow a global override on the command line */
	if (parms->saveThinkTime > 0)
	{
		parms->thinkTime = parms->saveThinkTime;
	}
}

