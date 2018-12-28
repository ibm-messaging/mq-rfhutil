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
/*   MQTEST is a derivative of MQPut2 designed for testing rather   */
/*   than as a performance measurement driver.  It reads each file  */
/*   in order and writes it to the specified queue.  Each file is   */
/*   written once. The parameter file is the same as used by the    */
/*   MQPut2 program.  Certain parameters such as sleep interval are */
/*   ignored.                                                       */
/*                                                                  */
/*   MQTEST has 1 required input parameter                          */
/*                                                                  */
/*      -f name of the parameter input file                         */
/*                                                                  */
/*   The input parameter file contains all values, including the    */
/*   name of the queue and queue manager to write data to, the      */
/*   total number of messages to write, any MQMD parameters and     */
/*   a list of files which contain the message data.                */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef SOLARIS
#include <sys/types.h>
#endif

/* includes for MQI */
#include <cmqc.h>

/* common subroutines include */
#include "int64defs.h"
#include "timesubs.h"
#include "comsubs.h"

/* parameter file processing routines */
#include "parmline.h"
#include "putparms.h"

/* MQ subroutines include */
#include "qsubs.h"
#include "rfhsubs.h"

#ifndef _WIN32
void Sleep(int amount)
{
	usleep(amount*1000);
}
#endif

static char copyright[] = "(C) Copyright IBM Corp, 2001 - 2014";
static char Version[]=\
"@(#)MQTest V3.0 - Test driver tool  - Jim MacNair ";

#ifdef _DEBUG
static char Level[]="mqtest.c V3.0 Debug version ("__DATE__" "__TIME__")";
#else
static char Level[]="mqtest.c V3.0 Release version ("__DATE__" "__TIME__")";
#endif

	MQHCONN			qm=0;			/* queue manager connection handle */
	MQHOBJ			q=0;			/* queue handle used for mqputs    */
	static int		filelist=0;		/* found [filelist] separator      */

	/* variables to keep track of parameter file sections */
	int		foundfiles=0;
	int		foundUsr=0;

	/* global termination switch */
	volatile int	terminate=0;
	volatile int	cancelled=0;

	/* counters and statistics */
	MQCHAR8		puttime;

	/* state information */
	int		groupOpen = 0;
	int		uow=0;

	/* name of the connected qm and open queue */
	char	connectName[MQ_Q_MGR_NAME_LENGTH + 4];
	char	openName[MQ_Q_NAME_LENGTH + 4];
	char	openRemoteName[MQ_Q_NAME_LENGTH + 4];

char * searchForDelimiter(char * msgPtr, const int maxlen, PUTPARMS * parms)

{
	char *	newPtr=NULL;
	int		i=0;

	/* return if there is no delimiter specified */
	if (0 == parms->delimiterLen)
	{
		return NULL;
	}

	while (1)
	{
		while ((i < (maxlen - parms->iDelimiterLen)) && (msgPtr[i] != parms->delimiter[0]))
		{
			i++;
		}

		/* did we run out of message to check? */
		if (i < (maxlen - parms->iDelimiterLen))
		{
			/* we found a delimiter that we need to check */
			/* check if we have more delimiter characters to compare */
			if (1 == parms->delimiterLen)
			{
				/* no, found a delimiter */
				/* set the new address pointer */
				newPtr = msgPtr + i;
				break;
			}
			else
			{
				/* do a full memory compare */
				if (0 == memcmp(msgPtr + i, parms->delimiter, parms->delimiterLen))
				{
					/* we have found a delimiter */
					newPtr = msgPtr + i;
					break;
				}
			}
		}
		else
		{
			break;
		}

		/* go on to the next entry */
		i++;
	}

	return newPtr;
}

void discQM(PUTPARMS * parms)

{
	MQLONG	compcode=0;
	MQLONG	reason=0;

	if (0 == qm)
	{
		return;
	}

	/* Disconnect from the queue manager */
	Log("disconnecting from the queue manager %s", parms->qmname);
	MQDISC(&qm, &compcode, &reason);

	/* check for errors */
	checkerror("MQDISC", compcode, reason, parms->qmname);

	/* indicate we no longer have a connection to the qm */
	qm = 0;
}

