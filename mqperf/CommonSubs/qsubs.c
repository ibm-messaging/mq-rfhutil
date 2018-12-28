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
/*   qsubs.c - Common MQSeries subroutines                          */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* includes for MQI */
#include <cmqc.h>
#include <cmqxc.h>

/* definitions of 64-bit values for platform independence */
#include "int64defs.h"

/* common subroutines include */
#include "comsubs.h"
#include "timesubs.h"
#include "parmline.h"
#include "qsubs.h"

#ifdef WIN32
#include <Windows.h>
#endif

#define MQMD_STRUC_ID_ASCII		0x4d, 0x44, 0x20, 0x20
#define MQMD_STRUC_ID_EBCDIC	0xd4, 0xc4, 0x40, 0x40

/**************************************************************/
/*                                                            */
/* Subroutine to check MQ results and issue an error message  */
/*  if there was an error.                                    */
/*                                                            */
/**************************************************************/

void checkerror(const char *mqcalltype, MQLONG compcode, MQLONG reason, const char *resource)

{
	if ((compcode != MQCC_OK) || (reason != MQRC_NONE))
	{
		Log("MQSeries error with %s on %s - compcode = %d, reason = %d",
				mqcalltype, resource, compcode, reason);
	}
}

/********************************************************************/
/*                                                                  */
/*  Subroutine to connect to a queue manager.                       */
/*                                                                  */
/********************************************************************/

void connect2QM(char * qmname, PMQHCONN qm, PMQLONG cc, PMQLONG reason)

{
	/* tell what we are doing */
	Log("connecting to queue manager %s",qmname);

	/* connect to the queue manager */
	MQCONN(qmname, qm, cc, reason);
	checkerror("MQCONN", *cc, *reason, qmname);
}

/********************************************************************/
/*                                                                  */
/*  Subroutine to connect to a queue manager.                       */
/*                                                                  */
/********************************************************************/

void clientConnect2QM(char * qmname, PMQHCONN qm, int *maxMsgLen, PMQLONG cc, PMQLONG reason)

