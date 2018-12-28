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
/*   putparms.c - Input parameter file processing subroutines       */
/*                                                                  */
/********************************************************************/

/**************************************************************/
/*                                                            */
/* Read the parameters file and process each line.            */
/*                                                            */
/**************************************************************/

#include <stdlib.h>
#include <stdio.h>
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

/* RFH and MQ processing subroutines include */
#include "rfhsubs.h"
#include "qsubs.h"

/* default code page to use */
#define DEF_EBCDIC_CODEPAGE	500
#ifdef WIN32
#define DEF_CODEPAGE	437
#else
#define DEF_CODEPAGE	850
#endif

void mallocError(const size_t len, size_t memUsed)

{
#ifdef MQ_64_BIT
#ifdef _WIN32
	printf("*****Error - memory allocation for %I64d bytes failed (total used previously %I64d)\n", len, memUsed);
#else
	printf("*****Error - memory allocation for %lld bytes failed (total used previously %lld)\n", len, memUsed);
#endif
#else
	printf("*****Error - memory allocation for %d bytes failed (total used previously %d)\n", len, memUsed);
#endif
#ifndef _WIN32
	printf(" use ulimit with the -d parameter to increase the heap size\n");
#endif
}

/**************************************************************/
/*                                                            */
/* Routine to check for an rfh in the file data.  This        */
/*  routine checks the first four characters for an RFH       */
/*  structure id in either ASCII or EBCDIC, and then checks   */
/*  the next four bytes for either a binary 1 or 2, trying    */
/*  both normal and reversed integers.                        */
/*                                                            */
/**************************************************************/

int findRFH(FILEPTR* fptr, size_t *rfhLen, PUTPARMS *parms)

{
	int		rfhversion=0;

	/* initialize the rfh length to zero */
	(*rfhLen) = 0;

	/* tell what we are doing */
	if (parms->verbose)
	{
		printf("Looking for rfh in file\n");
	}

	/* check for an RFH header */
	rfhversion = isRFH((unsigned char *)fptr->dataptr, fptr->length, rfhLen);

	/* was an RFH header found? */
	if (rfhversion > 0)
	{
		/* translate the RFH if necessary */
		translateRFH((unsigned char *)fptr->dataptr, fptr->length);

		/* make sure the length of the RFH makes sense */
		if ((*rfhLen) > fptr->length)
		{
			/* something is corrupt */
			(*rfhLen) = 0;
			Log("***** Error - invalid RFH header found in data - StrucLength too large");
		}
		else
		{
			/* was encoding for rfh specified in parameters file? */
			if (parms->encoding > 0)
			{
				/* yes, use it */
				fptr->Encoding = parms->encoding;
			}
			else
			{
				/* pick an intelligent default based on the platform - e.g. for PC Intel = 546 */
				fptr->Encoding = MQENC_NATIVE;
			}
		}
	}


	if (1 == parms->verbose)
	{
		/* did we find an rfh at the front of the file? */
		if (rfhversion > 0)
		{
			/* found rfh */
			printf("using rfh version %d found in file\n", rfhversion);
		}
		else
		{
			/* no rfh found in file - rfh set to no */
			printf("no rfh found in file\n");
		}
	}

	return rfhversion;
}

/**************************************************************/
/*                                                            */
/* Create a message data file block.                          */
/*                                                            */
/* This routine will check for the presence of an MQMD and    */
/* RFH header at the front of the data.  It will adjust the   */
/* user data pointer and length if either header is present.  */
/*                                                            */
/**************************************************************/

FILEPTR * createFileBlock(char * msgPtr, const char * userPtr, MQMD2 * mqmdPtr, char * allocPtr, const size_t msgLen, const size_t rfhlen, PUTPARMS *parms)

