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

#if !defined(AFX_SAVEMSGS_H__609CE7B9_1F57_4351_BAB9_0FB56C69B6CF__INCLUDED_)
#define AFX_SAVEMSGS_H__609CE7B9_1F57_4351_BAB9_0FB56C69B6CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//
// Savemsgs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSavemsgs dialog

class CSavemsgs : public CDialog
{
// Construction
public:
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	BOOL OnInitDialog();
	CSavemsgs(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSavemsgs)
	enum { IDD = IDD_SAVEMSGS };
	int		m_delimiter_type;
	int		m_remove;
	int		m_files_type;
	BOOL	m_incl_mqmd;
	BOOL	m_incl_headers;
	BOOL	m_append_file;
	CString	m_filename;
	CString	m_end_count;
	CString	m_prop_delim;
	CString	m_start_count;
	CString	m_delimiter;
	//}}AFX_DATA

	CString	m_q_name;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSavemsgs)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSavemsgs)
	afx_msg void OnSavemsgsBrowse();
	afx_msg void OnSavemsgsBrowseKey();
	afx_msg void OnUpdateSavemsgsBrowseKey(CCmdUI* pCmdUI);
	afx_msg void OnSavemsgsDelimCharKey();
	afx_msg void OnUpdateSavemsgsDelimCharKey(CCmdUI* pCmdUI);
	afx_msg void OnSavemsgsDelimHexKey();
	afx_msg void OnUpdateSavemsgsDelimHexKey(CCmdUI* pCmdUI);
	afx_msg void OnSavemsgsFilePerMsgKey();
	afx_msg void OnUpdateSavemsgsFilePerMsgKey(CCmdUI* pCmdUI);
	afx_msg void OnSavemsgsInclHeadersKey();
	afx_msg void OnUpdateSavemsgsInclHeadersKey(CCmdUI* pCmdUI);
	afx_msg void OnSavemsgsInclMqmdKey();
	afx_msg void OnUpdateSavemsgsInclMqmdKey(CCmdUI* pCmdUI);
	afx_msg void OnSavemsgsOnefileKey();
	afx_msg void OnUpdateSavemsgsOnefileKey(CCmdUI* pCmdUI);
	afx_msg void OnSavemsgsRemNoKey();
	afx_msg void OnUpdateSavemsgsRemNoKey(CCmdUI* pCmdUI);
	afx_msg void OnSavemsgsRemYesKey();
	afx_msg void OnUpdateSavemsgsRemYesKey(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	HACCEL m_hAccel;		// accelerator table for this dialog
	char strTitle[96];
public:
	CString m_savemsgs_maxFileSize;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAVEMSGS_H__609CE7B9_1F57_4351_BAB9_0FB56C69B6CF__INCLUDED_)