{
	char	*ptr;
	char	*connName;
	int		transType=MQXPT_TCP;
	size_t	slen;				/* work variable for string lengths */
	char	mqserver[512];
	MQLONG	opencode;
	MQLONG	rc;
	MQCHAR48	name;
	MQLONG	Select[1];			/* attribute selectors           */
	MQLONG	IAV[1];				/* integer attribute values      */
	MQHOBJ	Hobj;               /* object handle                 */
	MQCHAR	charAttrs[64];		/* character attribute area      */
	MQOD	od={MQOD_DEFAULT};  /* Object Descriptor             */
	MQCD	mqcd={MQCD_CLIENT_CONN_DEFAULT};
	MQCNO	mqcno={MQCNO_DEFAULT};

	/* make a local copy of the QM name */
	mqserver[0] = 0;

	/* make sure the source string is not too long */
	if (strlen(qmname) < sizeof(mqserver) - 1)
	{
		strcpy(mqserver, qmname);
	}

	ptr = strchr(mqserver, '/');
	if (NULL == ptr)
	{
		/* no forward slash in the name - must be a queue manager name */
		/* connect to the queue manager */
		/* tell what we are doing */
		Log("connecting to %s",qmname);

		MQCONN(qmname, qm, cc, reason);
		checkerror("MQCONN", *cc, *reason, qmname);
	}
	else
	{
		ptr[0] = 0;
		ptr++;

		/* look for second slash */
		connName = strchr(ptr, '/');
		if (connName != NULL)
		{
			connName[0] = 0;
			connName++;
		}

		/* get the transport type */
		if (strcmp(ptr, "TCP") == 0)
		{
			transType = MQXPT_TCP;
		}
		else if (strcmp(ptr, "LOCAL") == 0)
		{
			transType = MQXPT_LOCAL;
		}
		else if (strcmp(ptr, "LU62") == 0)
		{
			transType = MQXPT_LU62;
		}
		else if (strcmp(ptr, "NETBIOS") == 0)
		{
			transType = MQXPT_NETBIOS;
		}
		else if (strcmp(ptr, "SPX") == 0)
		{
			transType = MQXPT_SPX;
		}
		else if (strcmp(ptr, "DECNET") == 0)
		{
			transType = MQXPT_DECNET;
		}
		else if (strcmp(ptr, "UDP") == 0)
		{
			transType = MQXPT_UDP;
		}
		else
		{
			/* did not recognize the transport type */
			Log("Transport type not recognized - %s", ptr);
		}

		/* now build a channel entry */
		memset(mqcd.ChannelName, ' ', sizeof(mqcd.ChannelName));
		slen = strlen(mqserver);
		if (slen <= sizeof(mqcd.ChannelName))
		{
			memcpy(mqcd.ChannelName, mqserver, slen);
		}
		else
		{
			/* issue error message */
			Log("Channel name is too long - length=%d", slen);
		}

		memset(mqcd.ConnectionName, 0, sizeof(mqcd.ConnectionName));

		/* make sure that the connection name was found */
		if (connName != NULL)
		{
			slen = strlen(connName);
			if (slen <= sizeof(mqcd.ConnectionName))
			{
				memcpy(mqcd.ConnectionName, connName, slen);
			}
			else
			{
				/* issue error message */
				Log("Connection name is too long - length=%d", slen);
			}
		}
		else
		{
			/* issue error message */
			Log("Connection name not found");
		}

		/* set the transport type */
		mqcd.TransportType = transType;

		/* clear the queue manager name */
		/*memset(mqcd.QMgrName, 0, sizeof(mqcd.QMgrName));*/

		/* point to the channel definition */
		mqcno.ClientConnPtr = &mqcd;

		/* set the options */
		mqcno.Options = MQCNO_NONE;

		/* use a lower level for backwards compatibility (version 2 vs version 4) */
		mqcno.Version = MQCNO_VERSION_2;

		/* set version 4 instead of 7 for better backwards compatibility */
		mqcd.Version = MQCD_VERSION_4;
		mqcd.StrucLength = MQCD_LENGTH_4;

		/* was a maximum message length specified? */
		if (0 == maxMsgLen)
		{
			/* set the maximum message length to 4MB */
			mqcd.MaxMsgLength = 4 * 1024 * 1024;
		}
		else
		{
			mqcd.MaxMsgLength = (*maxMsgLen);
		}

		/* try to connect to the queue manager */
		memset(name, ' ', sizeof(name));
		name[0] = 0;

		/* tell what we are doing */
		Log("connecting using %s on channel %s at %s",ptr, mqserver, connName);
		MQCONNX(name, &mqcno, qm, cc, reason);
		checkerror("MQCONNX", *cc, *reason, qmname);

		if (MQCC_OK == *cc)
		{
			/* try to get the queue manager name and display it */
			/* Open a qmgr object */
			od.ObjectType = MQOT_Q_MGR;		/* open the queue manager object*/
			MQOPEN((*qm),                      /* connection handle            */
				   &od,                     /* object descriptor for queue  */
				   MQOO_INQUIRE +           /* open it for inquire          */
				   MQOO_FAIL_IF_QUIESCING,  /* but not if MQM stopping      */
				   &Hobj,                   /* object handle                */
				   &opencode,               /* MQOPEN completion code       */
				   &rc);					/* reason code                  */

			if (MQCC_OK == opencode)
			{
				memset(charAttrs, 0, sizeof(charAttrs));
				Select[0] = MQCA_Q_MGR_NAME;
				MQINQ((*qm),
					  Hobj,
					  1L,
					  Select,
					  0L,
					  NULL,
					  sizeof(charAttrs),
					  charAttrs,
					  &opencode,
					  &rc);

				if (MQCC_OK == opencode)
				{
					Log("Connected to %s", charAttrs);
				}

				/* try to get the maximum message length allowed */
				/* to avoid 2010 reason codes on MQGETs          */
				Select[0] = MQIA_MAX_MSG_LENGTH;
				IAV[0]=0;
				MQINQ((*qm),
					  Hobj,
					  1L,
					  Select,
					  1L,
					  IAV,
					  0L,
					  NULL,
					  &opencode,
					  &rc);

				if (MQCC_OK == opencode)
				{
					/* check if our maximum length is too large */
					if (((*maxMsgLen) > IAV[0]) && (IAV[0] > 0))
					{
						/* max message length is too large, so reduce it */
						(*maxMsgLen) = IAV[0];
					}
				}

				MQCLOSE((*qm),
						&Hobj,
						MQCO_NONE,
						&opencode,
						&rc);
			}
		}
	}
}

/**************************************************************/
/*                                                            */
/* Subroutine to format a time.                               */
/*                                                            */
/* The input time should be 8 numbers (hhmmsshh)              */
/*                                                            */
/**************************************************************/

void formatTime(char *timeOut, char *timeIn)

{
	timeOut[0] = timeIn[0];
	timeOut[1] = timeIn[1];
	timeOut[2] = ':';
	timeOut[3] = timeIn[2];
	timeOut[4] = timeIn[3];
	timeOut[5] = ':';
	timeOut[6] = timeIn[4];
	timeOut[7] = timeIn[5];
	timeOut[8] = ':';
	timeOut[9] = timeIn[6];
	timeOut[10] = timeIn[7];
	timeOut[11] = 0;
}

