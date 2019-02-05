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

// CapPubs.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "CapPubs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_USER_CLOSE		WM_APP+20

#define DEFAULT_DELIMITER	"#@#@#"
#define DEFAULT_PROP_DELIM	"$%$%$"

/////////////////////////////////////////////////////////////////////////////
// CCapPubs dialog


CCapPubs::CCapPubs(CWnd* pParent /*=NULL*/)
	: CDialog(CCapPubs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCapPubs)
	m_filename = _T("");
	m_delimiter = _T("");
	m_append_file = FALSE;
	m_end_count = _T("");
	m_incl_headers = FALSE;
	m_incl_mqmd = FALSE;
	m_prop_delim = _T("");
	m_delimiter_type = -1;
	m_max_wait = _T("");
	m_incl_topic = FALSE;
	m_incl_props = FALSE;
	//}}AFX_DATA_INIT

	// set a default delimiter
	m_prop_delim = DEFAULT_PROP_DELIM;
	m_delimiter = DEFAULT_DELIMITER;
	m_delimiter_type = SAVE_DELIM_CHAR;
	m_incl_mqmd = TRUE;
	m_incl_headers = TRUE;
	m_incl_topic = TRUE;
	m_incl_props = TRUE;
	m_hAccel = NULL;

	pDoc=NULL;
	thread = NULL;
	exitCode=STILL_ACTIVE;
	parms = NULL;
	helperMsg = 0;
}


void CCapPubs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCapPubs)
	DDX_Text(pDX, IDC_CAPTURE_FILENAME, m_filename);
	DDV_MaxChars(pDX, m_filename, 254);
	DDX_Text(pDX, IDC_CAPTURE_DELIMITER, m_delimiter);
	DDV_MaxChars(pDX, m_delimiter, 64);
	DDX_Check(pDX, IDC_CAPTURE_APPEND, m_append_file);
	DDX_Text(pDX, IDC_CAPTURE_END_COUNT, m_end_count);
	DDX_Check(pDX, IDC_CAPTURE_INCL_HEADERS, m_incl_headers);
	DDX_Check(pDX, IDC_CAPTURE_INCL_MQMD, m_incl_mqmd);
	DDX_Text(pDX, IDC_CAPTURE_PROP_DELIM, m_prop_delim);
	DDV_MaxChars(pDX, m_prop_delim, 64);
	DDX_Radio(pDX, IDC_CAPTURE_DELIM_CHAR, m_delimiter_type);
	DDX_Text(pDX, IDC_CAPTURE_MAX_WAIT, m_max_wait);
	DDV_MaxChars(pDX, m_max_wait, 8);
	DDX_Check(pDX, IDC_CAPTURE_INCL_TOPIC, m_incl_topic);
	DDX_Check(pDX, IDC_CAPTURE_INCL_PROPS, m_incl_props);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCapPubs, CDialog)
	//{{AFX_MSG_MAP(CCapPubs)
	ON_BN_CLICKED(IDC_CAPTURE_STOP, OnCaptureStop)
	ON_BN_CLICKED(IDC_CAPTURE, OnCapture)
	ON_BN_CLICKED(IDC_CAPTURE_BROWSE, OnCaptureBrowse)
	ON_COMMAND(IDC_CAPTURE_DELIM_CHAR_KEY, OnCapPubsDelimCharKey)
	ON_UPDATE_COMMAND_UI(IDC_CAPTURE_DELIM_CHAR_KEY, OnUpdateCapPubsDelimCharKey)
	ON_COMMAND(IDC_CAPTURE_DELIM_HEX_KEY, OnCapPubsDelimHexKey)
	ON_UPDATE_COMMAND_UI(IDC_CAPTURE_DELIM_HEX_KEY, OnUpdateCapPubsDelimHexKey)
	ON_COMMAND(IDC_CAPTURE_INCL_TOPIC_KEY, OnCapPubsInclTopicKey)
	ON_UPDATE_COMMAND_UI(IDC_CAPTURE_INCL_TOPIC_KEY, OnUpdateCapPubsInclTopicKey)
	ON_COMMAND(IDC_CAPTURE_INCL_PROPS_KEY, OnCapPubsInclPropsKey)
	ON_UPDATE_COMMAND_UI(IDC_CAPTURE_INCL_PROPS_KEY, OnUpdateCapPubsInclPropsKey)
	ON_COMMAND(IDC_CAPTURE_INCL_MQMD_KEY, OnCapPubsInclMQMDKey)
	ON_UPDATE_COMMAND_UI(IDC_CAPTURE_INCL_MQMD_KEY, OnUpdateCapPubsInclMQMDKey)
	ON_COMMAND(IDC_CAPTURE_INCL_HEADERS_KEY, OnCapPubsInclHeadersKey)
	ON_UPDATE_COMMAND_UI(IDC_CAPTURE_INCL_HEADERS_KEY, OnUpdateCapPubsInclHeadersKey)
	ON_COMMAND(IDC_CAPTURE_APPEND_KEY, OnCapPubsAppendFileKey)
	ON_UPDATE_COMMAND_UI(IDC_CAPTURE_APPEND_KEY, OnUpdateCapPubsAppendFileKey)
	ON_COMMAND(IDC_CAPTURE_STOP_KEY, OnCapPubsStopKey)
	ON_UPDATE_COMMAND_UI(IDC_CAPTURE_STOP_KEY, OnUpdateCapPubsStopKey)
	ON_COMMAND(IDC_CAPTURE_KEY, OnCapPubsWriteKey)
	ON_UPDATE_COMMAND_UI(IDC_CAPTURE_KEY, OnUpdateCapPubsAppendFileKey)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_USER_CLOSE, OnUserClose)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CCapPubs::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CCapPubs::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCapPubs message handlers

