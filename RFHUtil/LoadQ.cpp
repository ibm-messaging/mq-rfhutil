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

// LoadQ.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "LoadQ.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEFAULT_DELIMITER	"#@#@#"
#define DEFAULT_PROP_DELIM	"$%$%$"

/////////////////////////////////////////////////////////////////////////////
// CLoadQ dialog


CLoadQ::CLoadQ(CWnd* pParent /*=NULL*/)
	: CDialog(CLoadQ::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoadQ)
	m_delimiter = _T("");
	m_filename = _T("");
	m_files_type = 0;
	m_remove_hdrs = FALSE;
	m_remove_mqmd = FALSE;
	m_max_count = _T("");
	m_delimiter_type = -1;
	m_format = _T("");
	m_persistent = FALSE;
	m_ccsid = _T("");
	m_encoding = _T("");
	m_use_set_all = FALSE;
	m_new_msg_id = FALSE;
	m_start_msg = _T("");
	m_loadq_prop_delim = _T("");
	m_loadq_ignore_props = FALSE;
	m_batchSize = _T("");
	m_waitTime = _T("");
	m_write_once = FALSE;
	m_single_file = FALSE;
	m_hAccel = NULL;
	//}}AFX_DATA_INIT

	m_use_set_all = TRUE;
	m_new_msg_id = TRUE;
	m_delimiter = DEFAULT_DELIMITER;
	m_delimiter_type = SAVE_DELIM_CHAR;
	m_loadq_prop_delim = DEFAULT_PROP_DELIM;

	// set default for batchsize of 1
	m_batchSize = _T("1");

	// set default to write once
	m_write_once = TRUE;
	memset(strTitle, 0, sizeof(strTitle));
}


void CLoadQ::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoadQ)
	DDX_Text(pDX, IDC_LOADQ_DELIMITER, m_delimiter);
	DDV_MaxChars(pDX, m_delimiter, 16);
	DDX_Text(pDX, IDC_LOADQ_FILENAME, m_filename);
	DDV_MaxChars(pDX, m_filename, 255);
	DDX_Radio(pDX, IDC_LOADQ_ONEFILE, m_files_type);
	DDX_Check(pDX, IDC_LOADQ_REM_HEADERS, m_remove_hdrs);
	DDX_Check(pDX, IDC_LOADQ_REMOVE_MQMD, m_remove_mqmd);
	DDX_Text(pDX, IDC_LOADQ_MAX_COUNT, m_max_count);
	DDV_MaxChars(pDX, m_max_count, 6);
	DDX_Radio(pDX, IDC_LOADQ_DELIM_CHAR, m_delimiter_type);
	DDX_Text(pDX, IDC_LOADQ_FORMAT, m_format);
	DDV_MaxChars(pDX, m_format, 8);
	DDX_Check(pDX, IDC_LOADQ_PERSISTENT, m_persistent);
	DDX_Text(pDX, IDC_LOADQ_CCSID, m_ccsid);
	DDV_MaxChars(pDX, m_ccsid, 5);
	DDX_Text(pDX, IDC_LOADQ_ENCODING, m_encoding);
	DDV_MaxChars(pDX, m_encoding, 4);
	DDX_Check(pDX, IDC_LOADQ_USE_SET_ALL, m_use_set_all);
	DDX_Check(pDX, IDC_LOADQ_NEW_MSGID, m_new_msg_id);
	DDX_Text(pDX, IDC_LOADQ_START_MSG, m_start_msg);
	DDV_MaxChars(pDX, m_start_msg, 6);
	DDX_Text(pDX, IDC_LOADQ_PROP_DELIM, m_loadq_prop_delim);
	DDV_MaxChars(pDX, m_loadq_prop_delim, 16);
	DDX_Check(pDX, IDC_LOADQ_IGNORE_PROPS, m_loadq_ignore_props);
	DDX_Text(pDX, IDC_LOADQ_BATCH_SIZE, m_batchSize);
	DDV_MaxChars(pDX, m_batchSize, 5);
	DDX_Text(pDX, IDC_LOADQ_WAIT_TIME, m_waitTime);
	DDV_MaxChars(pDX, m_waitTime, 6);
	DDX_Check(pDX, IDC_LOADQ_WRITE_ONCE, m_write_once);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_LOADQ_SINGLE_FILE, m_single_file);
}


