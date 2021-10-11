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

// General.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "rfhutil.h"
#include "General.h"
#include "DataArea.h"
#include "comsubs.h"
#include "DispQ.h"
#include "Savemsgs.h"
#include "LoadQ.h"
#include "MoveQ.h"
#include "ConnUser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+2

// environment variable that is supposed to point to an SSL certificate key file 
#define MQSSLKEYR "MQSSLKEYR"

/////////////////////////////////////////////////////////////////////////////
// General property page

IMPLEMENT_DYNCREATE(General, CPropertyPage)

General::General() : CPropertyPage(General::IDD)

{
	CRfhutilApp		*app = (CRfhutilApp *)AfxGetApp();
	HDC				hDC = NULL;
	HWND			hWnd = NULL;
	TEXTMETRICA		tm;
	char			traceInfo[512];

	//{{AFX_DATA_INIT(General)
	m_q_name = _T("");
	m_qm_name = _T("");
	m_file_name = _T("");
	m_file_size = _T("");
	m_copybook_file_name = _T("");
	m_errmsg = _T("");
	m_remote_qm = _T("");
	m_cluster_bind = 0;
	m_queue_type = _T("");
	m_q_depth = _T("");
	m_all_avail = FALSE;
	m_logical_order = FALSE;
	m_set_UserID = FALSE;
	m_get_by_correlid = FALSE;
	m_get_by_groupid = FALSE;
	m_set_all = FALSE;
	m_get_by_msgid = FALSE;
	m_convert = FALSE;
	m_complete_msg = FALSE;
	m_alt_userid = FALSE;
	m_new_correl_id = FALSE;
	m_new_msg_id = TRUE;
	m_filter = _T("");
	m_mq_props = 0;
	m_file_codepage = _T("");
	m_close_options = Q_CLOSE_NONE;
	m_allow_seg = FALSE;
	//}}AFX_DATA_INIT
	pDoc = NULL;

	// set a default code page for files
	m_file_codepage.Format("%d",DEF_ASCII_CCSID);

	// initialize connection code page
	m_conn_ccsid = 0;

	// get the device context handle
	hDC = GetDC()->GetSafeHdc();

	// make sure th device context exists
	if (hDC != NULL)
	{
		// Get the display device characteristics
		app->technology = GetDeviceCaps(hDC, TECHNOLOGY);
		app->horSize = GetDeviceCaps(hDC, HORZSIZE);
		app->vertSize = GetDeviceCaps(hDC, VERTSIZE);
		app->horRes = GetDeviceCaps(hDC, HORZRES);
		app->vertRes = GetDeviceCaps(hDC, VERTRES);
		app->horLogPixels = GetDeviceCaps(hDC, LOGPIXELSX);
		app->vertLogPixels = GetDeviceCaps(hDC, LOGPIXELSY);

		if (app->horLogPixels > 0)
		{
			// calculate screen width in tenth of an inch
			app->scrWinch = (10 * app->width) / app->horLogPixels;
		}

		if (app->vertLogPixels > 0)
		{
			// calculate screen height in tenth of an inch
			app->scrHinch = (10 * app->height) / app->vertLogPixels;
		}

		// now get the text metrics of the 14 pt default fixed font set earlier
		if (GetTextMetricsA(hDC, &tm))
		{
			// calculate the width of the characters
			app->charWidthAvg = tm.tmAveCharWidth;
			app->charWidthMax = tm.tmMaxCharWidth;
			app->charHeight = tm.tmHeight;
			app->fontPitchFamily = tm.tmPitchAndFamily;
		}

		// build trace line
		sprintf(traceInfo, " technology=%d horSize=%d vertSize=%d", app->technology, app->horSize, app->vertSize);

		// add info to trace
		app->logTraceEntry(traceInfo);

		// build trace line
		sprintf(traceInfo, " horRes=%d vertRes=%d horLogPixels=%d vertLogPixels=%d", app->horRes, app->vertRes, app->horLogPixels, app->vertLogPixels);

		// add info to trace
		app->logTraceEntry(traceInfo);

		// build trace line
		sprintf(traceInfo, " scrWinch=%d.%d\" scrHinch=%d.%d\"", app->scrWinch / 10, app->scrWinch % 10, app->scrHinch / 10, app->scrHinch % 10);

		// add info to trace
		app->logTraceEntry(traceInfo);

		// build trace line
		sprintf(traceInfo, " charWidthAvg=%d charWidthMax=%d charHeight=%d fontPitchFamily=%1.1X", app->charWidthAvg, app->charWidthMax, app->charHeight, app->fontPitchFamily);

		// add info to trace
		app->logTraceEntry(traceInfo);
	}
	else
	{
		app->logTraceEntry(" hDC is NULL");
	}

	this->SetFont(&(app->m_fixed_font),1);
}

General::~General()

{
}

void General::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(General)
	DDX_Text(pDX, IDC_Q_NAME, m_q_name);
	DDV_MaxChars(pDX, m_q_name, 255);
	DDX_Text(pDX, IDC_QM, m_qm_name);
	DDX_Text(pDX, IDC_FILE_NAME, m_file_name);
	DDX_Text(pDX, IDC_FILE_SIZE, m_file_size);
	DDX_Text(pDX, IDC_COPYBOOK_FILE_NAME, m_copybook_file_name);
	DDX_Text(pDX, IDC_ERRMSG, m_errmsg);
	DDX_Text(pDX, IDC_REMOTE_QM, m_remote_qm);
	DDV_MaxChars(pDX, m_remote_qm, 48);
	DDX_Radio(pDX, IDC_BIND_AS_Q, m_cluster_bind);
	DDX_Text(pDX, IDC_QUEUE_TYPE, m_queue_type);
	DDX_Text(pDX, IDC_Q_DEPTH, m_q_depth);
	DDX_Check(pDX, IDC_ALL_AVAIL, m_all_avail);
	DDX_Check(pDX, IDC_LOGICAL_ORDER, m_logical_order);
	DDX_Check(pDX, IDC_SET_USERID, m_set_UserID);
	DDX_Check(pDX, IDC_GET_BY_CORRELID, m_get_by_correlid);
	DDX_Check(pDX, IDC_GET_BY_GROUP_ID, m_get_by_groupid);
	DDX_Check(pDX, IDC_SET_ALL, m_set_all);
	DDX_Check(pDX, IDC_GET_MSG_ID, m_get_by_msgid);
	DDX_Check(pDX, IDC_MAIN_CONVERT, m_convert);
	DDX_Check(pDX, IDC_COMPLETE_MSG, m_complete_msg);
	DDX_Check(pDX, IDC_ALT_USERID, m_alt_userid);
	DDX_Check(pDX, IDC_NEW_CORREL_ID, m_new_correl_id);
	DDX_Check(pDX, IDC_NEW_MSG_ID, m_new_msg_id);
	DDX_Text(pDX, IDC_MAIN_FILTER, m_filter);
	DDX_Radio(pDX, IDC_MAIN_PROPS_AS_QUEUE, m_mq_props);
	DDX_Text(pDX, IDC_MAIN_FILE_CODEPAGE, m_file_codepage);
	DDV_MaxChars(pDX, m_file_codepage, 5);
	DDX_Radio(pDX, IDC_CLOSE_NONE, m_close_options);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(General, CPropertyPage)
	//{{AFX_MSG_MAP(General)
	ON_BN_CLICKED(IDC_EXIT, OnExit)
	ON_BN_CLICKED(IDC_READQ, OnReadq)
	ON_BN_CLICKED(IDC_WRITEQ, OnWriteq)
	ON_BN_CLICKED(ID_READ_FILE, OnFileOpen)
	ON_BN_CLICKED(IDC_COPYBOOK_MAIN, OnCopybook)
	ON_BN_CLICKED(IDC_CLEAR_DATA, OnClearData)
	ON_BN_CLICKED(IDC_CLEAR_ALL, OnClearAll)
	ON_BN_CLICKED(IDC_WRITE_FILE, OnWriteFile)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_ENDBR, OnEndbr)
	ON_BN_CLICKED(IDC_BRNEXT, OnBrnext)
	ON_BN_CLICKED(IDC_STARTBR, OnStartbr)
	ON_BN_CLICKED(IDC_BIND_AS_Q, OnBindAsQ)
	ON_BN_CLICKED(IDC_BIND_NOT_FIXED, OnBindNotFixed)
	ON_BN_CLICKED(IDC_BIND_ON_OPEN, OnBindOnOpen)
	ON_BN_CLICKED(IDC_PURGE, OnPurgeQ)
	ON_CBN_DROPDOWN(IDC_QM, OnDropdownQm)
	ON_CBN_SELENDCANCEL(IDC_QM, OnSelendcancelQm)
	ON_CBN_SELCHANGE(IDC_QM, OnSelchangeQm)
	ON_BN_CLICKED(IDC_EXIT_BUTTON, OnExitButton)
	ON_CBN_DROPDOWN(IDC_Q_NAME, OnDropdownQName)
	ON_CBN_SELCHANGE(IDC_Q_NAME, OnSelchangeQName)
	ON_CBN_SELENDCANCEL(IDC_Q_NAME, OnSelendcancelQName)
	ON_CBN_EDITCHANGE(IDC_Q_NAME, OnEditchangeQName)
	ON_BN_CLICKED(IDC_CLOSEQ, OnCloseq)
	ON_BN_CLICKED(IDC_SET_USERID, OnSetUserid)
	ON_BN_CLICKED(IDC_LOAD_QUEUE_NAMES, OnLoadQueueNames)
	ON_CBN_SETFOCUS(IDC_QM, OnSetfocusQm)
	ON_CBN_EDITCHANGE(IDC_QM, OnEditchangeQm)
	ON_BN_CLICKED(IDC_SET_ALL, OnSetAll)
	ON_BN_CLICKED(IDC_BRPREV, OnBrprev)
	ON_BN_CLICKED(IDC_MAIN_DISPLAYQ, OnMainDisplayq)
	ON_BN_CLICKED(IDC_MAIN_SAVEQ, OnMainSaveq)
	ON_BN_CLICKED(IDC_MAIN_LOADQ, OnMainLoadq)
	ON_BN_CLICKED(IDC_SET_CONN_USER, OnSetConnUser)
	ON_BN_CLICKED(IDC_ALT_USERID, OnAltUserid)
	ON_BN_CLICKED(IDC_NEW_CORREL_ID, OnNewCorrelId)
	ON_BN_CLICKED(IDC_NEW_MSG_ID, OnNewMsgId)
	ON_BN_CLICKED(IDC_MAIN_MOVEQ, OnMoveQ)
	ON_EN_CHANGE(IDC_MAIN_FILE_CODEPAGE, OnChangeMainFileCodepage)
	ON_EN_KILLFOCUS(IDC_MAIN_FILE_CODEPAGE, OnKillfocusMainFileCodepage)
	ON_BN_CLICKED(IDC_CLOSE_NONE, OnCloseNone)
	ON_BN_CLICKED(IDC_CLOSE_DELETE, OnCloseDelete)
	ON_BN_CLICKED(IDC_CLOSE_PURGE, OnClosePurge)
	ON_CBN_SELENDOK(IDC_QM, OnSelendokQm)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, General::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, General::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// General message handlers

