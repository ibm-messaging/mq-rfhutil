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

// Savemsgs.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "Savemsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEFAULT_DELIMITER	"#@#@#"
#define DEFAULT_PROP_DELIM	"$%$%$"

/////////////////////////////////////////////////////////////////////////////
// CSavemsgs dialog


CSavemsgs::CSavemsgs(CWnd* pParent /*=NULL*/)
	: CDialog(CSavemsgs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSavemsgs)
	m_delimiter_type = -1;
	m_end_count = _T("");
	m_remove = -1;
	m_start_count = _T("");
	m_delimiter = _T("");
	m_files_type = -1;
	m_filename = _T("");
	m_incl_mqmd = FALSE;
	m_incl_headers = FALSE;
	m_prop_delim = _T("");
	m_append_file = FALSE;
	m_hAccel = NULL;
	memset(strTitle, 0, sizeof(strTitle));

	//}}AFX_DATA_INIT

	// set a default delimiter
	m_prop_delim = DEFAULT_PROP_DELIM;
	m_delimiter = DEFAULT_DELIMITER;
	m_delimiter_type = SAVE_DELIM_CHAR;
	m_remove = SAVE_REMOVE_MSGS_NO;
	m_files_type = SAVE_FILE_SAME_FILE;
	m_incl_mqmd = TRUE;

	// initialize maximum file size to 2G
	m_savemsgs_maxFileSize = _T("2G");
}


void CSavemsgs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSavemsgs)
	DDX_Radio(pDX, IDC_SAVEMSGS_DELIM_CHAR, m_delimiter_type);
	DDX_Text(pDX, IDC_SAVEMSGS_END_COUNT, m_end_count);
	DDX_Radio(pDX, IDC_SAVEMSGS_REM_NO, m_remove);
	DDX_Text(pDX, IDC_SAVEMSGS_START, m_start_count);
	DDX_Text(pDX, IDC_SAVEMSGS_DELIMITER, m_delimiter);
	DDV_MaxChars(pDX, m_delimiter, 16);
	DDX_Radio(pDX, IDC_SAVEMSGS_ONEFILE, m_files_type);
	DDX_Text(pDX, IDC_SAVEMSGS_FILENAME, m_filename);
	DDX_Check(pDX, IDC_SAVEMSGS_INCL_MQMD, m_incl_mqmd);
	DDX_Check(pDX, IDC_SAVEMSGS_INCL_HEADERS, m_incl_headers);
	DDX_Text(pDX, IDC_SAVEMSGS_PROP_DELIM, m_prop_delim);
	DDV_MaxChars(pDX, m_prop_delim, 16);
	DDX_Check(pDX, IDC_SAVEMSGS_APPEND, m_append_file);
	//}}AFX_DATA_MAP
	DDX_CBString(pDX, IDC_SAVEMSGS_MAXFILESIZE, m_savemsgs_maxFileSize);
	DDV_MaxChars(pDX, m_savemsgs_maxFileSize, 4);
}


