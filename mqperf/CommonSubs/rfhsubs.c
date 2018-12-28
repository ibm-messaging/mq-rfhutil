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
/*   rfhsubs.c - Common subroutines for processing RFH headers.     */
/*                                                                  */
/********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

/* includes for MQI */
#include <cmqc.h>

/* definitions of 64-bit values for platform independence */
#include "int64defs.h"

/* common subroutines include */
#include "comsubs.h"
#include "timesubs.h"

#include "parmline.h"
#include "rfhsubs.h"

#define CHAR_ASCII		0
#define CHAR_EBCDIC		1

#define RFH2_JMS_DST_BEGIN "<Dst>"
#define RFH2_JMS_DST_END "</Dst>"
#define RFH2_JMS_RTO_BEGIN "<Rto>"
#define RFH2_JMS_RTO_END "</Rto>"
#define RFH2_JMS_PRI_BEGIN "<Pri>"
#define RFH2_JMS_PRI_END "</Pri>"
#define RFH2_JMS_DLV_BEGIN "<Dlv>"
#define RFH2_JMS_DLV_END "</Dlv>"
#define RFH2_JMS_EXP_BEGIN "<Exp>"
#define RFH2_JMS_EXP_END "</Exp>"
#define RFH2_JMS_CID_BEGIN "<Cid>"
#define RFH2_JMS_CID_END "</Cid>"
#define RFH2_JMS_GID_BEGIN "<Gid>"
#define RFH2_JMS_GID_END "</Gid>"
#define RFH2_JMS_SEQ_BEGIN "<Seq>"
#define RFH2_JMS_SEQ_END "</Seq>"

#define JMS_TEXT		"jms_text"
#define JMS_BYTES		"jms_bytes"
#define JMS_OBJECT		"jms_object"
#define JMS_STREAM		"jms_stream"
#define JMS_MAP			"jms_map"
#define JMS_NONE		"jms_none"

#define MQRFH_STRUC_ID_ASCII	0x52, 0x46, 0x48, 0x20
#define MQRFH_STRUC_ID_EBCDIC	0xd9, 0xc6, 0xc8, 0x40

/**************************************************************/
/*                                                            */
/* Routine to check for an rfh in the file data. This         */
/*  routine checks the first four characters for an RFH       */
/*  structure id in either ASCII or EBCDIC, and then checks   */
/*  the next four bytes for either a binary 1 or 2, trying    */
/*  both normal and reversed integers.                        */
/*                                                            */
/**************************************************************/

int isRFH(unsigned char * data, size_t length, size_t *rfhLen)

{
	size_t	len=0;
	int		rc=0;
	int		v1r;											/* version 1 with bytes reversed */
	int		v2r;											/* version 2 with bytes reversed */
	MQRFH2	*rfh=(MQRFH2 *)data;							/* pointer to RFH */
	unsigned char	ebcdicID[4]={MQRFH_STRUC_ID_EBCDIC};	/* RFH structure ID field in EBCDIC */
	unsigned char	asciiID[4]={MQRFH_STRUC_ID_ASCII};		/* RFH structure ID field in ASCII */

	/* set the RFH length to 0 */
	(*rfhLen) = 0;

	/* is the data length long enough to contain an MQMD? */
	if (length < sizeof(MQRFH))
	{
		/* not enough data for a V1 RFH header - too short */
		return rc;
	}

	/* check for the RFH eye catcher */
	if ((memcmp(rfh->StrucId, asciiID, 4) != 0) && (memcmp(rfh->StrucId, ebcdicID, 4) != 0))
	{
		/* did not find the eyecatcher - return */
		return rc;
	}

	/* check if the RFH is in a different encoding sequence */
	v1r = reverseBytes4(MQRFH_VERSION_1);
	v2r = reverseBytes4(MQRFH_VERSION_2);

	/* check for either version 1 or version 2 */
	if ((rfh->Version == v2r) || (MQRFH_VERSION_2 == rfh->Version))
	{
		/* version 2 RFH */
		rc = MQRFH_VERSION_2;

		if (MQRFH_VERSION_2 == rfh->Version)
		{
			len = rfh->StrucLength;
		}
		else
		{
			len = reverseBytes4(rfh->StrucLength);
		}
	} else if ((rfh->Version == v1r) || (MQRFH_VERSION_1 == rfh->Version))
	{
		/* Version 1 RFH */
		rc = MQRFH_VERSION_1;

		if (MQRFH_VERSION_1 == rfh->Version)
		{
			len = rfh->StrucLength;
		}
		else
		{
			len = reverseBytes4(rfh->StrucLength);
		}
	}

	/* make sure the length makes sense */
	if ((len < sizeof(MQRFH)) || (len > length))
	{
		/* length is invalid */
		rc = 0;
	}
	else
	{
		/* reasonable structure length - return it */
		(*rfhLen) = len;
	}

	/* return RFH version or 0 if not recognized */
	return rc;
}

void translateRFH(unsigned char * data, size_t length)