void issueReply(MQHCONN qm, MQLONG	report, int *uow, char * msgId, PUTPARMS * parms)

{
	MQHOBJ	replyq=0;
	MQOD	replyObjdesc = {MQOD_DEFAULT};
	MQMD	replyMsgdesc = {MQMD_DEFAULT};
	MQLONG	openopt = 0;
	MQLONG	compcode;
	MQLONG	reason;
	MQPMO	pmo = {MQPMO_DEFAULT};

	/* set the open options */
	strncpy(replyObjdesc.ObjectName, parms->replyQname, MQ_Q_NAME_LENGTH);
	strncpy(replyObjdesc.ObjectQMgrName, parms->replyQMname, MQ_Q_MGR_NAME_LENGTH);
	openopt = MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING;

	/* open the reply to queue to */
	MQOPEN(qm, &replyObjdesc, openopt, &replyq, &compcode, &reason);
	checkerror("MQOPEN(replyQ)", compcode, reason, parms->replyQname);

	/* check if the open worked */
	if (MQCC_OK == compcode)
	{
		/* set the fields in the MQMD for the reply message */
		replyMsgdesc.MsgType = MQMT_REPORT;
		memcpy(replyMsgdesc.CorrelId, msgId, sizeof(replyMsgdesc.CorrelId));

		/* perform the put operation in a syncpoint */
		pmo.Options = MQPMO_SYNCPOINT | MQPMO_NEW_MSG_ID;

		/* write the report message */
		if (((report & MQRO_PAN) > 0) && (parms->fileDataPAN != NULL))
		{
			/* check if the data has an RFH */
			if ((parms->fileSizePAN > 32) && (memcmp(parms->fileDataPAN, "RFH ", 4) == 0))
			{
				memcpy(replyMsgdesc.Format, "MQHRF2  ", 8);
			}

			MQPUT(qm, replyq, &replyMsgdesc, &pmo, parms->fileSizePAN, parms->fileDataPAN, &compcode, &reason);
		}
		else
		{
			/* check if the data has an RFH */
			if ((parms->fileSizeNAN > 32) && (memcmp(parms->fileDataNAN, "RFH ", 4) == 0))
			{
				memcpy(replyMsgdesc.Format, "MQHRF2  ", 8);
			}

			MQPUT(qm, replyq, &replyMsgdesc, &pmo, parms->fileSizeNAN, parms->fileDataNAN, &compcode, &reason);
		}

		checkerror("MQOPEN(reply)", compcode, reason, parms->replyQname);

		/* force a syncpoint now */
		MQCMIT(qm, &compcode, &reason);
		checkerror("MQCMIT(reply)", compcode, reason, parms->qmname);
		(*uow) = 0;

		/* close the queue */
		MQCLOSE(qm, &replyq, MQCO_NONE , &compcode, &reason);
		checkerror("MQCLOSE(reply)", compcode, reason, parms->replyQname);

	}
}

void translateMQMD(void * mqmdPtr, int datalen)