{
	FILEPTR*		newfptr=NULL;							/* pointer to the newly allocated file block */
	size_t			memlen=sizeof(FILEPTR);					/* number of bytes to allocate */
	size_t			len = msgLen;
	size_t			templen;
	char			tempid[2 * MQ_CORREL_ID_LENGTH + 1];
	char			tempid2[2 * MQ_GROUP_ID_LENGTH + 1];

	/* increase the message count */
	parms->mesgCount++;

	/* allocate a new message data block */
	newfptr  = (FILEPTR *) malloc(memlen);

	/* check if malloc worked */
	if (newfptr != NULL)
	{
		/* calculate total memory used */
		parms->memUsed += memlen;

		/* do some initializstion */
		newfptr->hasMQMD = 0;

		if (1 == parms->ignoreMQMD)
		{
			newfptr->mqmdptr = NULL;
		}
		else
		{
			newfptr->mqmdptr = mqmdPtr;

			/* check if there is an MQMD */
			if (mqmdPtr != NULL)
			{
				/* there is an MQMD */
				newfptr->hasMQMD = 1;
				newfptr->newMsgId = parms->newMsgId;
				parms->foundMQMD = 1;			/* remember an MQMD was found so set all is specified on MQOPEN */
			}
		}

		newfptr->dataptr = msgPtr;			/* point to the start of the message data */
		newfptr->userDataPtr = userPtr;		/* remember where the user data starts    */
		newfptr->length = len;
		newfptr->hasRFH = parms->rfh;
		newfptr->useFileRFH = 0;
		newfptr->nextfile = NULL;
		newfptr->acqStorAddr = allocPtr;

		if ((parms->rfh != RFH_NO) && (parms->rfh != RFH_AUTO))
		{
			newfptr->rfhlen = parms->rfhlength;	/* remember the length of the rfh header */
		}

		newfptr->Codepage = parms->codepage;
		newfptr->Encoding = parms->encoding;
		newfptr->Expiry   = parms->expiry;
		newfptr->Msgtype  = parms->msgtype;
		newfptr->Persist  = parms->persist;
		newfptr->Priority = parms->priority;
		newfptr->Report = parms->report;
		newfptr->Feedback = parms->feedback;

		memset(newfptr->CorrelId, 0, sizeof(newfptr->CorrelId));
		newfptr->CorrelIdSet = parms->correlidSet;
		if (parms->correlidSet)
		{
			memcpy(newfptr->CorrelId, parms->correlid, sizeof(newfptr->CorrelId));
		}

		memset(newfptr->GroupId, 0, sizeof(newfptr->GroupId));
		newfptr->GroupIdSet = parms->groupidSet;
		if (parms->groupidSet)
		{
			memcpy(newfptr->GroupId, parms->groupid, sizeof(newfptr->GroupId));
		}

		/* capture the group information */
		newfptr->inGroup = parms->inGroup;
		newfptr->lastGroup = parms->lastGroup;

		/* check if the accounting token parameter was set */
		if (1 == parms->acctTokenSet)
		{
			newfptr->AcctTokenSet = 1;
			memcpy(newfptr->AccountingToken, parms->accountingToken, MQ_ACCOUNTING_TOKEN_LENGTH);
		}
		else
		{
			newfptr->AcctTokenSet = 0;
			memset(newfptr->AccountingToken, 0, MQ_ACCOUNTING_TOKEN_LENGTH);
		}

		memset(newfptr->Format, 0, sizeof(newfptr->Format));
		newfptr->FormatSet = parms->formatSet;
		if (parms->formatSet)
		{
			memcpy(newfptr->Format, parms->msgformat, sizeof(newfptr->Format));
		}

		strcpy(newfptr->ReplyQM, parms->replyQM);
		strcpy(newfptr->ReplyQ, parms->replyQ);
		strcpy(newfptr->UserId, parms->userId);

		/* check if we should scan for an RFH header embedded with the data in the file */
		if (RFH_AUTO == parms->rfh)
		{
			/* look for an RFH header in the data */
			newfptr->hasRFH = findRFH(newfptr, &templen, parms);

			/* was an RFH found? */
			if (RFH_NO != newfptr->hasRFH)
			{
				/* found RFH in data */
				newfptr->useFileRFH = 1;

				/* we need to update the user data pointer */
				newfptr->userDataPtr += templen;
				newfptr->rfhlen = templen;		/* remember the length of the rfh header */
			}
		}
		else if (RFH_V1 == parms->rfh)
		{
			/* set the format field to indicate the RFH1 header */
			memcpy(newfptr->Format, MQFMT_RF_HEADER_1, MQ_FORMAT_LENGTH);
		}
		else if (RFH_V2 == parms->rfh)
		{
			/* set the format field to indicate the RFH1 header */
			memcpy(newfptr->Format, MQFMT_RF_HEADER_2, MQ_FORMAT_LENGTH);
		}

		/* check for verbose output */
		if (parms->verbose)
		{
			if (1 == parms->correlidSet)
			{
				memset(tempid, 0, sizeof(tempid));
				AsciiToHex((unsigned char *)tempid, (unsigned char *)&(parms->correlid), MQ_CORREL_ID_LENGTH);
				Log("Correlation id set to %s", tempid);
			}

			if (1 == parms->groupidSet)
			{
				memset(tempid2, 0, sizeof(tempid2));
				AsciiToHex((unsigned char *)tempid2, (unsigned char *)&(parms->groupid), MQ_GROUP_ID_LENGTH);
				if (parms->verbose)
				Log("Group id set to %s", tempid2);
			}
		}

		newfptr->thinkTime = parms->thinkTime;
		newfptr->setTimeStamp = parms->setTimeStamp;
		newfptr->timeStampInAccountingToken = parms->timeStampInAccountingToken;
		newfptr->timeStampInCorrelId = parms->timeStampInCorrelId;
		newfptr->timeStampInGroupId = parms->timeStampInGroupId;
		newfptr->timeStampUserProp = parms->timeStampUserProp;
		newfptr->timeStampOffset = parms->timeStampOffset;
	}
	else
	{
		mallocError(memlen, parms->memUsed);
	}

	return newfptr;
}

