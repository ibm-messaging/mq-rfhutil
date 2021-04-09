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

// ConnUser.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "ConnUser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnUser dialog

CConnUser::CConnUser(CWnd* pParent /*=NULL*/)
	: CDialog(CConnUser::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConnUser)
	m_conn_userid = _T("");
	m_conn_password = _T("");
	m_use_ssl = FALSE;
	m_ssl_validate_client = FALSE;
	m_ssl_cipher = _T("");
	m_ssl_keyr = _T("");
	m_ssl_reset_count = _T("0");
	m_security_exit = _T("");
	m_security_data = _T("");
	m_local_address = _T("");
	m_conn_use_csp = TRUE;
	m_hAccel = NULL;
	//}}AFX_DATA_INIT
}


void CConnUser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnUser)
	DDX_Text(pDX, IDC_CONN_USERID, m_conn_userid);
	DDV_MaxChars(pDX, m_conn_userid, 255);
	DDX_Text(pDX, IDC_CONN_PASSWORD, m_conn_password);
	DDV_MaxChars(pDX, m_conn_password, 1024);
	DDX_Check(pDX, IDC_CONN_USE_SSL, m_use_ssl);
	DDX_Check(pDX, IDC_CONN_VALIDATE_CLIENT, m_ssl_validate_client);
	DDX_CBString(pDX, IDC_CONN_SSL_CIPHER, m_ssl_cipher);
	DDV_MaxChars(pDX, m_ssl_cipher, 32);
	DDX_Text(pDX, IDC_CONN_SSL_KEYR, m_ssl_keyr);
	DDV_MaxChars(pDX, m_ssl_keyr, 254);
	DDX_Text(pDX, IDC_CONN_SSL_RESET_COUNT, m_ssl_reset_count);
	DDX_Text(pDX, IDC_CONN_SECURITY_EXIT, m_security_exit);
	DDV_MaxChars(pDX, m_security_exit, 128);
	DDX_Text(pDX, IDC_CONN_SECURITY_DATA, m_security_data);
	DDV_MaxChars(pDX, m_security_data, 32);
	DDX_Text(pDX, IDC_CONN_LOCAL_ADDRESS, m_local_address);
	DDV_MaxChars(pDX, m_local_address, 48);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnUser, CDialog)
	//{{AFX_MSG_MAP(CConnUser)
	ON_BN_CLICKED(IDC_CONN_RESET, OnConnReset)
	ON_CBN_DROPDOWN(IDC_CONN_SSL_CIPHER, OnDropdownConnSslCipher)
	ON_BN_CLICKED(IDC_CONN_BROWSE, OnConnBrowse)
	ON_UPDATE_COMMAND_UI(IDC_CONN_USE_SSL_KEY, OnUpdateConnUseSslKey)
	ON_COMMAND(IDC_CONN_USE_SSL_KEY, OnConnUseSslKey)
	ON_COMMAND(IDC_CONN_VALIDATE_KEY, OnConnValidateKey)
	ON_UPDATE_COMMAND_UI(IDC_CONN_VALIDATE_KEY, OnUpdateConnValidateKey)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CConnUser::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CConnUser::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnUser message handlers

BOOL CConnUser::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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
//			else
//			{
//				pTTTW->lpszText = MAKEINTRESOURCE(nID);
//				pTTTW->hinst = AfxGetResourceHandle();
//			}

			pResult = 0;
			return(TRUE);
		}
	}

	return(FALSE);
}

void CConnUser::OnConnReset() 

{
	// clear the current connection parameters
	m_conn_userid.Empty();
	m_conn_password.Empty();
	m_ssl_cipher.Empty();
	m_ssl_validate_client = FALSE;
	m_conn_use_csp = TRUE;
	m_use_ssl = FALSE;
	m_ssl_reset_count = _T("0");
	m_security_exit.Empty();
	m_security_data.Empty();
	m_local_address.Empty();

	// Update the controls
	UpdateData (FALSE);	
}

BOOL CConnUser::OnInitDialog() 

{
	CEdit			*cedit;

	// call the super class
	CDialog::OnInitDialog();

	// Enable tooltips for this dialog
	EnableToolTips(TRUE);

#ifndef MQCLIENT
	// Suppress the controls that only apply to client connections
	((CComboBox *)GetDlgItem(IDC_CONN_SSL_CIPHER))->EnableWindow(FALSE);
	((CComboBox *)GetDlgItem(IDC_CONN_SSL_CIPHER))->ShowWindow(SW_HIDE);
	((CButton *)GetDlgItem(IDC_CONN_USE_SSL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_CONN_USE_SSL))->ShowWindow(SW_HIDE);
	((CButton *)GetDlgItem(IDC_CONN_VALIDATE_CLIENT))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_CONN_VALIDATE_CLIENT))->ShowWindow(SW_HIDE);
	((CButton *)GetDlgItem(IDC_CONN_BROWSE))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_CONN_BROWSE))->ShowWindow(SW_HIDE);
	((CEdit *)GetDlgItem(IDC_CONN_SSL_KEYR))->EnableWindow(FALSE);
	((CEdit *)GetDlgItem(IDC_CONN_SSL_KEYR))->ShowWindow(SW_HIDE);
	((CWnd *)GetDlgItem(IDC_CONN_SSL_CIPHER_LABEL))->ShowWindow(SW_HIDE);
	((CWnd *)GetDlgItem(IDC_CONN_STORE_LOC_LABEL))->ShowWindow(SW_HIDE);
	((CEdit *)GetDlgItem(IDC_CONN_SSL_RESET_COUNT))->EnableWindow(FALSE);
	((CEdit *)GetDlgItem(IDC_CONN_SSL_RESET_COUNT))->ShowWindow(SW_HIDE);
	((CWnd *)GetDlgItem(IDC_CONN_SSL_RESET_COUNT_LABEL))->ShowWindow(SW_HIDE);
	((CWnd *)GetDlgItem(IDC_CONN_SECURITY_EXIT_LABEL))->ShowWindow(SW_HIDE);
	((CEdit *)GetDlgItem(IDC_CONN_SECURITY_EXIT))->EnableWindow(FALSE);
	((CEdit *)GetDlgItem(IDC_CONN_SECURITY_EXIT))->ShowWindow(SW_HIDE);
	((CWnd *)GetDlgItem(IDC_CONN_SECURITY_DATA_LABEL))->ShowWindow(SW_HIDE);
	((CEdit *)GetDlgItem(IDC_CONN_SECURITY_DATA))->ShowWindow(SW_HIDE);
	((CEdit *)GetDlgItem(IDC_CONN_SECURITY_DATA))->EnableWindow(FALSE);
	((CEdit *)GetDlgItem(IDC_CONN_LOCAL_ADDRESS))->ShowWindow(SW_HIDE);
	((CEdit *)GetDlgItem(IDC_CONN_LOCAL_ADDRESS))->EnableWindow(FALSE);
	((CWnd *)GetDlgItem(IDC_CONN_LOCAL_ADDRESS_LABEL))->ShowWindow(SW_HIDE);
