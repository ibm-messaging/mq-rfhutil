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
/*   MQREPLY has the following parameters.                          */
/*                                                                  */
/*      -f name of the parameters file                              */
/*      -t sleep time in milliseconds (wait before replying)        */
/*      -m override the queue manager name in the parameters file.  */
/*      -q override the queue  name in the parameters file.         */
/*                                                                  */
/*    if no queue manager is specified, the default queue manager   */
/*    is used.                                                      */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "signal.h"

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
#include "comsubs.h"
#include "timesubs.h"
#include "parmline.h"
#include "putparms.h"

/* MQ subroutines include */
#include "qsubs.h"
#include "rfhsubs.h"

#ifndef WIN32
void Sleep(int amount)
{
	usleep(amount*1000);
}
#endif

	/* global termination switch */
	volatile int	terminate=0;
	volatile int	cancelled=0;

	/* reply queue open and close counters */
	int64_t			replyOpens=0;
	int64_t			replyCloses=0;

static char copyright[] = "(C) Copyright IBM Corp, 2001-2014\n";
static char Version[]=\
"@(#)MQReply V3.0 - MQ reply program  - Jim MacNair ";

#ifdef _DEBUG
static char Level[]="mqreply.c V3.0 Debug version ("__DATE__" "__TIME__")";
#else
static char Level[]="mqreply.c V3.0 Release version ("__DATE__" "__TIME__")";
#endif

/**************************************************************************************/
/*                                                                                    */
/* perform special processing on the jms folder when using it as the basis of a reply */
/* The Dst element should be removed and the Rto element should be renamed to Dst     */
/*                                                                                    */
/**************************************************************************************/

unsigned int processJMSfolder(char * inptr, char * outptr, size_t segLen)

{
	int		foundDst=0;
	int		foundRto=0;
	int		bytesMoved=0;
	int		i=0;
	int		extra;

	/* remove any trailing blanks */
	while ((segLen > 0) && (' ' == inptr[segLen - 1]))
	{
		segLen--;
	}

	/* start copying the input to the output */
	while (segLen > 0)
	{
		if ((1 == foundDst) || (1 == foundRto))
		{
			if ('<' == inptr[0])
			{
				/* check if we have found the end of the element */
				if (1 == foundRto)
				{
					/* check for the ending element tag */
					if (memcmp(inptr, "</Rto>", 6) == 0)
					{
						memcpy(outptr, "</Dst>", 6);
						inptr += 6;
						outptr += 6;
						bytesMoved += 6;
						segLen -= 6;
						foundRto = 0;
					}
					else
					{
						/* this should not occur but try to be graceful */
						outptr++[0] = inptr++[0];
						segLen--;
						bytesMoved++;
					}
				}
				else
				{
					/* check for the ending element tag */
					if (memcmp(inptr, "</Dst>", 6) == 0)
					{
						inptr += 6;
						segLen -= 6;
						foundDst = 0;
					}
					else
					{
						/* this should not occur but try to be graceful */
						inptr++;
						segLen--;
					}
				}
			}
			else
			{
				/* handle the next character of the element value */
				if (1 == foundRto)
				{
					outptr++[0] = inptr++[0];
					bytesMoved++;
				}
				else
				{
					inptr++;
				}

				segLen--;
			}
		}
		else
		{
			if ('<' == inptr[0])
			{
				if (memcmp(inptr, "<Dst>", 5) == 0)
				{
					foundDst = 1;
					inptr += 5;
					segLen -= 5;
				}
				else
				{
					if (memcmp(inptr, "<Rto>", 5) == 0)
					{
						foundRto = 1;
						memcpy(outptr, "<Dst>", 5);
						inptr += 5;
						outptr += 5;
						bytesMoved += 5;
						segLen -= 5;
					}
					else
					{
						/* Not what we are looking for - ignore it */
						outptr++[0] = inptr++[0];
						segLen--;
						bytesMoved++;
					}
				}
			}
			else
			{
				/* move the next byte of input */
				outptr++[0] = inptr++[0];
				segLen--;
				bytesMoved++;
			}
		}
	}

	/* insert padding characters if necessary */
	extra = bytesMoved % 4;
	if (extra > 0)
	{
		extra = 4 - extra;
	}

	while (extra > 0)
	{
		outptr++[0] = ' ';
		bytesMoved++;
		extra--;
	}

	/* return the new length */
	return bytesMoved;
}

int	BuildReply(char * msgdata, size_t datalen, char * inputData, int inputLen, MQMD2 *md, MQLONG qm, PUTPARMS * parms)