/**************************************************************/
/*                                                            */
/* Read message data from a file.                             */
/*                                                            */
/**************************************************************/

int readFileData(const char *filename, size_t *length, char ** dataptr, PUTPARMS * parms)

{
	size_t	datalen;
	size_t	memlen;
	char	*msgdata=NULL;
	FILE*	datafile;
	int		rc=0;

	if ((datafile = fopen(filename, "rb")) == NULL)
	{
		Log("Unable to open input file %s\n", filename);
		exit(1);
	}

	/* figure out how long the data is and read it into a buffer */
	fseek(datafile, 0L, SEEK_END);
	datalen = ftell(datafile);
	fseek(datafile, 0L, SEEK_SET);
	memlen = datalen + 1;
	msgdata = (char *) malloc(memlen);

	/* check if the malloc worked */
	if (msgdata != NULL)
	{
		/* initialize the memory */
		memset(msgdata, 0, memlen);

		/* calculate total memory used */
		parms->memUsed += memlen;

		fread(msgdata, 1, datalen, datafile);
		fclose(datafile);

		/* tell what we are doing */
		printf("\n%d bytes read from data file %s\n", datalen, filename);

		/* set the length and data pointers */
		(*length) = datalen;
		(*dataptr) = msgdata;
	}
	else
	{
		mallocError(memlen, parms->memUsed);
		rc = -1;
	}

	return rc;
}

/**************************************************************/
/*                                                            */
/* Routine to scan for a delimiter sequence in the data.      */
/*                                                            */
/**************************************************************/

const char * scanForDelim(const char * msgdata, const size_t datalen, PUTPARMS *parms)

{
	const char *		ptr=NULL;
	const char *		ptrend;

	/* do we have a delimiter? */
	if (parms->delimiterLen > 0)
	{
		ptr = msgdata;
		ptrend = msgdata + datalen - parms->delimiterLen;
		while ((ptr != NULL) && (ptr < ptrend) && (memcmp(ptr, parms->delimiter, parms->delimiterLen) != 0))
		{
			while ((ptr < ptrend) && (ptr[0] != parms->delimiter[0]))
			{
				ptr++;
			}

			/* could not find first character of delimiter - time to get out */
			if (ptr[0] != parms->delimiter[0])
			{
				ptr = NULL;
			}
			else
			{
				if ((ptr != NULL) && (memcmp(ptr, parms->delimiter, parms->delimiterLen) != 0))
				{
					ptr++;
				}
			}
		}
	}

	return ptr;
}