{
	size_t	len;
	int		v1r;											/* version 1 with bytes reversed */
	int		v2r;											/* version 2 with bytes reversed */
	MQRFH2	*rfh=(MQRFH2 *)data;							/* pointer to RFH */
	unsigned char	ebcdicID[4]={MQRFH_STRUC_ID_EBCDIC};	/* RFH structure ID field in EBCDIC */
	unsigned char	asciiID[4]={MQRFH_STRUC_ID_ASCII};		/* RFH structure ID field in ASCII */
	unsigned char	nativeID[4]={MQMD_STRUC_ID_ARRAY};		/* MQMD structure ID field in native characters */

	if (0 == isRFH(data, length, &len))
	{
		/* not an RFH - just return */
		return;
	}

	/* create MQMD version in a opposite encoding sequence */
	v1r = reverseBytes4(MQRFH_VERSION_1);
	v2r = reverseBytes4(MQRFH_VERSION_2);

	/* check if translation is necessary */
	if (memcmp(rfh->StrucId, nativeID, 4) != 0)
	{
		/* must translate */
		/* check if the MQMD is in ebcdic */
		if (memcmp(rfh->StrucId, ebcdicID, 4) == 0)
		{
			/* convert the character fields from EBCDIC to ASCII */
			EbcdicToAscii((unsigned char *)&rfh->StrucId, sizeof(rfh->StrucId), (unsigned char *)&rfh->StrucId);
			EbcdicToAscii((unsigned char *)&rfh->Format, sizeof(rfh->Format), (unsigned char *)&rfh->Format);
		}
		else if (memcmp(rfh->StrucId, asciiID, 4) == 0)
		{
			/* convert the character fields from EBCDIC to ASCII */
			AsciiToEbcdic((unsigned char *)&rfh->StrucId, (unsigned char *)&rfh->StrucId, sizeof(rfh->StrucId));
			AsciiToEbcdic((unsigned char *)&rfh->Format, (unsigned char *)&rfh->Format, sizeof(rfh->Format));
		}
	}

	if ((v1r == rfh->Version) || (v2r == rfh->Version))
	{
		/* it appears the rfh was captured on a system with different encoding */
		/* the byte ordering in the mqmd will be reversed */
		rfh->Version = reverseBytes4(rfh->Version); 
		rfh->StrucLength = reverseBytes4(rfh->StrucLength);
		rfh->Encoding = reverseBytes4(rfh->Encoding);
		rfh->CodedCharSetId = reverseBytes4(rfh->CodedCharSetId); 
		rfh->Flags = reverseBytes4(rfh->Flags); 

		/* check if this is a version 2 mqmd */
		if (MQRFH_VERSION_2 == rfh->Version)
		{
			/* deal with the message sequence, etc */
			rfh->NameValueCCSID = reverseBytes4(rfh->NameValueCCSID);
		}
	}
}

void releaseRFH(PUTPARMS * parms)

{
	if (parms->rfh_usr != NULL)
	{
		/* check if this was allocated or the pointer is to a static area of memory */
		if (0 == parms->timeStampUserProp)
		{
			free(parms->rfh_usr);
		}

		parms->rfh_usr = NULL;
		parms->max_usr = 0;
	}

	if (parms->rfh_jms != NULL)
	{
		free(parms->rfh_jms);
		parms->rfh_jms = NULL;
		parms->max_jms = 0;
	}

	if (parms->rfh_mcd != NULL)
	{
		free(parms->rfh_mcd);
		parms->rfh_mcd = NULL;
		parms->max_mcd = 0;
	}

	if (parms->rfh_psc != NULL)
	{
		free(parms->rfh_psc);
		parms->rfh_psc = NULL;
		parms->max_psc = 0;
	}

	if (parms->rfh_pscr != NULL)
	{
		free(parms->rfh_pscr);
		parms->rfh_pscr = NULL;
		parms->max_pscr = 0;
	}
}

/**************************************************************/
/*                                                            */
/* Routine to replace certain special characters with their   */
/* respective escape sequences, as follows:                   */
/*                                                            */
/* & &amp;                                                    */
/* " &quot;                                                   */
/* < &gt;                                                     */
/* > &lt;                                                     */
/* ' &apos;                                                   */
/*                                                            */
/**************************************************************/

void replaceChars(const char *value, char *valStr)

{
	size_t		i=0;
	size_t		j=0;
	size_t		len;

	/* get the total length of the input string */
	len = strlen(value);

	while (i < len)
	{
		switch (value[i])
		{
		case '&':
			{
				valStr[j++] = '&';
				valStr[j++] = 'a';
				valStr[j++] = 'm';
				valStr[j++] = 'p';
				valStr[j++] = ';';
				break;
			}
		case '<':
			{
				valStr[j++] = '&';
				valStr[j++] = 'l';
				valStr[j++] = 't';
				valStr[j++] = ';';
				break;
			}
		case '>':
			{
				valStr[j++] = '&';
				valStr[j++] = 'g';
				valStr[j++] = 't';
				valStr[j++] = ';';
				break;
			}
		case '"':
			{
				valStr[j++] = '&';
				valStr[j++] = 'q';
				valStr[j++] = 'u';
				valStr[j++] = 'o';
				valStr[j++] = 't';
				valStr[j++] = ';';
				break;
			}
		case '\'':
			{
				valStr[j++] = '&';
				valStr[j++] = 'a';
				valStr[j++] = 'p';
				valStr[j++] = 'o';
				valStr[j++] = 's';
				valStr[j++] = ';';
				break;
			}
		default:
			{
				valStr[j++] = value[i];
			}
		}

		i++;
	}

	/* terminate the result string */
	valStr[j] = 0;
}

/**************************************************************/
/*                                                            */
/* Routine to determine if a code page is ASCII or EBCDIC     */
/*                                                            */
/**************************************************************/

int getCcsidType(const int ccsid)

{
	int	result= CHAR_ASCII;

	if ((ccsid == 037) || (ccsid == 500) ||
		(ccsid == 924) || (ccsid == 1140) ||
		(ccsid == 273) || (ccsid == 1141) ||
		(ccsid == 273) || (ccsid == 1142) ||
		(ccsid == 277) || (ccsid == 1143) ||
		(ccsid == 278) || (ccsid == 1144) ||
		(ccsid == 284) || (ccsid == 1145) ||
		(ccsid == 285) || (ccsid == 1146) ||
		(ccsid == 297) || (ccsid == 1147) ||
		(ccsid == 871) || (ccsid == 1148) ||
		(ccsid == 870) || (ccsid == 1149))
	{
		result = CHAR_EBCDIC;
	}

	return result;
}