BOOL CCapPubs::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

BOOL CCapPubs::PreTranslateMessage(MSG* pMsg) 

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

UINT CaptureThread(LPVOID inParms)

{
	int			rc=MQRC_NONE;							// return code
	int			time=0;									// total number of seconds waited
	CRfhutilApp *app;									// pointer to MFC application object
	CAPTPARMS	*parms=(CAPTPARMS *)inParms;			// pointer to parameters area
	DataArea	*pDoc=(DataArea *)(parms->pDocument);	// pointer to DataArea object
	char		auditTxt[32];							// work area to build audit message
	char		traceInfo[512];							// work variable to build trace message

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	while ((0 == parms->err) && 
		   (0 == parms->stop) &&
		   (MQRC_NONE == rc) &&
		   ((0 == parms->msgCount) || (parms->count < parms->msgCount)) &&
		   ((0 == parms->maxTime) || (time < parms->maxTime)))
	{
		// try to capture another message
		rc = pDoc->captureMsg(parms);

		// was there no message available?
		if (MQRC_NO_MSG_AVAILABLE == rc)
		{
			if (app->isTraceEnabled())
			{
				// create the trace line
				sprintf(traceInfo, " No msg avail rc=%d count=%I64d time=%d maxTime=%d err=%d stop=%d errMsg=%s", rc, parms->count, time, parms->maxTime, parms->err, parms->stop, parms->errMsg);

				// trace exit from OnWritepubsPubMsgs
				app->logTraceEntry(traceInfo);
			}

			// no message available
			// sleep for 1 second
			Sleep(1000);

			// keep track of the total time (approximate)
			// if a maximum time was specified then this
			// will eventually cause the thread to end
			time++;

			// change the return code to a normal code so the loop keeps going
			rc = MQRC_NONE;
		}
		else
		{
			// check for other return codes
			if (rc != MQRC_NONE)
			{
				// indicate an error occured so the loop stops
				parms->err = 1;
			}
		}
	}

	// check for a timeout
	if ((parms->maxTime > 0) && (time >= parms->maxTime))
	{
		// timed out - set a messasge to let the user know
		strcpy(parms->errMsg, "Timed out ");
	}

	// call the audit routine to record the CaptureMsg action
	sprintf(auditTxt, "CaptureMsg %I64d msgs", parms->count);
	app->createAuditRecord((LPCTSTR)pDoc->currentQM, (LPCTSTR)pDoc->currentQ, auditTxt, rc);

	// close the file
	fclose(parms->outputFile);

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CaptureThread rc=%d count=%I64d msgCount=%I64d time=%d maxTime=%d err=%d stop=%d errMsg=%s auditTxt=%s", rc, parms->count, parms->msgCount, time, parms->maxTime, parms->err, parms->stop, parms->errMsg, auditTxt);

		// trace exit from OnWritepubsPubMsgs
		app->logTraceEntry(traceInfo);
	}

	// check if the user stopped the capture
	if (0 == parms->stop)
	{
		// try to shutdown the dialog using a custom message
		// this message will drive the OnCaptureStop() method
		// on the main message processing thread
		PostMessage(parms->m_hWnd, WM_USER_CLOSE, 0, 0L);
	}

	// return from the dialog
	return rc;
}