#endif

	// load the accelerator key table for this dialog
	m_hAccel = ::LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_CONNUSER_ACCELERTOR));

	// Set the initial focus so the user can tab between controls
	cedit = (CEdit *)GetDlgItem(IDC_CONN_USERID);
	cedit->SetFocus();

	return FALSE;

//	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConnUser::dlgItemAddString(const int dlgItem, const char *itemText)

{
#ifdef _UNICODE
	wchar_t	str[1024];

	// clear working storage
	memset(str, 0, sizeof(str));

	// convert the string to unicode
	mbstowcs(str, itemText, sizeof(str));

	// add the selection to the dialog box
	((CComboBox *)GetDlgItem(dlgItem))->AddString(str);
#else
	// add the selection to the dialog box
	((CComboBox *)GetDlgItem(dlgItem))->AddString(itemText);
#endif
}

void CConnUser::OnDropdownConnSslCipher() 
{
	// load the MQ SSL cipher options into the combo box
	// Only the TLS 1.2 ciphers are now included here as SSL3 and TLS1.0 are deprecated.
	// From MQ V9.1.1 you can use "ANY_TLS12" to select any valid ciphersuite from that protocol.
	((CComboBox *)GetDlgItem(IDC_CONN_SSL_CIPHER))->ResetContent();

	// insert items into the drop down list
	dlgItemAddString(IDC_CONN_SSL_CIPHER, " ");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ANY_TLS12");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ANY_TLS12_OR_HIGHER");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ANY_TLS13");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ANY_TLS13_OR_HIGHER");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "TLS_RSA_WITH_AES_128_CBC_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "TLS_RSA_WITH_AES_256_CBC_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "TLS_RSA_WITH_NULL_SHA256 ");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "TLS_RSA_WITH_AES_128_GCM_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "TLS_RSA_WITH_AES_256_GCM_SHA384");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_ECDSA_RC4_128_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_ECDSA_3DES_EDE_CBC_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_RSA_RC4_128_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_RSA_3DES_EDE_CBC_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_ECDSA_AES_128_CBC_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_ECDSA_AES_256_CBC_SHA384");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_RSA_AES_128_CBC_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_RSA_AES_256_CBC_SHA384");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_ECDSA_AES_128_GCM_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_ECDSA_AES_256_GCM_SHA384");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_RSA_AES_128_GCM_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_RSA_AES_256_GCM_SHA384");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_RSA_NULL_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "ECDHE_ECDSA_NULL_SHA256");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "TLS_RSA_WITH_NULL_NULL");
	dlgItemAddString(IDC_CONN_SSL_CIPHER, "TLS_RSA_WITH_RC4_128_SHA256");
}


void CConnUser::OnConnBrowse() 

{
	int				rc;
	int				len;
	char			fileName[512];

	// invoke standard dialog to choose file name
	CFileDialog fd(TRUE, NULL, NULL, OFN_PATHMUSTEXIST);
	rc = fd.DoModal();

	//	Did the user press the OK button? 
	if (rc == IDOK)
	{
		// get a copy of the file name
		strcpy(fileName, fd.GetPathName());

		// check for a bad file path
		if (fileName[0] != -35)
		{
			// check for a .kbd file extension
			len = strlen(fileName) - 4;
			if ((len > 0) && (strcmp(fileName + len, ".kdb") == 0))
			{
				// remove the file extension
				fileName[len] = 0;
			}

			// set the full file name as the location of the key file
			m_ssl_keyr = fileName;
		}
	}

	// update the controls from the instance variables
	UpdateData (FALSE);	
}

//////////////////////////////////////////////////////
//
// Override this function to support accelerator keys.
//
//////////////////////////////////////////////////////

BOOL CConnUser::PreTranslateMessage(MSG* pMsg) 

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

void CConnUser::OnUpdateConnUseSslKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void CConnUser::OnConnUseSslKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_use_ssl)
	{
		m_use_ssl = FALSE;
	}
	else
	{
		m_use_ssl = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CConnUser::OnConnValidateKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_ssl_validate_client)
	{
		m_ssl_validate_client = FALSE;
	}
	else
	{
		m_ssl_validate_client = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void CConnUser::OnUpdateConnValidateKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}
