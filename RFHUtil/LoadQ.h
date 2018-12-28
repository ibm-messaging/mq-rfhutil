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

#if !defined(AFX_LOADQ_H__A31EDE07_B29A_4DF2_A641_F0A235A3ACDD__INCLUDED_)
#define AFX_LOADQ_H__A31EDE07_B29A_4DF2_A641_F0A235A3ACDD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LoadQ.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoadQ dialog

class CLoadQ : public CDialog
{
// Construction
public:
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	CLoadQ(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLoadQ)
	enum { IDD = IDD_LOADQ };
	CString	m_delimiter;
	CString	m_filename;
	int		m_files_type;
	BOOL	m_remove_hdrs;
	BOOL	m_remove_mqmd;
	CString	m_max_count;
	int		m_delimiter_type;
	CString	m_format;
	BOOL	m_persistent;
	CString	m_ccsid;
	CString	m_encoding;
	BOOL	m_use_set_all;
	BOOL	m_new_msg_id;
	CString	m_start_msg;
	CString	m_loadq_prop_delim;
	BOOL	m_loadq_ignore_props;
	CString	m_batchSize;
	CString	m_waitTime;
	BOOL	m_write_once;
	BOOL	m_single_file;
	//}}AFX_DATA

	CString	m_q_name;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoadQ)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLoadQ)
	afx_msg void OnLoadqBrowse();
	afx_msg void OnLoadqDelimCharKey();
	afx_msg void OnUpdateLoadqDelimCharKey(CCmdUI* pCmdUI);
	afx_msg void OnLoadqDelimHexKey();
	afx_msg void OnUpdateLoadqDelimHexKey(CCmdUI* pCmdUI);
	afx_msg void OnLoadqFilePerMsgKey();
	afx_msg void OnUpdateLoadqFilePerMsgKey(CCmdUI* pCmdUI);
	afx_msg void OnLoadqNewMsgidKey();
	afx_msg void OnUpdateLoadqNewMsgidKey(CCmdUI* pCmdUI);
	afx_msg void OnLoadqOnefileKey();
	afx_msg void OnUpdateLoadqOnefileKey(CCmdUI* pCmdUI);
	afx_msg void OnLoadqPersistentKey();
	afx_msg void OnUpdateLoadqPersistentKey(CCmdUI* pCmdUI);
	afx_msg void OnLoadqRemHeadersKey();
	afx_msg void OnUpdateLoadqRemHeadersKey(CCmdUI* pCmdUI);
	afx_msg void OnLoadqUseSetAllKey();
	afx_msg void OnUpdateLoadqUseSetAllKey(CCmdUI* pCmdUI);
	afx_msg void OnLoadqRemoveMqmdKey();
	afx_msg void OnUpdateLoadqRemoveMqmdKey(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	HACCEL m_hAccel;		// accelerator table for this dialog
	char strTitle[96];		// dialog window title
	BOOL OnInitDialog();
public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOADQ_H__A31EDE07_B29A_4DF2_A641_F0A235A3ACDD__INCLUDED_)
