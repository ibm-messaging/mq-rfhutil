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

#if !defined(AFX_WRITEPUBS_H__79026277_65EB_41BD_8E56_88FD2CE490BA__INCLUDED_)
#define AFX_WRITEPUBS_H__79026277_65EB_41BD_8E56_88FD2CE490BA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WritePubs.h : header file
//

#include "DataArea.h"

/////////////////////////////////////////////////////////////////////////////
// WritePubs dialog

class WritePubs : public CDialog
{
// Construction
public:
	WritePubs(CWnd* pParent = NULL);   // standard constructor

	//  pointer to DataArea object
	DataArea	*pDoc;

	int			exitCode;				// indicator if worker thread is still running
	WRITEPARMS	*parms;					// parameters area

// Dialog Data
	//{{AFX_DATA(WritePubs)
	enum { IDD = IDD_WRITE_PUBS };
	CString	m_delimiter;
	CString	m_end_count;
	CString	m_error_msg;
	CString	m_file_name;
	CString	m_prop_delim;
	CString	m_wait_time;
	int		m_delim_type;
	CString	m_batchsize;
	BOOL	m_write_once;
	BOOL	m_new_correlid;
	BOOL	m_new_msgid;
	BOOL	m_use_mqmd;
	BOOL	m_use_topic;
	BOOL	m_use_props;
	BOOL	m_warn_no_match;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(WritePubs)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(WritePubs)
	afx_msg void OnStop();
	virtual BOOL OnInitDialog();
	afx_msg void OnWritepubsBrowse();
	afx_msg void OnWritepubsPubMsgs();
	afx_msg void OnWritepubsStopPub();
	afx_msg void OnWritepubsDelimCharKey();
	afx_msg void OnUpdateWritepubsDelimCharKey(CCmdUI* pCmdUI);
	afx_msg void OnWritepubsDelimHexKey();
	afx_msg void OnUpdateWritepubsDelimHexKey(CCmdUI* pCmdUI);
	afx_msg void OnWritepubsNewMsgIdKey();
	afx_msg void OnUpdateWritepubsNewMsgIdKey(CCmdUI* pCmdUI);
	afx_msg void OnWritepubsNewCorrelIdKey();
	afx_msg void OnUpdateWritepubsNewCorrelIdKey(CCmdUI* pCmdUI);
	afx_msg void OnWritepubsWriteOnceKey();
	afx_msg void OnUpdateWritepubsWriteOnceKey(CCmdUI* pCmdUI);
	afx_msg void OnWritepubsUseMQMDKey();
	afx_msg void OnUpdateWritepubsUseMQMDKey(CCmdUI* pCmdUI);
	afx_msg void OnWritepubsUsePropsKey();
	afx_msg void OnUpdateWritepubsUsePropsKey(CCmdUI* pCmdUI);
	afx_msg void OnWritepubsUseTopicKey();
	afx_msg void OnUpdateWritepubsUseTopicKey(CCmdUI* pCmdUI);
	afx_msg void OnWritepubsStopKey();
	afx_msg void OnUpdateWritepubsStopKey(CCmdUI* pCmdUI);
	afx_msg void OnWritepubsWriteKey();
	afx_msg void OnUpdateWritepubsWriteKey(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// private routines and variables

private:
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	LONG OnUserClose(UINT wParam, LONG lParam);

	HACCEL		m_hAccel;		// accelerator table for this dialog
	CWinThread	*thread;		// worker thread
	UINT		helperMsg;		// message to close dialog after time out, message count or error
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WRITEPUBS_H__79026277_65EB_41BD_8E56_88FD2CE490BA__INCLUDED_)