void listMessage(size_t datalen, int useFileRFH, PUTPARMS *parms)

{
	if (parms->verbose)
	{
		/* tell what we did */
		if (parms->msgformat[0] > 0)
		{
			printf("mqmd format is (\"%s\")\n", parms->msgformat);
		}

		printf("persistence = %d, code page %d, encoding %d expiry %d priority %d report %d feedback %d\n", 
				parms->persist, parms->codepage, parms->encoding, parms->expiry, parms->priority, parms->report, parms->feedback);

		printf("reply to QM (%s), reply to Queue (%s), userid(%s)\n", parms->replyQM, parms->replyQ, parms->userId);

		/* indicate whether RFH headers are to be used or not */
		switch (parms->rfh)
		{
		case RFH_NO:
			{
				printf("no record format headers (rfh) to be used\n");
				break;
			}
		case RFH_V1:
			{
				if (0 == useFileRFH)
				{
					printf("V1 record format headers (rfh) will be used\n");
					printf(" App group (%s) Format (%s)\n", parms->rfhappgroup, parms->rfhformat);
				}
				else
				{
					printf("V1 record format headers (rfh) found in file will be used\n");
				}

				break;
			}
		case RFH_V2:
			{
				if (0 == useFileRFH)
				{
					printf("V2 record format headers (rfh) will be used\n");
					printf(" Domain (%s) Set (%s) Type (%s) Fmt (%s)\n", 
							parms->rfhdomain, parms->rfhset, parms->rfhtype, parms->rfhfmt);
				}
				else
				{
					printf("V2 record format headers (rfh) found in file will be used\n");
				}

				break;
			}
		case RFH_XML:
			{
				printf("XML only record format headers (rfh) will be used\n");
				break;
			}
		case RFH_AUTO:
			{
				printf("Record format headers (rfh) will be used if found with message data\n");
				break;
			}
		}
	}
}

/**************************************************************/
/*                                                            */
/* Create a message data file block and read and process the  */
/* file data.                                                 */
/*                                                            */
/* There are several possibilities that must be taken into    */
/* account.  First, the file may contain one or more messages */
/* separated by a delimiter string.  Second, the individual   */
/* messages may contain an embedded MQMD.  Third, the         */
/* individual messages may contain an embedded RFH header.    */
/* Finally, the parameters file may indicate that an RFH      */
/* header is to be inserted into the data.                    */
/*                                                            */
/* In all cases, delimiters will break the file into multiple */
/* individual messages.  The search for a delimiter is done   */
/* first.                                                     */
/*                                                            */
/* Once the individual messages in the file have been found,  */
/* a check will be made for an MQMD at the front of the user  */
/* data.  If an MQMD is found, then this will be used.  No    */
/* RFH header will be inserted.                               */
/*                                                            */
/* If an RFH header is to be inserted at the front of the     */
/* user data (options 1, 2 or XML) then the RFH will be       */
/* inserted in front of the data.  If there is more than one  */
/* message in the file, then a new area of storage must be    */
/* allocated for each message after the first one, since      */
/* there is no room for the rfh header in the data buffer.    */
/*                                                            */
/* Finally, if the rfh option has been set to auto (A) then   */
/* a check will be made for an RFH header at the front of the */
/* data.                                                      */
/*                                                            */
/**************************************************************/

FILEPTR * getFileData(char *filename, PUTPARMS *parms)