BEGIN_MESSAGE_MAP(CLoadQ, CDialog)
	//{{AFX_MSG_MAP(CLoadQ)
	ON_BN_CLICKED(IDC_LOADQ_BROWSE, OnLoadqBrowse)
	ON_COMMAND(IDC_LOADQ_DELIM_CHAR_KEY, OnLoadqDelimCharKey)
	ON_UPDATE_COMMAND_UI(IDC_LOADQ_DELIM_CHAR_KEY, OnUpdateLoadqDelimCharKey)
	ON_COMMAND(IDC_LOADQ_DELIM_HEX_KEY, OnLoadqDelimHexKey)
	ON_UPDATE_COMMAND_UI(IDC_LOADQ_DELIM_HEX_KEY, OnUpdateLoadqDelimHexKey)
	ON_COMMAND(IDC_LOADQ_FILE_PER_MSG_KEY, OnLoadqFilePerMsgKey)
	ON_UPDATE_COMMAND_UI(IDC_LOADQ_FILE_PER_MSG_KEY, OnUpdateLoadqFilePerMsgKey)
	ON_COMMAND(IDC_LOADQ_NEW_MSGID_KEY, OnLoadqNewMsgidKey)
	ON_UPDATE_COMMAND_UI(IDC_LOADQ_NEW_MSGID_KEY, OnUpdateLoadqNewMsgidKey)
	ON_COMMAND(IDC_LOADQ_ONEFILE_KEY, OnLoadqOnefileKey)
	ON_UPDATE_COMMAND_UI(IDC_LOADQ_ONEFILE_KEY, OnUpdateLoadqOnefileKey)
	ON_COMMAND(IDC_LOADQ_PERSISTENT_KEY, OnLoadqPersistentKey)
	ON_UPDATE_COMMAND_UI(IDC_LOADQ_PERSISTENT_KEY, OnUpdateLoadqPersistentKey)
	ON_COMMAND(IDC_LOADQ_REM_HEADERS_KEY, OnLoadqRemHeadersKey)
	ON_UPDATE_COMMAND_UI(IDC_LOADQ_REM_HEADERS_KEY, OnUpdateLoadqRemHeadersKey)
	ON_COMMAND(IDC_LOADQ_USE_SET_ALL_KEY, OnLoadqUseSetAllKey)
	ON_UPDATE_COMMAND_UI(IDC_LOADQ_USE_SET_ALL_KEY, OnUpdateLoadqUseSetAllKey)
	ON_COMMAND(IDC_LOADQ_REMOVE_MQMD_KEY, OnLoadqRemoveMqmdKey)
	ON_UPDATE_COMMAND_UI(IDC_LOADQ_REMOVE_MQMD_KEY, OnUpdateLoadqRemoveMqmdKey)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CLoadQ::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CLoadQ::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoadQ message handlers

void CLoadQ::OnOK() 

