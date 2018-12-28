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

// mqsubs.cpp: common mqrelated subroutines.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "comsubs.h"
#include "rfhutil.h"
#include "cmqc.h"

#define MQINSKEY "SOFTWARE\\IBM\\MQSeries\\CurrentVersion\\Configuration\\AllQueueManagers"

///////////////////////////////////////////////////////////////////////
//
// Subroutine to recognize code pages and return the code page type.
//
// Code page types are:
//
//  ASCII			CHAR_ASCII
//  EBCDIC			CHAR_EBCDIC
//  SIMP CHINESE	CHAR_CHINESE
//  JAPANESE		CHAR_JAPANESE
//  TRAD CHINESE	CHAR_TRAD_CHIN
//  KOREAN			CHAR_KOREAN
//
///////////////////////////////////////////////////////////////////////

int getCcsidType(const int ccsid)

{
	int	result= CHAR_ASCII;

	if ((ccsid == 37) || (ccsid == 500) ||
		(ccsid == 290) || (ccsid == 930) || (ccsid == 5026) ||		// unique Japanese EBCDIC code pages with different lowercase latin code points
		(ccsid == 924) || (ccsid == 1153) ||
		(ccsid == 273) || (ccsid == 1140) ||
		(ccsid == 275) || (ccsid == 1141) ||
		(ccsid == 277) || (ccsid == 1142) ||
		(ccsid == 278) || (ccsid == 1143) ||
		(ccsid == 280) || (ccsid == 1144) ||
		(ccsid == 284) || (ccsid == 1145) ||
		(ccsid == 285) || (ccsid == 1146) ||
		(ccsid == 297) || (ccsid == 1147) ||
		(ccsid == 871) || (ccsid == 1148) ||
		(ccsid == 875) || (ccsid == 1149) ||
		(ccsid == 283) || (ccsid == 1047) ||
		(ccsid == 870) || (ccsid == 1026) ||
		(ccsid == 322) || (ccsid == 905) ||
		(ccsid == 20273) || (ccsid == 20277) ||
		(ccsid == 20278) || (ccsid == 20280) ||
		(ccsid == 20284) || (ccsid == 20285) ||
		(ccsid == 20290) || (ccsid == 20297) ||
		(ccsid == 20420) || (ccsid == 20423) ||
		(ccsid == 20424) || (ccsid == 20833) ||
		(ccsid == 20838) || (ccsid == 20871) ||
		(ccsid == 20880) || (ccsid == 20905) ||
		(ccsid == 20924) || (ccsid == 21025) ||
		(ccsid == 50930) || (ccsid == 50931) ||
		(ccsid == 50933) || (ccsid == 50935) ||
		(ccsid == 50936) || (ccsid == 50937) ||
		(ccsid == 50939))
	{
		result = CHAR_EBCDIC;
	}

	// check for Japanese code page
	if ((932 == ccsid) || (943 == ccsid))
	{
		// select japanese character display
		result = CHAR_JAPANESE;
	}

	// check for simplified chinese code page
	if ((936 == ccsid) || (1381 == ccsid) || (1386 == ccsid) || (5488 == ccsid))
	{
		// select simplified chinese character display
		result = CHAR_CHINESE;
	}

	// check for Korean code page
	if ((949 == ccsid) || (1363 == ccsid))
	{
		// select korean character display
		result = CHAR_KOREAN;
	}

	// check for Thai code page
	if ((838 == ccsid) || (874 == ccsid) || (10021 == ccsid) || (28605 == ccsid))
	{
		// select Thai character display
		result = CHAR_THAI;
	}

	// check for traditional chinese code page
	if (950 == ccsid)
	{
		// select traditional chinese character display
		result = CHAR_TRAD_CHIN;
	}

	return result;
}

/**************************************************************/
/*                                                            */
/* Routine to look up the default directory for the client    */
/* channel table, which is the MQ install directory.          */
/*                                                            */
/**************************************************************/

void findMQInstallDirectory(char *dirName, int dirNameLen)

{
	int			ret;
	int			ret2;
	int			length=0;
	HKEY		regkey;
	DWORD		valueType;
	DWORD		subKeyNameSize;

	dirName[0] = 0;
	length = dirNameLen;

	/* open the registry key that we will enumerate */
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					   MQINSKEY,
					   0,
					   KEY_QUERY_VALUE,
					   &regkey);

	if (ERROR_SUCCESS == ret)
	{
		subKeyNameSize;
		ret = RegQueryValueEx(regkey,
							  "DefaultPrefix",
							  0,
							  &valueType,
							  (unsigned char *)dirName,
							  (unsigned long *)&length);

		/* close the key */
		ret2 = RegCloseKey(regkey);
	}
	else
	{
		printf("Could not locate default MQ install directory\n");
		fflush(stdout);
	}
}