{
	int		v1r;											/* version 1 with bytes reversed */
	int		v2r;											/* version 2 with bytes reversed */
	MQMD2	*mqmd=(MQMD2 *)mqmdPtr;							/* pointer to MQMD */
	unsigned char	ebcdicID[4]={MQMD_STRUC_ID_EBCDIC};		/* MQMD structure ID field in EBCDIC */
	unsigned char	asciiID[4]={MQMD_STRUC_ID_ASCII};		/* MQMD structure ID field in ASCII */
	unsigned char	nativeID[4]={MQMD_STRUC_ID_ARRAY};		/* MQMD structure ID field in native characters */

	if (0 == checkMQMD(mqmdPtr, datalen))
	{
		/* not an MQMD - just return */
		return;
	}

	/* create MQMD version in a opposite encoding sequence */
	v1r = reverseBytes4(MQMD_VERSION_1);
	v2r = reverseBytes4(MQMD_VERSION_2);

	/* check if translation is necessary */
	if (memcmp(mqmd->StrucId, nativeID, 4) != 0)
	{
		/* must translate */
		/* check if the MQMD is in ebcdic */
		if (memcmp(mqmd->StrucId, ebcdicID, 4) == 0)
		{
			/* convert the character fields from EBCDIC to ASCII */
			EbcdicToAscii((unsigned char *)&mqmd->Format, sizeof(mqmd->Format), (unsigned char *)&mqmd->Format);
			EbcdicToAscii((unsigned char *)&mqmd->ReplyToQ, sizeof(mqmd->ReplyToQ), (unsigned char *)&mqmd->ReplyToQ);
			EbcdicToAscii((unsigned char *)&mqmd->ReplyToQMgr, sizeof(mqmd->ReplyToQMgr), (unsigned char *)&mqmd->ReplyToQMgr);
			EbcdicToAscii((unsigned char *)&mqmd->UserIdentifier, sizeof(mqmd->UserIdentifier), (unsigned char *)&mqmd->UserIdentifier);
			EbcdicToAscii((unsigned char *)&mqmd->ApplIdentityData, sizeof(mqmd->ApplIdentityData), (unsigned char *)&mqmd->ApplIdentityData);
			EbcdicToAscii((unsigned char *)&mqmd->PutApplName, sizeof(mqmd->PutApplName), (unsigned char *)&mqmd->PutApplName);
			EbcdicToAscii((unsigned char *)&mqmd->PutDate, sizeof(mqmd->PutDate), (unsigned char *)&mqmd->PutDate);
			EbcdicToAscii((unsigned char *)&mqmd->PutTime, sizeof(mqmd->PutTime), (unsigned char *)&mqmd->PutTime);
			EbcdicToAscii((unsigned char *)&mqmd->ApplOriginData, sizeof(mqmd->ApplOriginData), (unsigned char *)&mqmd->ApplOriginData);
		}
		else if (memcmp(mqmd->StrucId, asciiID, 4) == 0)
		{
			/* convert the character fields from EBCDIC to ASCII */
			AsciiToEbcdic((unsigned char *)&mqmd->Format, (unsigned char *)&mqmd->Format, sizeof(mqmd->Format));
			AsciiToEbcdic((unsigned char *)&mqmd->ReplyToQ, (unsigned char *)&mqmd->ReplyToQ, sizeof(mqmd->ReplyToQ));
			AsciiToEbcdic((unsigned char *)&mqmd->ReplyToQMgr, (unsigned char *)&mqmd->ReplyToQMgr, sizeof(mqmd->ReplyToQMgr));
			AsciiToEbcdic((unsigned char *)&mqmd->UserIdentifier, (unsigned char *)&mqmd->UserIdentifier, sizeof(mqmd->UserIdentifier));
			AsciiToEbcdic((unsigned char *)&mqmd->ApplIdentityData, (unsigned char *)&mqmd->ApplIdentityData, sizeof(mqmd->ApplIdentityData));
			AsciiToEbcdic((unsigned char *)&mqmd->PutApplName, (unsigned char *)&mqmd->PutApplName, sizeof(mqmd->PutApplName));
			AsciiToEbcdic((unsigned char *)&mqmd->PutDate, (unsigned char *)&mqmd->PutDate, sizeof(mqmd->PutDate));
			AsciiToEbcdic((unsigned char *)&mqmd->PutTime, (unsigned char *)&mqmd->PutTime, sizeof(mqmd->PutTime));
			AsciiToEbcdic((unsigned char *)&mqmd->ApplOriginData, (unsigned char *)&mqmd->ApplOriginData, sizeof(mqmd->ApplOriginData));
		}
	}

	if ((v1r == mqmd->Version) || (v2r == mqmd->Version))
	{
		/* it appears the mqmd was captured on a system with different encoding */
		/* the byte ordering in the mqmd will be reversed */
		mqmd->Version = reverseBytes4(mqmd->Version); 
		mqmd->Report = reverseBytes4(mqmd->Report);
		mqmd->MsgType = reverseBytes4(mqmd->MsgType); 
		mqmd->Expiry = reverseBytes4(mqmd->Expiry);
		mqmd->Feedback = reverseBytes4(mqmd->Feedback); 
		mqmd->Encoding = reverseBytes4(mqmd->Encoding);
		mqmd->CodedCharSetId = reverseBytes4(mqmd->CodedCharSetId); 
		mqmd->Priority = reverseBytes4(mqmd->Priority);
		mqmd->Persistence = reverseBytes4(mqmd->Persistence);
		mqmd->BackoutCount = reverseBytes4(mqmd->BackoutCount);
		mqmd->PutApplType = reverseBytes4(mqmd->PutApplType);

		/* check if this is a version 2 mqmd */
		if (MQMD_VERSION_2 == mqmd->Version)
		{
			/* deal with the message sequence, etc */
			mqmd->MsgSeqNumber = reverseBytes4(mqmd->MsgSeqNumber);
			mqmd->Offset = reverseBytes4(mqmd->Offset);
			mqmd->MsgFlags = reverseBytes4(mqmd->MsgFlags);
			mqmd->OriginalLength = reverseBytes4(mqmd->OriginalLength);
		}
	}
}