void CCapPubs::OnCaptureStop() 

{
	int		count=0;				// counter to end loop after 5 seconds

	// Signal the capture thread to end
	parms->stop = 1;
	
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

void CCapPubs::OnCapture() 

{
	int				rc=0;
	int				delimLen;
	CRfhutilApp		*app;					// pointer to MFC application object
	char			traceInfo[1024];		// work variable to build trace message

	// get the form data into the instance variables
	UpdateData(TRUE);

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCapPubs::OnCapture m_end_count=%s m_incl_headers=%d m_incl_mqmd=%d m_append_file=%d m_incl_props=%d m_incl_topic=%d m_max_wait=%s", (LPCTSTR)m_end_count, m_incl_headers, m_incl_mqmd, m_append_file, m_incl_props, m_incl_topic, (LPCTSTR)m_max_wait);

		// trace entry to OnCapture
		app->logTraceEntry(traceInfo);
	}

	// initialize the parms pointers, etc
	parms->m_hWnd = m_hWnd;

	// make sure there is a file name
	// do some basic editing
	// was a file name specified?
	if (m_filename.GetLength() == 0)
	{
		SetDlgItemText(IDC_CAPTURE_ERROR_MSG, "File name is required");
		return;
	}

	// check for a message delimiter
	// there is one case where a delimiter is not required - when a single
	// message is to be captured and not appended to an existing file
	if ((m_delimiter.GetLength() == 0) && (m_end_count != 1) && (!m_append_file))
	{
		// a delimiter must be specified
		SetDlgItemText(IDC_CAPTURE_ERROR_MSG, "Delimiter is required");
		return;
	}

	// get the maximum message count and wait time
	parms->msgCount = atoi((LPCTSTR)m_end_count);
	parms->maxTime = atoi((LPCTSTR)m_max_wait);

	// get the append option into the parameters area
	if (1 == m_append_file)
	{
		// append the data to existing file
		parms->appendFile = true;
	}
	else
	{
		// overwrite the file if it exists
		parms->appendFile = false;
	}

	// get the include MQMD option into the parameters area
	if (1 == m_incl_mqmd)
	{
		// include the MQMQ with the data
		parms->includeMQMD = true;
	}
	else
	{
		// do not include the MQMD with the data
		parms->includeMQMD = false;
	}

	// set the option to remove headers into the parameters area
	if (1 == m_incl_headers)
	{
		// headers are to be included (not removed)
		parms->removeHeaders = false;
	}
	else
	{
		// headers are to be removed
		parms->removeHeaders = true;
	}

	// set the option to include message properties
	if (1 == m_incl_props)
	{
		// capture user properties
		parms->inclProps = true;
	}
	else
	{
		// do not capture user properties
		parms->inclProps = false;
	}

	// set the option to include message properties
	if (1 == m_incl_topic)
	{
		// capture topic only
		parms->inclTopic = true;
	}
	else
	{
		// do not capture topic (may still be captured as general user property)
		parms->inclTopic = false;
	}

	// get the length of the delimiter
	delimLen = m_delimiter.GetLength();

	// is there a delimiter?
	if (delimLen > 0)
	{
		// set the delimiter in the parameters area
		// what type of delimiter do we have?
		if (SAVE_DELIM_HEX == m_delimiter_type)
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
		if (SAVE_DELIM_HEX == m_delimiter_type)
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

	// try to open the output file 
	// do this first to avoid the embarassment of reading a message and not being 
	// able to open the output file
	// the name of the first file will always be what the user specified with
	// nothing appended to the name
	/* check for an append option */
	if (parms->appendFile)
	{
		// open output file for append
		parms->outputFile = fopen((LPCTSTR)m_filename, "ab");
	}
	else
	{
		/* open output file */
		parms->outputFile = fopen((LPCTSTR)m_filename, "wb");
	}

	if (NULL == parms->outputFile)
	{
		// can't open the file
		// indicate the error
		SetDlgItemText(IDC_CAPTURE_ERROR_MSG, "Unable to open output file");

		if (app->isTraceEnabled())
		{
			// create the trace line
			sprintf(traceInfo, "Exiting CCapPubs::OnCapture Unable to open file m_filename=%.512s", (LPCTSTR)m_filename);

			// trace exit from OnCapture
			app->logTraceEntry(traceInfo);
		}

		// return to caller
		return;
	}

	// check for an append option
	if (parms->appendFile)
	{
		// check if the file is empty or being appended
		fseek(parms->outputFile, 0L, SEEK_END);
		parms->fileLen = ftell(parms->outputFile);
	}

	// make sure the thread started
	// start the capture thread
	thread = AfxBeginThread(CaptureThread, (void *)parms);

	// check if it worked
	if (thread != NULL)
	{
		// Disable the capture and cancel buttons and enable the stop capture button
		((CButton *)GetDlgItem(IDC_CAPTURE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDCANCEL))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CAPTURE_STOP))->EnableWindow(TRUE);
	}

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCapPubs::OnCapture rc=%d, delimLen=%d propDelimLen=%d exitCode=%d parms->fileLen=%d thread=%8.8X m_filename=%.512s", rc, parms->delimLen, parms->propDelimLen, exitCode, parms->fileLen, (unsigned int)thread, (LPCTSTR)m_filename);

		// trace exit from OnCapture
		app->logTraceEntry(traceInfo);
	}
}

