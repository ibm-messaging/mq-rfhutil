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

#if !defined(AFX_CAPPUBS_H__A2F849C7_BA5A_411A_B718_AE3DC80CEE47__INCLUDED_)
#define AFX_CAPPUBS_H__A2F849C7_BA5A_411A_B718_AE3DC80CEE47__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CapPubs.h : header file
//

#include "DataArea.h"

/////////////////////////////////////////////////////////////////////////////
// CCapPubs dialog

class CCapPubs : public CDialog
{
// Construction
public:
	CCapPubs(CWnd* pParent = NULL);		// standard constructor

	//  pointer to DataArea object
	DataArea	*pDoc;

	int			exitCode;				// indicator if worker thread is still running
	CAPTPARMS	*parms;					// parameters area

// Dialog Data
	//{{AFX_DATA(CCapPubs)
	enum { IDD = IDD_CAPTURE_PUBS };
	CString	m_filename;
	CString	m_delimiter;
	BOOL	m_append_file;
	CString	m_end_count;
	BOOL	m_incl_headers;
	BOOL	m_incl_mqmd;
	CString	m_prop_delim;
	int	m_delimiter_type;
	CString	m_max_wait;
	BOOL	m_incl_topic;
	BOOL	m_incl_props;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCapPubs)
	protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCapPubs)
	afx_msg void OnCaptureStop();
	afx_msg void OnCapture();
	virtual BOOL OnInitDialog();
	afx_msg void OnCaptureBrowse();
	afx_msg void OnCapPubsDelimCharKey();
	afx_msg void OnUpdateCapPubsDelimCharKey(CCmdUI* pCmdUI);
	afx_msg void OnCapPubsDelimHexKey();
	afx_msg void OnUpdateCapPubsDelimHexKey(CCmdUI* pCmdUI);
	afx_msg void OnCapPubsInclTopicKey();
	afx_msg void OnUpdateCapPubsInclTopicKey(CCmdUI* pCmdUI);
	afx_msg void OnCapPubsInclPropsKey();
	afx_msg void OnUpdateCapPubsInclPropsKey(CCmdUI* pCmdUI);
	afx_msg void OnCapPubsInclMQMDKey();
	afx_msg void OnUpdateCapPubsInclMQMDKey(CCmdUI* pCmdUI);
	afx_msg void OnCapPubsInclHeadersKey();
	afx_msg void OnUpdateCapPubsInclHeadersKey(CCmdUI* pCmdUI);
	afx_msg void OnCapPubsAppendFileKey();
	afx_msg void OnUpdateCapPubsAppendFileKey(CCmdUI* pCmdUI);
	afx_msg void OnCapPubsStopKey();
	afx_msg void OnUpdateCapPubsStopKey(CCmdUI* pCmdUI);
	afx_msg void OnCapPubsWriteKey();
	afx_msg void OnUpdateCapPubsWriteKey(CCmdUI* pCmdUI);
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

#endif // !defined(AFX_CAPPUBS_H__A2F849C7_BA5A_411A_B718_AE3DC80CEE47__INCLUDED_)