void General::OnExit() 

{
	// The exit button has been pushed
	AfxGetApp()->ExitInstance();
//	PostQuitMessage(0);
}

void General::OnReadq() 

{
	// User has pressed the Read Q button
	// Read a message from a queue
	CRfhutilApp *	app;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);

	// set the get options
	pDoc->m_all_avail = m_all_avail;
	pDoc->m_logical_order = m_logical_order;
	pDoc->m_get_by_msgid = m_get_by_msgid;
	pDoc->m_get_by_correlid = m_get_by_correlid;
	pDoc->m_get_by_groupid = m_get_by_groupid;
	pDoc->m_convert = m_convert;
	pDoc->m_complete_msg = m_complete_msg;
	pDoc->m_mq_props = m_mq_props;

	// set the selector
	pDoc->m_filter = m_filter;

	// Read the message
	app->BeginWaitCursor();
	pDoc->getMessage ((LPCTSTR)m_qm_name, (LPCTSTR)m_q_name, READQ);
	app->EndWaitCursor();

	// turn off the set User id flag, since the user id may have changed
	m_set_UserID = FALSE;
	pDoc->m_setUserID = FALSE;

	// Clear the file name, since we read the data from a queue
	pDoc->fileName[0] = 0;

	// update the form data from the instance variables
	UpdateData(FALSE);

	// set the windows title to the queue name
	if (app->m_mainWnd != NULL)
	{
		app->m_mainWnd->SetWindowText(m_q_name);
	}
	
	// refresh the variables on the screen
	UpdatePageData();
}

void General::OnWriteq() 

{
	// User has pressed the Write Q button
	// Write a message to a queue
	CRfhutilApp *	app;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);

	// set the segmentation variables in the document
	pDoc->m_logical_order = m_logical_order;
	pDoc->m_setUserID = m_set_UserID;
	pDoc->m_set_all = m_set_all;
	pDoc->m_mq_props = m_mq_props;
	pDoc->m_new_msg_id = m_new_msg_id;
	pDoc->m_new_correl_id = m_new_correl_id;

	// Now, call a routine in the document, passing the 
	// queue manager and queue names, to write the data
	// to the appropriate MQSeries queue
	app->BeginWaitCursor();
	pDoc->putMessage ((LPCTSTR)m_qm_name, (LPCTSTR)m_remote_qm, (LPCTSTR)m_q_name);
	app->EndWaitCursor();

	// refresh the variables on the screen
	UpdatePageData();
}

void General::OnBrowse() 

{
	// User has pressed the Browse Q button
	// Browse a message from a queue.  The message remains on the queue.
	CRfhutilApp *	app;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);

	// set the get options
	pDoc->m_all_avail = m_all_avail;
	pDoc->m_logical_order = m_logical_order;
	pDoc->m_get_by_msgid = m_get_by_msgid;
	pDoc->m_get_by_correlid = m_get_by_correlid;
	pDoc->m_get_by_groupid = m_get_by_groupid;
	pDoc->m_convert = m_convert;
	pDoc->m_complete_msg = m_complete_msg;
	pDoc->m_mq_props = m_mq_props;

	// set the selector
	pDoc->m_filter = m_filter;

	// Read the message
	pDoc->getMessage ((LPCTSTR)m_qm_name, (LPCTSTR)m_q_name, BROWSEQ);

	// Clear the file name, since we read the data from a queue
	pDoc->fileName[0] = 0;

	// turn off the set User id flag, since the user id may have changed
	m_set_UserID = FALSE;
	pDoc->m_setUserID = FALSE;

	// set the windows title to the queue name
	if (((CRfhutilApp *)AfxGetApp())->m_mainWnd != NULL)
	{
		((CRfhutilApp *)AfxGetApp())->m_mainWnd->SetWindowText(m_q_name);
	}
	
	// update the form data from the instance variables
	UpdateData(FALSE);

	// refresh the variables on the screen
	UpdatePageData();
}

void General::OnFileOpen() 

{
	// User has pressed the Read File button
	if (pDoc->traceEnabled)
	{
		pDoc->logTraceEntry("Entered General::OnFileOpen()");
	}

	// Read the contents of a file into memory
	pDoc->ReadDataFile();

	// refresh the variables on the screen
	UpdatePageData();
}

//////////////////////////////////////////////////
//
// write the current message data to a file
// This may include the MQMD and message headers
// as well as the message data.
//
// A standard Windows file dialog box is used
// to allow the user to select a file name and
// location to write the data.
//
//////////////////////////////////////////////////

void General::OnWriteFile() 

{
	// The user has pressed the write file button
	if (pDoc->traceEnabled)
	{
		pDoc->logTraceEntry("Entered General::OnWriteFile()");
	}

	// write the current data to a file
	pDoc->WriteDataFile();	

	// refresh the variables on the screen so they reflect the results of the writeFile
	UpdatePageData();
}

BOOL General::OnSetActive() 

{
	// This dialog is about to become the active dialog
#ifndef MQCLIENT
	//((CComboBox *)GetDlgItem(IDC_QM))->LimitText(255);
	m_qmComboBox.LimitText(MQ_Q_NAME_LENGTH);
#endif

	// get the form data into the instance variables
	UpdateData(TRUE);

	m_qm_name = pDoc->m_QM_name;
	m_q_name = pDoc->m_Q_name;
	m_remote_qm = pDoc->m_remote_QM;

	// update the form data from the instance variables
	UpdateData(FALSE);

	UpdatePageData();

	BOOL ret = CPropertyPage::OnSetActive();
	
	// send a custom message to switch the focus to the QM combo box
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return ret;
}

//////////////////////////////////////////////////
//
// This routine is called when the some operation
// has made a change that requires the current
// fields in the dialog to be updated with new
// values.
//
//////////////////////////////////////////////////

void General::UpdatePageData()

{
	char		traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, " General::UpdatePageData() pDoc->browseActive=%d pDoc->propertiesSupported=%d", pDoc->browseActive, pDoc->propertiesSupported);

		// trace entry to UpdatePageData
		pDoc->logTraceEntry(traceInfo);
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	m_copybook_file_name = pDoc->m_copybook_file_name;

	m_file_name = pDoc->fileName;
	m_file_size.Format("%d", pDoc->fileSize);

	if (pDoc->m_error_msg.GetLength() > 0)
	{
		pDoc->updateMsgText();
	}

	m_errmsg = pDoc->m_msg_text;

	m_q_depth.Empty();
	if (pDoc->m_q_depth >= 0)
	{
		m_q_depth.Format("%d", pDoc->m_q_depth);
	}

	m_queue_type = pDoc->m_queue_type;
	m_set_UserID = pDoc->m_setUserID;

	// check if we are in a browse or not
	if (1 == pDoc->browseActive)
	{
		// browse is active, set the buttons appropriately
		((CButton *)GetDlgItem(ID_READ_FILE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CLEAR_DATA))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CLEAR_ALL))->EnableWindow(FALSE);
#ifndef SAFEMODE
		((CButton *)GetDlgItem(IDC_READQ))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PURGE))->EnableWindow(FALSE);