{
	size_t	allocLen=0;
	size_t	segLen=0;
	size_t	outSegLen;
	size_t	totLen=0;
	size_t	rfhLen;
	size_t	copyLen;
	char	*replyData=msgdata;
	char	*inptr;
	char	*outptr;
	MQRFH2	*rfh2;
	MQLONG	cc=MQCC_OK;
	MQLONG	rc=MQRC_NONE;
	MQLONG	openopt=0;
	MQPMO	pmo = {MQPMO_DEFAULT};
	MQOD	od = {MQOD_DEFAULT};    /* Object Descriptor             */
	MQMD2	mqmd = {MQMD2_DEFAULT};
	char	qmName[MQ_Q_MGR_NAME_LENGTH + 8];
	char	qName[MQ_Q_NAME_LENGTH + 8];
	char	trimmedQMname[MQ_Q_MGR_NAME_LENGTH + 8];
	char	trimmedQname[MQ_Q_NAME_LENGTH + 8];

	memset(qmName, 0, sizeof(qmName));
	memset(qName, 0, sizeof(qName));

	/* check if this is a request message */
	if (md->MsgType != MQMT_REQUEST)
	{
		Log("Message type is not a request message - message not processed - type=%d", md->MsgType);
		return -1;
	}

	/* capture the reply to queue manager name */
	memcpy(qmName, md->ReplyToQMgr, MQ_Q_MGR_NAME_LENGTH);

	/* capture the reply to queue name */
	memcpy(qName, md->ReplyToQ, MQ_Q_NAME_LENGTH);

	/* get another copy of the queue name with trailing blanks removed for display purposes */
	strcpy(trimmedQname, qName);
	rtrim(trimmedQname);
	strcpy(trimmedQMname, qmName);
	rtrim(trimmedQMname);

	/* check if we are using the same queue as before */
	if (((strcmp(parms->replyQname, trimmedQname) != 0) || (strcmp(parms->replyQMname, trimmedQMname) != 0)) && (parms->replyQname[0] != 0) && (parms->replyQ != 0))
	{
		/* close the current reply to queue */
		MQCLOSE(qm, &(parms->hReplyQ), MQCO_NONE, &cc, &rc);
		parms->hReplyQ = 0;

		checkerror("MQCLOSE", cc, rc, parms->replyQMname);

		/* keep count of the number of closes */
		replyCloses++;
	}

	/* make sure the previous connect has worked */
	if (0 == cc)
	{
		/* check if we want to introduce a delay - specified in milliseconds */
		if (parms->sleeptime > 0)
		{
			Sleep(parms->sleeptime);
		}

		/* remember the queue manager we are connected to */
		strcpy(parms->replyQMname, trimmedQMname);

		/* check if we need to open the reply queue */
		if (0 == parms->hReplyQ)
		{
			/* set the queue open options */
			openopt = MQOO_OUTPUT + MQOO_FAIL_IF_QUIESCING;

			/* set the queue name */
			strncpy(od.ObjectName, trimmedQname, MQ_Q_NAME_LENGTH);

			/* check if the queue manager is the same as the connected queue manager */
			if (strcmp(trimmedQMname, parms->qmname) != 0)
			{
				/* looks like a remote queue manager name */
				strncpy(od.ObjectQMgrName, trimmedQMname, MQ_Q_MGR_NAME_LENGTH);
			}

			if (0 == parms->silent)
			{
				/* tell what we are doing */
				Log("opening reply queue %s", trimmedQname);
			}

			/* open the queue for output */
			MQOPEN(qm, &od, openopt, &(parms->hReplyQ), &cc, &rc);

			/* check for errors */
			checkerror("MQOPEN", cc, rc, qName);

			/* keep track of the number of opens */
			replyOpens++;
		}

		if (cc != MQCC_OK)
		{
			Log("Unable to open reply to queue %s for output", trimmedQname);
		}
		else
		{
			/* remember the queue we are connected to */
			strcpy(parms->replyQname, trimmedQname);

			/* check if the request message is to be sent back as the reply */
			if (1 == parms->useInputAsReply)
			{
				/* point to the request message */
				replyData = inputData;
				datalen = inputLen;

				/* use most of the original MQMD */
				memcpy(&mqmd, md, sizeof(mqmd));

				/* set the correlation id and clear the message id */
				/* clear the reply to queue and queue manager */
				/* set the message type to reply */
				memcpy(&(mqmd.CorrelId), md->MsgId, MQ_CORREL_ID_LENGTH);
				memcpy(&(mqmd.MsgId), MQMI_NONE, MQ_MSG_ID_LENGTH);
				mqmd.MsgType = MQMT_REPLY;
				memset(&(mqmd.ReplyToQ), 0, sizeof(mqmd.ReplyToQ));
				memset(&(mqmd.ReplyToQMgr), 0, sizeof(mqmd.ReplyToQMgr));
			}
			else
			{
				/* set the MQMD options from the original message */
				memcpy(mqmd.CorrelId, md->MsgId, MQ_CORREL_ID_LENGTH);
				memcpy(&(mqmd.MsgId), MQMI_NONE, MQ_MSG_ID_LENGTH);
				mqmd.MsgType = MQMT_REPLY;
				memcpy(mqmd.Format, parms->msgformat, MQ_FORMAT_LENGTH);

				/* check if a ccsid was specified */
				if (parms->codepage != 0)
				{
					mqmd.CodedCharSetId = parms->codepage;
				}

				/* check if encoding was specified */
				if (parms->encoding != 0)
				{
					mqmd.Encoding = parms->encoding;
				}

				/* check if we need to return the RFH2 header from the request message */
				/* some programs like WBI/SF may require this */
				if (1 == parms->resendRFH)
				{
					/* check if we have an RFH2 header */
					if ((memcmp(md->Format, MQFMT_RF_HEADER_2, MQ_FORMAT_LENGTH) == 0) && (inputLen >= MQRFH_STRUC_LENGTH_FIXED_2))
					{
						rfh2 = (MQRFH2 *)inputData;
						rfhLen = (unsigned int)rfh2->StrucLength;

						/* allocate memory to allocate for the reply */
						allocLen = rfhLen + datalen;
						replyData = (char *)malloc(allocLen);
						rfh2 = (MQRFH2 *)replyData;

						/* copy the fixed part of the RFH to the data buffer */
						memcpy(replyData, inputData, MQRFH_STRUC_LENGTH_FIXED_2);

						/* set the message format of the reply in the RFH2 header */
						memcpy(rfh2->Format, parms->msgformat, 8);

						/* calculate the remaining length */
						rfhLen -= MQRFH_STRUC_LENGTH_FIXED_2;
						totLen = MQRFH_STRUC_LENGTH_FIXED_2;

						/* get an input and an output pointer */
						outptr = replyData + MQRFH_STRUC_LENGTH_FIXED_2;
						inptr = inputData + MQRFH_STRUC_LENGTH_FIXED_2;

						while (rfhLen > 0)
						{
							/* get the length of this segment */
							memcpy((char *)&segLen, inptr, 4);
							inptr += 4;

							/* figure out what kind of folder this is */
							if (memcmp(inptr, "<jms>", 5) == 0)
							{
								/* need to remove the Dst element and change the Rto to Dst */
								outSegLen = processJMSfolder(inptr, outptr + 4, segLen);
							}
							else
							{
								/* no special processing necessary - just copy the data */
								memcpy(outptr + 4, inptr, segLen);

								/* output length is the same as the input length */
								outSegLen = segLen;
							}

							/* set the segment length */
							memcpy(outptr, (char *)&outSegLen, 4);

							/* calculate the total length of the RFH2 */
							totLen += outSegLen + 4;

							/* update the input and output pointers */
							inptr += segLen;
							outptr += outSegLen + 4;

							/* calculate the remaining bytes in the input RFH2 */
							rfhLen -= segLen + 4;
						}

						/* set the total length in the RFH header */
						rfh2->StrucLength = totLen;
					}

					/* copy the message data */
					copyLen = datalen;
					memcpy(replyData + totLen, msgdata, copyLen);
				}
				else
				{
					replyData = msgdata;
				}
			}

			/* write the reply message */
			/* perform the MQPUT */
			MQPUT(qm, parms->hReplyQ, &mqmd, &pmo, datalen + totLen, replyData, &cc, &rc);

			/* check for errors */
			checkerror("MQPUT", cc, rc, parms->replyQname);

			if (0 == cc)
			{
				parms->replyCount++;
				parms->byteswritten += datalen + totLen;

				if (1 == parms->verbose)
				{
					Log("Reply sent to queue %s on queue manager %s", trimmedQname, trimmedQMname);
				}
			}

			/* check if we acquired storage */
			if ((1 == parms->resendRFH) && (memcpy(md->Format, MQFMT_RF_HEADER_2, MQ_FORMAT_LENGTH) == 0) && (datalen >= MQRFH_STRUC_LENGTH_FIXED_2))
			{
				/* free the storage we acquired */
				free(replyData);
			}
		}
	}

	return cc;
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
	printf("%s\n", Level);
	printf("format is:\n");
	printf("   %s -f parm_filename <-t milliseconds> <-m QMname> <-q queue>\n", pgmName);
	printf("         -m will override the queue manager name\n");
	printf("         -q will override the queue name\n");
#ifdef MQCLIENT
	printf("         -m can be in the form of ChannelName/TCP/hostname(port)\n");
#endif
}