int checkMQMD(void * mqmdPtr, int datalen)

{
	int		rc=0;
	int		v1r;											/* version 1 with bytes reversed */
	int		v2r;											/* version 2 with bytes reversed */
	MQMD2	*mqmd=(MQMD2 *)mqmdPtr;							/* pointer to MQMD */
	unsigned char	ebcdicID[4]={MQMD_STRUC_ID_EBCDIC};		/* MQMD structure ID field in EBCDIC */
	unsigned char	asciiID[4]={MQMD_STRUC_ID_ASCII};		/* MQMD structure ID field in ASCII */

	/* is the data length long enough to contain an MQMD? */
	if (datalen < sizeof(MQMD))
	{
		/* not enough data for a V1 MQMD - cannot be an MQMD */
		return rc;
	}

	/* check for the MQMD eye catcher */
	if ((memcmp(mqmd->StrucId, asciiID, 4) != 0) && (memcmp(mqmd->StrucId, ebcdicID, 4) != 0))
	{
		/* did not find the eyecatcher - return */
		return rc;
	}

	/* check if the MQMD is in a different encoding sequence */
	v1r = reverseBytes4(MQMD_VERSION_1);
	v2r = reverseBytes4(MQMD_VERSION_2);

	/* check for either version 1 or version 2 */
	if ((mqmd->Version == v2r) || (MQMD_VERSION_2 == mqmd->Version))
	{
		/* version 2 MQMD */
		rc = MQMD_VERSION_2;
	} else if ((mqmd->Version == v1r) || (MQMD_VERSION_1 == mqmd->Version))
	{
		/* Version 1 MQMD */
		rc = MQMD_VERSION_1;
	}

	/* return MQMD version or 0 if not recognized */
	return rc;
}

/*****************************************************/
/*                                                   */
/* Routine to recognize an MQMD.                     */
/*                                                   */
/* This routine checks for either an ASCII or        */
/* EBCDIC eye catcher as well as a version number.   */
/* The version number can be in either big-endian or */
/* little-endian binary.  If it is not the normal    */
/* representation for the platform then the MQMD     */
/* will be converted.                                */
/*                                                   */
/* Returns a zero ('0') if no MQMD is found and a    */
/* one ('1') if an MQMD is found.  The MQMD will be  */
/* converted if necessary.                           */
/*                                                   */
/*****************************************************/

int checkAndXlateMQMD(void * mqmdPtr, int length)

{
	int				result=0;								/* did not find MQMD */

	/* check for an MQMD */
	if (0 == checkMQMD(mqmdPtr, length))
	{
		/* not an MQMD - just return */
		return result;
	}

	/* found an MQMD */
	result = 1;

	/* translate the MQMD if necessary */
	translateMQMD(mqmdPtr, length);

	/* return the result */
	return result;
}

void setContext(MQMD2 * mqmd)

{
	size_t			len=0;
	char			*user;
	time_t			today;
	char			dateTime[32];

	/* get todays date and time */
	time(&today);
	memset(&dateTime, 0, sizeof(dateTime));
	strftime(dateTime, sizeof(dateTime), "%Y%m%d%H%M%S00", localtime(&today));

	/* set the date and time in the MQMD */
	memcpy(&(mqmd->PutDate), dateTime, sizeof(mqmd->PutDate));
	memcpy(&(mqmd->PutTime), dateTime + 8, sizeof(mqmd->PutTime));

	/* set the application origin to spaces */
	memset(&(mqmd->ApplOriginData), ' ', sizeof(mqmd->ApplOriginData));

	/* set the application name */
#ifdef NOTUNE
	memcpy(&(mqmd->PutApplName), "mqputs", 6);
#else
	memcpy(&(mqmd->PutApplName), "mqput2", 6);
#endif

#ifdef WIN32
	/* set the application type to Windows */
	mqmd->PutApplType = MQAT_WINDOWS;

	/* get pointer to the user id */
	user = getenv("USERNAME");
#else
	/* set the application type to Unix */
	mqmd->PutApplType = MQAT_UNIX;

	/* get the user id */
	user = getenv("USER");
#endif

	/* make sure it worked */
	if (user != NULL)
	{
		/* find out how long the user id is */
		len = strlen(user);
		if (len > sizeof(mqmd->UserIdentifier))
		{
			/* limit the number of bytes copied */
			len = sizeof(mqmd->UserIdentifier);
		}

		/* copy in the user identifier */
		memcpy(&(mqmd->UserIdentifier), user, len);
	}
}