#endif
		((CButton *)GetDlgItem(IDC_WRITEQ))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_BROWSE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_STARTBR))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_BRNEXT))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_ENDBR))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CLOSEQ))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_LOAD_QUEUE_NAMES))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_DISPLAYQ))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_LOADQ))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_SAVEQ))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_LOGICAL_ORDER))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_COMPLETE_MSG))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_ALL_AVAIL))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_CONVERT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_MOVEQ))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_AS_QUEUE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_NONE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_YES))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_RFH2))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_COMPAT))->EnableWindow(FALSE);

		if (1 == pDoc->browsePrevActive)
		{
			((CButton *)GetDlgItem(IDC_BRPREV))->EnableWindow(TRUE);
		}
		else
		{
			((CButton *)GetDlgItem(IDC_BRPREV))->EnableWindow(FALSE);
		}
	}
	else
	{
		// browse is not active, set the buttons appropriately
		((CButton *)GetDlgItem(ID_READ_FILE))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CLEAR_DATA))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CLEAR_ALL))->EnableWindow(TRUE);
#ifndef SAFEMODE
		((CButton *)GetDlgItem(IDC_READQ))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PURGE))->EnableWindow(TRUE);
#endif
		((CButton *)GetDlgItem(IDC_WRITEQ))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_BROWSE))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_STARTBR))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_BRNEXT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_BRPREV))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_ENDBR))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_LOAD_QUEUE_NAMES))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_DISPLAYQ))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_LOADQ))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_SAVEQ))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_LOGICAL_ORDER))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_COMPLETE_MSG))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_ALL_AVAIL))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_CONVERT))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_MOVEQ))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_AS_QUEUE))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_NONE))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_YES))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_RFH2))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_MAIN_PROPS_COMPAT))->EnableWindow(TRUE);

		// check if we are connected to a queue manager
		((CButton *)GetDlgItem(IDC_CLOSEQ))->EnableWindow(pDoc->isConnectionActive());
	}

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void General::OnCopybook() 

{
	// Change or set the name of the COBOL copybook used to format the data
	int	rc;

	// set the copybook file and parse it
	rc = pDoc->readCopybookFile();

	// refresh the variables on the screen
	UpdatePageData();
}


void General::OnClearData() 

{
	// Reset the message data contents
	pDoc->clearFileData();

	// refresh the variables on the screen
	UpdatePageData();
}

void General::OnClearAll() 

{
	// Clear the message contents and headers
	pDoc->DeleteContents();

	// refresh the variables on the screen
	UpdatePageData();
}

BOOL General::OnKillActive() 

{
#ifndef MQCLIENT
	m_qmComboBox.LimitText(MQ_Q_NAME_LENGTH);
#endif

	// get the form data into the instance variables
	UpdateData(TRUE);

	pDoc->m_QM_name = m_qm_name;
	pDoc->m_Q_name = m_q_name;
	pDoc->m_remote_QM = m_remote_qm;
	pDoc->m_mq_props = m_mq_props;

	// update the selector in the DataArea
	pDoc->m_filter = m_filter;

	// set the flag to allow user ids to be set
	pDoc->m_setUserID = m_set_UserID;
	
	// set the flag to allow the message id to be set
	pDoc->m_get_by_msgid = m_get_by_msgid;

	// set the flag if set all is selected
	pDoc->m_set_all = m_set_all;

	// allow the user id to be set
	pDoc->m_alt_userid = m_alt_userid;

	return CPropertyPage::OnKillActive();
}

void General::OnEndbr() 

{
	// End the current browse operation
	// get the form data into the instance variables
	UpdateData(TRUE);

	// Read the message
	pDoc->endBrowse(false);

	// the getMessage routine will update the variables from the document
	// by calling the updateAllViews routine, which will call the OnUpdate routine

	// update the form data from the instance variables
	UpdateData(FALSE);

	// refresh the variables on the screen
	UpdatePageData();
}

void General::OnBrnext() 

{
	// Browse the next message in the queue
	// get the form data into the instance variables
	UpdateData(TRUE);

	ASSERT(pDoc != NULL);

	// set the get options
	pDoc->m_all_avail = m_all_avail;
	pDoc->m_logical_order = m_logical_order;

	// set a wait cursor
	((CRfhutilApp *)AfxGetApp())->BeginWaitCursor();

	// Read the message
	pDoc->browseNext(false);

	// end the wait cursor
	((CRfhutilApp *)AfxGetApp())->EndWaitCursor();

	// turn off the set User id flag, since the user id may have changed
	m_set_UserID = FALSE;
	pDoc->m_setUserID = FALSE;
	pDoc->m_convert = m_convert;
	pDoc->m_complete_msg = m_complete_msg;

	// the getMessage routine will update the variables from the document
	// by calling the updateAllViews routine, which will call the OnUpdate routine

	// update the form data from the instance variables
	UpdateData(FALSE);

	// refresh the variables on the screen
	UpdatePageData();
}

void General::OnBrprev() 

{
	// Move to the previous message
	// get the form data into the instance variables
	UpdateData(TRUE);
	
	// set the get options
	pDoc->m_all_avail = m_all_avail;
	pDoc->m_logical_order = m_logical_order;
	pDoc->m_convert = m_convert;
	pDoc->m_complete_msg = m_complete_msg;

	// set a wait cursor
	((CRfhutilApp *)AfxGetApp())->BeginWaitCursor();

	// Read the message
	pDoc->browsePrev();

	// end the wait cursor
	((CRfhutilApp *)AfxGetApp())->EndWaitCursor();

	// turn off the set User id flag, since the user id may have changed
	m_set_UserID = FALSE;
	pDoc->m_setUserID = FALSE;

	// update the form data from the instance variables
	UpdateData(FALSE);

	// refresh the variables on the screen
	UpdatePageData();
}

void General::OnStartbr() 

{
	// Start a browse operation on the current queue
	int		rc;

	// get the form data into the instance variables
	UpdateData(TRUE);

	// set the get options
	pDoc->m_all_avail = m_all_avail;
	pDoc->m_logical_order = m_logical_order;
	pDoc->m_get_by_msgid = m_get_by_msgid;
	pDoc->m_get_by_correlid = m_get_by_correlid;
	pDoc->m_get_by_groupid = m_get_by_groupid;
	pDoc->m_convert = m_convert;
	pDoc->m_complete_msg = m_complete_msg;
	pDoc->m_mq_props = m_mq_props;

	// set the selector
	pDoc->m_filter = m_filter;

	// Read the message
	rc = pDoc->startBrowse((LPCTSTR)m_qm_name, (LPCTSTR)m_q_name, false, -1);

	if (MQCC_OK == rc)
	{
		// Clear the file name, since we read the data from a queue
		pDoc->fileName[0] = 0;

		// set the windows title to the queue name
		if (((CRfhutilApp *)AfxGetApp())->m_mainWnd != NULL)
		{
			((CRfhutilApp *)AfxGetApp())->m_mainWnd->SetWindowText(m_q_name);
		}
	}
	
	// update the form data from the instance variables
	UpdateData(FALSE);

	// refresh the variables on the screen
	UpdatePageData();
}

void General::OnBindAsQ() 

{
	// Set the bind open option to as queue defined
	// Update the control variables
	UpdateData (TRUE);

	// Update the bind options in the dataarea object
	pDoc->m_bind_option = m_cluster_bind;

	// Update the controls
	UpdateData (FALSE);
}

void General::OnBindNotFixed() 

{
	// set the bind open option to bind not fixed
	// Update the control variables
	UpdateData (TRUE);

	// Update the bind options in the dataarea object
	pDoc->m_bind_option = m_cluster_bind;

	// Update the controls
	UpdateData (FALSE);
}

void General::OnBindOnOpen() 

{
	// Set the bind open option to bind on open
	// Update the control variables
	UpdateData (TRUE);

	// Update the bind options in the dataarea object
	pDoc->m_bind_option = m_cluster_bind;

	// Update the controls
	UpdateData (FALSE);
}

void General::OnPurgeQ() 

{
	int			rc;
	int			msgCount;
	char		msg[256];

	// Delete all the messages from the queue by reading them and discarding the results
	// Update the control variables, to make sure we have the queue name and QM names correctly
	UpdateData (TRUE);

	// check if there is a queue name
	if (m_q_name.GetLength() == 0)
	{
		// a queue name is required - issue error message and return
		pDoc->m_error_msg = "*Queue Name required";
		pDoc->updateMsgText();
		m_errmsg = pDoc->m_msg_text;

		// update the message display
		UpdateData(FALSE);
		return;
	}

	// update the instance variables from the controls
	UpdateData (TRUE);

	// get the number of messages in the queue
	msgCount = pDoc->getQueueDepth((LPCTSTR)m_qm_name, (LPCTSTR)m_q_name);

	// build the warning message
	sprintf(msg, "Purge all messages from queue %s?", (LPCTSTR)m_q_name);

	// Issue warning dialog box
	rc = AfxMessageBox(msg, MB_YESNO|MB_APPLMODAL|MB_ICONQUESTION);

	// check results
	if (IDYES == rc)
	{
		// make sure the message filter field is current in the dataarea object
		pDoc->m_filter = m_filter;

		CEdit	*cedit=(CEdit *)GetDlgItem(IDC_Q_DEPTH);

		// purge the queue
		((CRfhutilApp *)AfxGetApp())->BeginWaitCursor();
		pDoc->purgeQueue((LPCTSTR)m_qm_name, (LPCTSTR)m_q_name, cedit);
		((CRfhutilApp *)AfxGetApp())->EndWaitCursor();
	
		// set the windows title to the queue name
		if (((CRfhutilApp *)AfxGetApp())->m_mainWnd != NULL)
		{
			((CRfhutilApp *)AfxGetApp())->m_mainWnd->SetWindowText(m_q_name);
		}
	}

	// turn off the set User id flag, since the user id may have changed
	m_set_UserID = FALSE;
	pDoc->m_setUserID = FALSE;

	// Update the controls
	UpdateData (FALSE);

	// refresh the variables on the screen
	UpdatePageData();
}