BEGIN_MESSAGE_MAP(CSavemsgs, CDialog)
	//{{AFX_MSG_MAP(CSavemsgs)
	ON_BN_CLICKED(IDC_SAVEMSGS_BROWSE, OnSavemsgsBrowse)
	ON_COMMAND(IDC_SAVEMSGS_BROWSE_KEY, OnSavemsgsBrowseKey)
	ON_UPDATE_COMMAND_UI(IDC_SAVEMSGS_BROWSE_KEY, OnUpdateSavemsgsBrowseKey)
	ON_COMMAND(IDC_SAVEMSGS_DELIM_CHAR_KEY, OnSavemsgsDelimCharKey)
	ON_UPDATE_COMMAND_UI(IDC_SAVEMSGS_DELIM_CHAR_KEY, OnUpdateSavemsgsDelimCharKey)
	ON_COMMAND(IDC_SAVEMSGS_DELIM_HEX_KEY, OnSavemsgsDelimHexKey)
	ON_UPDATE_COMMAND_UI(IDC_SAVEMSGS_DELIM_HEX_KEY, OnUpdateSavemsgsDelimHexKey)
	ON_COMMAND(IDC_SAVEMSGS_FILE_PER_MSG_KEY, OnSavemsgsFilePerMsgKey)
	ON_UPDATE_COMMAND_UI(IDC_SAVEMSGS_FILE_PER_MSG_KEY, OnUpdateSavemsgsFilePerMsgKey)
	ON_COMMAND(IDC_SAVEMSGS_INCL_HEADERS_KEY, OnSavemsgsInclHeadersKey)
	ON_UPDATE_COMMAND_UI(IDC_SAVEMSGS_INCL_HEADERS_KEY, OnUpdateSavemsgsInclHeadersKey)
	ON_COMMAND(IDC_SAVEMSGS_INCL_MQMD_KEY, OnSavemsgsInclMqmdKey)
	ON_UPDATE_COMMAND_UI(IDC_SAVEMSGS_INCL_MQMD_KEY, OnUpdateSavemsgsInclMqmdKey)
	ON_COMMAND(IDC_SAVEMSGS_ONEFILE_KEY, OnSavemsgsOnefileKey)
	ON_UPDATE_COMMAND_UI(IDC_SAVEMSGS_ONEFILE_KEY, OnUpdateSavemsgsOnefileKey)
	ON_COMMAND(IDC_SAVEMSGS_REM_NO_KEY, OnSavemsgsRemNoKey)
	ON_UPDATE_COMMAND_UI(IDC_SAVEMSGS_REM_NO_KEY, OnUpdateSavemsgsRemNoKey)
	ON_COMMAND(IDC_SAVEMSGS_REM_YES_KEY, OnSavemsgsRemYesKey)
	ON_UPDATE_COMMAND_UI(IDC_SAVEMSGS_REM_YES_KEY, OnUpdateSavemsgsRemYesKey)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CSavemsgs::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CSavemsgs::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSavemsgs message handlers

void CSavemsgs::OnOK() 

{
	int delimErr;
	int	delimLen;

	// Update the control variables
	UpdateData (TRUE);

	// do some basic editing
	// was a file name specified?
	if (m_filename.GetLength() == 0)
	{
		SetDlgItemText(IDC_SAVEMSGS_ERROR_MSG, "File name is required");
		return;
	}

	// do we need to check the delimiter string?  It is ignored for separate files
	if (SAVE_FILE_SAME_FILE == m_files_type)
	{
		if (SAVE_DELIM_HEX == m_delimiter_type)
		{
			// validate if the delimiter is valid hex
			delimLen = m_delimiter.GetLength();
			delimErr = checkIfHex((LPCTSTR)m_delimiter, delimLen);

			// check that the delimiter string is an even number of valid characters
			if ((0 == delimErr) || ((delimLen & 1) > 0))
			{
				SetDlgItemText(IDC_SAVEMSGS_ERROR_MSG, "Invalid hex delimiter string");
				return;
			}

			// validate if the delimiter is valid hex
			delimLen = m_prop_delim.GetLength();
			delimErr = checkIfHex((LPCTSTR)m_prop_delim, delimLen);

			// check that the delimiter string is an even number of valid characters
			if ((0 == delimErr) || ((delimLen & 1) > 0))
			{
				SetDlgItemText(IDC_SAVEMSGS_ERROR_MSG, "Invalid hex property delimiter string");
				return;
			}
		}

		// check if the delimiter and property delimiter are the same
		if ((strcmp((LPCTSTR)m_delimiter, (LPCTSTR)m_prop_delim) == 0) && (m_prop_delim.GetLength() > 0))
		{
			SetDlgItemText(IDC_SAVEMSGS_ERROR_MSG, "Delimiter and property delimiter cannot be the same");
			return;
		}
	}

	// no errors - close the dialog box
	CDialog::OnOK();
}

////////////////////////////////////////////
//
// Use a standard file dialog to locate
// the file to save to.  If an existing
// file is selected it will be overwritten.
//
////////////////////////////////////////////

void CSavemsgs::OnSavemsgsBrowse() 

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

BOOL CSavemsgs::OnInitDialog()