void connectQM(PUTPARMS * parms)

{
	MQLONG	compcode=0;
	MQLONG	reason=0;

	/* check for the same queue manager */
	if (strcmp(parms->qmname, connectName) != 0)
	{
		/* check if we have a connection */
		if (qm != 0)
		{
			discQM(parms);
		}
	}

	/* check if we are already connected to the queue manager */
	if (0 == qm)
	{
		/* Connect to the queue manager */
		Log("\nconnecting to queue manager %s",parms->qmname);
		MQCONN(parms->qmname, &qm, &compcode, &reason);

		/* check for errors */
		checkerror("MQCONN", compcode, reason, parms->qmname);
		if (compcode != 0)
		{
			terminate = 1;
		}

		/* remember the current qm */
		strcpy(connectName, parms->qmname);
	}
}

void closeQ(PUTPARMS * parms)

{
	MQLONG	compcode=0;
	MQLONG	reason=0;

	/* was the queue ever opened? */
	if (0 == q)
	{
		return;
	}

	/* close the input queue */
	Log("closing the queue");
	MQCLOSE(qm, &q, MQCO_NONE, &compcode, &reason);

	/* report errors closing the queue */
	checkerror("MQCLOSE", compcode, reason, parms->qname);

	/* indicate q is no longer open */
	q = 0;
}

void openQ(PUTPARMS * parms)

{
	MQLONG	compcode=0;
	MQLONG	reason=0;
	MQLONG	openopt = 0;
	MQOD	objdesc = {MQOD_DEFAULT};

	/* check for the same queue */
	if ((strcmp(parms->qname, openName) != 0) || (strcmp(parms->remoteQM, openRemoteName) != 0))
	{
		/* check if we have an open queue */
		if (q != 0)
		{
			closeQ(parms);
		}
	}

	/* is the queue already open? */
	if (0 == q)
	{
		/* set the queue open options */
		strcpy(objdesc.ObjectName, parms->qname);
		strcpy(objdesc.ObjectQMgrName, parms->remoteQM);
		openopt = MQOO_OUTPUT + MQOO_FAIL_IF_QUIESCING;

		/* check if we need to set all context */
		if (MQMD_YES == parms->foundMQMD)
		{
			openopt |= MQOO_SET_ALL_CONTEXT;
		}

		/* open the queue for output */
		Log("opening queue %s for output", parms->qname);
		MQOPEN(qm, &objdesc, openopt, &q, &compcode, &reason);

		/* check for errors */
		checkerror("MQOPEN", compcode, reason, parms->qname);
		if (compcode != 0)
		{
			terminate = 1;
		}

		/* remember the current queue name */
		strcpy(openName, parms->qname);
		strcpy(openRemoteName, parms->remoteQM);
	}
}

void commitQ(PUTPARMS * parms)

{
	MQLONG	compcode=0;
	MQLONG	reason=0;

	MQCMIT(qm, &compcode, &reason);
	checkerror("MQCMIT", compcode, reason, parms->qname);
}

/**************************************************************/
/*                                                            */
/* This routine puts a message on the queue.                  */
/*                                                            */
/**************************************************************/

int putMessage(char * msgdata, 
			   MQLONG msglen,
			   char * mqmdptr,
			   PUTPARMS * parms)

