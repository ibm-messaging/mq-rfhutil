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

#if !defined(AFX_MOVEQ_H__B9F10E8E_18B6_4155_A820_34C22AB1988A__INCLUDED_)
#define AFX_MOVEQ_H__B9F10E8E_18B6_4155_A820_34C22AB1988A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MoveQ.h : header file
//
#include "DataArea.h"

/////////////////////////////////////////////////////////////////////////////
// MoveQ dialog

class MoveQ : public CDialog
{
// Construction
public:
	MoveQ(CWnd* pParent = NULL);   // standard constructor
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	DataArea* pDoc;

	int			added;			// number of queue names added to combo box
	int			skipped;		// number of queue names not added to combo box

// Dialog Data
	//{{AFX_DATA(MoveQ)
	enum { IDD = IDD_MOVEQ };
	CString	m_max_count;
	CString	m_new_queue_name;
	BOOL	m_remove_dlq;
	CString	m_start_msg;
	BOOL	m_pass_all;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(MoveQ)
	protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
//	virtual BOOL ProcessMessageFilter(int code, LPMSG lpMsg) ;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(MoveQ)
	afx_msg void OnMoveqRemoveDlqKey();
	afx_msg void OnUpdateMoveqRemoveDlqKey(CCmdUI* pCmdUI);
	afx_msg void OnMoveqPassAllKey();
	afx_msg void OnUpdateMoveqPassAllKey(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	HACCEL		m_hAccel;		// accelerator table for this dialog
	BOOL		OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOVEQ_H__B9F10E8E_18B6_4155_A820_34C22AB1988A__INCLUDED_)
