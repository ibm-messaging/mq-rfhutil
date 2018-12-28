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

// WritePubs.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "mqsubs.h"
#include "WritePubs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// WritePubs dialog

// this is the user-defined message
#define WM_USER_CLOSE		WM_APP+21

#define DEFAULT_DELIMITER	"#@#@#"
#define DEFAULT_PROP_DELIM	"$%$%$"


WritePubs::WritePubs(CWnd* pParent /*=NULL*/)
	: CDialog(WritePubs::IDD, pParent)
{
	CRfhutilApp			*app;									// pointer to MFC application object

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	//{{AFX_DATA_INIT(WritePubs)
	m_delimiter = _T("");
	m_end_count = _T("");
	m_error_msg = _T("");
	m_file_name = _T("");
	m_prop_delim = _T("");
	m_wait_time = _T("");
	m_delim_type = -1;
	m_batchsize = _T("");
	m_write_once = FALSE;
	m_new_correlid = FALSE;
	m_new_msgid = FALSE;
	m_use_mqmd = FALSE;
	m_use_topic = FALSE;
	m_use_props = FALSE;
	m_warn_no_match = FALSE;
	//}}AFX_DATA_INIT

	// set a default delimiter
	m_prop_delim = DEFAULT_PROP_DELIM;
	m_delimiter = DEFAULT_DELIMITER;
	m_delim_type = SAVE_DELIM_CHAR;

	// initialize the batchsize parameter to 1
	m_batchsize = _T("1");

	pDoc=NULL;
	thread = NULL;
	exitCode=STILL_ACTIVE;

	// select default options
	m_new_msgid = app->psWriteNewMsgid;
	m_new_correlid = app->psWriteNewCorrelid;
	m_use_topic = app->psWriteUseTopic;
	m_use_props = app->psWriteUseProps;
	m_use_mqmd = app->psWriteUseMQMD;
	m_write_once = app->psWriteWriteOnce;
	m_warn_no_match = app->psWriteWarnNoSub;
	m_wait_time = app->psWriteWaitTime;
	m_end_count = app->psWriteMsgCount;
	m_batchsize = app->psWriteBatchSize;
	m_file_name = app->psWriteFileName;
}