{
	int delimErr;
	int	delimLen;

	// Update the control variables
	UpdateData (TRUE);

	// do some basic editing
	// was a file name specified?
	if (m_filename.GetLength() == 0)
	{
		SetDlgItemText(IDC_LOADQ_ERROR_MESSAGE, "File name is required");
		return;
	}

	// do we need to check the delimiter string?  It is ignored for separate files
	if ((SAVE_DELIM_HEX == m_delimiter_type) && (SAVE_FILE_SAME_FILE == m_files_type))
	{
		// validate if the delimiter is valid hex
		delimLen = m_delimiter.GetLength();
		delimErr = checkIfHex((LPCTSTR)m_delimiter, delimLen);

		// check that the delimiter string is an even number of valid characters
		if ((0 == delimErr) || ((delimLen & 1) > 0))
		{
			SetDlgItemText(IDC_LOADQ_ERROR_MESSAGE, "Invalid hex delimiter string");
			return;
		}

		// validate if the delimiter is valid hex
		delimLen = m_loadq_prop_delim.GetLength();
		delimErr = checkIfHex((LPCTSTR)m_loadq_prop_delim, delimLen);

		// check that the delimiter string is an even number of valid characters
		if ((0 == delimErr) || ((delimLen & 1) > 0))
		{
			SetDlgItemText(IDC_LOADQ_ERROR_MESSAGE, "Invalid hex properties delimiter string");
			return;
		}
	}

	// no errors - close the dialog box
	CDialog::OnOK();
}

////////////////////////////////////////////
//
// Use a standard file dialog to locate
// the file to load from.
//
////////////////////////////////////////////

void CLoadQ::OnLoadqBrowse() 

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

BOOL CLoadQ::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

BOOL CLoadQ::OnInitDialog()

{
	CDialog::OnInitDialog();
	
	// tool tips are provided for this dialog and must be initialized
	EnableToolTips(TRUE);

	// clear the error text
	SetDlgItemText(IDC_LOADQ_ERROR_MESSAGE, "   ");

	// load the accelerator key table for this dialog
	m_hAccel = ::LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_LOADQ_ACCELERATOR));

	// set the dialog title
	sprintf(strTitle, "Load messages to Queue %s from file(s)", (LPCTSTR)m_q_name);
	SetWindowText(strTitle);

	return TRUE;
}

BOOL CLoadQ::PreTranslateMessage(MSG* pMsg) 

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

void CLoadQ::OnLoadqDelimCharKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_delimiter_type = SAVE_DELIM_CHAR;

	// Update the controls
	UpdateData (FALSE);
}

void CLoadQ::OnUpdateLoadqDelimCharKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CLoadQ::OnLoadqDelimHexKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_delimiter_type = SAVE_DELIM_HEX;

	// Update the controls
	UpdateData (FALSE);
}

void CLoadQ::OnUpdateLoadqDelimHexKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CLoadQ::OnLoadqFilePerMsgKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_files_type = SAVE_FILE_DIFF_FILES;

	// Update the controls
	UpdateData (FALSE);
}

void CLoadQ::OnUpdateLoadqFilePerMsgKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CLoadQ::OnLoadqOnefileKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_files_type = SAVE_FILE_SAME_FILE;

	// Update the controls
	UpdateData (FALSE);
}

void CLoadQ::OnUpdateLoadqOnefileKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CLoadQ::OnLoadqNewMsgidKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_new_msg_id)
	{
		m_new_msg_id = FALSE;
	}
	else
	{
		m_new_msg_id = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CLoadQ::OnUpdateLoadqNewMsgidKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CLoadQ::OnLoadqPersistentKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_persistent)
	{
		m_persistent = FALSE;
	}
	else
	{
		m_persistent = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CLoadQ::OnUpdateLoadqPersistentKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CLoadQ::OnLoadqRemHeadersKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_remove_hdrs)
	{
		m_remove_hdrs = FALSE;
	}
	else
	{
		m_remove_hdrs = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CLoadQ::OnUpdateLoadqRemHeadersKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CLoadQ::OnLoadqUseSetAllKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_use_set_all)
	{
		m_use_set_all = FALSE;
	}
	else
	{
		m_use_set_all = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CLoadQ::OnUpdateLoadqUseSetAllKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CLoadQ::OnLoadqRemoveMqmdKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_remove_mqmd)
	{
		m_remove_mqmd = FALSE;
	}
	else
	{
		m_remove_mqmd = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CLoadQ::OnUpdateLoadqRemoveMqmdKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