{
	size_t		datalen=0;
	size_t		remainLen=0;
	size_t		memlen;
	char		*msgdata=NULL;
	char		*userPtr;			/* pointer to user data             */
	const char	*delimPtr;
	char		*allocPtr=NULL;
	char		*allocMsg=NULL;
	MQMD2		*mqmdPtr;
	FILEPTR *	newfptr=NULL;
	FILEPTR *	currfptr=NULL;
	FILEPTR *	fptr=NULL;
	int			rc;
	int			mqmdVer=0;
	int			mqmdLen=0;
	int			insertRFH=0;

	/* read the message data file after inserting an RFH header */
	rc = readFileData(filename, &datalen, &msgdata, parms);

	/* was the read successful? */
	if (0 == rc) 
	{
		/* increase the file counter */
		parms->fileCount++;

		/* was the data length > 0 */
		if ((msgdata != NULL) && (datalen > 0))
		{
			/* remember the message data buffer */
			allocPtr = msgdata;
			userPtr = msgdata;

			/* check if we are treating all the messages as a single group */
			if (1 == parms->fileAsGroup)
			{
				/* indicate that we are in a group */
				parms->inGroup = 1;
			}

			do
			{
				/* check if we have a delimiter in the data */
				delimPtr = scanForDelim(userPtr, datalen, parms);

				/* did we find a delimiter in the data we just read in? */
				if (delimPtr != NULL)
				{
					/* found a delimiter */
					/* recalculate the data length and calculate the bytes remaining */
					remainLen = datalen;
					datalen = delimPtr - userPtr;
					remainLen -= (datalen + parms->delimiterLen);
				}
				else
				{
					/* last message in file */
					remainLen = 0;
				}

				/* check for an MQMD */
				mqmdVer = checkAndXlateMQMD(userPtr, datalen);
				if (mqmdVer > 0)
				{
					/* point to the embedded MQMD */
					mqmdPtr = (MQMD2 *)userPtr;

					/* is this a Version 2 MQMD? */
					if (MQMD_VERSION_2 == mqmdVer)
					{
						/* get length of MQMD V2 */
						mqmdLen = sizeof(MQMD2);
					}
					else
					{
						/* get length of MQMD V1 */
						mqmdLen = sizeof(MQMD);
					}

					/* move past the MQMD */
					datalen -= mqmdLen;
					userPtr += mqmdLen;
				}
				else
				{
					/* no MQMD */
					mqmdPtr = NULL;
					mqmdLen = 0;
				}

				/* check if we are inserting an RFH header at the front of the data */
				/* an rfh header will also be inserted if latency measurements are  */
				/* being made and the user property option has been selected.       */
				if ((RFH_V1 == parms->rfh) || (RFH_V2 == parms->rfh) || (RFH_XML == parms->rfh) || (1 == parms->timeStampUserProp))
				{
					/* need to allocate storage */
					memlen = datalen + parms->rfhlength + mqmdLen;
					allocMsg = (char *)malloc(memlen + 1);

					/* check if the malloc worked */
					if (allocMsg != NULL)
					{
						/* remember how much memory we have used */
						parms->memUsed += memlen;

						/* terminate the memory to avoid overruns */
						allocMsg[memlen] = 0;

						/* is there an MQMD to copy? */
						if (mqmdPtr != NULL)
						{
							/* copy the MQMD to the new message area */
							memcpy(allocMsg, mqmdPtr, mqmdLen);
						}

						/* copy the RFH header and the message data to the allocated storage */
						memcpy(allocMsg + mqmdLen, parms->rfhdata, parms->rfhlength);
						memcpy(allocMsg + mqmdLen + parms->rfhlength, userPtr, datalen);

						/* allocate a file pointer for this message */
						newfptr = createFileBlock(allocMsg, allocMsg + parms->rfhlength, mqmdPtr, allocMsg, datalen + parms->rfhlength, parms->rfhlength, parms);

						/* tell what we did */
						listMessage(datalen, newfptr->useFileRFH, parms);
					}
					else
					{
						/* malloc failed - issue error message and exit the program */
						Log("malloc failed for message with inserted RFH header - error 75");
						parms->err = 75;
					}
				}
				else
				{
					/* no RFH to insert - use the data in the original buffer */
					/* allocate a file pointer for this message */
					newfptr = createFileBlock(userPtr, userPtr + parms->rfhlength, mqmdPtr, allocPtr, datalen, 0, parms);

					/* make sure to only release the acquired storage once */
					allocPtr = NULL;

					/* tell what we did */
					listMessage(datalen, newfptr->useFileRFH, parms);
				}

				/* check if this is the first time through */
				if (NULL == currfptr)
				{
					/* remember the first one */
					fptr = newfptr;
				}
				else
				{
					/* chain the previous one to the new one */
					currfptr->nextfile = newfptr;
				}

				/* point to the new file block */
				currfptr = newfptr;
			} while ((remainLen > 0) && (0 == parms->err));

			/* check if we are treating a file as a group */
			if (1 == parms->fileAsGroup)
			{
				/* check if we found at least one message in the file */
				if (currfptr != NULL)
				{
					/* mark the last message in the file as last in group */
					currfptr->lastGroup = 1;
				}
			}
		}
	}
	else
	{
		printf("***** invalid data file name or zero length file %s *****\n", filename);
	}

	return fptr;
}