//////////////////////////////////////////////////
//
// Routine to load the queue manager combo box
// with the names of all the queue managers that
// are currently defined on this system.
//
// This routine will compile differently in the
// server and client versions.
//
//////////////////////////////////////////////////

void General::loadEditBox()

{
	// populate the drop down list of queue manager names
	// this routine is located in the main application class
	// since it is also used in the pubsub class
	(pDoc)->loadQMComboBox((CComboBox *)&m_qmComboBox);
}

void General::OnDropdownQm() 

{
	// Load the list of known queue managers into the 
	// drop down list
	loadEditBox();	
}

//
// Called whenever a dropdown box is cancelled
// The previous value of the edit box is restored
//

void General::OnSelendcancelQm() 

{
	// restore the previous setting of the combo box
	((CComboBox *)GetDlgItem(IDC_QM))->SetWindowText(m_qm_name);
}

//
// Called whenever a change is made to the data in the edit box using the keyboard or paste
//

void General::OnEditchangeQm() 

{
	// Update the control variables, to make sure we have the queue name correctly
	UpdateData (TRUE);

	// remember the new setting of the combo box
	((CComboBox *)GetDlgItem(IDC_QM))->GetWindowText(m_qm_name);

	// save the QM name in the document so the Show Cluster menu works in RFHUtilView
	pDoc->m_QM_name = m_qm_name;

	// Update the controls
	UpdateData (FALSE);
}

//
// Called whenever a selection is made using the dropdown list
//

void General::OnSelchangeQm() 

{
	// Queue manager name is about to be changed by the user
	int		index;
	int		len;
	char	qmName[256];

	UpdateData (TRUE);

	index = ((CComboBox *)GetDlgItem(IDC_QM))->GetCurSel();
	qmName[0] = 0;
	len = ((CComboBox *)GetDlgItem(IDC_QM))->GetLBText(index, qmName);
	if (len > 0)
	{
		m_qm_name = qmName;
	}
	else
	{
		m_qm_name.Empty();
	}

	// set the value in the edit box
	((CComboBox *)GetDlgItem(IDC_QM))->SetWindowText(m_qm_name);

	// save the QM name in the document so the Show Cluster menu works in RFHUtilView
	pDoc->m_QM_name = m_qm_name;

	// Update the controls
	UpdateData (FALSE);
}

void General::OnExitButton()

{
	// Exit button has been pushed
//	AfxGetApp()->ExitInstance();
	pDoc->ClearData();
	AfxGetApp()->CloseAllDocuments(TRUE);
	PostQuitMessage(0);
}

BOOL General::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

BOOL General::OnInitDialog() 

{
#ifdef MQCLIENT
	const char *	keyFile;
#endif

	CWnd		*wnd;
	CRfhutilApp * app=NULL;				// pointer to main application object
	int			menuHeight;
	int			borderWidth;
	int			mqrc;
	bool		moveWindow=false;
	CRect		rect, rect2, rectw;
	char		traceInfo[512];			// work variable to build trace message

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// load the mqm dll and resolve the MQ pointers
	mqrc = pDoc->loadMQdll();

	// check if user properties are supported
	if (pDoc->propertiesSupported)
	{
		// get the form data into the instance variables
		UpdateData(TRUE);

		// select the radio button for user properties
		m_mq_props = MQ_PROPS_AS_QUEUE;

		// update the form data from the instance variables
		UpdateData(FALSE);
	}

	// set the other radio buttons to initial values
	m_close_options = Q_CLOSE_NONE;
	m_cluster_bind = MQ_PROPS_AS_QUEUE;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering General::OnInitDialog() pDoc->propertiesSupported=%d m_mq_props=%d", pDoc->propertiesSupported, m_mq_props);

		// trace entry to OnInitDialog
		pDoc->logTraceEntry(traceInfo);
	}

	// run the method in the parent class first
	CPropertyPage::OnInitDialog();

	// get a pointer to the main window
	wnd = ((CRfhutilApp *)AfxGetApp())->m_mainWnd;

	// see if we can get the main frame window dimensions
	wnd->GetWindowRect(&rectw);
	wnd->GetClientRect(&rect);

	// get the dialog dimensions
	this->GetClientRect(&rect2);

	// calculate the height of the menus and borders
	// height of menus = height of window area - height of client area
	// border width = width of window area - width of client area
	menuHeight = (rectw.bottom - rectw.top) - (rect.bottom - rect.top);
	borderWidth = (rectw.right - rectw.left) - (rect.right - rect.left);

	// change the bottom and right dimensions to match the size of the dialog
	// new bottom = position of top of window + height of dialog + menu height
	// new right = position of left side + width of dialog + width of border
	rectw.bottom = rectw.top + rect2.bottom + menuHeight + borderWidth;
	rectw.right = rectw.left + rect2.right + borderWidth;

	// change the size of the frame window to match the actual size of the dialog
	wnd->MoveWindow(&rectw, TRUE);

	// set the window style
	ModifyStyle(0, (DWORD)WS_TABSTOP | WS_GROUP);

	LONG style2 = wnd->GetStyle();

	// get the window style
	LONG style = GetWindowLong(wnd->m_hWnd, GWL_STYLE);
	style |= WS_TABSTOP | WS_GROUP;
	SetWindowLong(wnd->m_hWnd, GWL_STYLE, style);

	// Enable tool tips support
	EnableToolTips(TRUE);
	
	// use the special MyComboBox subclass for the queue manager and queue name combo boxes
	m_qmComboBox.SubclassDlgItem(IDC_QM, this);
	m_q_nameComboBox.SubclassDlgItem(IDC_Q_NAME, this);

	// load the initial queue manager and queue names into the combo box
	// this is necessary for auto completion to work
	loadEditBox();
	OnDropdownQName();

	// get the initial values in the instance variables including capturing any
	// command line parameters to initialize the queue manager and queue names
	m_qm_name = app->initQMname;
	m_q_name = app->initQname;
	m_remote_qm = app->initRemoteQMname;

	// set the queue manager name in the dataarea to match the initial setting of the combo box
	// this avoids a problem if the user selects the view cluster queues menu item before any
	// other activity
	pDoc->m_QM_name = app->initQMname;
	pDoc->m_Q_name = app->initQname;
	pDoc->m_remote_QM = app->initRemoteQMname;

	// set the initial values in the combo boxes
	m_qmComboBox.SetWindowText((LPCTSTR)m_qm_name);
	m_q_nameComboBox.SetWindowText((LPCTSTR)m_q_name);

	// is MQ installed?
	if (!pDoc->checkForMQ())
	{
		// set an error message if MQ is not installed
#ifdef MQCLIENT
		pDoc->m_error_msg = "*****MQ client not installed";
#else
		pDoc->m_error_msg = "*****MQ not installed";
#endif

		// set the text in the edit box
		((CEdit *)GetDlgItem(IDC_ERRMSG))->SetWindowText((LPCTSTR)pDoc->m_error_msg);

		// update the error messages in the data object
		pDoc->updateMsgText();
	}

	// set a default code page for files
	m_file_codepage.Format("%d",DEF_ASCII_CCSID);
	
	// set the initial value of the code page field
	((CEdit *)GetDlgItem(IDC_MAIN_FILE_CODEPAGE))->SetWindowText((LPCTSTR)m_file_codepage);

	// get the connection user id and password that were last used from the registry
	pDoc->m_conn_userid = ((CRfhutilApp *)AfxGetApp())->initConnUser;
	pDoc->m_conn_password = ((CRfhutilApp *)AfxGetApp())->initConnPW;

#ifdef MQCLIENT

	// get the SSL values that were saved in the registry
	pDoc->m_use_ssl = ((CRfhutilApp *)AfxGetApp())->initUseSSL;
	pDoc->m_ssl_cipher = ((CRfhutilApp *)AfxGetApp())->initSSLCipherSpec;
	pDoc->m_ssl_keyr = ((CRfhutilApp *)AfxGetApp())->initSSLKeyR;
	pDoc->m_ssl_validate = ((CRfhutilApp *)AfxGetApp())->initSSLValidateClient;
	pDoc->m_ssl_reset_count = ((CRfhutilApp *)AfxGetApp())->initSSLResetCount;
	pDoc->m_security_exit = ((CRfhutilApp *)AfxGetApp())->initSecExit;
	pDoc->m_security_data = ((CRfhutilApp *)AfxGetApp())->initSecData;

	// do we have a saved value?
	if (pDoc->m_ssl_keyr.GetLength() == 0)
	{
		// try to get the MQSSLKEYR environment variable
		keyFile = getenv(MQSSLKEYR);
		if (keyFile != NULL)
		{
			// remember the location of the ssl key ring file
			pDoc->m_ssl_keyr = keyFile;
		}
	}
#endif
	
#ifdef SAFEMODE
	((CButton *)GetDlgItem(IDC_READQ))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_READQ))->ShowWindow(SW_HIDE);
	((CButton *)GetDlgItem(IDC_PURGE))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PURGE))->ShowWindow(SW_HIDE);