/**************************************************************/
/*                                                            */
/* Routine to construct an RFH V1 header.                     */
/*                                                            */
/**************************************************************/

int buildRFH1(char * rfhdata, PUTPARMS *parms)

{
	int				varlength;
	int				extra;
	MQRFH			tempRFH;
	char			tempfield[1024];

	/* get a pointer to the area to build the non-fixed part of the RFH */
	memset(tempfield, 0, sizeof(tempfield));

	/* move in the identifier */
	strcpy(tempfield, MQNVS_APPL_TYPE);

	/* get the application group name */
	strcat(tempfield, parms->rfhappgroup);
	strcat(tempfield, " ");

	/* next, copy in the message type name */
	strcat(tempfield, MQNVS_MSG_TYPE);

	/* get the message type into a string */
	strcat(tempfield, parms->rfhformat);

	/* calculate the length of the variable part of the rfh */
	varlength = iStrLen(tempfield);

	/* Round it up to a multiple of 4 if necessary */
	if ((varlength % 4) > 0)
	{
		extra = 4 - (varlength % 4);
		varlength += extra;
	}

	/* were any extra bytes required to round to a multiple of 4 */
	while (extra > 0)
	{
		/* append the extra characters */
		strcat(tempfield, " ");
		extra--;
	}

	/* Need to translate the variable rfh data to EBCDIC? */
	if (getCcsidType(parms->rfhccsid) == CHAR_ASCII)
	{
		/* ASCII, no need to translate */
		memcpy(rfhdata + MQRFH_STRUC_LENGTH_FIXED, tempfield, varlength);
	}
	else
	{
		/* translate to EBCDIC */
		AsciiToEbcdic((unsigned char *)rfhdata + MQRFH_STRUC_LENGTH_FIXED, (unsigned char *)tempfield, varlength);
	}

	/* initialize the header */
	tempRFH.Version = MQRFH_VERSION_1;
	tempRFH.StrucLength = MQRFH_STRUC_LENGTH_FIXED + varlength;
	tempRFH.Flags = MQRFH_NONE;

	/* was the RFH encoding field specified? */
	if (parms->rfhencoding > 0)
	{
		/* set the encoding for the user data */
		tempRFH.Encoding = parms->rfhencoding;
	}
	else
	{
		/* use a default if not set */
		tempRFH.Encoding = MQENC_NATIVE;
	}

	/* get the code page of the user data */
	if (parms->rfhccsid > 0)
	{
		tempRFH.CodedCharSetId = parms->rfhccsid;
	}
	else
	{
		/* use a default if not set */
		tempRFH.CodedCharSetId = MQCCSI_INHERIT;
	}

	/* find out if the character set is EBCDIC */
	if (getCcsidType(parms->codepage) == CHAR_ASCII)
	{
		/* ASCII - no need to translate */
		memcpy(tempRFH.StrucId, MQRFH_STRUC_ID, sizeof(tempRFH.StrucId));
		memcpy(tempRFH.Format, parms->msgformat, MQ_FORMAT_LENGTH);
	}
	else
	{
		/* EBCDIC - must translate from ASCII */
		AsciiToEbcdic((unsigned char *)tempRFH.StrucId, (unsigned char *)MQRFH_STRUC_ID, sizeof(tempRFH.StrucId));
		AsciiToEbcdic((unsigned char *)tempRFH.Format, (unsigned char *)&(parms->msgformat), MQ_FORMAT_LENGTH);
	}

	/* check if the encoding is host integers */
#if defined WIN32 || defined __i386__
	if ((parms->encoding & MQENC_INTEGER_REVERSED) == 0)
#else
	if ((parms->encoding & MQENC_INTEGER_REVERSED) != 0)
#endif
	{
		/* we need to reverse the integers in all of the RFH header fields */
		tempRFH.Version        = reverseBytes4(tempRFH.Version);
		tempRFH.StrucLength    = reverseBytes4(tempRFH.StrucLength);
		tempRFH.Encoding       = reverseBytes4(tempRFH.Encoding);
		tempRFH.CodedCharSetId = reverseBytes4(tempRFH.CodedCharSetId);
		tempRFH.Flags          = reverseBytes4(tempRFH.Flags);
	}

	/* copy the fixed portion of the header to the data area */
	memcpy(rfhdata, (char *) &tempRFH, MQRFH_STRUC_LENGTH_FIXED);

	/* return the length of the RFH header */
	return varlength + MQRFH_STRUC_LENGTH_FIXED;
}

unsigned int roundVarArea(char * varArea)

{
	unsigned int	extra;
	unsigned int	varlen;

	varlen = iStrLen(varArea);

	/* round to a multiple of four bytes */
	/* check if the length is already a multiple of 4 */
	extra = varlen % 4;
	if (extra > 0)
	{
		/* not a multiple of four - calculate bytes that must be added */
		extra = 4 - extra;
		varlen += extra;

		/* append the extra blanks to make a multiple of 4 */
		while (extra > 0)
		{
			strcat(varArea, " ");
			extra--;
		}
	}

	return varlen;
}

void setVarLength(void * ptr, int len, int encoding)

{
	/* set the binary length field at the front of the data */
#if defined WIN32 || defined __i386__
	if ((encoding & MQENC_INTEGER_REVERSED) == 0)
#else
	if ((encoding & MQENC_INTEGER_REVERSED) != 0)
#endif
	{
		/* we need to reverse the integers in all of the RFH header fields */
		len = reverseBytes4(len);
	}

	memcpy(ptr, (const void *) &len, 4);
}