/********************************************/
/*                                          */
/* Routine to read the parameters file.  It */
/*  reads one line at a time.  It then      */
/*  passes each line to the appropriate     */
/*  routine for processing.  The file can   */
/*  consist of two main parts.  The first   */
/*  part is a header section and contains   */
/*  name and value pairs.  The second part  */
/*  consists of one or more data files. If  */
/*  the readfiles value is zero then the    */
/*  data files are ignored.                 */
/*                                          */
/* A chain of file blocks is created in     */
/*  memory from the data files.  A pointer  */
/*  to the first block is returned.         */
/*                                          */
/********************************************/

FILEPTR * processParmFile(char * parmFileName, PUTPARMS * parms, int readFiles)

{
	size_t	len;
	int		foundfiles=0;
	int		foundUsr=0;
	int		parmlen;
	FILE *	parmfile;
	char *	ptr;
	FILEPTR * fileptr;
	FILEPTR * tempfptr;
	FILEPTR * fptr=NULL;
	char	parmline[512];

	/* initialize the parameter input area */
	memset(parmline, 0, sizeof(parmline));

	/* set the nextfile variable to the anchor */
	fileptr = fptr;

	/* open the parameters file */
	parmfile = fopen(parmFileName, "r");

	if (parmfile != NULL)
	{
		/* read the parameter file */
		while (fgets(parmline, sizeof(parmline) - 1, parmfile) != NULL)
		{
			/* check for a new line character at the end of the line */
			parmlen = strlen(parmline);
			while ((parmlen > 0) && (parmline[parmlen - 1] < ' '))
			{
				/* get rid of the new line character */
				parmline[strlen(parmline) - 1] = 0;
				parmlen--;
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
				/* strip trailing blanks */
				rtrim(ptr);

				if ('<' == ptr[0])
				{
					/* process this as an XML data entry */
					if (0 == foundUsr)
					{
						/* must be the first entry */
						foundUsr = processFirstUsrLine(ptr, parms, readFiles);
					}
					else
					{
						/* figure out what kind of entry this belongs to */
						foundUsr = processUsrLine(ptr, foundUsr, parms, readFiles);
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
						/* should data files be ignored? */
						if (READDATAFILES == readFiles)
						{
							/* process this line as a message data file name */
							tempfptr = getFileData(ptr, parms);

							/* do we create a file block? */
							if (tempfptr != NULL)
							{
								/* is this the first one? */
								if (fptr != NULL)
								{
									/* not the first - append to the end */
									fileptr = fptr;

									/* check if we need to update our nextfile pointer */
									while (fileptr->nextfile != NULL)
									{
										fileptr = (FILEPTR *)fileptr->nextfile;
									}

									/* insert the new one at the end of the chain */
									fileptr->nextfile = tempfptr;
								}
								else
								{
									/* first file block */
									fptr = tempfptr;
								}
							}
						}
					}
				}
			}

			/* re-initialize the parameter input area */
			memset(parmline, 0, sizeof(parmline));
		}

		fclose(parmfile);
	}
	else
	{
		printf("***** Error opening parameters file *****\n");
		printf("***** Using default parameters *****\n");

		/* indicate that error occurred */
		parms->err = 1;
	}

	return fptr;
}