{
	MQLONG	compcode=0;
	MQLONG	reason=0;
	int		hasRFH;
	int		rfhlen;
	MQMD	msgdesc = {MQMD_DEFAULT};
	MQPMO	mqpmo = {MQPMO_DEFAULT};
	MQRFH	*tempRFH;
	char	tempid[5];

	/* check if we are using an mqmd from the file */
	if (mqmdptr != NULL)
	{
		/* set the get message options */
		if (uow > 0)
		{
			/* use syncpoints */
			mqpmo.Options = MQPMO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING;
		}
		else
		{
			/* no synchpoint, each message as a separate UOW */
			mqpmo.Options = MQPMO_NO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING;
		}

		mqpmo.Options |= MQPMO_SET_ALL_CONTEXT;

		if (1 == parms->newMsgId)
		{
			mqpmo.Options |= MQPMO_NEW_MSG_ID;
		}

		/* set the MQMD */
		memcpy(&msgdesc, mqmdptr, sizeof(MQMD));
	}
	else
	{
		/* set the get message options */
		if (uow > 0)
		{
			/* use syncpoints */
			mqpmo.Options = MQPMO_SYNCPOINT | MQPMO_NEW_MSG_ID | MQPMO_FAIL_IF_QUIESCING;
		}
		else
		{
			/* no synchpoint, each message as a separate UOW */
			mqpmo.Options = MQPMO_NO_SYNCPOINT | MQPMO_NEW_MSG_ID | MQPMO_FAIL_IF_QUIESCING;
		}

		if ((1 == parms->inGroup) || (1 == parms->lastGroup))
		{
			mqpmo.Options |= MQPMO_LOGICAL_ORDER;

			if (1 == parms->inGroup)
			{
				msgdesc.MsgFlags |= MQMF_MSG_IN_GROUP;
			}

			if (1 == parms->lastGroup)
			{
				msgdesc.MsgFlags |= MQMF_LAST_MSG_IN_GROUP;
			}
		}

		/* Indicate V2 of MQMD */
		msgdesc.Version = MQMD_VERSION_2;

		/* set the persistence, etc if specified */
		msgdesc.Persistence = parms->persist;
		msgdesc.Encoding = parms->encoding;
		msgdesc.CodedCharSetId = parms->codepage;

		/* check if message expiry was specified */
		if (parms->expiry > 0)
		{
			msgdesc.Expiry = parms->expiry;
		}

		/* check if message type was specified */
		if (parms->msgtype > 0)
		{
			msgdesc.MsgType = parms->msgtype;
		}

		/* check if message priority was specified */
		if (parms->priority != MQPRI_PRIORITY_AS_Q_DEF)
		{
			msgdesc.Priority = parms->priority;
		}

		/* check if report options were specified */
		if (parms->report > 0)
		{
			msgdesc.Report = parms->report;
		}

		/* set the message format in the MQMD was specified */
		switch (parms->rfh)
		{
		case RFH_NO:
			{
				if (1 == parms->formatSet)
				{
					memcpy(msgdesc.Format, parms->msgformat, MQ_FORMAT_LENGTH);
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
		case 'A':
			{
				/* check if the message data contains an rfh header */
				hasRFH = 0;
				if (msglen >= (int)sizeof(MQRFH))
				{
					/* translate the first 4 bytes from EBCDIC to ASCII */
					memset(tempid, 0, sizeof(tempid));
					EbcdicToAscii((unsigned char *) msgdata, sizeof(MQRFH_STRUC_ID) - 1, (unsigned char *) &tempid);

					/* check for either an ASCII or EBCDIC structure id */
					if ((memcmp(tempid, MQRFH_STRUC_ID, sizeof(MQRFH_STRUC_ID) - 1) == 0) ||
						(memcmp(msgdata, MQRFH_STRUC_ID, sizeof(MQRFH_STRUC_ID) - 1) == 0))
					{
						/* the first four bytes match an RFH structure id */
						tempRFH = (MQRFH *) msgdata;
						hasRFH = tempRFH->Version;
						rfhlen = tempRFH->StrucLength;
						
						/* check for the RFH version field in PC format */
						if ((MQRFH_VERSION_1 != hasRFH) && (MQRFH_VERSION_2 != hasRFH))
						{
							/* reverse the order, and check for host integers */
							hasRFH = reverseBytes4(hasRFH);
							rfhlen = reverseBytes4(rfhlen);

							/* check for the RFH version field in host format */
							if ((MQRFH_VERSION_1 == hasRFH) || (MQRFH_VERSION_2 == hasRFH))
							{
								/* host encoding - check the length */
								if ((rfhlen < (int)sizeof(MQRFH)) || (rfhlen > msglen))
								{
									/* invalid length - do not treat as an RFH header */
									hasRFH = 0;
								}
							}
							else
							{
								/* do not recognize the version */
								hasRFH = 0;
							}
						}
						else
						{
							/* normal encoding - check the rfh length */
							if ((rfhlen < (int)sizeof(MQRFH)) || (rfhlen > msglen))
							{
								/* invalid length */
								hasRFH = 0;
							}
						}
					}

				}

				switch (hasRFH)
				{
				case 1:
					{
						memcpy(msgdesc.Format, MQFMT_RF_HEADER, sizeof(msgdesc.Format));
						break;
					}
				case 2:
					{
						memcpy(msgdesc.Format, MQFMT_RF_HEADER_2, sizeof(msgdesc.Format));
						break;
					}

				}

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
		if (parms->replyQM[0] != 0)
		{
			memcpy(msgdesc.ReplyToQMgr, parms->replyQM, strlen(parms->replyQM));
		}

		/* check if a reply to queue was specified */
		memset(msgdesc.ReplyToQ, 0, sizeof(msgdesc.ReplyToQ));
		if (parms->replyQ[0] != 0)
		{
			memcpy(msgdesc.ReplyToQ, parms->replyQ, strlen(parms->replyQ));
		}

		/* check if a correl id was specified */
		if (parms->correlidSet == 1)
		{
			memcpy(msgdesc.CorrelId, parms->correlid, MQ_CORREL_ID_LENGTH);
		}
		else
		{
			memset(msgdesc.CorrelId, 0, MQ_CORREL_ID_LENGTH);
		}

		/* check if a group id was specified */
		if (1 == groupOpen)
		{
			memcpy(msgdesc.GroupId, parms->saveGroupId, MQ_GROUP_ID_LENGTH);
		}
		else
		{
			if (1 == parms->groupidSet)
			{
				memcpy(msgdesc.GroupId, parms->groupid, MQ_GROUP_ID_LENGTH);
				msgdesc.MsgFlags |= MQMF_LAST_MSG_IN_GROUP | MQMF_MSG_IN_GROUP ;
			}
			else
			{
				memset(msgdesc.GroupId, 0, sizeof(msgdesc.GroupId));
			}
		}
	}

	/* perform the MQPUT */
	MQPUT(qm, q, &msgdesc, &mqpmo, msglen, msgdata, &compcode, &reason);

	/* check if we got to the end of the queue */
	/* check for errors */
	checkerror("MQPUT", compcode, reason, parms->qname);

	/* check if this message is part of a group */
	if (1 == parms->inGroup)
	{
		groupOpen = 1;
	}

	/* check if this is the last message in a group */
	if (1 == parms->lastGroup)
	{
		groupOpen = 0;
	}

	if (1 == groupOpen)
	{
		memset(parms->saveGroupId, 0, sizeof(parms->saveGroupId));
		memcpy(parms->saveGroupId, msgdesc.GroupId, MQ_GROUP_ID_LENGTH);
	}

	memcpy(puttime, msgdesc.PutTime, sizeof(msgdesc.PutTime));

	return compcode;
}

void processMessageFile(const char * fileName, PUTPARMS * parms)

{
	size_t			msglen=0;
	size_t			filedatalen=0;
	MQLONG			cc=MQCC_OK;
	MQLONG			mLen=0;
	int				rc;
	char *			mqmdptr=NULL;
	char *			buffer=NULL;
	const char *	ptr;

	/* pointer to data file areas */
	char *	filedata=NULL;
	char *	filedataOfs=NULL;
	char *	allocPtr=NULL;
	char *	mqdata=NULL;
	char	formTime[16];

	memset(puttime, 0, sizeof(puttime));

	/* process this line as a message data file name */
	/* read the message data file */
	rc = readFileData(fileName, &filedatalen, &filedata, parms);

	/* point to the beginning of the data */
	buffer = filedata;

	/* was the read file successful? */
	if ((0 == rc) && (filedata != NULL))
	{
		/* check if we have a connection to the queue manager */
		connectQM(parms);

		/* check if the queue has been opened */
		openQ(parms);

		while ((0 == terminate) && (filedatalen > 0))
		{
			/* check for delimiters in the data */
			ptr = scanForDelim(buffer, filedatalen, parms);

			if (ptr != NULL)
			{
				/* found a delimiter in the file data */
				/* shorten the data length */
				/* get the length of this message */
				msglen = ptr - buffer;

			}
			else
			{
				/* no delimiter - file data is the whole message */
				msglen = filedatalen;
			}

			mqmdptr = NULL;
			allocPtr = NULL;

			/* check for the presence of an MQMD */
			if ((msglen > sizeof(MQMD)) && (memcmp(buffer, MQMD_STRUC_ID, 4) == 0))
			{
				/* point to the MQMD */
				mqmdptr = buffer;

				/* skip past the MQMD to find the data */
				buffer += sizeof(MQMD);
				msglen -= sizeof(MQMD);
				filedatalen -= sizeof(MQMD);
			}

			/* get the length of the user data to write */
			mLen = (unsigned int)msglen;

			/* point to the user data in the buffer */
			mqdata = buffer;

			/* check if an MQMD was found */
			if (mqmdptr != NULL)
			{
				/* should the MQMD be used? */
				if (1 == parms->ignoreMQMD)
				{
					/* do not use the MQMD */
					mqmdptr = NULL;
				}
			}
			else
			{
				/* check if an RFH header should be inserted */
				/* no RFH header is inserted if there is an  */
				/* MQMD saved with the data                  */
				if ((parms->rfhdata != NULL) && (RFH_V1 == parms->rfh) || (RFH_V2 == parms->rfh) || (RFH_XML == parms->rfh))
				{
					/* allocate data for the RFH header and the user data */
					allocPtr = (char *)malloc(mLen + parms->rfhlength + 1);

					/* make sure the alloc worked */
					if (NULL == allocPtr)
					{
						/* tell the user what happened */
						Log("malloc failed trying to get memory for RFH header");
					}
					else
					{
						/* point to the message data */
						mqdata = allocPtr;

						/* copy the rfh header to the front of the data */
						memcpy(allocPtr, parms->rfhdata, parms->rfhlength);

						/* copy the user data data after the rfh */
						memcpy(allocPtr + parms->rfhlength, buffer, msglen);
						
						/* get the total length to write */
						mLen += parms->rfhlength;
					}
				}
			}

			/* put the data to the queue */
			cc = putMessage(mqdata,
							mLen,
							mqmdptr,
							parms);

			/* check if memory was allocated */
			if (allocPtr != NULL)
			{
				/* free the allocated memory */
				free(allocPtr);
			}

			/* check if the MQPUT was successful */
			if (MQCC_OK == cc)
			{
				if (0 == parms->msgwritten)
				{
					/* write out the time of the first message */
					formatTime(formTime, puttime);
					Log("first message written at %8.8s", formTime);
				}

				/* increment the message count */
				parms->msgwritten++;
				parms->byteswritten += mLen;

				if (0 == groupOpen)
				{
					commitQ(parms);
				}

				/* check if any thinktime has been specified */
				if (parms->thinkTime > 0)
				{
					Sleep(parms->thinkTime);
				}
			}
			else
			{
				/* end the program */
				terminate = 1;
			}


			/* update the data pointer */
			buffer += (msglen + parms->delimiterLen);

			/* decrement the remaining length */
			filedatalen -= msglen;

			/* check if a delimiter was found */
			if ((ptr != NULL) && (filedatalen >= parms->delimiterLen))
			{
				/* calculate the remaining bytes */
				filedatalen -= parms->delimiterLen;
			}
		}
	}
	else
	{
		if (rc != 0)
		{
			Log("Error reading file %s - rc %d", fileName, rc);
		}
	}

	if (filedata != NULL)
	{
		free(filedata);
	}
}

void procParmFile(FILE * parmfile, PUTPARMS * parms)

{
	size_t	len;
	char *	ptr;
	char	parmline[512];

	/* read the parameter file */
	while ((0 == terminate) && (fgets(parmline, sizeof(parmline) - 1, parmfile) != NULL))
	{
		/* check for a new line character at the end of the line */
		len = strlen(parmline);
		while ((len > 0) && (parmline[len - 1] < ' '))
		{
			/* get rid of the new line character */
			parmline[len - 1] = 0;
			len--;
		}

		/* point to the beginning of the line */
		/* skip any leading blanks */
		ptr = skipBlanks(parmline);

		/* truncate any trailing blanks */
		len = strlen(ptr);
		while ((len > 0) && (ptr[len] == ' '))
		{
			ptr[len] = 0;
			len--;
		}

		/* check for a comment or blank line */
		if ((ptr[0] != 0) && (ptr[0] != ';') && (ptr[0] != '#') && (ptr[0] != '*'))
		{
			if ('<' == ptr[0])
			{
				/* process this as an XML data entry */
				if (0 == foundUsr)
				{
					/* must be the first entry */
					foundUsr = processFirstUsrLine(ptr, parms, 1);
				}
				else
				{
					/* figure out what kind of entry this belongs to */
					foundUsr = processUsrLine(ptr, foundUsr, parms, 1);
				}
			}
			else
			{
				if ((0 == foundfiles) || ('[' == ptr[0]))
				{
					/* process this line as message parameters */
					foundfiles = processParmLine(ptr, parms);
				}
				else
				{
					processMessageFile(ptr, parms);
				}
			}
		}

		/* re-initialize the parameter input area */
		memset(parmline, 0, sizeof(parmline));
	}
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
	printf("  %s -f parm_file {-v}\n", pgmName);
	printf("   parm_file is the fully qualified name of the parameters file\n");
	printf("   -v verbose\n");
}

int main(int argc, char **argv)

{
	FILE *		parmfile;
	char		formTime[16];
	PUTPARMS	parms;

	/* print the copyright statement */
	Log(copyright);
	Log(Level);

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

	/* initialize the work areas */
	memset(formTime, 0, sizeof(formTime));

	/* initialize the open and connection names */
	memset(openName, ' ', sizeof(openName));
	memset(connectName, ' ', sizeof(connectName));
	memset(openRemoteName, 0, sizeof(openRemoteName));

	/* initialize the parameters area */
	initializeParms(&parms, sizeof(PUTPARMS));

	/* process any command line arguments */
	processArgs(argc, argv, &parms);

	/* check for error processing command line arguments */
	if (parms.err != 0)
	{
		/* display proper command format and exit */
		printHelp(argv[0]);
		exit(99);
	}

	/* was the name of a parameters file specified? */
	if (0 == parms.parmFilename[0])
	{
		/* report the error and show the proper command format */
		printf("Parameters file name not specified - use -f parameter to specify a parameters file\n");
		printHelp(argv[0]);
		exit(0);
	}

	/* set a termination handler */
	signal(SIGINT, InterruptHandler);

	/* open the parameters file */
	parmfile = fopen(parms.parmFilename, "r");

	if (parmfile != NULL)
	{
		/* start to process the parameters file a message at a time */
		procParmFile(parmfile, &parms);

		fclose(parmfile);
	}
	else
	{
		printf("Unable to open parameters file %s\n", parms.parmFilename);
		exit(99);
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

	if (parms.msgwritten > 0)
	{
		/* write out the time of the last message */
		formatTime(formTime, puttime);
		Log("last message written at %8.8s", formTime);

		/* dump out the total message count */
		Log("Total messages written " FMTI64, parms.msgwritten);
		Log("Total bytes written    " FMTI64, parms.byteswritten);
	}
	else
	{
		Log("No messages written - check parameters file");
	}

	/* close any open queues */
	closeQ(&parms);

	/* disconnect from the qm */
	discQM(&parms);

	/******************************************************************/
	/*                                                                */
	/* END OF PROGRAM                                                 */
	/*                                                                */
	/******************************************************************/

	Log("MQTEST program ended");

	return(0);
}