unsigned int appendFolder(char * ptr, char * folder, int encoding)

{
	unsigned int	len = 0;

	/* check for explicit folder definiion */
	if (folder != NULL)
	{
		strcat(ptr + 4, folder);				/* append the folder data                             */
		len = roundVarArea(ptr + 4);			/* get the total length of the folder                 */
		setVarLength(ptr, len, encoding);		/* set the length field including 4 byte length field */
		len += 4;								/* remember the length of the length field            */
	}

	/* return the length of the folder */
	return len;
}

void buildJMS(PUTPARMS *parms)

{
	char			*varArea;
	char			tempValue[256];
	char			tempfield[1024];

	/* get a pointer to the area to use */
	varArea = parms->rfh_jms;

	tempfield[0] = 0;

	if (strlen(parms->rfh_jms_dest) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_JMS_DST_BEGIN);

		/* get the message domain name and append it to the XML message */
		replaceChars(parms->rfh_jms_dest, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_JMS_DST_END);
	}

	if (strlen(parms->rfh_jms_reply) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_JMS_RTO_BEGIN);

		/* get the message domain name and append it to the XML message */
		replaceChars(parms->rfh_jms_reply, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_JMS_RTO_END);
	}

	if (strlen(parms->rfh_jms_correlid) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_JMS_CID_BEGIN);

		/* get the message domain name and append it to the XML message */
		replaceChars(parms->rfh_jms_correlid, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_JMS_CID_END);
	}

	if (strlen(parms->rfh_jms_groupid) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_JMS_GID_BEGIN);

		/* get the message domain name and append it to the XML message */
		replaceChars(parms->rfh_jms_groupid, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_JMS_GID_END);
	}

	if (strlen(parms->rfh_jms_expire) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_JMS_EXP_BEGIN);

		/* get the message domain name and append it to the XML message */
		replaceChars(parms->rfh_jms_groupid, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_JMS_EXP_END);
	}

	if (strlen(parms->rfh_jms_delmode) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_JMS_DLV_BEGIN);

		/* get the message domain name and append it to the XML message */
		replaceChars(parms->rfh_jms_delmode, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_JMS_DLV_END);
	}

	if (strlen(parms->rfh_jms_seq) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_JMS_SEQ_BEGIN);

		/* get the message domain name and append it to the XML message */
		replaceChars(parms->rfh_jms_seq, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_JMS_SEQ_END);
	}

	if (strlen(parms->rfh_jms_priority) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_JMS_PRI_BEGIN);

		/* get the message domain name and append it to the XML message */
		replaceChars(parms->rfh_jms_priority, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_JMS_PRI_END);
	}

	/* check if we found any jms fields */
	if (tempfield[0] != 0)
	{
		/* add the XML tags and data */
		strcpy(varArea, RFH2_JMS_BEGIN);
		strcat(varArea, tempfield);
		strcat(varArea, RFH2_JMS_END);

		/* round to a multiple of four bytes */
		roundVarArea(varArea);
	}
}

void appendOption(char * tempfield, const char * optfield, int reqType)

{
	switch (reqType)
	{
	case RFH_PSC_SUB:
		{
			strcat(tempfield, RFH2_PSC_REGOPT_BEGIN);
			break;
		}
	case RFH_PSC_UNSUB:
		{
			strcat(tempfield, RFH2_PSC_REGOPT_BEGIN);
			break;
		}
	case RFH_PSC_PUB:
		{
			strcat(tempfield, RFH2_PSC_PUBOPT_BEGIN);
			break;
		}
	case RFH_PSC_REQPUB:
		{
			strcat(tempfield, RFH2_PSC_REGOPT_BEGIN);
			break;
		}
	case RFH_PSC_DELPUB:
		{
			strcat(tempfield, RFH2_PSC_DELOPT_BEGIN);
			break;
		}
	}

	strcat(tempfield, optfield);

	switch (reqType)
	{
	case RFH_PSC_SUB:
		{
			strcat(tempfield, RFH2_PSC_REGOPT_END);
			break;
		}
	case RFH_PSC_UNSUB:
		{
			strcat(tempfield, RFH2_PSC_REGOPT_END);
			break;
		}
	case RFH_PSC_PUB:
		{
			strcat(tempfield, RFH2_PSC_PUBOPT_END);
			break;
		}
	case RFH_PSC_REQPUB:
		{
			strcat(tempfield, RFH2_PSC_REGOPT_END);
			break;
		}
	case RFH_PSC_DELPUB:
		{
			strcat(tempfield, RFH2_PSC_DELOPT_END);
			break;
		}
	}
}

void buildPSC(PUTPARMS * parms)

