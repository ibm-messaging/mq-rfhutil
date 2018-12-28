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

#if !defined(AFX_CONNUSER_H__AF37564D_AEEA_40D3_B3A6_C54F80D2A83D__INCLUDED_)
#define AFX_CONNUSER_H__AF37564D_AEEA_40D3_B3A6_C54F80D2A83D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConnUser.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConnUser dialog

class CConnUser : public CDialog
{
// Construction
public:
	CConnUser(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConnUser)
	enum { IDD = IDD_USERID };
	CString	m_conn_userid;
	CString	m_conn_password;
	BOOL	m_use_ssl;
	BOOL	m_ssl_validate_client;
	CString	m_ssl_cipher;
	CString	m_ssl_keyr;
	CString	m_ssl_reset_count;
	CString	m_security_exit;
	CString	m_security_data;
	CString m_local_address;
	BOOL	m_conn_use_csp;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnUser)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConnUser)
	afx_msg void OnConnReset();
	virtual BOOL OnInitDialog();
	afx_msg void OnDropdownConnSslCipher();
	afx_msg void OnConnBrowse();
	afx_msg void OnUpdateConnUseSslKey(CCmdUI* pCmdUI);
	afx_msg void OnConnUseSslKey();
	afx_msg void OnConnValidateKey();
	afx_msg void OnUpdateConnValidateKey(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void dlgItemAddString(const int dlgItem, const char *itemText);
	HACCEL m_hAccel; // accelerator table for this dialog
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONNUSER_H__AF37564D_AEEA_40D3_B3A6_C54F80D2A83D__INCLUDED_)