void WritePubs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(WritePubs)
	DDX_Text(pDX, IDC_WRITEPUBS_DELIMITER, m_delimiter);
	DDV_MaxChars(pDX, m_delimiter, 64);
	DDX_Text(pDX, IDC_WRITEPUBS_END_COUNT, m_end_count);
	DDV_MaxChars(pDX, m_end_count, 9);
	DDX_Text(pDX, IDC_WRITEPUBS_ERROR_MSG, m_error_msg);
	DDX_Text(pDX, IDC_WRITEPUBS_FILENAME, m_file_name);
	DDV_MaxChars(pDX, m_file_name, 500);
	DDX_Text(pDX, IDC_WRITEPUBS_PROP_DELIM, m_prop_delim);
	DDV_MaxChars(pDX, m_prop_delim, 64);
	DDX_Text(pDX, IDC_WRITEPUBS_WAIT, m_wait_time);
	DDV_MaxChars(pDX, m_wait_time, 7);
	DDX_Radio(pDX, IDC_WRITEPUBS_DELIM_CHAR, m_delim_type);
	DDX_Text(pDX, IDC_WRITEPUBS_BATCHSIZE, m_batchsize);
	DDX_Check(pDX, IDC_WRITEPUBS_WRITE_ONCE, m_write_once);
	DDX_Check(pDX, IDC_WRITEPUBS_NEW_CORRELID, m_new_correlid);
	DDX_Check(pDX, IDC_WRITEPUBS_NEW_MSGID, m_new_msgid);
	DDX_Check(pDX, IDC_WRITEPUBS_USE_MQMD, m_use_mqmd);
	DDX_Check(pDX, IDC_WRITEPUBS_USE_TOPIC, m_use_topic);
	DDX_Check(pDX, IDC_WRITEPUBS_USE_PROPS, m_use_props);
	DDX_Check(pDX, IDC_WRITEPUBS_WARN_NO_MATCH, m_warn_no_match);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(WritePubs, CDialog)
	//{{AFX_MSG_MAP(WritePubs)
	ON_BN_CLICKED(IDC_WRITEPUBS_BROWSE, OnWritepubsBrowse)
	ON_BN_CLICKED(IDC_WRITEPUBS_PUB_MSGS, OnWritepubsPubMsgs)
	ON_BN_CLICKED(IDC_WRITEPUBS_STOP_PUB, OnWritepubsStopPub)
	ON_COMMAND(IDC_WRITEPUBS_DELIM_CHAR_KEY, OnWritepubsDelimCharKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_DELIM_CHAR_KEY, OnUpdateWritepubsDelimCharKey)
	ON_COMMAND(IDC_WRITEPUBS_DELIM_HEX_KEY, OnWritepubsDelimHexKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_DELIM_HEX_KEY, OnUpdateWritepubsDelimHexKey)
	ON_COMMAND(IDC_WRITEPUBS_NEW_MSGID_KEY, OnWritepubsNewMsgIdKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_NEW_MSGID_KEY, OnUpdateWritepubsNewMsgIdKey)
	ON_COMMAND(IDC_WRITEPUBS_NEW_CORRELID_KEY, OnWritepubsNewCorrelIdKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_NEW_CORRELID_KEY, OnUpdateWritepubsNewCorrelIdKey)
	ON_COMMAND(IDC_WRITEPUBS_WRITE_ONCE_KEY, OnWritepubsWriteOnceKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_WRITE_ONCE_KEY, OnUpdateWritepubsWriteOnceKey)
	ON_COMMAND(IDC_WRITEPUBS_USE_MQMD_KEY, OnWritepubsUseMQMDKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_USE_MQMD_KEY, OnUpdateWritepubsUseMQMDKey)
	ON_COMMAND(IDC_WRITEPUBS_USE_PROPS_KEY, OnWritepubsUsePropsKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_USE_PROPS_KEY, OnUpdateWritepubsUsePropsKey)
	ON_COMMAND(IDC_WRITEPUBS_USE_TOPIC_KEY, OnWritepubsUseTopicKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_USE_TOPIC_KEY, OnUpdateWritepubsUseTopicKey)
	ON_COMMAND(IDC_WRITEPUBS_STOP_KEY, OnWritepubsStopKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_STOP_KEY, OnUpdateWritepubsStopKey)
	ON_COMMAND(IDC_WRITEPUBS_WRITE_KEY, OnWritepubsWriteKey)
	ON_UPDATE_COMMAND_UI(IDC_WRITEPUBS_WRITE_KEY, OnUpdateWritepubsWriteKey)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_USER_CLOSE, OnUserClose)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, WritePubs::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, WritePubs::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// WritePubs message handlers

BOOL WritePubs::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pTTTStruct;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pTTTStruct;
	CString strTipText;

	UINT nID = pTTTStruct->idFrom;

	if (((pTTTStruct->code == TTN_NEEDTEXTA) && (pTTTA->uFlags & TTF_IDISHWND)) ||
		((pTTTStruct->code == TTN_NEEDTEXTW) && (pTTTW->uFlags & TTF_IDISHWND)))
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
		if(nID != 0)
		{
			if (pTTTStruct->code == TTN_NEEDTEXTA)
			{
				pTTTA->lpszText = MAKEINTRESOURCE(nID);
				pTTTA->hinst = AfxGetResourceHandle();
			}
			else
			{
//				pTTTW->lpszText = MAKEINTRESOURCE(nID);
//				pTTTW->hinst = AfxGetResourceHandle();
			}

			pResult = 0;
			return(TRUE);
		}
	}

	return(FALSE);
}

BOOL WritePubs::PreTranslateMessage(MSG* pMsg) 