{
	char			*varArea;
	char			tempValue[256];
	char			tempfield[1024];

	/* get a pointer to the area to use */
	varArea = parms->rfh_psc;

	tempfield[0] = 0;

	if (parms->rfh_psc_reqtype > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_COMMAND_BEGIN);

		/* get the command and append it to the XML message */
		switch (parms->rfh_psc_reqtype)
		{
		case RFH_PSC_SUB:
			{
				strcat(tempfield, PSC_REGSUB);
				break;
			}
		case RFH_PSC_UNSUB:
			{
				strcat(tempfield, PSC_DEREGSUB);
				break;
			}
		case RFH_PSC_PUB:
			{
				strcat(tempfield, PSC_PUBLISH);
				break;
			}
		case RFH_PSC_REQPUB:
			{
				strcat(tempfield, PSC_REQUPDATE);
				break;
			}
		case RFH_PSC_DELPUB:
			{
				strcat(tempfield, PSC_DELETEPUB);
				break;
			}
		}

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_COMMAND_END);
	}

	if (strlen(parms->rfh_topic1) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_TOPIC_BEGIN);

		/* get the topic and append it to the XML message */
		replaceChars(parms->rfh_topic1, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_TOPIC_END);
	}

	if (strlen(parms->rfh_topic2) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_TOPIC_BEGIN);

		/* get the topic and append it to the XML message */
		replaceChars(parms->rfh_topic2, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_TOPIC_END);
	}

	if (strlen(parms->rfh_topic3) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_TOPIC_BEGIN);

		/* get the topic and append it to the XML message */
		replaceChars(parms->rfh_topic3, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_TOPIC_END);
	}

	if (strlen(parms->rfh_topic4) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_TOPIC_BEGIN);

		/* get the topic and append it to the XML message */
		replaceChars(parms->rfh_topic4, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_TOPIC_END);
	}

	if (strlen(parms->rfh_subpoint) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_SUBPOINT_BEGIN);

		/* get the subscription point and append it to the XML message */
		replaceChars(parms->rfh_subpoint, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_SUBPOINT_END);
	}

	if (strlen(parms->rfh_filter) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_FILTER_BEGIN);

		/* get the filter and append it to the XML message */
		replaceChars(parms->rfh_filter, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_FILTER_END);
	}

	if (strlen(parms->rfh_PscReplyQM) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_QMGRNAME_BEGIN);

		/* get the reply queue manager name and append it to the XML message */
		replaceChars(parms->rfh_PscReplyQM, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_QMGRNAME_END);
	}

	if (strlen(parms->rfh_PscReplyQ) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_QNAME_BEGIN);

		/* get the reply queue name and append it to the XML message */
		replaceChars(parms->rfh_PscReplyQ, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_QNAME_END);
	}

	if (strlen(parms->rfh_pubtime) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_PUBTIME_BEGIN);

		/* get the pub time and append it to the XML message */
		replaceChars(parms->rfh_pubtime, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_PUBTIME_END);
	}

	if (parms->rfh_psc_seqno > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_PSC_SEQNUM_BEGIN);

		/* get the sequence number in character format and append it to the XML message */
		sprintf(tempValue, "%d", parms->rfh_psc_seqno);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_PSC_SEQNUM_END);
	}

	if (parms->rfh_psc_local > 0)
	{
		appendOption(tempfield, RFH2_PSC_LOCAL, parms->rfh_psc_reqtype);
	}

	if (parms->rfh_psc_newonly > 0)
	{
		appendOption(tempfield, RFH2_PSC_NEWONLY, parms->rfh_psc_reqtype);
	}

	if (parms->rfh_psc_otheronly > 0)
	{
		appendOption(tempfield, RFH2_PSC_OTHERONLY, parms->rfh_psc_reqtype);
	}

	if (parms->rfh_psc_ondemand > 0)
	{
		appendOption(tempfield, RFH2_PSC_ONDEMAND, parms->rfh_psc_reqtype);
	}

	if (parms->rfh_psc_retainpub > 0)
	{
		appendOption(tempfield, RFH2_PSC_RETAINPUB, parms->rfh_psc_reqtype);
	}

	if (parms->rfh_psc_isretainpub > 0)
	{
		appendOption(tempfield, RFH2_PSC_ISRETAINPUB, parms->rfh_psc_reqtype);
	}

	if (parms->rfh_psc_correlid > 0)
	{
		appendOption(tempfield, RFH2_PSC_CORRELASID, parms->rfh_psc_reqtype);
	}

	if (parms->rfh_psc_deregall > 0)
	{
		appendOption(tempfield, RFH2_PSC_DEREGALL, parms->rfh_psc_reqtype);
	}

	if (parms->rfh_psc_infretain > 0)
	{
		appendOption(tempfield, RFH2_PSC_INFORMIFRET, parms->rfh_psc_reqtype);
	}

	if (parms->rfh_psc_pers_type > 0)
	{
		switch(parms->rfh_psc_pers_type)
		{
		case RFH_PSC_PERS:
			{
				appendOption(tempfield, RFH2_PSC_PERS, parms->rfh_psc_reqtype);
				break;
			}
		case RFH_PSC_NON_PERS:
			{
				appendOption(tempfield, RFH2_PSC_NON_PERS, parms->rfh_psc_reqtype);
				break;
			}
		case RFH_PSC_PERS_PUB:
			{
				appendOption(tempfield, RFH2_PSC_PERSASPUB, parms->rfh_psc_reqtype);
				break;
			}
		case RFH_PSC_PERS_QUEUE:
			{
				appendOption(tempfield, RFH2_PSC_PERSASQUEUE, parms->rfh_psc_reqtype);
				break;
			}
		}
	}

	/* check if there were any psc fields */
	if (tempfield[0] != 0)
	{
		/* add the XML tags and data */
		strcpy(varArea, RFH2_PSC_BEGIN);
		strcat(varArea, tempfield);
		strcat(varArea, RFH2_PSC_END);

		/* round to a multiple of four bytes */
		roundVarArea(varArea + 4);
	}
}

void buildMCD(PUTPARMS * parms)