{
	CDialog::OnInitDialog();

	// initialize maximum file size to 2G
	m_savemsgs_maxFileSize = "2G";

	// load the combo box for the maximum file size
	((CComboBox *)GetDlgItem(IDC_SAVEMSGS_MAXFILESIZE))->ResetContent();
	((CComboBox *)GetDlgItem(IDC_SAVEMSGS_MAXFILESIZE))->AddString("128M");
	((CComboBox *)GetDlgItem(IDC_SAVEMSGS_MAXFILESIZE))->AddString("256M");
	((CComboBox *)GetDlgItem(IDC_SAVEMSGS_MAXFILESIZE))->AddString("512M");
	((CComboBox *)GetDlgItem(IDC_SAVEMSGS_MAXFILESIZE))->AddString("1G");
	((CComboBox *)GetDlgItem(IDC_SAVEMSGS_MAXFILESIZE))->AddString("1.5G");
	((CComboBox *)GetDlgItem(IDC_SAVEMSGS_MAXFILESIZE))->AddString("2G");

	// set default selection of 2G
	((CComboBox *)GetDlgItem(IDC_SAVEMSGS_MAXFILESIZE))->SelectString(-1, "2G");
	
	// tool tips are provided for this dialog and must be initialized
	EnableToolTips(TRUE);

#ifdef SAFEMODE
	((CButton *)GetDlgItem(IDC_SAVEMSGS_REM_YES))->EnableWindow(FALSE);
#endif

	// load the accelerator key table for this dialog
	m_hAccel = ::LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_SAVEMSGS_ACCELERATOR));

	// set the dialog title
	sprintf(strTitle, "Save messages from Queue %s to file(s)", (LPCTSTR)m_q_name);
	SetWindowText(strTitle);

	// clear the error text
	SetDlgItemText(IDC_SAVEMSGS_ERROR_MSG, "   ");

	return TRUE;
}

BOOL CSavemsgs::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

BOOL CSavemsgs::PreTranslateMessage(MSG* pMsg) 

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

void CSavemsgs::OnSavemsgsBrowseKey() 

{
	// call the same routine as if the button had been pushed
	OnSavemsgsBrowse();
}

void CSavemsgs::OnUpdateSavemsgsBrowseKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CSavemsgs::OnSavemsgsDelimCharKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_delimiter_type = SAVE_DELIM_CHAR;

	// Update the controls
	UpdateData (FALSE);
}

void CSavemsgs::OnUpdateSavemsgsDelimCharKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CSavemsgs::OnSavemsgsDelimHexKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_delimiter_type = SAVE_DELIM_HEX;

	// Update the controls
	UpdateData (FALSE);
}

void CSavemsgs::OnUpdateSavemsgsDelimHexKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CSavemsgs::OnSavemsgsFilePerMsgKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_files_type = SAVE_FILE_DIFF_FILES;

	// Update the controls
	UpdateData (FALSE);
}

void CSavemsgs::OnUpdateSavemsgsFilePerMsgKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CSavemsgs::OnSavemsgsInclHeadersKey() 

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

void CSavemsgs::OnUpdateSavemsgsInclHeadersKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CSavemsgs::OnSavemsgsInclMqmdKey() 

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

void CSavemsgs::OnUpdateSavemsgsInclMqmdKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CSavemsgs::OnSavemsgsOnefileKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_files_type = SAVE_FILE_SAME_FILE;

	// Update the controls
	UpdateData (FALSE);
	m_files_type = SAVE_FILE_SAME_FILE;	
}

void CSavemsgs::OnUpdateSavemsgsOnefileKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CSavemsgs::OnSavemsgsRemNoKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_remove = SAVE_REMOVE_MSGS_NO;

	// Update the controls
	UpdateData (FALSE);
}

void CSavemsgs::OnUpdateSavemsgsRemNoKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CSavemsgs::OnSavemsgsRemYesKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	m_remove = SAVE_REMOVE_MSGS_YES;

	// Update the controls
	UpdateData (FALSE);
}

void CSavemsgs::OnUpdateSavemsgsRemYesKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