{
	if ((WM_KEYFIRST <= pMsg->message) && (pMsg->message <= WM_KEYLAST))
	{
		// check for an accelerator key
		if (m_hAccel && ::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
		{
			// accelerator key was handled so return TRUE
			return TRUE;
		}
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

void WritePubs::OnStop() 

{
	int		count=0;				// counter to end loop after 5 seconds

	// Signal the capture thread to end
	parms->stop = 1;
	
	// get the value of the exit code
	GetExitCodeThread(thread, (unsigned long *)&exitCode);

	// check if the thread has ended
	while ((count < 50) && (STILL_ACTIVE == exitCode))
	{
		// wait another tenth of a second
		Sleep(100);

		// get the thread exit code
		if (!GetExitCodeThread(thread, (unsigned long *)&exitCode))
		{
			// the thread has probably already exited
			// set a user exit code
			exitCode=20;
		}

		// increment the counter
		count++;
	}

	// end the dialog
	OnOK();
}

LONG WritePubs::OnUserClose(UINT wParam, LONG lParam)

{
	// shut down the dialog
	OnStop();

	return 0;
}

BOOL WritePubs::OnInitDialog()
 
{
	CDialog::OnInitDialog();

	// tool tips are provided for this dialog and must be initialized
	EnableToolTips(TRUE);

	// load the accelerator key table for this dialog
	m_hAccel = ::LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_WRITE_PUBS_ACCELERATOR));

	// clear the error text
	SetDlgItemText(IDC_WRITEPUBS_ERROR_MSG, "   ");
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void WritePubs::OnWritepubsBrowse() 

{
	int	rc;

	// invoke standard dialog to choose file name
	CFileDialog fd(TRUE, NULL, NULL);
	rc = fd.DoModal();

	//	Did the user press the OK button? 
	if ((rc == IDOK) && (strlen(fd.GetPathName()) > 0))
	{
		// get the current data from the dialog into the instance variables
		UpdateData(TRUE);

		// set the full file name in the DataArea object
		m_file_name = fd.GetPathName( );

		// update the form data from the instance variables
		UpdateData(FALSE);
	}
}

//////////////////////////////////////
//
// This routine runs under a separate
// user thread.  It will end if there
// is an error, when the user presses
// the Stop Publishing button or if
// a maximum number of messages to
// publish is specified.
//
//////////////////////////////////////

UINT WriteThread(LPVOID inParms)

{
	int					rc=MQRC_NONE;							// return code
	int					rc1=MQRC_NONE;							// return code
	int					time=0;									// total number of seconds waited
	int					sleep=0;								// indicator to sleep for a number of milliseconds
	int					mqmdLen;								// length of MQMD
	unsigned char *		delimPtr=NULL;							// pointer to delimiter or NULL if no delimiter found
	unsigned char *		propDelimPtr=NULL;						// pointer to properties delimiter or NULL if no delimiter found
	CRfhutilApp			*app;									// pointer to MFC application object
	WRITEPARMS			*parms=(WRITEPARMS *)inParms;			// pointer to parameters area
	DataArea			*pDoc=(DataArea *)(parms->pDocument);	// pointer to DataArea object
	BOOL				isMQMD;									// indicator if MQMD found
	char				auditTxt[32];							// work area to build audit message
	MQMD2				mqmd={MQMD2_DEFAULT};					// MQMD area
	MQMD2				mqmdDefault={MQMD2_DEFAULT};			// default MQMD area
	char				traceInfo[512];						// work variable to build trace message

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering WriteThread fileData=%8.8X fileSize=%d", (unsigned int)parms->fileData, parms->fileSize);

		// trace entry to WriteThread
		app->logTraceEntry(traceInfo);
	}

	// initialize the message length and data to include the entire file
	parms->msgData = parms->fileData;
	parms->msgSize = parms->fileSize;

	while ((0 == parms->err) && 
		   (0 == parms->stop) &&
		   (MQRC_NONE == rc) &&
		   (MQRC_NONE == rc1) &&
		   (parms->msgSize > 0) &&
		   ((0 == parms->msgCount) || (parms->count < parms->msgCount)))
	{
		// check for a delimiter between the messages
		if (parms->delimLen > 0)
		{
			// scan for a delimiter sequence
			delimPtr = pDoc->scanForDelim(parms->msgData, parms->msgSize, (char *)&(parms->delimiter), parms->delimLen);

			// did we find a delimiter?
			if (delimPtr != NULL)
			{
				// reduce the message length
				parms->msgSize = delimPtr - parms->msgData;
			}
		}

		// initialize the MQMD area
		memcpy(&mqmd, &mqmdDefault, sizeof(mqmd));
		parms->mqmd = &mqmd;

		// check for an MQMD in the input data
		// is the message long enough to have an MQMD?
		if (parms->msgSize >= sizeof(MQMD))
		{
			// check if there is an MQMD in the data and translate it
			// if necessary
			isMQMD = translateMQMD((MQMD2 *)parms->msgData);

			// was there an MQMD?
			if (isMQMD)
			{
				// the MQMD has been translated and converted to little-Endian if necessary
				// get the length of the MQMD (it could be V1)
				// get the length of the MQMD
				if (MQMD_VERSION_1 == ((MQMD2 *)parms->msgData)->Version)
				{
					// set the length to a V1 length
					mqmdLen = sizeof(MQMD);
				}
				else
				{
					// set the length to a V2 length
					mqmdLen = sizeof(MQMD2);
				}

				// should the MQMD be used?
				if (parms->useMQMD)
				{
					// copy the existing MQMD to the data area
					memcpy(&mqmd, parms->msgData, mqmdLen);
				}

				// reset the data pointer and length to bypass the MQMD
				parms->msgData += mqmdLen;
				parms->msgSize -= mqmdLen;

				// make sure the message length is not less than zero
				if (parms->msgSize < 0)
				{
					// reset the length to 0 to avoid problems
					parms->msgSize = 0;
				}
			}
		}

		// set user properties length and pointer to zero
		parms->propLen = 0;
		parms->userProp = NULL;

		// is there enough room for a properties delimiter?
		if ((parms->propDelimLen > 0) && (parms->msgSize > parms->propDelimLen))
		{
			// scan for a properties delimiter in the user data area
			propDelimPtr = pDoc->scanForDelim(parms->msgData, parms->msgSize, (char *)&(parms->propDelim), parms->propDelimLen);

			// check if a properties delimiter was found
			if (propDelimPtr != NULL)
			{
				// remember the start of the properties
				parms->userProp = (char *)parms->msgData;
				parms->propLen = propDelimPtr - parms->msgData;

				// move the user data pointer to point to the user data
				// and adjust the size of the user data
				parms->msgData = propDelimPtr + parms->propDelimLen;
				parms->msgSize -= parms->propLen + parms->propDelimLen;
			}

			// make sure the message length is not less than zero
			if (parms->msgSize < 0)
			{
				// reset the message size to zero
				// this should never be necessary but better to be safe
				parms->msgSize = 0;
			}
		}

		// set the pointer to the MQMD (either the default MQMD or a copy of the MQMD)
		// try to publish another message
		rc = pDoc->pubWriteMsg(parms);

		// check if it worked
		if (MQRC_NONE == rc)
		{
			// using units of work and at end of batch?
			if (parms->uow >= parms->batchSize)
			{
				// commit the unit of work - will reset the uow count to zero
				rc1 = pDoc->pubWriteMsgCommit(parms);
			}

			// check if a Sleep should be issued
			// sleep is issued every batchSize messages
			if ((parms->waitTime > 0) && (parms->batch >= parms->batchSize) && ((parms->count < parms->msgCount) || (0 == parms->msgCount)) && (0 == parms->err))
			{
				// check if there are messages to commit
				// a commit should always be issued before sleeping
				if (parms->uow > 0)
				{
					// commit the unit of work - will reset the uow count to zero
					rc1 = pDoc->pubWriteMsgCommit(parms);
				}

				// reset the sleep indicator
				parms->batch = 0;

				// sleep the specified number of milliseconds
				Sleep(parms->waitTime);
			}

			// check if there are more messages in the file
			if (NULL == delimPtr)
			{
				// last message in file processed
				// process the same file again
				// reset the data offset and length
				parms->msgData = parms->fileData;
				parms->msgSize = parms->fileSize;

				// check if one pass was selected
				if (parms->onePass)
				{
					// stop after one pass through the messages in the file
					parms->stop = TRUE;
				}
			}
			else
			{
				// move on to next message after the delimiter
				// calculate the offset and length of the next message
				parms->msgData = delimPtr + parms->delimLen;
				parms->msgSize = (parms->fileData + parms->fileSize) - parms->msgData;

				// check if there is any data left
				if (parms->msgSize <= 0)
				{
					// already processed the last message
					// handle the case where there is a delimiter at the end of the data
					//.normally there should not be a delimiter at the end of the data but
					// handle this case anyway
					// process the same file again
					// reset the data offset and length
					parms->msgData = parms->fileData;
					parms->msgSize = parms->fileSize;

					// check if one pass was selected
					if (parms->onePass)
					{
						// stop after one pass through the messages in the file
						parms->stop = TRUE;
					}
				}
			}
		}
	}

	// check if a unit of work is active
	if (parms->uow > 0)
	{
		// commit the rest of the messages
		rc1 = pDoc->pubWriteMsgCommit(parms);
	}

	// close the publication queue
	rc1 = pDoc->pubWriteMsgClose(parms);

	// release the file data area
	rfhFree(parms->fileData);

	// call the audit routine to record the publishe messages action
	sprintf(auditTxt, "WriteMsg %I64d msgs", parms->count);
	app->createAuditRecord((LPCTSTR)pDoc->currentQM, (LPCTSTR)pDoc->currentQ, auditTxt, rc);

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting WriteThread rc=%d rc1=%d count=%I64d err=%d stop=%d errMsg=%s", rc, rc1, parms->count, parms->err, parms->stop, parms->errMsg);

		// trace exit from WriteThread
		app->logTraceEntry(traceInfo);
	}

	// shutdown the dialog using a custom message
	// this message will drive the OnStop() method
	// on the main message processing thread
	PostMessage(parms->m_hWnd, WM_USER_CLOSE, 0, 0L);

	// return from the thread, which will end
	return rc;
}

