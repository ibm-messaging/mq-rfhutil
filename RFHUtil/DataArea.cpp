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

// DataArea.cpp: implementation of the DataArea class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <commdlg.h>
#include <winspool.h>
#include <windows.h>
#include <locale.h>
#include "rfhutil.h"
#include "DataArea.h"
#include "comsubs.h"
#include "xmlsubs.h"
#include "mqsubs.h"
#include "cics.h"
#include "ims.h"
#include "dlq.h"
#include "HexFind.h"
#include "MQMDPAGE.h"
#include "RFH.h"
#include "props.h"
#include "ps.h"
#include "usr.h"
#include "jsonparse.h"

// include for RFH2 constants, etc
#include <cmqpsc.h>
#include <cmqxc.h>

// include the copybook utility functions
#include "copybook.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// Definitions
#define MAX_ERR_MSG_LEN			8191

// maximum buffer when loading messages from a file
#define MAX_BUFFER_SIZE		256 * 1024 * 1024

// define a byte order mark (BOM) for code page 1208
#define BOM_1208		0xefbbbf

// registry location of the install directory for MQ
#define MQINSKEY		"SOFTWARE\\IBM\\MQSeries\\CurrentVersion"
#define MQ71INSKEY		"SOFTWARE\\IBM\\WebSphere MQ\\Installation"
#define MQCOMPKEY		"SOFTWARE\\IBM\\MQSeries\\CurrentVersion\\Components"
#define MQINSKEY64		"SOFTWARE\\Wow6432Node\\IBM\\MQSeries\\CurrentVersion"
#define MQ71INSKEY64	"SOFTWARE\\Wow6432Node\\IBM\\WebSphere MQ\\Installation"
#define MQCOMPKEY64		"SOFTWARE\\Wow6432Node\\IBM\\MQSeries\\CurrentVersion\\Components"

#define MAX_ASCII_CHAR	32
#define MAX_DISPLAY_MSGS	50000
#define MAX_DISPLAY_TIME	15				// 15 seconds
#define MAX_XML_INDENT_LEVELS	15
#define DEF_MAX_UOW		128
#define BAD_XML_MSG		"*****RFHUtil format problem - Data does not appear to be XML\r\n"
#define CDATA_START		"<![CDATA["
#define CDATA_END		"]]>"
#define ENTITY_START	"<!ENTITY"

// initialize memory allocation for name table entries
#define INIT_NAME_TABLE_SIZE		8192

// default buffer size to use for MQGET requests (64KB)
#define DEFAULT_BUFFER_SIZE 64 * 1024

// maximum size of message property value that can be handled
#define MQ_MAX_PROPERTY_VALUE_LENGTH	4 * 1024 * 1024

// special pub/sub user property names
#define MQTOPICSTR	"MQTopicString"
#define MQISRETAIN	"MQIsRetained"

#define ADMINQ "SYSTEM.ADMIN.COMMAND.QUEUE\0"

static char	xmlVarName[8192 + 1024];			// tag name
static char	tempVarValue[2 * 8192 + 1024];	// tag value
static char	tempVarName[8192 + 1];			// holding area for a tag name
static char	tempEndVarName[8192 + 1];
static char	tempVarAttr[2 * 8192 + 1];
static char	xmlAttrStr[65536];

////////////////////////////////////
//
// Channel table header structure
//
////////////////////////////////////

typedef struct {
	long	seglen;
	long	reclen;
	long	filler1;
	long	seeknext;
	long	seeklast;
	MQCD	cd;
} chanHeader;

//////////////////////////////////////////////////////////////////////////////////////////////
DataArea::DataArea()

{
	// initialize traceEnabled to false
	// this will be changed to true during the initinstance method in the application class
	// if the trace environment variable is found and the trace file is opened.
	traceEnabled = FALSE;

	// initialize the q and qm pointers
	q = MQHO_NONE;
	qm = MQHO_NONE;
	hReplyQ = MQHO_NONE;
	hAdminQ = MQHO_NONE;
	connected = false;
	characterSet = DEF_ASCII_CCSID;

	// initialize queue manager table pointer
	firstInstallation = NULL;
	firstQmgr = NULL;
	foundMQ71=FALSE;
	highestMQVersion = 0;
	highestMQRelease = 0;
	memset(higestVRMF, 0, sizeof(higestVRMF));
	memset(MQ71Path, 0, sizeof(MQ71Path));
	memset(highestLevelInstallationName, 0, sizeof(highestLevelInstallationName));

	// initialize dll instance handle
	mqmdll = NULL;
	pubsubSupported = FALSE;
	propertiesSupported = FALSE;

	// initialize the pub/sub name table pointers
	subNamesRoot = NULL;
	topicNamesRoot = NULL;
	queueNamesRoot = NULL;

	// initialize the set user id and set all context settings for the queue that is currently open
	m_save_set_user_setting = FALSE;
	m_save_set_all_setting = FALSE;

	// initialize the maximum message length fields and the QM ccsid
	maxMsgLenQM = -1;
	maxMsgLenQ = -1;
	MQCcsid = 0;

	// initialize the platform and level fields
	platform = 0;
	level = 0;

	// get operating system type
	is64bit = ((CRfhutilApp *)AfxGetApp())->is64bit;

	// allocate a default buffer for MQ gets
	defaultMQbuffer = NULL;
	defaultMQbuffer = (char *)rfhMalloc(DEFAULT_BUFFER_SIZE, "DEFBUFF ");

	// initialize the display data pointers
	m_data_ascii = NULL;
	m_data_ascii_UCS = NULL;
	m_data_hex = NULL;
	m_data_both = NULL;
	m_data_xml = NULL;
	m_data_parsed = NULL;
	m_data_json = NULL;
	m_data_fix = NULL;
	browseActive = 0;
	browsePrevActive = 0;
	saveCharCrlf = IGNORE_CRLF;
	saveCharFormat = CHAR_ASCII;
	saveBothFormat = CHAR_ASCII;
	m_bind_option = BIND_AS_Q;
	m_close_option = Q_CLOSE_NONE;

	m_msg_id_table = NULL;
	msgIdCount = 0;

	// initialize the pub/sub variables
	m_ps_broker_qm = _T("");
	m_ps_remove = FALSE;
	subQ = MQHO_NONE;				// handle for subscription queue
	hSub = MQHO_NONE;				// handle for subscription

	// initialize message properties processing control
	m_mq_props = MQ_PROPS_AS_QUEUE;	// initialize process message properties - will be set to TRUE if the entry points are found in MQM.dll
	propertiesSupported = FALSE;	// set to TRUE if the entry points are found in the MQM.dll
	pubsubSupported = FALSE;		// set to TRUE if the entry points are found in the MQM.dll

	// initialize variable to indicate if MQ was found
	foundMQ = TRUE;

	// initialize the MQ CCSID table values
	MQCcsidIn = 0;
	MQCcsidOut = 0;
	MQCcsidLen = 0;
	MQCcsidPtr = NULL;
	MQUcsCcsid = 0;
	MQUcsPtr = NULL;

	// initialize pointers to CICS, IMS and CDlq class objects
	cicsData = NULL;
	imsData = NULL;
	dlqData = NULL;

	m_copybook = NULL;
	m_copybook_file_name = _T("");
	m_char_format = CHAR_ASCII;
	fileData = NULL;

	m_QM_name.Empty();
	m_Q_name.Empty();
	transmissionQueue.Empty();
	m_queue_type.Empty();

	unitOfWorkActive = false;
	maxUOW = DEF_MAX_UOW;

	m_use_ssl = FALSE;
	m_ssl_validate = FALSE;
	m_ssl_reset_count = 0;
	m_ssl_cipher.Empty();
	m_local_address.Empty();

	// switch to ignore an error trying to get the default QMgr
	firstError = 0;

	currentQM.Empty();
	currentQ.Empty();
	currentConnect = Q_OPEN_NOT;
	memset(currentFilter, 0, sizeof(currentFilter));

	m_dataonly = FALSE;				// ignore RFH for file writes
	m_write_include_MQMD=TRUE;		// start with include mqmd on file saves
	m_read_ignore_header = FALSE;	// ignore RFH for file reads
	m_readfile_ascii = FALSE;		// strip CRLF from file on read
	m_read_unix = FALSE;			// convert Unix text file to PC format
	m_save_rfh = FALSE;				// preserve RFH settings when reading a file
	m_show_system_queues = FALSE;	// Display system queues in dropdown menu
	m_show_cluster_queues = FALSE;	// Display cluster queues in dropdown menu

	m_setUserID = FALSE;			// do not select set user id option
	m_set_all = FALSE;				// do not select set all option
	m_convert = FALSE;				// do not select convert option
	m_complete_msg = FALSE;			// do not select complete message option
	m_new_correl_id = FALSE;		// set new correlation id on put
	m_new_msg_id = TRUE;			// set new message id on put
	m_mq_props = MQ_PROPS_AS_QUEUE;	// use queue definition - default

	m_pd_numeric_format = NUMERIC_PC;
	m_float_numeric_format = FLOAT_PC;

	// clear the file name and the name of the last file successfully read
	memset(fileName, 0, sizeof(fileName));
	memset(lastFileRead, 0, sizeof(lastFileRead));

	// clear the real queue manager name of a client queue manager
	memset(QueueManagerRealName, 0, sizeof(QueueManagerRealName));

	// initialize storage freed indicator
	storageFreed = FALSE;

	// initialize other fields
	DeleteContents();

	memset(invalidCharTable, '.', sizeof(invalidCharTable));
	invalidCharTableNotBuilt = TRUE;

	m_error_msg.Empty();
}

DataArea::~DataArea()

{
	if (traceEnabled)
	{
		// trace entry to ~DataArea
		logTraceEntry("Entering DataArea::~DataArea()");
	}

	// release any acquired storage
	freeAllocations();
}

void DataArea::freeAllocations()

{
	MQLONG			reason;
	NAMESTRUCT		*namesPtr;
	CRfhutilApp *	app;
	char			traceInfo[256];			// work variable to build trace message

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// acquire the lock
	app->m_free_alloc.Lock();

	if (traceEnabled)
	{
		// trace entry to freeAllocations
		logTraceEntry("Entering DataArea::freeAllocations()");
	}

	// has the storage already been freed
	if (storageFreed)
	{
		// already done - just return
		return;
	}

	// don't do this twice
	storageFreed = TRUE;

	if (q != NULL)
	{
		closeQ(Q_CLOSE_NONE);
	}

	if (hReplyQ != NULL)
	{
		// close the reply queue
		closeReplyQ();
	}

	if (hAdminQ != NULL)
	{
		closeAdminQ(&reason);
	}

	if ((qm != NULL) && (connected))
	{
		discQM();
	}

	if (defaultMQbuffer != NULL)
	{
		rfhFree(defaultMQbuffer);
	}

	// release any data areas that were acquired
	resetDataArea();

	if (m_copybook != NULL)
	{
		delete m_copybook;
	}

	// Was an MQ translation table used?
	if (MQCcsidPtr != NULL)
	{
		// release the allocated storage
		rfhFree(MQCcsidPtr);
	}

	// was an MQ to UCS-2 translation table used?
	if (MQUcsPtr != NULL)
	{
		rfhFree(MQUcsPtr);
	}

	// check if the name tables were allocated
	while (subNamesRoot != NULL)
	{
		// remember the next object
		namesPtr = (NAMESTRUCT *)subNamesRoot->nextQM;

		// delete the names object
		if (subNamesRoot->names != NULL)
		{
			delete subNamesRoot->names;
		}

		// release the storage before exiting
		rfhFree(subNamesRoot);

		// update the root pointer
		subNamesRoot = namesPtr;
	}

	while (topicNamesRoot != NULL)
	{
		// remember the next object
		namesPtr = (NAMESTRUCT *)topicNamesRoot->nextQM;

		// delete the names object
		if (topicNamesRoot->names != NULL)
		{
			delete topicNamesRoot->names;
		}

		// release the storage before exiting
		rfhFree(topicNamesRoot);

		// update the root pointer
		topicNamesRoot = namesPtr;
	}

	while (queueNamesRoot != NULL)
	{
		// remember the next object
		namesPtr = (NAMESTRUCT *)queueNamesRoot->nextQM;

		// delete the names object
		if (queueNamesRoot->names != NULL)
		{
			delete queueNamesRoot->names;
		}

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "DataArea::freeAllocations() freed QMgr NAMESTRUCT at %8.8X next %8.8X", (unsigned int)queueNamesRoot, (unsigned int)namesPtr);

			// trace storage release in freeAllocations
			logTraceEntry(traceInfo);
		}

		// release the storage before exiting
		rfhFree(queueNamesRoot);

		// update the root pointer
		queueNamesRoot = namesPtr;
	}

	// release any installation table entries (MQ 7.1 and above)
	releaseInstallEntries();
	releaseQMgrEntries();

	if (mqmdll != NULL)
	{
		// done with the mqm dll
		FreeLibrary(mqmdll);
	}

	// release the lock
	app->m_free_alloc.Unlock();

	if (traceEnabled)
	{
		// trace exit from freeAllocations
		logTraceEntry("Exiting DataArea::freeAllocations()");
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// This routine will attempt to set the MQMD encoding based on the presence
// of an MQ header at the front of the data.  It is used when data is read
// from a file and no MQMD is present, so an intelligent guess must be made.
//
/////////////////////////////////////////////////////////////////////////////

int DataArea::assignMQMDEncoding(int encoding, const int tempVer, const int headerValue)

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	int			encode=encoding;
	char		traceInfo[256];					// work variable to build trace message

	// set the encoding
	if (tempVer == headerValue)
	{
		// looks like little endian encoding (PC)
		mqmdObj->setEncoding(NUMERIC_PC, NUMERIC_PC, FLOAT_PC);
	}
	else
	{
		// reverse the bytes since the encoding is big-endian
		encode = reverseBytes4(encoding);

		// check if the header uses Unix encoding so the MQMD will match
		if (encode == DEF_UNIX_ENCODING)
		{
			// set the MQMD to indicate Unix encoding for integers
			mqmdObj->setEncoding(NUMERIC_HOST, NUMERIC_HOST, FLOAT_UNIX);
		}
		else
		{
			// looks like big endian encoding (assume 390)
			mqmdObj->setEncoding(NUMERIC_HOST, NUMERIC_HOST, FLOAT_390);
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "DataArea::assignMQMDEncoding() tempVer=%d headerValue=%d encoding=%d mqmd->Encoding=%d", tempVer, headerValue, encode, mqmdObj->getEncoding());

		// trace entry to assignMQMDEncoding
		logTraceEntry(traceInfo);
	}

	return mqmdObj->getEncoding();
}

/////////////////////////////////////////////////////////////////////////////
//
// This routine will attempt to set the MQMD encoding based on the presence
// of an MQ header at the front of the data.  It is used when data is read
// from a file and no MQMD is present, so an intelligent guess must be made.
//
/////////////////////////////////////////////////////////////////////////////

void DataArea::assignMQMDccsid(const char *tempId, const char *headerName, const int ccsid, int encoding)

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	int			tempCcsid=ccsid;
	char		traceInfo[256];					// work variable to build trace message

	if (encoding != NUMERIC_PC)
	{
		tempCcsid = reverseBytes4(ccsid);
	}

	if (memcmp(tempId, headerName, 4) == 0)
	{
		// looks like data is in ASCII
		m_char_format = CHAR_ASCII;

		// is the code page in the header also ASCII?
		if (CHAR_ASCII == getCcsidType(tempCcsid))
		{
			// use the code page from the header
			mqmdObj->setCcsid(tempCcsid);
		}
		else
		{
			// need to set a default ASCII code page
			mqmdObj->setCcsid(DEF_ASCII_CCSID);
		}
	}
	else
	{
		// looks like data is in EBCDIC
		m_char_format = CHAR_EBCDIC;

		// is the code page in the header also EBCDIC?
		if (CHAR_EBCDIC == getCcsidType(tempCcsid))
		{
			// use the code page from the header
			mqmdObj->setCcsid(tempCcsid);
		}
		else
		{
			// need to set a default EBCDIC code page
			mqmdObj->setCcsid(DEF_EBCDIC_CCSID);
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "DataArea::assignMQMDccsid() headerName=%s, tempCcsid=%d mqmd->Ccsid=%d, m_char_format=%c, encoding=%d", headerName, tempCcsid, mqmdObj->getCcsid(), m_char_format, encoding);

		// trace entry to assignMQMDccsid
		logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Check message data read from a file for indications of an MQ header
// If found, set the MQMD format, codepage and encoding fields to match.
//
// This routine is called if a file is read and does not have an MQMD stored
// with the data.  If the file contains what appears to be an MQ header
// structure then the codepage and encoding fields in the MQMD will be set
// to match the first header in the file.  The format field in the MQMD will
// also be set to match the header.
//
/////////////////////////////////////////////////////////////////////////////

int DataArea::checkDataForHeader()

{
	MQDLH		*dlqptr;
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	int			foundHdr=0;
	int			tempVer;
	int			tempVer2;
	int			encoding;
	char		tempId[12];
	char		tempId2[12];
	char		traceInfo[128];		// work variable to build trace message

	if (traceEnabled)
	{
		// trace entry to checkDataForHeader
		logTraceEntry("Entering DataArea::checkDataForHeader()");
	}

	// initialize work areas
	memset(tempId, 0, sizeof(tempId));
	memset(tempId2, 0, sizeof(tempId2));

	// get the header and version number in both possible formats
	dlqptr = (MQDLH *)fileData;
	memcpy(tempId, dlqptr->StrucId, sizeof(dlqptr->StrucId));
	memcpy(&tempVer, &dlqptr->Version, sizeof(dlqptr->Version));

	// now try the opposite encoding and character set
	tempVer2 = reverseBytes4(tempVer);
	EbcdicToAscii((unsigned char *)tempId, 4, (unsigned char *)&tempId2);

	// check for a DLQ header
	// is there enough data?
	if (fileSize >= sizeof(MQDLH))
	{
		// does either structure id match?
		if ((memcmp(tempId, MQDLH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQDLH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQDLH_VERSION_1) || (tempVer2 == MQDLH_VERSION_1))
			{
				// it passes the test - assume this is a dead letter queue header
				foundHdr = 1;

				// set the mqmd format field
				mqmdObj->setFormat(MQFMT_DEAD_LETTER_HEADER);

				// assign MQMD encoding based on the header
				encoding = assignMQMDEncoding(dlqptr->Encoding, tempVer, MQDLH_VERSION_1);

				// set the character set and type
				assignMQMDccsid(tempId, MQDLH_STRUC_ID, dlqptr->CodedCharSetId, encoding);
			}
		}
	}

	// check for a CICS header
	// is there enough data?
	if (fileSize >= MQCIH_LENGTH_1 + 8)
	{
		// does either structure id match?
		if ((memcmp(tempId, MQCIH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQCIH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQCIH_VERSION_1) || (tempVer2 == MQCIH_VERSION_1) ||(tempVer == MQCIH_VERSION_2) || (tempVer2 == MQCIH_VERSION_2))
			{
				// it passes the test - assume this is a CICS header
				foundHdr = 1;

				// set the mqmd format field
				mqmdObj->setFormat(MQFMT_CICS);

				// assign MQMD encoding based on the header
				encoding = assignMQMDEncoding(((MQCIH*)dlqptr)->Encoding, tempVer, MQCIH_VERSION_1);

				// set the character set and type
				assignMQMDccsid(tempId, MQCIH_STRUC_ID, ((MQCIH*)dlqptr)->CodedCharSetId, encoding);
			}
		}
	}

	// check for an IMS header
	// is there enough data?
	if (fileSize >= MQIIH_LENGTH_1 + 8)
	{
		// does either structure id match?
		if ((memcmp(tempId, MQIIH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQIIH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQIIH_VERSION_1) || (tempVer2 == MQIIH_VERSION_1))
			{
				// it passes the test - assume this is an IMS header
				foundHdr = 1;

				// set the mqmd format field
				mqmdObj->setFormat(MQFMT_IMS);

				// assign MQMD encoding based on the header
				encoding = assignMQMDEncoding(((MQIIH*)dlqptr)->Encoding, tempVer, MQIIH_VERSION_1);

				// set the character set and type
				assignMQMDccsid(tempId, MQIIH_STRUC_ID, ((MQIIH*)dlqptr)->CodedCharSetId, encoding);
			}
		}
	}

	// check for an RFH1 header
	if (fileSize >= MQRFH_STRUC_LENGTH_FIXED)
	{
		// does either structure id match?
		if ((memcmp(tempId, MQRFH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQRFH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQRFH_VERSION_1) || (tempVer2 == MQRFH_VERSION_1))
			{
				// it passes the test - assume this is a RFH1 header
				foundHdr = 1;

				// set the mqmd format field
				mqmdObj->setFormat(MQFMT_RF_HEADER);

				// assign MQMD encoding based on the header
				encoding = assignMQMDEncoding(((MQRFH*)dlqptr)->Encoding, tempVer, MQRFH_VERSION_1);

				// set the character set and type
				assignMQMDccsid(tempId, MQRFH_STRUC_ID, ((MQRFH*)dlqptr)->CodedCharSetId, encoding);
			}
		}
	}

	// check for an RFH2 header
	if (fileSize >= MQRFH_STRUC_LENGTH_FIXED_2)
	{
		// does either structure id match?
		if ((memcmp(tempId, MQRFH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQRFH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQRFH_VERSION_2) || (tempVer2 == MQRFH_VERSION_2))
			{
				// it passes the test - assume this is an RFH2 header
				foundHdr = 1;

				// set the mqmd format field
				mqmdObj->setFormat(MQFMT_RF_HEADER_2);

				// assign MQMD encoding based on the header
				encoding = assignMQMDEncoding(((MQRFH2*)dlqptr)->Encoding, tempVer, MQRFH_VERSION_2);

				// set the character set and type
				assignMQMDccsid(tempId, MQRFH_STRUC_ID, ((MQRFH2*)dlqptr)->CodedCharSetId, encoding);
			}
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::checkDataForHeader() foundHdr=%d tempId=%s tempId2=%s", foundHdr, tempId, tempId2);

		// trace exit from checkDataForHeader
		logTraceEntry(traceInfo);
	}

	return foundHdr;
}

/////////////////////////////////////////////////////////////////////////////
//
// Routine to display a file dialog and if the user selects a file read it
// into memory.
//
/////////////////////////////////////////////////////////////////////////////

void DataArea::ReadDataFile()

{
	int				rc;
	CRfhutilApp *	app;
	_TCHAR			name[512];

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// invoke standard dialog to choose file name
	CFileDialog fd(TRUE, NULL, NULL);

	// check for an initial file path
	if (app->initFilePath.GetLength() > 0)
	{
		// set the initial file directory
		fd.m_ofn.lpstrInitialDir = app->initFilePath;
	}

	// set some filter patterns
	fd.m_ofn.lpstrFilter = _T("All (*.*)\0*.*\0XML (*.XML)\0*.XML\0Text (*.TXT)\0*.TXT\0");

	// execute the file dialog
	rc = fd.DoModal();

	//	Did the user press the OK button?
	// make sure the full file name is not too long and that it is not a null string
	if ((rc == IDOK) && (_tcslen((LPCTSTR)fd.GetPathName()) > 0) && (_tcslen((LPCTSTR)fd.GetPathName()) < sizeof(name) - 1))
	{
		// set the full file name in the DataArea object
		_tcscpy(name, (LPCTSTR)fd.GetPathName( ));

		// drive the document file processing
		app->BeginWaitCursor();
		ReadFileData(name);
		app->EndWaitCursor();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Read message data from a file
//
// The file ccsid and encoding fields will be set.  In some cases this can
// be done automatically.  A two byte terminator will be added to the end
// of the data to limit any damage from string functions that might otherwise
// run past the end of the data area.
//
// A check for a unicode byte order mark at the front of the data will
// be made.

// Check for an MQMD at the front of the data.  If an MQMD is found then
// things like the code page and encoding can be set automatically.  An MQMD
// is recognized by checking the first 8 bytes of the data area and also
// making sure that the number of bytes read is sufficient for an MQMD.
//
// Check for the presence of an RFH at the front of the data.  The check is
// based on the first 8 bytes of the file data.  If the first 4 bytes of the
// file are the characters "RFH " in ASCII or EBCDIC, and the next 4 bytes
// are an integer with a value of 1 or 2 (both big-endian and little-endian
// formats are checked), the file is assumed to have an RFH at the front.
// The RFH is then stripped and parsed separately.
//
/////////////////////////////////////////////////////////////////////////////

void DataArea::ReadFileData(LPCTSTR fname)

{
	int				rfh1Len=0;
	int				rfh2Len=0;
	int				dlqLen=0;
	int				cicsLen=0;
	int				imsLen=0;
	int				mqmdLen;
	int				offset=0;
	int				ccsid;
	int				encoding;
	int				foundHdr;
	FILE			*inputFile;
	char			*ptr;
	MQMDPAGE		*mqmdObj=(MQMDPAGE *)mqmdData;
	CRfhutilApp *	app;
	char			rfhmsg[1024];
	char			errtxt[1024];
	char			traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::ReadFileData() m_file_codepage=%d m_read_ignore_header=%d m_read_nocrlf=%d m_read_unix=%d fname=%s", m_file_codepage, m_read_ignore_header, m_read_nocrlf, m_read_unix, fname);

		// trace entry to ReadFileData
		logTraceEntry(traceInfo);
	}

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// clear the temporary message buffer
	m_error_msg.Empty();

	// check for a bad file name
	ptr = strchr((char *)fname, '\xEE');
	if (ptr != NULL)
	{
		sprintf(errtxt, "Invalid file name (%s)", fname);
		m_error_msg = errtxt;

		if (traceEnabled)
		{
			// trace error
			logTraceEntry(errtxt);
		}

		return;
	}

	// open the input file
	inputFile = fopen(fname, "rb");
	if (NULL == inputFile)
	{
		m_error_msg = "Could not open file ";
		m_error_msg += fname;

		// this file name will be first in the recent files list, so remember it
		strcpy(lastFileRead, fname);

		if (traceEnabled)
		{
			// trace error
			logTraceEntry("Could not open file");
		}
	}
	else
	{
		// need to set a default fileCcsid
		// let the user specify it in the MQMD ccsid field and encoding for this file
		ccsid = m_file_codepage;
		encoding = mqmdObj->m_mqmd_encoding;

		// did the user specify a ccsid?
		if (0 == ccsid)
		{
			// try the MQMD setting
			ccsid = atoi((LPCTSTR)mqmdObj->m_mqmd_ccsid);

			// was there a ccsid in the MQMD?
			if (0 == ccsid)
			{
				// use the ansi code page
				ccsid = app->ansi;

				// did that work?
				if (0 == ccsid)
				{
					// set a US English default
					ccsid = 1252;
				}
			}
		}

		// did the encoding get set?
		if (encoding < 0)
		{
			// set a default of PC (little-endian) encoding
			encoding = NUMERIC_PC;
		}

		if (groupActive || segmentActive)
		{
			// in a segment or a group so do not reset the MQMD settings
			resetDataArea();		// release the current data and display areas only
		}
		else
		{
			// release any previous data areas
			ClearData();

			// Change the code page and numeric encoding to
			// PC values, since this is probably PC data
			m_char_format = CHAR_ASCII;
			mqmdObj->setEncoding(NUMERIC_PC, NUMERIC_PC, FLOAT_PC);
		}

		// should the current MQ header settings be preserved?
		if (!m_save_rfh)
		{
			// clear the MQMD
			mqmdObj->clearMQMD();

			// clear the RFH header and any associated folders
			((RFH *)rfhData)->clearRFH();

			// is there a CICS header object?
			if (cicsData != NULL)
			{
				// clear all the data in the header
				((CCICS *)cicsData)->clearCICSheader();
			}

			// is there an IMS header object?
			if (imsData != NULL)
			{
				// clear all the data in the header
				((CIms *)imsData)->clearIMSheader();
			}

			// is there a dead letter queue header object?
			if (dlqData != NULL)
			{
				// clear all the data in the header
				((CDlq *)dlqData)->clearDLQheader();
			}

			// clear the user properties
			if (propData != NULL)
			{
				// clear the user properties
				((CProps *)propData)->clearUserProperties();
			}
		}

		// determine the length of the data
		fseek(inputFile, 0L, SEEK_END);
		fileSize = ftell(inputFile);

		// return the pointer to the beginning of the data
		fseek(inputFile, 0L, SEEK_SET);

		// allocate memory for the file
		fileData = (unsigned char *)rfhMalloc(fileSize + 2, "FILEDATA");

		// read it into memory
		fread(fileData, 1, fileSize, inputFile);

		// done with the file handle so close it
		fclose(inputFile);

		// update the current file name as the last file read from
		strcpy(fileName, fname);
		strcpy(lastFileRead, fileName);

		// add the file to the most recent file list
		AfxGetApp()->AddToRecentFileList(fileName);

		// update the most recent path in the application object
		app->setMostRecentFilePath(fileName);

		// just in case make sure there is a string terminator at the end of the data
		fileData[fileSize] = 0;
		fileData[fileSize+1] = 0;			// allow for Unicode termination character

		// check for a Byte Order Mark (BOM)
		if ((0xff == fileData[0]) && (0xfe == fileData[1]))
		{
			// override the current encoding setting - force to little endian
			encoding = NUMERIC_PC;
		}
		else if ((0xfe == fileData[0]) && (0xff == fileData[1]))
		{
			// override the current encoding setting - force to big endian
			encoding = NUMERIC_HOST;
		}

		// set the file ccsid and encoding
		fileCcsid = ccsid;
		fileIntFormat = encoding;

		// set the file format in certain cases
		m_char_format = getCcsidType(ccsid);

		// check for an MQMD at the front of the file
		// and if found, extract the fields from the MQMD
		mqmdLen = mqmdObj->processMQMD(fileData, fileSize);

		// found an MQMD?
		if (mqmdLen > 0)
		{
			// remove the MQMD from the data
			fileSize -= mqmdLen;
			if (fileSize > 0)
			{
				// shift the rest of the data to eliminate the MQMD
				memcpy(fileData, fileData + mqmdLen, fileSize);
			}

			// terminate the data, so that any string functions do not run past the end
			fileData[fileSize] = 0;
			fileData[fileSize+1] = 0;

			// capture the ccsid and integer format
			fileCcsid = _ttoi((LPCTSTR)mqmdObj->m_mqmd_ccsid);
			fileIntFormat = mqmdObj->m_mqmd_encoding;
		}
		else
		{
			// process any headers in the file
			if (!m_read_ignore_header && !m_save_rfh)
			{
				// check for header at front of file
				// if found, set the MQMD format, code page and encoding
				foundHdr = checkDataForHeader();

				// was a header found?
				if ((0 == foundHdr) && (0 == m_file_codepage))
				{
					// no header and user did not specify a code page
					// take a guess if this is ASCII or EBCDIC data
					checkForEBCDIC();
				}
			}
		}

		// check if headers in the file are to be processed
		if (!m_read_ignore_header && !m_save_rfh)
		{
			// now process any headers in the file
			offset = parseMsgHeaders();

			// get the lengths of the various headers
			rfh1Len = ((RFH *)rfhData)->getRFH1length();
			rfh2Len = ((RFH *)rfhData)->getRFH2length();
			dlqLen = ((CDlq *)dlqData)->getDLQlength();
			cicsLen = ((CCICS *)cicsData)->getCICSlength();
			imsLen = ((CIms *)imsData)->getIMSlength();
		}

		// let the user know what is going on
		if (rfh1Len + rfh2Len + cicsLen + imsLen + dlqLen > 0)
		{
			sprintf(rfhmsg, "%d bytes read (data %d dlq %d rfh1 %d, rfh2 %d cics %d ims %d) from file %s", fileSize + rfh1Len + rfh2Len + cicsLen + imsLen + dlqLen, fileSize, dlqLen, rfh1Len, rfh2Len, cicsLen, imsLen, fileName);
		}
		else
		{
			sprintf(rfhmsg, "%d bytes read from file %s", fileSize, fileName);
		}

		// display the results for the user
		m_error_msg += rfhmsg;

		// is trace turned on?
		if (traceEnabled)
		{
			// add results of ReadFileData to trace file
			logTraceEntry(rfhmsg);
		}

		if (m_read_nocrlf)
		{
			// remove any CR or LF characters
			fileSize = removeCrLf(fileData, fileSize);
		}
		else
		{
			// check for conversion of Unix text file
			if (m_read_unix)
			{
				changeUnixFile();
			}
		}

		// Is the file name too long for the display on the data tab?
		if (strlen(fname) > 75)
		{
			// only use the last 75 characters
			sprintf(errtxt, "...%s", fileName + strlen(fileName) - 75);
		}
		else
		{
			// use the entire name
			strcpy(errtxt, fileName);
		}

		// set the source of the message for display on the data tab
		fileSource.Format(_T("Message Data (%d) from %s"), fileSize, errtxt);

		// terminate the data allowing for a Unicode message
		fileData[fileSize] = 0;
		fileData[fileSize+1] = 0;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::ReadFileData() mqmdLen=%d offset=%d foundHdr=%d fileCcsid=%d fileIntFormat=%d ccsid=%d encoding=%d", mqmdLen, offset, foundHdr, fileCcsid, fileIntFormat, ccsid, encoding);

		// trace exit from ReadFileData
		logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to find the code page and encoding of a particular header.
// This routine will start with the MQMD and search the chain of MQ
// headers until a match is found or until the header is not found.
//
/////////////////////////////////////////////////////////////////////////

void DataArea::getHdrCcsid(const char * format, int *ccsid, int *encoding)

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	RFH			*rfhObj=(RFH *)rfhData;
	CCICS		*cicsObj=(CCICS *)cicsData;
	CIms		*imsObj=(CIms *)imsData;
	CDlq		*dlqObj=(CDlq *)dlqData;
	int			nextEncoding;
	int			nextCcsid;
	char		dlqHeader[MQ_FORMAT_LENGTH + 4];
	char		rfh1Header[MQ_FORMAT_LENGTH + 4];
	char		rfh2Header[MQ_FORMAT_LENGTH + 4];
	char		cicsHeader[MQ_FORMAT_LENGTH + 4];
	char		imsHeader[MQ_FORMAT_LENGTH + 4];
	char		nextFormat[MQ_FORMAT_LENGTH + 4];
	char		target[MQ_FORMAT_LENGTH + 4];
	bool		found=false;
	char		traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getHdrCcsid() format=%s ccsid=%d encoding=%d", format, (*ccsid), (*encoding));

		// trace entry to getHdrCcsid
		logTraceEntry(traceInfo);
	}

	// get the MQ format names
	strcpy(dlqHeader, MQFMT_DEAD_LETTER_HEADER);
	strcpy(rfh1Header, MQFMT_RF_HEADER);
	strcpy(rfh2Header, MQFMT_RF_HEADER_2);
	strcpy(cicsHeader, MQFMT_CICS);
	strcpy(imsHeader, MQFMT_IMS);
	Rtrim(dlqHeader);
	Rtrim(rfh1Header);
	Rtrim(rfh2Header);
	Rtrim(cicsHeader);
	Rtrim(imsHeader);

	strcpy(target, format);
	Rtrim(target);

	// get the format field from the mqmd
	mqmdObj->getFormat(nextFormat);

	// prime the ccsid and the encoding fields from the MQMD
	nextCcsid = getDefaultCcsid(mqmdObj->getCcsid());
	nextEncoding = getDefaultEncoding(mqmdObj->getEncoding());

	while (!found)
	{
		// remove any trailing blanks from the next format field
		Rtrim(nextFormat);

		if (strcmp(nextFormat, target) == 0)
		{
			found = true;
		}
		else
		{
			// move on to the next header
			// figure out what kind of header this is
			if (strcmp(nextFormat, dlqHeader) == 0)
			{
				dlqObj->getFormat(nextFormat);
				nextCcsid = dlqObj->getCcsid();
				nextEncoding = dlqObj->getEncoding();

				// avoid a loop
				if (strcmp(nextFormat, dlqHeader) == 0)
				{
					found = true;
				}
			}
			else if (strcmp(nextFormat, rfh1Header) == 0)
			{
				rfhObj->getRFH1Format(nextFormat);
				nextCcsid = rfhObj->getRFH1Ccsid();
				nextEncoding = rfhObj->getRFH1Encoding();

				// avoid a loop
				if (strcmp(nextFormat, rfh1Header) != 0)
				{
					found = true;
				}
			}
			else if (strcmp(nextFormat, rfh2Header) == 0)
			{
				rfhObj->getRFH2Format(nextFormat);
				nextCcsid = rfhObj->getRFH2Ccsid();
				nextEncoding = rfhObj->getRFH2Encoding();

				// avoid a loop
				if (strcmp(nextFormat, rfh2Header) != 0)
				{
					found = true;
				}
			}
			else if (strcmp(nextFormat, cicsHeader) == 0)
			{
				cicsObj->getFormat(nextFormat);
				nextCcsid = cicsObj->getCcsid();
				nextEncoding = cicsObj->getEncoding();

				// avoid a loop
				if (strcmp(nextFormat, cicsHeader) != 0)
				{
					found = true;
				}
			}
			else if (strcmp(nextFormat, imsHeader) == 0)
			{
				imsObj->getFormat(nextFormat);
				nextCcsid = imsObj->getCcsid();
				nextEncoding = imsObj->getEncoding();

				// avoid a loop
				if (strcmp(nextFormat, imsHeader) != 0)
				{
					found = true;
				}
			}
			else
			{
				// can't find the right header
				// end the loop
				found = true;
			}
		}
	}

	// set a reasonable default code page
	if (0 == nextCcsid)
	{
		nextCcsid = DEF_ASCII_CCSID;
	}

	// return the last one, assigning a default if it is not set
	(*ccsid) = getDefaultCcsid(nextCcsid);
	(*encoding) = getDefaultEncoding(nextEncoding);

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getHdrCcsid() nextCcsid=%d, nextEncoding=%d", getDefaultCcsid(nextCcsid), getDefaultEncoding(nextEncoding));

		// trace exit from getHdrCcsid
		logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to check if the encoding is not set (-1) and replace it
// with a default value if it is not set.
//
/////////////////////////////////////////////////////////////////////////

int DataArea::getDefaultEncoding(int encoding)

{
	// check if it is set
	if (-1 == encoding)
	{
		// set a reasonable default
		encoding = NUMERIC_PC;
	}

	return encoding;
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to check if the ccsid is set to 0 and replace it with a
// default value if it is.
//
/////////////////////////////////////////////////////////////////////////

int DataArea::getDefaultCcsid(int ccsid)

{
	// check if there is no ccsid specified
	if ((0 == ccsid) && (qm != NULL) && connected)
	{
		// get the default queue manager code page
		ccsid = characterSet;
	}

	// if the previous default did not work then set a reasonable default
	if (0 == ccsid)
	{
		ccsid = DEF_ASCII_CCSID;
	}

	return ccsid;
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to build MQ headers from the current settings of the
// dialog pages.
//
/////////////////////////////////////////////////////////////////////////

int DataArea::buildHeaders(unsigned char *tempbuf, int dlqLen, int cicsLen, int imsLen, int rfh1Len, int rfh2Len)

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	RFH			*rfhObj=(RFH *)rfhData;
	int			curOfs=0;
	int			charFormat;
	int			encoding;
	int			ccsid;
	int			charType;
	bool		found;
	char		dlqHeader[MQ_FORMAT_LENGTH + 4];
	char		rfh1Header[MQ_FORMAT_LENGTH + 4];
	char		rfh2Header[MQ_FORMAT_LENGTH + 4];
	char		cicsHeader[MQ_FORMAT_LENGTH + 4];
	char		imsHeader[MQ_FORMAT_LENGTH + 4];
	char		nextFormat[MQ_FORMAT_LENGTH + 4];
	char		traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::buildHeaders() dlqLen=%d rfh1Len=%d rfh2Len=%d cicsLen=%d imsLen=%d", dlqLen, rfh1Len, rfh2Len, cicsLen, imsLen);

		// trace entry to buildHeaders
		logTraceEntry(traceInfo);
	}

	// get the MQ format names
	strcpy(dlqHeader, MQFMT_DEAD_LETTER_HEADER);
	strcpy(rfh1Header, MQFMT_RF_HEADER);
	strcpy(rfh2Header, MQFMT_RF_HEADER_2);
	strcpy(cicsHeader, MQFMT_CICS);
	strcpy(imsHeader, MQFMT_IMS);
	Rtrim(dlqHeader);
	Rtrim(rfh1Header);
	Rtrim(rfh2Header);
	Rtrim(cicsHeader);
	Rtrim(imsHeader);

	// get the format field from the mqmd
	mqmdObj->getFormat(nextFormat);
	Rtrim(nextFormat);
	charFormat = m_char_format;
	ccsid = mqmdObj->getCcsid();

	// use a default value if the ccsid is not set
	ccsid = getDefaultCcsid(ccsid);

	// set the character type based on the ccsid
	charType = getCcsidType(ccsid);

	// get the encoding specified in the MQMD
	encoding = getDefaultEncoding(mqmdObj->getEncoding());

	found = true;
	while (found)
	{
		found = false;

		// The dead letter queue should be first, if present
		// put the DLQ header first
		if ((dlqLen > 0) && (strcmp(nextFormat, dlqHeader) == 0))
		{
			((CDlq *)dlqData)->createHeader(ccsid, encoding);
			memcpy(tempbuf, ((CDlq *)dlqData)->getDLQheader(), dlqLen);
			curOfs += dlqLen;

			// get the format field, ccsid and encoding from the dead letter queue header
			((CDlq *)dlqData)->setNextFormat(nextFormat, &charFormat, &encoding);
			ccsid = ((CDlq *)dlqData)->getCcsid();
			Rtrim(nextFormat);
		}

		// check for an RFH V1 header
		if ((rfh1Len > 0) && (strcmp(nextFormat, rfh1Header) == 0))
		{
			// move the rfh1 header to the data buffer
			curOfs += rfhObj->buildRFH1header(tempbuf + curOfs, ccsid, encoding);

			// check if any more headers are chained behind this one
			rfhObj->getRFH1Format(nextFormat);
			Rtrim(nextFormat);

			// prevent a loop, in case this header points to itself
			if (strcmp(nextFormat, rfh1Header) != 0)
			{
				found = true;
				ccsid = rfhObj->getRFH1Ccsid();
				charFormat = getCcsidType(ccsid);
				encoding = rfhObj->getRFH1Encoding();
			}
		}

		// check for an RFH V2 header
		if ((rfh2Len > 0) && (strcmp(nextFormat, rfh2Header) == 0))
		{
			// move the rfh2 header to the data buffer
			curOfs += rfhObj->buildRFH2header(tempbuf + curOfs, ccsid, encoding);

			// check if any more headers are chained behind this one
			rfhObj->getRFH2Format(nextFormat);
			Rtrim(nextFormat);

			// prevent a loop, in case this header points to itself
			if (strcmp(nextFormat, rfh2Header) != 0)
			{
				found = true;
				ccsid = rfhObj->getRFH2Ccsid();
				charFormat = getCcsidType(ccsid);
				encoding = rfhObj->getRFH2Encoding();
			}
		}

		// build the CICS header after the DLQ and RFH headers
		if ((cicsLen > 0) && (strcmp(nextFormat, cicsHeader) == 0))
		{
			((CCICS *)cicsData)->buildCICSheader(tempbuf + curOfs, ccsid, encoding);
			curOfs += cicsLen;

			// check if any more headers are chained behind this one
			((CCICS *)cicsData)->setNextFormat(nextFormat, &charFormat, &encoding);
			ccsid = ((CCICS *)cicsData)->getCcsid();

			// prevent a loop, in case this header points to itself
			if (strcmp(nextFormat, cicsHeader) != 0)
				found = true;
		}

		// build the IMS header last
		if ((imsLen > 0) && (strcmp(nextFormat, imsHeader) == 0))
		{
			((CIms *)imsData)->buildIMSheader(tempbuf + curOfs, ccsid, encoding);
			curOfs += imsLen;

			// check if any more headers are chained behind this one
			((CIms *)imsData)->setNextFormat(nextFormat, &charFormat, &encoding);
			ccsid = ((CIms *)imsData)->getCcsid();

			// prevent a loop, in case this header points to itself
			if (strcmp(nextFormat, imsHeader) != 0)
				found = true;
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::buildHeaders() with curOfs=%d", curOfs);

		// trace exit from buildHeaders
		logTraceEntry(traceInfo);
	}

	return curOfs;
}

/////////////////////////////////////////////////////////////////////////////
//
// DataArea WriteDataFile
//
//  Routine to display a standard file dialog and write the current data to
//  a file.
//
/////////////////////////////////////////////////////////////////////////////

void DataArea::WriteDataFile()

{
	int				rc;
	CRfhutilApp *	app;
	char			name[512];

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// invoke standard dialog to choose file name
	CFileDialog fd(FALSE, NULL, NULL, OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT);

	// check for an initial file path
	if (app->initFilePath.GetLength() > 0)
	{
		// set the initial file directory
		fd.m_ofn.lpstrInitialDir = app->initFilePath;
	}

	// execute the file dialog
	rc = fd.DoModal();

	//	Did the user press the OK button?
	// make sure the full file name is not too long and that it is not a null string
	if ((rc == IDOK) && (strlen(fd.GetPathName()) > 0) && (strlen(fd.GetPathName()) < sizeof(name) - 1))
	{
		// get the file name from the dialog
		strcpy(name, fd.GetPathName( ));

		// drive the document file processing
		app->BeginWaitCursor();
		WriteFile(name);
		app->EndWaitCursor();

		// update the most recent path in the application object
		app->setMostRecentFilePath(name);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// DataArea openOutputFile
//
//  Routine to open a file for output.  If there is an error then the error
//  code is gathered and an error message is constructed and written to the
//  trace file is trace is active.
//
/////////////////////////////////////////////////////////////////////////////

FILE * DataArea::openOutputFile(LPCTSTR fname, LPTSTR errMsg)

{
	FILE			*outputFile;

	// set the error message to a null string
	if (errMsg != NULL)
	{
		// set error message to zero length string
		errMsg[0] = 0;
	}

	// try to open the output file
	outputFile = fopen(fname, "wb");

	if ((outputFile == NULL) && (errMsg != NULL))
	{
		// try to get the last error
		DWORD errCode = GetLastError();

		// check for a sharing violation
		if (32 == errCode)
		{
			// provide a descriptive error message
			strcpy(errMsg, "Unable to open output file - sharing violation");
		}
		else if (33 == errCode)
		{
			// provide a descriptive error message
			strcpy(errMsg, "Unable to open output file - file locked");
		}
		else if (39 == errCode)
		{
			// provide a descriptive error message
			strcpy(errMsg, "Unable to open output file - disk full");
		}
		else if (3 == errCode)
		{
			// provide a descriptive error message
			strcpy(errMsg, "Unable to open output file - path not found");
		}
		else if (5 == errCode)
		{
			// provide a descriptive error message
			strcpy(errMsg, "Unable to open output file - access denied");
		}
		else
		{
			// build the error message
			sprintf(errMsg, "Unable to open output file - Error %d", errCode);
		}

		if (traceEnabled)
		{
			// trace results of WriteFile
			logTraceEntry(errMsg);
		}
	}

	// return the file handle
	return outputFile;
}

/////////////////////////////////////////////////////////////////////////////
//
// DataArea WriteFile
//
//  Routine to write current message data, with or without headers, to
//  a file.
//
/////////////////////////////////////////////////////////////////////////////

void DataArea::WriteFile(LPCTSTR fname)

{
	// Temporary data buffer
	int				rfh1Len=0;
	int				rfh2Len=0;
	int				cicsLen=0;
	int				imsLen=0;
	int				dlqLen=0;
	int				mqmdLen=0;
	int				curOfs=0;
	int				ccsid=0;
	int				encoding=0;
	unsigned char	*tempbuf=NULL;
	MQMDPAGE		*mqmdObj=(MQMDPAGE *)mqmdData;
	RFH				*rfhObj=(RFH *)rfhData;
	FILE			*outputFile;
	MQMD2			mqmd={MQMD2_DEFAULT};
	char			traceInfo[512];		// work variable to build trace message
	char			tempRFHMsg[128];
	char			tempMsg[128];

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::WriteFile() fname=%s", fname);

		// trace entry to WriteFile
		logTraceEntry(traceInfo);
	}

	m_error_msg.Empty();

	// clear the temporary buffer
	memset(fileName, 0, sizeof(fileName));
	strncpy(fileName, fname, sizeof(fileName) - 1);

	// check if need to build an MQMD
	if (m_write_include_MQMD)
	{
		mqmdObj->buildMQMD(&mqmd, TRUE, TRUE);
	}

	// Should the headers be included?
	if (!m_dataonly)
	{
		// figure out how large a buffer is required
		// Check if need to build an RFH1 header
		if (((RFH *)rfhData)->m_rfh1_include)
		{
			getHdrCcsid(MQFMT_RF_HEADER, &ccsid, &encoding);
			rfh1Len = rfhObj->buildRFH(ccsid, encoding);
		}

		// Check if need to build an RFH2 header
		if (((RFH *)rfhData)->m_rfh2_include)
		{
			getHdrCcsid(MQFMT_RF_HEADER_2, &ccsid, &encoding);
			rfh2Len = rfhObj->buildRFH2(ccsid, encoding);
		}

		// check if there is a CICS or IMS header
		cicsLen = ((CCICS *)cicsData)->getCICSlength();
		imsLen = ((CIms *)imsData)->getIMSlength();

		// check for a DLQ header
		if (((CDlq *)dlqData)->checkForDlq())
		{
			// figure out what ccsid and encoding to use
			getHdrCcsid(MQFMT_DEAD_LETTER_HEADER, &ccsid, &encoding);

			// create the DLQ header if necessary
			dlqLen = ((CDlq *)dlqData)->createHeader(ccsid, encoding);
		}

		// allocate a temporary buffer large enough for the headers
		if ((dlqLen + cicsLen + imsLen + rfh1Len + rfh2Len) > 0)
		{
			// allocate a memory area for the headers
			tempbuf = (unsigned char *)rfhMalloc(dlqLen + cicsLen + imsLen + rfh1Len + rfh2Len + 1, "HDRBUFF ");

			if (tempbuf != NULL)
			{
				// build the headers in the buffer
				curOfs = buildHeaders(tempbuf, dlqLen, cicsLen, imsLen, rfh1Len, rfh2Len);

				tempbuf[curOfs] = 0;
			}
		}
	}

	// try to open the output file
	outputFile = openOutputFile(fname, tempMsg);

	// was the open successful
	if (outputFile == NULL)
	{
		// tell the user what happened
		m_error_msg += tempMsg;
	}
	else
	{
		// check if need to write an MQMD
		if (m_write_include_MQMD)
		{
			// force the MQMD version to V2
			// the mqmd is always long enough and the V2 fields will always have default values
			mqmd.Version = MQMD_VERSION_2;

			// write the MQMD to the file
			fwrite(&mqmd, 1, sizeof(MQMD), outputFile);
			mqmdLen = sizeof(MQMD);
		}

		if (curOfs > 0)
		{
			// write the headers to the file
			fwrite(tempbuf, 1, curOfs, outputFile);
		}

		if (fileSize > 0)
		{
			// write the data to the file
			fwrite(fileData, 1, fileSize, outputFile);
		}

		// close the file
		fclose(outputFile);

		// update the message area
		if ((curOfs + mqmdLen) > 0)
		{
			// found something other than data
			// report the individual lengths
			// start with the data length
			sprintf(tempRFHMsg, " (data %d ", fileSize);

			// write an MQMD?
			if (mqmdLen > 0)
			{
				sprintf(tempMsg, " mqmd %d", mqmdLen);
				strcat(tempRFHMsg, tempMsg);
			}

			// write an RFH1 header?
			if (rfh1Len > 0)
			{
				sprintf(tempMsg, " rfh1 %d", rfh1Len);
				strcat(tempRFHMsg, tempMsg);
			}

			// write an RFH2 header?
			if (rfh2Len > 0)
			{
				sprintf(tempMsg, " rfh2 %d", rfh2Len);
				strcat(tempRFHMsg, tempMsg);
			}

			// write a DLQ header?
			if (dlqLen > 0)
			{
				sprintf(tempMsg, " dlq %d", dlqLen);
				strcat(tempRFHMsg, tempMsg);
			}

			// write a CICS header?
			if (cicsLen > 0)
			{
				sprintf(tempMsg, " cics %d", cicsLen);
				strcat(tempRFHMsg, tempMsg);
			}

			// write a IMS header?
			if (imsLen > 0)
			{
				sprintf(tempMsg, " ims %d", imsLen);
				strcat(tempRFHMsg, tempMsg);
			}

			// add the final parentheses to replace the final blank
			strcat(tempRFHMsg, ")");
		}
		else
		{
			// data only - no need to report individual lengths
			tempRFHMsg[0] = 0;
		}

		sprintf(tempMsg, "%d bytes%s written to file ", fileSize + curOfs + mqmdLen, tempRFHMsg);
		m_error_msg += tempMsg;
		m_error_msg += fname;

		if (traceEnabled)
		{
			// trace results of WriteFile
			logTraceEntry(tempMsg);
		}
	}

	if (tempbuf != NULL)
	{
		// release the temporary buffer
		rfhFree(tempbuf);
	}

	if (traceEnabled)
	{
		// trace exit from WriteFile
		logTraceEntry("Exiting DataArea::WriteFile()");
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRfhutilDoc diagnostics

#ifdef _DEBUG
void DataArea::AssertValid() const
{
	CObject::AssertValid();
}

void DataArea::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);
}
#endif //_DEBUG

//////////////////////////////////////////////////////////////////////
//
// Routine to clear and reinitialize all data items
//
//////////////////////////////////////////////////////////////////////

void DataArea::DeleteContents()
{
	// clear the current contents, including headers
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	RFH			*rfhObj=(RFH *)rfhData;

	if (traceEnabled)
	{
		// trace entry to DeleteContents
		logTraceEntry("Entering DataArea::DeleteContents()");
	}

	// Clear the current data in the display and reset defaults
	// data does not contain an RFH header
	if (m_copybook != NULL)
	{
		m_copybook->resetCopybook();
	}

	m_copybook_file_name.Empty();

	// clear the current data
	ClearData();

	// check if the MQMD object has been created yet
	if (mqmdObj != NULL)
	{
		mqmdObj->clearMQMD();
	}

	// clear any user properties
	if (propData != NULL)
	{
		((CProps *)propData)->clearUserProperties();
	}

	// this test is necessary since DeleteContents() is called by the DataArea constructor before the RFH object is created
	if (rfhObj != NULL)
	{
		// clear the current contents of the RFH1 and RFH2 headers
		rfhObj->clearRFH();
	}

	// clear the CICS header fields
	if (cicsData != NULL)
	{
		((CCICS *)cicsData)->clearCICSheader();
	}

	// clear the IMS header fields
	if (imsData != NULL)
	{
		((CIms *)imsData)->clearIMSheader();
	}

	// clear the DLQ header fields
	if (dlqData != NULL)
	{
		((CDlq *)dlqData)->clearDLQheader();
	}
}

//////////////////////////////////////////////////////////////////////
//
// Routine to get rid of all saved display data
//
//////////////////////////////////////////////////////////////////////

void DataArea::resetDisplayData()

{
	saveCharCrlf = IGNORE_CRLF;
	saveCharFormat = CHAR_ASCII;
	saveBothFormat = CHAR_ASCII;
	saveXMLFormat = CHAR_ASCII;
	saveParsedFormat = CHAR_ASCII;
	saveJsonFormat = CHAR_ASCII;

	if (traceEnabled)
	{
		// trace entry to resetDataArea
		logTraceEntry("Entering DataArea::resetDisplayData()");
	}

	// get rid of any saved search information
	resetParsedFind();
	resetXMLfind();
	resetFind();
	resetHexFind();
	resetFixFind();
	resetJsonFind();

	// remove the current file source
	fileSource = "Message Data";

	curDataLineNumber = 0;

	if (m_data_ascii != NULL)
	{
		rfhFree(m_data_ascii);
		m_data_ascii = NULL;
	}

	if (m_data_ascii_UCS != NULL)
	{
		rfhFree(m_data_ascii_UCS);
		m_data_ascii_UCS = NULL;
	}

	if (m_data_hex != NULL)
	{
		rfhFree(m_data_hex);
		m_data_hex = NULL;
	}

	if (m_data_both != NULL)
	{
		rfhFree(m_data_both);
		m_data_both = NULL;
	}

	if (m_data_xml != NULL)
	{
		rfhFree(m_data_xml);
		m_data_xml = NULL;
	}

	if (m_data_parsed != NULL)
	{
		rfhFree(m_data_parsed);
		m_data_parsed = NULL;
	}

	if (m_data_json != NULL)
	{
		rfhFree(m_data_json);
		m_data_json = NULL;
	}

	if (m_data_fix != NULL)
	{
		rfhFree(m_data_fix);
		m_data_fix = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
//
// Routine to get rid of all saved display data
//
//////////////////////////////////////////////////////////////////////

void DataArea::resetDataArea()
{
	fileSize = 0;
	fileCcsid = 0;
	fileIntFormat = NUMERIC_PC;

	if (traceEnabled)
	{
		// trace entry to resetDataArea
		logTraceEntry("Entering DataArea::resetDataArea()");
	}

	if (fileData != NULL)
	{
		rfhFree(fileData);
		fileData = NULL;
	}

	resetDisplayData();
}

/////////////////////////////////////////////////////////////////////////////
//
// Routine to clear the MQMD instance variables.
//
/////////////////////////////////////////////////////////////////////////////

void DataArea::ClearData()

{
	if (traceEnabled)
	{
		// trace entry to ClearData
		logTraceEntry("Entering DataArea::ClearData()");
	}

	// get rid of the current data area and any saved display data
	resetDataArea();

	// set the queue depth to zero for now
	m_q_depth = 0;

	// clear the file name and error message variables
	m_error_msg.Empty ();
	memset(fileName, 0, sizeof(fileName));
}

//////////////////////////////////////////////////////////////
//
// Subroutine to return the file data in a hex representation
//
//////////////////////////////////////////////////////////////

void DataArea::getHexData()

{
	// turn the file data into a hex format
	unsigned char	*hexStr;
	char			ch;				// work variable
	int				i=0;				// work variable
	int				k=0;				// work variable
	UINT			len;			// length of required area to acquire
	int				buffer=0;		// offset of the next character to processs
	int				remaining;		// number of bytes left to process
	int				offset;			// current location within the raw file data
	int				count=0;			// number of characters on a particular line
	int				location=0;		// offset of start of this line within the file
	char			traceInfo[512];	// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getHexData() with fileSize=%d m_data_hex=%8.8X", fileSize, (unsigned int)m_data_hex);

		// trace entry to getHexData
		logTraceEntry(traceInfo);
	}

	// check already have the proper data formatted
	if (m_data_hex != NULL)
	{
		// already formatted - nothing to do
		return;
	}

	// calculate the required length
	// use two characters per data character plus 14 more
	// characters for every 16 characters
	len = (2 * fileSize) + ((fileSize / 8) * 7) + 47;
	m_data_hex = (unsigned char *) rfhMalloc(len, "HEXDATA ");
	m_data_hex[0] = 0;
	hexStr = m_data_hex;

	remaining = fileSize;
	while (remaining > 0)
	{
		if (remaining > 16)
		{
			count = 16;
		}
		else
		{
			count = remaining;
		} // endif

		// initialize the output strings and buffer offset
		offset = buffer;
		i = 9;    // initialize variable offset
		k = 0;    // initialize counter

		// add the offset to the beginning of the line
		sprintf((char *) hexStr, "%8.8d ", location);

		// calculate the next location
		location += 16;

		while (k < count)
		{
			ch = (unsigned char) fileData[buffer] >> 4;
			ch = HEX_NUMBERS[ch];
			hexStr[i++] = ch;

			ch = (unsigned char) fileData[buffer] & 0x0F;
			ch = HEX_NUMBERS[ch];
			hexStr[i++] = ch;

			// Go on to the next character
			remaining--;
			buffer++;
			k++;   // increment counter

			// Insert a blank for every fourth character
			if ((k%4 == 0) && (k < count))
			{
				hexStr[i++] = ' ';
			} // endif
		} // endwhile

		// overwrite the last space
		if (remaining > 0)
		{
			hexStr[i++] = '\r';		// Add a CR character to the line
			hexStr[i++] = '\n';		// Add a NL character to the line
		}

		hexStr[i] = '\0';		// Terminate the string
		hexStr += i;
	} // endwhile

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getHexData() with strlen(m_data_hex)=%d m_data_hex=%8.8X buffer=%d hexStr-m_data_hex=%d count=%d i=%d", strlen((const char *)m_data_hex), (unsigned int)m_data_hex, buffer, hexStr-m_data_hex, count, i);

		// trace exit from getHexData
		logTraceEntry(traceInfo);
	}

	return;
}

////////////////////////////////////////////////////////////////
//
// Subroutine to return the file data in an ascii representation
//  This routine handles data that is in 2-byte Unicode
//  characters.  It creates an output page that is in unicode
//  characters.
//
////////////////////////////////////////////////////////////////

void DataArea::getCharDataUcs(const int charFormat, const int crlf, const int edi)

{
	wchar_t		*asciiStr;			// pointer to result string
	wchar_t		delim;				// delimiter character for X.12 messages - offset 106 in message
									// or offset 9 in a UNA segment in an edifact message
	wchar_t		escape;				// escape character for edifact messages
	wchar_t		*tempData=NULL;		// work area to reverse bytes for CP 1201
	wchar_t		*data=NULL;			// pointer to Unicode data
	char		*ptr;				// working pointer
	int			i;					// work variable
	int			k;					// work variable
	UINT		len;				// length of required area to acquire
	int			length;				// number of bytes of Unicode characters
	int			buffer=0;
	int			remaining;			// number of bytes left to process
	int			offset;				// current location within the raw file data
	int			count;				// number of lines
	int			location=0;			// offset of start of this line within the file
	int			indent=0;			// used for edi documents
	int			extraOffset=0;		// value is 2 if a byte order mark is found at the beginning of the data
	int			endian=0;			// set to 0 if data is little endian and 1 if the data is big endian
	bool		edimsg=false;
	bool		unicode=false;
	char		defChar= L'.';		// used for any non-display characters

	// check which byte order are dealing with
	// the first step is to look for a byte order mark
	extraOffset = checkForUnicodeMarker(fileData);

	// just need to point to the current data
	data = (wchar_t *)(fileData + extraOffset);
	length = fileSize - extraOffset;

	if (extraOffset > 0)
	{
		// found a byte order marker - use it to determine if the data is little or big endian
		if ((254 == fileData[0]) && (255 == fileData[1]))
		{
			// big endian data - set endian to 1
			endian = 1;
		}
	}
	else if (1201 == fileCcsid)
	{
		// code page 1201 is big endian
		endian = 1;
	}
	else if (fileIntFormat != NUMERIC_PC)
	{
		// MQ integer encoding is big endian
		endian = 1;
	}

	// Is the data big endian?
	if (1 == endian)
	{
		// need to allocate a new area and reverse the bytes
		tempData = (wchar_t *)rfhMalloc(length + 2, "TEMPDATA");

		// set up working variables so can reverse the byte ordering
		ptr = (char *)tempData;
		i = 0;
		while (i < length)
		{
			ptr[i] = fileData[i + extraOffset + 1];
			ptr[i + 1] = fileData[i + extraOffset];
			i++;
			i++;
		}

		// create a 16-bit termination character at the end of the data
		tempData[length] = 0;
		tempData[length] = 0;

		data = tempData;	// point to temporary data area
	}

	if (edi)
	{
		if  ((fileSize > 212) && (memcmp(data, L"ISA", 6) == 0))
		{
			delim = data[105];
			edimsg = true;
		}
		else if ((fileSize > 20) && (memcmp(data, L"UNA", 6) == 0))
		{
			delim = data[8];
			escape = data[6];
			edimsg = true;
		}
		else if ((fileSize > 20) && (memcmp(data, L"UNB", 6) == 0))
		{
			delim = L'\'';
			escape = L'?';
			edimsg = true;
		}
	}

	// calculate the length required
	if ((HONOR_CRLF == crlf) || (edimsg))
	{
		// have to allow for more characters, since there may be
		// an overhead of as much as 12 characters for every character
		// in the original data stream
		len = (length * 12) + 45;
	}
	else
	{
		// use one character per data character plus 3 more
		// characters for every 32 characters
		len = length + ((length / 4) * 3) + 45;
	}

	// allocate a display buffer
	m_data_ascii_UCS = (wchar_t *) rfhMalloc(len, "ASCIIUCS");

	// point to the output data area
	asciiStr = (wchar_t *)m_data_ascii_UCS;

	// handle cases where data length is zero
	asciiStr[0] = 0;

	// figure out how much data there is (in characters)
	remaining = length >> 1;

	// loop to write out all the text lines
	// note that remaining is a character count
	while (remaining > 0)
	{
		// check if honoring crlf sequences
		if (crlf == HONOR_CRLF)
		{
			// strip off any leading crlf sequences in the front of the data
			while ((remaining > 0) && ((data[buffer] == L'\n') || (data[buffer] == L'\r')))
			{
				remaining--;
				buffer++;
				location++;
			}
		}

		// add the offset to the beginning of the line
		swprintf(asciiStr, L"%8.8d ", location);

		// check for an EDI message
		if ((edimsg) && (delim != 0))
		{
			// see if can find a delimiter in the data
			count = findDelimW(data + buffer, remaining, delim, escape);
		}
		else
		{
			// handle normal case of 32 characters per line
			if (remaining > MAX_ASCII_CHAR)
			{
				// limit display to 32 characters per line
				count = MAX_ASCII_CHAR;
			}
			else
			{
				// less than 32 - take all remaining characters
				count = remaining;
			} // endif
		}

		// check if honoring crlf sequences in the text
		if (crlf == HONOR_CRLF)
		{
			// update the count to the number of characters, up to MAX_ASCII_CHAR
			// set the count to the remaining characters in case no crlf is found
			count = remaining;
			count = findcrlfW(data + buffer, count);
		}

		// update the location counter for the display in the first 8 characters
		location += count;

		// initialize the output strings and buffer offset
		offset = buffer;
		i = 9;    // initialize variable offset

		// check if indenting
		if (edimsg)
		{
			k = 0;
			while (k < indent)
			{
				asciiStr[i++] = ' ';
				k++;
			}
		}

		for (k=0; k < count; k++)
		{
			// move character to display buffer
			asciiStr[i++] = data[buffer++];

			// Go on to the next character
			remaining--;
		} // end for

		// have a next line?
		if (remaining > 0)
		{
			asciiStr[i++] = L'\r';	// Add a carriage return character to the line
			asciiStr[i++] = L'\n';	// Add a new line character to the line
		}

		asciiStr[i] = '\0';		// Terminate the string

		asciiStr += i;
	} // endwhile

	if (tempData != NULL)
	{
		rfhFree(tempData);
	}
}

int DataArea::xlateEbcdicChar(int fromCcsid, int toCcsid, const char *input, char *output)

{
	int			tcount=0;
	char		defChar = ' ';
	wchar_t		ucsChar[4];		// area to translate one character

	// try the translation using windows
	// the called routine is locate in comsubs.cpp
	tcount = EbcdicCharToAsciiChar(fromCcsid, toCcsid, input, output);

	// did the translation work?
	if (0 == tcount)
	{
		// Windows couldn't do the translation so try MQ next
		// see if MQ provides a translation table from the input CCSID to UCS-2
		tcount = MQEBC2UCS(ucsChar, (unsigned char *)input, 1, fromCcsid);

		// was there an MQ table?
		if (tcount > 0)
		{
			tcount = WideCharToMultiByte(fromCcsid, 0, ucsChar, 1, output, 2, &defChar, NULL);
		}

		// was the combination of MQ and Windows able to do the translation?
		if (0 == tcount)
		{
			// didn't work - see if MQ can do the translation directly
			// windows could not do the translation so see if an MQ table can be used
			// if this does not work then return a 0 to indicate failure
			tcount = MQEBC2ASC((unsigned char *)output, (unsigned char *)input, 1, fromCcsid, toCcsid);
		}
	}

	// return the number of characters translated (0 if none otherwise 1)
	return tcount;
}

////////////////////////////////////////////////////////////////
//
// Subroutine to return the file data in an ascii representation
//
////////////////////////////////////////////////////////////////

void DataArea::getCharacterData(const int charFormat, const int crlf, const int edi, const int BOM, BOOL indentOn)

{
	// turn the file data into an ASCII character format
	// replace any unprintable characters with periods
	unsigned char	*asciiStr;		// pointer to result string
	unsigned char	*dataPtr = fileData;
	unsigned char	ch;				// temporary working character
	unsigned char	delim;			// delimiter character for X.12 messages - offset 106 in message
									// or offset 9 in a UNA segment in an edifact message
	unsigned char	escape;			// escape character for edifact messages
	unsigned char	*data=NULL;		// pointer to data converted from Unicode
	char			*tempData;		// work area to reverse bytes for CP 1201
	int				fileLength = fileSize;
	int				ccsid;
	int				asciiCcsid=0;	// ASCII code page to use for EBCDIC to ASCII translations
	int				dataSize;
	int				dbcsCount;
	int				tcount=0;		// number of characters translated
	int				winXlate=0;		// number of EBCDIC characters translated using Windows
	int				MQXlate=0;		// number of EBCDIC characters translated using MQ translate tables
	int				defXlate=0;		// number of EBCDIC characters translated using built-in default table
	int				i;				// work variable
	int				j;				// work variable
	int				k;				// work variable
	int				newline;		// work variable
	UINT			len;			// length of required area to acquire
	int				buffer=0;		// offset of the next character to process
	int				remaining;		// number of bytes left to process
	int				offset;			// current location within the raw file data
	int				count;			// number of lines
	int				location=0;		// offset of start of this line within the file
	int				indent=0;		// indent is used only with edi
	CRfhutilApp *	app;			// pointer to the application object
	wchar_t			*wcsPtr;		// pointer to UCS-2 data
	bool			edimsg=false;	// process the message as an EDI message
	bool			unicode=false;
	bool			worked=true;	// indicator if MQ translation has failed, to avoid repeated overhead
	unsigned char	tempTxt[4];		// workarea to translate a single character
	char			defChar='.';	// default character to replace characters that do not translate properly
	char			traceInfo[512];	// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getCharacterData() with charFormat=%d crlf=%d edi=%d BOM=%d indentOn=%d saveCharFormat=%d m_data_ascii=%8.8X fileCcsid=%d", charFormat, crlf, edi, BOM, indentOn, saveCharFormat, (unsigned int)m_data_ascii, fileCcsid);

		// trace entry to getCharacterData
		logTraceEntry(traceInfo);
	}

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// check if already have the proper data formatted
	if (m_data_ascii != NULL)
	{
		// changed options?
		if ((charFormat == saveCharFormat) && (crlf == saveCharCrlf) && (edi == saveEdi) && (saveBOM == BOM) && (saveIndentOn == indentOn))
		{
			if (traceEnabled)
			{
				// trace early exit from getCharacterData
				logTraceEntry("Exit from DataArea::getCharacterData() - reuse existing data");
			}

			// no need to do anything - no changes to formating detected
			return;
		}

		rfhFree(m_data_ascii);
		m_data_ascii = NULL;
		m_data_ascii_UCS = FALSE;
	}

	// remember the type of display data
	saveCharCrlf = crlf;
	saveEdi = edi;
	saveBOM = BOM;
	saveIndentOn = indentOn;
	saveCharFormat = charFormat;
	escape = 0;

	// check for Unicode (UCS) input
	if (((1200 == fileCcsid) || (13488 == fileCcsid) || (17584 == fileCcsid)) && (m_char_format > CHAR_EBCDIC))
	{
//		getCharDataUcs(charFormat, crlf, edi);
//		return;
	}

	if (invalidCharTableNotBuilt)
	{
		// figure out what code page this system is using
		// and figure out what the valid code points are
		buildInvalidCharTable(invalidCharTable);
		invalidCharTableNotBuilt = FALSE;
	}

	// need to do EBCDIC to ASCII translation?
	if (CHAR_EBCDIC == charFormat)
	{
		// figure out what code page to translate to
		// only need to do this once
		asciiCcsid = matchEbcdicToAscii(fileCcsid);
	}

	// check for a byte order mark (BOM)
	if (BOM)
	{
		// look for a character of FE at the front of the data
		if (fileData[0] == 254)
		{
			dataPtr++;
			fileLength--;
		}
	}

	if (edi)
	{
		if  ((fileLength > 106) && (memcmp(dataPtr, "ISA", 3) == 0))
		{
			delim = dataPtr[105];
			edimsg = true;
		}
		else
		{
			if ((fileLength > 10) && (memcmp(dataPtr, "UNA", 3) == 0))
			{
				delim = dataPtr[8];
				escape = dataPtr[6];
				edimsg = true;
			}
			else
			{
				if ((fileLength > 10) && (memcmp(dataPtr, "UNB", 3) == 0))
				{
					delim = '\'';
					escape = '?';
					edimsg = true;
				}
			}
		}
	}

	// calculate the length required
	if ((HONOR_CRLF == crlf) || (edimsg))
	{
		// have to allow for more characters, since there may be
		// an overhead of as much as 12 characters for every character
		// in the original data stream
		len = (fileLength * 12) + 45;
	}
	else
	{
		// use one character per data character plus 3 more
		// characters for every 32 characters
		len = fileLength + ((fileSize / 8) * 3) + 45;
	}

	m_data_ascii = (unsigned char *) rfhMalloc(len, "ASCIIDAT");
	m_data_ascii[0] = 0;
	asciiStr = m_data_ascii;

	// figure out how much data
	remaining = fileLength;

	// check if the data is in Unicode
	// support is provided for code pages 1200, 13488 and 17584
//	if (((1200 == fileCcsid) || (13488 == fileCcsid) || (17584 == fileCcsid)) && (m_char_format != CHAR_EBCDIC))
	if (0)
	{
		unicode = true;		// remember we are dealing with wide characters

		// figure out what code page to use
		switch (m_char_format)
		{
		case CHAR_CHINESE:
			{
				ccsid = 936;
				break;
			}
		case CHAR_KOREAN:
			{
				ccsid = 949;
				break;
			}
		case CHAR_TRAD_CHIN:
			{
				ccsid = 950;
				break;
			}
		case CHAR_JAPANESE:
			{
				ccsid = 932;
				break;
			}
		case CHAR_RUSSIAN:
			{
				ccsid = 1251;
				break;
			}
		case CHAR_THAI:
			{
				ccsid = 874;
				break;
			}
		default:
			{
				// check what the oem code page is
				if ((app->ansi >= 1250) && (app->ansi <= 1258))
				{
					// use the proper code page for this system
					ccsid = app->ansi;
				}
				else
				{
					// pick a default
					ccsid = 1252;
				}

				break;
			}
		}

		// allocate a temporary data area for the multibyte data
		data = (unsigned char *)rfhMalloc(fileLength * 2 + 4, "WCSDATA ");
		wcsPtr = (wchar_t *)&dataPtr;

		// check if big-endian data rather than little-endian
		if (NUMERIC_PC == fileIntFormat)
		{
			// data appears to be little-Endian
			// check which translation to use
			if ((ccsid >= 1250) && (ccsid <= 1258))
			{
				// use the built-in translation routines
				for (i=0; i<(int)fileSize/2; i++)
				{
					switch (ccsid)
					{
					case 1250:
						{
							// translate the current character
							WCSto1250(wcsPtr++, data + i++);

							break;
						}
					case 1251:
						{
							// translate the current character
							WCSto1251(wcsPtr++, data + i++);

							break;
						}
					case 1252:
						{
							// translate the current character
							WCSto1252(wcsPtr++, data + i++);

							break;
						}
					case 1253:
						{
							// translate the current character
							WCSto1253(wcsPtr++, data + i++);

							break;
						}
					case 1254:
						{
							// translate the current character
							WCSto1254(wcsPtr++, data + i++);

							break;
						}
					case 1255:
						{
							// translate the current character
							WCSto1255(wcsPtr++, data + i++);

							break;
						}
					case 1256:
						{
							// translate the current character
							WCSto1256(wcsPtr++, data + i++);

							break;
						}
					case 1257:
						{
							// translate the current character
							WCSto1257(wcsPtr++, data + i++);

							break;
						}
					case 1258:
						{
							// translate the current character
							WCSto1258(wcsPtr++, data + i++);

							break;
						}
					}
				}

				// set the size of the translated data
				dataSize = fileSize >> 1;
			}
			else
			{
				// try to use Windoze to do the translation
				dataSize = WideCharToMultiByte(ccsid, WC_DEFAULTCHAR, (LPCWSTR)fileData, fileSize / 2, (char *)data, fileSize * 2, &defChar, NULL);
			}
		}
		else
		{
			// data is UCS-2 and appears to be big-Endian
			// allocate another temporary area to reverse the bytes into
			tempData = (char *)rfhMalloc(fileSize + 2, "TEMPDAT2");

			i = 0;
			while (i < (int)fileLength)
			{
				// get the second of a pair of bytes
				tempData[i] = dataPtr[i + 1];
				tempData[i + 1] = dataPtr[i];
				i += 2;
			}

			// check which translation to use
			if ((ccsid >= 1250) && (ccsid <= 1258))
			{
				// point to the UCS-2 data
				wcsPtr = (wchar_t *)tempData;

				// use the built-in translation routines
				for (i=0; i<(int)fileLength/2; i++)
				{
					switch (ccsid)
					{
					case 1250:
						{
							// translate the current character
							WCSto1250(wcsPtr++, data + i++);

							break;
						}
					case 1251:
						{
							// translate the current character
							WCSto1251(wcsPtr++, data + i++);

							break;
						}
					case 1252:
						{
							// translate the current character
							WCSto1252(wcsPtr++, data + i++);

							break;
						}
					case 1253:
						{
							// translate the current character
							WCSto1253(wcsPtr++, data + i++);

							break;
						}
					case 1254:
						{
							// translate the current character
							WCSto1254(wcsPtr++, data + i++);

							break;
						}
					case 1255:
						{
							// translate the current character
							WCSto1255(wcsPtr++, data + i++);

							break;
						}
					case 1256:
						{
							// translate the current character
							WCSto1256(wcsPtr++, data + i++);

							break;
						}
					case 1257:
						{
							// translate the current character
							WCSto1257(wcsPtr++, data + i++);

							break;
						}
					case 1258:
						{
							// translate the current character
							WCSto1258(wcsPtr++, data + i++);

							break;
						}
					}
				}

				// set the size of the translated data
				dataSize = fileLength >> 1;
			}
			else
			{
				// translate from Unicode to multi-byte
				dataSize = WideCharToMultiByte(ccsid, WC_DEFAULTCHAR, (LPCWSTR)tempData, fileSize / 2, (char *)data, fileSize * 2, &defChar, NULL);
			}

			// release the temporary area
			rfhFree(tempData);
		}

		// check if the translation worked
		// it will not work if the user does not have the right code pages installed
		if (0 == dataSize)
		{
			// translation did not work - ignore unicode code page
			rfhFree(data);
			data = fileData;
			dataSize = fileLength;
			unicode = false;
		}
		else
		{
			// get the number of translated characters
			remaining = dataSize;
		}
	}
	else
	{
		data = dataPtr;
		dataSize = fileLength;
	}

	// check if have to replace the number of bytes
	// This is necessary for certain multi-byte code pages
	// with the number of characters
	if (charFormat > CHAR_EBCDIC)
	{
		dbcsCount = 0;
		i = 0;
		while (i < dataSize)
		{
			// check for two byte character
			if (isLeadChar(data[i], m_char_format))
			{
				i++;
			}

			dbcsCount++;
			i++;
		}

		remaining = dbcsCount;
	}

	// loop to write out all the text lines
	// note that remaining is a character count
	// for ascii and ebcdic, there is only one
	// character per byte, but not for dbcs
	while (remaining > 0)
	{
		// check if honoring crlf sequences
		if (crlf == HONOR_CRLF)
		{
			// strip off any leading crlf sequences in the front of the data
			while ((remaining > 0) && (('\n' == data[buffer]) || ('\r' == data[buffer])))
			{
				remaining--;
				buffer++;
				location++;
			}
		}

		// add the offset to the beginning of the line
		sprintf((char *) asciiStr, "%8.8d ", location);

		if ((edimsg) && (delim != 0))
		{
			count = findDelim(data + buffer, remaining, delim, escape);
		}
		else
		{
			if (remaining > MAX_ASCII_CHAR)
			{
				count = MAX_ASCII_CHAR;
			}
			else
			{
				count = remaining;
			} // endif
		}

		// check if honoring crlf sequences in the text
		if (crlf == HONOR_CRLF)
		{
			// update the count to the number of characters, up to MAX_ASCII_CHAR
			// set the count to the remaining characters in case no crlf is found
			count = remaining;
			count = findcrlf(data + buffer, count, charFormat);
		}

		// update the location counter for the display of the offset (first 8 characters)
		location += count;

		// initialize the output strings and buffer offset
		offset = buffer;
		i = 9;    // initialize variable offset after offset
		k = 0;    // initialize counter

		if ((edimsg) && (delim != 0))
		{
			// make sure not pointing at a CR or LF
			newline = 0;
			while (data[buffer + newline] <= ' ')
			{
				newline++;
			}

			// check if should adjust the indent
			if (memcmp(data + buffer + newline, "IEA", 3) == 0)
			{
				indent = 0;
			}
			else if (memcmp(data + buffer + newline, "GE", 2) == 0)
			{
				indent--;
			}
			else if (memcmp(data + buffer + newline, "SE", 2) == 0)
			{
				indent--;
			}

			// check if indenting
			if (indentOn)
			{
				j = 0;
				while (j < indent)
				{
					asciiStr[i++] = ' ';
					j++;
				}
			}

			// check if should adjust the indent
			if (memcmp(data + buffer + newline, "ISA", 3) == 0)
			{
				indent = 1;

				// new ISA - the delimiters may have changed - grab the new one
				if  (remaining > 106 + newline)
				{
					delim = data[buffer + newline + 105];
				}
			}
			else if (memcmp(data + buffer + newline, "GS", 2) == 0)
			{
				indent++;
			}
			else if (memcmp(data + buffer + newline, "ST", 2) == 0)
			{
				indent++;
			}
			else if (memcmp(data + buffer + newline, "UNA", 3) == 0)
			{
				// update the delimiter and escape characters
				delim = data[buffer + newline + 8];
				escape = data[buffer + newline + 6];
			}
		}

		while (k < count)
		{
			if (charFormat > CHAR_EBCDIC)
			{
				// handle double byte characters in the text
				// get the first character from the buffer
				ch = data[buffer];

				// check for double-byte character
				if (isLeadChar(ch, m_char_format))
				{
					// handle first byte of double byte character
					asciiStr[i++] = ch;

					// double-byte character
					location++;
					buffer++;

					// make sure that embedded nulls do not terminate our string prematurely
					if (0 == data[buffer])
					{
						// do not truncate the string by accident
						asciiStr[i++] = '.';
					}
					else
					{
						// not a zero character - hopefully OK
						asciiStr[i++] = data[buffer];
					}
				}
				else
				{
					asciiStr[i++] = invalidCharTable[ch];
				}
			}
			else
			{
				if (CHAR_EBCDIC == charFormat)
				{
					if (asciiCcsid > 0)
					{
						// start by trying to do a proper translation, using Windows or MQ
						tcount = xlateEbcdicChar(fileCcsid, asciiCcsid, (char *)data + buffer, (char *)tempTxt);

						// check if the translation worked
						if (1 == tcount)
						{
							// Windows or MQ was able to perform the translation
							// make sure character is displayable and
							// move on to the next output character
							asciiStr[i++] = invalidCharTable[tempTxt[0]];
							winXlate++;
						}
					}

					// wind up with a single character properly translated?
					if ((asciiCcsid <= 0) || (0 == tcount))
					{
						// use a default translation table
						asciiStr[i++] = invalidCharTable[eatab[data[buffer]]];
						defXlate++;
					}
				}
				else
				{
					// check for non-displayable characters
					// note that this check assumes code page 1252
					asciiStr[i++] = invalidCharTable[data[buffer]];
				} // endif
			}

			// Go on to the next character
			remaining--;
			buffer++;
			k++;   // increment counter
		} // endwhile

		// check if honoring crlf sequences
		if (crlf == HONOR_CRLF)
		{
			// strip off any crlf sequences, in case that is all that is left
			while ((remaining > 0) && (('\n' == data[buffer]) || ('\r' == data[buffer])))
			{
				remaining--;
				buffer++;
				location++;
			}
		}

		if (remaining > 0)
		{
			asciiStr[i++] = '\r';	// Add a carriage return character to the line
			asciiStr[i++] = '\n';	// Add a new line character to the line
		}

		asciiStr[i] = '\0';		// Terminate the string

		asciiStr += i;
	} // endwhile

	if ((unicode) && (data != NULL))
	{
		rfhFree(data);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getCharacterData() with strlen(m_data_ascii)=%d m_data_ascii=%8.8X buffer=%d location=%d i=%d winXlate=%d MQXlate=%d defXlate=%d asciiCcsid=%d", strlen((const char *)m_data_ascii), (unsigned int)m_data_ascii, buffer, location, i, winXlate, MQXlate, defXlate, asciiCcsid);

		// trace exit from getCharacterData
		logTraceEntry(traceInfo);
	}
}

void DataArea::getBothData(const int charFormat, const int BOM)

{
	// turn the file data into a hex and character format
	// replace any unprintable characters with periods
	unsigned char	*asciiStr;
	unsigned char	*hexStr;
	unsigned char	*dataPtr = fileData;
	unsigned char	ch;			// work variable
	int			i;				// work variable
	int			j;				// work variable
	int			k;				// work variable
	int			tcount;			// number of characters translated
	int			winXlate=0;		// number of EBCDIC characters translated using Windows
	int			MQXlate=0;		// number of EBCDIC characters translated using MQ translate tables
	int			defXlate=0;		// number of EBCDIC characters translated using built-in default table
	int			asciiCcsid=0;	// ASCII code page to use for EBCDIC to ASCII translations
	UINT		len;			// length of required area to acquire
	int			fileLength = fileSize;
	int			buffer=0;
	int			remaining;		// number of bytes left to process
	int			offset;			// current location within the raw file data
	int			count;			// number of lines
	int			location=0;		// offset of start of this line within the file
	wchar_t		ucsChar[4];		// area to translate one character
	char		tempCh[4];		// work area for translation
	char		defChar='.';
	char		traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getBothData() with charFormat=%d BOM=%d m_data_both=%8.8X", charFormat, BOM, (unsigned int)m_data_both);

		// trace entry to getBothData
		logTraceEntry(traceInfo);

		// check if verbose trace is enabled
		if (verboseTrace)
		{
			// dump out the formatted data
			dumpTraceData("fileData", fileData, fileSize);
		}
	}

	// check if already have the proper data formatted
	if (m_data_both != NULL)
	{
		// did options change?
		if ((saveBothFormat == charFormat) && (saveBOM == BOM))
		{
			if (traceEnabled)
			{
				// trace early exit from getBothData
				logTraceEntry("Exit from DataArea::getBothData() - reuse existing data");
			}

			// no need to do anything - no changes to formating detected
			return;
		}

		// need to change the data to match the desired format
		rfhFree(m_data_both);
	}

	if (invalidCharTableNotBuilt)
	{
		// figure out what code page this system is using
		// and figure out what the valid code points are
		buildInvalidCharTable(invalidCharTable);
		invalidCharTableNotBuilt = FALSE;
	}

	// need to do EBCDIC to ASCII translation?
	if (CHAR_EBCDIC == charFormat)
	{
		// figure out what code page to translate to
		// only need to do this once
		asciiCcsid = matchEbcdicToAscii(fileCcsid);
	}

	// remember the character format
	saveBothFormat = charFormat;

	// check for BOM
	if (BOM)
	{
		// check for a byte order mark in the data
		if (dataPtr[0] == 254)
		{
			// adjust the temporary pointer and data length
			dataPtr++;
			fileLength--;
		}
	}
	// calculate the required length
	// use three characters per data character plus 14 more
	// characters for every 16 characters
	len = (fileLength << 2) + 65;
	m_data_both = (unsigned char *) rfhMalloc(len, "BOTHDATA");
	m_data_both[0] = 0;
	hexStr = m_data_both;

	// figure out how much data
	remaining = fileLength;

	while (remaining > 0)
	{
		// add the offset to the beginning of the line
		sprintf((char *) hexStr, "%8.8d ", location);

		location += 16;

		if (remaining > 16)
		{
			count = 16;
		}
		else
		{
			count = remaining;
		} // endif

		// initialize the output strings and buffer offset
		offset = buffer;
		asciiStr = hexStr + 45;
		i = 0;    // initialize variable offset
		j = 9;    // initial variable offset

		k = 0;    // initialize counter

		while (k < count)
		{
			if (CHAR_EBCDIC == charFormat)
			{
				// recognize the ccsid as an EBCDIC code page?
				if (asciiCcsid > 0)
				{
					// start by trying to do a proper translation, using Windows
					// this requires that the code page be installed
					tcount = MultiByteToWideChar(fileCcsid, 0, (char *)dataPtr + buffer, 1, ucsChar, 1);
					winXlate += tcount;

					// check if windows was able to do the translation
					if (0 == tcount)
					{
						// see if MQ provides a translation table for the codepage in question
						tcount = MQEBC2UCS(ucsChar, dataPtr + buffer, 1, fileCcsid);
						MQXlate += tcount;
					}

					// did the translation succeed?
					if (tcount > 0)
					{
						tcount = WideCharToMultiByte(asciiCcsid, 0, ucsChar, 1, (char *)&tempCh, 2, &defChar, NULL);
					}
				}

				// wind up with a single character properly translated?
				if ((asciiCcsid > 0) && (1 == tcount))
				{
					// move on to the next output character
					ch = tempCh[0];
				}
				else
				{
					// use a default EBCDIC/ASCII translation
					ch = eatab[dataPtr[buffer]];
					defXlate++;
				}
			}
			else
			{
				ch = dataPtr[buffer];
			} // endif

			// Make sure have a character that can be displayed
			asciiStr[i++] = invalidCharTable[ch];

			// always use the file data for the hex display
			ch = (unsigned char) dataPtr[buffer] >> 4;
			ch = HEX_NUMBERS[ch];
			hexStr[j++] = ch;

			ch = (unsigned char) dataPtr[buffer] & 0x0F;
			ch = HEX_NUMBERS[ch];
			hexStr[j++] = ch;

			// Go on to the next character
			remaining--;
			buffer++;
			k++;   // increment counter

			// Insert a blank for every fourth character in hex string
			if (k%4 == 0)
			{
				hexStr[j++] = ' ';
			} // endif

			// Insert a blank for every eighth character in ASCII string
			if (k%8 == 0)
			{
				asciiStr[i++] = ' ';
			} // endif
		} // endwhile

		// set first tab character
		//asciiStr[8] = '\t';

		if (remaining > 0)
		{
			asciiStr[i++] = '\r';	// Add a carriage return character to the line
			asciiStr[i++] = '\n';	// Add a new line character to the line
		}
		else
		{
			asciiStr[i] = 0;
			// pad the ascii string with blanks to proper length
			while (j < 45)
			{
				hexStr[j++] = ' ';
			}
		}

		hexStr = asciiStr + i;
	} // endwhile

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getBothData() with strlen(m_data_both)=%d m_data_both=%8.8X winXlate=%d MQXlate=%d defXlate=%d", strlen((const char *)m_data_both), (unsigned int)m_data_both, winXlate, MQXlate, defXlate);

		// trace exit from getBothData
		logTraceEntry(traceInfo);

		// check if verbose trace is enabled
		if (verboseTrace)
		{
			// dump out the formatted data
			dumpTraceData("Both Data", m_data_both, strlen((char *)m_data_both));
		}
	}
}

////////////////////////////////////////////////////////////////
//
// Subroutine to check for the FEFF or FFFE characters at
// the beginning of the data area.
//
////////////////////////////////////////////////////////////////

int DataArea::checkForUnicodeMarker(unsigned char *data)

{
	// check for byte order mark in either big endian or little endian
	if (((255 == data[0]) && (254 == data[1])) || ((254 == data[0]) && (255 == data[1])))
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

////////////////////////////////////////////////////////////////
//
// Subroutine to return the data in ASCII.  This can involve
// translation from EBCDIC to ASCII or UCS2 to Multi-byte
//
////////////////////////////////////////////////////////////////

unsigned char * DataArea::getTranslatedData(int * length, const int charFormat, const int ccsid)

{
	int				i=0;				// work variable
	int				j=0;				// work variable
	int				tcount;				// number of characters that were tranlated by Windows
	int				tcount2;			// number of characters that were tranlated by Windows
	int				winXlate=0;			// number of characters translated by Windows
	int				MQXlate=0;			// number of characters translated using MQ tables
	int				DefXlate=0;			// number of characters translated using default table
	int				asciiCcsid=0;		// ASCII code page to use for EBCDIC to ASCII translations
	int				charCount=0;		// number of UTF-8 characters after transformation from UCS-2
	int				extraOffset=0;		// will be 2 if there is a UCS-2 file marker at the front of the data
	int				endian=0;			// used for UCS-2 data only, endian is 0 for little endian data and 1 for big endian data
	int				mByte;				// indicator if data contains multibyte characters
	int				bom;				// indicator if byte order mark found for code page 1208
	int				failedChars=0;		// number of characters that could not be translated properly
	int				failedChars2=0;		// number of characters that could not be translated back properly
	int				chars1252=0;		// number of characters translated to 1252 code page
	int				charsASCII=0;		// number of characters that no translation was necessary for
	int				charsUCS=0;			// number of characters translated to UCS
	wchar_t			ucs;				// wide character area to translate one character
	wchar_t			ucsChar[4];			// wide character area to translate one character
	unsigned char	*newData=NULL;		// pointer to the ASCII or UTF-8 data area
	unsigned char	*tempData=NULL;		// temporary work area used to convert big-endian UCS-2 data to little-endian
	unsigned char	*mbcsData;			// pointer to mbcs data to check for multibyte characters
	CRfhutilApp *	app;				// pointer to the application object
	char			traceInfo[512];		// work variable to build trace message
	char			defChar='.';		// character to be used for characters that cannot be translated

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getTranslatedData() with charFormat=%d ccsid=%d fileSize=%d", charFormat, ccsid, fileSize);

		// trace entry to getTranslatedData
		logTraceEntry(traceInfo);

		// check for verbose trace
		if (verboseTrace)
		{
			// dump out the input data
			dumpTraceData("newData", fileData, fileSize);
		}
	}

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// initialize the length field to zero
	(*length) = 0;

	// check the ccsid for a UCS-2 data area
	if ((13488 == ccsid) || (17584 == ccsid) || (1200 == ccsid) || (1201 == ccsid))
	{
		// the data is in UCS-2 format
		// the next step is to figure out whether the 16-bit characters are big or little endian
		// the first step is to look for a byte order marker at the front of the data
		extraOffset = checkForUnicodeMarker(fileData);

		if (extraOffset > 0)
		{
			// there is a byte order marker
			// check if this is a big endian marker
			if ((255 == fileData[0]) && (254 == fileData[1]))
			{
				endian = 1;
			}
		}
		else
		{
			// no byte order marker, so have to look at the code page and encoding bits
			// check the code page
			if ((13488 == ccsid) || (17584 == ccsid) || (1200 == ccsid))
			{
				// check the data encoding
				if (fileIntFormat != NUMERIC_PC)
				{
					endian = 1;
				}
			}
			else
			{
				// code page 1201 is supposed to be big-endian
				endian = 1;
			}
		}

		// get a temporary area to hold the data after it is converted to multi-byte
		newData = (unsigned char *) rfhMalloc(fileSize * 2 + 1, "MULTIBYT");

		// check if the data is little-endian (1200) or big-endian (1201)
		if (0 == endian)
		{
			// little endian data - translate it as is
			// handle two byte unicode directly in the data area
			charCount = wcstombs((char *)newData, (LPCWSTR)(fileData + extraOffset), fileSize + 1 - extraOffset);

			// check if the conversion worked
			if ((0 == charCount) && ((1252 == app->ansi) || (1252 == app->oem)))
			{
				// the conversion failed
				// try translating to code page 1252 from UCS
				for (i=0; i<(int)fileSize; i++)
				{
					// translate a character from UCS to code page 1252
					charCount = WCSto1252(&ucsChar[i], newData + i);
				}

				// count for the trace line
				chars1252++;
			}

			// check if the conversion worked
			if (charCount > 0)
			{
				(*length) = strlen((char *)newData);
				winXlate += charCount;
			}
		}
		else
		{
			// big-endian so the byte ordering must be changed before the data can be translated
			// start by converting to little-endian
			// get a temporary area to hold the data after it is converted to little-endian
			tempData = (unsigned char *) rfhMalloc(fileSize + 2, "TEMPDAT3");

			// copy the original data reversing each pair of bytes
			for (i=0; i<(int)fileSize; i += 2)
			{
				tempData[i] = fileData[i + 1];
				tempData[i + 1] = fileData[i];
			}

			// terminate the string properly (termination character is 2 bytes)
			tempData[fileSize] = 0;
			tempData[fileSize + 1] = 0;

			// convert the data from 2-byte unicode to multi-byte
			charCount = wcstombs((char *)newData, (LPCWSTR)(tempData + extraOffset), (fileSize - extraOffset)/2 + 1);

			// check if the conversion worked
			if (charCount > 0)
			{
				(*length) = strlen((char *)newData);
				winXlate += charCount;
			}

			// release the temporary data area
			rfhFree(tempData);
		}
	}
	else if (1208 == ccsid)
	{
		// initialize some working variables
		mByte = 0;
		i = 0;

		// check for a BOM at the front of the data
		bom = 0;
		if ((0xef == fileData[0]) && (0xbb == fileData[1]) && (0xbf == fileData[2]))
		{
			// if found skip it
			bom = 1;
			i = 3;
		}

		// translate the data to UCS-2 first and then back to ascii characters
		// this might help to make some european characters easier to translate
		// first check if there are any multibyte characters in the data stream
		while ((i < (int)fileSize) && (0 == mByte))
		{
			if (fileData[i] > 127)
			{
				mByte = 1;
			}

			i++;
		}

		// check if any multibyte characters were in fact found
		if ((0 == mByte) && (0 == bom))
		{
			// nothing to do
			// return a pointer to the original data
			newData = fileData;

			// set the length
			(*length) = fileSize;
		}
		else
		{
			// try to translate some of the data to ASCII
			// point to the original data
			mbcsData = fileData;

			// allocate storage to hold the translated characters
			newData = (unsigned char *)rfhMalloc(fileSize + 1, "MBCSDATA");

			if (NULL == newData)
			{
				// malloc failed - just display the original data
				newData = fileData;

				if (traceEnabled)
				{
					// trace error allocating storage
					logTraceEntry("DataArea::getTranslatedData() malloc failed");
				}
			}
			else
			{
				i = 0;
				j = 0;
				// check for a byte order mark at the front of the data
				if (1 == bom)
				{
					// skip the byte order mark characters
					i = 3;
				}

				// check if the data needs to be translated
				if (1 == mByte)
				{
					// translate the individual characters
					while (i < (int)fileSize)
					{
						// does this character need translation?
						if (mbcsData[i] < 128)
						{
							// normal ASCII character - just copy it
							newData[j++] = mbcsData[i++];

							// count for the trace line
							charsASCII++;
						}
						else
						{
							// translate the character to a single UCS-2 character
							//tcount = mbtowc(&ucs, (const char *)mbcsData + i, numBytes);
							tcount = char1208toWCS(&ucs, mbcsData + i);

							// count for the trace line
							charsUCS++;

							// did the first translation work?
							if (tcount > 0)
							{
								// conversion to UCS-2 worked
								// try to translate to the code page for this system
								tcount2 = WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_DISCARDNS, &ucs, 1, (char *)newData + j, MB_CUR_MAX + 1, ".", NULL);

								// check if the translation failed
								if (0 == tcount2)
								{
									// try again with the OEM code page for this system
									tcount2 = WideCharToMultiByte(CP_OEMCP, WC_DEFAULTCHAR | WC_DISCARDNS, &ucs, 1, (char *)newData + j, MB_CUR_MAX + 1, ".", NULL);
								}

								// did the translation fail?
								if ((0 == tcount2) && ((1252 == app->ansi) || (1252 == app->oem)))
								{
									// try translating to code page 1252
									tcount2 = WCSto1252(&ucs, newData + j);

									// count for the trace line
									chars1252++;
								}
								else if ((0 == tcount2) && ((1250 == app->ansi) || (1250 == app->oem)))
								{
									// try translating to code page 1250
									tcount2 = WCSto1250(&ucs, newData + j);

									// count for the trace line
									chars1252++;
								}
								else if ((0 == tcount2) && ((1251 == app->ansi) || (1251 == app->oem)))
								{
									// try translating to code page 1251
									tcount2 = WCSto1251(&ucs, newData + j);

									// count for the trace line
									chars1252++;
								}
								else if ((0 == tcount2) && ((1253 == app->ansi) || (1253 == app->oem)))
								{
									// try translating to code page 1253
									tcount2 = WCSto1253(&ucs, newData + j);

									// count for the trace line
									chars1252++;
								}
								else if ((0 == tcount2) && ((1254 == app->ansi) || (1254 == app->oem)))
								{
									// try translating to code page 1254
									tcount2 = WCSto1254(&ucs, newData + j);

									// count for the trace line
									chars1252++;
								}
								else if ((0 == tcount2) && ((1255 == app->ansi) || (1255 == app->oem)))
								{
									// try translating to code page 1255
									tcount2 = WCSto1255(&ucs, newData + j);

									// count for the trace line
									chars1252++;
								}
								else if ((0 == tcount2) && ((1256 == app->ansi) || (1256 == app->oem)))
								{
									// try translating to code page 1256
									tcount2 = WCSto1256(&ucs, newData + j);

									// count for the trace line
									chars1252++;
								}
								else if ((0 == tcount2) && ((1257 == app->ansi) || (1257 == app->oem)))
								{
									// try translating to code page 1257
									tcount2 = WCSto1257(&ucs, newData + j);

									// count for the trace line
									chars1252++;
								}
								else if ((0 == tcount2) && ((1258 == app->ansi) || (1258 == app->oem)))
								{
									// try translating to code page 1258
									tcount2 = WCSto1258(&ucs, newData + j);

									// count for the trace line
									chars1252++;
								}

								// did the conversion back succeed?
								if (0 == tcount2)
								{
									// keep count for trace purposes
									failedChars2++;

									// it failed - copy the original characters to the output buffer
									newData[j++] = mbcsData[i++];
								}
								else
								{
									// move past the last character in the output buffer, allowing for the null character appended
									j += tcount2;

									// skip past the characters that were processed in the input buffer
									i += tcount;
								}
							}
							else
							{
								// keep count for trace purposes
								failedChars++;

								// not a valid character - just copy it
								newData[j++] = mbcsData[i++];
							}
						}
					}
				}
				else
				{
					// just removing a byte order mark
					// copy the data starting with the fourth byte
					j = (int)fileSize - 3;
					memcpy(newData, mbcsData + 3, j);
				}
			}

			// terminate the string
			newData[j] = 0;

			// set the length
			(*length) = j;
		}
	}
	else
	{
		// Get the translated file data into a buffer
		if (charFormat == CHAR_EBCDIC)
		{
			// figure out what code page to translate to
			// only need to do this once
			asciiCcsid = matchEbcdicToAscii(ccsid);

			newData = (unsigned char *) rfhMalloc(fileSize * 2 + 1, "EBCDIC  ");
			if (ccsid > 0)
			{
				// do a translation, first trying to use the MQ translation tables and if not found
				// try the windows translation tables

				i = 0;
				j = 0;
				while (i < (int)fileSize)
				{
					if (asciiCcsid > 0)
					{
						// start by trying to do a proper translation, using Windows
						tcount = MultiByteToWideChar(ccsid, 0, (char *)fileData + i, 1, ucsChar, 1);
						winXlate += tcount;

						// check if windows was able to do the translation
						if (0 == tcount)
						{
							// see if MQ provides a translation table for the codepage in question
							tcount = MQEBC2UCS(ucsChar, fileData + i, 1, ccsid);
							MQXlate += tcount;
						}

						// did the function succeed or were able to use an MQ table?
						if (tcount > 0)
						{
							tcount = WideCharToMultiByte(asciiCcsid, 0, ucsChar, 1, (char *)newData + j, 2, &defChar, NULL);

							// check if the translation worked
							if ((0 == tcount) && ((1252 == app->ansi) || (1252 == app->oem)))
							{
								// try translating to code page 1252 from UCS
								tcount2 = WCSto1252(&ucsChar[0], newData + j);

								// count for the trace line
								chars1252++;
							}
							else if ((0 == tcount) && ((1250 == app->ansi) || (1250 == app->oem)))
							{
								// try translating to code page 1250 from UCS
								tcount2 = WCSto1250(&ucsChar[0], newData + j);

								// count for the trace line
								chars1252++;
							}
							else if ((0 == tcount) && ((1251 == app->ansi) || (1251 == app->oem)))
							{
								// try translating to code page 1251 from UCS
								tcount2 = WCSto1251(&ucsChar[0], newData + j);

								// count for the trace line
								chars1252++;
							}
							else if ((0 == tcount) && ((1253 == app->ansi) || (1253 == app->oem)))
							{
								// try translating to code page 1253 from UCS
								tcount2 = WCSto1253(&ucsChar[0], newData + j);

								// count for the trace line
								chars1252++;
							}
							else if ((0 == tcount) && ((1254 == app->ansi) || (1254 == app->oem)))
							{
								// try translating to code page 1254 from UCS
								tcount2 = WCSto1254(&ucsChar[0], newData + j);

								// count for the trace line
								chars1252++;
							}
							else if ((0 == tcount) && ((1255 == app->ansi) || (1255 == app->oem)))
							{
								// try translating to code page 1255 from UCS
								tcount2 = WCSto1255(&ucsChar[0], newData + j);

								// count for the trace line
								chars1252++;
							}
							else if ((0 == tcount) && ((1256 == app->ansi) || (1256 == app->oem)))
							{
								// try translating to code page 1256 from UCS
								tcount2 = WCSto1256(&ucsChar[0], newData + j);

								// count for the trace line
								chars1252++;
							}
							else if ((0 == tcount) && ((1257 == app->ansi) || (1257 == app->oem)))
							{
								// try translating to code page 1257 from UCS
								tcount2 = WCSto1257(&ucsChar[0], newData + j);

								// count for the trace line
								chars1252++;
							}
							else if ((0 == tcount) && ((1258 == app->ansi) || (1258 == app->oem)))
							{
								// try translating to code page 1258 from UCS
								tcount2 = WCSto1258(&ucsChar[0], newData + j);

								// count for the trace line
								chars1252++;
							}
						}
					}

					// wind up with a single character properly translated?
					if ((asciiCcsid > 0) && (1 == tcount))
					{
						// Windows was able to perform the translation
						// make sure have a displayable character and
						// move on to the next output character
						newData[j] = invalidCharTable[newData[j]];
					}
					else
					{
						// check if MQ translation failed before, to avoid the overhead of
						// going to the registry for each character
						// windows could not do the translation so see if can use an MQ table
						tcount = MQEBC2ASC(newData + j, fileData + i, 1, ccsid, asciiCcsid);

						// check if the translation worked
						if (tcount > 0)
						{
							// use the translated character
							newData[j] = invalidCharTable[newData[j]];
						}
						else
						{
							// use a default translation table
							newData[j] = invalidCharTable[eatab[fileData[i]]];
						}
					}

					i++;
					j++;
				}
				// try to use the code pages to do a better translation
				//translateEbcdicData(newData, fileData, fileSize, ccsid);
			}
			else
			{
				// use the default translation tables
				EbcdicToAscii((const unsigned char *)fileData, fileSize, newData);
				DefXlate += fileSize;
			}
		}
		else
		{
			newData = fileData;
		}

		(*length) = fileSize;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getTranslatedData() newData=%8.8X length=%d charCount=%d extraOffset=%d winXlate=%d MQXlate=%d DefXlate=%d failedChars=%d, failedChars2=%d chars1252=%d charsASCII=%d charsUCS=%d bom=%d mByte=%d i=%d j=%d", (unsigned int)newData, (*length), charCount, extraOffset, winXlate, MQXlate, DefXlate, failedChars, failedChars2, chars1252, charsASCII, charsUCS, bom, mByte, i, j);

		// trace exit from getTranslatedData
		logTraceEntry(traceInfo);

		// check for verbose trace
		if (verboseTrace)
		{
			dumpTraceData("newData", newData, (*length));
		}
	}

	return newData;
}

////////////////////////////////////////////////////////////////
//
// Subroutine to return the data in an xml representation
//
// Each subsequent level will be offset by one more columns,
// until the 10th level has been reached, at which point
// the offsets will remain at 10.
//
// Each individual xml line will be limited to 512 characters
//
////////////////////////////////////////////////////////////////

void DataArea::getXmlData(const int charFormat)

{
	// turn the file data into a xml format and return it as a CString
	// replace any unprintable characters with periods
	//
	// Recognize the following constructs
	//
	// <?xml ... > - XML Header
	// <!...     > - XML non-element item
	// <varname>..vardata..</varname>
	// <varname/>
	// <varname attr=value..> or <varname attr=value.../>
	//
	unsigned char	*tempData;			// pointer to temporary work area
	unsigned char	*tempData2;			// pointer to temporary work area
	char			*errmsg=NULL;		// pointer to error message
	int				i;					// work variable
	int				j;					// work variable
	int				savei;				// work variable
	int				rc;					// return code from called subroutines
	int				buffer=0;			// index of the next character to process
	int				remaining=0;		// number of bytes left to process
	int				len;				// length of bytes of memory to acquire
	int				length=0;			// number of bytes in the data area
	int				currentLevel=0;		// current depth relative to root element (root = 0)
	int				currentOfs=0;		// number of spaces to insert before a line
	int				endvar=0;			// indicator that end tag has been found
	int				specvar=0;			// null tag found
	int				whitespace=0;		// number of whitespace characters found between tags
	int				tempbuffer;
	char			traceInfo[512];		// work area to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getXmlData() with charFormat=%d fileCcsid=%d fileSize=%d", charFormat, fileCcsid, fileSize);

		// trace entry to getXmlData
		logTraceEntry(traceInfo);
	}

	// check if already have the proper data formatted
	if (m_data_xml != NULL)
	{
		// did we change options?
		if (charFormat == saveXMLFormat)
		{
			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, "Exiting DataArea::getXmlData() already exists m_data_xml=%8.8X strlen(m_data_xml)=%d", (unsigned int)m_data_xml, strlen((char *)m_data_xml));

				// trace exit from getXmlData
				logTraceEntry(traceInfo);
			}

			// the area already exists - just return
			return;
		}

		rfhFree(m_data_xml);
		m_data_xml = NULL;
	}

	// remember the type of display data
	saveXMLFormat = charFormat;

	// calculate the length we require
	// we use one character per data character plus 3 more
	// characters for every 32 characters for any CR or LF characters
	len = (fileSize * 4) + 4096;
	m_data_xml = (unsigned char *) rfhMalloc(len, "XMLDATA ");
	m_data_xml[0] = 0;

	// check if there is any data to format
	if (0 == fileSize)
	{
		// nothing to do
		// this will result in m_data_xml pointing to a null string
		return;
	}

	// translate the data to ASCII as necessary
	// if no translation is required a pointer to the original data in fileData is returned
	tempData2 = getTranslatedData(&length, charFormat, fileCcsid);

	// check if the data length is greater than zero
	if (0 == length)
	{
		// nothing to process
		// release the acquired data area if necessary
		if (tempData2 != fileData)
		{
			// release the acquired memory so we don't leak memory
			rfhFree(tempData2);
		}

		// return with m_data_xml set as a zero length string
		return;
	}

	//.initialize a temporary memory area that will hold the
	// processed data with any unnecessary xml characters removed
	tempData = (unsigned char *) rfhMalloc(length + 1, "TEMPDAT4");

	// check if the malloc failed
	if (NULL == tempData)
	{
		// set an error message in the data area
		strcpy((char *)m_data_xml, "**Error allocating memory");

		// release the acquired data area if necessary
		if (tempData2 != fileData)
		{
			// release the acquired memory so we don't leak memory
			rfhFree(tempData2);
		}

		// check if trace is enabled
		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "DataArea::getXmlData() memory allocation failure for %d bytes", length + 1);

			// trace entry to getXmlData
			logTraceEntry(traceInfo);
		}

		// return with m_data_xml set to an error message
		return;
	}

	// initialize the allocated memory
	memset(tempData, 0, length + 1);

	// figure out how much data we have after we
	// remove comments, invalid characters, etc
	remaining = removeUnneededXML(tempData2, length, tempData);

	// Do a quick sanity check on the data and insert a message if
	// this doesn't look like XML
	// the check includes things like making sure the number of
	// begin and end brackets match.
	rc = checkIfXml(tempData, length, &errmsg);

	if (rc > 0)
	{
		// display a message instead of the data
		strcpy((char *) m_data_xml, BAD_XML_MSG);
		strcat((char *) m_data_xml, errmsg);

		// release the acquired data area if necessary
		if (tempData2 != fileData)
		{
			// release the acquired memory so we don't leak memory
			rfhFree(tempData2);
		}

		// release the second memory area
		rfhFree(tempData);

		// is trace enabled?
		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Exiting DataArea::getXmlData() Data does not appear to be XML - rc=%d errmsg=%.128s", rc, errmsg);

			// trace exit from getXmlData
			logTraceEntry(traceInfo);
		}

		return;
	}

	// do some initialization of variables and work areas
	i = 0;
	while ((buffer < remaining) && (i < (len - 2048)))
	{
		// first, check if we have a beginning bracket
		if (tempData[buffer] != '<')
		{
			// We have some junk, so we will just put
			// it on its own line until we find a begin bracket
			// find the next beginning bracket
			whitespace = 0;
			savei = i;

			while ((buffer < remaining) && (tempData[buffer] != '<'))
			{
				// check if we have something other than a space
				if (tempData[buffer] > ' ')
				{
					whitespace = 1;
				}

				// replace any newlines, carriage returns and tabs with periods
				if ((tempData[buffer] != '\r') &&
					(tempData[buffer] != '\n') &&
					(tempData[buffer] != 127) &&
					(tempData[buffer] != '\t'))
				{
					m_data_xml[i++] = tempData[buffer++];
				}
				else
				{
					m_data_xml[i++] = '.';
					buffer++;
				}
			}

			if (1 == whitespace)
			{
				// put this junk on its own line
				m_data_xml[i++] = '\r';
				m_data_xml[i++] = '\n';

				// at this point, we have built the next line, so
				// insert the proper offset and then append the data
				j = 0;
				while ((j < currentLevel) && (j < MAX_XML_INDENT_LEVELS))
				{
					m_data_xml[i++] = ' ';
					j++;
				}
			}
			else
			{
				// write over the junk
				i = savei;
			}
		}

		// check if we found a begin bracket
		if ((buffer < remaining) && ('<' == tempData[buffer]))
		{
			// we found a begin bracket
			m_data_xml[i++] = tempData[buffer++];

			endvar = 0;
			specvar = 0;
			// check if this is the end of a variable
			if ('/' == tempData[buffer])
			{
				// Ending variable, set a variable to remind us
				endvar = 1;

				// We need to back up one character, to get the offsets correct
				i--;

				// now, reinsert the begin bracket character
				m_data_xml[i - 1] = m_data_xml[i];
			}

			// check if this is special XML construct
			if (('?' == tempData[buffer]) || ('!' == tempData[buffer]))
			{
				// Ending variable, set a variable to remind us
				specvar = 1;
			}

			// now, see if we can find an end bracket, isolating the variable name
			while ((buffer < remaining) && (tempData[buffer] != '>'))
			{
				// if we are in a special part of the XML, we need to
				// look for square brackets and ignore anything in between
				if ((1 == specvar) && ('[' == tempData[buffer]))
				{
					// found square bracket, now we must find
					// the trailing square bracket, ignoring
					// anything in between
					// first, get the square bracket
					m_data_xml[i++] = tempData[buffer++];

					// now, capture all characters on this line until we
					// find the trailing bracket
					while ((buffer < remaining) && (tempData[buffer] != ']'))
					{
						m_data_xml[i++] = tempData[buffer++];
					}
				}

				if (buffer < remaining)
				{
					if ((tempData[buffer] >= ' ') && (tempData[buffer] != 127))
					{
						// check for an escape sequence
						if ('&' == tempData[buffer])
						{
							// found an escape sequence
							buffer += setEscChar(tempData + buffer, m_data_xml + i++);
						}
						else
						{
							m_data_xml[i++] = tempData[buffer++];
						}
					}
					else
					{
						m_data_xml[i++] = '.';
						buffer++;
					}
				}
			}

			if ((buffer < remaining) && ('>' == tempData[buffer]))
			{
				if ('/' == tempData[buffer-1])
				{
					specvar = 1;
				}

				// First, append the end bracket to the work area
				m_data_xml[i++] = tempData[buffer++];

				// We have find the end of a variable name
				// check if we are beginning a new variable
				// or ending a previous one
				if ((1 == endvar) || (1 == specvar))
				{
					if (1 == endvar)
					{
						// ending a previous variable
						// don't allow negative numbers in level
						if (currentLevel > 0)
						{
							// decrement the level of this variable
							currentLevel--;
						}
						else
						{
							// Result would have been negative - flag as error
							strcpy((char *) m_data_xml + i, "***** RFHUtil format error parsing XML - too few levels*****");
							i += strlen((char *) m_data_xml + i);
						}
					}

					// append this variable with the proper offset
					m_data_xml[i++] = '\r';
					m_data_xml[i++] = '\n';

					// at this point, we have built the next line, so
					// insert the proper offset and then append the data
					j = 0;
					while ((j < currentLevel) && (j < MAX_XML_INDENT_LEVELS))
					{
						m_data_xml[i++] = ' ';
						j++;
					}
				}
				else
				{
					// beginning a new variable name
					// first, increase the level
					currentLevel++;

					// we may have some whitespace between this item and the next
					// we need to possibly skip this
					tempbuffer = buffer;
					while ((tempbuffer < remaining) && (tempData[tempbuffer] != '<'))
					{
						tempbuffer++;
					}

					// At this point, we want to find the rest of the line,
					// starting with any data for this variable
					// If there is no data, we need to check if we are starting a
					// new level or if we just have a null variable
					if (('<' == tempData[tempbuffer]) &&
						(tempData[tempbuffer + 1] != '/') &&
						(memcmp(tempData + buffer, CDATA_START, sizeof(CDATA_START) - 1) != 0))
					{
						// start a new line
						m_data_xml[i++] = '\r';
						m_data_xml[i++] = '\n';

						j = 0;
						while ((j < currentLevel) && (j < MAX_XML_INDENT_LEVELS))
						{
							m_data_xml[i++] = ' ';
							j++;
						}
					}
					else
					{
						// We may have some data, so we will just put
						// it on the same line
						// check for CDATA
						if ((buffer < remaining - 10) &&
							(memcmp(tempData + buffer, CDATA_START, sizeof(CDATA_START) - 1) == 0))
						{
							// process data as CDATA
							// skip the CDATA header
							buffer += sizeof(CDATA_START) - 1;

							// append the CDATA header to the output buffer
							strcpy((char *) m_data_xml + i, CDATA_START);
							i += sizeof(CDATA_START) - 1;

							while ((buffer < remaining) &&
								   (memcmp(tempData + buffer, CDATA_END, sizeof(CDATA_END) - 1) != 0))
							{
								if ((tempData[buffer] != '\r') &&
									(tempData[buffer] != '\n') &&
									(tempData[buffer] != 127) &&
									(tempData[buffer] != '\t'))
								{
									m_data_xml[i++] = tempData[buffer++];
								}
								else
								{
									// count these characters as data
									whitespace = 1;
									m_data_xml[i++] = ' ';
									buffer++;
								}
							}

							// check if we found the CDATA end
							if (buffer < remaining)
							{
								// skip the end of the CDATA
								buffer += sizeof(CDATA_END) - 1;

								// append the CDATA end to the output buffer
								strcpy((char *) m_data_xml + i, CDATA_END);
								i += sizeof(CDATA_END) - 1;
							}
						}
						else
						{
							// handle as normal data
							while ((buffer < remaining) && (tempData[buffer] != '<'))
							{
								if ((tempData[buffer] >= ' ') && (tempData[buffer] != 127))
								{
									// check for an escape character in the data
									if ('&' == tempData[buffer])
									{
										// handle the escape sequence
										buffer += setEscChar(tempData + buffer, m_data_xml + i++);
									}
									else
									{
										m_data_xml[i++] = tempData[buffer++];
									}
								}
								else
								{
									// count these characters as data but substitute a period in the data display
									whitespace = 1;
									m_data_xml[i++] = '.';
									buffer++;
								}
							}
						}

						// check if we found only white space and a new variable or
						// if we found an end variable
						if ((buffer < remaining) && ('<' == tempData[buffer]) && ('/' == tempData[buffer + 1]))
						{
							// Now, check if we found a beginning bracket
							if (buffer < remaining)
							{
								// we must have found a beginning bracket,
								// so append it to the current line
								m_data_xml[i++] = tempData[buffer++];
							}

							// append the rest of the variable name to the work area
							while ((buffer < remaining) && (tempData[buffer] != '>'))
							{
								if ((tempData[buffer] != '\r') &&
									(tempData[buffer] != '\n') &&
									(tempData[buffer] != 127) &&
									(tempData[buffer] != '\t'))
								{
									m_data_xml[i++] = tempData[buffer++];
								}
								else
								{
									m_data_xml[i++] = '.';
									buffer++;
								}
							}

							// check if we found the trailing end bracket
							if (buffer < remaining)
							{
								// now, this should be an end variable
								// append the ending bracket
								m_data_xml[i++] = tempData[buffer++];
								currentLevel--;
							}
							else
							{
								// append an error message
								strcpy((char *) m_data_xml + i, "\r\n***** RFHUtil format error parsing XML - no trailing end bracket*****");
								i += strlen((char *) m_data_xml + i);
							}
						}

						// append a new line to the work area
						m_data_xml[i++] = '\r';
						m_data_xml[i++] = '\n';

						// at this point, we have built the next line, so
						// insert the proper offset and then append the data
						j = 0;
						while ((j < currentLevel) && (j < MAX_XML_INDENT_LEVELS))
						{
							m_data_xml[i++] = ' ';
							j++;
						}
					}
				}
			}
			else
			{
				// Didn't find an end bracket, put the rest of the line on a separate line
				strcpy((char *) m_data_xml + i, "\r\n***** RFHUtil format error - missing end bracket\r\n");
				i += strlen((char *) m_data_xml + i);
			}
		}

	}

	// check if there were a balanced number of begin and end tags
	if (currentLevel != 0)
	{
		strcpy((char *) m_data_xml + i, "\r\n***** RFHUtil format error - Unmatched beginning and ending tags found");
		i += strlen((char *) m_data_xml + i);
	}

	// terminate the string
	m_data_xml[i] = 0;

	Rtrim((char *)m_data_xml);

	// release the temporary data areas
	rfhFree(tempData);
	if (tempData2 != fileData)
	{
		rfhFree(tempData2);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getXmlData() len=%d strlen(m_data_xml)=%d i=%d buffer=%d remaining=%d", len, strlen((const char *)m_data_xml), i, buffer, remaining);

		// trace exit from getXmlData
		logTraceEntry(traceInfo);
	}

	return;
}

void DataArea::getJsonData(const int charFormat)

{
	int				result;				// result of parsing the user data
	int				len=0;				// length of formatted data area
	char			*ptr;				// pointer to formatted data area
	CJsonParse		json;				// JSON parser object
	char			traceInfo[512];		// work area to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getJsonData() with charFormat=%d fileCcsid=%d fileSize=%d", charFormat, fileCcsid, fileSize);

		// trace entry to getJsonData
		logTraceEntry(traceInfo);
	}

	// check if we already have the proper data formatted
	if (m_data_json != NULL)
	{
		// did we change options?
		if (charFormat == saveJsonFormat)
		{
			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, "Exiting DataArea::getJsonData() already exists m_data_json=%8.8X strlen(m_data_json)=%d", (unsigned int)m_data_json, strlen((char *)m_data_json));

				// trace exit from getJsonData
				logTraceEntry(traceInfo);
			}

			// the area already exists - just return
			return;
		}

		rfhFree(m_data_json);
		m_data_json = NULL;
	}

	// parse the user data to create a DOM-like table in the parser object
	result = json.parse((const char *)fileData, fileSize);

	// check if the parse worked
	if (result != PARSE_OK)
	{
		// error parsing usr area
		m_error_msg = "Error parsing user data";
		updateMsgText();

		if (traceEnabled)
		{
			// explain the error
			sprintf(traceInfo, "Error parsing user data - %d", result);

			// trace exit from getJsonData
			logTraceEntry(traceInfo);

			// dump out the usr area
			dumpTraceData("fileData", (unsigned char *)fileData, fileSize);
		}

		return;
	}

	// allocate storage for a temporary data to format the data into
	ptr = (char *)rfhMalloc(fileSize * 16 + 8192, "JSONDATA");

	// make sure the malloc worked
	if (NULL == ptr)
	{
		// tell the user what happened
		// error parsing usr area
		m_error_msg = "Error allocating storage";
		updateMsgText();

		if (traceEnabled)
		{
			// explain the error
			sprintf(traceInfo, "Memory allocation failed - %d in getJsonData()", fileSize * 16 + 8192);

			// trace exit from getJsonData
			logTraceEntry(traceInfo);
		}

		// return
		return;
	}

	// create the parsed area
	ptr[0] = 0;
	json.buildParsedArea(ptr, fileSize * 16 + 8000, FALSE);

	// get the amount of storage that was really used
	len = strlen(ptr);

	// allocate the necessary amount of storage
	m_data_json = (unsigned char *)rfhMalloc(len + 16, "JSONDAT2");

	// check if the malloc worked
	if (NULL == m_data_json)
	{
		// malloc failed
		// use the storage that was already acquired
		m_data_json = (unsigned char *)ptr;
	}
	else
	{
		// copy the formatted data to the actual area
		memcpy(m_data_json, ptr, len);

		// terminate the string
		m_data_json[len] = 0;
		m_data_json[len + 1] = 0;

		// free the acquired storage
		rfhFree(ptr);
	}

	// remember the type of display data
	saveJsonFormat = charFormat;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getJsonData() len=%d strlen(m_data_json)=%d", len, strlen((const char *)m_data_json));

		// trace exit from getJsonData
		logTraceEntry(traceInfo);
	}
}

void DataArea::getFixData(const int charFormat)

{
	int				len=0;				// length of formatted data area
	int				ofs=0;				// offset within the input data
	int				size=0;				// number of bytes to allocate
	int				tcount=0;			// indicator if character translation was successful
	int				asciiCcsid=0;		// ASCII code page to use for EBCDIC to ASCII translations
	char			tempTxt[4];			// work area to hold translated characters
	char			traceInfo[512];		// work area to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getFixData() with charFormat=%d fileCcsid=%d fileSize=%d", charFormat, fileCcsid, fileSize);

		// trace entry to getFixData
		logTraceEntry(traceInfo);
	}

	// check if we already have the proper data formatted
	if (m_data_fix != NULL)
	{
		// did we change options?
		if (charFormat == saveFixFormat)
		{
			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, "Exiting DataArea::getFixData() already exists m_data_fix=%8.8X strlen(m_data_fix)=%d", (unsigned int)m_data_fix, strlen((char *)m_data_fix));

				// trace exit from getFixData
				logTraceEntry(traceInfo);
			}

			// the area already exists - just return
			return;
		}

		rfhFree(m_data_fix);
		m_data_fix = NULL;
	}

	// allocate storage for a temporary data to format the data into
	// each binary x'01' will be replaced by a CRLF sequence - the maximum storage that can be used is 2X the input
	// if the data is not fix format a message will be placed in the area instead
	size = fileSize * 2 + 512;
	m_data_fix = (unsigned char *)rfhMalloc(size, "FIXDATA ");

	// make sure the malloc worked
	if (NULL == m_data_fix)
	{
		// tell the user what happened
		// error parsing usr area
		m_error_msg = "Error allocating storage";
		updateMsgText();

		if (traceEnabled)
		{
			// explain the error
			sprintf(traceInfo, "Memory allocation failed - %d in getFixData()", size);

			// trace exit from getFixData
			logTraceEntry(traceInfo);
		}

		// return
		return;
	}

	// do we need to do EBCDIC to ASCII translation?
	if (CHAR_EBCDIC == charFormat)
	{
		// figure out what code page to translate to
		// we only need to do this once
		asciiCcsid = matchEbcdicToAscii(fileCcsid);
	}

	// start to work through the data one character at a time
	while (ofs < (int)fileSize)
	{
		// check for a binary x'01'
		if (1 == fileData[ofs])
		{
			// insert a CRLF sequence in the output
			m_data_fix[len++] = '\r';
			m_data_fix[len++] = '\n';
		}
		else
		{
			// does the character need to be translated?
			if (CHAR_EBCDIC == charFormat)
			{
				// translate the character to EBCDIC
				if (asciiCcsid > 0)
				{
					// start by trying to do a proper translation, using Windows or MQ
					tcount = xlateEbcdicChar(fileCcsid, asciiCcsid, (char *)fileData + ofs, (char *)tempTxt);

					// check if the translation worked
					if (1 == tcount)
					{
						// Windows or MQ was able to perform the translation
						// make sure we have a displayable character and
						// move on to the next output character
						m_data_fix[len] = invalidCharTable[tempTxt[0]];
					}
				}

				// did we wind up with a single character properly translated?
				if ((asciiCcsid <= 0) || (0 == tcount))
				{
					// use a default translation table
					m_data_fix[len] = invalidCharTable[eatab[fileData[ofs]]];
				}
			}
			else
			{
				// move the input character to the output, making sure it is a valid display character
				m_data_fix[len] = invalidCharTable[fileData[ofs]];
			}

			// move on to the next output character
			len++;
		}

		// move on to the next input character
		ofs++;
	}

	// terminate the string
	m_data_fix[len] = 0;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getFixData() size=%d len=%d strlen(m_data_fix)=%d", size, len, strlen((const char *)m_data_fix));

		// trace exit from getFixData
		logTraceEntry(traceInfo);
	}
}

////////////////////////////////////////////////////////////////
//
// Subroutine to return the file data in an parsed
// representation.  The input data must be in xml format.
//
////////////////////////////////////////////////////////////////

void DataArea::getNameValueData(const unsigned char * input, int inputLen, unsigned char * output, int maxLength, int ccsid, const int charFormat)

{
	// turn the file data into a xml format and return it as a CString
	// replace any unprintable characters with periods.
	// drop comments, embedded DTDs, etc
	//
	// Recognize the following constructs
	//
	// <?xml ... > - XML Header
	// <varname>..vardata..</varname>
	// <varname/>
	// <varname attr=value..> or <varname attr=value.../>
	//
	
	char	*errmsg=NULL;
	unsigned char	*parsed_data=output;
	unsigned char	*tempData;
	unsigned char	*tempData2;
	int		endName;
	int		attrLen;
	int		i;					// work variable
	int		j;					// work variable
	int		k;					// work variable
	int		rc;					// return code from subroutine call
	int		templen;
	int		savei;
	int		varNameLen=0;		// length of the tag name
	int		buffer=0;			// index of the next character to process
	int		remaining;			// number of bytes left to process
	int		length=0;			// number of bytes in the data area after any translation
	int		currentLevel=0;		// current depth relative to root element (root = 0)
	int		currentOfs=0;		// number of spaces to insert at beginning of a line
	int		endvar=0;			// end tag found
	int		specvar=0;			// null tag found
	int		whitespace=0;		// number of whitespace characters found between tags
	char	traceInfo[512];		// work area to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getNameValueData() inputLen=%d maxLength=%d ccsid=%d charFormat=%d", inputLen, maxLength, ccsid, charFormat);

		// trace entry to getNameValueData
		logTraceEntry(traceInfo);
	}

	// check if there is any data to format
	if (0 == inputLen)
	{
		// nothing to do
		output[0] = 0;
		return;
	}

	// translate the data to ASCII as necessary
	// if no translation is required a pointer to the original data in fileData is returned
	length = inputLen;
	tempData2 = getTranslatedData(&length, charFormat, ccsid);

	// make sure that the translation worked
	if (0 == tempData2)
	{
		// just return
		return;
	}

	// check if the data length is greater than zero
	if (0 == length)
	{
		// nothing to process
		// release the acquired data area if necessary
		if (tempData2 != input)
		{
			// release the acquired memory so we don't leak memory
			rfhFree(tempData2);
		}

		// return with m_data_xml set as a zero length string
		return;
	}

	//.initialize a temporary memory area that will hold the
	// processed data with any unnecessary xml characters removed
	tempData = (unsigned char *) rfhMalloc(length + 1, "TEMPDAT5");
	memset(tempData, 0, length + 1);
	memset(xmlVarName, 0, sizeof(xmlVarName));
	memset(xmlAttrStr, 0, sizeof(xmlAttrStr));

	// figure out how much data we have after we
	// remove comments, invalid characters, etc
	remaining = removeUnneededXML(tempData2, length, tempData);

	// Do a quick sanity check on the data and inset a message if
	// this doesn't look like XML
	// the check includes things like making sure the number of
	// begin and end brackets match.
	rc = checkIfXml(tempData, length, &errmsg);

	if (rc > 0)
	{
		// display a message instead of the data
		strcpy((char *) parsed_data, BAD_XML_MSG);
		strcat((char *) parsed_data, errmsg);

		// release the acquired data areas if necessary
		rfhFree(tempData);
		if (tempData2 != input)
		{
			// release the acquired memory so we don't leak memory
			rfhFree(tempData2);
		}

		// is trace enabled?
		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Exiting DataArea::getNameValueData() Data does not appear to be XML - rc=%d errmsg=%.128s", rc, errmsg);

			// trace exit from getNameValueData
			logTraceEntry(traceInfo);
		}

		return;
	}

	// do some initialization of variables and work areas
	i = 0;

	while ((buffer < remaining) && (i < (maxLength - 2048)))
	{
		// first, check if we have a beginning bracket
		if (tempData[buffer] != '<')
		{
			// We have some junk, so we will just put
			// it on its own line until we find a begin bracket
			// find the next beginning bracket
			whitespace = 0;
			savei = i;

			while ((buffer < remaining) && (tempData[buffer] != '<'))
			{
				// check if we have something other than a space
				if ((tempData[buffer] > ' ') && (tempData[buffer] < 128))
				{
					whitespace = 1;
				}

				if ((tempData[buffer] > ' ') && (tempData[buffer] < 127))
				{
					parsed_data[i++] = tempData[buffer++];
				}
				else
				{
					// replace with periods
					parsed_data[i++] = '.';
					buffer++;
				}
			}

			if (1 == whitespace)
			{
				// put this junk on its own line
				parsed_data[i++] = '\r';
				parsed_data[i++] = '\n';

				// at this point, we have built the next line, so
				// insert the proper offset and then append the data
				j = 0;
				while (j < currentLevel)
				{
					parsed_data[i++] = ' ';
					j++;
				}
			}
			else
			{
				// write over the junk
				i = savei;
			}
		}

		// check if we found a begin bracket
		if ((buffer < remaining) && ('<' == tempData[buffer]))
		{
			// we found a begin bracket
			// skip it
			buffer++;

			endvar = 0;
			specvar = 0;
			tempVarName[0] = 0;
			j = 0;

			// check if this is the end of a variable
			if ('/' == tempData[buffer])
			{
				// Ending variable, set a variable to remind us
				endvar = 1;
			}

			// check if this is special XML
			if (('?' == tempData[buffer]) || ('!' == tempData[buffer]))
			{
				// Ending variable, set a variable to remind us
				specvar = 1;
			}

			// now, see if we can find an end bracket, isolating the variable name
			endName = 0;
			attrLen = 0;
			while ((buffer < remaining) && (tempData[buffer] != '>'))
			{
				if ((tempData[buffer] <= ' ') && (endName == 0))
				{
					// switch to the attributes if we find a space
					endName = 1;
					buffer++;
				}
				else
				{
					if (0 == endName)
					{
						//capture the variable name
						tempVarName[j++] = tempData[buffer++];
					}
					else
					{
						// ignore any leading spaces in the attributes
						if ((attrLen > 0) || (tempData[buffer] > ' '))
						{
							// capture the attributes
							tempVarAttr[attrLen++] = tempData[buffer++];
						}
						else
						{
							buffer++;
						}
					}
				}
			}

			// terminate the strings
			tempVarName[j] = 0;
			tempVarAttr[attrLen] = 0;

			// get rid of any trailing slash or space characters
			while ((attrLen > 0) && (('/' == tempVarAttr[attrLen]) || (tempVarAttr[attrLen] <= ' ')))
			{
				tempVarAttr[attrLen--] = 0;
			}

			if ('>' == tempData[buffer])
			{
				if ('/' == tempData[buffer-1])
				{
					specvar = 1;

					// remove the / from the variable name
					if ('/' == tempVarName[strlen(tempVarName) - 1])
					{
						tempVarName[strlen(tempVarName) - 1] = 0;
					}
				}

				// First, skip the end bracket
				buffer++;

				// We have the variable name isolated
				// check if we are beginning a new variable
				// or ending a previous one
				if ((1 == endvar) || (1 == specvar))
				{
					if (1 == endvar)
					{
						// ending a previous variable
						// don't allow negative numbers in level
						if (currentLevel > 0)
						{
							// decrement the level of this variable
							currentLevel--;

							// remove the last variable from the name list
							rc = removeVarName(xmlVarName, tempVarName);

							if (rc > 0)
							{
								// append an error message
								sprintf((char *) parsed_data + i, "***** RFHUtil format error - end name does not match %s %s\r\n", xmlVarName, tempVarName);
								i += strlen((char *) parsed_data + i);
							}
						}
						else
						{
							// Result would have been negative - flag as error
							strcpy((char *) parsed_data + i, "***** RFHUtil format error parsing XML - too few levels*****\r\n");
							i += strlen((char *) parsed_data + i);
						}
					}

					// check if this is an empty variable
					// otherwise it is ignored
					if ((1 == specvar) && ('/' == tempData[buffer-2]))
					{
						// create a null entry for this value
						if ('\\' == tempVarName[strlen(tempVarName) - 1])
						{
							tempVarName[strlen(tempVarName) - 1] = 0;
						}

						// get any leading names in the hierarchy
						if (strlen(xmlVarName) > 0)
						{
							strcpy((char *) parsed_data + i, xmlVarName);
							i += strlen(xmlVarName);
							parsed_data[i++] = '.';
						}

						// add the empty variable to the display
						strcpy((char *) parsed_data + i, tempVarName);
						i += strlen(tempVarName);
						strcpy((char *) parsed_data + i, "=\'\'");
						i += 3;

						// append a carriage return/new line pair to start a new line
						parsed_data[i++] = '\r';
						parsed_data[i++] = '\n';

						// this is an empty variable but check for attributes
						if (attrLen > 0)
						{
							// start with a zero length output string
							xmlAttrStr[0] = 0;

							// extract the individual attributes into a temporary string
							processAttributes(xmlVarName, tempVarName, tempVarAttr, xmlAttrStr);

							// capture the attributes in the output area
							strcpy((char *) parsed_data + i, xmlAttrStr);
							i += strlen(xmlAttrStr);
						}

					}
				}
				else
				{
					// beginning a new variable name
					// first, increase the level
					currentLevel++;

					// build a string with the current
					// higher level variables
					if (xmlVarName[0] != 0)
					{
						strcat(xmlVarName, ".");
					}

					// append the latest variable name
					strcat(xmlVarName, tempVarName);

					// At this point, we want to find the rest of the line,
					// starting with any data for this variable
					// If there is no data, we need to check if we are starting a
					// new level or if we just have a null variable
					if (('<' == tempData[buffer]) &&
						(tempData[buffer + 1] != '/') &&
						(memcmp(tempData + buffer, CDATA_START, 9) != 0))
					{
						// We are starting a new level, so let's stop at this point
						// append to the display data
						strcpy((char *) parsed_data + i, xmlVarName);
						i += strlen(xmlVarName);
						parsed_data[i++] = '\r';
						parsed_data[i++] = '\n';
					}
					else
					{
						strcpy((char *) parsed_data + i, xmlVarName);
						i += strlen(xmlVarName);

						// We may have some data, so we will collect it
						// in a temporary variable
						templen = 0;
						whitespace = 0;

						// check for a CDATA section
						if ((buffer < remaining - 10) && (memcmp(tempData + buffer, CDATA_START, 9) == 0))
						{
							// found a CDATA section
							buffer += 9;

							// now, gather the data
							while ((buffer < remaining) &&
								   (memcmp(tempData + buffer, CDATA_END, 3) != 0) &&
								   (templen < (sizeof(tempVarValue) - 1)))
							{
								if ((tempData[buffer] >= ' ') && (tempData[buffer] != 127))
								{
									tempVarValue[templen++] = tempData[buffer++];
								}
								else
								{
									// replace non-display characters with a period
									tempVarValue[templen++] = '.';
									buffer++;
								}
							}

							// check that the end of the CDATA section was found
							if ((buffer < remaining) && (memcmp(tempData + buffer, CDATA_END, 3) == 0))
							{
								// account for the terminating characters
								buffer += 3;

								// skip any whitespace at the end of the CDATA
								while ((buffer < remaining) && (tempData[buffer] < ' '))
								{
									// skip the whitespace
									buffer++;
								}
							}
						}
						else
						{
							// handle as ordinary data
							while ((buffer < remaining) && (tempData[buffer] != '<'))
							{
								// check if we have non-whitespace characters
								if (tempData[buffer] > ' ')
								{
									whitespace = 1;
								}

								// replace any non-printable characters with blanks
								if ((tempData[buffer] >= ' ') && (tempData[buffer] != 127))
								{
									// normal character
									tempVarValue[templen++] = tempData[buffer++];
								}
								else
								{
									// replace with a period
									tempVarValue[templen++] = '.';
									buffer++;
								}
							}
						}

						// terminate the value string, remove escape
						// sequences and append it
						tempVarValue[templen] = 0;

						// check if we should append the value
						if (('<' == tempData[buffer]) && ('/' == tempData[buffer + 1]))
						{
							parsed_data[i++] = '=';
							parsed_data[i++] = '\'';
							removeEscSeq(tempVarValue);
							strcpy((char *) parsed_data + i, tempVarValue);
							i += strlen(tempVarValue);
							parsed_data[i++] = '\'';
						}

						// append a new line to the work area
						parsed_data[i++] = '\r';
						parsed_data[i++] = '\n';

						// Now, check if we found a beginning bracket
						if ((buffer < remaining) && ('<' == tempData[buffer]) && ('/' == tempData[buffer + 1]))
						{
							// we must have found a beginning bracket,
							// so skip it
							buffer++;
							k = 0;

							// isolate the ending variable name
							while ((buffer < remaining) && (tempData[buffer] != '>'))
							{
								if (tempData[buffer] <= ' ')
								{
									// terminate the variable name
									tempEndVarName[k++] = 0;
									buffer++;
								}
								else
								{
									tempEndVarName[k++] = tempData[buffer++];
								}
							}

							// terminate the string
							tempEndVarName[k] = 0;

							// check if we found the trailing end bracket
							if (buffer < remaining)
							{
								// now, this should be an end variable
								// skip the ending bracket
								buffer++;
								currentLevel--;

								// Check if this is an ending element
								if ('/' == tempEndVarName[0])
								{
									// yes it is, check if it matches
									// remove the last variable from the name list
									rc = removeVarName(xmlVarName, tempEndVarName);

									if (rc > 0)
									{
										strcpy((char *) parsed_data + i, "***** RFHUtil format error - end name does not match\r\n");
										i += strlen((char *) parsed_data + i);
									}
								}
								else
								{
									strcpy((char *) parsed_data + i, "***** RFHUtil format error parsing XML - missing element end\r\n");
									i += strlen((char *) parsed_data + i);
								}

							}
							else
							{
								// append an error message
								strcpy((char *) parsed_data + i, "***** RFHUtil format error parsing XML - no trailing end bracket\r\n");
								i += strlen((char *) parsed_data + i);
							}
						}
					}

					// check if we have found any attributes we should display
					if (attrLen > 0)
					{
						// start with a zero length output string
						xmlAttrStr[0] = 0;

						// get the attributes in the right display format
						processAttributes(xmlVarName, "", tempVarAttr, xmlAttrStr);

						// append the attribute lines to the output
						strcpy((char *) parsed_data + i, xmlAttrStr);

						// account for the storage in the output
						i += strlen(xmlAttrStr);
					}
				}
			}
			else
			{
				// Didn't find an end bracket, put the rest of the line on a separate line
				strcpy((char *) parsed_data + i, "***** RFHUtil format error - missing end bracket\r\n");
				i += strlen((char *) parsed_data + i);
			}
		}

	}

	// terminate the string
	parsed_data[i] = 0;

	Rtrim((char *)parsed_data);

	// release the temporary buffers
	rfhFree(tempData);
	if (tempData2 != input)
	{
		rfhFree(tempData2);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getNameValueData() maxLength=%d strlen(parsed_data)=%d i=%d buffer=%d remaining=%d", maxLength, strlen((const char *)parsed_data), i, buffer, remaining);

		// trace exit from getNameValueData
		logTraceEntry(traceInfo);
	}

	return;
}

////////////////////////////////////////////////////////////////
//
// Each subsequent level will be offset by one more column,
// until the 10th level has been reached, at which point
// the offsets will remain at 10.
//
// Each individual xml line will be limited to 512 characters
//
////////////////////////////////////////////////////////////////

void DataArea::getParsedData(const int charFormat)

{
	// turn the file data into a xml format and return it as a CString
	// replace any unprintable characters with periods.
	// drop comments, embedded DTDs, etc
	//
	// Recognize the following constructs
	//
	// <?xml ... > - XML Header
	// <varname>..vardata..</varname>
	// <varname/>
	// <varname attr=value..> or <varname attr=value.../>
	//
	char	*errmsg=NULL;
	int		varNameLen=0;		// length of the tag name
	int		buffer=0;			// index of the next character to process
	int		len;				// number of bytes of storage acquired for the m_data_parsed area
	int		length=0;			// number of bytes in the data area after any translation
	int		currentLevel=0;		// current depth relative to root element (root = 0)
	int		currentOfs=0;		// number of spaces to insert at beginning of a line
	int		endvar=0;			// end tag found
	int		specvar=0;			// null tag found
	int		whitespace=0;		// number of whitespace characters found between tags
	char	traceInfo[512];		// work area to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getParsedData() with charFormat=%d fileCcsid=%d fileSize=%d", charFormat, fileCcsid, fileSize);

		// trace entry to getParsedData
		logTraceEntry(traceInfo);
	}

	// check if we already have the proper data formatted
	if (m_data_parsed != NULL)
	{
		// did we change options?
		if (charFormat == saveParsedFormat)
		{
			return;
		}

		rfhFree(m_data_parsed);
		m_data_parsed = NULL;
	}

	// remember the type of display data
	saveParsedFormat = charFormat;


	// calculate the length we require
	// we use one character per data character plus 3 more
	// characters for every 32 characters
	len = (fileSize * 8) + 4096;
	m_data_parsed = (unsigned char *) rfhMalloc(len, "PARSEDAT");

	// make sure it worked
	if (NULL == m_data_parsed)
	{
		// just return
		return;
	}

	// initialize to a zero length string
	m_data_parsed[0] = 0;

	// check if there is any data to format
	if (0 == fileSize)
	{
		// nothing to do
		// this will result in m_data_parsed pointing to a null string
		return;
	}

	// parse the input data
	getNameValueData(fileData, fileSize, m_data_parsed, len, fileCcsid, charFormat);

	// translate the data to ASCII as necessary
	// if no translation is required a pointer to the original data in fileData is returned
/*	tempData2 = getTranslatedData(&length, charFormat, fileCcsid);

	// check if the data length is greater than zero
	if (0 == length)
	{
		// nothing to process
		// release the acquired data area if necessary
		if (tempData2 != fileData)
		{
			// release the acquired memory so we don't leak memory
			rfhFree(tempData2);
		}

		// return with m_data_xml set as a zero length string
		return;
	}

	//.initialize a temporary memory area that will hold the
	// processed data with any unnecessary xml characters removed
	tempData = (unsigned char *) rfhMalloc(length + 1, "XMLREMOV");
	memset(tempData, 0, length + 1);
	memset(xmlVarName, 0, sizeof(xmlVarName));
	memset(xmlAttrStr, 0, sizeof(xmlAttrStr));

	// figure out how much data we have after we
	// remove comments, invalid characters, etc
	remaining = removeUnneededXML(tempData2, length, tempData);

	// Do a quick sanity check on the data and inset a message if
	// this doesn't look like XML
	// the check includes things like making sure the number of
	// begin and end brackets match.
	rc = checkIfXml(tempData, length, &errmsg);

	if (rc > 0)
	{
		// display a message instead of the data
		strcpy((char *) m_data_parsed, BAD_XML_MSG);
		strcat((char *) m_data_parsed, errmsg);

		// release the acquired data areas if necessary
		rfhFree(tempData);
		if (tempData2 != fileData)
		{
			// release the acquired memory so we don't leak memory
			rfhFree(tempData2);
		}

		return;
	}

	// do some initialization of variables and work areas
	i = 0;

	while ((buffer < remaining) && (i < (len - 2048)))
	{
		// first, check if we have a beginning bracket
		if (tempData[buffer] != '<')
		{
			// We have some junk, so we will just put
			// it on its own line until we find a begin bracket
			// find the next beginning bracket
			whitespace = 0;
			savei = i;

			while ((buffer < remaining) && (tempData[buffer] != '<'))
			{
				// check if we have something other than a space
				if ((tempData[buffer] > ' ') && (tempData[buffer] < 128))
				{
					whitespace = 1;
				}

				if ((tempData[buffer] > ' ') && (tempData[buffer] < 127))
				{
					m_data_parsed[i++] = tempData[buffer++];
				}
				else
				{
					// replace with periods
					m_data_parsed[i++] = '.';
					buffer++;
				}
			}

			if (1 == whitespace)
			{
				// put this junk on its own line
				m_data_parsed[i++] = '\r';
				m_data_parsed[i++] = '\n';

				// at this point, we have built the next line, so
				// insert the proper offset and then append the data
				j = 0;
				while (j < currentLevel)
				{
					m_data_parsed[i++] = ' ';
					j++;
				}
			}
			else
			{
				// write over the junk
				i = savei;
			}
		}

		// check if we found a begin bracket
		if ((buffer < remaining) && ('<' == tempData[buffer]))
		{
			// we found a begin bracket
			// skip it
			buffer++;

			endvar = 0;
			specvar = 0;
			tempVarName[0] = 0;
			j = 0;

			// check if this is the end of a variable
			if ('/' == tempData[buffer])
			{
				// Ending variable, set a variable to remind us
				endvar = 1;
			}

			// check if this is special XML
			if (('?' == tempData[buffer]) || ('!' == tempData[buffer]))
			{
				// Ending variable, set a variable to remind us
				specvar = 1;
			}

			// now, see if we can find an end bracket, isolating the variable name
			endName = 0;
			attrLen = 0;
			while ((buffer < remaining) && (tempData[buffer] != '>'))
			{
				if ((tempData[buffer] <= ' ') && (endName == 0))
				{
					// switch to the attributes if we find a space
					endName = 1;
					buffer++;
				}
				else
				{
					if (0 == endName)
					{
						//capture the variable name
						tempVarName[j++] = tempData[buffer++];
					}
					else
					{
						// ignore any leading spaces in the attributes
						if ((attrLen > 0) || (tempData[buffer] > ' '))
						{
							// capture the attributes
							tempVarAttr[attrLen++] = tempData[buffer++];
						}
						else
						{
							buffer++;
						}
					}
				}
			}

			// terminate the strings
			tempVarName[j] = 0;
			tempVarAttr[attrLen] = 0;

			// get rid of any trailing slash or space characters
			while ((attrLen > 0) && (('/' == tempVarAttr[attrLen]) || (tempVarAttr[attrLen] <= ' ')))
			{
				tempVarAttr[attrLen--] = 0;
			}

			if ('>' == tempData[buffer])
			{
				if ('/' == tempData[buffer-1])
				{
					specvar = 1;

					// remove the / from the variable name
					if ('/' == tempVarName[strlen(tempVarName) - 1])
					{
						tempVarName[strlen(tempVarName) - 1] = 0;
					}
				}

				// First, skip the end bracket
				buffer++;

				// We have the variable name isolated
				// check if we are beginning a new variable
				// or ending a previous one
				if ((1 == endvar) || (1 == specvar))
				{
					if (1 == endvar)
					{
						// ending a previous variable
						// don't allow negative numbers in level
						if (currentLevel > 0)
						{
							// decrement the level of this variable
							currentLevel--;

							// remove the last variable from the name list
							rc = removeVarName(xmlVarName, tempVarName);

							if (rc > 0)
							{
								// append an error message
								sprintf((char *) m_data_parsed + i, "***** RFHUtil format error - end name does not match %s %s\r\n", xmlVarName, tempVarName);
								i += strlen((char *) m_data_parsed + i);
							}
						}
						else
						{
							// Result would have been negative - flag as error
							strcpy((char *) m_data_parsed + i, "***** RFHUtil format error parsing XML - too few levels*****\r\n");
							i += strlen((char *) m_data_parsed + i);
						}
					}

					// check if this is an empty variable
					// otherwise it is ignored
					if ((1 == specvar) && ('/' == tempData[buffer-2]))
					{
						// create a null entry for this value
						if ('\\' == tempVarName[strlen(tempVarName) - 1])
						{
							tempVarName[strlen(tempVarName) - 1] = 0;
						}

						// get any leading names in the hierarchy
						if (strlen(xmlVarName) > 0)
						{
							strcpy((char *) m_data_parsed + i, xmlVarName);
							i += strlen(xmlVarName);
							m_data_parsed[i++] = '.';
						}

						// add the empty variable to the display
						strcpy((char *) m_data_parsed + i, tempVarName);
						i += strlen(tempVarName);
						strcpy((char *) m_data_parsed + i, "=\'\'");
						i += 3;
						// append a carriage return/new line pair to start a new line
						m_data_parsed[i++] = '\r';
						m_data_parsed[i++] = '\n';

						// this is an empty variable but check for attributes
						if (attrLen > 0)
						{
							// start with a zero length output string
							xmlAttrStr[0] = 0;

							// extract the individual attributes into a temporary string
							processAttributes(xmlVarName, tempVarName, tempVarAttr, xmlAttrStr);

							// capture the attributes in the output area
							strcpy((char *) m_data_parsed + i, xmlAttrStr);
							i += strlen(xmlAttrStr);
						}

					}
				}
				else
				{
					// beginning a new variable name
					// first, increase the level
					currentLevel++;

					// build a string with the current
					// higher level variables
					if (xmlVarName[0] != 0)
					{
						strcat(xmlVarName, ".");
					}

					// append the latest variable name
					strcat(xmlVarName, tempVarName);

					// At this point, we want to find the rest of the line,
					// starting with any data for this variable
					// If there is no data, we need to check if we are starting a
					// new level or if we just have a null variable
					if (('<' == tempData[buffer]) &&
						(tempData[buffer + 1] != '/') &&
						(memcmp(tempData + buffer, CDATA_START, 9) != 0))
					{
						// We are starting a new level, so let's stop at this point
						// append to the display data
						strcpy((char *) m_data_parsed + i, xmlVarName);
						i += strlen(xmlVarName);
						m_data_parsed[i++] = '\r';
						m_data_parsed[i++] = '\n';
					}
					else
					{
						strcpy((char *) m_data_parsed + i, xmlVarName);
						i += strlen(xmlVarName);

						// We may have some data, so we will collect it
						// in a temporary variable
						templen = 0;
						whitespace = 0;

						// check for a CDATA section
						if ((buffer < remaining - 10) && (memcmp(tempData + buffer, CDATA_START, 9) == 0))
						{
							// found a CDATA section
							buffer += 9;

							// now, gather the data
							while ((buffer < remaining) &&
								   (memcmp(tempData + buffer, CDATA_END, 3) != 0) &&
								   (templen < (sizeof(tempVarValue) - 1)))
							{
								if ((tempData[buffer] >= ' ') && (tempData[buffer] != 127))
								{
									tempVarValue[templen++] = tempData[buffer++];
								}
								else
								{
									// replace non-display characters with a period
									tempVarValue[templen++] = '.';
									buffer++;
								}
							}

							// account for the terminating characters
							buffer += 3;
						}
						else
						{
							// handle as ordinary data
							while ((buffer < remaining) && (tempData[buffer] != '<'))
							{
								// check if we have non-whitespace characters
								if (tempData[buffer] > ' ')
								{
									whitespace = 1;
								}

								// replace any non-printable characters with blanks
								if ((tempData[buffer] >= ' ') && (tempData[buffer] != 127))
								{
									// normal character
									tempVarValue[templen++] = tempData[buffer++];
								}
								else
								{
									// replace with a period
									tempVarValue[templen++] = '.';
									buffer++;
								}
							}
						}

						// terminate the value string, remove escape
						// sequences and append it
						tempVarValue[templen] = 0;

						// check if we should append the value
						if (('<' == tempData[buffer]) && ('/' == tempData[buffer + 1]))
						{
							m_data_parsed[i++] = '=';
							m_data_parsed[i++] = '\'';
							removeEscSeq(tempVarValue);
							strcpy((char *) m_data_parsed + i, tempVarValue);
							i += strlen(tempVarValue);
							m_data_parsed[i++] = '\'';
						}

						// append a new line to the work area
						m_data_parsed[i++] = '\r';
						m_data_parsed[i++] = '\n';

						// Now, check if we found a beginning bracket
						if ((buffer < remaining) && ('<' == tempData[buffer]) && ('/' == tempData[buffer + 1]))
						{
							// we must have found a beginning bracket,
							// so skip it
							buffer++;
							k = 0;

							// isolate the ending variable name
							while ((buffer < remaining) && (tempData[buffer] != '>'))
							{
								if (tempData[buffer] <= ' ')
								{
									// terminate the variable name
									tempEndVarName[k++] = 0;
									buffer++;
								}
								else
								{
									tempEndVarName[k++] = tempData[buffer++];
								}
							}

							// terminate the string
							tempEndVarName[k] = 0;

							// check if we found the trailing end bracket
							if (buffer < remaining)
							{
								// now, this should be an end variable
								// skip the ending bracket
								buffer++;
								currentLevel--;

								// Check if this is an ending element
								if ('/' == tempEndVarName[0])
								{
									// yes it is, check if it matches
									// remove the last variable from the name list
									rc = removeVarName(xmlVarName, tempEndVarName);

									if (rc > 0)
									{
										strcpy((char *) m_data_parsed + i, "***** RFHUtil format error - end name does not match\r\n");
										i += strlen((char *) m_data_parsed + i);
									}
								}
								else
								{
									strcpy((char *) m_data_parsed + i, "***** RFHUtil format error parsing XML - missing element end\r\n");
									i += strlen((char *) m_data_parsed + i);
								}

							}
							else
							{
								// append an error message
								strcpy((char *) m_data_parsed + i, "***** RFHUtil format error parsing XML - no trailing end bracket\r\n");
								i += strlen((char *) m_data_parsed + i);
							}
						}
					}

					// check if we have found any attributes we should display
					if (attrLen > 0)
					{
						// start with a zero length output string
						xmlAttrStr[0] = 0;

						// get the attributes in the right display format
						processAttributes(xmlVarName, "", tempVarAttr, xmlAttrStr);

						// append the attribute lines to the output
						strcpy((char *) m_data_parsed + i, xmlAttrStr);

						// account for the storage in the output
						i += strlen(xmlAttrStr);
					}
				}
			}
			else
			{
				// Didn't find an end bracket, put the rest of the line on a separate line
				strcpy((char *) m_data_parsed + i, "***** RFHUtil format error - missing end bracket\r\n");
				i += strlen((char *) m_data_parsed + i);
			}
		}

	}

	// terminate the string
	m_data_parsed[i] = 0;*/

	Rtrim((char *)m_data_parsed);

	// release the temporary buffers
/*	rfhFree(tempData);
	if (tempData2 != fileData)
	{
		rfhFree(tempData2);
	}*/

	if (traceEnabled)
	{
		// create the trace line
		//sprintf(traceInfo, "Exiting DataArea::getParsedData() len=%d strlen(m_data_parsed)=%d i=%d buffer=%d remaining=%d", len, strlen((const char *)m_data_parsed), i, buffer, remaining);
		sprintf(traceInfo, "Exiting DataArea::getParsedData() len=%d strlen(m_data_parsed)=%d buffer=%d", len, strlen((const char *)m_data_parsed), buffer);

		// trace exit from getParsedData
		logTraceEntry(traceInfo);
	}

	return;
}

///////////////////////////////////////////////////////////////
//
// Routine to write a message to an MQSeries queue
//
///////////////////////////////////////////////////////////////

void DataArea::putMessage(LPCTSTR QMname, LPCTSTR RemoteQM, LPCTSTR Queue)

{
	// define the MQ objects that we need
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=MQRC_NONE;					// MQ reason code
	MQLONG			cc2=MQCC_OK;					// MQ completion code
	MQLONG			rc2=MQRC_NONE;					// MQ reason code
	MQINT32			type=0;
	MQMDPAGE		*mqmdObj=(MQMDPAGE *)mqmdData;	// pointer to the MQMD object
	RFH				*rfhObj=(RFH *)rfhData;			// pointer to the RFH object
	CProps			*propObj=(CProps *)propData;	// pointer to the user property object
	MQHMSG			hMsg=MQHM_UNUSABLE_HMSG;		// message handle used to set message properties
	MQMD2			mqmd={MQMD2_DEFAULT};			// Message descriptor
	MQPMO			pmo={MQPMO_DEFAULT};			// Put message options
	MQCMHO			opts={MQCMHO_DEFAULT};			// options used to create message handle
	MQDMHO			dOpts={MQDMHO_DEFAULT};			// options used to delete message handle
	unsigned char	*tempbuf;						// Temporary data buffer
	CRfhutilApp *	app;							// pointer to the MFC application object
	int				msgSize=0;						// length of complete message, including RFH
	int				rfh1Len=0;						// length of the RFH1 header
	int				rfh2Len=0;						// length of the RFH2 header
	int				cicsLen=0;						// length of the CICS header
	int				imsLen=0;						// length of the IMS header
	int				dlqLen=0;						// length of the Dead Letter Queue header
	int				curOfs=0;						// current offset within the message buffer
	int				ccsid=0;
	int				encoding=0;
	int				tempOpt;
	int				propCount;						// count of number of user properties
	CString			tempFormat;						// message format for MQMD
	char			errtxt[512];					// work variable to build error message
	char			traceInfo[512];					// work variable to build trace message
	char			trace1[16];						// work area for trace info

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::putMessage() QMname=%s RemoteQM=%s Queue=%s m_set_all=%d m_setUserID=%d m_new_msg_id=%d m_new_correl_id=%d m_mq_props=%d", QMname, RemoteQM, Queue, m_set_all, m_setUserID, m_new_msg_id, m_new_correl_id, m_mq_props);

		// trace entry to putMessage
		logTraceEntry(traceInfo);
	}

	// make sure we have a queue name
	if ((Queue != NULL) && (strlen(Queue) == 0))
	{
		m_error_msg = "*Queue Name required* ";
		return;
	}

	// make sure we are connected to the correct QM
	if (!checkConnection(QMname))
	{
		return;
	}

	// now check if we have the correct queue open
	// or not with the correct options and
	// try to open the queue if necessary
	if (!openQ(Queue, RemoteQM, Q_OPEN_WRITE, FALSE))
	{
		// didn't work
		return;
	}

	// we have the queue open, so start to build the message
	// update the data type.  This is necessary because
	// we may have to fill in the rfh data
	m_char_format = getCcsidType(atoi((LPCTSTR)mqmdObj->m_mqmd_ccsid));

	// remember the format of the user data
	tempFormat = mqmdObj->m_mqmd_format;

	// Check if we need to build an RFH1 header
	if (((RFH *)rfhData)->m_rfh1_include)
	{
		// create the RFH1 header if necessary and get the length
		getHdrCcsid(MQFMT_RF_HEADER, &ccsid, &encoding);
		rfh1Len = rfhObj->buildRFH(ccsid, encoding);
	}

	// Check if we need to build an RFH2 header
	if (((RFH *)rfhData)->m_rfh2_include)
	{
		// create the RFH2 header if necessary and get the length
		getHdrCcsid(MQFMT_RF_HEADER_2, &ccsid, &encoding);
		rfh2Len = rfhObj->buildRFH2(ccsid, encoding);
	}

	// check if there is a CICS or IMS header
	cicsLen = ((CCICS *)cicsData)->getCICSlength();
	imsLen = ((CIms *)imsData)->getIMSlength();

	// check for a DLQ header
	if (((CDlq *)dlqData)->checkForDlq())
	{
		// build a dead letter header and get the length
		// figure out what ccsid and encoding to use
		getHdrCcsid(MQFMT_DEAD_LETTER_HEADER, &ccsid, &encoding);

		// create the DLQ header if necessary
		dlqLen = ((CDlq *)dlqData)->createHeader(ccsid, encoding);
	}

	// allocate a temporary buffer large enough for the data and the RFH headers
	tempbuf = (unsigned char *)rfhMalloc(fileSize + dlqLen + cicsLen + imsLen + rfh1Len + rfh2Len + 1, "HDRSDATA");

	// create any headers
	curOfs = buildHeaders(tempbuf, dlqLen, cicsLen, imsLen, rfh1Len, rfh2Len);

	if ((fileSize > 0) && (fileData != NULL))
	{
		// move the message data into the buffer
		memcpy(tempbuf + curOfs, fileData, fileSize);
	}

	// get the total message length
	msgSize = fileSize + curOfs;

	tempbuf[msgSize] = 0;

	// set the MQMD fields in the message object
	mqmdObj->setMessageMQMD(&mqmd, m_setUserID, m_set_all);

	// set the pmo version to 2
	pmo.Version = MQPMO_VERSION_2;

	// initialize the pmo.Options to include fail if quiescing
	pmo.Options = MQPMO_FAIL_IF_QUIESCING;

	// check for logical order selected
	if (m_logical_order)
	{
		// set logical order option
		pmo.Options |= MQPMO_LOGICAL_ORDER;
	}

	//. check if set all selected
	if (m_set_all)
	{
		// indicate setting all context
		pmo.Options |= MQPMO_SET_ALL_CONTEXT;
	}

	// check if set user id selected
	if (m_setUserID)
	{
		// indicate setting identity context
		pmo.Options |= MQPMO_SET_IDENTITY_CONTEXT;
	}

	// check if new message id option is to be set
	if (m_new_msg_id)
	{
		// set the new message id option
		pmo.Options |= MQPMO_NEW_MSG_ID;
	}

	// check if new correlation id option is to be set
	if (m_new_correl_id)
	{
		// set the new correlation id option
		pmo.Options |= MQPMO_NEW_CORREL_ID;
	}

	// check if n a group
	// This can either be a member of a group and not the last in the group
	// or the last in a group and a segment but not the last segment
	groupActive = mqmdObj->setGroupActive(m_logical_order);

	// check if we are in the middle of a segment
	segmentActive = mqmdObj->getSegmentActive();

	// is this message part of a segment or group?
	if (mqmdObj->m_mqmd_segment_yes || mqmdObj->m_mqmd_segment_last || mqmdObj->m_mqmd_group_yes || mqmdObj->m_mqmd_group_last)
	{
		// need to put this message under syncpoint
		pmo.Options |= MQPMO_SYNCPOINT;
	}
	else
	{
		// no need to use a syncpoint for this message
		pmo.Options |= MQPMO_NO_SYNCPOINT;
	}

	// check if message properties are to be processed
	propCount = propObj->GetPropertyCount();
	if (propertiesSupported  && (propCount > 0) && (MQ_PROPS_YES == m_mq_props))
	{
		// make sure that the message handle processing options are honored
		pmo.Version = MQPMO_VERSION_3;

		// set the message handle options
		opts.Options = MQCMHO_VALIDATE;

		// create a message handle
		XMQCrtMh(qm, &opts, &hMsg, &cc, &rc);

		// check if it was created
		if (cc != MQCC_OK)
		{
			// unable to create the message handle - report the error
			setErrorMsg(cc, rc, "MQCRTMH");

			// return noting error
			return;
		}

		// message handle has been created
		// set the message properties
		cc = setMsgProps(hMsg);

		if (cc != MQCC_OK)
		{
			// return if setting the message properties failed
			updateMsgText();
			m_error_msg = "Error setting message properties - message not sent";

			// return with error
			return;
		}

		// set the handle in the PMO and the action
		pmo.OriginalMsgHandle = hMsg;
		pmo.Action = MQACTP_NEW;
	}

	if (traceEnabled)
	{
		// build the trace line
		memset(trace1, 0, sizeof(trace1));
		tempOpt = pmo.Options;
		tempOpt = reverseBytes4(tempOpt);
		AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace1);
		sprintf(traceInfo, "putting message using options X\'%s\' propCount=%d msgSize=%d", trace1, propCount, msgSize);

		// write a trace log entry
		logTraceEntry(traceInfo);

		// check for verbose trace
		if (verboseTrace)
		{
			// dump the contents of the mqmd and pmo to the trace
			dumpTraceData("MQMD", (unsigned char *)&mqmd, sizeof(mqmd));
			dumpTraceData("PMO", (unsigned char *)&pmo, sizeof(pmo));
		}
	}

	// try to send the message
	XMQPut(qm, q, &mqmd, &pmo, msgSize, tempbuf, &cc, &rc);

	// check if a message handle was created
	if (propertiesSupported && (MQ_PROPS_YES == m_mq_props) && (propCount > 0))
	{
		// delete the message handle
		XMQDltMh(qm, &hMsg, &dOpts, &cc2, &rc2);

		// check if it worked
		if (cc2 != MQCC_OK)
		{
			// unable to delete the message handle - report and trace the error
			setErrorMsg(cc2, rc2, "MQDLTMH");
		}
	}

	// check the results
	if (cc != MQCC_OK)
	{
		// put failed, get the reason and generate an error message
		// get the completion code and reason code
		setErrorMsg(cc, rc, "Put");

		// release the temporary buffer
		rfhFree(tempbuf);

		// was this a warning?
		if (MQCC_FAILED == cc)
		{
			// check for a unit of work
			if (unitOfWorkActive)
			{
				rollbackUOW();
			}

			return;
		}
	}

	// extract certain MQMD fields from the message that was just sent
	mqmdObj->updateMQMDafterPut(&mqmd, msgSize);

	// worked, tell the user the message was sent
	sprintf(errtxt, "Message sent to %s length=%d", Queue, msgSize);
	m_error_msg = errtxt;

	// is there a unit of work open?
/*	if (unitOfWorkActive)
	{
		// check if we want to commit the unit of work
		// this can be either because we are writing the
		// last message in a group (either unsegmented or the last segment)
		// or the last segment in a message or are writing a single message
		// with segmentation allowed
		if (((m_MQMD_group_last) && (m_MQMD_segment_last || !m_MQMD_segment_yes)) ||
			((!m_logical_order) && (m_MQMD_segment_last)) ||
			(m_MQMD_segment_allow && (!m_MQMD_segment_yes) && (!m_logical_order)))
		{
			commitUOW(true);
		}
	}*/

	// check if we need to commit the current operation
	// this can be the end of a segmented operation if there is no group or the
	// end of a group
	// this can be if segmentation is allowed and we are not in the middle
	// of a segment or group or at the end of a segment and/or group.
	if (mqmdObj->m_mqmd_segment_yes)
	{
		// middle of a segment
		// is this also the end of a segment without a group or the end of both?
		if (mqmdObj->m_mqmd_segment_last && (!mqmdObj->m_mqmd_group_yes || mqmdObj->m_mqmd_group_last))
		{
			// the segment is over and either there is no group or the group is also done
			// commit the messages
			commitUOW(true);
		}
	}
	else
	{
		// no segment - check for a group finishing
		if (mqmdObj->m_mqmd_group_last)
		{
			// the group is done - commit the group
			commitUOW(true);
		}
	}

	// get the current depth of the queue
	getCurrentDepth();

	// release the temporary buffer
	rfhFree(tempbuf);

	// call the audit routine to record the put message action
	app = (CRfhutilApp *)AfxGetApp();
	app->createAuditRecord((LPCTSTR)currentQM, (LPCTSTR)currentQ, "MQPut", rc);

	if (traceEnabled)
	{
		// log the results
		logTraceEntry(errtxt);

		// build the trace entry
		sprintf(traceInfo, "Exiting DataArea::putMessage() propCount=%d cc=%d rc=%d", propCount, cc, rc);

		// trace exit from putMessage
		logTraceEntry(traceInfo);
	}

	// update the window title
	setWindowTitle((LPCSTR) Queue);
}

int DataArea::setMsgProps(MQHMSG hMsg)

{
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=MQRC_NONE;					// MQ reason code
	int				elem;							// XML element number
	int				propType;						// element type
	int				count=0;						// number of properties
	int				intCount=0;						// number of integer properties
	int				i64Count=0;						// number of 64-bit integer properties
	int				boolCount=0;					// number of boolean properties
	int				hexCount=0;						// number of byte string properties
	int				flt4Count=0;					// number of 32-bit floating point properties
	int				flt8Count=0;					// number of 64-bit floating point properties
	int				strCount=0;						// number of string properties
	int				nullCount=0;					// number of null properties
	int				len;							// length of hex string
	int				valueLen;						// length of the value
	int				intValue;						// integer value
	float			flt4;							// 32-bit floating point
	double			flt8;							// 64-bit floating point
	int64_t			i8;								// 64-bit integer
	const char		*valuePtr;						// pointer to property value
	char			*allocPtr=NULL;					// pointer to allocated storage
	CProps			*propObj=(CProps *)propData;	// pointer to the user property object
	MQCHARV			nameVar={MQCHARV_DEFAULT};		// variable character data control block for property name
	MQSMPO			smpo={MQSMPO_DEFAULT};			// Set property options
	MQPD			pd={MQPD_DEFAULT};				// property description
	char			value[10240];					// buffer to hold value
	char			traceInfo[512];					// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::setMsgProps()");

		// trace entry to setMsgProps
		logTraceEntry(traceInfo);
	}

	// add properties to the message handle
	// get the first element
	elem = propObj->getFirstProperty();
	while ((elem > 0) && (MQCC_OK == cc))
	{
		// set up the name variable
		nameVar.VSPtr = (void *)propObj->getPropName(elem);
		nameVar.VSLength = MQVS_NULL_TERMINATED;
		nameVar.VSCCSID = MQCCSI_APPL;

		// get a pointer to the value as a string and the data type
		valuePtr = propObj->getPropValue(elem);
		propType = propObj->getPropType(elem);

		// initialize the allocated storage pointer
		allocPtr = NULL;
		valueLen = MQVL_NULL_TERMINATED;

		// figure out what type of data this is
		switch (propType)
		{
			case MQTYPE_INT8:
			case MQTYPE_INT16:
			case MQTYPE_INT32:
				{
					// convert to an integer
					intValue = atoi(valuePtr);

					// set the value pointer
					valuePtr = (char *)&intValue;

					// set the value length
					if (MQTYPE_INT8 == propType)
					{
						valueLen = sizeof(MQINT8);
					}
					else if (MQTYPE_INT16 == propType)
					{
						valueLen = sizeof(MQINT16);
					}
					else if (MQTYPE_INT32 == propType)
					{
						valueLen = sizeof(MQINT32);
					}

					// count the number of integer properties
					intCount++;

					break;
				}
			case MQTYPE_INT64:
				{
					// convert to a 64-bit integer
					i8 = _atoi64(valuePtr);

					// set the value pointer and length
					valuePtr = (char *)&i8;
					valueLen = sizeof(MQINT64);

					// count the number of integer properties
					i64Count++;

					break;
				}
			case MQTYPE_BOOLEAN:
				{
					if ((strcmp(valuePtr, "1") == 1) || (strcmp(valuePtr, "TRUE") == 1) || (strcmp(valuePtr, "true") == 1))
					{
						// set the value to true
						intValue = TRUE;
					}
					else
					{
						// set the value to false
						intValue = FALSE;
					}

					// set the value pointer and length
					valuePtr = (char *)&intValue;
					valueLen = sizeof(MQBOOL);
					valueLen = 4;

					// count the number of boolean properties
					boolCount++;

					break;
				}
			case MQTYPE_BYTE_STRING:
				{
					// get the length of the hex string
					len = strlen(valuePtr);

					// check for X' at the front of the data
					if (('X' == valuePtr[0]) || ('x' == valuePtr[0]))
					{
						// skip the first character
						valuePtr++;
						len--;
					}

					// check for quotes at front of data
					if (('\'' == valuePtr[0]) || ('\"' == valuePtr[0]))
					{
						// skip the quote
						valuePtr++;
						len--;

						// check for a quote at the end of the data
						if (('\'' == valuePtr[len-1]) || ('\"' == valuePtr[len-1]))
						{
							// get rid of the other quote
							len--;
						}
					}

					// check if the data will fit in the value area
					if (len > sizeof(value) - 1)
					{
						// have to allocate storage
						allocPtr = (char *)rfhMalloc((len >> 1) + 1, "VALUE1  ");

						// convert the hex to bytes
						HexToAscii((unsigned char *)valuePtr, len >> 1, (unsigned char *)allocPtr);

						// point to the converted data
						valuePtr = allocPtr;
					}
					else
					{
						// convert to bytes
						HexToAscii((unsigned char *)valuePtr, len >> 1, (unsigned char *)value);

						// point to the converted data
						valuePtr = value;
					}

					// set the length
					valueLen = len >> 1;

					// count the number of byte array properties
					hexCount++;

					break;
				}
			case MQTYPE_FLOAT32:
				{
					// convert to floating point
					flt8 = atof(valuePtr);
					flt4 = (float)flt8;

					// set the value pointer and length
					valuePtr = (char *)&flt4;
					valueLen = sizeof(MQFLOAT32);

					// count the number of 32-bit floating point properties
					flt4Count++;

					break;
				}
			case MQTYPE_FLOAT64:
				{
					// convert to floating point
					flt8 = atof(valuePtr);

					// set the value pointer and length
					valuePtr = (char *)&flt8;
					valueLen = sizeof(MQFLOAT64);

					// count the number of 64-bit floating point properties
					flt8Count++;

					break;
				}
			case MQTYPE_NULL:
			case MQTYPE_STRING:
			default:
				{
					// nothing to do - value is already a string
					// also use string if the value type is not recognized
					// just count the property
					if (MQTYPE_NULL == propType)
					{
						// set the value length
						valueLen = 0;

						// count the null property
						nullCount++;
					}
					else
					{
						// set length as null terminated string
						valueLen = MQVL_NULL_TERMINATED;

						// count the string property
						strCount++;
					}

					break;
				}
		}

		// process the property
		XMQSetMp(qm, hMsg, &smpo, &nameVar, &pd, propType, valueLen, (void *)valuePtr, &cc, &rc);

		if (traceEnabled)
		{
			// build the trace entry
			sprintf(traceInfo, " SetMP cc=%d rc=%d propType=%d valueLen=%d nameVar.VSLength=%d nameVar.VSPtr=\"%.128s\"", cc, rc, propType, valueLen, nameVar.VSLength, (LPCTSTR)nameVar.VSPtr);

			// trace exit from setMsgProps
			logTraceEntry(traceInfo);

			// verbose trace?
			if (verboseTrace && (valuePtr != NULL) && (valueLen > 0))
			{
				// dump out the value
				dumpTraceData("valuePtr", (unsigned char *)valuePtr, valueLen);
			}
		}

		// release any acquired storage
		if (allocPtr != NULL)
		{
			// free the acquired storage
			rfhFree(allocPtr);
			allocPtr = NULL;
		}

		// check the completion code
		if (cc != MQCC_OK)
		{
			// unable to create the message property - report the error
			setErrorMsg(cc, rc, "MQSETMP");

			// return noting error
			return cc;
		}
		else
		{
			// count the number of properties
			count++;

			// move on to the next property
			elem = propObj->getNextProp(elem);
		}
	}

	if (traceEnabled)
	{
		// build the trace entry
		sprintf(traceInfo, "Exiting DataArea::setMsgProps() cc=%d count=%d strCount=%d intCount=%d i64Count=%d boolCount=%d hexCount=%d flt4Count=%d flt8Count=%d nullCount=%d",
				cc, count, strCount, intCount, i64Count, boolCount, hexCount, flt4Count, flt8Count, nullCount);

		// trace exit from setMsgProps
		logTraceEntry(traceInfo);
	}

	return cc;
}

//////////////////////////////////////////////////////////////
//
// Routine to process any message headers that arrived with
// the message data
//
// Any previous headers will be cleared.  The fileData and
// fileSize variables must be set before this routine is
// called.
//
// Any headers that are found and processed will be removed
// from the data area.
//
//////////////////////////////////////////////////////////////

int DataArea::parseMsgHeaders()

{
	int			curOfs=0;
	int			ccsid;
	int			charFormat;
	int			encoding;
	int			len;
	MQMDE		*mqmdePtr;
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;					// pointer to the MQMD object
	RFH			*rfhObj=(RFH *)rfhData;							// pointer to the RFH object
	char		dlqHeader[MQ_FORMAT_LENGTH + 4];				// Dead letter header string without padding
	char		rfh1Header[MQ_FORMAT_LENGTH + 4];				// RFH1 header string without padding
	char		rfh2Header[MQ_FORMAT_LENGTH + 4];				// RFH2 header string without padding
	char		cicsHeader[MQ_FORMAT_LENGTH + 4];				// CICS header string without padding
	char		imsHeader[MQ_FORMAT_LENGTH + 4];				// IMS header string without padding
	char		mqmdeHeader[MQ_FORMAT_LENGTH + 4];				// MQMD extension string without padding
	char		nextFormat[MQ_FORMAT_LENGTH + 4];
	bool		found;
	bool		foundRfh1=false;
	bool		foundRfh2=false;
	char	traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::parseMsgHeaders() mqmdObj->m_mqmd_format=%s", (LPCTSTR)mqmdObj->m_mqmd_format);

		// trace entry to parseMsgHeaders
		logTraceEntry(traceInfo);
	}

	// get the MQ format names
	strcpy(dlqHeader, MQFMT_DEAD_LETTER_HEADER);
	strcpy(rfh1Header, MQFMT_RF_HEADER);
	strcpy(rfh2Header, MQFMT_RF_HEADER_2);
	strcpy(cicsHeader, MQFMT_CICS);
	strcpy(imsHeader, MQFMT_IMS);
	strcpy(mqmdeHeader, MQFMT_MD_EXTENSION);

	// remove the padding characters
	Rtrim(dlqHeader);
	Rtrim(rfh1Header);
	Rtrim(rfh2Header);
	Rtrim(cicsHeader);
	Rtrim(imsHeader);
	Rtrim(mqmdeHeader);

	// get the format of the first header or data from the MQMD
	strcpy(nextFormat, (LPCTSTR)mqmdObj->m_mqmd_format);
	Rtrim(nextFormat);

	// get the ccsid and encoding of the first header or data from the MQMD
	charFormat = m_char_format;
	ccsid = mqmdObj->getCcsid();
	encoding = mqmdObj->getEncoding();

	// clear any previous headers
	((CDlq *)dlqData)->clearDLQheader();
	rfhObj->clearRFH();
	((CCICS *)cicsData)->clearCICSheader();
	((CIms *)imsData)->clearIMSheader();

	// process as many headers as are recognized
	found = true;
	while (found)
	{
		found = false;

		// check for a dead letter queue header at the beginning of the data
		if (strcmp(nextFormat, dlqHeader) == 0)
		{
			// parse the header data
			((CDlq *)dlqData)->parseDLQheader(fileData + curOfs, fileSize - curOfs, ccsid, encoding);

			curOfs += ((CDlq *)dlqData)->getDLQlength();

			// get the format of the next item
			((CDlq *)dlqData)->setNextFormat(nextFormat, &charFormat, &encoding);
			ccsid = ((CDlq *)dlqData)->getCcsid();
			Rtrim(nextFormat);

			// prevent loops
			if (strcmp(nextFormat, dlqHeader) != 0)
				found = true;
		}

		// check for a CICS header at the beginning of the data
		if (strcmp(nextFormat, cicsHeader) == 0)
		{
			// parse the header data
			curOfs += ((CCICS *)cicsData)->parseCICSheader(fileData + curOfs, fileSize - curOfs, ccsid, encoding);

			// get the format of the next item
			((CCICS *)cicsData)->setNextFormat(nextFormat, &charFormat, &encoding);
			ccsid = ((CCICS *)cicsData)->getCcsid();
			Rtrim(nextFormat);

			// prevent loops
			if (strcmp(nextFormat, cicsHeader) != 0)
				found = true;
		}

		// check for a IMS header at the beginning of the data
		if (strcmp(nextFormat, imsHeader) == 0)
		{
			// parse the header data
			curOfs += ((CIms *)imsData)->parseIMSheader(fileData + curOfs, fileSize - curOfs, ccsid, encoding);

			// get the format of the next item
			((CIms *)imsData)->setNextFormat(nextFormat, &charFormat, &encoding);
			ccsid = ((CIms *)imsData)->getCcsid();
			Rtrim(nextFormat);

			// prevent loops
			if (strcmp(nextFormat, imsHeader) != 0)
				found = true;
		}

		// check for an RFH header at the beginning of the data
		if (strcmp(nextFormat, rfh1Header) == 0)
		{
			// parse the header data
			curOfs += rfhObj->parseRFH(fileData + curOfs, fileSize - curOfs, ccsid, encoding);

			// get the format of the next item
			rfhObj->setNextFormatRfh1(nextFormat, &charFormat, &encoding);
			ccsid = ((RFH *)rfhObj)->getRFH1Ccsid();
			Rtrim(nextFormat);

			// prevent loops
			if (strcmp(nextFormat, rfh1Header) != 0)
				found = true;
		}

		// check for an RFH 2 header at the beginning of the data
		if (strcmp(nextFormat, rfh2Header) == 0)
		{
			// parse the header data
			curOfs += rfhObj->parseRFH2(fileData + curOfs, fileSize - curOfs, ccsid, encoding);
			ccsid = ((RFH *)rfhObj)->getRFH2Ccsid();

			// get the format of the next item
			rfhObj->setNextFormatRfh2(nextFormat, &charFormat, &encoding);
			Rtrim(nextFormat);

			// prevent loops
			if (strcmp(nextFormat, rfh2Header) != 0)
				found = true;
		}

		// check for an MQMD extension header
		// This will be processed.  However RFHUtil will never produce a message with
		// an MQMD extension header.  It will just recognize the header and capture
		// any values contained in the header.
		if (strcmp(nextFormat, mqmdeHeader) == 0)
		{
			// parse the header data
			mqmdePtr = (MQMDE *)fileData + curOfs;
			len = mqmdObj->processMQMDE(mqmdePtr, fileSize - curOfs, ccsid, encoding);

			// set the ccsid, character set type and encoding of the next item
			ccsid = mqmdePtr->CodedCharSetId;
			charFormat = getCcsidType(ccsid);
			encoding = mqmdePtr->Encoding;

			// get the format of the next item
			memcpy(nextFormat, mqmdePtr->Format, MQ_FORMAT_LENGTH);
			Rtrim(nextFormat);

			// move past this header
			curOfs += len;

			// prevent loops
			if (strcmp(nextFormat, mqmdeHeader) != 0)
				found = true;
		}
	}

	// set the encoding and character format of the data field, based on the last header encountered
	m_char_format = charFormat;
	m_numeric_format = encoding;
	if (NUMERIC_PC == encoding)
	{
		m_pd_numeric_format = NUMERIC_PC;
		m_float_numeric_format = FLOAT_PC;
	}
	else
	{
		m_pd_numeric_format = NUMERIC_HOST;
		m_float_numeric_format = FLOAT_390;
	}

	if (curOfs > 0)
	{
		// remove the headers from the length and data
		fileSize -= curOfs;
		memcpy(fileData, fileData + curOfs, fileSize);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exit from parseMsgHeaders m_char_format=%d m_numeric_format=%d curOfs=%d", m_char_format, m_numeric_format, curOfs);

		// trace exit from parseMsgHeaders
		logTraceEntry(traceInfo);
	}

	return curOfs;
}

//////////////////////////////////////////////////////////////
//
// Routine to process a message that has  been read from
// a queue
//
//////////////////////////////////////////////////////////////

void DataArea::processMessage(MQMD2 * mqmd)

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	int		rfhlength=0;		// length of RFH
	int		msgSize=0;			// length of complete message, including RFH
	int		curOfs=0;
	int		tempCcsid;
	MQLONG	encoding;
	char	errtxt[512];
	char	traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::processMessage() fileSize=%d", fileSize);

		// trace entry to processMessage
		logTraceEntry(traceInfo);
	}

	// insert a terminator at the end of the data to minimize the risk of addressing exceptions
	fileData[fileSize] = 0;			// add a termination character for safety
	fileData[fileSize+1] = 0;		// allow for Unicode termination character

	// extract MQMD fields
	mqmdObj->processMessageMQMD(mqmd);

	// check what the encoding is for the message
	encoding = mqmd->Encoding;
	if ((encoding & MQENC_INTEGER_REVERSED) > 0)
	{
		// normal PC little endian
		m_numeric_format = NUMERIC_PC;
		fileIntFormat = NUMERIC_PC;
	}
	else
	{
		// select big-endian
		m_numeric_format = NUMERIC_HOST;
		fileIntFormat = NUMERIC_HOST;
	}

	// get the code page of the message
	tempCcsid = mqmd->CodedCharSetId;
	fileCcsid = tempCcsid;

	if ((tempCcsid < 0) || (tempCcsid > 99999))
	{
		// try reversing the field
		tempCcsid = reverseBytes4(tempCcsid);

		if ((tempCcsid > 0) && (tempCcsid < 99999))
		{
			// this is a temporary workaround when connecting to the SIBus
			fileCcsid = tempCcsid;
		}
		else
		{
			// invalid code page value - issue error message and reset it to zero
			sprintf(errtxt, "Invalid code page in MQMD - %d\r\n", fileCcsid);
			m_error_msg += errtxt;
			fileCcsid = 0;
		}
	}

	// get the code page type
	m_char_format = getCcsidType(tempCcsid);

	// process any headers
	curOfs = parseMsgHeaders();

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting processMessage curOfs=%d encoding=%d ccsid=%d persistence=%d msgType=%d, putApplType=%d", curOfs, mqmd->Encoding, mqmd->CodedCharSetId, mqmd->Persistence, mqmd->MsgType, mqmd->PutApplType);

		// trace exit from processMessage
		logTraceEntry(traceInfo);
	}
}

//////////////////////////////////////////////////////
//
// Routine to set an initial buffer size for an MQGET.
//
// A default of 64K is used.  If the QMgr or queue
// has a lower limit then that value is used.
//
//////////////////////////////////////////////////////

int DataArea::setInitialBufferSize()

{
	int		bufSize = DEFAULT_BUFFER_SIZE;	// set default initial buffer size
	char	traceInfo[128];					// work variable to build trace message

	// check if the queue manager maximum is less than the initial value
	if ((maxMsgLenQM > 0) && (maxMsgLenQM < bufSize))
	{
		// use the lower value specified for the QMgr
		bufSize = maxMsgLenQM;
	}

	// check if the queue specified a lower value.
	if ((maxMsgLenQ > 0) && (maxMsgLenQ < bufSize))
	{
		// use the lower value specified for the queue
		bufSize = maxMsgLenQ;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::setInitialBufferSize() bufSize=%d maxMsgLenQM=%d maxMsgLenQ=%d", bufSize, maxMsgLenQM, maxMsgLenQ);

		// trace exit from setInitialBufferSize
		logTraceEntry(traceInfo);
	}

	// return the result
	return bufSize;
}

//////////////////////////////////////////
//
// Routine to extract message properties
// after an MQGET operation and write them
// to a file.
//
//////////////////////////////////////////

int DataArea::extractMsgProps(MQHMSG hMsg, int propDelimLen, const char *propDelim, char * buffer, int bufLen)

{
	MQLONG			cc=MQCC_OK;								// MQ completion code
	MQLONG			rc=MQRC_NONE;							// MQ reason code
	MQINT32			propType;								// property type (e.g. int, char, boolean)
	MQINT32			strType;								// property type set to string to force conversion
	MQLONG			options;								// inquire options
	MQLONG			nameLen=0;								// length of the name
	MQLONG			len=0;									// length of the value
	MQLONG			vBufLen=MQ_MAX_PROPERTY_VALUE_LENGTH+1;	// length of the value buffer (4M)
	MQLONG			nBufLen=MQ_MAX_PROPERTY_NAME_LENGTH+1;	// length of the name buffer
	int				count=0;								// count of number of properties found for trace
	int				numBytes;								// work variable used for trace
	int				bufOffset=0;							// offset within current buffer
	CProps			*propObj=(CProps *)propData;			// pointer to the user property object
	MQIMPO			inqOpts={MQIMPO_DEFAULT};				// options to inquire on message properties
	MQPD			pd={MQPD_DEFAULT};						// property description
	MQCHARV			name={MQPROP_INQUIRE_ALL};				// pointer to wildcard name
	char			traceInfo[512];							// work variable to build trace message
	char			*namePtr=NULL;							// name of the property
	char			*valuePtr=NULL;							// value of the property

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::extractMsgProps() propDelimLen=%d buffer=%8.8X bufLen=%d hMsg=%8.8X", propDelimLen, (unsigned int)buffer, bufLen, (unsigned int)hMsg);

		// trace entry to extractMsgProps
		logTraceEntry(traceInfo);
	}

	// allocate storage for the name and value
	// the allocations are 4096 bytes for the name and value initially.
	// if a name or value too big is returned then a larger buffer will be allocated.
	namePtr = (char *)rfhMalloc(nBufLen, "PROPNAME");
	valuePtr = (char *)rfhMalloc(vBufLen, "PROPVAL ");

	// if value malloc failed try a smaller value
	if (NULL == valuePtr)
	{
		vBufLen = MQ_MAX_PROPERTY_VALUE_LENGTH / 2;
		valuePtr = (char *)rfhMalloc(vBufLen, "PROPVAL ");

		// successful?
		if (NULL == valuePtr)
		{
			// try even smaller value
			vBufLen = MQ_MAX_PROPERTY_VALUE_LENGTH / 4;
			valuePtr = (char *)rfhMalloc(vBufLen, "PROPVAL ");
		}

		// successful?
		if (NULL == valuePtr)
		{
			// try even smaller value
			vBufLen = MQ_MAX_PROPERTY_VALUE_LENGTH / 16;
			valuePtr = (char *)rfhMalloc(vBufLen, "PROPVAL ");
		}

		// successful?
		if (NULL == valuePtr)
		{
			// last try
			vBufLen = MQ_MAX_PROPERTY_VALUE_LENGTH / 64;
			valuePtr = (char *)rfhMalloc(vBufLen, "PROPVAL ");
		}
	}

	// make sure the allocations worked
	if ((NULL == namePtr) || (NULL == valuePtr))
	{
		if (traceEnabled)
		{
			// trace error allocating storage
			logTraceEntry("Unable to allocate storage for name and value");
		}

		// release any allocated storage
		if (namePtr != NULL)
		{
			// release the storage
			rfhFree(namePtr);
		}

		// release any allocated storage
		if (valuePtr != NULL)
		{
			// release the storage
			rfhFree(valuePtr);
		}

		// try to be graceful
		return count;
	}

	// initialize some values
	// convert value will convert the name and string values to the local code page
	options = MQIMPO_INQ_FIRST | MQIMPO_CONVERT_VALUE;
	inqOpts.ReturnedName.VSPtr = namePtr;						// pointer to the area for the name
	inqOpts.ReturnedName.VSBufSize = nBufLen - 1;				// maximum number of characters for the name

	while (MQCC_OK == cc)
	{
		// start by just getting the length and type of the property
		// the length of the data and the length of the name will be returned
		inqOpts.Options = options | MQIMPO_QUERY_LENGTH;
		propType = MQTYPE_AS_SET;
		XMQInqMp(qm, hMsg, &inqOpts, &name, &pd, &propType, vBufLen-1, valuePtr, &len, &cc, &rc);

		// check if it worked
		if ((MQCC_OK == cc) || (MQRC_PROPERTY_NAME_TOO_BIG == rc))
		{
			// check if the name buffer is large enough
			if (nBufLen < inqOpts.ReturnedName.VSLength + 1)
			{
				// the name buffer is too small
				// release it and allocate one that is larger
				nBufLen = inqOpts.ReturnedName.VSLength + 512;
				rfhFree(namePtr);
				namePtr = (char *)rfhMalloc(nBufLen, "PROPNAM2");

				// reset the returned name pointer and buffer size
				inqOpts.ReturnedName.VSPtr = namePtr;
				inqOpts.ReturnedName.VSBufSize = nBufLen - 1;
			}

			// check if the value buffer is long enough
			if (vBufLen < (2 * len))
			{
				// the value buffer may be too small
				// release it and allocate one that is larger
				vBufLen = len + 512;
				rfhFree(valuePtr);
				valuePtr = (char *)rfhMalloc(vBufLen, "PROPVAL2");
			}

			// check for a NULL property
			if (MQTYPE_NULL == propType)
			{
				// no need to do anything else
				// just set the value to a null string
				valuePtr[0] = 0;

				// change the options to do the inq
				inqOpts.Options = MQIMPO_CONVERT_VALUE | MQIMPO_INQ_PROP_UNDER_CURSOR;

				// convert the value to a string
				strType = MQTYPE_AS_SET;
			}
			else
			{
				// change the options to do the inq
				inqOpts.Options = MQIMPO_CONVERT_VALUE | MQIMPO_CONVERT_TYPE | MQIMPO_INQ_PROP_UNDER_CURSOR;

				// convert the value to a string
				strType = MQTYPE_STRING;
			}

			// now issue the real MQINQMP
			XMQInqMp(qm, hMsg, &inqOpts, &name, &pd, &strType, vBufLen-1, valuePtr, &len, &cc, &rc);

			// check if trace is enabled
			if (traceEnabled)
			{
				// create the trace line
				nameLen = inqOpts.ReturnedName.VSLength;
				sprintf(traceInfo, " cc=%d rc=%d inqOpts.Options=%d ReturnedName.VSLength=%d propType=%d strType=%d len=%d hMsg=%8.8X", cc, rc, inqOpts.Options, nameLen, propType, strType, len, (unsigned int)hMsg);

				// trace results of MQInqMp call
				logTraceEntry(traceInfo);

				// check for verbose trace
				if (verboseTrace && (len > 0))
				{
					// figure out how long the returned value is
					numBytes = len;

					// limit to the size of the buffer since this value could be longer than the allocated storage
					if (numBytes >= vBufLen)
					{
						// limit to allocated storage
						numBytes = vBufLen;
					}

					// get the returned length
					dumpTraceData("valuePtr", (unsigned char *)valuePtr, numBytes);
				}
			}
		}

		// check if it worked
		if (cc != MQCC_OK)
		{
			switch (rc)
			{
			case MQRC_PROPERTY_NOT_AVAILABLE:
				{
					// do nothing since the last property has been read
					// the loop will now end since a non zero completion code was returned
					break;
				}
			default:
				{
					// report the error - not one that is handled
					setErrorMsg(cc, rc, "MQINQMP");
					updateMsgText();
					break;
				}
			}
		}
		else
		{
			// terminate the name and value strings
			namePtr[inqOpts.ReturnedName.VSLength] = 0;
			valuePtr[len] = 0;

			// check if a file was passed in
			if (NULL == buffer)
			{
				// process the property
				propObj->appendProperty(namePtr, valuePtr, len, propType);
			}
			else
			{
				// now append the property as a name=value pair with a LF character at the end
				bufOffset = appendPropsToBuffer(namePtr, valuePtr, len, propType, buffer, bufLen, bufOffset);

				if (-1 == bufOffset)
				{
					// ran out of room in the buffer
					m_error_msg = "***** message properties too large for memory";

					// stop the loop
					cc = -1;
				}
				else
				{
					// follow with a NL character
					buffer[bufOffset++] = '\n';
				}
			}

			// count the number of properties found
			count++;

			// move on to the next property
			options = MQIMPO_INQ_NEXT;
		}
	}

	// release the allocated storage
	rfhFree(namePtr);
	rfhFree(valuePtr);

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::extractMsgProps() properties count=%d rc=%d nBufLen=%d vBufLen=%d bufOffset=%d cc=%d", count, rc, nBufLen, vBufLen, bufOffset, cc);

		// trace exit from extractMsgProps
		logTraceEntry(traceInfo);
	}

	return bufOffset;
}

//////////////////////////////////////////////////////////////////////
//
// Routine to format message properties to store in a file and insert
// the results in a data buffer.  An offset within the buffer is
// returned.  If no properties are found than the offset that is
// returned will be zero.  If the property will not fit in the buffer
// then a -1 is returned in place of the offset.
//
//////////////////////////////////////////////////////////////////////

int DataArea::appendPropsToBuffer(const char *namePtr, const char *valuePtr, int len, int propType, char * buffer, int bufLen, int bufOfs)

{
	int		totLen=0;
	int		nameLen=0;
	int		dataLen=0;
	char	data[32];
	char	traceInfo[1024];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps::appendPropsToBuffer() propType=%d len=%d namePtr=%.256s valuePtr=%.256s bufOfs=%d bufLen=%d buffer=%8.8X", propType, len, namePtr, valuePtr, bufOfs, bufLen, (unsigned int)buffer);

		// trace entry to appendPropsToFile
		logTraceEntry(traceInfo);

		// check for verbose trace
		if ((verboseTrace) && (len > 256))
		{
			// dump out the value
			dumpTraceData("valuePtr", (unsigned char *)valuePtr, len);
		}
	}

	// check the property type
	switch(propType)
	{
	case MQTYPE_STRING:
		{
			strcpy(data, "=");
			break;
		}
	case MQTYPE_INT8:
		{
			strcpy(data, "(dt=\'i1\')=");
			break;
		}
	case MQTYPE_INT16:
		{
			strcpy(data, "(dt=\'i2\')=");
			break;
		}
	case MQTYPE_INT32:
		{
			strcpy(data, "(dt=\'i4\')=");
			break;
		}
	case MQTYPE_INT64:
		{
			strcpy(data, "(dt=\'i8\')=");
			break;
		}
	case MQTYPE_BOOLEAN:
		{
			strcpy(data, "(dt=\'boolean\')=");
			break;
		}
	case MQTYPE_BYTE_STRING:
		{
			// append the data as hex characters
			strcpy(data, "(dt=\'bin.hex\')=");
			break;
		}
	case MQTYPE_FLOAT32:
		{
			strcpy(data, "(dt=\'r4\')=");
			break;
		}
	case MQTYPE_FLOAT64:
		{
			strcpy(data, "(dt=\'r8\')=");
			break;
		}
	case MQTYPE_NULL:
		{
			strcpy(data, "(xsi:nil=\'true\')=");
			len = 0;
			break;
		}
	default:
		{
			// don't know what this is
			// just append it as an integer
			sprintf(data, "(dt=%d)=", propType);
			break;
		}
	}

	// get the total length of name and data
	nameLen = strlen(namePtr);
	dataLen = strlen(data);
	totLen = nameLen + dataLen;

	// check if it will fit in the current buffer
	if (totLen + bufOfs + len < bufLen)
	{
		// append the property name
		memcpy(buffer + bufOfs, namePtr, nameLen);
		bufOfs += nameLen;

		// append the data type and the equal sign
		memcpy(buffer + bufOfs, data, dataLen);
		bufOfs += dataLen;

		// check for property value
		if (len > 0)
		{
			// append the value
			memcpy(buffer + bufOfs, valuePtr, len);
			bufOfs += len;
		}
	}
	else
	{
		// signal an error by setting bufOfs to -1
		bufOfs = -1;
	}

	// return the new buffer offset
	return bufOfs;
}

//////////////////////////////////////////////////////////////
//
// Routine to issue an MQGET and return the results.
//
// This routine will handle any truncation issues as well
// as any cases where the buffer specified is too long.
//
// The logic is to try to read the message into a common 64KB buffer
// that is allocated when RFHUtil starts and released when RFHUtil
// is terminated.  If this buffer is too short then a new buffer
// is allocated and the MQGET is reissued.  If the original MQGET
// is successful then the message data is copied into a message
// buffer of the appropriate length.
//
//////////////////////////////////////////////////////////////

int DataArea::processMQGet(const char * getType, MQHOBJ handle, int options, int match, int waitTime, MQMD2 *mqmd, MQLONG * rc)

{
	MQLONG			tempOpt;
	MQLONG			cc=MQCC_OK;					// MQ completion code
	MQLONG			cc2=MQCC_OK;				// MQ completion code
	MQLONG			rc2=MQRC_NONE;				// MQ reason code
	MQLONG			msgLen=0;					// size of the message data
	int				bufSize;					// size of the message buffer
	unsigned char *	msgData=NULL;				// pointer to message data
	CRfhutilApp *	app;						// pointer to the MFC application object
	CProps			*propObj=(CProps *)propData;	// pointer to the user property object
	MQHMSG			hMsg=MQHM_UNUSABLE_HMSG;	// message handle used to get message properties
	MQCMHO			opts={MQCMHO_DEFAULT};		// options used to create message handle
	MQDMHO			dOpts={MQDMHO_DEFAULT};		// options used to delete message handle
	MQGMO			gmo={MQGMO_DEFAULT};		// Get message options
	char			traceInfo[512];				// work variable to build trace message
	char			trace1[16];					// work area for trace info
	char			trace2[16];					// work area for trace info

	// figure out the buffer size to use for the first MQGET
	// this routine will also allocate the memory and set the fileData to point to the data
	bufSize = setInitialBufferSize();

	if (traceEnabled)
	{
		// build the trace line
		memset(trace1, 0, sizeof(trace1));
		tempOpt = reverseBytes4(options);
		AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace1);
		memset(trace2, 0, sizeof(trace1));
		tempOpt = reverseBytes4(match);
		AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace2);
		sprintf(traceInfo, "DataArea::processMQGet getType=%s options=X\'%s\' matchoptions=X\'%s\' level=%d bufSize=%d", getType, trace1, trace2, level, bufSize);

		// write a trace log entry
		logTraceEntry(traceInfo);
	}

	// set the gmo options and match options
	gmo.Options = options;
	gmo.MatchOptions = match;

	// check if a wait time was specified
	if (waitTime > 0)
	{
		// set the wait time in the GMO
		gmo.WaitInterval = waitTime;

		// turn on the wait option
		gmo.Options = options | MQGMO_WAIT;
	}

	// make sure that the match options are honored
	gmo.Version = MQGMO_VERSION_2;

	// check if message properties are to be processed
	if (propertiesSupported && (MQ_PROPS_YES == m_mq_props) && ((0 == level) || (level >= MQCMDL_LEVEL_700)))
	{
		// make sure that the message handle processing options are honored
		gmo.Version = MQGMO_VERSION_4;

		if (MQ_PROPS_YES == m_mq_props)
		{
			// set the message handle options
			opts.Options = MQCMHO_VALIDATE;

			// set the message options
			gmo.Options |= MQGMO_PROPERTIES_IN_HANDLE;

			// create a message handle
			XMQCrtMh(qm, &opts, &hMsg, &cc2, rc);

			if (traceEnabled)
			{
				// enter the result in the trace
				sprintf(traceInfo, " MQCrtMh cc2=%d rc=%d hMsg=%8.8X", cc2, (*rc), (unsigned int)hMsg);

				// write a trace entry
				logTraceEntry(traceInfo);
			}

			// check if it worked
			if (cc2 != MQCC_OK)
			{
				// check the return code for a 2009 error (connection to QMgr lost)
				if (MQRC_CONNECTION_BROKEN == (*rc))
				{
					// the connection is gone - make sure everything gets cleaned up
					discQM();
				}

				// unable to create the message handle - report the error
				setErrorMsg(cc2, (*rc), "MQCRTMH");

				// return completion code
				return cc2;
			}

			// set the handle in the GMO
			gmo.MsgHandle = hMsg;
		}
		else if (MQ_PROPS_NONE == m_mq_props)
		{
			// set properties to none - they will be ignored
			gmo.Options |= MQGMO_NO_PROPERTIES;
		}
		else if (MQ_PROPS_RFH2 == m_mq_props)
		{
			// set properties to force into RFH2 header
			gmo.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;
		}
		else if (MQ_PROPS_COMPAT == m_mq_props)
		{
			// set properties to force into RFH2 header
			gmo.Options |= MQGMO_PROPERTIES_COMPATIBILITY;
		}
	}

	// is trace enabled?
	if (traceEnabled)
	{
		// build the trace line
		memset(trace1, 0, sizeof(trace1));
		tempOpt = reverseBytes4(gmo.Options);
		AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace1);
		memset(trace2, 0, sizeof(trace1));
		tempOpt = reverseBytes4(gmo.MatchOptions);
		AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace2);
		sprintf(traceInfo, " gmo.Options=X\'%s\' gmo.MatchOptions=X\'%s\'", trace1, trace2);

		// write a trace log entry
		logTraceEntry(traceInfo);
	}

	// Try to get a message into the default buffer
	XMQGet(qm, handle, mqmd, &gmo, bufSize, defaultMQbuffer, &msgLen, &cc, rc);

	// check for a 2046 error - unsupported option
	// this may be due to the properties in handle option
	if ((MQRC_OPTIONS_ERROR == (*rc)) && ((gmo.Options & MQGMO_PROPERTIES_IN_HANDLE) > 0))
	{
		// turn off properties in handle
		gmo.Options ^= MQGMO_PROPERTIES_IN_HANDLE;

		// retry the get
		XMQGet(qm, handle, mqmd, &gmo, bufSize, defaultMQbuffer, &msgLen, &cc, rc);
	}

	// check for a truncated response (2080)
	if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_FAILED == (*rc)))
	{
		// truncated data - buffer is not large enough
		// allocate a larger buffer to use for the message
		msgData = (unsigned char *)rfhMalloc(msgLen + 16, "MSGDATA2");

		// reissue the call using the larger buffer
		XMQGet(qm, handle, mqmd, &gmo, msgLen, msgData, &msgLen, &cc, rc);
	}
	else
	{
		// did the get work?
		if (MQCC_OK == cc)
		{
			// check the return code for a 2009 error (connection to QMgr lost)
			if (MQRC_CONNECTION_BROKEN == (*rc))
			{
				// the connection is gone - make sure everything gets cleaned up
				discQM();
			}

			// allocate the buffer to use for the message
			msgData = (unsigned char *)rfhMalloc(msgLen + 16, "MSGDATA3");

			// copy the data into the new message buffer
			memcpy(msgData, defaultMQbuffer, msgLen);
		}
	}

	// call the audit routine to record the MQGET
	app = (CRfhutilApp *)AfxGetApp();
	app->createAuditRecord((LPCTSTR)currentQM, (LPCTSTR)currentQ, getType, (*rc));

	// check for a read failure because the buffer is too large
	// this can happen in certain cases where the channel limit is smaller than
	// the queue or queue manager limit or if inquiry on the queue or queue manager
	// is not allowed due to security or other reasons.
/*	while ((MQCC_FAILED == cc) && (0000 == (*rc)) && (bufSize > 0))
	{
		// keep dividing the buffer size by 2 until we no longer see the same failure
		bufSize >>= 1;

		// reissue the call using a smaller value for the buffer
		XMQGet(qm, q, mqmd, &gmo, bufSize, msgData, (long *)&fileSize, &cc, rc);
	}*/

	if (MQCC_OK == cc)
	{
		// clear any previous data areas?
		resetDataArea();

		// clear any previous RFH headers
		((RFH *)rfhData)->clearRFH();

		// update the fileData and fileSize information
		fileData = msgData;
		fileSize = msgLen;

		// clear any previous user properties
		propObj->clearUserProperties();

		// check if message properties are to be processed
		if (propertiesSupported && (MQ_PROPS_YES == m_mq_props) && (level >= MQCMDL_LEVEL_700))
		{
			// extract the message properties
			extractMsgProps(hMsg, 0, NULL, NULL, 0);

			// delete the message handle
			XMQDltMh(qm, &hMsg, &dOpts, &cc2, &rc2);

			// check if it worked
			if (cc2 != MQCC_OK)
			{
				// unable to delete the message handle - report the error
				setErrorMsg(cc2, rc2, "MQDLTMH");
			}
		}
	}
	else
	{
		// check if storage was ever allocated
		if (msgData != NULL)
		{
			// release the storage that was acquired
			rfhFree(msgData);
		}

		// report the error
		// get the completion code and reason code
		setErrorMsg(cc, (*rc), getType);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::processMQGet() cc=%d rc=%d msglen=%d", cc, (*rc), msgLen);

		// trace exit from processMQGet
		logTraceEntry(traceInfo);
	}

	return cc;
}

//////////////////////////////////////////////////////////////
//
// Routine to read a single message from an MQSeries Queue and
//  check for the presence of any MQ message headers
//
//////////////////////////////////////////////////////////////

void DataArea::getMessage(LPCTSTR QMname, LPCTSTR Queue, int reqType)

{
	// define the MQ objects that we need
	MQMDPAGE		*mqmdObj=(MQMDPAGE *)mqmdData;
	int				cc;						// MQ completion code
	MQLONG			rc;						// MQ reason code
	int				options=0;				// GMO options
	int				matchOptions=0;			// GMO match options
	int				uow=0;
	int				request;
	int				rfh1Len;
	int				rfh2Len;
	int				dlqLen;
	int				cicsLen;
	int				imsLen;
	MQMD2			mqmd={MQMD2_DEFAULT};
	CString			remoteQM;
	char			tempTxt[16];			// work area for display message
	char			rfhmsg[128];
	char			errtxt[512];			// work variable to build error message
	char			traceInfo[512];			// work variable to build trace message

	m_error_msg.Empty();
	remoteQM.Empty();

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getMessage() QM=%s Q=%s reqType=%d m_convert=%d", QMname, Queue, reqType, m_convert);

		// trace entry to getMessage
		logTraceEntry(traceInfo);
	}

	// make sure we have a queue name
	if (strlen(Queue) == 0)
	{
		m_error_msg = "*Queue Name required*";

		return;
	}

	// make sure we are connected to the correct QM
	if (!checkConnection(QMname))
	{
		return;
	}

	if (BROWSEQ == reqType)
	{
		request = Q_OPEN_BROWSE;
		options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST;
	}
	else
	{
		request = Q_OPEN_READ;
		options = MQGMO_FAIL_IF_QUIESCING | MQGMO_NO_SYNCPOINT;
	}

	// set the mq options
	options = setMQoptions(options);

	// check if conversion of message was requested
	if (m_convert)
	{
		// is the code page specified on the MQMD tab?
		if (mqmdObj->m_mqmd_ccsid.GetLength() > 0)
		{
			// use the selected ccsid
			mqmd.CodedCharSetId = atoi((LPCTSTR)mqmdObj->m_mqmd_ccsid);
		}

		// set the encoding in case we are dealing with UCS-2
		if (NUMERIC_PC == mqmdObj->m_mqmd_encoding)
		{
			// use PC encoding
			mqmd.Encoding = DEF_PC_ENCODING;
		}
		else
		{
			// use host encoding
			mqmd.Encoding = DEF_UNIX_ENCODING;
		}

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, " Data conversion specified mqmd.CodedCharSetId=%d mqmd.Encoding=%d", mqmd.CodedCharSetId, mqmd.Encoding);

			// trace data conversion request
			logTraceEntry(traceInfo);
		}
	}

	// check for a get by message id request
	if (m_get_by_msgid && (memcmp(mqmdObj->m_message_id, MQMI_NONE, MQ_MSG_ID_LENGTH) != 0))
	{
		mqmdObj->setMsgId(&mqmd);
		matchOptions |= MQMO_MATCH_MSG_ID;
	}

	if (m_get_by_correlid && (memcmp(mqmdObj->m_correlid, MQCI_NONE, MQ_CORREL_ID_LENGTH) != 0))
	{
		mqmdObj->setCorrelId(&mqmd);
		matchOptions |= MQMO_MATCH_CORREL_ID;
	}

	if (m_get_by_groupid && (memcmp(mqmdObj->m_group_id, MQGI_NONE, MQ_GROUP_ID_LENGTH) != 0))
	{
		mqmdObj->setGroupId(&mqmd);
		matchOptions |= MQMO_MATCH_GROUP_ID;
	}

	// check if we need to set the user id
	if (m_setUserID)
	{
		mqmdObj->setUserId(&mqmd);
	}

	// try to open the queue
	if (!openQ(Queue, remoteQM, request, FALSE))
	{
		// didn't work
		return;
	}

	// try to get a message
	cc = processMQGet("Get", q, options, matchOptions, 0, &mqmd, &rc);

	// check if it worked
	if (cc != MQCC_OK)
	{
		// get failed, get the reason and generate an error message
		return;
	}

	// process the data from the message
	processMessage(&mqmd);

	// get the current depth of the queue
	getCurrentDepth();

	// update the source of the data
	fileSource.Format(_T("Message Data (%d) from %s"), fileSize, Queue);

	// tell the user what we did
	if (BROWSEQ == reqType)
	{
		// a browse operation was performed - the message is still on the queue
		strcpy(tempTxt, "browse");
	}
	else
	{
		// a get operation was performed - the message was removed from the queue
		strcpy(tempTxt, "read");
	}

	// set the message to a null string
	rfhmsg[0] = 0;

	// get the lengths of the various headers
	rfh1Len = ((RFH *)rfhData)->getRFH1length();
	rfh2Len = ((RFH *)rfhData)->getRFH2length();
	cicsLen = ((CCICS *)cicsData)->getCICSlength();
	imsLen = ((CIms *)imsData)->getIMSlength();
	dlqLen = ((CDlq *)dlqData)->getDLQlength();

	// were there any headers that were processed?
	if (rfh2Len + rfh1Len + cicsLen + imsLen + dlqLen > 0)
	{
		// create some message text to tell the users the lengths of the various headers that were processed
		sprintf(rfhmsg, " (data %d dlq %d rfh %d cics %d ims %d)", fileSize, dlqLen, rfh2Len + rfh1Len, cicsLen, imsLen);
	}

	// create a message for the user to tell what happened
	sprintf(errtxt, "Msg %s from %s length=%d%s", tempTxt, Queue, fileSize, rfhmsg);
	m_error_msg += errtxt;

	// check for a browse
	if (BROWSEQ == reqType)
	{
		// close the queue for browse operations only
		closeQ(Q_CLOSE_NONE);
	}

	if (traceEnabled)
	{
		// log the results
		logTraceEntry(errtxt);

		// trace exit from getMessage
		logTraceEntry("Exiting DataArea::getMessage()");
	}

	// update the window title to the name of the last queue that was read from
	// this provides a visual indication to the user, which is especially valuable
	// when the user has more than one RFHUtil session running at the same time
	setWindowTitle((LPCSTR) Queue);
}

//////////////////////////////////////////////////////////////
//
// Routine to read the first message from an MQSeries Queue
//  and process the message.
//
// This routine starts a browse operation.  The browse may or
// may not start with the first message in the queue depending
// on the match options that are specified.
//
//////////////////////////////////////////////////////////////

int DataArea::readFirstMessage(bool silent, LPCTSTR qName, int line)

{
	MQMDPAGE				*mqmdObj=(MQMDPAGE *)mqmdData;
	MQLONG					cc=MQCC_OK;						// MQ completion code
	MQLONG					rc;								// MQ reason code
	int						options;						// MQ GMO options
	int						matchOptions=0;					// MQ GMO match options
	int						rfh1Len;
	int						rfh2Len;
	int						dlqLen;
	int						cicsLen;
	int						imsLen;
	MQMD2					mqmd={MQMD2_DEFAULT};			// The message descriptor
	char					rfhmsg[128];
	char					errtxt[512];					// work variable to build message
	char					traceInfo[512];					// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::readFirstMessage() qName=%s line=%d", qName, line);

		// trace entry to readFirstMessage
		logTraceEntry(traceInfo);
	}

	// read the first message in the queue, unless a msg id or correl id match is requested
	// check for a get by message id request
	if (m_get_by_msgid && (memcmp(mqmdObj->m_message_id, MQMI_NONE, MQ_MSG_ID_LENGTH) != 0))
	{
		mqmdObj->setMsgId(&mqmd);
		matchOptions |= MQMO_MATCH_MSG_ID;
	}

	if (m_get_by_correlid && (memcmp(mqmdObj->m_correlid, MQCI_NONE, MQ_CORREL_ID_LENGTH) != 0))
	{
		mqmdObj->setCorrelId(&mqmd);
		matchOptions |= MQMO_MATCH_CORREL_ID;
	}

	if (m_get_by_groupid && (memcmp(mqmdObj->m_group_id, MQGI_NONE, MQ_GROUP_ID_LENGTH) != 0))
	{
		mqmdObj->setGroupId(&mqmd);
		matchOptions |= MQMO_MATCH_GROUP_ID;
	}

	// check if we need to set the user id
	if (m_setUserID)
	{
		mqmdObj->setUserId(&mqmd);
	}

	options =  MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST;

	// set the mq options
	options = setMQoptions(options);

	// check if conversion of message was requested
	if (m_convert)
	{
		// is the code page specified on the MQMD tab?
		if (mqmdObj->m_mqmd_ccsid.GetLength() > 0)
		{
			// use the selected ccsid
			mqmd.CodedCharSetId = atoi((LPCTSTR)mqmdObj->m_mqmd_ccsid);
		}

		// set the encoding in case we are dealing with UCS-2
		if (NUMERIC_PC == mqmdObj->m_mqmd_encoding)
		{
			// use the specified encoding
			mqmd.Encoding = DEF_PC_ENCODING;
		}
	}

	// try to get a message
	cc = processMQGet("StartBr", q, options, matchOptions, 0, &mqmd, &rc);

	// check if it worked
	if (cc != MQCC_OK)
	{
		// check for an empty queue
		if ((2 == cc) && (MQRC_NO_MSG_AVAILABLE == rc))
		{
			m_error_msg = "No messages in ";
			m_error_msg += qName;
			m_error_msg += " queue";
			m_q_depth = 0;
		}
		else
		{
			// get the completion code and reason code
			setErrorMsg(cc, rc, "Get (Browse)");
		}

		// close the queue and disconnect from the queue manager
		discQM();

		return cc;
	}

	// initialize the browse environment
	browseActive = 1;
	browsePrevActive = 0;
	if (line < 0)
	{
		browseCount = 1;
	}
	else
	{
		browseCount = line;
	}

	// process the data from the message
	processMessage(&mqmd);

	// get the current depth of the queue
	getCurrentDepth();

	rfhmsg[0] = 0;
	rfh1Len = ((RFH *)rfhData)->getRFH1length();
	rfh2Len = ((RFH *)rfhData)->getRFH2length();
	cicsLen = ((CCICS *)cicsData)->getCICSlength();
	imsLen = ((CIms *)imsData)->getIMSlength();
	dlqLen = ((CDlq *)dlqData)->getDLQlength();

	if (rfh2Len + rfh1Len + cicsLen + imsLen + dlqLen > 0)
	{
		sprintf(rfhmsg, " (data %d dlq %d rfh %d cics %d ims %d)", fileSize, dlqLen, rfh2Len + rfh1Len, cicsLen, imsLen);
	}

	// worked, tell the user the message was sent
	if (line < 1)
	{
		sprintf(errtxt, "First message read from %s length=%d%s", qName, fileSize, rfhmsg);
	}
	else
	{
		sprintf(errtxt, "Browse started from msg %d on %s length=%d %s", line, qName, fileSize, rfhmsg);
	}

	if (!silent)
	{
		m_error_msg += errtxt;
	}

	if (traceEnabled)
	{
		// log the results
		logTraceEntry(errtxt);

		// trace exit from readFirstMessage
		logTraceEntry("Exiting DataArea::readFirstMessage()");
	}

	return MQCC_OK;
}

//////////////////////////////////////////////////////////////
//
// Routine to read a single message from an MQSeries Queue and
//  check for the presence of an RFH header
//
//////////////////////////////////////////////////////////////

int DataArea::startBrowse(LPCTSTR QMname, LPCTSTR Queue, bool silent, int line)

{
	MQLONG	cc=MQCC_OK;			// MQ completion code
	char	traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::startBrowse() QM=%s Q=%s line=%d", QMname, Queue, line);

		// trace entry to startBrowse
		logTraceEntry(traceInfo);
	}

	m_error_msg.Empty();

	// set the queue depth to zero
	m_q_depth = 0;

	// make sure we have a queue name
	if (strlen(Queue) == 0)
	{
		m_error_msg = "*Queue Name required* ";

		return -1;
	}

	// connect to the queue manager
	if (!connect2QM(QMname))
	{
		return MQCC_FAILED;
	}

	// open the queue for a browse operation
	if (!openQ(Queue, _T(""), Q_OPEN_BROWSE, FALSE))
	{
		discQM();
		return MQCC_FAILED;
	}

	// try to browse the first message in the queue
	cc = readFirstMessage(silent, Queue, line);

	// update the source of the data
	if (line > 0)
	{
		fileSource.Format(_T("Message Data (%d) msg %d of %d from %s"), fileSize, line, m_q_depth, Queue);
	}
	else
	{
		fileSource.Format(_T("Message Data (%d) msg 1 of %d from %s"), fileSize, m_q_depth, Queue);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::startBrowse() cc=%d", cc);

		// trace exit from startBrowse
		logTraceEntry(traceInfo);
	}

	if (cc != MQCC_OK)
	{
		discQM();
		return cc;
	}

	// update the window title
	setWindowTitle((LPCSTR) Queue);

	return cc;
}

void DataArea::endBrowse(bool silent)

{
	if (traceEnabled)
	{
		// trace entry to endBrowse
		logTraceEntry("Entering DataArea::endBrowse()");
	}

	// make sure the q object has been created
	if (q != NULL)
	{
		// get the current depth of the queue
		getCurrentDepth();

		// close the queue
		closeQ(Q_CLOSE_NONE);

		// disconnect from the queue manager
		discQM();

		if (!silent)
		{
			// worked, tell the user the message was sent
			m_error_msg += "Browse operation ended";
		}
	}
}

int DataArea::browseNext(bool silent)

{
	MQMDPAGE				*mqmdObj=(MQMDPAGE *)mqmdData;
	int						cc;
	MQLONG					rc;
	int						options=0;		// MQ GMO options
	int						rfh1Len;
	int						rfh2Len;
	int						dlqLen;
	int						cicsLen;
	int						imsLen;
	MQMD2					mqmd={MQMD2_DEFAULT};
	char					rfhmsg[128];
	char					errtxt[512];	// work variable to build message
	char					traceInfo[512];	// work variable to build trace message

	// initialize the message area
	errtxt[0] = 0;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::browseNext() browseCount=%d saveCount=%d m_MQMD_seq_no=%s m_MQMD_offset=%s", browseCount, m_save_browse_count, (LPCTSTR)mqmdObj->m_mqmd_seq_no, (LPCTSTR)mqmdObj->m_mqmd_offset);

		// trace entry to browseNext
		logTraceEntry(traceInfo);
	}

	// make sure the q object has been created
	if (NULL == q)
	{
		// return immediately
		return -1;
	}

	// remember the current message
	memcpy(m_save_message_id, mqmdObj->m_message_id, sizeof(m_save_message_id));

	// set the message options
	options |= MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_NEXT;

	// set the mq options
	options = setMQoptions(options);

	// check if conversion of message was requested
	if (m_convert)
	{
		// is the code page specified on the MQMD tab?
		if (mqmdObj->m_mqmd_ccsid.GetLength() > 0)
		{
			// use the selected ccsid
			mqmd.CodedCharSetId = atoi((LPCTSTR)mqmdObj->m_mqmd_ccsid);
		}

		// set the encoding in case we are dealing with UCS-2
		if (NUMERIC_PC == mqmdObj->m_mqmd_encoding)
		{
			// use the specified encoding
			mqmd.Encoding = DEF_PC_ENCODING;
		}
	}

	// try to get a message
	cc = processMQGet("BrowseNext", q, options, 0, 0, &mqmd, &rc);

	// check if it worked
	if (cc != MQCC_OK)
	{
		// get failed, get the reason and generate an error message

		// check for an empty queue
		if ((2 == cc) && (MQRC_NO_MSG_AVAILABLE == rc))
		{
			m_error_msg = "No more messages in queue";
		}
		else
		{
			// get the completion code and reason code
			setErrorMsg(cc, rc, "GetNext");
		}

		// get the current depth of the queue
		getCurrentDepth();

		// close the queue and disconnect from the queue manager
		discQM();

		return cc;
	}

	// process the data from the message
	processMessage(&mqmd);

	// get the current depth of the queue
	getCurrentDepth();

	rfhmsg[0] = 0;
	rfh1Len = ((RFH *)rfhData)->getRFH1length();
	rfh2Len = ((RFH *)rfhData)->getRFH2length();
	cicsLen = ((CCICS *)cicsData)->getCICSlength();
	imsLen = ((CIms *)imsData)->getIMSlength();
	dlqLen = ((CDlq *)dlqData)->getDLQlength();

	if (rfh2Len + rfh1Len + cicsLen + imsLen + dlqLen > 0)
	{
		sprintf(rfhmsg, " (data %d dlq %d rfh %d cics %d ims %d)", fileSize, dlqLen, rfh2Len + rfh1Len, cicsLen, imsLen);
	}

	// worked, tell the user the message was read
	browseCount++;
	sprintf(errtxt, "Next message (%d) read from %s length=%d%s", browseCount, (LPCTSTR)currentQ, fileSize, rfhmsg);

	// check if we are to issue a message
	if (!silent)
	{
		m_error_msg += errtxt;

		// update the source of the data
		fileSource.Format(_T("Message Data (%d) msg %d of %d from %s"), fileSize, browseCount, m_q_depth, (LPCTSTR)currentQ);
	}

	// allow browse previous
	browsePrevActive = 1;

	if (traceEnabled)
	{
		// log the results
		logTraceEntry(errtxt);

		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::browseNext() browseCount=%d m_save_browse_count=%d", browseCount, m_save_browse_count);

		// trace exit from browseNext
		logTraceEntry(traceInfo);
	}

	return MQCC_OK;
}

int DataArea::findPrevMsgId(const char * Qname)

{
	MQLONG		cc=MQCC_OK;				// MQ completion code
	MQLONG		rc=0;					// MQ reason code
	MQLONG		cc2=MQCC_OK;			// MQ completion code for final close
	MQLONG		rc2=0;					// MQ reason code for final close
	MQLONG		msgLen;
	MQLONG		closeOpt=0;				// Queue close options
	int			options=0;
	int			seq;
	int			ofs;
	CString	baseName;
	MQGMO		gmo={MQGMO_DEFAULT};	// Get message options
	MQBYTE24	msg_id;				// id of the messages we read
	MQBYTE24	curr_id;				// the message id we are looking for - copied from m_save_message_id
	MQMD2		mqmd={MQMD2_DEFAULT};	// MQ message descriptor
	bool		saveMsgId=true;			// should we remember the current message id for browse previous
	char		traceInfo[512];			// work variable to build trace message
	char		traceMsgId[64];			// work area for hex display of message id

	m_error_msg.Empty();

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::findPrevMsgId() Qname=%s", Qname);

		// trace entry to findPrevMsgId
		logTraceEntry(traceInfo);
	}

	// make sure we have a queue name
	if (strlen(Qname) == 0)
	{
		return -1;
	}

	// set the queue depth to zero, in case of errors
	m_q_depth = 0;

	// close the queue to start with
	closeQ(Q_CLOSE_NONE);

	// open the queue for browse
	if (!openQ(Qname, _T(""), Q_OPEN_BROWSE, FALSE))
	{
		// open failed
		return MQCC_FAILED;
	}

	// figure out what options we are using
	if (m_logical_order)
	{
		options |= MQGMO_LOGICAL_ORDER;
	}

	if (m_complete_msg)
	{
		options |= MQGMO_COMPLETE_MSG;
	}

	// get the message id we are looking for from the instance variable
	memcpy(curr_id, m_save_message_id, sizeof(curr_id));

	// clear the previous message id and message id work area
	memset(m_save_message_id, 0, sizeof(m_save_message_id));
	memset(msg_id, 0, sizeof(msg_id));

	// reset the saved browse count
	m_save_browse_count = 0;

	// starting a browse operation
	// we do not need to read the data so accept truncated messages
	gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST | MQGMO_ACCEPT_TRUNCATED_MSG | options;

	// make sure that the match options are honored
	gmo.Version = MQGMO_VERSION_2;

	if (traceEnabled)
	{
		// convert the message id that we just read to hex
		AsciiToHex(curr_id, 24, (unsigned char *)traceMsgId);

		// build the trace line
		sprintf(traceInfo, "searching for message with curr_id=%s using options %d", traceMsgId, gmo.Options);

		// write a trace log entry
		logTraceEntry(traceInfo);
	}

	// try to browse the messages in the queue
	do
	{
		// clear the previous message id
		memcpy(mqmd.MsgId, curr_id, MQ_MSG_ID_LENGTH);

		// Try to get a message
		XMQGet(qm, q, &mqmd, &gmo, 0, NULL, &msgLen, &cc, &rc);
		if (cc != MQCC_OK)
		{
			// get failed, get the reason and generate an error message
			if ((cc != MQCC_OK) && (rc != MQRC_NO_MSG_AVAILABLE))
			{
				// get the completion code and reason code
				setErrorMsg(cc, rc, "Get (QBrowse)");
			}
		}
		else
		{
			// check if we should remember this message id
			saveMsgId = true;
			m_save_browse_count++;

			// are we in the middle of a message segment?
			ofs = mqmd.Offset;
			if (m_complete_msg && (ofs != 0))
			{
				// can't restart here
				saveMsgId = false;
			}

			// are we in the middle of a group or segment with logical ordering turned on?
			// N.B. It is not clear why the check for ofs > 0 is required.  This may be
			// a bug.
			seq = mqmd.MsgSeqNumber;
			if (m_logical_order && ((seq > 1) || (ofs > 0)))
			{
				// can't restart here
				saveMsgId = false;
			}

			// remember the message id
			memcpy(msg_id, mqmd.MsgId, MQ_MSG_ID_LENGTH);

			if (saveMsgId && (memcmp(msg_id, curr_id, sizeof(msg_id)) != 0))
			{
				// save this id as the previous id
				memcpy(m_save_message_id, msg_id, sizeof(m_save_message_id));
			}
		}

		if (traceEnabled)
		{
			// convert the message id that we just read to hex
			AsciiToHex(msg_id, 24, (unsigned char *)traceMsgId);

			// build the trace line
			sprintf(traceInfo, "Browsed message using options %d saveMsgId %d msg_id=%s", gmo.Options, saveMsgId, traceMsgId);

			// write a trace log entry
			logTraceEntry(traceInfo);
		}

		// switch to a browse next operation after the first get
		gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_NEXT | MQGMO_ACCEPT_TRUNCATED_MSG | options;
	} 	while ((MQCC_OK == cc) & (memcmp(msg_id, curr_id, sizeof(msg_id)) != 0));

	// close the queue
	closeQ(Q_CLOSE_NONE);

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::findPrevMsgId() cc=%d rc=%d", cc, rc);

		// trace entry to findPrevMsgId
		logTraceEntry(traceInfo);
	}

	return cc;
}

/////////////////////////////////////////////////////////////////////////
//
// This routine will try to do a browse previous operation.  There is
// no really clean way to do this.  During browse next operations, the
// message id of the previous message is saved.  When a browse previous
// request is made, the current browse operation is stopped and a new
// browse operation is started.  Messages are read until a message is
// found with the message id of the previous message.  At that point
// the browse operation has backed up one message.
//
// There are several problems with this approach.  First, another
// application may have read the previous message.  Second, the
// queue must be scanned sequentially to find the desired message.
// This is necessary to find the message before the desired message,
// so that browse previous works more than once.  Third, the
// message counters and queue depth may change during the browse
// operation.  If messages have been read from the queue, then the
// number of the previous message may be different than the current
// message number minus one.
//
/////////////////////////////////////////////////////////////////////////

int DataArea::browsePrev()

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	MQLONG		cc=MQCC_OK;					// MQ completion code
	MQLONG		rc=MQRC_NONE;				// MQ reason code
	MQLONG		cc2=MQCC_OK;				// MQ completion code
	MQLONG		rc2=MQRC_NONE;				// MQ reason code
	MQLONG		msgLen;						// length of the message data
	MQHMSG		hMsg=MQHM_UNUSABLE_HMSG;	// message handle used to get message properties
	MQCMHO		opts={MQCMHO_DEFAULT};		// options used to create message handle
	MQDMHO		dOpts={MQDMHO_DEFAULT};		// options used to delete message handle
	int			options=0;
	int			rfh1Len;					// length of the RFH V1 header
	int			rfh2Len;					// length of the RFH V2 header
	int			dlqLen;						// length of the dead letter queue header
	int			cicsLen;					// length of the CICS header
	int			imsLen;						// length of the IMS header
	int			bufLen=16*1024*1024;		// length of properties buffer
	CRfhutilApp *app;						// pointer to the MFC application object (change cursor to hour glass)
	char *		msgData=NULL;				// pointer to user data buffer
	MQBYTE24	msg_id;						// id of the messages we read
	MQBYTE24	curr_id;					// the message id we are looking for - copied from m_save_message_id
	MQMD2		mqmd={MQMD2_DEFAULT};		// MQ Message Descriptor
	MQGMO		gmo={MQGMO_DEFAULT};		// Get message options
	char		rfhmsg[128];				// work area to build display message including header lengths
	char		errtxt[512];				// work variable to build message
	char		traceInfo[512];				// work variable to build trace message

	// initialize message error
	errtxt[0] = 0;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::browsePrev() browseCount=%d m_save_browse_count=%d", browseCount, m_save_browse_count);

		// trace entry to browsePrev
		logTraceEntry(traceInfo);
	}

	// make sure the q has been opened
	if (!isQueueOpen())
	{
		// return immediately
		browsePrevActive = 0;
		return -1;
	}

	// check if we have a previous message id
	if (memcmp(m_save_message_id, MQMI_NONE, MQ_MSG_ID_LENGTH) == 0)
	{
		// no previous message id - just turn off browse previous and return
		browsePrevActive = 0;
		return 0;
	}

	// save the previous msg id
	memcpy(curr_id, m_save_message_id, sizeof(curr_id));

	// clear the previous message id and counts
	memset(m_save_message_id, 0, sizeof(m_save_message_id));
	m_save_browse_count = 0;
	browseCount = 0;

	// set some of the specified MQ GMO options
	options = setMQoptions(options);

	// start a new browse first operation
	// for the main loop no data will actually be read in
	// this can speed up the operation significantly
	gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST | MQGMO_ACCEPT_TRUNCATED_MSG | options;

	// make sure that the match options are honored
	gmo.Version = MQGMO_VERSION_2;

	// The purpose of this loop is to browse the queue looking for a particular message.
	// The message id of the previous message is saved.
	do
	{
		// clear the previous message id, correlation id and group in the message descriptor
		memset(mqmd.MsgId, 0, MQ_MSG_ID_LENGTH);
		memset(&mqmd.CorrelId, 0, MQ_CORREL_ID_LENGTH);
		memset(&mqmd.GroupId, 0, MQ_GROUP_ID_LENGTH);

		// Try to get a message
		XMQGet(qm, q, &mqmd, &gmo, 0, NULL, (long *)&msgLen, &cc, &rc);

		// check for a truncated message accepted (2079)
		if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_ACCEPTED == rc))
		{
			cc = MQCC_OK;
		}

		// check if it worked
		if (cc != MQCC_OK)
		{
			// get failed, get the reason and generate an error message
			if (rc != MQRC_NO_MSG_AVAILABLE)
			{
				// get the completion code and reason code
				setErrorMsg(cc, rc, "BrowsePrevGet");
			}
		}
		else
		{
			// increment the message counts
			browseCount++;

			// remember the message id
			memcpy(msg_id, mqmd.MsgId, MQ_MSG_ID_LENGTH);

			if (memcmp(msg_id, curr_id, sizeof(msg_id)) != 0)
			{
				// save this id as the previous id
				memcpy(m_save_message_id, msg_id, sizeof(m_save_message_id));
				m_save_browse_count++;
			}
		}

		// switch to a browse next operation after the first get
		gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_NEXT | MQGMO_ACCEPT_TRUNCATED_MSG | options;
	} 	while ((MQCC_OK == cc) & (memcmp(msg_id, curr_id, sizeof(msg_id)) != 0));

	// did we find the message we were looking for?
	if ((MQCC_OK == cc) & (memcmp(msg_id, curr_id, sizeof(msg_id)) == 0))
	{
		// found the message we were looking for and know how long it is
		// it must now be read again matching on message id to get the data
		// but first a buffer must be allocated for the message
		msgData = (char *)rfhMalloc(msgLen + 16, "MSGDATA3");

		// check if conversion of message was requested
		if (m_convert)
		{
			// is the code page specified on the MQMD tab?
			if (mqmdObj->m_mqmd_ccsid.GetLength() > 0)
			{
				// use the selected ccsid
				mqmd.CodedCharSetId = atoi((LPCTSTR)mqmdObj->m_mqmd_ccsid);
			}

			// set the encoding in case we are dealing with UCVS-2
			if (NUMERIC_PC == mqmdObj->m_mqmd_encoding)
			{
				// use the specified integer encoding for the get with convert
				mqmd.Encoding = DEF_PC_ENCODING;
			}
			else
			{
				// use the specified integer encoding for the get with convert
				mqmd.Encoding = DEF_UNIX_ENCODING;
			}
		}

		// read the message we are interested in this time getting all the data
		gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_MSG_UNDER_CURSOR | options;

		// check if message properties are to be processed
		if (propertiesSupported)
		{
			// make sure that the message handle processing options are honored
			gmo.Version = MQGMO_VERSION_4;

			// check if properties should be returned in a handle
			if (MQ_PROPS_YES == m_mq_props)
			{
				// set the message handle options
				opts.Options = MQCMHO_VALIDATE;

				// set the message options
				gmo.Options |= MQGMO_PROPERTIES_IN_HANDLE;

				// create a message handle
				XMQCrtMh(qm, &opts, &hMsg, &cc2, &rc2);

				if (traceEnabled)
				{
					// enter the result in the trace
					sprintf(traceInfo, " MQCrtMh cc2=%d rc2=%d hMsg=%8.8X", cc2, rc2, (unsigned int)hMsg);

					// write a trace entry
					logTraceEntry(traceInfo);
				}

				// check if it worked
				if (cc2 != MQCC_OK)
				{
					// unable to create the message handle - report the error
					setErrorMsg(cc2, rc2, "MQCRTMH");

					// return completion code
					return cc2;
				}

				// set the handle in the GMO
				gmo.MsgHandle = hMsg;
			}
			else if (MQ_PROPS_NONE == m_mq_props)
			{
				// set properties to none - they will be ignored
				opts.Options |= MQGMO_NO_PROPERTIES;
			}
			else if (MQ_PROPS_RFH2 == m_mq_props)
			{
				// set properties to force into RFH2 header
				opts.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;
			}
			else if (MQ_PROPS_COMPAT == m_mq_props)
			{
				// set properties to force into RFH2 header
				gmo.Options |= MQGMO_PROPERTIES_COMPATIBILITY;
			}
		}

		// read the message
		XMQGet(qm, q, &mqmd, &gmo, msgLen, msgData, (long *)&msgLen, &cc, &rc);

		if (MQCC_OK == cc)
		{
			// clear any previous data areas?
			resetDataArea();

			// clear any previous RFH headers
			((RFH *)rfhData)->clearRFH();

			// clear any previous user properties
			((CProps *)propData)->clearUserProperties();


			// check if message properties are to be processed
			if (propertiesSupported && (MQ_PROPS_YES == m_mq_props))
			{
				// extract the message properties
				extractMsgProps(hMsg, 0, NULL, NULL, 0);

				// delete the message handle
				XMQDltMh(qm, &hMsg, &dOpts, &cc2, &rc2);

				// check if it worked
				if (cc2 != MQCC_OK)
				{
					// unable to delete the message handle - report the error
					setErrorMsg(cc2, rc2, "MQDLTMH");
				}
			}

			// update the fileData and fileSize information
			fileData = (unsigned char *)msgData;
			fileSize = msgLen;

			// process the data from the message descriptor
			processMessage(&mqmd);

			// get the current depth of the queue
			getCurrentDepth();

			rfhmsg[0] = 0;
			rfh1Len = ((RFH *)rfhData)->getRFH1length();
			rfh2Len = ((RFH *)rfhData)->getRFH2length();
			cicsLen = ((CCICS *)cicsData)->getCICSlength();
			imsLen = ((CIms *)imsData)->getIMSlength();
			dlqLen = ((CDlq *)dlqData)->getDLQlength();

			if (rfh2Len + rfh1Len + cicsLen + imsLen + dlqLen > 0)
			{
				sprintf(rfhmsg, " (data %d dlq %d rfh %d cics %d ims %d)", fileSize, dlqLen, rfh2Len + rfh1Len, cicsLen, imsLen);
			}

			// tell the user what we did
			sprintf(errtxt, "Previous message (%d) read from %s length=%d%s", browseCount, (LPCTSTR)currentQ, fileSize, rfhmsg);
			m_error_msg += errtxt;

			// update the source of the data
			fileSource.Format("Message Data (%d) msg %d of %d from %s", fileSize, browseCount, m_q_depth, (LPCTSTR)currentQ);

			// indicate that a browse is active
			browseActive = 1;

			// check if we should leave browse previous active set on
			if ((memcmp(m_save_message_id, MQMI_NONE, MQ_MSG_ID_LENGTH) == 0) || (1 == browseCount))
			{
				// looks like we are back to the first message
				browsePrevActive = 0;
			}
			else
			{
				// allow browse previous operations from here
				browsePrevActive = 1;
			}
		}
		else
		{
			// check if any storage was acquired
			if (msgData != NULL)
			{
				// release the storage we acquired
				rfhFree(msgData);
			}

			// tell the user what happened
			setErrorMsg(cc, rc, "BrowsePrevGetMsg");
		}

	}
	else
	{
		// error trying to find the previous message
		// disconnect from the queue manager and release the browse items
		discQM();
		cc = -1;
	}

	// call the audit routine to record the MQGET
	app = (CRfhutilApp *)AfxGetApp();
	app->createAuditRecord((LPCTSTR)currentQM, (LPCTSTR)currentQ, "BrowsePrev", rc);

	if (traceEnabled)
	{
		// trace results of browsePrev
		logTraceEntry(errtxt);

		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::browsePrev() cc=%d rc=%d browsePrevActive=%d browseCount=%d msgLen=%d", cc, rc, browsePrevActive, browseCount, msgLen);

		// trace exit from browsePrev
		logTraceEntry(traceInfo);
	}

	return cc;
}

//////////////////////////////////////////////////////////
//
// Routine to process insert a variable number of blanks
// to offset the display.  This is used for displays of
// data formats that have a hierarchy, such as COBOL copy
// book displays.
//
//////////////////////////////////////////////////////////

void DataArea::setOfs(CString &currentData, const int currentLevel)

{
	int		i;
	char	tempstr[11];

	strcpy(tempstr, "");

	// determine the number of blanks to insert,
	// limiting to ten
	i = currentLevel;
	if (i > 10)
	{
		i = 10;
	}

	while (i > 0)
	{
		strcat(tempstr, " ");
		i--;
	}

	// append the proper number of blanks
	currentData += tempstr;
}

//////////////////////////////////////////////
//
// This routine will clean up after a 2009
// reason code has been received.  The 2009
// reason code indicates that the connection
// to the QMgr has been lost and therefore
// all handles are invalid.
//
//////////////////////////////////////////////

void DataArea::connectionLostCleanup()

{
	// clear the queue related fields
	q = NULL;							// queue handle
	Qopen = Q_OPEN_NOT;					// open type
	m_save_set_all_setting = FALSE;
	m_save_set_user_setting = FALSE;
	currentQ.Empty();					// name of the queue
	currentRemoteQM.Empty();			// name of remote QM where queue resides
	transmissionQueue.Empty();			// name of the transmission queue
	currentUserid.Empty();				// user id that queue was opened under

	// set the admin queue to NULL
	hAdminQ = NULL;

	// make sure hte reply Q handle is NULL
	hReplyQ = NULL;

	// clear the subscription handle
	hSub = MQHO_NONE;

	// clear the subscription queue handle
	subQ = MQHO_NONE;

	// indicate that the QM connection is gone
	// remember the connection is no longer active
	connected = false;
	qm = NULL;

	// reset the browse indicators
	browseActive = 0;
	browsePrevActive = 0;

	// get rid of the queue manager name that the connection is made to
	currentQM.Empty();

	// reset the unit of work indicator
	unitOfWorkActive = false;
}

//////////////////////////////////////////////////////////
//
// Routine to build an error message based on the MQ
// return code and reason.  The error message is appended
// to the current error messasge variable.
//
//////////////////////////////////////////////////////////

void DataArea::setErrorMsg(MQLONG cc, MQLONG rc, const char * operation)

{
	char	errtxt[512];	// work variable to build error message
	char	traceInfo[512];	// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::setErrorMsg() cc=%d, rc=%d, operation=%s", cc, rc, operation);

		// trace entry to browseNext
		logTraceEntry(traceInfo);
	}

	switch (rc)
	{
	case MQRC_ALIAS_BASE_Q_TYPE_ERROR:
		{
			strcpy(errtxt, "2001 Invalid base queue type for alias queue");
			break;
		}
	case MQRC_BUFFER_ERROR:
		{
			strcpy(errtxt, "2004 Buffer error");
			break;
		}
	case MQRC_BUFFER_LENGTH_ERROR:
		{
			strcpy(errtxt, "2005 Buffer length error");
			break;
		}
	case MQRC_CONNECTION_BROKEN:
		{
			strcpy(errtxt, "2009 Connection to QM has been lost - Q closed");

			// clean up the handles and indicators
			connectionLostCleanup();

			break;
		}
	case MQRC_DATA_LENGTH_ERROR:
		{
			sprintf(errtxt, "2010 Max msg length exceeded - check Q/QM/Channel defs - QMgr max=%d Queue max=%d", maxMsgLenQM, maxMsgLenQ);
			break;
		}
	case MQRC_EXPIRY_ERROR:
		{
			strcpy(errtxt, "2013 Invalid expiry value");
			break;
		}
	case MQRC_FEEDBACK_ERROR:
		{
			strcpy(errtxt, "2014 Invalid feedback value");
			break;
		}
	case MQRC_GET_INHIBITED:
		{
			strcpy(errtxt, "2016 Queue get inhibited");
			break;
		}
	case MQRC_HCONN_ERROR:
		{
			strcpy(errtxt, "2018 Connection handle error");
			break;
		}
	case MQRC_HOBJ_ERROR:
		{
			strcpy(errtxt, "2019 Queue handle error");
			break;
		}
	case MQRC_SYNCPOINT_LIMIT_REACHED:
		{
			strcpy(errtxt, "2024 Maximum uncommitted messages in UOW exceeded for queue manager");
			break;
		}
	case MQRC_MAX_CONNS_LIMIT_REACHED:
		{
			strcpy(errtxt, "2025 Maximum connections exceeded for queue manager");
			break;
		}
	case MQRC_MD_ERROR:
		{
			strcpy(errtxt, "2026 Error in MQMD");
			break;
		}
	case MQRC_MISSING_REPLY_TO_Q:
		{
			strcpy(errtxt, "2027 Reply to queue name missing - request message or report options specified");
			break;
		}
	case MQRC_MSG_TYPE_ERROR:
		{
			strcpy(errtxt, "2029 Invalid message type");
			break;
		}
	case MQRC_MSG_TOO_BIG_FOR_Q:
		{
			sprintf(errtxt, "2030 Message too large for queue (check q and qm settings) QMgr max=%d Queue max=%d", maxMsgLenQM, maxMsgLenQ);
			break;
		}
	case MQRC_MSG_TOO_BIG_FOR_Q_MGR:
		{
			sprintf(errtxt, "2031 Message is too large for queue manager QMgr max=%d Queue max=%d", maxMsgLenQM, maxMsgLenQ);
			break;
		}
	case MQRC_NO_MSG_AVAILABLE:
		{
			strcpy(errtxt, "2033 No messages in queue");
			m_q_depth = 0;
			break;
		}
	case MQRC_NOT_AUTHORIZED:
		{
			sprintf(errtxt, "2035 Not authorized (%s)", operation);
			break;
		}
	case MQRC_NOT_OPEN_FOR_INQUIRE:
		{
			sprintf(errtxt, "2038 Not open for inquire");
			break;
		}
	case MQRC_NOT_OPEN_FOR_OUTPUT:
		{
			sprintf(errtxt, "2039 Not open for output");
			break;
		}
	case MQRC_OBJECT_CHANGED:
		{
			strcpy(errtxt, "2041 Object definition changed");
			break;
		}
	case MQRC_OBJECT_IN_USE:
		{
			strcpy(errtxt, "2042 Object in use");
			break;
		}
	case MQRC_OPTION_NOT_VALID_FOR_TYPE:
		{
			strcpy(errtxt, "2045 Cannot read remote queue");
			break;
		}
	case MQRC_OPTIONS_ERROR:
		{
			sprintf(errtxt, "2046 Invalid option for operation %s", operation);
			break;
		}
	case MQRC_PERSISTENCE_ERROR:
		{
			strcpy(errtxt, "2047 Persistence error");
			break;
		}
	case MQRC_PERSISTENT_NOT_ALLOWED:
		{
			strcpy(errtxt, "2048 Persistence not supported on this queue");
			break;
		}
	case MQRC_PRIORITY_EXCEEDS_MAXIMUM:
		{
			strcpy(errtxt, "2049 Priority exceeds maximum");
			break;
		}
	case MQRC_PRIORITY_ERROR:
		{
			strcpy(errtxt, "2050 Invalid priority value");
			break;
		}
	case MQRC_PUT_INHIBITED:
		{
			strcpy(errtxt, "2051 Put inhibited");
			break;
		}
	case MQRC_Q_DELETED:
		{
			strcpy(errtxt, "2052 Queue has been deleted");
			break;
		}
	case MQRC_Q_FULL:
		{
			strcpy(errtxt, "2053 Queue full");
			break;
		}
	case MQRC_Q_SPACE_NOT_AVAILABLE:
		{
			sprintf(errtxt, "2056 Out of space on queue disk");
			break;
		}
	case MQRC_Q_TYPE_ERROR:
		{
			sprintf(errtxt, "2057 Invalid queue type for operation (%s)", operation);
			break;
		}
	case MQRC_Q_MGR_NAME_ERROR:
		{
			sprintf(errtxt, "2058 Queue manager name not found (%s)", operation);
			break;
		}
	case MQRC_Q_MGR_NOT_AVAILABLE:
		{
			sprintf(errtxt, "2059 Queue manager not available (%s) - may not be started", operation);
			break;
		}
	case MQRC_REPORT_OPTIONS_ERROR:
		{
			sprintf(errtxt, "2061 Invalid report options");
			break;
		}
	case MQRC_SECURITY_ERROR:
		{
			sprintf(errtxt, "2063 Security exit rejected connection");
			break;
		}
	case MQRC_STORAGE_NOT_AVAILABLE:
		{
			sprintf(errtxt, "2071 Main storage not available");
			break;
		}
	case MQRC_TRUNCATED_MSG_ACCEPTED:
		{
			sprintf(errtxt, "2079 Truncated message accepted");
			break;
		}
	case MQRC_TRUNCATED_MSG_FAILED:
		{
			sprintf(errtxt, "2080 Message too long for buffer");
			break;
		}
	case MQRC_UNKNOWN_OBJECT_NAME:
		{
			strcpy(errtxt, "2085 No such queue");
			break;
		}
	case MQRC_UNKNOWN_REMOTE_Q_MGR:
		{
			strcpy(errtxt, "2087 Invalid remote queue manager name");
			break;
		}
	case MQRC_NOT_OPEN_FOR_PASS_ALL:
		{
			strcpy(errtxt, "2093 Queue not open for pass all");
			break;
		}
	case MQRC_NOT_OPEN_FOR_PASS_IDENT:
		{
			strcpy(errtxt, "2094 Queue not open for pass identity");
			break;
		}
	case MQRC_NOT_OPEN_FOR_SET_ALL:
		{
			strcpy(errtxt, "2095 Queue not open for set all");
			break;
		}
	case MQRC_NOT_OPEN_FOR_SET_IDENT:
		{
			strcpy(errtxt, "2096 Queue not open for set identity");
			break;
		}
	case MQRC_OBJECT_DAMAGED:
		{
			strcpy(errtxt, "2101 Object has been corrupted");
			break;
		}
	case MQRC_RESOURCE_PROBLEM:
		{
			strcpy(errtxt, "2101 Resource shortage - see FFST record");
			break;
		}
	case MQRC_FORMAT_ERROR:
		{
			strcpy(errtxt, "2110 Convert requested but format does not allow conversion");
			break;
		}
	case MQRC_SOURCE_CCSID_ERROR:
		{
			strcpy(errtxt, "2111 Invalid or unsupported code page");
			break;
		}
	case MQRC_HEADER_ERROR:
		{
			strcpy(errtxt, "2142 Invalid MQ header");
			break;
		}
	case MQRC_Q_MGR_QUIESCING:
		{
			strcpy(errtxt, "2161 Queue manager is quiescing");
			break;
		}
	case MQRC_Q_MGR_STOPPING:
		{
			strcpy(errtxt, "2162 Queue manager is stopping");
			break;
		}
	case MQRC_STORAGE_MEDIUM_FULL:
		{
			strcpy(errtxt, "2192 No more space on disk");
			break;
		}
	case MQRC_PAGESET_ERROR:
		{
			strcpy(errtxt, "2193 Page set error");
			break;
		}
	case MQRC_UNEXPECTED_ERROR:
		{
			strcpy(errtxt, "2195 Internal error processing request");
			break;
		}
	case MQRC_CONNECTION_QUIESCING:
		{
			strcpy(errtxt, "2202 Connection is quiescing");
			break;
		}
	case MQRC_CONNECTION_STOPPING:
		{
			strcpy(errtxt, "2203 Connection is stopping");
			break;
		}
	case MQRC_CD_ERROR:
		{
			strcpy(errtxt, "2277 Invalid channel name");
			break;
		}
	case MQRC_SELECTOR_NOT_SUPPORTED:
		{
			strcpy(errtxt, "2318 QMgr does not support selectors");
			break;
		}
	case MQRC_RFH_ERROR:
		{
			strcpy(errtxt, "2334 Error in RFH");
			break;
		}
	case MQRC_RFH_STRING_ERROR:
		{
			strcpy(errtxt, "2335 RFH string error");
			break;
		}
	case MQRC_RFH_COMMAND_ERROR:
		{
			strcpy(errtxt, "2336 RFH command error");
			break;
		}
	case MQRC_RFH_PARM_ERROR:
		{
			strcpy(errtxt, "2337 RFH parameter error");
			break;
		}
	case MQRC_RFH_DUPLICATE_PARM:
		{
			strcpy(errtxt, "2338 Duplicate parameter in RFH");
			break;
		}
	case MQRC_RFH_PARM_MISSING:
		{
			strcpy(errtxt, "2339 Missing parameter in RFH");
			break;
		}
	case MQRC_KEY_REPOSITORY_ERROR:
		{
			strcpy(errtxt, "2381 Key repository error");
			break;
		}
	case MQRC_SSL_CONFIG_ERROR:
		{
			strcpy(errtxt, "2392 SSL configuration error");
			break;
		}
	case MQRC_SSL_INITIALIZATION_ERROR:
		{
			strcpy(errtxt, "2393 SSL unable to initialize - check SSL parms");
			break;
		}
	case MQRC_SSL_NOT_ALLOWED:
		{
			strcpy(errtxt, "2396 SSL connection not allowed");
			break;
		}
	case MQRC_SSL_PEER_NAME_MISMATCH:
		{
			strcpy(errtxt, "2398 Peer name mismatch");
			break;
		}
	case MQRC_SSL_PEER_NAME_ERROR:
		{
			strcpy(errtxt, "2399 Peer name error");
			break;
		}
	case MQRC_SSL_CERTIFICATE_REVOKED:
		{
			strcpy(errtxt, "2401 Certificate revoked");
			break;
		}
	case MQRC_SSL_CERT_STORE_ERROR:
		{
			strcpy(errtxt, "2402 Certificate store error");
			break;
		}
	case MQRC_SSL_KEY_RESET_ERROR:
		{
			strcpy(errtxt, "2409 SSL key reset error");
			break;
		}
	case MQRC_MSG_NOT_ALLOWED_IN_GROUP:
		{
			strcpy(errtxt, "2417 Published message cannot be part of group");
			break;
		}
	case MQRC_SD_ERROR:
		{
			strcpy(errtxt, "2424 Error in subscription descriptor");
			break;
		}
	case MQRC_TOPIC_STRING_ERROR:
		{
			strcpy(errtxt, "2425 Topic string error");
			break;
		}
	case MQRC_NO_SUBSCRIPTION:
		{
			strcpy(errtxt, "2428 No subscription");
			break;
		}
	case MQRC_SUBSCRIPTION_IN_USE:
		{
			strcpy(errtxt, "2429 Subscription in use");
			break;
		}
	case MQRC_SUB_USER_DATA_ERROR:
		{
			strcpy(errtxt, "2431 Subscription user data error");
			break;
		}
	case MQRC_SUB_ALREADY_EXISTS:
		{
			strcpy(errtxt, "2432 Subscription already exists");
			break;
		}
	case MQRC_SUB_NAME_ERROR:
		{
			strcpy(errtxt, "2440 Subscription name error");
			break;
		}
	case MQRC_PROPERTY_NAME_ERROR:
		{
			strcpy(errtxt, "2442 Property name error");
			break;
		}
	case MQRC_SELECTOR_SYNTAX_ERROR:
		{
			strcpy(errtxt, "2459 Selector syntax error");
			break;
		}
	case MQRC_PROP_TYPE_NOT_SUPPORTED:
		{
			strcpy(errtxt, "2467 Property type not supported");
			break;
		}
	case MQRC_PROPERTY_TYPE_ERROR:
		{
			strcpy(errtxt, "2473 Invalid property type");
			break;
		}
	case MQRC_PUBLICATION_FAILURE:
		{
			strcpy(errtxt, "2502 Publication failed");
			break;
		}
	case MQRC_SUB_INHIBITED:
		{
			strcpy(errtxt, "2503 Topic is subscribe inhibited");
			break;
		}
	case MQRC_SELECTOR_ALWAYS_FALSE :
		{
			strcpy(errtxt, "2504 Selector is always false");
			break;
		}
	case MQRC_PROPERTY_NAME_LENGTH_ERR:
		{
			strcpy(errtxt, "2513 Property name length error");
			break;
		}
	case MQRC_SELECTION_STRING_ERROR:
		{
			strcpy(errtxt, "2519 Selection string error");
			break;
		}
	case MQRC_SELECTOR_NOT_ALTERABLE:
		{
			strcpy(errtxt, "2524 Selector cannot be altered");
			break;
		}
	case MQRC_CONNECTION_STOPPED:
		{
			strcpy(errtxt, "2528 Connection stopped");
			break;
		}
	case MQRC_PUBSUB_INHIBITED:
		{
			strcpy(errtxt, "2531 Pub/sub inhibited");
			break;
		}
	case MQRC_CHANNEL_NOT_AVAILABLE:
		{
			strcpy(errtxt, "2537 Channel not available or all connections in use");
			break;
		}
	case MQRC_HOST_NOT_AVAILABLE:
		{
			strcpy(errtxt, "2538 Host not available");
			break;
		}
	case MQRC_CHANNEL_CONFIG_ERROR:
		{
			strcpy(errtxt, "2539 channel configuration error");
			break;
		}
	case MQRC_UNKNOWN_CHANNEL_NAME:
		{
			strcpy(errtxt, "2540 Invalid MQ server connection channel name");
			break;
		}
	case MQRC_STANDBY_Q_MGR:
		{
			strcpy(errtxt, "2543 Queue manager is in standby mode");
			break;
		}
	case MQRC_ATTRIBUTE_LOCKED:
		{
			strcpy(errtxt, "6104 Resouce locked - queue has been closed");

			// close the queue
			closeQ(Q_CLOSE_NONE);

			break;
		}
	default:
		{
			// build an error message
			sprintf(errtxt, "*Error cc=%d rc=%d Cannot %s", cc, rc, operation);
		}
	}

	// save the error in an instance variable
	// append the error data
	appendError(errtxt);

	// write the error message to the trace file if trace is enabled
	if (traceEnabled)
	{
		logTraceEntry(errtxt);
	}
}

////////////////////////////////////////////////////////////////
//
// Subroutine to return the file data as a COBOL copy book
// If the ASCII switch is false, then character data is
// translated from EBCDIC so it can be easily viewed.
//
////////////////////////////////////////////////////////////////

CString DataArea::getCobolData(const int charFormat,
							   const int numFormat,
							   const int pdNumFormat,
							   const int indent,
							   const int checkData)

{
	CString	cobolData;	// string to hold ascii representation of data
	int		rc;
	int		maxVars = 0;
	int		maxSize  = 0;
	char	*textArea;
	char	traceInfo[512];	// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getCobolData() charFormat=%d, numFormat=%d, pdNumFormat=%d indent=%d copybook=%s", charFormat, numFormat, pdNumFormat, indent, (LPCTSTR)m_copybook_file_name);

		// trace entry to getCobolData
		logTraceEntry(traceInfo);
	}

	//. initialize the string
	cobolData.Empty();

	if (m_copybook_file_name.GetLength() == 0)
	{
		rc = readCopybookFile();

		if (rc != IDOK)
		{
			return cobolData.AllocSysString();
		}
	}

	if ((fileSize > 0) && (fileData != NULL))
	{
		maxVars = m_copybook->getMaxLines();
		maxSize = (maxVars * 128) + 65536;
		textArea = (char *) rfhMalloc(maxSize + 8192, "COBOLDAT");
		textArea[0] = 0;

		// Now, build the output area
		m_copybook->formatData(textArea,
							   maxSize,
							   fileData,
							   fileSize,
							   charFormat,
							   numFormat,
							   pdNumFormat,
							   fileCcsid,
							   indent,
							   checkData);

		cobolData = "Level  Ofs  Len Type  Occ Variable Name                  Value\r\n";
		cobolData += textArea;
		rfhFree(textArea);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getCobolData() maxVars=%d, maxSize=%d, cobolData.GetLength()=%d", maxVars, maxSize, cobolData.GetLength());

		// trace exit from getCobolData
		logTraceEntry(traceInfo);
	}

	return cobolData.AllocSysString();
}

/////////////////////////////////////
//
// Routine to get the name of a
// COBOL copy book to be used
// to display the current file
// or message data.
//
/////////////////////////////////////


int DataArea::readCopybookFile()

{
	int		rc;
	char	tempmsg[128];
	char	traceInfo[512];	// work variable to build trace message

	if (traceEnabled)
	{
		// trace entry to readCopybookFile
		logTraceEntry("Entering DataArea::readCopybookFile()");
	}

	// invoke standard dialog to choose file name
	CFileDialog fd(TRUE, NULL, m_copybook_file_name);

	// set dialog box title
	fd.m_ofn.lpstrTitle = _T("COBOL copy book file");

	// do not show read only files
	fd.m_ofn.Flags |= OFN_HIDEREADONLY;

	// set some filter patterns
	fd.m_ofn.lpstrFilter = _T("All (*.*)\0*.*\0COBOL files (*.cbl;*.cpy)\0*.cbl;*.cpy\0Text (*.TXT)\0*.TXT\0");

	// execute the file dialog
	rc = fd.DoModal();

	//	Save the copy book file name
	if (IDOK == rc)
	{
		//m_copybook_file_name = fd.GetFileName();
		m_copybook_file_name = fd.GetPathName();

		if (m_copybook == NULL)
		{
			m_copybook = new CCopybook();
		}

		// clear the current copy book variables
		m_copybook->resetCopybook();

		// read the copy book from the file and parse it
		m_copybook->parseCopyBook((LPCTSTR)m_copybook_file_name);

		// tell the user
		m_error_msg = "copybook file read (" + m_copybook_file_name + ")";
	}
	else
	{
		if (rc != IDCANCEL)
		{
			sprintf(tempmsg, "%d error reading copy book file", rc);
			m_error_msg = tempmsg;
		}
	}

	if (traceEnabled)
	{
		if (IDOK == rc)
		{
			// create the trace line if the file dialog was successful
			sprintf(traceInfo, "Exiting DataArea::readCopybookFile() m_copybook_file_name=%s", (LPCTSTR)m_copybook_file_name);
		}
		else
		{
			// create the trace line if the dialog was cancelled or failed
			sprintf(traceInfo, "Exiting DataArea::readCopybookFile() rc=%d", rc);
		}

		// trace exit from readCopybookFile
		logTraceEntry(traceInfo);
	}

	return rc;
}

//////////////////////////////////////////////////////////////
//
// Routine to read all the messages from an MQSeries Queue,
//  removing them from the queue
//
//////////////////////////////////////////////////////////////

void DataArea::purgeQueue(LPCTSTR QMname, LPCTSTR Queue, CEdit * depthBox)

{
	// define the MQ objects that we need
	int			msgCount=0;
	int			uowCount=0;
	MQLONG		cc=MQCC_OK;
	MQLONG		rc;
	MQLONG		msgLen;
 	int			queueType;				// MQSeries type of queue
	CRfhutilApp *app;
	MQGMO		gmo={MQGMO_DEFAULT};	// Get message options
	MQMD2		mqmd={MQMD2_DEFAULT};	// MQ message descriptor (MQMD)
	CString		remoteQM;
	char		msgtxt[32];
	char		errtxt[512];			// work variable to build error message
	char		traceInfo[512];			// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::purgeQueue() QMname=%s Queue=%s", QMname, Queue);

		// trace entry to purgeQueue
		logTraceEntry(traceInfo);
	}

	m_error_msg.Empty();
	remoteQM.Empty();

	// make sure we have a queue name
	if (strlen(Queue) == 0)
	{
		m_error_msg = "*Queue Name required* ";
		return;
	}

	// make sure we are connected to the correct QM
	if (!checkConnection(QMname))
	{
		return;
	}

	// now check if we have the correct queue open
	// or not with the correct options or
	// try to open the queue if necessary
	if (!openQ(Queue, remoteQM, Q_OPEN_READ, FALSE))
	{
		// didn't work
		return;
	}

	m_queue_type.Empty();
	queueType = getQueueType(q);
	setQueueType(queueType);

	// set the message options
	gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_SYNCPOINT_IF_PERSISTENT | MQGMO_ACCEPT_TRUNCATED_MSG;

	// set the match options to none so that the message id and correl id fields in the MQMD will be ignored
	gmo.MatchOptions = MQMO_NONE;

	// make sure that the match options are honored
	gmo.Version = MQGMO_VERSION_2;

	while (MQCC_OK == cc)
	{
		// Try to get a message
		XMQGet(qm, q, &mqmd, &gmo, 0, defaultMQbuffer, &msgLen, &cc, &rc);

		// check for a 2024 (too many uncommitted messages)
		if (MQRC_SYNCPOINT_LIMIT_REACHED == rc)
		{
			// try to commit the current UOW and then reissue the request
			commitUOW(true);

			// try the get again
			XMQGet(qm, q, &mqmd, &gmo, 0, NULL, &msgLen, &cc, &rc);
		}

		// check for truncated message accepted - this is expected since we do not read the data
		if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_ACCEPTED == rc))
		{
			// change to a normal completion since we are discarding the message
			cc = MQCC_OK;
		}

		// did the get work?
		if (MQCC_OK == cc)
		{
			// count the number of messages we have purged
			msgCount++;
			uowCount++;

			// see if we need to do a commit - do not exceed 2000 msgs in a single UOW no matter what
			if ((uowCount == maxUOW - 1) || (uowCount > 2000))
			{
				// issue a commit and reset the UOW counter
				commitUOW(true);
				uowCount = 0;
			}

			// check if we should update the number of messsages remaining in the queue
			if ((msgCount % 100) == 0)
			{
				getCurrentDepth();
			}
		}
		else
		{
			// get the completion code and reason code
			if (rc != MQRC_NO_MSG_AVAILABLE)
			{
				setErrorMsg(cc, rc, "Get");
			}
			else
			{
				m_q_depth = 0;
			}
		}

		// clear the message id and correlation id fields to avoid spurious no message found reason codes
		memset(mqmd.MsgId, 0, MQ_MSG_ID_LENGTH);
		memset(mqmd.CorrelId, 0, MQ_CORREL_ID_LENGTH);
	}

	// skip this step if we ran into a log full error
	if (rc != MQRC_BACKED_OUT)
	{
		// issue a syncpoint to commit what we did
		commitUOW(true);
	}

	// tell the user what we did
	if (MQRC_NO_MSG_AVAILABLE == rc)
	{
		if (msgCount > 0)
		{
			sprintf(errtxt, "%d messages purged from %s", msgCount, Queue);
		}
		else
		{
			sprintf(errtxt, "Queue empty - no messages purged from %s", Queue);
		}
	}
	else
	{
		if (MQRC_BACKED_OUT == rc)
		{
			sprintf(errtxt, "Commit failed (rc=2003) - the MQ log may be too small");
		}
		else
		{
			sprintf(errtxt, "%d messages purged from %s before error rc=%d cc=%d", msgCount, Queue, cc, rc);
		}
	}

	// append the message
	appendError(errtxt);

	// get the current depth of the queue
	getCurrentDepth();

	// close the queue after a purge
	closeQ(Q_CLOSE_NONE);

	// call the audit routine to record the MQGET
	sprintf(msgtxt, "PurgeQ purged %d msgs", msgCount);
	app = (CRfhutilApp *)AfxGetApp();
	app->createAuditRecord((LPCTSTR)currentQM, (LPCTSTR)currentQ, msgtxt, rc);

	if (traceEnabled)
	{
		// trace results
		logTraceEntry(errtxt);

		// trace exit from purgeQueue
		logTraceEntry("Exiting DataArea::purgeQueue()");
	}
}

int DataArea::getQueueDepth(LPCTSTR QMname, LPCTSTR Queue)

{
	// define the MQ objects that we need
	CString			remoteQM;
	char	traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getQueueDepth() QMname=%s Queue=%s", QMname, Queue);

		// trace entry to getQueueDepth
		logTraceEntry(traceInfo);
	}

	remoteQM.Empty();

	// make sure we have a queue name
	if (strlen(Queue) == 0)
	{
		m_error_msg = "*Queue Name required* ";

		return -1;
	}

	// make sure we are connected to the correct QM
	if (!checkConnection(QMname))
	{
		return -1;
	}

	// check if we already have the right queue open or
	// try to open the queue if necessary
	if (!openQ(Queue, remoteQM, Q_OPEN_READ, FALSE))
	{
		// didn't work
		return -1;
	}

	// get the current depth of the queue
	getCurrentDepth();

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getQueueDepth() m_q_depth=%d", m_q_depth);

		// trace exit from getQueueDepth
		logTraceEntry(traceInfo);
	}

	return m_q_depth;
}

void DataArea::updateMsgText()

{
	int		len;
	time_t	ltime;			/* number of seconds since 1/1/70     */
	struct	tm *today;		/* today's date as a structure        */
	char	todaysDate[32];
	char	msg[8192];

	if (m_error_msg.GetLength() > 0)
	{
		// capture the next message
		// get a time stamp
		memset(todaysDate, 0, sizeof(todaysDate));
		time(&ltime);
		today = localtime(&ltime);
		strftime(todaysDate, sizeof(todaysDate) - 1, "%H.%M.%S", today);

		// indicate date and time the message was created
		sprintf(msg, "%s %s\r\n%s", todaysDate, (LPCTSTR)m_error_msg, (LPCTSTR)m_msg_text);

		// limit to 4096 bytes
		len = strlen(msg);
		if (len > MAX_ERR_MSG_LEN)
		{
			len = MAX_ERR_MSG_LEN;
			msg[len] = 0;
			while ((len > 0) && (msg[len - 1] != '\n') && (msg[len - 1] != '\r'))
			{
				msg[len] = 0;
				len--;
			}
		}

		while ((len > 0) && (('\r' == msg[len - 1]) || ('\n' == msg[len - 1])))
		{
			len--;
			msg[len] = 0;
		}

		m_msg_text = msg;

		// capture the information in the trace file
		if (traceEnabled)
		{
			logTraceEntry((LPCTSTR)m_error_msg);
		}

		// we have processed this message
		m_error_msg.Empty();
	}
}

void DataArea::setQueueType(int queuetype)

{
	switch (queuetype)
	{
	case MQQT_LOCAL:
		{
			m_queue_type = "Local";
			break;
		}
	case MQQT_MODEL:
		{
			m_queue_type = "Model";
			break;
		}
	case MQQT_ALIAS:
		{
			m_queue_type = "Alias";
			break;
		}
	case MQQT_REMOTE:
		{
			m_queue_type = "Remote";
			break;
		}
	case MQQT_CLUSTER:
		{
			m_queue_type = "Cluster";
			break;
		}
	}
}

void DataArea::printData(CEdit *cedit)

{
PRINTDLG pd;
char	buffer[256] = { 0 };

	memset (&pd, '\0', sizeof (PRINTDLG));
	pd.lStructSize = sizeof (PRINTDLG);

	HANDLE hPrinter;
	OpenPrinter (szPrinterName, &hPrinter, NULL);

//
//	Get a pointer to the DEVMODE structure for the
//	current printer.
//
	DWORD dwBytesReturned, dwBytesNeeded;
	GetPrinter(hPrinter, 2, NULL, 0, &dwBytesNeeded);
	PRINTER_INFO_2* pInfo2 = (PRINTER_INFO_2*)GlobalAlloc(GPTR, dwBytesNeeded);
	if (pInfo2 == NULL) {
		return;
	}
	if (GetPrinter(hPrinter, 2, (LPBYTE)pInfo2, dwBytesNeeded,
	   &dwBytesReturned) == 0)
	{
	   GlobalFree(pInfo2);
	   ClosePrinter(hPrinter);
	   return;
	}

//
//	Use the DEVMODE to initialize the dialog box.
//
	HANDLE phDevMode;
	DEVMODE *pTempDM;

	phDevMode = GlobalAlloc(GPTR, sizeof(DEVMODE));
	if (phDevMode == NULL)
		return;

	pTempDM = (DEVMODE *)GlobalLock(phDevMode);
	if (pTempDM == NULL)
		return;

	memcpy(pTempDM, pInfo2->pDevMode, sizeof(DEVMODE));
	pd.hDevMode = phDevMode;
	if (!PrintDlg (&pd))
	{
		GlobalFree (phDevMode);
		return;
	}

	DEVMODE* pDevMode = (DEVMODE*)GlobalLock(pd.hDevMode);
	if (pDevMode) {
		strcpy(buffer, (char *)pDevMode->dmDeviceName);
		GlobalUnlock(pd.hDevMode);
	}

	pd.hDC = CreateDC (NULL, buffer, NULL, NULL);

	HBRUSH hBrush = CreateSolidBrush (RGB(0xc0, 0xc0, 0xc0));
	HBRUSH hOldBrush = (HBRUSH) SelectObject (pd.hDC, hBrush);

	DOCINFO di = {sizeof (DOCINFO), _T("Test"), NULL, NULL, 0};

	pd.nFromPage = 1;
	pd.nMaxPage = -1;

	StartDoc (pd.hDC, &di);

	while (PrintPage (cedit, pd) == false)
		++pd.nFromPage;

	EndDoc (pd.hDC);

	GlobalFree (pd.hDevMode);
	GlobalFree (pd.hDevNames);
	SelectObject (pd.hDC, hOldBrush);
	DeleteObject (hBrush);

	DeleteDC (pd.hDC);
}

bool DataArea::PrintPage(CEdit *cedit, PRINTDLG &pd)

{
	char	buffer[256];

	memset (buffer, '\0', sizeof (buffer));
	DEVMODE* pDevMode = (DEVMODE*)GlobalLock(pd.hDevMode);

	int nMarginX = GetDeviceCaps (pd.hDC, PHYSICALOFFSETX);
	int nMarginY = GetDeviceCaps (pd.hDC, PHYSICALOFFSETY);
	int nPageX = GetDeviceCaps (pd.hDC, PHYSICALWIDTH);
	int nPageY = GetDeviceCaps (pd.hDC, PHYSICALHEIGHT);
	int nPageMaxX = nPageX - 2 * nMarginX;
	int nPageMaxY = nPageY - 2 * nMarginY;
	SaveDC (pd.hDC);
	StartPage (pd.hDC);

	Rectangle (pd.hDC, nMarginX, nMarginY, nMarginX + 600, nMarginY + 600);
	Rectangle (pd.hDC, nPageMaxX - 600, nPageMaxY - 600, nPageMaxX, nPageMaxY);

	EndPage (pd.hDC);
	RestoreDC (pd.hDC, -1);
	return (true);
}

///////////////////////////////////////////
//
// The following routine will look at the
// beginning of a file and take a guess if
// the file looks like an EBCDIC or ASCII
// file.
//
///////////////////////////////////////////

void DataArea::checkForEBCDIC()

{
	MQMDPAGE		*mqmdObj=(MQMDPAGE *)mqmdData;
	int				maxChars;
	int				aChars=0;
	int				eChars=0;
	int				eSpaces=0;
	int				aSpaces=0;
	unsigned char	*ptr;

	maxChars = fileSize;
	if (maxChars > 1024)
	{
		maxChars = 1024;
	}

	// Do not guess on very small files
	if (maxChars < 64)
	{
		return;
	}

	ptr = fileData;
	while (maxChars > 0)
	{
		// check for an EBCDIC blank
		if (64 == ptr[0])
		{
			eSpaces++;
		}
		else
		{
			if (32 == ptr[0])
			{
				aSpaces++;
			}

			if ((ptr[0] >= 'A') && (ptr[0] <= 'z'))
			{
				aChars++;
			}

			if ((ptr[0] >= '0') && (ptr[0] <= '9'))
			{
				aChars++;
			}

			if ((ptr[0] >= 240) && (ptr[0] <= 250))
			{
				eChars++;
			}

			if ((ptr[0] >= 193) && (ptr[0] <= 233))
			{
				eChars++;
			}

			if ((ptr[0] >= 129) && (ptr[0] <= 169))
			{
				eChars++;
			}
		}

		ptr++;
		maxChars--;
	}

	// check if we think this is EBCDIC
	if ((eChars + eSpaces) > (aChars + aSpaces + 32))
	{
		// seems like an EBCDIC file - therefore, set the EBCDIC display options
		m_char_format = CHAR_EBCDIC;
		m_numeric_format = NUMERIC_HOST;
		m_pd_numeric_format = NUMERIC_HOST;
		m_float_numeric_format = FLOAT_390;
		mqmdObj->setCcsid(DEF_EBCDIC_CCSID);
		mqmdObj->setEncoding(NUMERIC_HOST, NUMERIC_HOST, FLOAT_390);
	}
}


void DataArea::changeUnixFile()

{
	unsigned char *		newArea;
	unsigned int		i;
	unsigned int		j;

	if (fileSize > 0)
	{
		newArea = (unsigned char *)rfhMalloc(fileSize * 2 + 2, "UNIXDATA");

		i = 0;
		j = 0;
		while (i < fileSize)
		{
			// check for a \r\n combination
			if (('\r' == fileData[i]) && ('\n' == fileData[i + 1]))
			{
				// copy both characters
				newArea[j++] = fileData[i++];
				newArea[j++] = fileData[i++];
			}
			else
			{
				// check for just a \n by itself
				if ('\n' == fileData[i])
				{
					// insert a carriage return
					newArea[j++] = '\r';
				}

				// copy the data character
				newArea[j++] = fileData[i++];
			}
		}

		// terminate the string
		newArea[j] = 0;
		newArea[j+1] = 0;

		// change the file data area to use the new area and free the old one
		rfhFree (fileData);
		fileData = newArea;
		fileSize = j;
	}
}

////////////////////////////////////////////////////////
//
// Routine called from other classes to perform the
// clear data function.  Called from the General and
// view classes.
//
////////////////////////////////////////////////////////

void DataArea::clearFileData()

{
	// get rid of the current file data
	resetDataArea();

	if (m_copybook != NULL)
	{
		m_copybook->resetCopybook();
	}
}

///////////////////////////////////////////////////////////
//
// Routine to connect to the queue manager.  This routine
// will create a queue manager object if it does not exist.
// An existing connection will be used if the request is
// for the same queue manager.
//
// If the connection is successful then boolean true will
// be returned otherwise false is returned.
//
// When a successful connection is made the name of the
// queue manager and other connection information is
// saved in the registry.  This information is used to
// initialize the corresponding fields when RFHUtil is
// started.
//
// The queue manager name should either be a valid queue
// manager name or in the case of the client version can
// be in the format of an MQSERVER variable.  This format
// is channel name, transport type and connection name
// separated by slash characters.  If the name contains
// a slash it is assumed to be in MQSERVER format.
// Otherwise it is assumed to be the name of a queue
// manager.
//
///////////////////////////////////////////////////////////

bool DataArea::connect2QM(LPCTSTR QMname)

{
	MQLONG		cc;
	MQLONG		rc;
	MQLONG		rc1=MQRC_NONE;
	MQLONG		rc2=MQRC_NONE;
	MQLONG		rc3=MQRC_NONE;
	MQLONG		rc4=MQRC_NONE;
	MQLONG		rc5=MQRC_NONE;
	MQLONG		rc6=MQRC_NONE;
	MQHOBJ		hObj;					// object handle
	MQLONG		Selector;				// selector for inquiry
	MQLONG		ccsid=0;				// default CCSID of the QMgr
	int			slen;					// work variable for lengths of strings
	int			clientVersion;			// client version
	CRfhutilApp * app=NULL;				// pointer to main application object
#ifdef MQCLIENT
	int			transType=MQXPT_TCP;	// transport type from connection string
	char		*serverPtr;
	char		*ptr;
	char		*qmPtr;
	char		*connName;				// pointer to the connection name part of the string
	char		mqserver[1024];
#endif
	MQSCO		sco={MQSCO_DEFAULT};
	MQCD		cd={MQCD_CLIENT_CONN_DEFAULT};
	MQCSP		csp={MQCSP_DEFAULT};
	MQCNO		cno={MQCNO_DEFAULT};
	MQOD		od = {MQOD_DEFAULT};	// Object Descriptor
	char		traceInfo[2048];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::connect2QM() QMname=%s", QMname);

		// trace entry to connect2QM
		logTraceEntry(traceInfo);
	}

	// check if the entry point was found during initialization
	if (NULL == XMQConnX)
	{
		if (traceEnabled)
		{
			// log the error in the trace
			logTraceEntry("MQ entry point not resolved");
		}

		return false;
	}

	// is there a connection to the correct QM already?
	if ((qm != NULL) && connected && (strcmp(QMname, (LPCTSTR)currentQM) == 0))
	{
		// already connected - just return
		return true;
	}

	// is there a connection to a different QM?
	if (connected)
	{
		// disconnect from the current queue manager
		discQM();
	}

	// now build a connection options object (MQCNO)
	// use version 2 for better backwards compatibility, although this is set to V4 if SSL connection is used
	cno.Version = MQCNO_VERSION_2;

	// allow the connection handle to be shared between threads
	cno.Options = MQCNO_HANDLE_SHARE_NO_BLOCK;

	// point to the channel definition
	cno.ClientConnPtr = &cd;

	// now build the channel definition
	// set version 4 for better backwards compatibility, although this is set to V7 if SSL connection is used
	cd.Version = MQCD_VERSION_4;
	cd.StrucLength = MQCD_LENGTH_4;

	// set the maximum message length
	cd.MaxMsgLength = 104857600;

	// check if the client version is V6 or above
	// do not try to use SSL if the client is V5.3 or less
	clientVersion = ((CRfhutilApp *)AfxGetApp())->MQServerVersion;

	// was a different user id and/or password specified?
	slen = m_conn_userid.GetLength();
	if (slen > 0)
	{
		// Allow an override to put passwords in the CD structure if really necessary, but using CSP
		// is the default (no longer a dialog option)
		if (!getenv("RFHUTIL_PASSWORD_CD") && ((clientVersion >= 6) || (0 == clientVersion)))
		{
			// use an MQCSP
			// this will require that the client and the queue manager be at least V6
			// set the length and address of the user id
			csp.CSPUserIdLength = slen;
			csp.CSPUserIdPtr = (void *)((LPCTSTR)m_conn_userid);

			// get the length of the password
			csp.CSPPasswordLength = m_conn_password.GetLength();

			// set the password pointer as well
			csp.CSPPasswordPtr = (void *)((LPCTSTR)m_conn_password);

			// set the authentication type to user id and password
			csp.AuthenticationType = MQCSP_AUTH_USER_ID_AND_PWD;

			// set the pointer to the MQCSP area in the MQCNO
			cno.SecurityParmsPtr = &csp;

			// set the CNO version level so the MQCSP pointer is used
			if (cno.Version < MQCNO_VERSION_5) {
				cno.Version = MQCNO_VERSION_5;
			}
		}
		else
		{
			// make sure the length does not exceed the maximum allowed
			if (slen > sizeof(cd.UserIdentifier))
			{
				slen = sizeof(cd.UserIdentifier);
			}

			// use the specified user id
			memcpy(cd.UserIdentifier, (LPCTSTR)m_conn_userid, slen);

			// get the length of the password
			slen = m_conn_password.GetLength();

			// was the password specified as well?
			if (slen > 0)
			{
				// make sure the length does not exceed the maximum allowed
				if (slen > sizeof(cd.Password))
				{
					slen = sizeof(cd.Password);
				}

				// use the specified password
				memcpy(cd.Password, (LPCTSTR)m_conn_password, slen);
			}
		}

		if (traceEnabled)
		{
			// create the trace line - do not trace passwords. But indicate if one were set.
			sprintf(traceInfo, "DataArea::connect2QM() using m_conn_userid=%s password set=%s clientVersion=%d", (LPCTSTR)m_conn_userid, (m_conn_password.GetLength() > 0) ? "Y":"N", clientVersion);

			// log the data to the trace file
			logTraceEntry(traceInfo);
		}
	}

	// was a security exit specified?
	slen = m_security_exit.GetLength();
	if (slen > 0)
	{
		// make sure the length does not exceed the maximum allowed
		if (slen > sizeof(cd.SecurityExit))
		{
			slen = sizeof(cd.SecurityExit);
		}

		// use the specified security exit
		memcpy(cd.SecurityExit, (LPCTSTR)m_security_exit, slen);

		// get the length of the user data
		slen = m_security_data.GetLength();

		// was the user data specified as well?
		if (slen > 0)
		{
			// make sure the length does not exceed the maximum allowed
			if (slen > sizeof(cd.SecurityUserData))
			{
				slen = sizeof(cd.SecurityUserData);
			}

			// use the specified user data
			memcpy(cd.SecurityUserData, (LPCTSTR)m_security_data, slen);
		}

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "DataArea::connect2QM() using m_security_exit=%.128s m_security_data=%.32s", (LPCTSTR)m_security_exit, (LPCTSTR)m_security_data);

			// log the data to the trace file
			logTraceEntry(traceInfo);
		}
	}

#ifdef MQCLIENT
	// check if the client is V6 or later
	// also cover the case where the client version cannot be determined
	if ((clientVersion >= 6) || (0 == clientVersion))
	{
		// check if SSL is required and we have a cipher spec
		if (m_use_ssl && (m_ssl_cipher[0] != 0))
		{
			// make sure the cipher spec is not longer than the maximum allowed
			slen = m_ssl_cipher.GetLength();
			if (slen >= sizeof(cd.SSLCipherSpec))
			{
				slen = sizeof(cd.SSLCipherSpec) - 1;
			}

			// get the cipher spec
			memset(cd.SSLCipherSpec, 0, sizeof(cd.SSLCipherSpec));
			memcpy(cd.SSLCipherSpec, (LPCTSTR)m_ssl_cipher, slen);

			// check if client authorization is required
			if (m_ssl_validate)
			{
				// client authorization required
				cd.SSLClientAuth = MQSCA_REQUIRED;
			}
			else
			{
				// make client authorization optional
				cd.SSLClientAuth = MQSCA_OPTIONAL;
			}

			// get the length of the key ring
			// check if a location for the key respository was specified
			if ((m_ssl_keyr.GetLength() > 0) && (m_ssl_keyr.GetLength() < sizeof(sco.KeyRepository) - 1))
			{
				// set the location of the key respository
				strcpy(sco.KeyRepository, (LPCTSTR)m_ssl_keyr);
			}

			// set the ssl reset count in the sco
			sco.KeyResetCount = m_ssl_reset_count;

			// set the pointer to the mqsco control block
			cno.SSLConfigPtr = &sco;

			// must use version 7 or later for SSL
			if (cd.Version < MQCD_VERSION_7) {
				cd.Version = MQCD_VERSION_7;
				cd.StrucLength = MQCD_LENGTH_7;
			}

			// use version 4 so that the SSL information is picked up
			if (cno.Version < MQCNO_VERSION_4) {
				cno.Version = MQCNO_VERSION_4;
			}

			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, "DataArea::connect2QM() m_ssl_cipher=%s m_ssl_validate=%d m_ssl_keyr=%s m_ssl_reset_count=%d", (LPCTSTR)m_ssl_cipher, m_ssl_validate, (LPCTSTR)m_ssl_keyr, m_ssl_reset_count);

				// log the data to the trace file
				logTraceEntry(traceInfo);
			}
		}
	}

	// make a local copy of the QM name
	// RHFUtil supports two types of QM names
	// for server connections the QM name must be in the form of a name
	// for client connections the QM name can be in the format of an MQSERVER variable
	mqserver[0] = 0;

	// make sure the source string is not too long
	if (strlen(QMname) < sizeof(mqserver) - 1)
	{
		strcpy(mqserver, QMname);
	}

	// check for a channel table entry
	serverPtr = strchr(mqserver, '[');

	// channel table entry?
	if (serverPtr != NULL)
	{
		// terminate the QM name
		serverPtr[-1] = 0;

		// skip the square bracket
		serverPtr++;

		// search for the end bracket
		ptr = strchr(serverPtr, ']');
		if (ptr != NULL)
		{
			// get rid of the ending bracket
			ptr[0] = 0;
		}

		// point to the queue manager name
		qmPtr = (char *)&mqserver;
	}
	else
	{
		serverPtr = (char *)&mqserver;
	}

	// check if the name is in the form of an MQSERVER variable
	ptr = strchr(serverPtr, '/');

	// was a slash found?
	if (NULL == ptr)
	{
		// only have a queue manager name so point to it
		qmPtr = (char *)&mqserver;
	}
	else
	{
		// no queue manager name specified
		qmPtr = "";

		// break the name into 3 parts (channel name, transport type and connection name)
		ptr[0] = 0;
		ptr++;

		// look for the second slash
		connName = strchr(ptr, '/');
		if (connName != NULL)
		{
			connName[0] = 0;
			connName++;
		}

		// get the transport type
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

		memset(cd.ChannelName, ' ', sizeof(cd.ChannelName));
		if (strlen(mqserver) <= sizeof(cd.ChannelName))
		{
			// set the channel name
			memcpy(&cd.ChannelName, serverPtr, strlen(serverPtr));
		}

		//memset(mqcd.ConnectionName, ' ', sizeof(mqcd.ConnectionName));
		if ((connName != NULL) && (strlen(connName) <= sizeof(cd.ConnectionName)))
		{
			// set the connection name
			memcpy(cd.ConnectionName, connName, strlen(connName));
		}

		// set the transport type
		cd.TransportType = transType;

		// was a local address specified?
		slen = m_local_address.GetLength();
		if (slen > 0)
		{
			if (slen > sizeof(cd.LocalAddress))
			{
				slen = sizeof(cd.LocalAddress);
			}

			// use the specified local address
			memcpy(cd.LocalAddress, (LPCTSTR)m_local_address, slen);

			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, "DataArea::connect2QM() using m_local_address=%s", (LPCTSTR)m_local_address);

				// log the data to the trace file
				logTraceEntry(traceInfo);
			}
		}
	}

	// check for a verbose trace
	if (traceEnabled && verboseTrace)
	{
		// dump out the MQ data areas that are being used
		// dump out the CNO
		dumpTraceData("MQCNO", (unsigned char *)&cno, sizeof(MQCNO));

		// dump out the CD
		dumpTraceData("MQCD", (unsigned char *)&cd, sizeof(MQCD));

		// dump out the SCO
		dumpTraceData("MQSCO", (unsigned char *)&sco, sizeof(MQSCO));

		if (m_conn_userid.GetLength() > 0)
		{
			// dump out the MQCSP
			dumpTraceData("MQCSP", (unsigned char *)&csp, sizeof(MQCSP));

			// dump out the user id
			dumpTraceData("Userid", (unsigned char *)((LPCTSTR)m_conn_userid), m_conn_userid.GetLength());

			// Do not trace passwords
			
		}
	}


	// try to connect to the queue manager
	XMQConnX(qmPtr, &cno, &qm, &cc, &rc);

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "DataArea::connect2QM() qmPtr=%s mqserver=%s serverPtr=%s transType=%d m_use_ssl=%d m_ssl_cipher=%s", qmPtr, mqserver, serverPtr, transType, m_use_ssl, (LPCTSTR)m_ssl_cipher);

		// trace entry to connect2QM
		logTraceEntry(traceInfo);

		// create the trace line
		sprintf(traceInfo, "MQCONNX cc=%d, rc=%d to Connection=%s, channel=%s, transport=%d", cc, rc, connName, mqserver, cd.TransportType);

		// trace MQCONNX request
		logTraceEntry(traceInfo);
	}
#else
	// Using server bindings
		// check for a verbose trace
		if (traceEnabled && verboseTrace)
		{
			// dump out the MQ data areas that are being used
			// dump out the CNO
			dumpTraceData("MQCNO", (unsigned char *)&cno, sizeof(MQCNO));

			// dump out the CD
			dumpTraceData("MQCD", (unsigned char *)&cd, sizeof(MQCD));

			// dump out the SCO
			dumpTraceData("MQSCO", (unsigned char *)&sco, sizeof(MQSCO));

			if (m_conn_userid.GetLength() > 0)
			{
				// dump out the MQCSP
				dumpTraceData("MQCSP", (unsigned char *)&csp, sizeof(MQCSP));

				// dump out the user id
				dumpTraceData("Userid", (unsigned char *)((LPCTSTR)m_conn_userid), m_conn_userid.GetLength());

				// Do not trace passwords
				
			}
		}

	// perform a normal connection to the queue manager
	XMQConnX((char *)QMname, &cno, &qm, &cc, &rc);
#endif

	// check if the connection worked
	if (cc != MQCC_OK)
	{
		// check if this is an attempt to find a default QM
		// the default QM may not exist on this system so ignore the first attempt
		if ((strlen(QMname) > 0) || (1 == firstError))
		{
			// error, get the completion and reason codes
			setErrorMsg(cc, rc, "Connect");
		}

		// only ignore the first attempt
		firstError = 1;

		// indicate that there is no connection
		qm = NULL;
		connected = false;

		// return false
		return false;
	}

	currentQM = QMname;
	connected = true;

	// remember the last QM that we connected to as well as the user id and password that were used
	app = (CRfhutilApp *)AfxGetApp();
	app->initQMname = QMname;
	app->initConnUser = m_conn_userid;
	app->initConnPW = m_conn_password;

	// remember the security exit name and data that were used
	app->initSecExit = m_security_exit;
	app->initSecData = m_security_data;

#ifdef MQCLIENT
	// remember the SSL parameters that were used
	app->initUseSSL = m_use_ssl;
	app->initSSLCipherSpec = m_ssl_cipher;
	app->initSSLKeyR = m_ssl_keyr;
	app->initSSLResetCount = m_ssl_reset_count;

	// remember the local address that was used
	app->initLocalAddress = m_local_address;

	// remember the latest queue manager that was accessed
	app->saveClientQM((LPCTSTR)QMname);
#endif

	// do some inquires against the QMgr that will be used later
	// the following QMgr parameters are queried
	//
	//  queue manager name
	//  maximum message size
	//  maximum number of messages in a UOW
	//  platform type
	//  mq version (command level)
	//  character set
	//
	// open a QMgr object to get the characteristics of the QM
	// set the object type we are looking for
	od.ObjectType = MQOT_Q_MGR;			// open the queue manager object
	XMQOpen( qm,							// connection handle to queue manager
			 &od,						// object descriptor for queue
			 MQOO_INQUIRE +				// open it for inquire
			 MQOO_FAIL_IF_QUIESCING,		// but not if MQM stopping
			 &hObj,						// object handle
			 &cc,						// MQOPEN completion code
			 &rc);						// reason code

	// did the open work?
	if (MQCC_OK == cc)
	{
		// set the selector for max message length
		Selector = MQIA_MAX_MSG_LENGTH;

		// do an inquiry for the maximum message length
		XMQInq(qm,						// connection handle to queue manager
			   hObj,						// object handle for q manager
			   1,						// inquire only one selector
			   &Selector,				// the selector to inquire
			   1,						// number of integer attributes needed
			   &maxMsgLenQM,				// pointer to the result
			   0,						// inquiring on integer value
			   NULL,						// no character results
			   &cc,						// completion code
			   &rc1);						// reason code

		// set the selector to get platform type (MVS or not)
		Selector = MQIA_PLATFORM;
		platform = 0;					// just in case it doesn't work

		// do an inquiry for the Queue Manager platform type
		XMQInq(qm,						// connection handle to queue manager
			   hObj,						// object handle for q manager
			   1,						// inquire only one selector
			   &Selector,				// the selector to inquire
			   1,						// number of integer attributes needed
			   &platform,				// pointer to the result
			   0,						// inquiring on integer value
			   NULL,						// no character results
			   &cc,						// completion code
			   &rc2);						// reason code

		// set the selector for level
		Selector = MQIA_COMMAND_LEVEL;
		level = 0;						// just in case it doesn't work

		// do an inquiry for the Queue Manager level
		XMQInq(qm,						// connection handle to queue manager
			   hObj,						// object handle for q manager
			   1,						// inquire only one selector
			   &Selector,				// the selector to inquire
			   1,						// number of integer attributes needed
			   &level,					// pointer to the result
			   0,						// inquiring on integer value
			   NULL,						// no character results
			   &cc,						// completion code
			   &rc3);						// reason code

		// set the selector for max uncommitted messages
		Selector = MQIA_MAX_UNCOMMITTED_MSGS;

		// do an inquiry for the maximum number of messages in a unit of work
		XMQInq(qm,						// connection handle to queue manager
			   hObj,						// object handle for q manager
			   1,						// inquire only one selector
			   &Selector,				// the selector to inquire
			   1,						// number of integer attributes needed
			   &maxUOW,					// pointer to the result
			   0,						// inquiring on integer value
			   NULL,						// no character results
			   &cc,						// completion code
			   &rc4);						// reason code

		// set the selector for CCSID of the queue manager
		Selector = MQIA_CODED_CHAR_SET_ID;

		// do an inquiry for the CCSDI of the queue manager
		XMQInq(qm,						// connection handle to queue manager
			   hObj,						// object handle for q manager
			   1,						// inquire only one selector
			   &Selector,				// the selector to inquire
			   1,						// number of integer attributes needed
			   &ccsid,					// pointer to the result
			   0,						// inquiring on integer value
			   NULL,						// no character results
			   &cc,						// completion code
			   &rc5);						// reason code

		// was the inq successful?
		if (MQCC_OK == cc)
		{
			// capture the ccsid of the queue manager
			MQCcsid = ccsid;
		}

		// do an inquiry for the queue manager name
		Selector = MQCA_Q_MGR_NAME;

		// clear the real queue manager name of a client queue manager
		memset(QueueManagerRealName, 0, sizeof(QueueManagerRealName));

		XMQInq(qm,						// connection handle to queue manager
			   hObj,						// object handle for q manager
			   1,						// inquire only one selector
			   &Selector,				// the selector to inquire
			   0,						// no integer attributes needed
			   NULL,						// pointer to the integer result
			   sizeof(QueueManagerRealName),			// inquiring on character value
			   QueueManagerRealName,					// pointer to the character results
			   &cc,						// completion code
			   &rc6);						// reason code

		// did it work?
		if (MQCC_OK == rc6)
		{
			// trim the queue manager name
			Rtrim(QueueManagerRealName);
		}

		// close the queue handle
		XMQClose(qm, &hObj, MQCO_NONE, &cc, &rc);

		if (traceEnabled)
		{
			// did any of the inquiries fail?
			if ((rc1 != 0) || (rc2 != 0) || (rc3 != 0) || (rc4 != 0) || (rc5 != 0) || (rc6 != 0))
			{
				// build the trace line
				sprintf(traceInfo, "rc1=%d rc2=%d rc3=%d rc4=%d rc5=%d rc6=%d", rc1, rc2, rc3, rc4, rc5, rc6);

				// trace the type of queue manager
				logTraceEntry(traceInfo);
			}

			// build the trace line
			sprintf(traceInfo, "platform=%d level=%d maxMsgLenQM=%d maxUOW=%d charset=%d QueueManagerRealName=%s", platform, level, maxMsgLenQM, maxUOW, ccsid, QueueManagerRealName);

			// trace the type of queue manager
			logTraceEntry(traceInfo);
		}
	}

	if (traceEnabled)
	{
		// trace exit from connect2QM
		sprintf(traceInfo, "Exiting DataArea::connect2QM() returned true cc=%d QMname=%s initQMname=%s", cc, QMname, (LPCTSTR)app->initQMname);

		// trace entry to connect2QM
		logTraceEntry(traceInfo);
	}

	return true;
}

///////////////////////////////////////////
//
// Routine to indicate if there is a
// current connection to the queue manager.
// This routine is called externally by
// other classes to indicate which menu
// options or buttons should be enabled.
//
///////////////////////////////////////////

BOOL DataArea::isConnectionActive()

{
	// return the current connection status
	return connected;
}

/////////////////////////////////////////////
//
// Routine to disconnect from the current
// queue manager.  If a queue is open then
// the queue is closed before disconnecting.
//
/////////////////////////////////////////////

void DataArea::discQM()

{
	MQLONG		cc=MQCC_OK;
	MQLONG		rc=MQRC_NONE;
	char		traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::discQM() connected=%d Qopen=%d currentQM=%s",  connected, Qopen, (LPCTSTR)currentQM);

		// trace entry to discQM
		logTraceEntry(traceInfo);
	}

	// disconnect from the current queue manager
	if (qm != NULL)
	{
		// check if we have any queues open
		if (q != NULL)
		{
			// close the open queue before we try to disconnect
			closeQ(Q_CLOSE_NONE);
		}

		if (connected)
		{
			// attempt to disconnect from the current queue manager
			XMQDisc(&qm, &cc, &rc);

			// check if it worked
			if (cc != MQCC_OK)
			{
				// error reported, get the completion and reason codes
				setErrorMsg(cc, rc, "Disc");
			}
		}
	}

	// remember the connection is no longer active
	connected = false;
	qm = NULL;

	// reset the browse indicators
	browseActive = 0;
	browsePrevActive = 0;

	// get rid of the queue manager name that the connection is made to
	currentQM.Empty();

	// reset the unit of work indicator
	unitOfWorkActive = false;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::discQM() cc=%d rc=%d initQMname=%s", cc, rc, (LPCTSTR)((CRfhutilApp *)AfxGetApp())->initQMname);

		// trace exit from discQM
		logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////////
//
// This routine is called externally from
// the view class when the MQDISC menu item
// is selected.  It calls the disconnect
// routine and then issues a message that
// is displayed in the message area.
//
/////////////////////////////////////////////

void DataArea::explicitDiscQM()

{
	// disconnect from the queue manager
	discQM();

	// add a line of text to the messages edit box
	appendError("Disconnected from queue manager");
}

///////////////////////////////////////////////
//
// This routine is called externally from
// the view class when the MQCONN menu item
// is selected.  It calls the connect
// routine and if the connection is successful
// issues a message that is displayed in the
// message area.
//
///////////////////////////////////////////////

bool DataArea::explicitConnect(LPCTSTR qmName)

{
	char	errtxt[128];
	bool	result=false;

	// issue the connect
	result = connect2QM(qmName);

	// check if the connection worked
	if (isConnectionActive())
	{
#ifdef MQCLIENT
		// update the messages to indicate a successful connection
		// did the inquire for the QMgr name work?
		if (QueueManagerRealName[0] != 0)
		{
			sprintf(errtxt, "Connected to %s", QueueManagerRealName);
		}
		else
		{
			// update the messages to indicate a successful connection
			sprintf(errtxt, "Connected to %s", qmName);
		}
#else
		// update the messages to indicate a successful connection
		sprintf(errtxt, "Connected to %s", qmName);
#endif
		appendError(errtxt);
	}

	// indicate if it was successful
	return result;
}

bool DataArea::openQ(LPCTSTR Queue, LPCTSTR RemoteQM, int openType, BOOL passAll)

{
	bool		result=false;
	MQLONG		cc = MQCC_OK;
	MQLONG		rc;
	MQLONG		openOpt=0;
	MQLONG		cc2=MQCC_OK;							// MQ completion code from MQINQ
	MQLONG		rc2=0;									// MQ reason code from MQINQ
	MQLONG		Select[1];								// attribute selectors for MQINQ
	MQLONG		IAV[1];									// integer attribute values for MQINQ
	int			queueType;								// type of queue (local, alias, etc)
	int			tempOpt;
	int			slen;									// work variable for string lengths
	MQOD		od={MQOD_DEFAULT};						// MQ open descriptor
	char		trace1[16];								// work area for trace info
	char		xmitQ[MQ_Q_MGR_NAME_LENGTH + 8];
	char		baseName[MQ_Q_MGR_NAME_LENGTH + 8];
	char		traceInfo[768];							// work variable to build trace message
	char		resObjName[MQ_TOPIC_STR_LENGTH];		// resolved object string

	// initialize the transmission queue name
	xmitQ[0] = 0;
	memset(resObjName, 0, sizeof(resObjName));			// initialize the string so trace does not pick up garbage

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::openQ() Queue %s RemoteQM %s openType=%d passALL=%d m_setUserID=%d m_set_all=%d m_user_id=%.256s", Queue, RemoteQM, openType, passAll, m_setUserID, m_set_all, (LPCTSTR)m_user_id);

		// trace entry to openQ
		logTraceEntry(traceInfo);
	}

	// check if we have a queue name
	if (0 == Queue[0])
	{
		m_error_msg = "*Queue Name required*";
		return false;
	}

	// check if we already have the right queue open
	if ((strcmp(Queue, (LPCTSTR)currentQ) == 0) &&
		(strcmp(RemoteQM, (LPCTSTR)currentRemoteQM) == 0) &&
		(strcmp(currentFilter, (LPCTSTR)m_filter) == 0) &&
		(openType == Qopen) &&
		(m_set_all == m_save_set_all_setting) &&
		(m_setUserID == m_save_set_user_setting))
	{
		// check if we are setting the user id, and if so if it is still the same
		if (m_setUserID || m_set_all)
		{
			// check if the user id changed
			if (currentUserid.Compare(m_user_id) == 0)
			{
				// queue is already open with the right user id
				return true;
			}
		}
		else
		{
			// the correct queue is open with the right options
			return true;
		}
	}

	// is a queue open?
	if (q != NULL)
	{
		// close the current queue
		closeQ(Q_CLOSE_NONE);
	}

	// get the length of the queue name
	slen = strlen(Queue);
	if (slen > MQ_Q_NAME_LENGTH)
	{
		// make sure we don't copy too many characters
		slen = MQ_Q_NAME_LENGTH;
	}

	// set the name of the queue
	memset(od.ObjectName, 0, sizeof(od.ObjectName));
	memcpy(od.ObjectName, Queue, slen);

	// set the open options for the queue
	// Note that the extra options are necessary to prevent the queue from being closed too
	// quickly.  The extra options (INPUT_SHARED and INQUIRE) result in
	// the queue remaining open.

	if (Q_OPEN_WRITE == openType)
	{
		// put operation
		// check if this is a remote queue that we are explicitly addressing
		slen = strlen(RemoteQM);
		if (slen > 0)
		{
			// make sure the name is not too long
			if (slen > MQ_Q_MGR_NAME_LENGTH)
			{
				// limit to the maximum number of characters
				slen = MQ_Q_MGR_NAME_LENGTH;
			}

			// set the name of the remote QM where the queue resides
			memcpy(od.ObjectQMgrName, RemoteQM, slen);
		}

		// set the open options for the queue
		// Note that the extra options are necessary to prevent the queue from being closed too
		// quickly.  If the queue was a dynamic queue, it could be lost due to the automatic
		// closing of the C++ classes.  The extra options (INPUT_SHARED and INQUIRE) result in
		// the queue remaining open.

		openOpt = MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING | MQOO_INQUIRE;
		switch (m_bind_option)
		{
		case BIND_ON_OPEN:
			{
				openOpt |= MQOO_BIND_ON_OPEN;
				break;
			}
		case BIND_NOT_FIXED:
			{
				openOpt |= MQOO_BIND_NOT_FIXED;
				break;
			}
		case BIND_GROUP:
			{
				openOpt |= MQOO_BIND_ON_GROUP;
				break;
			}
		default:
			{
				break;
			}
		}

		// check if pass all has been requested
		if (passAll)
		{
			// set the open option
			openOpt |= MQOO_PASS_ALL_CONTEXT;
		}

		// was set all requested?
		if (m_set_all)
		{
			openOpt |= MQOO_SET_ALL_CONTEXT;
		}
		else
		{
			// check if we want to use an alternate user id
			// the alternate user id must be specified in the MQMD user id field
			// after the set user id check box is selected
			if (m_setUserID)
			{
				openOpt |= MQOO_SET_IDENTITY_CONTEXT;
			}
		}

		// was an alternate user id specified?
		if (m_alt_userid)
		{
			// set the user id specified on the MQMD page
			openOpt |= MQOO_ALTERNATE_USER_AUTHORITY;

			// get the length of the user id to use
			slen = m_user_id.GetLength();

			// Is it too long?
			if (slen > MQ_USER_ID_LENGTH)
			{
				// truncate to 12 characters
				slen = MQ_USER_ID_LENGTH;
			}

			// copy the user id into the object descriptor
			memset(od.AlternateUserId, 0, MQ_USER_ID_LENGTH);
			memcpy(od.AlternateUserId, (LPCTSTR)m_user_id, slen);
		}

		if (traceEnabled)		// trace the open options
		{
			// build the trace line
			memset(trace1, 0, sizeof(trace1));
			tempOpt = reverseBytes4(openOpt);
			AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace1);
			sprintf(traceInfo, "Opening queue using openOptions X\'%s\'", trace1);

			// write a trace log entry
			logTraceEntry(traceInfo);
		}

		// try to open the queue
		XMQOpen(qm, &od, openOpt, &q, &cc, &rc);

		// check for a not authorized
		// this is see if the problem is that the user is not authorized for inquiry operations
		// but is otherwise allowed to open the queue
		if (cc != MQCC_OK)
		{
			// if this is a remote queue, it should create a 2045 reason code
			// if the problem is not authorized we should retry without inquire
			if ((MQCC_FAILED == cc) && ((MQRC_OPTION_NOT_VALID_FOR_TYPE == rc) || (MQRC_NOT_AUTHORIZED == rc)))
			{
				// turn off the inquire option
				openOpt ^= MQOO_INQUIRE;

				// retry the open
				XMQOpen(qm, &od, openOpt, &q, &cc, &rc);
			}
		}

		// check if the queue open worked
		if (cc != MQCC_OK)
		{
			// open failed, generate message and return
			setErrorMsg(cc, rc, "Open");
		}
		else
		{
			result = true;
			currentRemoteQM = RemoteQM;

			// is there a transmission queue name?
			if (od.ResolvedQMgrName[0] != 0)
			{
				// get the name of the transmit queue
				memset(xmitQ, 0, sizeof(xmitQ));
				memcpy(xmitQ, od.ResolvedQMgrName, MQ_Q_MGR_NAME_LENGTH);
				Rtrim(xmitQ);
				transmissionQueue = xmitQ;

				// cature the resolved queue name
				memcpy(resObjName, od.ResolvedQName, MQ_Q_NAME_LENGTH);
			}

			// save the current set all and set user id settings
			m_save_set_all_setting = m_set_all;
			m_save_set_user_setting = m_setUserID;
		}
	}
	else
	{
		if ((Q_OPEN_READ == openType) || (Q_OPEN_BROWSE == openType) || (Q_OPEN_READ_BROWSE == openType))
		{
			if (Q_OPEN_READ == openType)
			{
				// get operation
				openOpt =  MQOO_FAIL_IF_QUIESCING | MQOO_INQUIRE | MQOO_INPUT_SHARED;
			}
			else if (Q_OPEN_BROWSE == openType)
			{
				// browse operation
				openOpt = MQOO_FAIL_IF_QUIESCING | MQOO_INQUIRE | MQOO_BROWSE;
			}
			else if (Q_OPEN_READ_BROWSE == openType)
			{
				// browse with get operation
				openOpt = MQOO_FAIL_IF_QUIESCING | MQOO_INQUIRE | MQOO_BROWSE| MQOO_INPUT_SHARED;
			}

			if (passAll)
			{
				openOpt |= MQOO_SAVE_ALL_CONTEXT;
			}

			slen = m_filter.GetLength();
			if (slen > 0)
			{
				// using a selector
				// set the version of the object descriptor to V4
				od.Version = MQOD_VERSION_4;

				// make sure the filter fits in the allocated memory
				if (slen > sizeof(currentFilter) - 1)
				{
					// too big - don't overwrite memory
					slen = sizeof(currentFilter) - 1;
				}

				// copy the filter to the currentFilter field
				memcpy(currentFilter, (LPCTSTR)m_filter, slen);
				currentFilter[slen] = 0;

				// set the selection string pointer
				od.SelectionString.VSPtr = (void *)&currentFilter;		// point to the user data
				od.SelectionString.VSLength = slen;						// length of string
				od.SelectionString.VSBufSize = 0;						// input field - no output required

				// the following should not be necessary but V7 returns a 2520 error if this field is not set
				// set up the resolved object string field
				od.ResObjectString.VSPtr = &resObjName;					// pointer to buffer
				od.ResObjectString.VSBufSize = sizeof(resObjName);		// maximum buffer size

				if (traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " Filter slen=%d VSPtr=%8.8X VSOffset=%d VSLength=%d VSBufSize=%d &currentFilter=%8.8X currentFilter (%.256s)", slen, (unsigned int)od.SelectionString.VSPtr, od.SelectionString.VSOffset, od.SelectionString.VSLength, od.SelectionString.VSBufSize, (unsigned int)&currentFilter, currentFilter);

					// trace filter parameters
					logTraceEntry(traceInfo);
				}
			}
			else
			{
				// set the version of the object descriptor to V3
				od.Version = MQOD_VERSION_3;
				currentFilter[0] = 0;
			}

			// check if we want to use an alternate user id
			// the alternate user id must be specified in the MQMD user id field
			// after the set user id check box is selected
			if (m_alt_userid)
			{
				// set the user id specified on the MQMD page
				openOpt |= MQOO_ALTERNATE_USER_AUTHORITY;

				// get the length of the user id to use
				slen = m_user_id.GetLength();

				// Is it too long?
				if (slen > MQ_USER_ID_LENGTH)
				{
					// truncate to 12 characters
					slen = MQ_USER_ID_LENGTH;
				}

				// copy the user id into the object descriptor
				memset(od.AlternateUserId, 0, MQ_USER_ID_LENGTH);
				memcpy(od.AlternateUserId, (LPCTSTR)m_user_id, slen);
			}

			if (traceEnabled)
			{
				// build the trace line
				memset(trace1, 0, sizeof(trace1));
				tempOpt = reverseBytes4(openOpt);
				AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace1);
				sprintf(traceInfo, "Opening queue using openOptions X\'%s\'", trace1);

				// write a trace log entry
				logTraceEntry(traceInfo);
			}

			// try to open the queue
			XMQOpen(qm, &od, openOpt, &q, &cc, &rc);

			// check for a not authorized
			// this is see if the problem is that the user is not authorized for inquiry operations
			// but is otherwise allowed to open the queue
			if (cc != MQCC_OK)
			{
				// if this is a remote queue, it should create a 2045 reason code
				// if the problem is not authorized we should retry without inquire
				if ((MQCC_FAILED == cc) && ((MQRC_OPTION_NOT_VALID_FOR_TYPE == rc) || (MQRC_NOT_AUTHORIZED == rc)))
				{
					// turn off the inquire option
					openOpt ^= MQOO_INQUIRE;

					// retry the open
					XMQOpen(qm, &od, openOpt, &q, &cc, &rc);
				}
			}

			if (MQCC_OK == cc)
			{
				result = true;
				currentRemoteQM.Empty();
			}
			else
			{
				// open failed, generate message and return
				// get the completion code and reason code
				setErrorMsg(cc, rc, "Open");
			}
		}
	}

	m_queue_type.Empty();

	if (result)
	{
		currentQ = Queue;
		Qopen = openType;
		if (m_alt_userid)
		{
			currentUserid = m_user_id;
		}
		else
		{
			currentUserid.Empty();
		}

		// remember the last queue that we opened
		((CRfhutilApp *)AfxGetApp())->initQname = Queue;
		((CRfhutilApp *)AfxGetApp())->initRemoteQMname = RemoteQM;

		// get the queue type
		queueType = getQueueType(q);
		setQueueType(queueType);

		while ((MQQT_ALIAS == queueType) && 0)
		{
			// close the alias queue
			XMQClose(qm, &q, MQCO_NONE, &cc, &rc);

			// get the base name and base queue name
			memset(baseName, 0, sizeof(baseName));
			memcpy(baseName, od.ObjectName, MQ_Q_NAME_LENGTH);
			memset(xmitQ, 0, sizeof(xmitQ));
			memcpy(xmitQ, od.ObjectQMgrName, MQ_Q_MGR_NAME_LENGTH);

			// remove any trailing blanks
			Rtrim(baseName);
			Rtrim(xmitQ);

			// update the resolved queue name and queue manager name
			memcpy(od.ObjectName, od.ResolvedQName, MQ_Q_NAME_LENGTH);
			memcpy(od.ObjectQMgrName, od.ResolvedQMgrName, MQ_Q_MGR_NAME_LENGTH);

			if (traceEnabled)
			{
				// create a trace entry
				sprintf(traceInfo, "Opening base of alias queue %s qMgr %s", baseName, xmitQ);

				// write the trace info to the trace file
				logTraceEntry(traceInfo);
			}

			// try to open the base queue
			XMQOpen(qm, &od, openOpt, &q, &cc, &rc);

			// check if it worked
			if (cc != MQCC_OK)
			{
				// open failed, generate message and return
				// get the completion code and reason code
				m_error_msg += "Error opening base queue ";
				m_error_msg += baseName;
				m_error_msg += "\r\n";
				setErrorMsg(cc, rc, "OpenBase");
				updateMsgText();

				currentQ.Empty();
				currentRemoteQM.Empty();
				Qopen = Q_OPEN_NOT;
				return false;
			}
			else
			{
				queueType = getQueueType(q);
			}
		}

		// now try to inquire about the maximum message length for this queue
		// set the selector to request the maximum message length
		Select[0] = MQIA_MAX_MSG_LENGTH;

		// try to get the queue depth
		XMQInq(qm, q, 1L, Select, 1L, IAV, 0L, NULL, &cc2, &rc2);

		// check if it worked
		if (MQCC_OK == cc)
		{
			// set the queue depth variable
			maxMsgLenQ = IAV[0];
		}
		else
		{
			// failed - set the depth to zero
			maxMsgLenQ = -1;
		}
	}
	else
	{
		// open failed - clear this field
		setQueueType(0);
	}

	if (MQCC_OK == cc)
	{
		// capture the resolved queue name
		Rtrim(resObjName);
		m_resolvedQname = resObjName;

		// check if the queue name resolved to a different name
		if ((m_resolvedQname.GetLength() > 0) && (m_resolvedQname.Compare(Queue) != 0))
		{
			// tell the user what the real name of the queue is
			m_error_msg.Format("Queue name resolved to %.48s", (LPCTSTR)m_resolvedQname);
			updateMsgText();
		}
	}
	else
	{
		m_resolvedQname.Empty();
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::openQ() Qopen=%d openOpt=%d result=%d queueType=%d od.Version=%d xmitQ=%.48s ResolvedType=%d resObjName=%.256s", Qopen, openOpt, result, queueType, od.Version, xmitQ, od.ResolvedType, resObjName);

		// trace exit from browseNext
		logTraceEntry(traceInfo);
	}

	return result;
}

bool DataArea::explicitOpen(LPCTSTR Queue, LPCTSTR RemoteQM, int openType)

{
	char	errtxt[192];
	bool	result=false;

	// issue the connect
	result = openQ(Queue, RemoteQM, openType, FALSE);

	// check if the connection worked
	if (result && isConnectionActive())
	{
		// update the messages to indicate a successful connection
		if (0 == RemoteQM[0])
		{
			sprintf(errtxt, "Opened queue %s", Queue);
		}
		else
		{
			sprintf(errtxt, "Opened queue %s on %s", Queue, RemoteQM);

			if (transmissionQueue.GetLength() > 0)
			{
				if (!transmissionQueue.Compare(RemoteQM))
				{
					strcat(errtxt, " xmitQ=");
					strcat(errtxt, (LPCTSTR)transmissionQueue);
				}
			}
		}

		switch (openType)
		{
		case Q_OPEN_WRITE:
			{
				strcat(errtxt, " for put");
				break;
			}
		case Q_OPEN_READ:
			{
				strcat(errtxt, " for read");
				break;
			}
		case Q_OPEN_BROWSE:
			{
				strcat(errtxt, " for browse");
				break;
			}
		case Q_OPEN_READ_BROWSE:
			{
				strcat(errtxt, " for read/browse");
				break;
			}
		}

		appendError(errtxt);
	}

	return result;
}

/////////////////////////////////////////////////
//
// Routine to close a queue
//
//  Close options can be passed in
//
//  0 = MQCO_NONE
//  1 = MQCO_DELETE
//  2 = MQCO_DELETE_PURGE
//
/////////////////////////////////////////////////

void DataArea::closeQ(MQLONG options)

{
	MQLONG		cc = MQCC_OK;
	MQLONG		rc = MQRC_NONE;
	MQLONG		closeOptions=MQCO_NONE;
	char		traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::closeQ() Qopen=%d options=%d m_close_option=%d", Qopen, options, m_close_option);

		// trace entry to closeQ
		logTraceEntry(traceInfo);
	}

	if (q != NULL)
	{
		if (Qopen != Q_OPEN_NOT)
		{
			// is there an active unit of work?
			if (unitOfWorkActive)
			{
				// end any units of work
				rollbackUOW();
			}

			// set the appropriate options
			if (Q_CLOSE_DELETE == options)
			{
				// set delete option
				closeOptions = MQCO_DELETE;
			}
			else if (Q_CLOSE_PURGE == options)
			{
				// set delete and purge option
				closeOptions = MQCO_DELETE_PURGE;
			}

			// close the queue
			XMQClose(qm, &q, closeOptions, &cc, &rc);

			// check if it worked
			if (cc != MQCC_OK)
			{
				// close failed, get the reason and generate an error message
				setErrorMsg(cc, rc, "Close");
			}
		}
	}

	// clear all queue related fields
	q = NULL;							// queue handle
	Qopen = Q_OPEN_NOT;					// open type
	m_save_set_all_setting = FALSE;
	m_save_set_user_setting = FALSE;
	currentQ.Empty();					// name of the queue
	currentRemoteQM.Empty();			// name of remote QM where queue resides
	transmissionQueue.Empty();			// name of the transmission queue
	currentUserid.Empty();				// user id that queue was opened under

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::closeQ() cc=%d rc=%d closeOptions=%d", cc, rc, closeOptions);

		// trace exit from closeQ
		logTraceEntry(traceInfo);
	}
}

void DataArea::explicitCloseQ()

{
	closeQ(m_close_option);
	appendError("Queue closed");
}

BOOL DataArea::isQueueOpen()

{
	if (Q_OPEN_NOT == Qopen)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

bool DataArea::checkConnection(LPCTSTR QMname)

{
	if ((qm != NULL) && connected && (strcmp(QMname, currentQM) != 0))
	{
		// close a queue if one is open
		closeQ(Q_CLOSE_NONE);

		// disconnect from this QM
		discQM();
	}

	// check if we need to connect to the QM
	if ((NULL == qm) || !connected)
	{
		// connect to the correct QM
		if (!connect2QM(QMname))
		{
			// unable to connect to QM
			return false;
		}
	}

	return true;
}

void DataArea::beginUOW(bool silent)

{
	MQLONG		cc;
	MQLONG		rc;
	MQBO		bo={MQBO_DEFAULT};

	// make sure we have a queue manager connection
	if ((qm != NULL) && connected)
	{
		// issue the MQBEGIN
		//MQBEGIN(qm, &bo, &cc, &rc);
		XMQBegin(qm, &bo, &cc, &rc);

		// check if it worked
		if (MQCC_OK == cc)
		{
			unitOfWorkActive = true;

			// check if we are supposed to tell what we did
			if (!silent)
			{
				// tell the user what we did
				m_error_msg += "Unit of work created\r\n";
			}
		}
		else
		{
			setErrorMsg(cc, rc, "begin");
		}
	}
}

void DataArea::commitUOW(bool silent)

{
	MQLONG		cc=MQCC_OK;
	MQLONG		rc=MQRC_NONE;
	char		traceInfo[128];				// work variable to build trace message

	// make sure we have a queue manager connection
	if ((qm != NULL) && connected)
	{
		// issue the MQCMIT
		//MQCMIT(qm, &cc, &rc);
		XMQCmit(qm, &cc, &rc);

		// check if it worked
		if (MQCC_OK == cc)
		{
			unitOfWorkActive = false;

			// check if we are supposed to tell what we did
			if (!silent)
			{
				// tell what we did
				appendError("Unit of work committed");
			}
		}
		else
		{
			setErrorMsg(cc, rc, "commit");
			discQM();
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::commitUOW() connected=%d silent=%d unitOfWorkActive=%d cc=%d rc=%d", connected, silent, unitOfWorkActive, cc, rc);

		// trace exit from commitUOW
		logTraceEntry(traceInfo);
	}
}

void DataArea::rollbackUOW()

{
	// roll back the unit of work
	MQLONG		cc;
	MQLONG		rc;

	// make sure we have a queue manager connection
	if ((qm != NULL) && connected)
	{
		// issue the MQBACK
		XMQBack(qm, &cc, &rc);

		// check if it worked
		if (MQCC_OK == cc)
		{
			unitOfWorkActive = false;

			// tell the user what we did
			appendError("Unit of work rolled back");
		}
		else
		{
			setErrorMsg(cc, rc, "rollback");
			discQM();
		}
	}
}

void DataArea::setWindowTitle(const char * queue)

{
	// change the windows title to the latest queue name
	CWnd * window = AfxGetApp()->GetMainWnd();

	if (window != NULL)
	{
		window->SetWindowText((LPCTSTR) queue);
	}
}

///////////////////////////////////////////////////////////////
//
// This routine returns the line number of the line
// with the desired string
//
///////////////////////////////////////////////////////////////

int DataArea::findNext(unsigned char * value, int len, int upDown)

{
	BOOL			notFound=TRUE;
	int				maxOffset;
	unsigned char	transValue[256];
	char			traceInfo[128];				// work variable to build trace message

	// clear the value area
	memset(transValue, 0, sizeof(transValue));

	// make sure the input is not too long
	if (len >= sizeof(transValue))
	{
		// only use 255 characters
		len = sizeof(transValue) - 1;
	}

	// do we need to translate the find data?
	if (CHAR_EBCDIC == m_char_format)
	{
		// yes, translate the find data
		AsciiToEbcdic(value, len, (unsigned char *)transValue);
	}
	else
	{
		// no, just make a copy
		memcpy(transValue, value, len);
	}

	if (SEARCH_UP == upDown)
	{
		if (-1 == findStartOffset)
		{
			// no previous operation, so start from the end of the data
			findStartOffset = fileSize - 1 - len;
		}
		else
		{
			findStartOffset--;
		}

		// see if we can find the value in the current data area
		while ((notFound) && (findStartOffset >= 0))
		{
			while ((findStartOffset >= 0) && (fileData[findStartOffset] != transValue[0]))
			{
				findStartOffset--;
			}

			if (findStartOffset >= 0)
			{
				if (len > 1)
				{
					if (memcmp(fileData + findStartOffset, transValue, len) == 0)
					{
						notFound = FALSE;
					}
					else
					{
						findStartOffset--;
					}
				}
				else
				{
					// only searching for a single character
					notFound = FALSE;
				}
			}
		}
	}
	else
	{
		// calculate the maximum offset into the data to search
		maxOffset = fileSize - len;

		// move to the next character
		findStartOffset++;

		// see if we can find the value in the current data area
		while ((notFound) && (findStartOffset < maxOffset))
		{
			while ((findStartOffset < maxOffset) && (fileData[findStartOffset] != transValue[0]))
			{
				findStartOffset++;
			}

			if (findStartOffset < maxOffset)
			{
				if (len > 1)
				{
					if (memcmp(fileData + findStartOffset, transValue, len) == 0)
					{
						notFound = FALSE;
					}
					else
					{
						findStartOffset++;
					}
				}
				else
				{
					// only searching for a single character
					notFound = FALSE;
				}
			}
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::findNext() upDown=%d findStartOffset=%d value=%.32s", upDown, findStartOffset, value);

		// trace exit from findNext
		logTraceEntry(traceInfo);
	}

	if (notFound)
	{
		// reset the search to the beginning of the data
		resetFind();
		return -1;
	}
	else
	{
		// figure out what line number this item is located on
		return findStartOffset;
	}
}

///////////////////////////////////////////////////////////////
//
// This routine resets the search location to the top of the file
//
///////////////////////////////////////////////////////////////

void DataArea::resetFind()

{
	// reset the current find location to zero
	findStartOffset = -1;
}

int DataArea::getLineNumber(int ofs, int dataFormat, int crlf, int edi, int BOM)

{
	int					line=0;
	int					maxChar = 0;
	unsigned char *		ptr;
	unsigned char *		endptr;

	// temporary placeholder
	switch (dataFormat)
	{
	case DATA_CHARACTER:
		{
			if (0 == crlf)
			{
				if (0 == edi)
				{
					line = ofs / 32;
				}
				else
				{
					line = 0;
				}
			}
			else
			{
				// count the number of lines until we reach the offset
				ptr = fileData;
				endptr = ptr + ofs;
				while (ptr < endptr)
				{
					if ('\n' == ptr[0])
					{
						maxChar = 0;
						line++;
					}
					else
					{
						maxChar++;
					}

					ptr++;
				}
			}

			break;
		}

	case DATA_HEX:
		{
			line = ofs / 16;
			break;
		}

	case DATA_BOTH:
		{
			line = ofs / 16;
			break;
		}

	case DATA_XML:
		{
			break;
		}

	case DATA_PARSED:
		{
			break;
		}

	case DATA_COBOL:
		{
			break;
		}
	}

	return line;
}

int DataArea::getLineOffset(int ofs, int dataFormat, int crlf, int edi, int BOM)

{
	int offset=0;

	switch (dataFormat)
	{
	case DATA_CHARACTER:
		{
			if ((0 == crlf) && (0 == edi))
			{
				offset = (ofs % 32) + 9;
			}

			break;
		}
	case DATA_HEX:
		{
			offset = (ofs % 16) << 1;
			if (offset < 8)
			{
				offset += 9;
			}
			else
			{
				if (offset < 16)
				{
					offset += 10;
				}
				else
				{
					if (offset < 24)
					{
						offset += 11;
					}
					else
					{
						offset += 12;
					}
				}
			}

			break;
		}
	case DATA_BOTH:
		{
			offset = ofs % 16;
			if (offset < 8)
			{
				offset += 9;
			}
			else
			{
				offset += 10;
			}

			break;
		}
	}

	return offset;
}

// this routine will search the XML formatted data
// for a particular data item and will return the
// line number where it finds the next occurence

int DataArea::getXMLLineNumber(LPCTSTR value, int dir)

{
	BOOL				notFound=TRUE;
	int					len;
	unsigned char *		ptr;
	unsigned char *		endptr;
	char				traceInfo[192];			// work variable to build trace message

	if (m_data_xml != NULL)
	{
		len = strlen(value);

		// check which direction we are moving in
		if (SEARCH_DOWN == dir)
		{
			findXMLOffset++;
			ptr = m_data_xml + findXMLOffset;
			endptr = ptr  + strlen((char *)ptr) - len;

			// search forwards
			// see if we can find the value in the current data area
			while ((notFound) && (ptr < endptr))
			{
				while ((ptr <= endptr) && (ptr[0] != value[0]))
				{
					if ('\n' == ptr[0])
					{
						findXMLline++;
						findXMLofs = 0;
					}
					else
					{
						findXMLofs++;
					}

					ptr++;
					findXMLOffset++;
				}

				if (ptr <= endptr)
				{
					if (len > 1)
					{
						if (memcmp(ptr, value, len) == 0)
						{
							notFound = FALSE;
						}
						else
						{
							ptr++;
							findXMLOffset++;
							findXMLofs++;
						}
					}
					else
					{
						// only searching for a single character
						notFound = FALSE;
					}
				}
			}
		}
		else
		{
			if (-1 == findXMLOffset)
			{
				findXMLOffset = strlen((char *)m_data_xml) - len;
			}
			else
			{
				findXMLOffset--;
			}

			ptr = m_data_xml + findXMLOffset;
			endptr = m_data_xml;

			// search backwards
			while ((notFound) && (ptr >= m_data_xml))
			{
				while ((ptr >= m_data_xml) && (ptr[0] != value[0]))
				{
					ptr--;
					findXMLOffset--;
				}

				if (ptr >= m_data_xml)
				{
					if (len > 1)
					{
						if (memcmp(ptr, value, len) == 0)
						{
							notFound = FALSE;
						}
						else
						{
							ptr--;
							findXMLOffset--;
						}
					}
					else
					{
						// only searching for a single character
						notFound = FALSE;
					}
				}
			}

			// now calculate the number of lines from the beginning to the current location
			findXMLline = 0;
			findXMLofs = 0;

			endptr = ptr;
			ptr = m_data_xml;
			while (ptr <= endptr)
			{
				if ('\n' == ptr[0])
				{
					findXMLline++;
					findXMLofs = 0;
				}
				else
				{
					findXMLofs++;
				}

				ptr++;
			}

			findXMLofs--;
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getXMLLineNumber() dir=%d notFound=%d findXMLOffset=%d findXMLline=%d findXMLofs=%d value=%.32s", dir, notFound, findXMLOffset, findXMLline, findXMLofs, value);

		// trace exit from getXMLLineNumber
		logTraceEntry(traceInfo);
	}

	if (notFound)
	{
		// reset the search to the beginning of the data
		resetXMLfind();
	}

	return findXMLline;
}

// this routine will search the FIX formatted data
// for a particular data item and will return the
// line number where it finds the next occurence

int DataArea::getFixLineNumber(LPCTSTR value, int dir)

{
	BOOL				notFound=TRUE;
	int					len;
	unsigned char *		ptr;
	unsigned char *		endptr;
	char				traceInfo[192];			// work variable to build trace message

	if (m_data_fix != NULL)
	{
		len = strlen(value);

		// check which direction we are moving in
		if (SEARCH_DOWN == dir)
		{
			findFixOffset++;
			ptr = m_data_fix + findFixOffset;
			endptr = ptr  + strlen((char *)ptr) - len;

			// search forwards
			// see if we can find the value in the current data area
			while ((notFound) && (ptr < endptr))
			{
				while ((ptr <= endptr) && (ptr[0] != value[0]))
				{
					if ('\n' == ptr[0])
					{
						findFixLine++;
						findFixOfs = 0;
					}
					else
					{
						findFixOfs++;
					}

					ptr++;
					findFixOffset++;
				}

				if (ptr <= endptr)
				{
					if (len > 1)
					{
						if (memcmp(ptr, value, len) == 0)
						{
							notFound = FALSE;
						}
						else
						{
							ptr++;
							findFixOffset++;
							findFixOfs++;
						}
					}
					else
					{
						// only searching for a single character
						notFound = FALSE;
					}
				}
			}
		}
		else
		{
			if (-1 == findFixOffset)
			{
				findFixOffset = strlen((char *)m_data_fix) - len;
			}
			else
			{
				findFixOffset--;
			}

			ptr = m_data_fix + findFixOffset;
			endptr = m_data_fix;

			// search backwards
			while ((notFound) && (ptr >= m_data_fix))
			{
				while ((ptr >= m_data_fix) && (ptr[0] != value[0]))
				{
					ptr--;
					findFixOffset--;
				}

				if (ptr >= m_data_fix)
				{
					if (len > 1)
					{
						if (memcmp(ptr, value, len) == 0)
						{
							notFound = FALSE;
						}
						else
						{
							ptr--;
							findFixOffset--;
						}
					}
					else
					{
						// only searching for a single character
						notFound = FALSE;
					}
				}
			}

			// now calculate the number of lines from the beginning to the current location
			findFixLine = 0;
			findFixOfs = 0;

			endptr = ptr;
			ptr = m_data_fix;
			while (ptr <= endptr)
			{
				if ('\n' == ptr[0])
				{
					findFixLine++;
					findFixOfs = 0;
				}
				else
				{
					findFixOfs++;
				}

				ptr++;
			}

			findFixOfs--;
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getFixLineNumber() dir=%d notFound=%d findFixOffset=%d findFixLine=%d findFixOfs=%d value=%.32s", dir, notFound, findFixOffset, findFixLine, findFixOfs, value);

		// trace exit from getFixLineNumber
		logTraceEntry(traceInfo);
	}

	if (notFound)
	{
		// reset the search to the beginning of the data
		resetFixFind();
	}

	return findFixLine;
}

// this routine will search the JSON formatted data
// for a particular data item and will return the
// line number where it finds the next occurence

int DataArea::getJsonLineNumber(LPCTSTR value, int dir)

{
	BOOL				notFound=TRUE;
	int					len;
	unsigned char *		ptr;
	unsigned char *		endptr;
	char				traceInfo[192];			// work variable to build trace message

	if (m_data_json != NULL)
	{
		len = strlen(value);

		// check which direction we are moving in
		if (SEARCH_DOWN == dir)
		{
			findJsonOffset++;
			ptr = m_data_json + findJsonOffset;
			endptr = ptr  + strlen((char *)ptr) - len;

			// search forwards
			// see if we can find the value in the current data area
			while ((notFound) && (ptr < endptr))
			{
				while ((ptr <= endptr) && (ptr[0] != value[0]))
				{
					if ('\n' == ptr[0])
					{
						findJsonLine++;
						findJsonOfs = 0;
					}
					else
					{
						findJsonOfs++;
					}

					ptr++;
					findJsonOffset++;
				}

				if (ptr <= endptr)
				{
					if (len > 1)
					{
						if (memcmp(ptr, value, len) == 0)
						{
							notFound = FALSE;
						}
						else
						{
							ptr++;
							findJsonOffset++;
							findJsonOfs++;
						}
					}
					else
					{
						// only searching for a single character
						notFound = FALSE;
					}
				}
			}
		}
		else
		{
			if (-1 == findJsonOffset)
			{
				findJsonOffset = strlen((char *)m_data_json) - len;
			}
			else
			{
				findJsonOffset--;
			}

			ptr = m_data_json + findJsonOffset;
			endptr = m_data_json;

			// search backwards
			while ((notFound) && (ptr >= m_data_json))
			{
				while ((ptr >= m_data_json) && (ptr[0] != value[0]))
				{
					ptr--;
					findJsonOffset--;
				}

				if (ptr >= m_data_json)
				{
					if (len > 1)
					{
						if (memcmp(ptr, value, len) == 0)
						{
							notFound = FALSE;
						}
						else
						{
							ptr--;
							findJsonOffset--;
						}
					}
					else
					{
						// only searching for a single character
						notFound = FALSE;
					}
				}
			}

			// now calculate the number of lines from the beginning to the current location
			findJsonLine = 0;
			findJsonOfs = 0;

			endptr = ptr;
			ptr = m_data_json;
			while (ptr <= endptr)
			{
				if ('\n' == ptr[0])
				{
					findJsonLine++;
					findJsonOfs = 0;
				}
				else
				{
					findJsonOfs++;
				}

				ptr++;
			}

			findJsonOfs--;
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getJsonLineNumber() dir=%d notFound=%d findJsonOffset=%d findJsonLine=%d findJsonOfs=%d value=%.32s", dir, notFound, findJsonOffset, findJsonLine, findJsonOfs, value);

		// trace exit from getJsonLineNumber
		logTraceEntry(traceInfo);
	}

	if (notFound)
	{
		// reset the search to the beginning of the data
		resetJsonFind();
	}

	return findJsonLine;
}

void DataArea::resetXMLfind()

{
	findXMLOffset = -1;
	findXMLline = 0;
	findXMLofs = 0;
}

void DataArea::resetFixFind()

{
	findFixOffset = -1;
	findFixLine = 0;
	findFixOfs = 0;
}

void DataArea::resetJsonFind()

{
	findJsonOffset = -1;
	findJsonLine = 0;
	findJsonOfs = 0;
}

int DataArea::findNextHex(unsigned char *value, int length, int upDown)
{
	BOOL	notFound=TRUE;
	int		maxOffset;

	maxOffset = fileSize - length;

	// move to the next character
	findHexOffset++;

	// see if we can find the value in the current data area
	while ((notFound) && (findHexOffset < maxOffset))
	{
		while ((findHexOffset < maxOffset) && (fileData[findHexOffset] != value[0]))
		{
			findHexOffset++;
		}

		if (findHexOffset < maxOffset)
		{
			if (length > 1)
			{
				if (memcmp(fileData + findHexOffset, value, length) == 0)
				{
					notFound = FALSE;
				}
				else
				{
					findHexOffset++;
				}
			}
			else
			{
				// only searching for a single character
				notFound = FALSE;
			}
		}
	}

	if (notFound)
	{
		// reset the search to the beginning of the data
		resetHexFind();
		return -1;
	}
	else
	{
		// figure out what line number this item is located on
		return findHexOffset;
	}
}

void DataArea::resetHexFind()

{
	findHexOffset = -1;
}

int DataArea::getParsedLineNumber(LPCTSTR value, int dir)

{
	BOOL	notFound=TRUE;
	int		len;
	unsigned char *		ptr;
	unsigned char *		endptr;
	char				traceInfo[192];			// work variable to build trace message

	if (m_data_parsed != NULL)
	{
		len = strlen(value);

		// check which direction we are moving in
		if (SEARCH_DOWN == dir)
		{
			findParsedOffset++;
			ptr = m_data_parsed + findParsedOffset;
			endptr = ptr  + strlen((char *)ptr) - len;

			// search forwards
			// see if we can find the value in the current data area
			while ((notFound) && (ptr < endptr))
			{
				while ((ptr <= endptr) && (ptr[0] != value[0]))
				{
					if ('\n' == ptr[0])
					{
						findParsedLine++;
						findParsedOfs = 0;
					}
					else
					{
						findParsedOfs++;
					}

					ptr++;
					findParsedOffset++;
				}

				if (ptr <= endptr)
				{
					if (len > 1)
					{
						if (memcmp(ptr, value, len) == 0)
						{
							notFound = FALSE;
						}
						else
						{
							ptr++;
							findParsedOffset++;
							findParsedOfs++;
						}
					}
					else
					{
						// only searching for a single character
						notFound = FALSE;
					}
				}
			}
		}
		else
		{
			if (-1 == findParsedOffset)
			{
				findParsedOffset = strlen((char *)m_data_parsed) - len;
			}
			else
			{
				findParsedOffset--;
			}

			ptr = m_data_parsed + findParsedOffset;

			// search backwards
			while ((notFound) && (ptr >= m_data_parsed))
			{
				while ((ptr >= m_data_parsed) && (ptr[0] != value[0]))
				{
					ptr--;
					findParsedOffset--;
				}

				if (ptr >= m_data_parsed)
				{
					if (len > 1)
					{
						if (memcmp(ptr, value, len) == 0)
						{
							notFound = FALSE;
						}
						else
						{
							ptr--;
							findParsedOffset--;
						}
					}
					else
					{
						// only searching for a single character
						notFound = FALSE;
					}
				}
			}

			// now calculate the number of lines from the beginning to the current location
			findParsedLine = 0;
			findParsedOfs = 0;

			endptr = ptr;
			ptr = m_data_parsed;
			while (ptr <= endptr)
			{
				if ('\n' == ptr[0])
				{
					findParsedLine++;
					findParsedOfs = 0;
				}
				else
				{
					findParsedOfs++;
				}

				ptr++;
			}

			findParsedOfs--;
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getParsedLineNumber() dir=%d notFound=%d findParsedOffset=%d findParsedLine=%d findParsedOfs=%d value=%.32s", dir, notFound, findParsedOffset, findParsedLine, findParsedOfs, value);

		// trace exit from getParsedLineNumber
		logTraceEntry(traceInfo);
	}

	if (notFound)
	{
		// reset the search to the beginning of the data
		resetParsedFind();
	}

	return findParsedLine;
}

void DataArea::resetParsedFind()

{
	findParsedLine = 0;
	findParsedOfs = 0;
	findParsedOffset = -1;
}

int DataArea::getXMLofs()

{
	return findXMLofs;
}

int DataArea::getParsedOfs()

{
	return findParsedOfs;
}

int DataArea::getFixOfs()

{
	return findFixOfs;
}

int DataArea::getJsonOfs()

{
	return findJsonOfs;
}

//////////////////////////////////////////////////
//
// This routine sets the previous message for
// browse functions if a queue was displayed.
// We can do this because we have a list of
// message ids for the queue.
//
//////////////////////////////////////////////////

void DataArea::setBrowsePrevMsg(char * prevMsgId)

{
	// check if the message id is set and that a browse is active
	if ((memcmp(prevMsgId, MQMI_NONE, MQ_MSG_ID_LENGTH) != 0) && (1 == browseActive))
	{
		memcpy(m_save_message_id, prevMsgId, MQ_MSG_ID_LENGTH);
		browsePrevActive = 1;
	}
}

//////////////////////////////////////////////////////////////////
//
// This routine sets up a browse operation on the specified
// queue.  It returns the MQMD of the first message on the
// queue.  It does not read the message data, ignoring truncation.
//
//////////////////////////////////////////////////////////////////

int DataArea::startMsgDisplay(const char *QMname, const char *Queue, MQMD2 * mqmd, MQLONG *msgLen)

{
	MQLONG	cc=MQCC_OK;								// MQ completion code
	MQLONG	rc;										// MQ reason code
	MQGMO	gmo={MQGMO_DEFAULT};					// Get message options
	char	traceInfo[512];							// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::startMsgDisplay() QMname=%s Queue=%s", QMname, Queue);

		// trace entry to startMsgDisplay
		logTraceEntry(traceInfo);
	}

	m_error_msg.Empty();

	// make sure we have a queue name
	if (strlen(Queue) == 0)
	{
		m_error_msg = "*Queue Name required* ";

		return -1;
	}

	// set the queue depth to zero
	m_q_depth = 0;

	// connect to the queue manager
	if (!connect2QM(QMname))
	{
		return -1;
	}

	// open the queue
	if (!openQ(Queue, _T(""), Q_OPEN_BROWSE, FALSE))
	{
		// disconnect from the queue manager
		discQM();

		// indicate the error
		return -1;
	}

	// make sure that the match options are honored
	gmo.Version = MQGMO_VERSION_2;

	// clear the message id, correlation id and group id
	memset(mqmd->MsgId, 0, MQ_MSG_ID_LENGTH);
	memset(mqmd->CorrelId, 0, MQ_CORREL_ID_LENGTH);
	memset(mqmd->GroupId, 0, MQ_GROUP_ID_LENGTH);

	// start a browse operation on the qeue
	gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST | MQGMO_ACCEPT_TRUNCATED_MSG;

	// Try to get a message
	XMQGet(qm, q, mqmd, &gmo, 0, NULL, msgLen, &cc, &rc);

	// check for truncated message accepted
	if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_ACCEPTED == rc))
	{
		// change to normal completion
		cc = MQCC_OK;
		rc = MQRC_NONE;
	}

	// check if the get worked
	if (cc != MQCC_OK)
	{
		// get failed, get the reason and generate an error message
		if (!((MQCC_FAILED == cc) && (MQRC_NO_MSG_AVAILABLE == rc)))
		{
			// get the completion code and reason code
			setErrorMsg(cc, rc, "Get (Browse)");
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::startMsgDisplay() cc=%d rc=%d msgLen=%d", cc, rc, (*msgLen));

		// trace exit from buildMsgTables
		logTraceEntry(traceInfo);
	}

	return rc;
}

////////////////////////////////////////////////////////////////
//
// This routine browses the next message on the queue and
// returns the MQMD and message length.  It does not read the
// message data, ignoring truncation..
//
////////////////////////////////////////////////////////////////

int DataArea::getNextMsgDisplay(MQMD2 *mqmd, MQLONG *msgLen)

{
	MQLONG	cc=MQCC_OK;								// MQ completion code
	MQLONG	rc;										// MQ reason code
	MQGMO	gmo={MQGMO_DEFAULT};					// Get message options
	char	traceInfo[512];							// work variable to build trace message

	// make sure that the match options are honored
	gmo.Version = MQGMO_VERSION_2;

	// clear the message id, correlation id and group id
	memset(mqmd->MsgId, 0, MQ_MSG_ID_LENGTH);
	memset(mqmd->CorrelId, 0, MQ_CORREL_ID_LENGTH);
	memset(mqmd->GroupId, 0, MQ_GROUP_ID_LENGTH);

	// start a browse operation on the qeue
	gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_NEXT | MQGMO_ACCEPT_TRUNCATED_MSG;

	// Try to get a message
	XMQGet(qm, q, mqmd, &gmo, 0, NULL, msgLen, &cc, &rc);

	// check for truncated message accepted
	if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_ACCEPTED == rc))
	{
		// change to normal completion
		cc = MQCC_OK;
		rc = MQRC_NONE;
	}

	// check if the get worked
	if (cc != MQCC_OK)
	{
		// get failed, get the reason and generate an error message
		if (!((MQCC_FAILED == cc) && (MQRC_NO_MSG_AVAILABLE == rc)))
		{
			// get the completion code and reason code
			setErrorMsg(cc, rc, "Get (Browse)");
		}
	}

	if (traceEnabled && verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getNextMsgDisplay() cc=%d rc=%d msgLen=%d", cc, rc, (*msgLen));

		// trace exit from getNextMsgDisplay
		logTraceEntry(traceInfo);
	}

	return rc;
}

////////////////////////////////////////////////////////////////
//
// This routine will set the message id field in the MQMD.
// This allows an MQGET by message id to be performed to
// retrieve a specific message.
//
////////////////////////////////////////////////////////////////

void DataArea::setMsgId(char * msgId)

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;

	// set the message id in the MQMD object
	mqmdObj->setMsgId(msgId);
}

////////////////////////////////////////////////////////////////
//
// This routine releases the storage acquired by the routines
// that build a message display string and message id table.
//
////////////////////////////////////////////////////////////////

void DataArea::releaseMsgIdTable()

{
	if (m_msg_id_table != NULL)
	{
		rfhFree(m_msg_id_table);
		m_msg_id_table = NULL;
	}

	msgIdCount = 0;
}

////////////////////////////////////////////////////////////////////////////
//
// Insert an MQ header into the header chain
//
// This routine will be called when the include check box on any of the
// supported headers is changed so that it is selected.  The format,
// ccsid and encoding values of the previous header or MQMD will be
// updated to reflect the new header. The format, ccsid and
//.encoding will be updated to reflect the next header in the chain.
//
// A dead letter queue header will be inserted first, whereas a CICS or IMS
// header will be inserted at the end of the header chain.  This routine
// updates the format, character set id and encoding, and returns
// the previous values.
//
////////////////////////////////////////////////////////////////////////////

void DataArea::insertMQheader(int hdrType, int ccsid, int encoding, int pdEncoding, int floatEncoding)

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	RFH			*rfhObj=(RFH *)rfhData;
	CCICS		*cicsObj=(CCICS *)cicsData;
	CIms		*imsObj=(CIms *)imsData;
	CDlq		*dlqObj=(CDlq *)dlqData;
	char		mqmdFormat[MQ_FORMAT_LENGTH + 4];
	char		dlqFormat[MQ_FORMAT_LENGTH + 4];
	char		rfh1Format[MQ_FORMAT_LENGTH + 4];
	char		rfh2Format[MQ_FORMAT_LENGTH + 4];
	char		cicsFormat[MQ_FORMAT_LENGTH + 4];
	char		imsFormat[MQ_FORMAT_LENGTH + 4];
	char		mqformat[MQ_FORMAT_LENGTH + 4];
	char		dlqHeader[MQ_FORMAT_LENGTH + 4];
	char		rfh1Header[MQ_FORMAT_LENGTH + 4];
	char		rfh2Header[MQ_FORMAT_LENGTH + 4];
	char		cicsHeader[MQ_FORMAT_LENGTH + 4];
	char		imsHeader[MQ_FORMAT_LENGTH + 4];
	char		traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::insertMQheader() hdrType=%d ccsid=%d encoding=%d", hdrType, ccsid, encoding);

		// trace entry to insertMQheader
		logTraceEntry(traceInfo);
	}

	// get the actual format name
	switch (hdrType)
	{
	case MQHEADER_DLQ:
		{
			strcpy(mqformat, MQFMT_DEAD_LETTER_HEADER);
			break;
		}
	case MQHEADER_RFH1:
		{
			strcpy(mqformat, MQFMT_RF_HEADER);
			break;
		}
	case MQHEADER_RFH2:
		{
			strcpy(mqformat, MQFMT_RF_HEADER_2);
			break;
		}
	case MQHEADER_CICS:
		{
			strcpy(mqformat, MQFMT_CICS);
			break;
		}
	case MQHEADER_IMS:
		{
			strcpy(mqformat, MQFMT_IMS);
			break;
		}
	default:
		{
			mqformat[0] = 0;
			break;
		}
	}

	// get the MQ format names
	strcpy(dlqHeader, MQFMT_DEAD_LETTER_HEADER);
	strcpy(rfh1Header, MQFMT_RF_HEADER);
	strcpy(rfh2Header, MQFMT_RF_HEADER_2);
	strcpy(cicsHeader, MQFMT_CICS);
	strcpy(imsHeader, MQFMT_IMS);

	// get the format fields
	mqmdObj->getFormat(mqmdFormat);
	rfhObj->getRFH1Format(rfh1Format);
	rfhObj->getRFH2Format(rfh2Format);
	dlqObj->getFormat(dlqFormat);
	cicsObj->getFormat(cicsFormat);
	imsObj->getFormat(imsFormat);

	// strip trailing blanks so our compaisons are accurate
	Rtrim(mqformat);
	Rtrim(mqmdFormat);
	Rtrim(rfh1Format);
	Rtrim(rfh2Format);
	Rtrim(dlqFormat);
	Rtrim(cicsFormat);
	Rtrim(imsFormat);
	Rtrim(dlqHeader);
	Rtrim(rfh1Header);
	Rtrim(rfh2Header);
	Rtrim(cicsHeader);
	Rtrim(imsHeader);

	// check if the header is already in the chain somewhere
	if ((strcmp(mqmdFormat, mqformat) == 0) ||
		(strcmp(rfh1Format, mqformat) == 0) ||
		(strcmp(rfh2Format, mqformat) == 0) ||
		(strcmp(dlqFormat, mqformat) == 0) ||
		(strcmp(cicsFormat, mqformat) == 0) ||
		(strcmp(imsFormat, mqformat) == 0))
	{
		// header is already in the chain, so nothing to do - return immediately
		return;
	}

	// check if we are inserting a dead letter header - it should be first
	if (MQHEADER_DLQ == hdrType)
	{
		// insert the current values into the DLQ
		dlqObj->updateFields(mqmdFormat, mqmdObj->getCcsid(), mqmdObj->getEncoding(), mqmdObj->getPdEncoding(), mqmdObj->getFloatEncoding());

		// insert the dead letter header at the beginning of the chain
		mqmdObj->setFormat(MQFMT_DEAD_LETTER_HEADER);
	}

	// check if we are inserting an rfh v1 header - it should be after the DLQ header if present
	if (MQHEADER_RFH1 == hdrType)
	{
		// check if we have a dead letter header - if so we go after this
		if (strcmp(mqmdFormat, dlqHeader) == 0)
		{
			// get the next header in the chain
			// insert the current values from the DLQ header
			rfhObj->updateRFH1Fields(dlqFormat, dlqObj->getCcsid(), dlqObj->getEncoding(), dlqObj->getPdEncoding(), dlqObj->getFloatEncoding());

			// now update the fields in the DLQ
			dlqObj->updateFields(mqformat, ccsid, encoding, pdEncoding, floatEncoding);
		}
		else
		{
			// insert it first, since there is no dead letter header
			// insert the current values from MQMD
			rfhObj->updateRFH1Fields(mqmdFormat, mqmdObj->getCcsid(), mqmdObj->getEncoding(), mqmdObj->getPdEncoding(), mqmdObj->getFloatEncoding());

			// insert the rfh v1 header at the beginning of the chain
			mqmdObj->setFormat(MQFMT_RF_HEADER);
		}
	}

	// check if we are inserting an rfh v2 header - it should be after any DLQ or rfh1 headers
	if (MQHEADER_RFH2 == hdrType)
	{
		// look for an rfh1 header - either first or after the dead letter header
		// if found we insert after this
		if ((strcmp(mqmdFormat, rfh1Header) == 0) || (strcmp(dlqFormat, rfh1Header) == 0))
		{
			// insert the rfh2 after the rfh1 - no change to the DLQ
			rfhObj->updateRFH2Fields(rfh1Format, rfhObj->getRFH1Ccsid(), rfhObj->getRFH1Encoding(), rfhObj->getRFH1PdEncoding(), rfhObj->getRFH1FloatEncoding());

			// insert the rfh v2 after the rfh v1
			rfhObj->updateRFH1Fields(MQFMT_RF_HEADER_2, ccsid, encoding, pdEncoding, floatEncoding);
		}
		else
		{
			// no rfh1 header - look for a dlq header
			// is there a dead letter header
			if (strcmp(mqmdFormat, dlqHeader) == 0)
			{
				// no rfh1 - insert the rfh2 after the dead letter queue header
				// insert the current values from the DLQ header
				rfhObj->updateRFH2Fields(dlqFormat, dlqObj->getCcsid(), dlqObj->getEncoding(), dlqObj->getPdEncoding(), dlqObj->getFloatEncoding());

				// now update the fields in the DLQ to point to the RFH2
				dlqObj->updateFields(MQFMT_RF_HEADER_2, ccsid, encoding, pdEncoding, floatEncoding);
			}
			else
			{
				// no other headers that would go before this header
				// insert the RFH2 header first
				// insert the current values from MQMD
				rfhObj->updateRFH2Fields(mqmdFormat, mqmdObj->getCcsid(), mqmdObj->getEncoding(), mqmdObj->getPdEncoding(), mqmdObj->getFloatEncoding());

				// insert the rfh v1 letter header at the beginning of the chain
				mqmdObj->setFormat(MQFMT_RF_HEADER_2);
			}
		}
	}

	// check if we are inserting a cics header - it should be last
	if (MQHEADER_CICS == hdrType)
	{
		// check if we have an RFH2 header - if so insert after that
		if ((strcmp(mqmdFormat, rfh2Header) == 0) || (strcmp(dlqFormat, rfh2Header) == 0) || (strcmp(rfh1Format, rfh2Header) == 0))
		{
			// RFH2 present - insert the cics header after the rfh2 - no change to the DLQ or rfh1
			// insert the current values from the RFH2 header
			cicsObj->updateFields(rfh2Format, rfhObj->getRFH2Ccsid(), rfhObj->getRFH2Encoding(), rfhObj->getRFH2PdEncoding(), rfhObj->getRFH2FloatEncoding());

			// insert the cics header after the RFH2 leaving the ccsid and encoding fields in the RFH2 unchanged
			rfhObj->updateRFH2Fields(MQFMT_CICS, rfhObj->getRFH2Ccsid(), rfhObj->getRFH2Encoding(), rfhObj->getRFH2PdEncoding(), rfhObj->getRFH2FloatEncoding());
		}
		else
		{
			// check for an RFH1 header next
			if ((strcmp(mqmdFormat, rfh1Header) == 0) || (strcmp(dlqFormat, rfh1Header) == 0))
			{
				// RFH1 present - insert the cics header after the rfh1 - no change to the DLQ
				// insert the current values from the RFH1 header
				cicsObj->updateFields(rfh1Format, rfhObj->getRFH1Ccsid(), rfhObj->getRFH1Encoding(), rfhObj->getRFH1PdEncoding(), rfhObj->getRFH1FloatEncoding());

				// insert the cics header after the RFH1 leaving the ccsid and encoding fields in the RFH1 unchanged
				rfhObj->updateRFH1Fields(MQFMT_CICS, rfhObj->getRFH1Ccsid(), rfhObj->getRFH1Encoding(), rfhObj->getRFH1PdEncoding(), rfhObj->getRFH1FloatEncoding());
			}
			else
			{
				// check for a dead letter queue header
				if (strcmp(mqmdFormat, dlqHeader) == 0)
				{
					// dead letter header found
					// insert the cics header after the dead letter header
					// capture the current values from the DLQ header
					cicsObj->updateFields(dlqFormat, dlqObj->getCcsid(), dlqObj->getEncoding(), dlqObj->getPdEncoding(), dlqObj->getFloatEncoding());

					// now update the format in the DLQ to point to the cics header
					dlqObj->updateFields(MQFMT_CICS, dlqObj->getCcsid(), dlqObj->getEncoding(), dlqObj->getPdEncoding(), dlqObj->getFloatEncoding());
				}
				else
				{
					// insert the cics header first - it is the only header we recognize
					// capture the current values from MQMD
					cicsObj->updateFields(mqmdFormat, mqmdObj->getCcsid(), mqmdObj->getEncoding(), mqmdObj->getPdEncoding(), mqmdObj->getFloatEncoding());

					// update the MQMD to reflect the CICS header
					mqmdObj->setFormat(MQFMT_CICS);
				}
			}
		}
	}


	// check if we are inserting a ims header - it should be last
	if (MQHEADER_IMS == hdrType)
	{
		if (strcmp(mqmdFormat, cicsHeader) == 0)
		{
			// this should not occur, since you should not have both a
			// cics and ims header in the same message
			// cics header present - insert the ims header after the cics
			// no change to the DLQ or rfh1 or rfh2
			// insert the current values from the cics header
			imsObj->updateFields(cicsFormat, cicsObj->getCcsid(), cicsObj->getEncoding(), cicsObj->getPdEncoding(), cicsObj->getFloatEncoding());

			// insert the cics header after the rfh2
			cicsObj->updateFields(MQFMT_IMS, cicsObj->getCcsid(), cicsObj->getEncoding(), cicsObj->getPdEncoding(), cicsObj->getFloatEncoding());
		}
		else
		{
			// check if we have an RFH2 header - if so insert after that
			if ((strcmp(mqmdFormat, rfh2Header) == 0) || (strcmp(dlqFormat, rfh2Header) == 0) || (strcmp(rfh1Format, rfh2Header) == 0))
			{
				// RFH2 present - insert the ims header after the rfh2 - no change to the DLQ or rfh1
				// insert the current values from the RFH2 header
				imsObj->updateFields(rfh2Format, rfhObj->getRFH2Ccsid(), rfhObj->getRFH2Encoding(), rfhObj->getRFH2PdEncoding(), rfhObj->getRFH2FloatEncoding());

				// insert the ims header after the rfh2
				rfhObj->updateRFH2Fields(MQFMT_IMS, rfhObj->getRFH2Ccsid(), rfhObj->getRFH2Encoding(), rfhObj->getRFH2PdEncoding(), rfhObj->getRFH2FloatEncoding());
			}
			else
			{
				// check for an RFH1 header next
				if ((strcmp(mqmdFormat, rfh1Header) == 0) || (strcmp(dlqFormat, rfh1Header) == 0))
				{
					// RFH1 present - insert the ims header after the rfh1 - no change to the DLQ
					// insert the current values from the RFH1 header
					imsObj->updateFields(rfh1Format, rfhObj->getRFH1Ccsid(), rfhObj->getRFH1Encoding(), rfhObj->getRFH1PdEncoding(), rfhObj->getRFH1FloatEncoding());

					// insert the ims header after the rfh1
					rfhObj->updateRFH1Fields(MQFMT_IMS, rfhObj->getRFH1Ccsid(), rfhObj->getRFH1Encoding(), rfhObj->getRFH1PdEncoding(), rfhObj->getRFH1FloatEncoding());
				}
				else
				{
					// check for a dead letter queue header only
					if (strcmp(mqmdFormat, dlqHeader) == 0)
					{
						// dead letter header found
						// insert the ims header after the dead letter header
						// capture the current values from the DLQ header
						imsObj->updateFields(dlqFormat, dlqObj->getCcsid(), dlqObj->getEncoding(), dlqObj->getPdEncoding(), dlqObj->getFloatEncoding());

						// now update the fields in the DLQ to point to the ims header
						dlqObj->updateFields(MQFMT_IMS, dlqObj->getCcsid(), dlqObj->getEncoding(), dlqObj->getPdEncoding(), dlqObj->getFloatEncoding());
					}
					else
					{
						// insert the ims header first - it is the only header we recognize
						// capture the current values from MQMD
						imsObj->updateFields(mqmdFormat, mqmdObj->getCcsid(), mqmdObj->getEncoding(), mqmdObj->getPdEncoding(), mqmdObj->getFloatEncoding());

						// update the MQMD to reflect the ims header
						mqmdObj->setFormat(MQFMT_IMS);
					}
				}
			}
		}
	}

	if (traceEnabled)
	{
		// get the format fields
		mqmdObj->getFormat(mqmdFormat);
		rfhObj->getRFH1Format(rfh1Format);
		rfhObj->getRFH2Format(rfh2Format);
		dlqObj->getFormat(dlqFormat);
		cicsObj->getFormat(cicsFormat);
		imsObj->getFormat(imsFormat);

		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::insertMQheaderheader() mqmdFormat=%s, mqmdCcsid=%d, rfh1Format=%s rfh2Format=%s dlqFormat=%s cicsFormat=%s imsFormat=%s", mqmdFormat, mqmdObj->getCcsid(), rfh1Format, rfh2Format, dlqFormat, cicsFormat, imsFormat);

		// trace exit from insertMQheaderheader
		logTraceEntry(traceInfo);
	}

	return;
}

////////////////////////////////////////////////////////////////////////////
//
// Remove an MQ header from the header chain
//
// This routine will be called when the include check box on any of the
// supported headers is changed so that it is no longer selected.  The format,
// ccsid and encoding values of the previous header or MQMD will be
// updated to point to the header that follows this header.
//
////////////////////////////////////////////////////////////////////////////

void DataArea::removeMQheader(int hdrType, const char * format, int ccsid, int encoding, int pdEncoding, int floatEncoding)

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	RFH			*rfhObj=(RFH *)rfhData;
	CCICS		*cicsObj=(CCICS *)cicsData;
	CIms		*imsObj=(CIms *)imsData;
	CDlq		*dlqObj=(CDlq *)dlqData;
	char		mqmdFormat[MQ_FORMAT_LENGTH + 4];
	char		dlqFormat[MQ_FORMAT_LENGTH + 4];
	char		rfh1Format[MQ_FORMAT_LENGTH + 4];
	char		rfh2Format[MQ_FORMAT_LENGTH + 4];
	char		cicsFormat[MQ_FORMAT_LENGTH + 4];
	char		imsFormat[MQ_FORMAT_LENGTH + 4];
	char		mqformat[MQ_FORMAT_LENGTH + 4];
	char		dlqHeader[MQ_FORMAT_LENGTH + 4];
	char		rfh1Header[MQ_FORMAT_LENGTH + 4];
	char		rfh2Header[MQ_FORMAT_LENGTH + 4];
	char		cicsHeader[MQ_FORMAT_LENGTH + 4];
	char		imsHeader[MQ_FORMAT_LENGTH + 4];
	char	traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::removeMQheader() hdrType=%d format=%s ccsid=%d encoding=%d", hdrType, format, ccsid, encoding);

		// trace entry to removeMQheader
		logTraceEntry(traceInfo);
	}

	// get the actual format name
	switch (hdrType)
	{
	case MQHEADER_DLQ:
		{
			strcpy(mqformat, MQFMT_DEAD_LETTER_HEADER);
			break;
		}
	case MQHEADER_RFH1:
		{
			strcpy(mqformat, MQFMT_RF_HEADER);
			break;
		}
	case MQHEADER_RFH2:
		{
			strcpy(mqformat, MQFMT_RF_HEADER_2);
			break;
		}
	case MQHEADER_CICS:
		{
			strcpy(mqformat, MQFMT_CICS);
			break;
		}
	case MQHEADER_IMS:
		{
			strcpy(mqformat, MQFMT_IMS);
			break;
		}
	default:
		{
			mqformat[0] = 0;
			break;
		}
	}

	// get the MQ format names
	strcpy(dlqHeader, MQFMT_DEAD_LETTER_HEADER);
	strcpy(rfh1Header, MQFMT_RF_HEADER);
	strcpy(rfh2Header, MQFMT_RF_HEADER_2);
	strcpy(cicsHeader, MQFMT_CICS);
	strcpy(imsHeader, MQFMT_IMS);

	// get the format fields
	strcpy(mqmdFormat, (LPCTSTR)mqmdObj->m_mqmd_format);
	strcpy(rfh1Format, (LPCTSTR)rfhObj->m_rfh1_format);
	strcpy(rfh2Format, (LPCTSTR)rfhObj->m_rfh_format);
	dlqObj->getFormat(dlqFormat);
	cicsObj->getFormat(cicsFormat);
	imsObj->getFormat(imsFormat);

	// strip trailing blanks so our compaisons are accurate
	Rtrim(mqformat);
	Rtrim(mqmdFormat);
	Rtrim(rfh1Format);
	Rtrim(rfh2Format);
	Rtrim(dlqFormat);
	Rtrim(cicsFormat);
	Rtrim(imsFormat);
	Rtrim(dlqHeader);
	Rtrim(rfh1Header);
	Rtrim(rfh2Header);
	Rtrim(cicsHeader);
	Rtrim(imsHeader);

	// check if the header is already in the chain somewhere
	if (strcmp(mqmdFormat, mqformat) == 0)
	{
		// the header is the first in the chain
		mqmdObj->updateFields(format, ccsid, encoding, pdEncoding, floatEncoding);
	}

	if (strcmp(rfh1Format, mqformat) == 0)
	{
		// the header is chained to the rfh1 header
		rfhObj->updateRFH1Fields(format, ccsid, encoding, pdEncoding, floatEncoding);
	}

	if (strcmp(rfh2Format, mqformat) == 0)
	{
		// the header is chained to the rfh2 header
		rfhObj->updateRFH2Fields(format, ccsid, encoding, pdEncoding, floatEncoding);
	}

	if (strcmp(dlqFormat, mqformat) == 0)
	{
		// the header is chained to the dead letter queue header
		dlqObj->updateFields(format, ccsid, encoding, pdEncoding, floatEncoding);
	}

	if (strcmp(cicsFormat, mqformat) == 0)
	{
		// the header is chained to the cics header
		cicsObj->updateFields(format, ccsid, encoding, pdEncoding, floatEncoding);
	}

	if (strcmp(imsFormat, mqformat) == 0)
	{
		// the header is chained to the ims header
		imsObj->updateFields(format, ccsid, encoding, pdEncoding, floatEncoding);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::removeMQheader() mqformat=%s", mqformat);

		// trace exit from removeMQheader
		logTraceEntry(traceInfo);
	}
}

////////////////////////////////////////////////////////////
//
// Free the header raw data area corresponding to the
// provided format name
//
// When a message is read from a file or queue, the original
// contents of the headers are saved in binary format.
// The binary format is reused if a message is then
// written to a file or queue.  However, it must be
// discarded if a change to the code page or encoding is
// made in a previous header, since the header must be
// rebuilt using the new code page or encoding values.
// This routine determines the affected header and
// discards the corresponding binary data area.
//
////////////////////////////////////////////////////////////

void DataArea::freeHeader(const char * format)

{
	char	mqformat[MQ_FORMAT_LENGTH + 4];
	char	dlqHeader[MQ_FORMAT_LENGTH + 4];
	char	rfh1Header[MQ_FORMAT_LENGTH + 4];
	char	rfh2Header[MQ_FORMAT_LENGTH + 4];
	char	cicsHeader[MQ_FORMAT_LENGTH + 4];
	char	imsHeader[MQ_FORMAT_LENGTH + 4];
	char	traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::freeHeader() format=%s", format);

		// trace entry to freeHeader
		logTraceEntry(traceInfo);
	}

	// get the MQ format names
	strcpy(dlqHeader, MQFMT_DEAD_LETTER_HEADER);
	strcpy(rfh1Header, MQFMT_RF_HEADER);
	strcpy(rfh2Header, MQFMT_RF_HEADER_2);
	strcpy(cicsHeader, MQFMT_CICS);
	strcpy(imsHeader, MQFMT_IMS);

	// get the format name into a work area
	strcpy(mqformat, format);

	// strip trailing blanks so our compaisons are accurate
	Rtrim(mqformat);
	Rtrim(dlqHeader);
	Rtrim(rfh1Header);
	Rtrim(rfh2Header);
	Rtrim(cicsHeader);
	Rtrim(imsHeader);

	if (strcmp(dlqHeader, mqformat) == 0)
	{
		// delete the current dead letter queue header area
		((CDlq *)dlqData)->freeCurrentHeader(1);
	}

	if (strcmp(cicsHeader, mqformat) == 0)
	{
		// delete the current dead letter queue header area
		((CCICS *)cicsData)->freeCurrentHeader(1);
	}

	if (strcmp(imsHeader, mqformat) == 0)
	{
		// delete the current dead letter queue header area
		((CIms *)imsData)->freeCurrentHeader(1);
	}

	if (strcmp(rfh1Header, mqformat) == 0)
	{
		// delete the current rfh1 header area
		((RFH *)rfhData)->freeRfh1Area();
	}

	if (strcmp(rfh2Header, mqformat) == 0)
	{
		// delete the current rfh1 header area
		((RFH *)rfhData)->freeRfhArea();
	}
}

//////////////////////////////////////////////
//
// Common routine that is used multiple times
// in the saveMsgs routine.
//
// If no error has occurred the return is
// true.  Otherwise the return is false,
// indicating that an error has occurred.
//
//////////////////////////////////////////////

bool DataArea::processSaveMsgsCC(MQLONG cc, MQLONG rc, const char *operation)

{
	// check the MQ completion code
	if (cc != MQCC_OK)
	{
		// get failed, get the reason and generate an error message
		// get the completion code and reason code
		if (rc != MQRC_NO_MSG_AVAILABLE)
		{
			// some other error - build an error message
			setErrorMsg(cc, rc, operation);
		}

		// indicate we had an error
		return false;
	}
	else
	{
		// no error
		return true;
	}
}

///////////////////////////////////////////////////////////////////
//
// Routine to save all or some of the messages in a
// given queue to one or more files.  The messages can
// be read or browsed.
//
// Input parameters:
//  fileName - name of the file to store messages in
//  QMname - name of the queue manager to connect to
//  Qname - name of the queue
//  startCount - first message to be saved - first message
//   is message 1.
//  msgCount - maximum number of messages to save
//  filesType - indicator if single file or file per message
//   is to be used.  If file per message is indicated, then
//   the file name will have the message number appended
//   to the file name and before the file extension.
//  browseMsgs - indicator if messages are to be read or browsed.
//  includeMQMD - indicator if MQMDs are to be saved with data
//  removeHeaders - indicator if MQ headers are to be removed.
//  delimiter - delimiter string to be used if all messages are
//   stored in a single file.  Ignored if file per message
//   is used.
//  delimLen - number of bytes in the delimiter string
//
///////////////////////////////////////////////////////////////////

void DataArea::saveMsgs(SAVEPARMS * parms)

{
	// define the MQ objects and variables that we need
	MQLONG		cc=MQCC_OK;				// MQ completion code
	MQLONG		rc=MQRC_NONE;			// MQ reason code
	MQLONG		cc2=MQCC_OK;			// MQ completion code
	MQLONG		rc2=MQRC_NONE;			// MQ reason code
	MQLONG		msgLen;					// length of the message
	MQLONG		msgLen2;				// length of the message - used only to read messages to remove them from the queue
	MQLONG		bufLen;					// length of the default message buffer
	MQLONG		options;				// MQ GMO options
	MQLONG		matchOptions;			// MQ GMO match options
	MQLONG		tempOpt;				// work variable for trace
	int			slen;					// work variable for lengths of strings
	int			i;						// work variable
	int			uowCount=0;				// number of messages in current UOW
	int			request;				// type of MQGet to read - browse or destructive read
	int			count=0;				// number of messages processed
	int			fileCount=0;			// number of files processed
	int			groupOpt=0;				// GMO group options (options that require a group index under MVS)
	int			hdrLen=0;				// length of any headers
	int			allocCount=0;			// number of buffers that were allocated besides default
	int			freeCount=0;			// number of buffers that were freed
	int			fileLen=0;				// starting length of output file
	int			fileOfs=0;				// offset in the current file
	int			totLen=0;				// number of bytes being added to file
	int			bufOfs=0;				// number of bytes in properties buffer
	int			propLen=16*1024*1024;	// length of properties buffer
	char		*msgData=NULL;			// pointer to message data
	char		*buffer=NULL;			// buffer to hold message properties
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;
	FILE		*outputFile = NULL;		// output file
	CRfhutilApp *app;					// pointer to MFC application object
	bool		noErr=true;				// error switch - terminate processing after error
	bool		errMsgSet=false;		// error text has been set
	MQGMO		gmo={MQGMO_DEFAULT};	// Get message options
	MQGMO		gmo2={MQGMO_DEFAULT};	// Get message options
	MQMD2		mqmd={MQMD2_DEFAULT};	// mqmd
	MQHMSG		hMsg=MQHM_UNUSABLE_HMSG;	// message handle used to get message properties
	MQCMHO		opts={MQCMHO_DEFAULT};		// options used to create message handle
	MQDMHO		dOpts={MQDMHO_DEFAULT};		// options used to delete message handle
	char		auditTxt[32];			// work area to build audit message
	char		errtxt[256];			// message work area
	char		newFileName[512] = { 0 };	// file name to open - needed for one file per message option
	char		traceInfo[512];			// work variable to build trace message

	if (propertiesSupported && (MQ_PROPS_YES == m_mq_props))
	{
		buffer = (char *)rfhMalloc(propLen, "PROPBUFF");

		// if value malloc failed try smaller values
		while ((NULL == buffer) && (propLen > 10240))
		{
			propLen = propLen >> 1;
			buffer = (char *)rfhMalloc(propLen, "PROPBUFF ");
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::saveMsgs() qmName=%s aName=%s delimLen=%d startCount=%d endCount=%d filesType=%d browseMsgs=%d includeMQMD=%d removeHdrs=%d appendFile=%d maxFileSize=%d fileName=%.256s", parms->qmName, parms->qName, parms->delimLen, parms->startCount, parms->endCount, parms->filesType, parms->browseMsgs, parms->includeMQMD, parms->removeHdrs, parms->appendFile, parms->maxfileSize, parms->fileName);

		// trace entry to saveMsgs
		logTraceEntry(traceInfo);

		// create the trace line
		sprintf(traceInfo, "  m_get_by_msgid=%d m_get_by_correlid=%d m_get_by_groupid=%d m_logical_order=%d m_all_avail=%d m_complete_msg=%d m_setUserID=%d", m_get_by_msgid, m_get_by_correlid, m_get_by_groupid, m_logical_order, m_all_avail, m_complete_msg, m_setUserID);

		// trace entry to saveMsgs
		logTraceEntry(traceInfo);

		// create the trace line
		sprintf(traceInfo, "  buffer=%8.8X propLen=%d", (unsigned int)buffer, propLen);

		// trace entry to saveMsgs
		logTraceEntry(traceInfo);
	}

	// make sure there is a properties buffer if needed
	if (propertiesSupported && (MQ_PROPS_YES == m_mq_props) && (NULL == buffer))
	{
		// return with error
		m_error_msg = "Memory allocation failure - try different properties option";
		return;
	}

	// initialize work variables
	memset(newFileName, 0, sizeof(newFileName));
	m_error_msg.Empty();

	// make sure we have a queue name
	if (strlen(parms->qName) == 0)
	{
		// no queue name - set error message and return
		m_error_msg = "*Queue Name required* ";

		if (buffer != NULL)
		{
			rfhFree(buffer);
		}

		return;
	}

	// check for no file name
	if (0 == parms->fileName[0])
	{
		// file name is missing - set error message and exit
		m_error_msg = "*File name required* ";

		if (buffer != NULL)
		{
			rfhFree(buffer);
		}

		return;
	}

	// connect to the queue manager
	if (!checkConnection(parms->qmName))
	{
		// can't connect - just return
		return;
	}

	// check if only complete messages are to be read
	if (m_complete_msg)
	{
		groupOpt |=  MQGMO_COMPLETE_MSG;
	}

	// check if logical order is to be maintained
	if (m_logical_order)
	{
		groupOpt |= MQGMO_LOGICAL_ORDER;
	}

	// check if only complete messages or groups are to be returned
	if (m_all_avail)
	{
		groupOpt |= MQGMO_ALL_MSGS_AVAILABLE | MQGMO_ALL_SEGMENTS_AVAILABLE;
	}

	// set default options
	options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST | groupOpt;

	// make sure that the match options are honored
	gmo.Version = MQGMO_VERSION_2;
	gmo2.Version = MQGMO_VERSION_2;

	// check if we are simply browsing the messages
	if (parms->browseMsgs)
	{
		// we just need a browse operation
		request = Q_OPEN_BROWSE;
		gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST | groupOpt;
		options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST | groupOpt;
	}
	else
	{
		// check if we are starting from the beginning of the queue
		if (parms->startCount > 1)
		{
			// we are using destructive reads but not from the beginning of the queue
			// we are going to have to browse the queue and then read the messages under the cursor
			// we need to accept truncated messages since we use a zero length buffer for the browse
			request = Q_OPEN_READ_BROWSE;
			gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST | groupOpt;

			// set the options to read the message from the queue
			// it is necessary to browse the queue to skip the first number of messages
			gmo2.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_MSG_UNDER_CURSOR | MQGMO_ACCEPT_TRUNCATED_MSG | groupOpt;
		}
		else
		{
			// we are simply reading the queue, starting with the first message
			request = Q_OPEN_READ;
			gmo.Options = MQGMO_FAIL_IF_QUIESCING | groupOpt;

			// override the default options to remove browse option
			options = MQGMO_FAIL_IF_QUIESCING | groupOpt;;
		}
	}

	// try to open the queue
	if (!openQ(parms->qName, _T(""), request, FALSE))
	{
		// didn't work - disconnect from queue manager and return
		discQM();

		if (buffer != NULL)
		{
			rfhFree(buffer);
		}

		return;
	}

	// try to open the output file
	// do this first to avoid the embarassment of reading a message and not being
	// able to open the output file
	// the name of the first file will always be what the user specified with
	// nothing appended to the name
	// check for an append option
	if (parms->appendFile)
	{
		// open output file for append
		outputFile = fopen(parms->fileName, "ab");
	}
	else
	{
		// open output file
		outputFile = fopen(parms->fileName, "wb");
	}

	// make sure that the file count is correct if multiple messages per file
	if (SAVE_FILE_SAME_FILE == parms->filesType)
	{
		fileCount++;
	}

	// was the open successful?
	if (NULL == outputFile)
	{
		// can't open the file
		// indicate the error
		appendError("Unable to open output file");

		// close the queue and disconnect from the queue manager
		closeQ(Q_CLOSE_NONE);
		discQM();

		if (buffer != NULL)
		{
			rfhFree(buffer);
		}

		// return to caller
		return;
	}

	// check for an append option
	if (parms->appendFile)
	{
		// check if the file is empty or being appended
		fseek(outputFile, 0L, SEEK_END);
		fileLen = ftell(outputFile);
	}

	// get the size to use for the initial MQGET
	bufLen = setInitialBufferSize();

	// check for a start count after the first message
	// if a start count > 1 is specified then the queue
	// must be browsed to find the first message to be saved
	if (parms->startCount > 1)
	{
		// indicate we will accept truncated messages
		// truncated messages are accepted since the messages are being skipped
		gmo.Options |= MQGMO_ACCEPT_TRUNCATED_MSG;

		// starting beyond the first message - skip the first n messages
		i = 1;
		while ((i < parms->startCount) && noErr)
		{
			// clear or set the message id, correl id and group id
			memset(mqmd.MsgId, 0, MQ_MSG_ID_LENGTH);
			memset(mqmd.CorrelId, 0, MQ_CORREL_ID_LENGTH);
			memset(mqmd.GroupId, 0, MQ_GROUP_ID_LENGTH);

			// check for a get by message id request
			if (m_get_by_msgid && (memcmp(mqmdObj->m_message_id, MQMI_NONE, MQ_MSG_ID_LENGTH) != 0))
			{
				mqmdObj->setMsgId(&mqmd);
				matchOptions |= MQMO_MATCH_MSG_ID;
			}

			if (m_get_by_correlid && (memcmp(mqmdObj->m_correlid, MQCI_NONE, MQ_CORREL_ID_LENGTH) != 0))
			{
				mqmdObj->setCorrelId(&mqmd);
				matchOptions |= MQMO_MATCH_CORREL_ID;
			}

			if (m_get_by_groupid && (memcmp(mqmdObj->m_group_id, MQGI_NONE, MQ_GROUP_ID_LENGTH) != 0))
			{
				mqmdObj->setGroupId(&mqmd);
				matchOptions |= MQMO_MATCH_GROUP_ID;
			}

			// check if we need to set the user id
			if (m_setUserID)
			{
				// get the length of the user id
				slen = m_user_id.GetLength();

				// check if it is longer than the field
				if (slen > MQ_USER_ID_LENGTH)
				{
					// truncate to maximum length allowed
					slen = MQ_USER_ID_LENGTH;
				}

				// set the alternate id in the MQMD
				memset(mqmd.UserIdentifier, 0, MQ_USER_ID_LENGTH);
				memcpy(mqmd.UserIdentifier, (LPCTSTR)m_user_id, slen);
			}

			// browse the messages in the queue but do not read any data
			XMQGet(qm, q, &mqmd, &gmo, 0, NULL, &msgLen, &cc, &rc);

			if (traceEnabled && verboseTrace)
			{
				// turn the MQ GMO options into a hex string
				memset(auditTxt, 0, sizeof(auditTxt));
				tempOpt = reverseBytes4(gmo.Options);
				AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)auditTxt);

				// create the trace line
				sprintf(traceInfo, " saveMsgs() MQPUT cc=%d rc=%d gmo.Options=X\'%s\' msgLen=%d uowCount=%d", cc, rc, auditTxt, msgLen, uowCount);

				// trace entry to saveMsgs
				logTraceEntry(traceInfo);
			}

			// check if it worked or we got a truncation error
			if ((cc != MQCC_OK) && !((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_ACCEPTED == rc)))
			{
				// check for a 2394 (MVS only - indicates queue does not have index on group id)
				// note that this should occur on the first message so we retry from the beginning
				if (rc == MQRC_Q_INDEX_TYPE_ERROR)
				{
					// turn off the unsupported options
					groupOpt = 0;
					gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST | MQGMO_ACCEPT_TRUNCATED_MSG;
					gmo2.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_MSG_UNDER_CURSOR | MQGMO_ACCEPT_TRUNCATED_MSG;

					// try the operation again
					XMQGet(qm, q, &mqmd, &gmo, 0, NULL, &msgLen, &cc, &rc);

					// check if it worked
					if (cc != MQCC_OK)
					{
						// get failed, get the reason and generate an error message
						// get the completion code and reason code
						setErrorMsg(cc, rc, "Get");

						// indicate we had an error
						noErr = false;
					}
				}
				else
				{
					// get failed, get the reason and generate an error message
					// get the completion code and reason code
					setErrorMsg(cc, rc, "Get");

					// indicate we had an error
					noErr = false;
				}
			}

			// switch from browse first to browse next
			gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_NEXT | MQGMO_ACCEPT_TRUNCATED_MSG | groupOpt;

			// move on to the next message
			i++;
			uowCount++;

			// should be do a commit
			if (uowCount == maxUOW - 1)
			{
				//MQCMIT(qm, &cc2, &rc2);
				XMQCmit(qm, &cc2, &rc2);
			}
		}

		// did we read past the end of the queue?
		if (MQRC_NO_MSG_AVAILABLE == rc)
		{
			// change the error message to something more meaningful
			m_error_msg = "Start count exceeds number of messages in queue - no messages saved";
			errMsgSet = true;

			// skip the processing
			noErr = false;
		}

		// turn off the accept truncated messages option
		// from now on the message data is going to be read so
		// truncated messages should no longer be accepted
		gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_NEXT | groupOpt;
	}

	// check if message properties are to be saved
	if (propertiesSupported && noErr && (MQ_PROPS_YES == m_mq_props))
	{
		// create a message handle
		// make sure that the message handle processing options are honored
		gmo.Version = MQGMO_VERSION_4;

		// check if properties should be returned in a handle
		if (MQ_PROPS_YES == m_mq_props)
		{
			// set the message handle options
			opts.Options = MQCMHO_VALIDATE;

			// set the message options
			gmo.Options |= MQGMO_PROPERTIES_IN_HANDLE;

			// create a message handle
			XMQCrtMh(qm, &opts, &hMsg, &cc2, &rc2);

			if (traceEnabled)
			{
				// enter the result in the trace
				sprintf(traceInfo, " MQCrtMh cc2=%d rc2=%d hMsg=%8.8X", cc2, rc2, (unsigned int)hMsg);

				// write a trace entry
				logTraceEntry(traceInfo);
			}

			// check if it worked
			if (cc2 != MQCC_OK)
			{
				// unable to create the message handle - report the error
				setErrorMsg(cc2, rc2, "MQCRTMH");

				if (buffer != NULL)
				{
					rfhFree(buffer);
				}

				// return completion code
				return;
			}

			// set the handle in the GMO
			gmo.MsgHandle = hMsg;
		}
		else if (MQ_PROPS_NONE == m_mq_props)
		{
			// set properties to none - they will be ignored
			opts.Options |= MQGMO_NO_PROPERTIES;
		}
		else if (MQ_PROPS_RFH2 == m_mq_props)
		{
			// set properties to force into RFH2 header
			opts.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;
		}
	}

	while (noErr && (outputFile != NULL) && ((0 == parms->endCount) || (count < parms->endCount)))
	{
		// clear or set the message id, correl id and group id
		// we just want the next message
		memset(mqmd.MsgId, 0, MQ_MSG_ID_LENGTH);
		memset(mqmd.CorrelId, 0, MQ_CORREL_ID_LENGTH);
		memset(mqmd.GroupId, 0, MQ_GROUP_ID_LENGTH);

		// check for a get by message id request
		if (m_get_by_msgid && (memcmp(mqmdObj->m_message_id, MQMI_NONE, MQ_MSG_ID_LENGTH) != 0))
		{
			mqmdObj->setMsgId(&mqmd);
			matchOptions |= MQMO_MATCH_MSG_ID;
		}

		if (m_get_by_correlid && (memcmp(mqmdObj->m_correlid, MQCI_NONE, MQ_CORREL_ID_LENGTH) != 0))
		{
			mqmdObj->setCorrelId(&mqmd);
			matchOptions |= MQMO_MATCH_CORREL_ID;
		}

		if (m_get_by_groupid && (memcmp(mqmdObj->m_group_id, MQGI_NONE, MQ_GROUP_ID_LENGTH) != 0))
		{
			mqmdObj->setGroupId(&mqmd);
			matchOptions |= MQMO_MATCH_GROUP_ID;
		}

		// Try to get a message - this is always a browse operation
		XMQGet(qm, q, &mqmd, &gmo, bufLen, defaultMQbuffer, &msgLen, &cc, &rc);

		// check if the buffer is too small
		if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_FAILED == rc))
		{
			// allocate a larger buffer and retry the get
			msgData = (char *)rfhMalloc(msgLen + 16, "MSGDATA4");
			allocCount++;

			// retry the MQGET with a buffer that is long enough
			XMQGet(qm, q, &mqmd, &gmo, msgLen, msgData, &msgLen, &cc, &rc);
		}
		else
		{
			// point to the data
			// N.B. It does not matter if the pointer is pointing to the default buffer
			// since the completion and reason codes will be checked
			msgData = defaultMQbuffer;
		}

		// check if the get failed
		if (cc != MQCC_OK)
		{
			// check for a 2024 (too many uncommitted messages)
			if (MQRC_SYNCPOINT_LIMIT_REACHED == rc)
			{
				// try to commit the current UOW and then reissue the request
				XMQCmit(qm, &cc2, &rc2);

				// try the get again
				XMQGet(qm, q, &mqmd, &gmo, bufLen, defaultMQbuffer, &msgLen, &cc, &rc);

				// check if the buffer is too small
				if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_FAILED == rc))
				{
					// allocate a larger buffer and retry the get
					msgData = (char *)rfhMalloc(msgLen + 16, "MSGDATA5");
					allocCount++;

					// retry the MQGET with a buffer that is long enough
					XMQGet(qm, q, &mqmd, &gmo, msgLen, msgData, &msgLen, &cc, &rc);
				}
				else
				{
					// point to the data
					// N.B. It does not matter if the pointer is pointing to the default buffer
					// since the completion and reason codes will be checked
					msgData = defaultMQbuffer;
				}

				// check if the get worked
				noErr = processSaveMsgsCC(cc, rc, "GetSaveMsgs");
			}
			else if (MQRC_Q_INDEX_TYPE_ERROR == rc)
			{
				// this is an MVS specific error caused when the queue does not have a group index defined
				// turn off the unsupported options and try again
				groupOpt = 0;
				if (parms->browseMsgs)
				{
					gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_FIRST;
				}
				else if (parms->startCount > 1)
				{
					// repeat the browse next without the group options
					gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_NEXT;
					gmo2.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_MSG_UNDER_CURSOR;
				}
				else
				{
					// simply read the message without the group options
					gmo.Options = MQGMO_FAIL_IF_QUIESCING;
				}

				// try the get again
				XMQGet(qm, q, &mqmd, &gmo, 0, NULL, &msgLen, &cc, &rc);

				// check if the buffer is too small
				if ((MQCC_FAILED == cc) && (MQRC_TRUNCATED_MSG_FAILED == rc))
				{
					// allocate a larger buffer and retry the get
					msgData = (char *)rfhMalloc(msgLen + 16, "MSGDATA6");
					allocCount++;

					// retry the MQGET with a buffer that is long enough
					XMQGet(qm, q, &mqmd, &gmo, msgLen, msgData, &msgLen, &cc, &rc);
				}
				else
				{
					// point to the data
					// N.B. It does not matter if the pointer is pointing to the default buffer
					// since the completion and reason codes will be checked
					msgData = defaultMQbuffer;
				}

				// check if the get worked
				noErr = processSaveMsgsCC(cc, rc, "Get2SaveMsgs");
			}
			else
			{
				// get failed, get the reason and generate an error message
				noErr = processSaveMsgsCC(cc, rc, "Get3SaveMsgs");
			}
		}

		// check if are getting messages but not starting at the beginning of the queue
		// in this case, we must browse the queue and then get the message under the browse cursor
		if (noErr && !(parms->browseMsgs) && (parms->startCount > 1))
		{
			// we must now read the message we just browsed by reading the message under the cursor
			// no data is necessary
			// this is done using the get message under cursor option
			XMQGet(qm, q, &mqmd, &gmo2, 0, NULL, &msgLen2, &cc, &rc);

			// check for a truncation error
			if (MQRC_TRUNCATED_MSG_ACCEPTED == rc)
			{
				// suppress the error by changing the completion to a normal completion
				cc = MQCC_OK;
			}

			// check if the get worked
			if (cc != MQCC_OK)
			{
				// check for a 2024 (too many uncommitted messages)
				if (MQRC_SYNCPOINT_LIMIT_REACHED == rc)
				{
					// try to commit the current UOW and then reissue the request
					XMQCmit(qm, &cc2, &rc2);

					// try the get again
					// the purpose of this get is to dispose of the message
					// the data has already been read
					XMQGet(qm, q, &mqmd, &gmo2, 0, NULL, &msgLen2, &cc, &rc);

					// check if the get worked
					noErr = processSaveMsgsCC(cc, rc, "Get under cursor");
				}
				else
				{
					// get failed, get the reason and generate an error message
					// get the completion code and reason code
					setErrorMsg(cc, rc, "Get under cursor");

					// indicate we had an error
					noErr = false;
				}
			}
		}

		// did an error occur?
		if (noErr)
		{
			// no error getting the message so time to process the message
			// count the message
			count++;
			uowCount++;

			// are we at the maximum number of messages in a unit of work?
			if (uowCount == maxUOW - 1)
			{
				// commit the current messages
				XMQCmit(qm, &cc2, &rc2);

				// reset the UOW counter
				uowCount = 0;
			}

			// check if message properties are to be processed
			if (propertiesSupported && (MQ_PROPS_YES == m_mq_props))
			{
				// extract the message properties
				// if there are properties with this message they will be placed in buffer
				// the length of the property data will be returned as bufOfs
				// if there are no properties then bufOfs will be 0
				bufOfs = extractMsgProps(hMsg, parms->propDelimLen, parms->propDelim, buffer, propLen);
			}

			// check if we are removing headers
			if (parms->removeHdrs)
			{
				// get the total length of any headers we recognize
				hdrLen = savemsgsRemoveHeaders(msgData, &mqmd, msgLen, mqmd.CodedCharSetId, mqmd.Encoding, mqmd.Format);
			}

			// no need to calculate if one message per file
			if (parms->filesType != SAVE_FILE_DIFF_FILES)
			{
				fileLen = ftell(outputFile);

				// calculate the total number of bytes to store this message in the file
				// start with the user data
				totLen = msgLen - hdrLen;

				// delimiter between messages required?
				if (fileLen > 0)
				{
					totLen += parms->delimLen;
				}

				// check for MQMD
				if (parms->includeMQMD)
				{
					// add the size of an MQMD2
					totLen += sizeof(MQMD2);
				}

				// are properties being saved separately?
				if (bufOfs > 0)
				{
					totLen += bufOfs + parms->propDelimLen;
				}
			}

			// check if this message will fit in the current file
			if (fileLen + totLen > parms->maxfileSize)
			{
				// close this file and open a new one
				fclose(outputFile);

				// create the next file name
				createNextFileName(parms->fileName, newFileName, fileCount);
				fileCount++;

				// set the file length to zero, since this is a new file
				fileLen = 0;

				// try to open the new file
				outputFile = fopen(newFileName, "wb");

				// check if trace is active
				if (traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " DataArea::saveMsgs() opening new file %s", newFileName);

					// trace exit from saveMsgs
					logTraceEntry(traceInfo);
				}

				// check if it worked
				if (NULL == outputFile)
				{
					// tell what happened
					m_error_msg = "*****Error opening next file";

					if (buffer != NULL)
					{
						rfhFree(buffer);
					}

					if (msgData != defaultMQbuffer)
					{
						// release the acquired storage
						rfhFree(msgData);
					}

					return;
				}
			}

			// check if we need to add a delimiter to the file
			// we need to add the delimiter except on the first message in a file
			if ((parms->filesType != SAVE_FILE_DIFF_FILES) && (fileLen > 0))
			{
				// insert a delimiter into the file
				fwrite(parms->delimiter, parms->delimLen, 1, outputFile);
			}

			// write the message properties next
			if (bufOfs > 0)
			{
				// add the message properties and a delimiter to the output file
				fwrite(buffer, 1, bufOfs, outputFile);
				fwrite(parms->propDelim, 1, parms->propDelimLen, outputFile);
			}

			// check if we need to write an MQMD
			if (parms->includeMQMD)
			{
				fwrite(&mqmd, 1, sizeof(MQMD2), outputFile);
			}

			// do we have any data to write?
			if (msgLen > 0)
			{
				// write the data to the file
				fwrite(msgData + hdrLen, 1, msgLen - hdrLen, outputFile);
			}

			// check if we acquired storage for this message
			if (msgData != defaultMQbuffer)
			{
				// release the acquired storage
				rfhFree(msgData);
				freeCount++;
				msgData = NULL;
			}

			// check if we are using individual files
			if (SAVE_FILE_DIFF_FILES == parms->filesType)
			{
				// using one file per message
				if (outputFile != NULL)
				{
					// close the current file
					fclose(outputFile);
					outputFile = NULL;
				}

				// increment the file counter
				fileCount++;

				// build the next file name to use
				createNextFileName(parms->fileName, newFileName, fileCount);

				// try to open the new output file
				outputFile = fopen(newFileName, "wb");
			}
			else
			{
				// force the output to the file, so we don't lose messages
				fflush(outputFile);

				// set the file count to 1 so our message comes out correctly
				//fileCount = 1;
			}

			// change gmo to browse next from browse first
			if (parms->browseMsgs || (parms->startCount > 1))
			{
				gmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_BROWSE_NEXT | groupOpt;
			}
		}
		else
		{
			// check if storage was acquired
			if (msgData != defaultMQbuffer)
			{
				// release the acquired storage
				rfhFree(msgData);
				freeCount++;
				msgData = NULL;
			}
		}
	}

	if (buffer != NULL)
	{
		// release the properties buffer, if one was acquired
		rfhFree(buffer);
	}

	// close the last file
	if (outputFile != NULL) {
		fclose(outputFile);
	}

	// check if message properties are to be processed
	if (propertiesSupported && (MQ_PROPS_YES == m_mq_props) && (hMsg != MQHM_UNUSABLE_HMSG))
	{
		// delete the message handle
		XMQDltMh(qm, &hMsg, &dOpts, &cc2, &rc2);

		// check if it worked
		if (cc2 != MQCC_OK)
		{
			// unable to delete the message handle - report the error
			setErrorMsg(cc2, rc2, "MQDLTMH");
		}
	}

	// files are created before reading messages
	// delete the last file since it was not used
	if (SAVE_FILE_DIFF_FILES == parms->filesType)
	{
		// delete the last file we created
		remove(newFileName);
	}

	// update the current depth of the queue
	getCurrentDepth();

	// call the audit routine to record the SaveMsgs action
	sprintf(auditTxt, "SaveMsg %d msgs", count);
	app = (CRfhutilApp *)AfxGetApp();
	app->createAuditRecord((LPCTSTR)currentQM, (LPCTSTR)currentQ, auditTxt, rc);

	// was an error message already set? if so preserve the previous error
	// do this before we close the queue, so any error status is still there
	if (!errMsgSet)
	{
		// check if we reached the end of the queue
		if ((cc != MQCC_OK) && (rc != MQRC_NO_MSG_AVAILABLE))
		{
			// unexpected mqseries error reading queue - report it
			setErrorMsg(cc, rc, "Getting message");
		}
		else
		{
			// was there only one file
			if (1 == fileCount)
			{
				sprintf(errtxt, "%d messages added to %.192s", count, parms->fileName);
			}
			else
			{
				sprintf(errtxt, "%d messages added to %d file(s)", count, fileCount);
			}

			// add the message to the message display
			appendError(errtxt);
		}
	}

	// close the queue, leaving the connection active
	closeQ(Q_CLOSE_NONE);

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::saveMsgs() Allocated=%d Freed=%d", allocCount, freeCount);

		// trace exit from saveMsgs
		logTraceEntry(traceInfo);
	}

	// update the window title to the name of the last queue that was read from
	// this provides a visual indication to the user, which is especially valuable
	// when the user has more than one RFHUtil session running at the same time
	setWindowTitle(parms->qName);
}

///////////////////////////////////////////////////
//
// Routine to capture a message from a subscription
//  queue and write it to a file.
//
///////////////////////////////////////////////////

int DataArea::captureMsg(CAPTPARMS *parms)

{
	// define the MQ objects and variables
	MQLONG		cc=MQCC_OK;						// MQ completion code
	MQLONG		rc=MQRC_NONE;					// MQ reason code
	MQLONG		cc2=MQCC_OK;					// MQ completion code
	MQLONG		rc2=MQRC_NONE;					// MQ reason code
	MQLONG		msgLen;							// length of the message
	MQLONG		bufLen;							// length of the default message buffer
	MQLONG		options;
	int			hdrLen=0;						// length of any headers
	int			propLen=16*1024*1024;			// length of properties buffer
	int			bufOfs=0;						// length of extracted properties
	char		*msgData=NULL;					// pointer to message data
	char		*buffer=NULL;					// buffer for message properties
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)mqmdData;	// pointer to MQMD object
	MQGMO		gmo={MQGMO_DEFAULT};			// Get message options
	MQMD2		mqmd={MQMD2_DEFAULT};			// mqmd
	MQHMSG		hMsg=MQHM_UNUSABLE_HMSG;		// message handle used to get message properties
	MQCMHO		opts={MQCMHO_DEFAULT};			// options used to create message handle
	MQDMHO		dOpts={MQDMHO_DEFAULT};			// options used to delete message handle
	char		traceInfo[512];					// work variable to build trace message

	if (propertiesSupported && (MQ_PROPS_YES == m_mq_props))
	{
		buffer = (char *)rfhMalloc(propLen, "PROPBUFF");

		// if value malloc failed try smaller values
		while ((NULL == buffer) && (propLen > 10240))
		{
			propLen = propLen >> 1;
			buffer = (char *)rfhMalloc(propLen, "PROPBUFF ");
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::captureMsg() delimLen=%d includeMQMD=%d removeHeaders=%d appendFile=%d parms->fileLen=%d parms->count=%I64d propLen=%d buffer=%8.8X", parms->delimLen, parms->includeMQMD, parms->removeHeaders, parms->appendFile, parms->fileLen, parms->count, propLen, (unsigned int)buffer);

		// trace entry to captureMsgs
		logTraceEntry(traceInfo);
	}

	// initialize work variables
	m_error_msg.Empty();

	// make sure we have a subscription request
	if (MQHO_NONE == hSub)
	{
		// tell the user what the problem is
		// N.B. This should not happen since this option should
		// only be active if there is an active subscription
		m_error_msg = "No subscription active";

		// return immediately
		return -1;
	}

	// set default options
	options = MQGMO_FAIL_IF_QUIESCING | MQGMO_SYNCPOINT;

	// make sure that the atch options are honored
	gmo.Version = MQGMO_VERSION_2;

	// get the size to use for the initial MQGET
	bufLen = setInitialBufferSize();

	// check if message properties are to be saved
	if (propertiesSupported && (parms->inclProps || parms->inclTopic))
	{
		// create a message handle
		// make sure that the message handle processing options are honored
		gmo.Version = MQGMO_VERSION_4;

		// set the message handle options
		opts.Options = MQCMHO_VALIDATE;

		// set the message options
		gmo.Options |= MQGMO_PROPERTIES_IN_HANDLE;

		// create a message handle
		XMQCrtMh(qm, &opts, &hMsg, &cc2, &rc2);

		if (traceEnabled)
		{
			// enter the result in the trace
			sprintf(traceInfo, " MQCrtMh cc2=%d rc2=%d hMsg=%8.8X", cc2, rc2, (unsigned int)hMsg);

			// write a trace entry
			logTraceEntry(traceInfo);
		}

		// check if it worked
		if (cc2 != MQCC_OK)
		{
			// unable to create the message handle - report the error
			setErrorMsg(cc2, rc2, "MQCRTMH");

			// return completion code
			return rc2;
		}

		// set the handle in the GMO
		gmo.MsgHandle = hMsg;
	}
	else
	{
		// set properties to none - they will be ignored
		opts.Options |= MQGMO_NO_PROPERTIES;
	}

	// Try to get a message
	XMQGet(qm, subQ, &mqmd, &gmo, bufLen, defaultMQbuffer, &msgLen, &cc, &rc);

	// check if the buffer is too small
	if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_FAILED == rc))
	{
		// allocate a larger buffer and retry the get
		msgData = (char *)rfhMalloc(msgLen + 16, "MSGDATA7");
		parms->allocCount++;

		// retry the MQGET with a buffer that is long enough
		XMQGet(qm, subQ, &mqmd, &gmo, msgLen, msgData, &msgLen, &cc, &rc);
	}
	else
	{
		// point to the data
		// N.B. It does not matter if the pointer is pointing to the default buffer
		// since the completion and reason codes will be checked
		msgData = defaultMQbuffer;
	}

	// did an error occur?
	if (MQCC_OK == cc)
	{
		// no error getting the message so time to process the message
		// need to add a delimiter to the file?
		// add the delimiter except on the first message in a file
		// if appending to a file that is not empty the fileLen will be > 0
		if ((parms->count >= 1) || (parms->fileLen > 0))
		{
			// insert a delimiter into the file
			fwrite(parms->delimiter, parms->delimLen, 1, parms->outputFile);
		}

		// check if message properties are to be processed
		if (propertiesSupported && (parms->inclProps || parms->inclTopic))
		{
			// extract the message properties
			bufOfs = extractMsgProps(hMsg, parms->propDelimLen, parms->propDelim, buffer, propLen);

			// were any properties found?
			if (bufOfs > 0)
			{
				// write the message properties to the file
				fwrite(buffer, bufOfs, 1, parms->outputFile);
			}
		}

		// check if we are removing headers
		if (parms->removeHeaders)
		{
			// get the total length of any headers we recognize
			hdrLen = savemsgsRemoveHeaders(msgData, &mqmd, msgLen, mqmd.CodedCharSetId, mqmd.Encoding, mqmd.Format);
		}

		// need to write an MQMD?
		if (parms->includeMQMD)
		{
			fwrite(&mqmd, 1, sizeof(MQMD2), parms->outputFile);
		}

		// any data to write?
		if (msgLen - hdrLen > 0)
		{
			// write the data to the file
			fwrite(msgData + hdrLen, 1, msgLen - hdrLen, parms->outputFile);
		}

		// force the output to the file, so we don't lose messages
		fflush(parms->outputFile);

		// count the message
		parms->count++;

		// keep track of the number of bytes captured
		parms->totalBytes += msgLen;

		// commit the current message
		XMQCmit(qm, &cc2, &rc2);
	}
	else
	{
		// check for no message found error
		// this will not be logged
		if (rc != MQRC_NO_MSG_AVAILABLE)
		{
			// MQ error
			setErrorMsg(cc, rc, "MQGETCAPT");
		}
	}

	// check if storage was acquired
	if (msgData != defaultMQbuffer)
	{
		// release the acquired storage
		rfhFree(msgData);
		parms->freeCount++;
	}

	// check if message properties are to be processed
	if (propertiesSupported && (parms->inclProps || parms->inclTopic) && (hMsg != MQHM_UNUSABLE_HMSG))
	{
		// delete the message handle
		XMQDltMh(qm, &hMsg, &dOpts, &cc2, &rc2);

		// check if it worked
		if (cc2 != MQCC_OK)
		{
			// unable to delete the message handle - report the error
			setErrorMsg(cc2, rc2, "MQDLTMH");
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::captureMsg() cc=%d rc=%d cc2=%d rc2=%d parms->count=%I64d parms->fileLen=%d Allocated=%d Freed=%d", cc, rc, cc2, rc2, parms->count, parms->fileLen, parms->allocCount, parms->freeCount);

		// trace exit from captureMsgs
		logTraceEntry(traceInfo);
	}

	// was a properties buffer acquired?
	if (buffer != NULL)
	{
		// free the properties buffer
		rfhFree(buffer);
	}

	return rc;
}

///////////////////////////////////////////////////////////////////
//
// Routine to create a file name from a template file name and
// a message number.  The message number will be inserted before
// any file extension.  A file extension is considered to be
// any characters that follow a period on the end of the file name.
//
///////////////////////////////////////////////////////////////////

void DataArea::createNextFileName(const char *fileName, char *newFileName, int fileCount)

{
	char		*ptr;
	char		tempCount[16];
	char		fileExt[512];
	char		traceInfo[512];			// work variable to build trace message

	// make sure we have a file name to use as a template
	if (strlen(fileName) == 0)
	{
		// no file name template - get out as gracefully as possible
		newFileName[0] = 0;
		return;
	}

	// get the file name into a work area
	strcpy(newFileName, fileName);

	// check if we have a file extension
	ptr = newFileName + strlen(newFileName);

	// look for a period before we find a directory
	// the check for a slash is necessary to handle a file name with no extension
	// and a directory name with a period
	while ((ptr > newFileName) && ('.' != ptr[0]) && ('\\' != ptr[0]) && ('/' != ptr[0]))
	{
		ptr--;
	}

	// did we find a period?
	if ((ptr >= newFileName) && ('.' == ptr[0]))
	{
		// save the file extension
		strcpy(fileExt, ptr);

		// get rid of the period
		ptr[0] = 0;
	}
	else
	{
		// no file extension
		fileExt[0] = 0;

		// point to the end of the file name, so we can append
		ptr = newFileName + strlen(newFileName);
	}

	// insert a file count on the end of the file name
	memset(tempCount, 0, sizeof(tempCount));
	sprintf(tempCount, "%d", fileCount);
	strcat(ptr, tempCount);

	// restore the file extension
	strcat(ptr, fileExt);

	// check if verbose trace is enabled
	if (traceEnabled && verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::createNextFileName() strlen(fileExt)=%d newFileName=%.384s", strlen(fileExt), newFileName);

		// trace exit from createNextFileName
		logTraceEntry(traceInfo);
	}
}

///////////////////////////////////////////////////////////////////
//
// Routine to remove any MQ headers from a message before it is
// saved in a file.
//
///////////////////////////////////////////////////////////////////

int DataArea::savemsgsRemoveHeaders(char * msgData, MQMD2 * mqmd, int msgLen, int ccsid, int encoding, char * format)

{
	int		hdrLen;								// length of the current header
	int		totLen=0;							// total length of all recognized headers
	int		version;							// header version
	int		version2;							// header version that is byte-reversed
	int		encodeType;							// encoding type of the next header
	int		charType;							// code page type of the next header
	int		hdrCount=0;							// number of headers found - only used in trace
	int		invalidHdr=0;						// indicator if invalid header length was found - only used in trace
	MQDLH	*dlqPtr;							// pointer to dead letter queue header
	MQRFH2	*rfh2Ptr;							// pointer to RFH header
	MQCIH	*cicsPtr;							// pointer to CICS header
	MQIIH	*imsPtr;							// pointer to IMS header
	char	dlqFormat[MQ_FORMAT_LENGTH + 4];	// dead letter header MQ format name with trailing blanks removed
	char	cicsFormat[MQ_FORMAT_LENGTH + 4];	// CICS MQ format name with trailing blanks removed
	char	imsFormat[MQ_FORMAT_LENGTH + 4];	// IMS MQ format name with trailing blanks removed
	char	rfhFormat[MQ_FORMAT_LENGTH + 4];	// RFH MQ format name with trailing blanks removed
	char	msgFormat[MQ_FORMAT_LENGTH + 4];	// current MQ format name with trailing blanks removed
	boolean	foundHdr;							// switch to indicate a header was found
	boolean updateMQMD=false;					// indicator if MQMD needs to be updated after headers are removed
	char	traceInfo[512];						// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::savemsgsRemoveHeaders() msgLen=%d format=%.8s ccsid=%d encoding=%d", msgLen, format, ccsid, encoding);

		// trace entry to savemsgsRemoveHeaders
		logTraceEntry(traceInfo);
	}

	// initialize the format fields
	memset(msgFormat, 0, sizeof(msgFormat));
	memset(dlqFormat, 0, sizeof(dlqFormat));
	memset(rfhFormat, 0, sizeof(rfhFormat));
	memset(cicsFormat, 0, sizeof(cicsFormat));
	memset(imsFormat, 0, sizeof(imsFormat));
	memcpy(dlqFormat, MQFMT_DEAD_LETTER_HEADER, MQ_FORMAT_LENGTH);
	memcpy(rfhFormat, MQFMT_RF_HEADER_2, MQ_FORMAT_LENGTH);
	memcpy(cicsFormat, MQFMT_CICS, MQ_FORMAT_LENGTH);
	memcpy(imsFormat, MQFMT_IMS, MQ_FORMAT_LENGTH);
	Rtrim(dlqFormat);
	Rtrim(rfhFormat);
	Rtrim(cicsFormat);
	Rtrim(imsFormat);

	// get the version field - it is always 4 bytes into the header
	memcpy((void *)&version, msgData + 4, 4);
	version2 = reverseBytes4(version);

	// get the message format field
	// the format field in the MQMD is always translated to ASCII since RFHUtil is running on Windoze
	memset(msgFormat, 0, sizeof(msgFormat));
	memcpy(msgFormat, format, MQ_FORMAT_LENGTH);
//	if (CHAR_EBCDIC == charType)
//	{
//		EbcdicToAscii((unsigned char *)format, 	MQ_FORMAT_LENGTH, (unsigned char *)msgFormat);
//	}
//	else
//	{
//		memcpy(msgFormat, format, MQ_FORMAT_LENGTH);
//	}

	// truncate any trailing blanks
	Rtrim(msgFormat);

	// check if we have a message header
	do
	{
		foundHdr = false;
		hdrLen = 0;

		// get the ccsid and encoding types
		// this will be updated after each subsequent MQ header is found
		encodeType = getIntEncode(encoding);
		charType = getCcsidType(ccsid);

		// remove the header
		// figure out what type of header this is
		if (strcmp(dlqFormat, msgFormat) == 0)
		{
			// found a header
			foundHdr = true;
			updateMQMD = true;
			hdrCount++;

			// get a pointer to the dlq
			dlqPtr = (MQDLH *)msgData;

			// get the new format, code page, encoding and the length of the header
			hdrLen = sizeof(MQDLH);
			if (NUMERIC_PC == encodeType)
			{
				ccsid = dlqPtr->CodedCharSetId;
				encoding = dlqPtr->Encoding;
			}
			else
			{
				ccsid = reverseBytes4(dlqPtr->CodedCharSetId);
				encoding = reverseBytes4(dlqPtr->Encoding);
			}

			// get the message format field
			if (CHAR_EBCDIC == charType)
			{
				EbcdicToAscii((unsigned char *)(&dlqPtr->Format), 	MQ_FORMAT_LENGTH, (unsigned char *)msgFormat);
			}
			else
			{
				memcpy(msgFormat, &dlqPtr->Format, MQ_FORMAT_LENGTH);
			}

			Rtrim(msgFormat);
		}
		else if (strcmp(cicsFormat, msgFormat) == 0)
		{
			// found a header
			foundHdr = true;
			updateMQMD = true;
			hdrCount++;

			// get a pointer to the CIH
			cicsPtr = (MQCIH *)msgData;

			// get the new format, code page, encoding and the length of the header
			if (NUMERIC_PC == encodeType)
			{
				ccsid = cicsPtr->CodedCharSetId;
				encoding = cicsPtr->Encoding;
				hdrLen = cicsPtr->StrucLength;
			}
			else
			{
				ccsid = reverseBytes4(cicsPtr->CodedCharSetId);
				encoding = reverseBytes4(cicsPtr->Encoding);
				hdrLen = reverseBytes4(cicsPtr->StrucLength);
			}

			// get the message format field
			if (CHAR_EBCDIC == charType)
			{
				EbcdicToAscii((unsigned char *)(&cicsPtr->Format), 	MQ_FORMAT_LENGTH, (unsigned char *)msgFormat);
			}
			else
			{
				memcpy(msgFormat, &cicsPtr->Format, MQ_FORMAT_LENGTH);
			}


			Rtrim(msgFormat);
		}
		else if (strcmp(imsFormat, msgFormat) == 0)
		{
			// found a header
			foundHdr = true;
			updateMQMD = true;
			hdrCount++;

			// get a pointer to the IIH
			imsPtr = (MQIIH *)msgData;

			// get the new format, code page, encoding and the length of the header
			if (NUMERIC_PC == encodeType)
			{
				ccsid = imsPtr->CodedCharSetId;
				encoding = imsPtr->Encoding;
				hdrLen = imsPtr->StrucLength;
			}
			else
			{
				ccsid = reverseBytes4(imsPtr->CodedCharSetId);
				encoding = reverseBytes4(imsPtr->Encoding);
				hdrLen = reverseBytes4(imsPtr->StrucLength);
			}

			// get the message format field
			if (CHAR_EBCDIC == charType)
			{
				EbcdicToAscii((unsigned char *)(&imsPtr->Format), 	MQ_FORMAT_LENGTH, (unsigned char *)msgFormat);
			}
			else
			{
				memcpy(msgFormat, &imsPtr->Format, MQ_FORMAT_LENGTH);
			}


			Rtrim(msgFormat);;
		}
		else if (strcmp(rfhFormat, msgFormat) == 0)
		{
			// found a header
			foundHdr = true;
			updateMQMD = true;
			hdrCount++;

			// get a pointer to the RFH
			rfh2Ptr = (MQRFH2 *)msgData;

			// get the new format, code page, encoding and the length of the header
			if (NUMERIC_PC == encodeType)
			{
				ccsid = rfh2Ptr->CodedCharSetId;
				encoding = rfh2Ptr->Encoding;
				hdrLen = rfh2Ptr->StrucLength;
			}
			else
			{
				ccsid = reverseBytes4(rfh2Ptr->CodedCharSetId);
				encoding = reverseBytes4(rfh2Ptr->Encoding);
				hdrLen = reverseBytes4(rfh2Ptr->StrucLength);
			}

			// get the message format field
			if (CHAR_EBCDIC == charType)
			{
				EbcdicToAscii((unsigned char *)(&rfh2Ptr->Format), 	MQ_FORMAT_LENGTH, (unsigned char *)msgFormat);
			}
			else
			{
				memcpy(msgFormat, &rfh2Ptr->Format, MQ_FORMAT_LENGTH);
			}

			Rtrim(msgFormat);;
		}

		// make sure the header length is not invalid
		// the purpose of this check is to avoid obnoxious failures (traps)
		if ((hdrLen > msgLen) || (hdrLen < 0))
		{
			// try to be graceful when the data is invalid
			// just set it to the total remaining length
			hdrLen = msgLen;
			invalidHdr = 1;
		}

		msgLen -= hdrLen;
		msgData += hdrLen;
		totLen += hdrLen;
	} while ((foundHdr) && (msgLen > 8));

	// did we find a header?
	if (updateMQMD)
	{
		// update the MQMD data to refect the user data
		// copy the format without a trailing null character
		// padding with spaces
		memset(mqmd->Format, ' ', MQ_FORMAT_LENGTH);
		memcpy(mqmd->Format, msgFormat, strlen(msgFormat));
		mqmd->CodedCharSetId = ccsid;
		mqmd->Encoding = encoding;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::savemsgsRemoveHeaders() totLen=%d msgLen=%d hdrLen=%d updateMQMD=%d ccsid=%d encoding=%d hdrCount=%d invalidHdr=%d msgFormat=%.8s", totLen, msgLen, hdrLen, updateMQMD, ccsid, encoding, hdrCount, invalidHdr, msgFormat);

		// trace exit from savemsgsRemoveHeaders
		logTraceEntry(traceInfo);
	}

	return totLen;
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to read the next chunk of data when loading data from a file.
//
//  This routine is necessary to avoid running out of memory when the
//  input file is greater than 1GB, which is the maximum amount that can
//  be allocated in a malloc.  As a result the file input buffer is
//  limited to 256 MB.  This can handle any length message since the
//  maximum length of an MQ message is 100 MB.
//
/////////////////////////////////////////////////////////////////////////

int DataArea::readFileChunk(FILE * input, int *remaining, const int offset, unsigned char * buffer, bool firstTime)

{
	int			bytesRead=0;
	int			leftOver=0;
	char		traceInfo[768];				// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::readFileChunk() remaining=%d offset=%d buffer=%8.8X leftOver=%d firstTime=%d", *remaining, offset, (unsigned int)buffer, leftOver, firstTime);

		// trace entry to readFileChunk
		logTraceEntry(traceInfo);
	}

	if (!firstTime)
	{
		leftOver=MAX_BUFFER_SIZE-offset;
	}

	// check for any left over bytes
	if (leftOver > 0)
	{
		// move the left over bytes to the front of the buffer
		memcpy(buffer, buffer + offset, leftOver);
	}

	// figure out how much data to read
	// start with the number of bytes remaining
	bytesRead = (*remaining);

	// will it all fit in the buffer?
	if (bytesRead >= MAX_BUFFER_SIZE - leftOver)
	{
		// too much data left - have to reduce to buffer size
		bytesRead = MAX_BUFFER_SIZE - leftOver;
	}

	// read the data
	fread(buffer + leftOver, 1, bytesRead, input);				// read the contents of the file

	// terminate the data properly
	buffer[bytesRead + leftOver] = 0;							// append a string terminator

	// calculate the number of bytes remaining
	(*remaining) = (*remaining) - bytesRead;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::readFileChunk() remaining=%d bytesRead=%d", (*remaining), bytesRead);

		// trace exit from readFileChunk
		logTraceEntry(traceInfo);
	}

	// return the remaining bytes that have not been read yet
	return bytesRead;
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to load messages previously saved to a
// given queue from one or more files.
//
// Input parameters:
//  fileName - name of the file containing the message data
//  QMname - name of the queue manager to connect to
//  Qname - name of the queue
//  startCount - first message to be loaded - first message
//   is message 1.  Used only for file per message option
//  maxCount - maximum number of messages to load
//  filesType - indicator if single file or file per message
//   is to be used.  If file per message is indicated, then
//   the file name will have the message number appended
//   to the file name and before the file extension.
//  removeMQMD - indicator if MQMDs saved with data are to be removed
//  removeHdrs - indicator if MQ headers are to be removed.
//  useSetAll - indicator if set all context is to be used
//  newMsgId - new message ids are to be generated
//  persistent - persistence of messages to be written
//  delimiter - delimiter string to be used if all messages are
//   stored in a single file.  Ignored if file per message
//   is used.
//  propDelim - delimiter string to separate properties from rest
//   of message data.
//  delimLen - number of bytes in the delimiter string
//  propDelimLen - length of the property delimiter string in bytes
//
/////////////////////////////////////////////////////////////////////////

void DataArea::loadMsgs(LOADPARMS * parms)

{
	MQMDPAGE			*mqmdObj=(MQMDPAGE *)mqmdData;
	MQLONG				cc=MQCC_OK;					// MQ completion code
	MQLONG				rc=MQRC_NONE;				// MQ reason code
	MQLONG				cc2=MQCC_OK;				// MQ completion code
	MQLONG				rc2=MQRC_NONE;				// MQ reason code
	MQLONG				tempOpt;					// temporary options for trace
	int					count=0;					// messages written
	int					uowCount=0;					// messages written in current uow
	int					fileCount=0;				// files read
	int					fileSeq=0;					// number of next file
	int					msgLen;						// work variable - message length
	int					hdrLen=0;					// length of any RFH headers
	int					mqmdLen=0;					// length of any embedded MQMD
	int					remaining=0;				// remaining bytes still to be processed - used when multiple msgs in file
	int					unread=0;					// remaining bytes still to be read from file
	int					bytesLeftInBuffer=0;		// number of bytes in buffer not yet processed
	int					ccsid=0;					// code page identifier
	int					encoding=0;					// encoding value
	int					foundHdr;					// indicator that a header was found in the message
	int					propLen=0;					// length of properties within the message data
	int					propCount=0;				// number of message properties added to message
	int					bufSize=0;					// number of bytes to allocate for a data buffer
	int					bytesRead=0;				// number of bytes read from file in one read
	unsigned char *		fData=NULL;					// pointer to file data used to split out individual messages
	unsigned char *		msgData=NULL;				// pointer to message data within a file
	unsigned char *		delimPtr=NULL;				// pointer to delimiter between messages
	unsigned char *		propDelimPtr=NULL;			// pointer to properties delimiter
	unsigned char *		propPtr=NULL;				// pointer to beginning of properties data
	CRfhutilApp			*app;						// pointer to MFC application object
	FILE				*inputFile;					// file
	BOOL				save_set_all;				// save area for current setting of set all selection on general page
	BOOL				save_set_user;				// save area for current setting of set user id selection on general page
	bool				sleepInd=false;				// indicator if batchsize messages have been written and sleep may be desired
	bool				firstTime=true;				// first pass indicator
	bool				noErr=true;					// error switch - used to terminate processing in event of error
	bool				fileDone=false;				// work variable - indicates we are done with the current file
	bool				done=false;					// indicator used when writing file once
	bool				mallocFailed=false;			// indicator that memory allocation failed
	char				errtxt[256];				// work variable to build messages
	char				newFileName[512];			// file name to open - needed when more than one message per file
	char				currUserId[MQ_USER_ID_LENGTH + 4];	// save area for current user id on MQMD page
	MQHMSG				hMsg=MQHM_UNUSABLE_HMSG;	// message handle used to get message properties
	MQCMHO				opts={MQCMHO_DEFAULT};		// options used to create message handle
	MQDMHO				dOpts={MQDMHO_DEFAULT};		// options used to delete message handle
	MQPMO				pmo={MQPMO_DEFAULT};		// Put message options
	MQMD2				*mqmdPtr=NULL;				// pointer to mqmd - either to MQMD found in file or newMQMD area below
	MQMD2				newMQMD={MQMD2_DEFAULT};	// new MQMD used if embedded MQMDs are not to be used
	MQMD2				defMQMD={MQMD2_DEFAULT};	// MQMD initialized to default values
	char				auditTxt[32];				// work area to build audit message
	char				traceInfo[768];				// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::loadMsgs() qmName=%s qName=%s startFileNumber=%d maxCount=%d writeOnce=%d filesType=%d singleFile=%d removeMQMD=%d removeHdrs=%d waitTime=%d batchSize=%d propertiesSupported=%d ignoreProps=%d fileName=%.256s", parms->qmName, parms->qName, parms->startFileNumber, parms->maxCount, parms->writeOnce, parms->filesType, parms->singleFile, parms->removeMQMD, parms->removeHdrs, parms->waitTime, parms->batchSize, propertiesSupported, parms->ignoreProps, parms->fileName);

		// trace entry to loadMsgs
		logTraceEntry(traceInfo);
	}

	// initialize work areas
	memset(newFileName, 0, sizeof(newFileName));
	memset(currUserId, 0, sizeof(currUserId));

	// clear the current message variable in the DataArea object
	m_error_msg.Empty();

	// make sure we have a queue name
	if (strlen(parms->qName) == 0)
	{
		// queue name is missing - set error message and exit
		m_error_msg = "*Queue Name required* ";
		return;
	}

	// check for no file name
	if (0 == parms->fileName[0])
	{
		// file name is missing - set error message and exit
		m_error_msg = "*File name required* ";
		return;
	}

	// save the current setting for m_set_all and m_set_user
	save_set_all = m_set_all;
	save_set_user = m_setUserID;

	if ((SAVE_FILE_DIFF_FILES == parms->filesType) && (parms->startFileNumber > 0))
	{
		// create the name of the first file to read
		createNextFileName(parms->fileName, newFileName, parms->startFileNumber);
	}
	else
	{
		// only one file - no need to append anything
		strcpy(newFileName, parms->fileName);
	}

	// try to open the first input file
	inputFile = fopen(newFileName, "rb");
	if (inputFile == NULL)
	{
		m_error_msg.Format("Unable to open input file %s", newFileName);
	}
	else
	{
		// increment the number of files read
		fileCount++;
		fileSeq++;

		// connect to the queue manager
		if (!checkConnection(parms->qmName))
		{
			// can't connect to the queue manager
			// close the file and return
			fclose(inputFile);
			return;
		}

		// see if we need to set all context
		if (parms->removeMQMD || (!(parms->useSetAll)))
		{
			// do not need set all authority
			m_set_all = FALSE;
			m_setUserID = FALSE;
		}
		else
		{
			// we want to open the queue with set all authority
			// messages that have an MQMD stored with them will be written with a set all context option
			m_set_all = TRUE;
			m_setUserID = FALSE;
		}

		// try to open the queue
		if (!openQ(parms->qName, parms->RemoteQM, Q_OPEN_WRITE, FALSE))
		{
			// didn't work
			// close the file, disconnect from the queue manager and return
			fclose(inputFile);
			discQM();

			// restore the set all and set user id settings
			m_set_all = save_set_all;
			m_setUserID = save_set_user;

			return;
		}

		while ((noErr) && (!done) && (inputFile != NULL) && ((0 == parms->maxCount) || (count < parms->maxCount)))
		{
			// has the data been read?
			if (firstTime || (SAVE_FILE_DIFF_FILES == parms->filesType))
			{
				// determine the length of the data and read it
				// it into memory, up to a maximum of 256 MB
				fseek(inputFile, 0L, SEEK_END);						// move to the end of the file
				remaining = ftell(inputFile);						// get the offset which is the length of the file
				bufSize = remaining;									// size of buffer to allocate - will be file size unless the file is really big
				unread = remaining;									// total number of bytes in the file
				fseek(inputFile, 0L, SEEK_SET);						// move back to the beginning of the file

				// check if the file is greater than 256 MB
				if (bufSize > MAX_BUFFER_SIZE)
				{
					// limit the size of the buffer allocation to 256 MB
					bufSize = MAX_BUFFER_SIZE;
				}

				// check if this is a repeat for the same file
				if (NULL == fData)
				{
					// allocate a buffer to hold the entire file or 256 MB
					fData = (unsigned char *) rfhMalloc(bufSize + 32, "LOADFILE");
				}

				// make sure that the malloc worked
				if (fData != NULL)
				{
					// clear the input buffer
					memset(fData, 0, bufSize + 32);

					// read up to bufSize bytes
					bytesRead = readFileChunk(inputFile, &unread, bufSize, fData, true);
					bytesLeftInBuffer = bytesRead;
				}
				else
				{
					// the malloc failed - remember the error
					noErr = false;
					mallocFailed = true;
				}

				// is trace enabled?
				if (traceEnabled && verboseTrace)
				{
					// create the trace line
					sprintf(traceInfo, " remaining=%d bufSize=%d unread=%d bytesRead=%d bytesLeftInBuffer=%d fData=%8.8X count=%d mallocFailed=%d fileDone=%d newFileName=%.256s", remaining, bufSize, unread, bytesRead, bytesLeftInBuffer, (unsigned int)fData, count, mallocFailed, fileDone, newFileName);

					// trace entry to loadMsgs
					logTraceEntry(traceInfo);
				}

				// only force this once
				firstTime = false;
			}

			// initialize some variables before entering the main loop to process the file
			fileDone = false;
			msgData = fData;
			msgLen = bytesRead;

			// process the messages in this file
			while (!fileDone && noErr)
			{
				// initialize variable
				foundHdr = 0;

				// initialize put options
				//pmo.Options = MQPMO_FAIL_IF_QUIESCING | MQPMO_NO_SYNCPOINT;
				pmo.Options = MQPMO_FAIL_IF_QUIESCING | MQPMO_SYNCPOINT;

				// check for a delimiter between the messages
				if (parms->delimLen > 0)
				{
					// scan for a delimiter sequence
					delimPtr = scanForDelim(msgData, msgLen, parms->delimiter, parms->delimLen);

					// delimiter found?
					if (delimPtr != NULL)
					{
						// reduce the message length
						msgLen = delimPtr - msgData;
					}
					else
					{
						// check if all the data has been read from the file
						if (unread > 0)
						{
							// not all of the data has been read so read the next chunk
							// any residual data will be moved to the front of the buffer
							bytesRead = readFileChunk(inputFile, &unread, bufSize - msgLen, fData, false);

							// calculate the new end of the data and point to the beginning of the buffer
							msgData = fData;
							msgLen += bytesRead;
							bytesLeftInBuffer += bytesRead;

							// check for a delimiter again
							delimPtr = scanForDelim(msgData, msgLen, parms->delimiter, parms->delimLen);

							if (delimPtr != NULL)
							{
								// reduce the message length to the actual message length
								msgLen = delimPtr - msgData;
							}
						}
					}
				}

				// set the properties length to 0 to avoid trying to delete a handle that was not created
				propLen = 0;

				// no message handle
				pmo.OriginalMsgHandle = MQHM_NONE;

				// check for a properties delimiter
				if (parms->propDelimLen > 0)
				{
					// scan for a delimiter sequence
					propDelimPtr = scanForDelim(msgData, msgLen, parms->propDelim, parms->propDelimLen);

					// were any properties stored with the data?
					if (propDelimPtr != NULL)
					{
						// calculate the length of the properties and point to the beginning of the message properties
						propLen = propDelimPtr - msgData;
						propPtr = msgData;

						// move past the message properties and adjust the data length of this message
						msgData += propLen + parms->propDelimLen;
						msgLen -= propLen + parms->propDelimLen;
						remaining -= propLen + parms->propDelimLen;
						bytesLeftInBuffer -= propLen + parms->propDelimLen;

						// are properties supported by this version of MQ?
						if (propertiesSupported && !parms->ignoreProps)
						{
							// create a message handle
							// make sure that the message handle processing options are honored
							pmo.Version = MQPMO_VERSION_3;

							// set the message handle options
							opts.Options = MQCMHO_VALIDATE;

							// create a message handle
							XMQCrtMh(qm, &opts, &hMsg, &cc2, &rc2);

							if (traceEnabled)
							{
								// enter the result in the trace
								sprintf(traceInfo, " MQCrtMh cc2=%d rc2=%d hMsg=%8.8X", cc2, rc2, (unsigned int)hMsg);

								// write a trace entry
								logTraceEntry(traceInfo);
							}

							// check if it worked
							if (cc2 != MQCC_OK)
							{
								// unable to create the message handle - report the error
								setErrorMsg(cc2, rc2, "MQCRTMH");

								// return completion code
								return;
							}
							else
							{
								// load the message properties into the message handle
								propCount = findMsgProperties(hMsg, propPtr, propLen);

								// were any properties added?
								if (propCount > 0)
								{
									// set the handle in the GMO
									pmo.OriginalMsgHandle = hMsg;
								}
							}
						}
					}
				}

				// check for an MQMD in the file data
				mqmdLen = mqmdObj->getMQMD(msgData, msgLen, &ccsid, &encoding);

				// reset the default MQMD
				memcpy(&newMQMD, &defMQMD, sizeof(newMQMD));

				if (mqmdLen > 0)
				{
					// should we remove it?
					if (parms->removeMQMD)
					{
						mqmdPtr = &newMQMD;

						// set specified default ccsid and codepage
						mqmdPtr->CodedCharSetId = parms->defaultCcsid;
						mqmdPtr->Encoding = parms->defaultEncoding;
						memcpy(&mqmdPtr->Format, parms->format, MQ_FORMAT_LENGTH);

						// check for an MQ header, so we can set the format, ccsid and encoding to match
						foundHdr = checkForMQhdrs(msgData + mqmdLen, msgLen - mqmdLen, (char *)&mqmdPtr->Format, (int *)&mqmdPtr->CodedCharSetId, (int *)&mqmdPtr->Encoding);

						if (parms->persistent)
						{
							mqmdPtr->Persistence = MQPER_PERSISTENT;
						}
					}
					else
					{
						mqmdPtr = (MQMD2 *)msgData;

						// should set all be used?
						if (parms->useSetAll)
						{
							// use set all context option
							pmo.Options = MQPMO_FAIL_IF_QUIESCING | MQPMO_NO_SYNCPOINT | MQPMO_SET_ALL_CONTEXT;

							// new message id?
							if (parms->newMsgId)
							{
								if ((platform != MQPL_MVS) || (level >= MQCMDL_LEVEL_600))
								{
									// use PMO option
									pmo.Options |= MQPMO_NEW_MSG_ID;
								}
								else
								{
									// check if we are using MVS and the level is less than V6
									if (parms->newMsgId)
									{
										// cannot use the PMO option
										// clear the message id in the MQMD
										memcpy(mqmdPtr->MsgId, MQMI_NONE, sizeof(mqmdPtr->MsgId));
									}
								}
							}
						}
					}

					// found an MQMD
					msgData += mqmdLen;
					remaining -= mqmdLen;
					bytesLeftInBuffer -= mqmdLen;
					msgLen -= mqmdLen;
				}
				else
				{
					// no MQMD found
					mqmdPtr = &newMQMD;

					// check for an MQ header, so we can set the format, ccsid and encoding to match
					foundHdr = checkForMQhdrs(msgData, msgLen, (char *)&mqmdPtr->Format, (int *)&mqmdPtr->CodedCharSetId, (int *)&mqmdPtr->Encoding);

					if (0 == foundHdr)
					{
						// set default ccsid, codepage and format
						mqmdPtr->CodedCharSetId = parms->defaultCcsid;
						mqmdPtr->Encoding = parms->defaultEncoding;
						memcpy(&mqmdPtr->Format, parms->format, MQ_FORMAT_LENGTH);
					}

					// no MQMD, so see if we need to override the persistence setting
					if (parms->persistent)
					{
						mqmdPtr->Persistence = MQPER_PERSISTENT;
					}
				}

				// check if headers are to be removed
				if (parms->removeHdrs)
				{
					// remove any MQ headers that we recognize
					// update the MQMD format, ccsid and encoding
					hdrLen = this->stripMQhdrs(msgData, msgLen, (char *)&mqmdPtr->Format, (int *)&mqmdPtr->CodedCharSetId, (int *)&mqmdPtr->Encoding);
					msgData += hdrLen;
					msgLen -= hdrLen;
					remaining -= hdrLen;
					bytesLeftInBuffer -= hdrLen;
				}

				// write the message to the queue
				XMQPut(qm, q, mqmdPtr, &pmo, msgLen, msgData, &cc, &rc);

				if (traceEnabled && verboseTrace)
				{
					// turn the MQ PMO options into a hex string
					memset(auditTxt, 0, sizeof(auditTxt));
					tempOpt = reverseBytes4(pmo.Options);
					AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)auditTxt);

					// create the trace line
					sprintf(traceInfo, " loadMsgs() MQPUT cc=%d rc=%d pmo.Options=X\'%s\' msgLen=%d uowCount=%d", cc, rc, auditTxt, msgLen, uowCount);

					// trace entry to loadMsgs
					logTraceEntry(traceInfo);
				}

				// are properties supported and were any properties found?
				if (propertiesSupported && (parms->propDelimLen > 0) && (hMsg != MQHM_UNUSABLE_HMSG) && (propLen > 0))
				{
					// delete the message handle
					XMQDltMh(qm, &hMsg, &dOpts, &cc2, &rc2);
					hMsg=MQHM_UNUSABLE_HMSG;

					// check if it worked
					if (cc2 != MQCC_OK)
					{
						// unable to delete the message handle - report the error
						setErrorMsg(cc2, rc2, "MQDLTMH");
					}
				}

				// check if it worked
				if (cc != MQCC_OK)
				{
					noErr = false;

					// put failed, get the reason and generate an error message
					// get the completion code and reason code
					setErrorMsg(cc, rc, "Put");
				}

				// check for error writing message
				if (noErr)
				{
					// count the number of messages written
					count++;
					uowCount++;
					sleepInd = false;

					// see if we need to do a commit
					if ((uowCount == maxUOW - 1) || ((parms->batchSize > 0) && (uowCount > parms->batchSize)))
					{
						// issue a commit and reset the counter
						commitUOW(true);
						uowCount = 0;
						sleepInd = true;
					}

					// finished a batch?
					if ((parms->waitTime > 0) && (parms->batchSize > 0) && sleepInd)
					{
						// sleep for waitTime milliseconds
						Sleep(parms->waitTime);
					}
				}

				// move on to the next message in the file
				msgData += msgLen;
				remaining -= msgLen;
				bytesLeftInBuffer -= msgLen;
				msgLen = bytesLeftInBuffer;

				// did we find a delimiter?
				if (delimPtr != NULL)
				{
					// skip the delimiter
					msgData += parms->delimLen;
					msgLen -= parms->delimLen;
					remaining -= parms->delimLen;
					bytesLeftInBuffer -= parms->delimLen;
				}

				// check if at end of current file or parms->maxCount messages have been written
				if ((remaining <= 0) || ((parms->maxCount > 0) && (count >= parms->maxCount)))
				{
					// finished with this file
					fileDone = true;
				}
			}

			// check if maximum count has been reached or writing a single file once
			// in either case the process is done
			if (((parms->maxCount > 0) && (count >= parms->maxCount)) || ((parms->writeOnce) && (parms->singleFile)))
			{
				// close the current file
				fclose(inputFile);
				inputFile = NULL;

				// is there any storage to free?
				if (fData != NULL)
				{
					// done with the file - free the storage
					rfhFree(fData);
				}

				// clear the pointers
				fData = NULL;
				msgData = NULL;

				// enough messages so exit
				done = true;
			}
			else
			{
				// check if single file has been specified
				if (parms->singleFile)
				{
					// only using a single file
					// read the data again
					firstTime = true;
				}
				else
				{
					// close the current file
					fclose(inputFile);
					inputFile = NULL;

					// is there any storage to free?
					if (fData != NULL)
					{
						// done with the file - free the storage
						rfhFree(fData);
					}

					// clear the pointers
					fData = NULL;
					msgData = NULL;

					// look for another file in sequence
					// create the name of the next file to read
					createNextFileName(parms->fileName, newFileName, fileSeq + parms->startFileNumber);

					// try to open the next input file
					inputFile = fopen(newFileName, "rb");

					// was the open successful?
					if (inputFile != NULL)
					{
						// count the number of files we have used
						fileCount++;
						fileSeq++;

						// read the data
						firstTime = true;
					}
					else
					{
						// no more files in sequence, so check if writing only once
						if (parms->writeOnce)
						{
							// all messages written once - process is done
							// just exit
							done = true;
						}
						else
						{
							// wrap around to first file
							strcpy(newFileName, parms->fileName);
							inputFile = fopen(newFileName, "rb");

							// check if it worked
							if (NULL == inputFile)
							{
								// should not get here but just in case
								done = true;
							}
							else
							{
								// read the data again
								firstTime = true;

								// reset the file sequence number to the beginning
								fileSeq = 1;
							}
						}
					}
				}
			}

			// check for verbose trace
			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, " loadMsgs() count=%d fileCount=%d fileSeq=%d msgLen=%d propCount=%d bufSize=%d bytesRead=%d remaining=%d bytesLeftInBuffer=%d unread=%d", count, fileCount, fileSeq, msgLen, propCount, bufSize, bytesRead, remaining, bytesLeftInBuffer, unread);

				// trace the message
				logTraceEntry(traceInfo);
			}
		}

		// any leftover messages to commit?
		if (uowCount > 0)
		{
			// commit the remaining messages that were written
			commitUOW(true);
		}

		// check if the file needs to be closed
		if (inputFile != NULL)
		{
			// close the input file
			fclose(inputFile);									// close the input file
			inputFile = NULL;									// set the file handle to NULL since the file is now closed
		}

		// update the queue depth
		getCurrentDepth();

		// done - close the queue
		closeQ(Q_CLOSE_NONE);
	}

	// check for a memory allocation failure
	if (mallocFailed)
	{
		// display error message
		appendError("*****Memory allocation failed");
		updateMsgText();
	}

	// tell the user what was done
	if (1 == fileCount)
	{
		sprintf(errtxt, "%d messages added from %.192s", count, parms->fileName);
	}
	else
	{
		sprintf(errtxt, "%d messages added from %d files", count, fileCount);
	}

	// append the message to the message display
	appendError(errtxt);

	// call the audit routine to record the loadMsgs action
	sprintf(auditTxt, "SaveMsg %d msgs", count);
	app = (CRfhutilApp *)AfxGetApp();
	app->createAuditRecord((LPCTSTR)currentQM, (LPCTSTR)currentQ, auditTxt, rc);

	// restore the previous settings of set all and set user id
	m_set_all = save_set_all;
	m_setUserID = save_set_user;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::loadMsgs() count=%d fileCount=%d propCount=%d bufSize=%d", count, fileCount, propCount, bufSize);

		// trace exit from loadMsgs
		logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to scan for a properties delimiter in the data read and
// process the message properties if the delimiter is found.
//
// A count of the number of properties inserted into the handle
// is returned.
//
/////////////////////////////////////////////////////////////////////////

int DataArea::findMsgProperties(MQHMSG hMsg, unsigned char *propData, int propLen)

{
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=MQRC_NONE;					// MQ reason code
	int				count=0;						// number of properties added to message
	int				strCount=0;						// number of string properties
	int				i64count=0;						// number of 64-bitinteger properties
	int				i32count=0;						// number of 32-bitinteger properties
	int				i16count=0;						// number of 16-bitinteger properties
	int				i8count=0;						// number of 8-bitinteger properties
	int				fp4count=0;						// number of short floating point values
	int				fp8count=0;						// number of long floating point values
	int				byteCount=0;					// number of byte array values
	int				nullCount=0;					// number of null values
	int				defCount=0;						// number of unrecognized values
	int				valueLen = 0;					// length of the value to be added to the message handle
	int				type;							// MQ property type
	int				i4;								// 32-bit floating point value
	float			fp4;							// 32-bit floating point value
	double			fp8;							// 64-bit floating point value
	_int64			i64;							// 64-bit integer value
	const char		*propPtr;						// pointer within the message
	const char		*propEnd;						// end of current property (LF)
	const char		*namePtr;						// beginning of property name
	const char		*endName;						// end of property name (first character after name - should be '=')
	const char		*valuePtr;						// pointer to the value (after the '=' character)
	const char		*delimPtr;						// pointer to end of property data
	void			*allocPtr=NULL;					// pointer to allocated storage (NULL if no storage allocated)
	void			*ptr;							// pointer to converted value
	MQSMPO			setOpts={MQSMPO_DEFAULT};		// options for set message properties
	MQPD			pd={MQPD_DEFAULT};				// property description
	MQCHARV			propName={MQCHARV_DEFAULT};		// MQCHARV structure to hold property name
	char			value[2048];					// work area to build a hex value without using a malloc and free
	char			traceInfo[512];					// work variable to build trace message

	// check if trace is enabled
	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::findMsgProperties() hMsg=%8.8X hMsg, propLen=%d", (unsigned int)hMsg, propLen);

		// trace entry to findMsgProperties
		logTraceEntry(traceInfo);

		// check if verbose trace is enabled
		if (traceEnabled && verboseTrace && (propData != NULL))
		{
			// dump out the properties and the delimiter
			dumpTraceData("propData", propData, propLen);
		}
	}

	// was a properties pointer given?
	if (propData != NULL)
	{
		// find each individual property
		// set pointers to the beginning and end of the properties data
		delimPtr = (char *)propData + propLen;
		propPtr = skipWhiteSpace((char *)propData, delimPtr);

		// start the main loop
		while ((propPtr < delimPtr) && (MQCC_OK == cc))
		{
			// find the end of this property
			propEnd = findPropEnd(propPtr, delimPtr);

			// point to the name
			namePtr = propPtr;

			// find the end of the name
			while ((propPtr < propEnd) && (propPtr[0] != '=') && (propPtr[0] != '('))
			{
				// move on to the next character
				propPtr++;
			}

			// get the end of the name
			endName = propPtr;

			// set a default type
			type = MQTYPE_STRING;

			// confirm that the next characters are dt=
			if ((propPtr + 3 < propEnd) && (memcmp(propPtr, "(dt=", 4) == 0))
			{
				// skip the dt= characters
				propPtr += 4;

				// check for a beginning quote
				if ((propPtr < propEnd) && (('\'' == propPtr[0]) || ('\"' == propPtr[0])))
				{
					// skip the quote
					propPtr++;
				}

				// get the data type
				type = getPropType(propPtr, propEnd);

				// find the end of the type by looking for the equal sign
				while ((propPtr < propEnd) && (propPtr[0] != '='))
				{
					// skip the next character
					propPtr++;
				}
			}
			else if ((propPtr + 5 < propEnd) && (memcmp(propPtr, "(xsi:nil=", 9) == 0))
			{
				// set the data type to NULL
				type = MQTYPE_NULL;

				// find the end of the type by looking for the equal sign
				while ((propPtr < propEnd) && (propPtr[0] != '='))
				{
					// skip the next character
					propPtr++;
				}
			}

			// check if the equal sign was found
			if ((propPtr < propEnd) && ('=' == propPtr[0]))
			{
				// point to the value
				valuePtr = propPtr + 1;

				// check if this is a floating point or integer request
				if ((MQTYPE_FLOAT64 == type) || (MQTYPE_FLOAT32 == type) || (MQTYPE_INT64 == type) || (MQTYPE_INT32 == type) || (MQTYPE_INT16 == type) || (MQTYPE_INT8 == type))
				{
					// copy up to 32 bytes into a temporary area so the string can be terminated
					valueLen = 0;
					while ((valuePtr + valueLen < propEnd) && (valueLen < 32))
					{
						value[valueLen] = valuePtr[valueLen];
						valueLen++;
					}

					// terminate the string
					value[valueLen] = 0;
				}
				else
				{
					value[0] = 0;
				}

				// figure what the value represents
				switch (type)
				{
				case MQTYPE_STRING:
					{
						ptr = (void *)valuePtr;
						valueLen = propEnd - valuePtr;
						strCount++;
						break;
					}
				case MQTYPE_INT8:
					{
						i4 = atoi(value);
						ptr = (void *)&i4;
						valueLen = sizeof(MQBYTE);
						i8count++;
						break;
					}
				case MQTYPE_INT16:
					{
						i4 = atoi(value);
						ptr = (void *)&i4;
						valueLen = sizeof(MQINT16);
						i16count++;
						break;
					}
				case MQTYPE_INT32:
					{
						i4 = atoi(value);
						ptr = (void *)&i4;
						valueLen = sizeof(MQINT32);
						i32count++;
						break;
					}
				case MQTYPE_INT64:
					{
						i64 = _atoi64(value);
						ptr = (void *)&i64;
						valueLen = sizeof(MQINT64);
						i64count++;
						break;
					}
				case MQTYPE_FLOAT32:
					{
						fp8 = atof(value);
						fp4 = (float)fp8;
						ptr = (void *)&fp4;
						valueLen = sizeof(MQFLOAT32);
						fp4count++;
						break;
					}
				case MQTYPE_FLOAT64:
					{
						fp8 = atof(value);
						ptr = (void *)&fp8;
						valueLen = sizeof(MQFLOAT64);
						fp8count++;
						break;
					}
				case MQTYPE_NULL:
					{
						value[0] = 0;
						valueLen = 0;
						ptr = value;
						nullCount++;
						break;
					}
				case MQTYPE_BYTE_STRING:
					{
						// get the length
						valueLen = (propEnd - valuePtr) >> 1;

						if (valueLen > sizeof(value))
						{
							// too big for the work area on the stack
							// have to allocate storage
							allocPtr = rfhMalloc(valueLen + 8, "BYTEPROP");
							ptr = allocPtr;
							HexToAscii((unsigned char *)valuePtr, valueLen, (unsigned char *)ptr);
						}
						else
						{
							// get the value into a buffer
							HexToAscii((unsigned char *)valuePtr, valueLen, (unsigned char *)value);
							ptr = (void *)value;
						}

						byteCount++;
						break;
					}
				default:
					{
						// don't know what this is - treat as a string
						ptr = (void *)valuePtr;
						valueLen = propEnd - valuePtr;
						defCount++;
						break;
					}
				}  // switch

				// set the property name
				propName.VSPtr = (void *)namePtr;
				propName.VSLength = endName - namePtr;

				// add the property to the message handle
				XMQSetMp(qm, hMsg, &setOpts, &propName, &pd,  type, valueLen, ptr, &cc, &rc);

				// get rid of any acquired storage
				if (allocPtr != NULL)
				{
					// free the acquired storage
					rfhFree(allocPtr);
					allocPtr = NULL;
				}

				// check the return code
				if (cc != MQCC_OK)
				{
					// issue an error message
					// the completion code will stop the loop
					setErrorMsg(cc, rc, "MQSETMP");
					updateMsgText();
				}
				else
				{
					// increase the count of the number of properties added
					count++;
				}
			}

			// move on to the next property, skipping the LF character
			propPtr = propEnd + 1;
		} // while - main loop
	} // if

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "strCount=%d i64count=%d i32count=%d i16count=%d i8count=%d fp4count=%d fp8count=%d byteCount=%d nullCount=%d unrecognized=%d", strCount, i64count, i32count, i16count, i8count, fp4count, fp8count, byteCount, nullCount, defCount);

		// trace the individual counts
		logTraceEntry(traceInfo);

		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::findMsgProperties() propLen=%d count=%d cc=%d rc=%d", propLen, count, cc, rc);

		// trace exit from findMsgProperties
		logTraceEntry(traceInfo);
	}

	return count;
}

////////////////////////////////////
//
// Convert a string in the data file
// to an MQ property data type.
//
////////////////////////////////////

int DataArea::getPropType(const char *propPtr, const char * endProp)

{
	int		type=MQTYPE_STRING;
	int		len=0;
	char	propStr[16];

	// initialize the property type string
	memset(propStr, 0, sizeof(propStr));

	// capture the property type as a string
	while ((propPtr < endProp) && (len < sizeof(propStr) - 1) && (propPtr[0] != '\'') && (propPtr[0] != '\"') && (propPtr[0] != ')') && (propPtr[0] != '='))
	{
		// capture the next character
		propStr[len++] = propPtr++[0];
	}

	// convert the string to lower case for comparisons
	_strlwr(propStr);

	type = ((CProps *)propData)->propTypeToInt(propStr);

	return type;
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to scan for a delimiter in the data read from a file.
//
/////////////////////////////////////////////////////////////////////////

unsigned char * DataArea::scanForDelim(unsigned char *msgData, int msgLen, const char *delimiter, int delimLen)

{
	int					remaining;
	unsigned char *		delimPtr=NULL;
	boolean				notDone=true;

	// check that we have a delimiter and that we have enough characters to check
	if ((0 == delimLen) || (msgLen < delimLen))
	{
		return NULL;
	}

	// get the maximum number of characters to scan
	remaining = msgLen - delimLen;

	while ((remaining > 0) && notDone)
	{
		if (delimiter[0] != msgData[0])
		{
			msgData++;
			remaining--;
		}
		else
		{
			// check if the whole delimiter matches
			if (memcmp(delimiter, msgData, delimLen) == 0)
			{
				notDone = false;
			}
			else
			{
				msgData++;
				remaining--;
			}
		}
	}

	if (memcmp(delimiter, msgData, delimLen) == 0)
	{
		delimPtr = msgData;
	}

	return delimPtr;
}

const char * DataArea::findPropEnd(const char *propPtr, const char *endPtr)

{
	// search for a LF character
	while ((propPtr < endPtr) && (propPtr[0] != '\n') && (propPtr[0] != '\r') && (propPtr[0] != 0))
	{
		// move on to the next character
		propPtr++;
	}

	return propPtr;
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to look for MQ headers in data read from a file.
//
/////////////////////////////////////////////////////////////////////////

int DataArea::checkForMQhdrs(unsigned char *msgData, int msgLen, char *format, int *ccsid, int *encoding)

{
	int		foundHdr=0;
	MQDLH	*dlqptr;
	int		tempVer;
	int		tempVer2;
	char	tempId[8];
	char	tempId2[8];
	char		traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::checkForMQhdrs() msgLen=%d format=%.8s ccsid=%d encoding=%d", msgLen, format, *ccsid, *encoding);

		// trace entry to checkForMQhdrs
		logTraceEntry(traceInfo);
	}

	// initialize work areas
	memset(tempId, 0, sizeof(tempId));
	memset(tempId2, 0, sizeof(tempId2));

	// get the header and version number in both possible formats
	dlqptr = (MQDLH *)msgData;
	memcpy(tempId, dlqptr->StrucId, sizeof(dlqptr->StrucId));
	memcpy(&tempVer, &dlqptr->Version, sizeof(dlqptr->Version));

	// now try the opposite encoding and character set
	tempVer2 = reverseBytes4(tempVer);
	EbcdicToAscii((unsigned char *)tempId, 4, (unsigned char *)&tempId2);

	// check for a DLQ header
	// do we have enough data?
	if (msgLen >= sizeof(MQDLH))
	{
		// does either structure id match?
		if ((memcmp(tempId, MQDLH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQDLH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQDLH_VERSION_1) || (tempVer2 == MQDLH_VERSION_1))
			{
				// it passes the test - assume this is a dead letter queue header
				// set the mqmd format field
				memcpy(format, MQFMT_DEAD_LETTER_HEADER, MQ_FORMAT_LENGTH);

				// set the character set and type
				if (memcmp(tempId, MQDLH_STRUC_ID, 4) == 0)
				{
					// looks like data is in ASCII
					*ccsid = DEF_ASCII_CCSID;
				}
				else
				{
					// looks like data is in EBCDIC
					*ccsid = DEF_EBCDIC_CCSID;
				}

				// set the encoding
				if (tempVer == MQDLH_VERSION_1)
				{
					// looks like little endian encoding (PC)
					*encoding = DEF_PC_ENCODING;
				}
				else
				{
					// looks like big endian encoding (assume 390)
					*encoding = DEF_HOST_ENCODING;
				}
			}
		}
	}

	// check for a CICS header
	// do we have enough data?
	if (msgLen >= MQCIH_LENGTH_1 + 8)
	{
		// does either structure id match?
		if ((memcmp(tempId, MQCIH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQCIH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQCIH_VERSION_1) || (tempVer2 == MQCIH_VERSION_1) ||(tempVer == MQCIH_VERSION_2) || (tempVer2 == MQCIH_VERSION_2))
			{
				// it passes the test - assume this is an CICS header
				foundHdr = 1;

				// set the mqmd format field
				memcpy(format, MQFMT_CICS, MQ_FORMAT_LENGTH);

				// set the character set and type
				if (memcmp(tempId, MQCIH_STRUC_ID, 4) == 0)
				{
					// looks like data is in ASCII
					*ccsid = DEF_ASCII_CCSID;
				}
				else
				{
					// looks like data is in EBCDIC
					*ccsid = DEF_EBCDIC_CCSID;
				}

				// set the encoding
				if ((tempVer == MQCIH_VERSION_1) || (tempVer == MQCIH_VERSION_2))
				{
					// looks like little endian encoding (PC)
					*encoding = DEF_PC_ENCODING;
				}
				else
				{
					// looks like big endian encoding (assume 390)
					*encoding = DEF_HOST_ENCODING;
				}
			}
		}
	}

	// check for a IMS header
	// do we have enough data?
	if (msgLen >= MQIIH_LENGTH_1)
	{
		// does either structure id match?
		if ((memcmp(tempId, MQIIH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQIIH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQIIH_VERSION_1) || (tempVer2 == MQIIH_VERSION_1))
			{
				// it passes the test - assume this is an CICS header
				foundHdr = 1;

				// set the mqmd format field
				memcpy(format, MQFMT_IMS, MQ_FORMAT_LENGTH);

				// set the character set and type
				if (memcmp(tempId, MQIIH_STRUC_ID, 4) == 0)
				{
					// looks like data is in ASCII
					*ccsid = DEF_ASCII_CCSID;
				}
				else
				{
					// looks like data is in EBCDIC
					*ccsid = DEF_EBCDIC_CCSID;
				}

				// set the encoding
				if (tempVer == MQIIH_VERSION_1)
				{
					// looks like little endian encoding (PC)
					*encoding = DEF_PC_ENCODING;
				}
				else
				{
					// looks like big endian encoding (assume 390)
					*encoding = DEF_HOST_ENCODING;
				}
			}
		}
	}

	// check for an RFH1 header
	if (msgLen >= MQRFH_STRUC_LENGTH_FIXED)
	{
		// does either structure id match?
		if ((memcmp(tempId, MQRFH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQRFH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQRFH_VERSION_1) || (tempVer2 == MQRFH_VERSION_1))
			{
				// it passes the test - assume this is an RFH1 header
				foundHdr = 1;

				// set the mqmd format field
				memcpy(format, MQFMT_RF_HEADER, MQ_FORMAT_LENGTH);

				// set the character set and type
				if (memcmp(tempId, MQRFH_STRUC_ID, 4) == 0)
				{
					// looks like data is in ASCII
					*ccsid = DEF_ASCII_CCSID;
				}
				else
				{
					// looks like data is in EBCDIC
					*ccsid = DEF_EBCDIC_CCSID;
				}

				// set the encoding
				if (tempVer == MQRFH_VERSION_1)
				{
					// looks like little endian encoding (PC)
					*encoding = DEF_PC_ENCODING;
				}
				else
				{
					// looks like big endian encoding (assume 390)
					*encoding = DEF_HOST_ENCODING;
				}
			}
		}
	}

	// check for an RFH2 header
	if (msgLen >= MQRFH_STRUC_LENGTH_FIXED_2)
	{
		// does either structure id match?
		if ((memcmp(tempId, MQRFH_STRUC_ID, 4) == 0) || (memcmp(tempId2, MQRFH_STRUC_ID, 4) == 0))
		{
			// does either version match?
			if ((tempVer == MQRFH_VERSION_2) || (tempVer2 == MQRFH_VERSION_2))
			{
				// it passes the test - assume this is an RFH2 header
				foundHdr = 1;

				// set the mqmd format field
				memcpy(format, MQFMT_RF_HEADER_2, MQ_FORMAT_LENGTH);

				// set the character set and type
				if (memcmp(tempId, MQRFH_STRUC_ID, 4) == 0)
				{
					// looks like data is in ASCII
					*ccsid = DEF_ASCII_CCSID;
				}
				else
				{
					// looks like data is in EBCDIC
					*ccsid = DEF_EBCDIC_CCSID;
				}

				// set the encoding
				if (tempVer == MQRFH_VERSION_2)
				{
					// looks like little endian encoding (PC)
					*encoding = DEF_PC_ENCODING;
				}
				else
				{
					// looks like big endian encoding (assume 390)
					*encoding = DEF_HOST_ENCODING;
				}
			}
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::checkForMQhdrs() foundHdr=%d format=%.8s ccsid=%d encoding=%d", foundHdr, format, (*ccsid), (*encoding));

		// trace exit from checkForMQhdrs
		logTraceEntry(traceInfo);
	}

	return foundHdr;
}

/////////////////////////////////////////////////////////////////////////
//
// Routine to remove MQ headers from data read from a file.
//
/////////////////////////////////////////////////////////////////////////

int DataArea::stripMQhdrs(unsigned char *msgData, int msgLen, char *format, int *ccsid, int *encoding)

{
	int			hdrLen;
	int			totLen=0;
	int			charType;
	int			encodeType;
	MQDLH *		dlqPtr;
	MQCIH *		cicsPtr;
	MQIIH *		imsPtr;
	MQRFH2 *	rfh2Ptr;
	char		traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::stripMQhdrs() msgLen=%d format=%s ccsid=%d encoding=%d", msgLen, format, *ccsid, *encoding);

		// trace entry to stripMQhdrs
		logTraceEntry(traceInfo);
	}

	// get the initial values for encoding and character set types
	encodeType = getIntEncode(*encoding);
	charType = getCcsidType(*ccsid);

	// check if we have a message header
	do
	{
		hdrLen = 0;

		// remove the header
		// figure out what type of header this is
		if (memcmp(format, MQFMT_DEAD_LETTER_HEADER, MQ_FORMAT_LENGTH) == 0)
		{
			// found a header
			// get a pointer to the dlq
			dlqPtr = (MQDLH *)msgData;

			// get the new format, code page, encoding and the length of the header
			hdrLen = sizeof(MQDLH);
			if (NUMERIC_PC == encodeType)
			{
				*ccsid = dlqPtr->CodedCharSetId;
				*encoding = dlqPtr->Encoding;
			}
			else
			{
				*ccsid = reverseBytes4(dlqPtr->CodedCharSetId);
				*encoding = reverseBytes4(dlqPtr->Encoding);
			}

			// get the message format field
			if (CHAR_EBCDIC == charType)
			{
				EbcdicToAscii((unsigned char *)(&dlqPtr->Format), 	MQ_FORMAT_LENGTH, (unsigned char *)format);
			}
			else
			{
				memcpy(format, &dlqPtr->Format, MQ_FORMAT_LENGTH);
			}
		}
		else if (memcmp(format, MQFMT_CICS, MQ_FORMAT_LENGTH) == 0)
		{
			// found a header
			// get a pointer to the CIH
			cicsPtr = (MQCIH *)msgData;

			// get the new format, code page, encoding and the length of the header
			hdrLen = cicsPtr->StrucLength;
			if (NUMERIC_PC == encodeType)
			{
				*ccsid = cicsPtr->CodedCharSetId;
				*encoding = cicsPtr->Encoding;
			}
			else
			{
				*ccsid = reverseBytes4(cicsPtr->CodedCharSetId);
				*encoding = reverseBytes4(cicsPtr->Encoding);
			}

			// get the message format field
			if (CHAR_EBCDIC == charType)
			{
				EbcdicToAscii((unsigned char *)(&cicsPtr->Format), 	MQ_FORMAT_LENGTH, (unsigned char *)format);
			}
			else
			{
				memcpy(format, &cicsPtr->Format, MQ_FORMAT_LENGTH);
			}
		}
		else if (memcmp(format, MQFMT_IMS, MQ_FORMAT_LENGTH) == 0)
		{
			// found a header
			// get a pointer to the IIH
			imsPtr = (MQIIH *)msgData;

			// get the new format, code page, encoding and the length of the header
			hdrLen = imsPtr->StrucLength;
			if (NUMERIC_PC == encodeType)
			{
				*ccsid = imsPtr->CodedCharSetId;
				*encoding = imsPtr->Encoding;
			}
			else
			{
				*ccsid = reverseBytes4(imsPtr->CodedCharSetId);
				*encoding = reverseBytes4(imsPtr->Encoding);
			}

			// get the message format field
			if (CHAR_EBCDIC == charType)
			{
				EbcdicToAscii((unsigned char *)(&imsPtr->Format), 	MQ_FORMAT_LENGTH, (unsigned char *)format);
			}
			else
			{
				memcpy(format, &imsPtr->Format, MQ_FORMAT_LENGTH);
			}
		}
		else if ((memcmp(format, MQFMT_RF_HEADER, MQ_FORMAT_LENGTH) == 0) || (memcmp(format, MQFMT_RF_HEADER_2, MQ_FORMAT_LENGTH) == 0))
		{
			// found a header
			// get a pointer to the RFH
			rfh2Ptr = (MQRFH2 *)msgData;

			// get the new format, code page, encoding and the length of the header
			hdrLen = rfh2Ptr->StrucLength;
			if (NUMERIC_PC == encodeType)
			{
				*ccsid = rfh2Ptr->CodedCharSetId;
				*encoding = rfh2Ptr->Encoding;
			}
			else
			{
				*ccsid = reverseBytes4(rfh2Ptr->CodedCharSetId);
				*encoding = reverseBytes4(rfh2Ptr->Encoding);
			}

			// get the message format field
			if (CHAR_EBCDIC == charType)
			{
				EbcdicToAscii((unsigned char *)(&rfh2Ptr->Format), 	MQ_FORMAT_LENGTH, (unsigned char *)format);
			}
			else
			{
				memcpy(format, &rfh2Ptr->Format, MQ_FORMAT_LENGTH);
			}
		}

		if (hdrLen > 0)
		{
			// update the encoding and character set types
			encodeType = getIntEncode(*encoding);
			charType = getCcsidType(*ccsid);
			msgLen -= hdrLen;
			msgData += hdrLen;
			totLen += hdrLen;
		}
	} while ((hdrLen > 0) && (msgLen >= 32));

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::stripMQhdrs() totLen=%d", totLen);

		// trace exit from stripMQhdrs
		logTraceEntry(traceInfo);
	}

	return totLen;
}

/////////////////////////////////////////////////
//
// This routine will append a new error message
// to the current message text, inserting a
// carriage return and line feed if necessary.
//
/////////////////////////////////////////////////

void DataArea::appendError(const char *errtxt)

{
	// check if we need to append a CRLF sequence first
	if (m_error_msg.GetLength() > 0)
	{
		// the message field already contains a message,
		// so we need to first append a CRLF sequence
		m_error_msg += "\r\n";
	}

	// append the new message text
	m_error_msg += errtxt;
}

BOOL DataArea::isLeadChar(unsigned char ch, const int charFormat)

{
	BOOL	result=FALSE;

	// what type of characters are we dealing with?
	switch (charFormat)
	{
	case CHAR_CHINESE:
		{
			// assume code page 936
			if ((ch > 128) && (ch < 253))
			{
				result = TRUE;
			}

			break;
		}
	case CHAR_KOREAN:
		{
			// assume code page 949
			if ((ch > 142) && (ch < 255))
			{
				result = TRUE;
			}

			break;
		}
	case CHAR_TRAD_CHIN:
		{
			// assume code page 950
			if ((ch > 128) && (ch < 255))
			{
				result = TRUE;
			}

			break;
		}
	case CHAR_JAPANESE:
		{
			// assume code page 932
			if (((ch > 128) && (ch < 160)) || ((ch >= 224 ) && (ch < 253)))
			{
				result = TRUE;
			}

			break;
		}
	case CHAR_THAI:
		{
			// assume code page 874
			if ((ch > 128) && (ch < 255))
			{
				result = TRUE;
			}

			break;
		}
	case CHAR_RUSSIAN:
		{
			// assume code page 1251
			if ((ch > 128) && (ch < 255))
			{
				result = TRUE;
			}

			break;
		}
	}

	return result;
}

///////////////////////////////////////////////////////////
//
// Search for a CR or LF character within the data.
//
//  maxchar is the maximum number of characters to search
//  it is the same as the number of bytes, unless the
//  charset is a multi-byte character set.
//
// The return value is the offset of the number of
// characters before the CR or LF character
//
///////////////////////////////////////////////////////////

int DataArea::findcrlf(const unsigned char *datain, const int maxchar, const int charSet)

{
	int		count=0;
	int		i=0;
	char	ch;

	// get the first character to check
	ch = datain[0];
	while ((count < maxchar) && (ch != '\r') && (ch != '\n'))
	{
		if (isLeadChar(datain[i], charSet))
		{
			// skip the extra byte for double byte characters
			i++;
		}

		// move on to the next byte
		i++;

		// count the number of characters
		count++;

		// get the next character
		ch = datain[i];
	}

	return count;
}

int DataArea::setMQoptions(int options)

{
	// check for z/OS queue manager or high enough level
	if ((platform != MQPL_MVS) || (level >= MQCMDL_LEVEL_600))
	{
		// check if only complete messages are to be read
		if (m_complete_msg)
		{
			options |= MQGMO_COMPLETE_MSG;
		}

		// check if logical order is to be maintained
		if (m_logical_order)
		{
			// either not z/OS or level is high enough
			options |= MQGMO_LOGICAL_ORDER;
		}
	}

	// check if only complete messages or groups are to be returned
	if (m_all_avail)
	{
		// check the queue manager type
		if (platform != MQPL_MVS)
		{
			// not a zOS queue manager
			// therefore set both options
			options |= MQGMO_ALL_MSGS_AVAILABLE | MQGMO_ALL_SEGMENTS_AVAILABLE;
		}
		else
		{
			// z/OS queue manager
			// check the level of the queue manager
			if (level >= MQCMDL_LEVEL_600)
			{
				options |= MQGMO_ALL_MSGS_AVAILABLE;
			}
		}
	}

	// check if conversion of message was requested
	if (m_convert)
	{
		// set the convert option
		options |= MQGMO_CONVERT;
	}

	return options;
}

void DataArea::setJMSdomain(const char *domain)

{
	((RFH *)rfhData)->setJMSdomain(domain);
}

void DataArea::freeRfhArea()

{
	((RFH *)rfhData)->freeRfhArea();
}

void DataArea::freeRfh1Area()

{
	((RFH *)rfhData)->freeRfh1Area();
}

void DataArea::setPubSubVersion()

{
	((RFH *)rfhData)->setPubSubVersion();
}


void DataArea::setRFHV2()

{
	((RFH *)rfhData)->setRFHV2();
}

void DataArea::setRFHV1()

{
	((RFH *)rfhData)->setRFHV1();
}

//////////////////////////////////////////////////////////
//
// This routine translates an input character to an
// output character using the MQ translation tables.
// It attempts to locate the appropriate MQ translation
// table using the MQ installation location in the
// Windows registry.
//
//////////////////////////////////////////////////////////

int DataArea::MQEBC2ASC(unsigned char *out, const unsigned char *in, int len, int ccsidIn, int ccsidOut)

{
	int		ret=0;
	int		dirSize;
	char	fileName[512];
	FILE	*tbl;

	// this routine does not handle UCS-2 for now, so return if one of the pages is UCS-2
	if ((13488 == ccsidIn) || (17584 == ccsidIn) || (1200 == ccsidIn) || (1201 == ccsidIn))
	{
		return 0;
	}

	// check if we already tried to read this table
	if ((NULL == MQCcsidPtr) && (MQCcsidIn == ccsidIn) && (MQCcsidOut == ccsidOut))
	{
		// don't try to load the same table each time this routine is called
		return 0;
	}

	// check if we already have the right table loaded
	if ((NULL == MQCcsidPtr) || (MQCcsidIn != ccsidIn) || (MQCcsidOut != ccsidOut))
	{
		// check if there is already a table that must be released
		if (MQCcsidPtr != NULL)
		{
			// release the current table
			rfhFree(MQCcsidPtr);

			// clear the variables
			MQCcsidPtr = NULL;
			MQCcsidLen = ccsidOut;
		}

		// remember what table is being read
		MQCcsidIn = ccsidIn;
		MQCcsidOut = 0;

		// get the file path where MQ is installed
		dirSize = sizeof(fileName);
		findMQInstallDirectory((unsigned char *)fileName, dirSize);

		// create a table name using the two CCSID values and the install path
		sprintf(fileName + strlen(fileName), "\\conv\\table\\%4.4X%4.4X.tbl", ccsidIn, ccsidOut);

		// now try to open the table
		tbl = fopen(fileName, "rb");
		if (tbl != NULL)
		{
			// figure out how long the file is
			// read it into memory
			fseek(tbl, 0L, SEEK_END);
			MQCcsidLen = ftell(tbl);

			// return the pointer to the beginning of the data
			fseek(tbl, 0L, SEEK_SET);

			// only read the table if it is 256 bytes long
			if (256 == MQCcsidLen)
			{
				// allocate memory for the translation table
				MQCcsidPtr = (unsigned char *)rfhMalloc(MQCcsidLen, "TRANTBL ");

				// try to read the contents of the file into memory
				fread(MQCcsidPtr, 1, MQCcsidLen, tbl);
			}

			// done with the file - close the file handle
			fclose(tbl);
		}
	}

	// check if the table is loaded in memory and avoid tables that are not simple translations
	if ((MQCcsidPtr != NULL) && (256 == MQCcsidLen) && (MQCcsidIn == ccsidIn) && (MQCcsidOut == ccsidOut))
	{
		// have the right table loaded - do the translation
		while (ret < len)
		{
			out[ret] = MQCcsidPtr[in[ret]];
			ret++;
		}
	}

	// return the number of characters that were translated
	return ret;
}

void DataArea::findMQInstallDirectory(unsigned char *dirName, unsigned long dirNameLen)

{
	int				ret=0;
	int				ret2=0;
	HKEY			regkey;
	DWORD			valueType=0;
	const char *	keyName;
	char			traceInfo[512];		// work variable to build trace message

	// initialize the directory name in case the lookup fails
	dirName[0] = 0;

	// check if 64-bit OS
	if (is64bit)
	{
		// 64-bit OS
		keyName = MQINSKEY64;
	}
	else
	{
		// 32-bit OS
		keyName = MQINSKEY;
	}

	// open the registry key that we will enumerate
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					   keyName,
					   0,
					   KEY_QUERY_VALUE,
					   &regkey);

	if (ERROR_SUCCESS == ret)
	{
		ret = RegQueryValueEx(regkey,
							  "FilePath",
							  0,
							  &valueType,
							  dirName,
							  &dirNameLen);

		// close the key
		ret2 = RegCloseKey(regkey);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::findMQInstallDirectory() ret=%d ret2=%d valueType=%d dirName=%s", ret, ret2, valueType, dirName);

		// trace exit from findMQInstallDirectory
		logTraceEntry(traceInfo);
	}
}

//////////////////////////////////////////////////////////
//
// This routine translates an input character to an
// output character using the MQ translation tables.
// It attempts to locate the appropriate MQ translation
// table using the MQ installation location in the
// Windows registry.  The output character is assumed to
// be in UCS-2 format.
//
// MQ provides translation tables for both code pages
// 13488 and 17584.  The routine first checks for
// 17584 and if not found tries 13488.  In all cases
// the translate table must be 512 bytes long or it
// is ignored.  This is because we are translating
// from a single input byte to a UCS-2 output character.
//
//////////////////////////////////////////////////////////

int DataArea::MQEBC2UCS(wchar_t *out, const unsigned char *in, int len, int ccsidIn)

{
	int		ret=0;
	int		tabLen;
	int		dirSize;
	char	* endInstallDir;
	FILE	*tbl;
	char	fileName[512];

	// this routine does not handle UCS-2 input for now, so return if one of the pages is UCS-2
	if ((13488 == ccsidIn) || (17584 == ccsidIn) || (1200 == ccsidIn) || (1201 == ccsidIn))
	{
		return 0;
	}

	// check if we already tried to read this table
	if ((NULL == MQUcsPtr) && (MQUcsCcsid == ccsidIn))
	{
		// don't try to load the same table each time this routine is called
		return 0;
	}

	// check if we already have the right table loaded
	if ((NULL == MQUcsPtr) || (MQUcsCcsid != ccsidIn))
	{
		// we need to try and read the table into memory
		// check if there is already a table that must be released
		if (MQUcsPtr != NULL)
		{
			// release the current table
			rfhFree(MQUcsPtr);

			// clear the variables
			MQUcsPtr = NULL;
		}

		MQUcsCcsid = ccsidIn;

		// get the file path where MQ is installed
		dirSize = sizeof(fileName);
		findMQInstallDirectory((unsigned char *)fileName, dirSize);

		// remember where the install directory name ends
		endInstallDir = fileName + strlen(fileName);

		// create a table name using the install path plus the CCSID value and 17584 as the target
		sprintf(endInstallDir, "\\conv\\table\\%4.4X44B0.tbl", ccsidIn);

		// now try to open the table
		tbl = fopen(fileName, "rb");

		// check if we found the translate table
		if (NULL == tbl)
		{
			// didn't work - try 13488 instead
			sprintf(endInstallDir, "\\conv\\table\\%4.4X34B0.tbl", ccsidIn);

			// now try to open the table
			tbl = fopen(fileName, "rb");
		}

		if (tbl != NULL)
		{
			// figure out how long the file is
			// read it into memory
			fseek(tbl, 0L, SEEK_END);
			tabLen = ftell(tbl);

			// return the pointer to the beginning of the data
			fseek(tbl, 0L, SEEK_SET);

			// only read the table if it is 256 bytes long
			if (512 == tabLen)
			{
				// allocate memory for the translation table
				MQUcsPtr = (wchar_t *)rfhMalloc(tabLen, "TRANTBL2");

				// try to read the contents of the file into memory
				fread(MQUcsPtr, 1, tabLen, tbl);
			}

			// done with the file - close the file handle
			fclose(tbl);
		}
	}

	// check if the table is loaded in memory and avoid tables that are not simple translations
	if ((MQUcsPtr != NULL) && (MQUcsCcsid == ccsidIn))
	{
		// have the right table loaded - do the translation
		while (ret < len)
		{
			out[ret] = reverseBytes((short *)&MQUcsPtr[in[ret]]);
			ret++;
		}
	}

	return ret;
}


void DataArea::logTraceEntry(const char *traceInfo)

{
	((CRfhutilApp *)AfxGetApp())->logTraceEntry(traceInfo);
}

void DataArea::dumpTraceData(const char *label, const unsigned char *data, unsigned int length)

{
	((CRfhutilApp *)AfxGetApp())->dumpTraceData(label, data, length);
}

////////////////////////////////////////////
//
// Routine to get the current depth of the
// queue.  It will set the m_queue_depth
// variable.
//
// If the MQINQ request fails the depth is
// set to 0.
//
////////////////////////////////////////////

void DataArea::getCurrentDepth()

{
	MQLONG		cc=MQCC_OK;			// MQ completion code
	MQLONG		rc=0;				// MQ reason code
	MQLONG		Select[1];			// attribute selectors
	MQLONG		IAV[1];				// integer attribute values
	char		traceInfo[128];		// buffer to build trace information

	// set the selector to request the current queue depth
	Select[0] = MQIA_CURRENT_Q_DEPTH;

	// try to get the queue depth
	XMQInq(qm, q, 1L, Select, 1L, IAV, 0L, NULL, &cc, &rc);

	// check if it worked
	if (MQCC_OK == cc)
	{
		// set the queue depth variable
		m_q_depth = IAV[0];
	}
	else
	{
		// failed - set the depth to zero
		m_q_depth = 0;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getCurrentDepth() cc=%d rc=%d m_q_depth=%d", cc, rc, m_q_depth);

		// trace exit from getCurrentDepth
		logTraceEntry(traceInfo);
	}
}

//////////////////////////////////////////
//
// Routine to do an MQINQ to get the
// type of queue that was opened.
//
//////////////////////////////////////////

int DataArea::getQueueType(MQHOBJ queue)

{
	MQLONG		cc=MQCC_OK;			// MQ completion code
	MQLONG		rc=0;				// MQ reason code
	MQLONG		Select[1];			// attribute selectors
	MQLONG		IAV[1];				// integer attribute values
	int			queueType;
	char		traceInfo[128];		// buffer to build trace information

	// set the selector to request the current queue type
	Select[0] = MQIA_Q_TYPE;

	// try to get the queue type
	XMQInq(qm, q, 1L, Select, 1L, IAV, 0L, NULL, &cc, &rc);

	// check if it worked
	if (MQCC_OK == cc)
	{
		// set the queue depth variable
		queueType = IAV[0];
	}
	else
	{
		// failed - set the depth to zero
		queueType = 0;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getQueueType() cc=%d rc=%d queueType=%d", cc, rc, queueType);

		// trace exit from getQueueType
		logTraceEntry(traceInfo);
	}

	return queueType;
}

////////////////////////////////////////////////////////////////
//
// Routine to open a temporary queue and return the handle.
//
//
// The realQname parameter must be at least 49 bytes long.
// If the open is successful, the real name of the queue will
// be returned in this field.  The name will have any padding
// characters removed and the result will be a null terminated
// sring.
//
// The hConn parameter must be a valid handle to the desired
// queue manager.
//
// If successful, a valid queue handle will be returned.
// If unsuccessful, NULL will be returned and the ret value
// will be set the MQSeries reason code.
//
////////////////////////////////////////////////////////////////

MQLONG DataArea::OpenTempQ(LPCTSTR QMname, LPCTSTR qName, MQLONG openOpts, char *realQname, MQLONG *ret)

{
	MQLONG	cc=MQCC_FAILED;
	int		slen;
	MQOD	od={MQOD_DEFAULT};		// Object Descriptor
	char	traceInfo[256];

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::OpenTempQ() openOpts=%d QMname=%s qName=%s", openOpts, QMname, qName);

		// trace entry to OpenTempQ
		logTraceEntry(traceInfo);
	}

	// initialize the realQname to a null string
	realQname[0] = 0;

	// make sure the queue manager is connected
	if (!connect2QM(QMname))
	{
		return cc;
	}

	// Open a queue object
	od.ObjectType = MQOT_Q;

	// use the standard model queue
	strcpy(od.ObjectName, "SYSTEM.DEFAULT.MODEL.QUEUE");

	// get the length
	slen = strlen(qName);
	if (slen >= sizeof(od.DynamicQName))
	{
		slen = sizeof(od.DynamicQName) - 1;
	}

	// get the name template as a null terminated string
	memcpy(od.DynamicQName, qName, slen);
	od.DynamicQName[slen] = 0;

	// was an alternate user id specified?
	if (m_alt_userid)
	{
		// set the user id specified on the MQMD page
		openOpts |= MQOO_ALTERNATE_USER_AUTHORITY;

		// get the length of the user id to use
		slen = m_user_id.GetLength();

		// Is it too long?
		if (slen > MQ_USER_ID_LENGTH)
		{
			// truncate to 12 characters
			slen = MQ_USER_ID_LENGTH;
		}

		// copy the user id into the object descriptor
		memset(od.AlternateUserId, 0, MQ_USER_ID_LENGTH);
		memcpy(od.AlternateUserId, (LPCTSTR)m_user_id, slen);
	}

	XMQOpen(qm,					// connection handle
			&od,                     // object descriptor for queue
			openOpts,				// open it for inquire
			&hReplyQ,				// object handle
			&cc,						// MQOPEN completion code
			ret);					// reason code

	if (MQCC_OK == cc)
	{
		// get the real name of the queue
		memcpy(realQname, od.ObjectName, MQ_Q_NAME_LENGTH);
		realQname[MQ_Q_NAME_LENGTH] = 0;
		Rtrim(realQname);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting QNames::OpenTempQ cc=%d ret=%d realQname=%s", cc, (*ret), realQname);

		// trace exit from OpenTempQ
		logTraceEntry(traceInfo);
	}

	return cc;
}

////////////////////////////////////////////////////////////////
//
// Routine to read a message from the reply queue.
//
////////////////////////////////////////////////////////////////

MQLONG DataArea::getReplyMsg(MQLONG MQParm, char *UserMsg, char *errtxt, MQLONG ReadBufferLen, MQLONG *bytesRead, MQLONG *reason, MQLONG *ccsid, MQLONG *encoding)

{
	MQLONG	cc;
	MQGMO	mqgmo = {MQGMO_DEFAULT};
	MQMD2	msgdesc = {MQMD2_DEFAULT};
	char	traceInfo[512];				// work variable to build trace message

	// initialize the read buffer to zeros
	memset(UserMsg, 0, ReadBufferLen);
	(*bytesRead) = 0;

	// wait for 10 seconds
	mqgmo.Options = MQGMO_FAIL_IF_QUIESCING | MQGMO_WAIT | MQGMO_NO_SYNCPOINT | MQGMO_CONVERT;
	mqgmo.WaitInterval = 3 * 1000;

	// perform the MQGET
	XMQGet(qm, hReplyQ, &msgdesc, &mqgmo, ReadBufferLen, UserMsg, bytesRead, &cc, reason);

	// check if we exceeded our unit of work limit
	if ((MQCC_FAILED == cc) && (MQRC_SYNCPOINT_LIMIT_REACHED == (*reason)))
	{
		// issue a commit
		XMQCmit(qm, &cc, reason);

		// try the get again
		XMQGet(qm, hReplyQ, &msgdesc, &mqgmo, ReadBufferLen, UserMsg, bytesRead, &cc, reason);
	}

	if (cc != MQCC_OK)
	{
		// issue an error message
		sprintf(errtxt, "** Error reading Admin message from Temporary queue cc=%d reason=%d", cc, (*reason));
	}
	else
	{
		// return the ccsid and encoding of the received message
		(*ccsid) = msgdesc.CodedCharSetId;
		(*encoding) = msgdesc.Encoding;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting QNames::getReplyMsg cc=%d reason=%d bytesRead=%d ccsid=%d encoding=%d", cc, (*reason), (*bytesRead), msgdesc.CodedCharSetId, msgdesc.Encoding);

		// trace exit from getReplyMsg
		logTraceEntry(traceInfo);
	}

	return cc;
}

void DataArea::closeReplyQ()

{
	MQLONG	cc=-1;
	MQLONG	rc=-1;
	char	traceInfo[256];

	// make sure we have valid handles
	if ((qm != NULL) && (hReplyQ != NULL))
	{
		// issue a commit before we close the queue
		XMQCmit(qm, &cc, &rc);

		// close the temporary reply queue
		XMQClose(qm, &hReplyQ, MQCO_DELETE_PURGE, &cc, &rc);
	}

	// make sure hte reply Q handle is NULL
	hReplyQ = NULL;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting QNames::closeReplyQ cc=%d rc=%d", cc, rc);

		// trace exit from closeReplyQ
		logTraceEntry(traceInfo);
	}
}

MQLONG DataArea::openAdminQ(LPCTSTR QMname, LPCTSTR remoteQM, MQLONG * reason)

{
	MQLONG	cc=-1;
	MQLONG	openOpts;
	int		slen;
	MQOD	od={MQOD_DEFAULT};		// Object Descriptor
	char	traceInfo[512];

	if (traceEnabled)
	{
		if ((remoteQM != NULL) && (strlen(remoteQM) > 0))
		{
			// create the trace line
			sprintf(traceInfo, "Entering DataArea::openAdminQ() QMname=%s remoteQM=%s", QMname, remoteQM);
		}
		else
		{
			// create the trace line
			sprintf(traceInfo, "Entering DataArea::openAdminQ() QMname=%s", QMname);
		}

		// trace entry to openAdminQ
		logTraceEntry(traceInfo);
	}

	// make sure the queue manager is connected
	if (!connect2QM(QMname))
	{
		// the connection failed
		return cc;
	}

	// Open a queue object
	od.ObjectType = MQOT_Q;

	// use the standard model queue
	strcpy(od.ObjectName, ADMINQ);

	// check for a remote queue manager name
	if ((remoteQM != NULL) && (strlen(remoteQM) > 0))
	{
		memset(od.ObjectQMgrName, 0, sizeof(od.ObjectQMgrName));
		strncpy(od.ObjectQMgrName, remoteQM, sizeof(od.ObjectQMgrName));
	}

	// set the Open Options
	openOpts = MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING;

	// was an alternate user id specified?
	if (m_alt_userid)
	{
		// set the user id specified on the MQMD page
		openOpts |= MQOO_ALTERNATE_USER_AUTHORITY;

		// get the length of the user id to use
		slen = m_user_id.GetLength();

		// Is it too long?
		if (slen > MQ_USER_ID_LENGTH)
		{
			// truncate to 12 characters
			slen = MQ_USER_ID_LENGTH;
		}

		// copy the user id into the object descriptor
		memset(od.AlternateUserId, 0, MQ_USER_ID_LENGTH);
		memcpy(od.AlternateUserId, (LPCTSTR)m_user_id, slen);
	}

	XMQOpen(qm,					// connection handle
			&od,                     // object descriptor for queue
			openOpts,				// open it for inquire
			&hAdminQ,				// object handle
			&cc,						// MQOPEN completion code
			reason);					// reason code

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting QNames::openAdminQ cc=%d reason=%d", cc, (*reason));

		// trace exit from openAdminQ
		logTraceEntry(traceInfo);
	}

	return cc;
}

////////////////////////////////////////////////////////////////
//
// Routine to write a request message to a queue.
//
// This routine is used to put a message to the admin queue for
// PCF messages.
//
// The MsgFormat field will be placed in the MQMD format field.
//
// The QName field is used to generate any error messages
// that are returned.
//
// The UserMsg is assumed to point to the message data, which
// should be UserMsgLen bytes long.
//
// The ccsid field is placed in the character set field in
// the MQMD.
//
// The replyQName field will be placed in the reply queue
// name field in the MQMD
//
// If successful, zero will be returned.
// If unsuccessful, the MQ reason code will be returned and
// the errtxt field will contain an error message.  The errtxt
// field should be at least 128 bytes long.
//
////////////////////////////////////////////////////////////////

MQLONG DataArea::PutAdminMsg(MQCHAR8 MsgFormat, char *UserMsg, MQLONG UserMsgLen, MQLONG ccsid, char *errtxt, const char *replyQName, MQLONG * rc)

{
	MQLONG	cc;
	MQMD2	msgdesc = {MQMD2_DEFAULT};
	MQPMO	mqpmo = {MQPMO_DEFAULT};
	char	traceInfo[512];				// work variable to build trace message

	// check if the queue is already open
	if (NULL == hAdminQ)
	{
		// indicate there is an error
		return -1;
	}

	// do we have a message format?
	if (strlen(MsgFormat) > 0)
	{
		// set the message format
		memcpy(msgdesc.Format, MsgFormat, sizeof(msgdesc.Format));
	}

	// check for a reply to queue name
	if ((strlen(replyQName) > 0) && (strlen(replyQName) < MQ_Q_NAME_LENGTH))
	{
		// set the message type to request and copy in the reply to queue name
		msgdesc.MsgType = MQMT_REQUEST;
		strcpy(msgdesc.ReplyToQ, replyQName);
	}

	// use non-persistent messages
	msgdesc.Persistence = MQPER_NOT_PERSISTENT;

	// set a 10-second expiration on this message
	msgdesc.Expiry = 10 * 1000;

	// issue a synchpoint immediately
	mqpmo.Options |= MQPMO_NO_SYNCPOINT;

	// perform the MQPUT
	XMQPut(qm, hAdminQ, &msgdesc, &mqpmo, UserMsgLen, UserMsg, &cc, rc);

	if (cc != MQCC_OK)
	{
		// issue an error message
		sprintf(errtxt, "***** Error writing to Admin queue cc=%d, reason=%d", cc, (*rc));
	}


	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::PutAdminMsg UserMsgLen=%d ccsid=%d cc=%d, reason=%d", UserMsgLen, ccsid, cc, (*rc));

		// enter trace info
		logTraceEntry(traceInfo);
	}

	return cc;
}

MQLONG DataArea::closeAdminQ(MQLONG * reason)

{
	MQLONG		cc = MQCC_OK;						// MQ completion code
	char		traceInfo[128];			// work variable to build trace message

	// is the Admin Q open?
	if (hAdminQ != NULL)
	{
		XMQClose(qm, &hAdminQ, MQCO_NONE, &cc, reason);
	}

	// set the admin queue to NULL
	hAdminQ = NULL;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::closeAdminQ cc=%d, reason=%d", cc, (*reason));

		// trace exit from closeAdminQ
		logTraceEntry(traceInfo);
	}

	return cc;
}

////////////////////////////////////////////////////////////////
//
// Routine to move messages from one queue to another
//
////////////////////////////////////////////////////////////////

void DataArea::moveMessages(const char *qmName, const char *qName, const char * newQName, int start, int count, BOOL removeDlq, BOOL passAll)

{
	MQLONG				cc=MQCC_OK;					// MQ completion code
	MQLONG				rc=MQRC_NONE;				// MQ reason code
	MQLONG				cc2=MQCC_OK;				// MQ completion code
	MQLONG				rc2=MQRC_NONE;				// MQ reason code
	MQLONG				cc3=MQCC_OK;				// MQ completion code
	MQLONG				rc3=MQRC_NONE;				// MQ reason code
	MQLONG				openOpts=0;					// MQ open options for the output queue
	MQLONG				bufLen;						// length of the input buffer
	MQLONG				msgLen;						// length of the message
	MQLONG				options=MQGMO_NONE;			// MQGET options
	int					moved=0;					// number of messages that were moved
	int					committed=0;				// number of messages that were moved and committed
	int					read=0;						// number of messages that were read
	int					uow=0;						// number of messages in current unit of work
	int					removed=0;					// number of dead letter headers removed
	int					slen;						// Working variable for string lengths
	MQHOBJ				outQ;						// Handle for output queue
/*	MQHMSG				hMsg=MQHM_UNUSABLE_HMSG;	// message handle used to get message properties
	MQCMHO				opts={MQCMHO_DEFAULT};		// options used to create message handle
	MQDMHO				dOpts={MQDMHO_DEFAULT};		// options used to delete message handle */
	BOOL				save_set_all=m_set_all;		// save area for current set all setting
	BOOL				save_set_user=m_setUserID;	// save area for current set user id setting
	BOOL				putOK=TRUE;					// detect if MQPUT has failed
	BOOL				cmitOK=TRUE;				// detect if MQCMIT has failed
	BOOL				readErr=FALSE;				// problem trying to move to first message to move
	char				*buffer=NULL;				// pointer to buffer
	MQDLH				*dlh;						// pointer to dead letter queue header
	CRfhutilApp			*app;						// pointer to application object
	MQOD				od={MQOD_DEFAULT};			// Object Descriptor
	MQGMO				gmo={MQGMO_DEFAULT};		// Get message options
	MQPMO				pmo={MQPMO_DEFAULT};		// Put message options
	MQMD2				mqmd;						// MQMD for input and output messages
	MQMD2				defMQMD={MQMD2_DEFAULT};	// MQMD default values used to reset the MQMD after each MQGET
	char				errtxt[128];				// work area to build message text for error messages
	char				traceInfo[512];				// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::moveMessages() qmName=%s qName=%s newQName=%s start=%d count=%d removeDlq=%d passAll=%d", qmName, qName, newQName, start, count, removeDlq, passAll);

		// trace entry to moveMessages
		logTraceEntry(traceInfo);
	}

	// clear the current message variable in the DataArea object
	m_error_msg.Empty();

	// make sure we have a queue name
	if ((strlen(qName) == 0) || (strlen(newQName) == 0))
	{
		// queue name is missing - set error message and exit
		m_error_msg = "*Queue Name required*";
		return;
	}

	// make sure the queue names are not the same
	if (strcmp(qName, newQName) == 0)
	{
		// reject the request with an error message
		m_error_msg = "*Queue names must be different";
		return;
	}

	// connect to the queue manager
	if (!checkConnection(qmName))
	{
		// can't connect to the queue manager - the error message will be set by the connection routine
		return;
	}

	// try to open the input queue
	if (!openQ(qName, _T(""), Q_OPEN_READ_BROWSE, passAll))
	{
		// didn't work - the error message will be set by the open routine
		// disconnect from the Queue Manager
		discQM();

		// restore the set all and set user id settings
		m_set_all = save_set_all;
		m_setUserID = save_set_user;

		// free the buffer
		rfhFree(buffer);

		return;
	}

	// now try to open the output queue
	// Open a queue object
	od.ObjectType = MQOT_Q;

	// get the length of the queue name
	slen = strlen(newQName);
	if (slen > MQ_Q_NAME_LENGTH)
	{
		// make sure we don't copy too many characters
		slen = MQ_Q_NAME_LENGTH;
	}

	// set the name of the queue
	memset(od.ObjectName, 0, sizeof(od.ObjectName));
	memcpy(od.ObjectName, newQName, slen);

	// set the version of the object descriptor to V3
	od.Version = MQOD_VERSION_3;

	// set the open options
	openOpts = MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING;

	// was an alternate user id specified?
	if (m_alt_userid)
	{
		// set the user id specified on the MQMD page
		openOpts |= MQOO_ALTERNATE_USER_AUTHORITY;

		// get the length of the user id to use
		slen = m_user_id.GetLength();

		// Is it too long?
		if (slen > MQ_USER_ID_LENGTH)
		{
			// truncate to 12 characters
			slen = MQ_USER_ID_LENGTH;
		}

		// copy the user id into the object descriptor
		memset(od.AlternateUserId, 0, MQ_USER_ID_LENGTH);
		memcpy(od.AlternateUserId, (LPCTSTR)m_user_id, slen);
	}

	// check if pass all was requested
	if (passAll)
	{
		// set the pass all option
		openOpts |= MQOO_PASS_ALL_CONTEXT;
	}

	XMQOpen(qm,				// connection handle
			&od,				// object descriptor for queue
			openOpts,		// open it for inquire
			&outQ,			// object handle
			&cc,				// MQOPEN completion code
			&rc);			// reason code

	// check if the MQOPEN for the output queue was successful
	if (MQCC_OK == cc)
	{
		// get the maximum message length that the QMgr allows
		bufLen = maxMsgLenQM;

		// check if the max message size on the queue is less than the QM allows
		if (bufLen > maxMsgLenQ)
		{
			// use the smaller number
			bufLen = maxMsgLenQ;
		}

		// allocate a buffer that is the maximum size the queue manager and queue allows
		buffer = (char *)rfhMalloc(bufLen + 16, "MQBUFFER");

		// check if the malloc worked
		if (NULL == buffer)
		{
			// malloc failed - create an error message
			m_error_msg = "Buffer allocation failed";

			// close the queues and disconnect
			XMQClose(qm, &outQ, MQCO_NONE, &cc3, &rc3);
			closeQ(Q_CLOSE_NONE);
			discQM();

			// try to be graceful by just returning with an error message
			return;
		}

		// make sure that the match options are honored
		gmo.Version = MQGMO_VERSION_2;

		// get message properties in an RFH2 header
		options |= MQGMO_PROPERTIES_FORCE_MQRFH2;

		// check if only complete messages are to be read
		if (m_complete_msg)
		{
			options |= MQGMO_COMPLETE_MSG;
		}

		// check if logical order is to be maintained
		if (m_logical_order)
		{
			options |= MQGMO_LOGICAL_ORDER;
		}

		// check if only complete messages or groups are to be returned
		if (m_all_avail)
		{
			options |= MQGMO_ALL_MSGS_AVAILABLE | MQGMO_ALL_SEGMENTS_AVAILABLE;
		}

		// check if we need to browse through a number of messages before we start
		while ((MQCC_OK == cc) && (read < start - 1))
		{
			// clear the MQMD fields so we do not try to read a particular message
			memcpy(&mqmd, &defMQMD, sizeof(MQMD2));

			// set the GMO options to use
			// do not read any of the message contents since we are just skipping some messages
			if (0 == read)
			{
				// haven't read any messages yet so start at the beginning
				gmo.Options = options | MQGMO_BROWSE_FIRST | MQGMO_FAIL_IF_QUIESCING | MQGMO_NO_SYNCPOINT | MQGMO_ACCEPT_TRUNCATED_MSG;
			}
			else
			{
				// browse the next message in the queue
				gmo.Options = options | MQGMO_BROWSE_NEXT | MQGMO_FAIL_IF_QUIESCING | MQGMO_NO_SYNCPOINT | MQGMO_ACCEPT_TRUNCATED_MSG;
			}

			// set the match options to none - not looking for any particular message
			gmo.MatchOptions = MQMO_NONE;

			// Try to get a message with no data
			XMQGet(qm, q, &mqmd, &gmo, 0, NULL, &msgLen, &cc, &rc);

			// check for truncated message accepted - this is expected since we do not read the data
			if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_ACCEPTED == rc))
			{
				// change to a normal completion since we are ignoring the message
				cc = MQCC_OK;
			}

			if (MQCC_OK == cc)
			{
				// count the number of messages that were read
				read++;
				uow++;
			}
			else
			{
				// remember we had a problem with the start message number
				// the cc value will ensure that no messages are moved
				readErr = TRUE;
			}
		}

		// check if message properties are to be processed
/*		if (propertiesSupported)
		{
			// create a message handle to properly move message properties
			// make sure that the message handle processing options are honored
			gmo.Version = MQGMO_VERSION_4;
			pmo.Version = MQPMO_VERSION_3;

			// set the message handle options
			opts.Options = MQCMHO_VALIDATE;

			// set the message options
			gmo.Options |= MQGMO_PROPERTIES_IN_HANDLE;

			// create a message handle
			// note that a failure will not move any messages since cc is not MQCC_OK
			XMQCrtMh(qm, &opts, &hMsg, &cc, &rc);

			if (traceEnabled)
			{
				// enter the result in the trace
				sprintf(traceInfo, " MQCrtMh cc=%d rc=%d hMsg=%8.8X", cc, rc, hMsg);

				// write a trace entry
				logTraceEntry(traceInfo);
			}

			// check the completion code
			if (cc != MQCC_OK)
			{
				// report the error
				setErrorMsg(cc, rc, "MQCRTMH");
				updateMsgText();
			}

			// set the handle in the GMO
			gmo.MsgHandle = hMsg;
			pmo.OriginalMsgHandle = hMsg;
		}*/

		// start processing messages
		while ((MQCC_OK == cc) && putOK && cmitOK && ((0 == count) || (moved < count)))
		{
			// read the next message
			// clear the MQMD fields so we do not try to read a particular message
			memcpy(&mqmd, &defMQMD, sizeof(MQMD2));

			// set the match options to none - not looking for any particular message
			gmo.MatchOptions = MQMO_NONE;

			// check if we are starting from the first message or not
			if (start < 2)
			{
				// starting at the first message in the queue so we can just read the queue normally
				// set the get message options
				gmo.Options = options | MQGMO_FAIL_IF_QUIESCING | MQGMO_SYNCPOINT;

				// Try to get a message into the buffer
				XMQGet(qm, q, &mqmd, &gmo, bufLen, buffer, &msgLen, &cc, &rc);
			}
			else
			{
				// starting from some message other than the first so must continue the browse
				// and then get the message under cursor
				// set the get message options
				gmo.Options = options | MQGMO_BROWSE_NEXT | MQGMO_FAIL_IF_QUIESCING | MQGMO_NO_SYNCPOINT | MQGMO_ACCEPT_TRUNCATED_MSG;

				// Try to get a message with no data to advance the cursor
				XMQGet(qm, q, &mqmd, &gmo, 0, NULL, &msgLen, &cc, &rc);

				// check for truncated message accepted - this is expected since we do not read the data
				if ((MQCC_WARNING == cc) && (MQRC_TRUNCATED_MSG_ACCEPTED == rc))
				{
					// change to a normal completion since we are ignoring the message
					cc = MQCC_OK;
				}

				// check if the browse next worked
				if (MQCC_OK == cc)
				{
					// now change options and try to read the actual message we want
					gmo.Options = options | MQGMO_MSG_UNDER_CURSOR | MQGMO_FAIL_IF_QUIESCING | MQGMO_SYNCPOINT;

					// Try to get a message contents into the buffer
					XMQGet(qm, q, &mqmd, &gmo, bufLen, buffer, &msgLen, &cc, &rc);
				}
			}

			// check if it worked
			if (MQCC_OK == cc)
			{
				// count the number of messages we have read
				read++;
				uow++;

				// check if the DLQ header should be removed
				if (removeDlq && (memcmp(mqmd.Format, MQFMT_DEAD_LETTER_HEADER, MQ_FORMAT_LENGTH) == 0) && (msgLen >= sizeof(MQDLH)))
				{
					// get a pointer to the front of the data
					dlh = (MQDLH *)buffer;

					memcpy(mqmd.Format, dlh->Format, MQ_FORMAT_LENGTH);
					mqmd.Encoding = dlh->Encoding;
					mqmd.CodedCharSetId = dlh->CodedCharSetId;
					mqmd.PutApplType = dlh->PutApplType;
					memcpy(mqmd.PutApplName, dlh->PutApplName, MQ_APPL_NAME_LENGTH);

					pmo.Options = MQPMO_FAIL_IF_QUIESCING | MQPMO_SYNCPOINT;
					pmo.Context = NULL;

					if (passAll)
					{
						 pmo.Options |= MQPMO_SET_ALL_CONTEXT;
					}

					// write it to the output queue without the MQ dead letter header
					XMQPut(qm, outQ, &mqmd, &pmo, msgLen - sizeof(MQDLH), buffer + sizeof(MQDLH), &cc, &rc);

					// count the number of DLQ headers that were removed
					removed++;
				}
				else
				{
					// set the put options
					if (passAll)
					{
						pmo.Options = MQPMO_FAIL_IF_QUIESCING | MQPMO_SYNCPOINT | MQPMO_PASS_ALL_CONTEXT;
						pmo.Context = q;
					}
					else
					{
						pmo.Options = MQPMO_FAIL_IF_QUIESCING | MQPMO_SYNCPOINT | MQPMO_NEW_MSG_ID;
						pmo.Context = NULL;
					}

					// write it to the output queue
					XMQPut(qm, outQ, &mqmd, &pmo, msgLen, buffer, &cc, &rc);
				}

				// check if it worked
				if (MQCC_OK == cc)
				{
					// increase the counter
					moved++;
					uow++;

					// check if we need to do a commit
					// the maximum unit of work allowed is 2000
					if ((uow >= maxUOW) || (uow > 2000))
					{
						// commit the work that we did
						XMQCmit(qm, &cc2, &rc2);

						if (MQCC_OK == cc2)
						{
							// keep track of how many messages we have moved and committed in case of an error later
							committed = moved;

							// reset the counter
							uow = 0;
						}
						else
						{
							// remember the failure trying to commit the UOW
							cmitOK = FALSE;
						}
					}
				}
				else
				{
					// remember we had a problem with an MQPUT
					// the unit of work will be rolled back
					putOK = FALSE;

					if (traceEnabled)
					{
						// create the trace line
						sprintf(traceInfo, " DataArea::moveMessages() MQPUT error cc=%d rc=%d moved=%d committed=%d read=%d uow=%d removed %d cmitOK=%d", cc, rc, moved, committed, read, uow, removed, cmitOK);

						// trace error in moveMessages
						logTraceEntry(traceInfo);
					}
				}
			}
		}

		// check if there was an error trying to get to the first message
		if (!readErr && (uow > 0))
		{
			// check if a put or commit failed
			if (putOK && cmitOK)
			{
				// commit the work that we did
				XMQCmit(qm, &cc2, &rc2);

				// check if the commit worked
				if (cc2 != MQCC_OK)
				{
					// create an error message to indicate there was a problem
					sprintf(errtxt, "Commit failed cc=%d rc=%d moved=%d newQName=%s", cc2, rc2, moved, newQName);
					cmitOK = FALSE;
				}
				else
				{
					// keep track of how many messages were moved and committed
					committed = moved;
				}
			}
			else
			{
				// perform a rollback
				XMQBack(qm, &cc2, &rc2);
			}
		}

		// free the allocated buffer
		rfhFree(buffer);

		// check if message properties are to be processed
/*		if (propertiesSupported)
		{

			// delete the message handle
			XMQDltMh(qm, &hMsg, &dOpts, &cc, &rc);

			// check if it worked
			if (cc != MQCC_OK)
			{
				// unable to delete the message handle - report the error
				setErrorMsg(cc, rc, "MQDLTMH");
				updateMsgText();
			}
		}*/

		// close the output queue
		// use a different cc and rc so that previous errors are retained
		XMQClose(qm, &outQ, MQCO_NONE, &cc3, &rc3);

		// update the queue depth of the input queue
		getCurrentDepth();

		// check if there was an error trying to get to the first message
		if (readErr)
		{
			sprintf(errtxt, "Error reading to first message cc=%d rc=%d read=%d newQName=%s", cc, rc, read, newQName);

			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, " DataArea::moveMessages() MQGET error cc=%d rc=%d read=%d", cc, rc, read);

				// trace error in moveMessages
				logTraceEntry(traceInfo);
			}
		}
		else
		{
			// check if we had a problem with an MQPUT
			if (putOK)
			{
				// check for an error with the MQCMIT
				if (cmitOK)
				{
					// check for get error other than no more messages in queue
					if ((MQRC_NO_MSG_AVAILABLE == rc) || (MQRC_NONE == rc))
					{
						// tell the user what we happened
						sprintf(errtxt, "%d messages moved to %s", committed, newQName);
					}
					else
					{
						// tell the user what we happened
						sprintf(errtxt, "Get failed cc=%d rc=%d %d messages moved to %s", cc, rc, committed, newQName);

						if (traceEnabled)
						{
							// create the trace line
							sprintf(traceInfo, " DataArea::moveMessages() MQGET error cc=%d rc=%d moved=%d read=%d removed=%d", cc, rc, moved, read, removed);

							// trace error in moveMessages
							logTraceEntry(traceInfo);
						}
					}
				}
				else
				{
					// MQCMIT failed
					// tell the user what we happened
					sprintf(errtxt, "Commit failed cc=%d rc=%d %d messages moved to %s", cc2, rc2, committed, newQName);

					if (traceEnabled)
					{
						// create the trace line
						sprintf(traceInfo, " DataArea::moveMessages() MQCMIT error cc=%d rc=%d moved=%d read=%d removed=%d committed=%d", cc2, rc2, moved, read, removed, committed);

						// trace error in moveMessages
						logTraceEntry(traceInfo);
					}
				}
			}
			else
			{
				// put failed
				if (0 == committed)
				{
					// create an error message
					sprintf(errtxt, "MQPUT failed cc=%d rc=%d (no messages moved) newQName=%s", cc, rc, newQName);
				}
				else
				{
					// create an error message indicating some messages were already moved and  committed
					sprintf(errtxt, "MQPUT failed cc=%d rc=%d (%d messages moved) %s", cc, rc, committed, newQName);
				}
			}
		}

		// display a message telling the user what happened
		appendError(errtxt);

		if (putOK)
		{
			// call the audit routine to record the MoveMsgs action
			sprintf(errtxt, "Moved %d msgs", moved);
			app = (CRfhutilApp *)AfxGetApp();
			app->createAuditRecord((LPCTSTR)currentQM, (LPCTSTR)currentQ, errtxt, rc);
		}

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Exiting DataArea::moveMessages() moved=%d read=%d removed %d putOK=%d cmitOK=%d", moved, read, removed, putOK, cmitOK);

			// trace exit from moveMessages
			logTraceEntry(traceInfo);
		}
	}
	else
	{
		// error opening output queue
		sprintf(errtxt, "Error cc=%d rc=%d opening output queue %s", cc, rc, newQName);
		appendError(errtxt);

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Exiting DataArea::moveMessages() %s", errtxt);

			// trace exit from moveMessages
			logTraceEntry(traceInfo);
		}
	}

	// we are done - close the input queue
	closeQ(Q_CLOSE_NONE);

	// restore the previous settings of set all and set user id
	m_set_all = save_set_all;
	m_setUserID = save_set_user;
}

bool DataArea::openSubQ(const char *Queue, const char * remoteQM)

{
	MQLONG			cc=MQCC_OK;				// MQ completion code
	MQLONG			rc=MQRC_NONE;			// MQ reason code
	MQLONG			options=0;				// open options
	int				slen;					// work variable for string lengths
	MQOD			od={MQOD_DEFAULT};		// MQ open descriptor
	bool			ret=false;				// return value
	char			traceInfo[256];		// work variable to build trace message

	// set the open options
	options = MQOO_INPUT_SHARED | MQOO_INQUIRE | MQOO_FAIL_IF_QUIESCING;

	// get the length of the queue name
	slen = strlen(Queue);
	if (slen > MQ_Q_NAME_LENGTH)
	{
		// make sure we don't copy too many characters
		slen = MQ_Q_NAME_LENGTH;
	}

	// set the name of the queue
	memset(od.ObjectName, 0, sizeof(od.ObjectName));
	memcpy(od.ObjectName, Queue, slen);

	// check if this is a remote queue that we are explicitly addressing
	slen = strlen(remoteQM);
	if (slen > 0)
	{
		// make sure the name is not too long
		if (slen > MQ_Q_MGR_NAME_LENGTH)
		{
			// limit to the maximum number of characters
			slen = MQ_Q_MGR_NAME_LENGTH;
		}

		// set the name of the remote QM where the queue resides
		memcpy(od.ObjectQMgrName, remoteQM, slen);
	}

	// open the queue for input
	XMQOpen(qm, &od, options, &subQ, &cc, &rc);

	// check for a not authorized
	// this is see if the problem is that the user is not authorized for inquiry operations
	// but is otherwise allowed to open the queue
	if (cc != MQCC_OK)
	{
		// if this is a remote queue, it should create a 2045 reason code
		// if the problem is not authorized we should retry without inquire
		if ((MQCC_FAILED == cc) && ((MQRC_OPTION_NOT_VALID_FOR_TYPE == rc) || (MQRC_NOT_AUTHORIZED == rc)))
		{
			// turn off the inquire option
			options ^= MQOO_INQUIRE;

			// retry the open
			XMQOpen(qm, &od, options, &subQ, &cc, &rc);
		}
	}

	// check if it worked
	if (MQCC_OK == cc)
	{
		// indicate success
		ret = true;
	}
	else
	{
		// set an error message to indicate the error
		setErrorMsg(cc, rc, "OpenSubQ");
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::openSubQ() cc=%d rc=%d options=%d subQ=%8.8X Queue=%.48s", cc, rc, options, subQ, Queue);

		// trace exit from openSubQ
		logTraceEntry(traceInfo);
	}

	return ret;
}

int DataArea::subscribe(SUBPARMS * parms)

{
	MQLONG			cc=MQCC_OK;							// MQ completion code
	MQLONG			rc=MQRC_NONE;						// MQ reason code
	MQLONG			cc2=MQCC_OK;						// MQ completion code for MQINQ
	MQLONG			rc2=MQRC_NONE;						// MQ reason code for MQINQ
	MQLONG			options;							// subscription options
	MQLONG			Selector;							// used for MQINQ
	int				slen;								// work variable for string lengths
	CRfhutilApp *	app;								// pointer to the MFC application object
	char			*durmsg;							// pointer to message text
	MQMDPAGE		*mqmdObj=(MQMDPAGE *)mqmdData;		// pointer to MQMD object
    MQMD2			mqmd={MQMD2_DEFAULT};				// Message descriptor
	MQSD			sd={MQSD_DEFAULT};					// Subscription Descriptor
	char			queueName[MQ_Q_NAME_LENGTH+8];		// work area to retrieve the name of the managed queue
	const char *	auditQ;								// pointer to queue name used by subscription
	char			traceInfo[1536];					// work variable to build trace message
	char			fullTopic[MQ_TOPIC_NAME_LENGTH+8];	// work area to get the full topic name that is returned from MQSUB

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::subscribe() subType=%d QM=%s queue=%s remoteQM=%s subName=%s durable=%d managed=%d onDemand=%d newOnly=%d local=%d anyUserId=%d setCorrelId=%d subLevel=%d topicName=%.256s Topic=%.512s",
				parms->subType, parms->QMname, parms->queue, parms->remoteQM, parms->subName, parms->durable, parms->managed, parms->onDemand, parms->newOnly, parms->local, parms->anyUserId, parms->setCorrelId, parms->level, parms->topicName, parms->topic);

		// trace entry to subscribe
		logTraceEntry(traceInfo);
	}

	// check if pubsub entry points were found
	if (!pubsubSupported)
	{
		m_error_msg = "Pub/Sub not supported on this level of MQ";
		return MQCC_FAILED;
	}

	// make sure the returned queue name is properly initialized
	parms->managedQName[0] = 0;

	// make sure the returned topic and topic name are properly initialized
	parms->resumeTopic[0] = 0;
	parms->resumeTopicName[0] = 0;

	// make sure we have a topic name or topic string to subscribe to
	if ((strlen(parms->topicName) == 0) && (strlen(parms->topic) == 0) && (PS_SUB_TYPE_CREATE == parms->subType))
	{
		m_error_msg = "*Topic name and/or topic string required* ";
		return -1;
	}

	// make sure we have a subscription name
	if ((parms->subName != NULL) && (strlen(parms->subName) == 0) && parms->durable)
	{
		m_error_msg = "*Subscription name required* ";
		return -1;
	}

	// check for a provided subscription with no queue name
	if ((!(parms->managed)) && (strlen(parms->queue) == 0) && (PS_SUB_TYPE_CREATE == parms->subType))
	{
		m_error_msg = "*Queue name required if not managed*";
		return -1;
	}

	// make sure we are connected to the correct QM
	if (!checkConnection(parms->QMname))
	{
		return -1;
	}

	// check the level of the QMgr
	if (level < MQCMDL_LEVEL_700)
	{
		// QMgr level must be at least V7 to get sub or topic names
		m_error_msg = "QMgr must be V7 or above";
		discQM();

		if (traceEnabled)
		{
			// log the problem
			logTraceEntry("Queue manager level below 7.0");
		}

		return -1;
	}

	// set the MQMD fields in the message object
	mqmdObj->setMessageMQMD(&mqmd, m_setUserID, m_set_all);

	// start with options of fail if quiescing
	options = MQSO_FAIL_IF_QUIESCING;

	// check for a resume of existing subscription
	if (PS_SUB_TYPE_RESUME == parms->subType)
	{
		// set the options required for a resume
		// it must be a durable subscription to be resumed
		options |= MQSO_RESUME | MQSO_DURABLE;

		// N.B. selection strings cannot be altered so nothing is set on an alter
		// set the selection string pointer regardless
		sd.SelectionString.VSPtr = (void *)parms->selection;

		// to work around a bug this field will not be set except if the QMgr is on Windows
		// check if the QMgr is running on Windows
		if (MQPL_WINDOWS_NT == platform)
		{
			sd.SelectionString.VSBufSize = parms->maxSelLen;
		}
	}
	else
	{
		// set the options appropriate for creating a new or altering an existing subscription
		if (PS_SUB_TYPE_ALTER == parms->subType)
		{
			// alter request
			// assume a durable subscription since a non-durable subscription cannot be altered
			options |= MQSO_ALTER | MQSO_DURABLE;
		}
		else
		{
			// not a resume or alter so try to create the subscription
			options |= MQSO_CREATE;

			// check for durable subscription - cannot be altered
			if ((parms->durable))
			{
				// durable subscription
				options |= MQSO_DURABLE;
			}
			else
			{
				// non-durable subscription - deleted when application disconnects
				options |= MQSO_NON_DURABLE;
			}

			// check for subscription grouping
			if ((parms->groupSub) > 0)
			{
				// set the subscription group option
				options |= MQSO_GROUP_SUB;
			}

			// was new only specified?
			if ((parms->newOnly))
			{
				// set the option
				options |= MQSO_NEW_PUBLICATIONS_ONLY;
			}

			// check for a subscription level
			if ((parms->level) > 0)
			{
				// set the subscription level
				sd.SubLevel = (parms->level);
			}

			// check the type of wildcards to use
			if (PS_WILDCARD_CHAR == (parms->wildcard))
			{
				// wildcards apply to characters - for compatibility with Fuji broker
				options |= MQSO_WILDCARD_CHAR;
			}
			else
			{
				// wildcards apply to topic levels
				options |= MQSO_WILDCARD_TOPIC;
			}

			// set the selection string pointer and length
			sd.SelectionString.VSPtr = (void *)parms->selection;
			sd.SelectionString.VSBufSize = parms->userDataLength;
		}

		// was local specified
		if (parms->local)
		{
			// set the option
			options |= MQSO_SCOPE_QMGR;
		}

		if (parms->onDemand)
		{
			// set the option
			options |= MQSO_PUBLICATIONS_ON_REQUEST;
		}

		// check for a any user id option
		if (parms->anyUserId > 0)
		{
			// set the subscription group option
			options |= MQSO_ANY_USERID;
		}

		// check for a priority
		if (parms->priority > 0)
		{
			// set the subscription priority
			sd.PubPriority = parms->priority;
		}

		// check for subscription expiration
		if (parms->expiry > 0)
		{
			// set the subscription expiration
			sd.SubExpiry = parms->expiry;
		}

		// check for set identity
		if (parms->setIdent)
		{
			// set the option
			options |= MQSO_SET_IDENTITY_CONTEXT;

			// copy the values into the Subscription Descriptor
			memcpy(sd.PubApplIdentityData, parms->applIdent, MQ_APPL_IDENTITY_DATA_LENGTH);
			memcpy(sd.PubAccountingToken, parms->acctToken, MQ_ACCOUNTING_TOKEN_LENGTH);
		}

		// check if the subscription correlation id is to be set
		// it cannot be used with a managed subscription
		// if the subscription is managed then this option is ignored
		if (parms->setCorrelId && !(parms->managed))
		{
			// set the option
			options |= MQSO_SET_CORREL_ID;

			// set the subscription correlation id
			memcpy(sd.SubCorrelId, parms->correlId, MQ_CORREL_ID_LENGTH);
		}
	}

	if (parms->managed)
	{
		// set handle to none
		subQ = MQHO_NONE;

		// set managed option
		options |= MQSO_MANAGED;
	}
	else
	{
		// open the subscription queue
		if (!openSubQ(parms->queue, parms->remoteQM))
		{
			// queue open failed so just return
			return -1;
		}
	}

	// point to a user data area
	sd.SubUserData.VSPtr = parms->userData;

	// set the user data and user data length
	if (parms->userData != NULL)
	{
		if (parms->subType != PS_SUB_TYPE_RESUME)
		{
			// set the length of the data
			sd.SubUserData.VSLength = parms->userDataLength;
		}

		// to work around a bug this field will not be set except for Windows
		// check if the QMgr is running on Windows
		if (MQPL_WINDOWS_NT == platform)
		{
			// set the size of the data area
			sd.SubUserData.VSBufSize = parms->userDataBufSize;
		}
	}

	// set the subscription options
	sd.Options =  MQSO_FAIL_IF_QUIESCING | options;

	// set up the topic
	sd.ObjectString.VSPtr = (void *)parms->topic;
	sd.ObjectString.VSLength = MQVS_NULL_TERMINATED;

	// set up the subscription name
	sd.SubName.VSPtr = (void *)parms->subName;
	sd.SubName.VSLength = MQVS_NULL_TERMINATED;

	// set up the topic name
	slen = strlen(parms->topicName);
	if (slen > sizeof(sd.ObjectName))
	{
		slen = sizeof(sd.ObjectName);
	}

	// set the topic name
	memcpy(sd.ObjectName, parms->topicName, slen);

	// next check for a selector when creating a subscription
	if (parms->selection[0] != 0 && (PS_SUB_TYPE_CREATE == parms->subType))
	{
		// set the selection string pointer and indicate it is a null terminated string
		sd.SelectionString.VSPtr = (void *)parms->selection;
		sd.SelectionString.VSLength = MQVS_NULL_TERMINATED;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, " Filter slen=%d maxSelLen=%d VSPtr=%8.8X VSOffset=%d VSLength=%d VSBufSize=%d &selection=%8.8X selection=%.256s", slen, parms->maxSelLen, (unsigned int)sd.SelectionString.VSPtr, sd.SelectionString.VSOffset, sd.SelectionString.VSLength, sd.SelectionString.VSBufSize, (unsigned int)parms->selection, parms->selection);

		// write data to trace
		logTraceEntry(traceInfo);
	}

	// set up a buffer to capture the full topic name that was subscribed to
	memset(fullTopic, 0, sizeof(fullTopic));
	sd.ResObjectString.VSPtr = fullTopic;
	sd.ResObjectString.VSBufSize = sizeof(fullTopic);

#ifdef MQCLIENT
	// check for windows queue manager
	if ((platform != MQPL_WINDOWS_NT) && (level < MQCMDL_LEVEL_701))
	{
		// set the code page explicitly to work around another bug
		// otherwise the data is not properly translated
		// this is only necessary in the case of a client
		sd.ResObjectString.VSCCSID = 437;
	}
#endif

	// check if trace is enabled
	// this is done to capture the original failure code
	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, " before MQSUB sd.Options=%8.8X subQ=%8.8X hSub=%8.8X managed=%d durable=%d onDemand=%d subName=%.64s", sd.Options, subQ, hSub, parms->managed, parms->durable, parms->onDemand, parms->subName);

		// trace exit from subscribe
		logTraceEntry(traceInfo);
	}

   ////////////////////////////////////////////////////////////////////
   //
   //   Try to subscribe to the topic
   //
   ////////////////////////////////////////////////////////////////////

	// create the subscription
	XMQSub(qm,					// connection handle
		   &sd,					// object descriptor for subscription
		   &subQ,				// object handle (output) for subscription
		   &hSub,				// object handle (output) for reading messages
		   &cc,					// completion code
		   &rc);				// reason code

	// check if trace is enabled
	// this is done to capture the original failure code
	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, " after MQSUB cc=%d rc=%d sd.Options=%8.8X subQ=%8.8X hSub=%8.8X sd.SelectionString.VSLength=%d sd.SubUserData.VSLength=%d", cc, rc, sd.Options, subQ, hSub, sd.SelectionString.VSLength, sd.SubUserData.VSLength);

		// trace exit from subscribe
		logTraceEntry(traceInfo);
	}

	// check for a 2019 error (hobj not valid) - this can be caused by using the wrong managed setting
	// this section will reverse the managed setting and retry
	// this allows a user to guess wrong on the managed setting and still be successful
	if ((MQCC_FAILED == cc) && (MQRC_HOBJ_ERROR == rc) && (PS_SUB_TYPE_RESUME == parms->subType))
	{
		// try to recover from this failure by changing the destination class
		if (parms->managed)
		{
			// have to open the subscription queue - not really a managed subscription
			if (!openSubQ(parms->queue, parms->remoteQM))
			{
				// queue open failed so just return
				return -1;
			}
			// not a managed subscription
			sd.Options ^= MQSO_MANAGED;

			// change the destination type
			parms->managed = FALSE;
		}
		else
		{
			// was a queue opened?
			if (subQ != MQHO_NONE)
			{
				// close the subscription queue
				XMQClose(qm, &subQ, MQCO_NONE, &cc2, &rc2);

				// clear the subscription handle
				subQ = MQHO_NONE;
			}

			// change to managed subscription
			sd.Options |= MQSO_MANAGED;

			// change the destination type
			parms->managed = TRUE;
		}

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, " Before MQSUB retry sd.Options=%8.8X subQ=%8.8X hSub=%8.8X managed=%d", sd.Options, subQ, hSub, parms->managed);

			// trace entry to subscribe
			logTraceEntry(traceInfo);
		}

		// reissue the MQSUB
		XMQSub(qm,					// connection handle
			   &sd,					// object descriptor for subscription
			   &subQ,				// object handle (output) for subscription
			   &hSub,				// object handle (output) for reading messages
			   &cc2,				// completion code
			   &rc2);				// reason code

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, " After MQSUB retry cc2=%d rc2=%d sd.Options=%8.8X subQ=%8.8X hSub=%8.8X", cc2, rc2, sd.Options, subQ, hSub);

			// trace entry to subscribe
			logTraceEntry(traceInfo);
		}

		// check if it worked the second time
		if (MQCC_OK == cc2)
		{
			// change the original cc and rc
			cc = MQCC_OK;
			rc = MQRC_NONE;
		}
		else
		{
			// check if the queue is open
			if (subQ != MQHO_NONE)
			{
				// close the subscription queue
				XMQClose(qm, &subQ, MQCO_NONE, &cc2, &rc2);

				// clear the subscription handle
				subQ = MQHO_NONE;
			}

			// go back to the original setting for managed
			if (parms->managed)
			{
				// reset to provided
				parms->managed = FALSE;
			}
			else
			{
				// reset to provided
				parms->managed = TRUE;
			}
		}
	}

	// check the results
	if (cc != MQCC_OK)
	{
		// put failed, get the reason and generate an error message
		// get the completion code and reason code
		setErrorMsg(cc, rc, "MQSub");

		// check if the queue needs to be closed
		if (subQ != MQHO_NONE)
		{
			// try to close the queue to avoid leaking handles
			// ignore the return code
			XMQClose(qm, &subQ, MQCO_NONE, &cc2, &rc2);
		}
	}
	else
	{
		// tell the user what happened
		if (PS_SUB_TYPE_RESUME == parms->subType)
		{
			m_error_msg.Format("Subscription resumed to %s", parms->subName);
		}
		else if (PS_SUB_TYPE_ALTER == parms->subType)
		{
			m_error_msg.Format("Subscription altered %s", parms->subName);
		}
		else
		{
			if (parms->durable)
			{
				// a durable subscription was created
				durmsg = "Durable ";
			}
			else
			{
				// a non-durable subscription was created
				durmsg = "";
			}

			// create the message
			m_error_msg.Format("%sSubscription %s created", durmsg, parms->subName);
		}

		// return the full topic name
		if (sd.ResObjectString.VSPtr != NULL)
		{
			// check if the string is null terminated
			if (MQVS_NULL_TERMINATED == sd.ResObjectString.VSLength)
			{
				// set the full topic name using a strcpy
				strcpy(parms->resumeTopic, (const char *)sd.ResObjectString.VSPtr);
			}
			else if (sd.ResObjectString.VSLength > 0)
			{
				// length is specified so use memcpy and remember to terminate the string
				memcpy(parms->resumeTopic, sd.ResObjectString.VSPtr, sd.ResObjectString.VSLength);
				parms->resumeTopic[sd.ResObjectString.VSLength] = 0;
			}
		}

		// return the subscription correlation id
		memcpy(parms->correlId, sd.SubCorrelId, MQ_CORREL_ID_LENGTH);

		if ((PS_SUB_TYPE_RESUME == parms->subType) || (PS_SUB_TYPE_ALTER == parms->subType))
		{
//			parms->managed = (sd.Options & MQSO_MANAGED) == MQSO_MANAGED;
			parms->durable = (sd.Options & MQSO_DURABLE) == MQSO_DURABLE;
		}

		// was the subscription was resumed?
		if (PS_SUB_TYPE_RESUME == parms->subType)
		{
			// retrieve the values for the various subscription attributes
			parms->onDemand = (sd.Options & MQSO_PUBLICATIONS_ON_REQUEST) == MQSO_PUBLICATIONS_ON_REQUEST;
			parms->groupSub = (sd.Options & MQSO_GROUP_SUB) == MQSO_GROUP_SUB;
			parms->anyUserId = (sd.Options & MQSO_ANY_USERID) == MQSO_ANY_USERID;
			parms->level = sd.SubLevel;
			parms->priority = sd.PubPriority;
			parms->expiry = sd.SubExpiry;

			// get the wildcard option
			if ((sd.Options & MQSO_WILDCARD_CHAR) == MQSO_WILDCARD_CHAR)
			{
				parms->wildcard = PS_WILDCARD_CHAR;
			}
			else
			{
				parms->wildcard = PS_WILDCARD_TOPIC;
			}

			// check for set identity option
			if (parms->setIdent)
			{
				// capture the current identity context for this subscription
				memcpy(parms->applIdent, sd.PubApplIdentityData, MQ_APPL_IDENTITY_DATA_LENGTH);
				memcpy(parms->acctToken, sd.PubAccountingToken, MQ_ACCOUNTING_TOKEN_LENGTH);
			}
			else
			{
				// clear the identity context
				memset(parms->applIdent, 0, MQ_APPL_IDENTITY_DATA_LENGTH);
				memset(parms->acctToken, 0, MQ_ACCOUNTING_TOKEN_LENGTH);
			}

			// check if a second MQSUB must be issued to capture the user data and the selector
			// this is required due to a bug in the first release of MQ V7
			// this is not required on Windows since there is no code page or integer encoding conversion required
			if ((platform != MQPL_WINDOWS_NT) && ((sd.SubUserData.VSLength > 0) || (sd.SelectionString.VSLength > 0)))
			{
				// first close the subscription before reissuing the call
				XMQClose(qm, &hSub, MQCO_NONE, &cc2, &rc2);

				// clear the subscription handle
				hSub = MQHO_NONE;

				if (traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " After MQCLOSE for second issue cc2=%d rc2=%d", cc2, rc2);

					// write the trace entry to the log
					logTraceEntry(traceInfo);
				}

				// check for a managed subscription
				if ((sd.Options & MQSO_MANAGED) != 0)
				{
					// close the subscription queue
					XMQClose(qm, &subQ, MQCO_NONE, &cc2, &rc2);

					// clear the subscription queue handel
					subQ = MQHO_NONE;

					// if (traceEnabled)
					{
						// create the trace line
						sprintf(traceInfo, " After MQCLOSE of managed queue cc2=%d rc2=%d", cc2, rc2);

						// write the trace entry to the log
						logTraceEntry(traceInfo);
					}
				}

				// set the buffer size to the required length
				sd.SubUserData.VSBufSize = sd.SubUserData.VSLength;
				sd.SelectionString.VSBufSize = sd.SelectionString.VSLength;

				// reset the resume bit in the options
				sd.Options |= MQSO_RESUME;

				// reissue the MQSUB
				XMQSub(qm,					// connection handle
					   &sd,					// object descriptor for subscription
					   &subQ,				// object handle (output) for subscription
					   &hSub,				// object handle (output) for reading messages
					   &cc2,				// completion code
					   &rc2);				// reason code

				if (traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " After MQSUB second issue cc2=%d rc2=%d sd.Options=%8.8X subQ=%8.8X hSub=%8.8X", cc2, rc2, sd.Options, subQ, hSub);

					// write the trace entry to the log
					logTraceEntry(traceInfo);

					// check for user data
					if (verboseTrace && (sd.SubUserData.VSLength > 0))
					{
						// dump out the user data
						dumpTraceData("user data", (unsigned char *)sd.SubUserData.VSPtr, sd.SubUserData.VSLength);
					}
				}
			}

			// get the length of the user data
			// the data itself will be in the userData field that was passed in
			parms->userDataLength = sd.SubUserData.VSLength;

			// capture the selection string
			if (sd.SelectionString.VSLength > 0)
			{
				if (MQVS_NULL_TERMINATED == sd.SelectionString.VSLength)
				{
					// use a string copy
					strcpy(parms->selection, (const char *)sd.SelectionString.VSPtr);
				}
				else
				{
					// use a memcpy
					memcpy(parms->selection, sd.SelectionString.VSPtr, sd.SelectionString.VSLength);
					parms->selection[sd.SelectionString.VSLength] = 0;
				}
			}
			else
			{
				// just return a null string
				parms->selection[0] = 0;
			}
		}

		// point to the queue name
		auditQ = parms->queue;

		// check if the subscription was successful and the subscription was managed
		// also get the queue name if the subscription was resumed (or altered)
		if ((parms->managed) || (parms->subType != PS_SUB_TYPE_CREATE))
		{
			// get the name of the managed queue
			// do an inquiry for the queue name
			Selector = MQCA_Q_NAME;
			memset(queueName, 0, sizeof(queueName));
			XMQInq(qm,						// connection handle to queue manager
				   subQ,					// object handle for q
				   1,						// inquire only one selector
				   &Selector,				// the selector to inquire
				   0,						// no integer attributes needed
				   NULL,					// pointer to the integer result
				   sizeof(queueName),		// inquiring on character value
				   queueName,				// pointer to the character results
				   &cc2,					// completion code
				   &rc2);					// reason code

			// check if the inq worked
			if (MQCC_OK == cc2)
			{
				// return the name of the managed queue
				strcpy(parms->managedQName, queueName);
				auditQ = queueName;
			}

			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, " MQINQ cc2=%d rc2=%d managed=%d queueName=%s", cc2, rc2, parms->managed, queueName);

				// trace exit from subscribe
				logTraceEntry(traceInfo);
			}
		}

		// call the audit routine to record the subscribe action
		app = (CRfhutilApp *)AfxGetApp();
		app->createAuditRecord(parms->QMname, auditQ, "MQSub", rc);
	}

	if (traceEnabled)
	{
		if (MQCC_OK == cc)
		{
			logTraceEntry((LPCTSTR)m_error_msg);
		}

		if (sd.ResObjectString.VSPtr != NULL)
		{
			// create the trace line
			sprintf(traceInfo, "Exiting DataArea::subscribe() cc=%d rc=%d sd.Options=%8.8X managed=%d durable=%d onDemand=%d managedQName=%s result=%.1024s", cc, rc, sd.Options, parms->managed, parms->durable, parms->onDemand, parms->managedQName, parms->resumeTopic);
		}
		else
		{
			// create the trace line
			sprintf(traceInfo, "Exiting DataArea::subscribe() cc=%d rc=%d sd.Options=%8.8X managed=%d durable=%d onDemand=%d managedQName=%s", cc, rc, sd.Options, parms->managed, parms->durable, parms->onDemand, parms->managedQName);
		}

		// trace exit from subscribe
		logTraceEntry(traceInfo);
	}

	return cc;
}

void DataArea::getPSMessage(const char * subName, int waitTime, BOOL * isRetained)

{
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=0;							// MQ reason code
	MQLONG			cc2=MQCC_OK;					// MQ completion code
	MQLONG			rc2=0;							// MQ reason code
	MQLONG			Select[1];						// attribute selectors
	MQLONG			IAV[1];							// integer attribute values
	MQLONG			options=0;						// GMO options
	MQLONG			matchOptions=0;					// GMO match options
	int				rfh1Len;
	int				rfh2Len;
	int				dlqLen;
	int				cicsLen;
	int				imsLen;
	CRfhutilApp *	app;							// pointer to the MFC application object
	MQMDPAGE		*mqmdObj=(MQMDPAGE *)mqmdData;	// pointer to MQMD object
    MQMD2			mqmd={MQMD2_DEFAULT};			// Message descriptor
	MQGMO			gmo={MQGMO_DEFAULT};			// Get message options
	char			errtxt[512];					// work variable to build error message
	char			rfhmsg[128];
	char			traceInfo[256];					// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getPSMessage() waitTime=%d subName=%s", waitTime, subName);

		// trace entry to getPSMessage
		logTraceEntry(traceInfo);
	}

	// clear the last message
	m_error_msg.Empty();

	// set the is retained value to false
	(*isRetained) = FALSE;

	// make sure we have a subscription request
	if (MQHO_NONE == hSub)
	{
		m_error_msg = "No subscription active";

		return;
	}

	// set the mq options
	options = setMQoptions(options);

	// check if conversion of message was requested
	if (m_convert)
	{
		// is the code page specified on the MQMD tab?
		if (mqmdObj->m_mqmd_ccsid.GetLength() > 0)
		{
			// use the selected ccsid
			mqmd.CodedCharSetId = atoi((LPCTSTR)mqmdObj->m_mqmd_ccsid);
		}

		// set the encoding in case we are dealing with UCVS-2
		if (NUMERIC_PC == mqmdObj->m_mqmd_encoding)
		{
			// use PC encoding
			mqmd.Encoding = DEF_PC_ENCODING;
		}
		else
		{
			// use host encoding
			mqmd.Encoding = DEF_UNIX_ENCODING;
		}

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Data conversion specified ccsid=%s m_MQMD_encoding=%d", (LPCTSTR)mqmdObj->m_mqmd_ccsid, mqmdObj->m_mqmd_encoding);

			// trace data conversion request
			logTraceEntry(traceInfo);
		}
	}

	// check for a get by message id request
	if (m_get_by_msgid && (memcmp(mqmdObj->m_message_id, MQMI_NONE, MQ_MSG_ID_LENGTH) != 0))
	{
		mqmdObj->setMsgId(&mqmd);
		matchOptions |= MQMO_MATCH_MSG_ID;
	}

	if (m_get_by_correlid && (memcmp(mqmdObj->m_correlid, MQCI_NONE, MQ_CORREL_ID_LENGTH) != 0))
	{
		mqmdObj->setCorrelId(&mqmd);
		matchOptions |= MQMO_MATCH_CORREL_ID;
	}

	if (m_get_by_groupid && (memcmp(mqmdObj->m_group_id, MQGI_NONE, MQ_GROUP_ID_LENGTH) != 0))
	{
		mqmdObj->setGroupId(&mqmd);
		matchOptions |= MQMO_MATCH_GROUP_ID;
	}

	// check if we need to set the user id
	if (m_setUserID)
	{
		mqmdObj->setUserId(&mqmd);
	}

	// try to get a message from the subscription queue
	cc = processMQGet("PSGet", subQ, options, matchOptions, waitTime, &mqmd, &rc);

	// check if it worked
	if (cc != MQCC_OK)
	{
		// check the return code for a 2009 error (connection to QMgr lost)
		if (MQRC_CONNECTION_BROKEN == rc)
		{
			// get rid of the subscription and other handles which are no longer valid
			connectionLostCleanup();
		}

		// get failed, the reason is in the error message
		return;
	}

	// set the selector to request the current queue depth
	Select[0] = MQIA_CURRENT_Q_DEPTH;

	// try to get the queue depth
	XMQInq(qm, subQ, 1L, Select, 1L, IAV, 0L, NULL, &cc2, &rc2);

	// check if it worked
	if (MQCC_OK == cc2)
	{
		// set the queue depth variable
		m_q_depth = IAV[0];
	}
	else
	{
		// failed - set the depth to zero
		m_q_depth = 0;
	}

	// process the data from the message
	processMessage(&mqmd);

	// set the message to a null string
	rfhmsg[0] = 0;

	// get the lengths of the various headers
	rfh1Len = ((RFH *)rfhData)->getRFH1length();
	rfh2Len = ((RFH *)rfhData)->getRFH2length();
	cicsLen = ((CCICS *)cicsData)->getCICSlength();
	imsLen = ((CIms *)imsData)->getIMSlength();
	dlqLen = ((CDlq *)dlqData)->getDLQlength();

	// were there any headers that were processed?
	if (rfh2Len + rfh1Len + cicsLen + imsLen + dlqLen > 0)
	{
		// create some message text to tell the users the lengths of the various headers that were processed
		sprintf(rfhmsg, " (data %d dlq %d rfh %d cics %d ims %d)", fileSize, dlqLen, rfh2Len + rfh1Len, cicsLen, imsLen);
	}

	// make sure the property object is not null
	if (propData != NULL)
	{
		// check for an MQIsRetained property
		((CProps *)propData)->checkForRetained(isRetained);
	}

	// if not found in MQ properties check rfh2 usr properties
	if (!(*isRetained))
	{
		// make sure the usrData object exists
		if (usrData != NULL)
		{
			// check if the properties were passed in as a usr folder
			((Usr *)usrData)->checkForRetained(isRetained);
		}
	}

	// create a message for the user to tell what happened
	sprintf(errtxt, "Published msg read from %.256s length=%d%s", subName, fileSize, rfhmsg);
	m_error_msg += errtxt;

	// call the audit routine to record the get message action
	app = (CRfhutilApp *)AfxGetApp();
	app->createAuditRecord("", subName, "GetPS", rc);

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getPSMessage() cc=%d rc=%d cc2=%d rc2=%d isRetained=%d m_q_depth=%d", cc, rc, cc2, rc2, (*isRetained), m_q_depth);

		// trace exit from getPSMessage
		logTraceEntry(traceInfo);
	}
}

void DataArea::closePSQueue(BOOL removeSub)

{
	MQLONG			cc=MQCC_OK;					// MQ completion code
	MQLONG			rc=0;						// MQ reason code
	MQLONG			cc2=MQCC_OK;				// MQ completion code
	MQLONG			rc2=0;						// MQ reason code
	MQLONG			options=MQCO_NONE;			// GMO options
	char			traceInfo[128];				// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::closePSQueue() removeSub=%d", removeSub);

		// trace entry to closePSQueue
		logTraceEntry(traceInfo);
	}

	// should we reomve the subscription?
	if (removeSub)
	{
		options |= MQCO_REMOVE_SUB;
	}

	if (hSub != MQHO_NONE)
	{
		// close the subscription
		XMQClose(qm, &hSub, options, &cc, &rc);

		// check for an error
		if (MQRC_OPTION_NOT_VALID_FOR_TYPE == rc)
		{
			// this looks like a non-durable subscription - try without the keep option
			XMQClose(qm, &hSub, MQCO_NONE, &cc, &rc);
		}

		// clear the subscription handle
		hSub = MQHO_NONE;

		// check if it worked
		if (cc != MQCC_OK)
		{
			// close failed, get the reason and generate an error message
			setErrorMsg(cc, rc, "CloseSub");
		}
	}

	if (subQ != MQHO_NONE)
	{
		// close the subscription queue
		XMQClose(qm, &subQ, MQCO_NONE, &cc2, &rc2);

		// clear the subscription queue handle
		subQ = MQHO_NONE;

		// check if it worked
		if (cc2 != MQCC_OK)
		{
			// close failed, get the reason and generate an error message
			setErrorMsg(cc2, rc2, "CloseSubQ");
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::closePSQueue() cc=%d rc=%d cc2=%d rc2=%d", cc, rc, cc2, rc2);

		// trace exit from closePSQueue
		logTraceEntry(traceInfo);
	}
}

void DataArea::publishMsg(LPCTSTR QMname,
						  const char * topic,
						  const char * topicName,
						  int pubLevel,
						  BOOL retained,
						  BOOL local,
						  BOOL notOwn,
						  BOOL suppressReply,
						  BOOL warnNoSub,
						  BOOL newCorrelId,
						  BOOL noMulticast)

{
	// define the MQ objects that we need
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=0;							// MQ reason code
	MQLONG			cc2=MQCC_OK;					// MQ completion code
	MQLONG			rc2=0;							// MQ reason code
	MQLONG			openOpts=0;
	MQHOBJ			hTopic=MQHO_NONE;
	MQMDPAGE		*mqmdObj=(MQMDPAGE *)mqmdData;
	CProps			*propObj=(CProps *)propData;	// pointer to the user property object
	RFH				*rfhObj=(RFH *)rfhData;
	MQMD2			mqmd={MQMD2_DEFAULT};			// Message descriptor
	MQPMO			pmo={MQPMO_DEFAULT};			// Put message options
	MQOD			od={MQOD_DEFAULT};				// MQ open descriptor
	MQHMSG			hMsg=MQHM_UNUSABLE_HMSG;		// message handle used to set message properties
	MQCMHO			opts={MQCMHO_DEFAULT};			// options used to create message handle
	MQDMHO			dOpts={MQDMHO_DEFAULT};			// options used to delete message handle
	unsigned char	*tempbuf;						// Temporary data buffer
	const char		*cptr;							// pointer to topic string for trace purposes
	CRfhutilApp *	app;							// pointer to the MFC application object
	int				msgSize=0;						// length of complete message, including RFH
	int				rfh1Len=0;						// length of the RFH1 header
	int				rfh2Len=0;						// length of the RFH2 header
	int				cicsLen=0;						// length of the CICS header
	int				imsLen=0;						// length of the IMS header
	int				dlqLen=0;						// length of the Dead Letter Queue header
	int				curOfs=0;						// current offset within the message buffer
	int				ccsid=0;
	int				encoding=0;
	int				tempOpt;
	int				propCount;						// number of message properties
	bool			noTopic;						// is there a topic object name?
	bool			noTopicName;					// is there a topic string?
	CString			tempFormat;						// message format for MQMD
	char			trace1[16];						// work area for trace info
	char			errtxt[640];					// work variable to build error message
	char			traceInfo[1024];				// work variable to build trace message

	if (traceEnabled)
	{
		// make sure the topic is not null
		if (NULL == topic)
		{
			// point to zero length string
			cptr = "";
		}
		else
		{
			// point to topic string
			cptr = topic;
		}

		// create the trace line
		sprintf(traceInfo, "Entering DataArea::publishMsg() QMname=%s level=%d retained=%d local=%d notOwn=%d suppressReply=%d newCorrelId=%d pubLevel=%d topicName=%.256s topic=%.512s", QMname, level, retained, local, notOwn, suppressReply, newCorrelId, pubLevel, topicName, cptr);

		// trace entry to publishMsg
		logTraceEntry(traceInfo);
	}

	// make sure we have a topic name and/or topic object
	noTopic = (topic == NULL) || (strlen(topic) == 0);
	noTopicName = (topicName == NULL) || (strlen(topicName) == 0);
	if (noTopic && noTopicName)
	{
		m_error_msg = "*Topic is required*";
		updateMsgText();
		return;
	}

	// make sure we are connected to the correct QM
	if (!checkConnection(QMname))
	{
		return;
	}

	// check the level of the QMgr
	if (level < MQCMDL_LEVEL_700)
	{
		// QMgr level must be at least V7 to get sub or topic names
		m_error_msg = "QMgr must be V7 or above";
		discQM();

		if (traceEnabled)
		{
			// log the problem
			logTraceEntry("Queue manager level below 7.0");
		}

		return;
	}

	// set the open options
	openOpts = MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING;

	// check for no multicast option
	if ((level >= MQCMDL_LEVEL_710) && noMulticast)
	{
		openOpts |= MQOO_NO_MULTICAST;
	}

	// set up the object descriptor for the topic
	od.Version = MQOD_VERSION_4;
	od.ObjectType = MQOT_TOPIC;
	od.ObjectString.VSPtr = (void *)topic;
	od.ObjectString.VSLength = MQVS_NULL_TERMINATED;

	// set the topic name
	strncpy(od.ObjectName, topicName, sizeof(od.ObjectName));

	if (traceEnabled)
	{
		// build the trace line
		memset(trace1, 0, sizeof(trace1));
		tempOpt = reverseBytes4(openOpts);
		AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace1);
		sprintf(traceInfo, "Open options X\'%s\'", trace1);

		// write a trace log entry
		logTraceEntry(traceInfo);

		if (verboseTrace)
		{
			// dump the contents of the open descriptor to the trace
			dumpTraceData("MQOD", (unsigned char *)&od, sizeof(od));

			// dump out the topic string
			if ((topic != NULL) && (strlen(topic) > 0))
			{
				// dump out the topic string
				dumpTraceData("topic", (unsigned char *)topic, strlen(topic));
			}
		}
	}

	// try to open the topic
	XMQOpen(qm, &od, openOpts, &hTopic, &cc, &rc);

	// check if it worked
	if (cc != MQCC_OK)
	{
		// open failed, get the reason and generate an error message
		setErrorMsg(cc, rc, "PubOpen");
		return;
	}

	// we have the queue open, so start to build the message
	// update the data type.  This is necessary because
	// we may have to fill in the rfh data
	m_char_format = getCcsidType(atoi((LPCTSTR)mqmdObj->m_mqmd_ccsid));

	// remember the format of the user data
	tempFormat = mqmdObj->m_mqmd_format;

	// Check if we need to build an RFH1 header
	if (((RFH *)rfhData)->m_rfh1_include)
	{
		// create the RFH1 header if necessary and get the length
		getHdrCcsid(MQFMT_RF_HEADER, &ccsid, &encoding);
		rfh1Len = rfhObj->buildRFH(ccsid, encoding);
	}

	// Check if we need to build an RFH2 header
	if (((RFH *)rfhData)->m_rfh2_include)
	{
		// create the RFH2 header if necessary and get the length
		getHdrCcsid(MQFMT_RF_HEADER_2, &ccsid, &encoding);
		rfh2Len = rfhObj->buildRFH2(ccsid, encoding);
	}

	// check if there is a CICS or IMS header
	cicsLen = ((CCICS *)cicsData)->getCICSlength();
	imsLen = ((CIms *)imsData)->getIMSlength();

	// check for a DLQ header
	if (((CDlq *)dlqData)->checkForDlq())
	{
		// build a dead letter header and get the length
		// figure out what ccsid and encoding to use
		getHdrCcsid(MQFMT_DEAD_LETTER_HEADER, &ccsid, &encoding);

		// create the DLQ header if necessary
		dlqLen = ((CDlq *)dlqData)->createHeader(ccsid, encoding);
	}

	// allocate a temporary buffer large enough for the data and the RFH headers
	tempbuf = (unsigned char *)rfhMalloc(fileSize + dlqLen + cicsLen + imsLen + rfh1Len + rfh2Len + 1, "TEMPBUF2");

	// create any headers
	curOfs = buildHeaders(tempbuf, dlqLen, cicsLen, imsLen, rfh1Len, rfh2Len);

	if ((fileSize > 0) && (fileData != NULL))
	{
		// move the message data into the buffer
		memcpy(tempbuf + curOfs, fileData, fileSize);
	}

	// get the total message length
	msgSize = fileSize + curOfs;

	tempbuf[msgSize] = 0;

	// set the MQMD fields in the message object
	mqmdObj->setMessageMQMD(&mqmd, m_setUserID, m_set_all);

	// set the initial PMO Options
	pmo.Options = MQPMO_NO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING;

	if (m_set_all)
	{
		// indicate we are setting all context
		pmo.Options |= MQPMO_SET_ALL_CONTEXT;
	}

	if (m_setUserID)
	{
		// indicate we are setting identity context
		pmo.Options |= MQPMO_SET_IDENTITY_CONTEXT;
	}

	// check if the new message id option is to be set
	if (m_new_msg_id)
	{
		// set the new message id option
		pmo.Options |= MQPMO_NEW_MSG_ID;
	}

	// check if the new correlation id option is to be set
	if (m_new_correl_id)
	{
		// set the new correlation id option
		pmo.Options |= MQPMO_NEW_CORREL_ID;
	}

	if (retained)
	{
		pmo.Options |= MQPMO_RETAIN;
		pmo.Version = MQPMO_VERSION_3;
	}

	if (local)
	{
		pmo.Options |= MQPMO_SCOPE_QMGR;
		pmo.Version = MQPMO_VERSION_3;
	}

	if (notOwn)
	{
		pmo.Options |= MQPMO_NOT_OWN_SUBS;
		pmo.Version = MQPMO_VERSION_3;
	}

	if (suppressReply)
	{
		pmo.Options |= MQPMO_SUPPRESS_REPLYTO;
		pmo.Version = MQPMO_VERSION_3;
	}

	if (newCorrelId)
	{
		pmo.Options |= MQPMO_NEW_CORREL_ID;
		pmo.Version = MQPMO_VERSION_3;
	}

	// check for a publication level
	if (pubLevel > 0)
	{
		pmo.PubLevel = pubLevel;
		pmo.Version = MQPMO_VERSION_3;
	}

	// check for the new warn if no pubs option
	if ((level >= MQCMDL_LEVEL_701) && warnNoSub)
	{
		pmo.Options |= MQPMO_WARN_IF_NO_SUBS_MATCHED;
	}

	// check if message properties are to be processed
	propCount = propObj->GetPropertyCount();
	if ((propCount > 0) && (MQ_PROPS_YES == m_mq_props))
	{
		// make sure that the message handle processing options are honored
		pmo.Version = MQPMO_VERSION_3;

		// set the message handle options
		opts.Options = MQCMHO_VALIDATE;

		// create a message handle
		XMQCrtMh(qm, &opts, &hMsg, &cc, &rc);

		// check if it was created
		if (cc != MQCC_OK)
		{
			// unable to create the message handle - report the error
			setErrorMsg(cc, rc, "MQCRTMH");

			// return noting error
			return;
		}

		// message handle has been created
		// set the message properties
		cc = setMsgProps(hMsg);

		if (cc != MQCC_OK)
		{
			// return if setting the message properties failed
			updateMsgText();
			m_error_msg = "Error setting message properties - message not sent";

			// return with error
			return;
		}

		// set the handle in the PMO and the action
		pmo.OriginalMsgHandle = hMsg;
		pmo.Action = MQACTP_NEW;
	}

	if (traceEnabled)
	{
		// build the trace line
		memset(trace1, 0, sizeof(trace1));
		tempOpt = pmo.Options;
		tempOpt = reverseBytes4(tempOpt);
		AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace1);
		sprintf(traceInfo, "publishing message using options X\'%s\' propCount=%d m_mq_props=%d", trace1, propCount, m_mq_props);

		// write a trace log entry
		logTraceEntry(traceInfo);

		// get the values for the topic string variable string
		sprintf(traceInfo, "ObjectString od.ObjectString.VSBufSize=%d od.ObjectString.VSLength=%d od.ObjectString.VSCCSID=%d", od.ObjectString.VSBufSize, od.ObjectString.VSLength, od.ObjectString.VSCCSID);

		// write a trace log entry
		logTraceEntry(traceInfo);

		// get the values for the selection string variable string
		sprintf(traceInfo, "SelectionString od.SelectionString.VSBufSize=%d od.SelectionString.VSLength=%d od.SelectionString.VSCCSID=%d", od.SelectionString.VSBufSize, od.SelectionString.VSLength, od.SelectionString.VSCCSID);

		// write a trace log entry
		logTraceEntry(traceInfo);

		// get the values for the resolved topic string variable string
		sprintf(traceInfo, "ResObjectString od.ResObjectString.VSBufSize=%d od.ResObjectString.VSLength=%d od.ResObjectString.VSCCSID=%d", od.ResObjectString.VSBufSize, od.ResObjectString.VSLength, od.ResObjectString.VSCCSID);

		// write a trace log entry
		logTraceEntry(traceInfo);

		// check for verbose trace
		if (verboseTrace)
		{
			// dump the contents of the mqmd and pmo to the trace
			dumpTraceData("MQMD", (unsigned char *)&mqmd, sizeof(mqmd));
			dumpTraceData("PMO", (unsigned char *)&pmo, sizeof(pmo));
		}
	}

	// try to send the message
	XMQPut(qm, hTopic, &mqmd, &pmo, msgSize, tempbuf, &cc, &rc);

	// check for a no publish message
	if (MQRC_NO_SUBS_MATCHED == rc)
	{
		// tell the user there were no subscribers
		m_error_msg = "No subscribers matched published message";
		updateMsgText();

		// change the return code to normal
		cc = MQCC_OK;
		rc = MQRC_NONE;
	}

	// check if a message handle was created
	if ((MQ_PROPS_YES == m_mq_props) && (propCount > 0))
	{
		// delete the message handle
		XMQDltMh(qm, &hMsg, &dOpts, &cc2, &rc2);

		// check if it worked
		if (cc2 != MQCC_OK)
		{
			// unable to delete the message handle - report and trace the error
			setErrorMsg(cc2, rc2, "MQDLTMH");
		}
	}

	// check the results
	if (cc != MQCC_OK)
	{
		// put failed, get the reason and generate an error message
		// get the completion code and reason code
		setErrorMsg(cc, rc, "Put");

		// release the temporary buffer before returning
		rfhFree(tempbuf);

		// was this a warning?
		if (MQCC_FAILED == cc)
		{
			// check for a unit of work
			if (unitOfWorkActive)
			{
				rollbackUOW();
			}
		}
	}

	// close the topic object
	XMQClose(qm, &hTopic, MQCO_NONE, &cc2, &rc2);

	// check the results
	if (MQCC_OK == cc)
	{

		// extract certain MQMD fields from the message that was just sent
		mqmdObj->updateMQMDafterPut(&mqmd, msgSize);

		// worked, tell the user the message was sent
		sprintf(errtxt, "Message published length=%d topicName %.256s topic %.256s", msgSize, topicName, topic);
		m_error_msg = errtxt;

		// release the temporary buffer before returning
		rfhFree(tempbuf);

		// call the audit routine to record the get message action
		app = (CRfhutilApp *)AfxGetApp();
		app->createAuditRecord(QMname, topicName, "Publish", rc);
	}

	if (traceEnabled)
	{
		// log the results
		logTraceEntry(errtxt);

		// build a trace line
		sprintf(traceInfo, "Exiting DataArea::publishMsg() cc=%d rc=%d cc2=%d rc2=%d", cc, rc, cc2, rc2);

		// trace exit from publishMsg
		logTraceEntry(traceInfo);
	}
}

int DataArea::nameToType(char *name)

{
	char *		ptr;				// pointer to parentheses
	MQLONG		type;				// property type

	// default to a string type
	type = MQTYPE_STRING;

	// check for a data type qualifier
	ptr = strchr(name, '(');

	// check if a parentheses was found in the name
	if (NULL == ptr)
	{
		// just return the default of string type
		return type;
	}

	// found a data type qualifier?
	// terminate the name string
	ptr[0] = 0;

	// skip the open parentheses
	ptr++;

	// check for dt=
	if (memcmp(ptr, "dt=", 3) == 0)
	{
		// found the data type attribute
		ptr += 3;

		// check the data type
		if (memcmp(ptr, "i4", 2) == 0)
		{
			// four byte integer
			type = MQTYPE_INT32;
		}
		else if (memcmp(ptr, "i8", 2) == 0)
		{
			// eight byte integer
			type = MQTYPE_INT64;
		}
		else if (memcmp(ptr, "i1", 2) == 0)
		{
			// one byte integer
			type = MQTYPE_INT8;
		}
		else if (memcmp(ptr, "i2", 2) == 0)
		{
			// two byte integer
			type = MQTYPE_INT16;
		}
		else if (memcmp(ptr, "boolean", 7) == 0)
		{
			// boolean
			type = MQTYPE_BOOLEAN;
		}
		else if (memcmp(ptr, "bin.hex", 7) == 0)
		{
			// byte string
			type = MQTYPE_BYTE_STRING;
		}
		else if (memcmp(ptr, "r4", 2) == 0)
		{
			// four byte floating point
			type = MQTYPE_FLOAT32;
		}
		else if (memcmp(ptr, "r8", 2) == 0)
		{
			// eight byte floating point
			type = MQTYPE_FLOAT64;
		}
	}
	else if (memcmp(ptr, "xsi:nil=", 8) == 0)
	{
		// NULL value
		type = MQTYPE_NULL;
	}

	return type;
}

////////////////////////////////////////////////
//
// Routine to process message properties
//  as a string.  The properties will
//  be added to the message handle.
//
// Two properties will be handled differently.
//  If a isRetained property is found it will
//  be ignored.  If a topic string property
//  is found it will be noted in the parameters
//  but will not be added as a user property.
//
////////////////////////////////////////////////

int DataArea::pubWriteMsgAddProps(WRITEPARMS *parms, MQHMSG hMsg)

{
	size_t			valueLen = 0;					// length of property value
	size_t			i;								// work variable
	char *	ptr;									// pointer to message properties
	const char *	endPtr;							// end of message properties
	const char *	valuePtr;						// pointer to value
	void *			dataPtr="";						// pointer to data in proper format
	int64_t			i8=0;							// eight byte integer value
	double			r8=0.0;							// 8-byte floating point value
	float			r4=0.0;							// 4-byte floating point value
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			cc2=MQCC_OK;					// MQ completion code
	MQLONG			rc=MQRC_NONE;					// MQ reason code
	MQLONG			type= MQTYPE_STRING;			// property type
	int				propCount=0;					// number of user properties found
	int				i4=0;							// four byte integer value
	int				chgTopic=0;						// used for trace to indicate topic was changed
	int				foundRetain=0;					// used for trace to indicate found MQIsRetained property
	BOOL			boolVal=FALSE;					// boolean value
	MQSMPO			smpo={MQSMPO_DEFAULT};			// Set property options
	MQPD			pd={MQPD_DEFAULT};				// property description
	char			name[MQ_MAX_PROPERTY_NAME_LENGTH+33];	// name of property
	char			value[MQ_MAX_PROPERTY_NAME_LENGTH+1];	// value if originally in hex
	char			traceInfo[1024];				// work variable to build trace message

	// clear the name
	memset(name, 0, sizeof(name));

	// check for a pointer to message properties
	ptr = (char *)parms->userProp;
	if ((NULL == ptr) || (0 == parms->propLen))
	{
		// no properties - just return
		return 0;
	}

	// get a pointer to the end of the message properties
	endPtr = ptr + parms->propLen;

	// process each property in turn
	while ((ptr < endPtr) && (MQCC_OK == cc))
	{
		// pointer to the name
		i = 0;

		// find the end of the name
		while ((ptr < endPtr) && (ptr[0] != '=') && (i < MQ_MAX_PROPERTY_NAME_LENGTH))
		{
			// move on to the next character
			name[i] = ptr[0];
			ptr++;
			i++;
		}

		// terminate the name string
		name[i] = 0;

		// skip the equal sign and point to the value
		ptr++;
		valuePtr = ptr;
		valueLen = 0;

		// find the end of the value
		while ((ptr < endPtr) && (ptr[0] != '\n') && (ptr[0] != 0))
		{
			// move on to the next character
			ptr++;
			valueLen++;
		}

		// check for a new line
		if ('\n' == ptr[0])
		{
			// convert to a termination character
			ptr[0] = 0;
		}

		// get the data type and remove the attribute from the name
		type = nameToType(name);

		// check for special cases
		if (strcmp(name, MQISRETAIN) == 0)
		{
			// ignore this property
			foundRetain = 1;
		}
		else if (strcmp(name, MQTOPICSTR) == 0)
		{
			// check if topics in the file are being used
			// otherwise this property will be ignored
			if (parms->useTopic)
			{
				// process as a new topic
				// is the topic the same as the current topic?
				if (memcmp(parms->resTopicStr, valuePtr, valueLen) != 0)
				{
					// have to switch topics
					// remember that the topic was changed
					chgTopic = 1;

					// change the topic string
					memset(parms->topic, 0, sizeof(parms->topic));
					strncpy(parms->topic, valuePtr, MQ_TOPIC_STR_LENGTH);
					parms->topicName = "";
				}
			}
		}
		else
		{
			// normal user property - just add it to the handle
			// check for the type of value
			switch (type)
			{
			case MQTYPE_STRING:
				{
					dataPtr = (void *)valuePtr;
					break;
				}
			case MQTYPE_INT64:
				{
					i8 = _atoi64(valuePtr);
					dataPtr = (void *)&i8;
					valueLen = 8;
					break;
				}
			case MQTYPE_INT32:
				{
					i4 = atoi(valuePtr);
					dataPtr = (void *)&i4;
					valueLen = 4;
					break;
				}
			case MQTYPE_INT16:
				{
					i4 = atoi(valuePtr);
					dataPtr = (void *)&i4;
					valueLen = 2;
					break;
				}
			case MQTYPE_INT8:
				{
					i4 = atoi(valuePtr);
					dataPtr = (void *)&i4;
					valueLen = 1;
					break;
				}
			case MQTYPE_BOOLEAN:
				{
					if (memcmp(valuePtr, "TRUE", 4) == 0)
					{
						boolVal = TRUE;
					}
					else
					{
						boolVal = FALSE;
					}

					dataPtr = (void *)&boolVal;
					valueLen = 1;
					break;
				}
			case MQTYPE_FLOAT32:
				{
					r4 = (float)atof(valuePtr);
					dataPtr = (void *)&r4;
					valueLen = 4;
					break;
				}
			case MQTYPE_FLOAT64:
				{
					r8 = atof(valuePtr);
					dataPtr = (void *)&r4;
					valueLen = 8;
					break;
				}
			case MQTYPE_BYTE_STRING:
				{
					// get the number of bytes in the result
					valueLen <<= 1;

					// check if the value field is long enough
					if (valueLen < sizeof(value) - 1)
					{
						// just use the area on the stack - avoid a malloc
						// convert the hex characters to their binary representation
						HexToAscii((unsigned char *)valuePtr, valueLen, (unsigned char *)&value);

						// point to the binary representation of the data
						dataPtr = &value;
					}
					else
					{
						// have to malloc a larger area
						dataPtr = rfhMalloc(valueLen + 1, "PBYTESTR");

						if (dataPtr != NULL)
						{
							// convert the hex characters to their binary representation
							HexToAscii((unsigned char *)valuePtr, valueLen, (unsigned char *)dataPtr);
						}
						else
						{
							// error - just write some of the hex bytes
							dataPtr = (void *)valuePtr;
						}
					}

					break;
				}
			default:
				{
					// treat as null value
					dataPtr = "";
					valueLen = 0;
				}
			}

			// insert the value into the handle
			XMQSetMp(qm, hMsg, &smpo, &name, &pd, type, valueLen, dataPtr, &cc, &rc);

			// check if a malloc was performed
			if (MQTYPE_BYTE_STRING == type)
			{
				// check if the data pointer is not pointing to value
				if ((dataPtr != NULL) && (dataPtr != &value))
				{
					// must have acquired storage so free it now
					rfhFree(dataPtr);
				}
			}

			// check if it worked
			if (cc != MQCC_OK)
			{
				// report the error
				// Set failed, get the reason and generate an error message
				setErrorMsg(cc, rc, "PubXMQSetMp");
			}
			else
			{
				// count the property that was added
				propCount++;
			}
		}

		// skip the end of the NULL character
		ptr++;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::pubWriteMsgAddProps propCount=%d cc=%d rc=%d count=%I64d type=%d valueLen=%d chgTopic=%d foundRetain=%d ptr=%8.8X endPtr=%8.8X parms->userProp=%8.8X name=%.756s", propCount, cc, rc, parms->count, type, valueLen, chgTopic, foundRetain, (unsigned int)ptr, (unsigned int)endPtr, (unsigned int)parms->userProp, name);

		// trace exit from pubWriteMsgAddProps
		logTraceEntry(traceInfo);
	}

	return propCount;
}

MQLONG DataArea::pubWriteMsgOpen(WRITEPARMS *parms)

{
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=MQRC_NONE;					// MQ reason code
	MQLONG			openOpts=0;
	int				tempOpt;
	MQMD2			mqmd={MQMD2_DEFAULT};			// Message descriptor
	MQPMO			pmo={MQPMO_DEFAULT};			// Put message options
	MQOD			od={MQOD_DEFAULT};				// MQ open descriptor
	char			trace1[16];						// work area for trace info
	char			traceInfo[1024];				// work variable to build trace message

	// make sure we have a topic name
	if ((NULL == parms->topic) || (strlen(parms->topic) == 0))
	{
		strcpy(parms->errMsg, "*Topic is required*\r\n");
		return -1;
	}

	// make sure we are connected to the correct QM
	if (!checkConnection(parms->QMname))
	{
		return -2;
	}

	// check the level of the QMgr
	if (level < MQCMDL_LEVEL_700)
	{
		// QMgr level must be at least V7 to get sub or topic names
		strcpy(parms->errMsg, "QMgr must be V7 or above");
		discQM();

		if (traceEnabled)
		{
			// log the problem
			logTraceEntry("Queue manager level below 7.0");
		}

		return -3;
	}

	// set the open options
	openOpts = MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING;

	// set up the object descriptor for the topic
	od.Version = MQOD_VERSION_4;
	od.ObjectType = MQOT_TOPIC;
	od.ObjectString.VSPtr = (void *)parms->topic;
	od.ObjectString.VSLength = MQVS_NULL_TERMINATED;

	// capture the full topic string after the open into the parameters area
	od.ResObjectString.VSPtr = (void *)&(parms->resTopicStr);
	od.ResObjectString.VSLength = MQVS_NULL_TERMINATED;
	od.ResObjectString.VSBufSize = MQ_TOPIC_STR_LENGTH;

	// clear the current resolved topic string
	memset(&(parms->resTopicStr), 0, sizeof(parms->resTopicStr));

	if (traceEnabled)
	{
		// build the trace line
		memset(trace1, 0, sizeof(trace1));
		tempOpt = reverseBytes4(openOpts);
		AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace1);
		sprintf(traceInfo, "Open options X\'%s\' topic=%.512s", trace1, &(parms->resTopicStr));

		// write a trace log entry
		logTraceEntry(traceInfo);

		if (verboseTrace)
		{
			// dump the contents of the open descriptor to the trace
			dumpTraceData("MQOD", (unsigned char *)&od, sizeof(od));

			// dump out the topic string
			if ((parms->topic != NULL) && (strlen(parms->topic) > 0))
			{
				// dump out the topic string
				dumpTraceData("topic", (unsigned char *)parms->topic, strlen(parms->topic));
			}
		}
	}

	// try to open the topic
	XMQOpen(qm, &od, openOpts, &(parms->hTopic), &cc, &rc);

	// check if it worked
	if (cc != MQCC_OK)
	{
		// open failed, get the reason and generate an error message
		setErrorMsg(cc, rc, "PubOpen");
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::pubWriteMsgOpen cc=%d rc=%d count=%I64d", cc, rc, parms->count);

		// trace exit from pubWriteMsgOpen
		logTraceEntry(traceInfo);
	}

	return rc;
}

MQLONG DataArea::pubWriteMsgClose(WRITEPARMS *parms)

{
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=MQRC_NONE;					// MQ reason code
	char			traceInfo[128];				// work variable to build trace message

	// close the topic object
	XMQClose(qm, &(parms->hTopic), MQCO_NONE, &cc, &rc);

	// check if it worked
	if (cc != MQCC_OK)
	{
		// unable to close the handle - report and trace the error
		setErrorMsg(cc, rc, "MQCLOSE");
	}

	// clear the topic handle and resolved topic string
	parms->hTopic = NULL;
	memset(parms->resTopicStr, 0, sizeof(parms->resTopicStr));

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::pubWriteMsgClose cc=%d rc=%d uow=%d count=%I64d", cc, rc, parms->uow, parms->count);

		// trace exit from pubWriteMsgClose
		logTraceEntry(traceInfo);
	}

	return rc;
}

MQLONG DataArea::pubWriteMsgCommit(WRITEPARMS *parms)

{
	MQLONG			cc=MQCC_OK;					// MQ completion code
	MQLONG			rc=MQRC_NONE;				// MQ reason code
	MQLONG			uow;						// variable to hold uow count for trace
	char			traceInfo[128];				// work variable to build trace message

	// commit the messages
	XMQCmit(qm, &cc, &rc);

	// reset the unit of work count
	uow = parms->uow;
	parms->uow = 0;

	// check if it worked
	if (cc != MQCC_OK)
	{
		// unable to commit the unit of work - report and trace the error
		setErrorMsg(cc, rc, "MQCMIT");
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::pubWriteMsgCommit cc=%d rc=%d uow=%d count=%I64d", cc, rc, uow, parms->count);

		// trace exit from pubWriteMsgCommit
		logTraceEntry(traceInfo);
	}

	return rc;
}

MQLONG DataArea::pubWriteMsg(WRITEPARMS *parms)

{
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=MQRC_NONE;					// MQ reason code
	MQLONG			cc2=MQCC_OK;					// MQ completion code
	MQLONG			rc2=MQRC_NONE;					// MQ reason code
	MQHMSG			hMsg=MQHM_UNUSABLE_HMSG;		// message handle used to set message properties
	int				tempOpt;
	int				propCount=0;					// number of message properties added to handle
	MQMD2			mqmd={MQMD2_DEFAULT};			// Message descriptor
	MQPMO			pmo={MQPMO_DEFAULT};			// Put message options
	MQCMHO			opts={MQCMHO_DEFAULT};			// options used to create message handle
	MQDMHO			dOpts={MQDMHO_DEFAULT};			// options used to delete message handle
	char			trace1[16];						// work area for trace info
	char			traceInfo[1024];				// work variable to build trace message

	// make sure there is a connection the queue manager
	connect2QM(parms->QMname);

	// check if the batchsize is greater than one
	if (parms->batchSize > 1)
	{
		// set the initial PMO Options
		// use a unit of work
		pmo.Options = MQPMO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING;
	}
	else
	{
		// set the initial PMO Options
		// batchsize is one - no need for a unit of work
		pmo.Options = MQPMO_NO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING;
	}

	// check if the new message id option is to be set
	if (parms->new_msg_id)
	{
		// set the new message id option
		pmo.Options |= MQPMO_NEW_MSG_ID;
	}

	// check if the new correlation id option is to be set
	if (parms->new_correl_id)
	{
		// set the new correlation id option
		pmo.Options |= MQPMO_NEW_CORREL_ID;
	}

	if (parms->retained)
	{
		pmo.Options |= MQPMO_RETAIN;
		pmo.Version = MQPMO_VERSION_3;
	}

	if (parms->local)
	{
		pmo.Options |= MQPMO_SCOPE_QMGR;
		pmo.Version = MQPMO_VERSION_3;
	}

	if (parms->suppressReply)
	{
		pmo.Options |= MQPMO_SUPPRESS_REPLYTO;
		pmo.Version = MQPMO_VERSION_3;
	}

	// check for a publication level
	if (parms->pubLevel > 0)
	{
		pmo.PubLevel = parms->pubLevel;
		pmo.Version = MQPMO_VERSION_3;
	}

	// check for the new warn if no pubs option
	if ((level >= MQCMDL_LEVEL_701) && (parms->warnNoSub))
	{
		pmo.Options |= MQPMO_WARN_IF_NO_SUBS_MATCHED;
	}

	// check if message properties are to be processed
	if (parms->useProps || parms->useTopic)
	{
		// make sure that the message handle processing options are honored
		pmo.Version = MQPMO_VERSION_3;

		// set the message handle options
		opts.Options = MQCMHO_VALIDATE;

		// create a message handle
		XMQCrtMh(qm, &opts, &hMsg, &cc, &rc);

		// check if it was created
		if (cc != MQCC_OK)
		{
			// unable to create the message handle - report the error
			setErrorMsg(cc, rc, "MQCRTMH");

			// return noting error
			return rc;
		}

		// message handle has been created
		// set the message properties
		propCount = pubWriteMsgAddProps(parms, hMsg);

		// check if there were any user properties
		if (propCount > 0)
		{
			// set the handle in the PMO and the action
			pmo.OriginalMsgHandle = parms->hTopic;
			pmo.Action = MQACTP_NEW;
		}
	}

	if (traceEnabled)
	{
		// build the trace line
		memset(trace1, 0, sizeof(trace1));
		tempOpt = pmo.Options;
		tempOpt = reverseBytes4(tempOpt);
		AsciiToHex((unsigned char *)(&tempOpt), 4, (unsigned char *)trace1);
		sprintf(traceInfo, "publishing message using options X\'%s\'", trace1);

		// write a trace log entry
		logTraceEntry(traceInfo);

		// check for verbose trace
		if (verboseTrace)
		{
			// dump the contents of the mqmd and pmo to the trace
			dumpTraceData("MQMD", (unsigned char *)&mqmd, sizeof(mqmd));
			dumpTraceData("PMO", (unsigned char *)&pmo, sizeof(pmo));
		}
	}

	// did the topic name change?
	if ((parms->resTopicStr[0] != 0) && (strcmp(parms->resTopicStr, parms->topic) != 0))
	{
		// is there anything to commit?
		if (parms->uow > 0)
		{
			// force a commit
			pubWriteMsgCommit(parms);
		}

		// close the existing topic
		pubWriteMsgClose(parms);
	}

	// check if the topic is already open
	if ((0 == parms->resTopicStr[0]) || (NULL == parms->hTopic))
	{
		// try to connect to the queue manager and open the topic
		rc = pubWriteMsgOpen(parms);

		if (rc != MQRC_NONE)
		{
			// return with an error
			return rc;
		}
	}

	// try to send the message
	XMQPut(qm, (parms->hTopic), &mqmd, &pmo, parms->msgSize, parms->msgData, &cc, &rc);

	// check for a no publish message
	if (MQRC_NO_SUBS_MATCHED == rc)
	{
		// count the number of publications where there were no subscribers
		parms->noMatchCount++;

		// change the return code to normal
		cc = MQCC_OK;
		rc = MQRC_NONE;
	}

	// check if a message handle was created
	if (hMsg != MQHM_UNUSABLE_HMSG)
	{
		// delete the message handle
		XMQDltMh(qm, &hMsg, &dOpts, &cc2, &rc2);

		// check if it worked
		if (cc2 != MQCC_OK)
		{
			// unable to delete the message handle - report and trace the error
			setErrorMsg(cc2, rc2, "MQDLTMH");

			// set the error indicator to stop the publication
			parms->err = 1;
		}
	}

	// check the results
	if (MQCC_OK == cc)
	{
		// increment the message and batch counters
		parms->count++;
		parms->batch++;

		// capture the total number of bytes written
		parms->totalBytes += parms->msgSize;

		// check if units of work are being used
		if (parms->batchSize > 1)
		{
			// increase the uow count - this will eventually force a commit
			parms->uow++;
		}
	}
	else
	{
		// put failed, get the reason and generate an error message
		// the error flag will set to stop the publication
		parms->err = 1;

		// get the completion code and reason code
		setErrorMsg(cc, rc, "Put");

		// was this a warning?
		if (MQCC_FAILED == cc)
		{
			// check for a unit of work
			if (unitOfWorkActive)
			{
				rollbackUOW();
			}
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::pubWriteMsg propCount=%d cc=%d rc=%d parms->count=%I64d parms->msgSize=%d parms->uow=%d parms->err=%d parms->topic=%.756s", propCount, cc, rc, parms->count, parms->msgSize, parms->uow, parms->err, parms->topic);

		// trace exit from pubWriteMsgAddProps
		logTraceEntry(traceInfo);
	}

	return rc;
}

int DataArea::loadNames(const char * QMname, const char * brokerQM, int nameType, MQLONG qType, BOOL inclCluster, BOOL resetList)

{
	MQLONG			rc2;
	MQLONG			rc=MQCC_OK;							// routine return code
	MQLONG			cc;									// MQ completion code
	int				ret=0;								// return code
	NAMESTRUCT		*namesPtr=NULL;						// Ptr to name table
	CRfhutilApp *	app;								// pointer to the MFC application object
	CString			saveInitQMname;						// working variable
	char			replyQName[MQ_Q_NAME_LENGTH + 4];	// name of the temporary queue for PCF replies
	char			traceInfo[512];						// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::loadNames nameType=%d QMname=%s brokerQM=%s qType=%d inclCluster=%d resetList=%d", nameType, QMname, brokerQM, qType, inclCluster, resetList);

		// trace entry to loadNames
		logTraceEntry(traceInfo);
	}

	// clear the name of the reply queue
	memset(replyQName, 0, sizeof(replyQName));

	// save the initial queue manager name - otherwise the connection will change the
	// saved queue manager name to reflect the last local queue manager name that was
	// queried during startup
	app = (CRfhutilApp *)AfxGetApp();
	saveInitQMname = app->initQMname;

	// connect to the queue manager
	if (!connect2QM(QMname))
	{

		// return indicating failure
		return -1;
	}

	// restore the initial queue manager name
	app->initQMname = saveInitQMname;

	// check if V7 is required
	if ((nameType != LOAD_NAMES_QUEUES) && (level < MQCMDL_LEVEL_700))
	{
		// QMgr level must be at least V7 to get sub or topic names
		m_error_msg = "QMgr must be V7 or above";
		discQM();

		if (traceEnabled)
		{
			// log the problem
			logTraceEntry("Queue manager level below 7.0");
		}

		return -1;
	}

	// check for MVS before V6
	// Since MQ on MVS does not support PCF, the list of queue names cannot be retrieved.
	if ((MQPL_MVS == platform) && (level < MQCMDL_LEVEL_600))
	{
		// earlier version of MVS - doesn't support PCF
		// issue error message so user knows what is going on and return
		m_error_msg = "MQ on 390 does not support PCF prior to V6";
		discQM();

		if (traceEnabled)
		{
			// log the problem
			logTraceEntry("MVS Queue manager level below 6.0");
		}

		return -1;
	}

	// create a temporary queue to receive the replies from the queue manager
	cc = OpenTempQ( QMname, "RFHUTIL.REPLY.QUEUE*\0", MQOO_INPUT_SHARED, replyQName, &rc );

	// check if it worked
	if (cc != MQCC_OK)
	{
		// build an error message so user knows what happened
		m_error_msg.Format("Open for temporary reply queue failed rc=%d", rc);

		// disconnect from the queue manager
		discQM();

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "ReplyQ open failed cc=%d", cc);

			// log the problem
			logTraceEntry(traceInfo);
		}

		// return indicating an error occurred
		return cc;
	}

	// open the admin queue as the normal RFHUtil queue, specifying output usage
	cc = openAdminQ( QMname, brokerQM, &rc );

	// check if it worked
	if (cc != MQCC_OK)
	{
		// build an error message so the user knows what happened
		m_error_msg.Format("Open for admin queue failed rc=%d", rc);

		// close and delete the temporary reply queue
		closeReplyQ();

		// disconnect from the queue manager
		discQM();

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "AdminQ open failed rc=%d", rc);

			// log the problem
			logTraceEntry(traceInfo);
		}

		// return indicating an error occurred
		return cc;
	}

	// check what type of names are to be loaded
	if (LOAD_NAMES_SUBS == nameType)
	{
		if (strlen(brokerQM) > 0)
		{
			// get a pointer to the subscription names structure
			namesPtr = getSubNamesPtr(brokerQM, TRUE);
		}
		else
		{
			// get a pointer to the subscription names structure
			namesPtr = getSubNamesPtr(QMname, TRUE);
		}

		// get the subscription names
		ret = getListOfNames(namesPtr, brokerQM, QMname, replyQName, nameType, 0, FALSE);
	}
	else if (LOAD_NAMES_TOPICS == nameType)
	{
		if (strlen(brokerQM) > 0)
		{
			// get a pointer to the topic names structure
			namesPtr = getTopicNamesPtr(brokerQM, TRUE);
		}
		else
		{
			// get a pointer to the topic names structure
			namesPtr = getTopicNamesPtr(QMname, TRUE);
		}

		// get topic names
		ret = getListOfNames(namesPtr, brokerQM, QMname, replyQName, nameType, 0, FALSE);
	}
	else if (LOAD_NAMES_QUEUES == nameType)
	{
		// get a pointer to the queue names structure
		namesPtr = getQueueNamesPtr(QMname, resetList);

		// get topic names
		ret = getListOfNames(namesPtr, QMname, QMname, replyQName, nameType, qType, m_show_cluster_queues);
	}

	// done - close the queues and disconnect from the queue manager
	closeReplyQ();
	closeAdminQ(&rc2);
	discQM();

	if (traceEnabled)
	{
		// create the trace line showing all the results
		sprintf(traceInfo, "Exiting DataArea::loadNames cc=%d rc=%d rc2=%d ret=%d", cc, rc, rc2, ret);

		// trace exit from loadNames
		logTraceEntry(traceInfo);
	}

	// base the return code on the local queues
	// if this works we have enough
	return ret;
}

/////////////////////////////////////////////
//
// This routine will process a PCF reply
// message to extract the data for a
// particular queue.
//
/////////////////////////////////////////////

void DataArea::processQueuesReply(char *msg, NAMESTRUCT *namesPtr, const char * replyQName, MQLONG ccsid, MQLONG encoding)

{
	int				Index;							// index used within PCF reply messages
	int				cnt;							// counter to optimize performance - know when all parms of interest have been found and stop the loop
	MQLONG			*pPCFType;						// Type field of PCF message parm
	MQCFH			*pPCFHeader;					// Ptr to PCF header parm block
	MQCFST			*pPCFString;					// Ptr to PCF string parm block
	MQCFIN			*pPCFInteger;					// Ptr to PCF integer parm block
	MQCFIL			*pPCFIntList;					// Ptr to PCF integer list block
	MQCFBS			*pPCFByteString;				// Ptr to PCF byte string parm block
	MQCFSL			*pPCFStrList;					// Ptr to PCF string list parm block
	LocalQueueParms	qParms;							// work area to gather topic information from PCF reply message
	char			qName[MQ_Q_NAME_LENGTH + 4];	// name of the queue
	char			traceInfo[512];					// work variable to build trace message

	// point to the first parameter
	pPCFType = (MQLONG *)(msg + MQCFH_STRUC_LENGTH);

	// initialize counter
	Index = 1;

	// clear the queue parameters area
	memset(&qParms, 0, sizeof(qParms));

	// point to the PCF header in the reply message
	pPCFHeader = (MQCFH *)msg;

	cnt = 0;			// performance optimization

	while ( (Index <= pPCFHeader->ParameterCount) && (cnt < 2) )
	{
 		// check if we received a reply in the wrong encoding
		if (getIntEncode(encoding) != NUMERIC_PC)
		{
			(*pPCFType) = (MQLONG)reverseBytes4((*pPCFType));
		}

		// Establish the type of each parameter and allocate
		// a pointer of the correct type to reference it.
		// then move on to the next parameter
		switch ( *pPCFType ) {
		case MQCFT_INTEGER:
			pPCFInteger = (MQCFIN *)pPCFType;
			convertIntHeader(pPCFInteger, encoding);

			// performance optimization - only look for queue type
			if (MQIA_Q_TYPE == pPCFInteger->Parameter)
			{
				// capture the parameter
				qParms.QType = pPCFInteger->Value;

				// found one that we are looking for
				cnt++;
			}

			//ProcessIntegerParm( pPCFInteger, &qParms );
			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFInteger->StrucLength );
			break;
		case MQCFT_STRING:
			pPCFString = (MQCFST *)pPCFType;
			convertStringHeader(pPCFString, ccsid, encoding);

			// performance optimization - only look for queue name
			if (MQCA_Q_NAME == pPCFString->Parameter)
			{
				MQParmCpy( (char *)&qParms.QName, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				cnt++;
			}

			//ProcessStringParm( pPCFString, &qParms );
			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFString->StrucLength );
			break;
		case MQCFT_BYTE_STRING:
			// byte string
			// skip this parameter
			pPCFByteString = (MQCFBS *)pPCFType;
			convertByteStringHeader(pPCFByteString, ccsid, encoding);

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFByteString->StrucLength );
			break;
		case MQCFT_INTEGER_LIST:
			// integer list
			// skip this parameter
			pPCFIntList = (MQCFIL *)pPCFType;
			convertIntListHeader(pPCFIntList, encoding);

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFIntList->StrucLength );
			break;
		case MQCFT_STRING_LIST:
			// integer list
			// skip this parameter
			pPCFStrList = (MQCFSL *)pPCFType;
			convertStrListHeader(pPCFStrList, ccsid, encoding);

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFStrList->StrucLength );
			break;
		default:
			// don't recognize this - get the structure length and skip it
			pPCFInteger = (MQCFIN *)pPCFType;

			if (getIntEncode(encoding) != NUMERIC_PC)
			{
				pPCFInteger->StrucLength = reverseBytes4(pPCFInteger->StrucLength);
			}

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFInteger->StrucLength );
			break;
		} // endswitch

		Index++;
	}

	if ((qParms.QType > 0) && (strcmp(replyQName, qParms.QName) != 0))
	{
		// capture the queue name
		switch (qParms.QType)
		{
		case MQQT_LOCAL:
			{
				qName[0] = 'L';		// normal local queue
				lcount++;			// count in case trace is turned on
				break;
			}
		case MQQT_MODEL:
			{
				qName[0] = 'M';		// model queue
				mcount++;			// count in case trace is turned on
				break;
			}
		case MQQT_ALIAS:
			{
				qName[0] = 'A';		// alias queue
				acount++;			// count in case trace is turned on
				break;
			}
		case MQQT_REMOTE:
			{
				qName[0] = 'R';		// remote queue definition
				rcount++;			// count in case trace is turned on
				break;
			}
		case MQQT_CLUSTER:
			{
				qName[0] = 'C';		// clustered queue
				ccount++;			// count in case trace is turned on
				break;
			}
		default:
			{
				qName[0] = ' ';		// unrecognized - make sure we behave normally
				ocount++;			// count in case trace is turned on
				break;
			}
		}

		if (qName[0] != 'M')
		{
			// finish building the name
			strcpy(qName + 1, qParms.QName);

			// insert the queue name into the table
			namesPtr->names->insertName(qName);

			// increment the queue count
			namesPtr->count++;
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::processQueuesReply Queue Type=%d Name=%s", qParms.QType, qName);

		// trace entry to routine
		logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////////
//
// This routine will process a PCF reply
// message to extract the data for a
// particular topic.
//
/////////////////////////////////////////////

void DataArea::processTopicReply(char * msg, NAMESTRUCT * namesPtr, MQLONG ccsid, MQLONG encoding)

{
	int				Index;							// index used within PCF reply messages
	MQLONG			*pPCFType;						// Type field of PCF message parm
	MQCFH			*pPCFHeader;					// Ptr to PCF header parm block
	MQCFST			*pPCFString;					// Ptr to PCF string parm block
	MQCFIN			*pPCFInteger;					// Ptr to PCF integer parm block
	MQCFIL			*pPCFIntList;					// Ptr to PCF integer list block
	MQCFBS			*pPCFByteString;				// Ptr to PCF byte string parm block
	MQCFSL			*pPCFStrList;					// Ptr to PCF string list parm block
	char			traceInfo[512];					// work variable to build trace message
	LocalTopicParms	topicParms;						// work area to gather queue information from PCF reply message

	// point to the first parameter
	pPCFType = (MQLONG *)(msg + MQCFH_STRUC_LENGTH);

	// initialize counter
	Index = 1;

	// clear the queue parameters area
	memset(&topicParms, 0, sizeof(topicParms));

	// point to the PCF header in the reply message
	pPCFHeader = (MQCFH *)msg;

	while (Index <= pPCFHeader->ParameterCount)
	{
 		// check if we received a reply in the wrong encoding
		if (getIntEncode(encoding) != NUMERIC_PC)
		{
			(*pPCFType) = (MQLONG)reverseBytes4((*pPCFType));
		}

		// Establish the type of each parameter and allocate
		// a pointer of the correct type to reference it.
		// then move on to the next parameter
		switch ( *pPCFType ) {
		case MQCFT_INTEGER:
			pPCFInteger = (MQCFIN *)pPCFType;
			convertIntHeader(pPCFInteger, encoding);

			// check for topic attributes
			if (MQIACF_TOPIC_ATTRS == pPCFInteger->Parameter)
			{
				// capture the parameter
				topicParms.attrs = pPCFInteger->Value;
			}

			// check for topic status
			if (MQIACF_TOPIC_STATUS == pPCFInteger->Parameter)
			{
				// capture the parameter
				topicParms.status = pPCFInteger->Value;
			}

			// check for sub allowed
			if (MQIACF_TOPIC_SUB == pPCFInteger->Parameter)
			{
				// capture the parameter
				topicParms.sub = pPCFInteger->Value;
			}

			// check for pub allowed
			if (MQIACF_TOPIC_PUB == pPCFInteger->Parameter)
			{
				// capture the parameter
				topicParms.pub = pPCFInteger->Value;
			}

			// check for topic type
			if (MQIACF_TOPIC_STATUS_TYPE == pPCFInteger->Parameter)
			{
				// capture the parameter
				topicParms.type = pPCFInteger->Value;
			}

			//ProcessIntegerParm( pPCFInteger, &qParms );
			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFInteger->StrucLength );
			break;
		case MQCFT_STRING:
			pPCFString = (MQCFST *)pPCFType;
			convertStringHeader(pPCFString, ccsid, encoding);

			// check for topic string
			if (MQCA_TOPIC_STRING == pPCFString->Parameter)
			{
				// found topic string
				MQParmCpy( (char *)&topicParms.topic, (char *)&pPCFString->String, pPCFString->StringLength );
			}

			// check for topic name
			if (MQCA_TOPIC_NAME == pPCFString->Parameter)
			{
				// found topic name
				MQParmCpy( (char *)&topicParms.Name, (char *)&pPCFString->String, pPCFString->StringLength );
			}

			//ProcessStringParm( pPCFString, &qParms );
			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFString->StrucLength );
			break;
		case MQCFT_BYTE_STRING:
			// byte string
			// skip this parameter
			pPCFByteString = (MQCFBS *)pPCFType;
			convertByteStringHeader(pPCFByteString, ccsid, encoding);

			// check for subscription id
/*			if (MQBACF_SUB_ID == pPCFByteString->Parameter)
			{
				len = pPCFByteString->StringLength;

				// check if it fits
				if (len > sizeof(subParms.subID))
				{
					len = sizeof(subParms.subID);
				}

				// capture the data
				memcpy(subParms.subID, pPCFByteString->String, len);
			}*/

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFByteString->StrucLength );
			break;
		case MQCFT_INTEGER_LIST:
			// integer list
			// skip this parameter
			pPCFIntList = (MQCFIL *)pPCFType;
			convertIntListHeader(pPCFIntList, encoding);

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFIntList->StrucLength );
			break;
		case MQCFT_STRING_LIST:
			// integer list
			// skip this parameter
			pPCFStrList = (MQCFSL *)pPCFType;
			convertStrListHeader(pPCFStrList, ccsid, encoding);

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFStrList->StrucLength );
			break;
		default:
			// don't recognize this - get the structure length and skip it
			pPCFInteger = (MQCFIN *)pPCFType;

			if (getIntEncode(encoding) != NUMERIC_PC)
			{
				pPCFInteger->StrucLength = reverseBytes4(pPCFInteger->StrucLength);
			}

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFInteger->StrucLength );
			break;
		} // endswitch

		Index++;
	}

	// insert the queue name into the table
	namesPtr->names->insertName(topicParms.Name);

	// increment the queue count
	namesPtr->count++;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, " topic name=%.384s", topicParms.Name);

		// trace entry to routine
		logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////////
//
// This routine will process a PCF reply
// message to extract the data for a
// particular subscription.
//
// The PCF message contains the subscription
// name as well as whether or not the
// subscription is durable or not and managed
// or not.  It also contains the queue name
// (destination) and queue manager name.
//
// Indicator characters are placed in the
// first two positions of the name string,
// followed by the names of the subscription,
// queue and queue manager respectively.
// Ampersand characters are used to delimit
// the names.
//
/////////////////////////////////////////////

void DataArea::processSubNamesReply(char * msg, NAMESTRUCT * namesPtr, MQLONG ccsid, MQLONG encoding)

{
	int				Index;							// index used within PCF reply messages
	int				cnt;							// count of number of specific parameters found that are needed - for performance
	int				len;							// work variable to capture lengths
	MQLONG			*pPCFType;						// Type field of PCF message parm
	MQCFH			*pPCFHeader;					// Ptr to PCF header parm block
	MQCFST			*pPCFString;					// Ptr to PCF string parm block
	MQCFIN			*pPCFInteger;					// Ptr to PCF integer parm block
	MQCFIL			*pPCFIntList;					// Ptr to PCF integer list block
	MQCFBS			*pPCFByteString;				// Ptr to PCF byte string parm block
	MQCFSL			*pPCFStrList;					// Ptr to PCF string list parm block
	char			sName[MQ_SUB_NAME_LENGTH+MQ_Q_MGR_NAME_LENGTH+MQ_Q_NAME_LENGTH+8];	// saved subscription name - consists of character D for durable followed by subname;
	char			traceInfo[512];					// work variable to build trace message
	LocalSubParms	subParms;						// work area to gather subscription information from PCF reply message

	// point to the first parameter
	pPCFType = (MQLONG *)(msg + MQCFH_STRUC_LENGTH);

	Index = 1;
	cnt = 0;			// performance optimization

	// clear the queue parameters area
	memset(&subParms, 0, sizeof(subParms));

	// point to the PCF header in the reply message
	pPCFHeader = (MQCFH *)msg;

	while ( (Index <= pPCFHeader->ParameterCount) && (cnt < 5) )
	{
 		// check if we received a reply in the wrong encoding
		if (getIntEncode(encoding) != NUMERIC_PC)
		{
			(*pPCFType) = (MQLONG)reverseBytes4((*pPCFType));
		}

		// Establish the type of each parameter and allocate
		// a pointer of the correct type to reference it.
		// then move on to the next parameter
		switch ( *pPCFType ) {
		case MQCFT_INTEGER:
			pPCFInteger = (MQCFIN *)pPCFType;
			convertIntHeader(pPCFInteger, encoding);

			// check for subscription type
			if (MQIACF_SUB_TYPE == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.subType = pPCFInteger->Value;
			}

			// check for destination class
			if (MQIACF_DESTINATION_CLASS == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.destClass = pPCFInteger->Value;
			}

			// check for durable subscription
			if (MQIACF_DURABLE_SUBSCRIPTION == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.durable = pPCFInteger->Value;

				// performance optimization - only look for subscription type and duration
				// found one that we are looking for
				cnt++;
			}

			// check for subscription expiry
			if (MQIACF_EXPIRY == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.expiry = pPCFInteger->Value;
			}

			// check for subscription properties
			if (MQIACF_PUBSUB_PROPERTIES == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.properties = pPCFInteger->Value;
			}

			// check for subscription priority
			if (MQIACF_PUB_PRIORITY == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.priority = pPCFInteger->Value;
			}

			// check for subscription request only
			if (MQIACF_REQUEST_ONLY == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.requestOnly = pPCFInteger->Value;
			}

			// check for subscription scope
			if (MQIACF_SUBSCRIPTION_SCOPE == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.scope = pPCFInteger->Value;
			}

			// check for subscription level
			if (MQIACF_SUB_LEVEL == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.subLevel = pPCFInteger->Value;
			}

			// check for variable user id
			if (MQIACF_VARIABLE_USER_ID == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.varUser = pPCFInteger->Value;
			}

			// check for wildcard schema
			if (MQIACF_WILDCARD_SCHEMA == pPCFInteger->Parameter)
			{
				// capture the parameter
				subParms.wildCard = pPCFInteger->Value;
			}

			//ProcessIntegerParm( pPCFInteger, &qParms );
			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFInteger->StrucLength );
			break;
		case MQCFT_STRING:
			pPCFString = (MQCFST *)pPCFType;
			convertStringHeader(pPCFString, ccsid, encoding);

			// performance optimization - only look for certain parameters
			if (MQCACF_SUB_NAME == pPCFString->Parameter)
			{
				MQParmCpy( (char *)&subParms.Name, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				cnt++;
			}

			// check for topic string
			if (MQCA_TOPIC_STRING == pPCFString->Parameter)
			{
				// found topic string
				// ignore this for now
				//MQParmCpy( (char *)&subParms.topic, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			// check for topic name
			if (MQCA_TOPIC_NAME == pPCFString->Parameter)
			{
				// found topic name
				// ignore this for now
				//MQParmCpy( (char *)&subParms.topicName, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			// check for destination
			if (MQCACF_DESTINATION == pPCFString->Parameter)
			{
				// found destination (queue name)
				MQParmCpy( (char *)&subParms.destName, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				cnt++;
			}

			// check for destination QM name
			if (MQCACF_DESTINATION_Q_MGR == pPCFString->Parameter)
			{
				// found destination QM name
				MQParmCpy( (char *)&subParms.destQMName, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				cnt++;
			}

			// check for application identity
			if (MQCACF_APPL_IDENTITY_DATA == pPCFString->Parameter)
			{
				// found application identity
				// ignore this for now
				//MQParmCpy( (char *)&subParms.ApplIdentity, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			// check for selector
			if (MQCACF_SUB_SELECTOR == pPCFString->Parameter)
			{
				// found selector
				// ignore this for now
				//MQParmCpy( (char *)&subParms.selector, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			// check for user data
			if (MQCACF_SUB_USER_DATA == pPCFString->Parameter)
			{
				// found user data
				// ignore this for now
				//MQParmCpy( (char *)&subParms.userData, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			// check for user id
			if (MQCACF_SUB_USER_ID == pPCFString->Parameter)
			{
				// found user id
				// ignore this for now
				//MQParmCpy( (char *)&subParms.userID, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			// check for creation date
			if (MQCA_CREATION_DATE == pPCFString->Parameter)
			{
				// found creation date
				// ignore this for now
				//MQParmCpy( (char *)&subParms.creationDate, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			// check for creation time
			if (MQCA_CREATION_TIME == pPCFString->Parameter)
			{
				// found creation time
				// ignore this for now
				//MQParmCpy( (char *)&subParms.creationTime, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			// check for alteration date
			if (MQCA_ALTERATION_DATE == pPCFString->Parameter)
			{
				// found creation date
				// ignore this for now
				//MQParmCpy( (char *)&subParms.alterationDate, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			// check for alteration time
			if (MQCA_ALTERATION_TIME == pPCFString->Parameter)
			{
				// found alteration time
				// ignore this for now
				//MQParmCpy( (char *)&subParms.alterationTime, (char *)&pPCFString->String, pPCFString->StringLength );

				// found one that we are looking for
				//cnt++;
			}

			//ProcessStringParm( pPCFString, &qParms );
			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFString->StrucLength );
			break;
		case MQCFT_BYTE_STRING:
			// byte string
			// skip this parameter
			pPCFByteString = (MQCFBS *)pPCFType;
			convertByteStringHeader(pPCFByteString, ccsid, encoding);

			// check for subscription id
			if (MQBACF_SUB_ID == pPCFByteString->Parameter)
			{
				len = pPCFByteString->StringLength;

				// check if it fits
				if (len > sizeof(subParms.subID))
				{
					len = sizeof(subParms.subID);
				}

				// capture the data
				memcpy(subParms.subID, pPCFByteString->String, len);
			}

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFByteString->StrucLength );
			break;
		case MQCFT_INTEGER_LIST:
			// integer list
			// skip this parameter
			pPCFIntList = (MQCFIL *)pPCFType;
			convertIntListHeader(pPCFIntList, encoding);

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFIntList->StrucLength );
			break;
		case MQCFT_STRING_LIST:
			// integer list
			// skip this parameter
			pPCFStrList = (MQCFSL *)pPCFType;
			convertStrListHeader(pPCFStrList, ccsid, encoding);

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFStrList->StrucLength );
			break;
		default:
			// don't recognize this - get the structure length and skip it
			pPCFInteger = (MQCFIN *)pPCFType;

			if (getIntEncode(encoding) != NUMERIC_PC)
			{
				pPCFInteger->StrucLength = reverseBytes4(pPCFInteger->StrucLength);
			}

			// Increment the pointer to the next parameter by the
			// length of the current parm.
			pPCFType = (MQLONG *)( (MQBYTE *)pPCFType + pPCFInteger->StrucLength );
			break;
		} // endswitch

		Index++;
	}

	// capture the durable subscriptions
	if (MQSUB_DURABLE_YES == subParms.durable)
	{
		// durable subscription
		sName[0] = 'D';
	}
	else if (MQSUB_DURABLE_NO == subParms.durable)
	{
		// non-durable subscription
		sName[0] = 'N';
	}
	else
	{
		// not recognized
		sName[0] = 'X';
	}

	// check for a managed subscription
	if (MQDC_MANAGED == subParms.destClass)
	{
		// managed subscription
		sName[1] = 'M';
	}
	else if (MQDC_PROVIDED == subParms.destClass)
	{
		// provided subscription
		sName[1] = 'P';
	}
	else
	{
		// not recognized - do not try to set the managed flag
		sName[1] = 'X';
	}

	// finish building the name
	strcpy(sName + 2, subParms.Name);

	// capture the destination queue name and destination QMgr name
	strcat(sName + 2, "&");
	strcat(sName + 2, subParms.destName);
	strcat(sName + 2, "&");
	strcat(sName + 2, subParms.destQMName);

	// insert the queue name into the table
	namesPtr->names->insertName(sName);

	// increment the queue count
	namesPtr->count++;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, " durable=%d destClass=%d Name=%.384s", subParms.durable, subParms.destClass, sName + 2);

		// trace entry to routine
		logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////////
//
// This routine builds the PCF request
// message to get a list of queue
// names to be used in a drop down list
//
/////////////////////////////////////////////

void DataArea::buildPCFRequestQueues(char * pAdminMsg, MQLONG qType, BOOL inclCluster)

{
	MQCFH			*pPCFHeader;					// Ptr to PCF header parm block
	MQCFST			*pPCFString;					// Ptr to PCF string parm block
	MQCFIN			*pPCFInteger;					// Ptr to PCF integer parm block
	MQCFIL			*pPCFIntList;					// Ptr to PCF integer list block
	char			traceInfo[512];					// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::buildPCFRequestQueues qType=%d platform=%d level=%d inclCluster=%d", qType, platform, level, inclCluster);

		// trace entry to routine
		logTraceEntry(traceInfo);
	}

	////////////////////////////////////////////////////////////////////////
	//
	// Put a message to the SYSTEM.ADMIN.COMMAND.QUEUE to inquire all
	// the local queues defined on the queue manager.
	//
	// The request consists of a Request Header and a parameter block
	// used to specify the generic search. The header and the parameter
	// block follow each other in a contiguous buffer which is pointed
	// to by the variable pAdminMsg. This entire buffer is then put to
	// the queue.
	//
	// The command server, (use STRMQCSV to start it), processes the
	// SYSTEM.ADMIN.COMMAND.QUEUE and puts a reply on the application
	// ReplyToQ for each defined queue.
	//
	////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	//
	// Set pointers to message data buffers
	//
	// pAdminMsg points to the start of the message buffer
	//
	// pPCFHeader also points to the start of the message buffer. It is
	// used to indicate the type of command we wish to execute and the
	// number of parameter blocks following in the message buffer.
	//
	// pPCFString points into the message buffer immediately after the
	// header and is used to map the following bytes onto a PCF string
	// parameter block. In this case the string is used to indicate the
	// nameof the queue we want details about, * indicating all queues.
	//
	// pPCFInteger points into the message buffer immediately after the
	// string block described above. It is used to map the following
	// bytes onto a PCF integer parameter block. This block indicates
	// the type of queue we wish to receive details about, thereby
	// qualifying the generic search set up by passing the previous
	// string parameter.
	//
	// Note that this example is a generic search for all attributes of
	// all local queues known to the queue manager. By using different,
	// or more, parameter blocks in the request header it is possible
	// to narrow the search.
	//
	////////////////////////////////////////////////////////////////////////

	pPCFHeader  = (MQCFH *)pAdminMsg;

	pPCFString  = (MQCFST *)(pAdminMsg
						  + MQCFH_STRUC_LENGTH
						  );

	pPCFInteger = (MQCFIN *)( pAdminMsg
						  + MQCFH_STRUC_LENGTH
						  + MQCFST_STRUC_LENGTH_FIXED + 4
						  );

	// check if we are V6 or above
	if (level >= MQCMDL_LEVEL_600)
	{
		// we need to use version 3 for Z/OS
		// it only accepts version 3
		pPCFHeader->Type    = MQCFT_COMMAND_XR;
		pPCFHeader->Version = MQCFH_VERSION_3;
	}
	else
	{
		// use a lower level, since the queue manager
		// does not recognize version 3
		pPCFHeader->Type    = MQCFT_COMMAND;
		pPCFHeader->Version = MQCFH_VERSION_1;
	}

	// Setup the rest of the request header
	pPCFHeader->StrucLength    = MQCFH_STRUC_LENGTH;
	pPCFHeader->Command        = MQCMD_INQUIRE_Q;
	pPCFHeader->MsgSeqNumber   = 1;
	pPCFHeader->Control        = MQCFC_LAST;
	pPCFHeader->CompCode       = 0;
	pPCFHeader->Reason         = 0;
	pPCFHeader->ParameterCount = 3;

	// Setup parameter block
	pPCFString->Type           = MQCFT_STRING;
	pPCFString->StrucLength    = MQCFST_STRUC_LENGTH_FIXED + 4;
	pPCFString->Parameter      = MQCA_Q_NAME;
	pPCFString->CodedCharSetId = MQCCSI_DEFAULT;
	pPCFString->StringLength   = 1;
	memset( pPCFString->String, 0, 4 );
	pPCFString->String[0] = '*';

	// check if we need to translate the parameter to EBCDIC
//	if ((getCcsidType(currQMptr->ccsid) == CHAR_EBCDIC) || (MQPL_MVS == platform))
//	{
		// translate the parameter to EBCDIC
//		AsciiToEbcdic((unsigned char *)&pPCFString->String, MQ_Q_NAME_LENGTH, (unsigned char *)&pPCFString->String);
//	}

	// Setup parameter block
	pPCFInteger->Type        = MQCFT_INTEGER;
	pPCFInteger->StrucLength = MQCFIN_STRUC_LENGTH;
	pPCFInteger->Parameter   = MQIA_Q_TYPE;
	pPCFInteger->Value       = qType;

	// check for a cluster queue query
	if (inclCluster)
	{
		// must add a filter parameter
		// this parameter is necessary to get cluster queue names
		// it is the CLUSINFO parameter on a RUNMQSC command of DIS Q(*) CLUSINFO
		// Setup parameter block
		pPCFInteger++;
		pPCFInteger->Type        = MQCFT_INTEGER;
		pPCFInteger->StrucLength = MQCFIN_STRUC_LENGTH;
		pPCFInteger->Parameter   = MQIACF_CLUSTER_INFO;
		pPCFInteger->Value       = 0;

		// update the number of parameters in the message
		pPCFHeader->ParameterCount++;

		// there are two integer parameters in the message
		pPCFIntList = (MQCFIL *)( pAdminMsg
					  + MQCFH_STRUC_LENGTH
					  + MQCFST_STRUC_LENGTH_FIXED + 4
					  + (MQCFIN_STRUC_LENGTH * 2)
					  );
	}
	else
	{
		// just one integer parameter in the message
		pPCFIntList = (MQCFIL *)( pAdminMsg
					  + MQCFH_STRUC_LENGTH
					  + MQCFST_STRUC_LENGTH_FIXED + 4
					  + MQCFIN_STRUC_LENGTH
					  );
	}

	// finally build an integer list to request only the queue name and queue type in the reply messages
	pPCFIntList->Type = MQCFT_INTEGER_LIST;
	pPCFIntList->StrucLength	= MQCFIL_STRUC_LENGTH_FIXED + 8;
	pPCFIntList->Parameter		= MQIACF_Q_ATTRS;
	pPCFIntList->Count			= 2;
	pPCFIntList->Values[0]		= MQCA_Q_NAME;
	pPCFIntList->Values[1]		= MQIA_Q_TYPE;

}

/////////////////////////////////////////////
//
// This routine builds the PCF request
// message to get a list of topic
// names to be used in a drop down list
//
/////////////////////////////////////////////

void DataArea::buildPCFRequestTopics(char * pAdminMsg)

{
	MQCFH			*pPCFHeader;					// Ptr to PCF header parm block
	MQCFST			*pPCFString;					// Ptr to PCF string parm block
	MQCFIL			*pPCFIntList;					// Ptr to PCF integer list block

	// build the appropriate request message
	// initialize pointers
	pPCFHeader = (MQCFH *)pAdminMsg;

	pPCFString  = (MQCFST *)(pAdminMsg
						  + MQCFH_STRUC_LENGTH
						  );

	// just one character parameter in the message
	pPCFIntList = (MQCFIL *)( pAdminMsg
				  + MQCFH_STRUC_LENGTH
				  + MQCFST_STRUC_LENGTH_FIXED + 4
				  );

	// check if we are V6 or above
	if (level >= MQCMDL_LEVEL_600)
	{
		// we need to use version 3 for Z/OS
		// it only accepts version 3
		pPCFHeader->Type    = MQCFT_COMMAND_XR;
		pPCFHeader->Version = MQCFH_VERSION_3;
	}
	else
	{
		// use a lower level, since the queue manager
		// does not recognize version 3
		pPCFHeader->Type    = MQCFT_COMMAND;
		pPCFHeader->Version = MQCFH_VERSION_1;
	}

	// Setup the rest of the request header
	pPCFHeader->StrucLength    = MQCFH_STRUC_LENGTH;
	pPCFHeader->Command        = MQCMD_INQUIRE_TOPIC;
	pPCFHeader->MsgSeqNumber   = 1;
	pPCFHeader->Control        = MQCFC_LAST;
	pPCFHeader->CompCode       = 0;
	pPCFHeader->Reason         = 0;
	pPCFHeader->ParameterCount = 2;

	// Setup parameter block
	pPCFString->Type           = MQCFT_STRING;
	pPCFString->StrucLength    = MQCFST_STRUC_LENGTH_FIXED + 4;
	pPCFString->Parameter      = MQCA_TOPIC_NAME;
	pPCFString->CodedCharSetId = MQCCSI_DEFAULT;
	pPCFString->StringLength   = 1;
	memset( pPCFString->String, 0, 4 );
	pPCFString->String[0] = '*';

	// build an integer list to request only the subscription name and durable options in the reply messages
	pPCFIntList->Type = MQCFT_INTEGER_LIST;
	pPCFIntList->StrucLength	= MQCFIL_STRUC_LENGTH_FIXED + 4;
	pPCFIntList->Parameter		= MQIACF_TOPIC_ATTRS;
	pPCFIntList->Count			= 1;
//	pPCFIntList->Values[0]		= MQIACF_ALL;
	pPCFIntList->Values[0]		= MQCA_TOPIC_NAME;
//	pPCFIntList->Values[1]		= MQCACF_REG_TOPIC;
}

/////////////////////////////////////////////
//
// This routine builds the PCF request
// message to get a list of subscription
// names to be used in a drop down list
//
/////////////////////////////////////////////

void DataArea::buildPCFRequestSubs(char * pAdminMsg)

{
	MQCFH			*pPCFHeader;					// Ptr to PCF header parm block
	MQCFST			*pPCFString;					// Ptr to PCF string parm block
	MQCFIL			*pPCFIntList;					// Ptr to PCF integer list block

	// build the appropriate request message
	// initialize pointers
	pPCFHeader = (MQCFH *)pAdminMsg;

	pPCFString  = (MQCFST *)(pAdminMsg
						  + MQCFH_STRUC_LENGTH
						  );


	// just one character parameter in the message
	pPCFIntList = (MQCFIL *)( pAdminMsg
				  + MQCFH_STRUC_LENGTH
				  + MQCFST_STRUC_LENGTH_FIXED + 4
				  );

	// check if we are V6 or above
	if (level >= MQCMDL_LEVEL_600)
	{
		// we need to use version 3 for Z/OS
		// it only accepts version 3
		pPCFHeader->Type    = MQCFT_COMMAND_XR;
		pPCFHeader->Version = MQCFH_VERSION_3;
	}
	else
	{
		// use a lower level, since the queue manager
		// does not recognize version 3
		pPCFHeader->Type    = MQCFT_COMMAND;
		pPCFHeader->Version = MQCFH_VERSION_1;
	}

	// Setup the rest of the request header
	pPCFHeader->StrucLength    = MQCFH_STRUC_LENGTH;
	pPCFHeader->Command        = MQCMD_INQUIRE_SUBSCRIPTION;
	pPCFHeader->MsgSeqNumber   = 1;
	pPCFHeader->Control        = MQCFC_LAST;
	pPCFHeader->CompCode       = 0;
	pPCFHeader->Reason         = 0;
	pPCFHeader->ParameterCount = 2;

	// Setup parameter block
	pPCFString->Type           = MQCFT_STRING;
	pPCFString->StrucLength    = MQCFST_STRUC_LENGTH_FIXED + 4;
	pPCFString->Parameter      = MQCACF_SUB_NAME;
	pPCFString->CodedCharSetId = MQCCSI_DEFAULT;
	pPCFString->StringLength   = 1;
	memset( pPCFString->String, 0, 4 );
	pPCFString->String[0] = '*';

	// build an integer list to request only the subscription name, type and durabiliity in the reply messages
	pPCFIntList->Type = MQCFT_INTEGER_LIST;
	pPCFIntList->StrucLength	= MQCFIL_STRUC_LENGTH_FIXED + 20;
	pPCFIntList->Parameter		= MQIACF_SUB_ATTRS;
	pPCFIntList->Count			= 5;
	pPCFIntList->Values[0]		= MQCACF_SUB_NAME;
	pPCFIntList->Values[1]		= MQIACF_DESTINATION_CLASS;			// managed or provided
	pPCFIntList->Values[2]		= MQIACF_DURABLE_SUBSCRIPTION;		// durable or not durable
	pPCFIntList->Values[3]		= MQCACF_DESTINATION;				// queue name
	pPCFIntList->Values[4]		= MQCACF_DESTINATION_Q_MGR;			// remote queue manager name where queue is located
}

/////////////////////////////////////////////
//
// This routine is called to use PCF
// messages to get a list of subscription
// names to be used in a drop down list
//
/////////////////////////////////////////////

int DataArea::getListOfNames(NAMESTRUCT * namesPtr, const char * brokerQM, const char * replyQM, const char * replyQName, int nameType, MQLONG qType, BOOL inclCluster)

{
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=MQRC_NONE;					// MQ reason code
	MQLONG			ccsid;							// character set
	MQLONG			encoding;						// encoding of binary data
	MQLONG			AdminMsgLen;					// Length of user message buffer
	MQLONG			bytesRead;						// Number of bytes actually read
	int				Index;							// index used within PCF reply messages
	int				ret=0;							// return code
	MQLONG			*pPCFType;						// Type field of PCF message parm
	MQCFH			*pPCFHeader;					// Ptr to PCF header structure
	MQCFIN			*pPCFInteger;					// Ptr to PCF integer parm block
	char			*pAdminMsg;						// Ptr to outbound data buffer
	char			errtxt[256];					// error messages from subroutines
	char			traceInfo[512];					// work variable to build trace message
	bool			newCcsid=false;

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getListOfNames replyQM=%s replyQName=%s", replyQM, replyQName);

		// trace entry to getListOfNames
		logTraceEntry(traceInfo);
	}

	// initialize some counters that are only used when trace is enabled
	lcount = 0;
	mcount = 0;
	acount = 0;
	rcount = 0;
	ccount = 0;
	ocount = 0;

	////////////////////////////////////////////////////////////////////////
	//
	// Put a message to the SYSTEM.ADMIN.COMMAND.QUEUE to inquire all
	// the local queues defined on the queue manager.
	//
	// The request consists of a Request Header and a parameter block
	// used to specify the generic search. The header and the parameter
	// block follow each other in a contiguous buffer which is pointed
	// to by the variable pAdminMsg. This entire buffer is then put to
	// the queue.
	//
	// The command server, (use STRMQCSV to start it), processes the
	// SYSTEM.ADMIN.COMMAND.QUEUE and puts a reply on the application
	// ReplyToQ for each defined queue.
	//
	////////////////////////////////////////////////////////////////////////

	// Set the length for the message buffer for the request message
	switch (nameType)
	{
	case LOAD_NAMES_SUBS:
		{
			AdminMsgLen = MQCFH_STRUC_LENGTH
				  + MQCFST_STRUC_LENGTH_FIXED + 4
				  + MQCFIL_STRUC_LENGTH_FIXED + 20
				  ;
			break;
		}
	case LOAD_NAMES_TOPICS:
		{
			AdminMsgLen = MQCFH_STRUC_LENGTH
				  + MQCFST_STRUC_LENGTH_FIXED + 4
				  + MQCFIL_STRUC_LENGTH_FIXED + 4
				  ;
			break;
		}
	case LOAD_NAMES_QUEUES:
		{
			AdminMsgLen = MQCFH_STRUC_LENGTH
				  + MQCFST_STRUC_LENGTH_FIXED + 4
				  + MQCFIN_STRUC_LENGTH
				  + MQCFIL_STRUC_LENGTH_FIXED + 8
				  ;

			// check for cluster filter
			if (inclCluster)
			{
				AdminMsgLen += MQCFIN_STRUC_LENGTH;	// allow for cluster filter
			}

			break;
		}
	}


	///////////////////////////////////////////////////////////////////////
	//
	// Set pointers to message data buffers
	//
	// pAdminMsg points to the start of the message buffer
	//
	// pPCFHeader also points to the start of the message buffer. It is
	// used to indicate the type of command we wish to execute and the
	// number of parameter blocks following in the message buffer.
	//
	// pPCFString points into the message buffer immediately after the
	// header and is used to map the following bytes onto a PCF string
	// parameter block. In this case the string is used to indicate the
	// nameof the queue we want details about, * indicating all queues.
	//
	// pPCFInteger points into the message buffer immediately after the
	// string block described above. It is used to map the following
	// bytes onto a PCF integer parameter block. This block indicates
	// the type of queue we wish to receive details about, thereby
	// qualifying the generic search set up by passing the previous
	// string parameter.
	//
	// Note that this example is a generic search for all attributes of
	// all local queues known to the queue manager. By using different,
	// or more, parameter blocks in the request header it is possible
	// to narrow the search.
	//
	////////////////////////////////////////////////////////////////////////

	// allocate storage for the PCF request message
	pAdminMsg   = (char *)rfhMalloc( AdminMsgLen, "ADMINMSG" );

	switch (nameType)
	{
	case LOAD_NAMES_SUBS:
		{
			// build the request message
			buildPCFRequestSubs(pAdminMsg);
			break;
		}
	case LOAD_NAMES_TOPICS:
		{
			// build the request message
			buildPCFRequestTopics(pAdminMsg);
			break;
		}
	case LOAD_NAMES_QUEUES:
		{
			// build the request message
			buildPCFRequestQueues(pAdminMsg, qType, inclCluster);
			break;
		}
	}

	if (traceEnabled)
	{
		// dump the contents of the message to the trace
		dumpTraceData("Admin Msg", (unsigned char *)pAdminMsg, AdminMsgLen);
	}

	// request a list of the subscriptions
	// there will be one message returned per subscription
	cc = PutAdminMsg( MQFMT_ADMIN				// Format of message
					  , pAdminMsg				// Data part of message to put
					  ,  AdminMsgLen			// length of the data to put
					  ,  namesPtr->ccsid		// ccsid of the connected QM
					  ,  errtxt					// pointer to string area for error messages
					  ,  replyQName				// name of the reply to queue
					  ,  &rc					// MQ reason code
					  );

	// we are done with the request message - release the storage
	rfhFree( pAdminMsg );

	// check if the put worked
	if (cc  != MQCC_OK)
	{
		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "***** Put of admin message failed - cc=%d reason=%d AdminMsgLen=%d", cc, rc, AdminMsgLen);

			// trace entry to routine
			logTraceEntry(traceInfo);
		}

		// note that the error message was set by the put command
		return rc;
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Admin message sent - length=%d", AdminMsgLen);

		// trace entry to routine
		logTraceEntry(traceInfo);
	}

	// reset the current table entry if it exists

	////////////////////////////////////////////////////////////////////////
	//
	// Get and process the replies received from the command server onto
	// the applications ReplyToQ.
	//
	// There will be one message per defined local queue.
	//
	// The last message will have the Control field of the PCF header
	// set to MQCFC_LAST. All others will be MQCFC_NOT_LAST.
	//
	// An individual Reply message consists of a header followed by a
	// number a parameters, the exact number, type and order will depend
	// upon the type of request.
	//
	//--------------------------------------------------------------------
	//
	// The message is retrieved into a buffer pointed to by pAdminMsg.
	// This buffer as been allocated to be large enough to hold all the
	// parameters for a local queue definition.
	//
	// pPCFHeader is then allocated to point also to the beginning of
	// the buffer and is used to access the PCF header structure. The
	// header contains several fields. The one we are specifically
	// interested in is the ParameterCount. This tells us how many
	// parameters follow the header in the message buffer. There is
	// one parameter for each local queue attribute known by the
	// queue manager.
	//
	// At this point we do not know the order or type of each parameter
	// block in the buffer, the first MQLONG of each block defines its
	// type; they may be parameter blocks containing either strings or
	// integers.
	//
	// pPCFType is used initially to point to the first byte beyond the
	// known parameter block. Initially then, it points to the first byte
	// after the PCF header. Subsequently it is incremented by the length
	// of the identified parameter block and therefore points at the
	// next. Looking at the value of the data pointed to by pPCFType we
	// can decide how to process the next group of bytes, either as a
	// string, or an integer.
	//
	// In this way we parse the message buffer extracting the values of
	// each of the parameters we are interested in.
	//
	////////////////////////////////////////////////////////////////////////

	// AdminMsgLen is to be set to the length of the expected reply
	// message plus 4096. This structure is specific to Local Queues
	// but the 4096 covers any additional fields
	AdminMsgLen = MQCFH_STRUC_LENGTH
			  + (MQCFIN_STRUC_LENGTH * 3)
			  + MQ_Q_MGR_NAME_LENGTH
			  + MQ_Q_NAME_LENGTH + 32*1024;

	// Set pointers to message data buffers
	pAdminMsg = (char *)rfhMalloc( AdminMsgLen, "ADMINMS2" );

	// point to the header at the front of the message
	pPCFHeader = (MQCFH *)pAdminMsg;

	// start reading the reply messages
	do {
		cc = getReplyMsg(   MQGMO_WAIT				// Parameters on Get
						    , pAdminMsg				// pointer to message area
							, errtxt				// area for error messages
							, AdminMsgLen			// length of get buffer
							, &bytesRead			// number of bytes read
							, &rc					// MQSeries reason code
							, &ccsid				// character set id
							, &encoding				// MQMD encoding value
							);

		// did the MQGet work?
		if (MQCC_OK == cc)
		{
			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, "Reply message received, length=%d, maxLen=%d cc=%d type=%d ParameterCount=%d", bytesRead, AdminMsgLen, pPCFHeader->CompCode, pPCFHeader->Type, pPCFHeader->ParameterCount);

				// trace entry to routine
				logTraceEntry(traceInfo);

				if (verboseTrace)
				{
					// dump the contents of the message to the trace
					dumpTraceData(NULL, (unsigned char *)pAdminMsg, bytesRead);
				}
			}

			// check the completion code in the response
			if (MQCC_OK == pPCFHeader->CompCode)
			{
				// it worked
				// now check if this is an item we want or not
				// Z/OS returns extended items, among other things, and earlier versions of
				// MQ return responses
				if ((MQCFT_XR_ITEM == pPCFHeader->Type) || (MQCFT_RESPONSE == pPCFHeader->Type))
				{
					switch (nameType)
					{
					case LOAD_NAMES_SUBS:
						{
							// process the reply message
							processSubNamesReply(pAdminMsg, namesPtr, ccsid, encoding);
							break;
						}
					case LOAD_NAMES_TOPICS:
						{
							processTopicReply(pAdminMsg, namesPtr, ccsid, encoding);
							break;
						}
					case LOAD_NAMES_QUEUES:
						{
							processQueuesReply(pAdminMsg, namesPtr, replyQName, ccsid, encoding);
							break;
						}
					}
				}
			}
			else if (MQCC_FAILED == pPCFHeader->CompCode)
			{
				// Examine first error parameter
				pPCFType = (MQLONG *)(pAdminMsg + MQCFH_STRUC_LENGTH);

				if (traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, "Failure getting subscription names - cc=%d rc=%d\n", pPCFHeader->CompCode, pPCFHeader->Reason);

					// trace entry to routine
					logTraceEntry(traceInfo);

					// create the trace line
					sprintf(traceInfo, "Error indicated in reply message cc=%d rc=%d parm count %d\n", pPCFHeader->CompCode, pPCFHeader->Reason, pPCFHeader->ParameterCount);

					// trace entry to routine
					logTraceEntry(traceInfo);
				}

				// process any additional integer parameters
				Index = 0;
				pPCFInteger = (MQCFIN *)(pAdminMsg + MQCFH_STRUC_LENGTH);
				while (Index < pPCFHeader->ParameterCount)
				{
					convertIntHeader(pPCFInteger, encoding);
					if (MQCFT_INTEGER == pPCFInteger->Type)
					{
						switch (pPCFInteger->Parameter)
						{
						case MQIACF_PARAMETER_ID:
							{
								sprintf(errtxt, "Parameter with invalid value is %d", pPCFInteger->Value);
								break;
							}
						case MQIACF_ERROR_ID:
							{
								sprintf(errtxt, "Unexpect error reason %d", pPCFInteger->Value);
								break;
							}
						case MQIACF_SELECTOR:
							{
								sprintf(errtxt, "Selector error with parameter %d", pPCFInteger->Value);
								break;
							}
						case MQIA_CODED_CHAR_SET_ID:
							{
								// capture the correct ccsid to use
								if (namesPtr->ccsid != pPCFInteger->Value)
								{
									namesPtr->ccsid = pPCFInteger->Value;
									newCcsid = true;
								}
#ifdef _DEBUG
								TRACE("Invalid ccsid - please reissue with ccsid=%d\n", pPCFInteger->Value);
#endif
								break;
							}
						default:
							{
								sprintf(errtxt, " Additional parameter %d value %d\n", pPCFInteger->Parameter, pPCFInteger->Value);
								break;
							}
						}
					}

					// move on to the next parameter
					pPCFInteger = (MQCFIN *)((char *)pPCFInteger + pPCFInteger->StrucLength);
					Index++;
				}
			}
			else
			{
				sprintf(errtxt, "getQueuesByType: MQGet failed - cc=%d, rc=%d", cc, rc);
			}
		}
		else
		{
			// was the error no more messages?
			if (rc != 2033)
			{
				if (traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, "Error getting reply message, rc=%d", ret);

					// trace entry to routine
					logTraceEntry(traceInfo);
				}

				// some other error - return the error
				ret = rc;
			}
		}
	} while ((MQCC_OK == cc) && (MQCFC_NOT_LAST == pPCFHeader->Control));

	// release the admin msg storage
	rfhFree(pAdminMsg);

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getListOfNames result=%d", ret);

		// trace exit from getListOfNames
		logTraceEntry(traceInfo);
	}

	return rc;
}

////////////////////////////////////
//
// Routine to clear the names from
// a name table structure.  The
// QM name will be inserted into
// the new Names object.
//
////////////////////////////////////

void DataArea::resetNames(NAMESTRUCT *namePtr, const char *QMname)

{
	if ((namePtr != NULL) && (namePtr->names != NULL))
	{
		// release the names object for this queue manager
		delete(namePtr->names);
		namePtr->names = NULL;

		// allocate a new names object for this queue manager
		namePtr->names = new Names(INIT_NAME_TABLE_SIZE);

		// insert the queue manager name back in the table
		namePtr->qmName = namePtr->names->insertName(QMname);
	}
}

//////////////////////////////////////
//
// Routine to search a linked list
// of NAMESTRUCT structures looking
// for one for a given QM name.
//
// This routine will return a pointer
// to the correct structure or NULL
// if there is no match.
//
//////////////////////////////////////

NAMESTRUCT * DataArea::findNameInList(const char *QMname, NAMESTRUCT *namePtr)

{
	int				ofs;
	char *			ptr=NULL;
	bool			notFound=true;

	// search the linked list of structures until the correct one is found
	while ((namePtr != NULL) && notFound)
	{
		// get a pointer to the first entry in the Names object, which should be the QM name for this entry
		ofs = namePtr->names->getFirstEntry();

		// convert the offset to a pointer to the actual name
		ptr = namePtr->names->getNameAddr(ofs);

		// check if this is the right one
		if (strcmp(ptr, QMname) != 0)
		{
			// not the one - move on to the next entry
			namePtr = (NAMESTRUCT *)namePtr->nextQM;
		}
		else
		{
			// found the right one - stop the loop
			notFound = FALSE;
		}
	}

	// return the results - either a pointer to the matching entry or NULL if not found
	return namePtr;
}

/////////////////////////////////////////////
//
// This routine is called to find the
// correct name structure for the specified
// queue manager.
//
/////////////////////////////////////////////

const char * DataArea::getNamesListPtr(const char *QMname, NAMESTRUCT * namePtr)

{
	int				ofs;
	char *			ptr=NULL;
	BOOL			notFound=TRUE;

	while ((namePtr != NULL) && notFound)
	{
		ofs = namePtr->names->getFirstEntry();
		ptr = namePtr->names->getNameAddr(ofs);
		if (strcmp(ptr, QMname) != 0)
		{
			namePtr = (NAMESTRUCT *)namePtr->nextQM;
		}
		else
		{
			notFound = FALSE;
		}
	}

	if (namePtr != NULL)
	{
		// assume that the queue manager is the first entry in the names table
		// skip the queue manager name and terminating null
		// get the second entry in the table
		ofs = namePtr->names->getNextEntry(ofs);
		ptr = namePtr->names->getNameAddr(ofs);
	}
	else
	{
		ptr = NULL;
	}

	return ptr;
}

NAMESTRUCT * DataArea::getSubNamesPtr(const char * QMname, BOOL reset)

{
	NAMESTRUCT		*namePtr=NULL;

	// see if there is already a table
	namePtr = findNameInList(QMname, subNamesRoot);

	// not found - try to create one and link it into the list
	if (NULL == namePtr)
	{
		// this is the first one so create a new one
		namePtr = allocateNameStruct(QMname, "SubN");

		// insert into the list
		namePtr->nextQM = subNamesRoot;
		subNamesRoot = namePtr;
	}
	else if (reset)
	{
		// clear the existing names
		resetNames(namePtr, QMname);
	}

	return namePtr;
}

const char * DataArea::getSubNamesListPtr(const char *QMname)

{
	const char *	ptr=NULL;

	// find the right table based on the QM name
	ptr = getNamesListPtr(QMname, subNamesRoot);

	return ptr;
}

NAMESTRUCT * DataArea::getTopicNamesPtr(const char * QMname, BOOL reset)

{
	NAMESTRUCT		*namePtr=NULL;

	// see if there is already a table
	namePtr = findNameInList(QMname, topicNamesRoot);

	// not found - try to create one and link it into the list
	if (NULL == namePtr)
	{
		// this is the first one so create a new one
		namePtr = allocateNameStruct(QMname, "TopN");

		// insert into the list
		namePtr->nextQM = topicNamesRoot;
		topicNamesRoot = namePtr;
	}
	else if (reset)
	{
		// clear the existing names
		resetNames(namePtr, QMname);
	}

	return namePtr;
}

const char * DataArea::getTopicNamesListPtr(const char *QMname)

{
	const char *		ptr=NULL;

	// find the right table based on the QM name
	ptr = getNamesListPtr(QMname, topicNamesRoot);

	return ptr;
}

NAMESTRUCT * DataArea::getQueueNamesPtr(const char *QMname, BOOL reset)
{
	NAMESTRUCT		*namePtr=NULL;

	// see if there is already a table
	namePtr = findNameInList(QMname, queueNamesRoot);

	// not found - try to create one and link it into the list
	if (NULL == namePtr)
	{
		// this is the first one so create a new one
		namePtr = allocateNameStruct(QMname, "Que ");

		// insert into the list
		namePtr->nextQM = queueNamesRoot;
		queueNamesRoot = namePtr;
	}
	else if (reset)
	{
		// clear the existing names
		resetNames(namePtr, QMname);
	}

	return namePtr;
}

const char * DataArea::getQueueNamesListPtr(const char *QMname)

{
	const char *		ptr=NULL;

	// find the right table based on the QM name
	ptr = getNamesListPtr(QMname, queueNamesRoot);

	return ptr;
}

NAMESTRUCT * DataArea::allocateNameStruct(const char *QMname, const char *nameType)

{
	NAMESTRUCT		*namesPtr=NULL;
	char			mallocStr[16];
	char			err[32];				// error message
	char			traceInfo[256];			// work variable to build trace message

	// initialize the error message and malloc eye catcher
	memset(err, 0, sizeof(err));
	memset(mallocStr, 0, sizeof(mallocStr));

	// allocate the memory structure
	memcpy(mallocStr, "NSTR", 4);
	strncpy(mallocStr + 4, nameType, 4);
	namesPtr = (NAMESTRUCT *)rfhMalloc(sizeof(NAMESTRUCT), mallocStr);

	// check if it worked
	if (NULL == namesPtr)
	{
		// failed - trace the result
		strcpy(err, "malloc fail for NAMESTRUCT");
	}
	else
	{
		// initialize the fields
		memcpy(namesPtr->id, nameType, sizeof(namesPtr->id));
		namesPtr->nextQM = NULL;
		namesPtr->count = 0;
		namesPtr->ccsid = MQCcsid;

		// allocate a Names object
		namesPtr->names = new Names(INIT_NAME_TABLE_SIZE);

		// check if it worked
		if (NULL == namesPtr->names)
		{
			// set an error message
			strcpy(err, "new fail for Names object");
		}
		else
		{
			// insert the queue manager name
			namesPtr->qmName = namesPtr->names->insertName(QMname);
		}
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::allocateNameStruct() QMname=%s nameType=%s namesPtr=%8.8X err=%s", QMname, QMname, (unsigned int)namesPtr, err);

		// trace exit from allocateNameStruct
		logTraceEntry(traceInfo);
	}

	// return the new control block
	return namesPtr;
}

//////////////////////////////////////////////////////////////////////////////
//
// The queue manager returns strings of the maximum length for each
// specific parameter, padded with blanks.
//
// We are interested in only the nonblank characters so will extract them
// from the message buffer, and terminate the string with a null, \0.
//
//////////////////////////////////////////////////////////////////////////////

void DataArea::MQParmCpy(char *target, char *source, int length)

{
	int   counter=0;

	while ( counter < length && source[counter] != ' ' )
	{
		target[counter] = source[counter];
		counter++;
	} // endwhile

	if ( counter < length)
	{
		target[counter] = '\0';
	} // endif
}

void DataArea::convertPCFHeader(MQCFH *pPCFHeader, MQLONG encoding)

{
	if (getIntEncode(encoding) != NUMERIC_PC)
	{
		pPCFHeader->Type = reverseBytes4(pPCFHeader->Type);
		pPCFHeader->StrucLength = reverseBytes4(pPCFHeader->StrucLength);
		pPCFHeader->Version = reverseBytes4(pPCFHeader->Version);
		pPCFHeader->Command = reverseBytes4(pPCFHeader->Command);
		pPCFHeader->MsgSeqNumber = reverseBytes4(pPCFHeader->MsgSeqNumber);
		pPCFHeader->Control = reverseBytes4(pPCFHeader->Control);
		pPCFHeader->CompCode = reverseBytes4(pPCFHeader->CompCode);
		pPCFHeader->Reason = reverseBytes4(pPCFHeader->Reason);
		pPCFHeader->ParameterCount = reverseBytes4(pPCFHeader->ParameterCount);
	}
}

void DataArea::convertIntHeader(MQCFIN *pPCFInteger, MQLONG encoding)

{
	if (getIntEncode(encoding) != NUMERIC_PC)
	{
		pPCFInteger->StrucLength	= reverseBytes4(pPCFInteger->StrucLength);
		pPCFInteger->Parameter		= reverseBytes4(pPCFInteger->Parameter);
		pPCFInteger->Value			= reverseBytes4(pPCFInteger->Value);
	}
}

void DataArea::convertIntListHeader(MQCFIL *pPCFIntList, MQLONG encoding)

{
	int		i;
	int		*iptr;

	if (getIntEncode(encoding) != NUMERIC_PC)
	{
		pPCFIntList->StrucLength	= reverseBytes4(pPCFIntList->StrucLength);
		pPCFIntList->Parameter		= reverseBytes4(pPCFIntList->Parameter);
		pPCFIntList->Count			= reverseBytes4(pPCFIntList->Count);

		i = 0;
		iptr = (int *)&pPCFIntList->Values;
		while (i < pPCFIntList->Count)
		{
			(*iptr) = reverseBytes4((*iptr));
			iptr++;
			i++;
		}
	}
}

void DataArea::convertStringHeader(MQCFST *pPCFString, MQLONG ccsid, MQLONG encoding)

{
	if (getIntEncode(encoding) != NUMERIC_PC)
	{
		pPCFString->StrucLength		= reverseBytes4(pPCFString->StrucLength);
		pPCFString->Parameter		= reverseBytes4(pPCFString->Parameter);
		pPCFString->CodedCharSetId	= reverseBytes4(pPCFString->CodedCharSetId);
		pPCFString->StringLength	= reverseBytes4(pPCFString->StringLength);
	}

	// check the ccsid to see if we have to translate from EBCDIC to ASCII
	if (getCcsidType(ccsid) == CHAR_EBCDIC)
	{
		// translate the string value
		EbcdicToAscii((unsigned char *)&pPCFString->String, pPCFString->StringLength, (unsigned char *)pPCFString->String);
	}
}

void DataArea::convertByteStringHeader(MQCFBS *pPCFByteString, int ccsid, int encoding)

{
	if (getIntEncode(encoding) != NUMERIC_PC)
	{
		pPCFByteString->StrucLength		= reverseBytes4(pPCFByteString->StrucLength);
		pPCFByteString->Parameter		= reverseBytes4(pPCFByteString->Parameter);
		pPCFByteString->StringLength	= reverseBytes4(pPCFByteString->StringLength);
	}
}

void DataArea::convertStrListHeader(MQCFSL *pPCFStrList, MQLONG ccsid, MQLONG encoding)

{
	if (getIntEncode(encoding) != NUMERIC_PC)
	{
		pPCFStrList->StrucLength	= reverseBytes4(pPCFStrList->StrucLength);
		pPCFStrList->Parameter		= reverseBytes4(pPCFStrList->Parameter);
		pPCFStrList->CodedCharSetId	= reverseBytes4(pPCFStrList->CodedCharSetId);
		pPCFStrList->Count			= reverseBytes4(pPCFStrList->Count);
		pPCFStrList->StringLength	= reverseBytes4(pPCFStrList->StringLength);
	}

	// check the ccsid to see if we have to translate from EBCDIC to ASCII
	if (getCcsidType(ccsid) == CHAR_EBCDIC)
	{
		// translate the string value
		EbcdicToAscii((unsigned char *)&pPCFStrList->Strings, pPCFStrList->StringLength * pPCFStrList->Count, (unsigned char *)pPCFStrList->Strings);
	}
}

///////////////////////////////////////////////
//
// Routine to process a request publication,
// which will return the current retained
// publication associated with a topic.
//
///////////////////////////////////////////////

void DataArea::ReqPub(const char *QMname, const char *brokerQM, const char *topic, const char *subName)

{
	MQLONG			cc=MQCC_OK;						// MQ completion code
	MQLONG			rc=0;							// MQ reason code
	MQLONG			options=0;						// subscription options
	CRfhutilApp *	app;							// pointer to the MFC application object
	MQMDPAGE		*mqmdObj=(MQMDPAGE *)mqmdData;	// pointer to MQMD object
    MQMD2			mqmd={MQMD2_DEFAULT};			// Message descriptor
	MQSRO			sro={MQSRO_DEFAULT};			// request options
	char			traceInfo[1024];				// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::ReqPub() QM=%s broker=%s subName=%s Topic=%.512s", QMname, brokerQM, subName, topic);

		// trace entry to ReqPub
		logTraceEntry(traceInfo);
	}

	// make sure we have a subscription name
	if ((subName != NULL) && (strlen(subName) == 0))
	{
		m_error_msg = "*Subscription name required* ";
		return;
	}

	// make sure we are connected to the correct QM
	if (!checkConnection(QMname))
	{
		return;
	}

	// make sure we have a subscription request
	if (MQHO_NONE == hSub)
	{
		m_error_msg = "No subscription active";

		return;
	}

	// set the MQMD fields in the message object
	mqmdObj->setMessageMQMD(&mqmd, m_setUserID, m_set_all);

   ////////////////////////////////////////////////////////////////////
   //
   //   Try to get the retained publication
   //
   ////////////////////////////////////////////////////////////////////

	XMQSubRq(qm,							// connection handle
			 hSub,						// object handle (output) for reading messages
			 MQSR_ACTION_PUBLICATION,
			 &sro,						// request options
			 &cc,						// completion code
			 &rc);						// reason code

	// check the results
	if (cc != MQCC_OK)
	{
		// check for no retained publications found
		if (MQRC_NO_RETAINED_MSG == rc)
		{
			// tell the user what happened
			m_error_msg = "No retained publications found";
		}
		else
		{
			// put failed, get the reason and generate an error message
			// get the completion code and reason code
			setErrorMsg(cc, rc, "MQSubRQ");
		}
	}
	else
	{
		m_error_msg.Format("%d Publications received from %.256s", sro.NumPubs, subName);
	}

	// call the audit routine to record the put message action
	app = (CRfhutilApp *)AfxGetApp();
	app->createAuditRecord(QMname, subName, "MQSUBRQ", rc);

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::ReqPub() cc=%d rc=%d", cc, rc);

		// trace exit from ReqPub
		logTraceEntry(traceInfo);
	}
}

BOOL DataArea::isSubscriptionActive()

{
	// return TRUE if subscription handle is not NULL
	return (hSub != MQHO_NONE);
}

BOOL DataArea::checkForMQ()

{
	// return true if the DLL entry points were found and false if not
	return XMQConnX != NULL;
}

void DataArea::loadMQreportError(const char * mqmPath, const char * functionName)

{
	char	traceInfo[640];				// work variable to build trace message

	if (traceEnabled)
	{
		// get the last error code
		DWORD lastError = GetLastError();

		// create the trace line
		sprintf(traceInfo, " DataArea::loadMQdll() %s failed error=%d mqmPath=%s", functionName, lastError, mqmPath);

		// trace exit from loadMQdll
		logTraceEntry(traceInfo);
	}
}

int DataArea::loadMQdll()

{
	int		rc=0;
	int		pathLength=0;
	int		MQ71PathLength=0;
	BOOL	foundMQ71=FALSE;
	const char *	libName;
	char	mqmPath[512];
	char	*newPath = NULL;			
	int     newPathLen = 4096 * 16;      // twice the maximum size of the environment on Windows 2008
	char	traceInfo[640];				// work variable to build trace message

	// initialize the mqmPath variable
	memset(mqmPath, 0, sizeof(mqmPath));
	
	newPath = (char *)rfhMalloc(newPathLen, "NEWPATH1");
	if (!newPath) {
		return -1;
	}
	memset(newPath, 0, newPathLen);

#ifdef MQCLIENT
	libName = "mqic32.dll";
#else
	libName = "mqm.dll";
#endif

	// set the MQ environment variable
	if (MQ71Path[0] != 0)
	{
		// found MQ V7.1 or greater
		// the following code is the equivalent of setmqenv
		BOOL setResult = SetEnvironmentVariable("MQ_FILE_PATH", MQ71Path);
		setResult = SetEnvironmentVariable("MQ_ENV_MODE", "32");
		setResult = SetEnvironmentVariable("MQ_INSTALLATION_NAME", highestLevelInstallationName);
		setResult = SetEnvironmentVariable("MQ_INSTALLATION_PATH", MQ71Path);

		// modify the path - this must be done if MQ 7.1 is not the primary installation
		strcpy(newPath, MQ71Path);
		strcat(newPath, "\\bin\\;");
		MQ71PathLength = strlen(newPath);
		pathLength = GetEnvironmentVariable("PATH", newPath + MQ71PathLength, sizeof(newPath) - MQ71PathLength);
		setResult = SetEnvironmentVariable("PATH", newPath);

		// point to the mqm.dll location
		// the mqm.dll library will always be used for V7.1 and above
		strcpy(mqmPath, MQ71Path);
		strcat(mqmPath, "\\bin\\mqm.dll");

		// try to load the dll from the system path
		// this should work if MQ is installed since
		// the installer sets this up.
		mqmdll = LoadLibrary(mqmPath);
	}
	else
	{
		// no MQ 7.1 installed on this system
		// append the proper library name
		strcat(mqmPath, libName);

		// try to load the dll from the system path
		// this should work if MQ is installed since
		// the installer sets this up.
		mqmdll = LoadLibrary(mqmPath);

		// was it successful?
		if (NULL == mqmdll)
		{
			// report the error in the trace
			loadMQreportError(mqmPath, "LoadLibrary");

			// find the MQ install directory
			findMQInstallDirectory((unsigned char *)&mqmPath, sizeof(mqmPath) - 1);

			// append the DLL name
			strcat(mqmPath, "\\bin\\");
			strcat(mqmPath, libName);

			// load the DLL using the fully qualified name
			mqmdll = LoadLibrary(mqmPath);
		}
	}

	// was the DLL found?
	if (mqmdll != NULL)
	{
		// get the entry point for MQCONNX function
		XMQConnX = (XMQCONNX)GetProcAddress(mqmdll, "MQCONNX");
		XMQDisc = (XMQDISC)GetProcAddress(mqmdll, "MQDISC");
		XMQBegin = (XMQBEGIN)GetProcAddress(mqmdll, "MQBEGIN");
		XMQCmit = (XMQCMIT)GetProcAddress(mqmdll, "MQCMIT");
		XMQBack = (XMQBACK)GetProcAddress(mqmdll, "MQBACK");
		XMQOpen = (XMQOPEN)GetProcAddress(mqmdll, "MQOPEN");
		XMQClose = (XMQCLOSE)GetProcAddress(mqmdll, "MQCLOSE");
		XMQGet = (XMQGET)GetProcAddress(mqmdll, "MQGET");
		XMQPut = (XMQPUT)GetProcAddress(mqmdll, "MQPUT");
		XMQInq = (XMQINQ)GetProcAddress(mqmdll, "MQINQ");
		XMQSub = (XMQSUB)GetProcAddress(mqmdll, "MQSUB");
		XMQSubRq = (XMQSUBRQ)GetProcAddress(mqmdll, "MQSUBRQ");
		XMQCrtMh = (XMQCRTMH)GetProcAddress(mqmdll, "MQCRTMH");
		XMQDltMh = (XMQDLTMH)GetProcAddress(mqmdll, "MQDLTMH");
		XMQInqMp = (XMQINQMP)GetProcAddress(mqmdll, "MQINQMP");
		XMQSetMp = (XMQSETMP)GetProcAddress(mqmdll, "MQSETMP");
	}
	else
	{
		// report the error in the trace
		loadMQreportError(mqmPath, "Second LoadLibrary");
	}

	// check if MQM.dll was found
	if (NULL == XMQConnX)
	{
		// disable MQ functions
		foundMQ = FALSE;
	}

	// check if the entry points for pub/sub were found
	if ((XMQSub != NULL) && (XMQSubRq != NULL))
	{
		pubsubSupported = TRUE;
	}

	// check if the entry points for message properties were found
	if ((XMQCrtMh != NULL) && (XMQDltMh != NULL) && (XMQInqMp != NULL) && (XMQSetMp != NULL))
	{
		propertiesSupported = TRUE;
	}

	if (newPath) {
		rfhFree(newPath);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::loadMQdll() pubsubSupported=%d propertiesSupported=%d foundMQ=%d mqmdll=%8.8X mqmPath=%s", pubsubSupported, propertiesSupported, foundMQ, (unsigned int)mqmdll, mqmPath);

		// trace exit from loadMQdll
		logTraceEntry(traceInfo);

		// create the trace line
		sprintf(traceInfo, " XMQConnX=%8.8X XMQDisc=%8.8X XMQOpen=%8.8X XMQClose=%8.8X XMQGet=%8.8X XMQPut=%8.8X", (unsigned int)XMQConnX, (unsigned int)XMQDisc, (unsigned int)XMQOpen, (unsigned int)XMQClose, (unsigned int)XMQGet, (unsigned int)XMQPut);

		// trace entry points
		logTraceEntry(traceInfo);

		// create the trace line
		sprintf(traceInfo, " XMQSub=%8.8X XMQSubRq=%8.8X XMQCrtMh=%8.8X XMQDltMh=%8.8X XMQInqMp=%8.8X XMQSetMp=%8.8X", (unsigned int)XMQSub, (unsigned int)XMQSubRq, (unsigned int)XMQCrtMh, (unsigned int)XMQDltMh, (unsigned int)XMQInqMp, (unsigned int)XMQSetMp);

		// trace entry points
		logTraceEntry(traceInfo);
	}

	return rc;
}

void DataArea::getDefaultQM()

{
#ifndef MQCLIENT
	MQHCONN			hConn=0;				// handle to connected queue mgr
	MQOD			od = {MQOD_DEFAULT};	// Object Descriptor
	MQHOBJ			hObj;					// object handle
	MQLONG			Selector;				// selector for inquiry
	MQLONG			cc;						// completion code
	MQLONG			rc;						// reason code
	char			QMName[MQ_Q_MGR_NAME_LENGTH + 4];
	char			traceInfo[256];			// work variable to build trace message

	// try to connect to the queue manager
	if (connect2QM(""))
	{
		// set the object type we are looking for
		od.ObjectType = MQOT_Q_MGR;			// open the queue manager object
		XMQOpen(qm,							// connection handle
				&od,						// object descriptor for queue
				MQOO_INQUIRE +				// open it for inquire
				MQOO_FAIL_IF_QUIESCING,		// but not if MQM stopping
				&hObj,						// object handle
				&cc,						// MQOPEN completion code
				&rc);						// reason code

		// did the open work?
		if (MQCC_OK == cc)
		{
			// set the selector for character parameters
			Selector = MQCA_Q_MGR_NAME;
			memset(QMName, 0, sizeof(QMName));

			// do an inquiry for the Queue Manager name
			XMQInq(hConn,					// connection handle
				   hObj,						// object handle for q manager
				   1,						// inquire only one selector
				   &Selector,				// the selector to inquire
				   0,						// no integer attributes needed
				   NULL,						// so no buffer supplied
				   MQ_Q_MGR_NAME_LENGTH,		// inquiring a q manager name
				   QMName,					// the buffer for the name
				   &cc,						// completion code
				   &rc);						// reason code

			// did the inquiry work?
			if (rc == MQRC_NONE)
			{
				// remember the default queue manager name
				defaultQMname = QMName;
			}

			// close the queue handle
			XMQClose(qm, &hObj, MQCO_NONE, &cc, &rc);
		}

		// disconnect from the default queue manager
		discQM();
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getDefaultQM() defaultQMname=%s", (LPCTSTR)defaultQMname);

		// trace exit from getDefaultQM
		logTraceEntry(traceInfo);
	}
#endif
}

void DataArea::getUserPropertyData(CString &data)

{
	// get the user property data from the user properties tab
	((CProps *)propData)->getPropertyData(data);
}

void DataArea::getRfhUsrData(CString &data)

{
	((Usr *)usrData)->getUsrData(data);
}

void DataArea::clearUserProperties()

{
	((CProps *)propData)->clearUserProperties();
}

void DataArea::clearRfhUsrData()

{
	// clear the data in the usr folder
	((Usr *)usrData)->clearUsrData();

	// deselect the usr folder as well
	((RFH *)rfhData)->selectUsrFolder(FALSE);
}

const char * DataArea::getFirstQMname()

{
	char *	ptr=NULL;

	if (queueNamesRoot != NULL)
	{
		ptr = queueNamesRoot->names->getNameAddr(queueNamesRoot->qmName);
	}

	return ptr;
}

const char * DataArea::getNextQMname(const char *name)

{
	NAMESTRUCT *	currQMptr;
	char *			namePtr;

	currQMptr = queueNamesRoot;
	namePtr = currQMptr->names->getNameAddr(currQMptr->qmName);
	while ((currQMptr != NULL) && (strcmp(name, namePtr) != 0))
	{
		// move on to the next entry
		currQMptr = (NAMESTRUCT *)currQMptr->nextQM;

		// is there a next entry?
		if (currQMptr != NULL)
		{
			// get a pointer to the name of the queue manager
			namePtr = currQMptr->names->getNameAddr(currQMptr->qmName);
		}
	}

	// see if we found the name
	if (currQMptr != NULL)
	{
		currQMptr = (NAMESTRUCT *)currQMptr->nextQM;

		// is there a next entry?
		if (currQMptr != NULL)
		{
			// get a pointer to the name of the queue manager
			namePtr = currQMptr->names->getNameAddr(currQMptr->qmName);
		}
		else
		{
			// the name was the last item - no more entries, so return NULL
			namePtr = NULL;
		}
	}
	else
	{
		namePtr = NULL;
	}

	return namePtr;
}

void DataArea::CreateInstallEntry(const char * name, const char * filepath, const char * VRMF, int version, int release, const char * installation, const char * package)

{
	InstTable	*newInstall;
#ifdef MQCLIENT
	int			instType=MQ71_CLIENT_INST;
#else
	int			instType=MQ71_SERVER_INST;
#endif
	char		traceInfo[1024];		// work variable to build trace message

	// allocate storage for a new installation
	newInstall = (InstTable *)rfhMalloc(sizeof(InstTable), "InstTabl");

	// check if the malloc worked
	if (newInstall != NULL)
	{
		// clear the memory
		memset(newInstall, 0, sizeof(InstTable));

		// set the values in the new table entry
		strcpy(newInstall->name, name);
		strcpy(newInstall->filePath, filepath);
		strcpy(newInstall->VRMF, VRMF);
		newInstall->release = release;
		newInstall->version = version;
		strcpy(newInstall->installation, installation);

		// check what kind of installation this is
		if ('S' == package[0])
		{
			// server installation
			newInstall->type = MQ71_SERVER_INST;
		}
		else
		{
			// client installation
			newInstall->type = MQ71_CLIENT_INST;
		}

		// link the new entry
		newInstall->next = firstInstallation;
		firstInstallation = newInstall;

		// check if this is the right kind of install
		if (instType == newInstall->type)
		{
			// check if this is the first or a higher version or release
			if ((highestMQVersion < version) || ((highestMQVersion == version) && (highestMQRelease < release)) || (strcmp(VRMF, higestVRMF) > 0))
			{
				highestMQVersion = version;
				highestMQRelease = release;
				strcpy(higestVRMF, VRMF);
				strcpy(MQ71Path, filepath);
				strcpy(highestLevelInstallationName, name);
			}
		}
	}

	if (traceEnabled && verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::CreateInstallEntry() version=%d release=%d package=%s name=%s VRMF=%s filepath=%s", version, release, package, name, VRMF, filepath);

		// trace exit from CreateInstallEntry
		logTraceEntry(traceInfo);
	}
}

void DataArea::processMQcomponents()

{
	int				ret1;
	int				ret2=0;
	int				rc1=0;
	int				rc2=0;
	int				rc3=0;
	int				rc4=0;
	int				rc5=0;
	int				rc6=0;
	int				rc7=0;
	int				rc8=0;
	int				rc9=0;
	int				rc10=0;
	HKEY			regkey;
	unsigned long	valueLen=0;
	DWORD			valueType=0;
	const char *	regKeyName;
	char			server[16];
	char			client[16];
	char			amqp[16];
	char			ams[16];
	char			devtools[16];
	char			explorer[16];
	char			javamsg[16];
	char			mftagent[16];
	char			mftlogger[16];
	char			mftservice[16];
	char			web[16];
	char			xrservice[16];
	char			traceInfo[1024];		// work variable to build trace message

	// initialize the memory for the key values
	memset(server, 0, sizeof(server));
	memset(client, 0, sizeof(client));
	memset(amqp, 0, sizeof(amqp));
	memset(ams, 0, sizeof(ams));
	memset(devtools, 0, sizeof(devtools));
	memset(explorer, 0, sizeof(explorer));
	memset(javamsg, 0, sizeof(javamsg));
	memset(mftagent, 0, sizeof(mftagent));
	memset(mftlogger, 0, sizeof(mftlogger));
	memset(mftservice, 0, sizeof(mftservice));
	memset(web, 0, sizeof(web));
	memset(xrservice, 0, sizeof(xrservice));

	if (is64bit)
	{
		// 64-bit OS
		regKeyName = MQCOMPKEY64;
	}
	else
	{
		// 32-bit OS
		regKeyName = MQCOMPKEY;
	}

	// create key for installed components
	ret1 = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						regKeyName,
						NULL,
						KEY_QUERY_VALUE,
						&regkey);

	if (ERROR_SUCCESS == ret1)
	{
		valueLen = sizeof(server) - 1;
		rc1 = RegQueryValueEx(regkey,
							  "Server",
							  NULL,
							  &valueType,
							  (unsigned char *)&server,
							  &valueLen);

		valueLen = sizeof(client) - 1;
		rc2 = RegQueryValueEx(regkey,
							  "Local Clients\\Windows NT Client",
							  NULL,
							  &valueType,
							  (unsigned char *)client,
							  &valueLen);

		valueLen = sizeof(amqp) - 1;
		rc3 = RegQueryValueEx(regkey,
			"AMQP",
			NULL,
			&valueType,
			(unsigned char *)amqp,
			&valueLen);

		valueLen = sizeof(ams) - 1;
		rc4 = RegQueryValueEx(regkey,
			"AMS",
			NULL,
			&valueType,
			(unsigned char *)ams,
			&valueLen);

		valueLen = sizeof(devtools) - 1;
		rc5 = RegQueryValueEx(regkey,
			"Development Toolkit",
			NULL,
			&valueType,
			(unsigned char *)devtools,
			&valueLen);

		valueLen = sizeof(web) - 1;
		rc6 = RegQueryValueEx(regkey,
			"Web",
			NULL,
			&valueType,
			(unsigned char *)web,
			&valueLen);

		valueLen = sizeof(xrservice) - 1;
		rc7 = RegQueryValueEx(regkey,
			"XR_Service",
			NULL,
			&valueType,
			(unsigned char *)xrservice,
			&valueLen);

		valueLen = sizeof(mftagent) - 1;
		rc8 = RegQueryValueEx(regkey,
			"MFT_AGENT",
			NULL,
			&valueType,
			(unsigned char *)mftagent,
			&valueLen);

		valueLen = sizeof(mftservice) - 1;
		rc9 = RegQueryValueEx(regkey,
			"MFT_Service",
			NULL,
			&valueType,
			(unsigned char *)mftservice,
			&valueLen);

		valueLen = sizeof(mftlogger) - 1;
		rc10 = RegQueryValueEx(regkey,
			"MFT_Logger",
			NULL,
			&valueType,
			(unsigned char *)mftlogger,
			&valueLen);

		// close the key
		ret2 = RegCloseKey(regkey);
	}

	if (traceEnabled && verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, " Installed components ret1=%d ret2=%d rc1=%d rc2=%d Server=%s Client=%s", ret1, ret2, rc1, rc2, server, client);

		// trace installed components
		logTraceEntry(traceInfo);

		// create the trace line
		sprintf(traceInfo, " Installed components rc3=%d rc4=%d rc5=%d rc6=%d rc7=%d AMQP=%s AMS=%s Dev Tools=%s Web=%s XR_Service=%s", rc3, rc4, rc5, rc6, rc7, amqp, ams, devtools, web, xrservice);

		// trace installed components
		logTraceEntry(traceInfo);

		// create the trace line
		sprintf(traceInfo, " Installed components rc8=%d rc9=%d rc10=%d MFT Agent=%s MFT Service=%s MFT Logger=%s", rc8, rc9, rc10, mftagent, mftservice, mftlogger);

		// trace installed components
		logTraceEntry(traceInfo);
	}
}

void DataArea::processMQinstallation(HKEY regkey)

{
	int				rc1;
	int				rc2;
	int				rc3;
	int				rc4;
	int				rc5;
	int				rc6;
	int				rc7;
	int				version=0;
	int				release=0;
	unsigned long	valueLen=0;
	DWORD			valueType=0;
	char			filePath[512];
	char			name[MQ_INSTALLATION_NAME_LENGTH + 8];
	char			package[16];
	char			VRMF[16];
	char			instance[16];
	char			traceInfo[1024];		// work variable to build trace message

	memset(filePath, 0, sizeof(filePath));
	memset(name, 0, sizeof(name));
	memset(package, 0, sizeof(package));
	memset(VRMF, 0, sizeof(VRMF));
	memset(instance, 0, sizeof(instance));

	if (traceEnabled && verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::processMQinstallation() regkey=%8.8X", (unsigned int)regkey);

		// trace entry to processMQinstallation
		logTraceEntry(traceInfo);
	}

	valueLen = sizeof(filePath) - 1;
	rc1 = RegQueryValueEx(regkey,
						  "FilePath",
						  NULL,
						  &valueType,
						  (unsigned char *)filePath,
						  &valueLen);

	valueLen = sizeof(name) - 1;
	rc2 = RegQueryValueEx(regkey,
						  "Name",
						  NULL,
						  &valueType,
						  (unsigned char *)name,
						  &valueLen);

	valueLen = sizeof(VRMF) - 1;
	rc3 = RegQueryValueEx(regkey,
						  "VRMF",
						  NULL,
						  &valueType,
						  (unsigned char *)VRMF,
						  &valueLen);

	valueLen = sizeof(version);
	rc4 = RegQueryValueEx(regkey,
						  "MQServerVersion",
						  NULL,
						  &valueType,
						  (unsigned char *)&version,
						  &valueLen);

	valueLen = sizeof(release);
	rc5 = RegQueryValueEx(regkey,
						  "MQServerRelease",
						  NULL,
						  &valueType,
						  (unsigned char *)&release,
						  &valueLen);

	valueLen = sizeof(instance) - 1;
	rc6 = RegQueryValueEx(regkey,
						  "MSIInstance",
						  NULL,
						  &valueType,
						  (unsigned char *)instance,
						  &valueLen);

	valueLen = sizeof(package) - 1;
	rc7 = RegQueryValueEx(regkey,
						  "Package",
						  NULL,
						  &valueType,
						  (unsigned char *)package,
						  &valueLen);

	// create an entry in the MQ installation table
	CreateInstallEntry(name, filePath, VRMF, version, release, instance, package);

	if (traceEnabled && verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::processMQinstallation() rc1=%d rc2=%d rc3=%d rc4=%d rc5=%d rc6=%d rc7=%d filePath=%s name=%s VRMF=%s version=%d release=%d instance=%s package=%s", rc1, rc2, rc3, rc4, rc5, rc6, rc7, filePath, name, VRMF, version, release, instance, package);

		// trace exit from processMQinstallation
		logTraceEntry(traceInfo);
	}
}

void DataArea::SearchForMQ71()

{
	int				ret=-1;
	int				ret2=-1;
	int				ret3 = -1;
	int				ret4 = -1;
	int				keyLen=0;
	const char *	regKeyName;
	HKEY			regkey;
	HKEY			regkey2;
	DWORD			valueType=0;
	DWORD			index=0;
	char			subKeyName[MQ_INSTALLATION_NAME_LENGTH + 8];
	char			fullName[384];
	char			traceInfo[1024];		// work variable to build trace message

	// initialize the directory name in case the lookup fails
	memset(subKeyName, 0, sizeof(subKeyName));

	if (is64bit)
	{
		// 64-bit OS
		regKeyName = MQ71INSKEY64;
	}
	else
	{
		// 32-bit OS
		regKeyName = MQ71INSKEY;
	}

	// open the registry key that we will enumerate
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					   regKeyName,
					   0,
					   KEY_ENUMERATE_SUB_KEYS,
					   &regkey);

	if (ERROR_SUCCESS == ret)
	{
		// enumerate subkeys of the installation key
		ret2 = RegEnumKey(regkey,
						  index,
						  (char *)&subKeyName,
						  sizeof(subKeyName) - 1);

		// check if it worked
		while (ERROR_SUCCESS == ret2)
		{
			// indicate that at least one 7.1 or later QMgr was found
			foundMQ71 = TRUE;

			// initialize the memory for the full registry key
			memset(fullName, 0, sizeof(fullName));

			// build the full subkey name
			strcpy(fullName, regKeyName);
			strcat(fullName, "\\");
			strcat(fullName, subKeyName);

			// open the subkey
			ret3 = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
							   fullName,
							   0,
							   KEY_QUERY_VALUE,
							   &regkey2);

			// check if it worked
			if (ERROR_SUCCESS == ret3)
			{
				// process this key
				processMQinstallation(regkey2);

				// close the key
				ret3 = RegCloseKey(regkey2);

			}

			// move on to the next subkey
			index++;
			ret2 = RegEnumKey(regkey,
							  index,
							  (char *)&subKeyName,
							  sizeof(subKeyName) - 1);
		}

		// close the key
		ret2 = RegCloseKey(regkey);

		// find the installed components and display them in the trace
		processMQcomponents();
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::SearchForMQ71() foundMQ71=%d ret=%d ret2=%d ret3=%d highestMQVersion=%d highestMQRelease=%d higestVRMF=%s subKeyName=%s MQ71Path=%s", foundMQ71, ret, ret2, ret3, highestMQVersion, highestMQRelease, higestVRMF, subKeyName, MQ71Path);

		// trace exit from SearchForMQ71
		logTraceEntry(traceInfo);
	}
}

const char * DataArea::getMQ71serverInstPath()

{
	InstTable		*instTab;
	const char		*ptr=NULL;
	char			VRMF[16];
	char			traceInfo[1024];		// work variable to build trace message

	// initialize higest version
	memset(VRMF, 0, sizeof(VRMF));
	strcpy(VRMF, ""); // Visual Studio code analyser doesn't understand memset and gives "might not be zero-terminated" warning

	// point to the first MQ 7.1 installation
	instTab = firstInstallation;

	// loop through all the installations that were found
	while (instTab != NULL)
	{
		// is this a server installation?
		if (MQ71_SERVER_INST == instTab->type)
		{
			// server installation
			// check the level
			if (strcmp(instTab->VRMF, VRMF) > 0)
			{
				// newer version - use the latest
				ptr = instTab->filePath;
				strcpy(VRMF, instTab->VRMF);
			}
		}

		// move on to the next installation
		instTab = (InstTable *)instTab->next;
	}

	if (traceEnabled)
	{
		// check if an installation was found
		if (ptr != NULL)
		{
			// create the trace line
			sprintf(traceInfo, "Exiting DataArea::getMQ71serverInstPath() VRMF=%s ptr=%8.8X path=%s", VRMF, (unsigned int)ptr, ptr);
		}
		else
		{
			// create no server install found trace entry
			strcpy(traceInfo, "Exiting DataArea::getMQ71serverInstPath() No MQ 7.1 server installation found");
		}

		// trace exit from getMQ71serverInstPath
		logTraceEntry(traceInfo);
	}

	// return pointer to install directory or null
	return ptr;
}

void DataArea::releaseInstallEntries()

{
	InstTable	*nextEntry=NULL;

	// loop through all the installation entries in the table
	while (firstInstallation != NULL)
	{
		// get a pointer to the entry
		nextEntry = (InstTable *)firstInstallation->next;

		// release the storage
		rfhFree(firstInstallation);

		// move on to the next entry
		firstInstallation = nextEntry;
	}

	if (traceEnabled)
	{
		// trace exit from releaseInstallEntries
		logTraceEntry("Exiting DataArea::releaseInstallEntries()");
	}
}

////////////////////////////////////////////////
//
// Routine to parse the output of the DSPMQ
// command and create an entry linking the
// queue manager to an installation.  There will
// only be an installation if the queue manager
// is at V7.1 or higher.  This routine will also
// check if the queue manager is a standby or
// default queue manager.
//
////////////////////////////////////////////////

QmgrInstTable * DataArea::CreateQMgrInstEntry(const char * dspmqOutput, const char * qmgrName)

{
	const char *	ptr=NULL;
	const char *	endPtr=NULL;
	QmgrInstTable	*newQmgr;
	char			traceInfo[1024];		// work variable to build trace message

	// create a queue manager entry
	// allocate storage for a new installation
	newQmgr = (QmgrInstTable *)rfhMalloc(sizeof(QmgrInstTable), "QmgrTabl");

	// check if the malloc worked
	if (newQmgr != NULL)
	{
		// clear the memory
		memset(newQmgr, 0, sizeof(QmgrInstTable));

		// set the values in the new table entry
		strcpy(newQmgr->name, qmgrName);

		// look for the default key word
		ptr = strstr(dspmqOutput, "DEFAULT(");

		// check if it was found
		if (ptr != NULL)
		{
			// process the result - check for Yes
			if ('y' == ptr[8])
			{
				// found the default queue manager
				newQmgr->isDefault = TRUE;
				defaultQMname = qmgrName;
			}
		}

		// look for the status key word
		ptr = strstr(dspmqOutput, "STATUS(");

		// check if it was found
		if (ptr != NULL)
		{
			// process the result - check for Running as standby
			if (memcmp(ptr + 7, "Running as standby", 18) == 0)
			{
				// found the default queue manager
				newQmgr->isStandby = TRUE;
			}
			else if (memcmp(ptr + 7, "Running)", 8) == 0)
			{
				// found the default queue manager
				newQmgr->isRunning = TRUE;
			}
		}

		// look for the INSTVER key word
		ptr = strstr(dspmqOutput, "INSTVER(");

		// check if it was found
		if (ptr != NULL)
		{
			endPtr = strchr(ptr, ')');
			if ((ptr != NULL) && (endPtr - ptr < sizeof(newQmgr->VRMF)))
			{
				// capture the version and release
				memcpy(newQmgr->VRMF, ptr, endPtr - ptr);
			}
		}

		// look for the INSTNAME key word
		ptr = strstr(dspmqOutput, "INSTNAME(");

		// check if it was found
		if (ptr != NULL)
		{
			endPtr = strchr(ptr, ')');
			if ((ptr != NULL) && (endPtr - ptr < sizeof(newQmgr->installation)))
			{
				// capture the installation name
				memcpy(newQmgr->installation, ptr, endPtr - ptr);
			}
		}

		// look for the INSTPATH key word
		ptr = strstr(dspmqOutput, "INSTPATH(");

		// check if it was found
		if (ptr != NULL)
		{
			endPtr = strchr(ptr, ')');
			if ((ptr != NULL) && (endPtr - ptr < sizeof(newQmgr->filePath)))
			{
				// capture the file path
				memcpy(newQmgr->filePath, ptr, endPtr - ptr);
			}
		}

		// link this entry in the list of queue managers
		newQmgr->next = firstQmgr;
		firstQmgr = newQmgr;

		if (traceEnabled && verboseTrace)
		{
			// create the trace line
			sprintf(traceInfo, "Exiting DataArea::CreateQMgrInstEntry() name=%s isDefault=%d isStandby=%d VRMF=%s installation=%s filePath=%s ", newQmgr->name, newQmgr->isDefault, newQmgr->isStandby, newQmgr->VRMF, newQmgr->installation, newQmgr->filePath);

			// trace exit from CreateQMgrInstEntry
			logTraceEntry(traceInfo);
		}
	}

	// return a pointer to the new entry
	return newQmgr;
}

void DataArea::releaseQMgrEntries()

{
	QmgrInstTable	*nextEntry=NULL;

	// loop through all the installation entries in the table
	while (firstQmgr != NULL)
	{
		// get a pointer to the entry
		nextEntry = (QmgrInstTable *)firstQmgr->next;

		// release the storage
		rfhFree(firstQmgr);

		// move on to the next entry
		firstQmgr = nextEntry;
	}

	if (traceEnabled)
	{
		// trace exit from releaseQMgrEntries
		logTraceEntry("Exiting DataArea::releaseQMgrEntries()");
	}
}

////////////////////////////////////////////////
//
// Routine to enumerate any local QMgrs using
// the DSPMQ command and load the names of the
// associated queues for any that are running
// using PCF messages.
//
////////////////////////////////////////////////

int DataArea::processLocalQMgrs()

{
#ifndef MQCLIENT
	DWORD dwRead = 0;
	HANDLE	hChildStdoutRd, hChildStdoutWr, hChildStdoutRdDup, hStdout;
 	int		charsRead;
	int		i;
	int		ret=-1;
	int		qmRunning=0;
	int		qmStopped=0;
	int		qmStandby=0;
	NAMESTRUCT *	nextQMPtr;
	QmgrInstTable *	nextQMgr;
	char	*ptr;
	char	*qmName;
	char	*buffer = NULL;
	SECURITY_ATTRIBUTES saAttr;
	BOOL	fSuccess;
	char	traceInfo[512];			// work variable to build trace message

	if (traceEnabled)
	{
		// trace entry to routine
		logTraceEntry("Entering DataArea::processLocalQMgrs");
	}

 	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Get the handle to the current STDOUT.
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	// Create a pipe for the child process's STDOUT.
	if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
	{
		return -1;
	}

	// Create noninheritable read handle and close the inheritable read
	// handle.
	fSuccess = DuplicateHandle(GetCurrentProcess(), hChildStdoutRd,
								GetCurrentProcess(), &hChildStdoutRdDup , 0,
								FALSE,
								DUPLICATE_SAME_ACCESS);

	if( !fSuccess  )
	{
		return -2;
	}

    CloseHandle(hChildStdoutRd);

	// Now create the child process
	// This will execute the dspmq program to list local queue managers
	fSuccess = CreateChildProcess(hChildStdoutWr);
	if (!fSuccess)
	{
		return -3;
	}

	// Close the write end of the pipe before reading from the
	// read end of the pipe.
	if (!CloseHandle(hChildStdoutWr))
	{
	   return -4;
	}

	// Read from pipe that is the standard output for child process.
	buffer = (char *)rfhMalloc(128 * 1024, "BUFSTOUT");
	if (!buffer) {
		return -5;
	}

	// Read output from the child process, and write to parent's STDOUT.
	if( !ReadFile( hChildStdoutRdDup, buffer, sizeof(buffer) - 1, &dwRead, NULL))
	{
		dwRead = 0;
	}

	// get the number of characters returned by the command
	charsRead = dwRead;

	if (traceEnabled)
	{
		// trace results of the dspmq command
		dumpTraceData("dspmq output", (unsigned char *)buffer, charsRead);
	}

	// now process the data buffer
	i = 0;
	if (charsRead >= 2) {
		while (i < charsRead)
		{
			ptr = buffer + i;
			while ((i < charsRead) && (buffer[i] != '\n'))
			{
				i++;
			}

			if (i <= charsRead)
			{
				// terminate the string
				buffer[i] = 0;

				if (traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, "input buffer=%s", buffer);

					// write trace entry
					logTraceEntry(traceInfo);
				}

				// check for a QMName entry
				if (dwRead >= 7 && memcmp(ptr, "QMNAME(", 7) == 0)
				{
					// get the queue manager name and create an entry for it
					qmName = ptr + 7;

					// look for the end of the queue manager name
					ptr = strchr(qmName, ')');
					if (ptr != NULL)
					{
						// terminate the queue manager name
						ptr[0] = 0;
						ptr++;

						// create a control block entry for this qm
						nextQMPtr = allocateNameStruct(qmName, "QMgr");

						// check that the allocations worked
						if (nextQMPtr != NULL)
						{
							// Insert queue manager name into the list
							nextQMPtr->nextQM = queueNamesRoot;
							queueNamesRoot = nextQMPtr;

							// check the status of the queue manager
							// first, skip any intervening blanks
							while (' ' == ptr[0])
							{
								ptr++;
							}

							// create a qmgr table entry
							nextQMgr = CreateQMgrInstEntry(ptr, qmName);

							// check if create worked
							if (nextQMgr != NULL)
							{
								// check if this is a standby queue manager
								if (nextQMgr->isStandby)
								{
									// queue manager in standby state
									// therefore, do not try to connect
									nextQMPtr->names->insertName(" <Standby QM - press Load Names to retry>");
									nextQMPtr->count++;
									qmStandby++;

									if (traceEnabled)
									{
										// trace queue manager status
										sprintf(traceInfo, "QM status is standby for %s", qmName);

										// enter trace info
										logTraceEntry(traceInfo);
									}
								}
								else if (nextQMgr->isRunning)
								{
									// queue manager is running
									// count used in trace entry
									qmRunning++;

									// use PCF messages to get the list of queue names
									ret = getQueueNames(qmName);

									// check if any queues were found before
									if (0 == nextQMPtr->count)
									{
										// error trying to load the names and no queue names found
										// display an error message rather than queue names
										nextQMPtr->names->insertName(" <No Queues found - press Load Names to retry>");
										nextQMPtr->count++;
									}
								}
								else
								{
									// queue manager in Ended state - probably not running
									// therefore, do not try to connect
									nextQMPtr->names->insertName(" <QM is not available - press Load Names to retry>");
									nextQMPtr->count++;
									qmStopped++;

									if (traceEnabled)
									{
										// trace queue manager status
										sprintf(traceInfo, "QM status is not availble for %s", qmName);

										// enter trace info
										logTraceEntry(traceInfo);
									}
								}
							}

							if (traceEnabled)
							{
								// create the trace line
								sprintf(traceInfo, "Queue count=%d for QMgr %s", nextQMPtr->count, qmName);

								// enter trace info
								logTraceEntry(traceInfo);
							}
						}
					}
				}
			}

			// move on to the next entry
#pragma warning(suppress: 6385)
			while ((buffer[i] <= ' ') && (i < charsRead))
			{
				i++;
			}
		}
	}

	if (buffer) {
		rfhFree(buffer);
	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::processLocalQMgrs running=%d ended=%d standby=%d", qmRunning, qmStopped, qmStandby);

		// trace exit from processLocalQMgrs
		logTraceEntry(traceInfo);
	}
#endif
	
	return 0;
}

int DataArea::getQueueNames(const char *qmName)

{
	int		ret=-1;
	int		ret2;
	int		ret3;
	char	traceInfo[128];			// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::getQueueNames QMname=%s", qmName);

		// trace entry to getQueueNames
		logTraceEntry(traceInfo);
	}

	//if (loadClientQnames(qmName, errtxt, NULL, NULL, FALSE, NULL, NULL, FALSE, 0) != MQCC_OK)
	ret = (loadNames(qmName, "", LOAD_NAMES_QUEUES, MQQT_LOCAL, m_show_cluster_queues, TRUE) != MQCC_OK);
	ret2 = (loadNames(qmName, "", LOAD_NAMES_QUEUES, MQQT_REMOTE, m_show_cluster_queues, FALSE) != MQCC_OK);
	ret3 = (loadNames(qmName, "", LOAD_NAMES_QUEUES, MQQT_ALIAS, m_show_cluster_queues, FALSE) != MQCC_OK);

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting DataArea::getQueueNames ret=%d ret2=%d ret3=%d", ret, ret2, ret3);

		// trace exit from getQueueNames
		logTraceEntry(traceInfo);
	}

	return ret;
}

BOOL DataArea::CreateChildProcess(HANDLE hChildStdoutWr)

{
	BOOL	worked=TRUE;

#ifndef MQCLIENT
	PROCESS_INFORMATION		piProcInfo;
	STARTUPINFO				siStartInfo;
	BOOL					bFuncRetn=FALSE;
	const char *			MQPtr=NULL;
 	char					progName[512];

	// check if there is an MQ 7.1 server installation
	// there might be an MQ 7.1 client install on this system
	// the client will not have the dspmq command
	MQPtr = getMQ71serverInstPath();

	// initialize the path to the program name
	if (MQPtr != NULL)
	{
		// MQ 7.1 server is installed on this system
		// use the highest level of dspmq installed on the system
		strcpy(progName, MQPtr);

		// append the program name and parameters
		strcat(progName, "\\bin\\dspmq -o all");
	}
	else
	{
		// no MQ 7.1 - should be able to run dspmq without qualification
		// this is original behavior of RFHUtil prior to V7.1
		strcpy(progName, "dspmq");
	}

	// Set up members of the PROCESS_INFORMATION structure.
	memset(&piProcInfo, 0, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure.
	memset(&siStartInfo, 0, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = hChildStdoutWr;
	siStartInfo.hStdOutput = hChildStdoutWr;
	siStartInfo.hStdInput = 0;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	siStartInfo.wShowWindow = SW_HIDE;

	// Create the child process.
	bFuncRetn = CreateProcess(NULL,
							  progName,       // command line
							  NULL,          // process security attributes
							  NULL,          // primary thread security attributes
							  TRUE,          // handles are inherited
							  0,             // creation flags
							  NULL,          // use parent's environment
							  NULL,          // use parent's current directory
							  &siStartInfo,  // STARTUPINFO pointer
							  &piProcInfo);  // receives PROCESS_INFORMATION

	if (0 == bFuncRetn)
	{
		worked = FALSE;
	}
	else
	{
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
#endif

	return worked;
}

///////////////////////////////////
//
// Set the check boxes to select
// both the RFH2 header and the
// usr folder
//
// Called from usr.cpp when the
// message properties are moved
// to the usr folder
//
///////////////////////////////////

void DataArea::selectUsrFolder()

{
	// deselect the usr folder as well
	((RFH *)rfhData)->selectUsrFolder(TRUE);
}

void DataArea::dumpIndFontName(CFont *cf, const char *name)

{
/*	HDC					hDC;
	OUTLINETEXTMETRIC	*otm;
	char				txtMetrics[16384];


	hDC = GetDC(NULL);

	if (hDC != NULL)
	{
		// point to the storage area
		otm = (OUTLINETEXTMETRIC *)&txtMetrics;

		// select the font
		hDC.SelectObject(cf);

		GetOutlineTextMetrics(pDC, &length, otm);

		// release the device context
		ReleaseDC(pDC);
	}*/
}

void DataArea::insertTraceString(CComboBox *cb, const char *str)

{
#ifdef MQCLIENT
	char	traceInfo[512];			// work variable to build trace message

	// check for a duplicate
	if (cb->FindStringExact(-1, str) == CB_ERR)
	{
		// insert the string into the combo box
		cb->AddString(str);

		// check if trace is enabled
		if (traceEnabled && verboseTrace)
		{
			// create the trace line
			sprintf(traceInfo, " Inserting QMgr=%.256s into combo box", str);

			// write data to trace
			logTraceEntry(traceInfo);
		}
	}
	else
	{
		// check if trace is enabled
		if (traceEnabled && verboseTrace)
		{
			// create the trace line
			sprintf(traceInfo, " Ignoring duplicate QMgr=%.256s", str);

			// write data to trace
			logTraceEntry(traceInfo);
		}
	}
#endif
}

///////////////////////////////////////////////////////////
//
// This routine will load the drop down list of queue
// manager names in a combo box.
//
///////////////////////////////////////////////////////////

void DataArea::loadQMComboBox(CComboBox *cb)

{
	char	traceInfo[512];			// work variable to build trace message

	// delete the current contents of the combo box
	//((CComboBox *)GetDlgItem(IDC_QM))->ResetContent();
	cb->ResetContent();

#ifdef MQCLIENT
	int			i;
	int			filelen;
	char		*ptr;
	char		*tabdata;
	FILE		*tabfile;
	char		MQServer[512];
	CString		qm;
	CString		server;
	CRfhutilApp *	app;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	qm.Empty();

	/* get the MQSERVER environment variable */
	ptr = getenv("MQSERVER");

	/* check if MQSERVER is set */
	if (NULL != ptr)
	{
		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Entering DataArea::loadQMComboBox MQSERVER=%.256s", ptr);

			// write data to trace
			logTraceEntry(traceInfo);
		}

		// make sure it is not too long
		if (strlen(ptr) < sizeof(MQServer))
		{
			/* capture the environment variable */
			if (strlen(ptr) < sizeof(MQServer) - 1)
			{
				strcpy(MQServer, ptr);
			}
			else
			{
				MQServer[0] = 0;
			}

			// insert the queue manager name into the combo box
			if (MQServer[0] != 0)
			{
				//((CComboBox *)GetDlgItem(IDC_QM))->AddString(MQServer);
				cb->AddString(MQServer);
			}
		}
	}
	else
	{
		/* check if the channel table path variable is set */
		ptr = getenv("MQCHLLIB");
		if (NULL == ptr)
		{
			MQServer[0] = 0;
			findMQInstallDirectory((unsigned char *)&MQServer, sizeof(MQServer));
		}
		else
		{
			// will the directory name fit?
			if (strlen(ptr) < sizeof(MQServer) - 16)
			{
				// get the directory name
				strcpy(MQServer, ptr);
			}
			else
			{
				// directory name is too long to handle
				MQServer[0] = 0;
			}
		}

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Entering DataArea::loadQMComboBox MQCHLLIB=%.256s", ptr);

			// write data to trace
			logTraceEntry(traceInfo);
		}

		/* check if the last character is a backslash */
		if (MQServer[strlen(MQServer) - 1] != '\\')
		{
			/* insert the trailing backslash */
			strcat(MQServer, "\\");
		}

		/* check if the channel table file name variable is set */
		ptr = getenv("MQCHLTAB");
		if (NULL == ptr)
		{
			strcat(MQServer, "amqclchl.tab");
		}
		else
		{
			// check if the name will fit
			if (strlen(ptr) + strlen(MQServer) < sizeof(MQServer) - 2)
			{
				// capture the file name
				strcat(MQServer, ptr);
			}
		}

		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Entering DataArea::loadQMComboBox MQCHLTAB=%.512s", MQServer);

			// write data to trace
			logTraceEntry(traceInfo);
		}

		/* try to open the file read only in binary mode */
		tabfile = fopen(MQServer, "rb");
		if (NULL == tabfile)
		{
			if (traceEnabled)
			{
				// write data to trace
				logTraceEntry(" No channel table file found");
			}
		}
		else
		{
			/* get the total length of the file */
			fseek(tabfile, 0L, SEEK_END);
			filelen = ftell(tabfile);

			/* allocate an area of memory for the file data */
			tabdata = (char *) rfhMalloc(filelen + 16, "TABDATA ");

			/* check if the allocation worked */
			if (NULL == tabdata)
			{
				// report the error
				logTraceEntry("error allocating memory for client table");
			}
			else
			{
				memset(tabdata, 0, filelen + 16);

				/* point back to the beginning of the file and read the data */
				fseek(tabfile, 0L, SEEK_SET);
				fread(tabdata, 1, filelen, tabfile);

				/* done with the file, close it */
				fclose(tabfile);

				/* process the contents of the file */
				listClientTable(tabdata, filelen, cb);

				/* release the memory we acquired */
				rfhFree(tabdata);
			}
		}
	}

	// insert any queue manager names that were saved in the registry
	for (i=0; i < RFHUTIL_LAST_CLIENT_MAX; i++)
	{
		if (app->lastClientQM[i].GetLength() > 0)
		{
			insertTraceString(cb, (LPCTSTR)(app->lastClientQM[i]));
		}
	}
#else
	const char *	qmName;

	// get a pointer to the first QM name
	qmName = getFirstQMname();

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::loadQMComboBox qmName=%.64s", qmName);

		// write data to trace
		logTraceEntry(traceInfo);
	}

	// loop through all the QM names that we found
	while (qmName != NULL)
	{
		// insert the queue manager name into the combo box
		cb->AddString(qmName);

		// move on to the next item
		qmName = getNextQMname(qmName);

		if (traceEnabled && verboseTrace && (qmName != NULL))
		{
			// create the trace line
			sprintf(traceInfo, "Next queue manager loadQMComboBox qmName=%.64s", qmName);

			// write data to trace
			logTraceEntry(traceInfo);
		}
	}
#endif

	if (traceEnabled)
	{
		// write data to trace
		logTraceEntry("Exiting DataArea::loadQMComboBox");
	}
}

/////////////////////////////////////////////////////////////
//
// Routine to list any queue managers found in the amqclchl
// file and add them to the drop down list in a combo box.
//
/////////////////////////////////////////////////////////////

void DataArea::listClientTable(char *tabdata, int filelen, CComboBox *cb)

{
#ifdef MQCLIENT
	chanHeader	*chPtr;
	char		qmName[MQ_Q_MGR_NAME_LENGTH + 1];
	char		chanName[MQ_CHANNEL_NAME_LENGTH + 1];
	char		connName[MQ_CONN_NAME_LENGTH + 1];
	char		transName[16];
	char		MQServer[1024];
	char		traceInfo[512];							// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering DataArea::listClientTable filelen=%d", filelen);

		// write data to trace
		logTraceEntry(traceInfo);

		// dump out the channel table
		dumpTraceData("CHANTAB ", (unsigned char *)tabdata, filelen);
	}

	if (memcmp(tabdata, "AMQR", 4) == 0)
	{
		/* find the beginning of the channel table entries */
		tabdata += 4;

		/* now enumerate the queue managers */
		chPtr = (chanHeader *)tabdata;
		while (chPtr->seglen > 0)
		{
			// is there a valid channel and connection?
			if ((chPtr->reclen != 0) &&
				(MQCHT_CLNTCONN == chPtr->cd.ChannelType) &&
				(chPtr->cd.ChannelName[0] > ' ') &&
				(chPtr->cd.ConnectionName[0] > ' '))
			{
				// get the qm name
				memcpy(qmName, chPtr->cd.QMgrName, MQ_Q_MGR_NAME_LENGTH);
				qmName[MQ_Q_MGR_NAME_LENGTH] = 0;
				Rtrim(qmName);

				// get the channel name
				memcpy(chanName, chPtr->cd.ChannelName, MQ_CHANNEL_NAME_LENGTH);
				chanName[MQ_CHANNEL_NAME_LENGTH] = 0;
				Rtrim(chanName);

				// get the connection name
				memcpy(connName, chPtr->cd.ConnectionName, MQ_CONN_NAME_LENGTH);
				connName[MQ_CONN_NAME_LENGTH] = 0;
				Rtrim(connName);

				// get the transport type as an ASCII string
				memset(transName, 0, sizeof(transName));
				switch (chPtr->cd.TransportType)
				{
					case MQXPT_LOCAL:
					{
						strcpy(transName, "LOCAL");
						break;
					}
					case MQXPT_LU62:
					{
						strcpy(transName, "LU62");
						break;
					}
					case MQXPT_TCP:
					{
						strcpy(transName, "TCP");
						break;
					}
					case MQXPT_NETBIOS:
					{
						strcpy(transName, "NETBIOS");
						break;
					}
					case MQXPT_SPX:
					{
						strcpy(transName, "SPX");
						break;
					}
					case MQXPT_DECNET:
					{
						strcpy(transName, "DECNET");
						break;
					}
					case MQXPT_UDP:
					{
						strcpy(transName, "UDP");
						break;
					}
				}

				// insert the queue manager name into the combo box
				if (qmName[0] > ' ')
				{
					// create an entry including the channel name and connection name
					sprintf(MQServer, "%s [%s/%s/%s]", qmName, chanName, transName, connName);


					// check if the name is a duplicate
					if (cb->FindStringExact(-1, MQServer) == CB_ERR)
					{
						// add the queue manager name to the combo box
						//((CComboBox *)GetDlgItem(IDC_QM))->AddString(qmName);
						cb->AddString(MQServer);

						if (traceEnabled && verboseTrace)
						{
							// create the trace line
							sprintf(traceInfo, "Next queue manager qmName=%.64s", MQServer);

							// write data to trace
							logTraceEntry(traceInfo);
						}
					}
					else
					{
						// is trace enabled?
						if (traceEnabled && verboseTrace)
						{
							// create the trace line
							sprintf(traceInfo, "Duplicate entry not loaded qmName=%.64s", MQServer);

							// write data to trace
							logTraceEntry(traceInfo);
						}
					}
				}
				else
				{
					// create an MQServer form of entry
					// create the client string
					sprintf(MQServer, "%s/%s/%s", chanName, transName, connName);

					// check if the name is a duplicate
					if (cb->FindStringExact(-1, MQServer) == CB_ERR)
					{
						// add the name to the combo box
						//((CComboBox *)GetDlgItem(IDC_QM))->AddString(MQServer);
						cb->AddString(MQServer);

						if (traceEnabled && verboseTrace)
						{
							// create the trace line
							sprintf(traceInfo, "Next queue manager MQServer=%.64s", MQServer);

							// write data to trace
							logTraceEntry(traceInfo);
						}
					}
					else
					{
						// is trace enabled?
						if (traceEnabled && verboseTrace)
						{
							// create the trace line
							sprintf(traceInfo, "Duplicate entry not loaded qmName=%.64s", MQServer);

							// write data to trace
							logTraceEntry(traceInfo);
						}
					}
				}
			}

			tabdata += chPtr->seglen;
			chPtr = (chanHeader *)tabdata;
		}
	}
	else
	{
		/* Not a valid client channel table */
		if (traceEnabled)
		{
			logTraceEntry("Channel table header missing - see AMCHLTAB");
		}
	}
#endif
}