#endif

	// set the check box for the new message id option
	((CButton *)GetDlgItem(IDC_NEW_MSG_ID))->SetCheck(TRUE);

	// send a custom message to switch the focus to the QM combo box
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	// check if user properties and selectors are supported
	if (!pDoc->propertiesSupported)
	{
		((CEdit *)GetDlgItem(IDC_MAIN_FILTER))->SetReadOnly(TRUE);			// do not allow any selectors
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting General::OnInitDialog() style=%d, style2=%d, initQMname=%s", style, style2, (LPCTSTR)app->initQMname);

		// trace entry to OnInitDialog
		pDoc->logTraceEntry(traceInfo);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL General::PreCreateWindow(CREATESTRUCT& cs) 

{
	// Adjust the window style 
	cs.style |= WS_TABSTOP;
	cs.dwExStyle |= WS_EX_CONTROLPARENT;

	return CPropertyPage::PreCreateWindow(cs);
}

BOOL General::PreTranslateMessage(MSG* pMsg) 

{
	//
	// This routine is necessary for the tab and backspace keys to work correctly
	// It is also used for the enter key to work in a combobox control that has been subclassed
	//
	CString txt;

	if (pMsg->message != WM_KEYDOWN)
		return CPropertyPage::PreTranslateMessage(pMsg);

	// check for a backspace key
	if (VK_BACK == pMsg->wParam)
	{
		CWnd * curFocus = GetFocus();
		if (curFocus != NULL)
		{
			int id = curFocus->GetDlgCtrlID();

			// check if this is an edit box control
			if (IDC_MAIN_FILTER == id)
			{
				processBackspace(curFocus);
				return TRUE;
			}
		}
	}

	// check for an enter key
	// this code allows a user to press the enter or escape keys 
	// to end the use of a combo box drop down list
	if ((WM_KEYDOWN == pMsg->message) && (VK_RETURN == pMsg->wParam))
	{
		HWND hQmgrCombo = ::GetDlgItem(m_hWnd, IDC_QM);
		HWND hQnameCombo = ::GetDlgItem(m_hWnd, IDC_Q_NAME);

		if ((hQmgrCombo != NULL) && (hQnameCombo != NULL))
		{
			HWND hQmgrEdit = ::GetDlgItem(hQmgrCombo, 0x03e9);
			HWND hQnameEdit = ::GetDlgItem(hQnameCombo, 0x03e9);

			if (pMsg->hwnd == hQmgrEdit)
			{
				// check if there is a selection in the list box
				CComboBox * combo = (CComboBox *)GetDlgItem(IDC_QM);

				// send a message to the ComboBox
				::SendMessage(hQmgrCombo, WM_KEYDOWN, VK_FINAL, 0);
			}

			if (pMsg->hwnd == hQnameEdit)
			{
				// capture the current value
				CComboBox * combo = (CComboBox *)GetDlgItem(IDC_Q_NAME);
				int sel = combo->GetCurSel();

				if (sel != LB_ERR)
				{
					// get the text from the control
					combo->GetLBText(sel, txt);

					if (txt.GetLength() > 0)
					{
						combo->SetWindowText((LPCTSTR) txt);
					}
				}

TRACE("Sending VK_FINAL to QName combobox sel=%d\n",sel);
				// send a message to the ComboBox
				::SendMessage(hQnameCombo, WM_KEYDOWN, VK_FINAL, 0);
			}
		}
	}

	if (IsDialogMessage(pMsg))
		return TRUE;
	else
		return CPropertyPage::PreTranslateMessage(pMsg);
}

void General::OnDropdownQName() 

{
	int			added=0;
	int			skipped=0;
	const char	*qNamePtr=NULL;
	char		traceInfo[512];		// work variable to build trace message

	// remove the current drop down list entries
	m_q_nameComboBox.ResetContent();

	// see if we can find a list of queue names for the queue manager
	if (m_qm_name.GetLength() > 0)
	{
		qNamePtr = pDoc->getQueueNamesListPtr((LPCTSTR)m_qm_name);
	}
	else
	{
		qNamePtr = pDoc->getQueueNamesListPtr((LPCTSTR)pDoc->defaultQMname);
	}

	if (qNamePtr != NULL)
	{
		while (qNamePtr[0] != 0)
		{
			if ((pDoc->m_show_system_queues) || (memcmp(qNamePtr + 1, "SYSTEM.", 7) != 0))
			{
				m_q_nameComboBox.AddString(qNamePtr + 1);
				added++;						// in case trace is turned on

				if (pDoc->traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " General::OnDropdownQName() adding=%s", qNamePtr);

					// trace entry to updateIdFields
					pDoc->logTraceEntry(traceInfo);
				}
			}
			else
			{
				skipped++;					// in case trace is turned on

				if (pDoc->traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " General::OnDropdownQName() ignoring=%s", qNamePtr);

					// trace entry to updateIdFields
					pDoc->logTraceEntry(traceInfo);
				}
			}

			qNamePtr = Names::getNextName(qNamePtr);
		}
	}
					
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting General::OnDropdownQName() added=%d skipped=%d qNamePtr=%8.8X", added, skipped, (unsigned int)qNamePtr);

		// trace entry to updateIdFields
		pDoc->logTraceEntry(traceInfo);
	}
}

void General::OnSelchangeQName() 

{
	int		index;
	int		len;
	char	qName[256];

	// A new Queue name is being selected by the user
	UpdateData (TRUE);
TRACE("General::OnSelchangeQName() %s\n", (LPCTSTR) m_q_name);

	// remember the new setting of the combo box
	index = ((CComboBox *)GetDlgItem(IDC_Q_NAME))->GetCurSel();
	len = ((CComboBox *)GetDlgItem(IDC_Q_NAME))->GetLBText(index, qName);
	if (len > 0)
	{
		m_q_name = qName;
	}
	else
	{
		m_q_name.Empty();
	}

	((CComboBox *)GetDlgItem(IDC_Q_NAME))->SetWindowText(m_q_name);

	// save the Q name in the document
	pDoc->m_Q_name = m_q_name;
TRACE("General::OnSelchangeQName() after %s\n", (LPCTSTR) m_q_name);

	// Update the controls
	UpdateData (FALSE);
}

void General::OnSelendcancelQName() 

{
	// restore the previous setting of the combo box
	((CComboBox *)GetDlgItem(IDC_Q_NAME))->SetWindowText(m_q_name);
}

void General::OnEditchangeQName()
 
{
	// Update the control variables, to make sure we have the queue name correctly
	UpdateData (TRUE);

TRACE("General::OnEditchangeQName() %s\n", (LPCTSTR) m_q_name);
	// remember the new setting of the combo box
	((CComboBox *)GetDlgItem(IDC_Q_NAME))->GetWindowText(m_q_name);

	// save the QM name in the document so the Show Cluster menu works in RFHUtilView
	pDoc->m_Q_name = m_q_name;

TRACE("General::OnEditchangeQName() after %s\n", (LPCTSTR) m_q_name);
	// Update the controls
	UpdateData (FALSE);
}

void General::OnCloseq() 

{
	// close the queue and disconnect from the QM
	pDoc->closeQ(m_close_options);	
	pDoc->discQM();

	// refresh the variables on the screen (to disable the close Q button)
	UpdatePageData();
}

void General::OnSetUserid() 

{
	// The set user id check box is being modified by the user
	if (m_set_UserID)
	{
		pDoc->m_setUserID = FALSE;
	}
	else
	{
		pDoc->m_setUserID = TRUE;
	}
}

void General::OnLoadQueueNames() 

{
	// The Load Names button has been pushed
	// Try to use PCF to try and get the queue names from the queue manager.
	int				rc;
	CRfhutilApp *	app;
	char			errtxt[256];
	char			traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering General::OnLoadQueueNames() m_qm_name=%s", (LPCTSTR)m_qm_name);

		// trace entry to OnPsGetSubs
		pDoc->logTraceEntry(traceInfo);
	}


	// initialize error variable
	memset(errtxt, 0, sizeof(errtxt));

	// make sure the instance variables are up to date
	UpdateData(TRUE);

	// set the cursor to an hourglass since this might take a while
	app = (CRfhutilApp *)AfxGetApp();
	app->BeginWaitCursor();

	// do we have a connection to a queue manager?
	if (pDoc->isConnectionActive())
	{
		// do we have a queue open?
		if (pDoc->isQueueOpen())
		{
			// close the current queue ignoring any close options (so queue will not be deleted)
			pDoc->closeQ(Q_CLOSE_NONE);
		}

		// disconnect from the current queue manager since we use a different connection to get the queue names
		pDoc->discQM();
	}

	// connect to the specified queue manager and get a list of queue names
	rc = pDoc->getQueueNames((LPCTSTR)m_qm_name);

	// update the status of the closeQ button
	((CButton *)GetDlgItem(IDC_CLOSEQ))->EnableWindow(pDoc->isConnectionActive());

	// check if we had an error
	if (rc != MQRC_NONE)
	{
		// did we get an error message back?
		if (errtxt[0] != 0)
		{
			pDoc->m_error_msg = errtxt;
			pDoc->m_error_msg += "\r\n";
		}

		// check if there is already a message
		if (pDoc->m_error_msg.GetLength() > 0)
		{
			// already an MQ error message - insert a CRLF sequeunce to force this text onto the next line
			pDoc->m_error_msg += "\r\n";
		}

		// report the error
		pDoc->m_error_msg += "Error getting queue names";
		pDoc->updateMsgText();
	}

	// refresh the variables on the screen (to display any error messages)
	UpdatePageData();

	// done with the wait cursor
	((CRfhutilApp *)AfxGetApp())->EndWaitCursor();
}