void WritePubs::OnWritepubsPubMsgs() 

{
	int				rc=0;
	int				delimLen;				// length of the delimiter between messages
	FILE			*inputFile;				// input file
	CRfhutilApp		*app;					// pointer to MFC application object
	char			traceInfo[1024];		// work variable to build trace message

	// get the form data into the instance variables
	UpdateData(TRUE);

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering WritePubs::OnWritepubsPubMsgs m_end_count=%s QMname=%s topic=%.256s m_new_msgid=%d m_use_mqmd=%d m_write_once=%d", (LPCTSTR)m_end_count, parms->QMname, parms->topic, m_new_msgid, m_use_mqmd, m_write_once);

		// trace entry to OnWritepubsPubMsgs
		app->logTraceEntry(traceInfo);
	}

	// initialize the parms pointers, etc
	parms->pDocument = pDoc;
	parms->m_hWnd = m_hWnd;

	// make sure there is a file name
	// do some basic editing
	// was a file name specified?
	if (m_file_name.GetLength() == 0)
	{
		SetDlgItemText(IDC_WRITEPUBS_ERROR_MSG, "File name is required");
		return;
	}

	// check for a message delimiter
	if ((m_delimiter.GetLength() == 0) && (atoi((LPCTSTR)m_end_count) > 1))
	{
		// a delimiter must be specified
		SetDlgItemText(IDC_WRITEPUBS_ERROR_MSG, "Delimiter is required if count > 1");
		return;
	}

	// try to open the input file 
	// the name of the first file will always be what the user specified with
	// nothing appended to the name
	inputFile = fopen((LPCTSTR)m_file_name, "rb");

	if (NULL == inputFile)
	{
		// can't open the file
		// indicate the error
		SetDlgItemText(IDC_WRITEPUBS_ERROR_MSG, "Unable to open input file");

		// return to caller
		return;
	}

	// figure out how big the file is
	// determine the length of the data
	fseek(inputFile, 0L, SEEK_END);
	parms->fileSize = ftell(inputFile);

	// return the pointer to the beginning of the data
	fseek(inputFile, 0L, SEEK_SET);

	// allocate memory for the file
	parms->fileData = (unsigned char *)rfhMalloc(parms->fileSize + 2, "WRIFILED");

	// make sure it worked
	if (NULL == parms->fileData)
	{
		// tell the user what happened
		SetDlgItemText(IDC_WRITEPUBS_ERROR_MSG, "Unable to allocate memory for file");

		// return to caller
		return;
	}

	// read it into memory
	fread(parms->fileData, 1, parms->fileSize, inputFile);

	// done with the file handle so close it
	fclose(inputFile);

	// get the maximum message count and wait time
	parms->msgCount = atoi((LPCTSTR)m_end_count);
	parms->waitTime = atoi((LPCTSTR)m_wait_time);
	parms->batchSize = atoi((LPCTSTR)m_batchsize);

	// check for one pass through file option
	parms->onePass = m_write_once;

	// get the option to use MQMDs, properties and topics embedded with the data
	parms->useMQMD = m_use_mqmd;
	parms->useTopic = m_use_topic;
	parms->useProps = m_use_props;

	// indicate if warn if no match is selected
	parms->warnNoSub = m_warn_no_match;

	// capture settings from dialog
	parms->new_msg_id = m_new_msgid;
	parms->new_correl_id = m_new_correlid;

	// get the length of the delimiter
	delimLen = m_delimiter.GetLength();

	// is there a delimiter?
	if (delimLen > 0)
	{
		// set the delimiter in the parameters area
		// what type of delimiter do we have?
		if (SAVE_DELIM_HEX == m_delim_type)
		{
			// delimiter is specified as hex
			delimLen >>= 1;
			HexToAscii((unsigned char *)((LPCTSTR)m_delimiter), delimLen << 1, (unsigned char *)&(parms->delimiter));
		}
		else
		{
			// set the delimiter value
			strcpy((char *)&(parms->delimiter), (LPCTSTR)m_delimiter);
		}

		// set the length of the delimiter
		parms->delimLen = delimLen;
	}

	// get the length of the properties delimiter
	delimLen = m_prop_delim.GetLength();

	// get the properties delimiter if there is one
	if (delimLen > 0)
	{
		// what type of delimiter do we have?
		if (SAVE_DELIM_HEX == m_delim_type)
		{
			// delimiter is specified as hex
			delimLen >>= 1;
			HexToAscii((unsigned char *)((LPCTSTR)m_prop_delim), delimLen << 1, (unsigned char *)&(parms->propDelim));
		}
		else
		{
			strcpy((char *)&(parms->propDelim), (LPCTSTR)m_prop_delim);
		}

		// set the length of the properties delimiter
		parms->propDelimLen = delimLen;
	}

	// try to remember the previous settings
	app->psWriteNewMsgid = m_new_msgid;
	app->psWriteNewCorrelid = m_new_correlid;
	app->psWriteUseTopic = m_use_topic;
	app->psWriteUseProps = m_use_props;
	app->psWriteUseMQMD = m_use_mqmd;
	app->psWriteWriteOnce = m_write_once;
	app->psWriteWarnNoSub = m_warn_no_match;
	app->psWriteWaitTime = m_wait_time;
	app->psWriteMsgCount = m_end_count;
	app->psWriteBatchSize = m_batchsize;
	app->psWriteFileName = m_file_name;

	// start the capture thread
	thread = NULL;
	thread = AfxBeginThread(WriteThread, (void *)parms);

	// check if it worked
	if (thread != NULL)
	{
		// Disable the capture and cancel buttons and enable the stop capture button
		((CButton *)GetDlgItem(IDC_WRITEPUBS_PUB_MSGS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDCANCEL))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_WRITEPUBS_STOP_PUB))->EnableWindow(TRUE);
	}
	else
	{
		// free the data area - this is normally done when the thread terminates but in this case
		// the additional thread was not started so the acquired storage will be freed here
		rfhFree(parms->fileData);
	}

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting WritePubs::OnWritepubsPubMsgs rc=%d delimLen=%d propDelimLen=%d waitTime=%d exitCode=%d fileSize=%d fileData=%8.8X thread=%8.8X", rc, parms->delimLen, parms->propDelimLen, parms->waitTime, exitCode, parms->fileSize, (unsigned int)parms->fileData, (unsigned int)thread);

		// trace exit from OnWritepubsPubMsgs
		app->logTraceEntry(traceInfo);
	}
}