int main(int argc, char **argv)
{
	size_t		replyDataLen=0;
	size_t		memSize;
	int64_t		msgsRead=0;
	int64_t		bytesRead=0;
	int64_t		avgbytes;
	int			maxrate=0;
	int			firstsec=0;
	int			secondcount=0;
	int			firstTime=0;
	int			rfhlength=0;
	int			remainingTime;
	MQLONG		qm=0;
	MQLONG		q=0;
	MQLONG		maxLen;
	MQLONG		compcode;
	MQLONG		reason;
	MQLONG		Select[1];			/* attribute selectors           */
	MQLONG		IAV[1];				/* integer attribute values      */
	MQOD		objdesc = {MQOD_DEFAULT};
	MQMD2		msgdesc = {MQMD2_DEFAULT};
	MQLONG		openopt = 0;
	MQGMO		mqgmo = {MQGMO_DEFAULT};
	MQLONG		datalen=0;
	char		*msgdata;
	FILE		*replyDataFile;
	char		*replyData=0;
	MQOD		od = {MQOD_DEFAULT};    /* Object Descriptor             */
	PUTPARMS	parms;					/* Input parameters and global variables */

	/* print the copyright statement */
	printf(copyright);
	printf(Level);

	/* initialize the work areas */
	initializeParms(&parms, sizeof(PUTPARMS));

	/* check for too few input parameters */
	if (argc < 2)
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

	/* read the parameters file to get the options */
	processParmFile(parms.parmFilename, &parms, READDATAFILES);

	/* check for overrides */
	processOverrides(&parms);

	/* exit if queue name not found */
	if (0 == parms.qname[0])
	{
		Log("***** Queue name not found in parameters file");
		return 2;
	}

	/* check for a reply file or using input message as reply */
	if ((0 == parms.replyFilename) && (0 == parms.useInputAsReply))
	{
		Log("***** reply file name missing and not using input message as reply");
		exit(97);
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

	if (parms.sleeptime > 0)
	{
		/* print out the sleep time parameter */
		Log("Sleep time set to %d milliseconds", parms.sleeptime);
	}

	/* check for help request */
	if ((argv[1][0] == '?') || (argv[1][1] == '?'))
	{
		printHelp(argv[0]);
		exit(0);
	}

	/* check if sending input message back as reply */
	if (0 == parms.useInputAsReply)
	{
		/* try to read the reply data file */
		replyDataFile = fopen(parms.replyFilename, "rb");

		if (0 == replyDataFile)
		{
			/* error opening reply data file */
			Log("Unable to open reply data file %s", parms.replyFilename);
			exit(85);
		}

		/* determine the file length */
		/* figure out how long the data is and read it into a buffer */
		fseek(replyDataFile, 0L, SEEK_END);
		replyDataLen = ftell(replyDataFile);
		fseek(replyDataFile, 0L, SEEK_SET);
		replyData = (char *) malloc(replyDataLen + 1);

		/* read the data into the buffer */
		fread(replyData, 1, replyDataLen, replyDataFile);

		/* tell what we are doing */
		Log("\n%d bytes read from reply data file %s", replyDataLen, parms.replyFilename);

		/* close the reply data file */
		fclose(replyDataFile);
	}
	else
	{
		/* tell what we are doing */
		Log("Sending input message back as reply");
	}

	/* set a termination handler */
	signal(SIGINT, InterruptHandler);

	/* Connect to the queue manager */
#ifdef MQCLIENT
	clientConnect2QM(parms.qmname, &qm, &(parms.maxMsgLen), &compcode, &reason);
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

	/* set the queue open options */
	openopt = MQOO_INPUT_SHARED + MQOO_FAIL_IF_QUIESCING + MQOO_INQUIRE;

	/* open the queue for input */
	Log("opening queue %s for input\n", parms.qname);
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

	/* check if it worked */
	if (MQCC_OK == compcode)
	{
		if (IAV[0] < parms.maxmsglen)
		{
			/* change the maximum message length to the maximum allowed */
			parms.maxmsglen = IAV[0];
		}
	}

	/* allocate a buffer for the message */
	memSize = (unsigned int)parms.maxmsglen + 1;
	msgdata = (char *)malloc(memSize);

	/* make sure the malloc worked */
	if (NULL == msgdata)
	{
		/* tell what happened */
		Log("*****Error - memory allocation for buffer failed");

		/* exit */
		return 85;
	}

	/* initialize the buffer */
	memset(msgdata, 0, memSize);

	/* enter get message loop */
	while ((compcode == MQCC_OK) && (0 == terminate) && ((0 == parms.totcount) || (msgsRead < parms.totcount)))
	{
		/* set the get message options */
		if (0 == parms.maxWaitTime)
		{
			mqgmo.Options = MQGMO_NO_WAIT | MQGMO_FAIL_IF_QUIESCING;
		}
		else
		{
			mqgmo.Options = MQGMO_WAIT | MQGMO_FAIL_IF_QUIESCING;
			mqgmo.WaitInterval = parms.maxWaitTime * 1000;
		}

		mqgmo.MatchOptions = MQGMO_NONE;

		/* check if we are using logical order */
		if (1 == parms.logicalOrder)
		{
			mqgmo.Options |= MQGMO_LOGICAL_ORDER;
		}

		/* reset the msgid and correlid */
		memcpy(msgdesc.MsgId, MQMI_NONE, sizeof(msgdesc.MsgId));
		memcpy(msgdesc.CorrelId, MQCI_NONE, sizeof(msgdesc.CorrelId));

		remainingTime = parms.maxWaitTime;
		do
		{
			/* only wait for 1 second */
			mqgmo.WaitInterval = 1000;
			remainingTime--;

			/* since we have a signal handler installed, we do not want to be in an MQGET for a long time */
			/* perform the MQGET */
			maxLen = parms.maxmsglen;
			MQGET(qm, q, &msgdesc, &mqgmo, maxLen, msgdata, &datalen, &compcode, &reason);
		} while ((remainingTime > 0) && (MQCC_FAILED == compcode) && (2033 == reason) && (0 == terminate));

		if ((2 == compcode) && (2033 == reason))
		{
			Log("\nTimeout period expired - program is ending");
		}
		else
		{
			checkerror("MQGET", compcode, reason, parms.qname);
		}

		if ((MQCC_OK == compcode) && (0 == terminate))
		{
			/* count the total number of messages read */
			msgsRead++;

			/* calculate the total bytes in the message */
			bytesRead += datalen;

			/* generate a reply message */
			BuildReply(replyData, replyDataLen, msgdata, datalen, &msgdesc, qm, &parms);
		}
	}

	/* close the input queue */
	Log("closing the input queue");
	MQCLOSE(qm, &q, MQCO_NONE, &compcode, &reason);

	checkerror("MQCLOSE", compcode, reason, parms.qname);

	/* check if we have an open reply to queue */
	if (parms.hReplyQ != 0)
	{
		Log("closing the reply queue");
		MQCLOSE(qm, &(parms.hReplyQ), MQCO_NONE, &compcode, &reason);

		checkerror("MQCLOSE", compcode, reason, parms.replyQname);

		/* account for the last close */
		replyCloses++;
	}

	/* dump out the statistics for reply queue opens and closes */
	Log("Reply queue opened " FMTI64 " closed " FMTI64 " times", replyOpens, replyCloses);

	/* Disconnect from the queue manager */
	Log("disconnecting from the queue manager");
	MQDISC(&qm, &compcode, &reason);

	checkerror("MQDISC", compcode, reason, parms.qmname);

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
	Log("\nTotal messages received " FMTI64, msgsRead);

	/* give the total and average message size */
	if (bytesRead > 0)
	{
		avgbytes = bytesRead / msgsRead;
		Log("total bytes received in all messages " FMTI64, bytesRead);
		Log("average message size " FMTI64, avgbytes);
		Log("total replies sent " FMTI64, parms.replyCount);

		/* were any replies sent? */
		if (parms.replyCount > 0)
		{
			/* display reply statistics */
			Log("total bytes in all replies " FMTI64, parms.byteswritten);

			/* calculate the average size of the reply messages */
			avgbytes = parms.byteswritten / parms.replyCount;
			Log("average reply message size " FMTI64, avgbytes);
		}
	}

	/* release any acquired storage */
	if (replyData != 0)
	{
		free(replyData);
		replyData = 0;
	}

	/******************************************************************/
	/*                                                                */
	/* END OF PROGRAM                                                 */
	/*                                                                */
	/******************************************************************/

	Log("\nmqreply program ended");
	return 0;
}