void General::OnSetfocusQm() 

{
	// the focus is moving to the Queue Manager dialog box
	// The maximum size of the text must be set depending
	// on whether this is a client implementation or
	// normal server bindings are used.  If server bindings
	// are used then the name of the queue manager is limited
	// to 48 characters.  Otherwise the channel/TCP/hostname
	// combination can be up to 255 characters in length
#ifndef MQCLIENT
	//((CComboBox *)GetDlgItem(IDC_QM))->LimitText(MQ_Q_NAME_LENGTH);
	m_qmComboBox.LimitText(MQ_Q_NAME_LENGTH);
#endif
}

void General::OnSetAll() 
{
	// The Set all context check box selection has been changed by the user
	if (m_set_all)
	{
		pDoc->m_set_all = FALSE;
	}
	else
	{
		pDoc->m_set_all = TRUE;
	}
}

void General::OnAltUserid() 

{
	// The alternate user id check box has been changed by the user
	if (m_alt_userid)
	{
		pDoc->m_alt_userid = FALSE;
	}
	else
	{
		pDoc->m_alt_userid = TRUE;
	}
}

//////////////////////////////////////////////////////////////////////
//
// The Display Q button has been pushed by the user
// This will read the messages in the queue and display some summary
// information about each message in the queue in a dialog
// The user can select a message and then choose to read, browse 
// or start a browse operation from the selected message.
//
//////////////////////////////////////////////////////////////////////

void General::OnMainDisplayq() 

{
	int			ret;
	int			line;
	int			readType;
	BOOL		saveReadMsgId;
	BOOL		saveReadCorrelId;
	BOOL		saveReadGroupId;

	// Update the control variables, to make sure we have the queue name correctly
	UpdateData (TRUE);

	// check if there is a queue name
	if (m_q_name.GetLength() == 0)
	{
		// a queue name is required - issue error message and return
		pDoc->m_error_msg = "*Queue Name required";
		pDoc->updateMsgText();
		m_errmsg = pDoc->m_msg_text;

		// update the message display
		UpdateData(FALSE);
		return;
	}

	// set the selector
	pDoc->m_filter = m_filter;

	CDispQ		dq(this);

	// set some global variables in the dialog before executing it
	dq.m_qmgr_name = (LPCTSTR)m_qm_name;
	dq.m_q_name = (LPCTSTR)m_q_name;
	dq.pDoc = pDoc;

	// Read the messages - this may take a while
	((CRfhutilApp *)AfxGetApp())->BeginWaitCursor();
	ret = dq.DoModal();
	((CRfhutilApp *)AfxGetApp())->EndWaitCursor();

	if (IDOK == ret)
	{
		// get the selected line
		//line = dq.m_selectedLine;
		line = dq.m_sel;

		// was there a selection?
		if (line >= 0)
		{
			// set the type of read
			readType = dq.m_read_type;

			// set the message id of the message to be read
			pDoc->setMsgId((char *)dq.m_msg_id);

			// remember the current settings of the read by msg id and correl id
			saveReadMsgId = pDoc->m_get_by_msgid;
			saveReadCorrelId = pDoc->m_get_by_correlid;
			saveReadGroupId = pDoc->m_get_by_groupid;

			// set the read to use msg id
			pDoc->m_get_by_msgid = TRUE;
			pDoc->m_get_by_correlid = FALSE;
			pDoc->m_get_by_groupid = FALSE;

			// set other read options
			pDoc->m_complete_msg = m_complete_msg;
			pDoc->m_all_avail = m_all_avail;
			pDoc->m_set_all = m_set_all;
			pDoc->m_setUserID = m_set_UserID;
			pDoc->m_convert = m_convert;
			pDoc->m_logical_order = m_logical_order;
			pDoc->m_alt_userid = m_alt_userid;
			pDoc->m_mq_props = m_mq_props;

			((CRfhutilApp *)AfxGetApp())->BeginWaitCursor();

			if (STARTBR == readType)
			{
				// start the browse, using the saved message id
				pDoc->startBrowse((LPCTSTR)m_qm_name, (LPCTSTR)m_q_name, false, line + 1);
				pDoc->setBrowsePrevMsg((char *)dq.m_prev_msg_id);
			}
			else
			{
				// Read the message
				pDoc->getMessage((LPCTSTR)m_qm_name, (LPCTSTR)m_q_name, readType);
			}

			((CRfhutilApp *)AfxGetApp())->EndWaitCursor();

			//restore the msg id and correl id settings
			pDoc->m_get_by_msgid = saveReadMsgId;
			pDoc->m_get_by_correlid = saveReadCorrelId;
			pDoc->m_get_by_groupid = saveReadGroupId;
		}
	}

	UpdatePageData();
}

///////////////////////////////////////////////////////////////////
//
// This routine will save one or more messages to a file or files
//
// A dialog is presented to allow the user to choose whatever
// options they want.  If OK is then pressed, the messages will
// either be saved in a single file with delimiters or in 
// individual files
//
///////////////////////////////////////////////////////////////////

void General::OnMainSaveq() 

{
	int				rc;
	int				delimiter_type;
	int				filesType=0;
	int				maxFileSize=(1024 * 1024 * 1024 - 1) + (1024 * 1024 * 1024);
	unsigned char	delimiter[64];
	unsigned char	propDelim[64];
	CString			fileName;
	CString			strMaxFileSize;
	CSavemsgs		dlg;
	SAVEPARMS		parms;

	// Update the control variables, to make sure we have the queue name correctly
	UpdateData (TRUE);

	// initialize the parms area
	memset(&parms, 0, sizeof(parms));
	parms.includeMQMD = false;
	parms.removeHdrs = false;
	parms.browseMsgs = true;
	parms.appendFile = false;

	// check if there is a queue name
	if (m_q_name.GetLength() == 0)
	{
		// a queue name is required - issue error message and return
		pDoc->m_error_msg = "*Queue Name required";
		pDoc->updateMsgText();
		m_errmsg = pDoc->m_msg_text;

		// update the message display
		UpdateData(FALSE);
		return;
	}

	// set the selector and user property handling
	pDoc->m_filter = m_filter;
	pDoc->m_mq_props = m_mq_props;

	// initialize some dialog fields from current selections
	dlg.m_incl_headers = !pDoc->m_dataonly;
	dlg.m_incl_mqmd = pDoc->m_write_include_MQMD;
	dlg.m_q_name = (LPCTSTR)m_q_name;

	// run the dialog
	rc = dlg.DoModal();

	// did the user press the OK button
	if (IDOK == rc)
	{
		// extract the data from the dialog
		fileName = dlg.m_filename;

		// make sure a file name was entered
		if (fileName.GetLength() > 0)
		{
			parms.startCount = atoi(dlg.m_start_count);
			parms.endCount = atoi(dlg.m_end_count);
			parms.filesType = dlg.m_files_type;

			if (dlg.m_incl_mqmd)
			{
				parms.includeMQMD = true;
			}

#ifndef SAFEMODE
			// Are the messages to be removed from the queue
			if (SAVE_REMOVE_MSGS_YES == dlg.m_remove)
			{
				// read the messsages rather than browsing the queue
				parms.browseMsgs = false;
			}
#endif

			// understand if MQ headers are to be stripped before messages are saved
			parms.removeHdrs = (dlg.m_incl_headers == FALSE);

			// get the file append option from the dialog
			parms.appendFile = (dlg.m_append_file == TRUE);

			// get the delimiter type (HEX or ASCII)
			delimiter_type = dlg.m_delimiter_type;

			// clear the delimiter area
			memset(delimiter, 0, sizeof(delimiter));

			// get the length of the delimiter
			parms.delimLen = dlg.m_delimiter.GetLength();

			// get the delimiter, if there is one
			if (parms.delimLen > 0)
			{
				// what type of delimiter do we have?
				if (SAVE_DELIM_HEX == delimiter_type)
				{
					// delimiter is specified as hex
					parms.delimLen >>= 1;
					HexToAscii((unsigned char *)((LPCTSTR)dlg.m_delimiter), (parms.delimLen << 1), delimiter);
				}
				else
				{
					strcpy((char *)delimiter, dlg.m_delimiter);
				}
			}

			// check for a properties delimiter
			memset(propDelim, 0, sizeof(propDelim));

			// get the length of the delimiter
			parms.propDelimLen = dlg.m_prop_delim.GetLength();

			// get the properties delimiter if there is one
			if (parms.propDelimLen > 0)
			{
				// what type of delimiter do we have?
				if (SAVE_DELIM_HEX == delimiter_type)
				{
					// delimiter is specified as hex
					parms.propDelimLen >>= 1;
					HexToAscii((unsigned char *)((LPCTSTR)dlg.m_prop_delim), (parms.propDelimLen << 1), propDelim);
				}
				else
				{
					strcpy((char *)propDelim, dlg.m_prop_delim);
				}
			}

			// get the maximum file size parameter
			strMaxFileSize = dlg.m_savemsgs_maxFileSize;

			// check the possiblities for the size
			if (strMaxFileSize.Compare("128M") == 0)
			{
				// set maxFileSize to 128M
				maxFileSize = 128 * 1024 * 1024;
			} 
			else if (strMaxFileSize.Compare("256M") == 0)
			{
				// set maxFileSize to 256M
				maxFileSize = 256 * 1024 * 1024;
			} 
			else if (strMaxFileSize.Compare("512M") == 0)
			{
				// set maxFileSize to 512M
				maxFileSize = 512 * 1024 * 1024;
			}
			else if (strMaxFileSize.Compare("1G") == 0)
			{
				// set maxFileSize to 1G
				maxFileSize = 1024 * 1024 * 1024;
			}
			else if (strMaxFileSize.Compare("1.5G") == 0)
			{
				// set maxFileSize to 1.5G
				maxFileSize = 1536 * 1024 * 1024;
			}
			else
			{
				// set maxFileSize to default of 2G - 1
				maxFileSize = (1024 * 1024 * 1024 - 1) + (1024 * 1024 * 1024);
			}

			// set the parameter
			parms.maxfileSize = maxFileSize;

			// make sure the instance variables are up to date
			UpdateData(TRUE);

			// set the get options in the dataarea object
			pDoc->m_all_avail = m_all_avail;
			pDoc->m_logical_order = m_logical_order;
			pDoc->m_get_by_msgid = m_get_by_msgid;
			pDoc->m_get_by_correlid = m_get_by_correlid;
			pDoc->m_get_by_groupid = m_get_by_groupid;
			pDoc->m_mq_props = m_mq_props;

			// populate the parameters area
			parms.fileName = (LPCTSTR)fileName;
			parms.qmName = (LPCTSTR)m_qm_name;
			parms.qName = (LPCTSTR)m_q_name;
			parms.delimiter = (char *)&delimiter;
			parms.propDelim = (char *)&propDelim;

			// display an hourglass cursor (busy)
			((CRfhutilApp *)AfxGetApp())->BeginWaitCursor();

			// save the messages from the queue into one or more files
			pDoc->saveMsgs(&parms); 

			// done with the busy cursor
			((CRfhutilApp *)AfxGetApp())->EndWaitCursor();
		}
		else
		{
			pDoc->m_error_msg = "File name required";
		}
	}

	UpdatePageData();
}