BOOL CCapPubs::OnInitDialog()
 
{
	CDialog::OnInitDialog();

	// tool tips are provided for this dialog and must be initialized
	EnableToolTips(TRUE);

	// load the accelerator key table for this dialog
	m_hAccel = ::LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_CAP_PUBS_ACCELERATOR));

	// clear the error text
	SetDlgItemText(IDC_CAPTURE_ERROR_MSG, "   ");
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LONG CCapPubs::OnUserClose(UINT wParam, LONG lParam)

{
	// shut down the dialog
	OnCaptureStop();

	return 0;
}

void CCapPubs::OnCaptureBrowse() 

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
		m_filename = fd.GetPathName( );

		// update the form data from the instance variables
		UpdateData(FALSE);
	}
}

void CCapPubs::OnCapPubsDelimCharKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_delimiter_type = SAVE_DELIM_CHAR;

	// Update the controls
	UpdateData (FALSE);
}

void CCapPubs::OnUpdateCapPubsDelimCharKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CCapPubs::OnCapPubsDelimHexKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_delimiter_type = SAVE_DELIM_HEX;

	// Update the controls
	UpdateData (FALSE);
}

void CCapPubs::OnUpdateCapPubsDelimHexKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CCapPubs::OnCapPubsInclTopicKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_incl_topic)
	{
		m_incl_topic = FALSE;
	}
	else
	{
		m_incl_topic = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CCapPubs::OnUpdateCapPubsInclTopicKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CCapPubs::OnCapPubsInclPropsKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_incl_props)
	{
		m_incl_props = FALSE;
	}
	else
	{
		m_incl_props = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CCapPubs::OnUpdateCapPubsInclPropsKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CCapPubs::OnCapPubsInclMQMDKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_incl_mqmd)
	{
		m_incl_mqmd = FALSE;
	}
	else
	{
		m_incl_mqmd = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CCapPubs::OnUpdateCapPubsInclMQMDKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CCapPubs::OnCapPubsInclHeadersKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_incl_headers)
	{
		m_incl_headers = FALSE;
	}
	else
	{
		m_incl_headers = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CCapPubs::OnUpdateCapPubsInclHeadersKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CCapPubs::OnCapPubsAppendFileKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_append_file)
	{
		m_append_file = FALSE;
	}
	else
	{
		m_append_file = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CCapPubs::OnUpdateCapPubsAppendFileKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CCapPubs::OnCapPubsStopKey() 

{
	// handle accelerator key (alt+x)
	// should only get here if there is a write operation in progress
	// Stop the current operation
	OnCaptureStop();
}

void CCapPubs::OnUpdateCapPubsStopKey(CCmdUI* pCmdUI) 

{
	// check if a publish operation is in progress
	// enable the accelerator key
	pCmdUI->Enable(((CButton *)GetDlgItem(IDC_CAPTURE_STOP))->IsWindowEnabled());
}

void CCapPubs::OnCapPubsWriteKey() 

{
	// handle accelerator key (alt+w)
	// check if a publish operation is in progress
	OnCapture();
}

void CCapPubs::OnUpdateCapPubsWriteKey(CCmdUI* pCmdUI) 

{
	// check if a publish operation is in progress
	// enable the accelerator key
	pCmdUI->Enable(((CButton *)GetDlgItem(IDC_CAPTURE))->IsWindowEnabled());
}