{
	char	*varArea;
	char	tempValue[256];
	char	tempfield[1024];

	/* get a pointer to the area to use */
	varArea = parms->rfh_mcd;

	/* initialize the string to a null string */
	tempfield[0] = 0;

	if (parms->rfh_jms_reqtype > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_MSD_BEGIN);

		switch (parms->rfh_jms_reqtype)
		{
		case RFH_JMS_TEXT:
			{
				strcat(tempfield, JMS_TEXT);
				break;
			}
		case RFH_JMS_BYTES:
			{
				strcat(tempfield, JMS_BYTES);
				break;
			}
		case RFH_JMS_OBJECT:
			{
				strcat(tempfield, JMS_OBJECT);
				break;
			}
		case RFH_JMS_STREAM:
			{
				strcat(tempfield, JMS_STREAM);
				break;
			}
		case RFH_JMS_MAP:
			{
				strcat(tempfield, JMS_MAP);
				break;
			}
		case RFH_JMS_NONE:
			{
				strcat(tempfield, JMS_NONE);
				break;
			}
		}

		/* Add the closing tag */
		strcat(tempfield, RFH2_MSD_END);
	}
	else
	{
		if (strlen(parms->rfhdomain) > 0)
		{
			/* add the XML tag */
			strcat(tempfield, RFH2_MSD_BEGIN);

			/* get the message domain name and append it to the XML message */
			replaceChars(parms->rfhdomain, tempValue);
			strcat(tempfield, tempValue);

			/* Add the closing tag */
			strcat(tempfield, RFH2_MSD_END);
		}
	}

	if (strlen(parms->rfhset) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_SET_BEGIN);

		/* get the message set name and append it to the XML message */
		replaceChars(parms->rfhset, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_SET_END);
	}

	if (strlen(parms->rfhtype) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_TYPE_BEGIN);

		/* get the message type name and append it to the XML message */
		replaceChars(parms->rfhtype, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_TYPE_END);
	}

	if (strlen(parms->rfhfmt) > 0)
	{
		/* add the XML tag */
		strcat(tempfield, RFH2_FMT_BEGIN);

		/* get the message fmt name and append it to the XML message */
		replaceChars(parms->rfhfmt, tempValue);
		strcat(tempfield, tempValue);

		/* Add the closing tag */
		strcat(tempfield, RFH2_FMT_END);
	}

	/* check if there were any mcd fields */
	if (tempfield[0] != 0)
	{
		/* add the XML tags and data */
		strcpy(varArea, RFH2_MCD_BEGIN);
		strcat(varArea, tempfield);
		strcat(varArea, RFH2_MCD_END);

		/* round to a multiple of four bytes */
		roundVarArea(varArea + 4);
	}
}

/**************************************************************/
/*                                                            */
/* Routine to construct an RFH V2 header.                     */
/*                                                            */
/**************************************************************/

int buildRFH2(char * rfhdata, PUTPARMS *parms, int xmlOnly)

{
	int				rfhlength;
	int				varlength=0;
	MQRFH2			tempRFH;
	char			tempfield[USR_AREA_SIZE];

	/* get a pointer to the area to build the variable part of the RFH */
	memset(tempfield, 0, sizeof(tempfield));

	/* initialize the RFH length */
	rfhlength = MQRFH_STRUC_LENGTH_FIXED_2;

	/* check if we have a variable part */
	if (1 == xmlOnly)
	{
		tempfield[0] = 0;

		/* check for explicit folder definiions */
		varlength += appendFolder(tempfield + varlength, parms->rfh_mcd, parms->encoding);
		varlength += appendFolder(tempfield + varlength, parms->rfh_jms, parms->encoding);
		varlength += appendFolder(tempfield + varlength, parms->rfh_psc, parms->encoding);
		varlength += appendFolder(tempfield + varlength, parms->rfh_pscr, parms->encoding);
		varlength += appendFolder(tempfield + varlength, parms->rfh_usr, parms->encoding);

		/* Allow for the variable part length and data */
		rfhlength += varlength;
	}

	/* initialize the header */
	tempRFH.Version = MQRFH_VERSION_2;
	tempRFH.Flags = MQRFH_NONE;
	tempRFH.StrucLength = rfhlength;

	if (parms->nameValueCCSID > 0)
	{
		tempRFH.NameValueCCSID = parms->nameValueCCSID;
	}
	else
	{
		tempRFH.NameValueCCSID = 1208;
	}

	/* set the encoding for the user data */
	if (parms->rfhencoding > 0)
	{
		tempRFH.Encoding = parms->rfhencoding;
	}
	else
	{
		tempRFH.Encoding = MQENC_NATIVE;
	}

	/* get the code page of the user data */
	if (parms->rfhccsid > 0)
	{
		/* set the ccsid field in the RFH header */
		tempRFH.CodedCharSetId = parms->rfhccsid;
	}
	else
	{
		/* choose an intelligent default */
		tempRFH.CodedCharSetId = MQCCSI_INHERIT;
	}

	/* find out if the character set is EBCDIC */
	if (getCcsidType(parms->codepage) == CHAR_ASCII)
	{
		/* set the structure id and format fields in ASCII */
		memcpy(tempRFH.StrucId, MQRFH_STRUC_ID, sizeof(tempRFH.StrucId));
		memcpy(tempRFH.Format, parms->msgformat, MQ_FORMAT_LENGTH);
	}
	else
	{
		/* set the structure id and format fields in EBCDIC */
		AsciiToEbcdic((unsigned char *)tempRFH.StrucId, (unsigned char *)MQRFH_STRUC_ID, sizeof(tempRFH.StrucId));
		AsciiToEbcdic((unsigned char *)tempRFH.Format, (unsigned char *)&(parms->msgformat), MQ_FORMAT_LENGTH);
	}

#if defined WIN32 || defined __i386__
	if ((parms->encoding & MQENC_INTEGER_REVERSED) == 0)
#else
	if ((parms->encoding & MQENC_INTEGER_REVERSED) != 0)
#endif
	{
		/* we need to reverse the integers in all of the RFH header fields */
		tempRFH.Version        = reverseBytes4(tempRFH.Version);
		tempRFH.StrucLength    = reverseBytes4(tempRFH.StrucLength);
		tempRFH.Encoding       = reverseBytes4(tempRFH.Encoding);
		tempRFH.CodedCharSetId = reverseBytes4(tempRFH.CodedCharSetId);
		tempRFH.Flags          = reverseBytes4(tempRFH.Flags);
		tempRFH.NameValueCCSID = reverseBytes4(tempRFH.NameValueCCSID);
	}

	/* copy the fixed portion of the header and the length of the */
	/* variable part to the data area */
	memcpy(rfhdata, (char *) &tempRFH, MQRFH_STRUC_LENGTH_FIXED_2);

	/* check if we have a variable part */
	if (varlength > 0)
	{
		memcpy(rfhdata + MQRFH_STRUC_LENGTH_FIXED_2, tempfield, varlength);
	}

	/* return the length of the RFH2 header */
	return rfhlength;
}