const char * getApplTypeDisplay(int putType)

{
	const char *	ptr;

	switch (putType)
	{
	case MQAT_NO_CONTEXT:
		{
			ptr = "0 None";
			break;
		}
	case MQAT_CICS:
		{
			ptr = "1 CICS";
			break;
		}
	case MQAT_MVS:
		{
			ptr = "2 OS/390 or z/OS";
			break;
		}
	case MQAT_IMS:
		{
			ptr = "3 IMS";
			break;
		}
	case MQAT_OS2:
		{
			ptr = "4 OS/2";
			break;
		}
	case MQAT_DOS:
		{
			ptr = "5 PC Dos";
			break;
		}
	case MQAT_AIX:
		{
			ptr = "6 Unix";
			break;
		}
	case MQAT_QMGR:
		{
			ptr = "7 QMGR";
			break;
		}
	case MQAT_OS400:
		{
			ptr = "8 OS/400";
			break;
		}
	case MQAT_WINDOWS:
		{
			ptr = "9 Windows";
			break;
		}
	case MQAT_CICS_VSE:
		{
			ptr = "10 CICS/VSE";
			break;
		}
	case MQAT_WINDOWS_NT:
		{
			ptr = "11 32-bit Windows";
			break;
		}
	case MQAT_VMS:
		{
			ptr = "12 VMS";
			break;
		}
	case MQAT_GUARDIAN:
		{
			ptr = "13 Guardian or Non Stop";
			break;
		}
	case MQAT_VOS:
		{
			ptr = "14 VOS";
			break;
		}
	case MQAT_IMS_BRIDGE:
		{
			ptr = "19 IMS Bridge";
			break;
		}
	case MQAT_XCF:
		{
			ptr = "20 XCF";
			break;
		}
	case MQAT_CICS_BRIDGE:
		{
			ptr = "21 CICS Bridge";
			break;
		}
	case MQAT_NOTES_AGENT:
		{
			ptr = "22 Notes";
			break;
		}
	case MQAT_USER:
		{
			ptr = "25 User";
			break;
		}
	case MQAT_BROKER:
		{
			ptr = "26 Broker";
			break;
		}
	case MQAT_JAVA:
		{
			ptr = "28 Java";
			break;
		}
	case MQAT_DQM:
		{
			ptr = "29 DQM";
			break;
		}
	case MQAT_CHANNEL_INITIATOR:
		{
			ptr = "30 Chan Init";
			break;
		}
	case MQAT_WLM:
		{
			ptr = "31 WLM";
			break;
		}
	case MQAT_BATCH:
		{
			ptr = "32 Batch";
			break;
		}
	case MQAT_RRS_BATCH:
		{
			ptr = "33 RRS Batch";
			break;
		}
	case MQAT_SIB:
		{
			ptr = "34 SIBus";
			break;
		}
	case MQAT_SYSTEM_EXTENSION:
		{
			ptr = "35 System";
			break;
		}
	case MQAT_MCAST_PUBLISH:
		{
			ptr = "36 Multicast";
			break;
		}
	case MQAT_AMQP:
		{
			ptr = "37 AMQP";
			break;
		}
	default:
		{
			ptr = "";
		}
	}

	return ptr;
}
 
//////////////////////////////////
//
// Routine to return the actual
// encoding value used in the
// MQMD from a selected type
//
//////////////////////////////////

int getEncodingValue(int encodeType, int pdEncodeType, int floatEncodeType)