void WritePubs::OnWritepubsStopPub() 

{
	// call the OnStop method to terminate the thread
	OnStop();
}

void WritePubs::OnWritepubsDelimCharKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_delim_type = SAVE_DELIM_CHAR;

	// Update the controls
	UpdateData (FALSE);
}

void WritePubs::OnUpdateWritepubsDelimCharKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void WritePubs::OnWritepubsDelimHexKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_delim_type = SAVE_DELIM_HEX;

	// Update the controls
	UpdateData (FALSE);
}

void WritePubs::OnUpdateWritepubsDelimHexKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void WritePubs::OnWritepubsNewMsgIdKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_new_msgid)
	{
		m_new_msgid = FALSE;
	}
	else
	{
		m_new_msgid = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void WritePubs::OnUpdateWritepubsNewMsgIdKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void WritePubs::OnWritepubsNewCorrelIdKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_new_correlid)
	{
		m_new_correlid = FALSE;
	}
	else
	{
		m_new_correlid = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void WritePubs::OnUpdateWritepubsNewCorrelIdKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void WritePubs::OnWritepubsWriteOnceKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_write_once)
	{
		m_write_once = FALSE;
	}
	else
	{
		m_write_once = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void WritePubs::OnUpdateWritepubsWriteOnceKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void WritePubs::OnWritepubsUseMQMDKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_use_mqmd)
	{
		m_use_mqmd = FALSE;
	}
	else
	{
		m_use_mqmd = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void WritePubs::OnUpdateWritepubsUseMQMDKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void WritePubs::OnWritepubsUsePropsKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_use_props)
	{
		m_use_props = FALSE;
	}
	else
	{
		m_use_props = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void WritePubs::OnUpdateWritepubsUsePropsKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void WritePubs::OnWritepubsUseTopicKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_use_topic)
	{
		m_use_topic = FALSE;
	}
	else
	{
		m_use_topic = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void WritePubs::OnUpdateWritepubsUseTopicKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void WritePubs::OnWritepubsStopKey() 

{
	// handle accelerator key (alt+x)
	// should only get here if there is a write operation in progress
	// Stop the current operation
	OnStop();
}

void WritePubs::OnUpdateWritepubsStopKey(CCmdUI* pCmdUI) 

{
	// check if a publish operation is in progress
	// enable the accelerator key
	pCmdUI->Enable(((CButton *)GetDlgItem(IDC_WRITEPUBS_STOP_PUB))->IsWindowEnabled());
}

void WritePubs::OnWritepubsWriteKey() 

{
	// handle accelerator key (alt+w)
	// check if a publish operation is in progress
	OnWritepubsPubMsgs();
}

void WritePubs::OnUpdateWritepubsWriteKey(CCmdUI* pCmdUI) 

{
	// check if a publish operation is in progress
	// enable the accelerator key
	pCmdUI->Enable(((CButton *)GetDlgItem(IDC_WRITEPUBS_PUB_MSGS))->IsWindowEnabled());
}