void resetPtr(char **ptr, unsigned int *len)

{
	/* do we have an existing area? */
	if (*ptr != NULL)
	{
		free((*ptr));		/* release the storage              */
		(*ptr) = NULL;		/* remember we released the storage */
		(*len) = 0;			/* reset the length of the storage  */
	}
}

char * allocateFolderArea(size_t len)

{
	char *	tempPtr;

	tempPtr = (char *)malloc(len + 16);		/* allocate a new folder area */
	memset(tempPtr, 0, len + 16);			/* clear the storage          */

	return tempPtr;
}

void appendPtr(char **ptr, unsigned int *length, char * data)

{
	char *			tempPtr;
	char *			newPtr;
	unsigned int	len;

	tempPtr = (*ptr);
	len = (*length);

	if ((NULL == tempPtr) || (0 == len))
	{
		/* allocate a new area to store the usr folder */
		tempPtr = allocateFolderArea(USR_AREA_SIZE);
		len = USR_AREA_SIZE;
		(*ptr) = tempPtr;
		(*length) = len;
	}

	/* check if we are going to overflow the existing area */
	if ((strlen(data) + strlen(tempPtr)) > len)
	{
		/* allocate a new area that is twice as large */
		len *= 2;							/* double the size               */
		newPtr = (char *)malloc(len + 16);	/* allocate a new larger area	 */
		memset(newPtr, 0, len + 16);		/* clear the new area            */
		strcpy(newPtr, tempPtr);			/* capture the previous data     */
		free(tempPtr);						/* release the old storage area  */
		tempPtr = newPtr;					/* point to the new storage area */
		(*ptr) = newPtr;					/* remember the new storage area */
		(*length) = len;					/* and the new length            */
	}

	strcat(tempPtr, data);
}

void resetMCD(PUTPARMS * parms)

{
	/* get rid of any previous values */
	resetPtr(&(parms->rfh_mcd), &parms->max_mcd);
}

void appendMCD(char * data, PUTPARMS * parms)

{
	appendPtr(&(parms->rfh_mcd), &parms->max_mcd, data);
}

void resetPSC(PUTPARMS * parms)

{
	/* get rid of any previous values */
	resetPtr(&(parms->rfh_psc), &parms->max_psc);
}

void appendPSC(char * data, PUTPARMS * parms)

{
	appendPtr(&(parms->rfh_psc), &parms->max_psc, data);
}


void resetPSCR(PUTPARMS * parms)

{
	/* get rid of any previous values */
	resetPtr(&(parms->rfh_pscr), &parms->max_pscr);
}

void appendPSCR(char * data, PUTPARMS * parms)

{
	appendPtr(&(parms->rfh_pscr), &parms->max_pscr, data);
}

void resetJMS(PUTPARMS * parms)

{
	/* get rid of any previous values */
	resetPtr(&(parms->rfh_jms), &parms->max_jms);
}

void appendJMS(char * data, PUTPARMS * parms)

{
	appendPtr(&(parms->rfh_jms), &parms->max_jms, data);
}

void resetUSR(PUTPARMS * parms)

{
	/* get rid of any previous values */
	resetPtr(&parms->rfh_usr, &parms->max_usr);
}

void appendUSR(char * data, PUTPARMS * parms)

{
	appendPtr(&(parms->rfh_usr), &parms->max_usr, data);
}

void rebuildMCD(PUTPARMS * parms)

{
	resetMCD(parms);
	parms->rfh_mcd = allocateFolderArea(USR_AREA_SIZE);
	parms->max_mcd = USR_AREA_SIZE;
	buildMCD(parms);
}

void rebuildJMS(PUTPARMS * parms)

{
	resetJMS(parms);
	parms->rfh_jms = allocateFolderArea(USR_AREA_SIZE);
	parms->max_jms = USR_AREA_SIZE;
	buildJMS(parms);
}

void rebuildPSC(PUTPARMS * parms)

{
	resetPSC(parms);
	parms->rfh_psc = allocateFolderArea(USR_AREA_SIZE);
	parms->max_psc = USR_AREA_SIZE;
	buildPSC(parms);
}

unsigned int checkRFH(const char * msgdata, const size_t datalen, MQMD2 *mqmd, PUTPARMS * parms)