{
	int	tempEncode=MQENC_INTEGER_REVERSED;

	if (NUMERIC_HOST == encodeType)
	{
		// set for 390 (normal integers=1)
		tempEncode = MQENC_INTEGER_NORMAL;
	}

	if (NUMERIC_HOST == pdEncodeType)
	{
		// big endian (host, unix, Java)
		tempEncode |= MQENC_DECIMAL_NORMAL;
	}
	else
	{
		// little endian (PC)
		tempEncode |= MQENC_DECIMAL_REVERSED;
	}

	if (FLOAT_UNIX == floatEncodeType)
	{
		// big endian (unix, Java) floating point format
		tempEncode |= MQENC_FLOAT_IEEE_NORMAL;
	}
	else if (FLOAT_390 == floatEncodeType)
	{
		// 390 format floating point
		tempEncode |= MQENC_FLOAT_S390;
	}
	else if (FLOAT_TNS == floatEncodeType)
	{
		// TNS floating point format
		tempEncode |= MQENC_FLOAT_TNS;
	}
	else
	{
		// assume PC (little-endian) floating point format
		tempEncode |= MQENC_FLOAT_IEEE_REVERSED;
	}

	return tempEncode;
}

int getIntEncode(int encoding)

{
	int	numFormat;

	if ((encoding & MQENC_INTEGER_REVERSED) > 0)
	{
		// normal PC little-endian
		numFormat = NUMERIC_PC;
	}
	else
	{
		// host big-endian
		numFormat = NUMERIC_HOST;
	}

	return numFormat;
}

int getPDEncode(int encoding)

{
	int	numFormat;

	if ((encoding & MQENC_DECIMAL_REVERSED) > 0)
	{
		// normal PC little-endian
		numFormat = NUMERIC_PC;
	}
	else
	{
		// host big-endian
		numFormat = NUMERIC_HOST;
	}

	return numFormat;
}

int getFloatEncode(int encoding)

{
	int	floatFormat=FLOAT_PC;

	if ((encoding & MQENC_FLOAT_IEEE_NORMAL) > 0)
	{
		// normal PC little endian
		floatFormat = FLOAT_PC;
	}
	else if ((encoding & MQENC_FLOAT_IEEE_REVERSED) > 0)
	{
		// host big endian
		floatFormat = FLOAT_UNIX;
	}
	else if ((encoding & MQENC_FLOAT_S390) > 0)
	{
		// 390 floating point
		floatFormat = FLOAT_390;
	}
	else if ((encoding & MQENC_FLOAT_TNS) > 0)
	{
		// TNS floating point
		floatFormat = FLOAT_TNS;
	}

	return floatFormat;
}

BOOL translateMQMD(MQMD2 * mqmd)

{
	int		v1r;
	int		v2r;
	unsigned char	ebcdicID[4];
	unsigned char	asciiID[4];

	// check for the MQMD eye catcher
	memcpy(asciiID, MQMD_STRUC_ID, 4);
	AsciiToEbcdic(asciiID, 4, ebcdicID);
	if ((memcmp(mqmd->StrucId, asciiID, 4) != 0) && (memcmp(mqmd->StrucId, ebcdicID, 4) != 0))
	{
		// did not find the eyecatcher - return
		return FALSE;
	}

	// check if the MQMD is in a different encoding sequence
	v1r = reverseBytes4(MQMD_VERSION_1);
	v2r = reverseBytes4(MQMD_VERSION_2);

	// check for either version 1 or version 2
	if ((mqmd->Version != v1r) &&
		(mqmd->Version != v2r) &&
		(mqmd->Version != MQMD_VERSION_1) &&
		(mqmd->Version != MQMD_VERSION_2))
	{
		// do not recognize version
		return FALSE;
	}

	// check if the MQMD is in ebcdic
	if (memcmp(mqmd->StrucId, ebcdicID, 4) == 0)
	{
		// convert the character fields to ASCII
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

	if ((v1r == mqmd->Version) || (v2r == mqmd->Version))
	{
		// it appears the mqmd was captured on a big-endian system
		// the byte ordering in the mqmd will be reversed
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

		// deal with the message id, correl id, accounting token
		//reverseBytes24(mqmd->MsgId, mqmd->MsgId);
		//reverseBytes24(mqmd->CorrelId, mqmd->CorrelId);
		//reverseBytes32(mqmd->AccountingToken, mqmd->AccountingToken);

		// check if this is a version 1 or version 2 mqmd
		if (mqmd->Version!= MQMD_VERSION_1)
		{
			// deal with the group id, etc
			//reverseBytes24(mqmd->GroupId, mqmd->GroupId);
			mqmd->MsgSeqNumber = reverseBytes4(mqmd->MsgSeqNumber);
			mqmd->Offset = reverseBytes4(mqmd->Offset);
			mqmd->MsgFlags = reverseBytes4(mqmd->MsgFlags);
			mqmd->OriginalLength = reverseBytes4(mqmd->OriginalLength);
		}
	}

	return TRUE;
}