/////////////////////////////////////////////////
//
// This routine reads messages from one or more
// files and puts them to the selected queue.
//
/////////////////////////////////////////////////

void General::OnMainLoadq() 

{
	int				rc;
	int				delimiter_type;
	unsigned char	delimiter[64];
	unsigned char	propDelim[64];
	char			format[MQ_FORMAT_LENGTH + 8];
	CString			fileName;
	CLoadQ			dlg;
	LOADPARMS		parms;

	// Update the control variables, to make sure we have the queue name correctly
	UpdateData (TRUE);

	// initialize the parms area
	memset(&parms, 0, sizeof(parms));
	parms.removeMQMD = false;
	parms.removeHdrs = false;
	parms.persistent = false;
	parms.useSetAll = false;
	parms.newMsgId = false;
	parms.ignoreProps = false;
	parms.writeOnce = false;

	// check if there is a queue name
	if (m_q_name.GetLength() == 0)
	{
		// a queue name is required - issue error message and return
		pDoc->m_error_msg = "*Queue Name required";
		pDoc->updateMsgText();
		m_errmsg = pDoc->m_msg_text;

		// update the message display
		UpdateData(FALSE);
		return;
	}

	// initialize some dialog fields from current selections
	dlg.m_remove_hdrs = FALSE;
	dlg.m_remove_mqmd = FALSE;
	dlg.m_q_name = (LPCTSTR)m_q_name;

	// run the dialog
	rc = dlg.DoModal();

	// did the user press the OK button
	if (IDOK == rc)
	{
		// extract the data from the dialog
		fileName = dlg.m_filename;

		if (fileName.GetLength() > 0)
		{
			parms.maxCount = atoi((LPCTSTR)dlg.m_max_count);
			parms.startFileNumber = atoi((LPCTSTR)dlg.m_start_msg);
			parms.filesType = dlg.m_files_type;
			parms.defaultCcsid = atoi(dlg.m_ccsid);
			parms.defaultEncoding = atoi(dlg.m_encoding);

			if (dlg.m_remove_mqmd)
			{
				parms.removeMQMD = true;
			}

			if (dlg.m_remove_hdrs)
			{
				parms.removeHdrs = true;
			}

			if (dlg.m_use_set_all)
			{
				parms.useSetAll = true;
			}

			if (dlg.m_new_msg_id)
			{
				parms.newMsgId = true;
			}

			if (dlg.m_write_once)
			{
				parms.writeOnce = true;
			}

			if (dlg.m_single_file)
			{
				parms.singleFile = true;
			}

			if (dlg.m_loadq_ignore_props)
			{
				parms.ignoreProps = true;
			}

			// was a batchSize specified?
			if (dlg.m_batchSize.GetLength() > 0)
			{
				// get the batch size
				parms.batchSize = atoi((LPCTSTR)dlg.m_batchSize);
			}
			else
			{
				// default to one
				parms.batchSize = 0;
			}

			// make sure the batch size is at least one
			if (parms.batchSize < 0)
			{
				// force the batch size to zero
				parms.batchSize = 0;
			}

			// get the wait time between batches in milliseconds
			if (dlg.m_waitTime.GetLength() > 0)
			{
				// set the wait time in milliseconds between batches
				parms.waitTime = atoi((LPCTSTR)dlg.m_waitTime);
			}

			// make sure the wait time is > 0
			if (parms.waitTime < 0)
			{
				// force wait time to 0
				parms.waitTime = 0;
			}

			// get the default values
			if (dlg.m_persistent)
			{
				parms.persistent = true;
			}

			memset(format, ' ', MQ_FORMAT_LENGTH);
			format[MQ_FORMAT_LENGTH] = 0;
			if (dlg.m_format.GetLength() > 0)
			{
				// copy the specified format
				memcpy(format, (LPCTSTR)dlg.m_format, dlg.m_format.GetLength());
			}

			// get the type of delimiter (ASCII or HEX)
			delimiter_type = dlg.m_delimiter_type;

			// clear the delimiter area
			memset(delimiter, 0, sizeof(delimiter));

			// get the length of the delimiter
			parms.delimLen = dlg.m_delimiter.GetLength();

			if (parms.delimLen > 0)
			{
				// what type of delimiter do we have?
				if (SAVE_DELIM_HEX == delimiter_type)
				{
					// delimiter is specified as hex
					parms.delimLen >>= 1;
					HexToAscii((unsigned char *)((LPCTSTR)dlg.m_delimiter), (parms.delimLen << 1), delimiter);
				}
				else
				{
					strcpy((char *)delimiter, (LPCTSTR)(dlg.m_delimiter));
				}
			}

			// get the length of the properties delimiter
			parms.propDelimLen = dlg.m_loadq_prop_delim.GetLength();

			// clear the properties delimiter area
			memset(propDelim, 0, sizeof(propDelim));

			if (parms.propDelimLen > 0)
			{
				// what type of delimiter do we have?
				if (SAVE_DELIM_HEX == delimiter_type)
				{
					// delimiter is specified as hex
					parms.propDelimLen >>= 1;
					HexToAscii((unsigned char *)((LPCTSTR)dlg.m_loadq_prop_delim), (parms.propDelimLen << 1), propDelim);
				}
				else
				{
					strcpy((char *)propDelim, (LPCTSTR)(dlg.m_loadq_prop_delim));
				}
			}
		}
		else
		{
			pDoc->m_error_msg = "File name required";
		}

		UpdateData(TRUE);

		// make sure the get options and the queue and queue manager names are up to date
		pDoc->m_all_avail = m_all_avail;
		pDoc->m_logical_order = m_logical_order;
		pDoc->m_get_by_msgid = m_get_by_msgid;
		pDoc->m_get_by_correlid = m_get_by_correlid;
		pDoc->m_get_by_groupid = m_get_by_groupid;
		pDoc->m_mq_props = m_mq_props;

		// populate the parameters area
		parms.fileName = (LPCTSTR)fileName;
		parms.qmName = (LPCTSTR)m_qm_name;
		parms.RemoteQM = (LPCTSTR)m_remote_qm;
		parms.qName = (LPCTSTR)m_q_name;
		parms.format = (const char *)&format;
		parms.delimiter = (char *)&delimiter;
		parms.propDelim = (char *)&propDelim;

		// Read and save the messages into files
		((CRfhutilApp *)AfxGetApp())->BeginWaitCursor();
		pDoc->loadMsgs(&parms);
		((CRfhutilApp *)AfxGetApp())->EndWaitCursor();
	}

	UpdatePageData();
}

int General::getMaxX()

{
	CRect	rect;

	// get the location of the exit button
	((CButton *)GetDlgItem(IDC_OK))->GetWindowRect(&rect);

	return rect.BottomRight().x;
}

int General::getMaxY()

{
	CRect	rect;

	// get the location of the exit button
	((CButton *)GetDlgItem(IDC_OK))->GetWindowRect(&rect);

	return rect.BottomRight().y;
}

void General::OnSetFont(CFont* pFont) 