{
	MQRFH2			tempRFH;
	char			tempEbcdic[32];
	int				ccsid;
	int				encoding;
	unsigned int	rfhlength=0;
	int				rfhversion;
	char			tempFormat[9];

	/* check if the message is long enough to have an RFH */
	if (datalen < sizeof(tempRFH))
	{
		return 0;
	}

	/* get a copy of the first 32 bytes of the message */
	memcpy(&tempRFH, msgdata, sizeof(tempRFH));
	memcpy(tempFormat, tempRFH.Format, 8);
	tempFormat[8] = 0;

	if (memcmp(tempRFH.StrucId, MQRFH_STRUC_ID, sizeof(tempRFH.StrucId)) != 0)
	{
		/* try converting the StrucId and format fields to EBCDIC */
		EbcdicToAscii((unsigned char *) &tempRFH.StrucId, sizeof(tempRFH.StrucId), (unsigned char *)tempEbcdic);
		EbcdicToAscii((unsigned char *) &tempRFH.Format, sizeof(tempRFH.Format), (unsigned char *)tempFormat);
		memcpy(&tempRFH.StrucId, tempEbcdic, sizeof(tempRFH.StrucId));

		/* see if we have a structure identifier in EBCDIC */
		if (memcmp(tempRFH.StrucId, MQRFH_STRUC_ID, sizeof(tempRFH.StrucId)) != 0)
		{
			/* no RFH found */
			return 0;
		}
	}

	/* see the version and length */
	rfhversion = tempRFH.Version;
	rfhlength = (unsigned int)tempRFH.StrucLength;
	ccsid = tempRFH.CodedCharSetId;
	encoding = tempRFH.Encoding;

	/* check if we have the right encoding */
	if ((MQRFH_VERSION_1 != rfhversion) && (MQRFH_VERSION_2 != rfhversion))
	{
		/* reverse the bytes and see if the version is 1 or 2 */
		rfhversion = reverseBytes4(rfhversion);
		rfhlength = reverseBytes4(rfhlength);
		ccsid = reverseBytes4(ccsid);
		encoding = reverseBytes4(encoding);

		if ((MQRFH_VERSION_1 != rfhversion) && (MQRFH_VERSION_2 != rfhversion))
		{
			return 0;
		}
	}

	/* finally check that the length makes sense */
	if ((rfhlength < MQRFH_STRUC_LENGTH_FIXED) || (rfhlength > datalen))
	{
		/* length doesn't make sense */
		return 0;
	}

	/* check if we are removing the RFH header and saving the MQMD */
	if ((1 == parms->saveMQMD) && (RFHSTRIP == parms->striprfh))
	{
		/* update the format, encoding and codepage fields in the MQMD */
		memcpy(mqmd->Format, tempRFH.Format, sizeof(mqmd->Format));
		if (ccsid > 0)
		{
			mqmd->CodedCharSetId = ccsid;
		}

		if (encoding > 0)
		{
			mqmd->Encoding = encoding;
		}
	}

	return (unsigned int)rfhlength;
}

char * checkForRFH(char * msgdata, MQMD2 * msgdesc)

{
	int		rfhLen=0;
	int		encoding=msgdesc->Encoding;
	MQRFH	*tempRFH;

	/* N.B. this routine will only skip the first header - if there are more than one RFH header this will not work */
	if ((memcmp(msgdesc->Format, MQFMT_RF_HEADER, MQ_FORMAT_LENGTH) == 0) || (memcmp(msgdesc->Format, MQFMT_RF_HEADER_2, MQ_FORMAT_LENGTH) == 0))
	{
		tempRFH = (MQRFH *)msgdata;

		rfhLen = tempRFH->StrucLength;

		/* check what the native encoding is on this platform */
		if ((MQENC_NATIVE | MQENC_INTEGER_MASK) == MQENC_INTEGER_NORMAL)
		{
			/* host encoding */
			if ((encoding | MQENC_INTEGER_MASK) != MQENC_INTEGER_NORMAL)
			{
				rfhLen = reverseBytes4(rfhLen);
			}
		}
		else
		{
			/* pc encoding */
			if ((encoding | MQENC_INTEGER_MASK) == MQENC_INTEGER_NORMAL)
			{
				rfhLen = reverseBytes4(rfhLen);
			}
		}
	}

	return msgdata + rfhLen;
}

/**************************************************************/
/*                                                            */
/* Routine to create an RFH header, if necessary, to insert   */
/*  before the message data.  This routine will create an     */
/*  rfh if the rfh parameter is set to V1, V2 or XML.         */
/*                                                            */
/**************************************************************/

void createRFH(PUTPARMS *parms)

{
	int		varLength=strlen(LATENCYHEADER) + 4;

	/* check for latency in user properties option */
	if (1 == parms->timeStampUserProp)
	{
		/* point to the rfh2 header usr area */
		parms->rfh_usr = LATENCYHEADER;
		parms->rfh_mcd = NULL;
		parms->rfh_jms = NULL;
		parms->rfh_psc = NULL;
		parms->rfh_pscr = NULL;

		/* construct the RFH2 header */
		parms->rfhlength = buildRFH2((char *)parms->rfhdata, parms, 1);

		/* force the option to include the RFH2 header */
		parms->rfh = RFH_V2;
	}
	else
	{
		/* pad with blanks to make 8 characters */
		while (strlen(parms->msgformat) < MQ_FORMAT_LENGTH)
		{
			strcat(parms->msgformat, " ");
		}

		/* check if we need to build an RFH header to */
		/* insert before the message data for each message */
		switch (parms->rfh)
		{
		case RFH_V1:
			{
				parms->rfhlength = buildRFH1((char *)parms->rfhdata, parms);
				break;
			}
		case RFH_V2:
			{
				parms->rfhlength = buildRFH2((char *)parms->rfhdata, parms, 1);
				break;
			}
		case RFH_XML:
			{
				strcpy(parms->msgformat, "xml     ");
				parms->rfhlength = buildRFH2((char *)parms->rfhdata, parms, 0);
				break;
			}
		}
	}
}