/********************************************************************/
/*                                                                  */
/* Routine to create a file name from a template file name and      */
/* a message number.  The message number will be inserted before    */
/* any file extension.  A file extension is considered to be        */
/* any characters that follow a period on the end of the file name. */
/*                                                                  */
/********************************************************************/

void createNextFileName(const char *fileName, char *newFileName, int fileCount)

{
	char	*ptr;
	char	tempCount[16];
	char	fileExt[756];

	/* make sure we have a file name to use as a template */
	if (strlen(fileName) == 0)
	{
		/* no file name - get out as gracefully as possible */
		newFileName[0] = 0;
		return;
	}

	/* get the file name into a work area */
	strcpy(newFileName, fileName);

	/* check for a file extension */
	ptr = newFileName + strlen(newFileName);

	/* look for a period before we find a directory                                 */
	/* the check for a slash is necessary to handle a file name with no extension   */
	/* and a directory name with a period                                           */
	while ((ptr > newFileName) && ('.' != ptr[0]) && ('\\' != ptr[0]) && ('/' != ptr[0]))
	{
		ptr--;
	}

	/* found a period? */
	if ((ptr >= newFileName) && ('.' == ptr[0]))
	{
		/* save the file extension */
		strcpy(fileExt, ptr);

		/* get rid of the period */
		ptr[0] = 0;
	}
	else
	{
		/* no file extension */
		fileExt[0] = 0;

		/* point to the end of the file name to append a number */
		ptr = newFileName + strlen(newFileName);
	}

	/* insert a file count on the end of the file name */
	memset(tempCount, 0, sizeof(tempCount));
	sprintf(tempCount, "%d", fileCount);
	strcat(ptr, tempCount);

	/* restore the file extension */
	strcat(ptr, fileExt);
}

void appendTimeStamp(PUTPARMS * parms)

{
	char	*ptr;
	char	*savePtr;
	char	*beginPtr;
	time_t	ltime;			/* number of seconds since 1/1/70     */
	struct	tm *today;		/* today's date as a structure        */
	char	timeStamp[32];
	char	fileExt[1024];

	/* check for a file extension */
	beginPtr = (char *)&(parms->outputFilename);
	ptr = beginPtr + strlen(beginPtr);
	savePtr = ptr;

	/* check if the file extension will fit */
	if (ptr - beginPtr > sizeof(parms->outputFilename) - sizeof(timeStamp))
	{
		/* produce error message */
		printf("*****File name too long to insert time stamp\n");

		/* indicate error and return */
		parms->err = 1;
		return;
	}

	/* look for a period before a directory is found */
	/* the check for a slash is necessary to handle a file name with no extension   */
	/* and a directory name with a period                                           */
	while ((ptr > beginPtr) && ('.' != ptr[0]) && ('\\' != ptr[0]) && ('/' != ptr[0]))
	{
		ptr--;
	}

	/* found a period? */
	if ((ptr >= beginPtr) && ('.' == ptr[0]))
	{
		/* save the file extension */
		strcpy(fileExt, ptr);

		/* get rid of the period */
		ptr[0] = 0;
	}
	else
	{
		/* no file extension */
		fileExt[0] = 0;

		/* point to the end of the file name to append a time stamp */
		ptr = savePtr;
	}

	/* insert a file count on the end of the file name */
	memset(timeStamp, 0, sizeof(timeStamp));
	time(&ltime);
	today = localtime(&ltime);
	strftime(timeStamp, sizeof(timeStamp) - 1, "%Y%m%dT%H%M%S", today);

	/* append the time stamp */
	strcat(ptr, timeStamp);

	/* restore the file extension */
	strcat(ptr, fileExt);
}

