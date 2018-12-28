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

#if !defined(AFX_GENERAL_H__12C3452F_A3E3_46D8_B197_FAC4CBC4EE9B__INCLUDED_)
#define AFX_GENERAL_H__12C3452F_A3E3_46D8_B197_FAC4CBC4EE9B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// General.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// General dialog

#include "resource.h"
#include "DataArea.h"

#include "MyComboBox.h"	// Added by ClassView

class General : public CPropertyPage
{
	DECLARE_DYNCREATE(General)

// Construction
public:
	void EndBr();
	void StartBr();
	void WriteQ();
	void SaveQ();
	void ReadQ();
	void LoadQ();
	void CloseQ();
	void ClearData();
	void ClearAll();
	void SetUserId();
	void LoadNames();
	void PurgeQ();
	void BrowseQ();
	void DisplayQ();
	void getRemoteQM(CString &remoteQM);
	void getQname(CString& qName);
	void getQMname(CString& qmName);
	int getMaxY();
	int getMaxX();
	void UpdatePageData();
	DataArea* pDoc;
	General();
	~General();

// Dialog Data
	//{{AFX_DATA(General)
	enum { IDD = IDD_MAIN };
	CString	m_q_name;
	CString	m_qm_name;
	CString	m_file_name;
	CString	m_file_size;
	CString	m_copybook_file_name;
	CString	m_errmsg;
	CString	m_remote_qm;
	int		m_cluster_bind;
	CString	m_queue_type;
	CString	m_q_depth;
	BOOL	m_allow_seg;
	BOOL	m_all_avail;
	BOOL	m_logical_order;
	BOOL	m_set_UserID;
	BOOL	m_get_by_correlid;
	BOOL	m_get_by_groupid;
	BOOL	m_set_all;
	BOOL	m_get_by_msgid;
	BOOL	m_convert;
	BOOL	m_complete_msg;
	BOOL	m_alt_userid;
	BOOL	m_new_correl_id;
	BOOL	m_new_msg_id;
	CString	m_filter;
	int		m_mq_props;
	CString	m_file_codepage;
	int		m_close_options;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(General)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnSetFont(CFont* pFont);
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(General)
	afx_msg void OnExit();
	afx_msg void OnReadq();
	afx_msg void OnWriteq();
	afx_msg void OnFileOpen();
	afx_msg void OnCopybook();
	afx_msg void OnClearData();
	afx_msg void OnClearAll();
	afx_msg void OnWriteFile();
	afx_msg void OnBrowse();
	afx_msg void OnEndbr();
	afx_msg void OnBrnext();
	afx_msg void OnStartbr();
	afx_msg void OnBindAsQ();
	afx_msg void OnBindNotFixed();
	afx_msg void OnBindOnOpen();
	afx_msg void OnPurgeQ();
	afx_msg void OnDropdownQm();
	afx_msg void OnSelendcancelQm();
	afx_msg void OnSelchangeQm();
	afx_msg void OnExitButton();
	virtual BOOL OnInitDialog();
	afx_msg void OnDropdownQName();
	afx_msg void OnSelchangeQName();
	afx_msg void OnSelendcancelQName();
	afx_msg void OnEditchangeQName();
	afx_msg void OnCloseq();
	afx_msg void OnSetUserid();
	afx_msg void OnLoadQueueNames();
	afx_msg void OnSetfocusQm();
	afx_msg void OnEditchangeQm();
	afx_msg void OnSetAll();
	afx_msg void OnBrprev();
	afx_msg void OnMainDisplayq();
	afx_msg void OnMainSaveq();
	afx_msg void OnMainLoadq();
	afx_msg void OnSetConnUser();
	afx_msg void OnAltUserid();
	afx_msg void OnNewCorrelId();
	afx_msg void OnNewMsgId();
	afx_msg void OnMoveQ();
	afx_msg void OnChangeMainFileCodepage();
	afx_msg void OnKillfocusMainFileCodepage();
	afx_msg void OnCloseNone();
	afx_msg void OnCloseDelete();
	afx_msg void OnClosePurge();
	afx_msg void OnSelendokQm();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int	m_conn_ccsid;					// code page to translate user id and password to when connecting to queue manager

	MyComboBox m_qmComboBox;
	MyComboBox m_q_nameComboBox;
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	void loadEditBox();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENERAL_H__12C3452F_A3E3_46D8_B197_FAC4CBC4EE9B__INCLUDED_)