{
	CRfhutilApp *app = (CRfhutilApp *)AfxGetApp();
	//this->SetFont(&(app->m_fixed_font), 0);
	CPropertyPage::OnSetFont(&(app->m_fixed_font));
}

void General::OnSetConnUser() 

{
	// Use a dialog box to get the user id and password to use
	// when connecting to a queue manager.
	int			rc;
	char		tempCount[16];
	CConnUser	dlg;
	char		traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, " General::OnSetConnUser() pDoc->m_conn_userid=%s", (LPCTSTR)pDoc->m_conn_userid);

		// trace entry to OnSetConnUser
		pDoc->logTraceEntry(traceInfo);
	}

	// set the current values in the dialog
	dlg.m_conn_userid = (LPCTSTR)pDoc->m_conn_userid;
	dlg.m_conn_password = (LPCTSTR)pDoc->m_conn_password;
	dlg.m_use_ssl = pDoc->m_use_ssl;
	dlg.m_ssl_validate_client = pDoc->m_ssl_validate;
	dlg.m_ssl_cipher = (LPCTSTR)pDoc->m_ssl_cipher;
	dlg.m_ssl_keyr = (LPCTSTR)pDoc->m_ssl_keyr;
	dlg.m_security_exit = (LPCTSTR)pDoc->m_security_exit;
	dlg.m_security_data = (LPCTSTR)pDoc->m_security_data;
	sprintf(tempCount, "%d", pDoc->m_ssl_reset_count);
	dlg.m_ssl_reset_count = tempCount;

	// run the dialog
	rc = dlg.DoModal();

	// did the user press the OK button
	if (IDOK == rc)
	{
		// extract the data from the dialog
		pDoc->m_conn_userid = (LPCTSTR)dlg.m_conn_userid;
		pDoc->m_conn_password = (LPCTSTR)dlg.m_conn_password;
		pDoc->m_ssl_cipher = (LPCTSTR)dlg.m_ssl_cipher;
		pDoc->m_ssl_validate = dlg.m_ssl_validate_client;
		pDoc->m_use_ssl = dlg.m_use_ssl;
		pDoc->m_ssl_keyr = (LPCTSTR)dlg.m_ssl_keyr;
		pDoc->m_security_exit = (LPCTSTR)dlg.m_security_exit;
		pDoc->m_security_data = (LPCTSTR)dlg.m_security_data;
		pDoc->m_ssl_reset_count = atoi(dlg.m_ssl_reset_count);
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, " General::OnSetConnUser() rc=%d pDoc->m_conn_userid=%s", rc, (LPCTSTR)pDoc->m_conn_userid);

		// trace exit from OnSetConnUser
		pDoc->logTraceEntry(traceInfo);
	}
}

//////////////////////////////////////////////////////////////////
//
// Routine to get the current setting of the queue manager name
// This routine is used by the connect menu item, which has to 
// get the current setting of the queue manager name
//
//////////////////////////////////////////////////////////////////

void General::getQMname(CString &qmName)

{
	UpdateData (TRUE);
	qmName = (LPCTSTR)m_qm_name;
}

//////////////////////////////////////////////////////////////////
//
// Routine to get the current setting of the queue  name
// This routine is used by the open queue menu items, which have
// to get the current setting of the queue name
//
//////////////////////////////////////////////////////////////////

void General::getQname(CString &qName)

{
	UpdateData (TRUE);
	qName = (LPCTSTR)m_q_name;
}

///////////////////////////////////////////////////////////////////////
//
// Routine to get the current setting of the remote queue manager name
// This routine is used by the open queue menu items, which have to 
// get the current setting of the remote queue manager name
//
///////////////////////////////////////////////////////////////////////

void General::getRemoteQM(CString &remoteQM)

{
	UpdateData (TRUE);
	remoteQM = (LPCTSTR)m_remote_qm;
}

void General::DisplayQ()

{
	OnMainDisplayq();
}

void General::BrowseQ()

{
	OnBrowse();
}

void General::PurgeQ()

{
	OnPurgeQ();
}

void General::LoadNames()

{
	OnLoadQueueNames();
}

void General::SetUserId()

{
	OnSetConnUser();
}

void General::ClearAll()

{
	OnClearAll();
}

void General::ClearData()

{
	OnClearData();
}

void General::CloseQ()

{
	OnCloseq();
}

void General::LoadQ()

{
	OnMainLoadq();
}

void General::ReadQ()

{
	OnReadq();
}

void General::SaveQ()

{
	OnMainSaveq();
}

void General::WriteQ()

{
	OnWriteq();
}

void General::StartBr()

{
	OnStartbr();
}

void General::EndBr()

{
	OnEndbr();
}

//////////////////////////////////////////////////////
//
// custom message handler to force the focus to the
// queue manager combo box control
//
//////////////////////////////////////////////////////

LONG General::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the queue manager name combo box
	//((CComboBox *)GetDlgItem(IDC_QM))->SetFocus();
	m_qmComboBox.SetFocus();

	return 0;
}

void General::OnNewCorrelId() 

{
	// The use new correlation check box selection has been changed by the user
	// capture the new setting
	pDoc->m_new_correl_id = ((CButton *)GetDlgItem(IDC_NEW_CORREL_ID))->GetCheck();
}

void General::OnNewMsgId() 

{
	// The use new correlation check box selection has been changed by the user
	// capture the new setting
	pDoc->m_new_msg_id = ((CButton *)GetDlgItem(IDC_NEW_MSG_ID))->GetCheck();
}

void General::OnOK() 
{
}

///////////////////////////////////////////////
//
// Routine to move messages from one queue to
// another queue.
//
///////////////////////////////////////////////

void General::OnMoveQ() 

{
	int				rc;
	int				maxCount=0;
	int				startMsg=1;
	CRfhutilApp *	app;
	MoveQ			dlg;
	char			traceInfo[512];		// work variable to build trace message

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();
	
	// get the form data into the instance variables
	UpdateData(TRUE);

	// check if there is a queue name
	if (m_q_name.GetLength() == 0)
	{
		// a queue name is required - issue error message and return
		pDoc->m_error_msg = "*Queue Name required";
		pDoc->updateMsgText();
		m_errmsg = pDoc->m_msg_text;

		// update the message display
		UpdateData(FALSE);
		return;
	}

	// set the selector and message properties handling options
	pDoc->m_filter = m_filter;
	pDoc->m_mq_props = m_mq_props;

	// initialize some dialog fields from current selections
	dlg.m_max_count = "0";
	dlg.m_remove_dlq = FALSE;
	dlg.m_start_msg = "1";
	dlg.m_pass_all = TRUE;
	dlg.m_new_queue_name = "";
	dlg.pDoc = pDoc;

	// run the dialog
	rc = dlg.DoModal();

	// did the user press the OK button
	if (IDOK == rc)
	{
		// extract the data from the dialog
		startMsg = atoi(dlg.m_start_msg);
		maxCount = atoi(dlg.m_max_count);

		// perform the move operation
		app->BeginWaitCursor();
		pDoc->moveMessages((LPCTSTR)m_qm_name, (LPCTSTR)m_q_name, (LPCTSTR)dlg.m_new_queue_name, startMsg, maxCount, dlg.m_remove_dlq, dlg.m_pass_all);
		app->EndWaitCursor();
	}

	// update the form data from the instance variables
	UpdateData(FALSE);
	
	// refresh the variables on the screen
	UpdatePageData();

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting General::OnMoveQ() rc=%d startMsg=%d maxCount=%d m_remove_dlq=%d, m_pass_all=%d added=%d skipped=%d m_new_queue_name=%s", rc, startMsg, maxCount, dlg.m_remove_dlq, dlg.m_pass_all, dlg.added, dlg.skipped, (LPCTSTR)dlg.m_new_queue_name);

		// trace entry to updateIdFields
		pDoc->logTraceEntry(traceInfo);
	}
}


void General::OnChangeMainFileCodepage() 

{
	// get the contents of the control into the instance variables
	UpdateData(TRUE);

	// Update the field in the DataArea object so the two fields stay in sync
	pDoc->m_file_codepage = atoi((LPCTSTR)m_file_codepage);
}

void General::OnKillfocusMainFileCodepage() 

{
	// get the contents of the control into the instance variables
	UpdateData(TRUE);

	// Update the field in the DataArea object so the two fields stay in sync
	pDoc->m_file_codepage = atoi((LPCTSTR)m_file_codepage);
}

void General::OnCloseNone() 

{
	// Set the close option to none
	// Update the control variables
	UpdateData (TRUE);

	// Update the close options in the dataarea object
	pDoc->m_close_option = m_close_options;

	// Update the controls
	UpdateData (FALSE);
}

void General::OnCloseDelete() 

{
	// Set the close option to delete
	// Update the control variables
	UpdateData (TRUE);

	// Update the close options in the dataarea object
	pDoc->m_close_option = m_close_options;

	// Update the controls
	UpdateData (FALSE);
	
}

void General::OnClosePurge() 

{
	// Set the close option to purge and delete
	// Update the control variables
	UpdateData (TRUE);

	// Update the close options in the dataarea object
	pDoc->m_close_option = m_close_options;

	// Update the controls
	UpdateData (FALSE);
}

void General::OnSelendokQm() 

{
	// need to load the queue names	
	OnDropdownQName();
}
