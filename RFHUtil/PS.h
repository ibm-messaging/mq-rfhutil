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

#if !defined(AFX_PS_H__A1D82948_CC5E_4D71_961F_1F0C8E466C5F__INCLUDED_)
#define AFX_PS_H__A1D82948_CC5E_4D71_961F_1F0C8E466C5F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PS.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPS dialog

#include "DataArea.h"

#include "MyComboBox.h"	// Added by ClassView
#include "IdEdit.h"	// Added by ClassView

class CPS : public CPropertyPage
{
	DECLARE_DYNCREATE(CPS)

// Construction
public:
	void WriteMsgs();
	void SaveMsgs();
	void GetTopics();
	void Subscribe();
	void Resume();
	void Publish();
	void GetSubs();
	void ClearAll();
	void GetMsg();
	void ReqPub();
	void CloseQ();
	void AlterSub();
	void UpdatePageData();
	CPS();
	~CPS();

	DataArea* pDoc;

	MQBYTE24 m_correlid;			// subscription correlation id

// Dialog Data
	//{{AFX_DATA(CPS)
	enum { IDD = IDD_V7PUBSUB };
	CString	m_ps_broker_qm;
	CString	m_ps_qm;
	CString	m_ps_subname;
	CString	m_ps_topic;
	BOOL	m_ps_durable;
	BOOL	m_ps_remove;
	CString	m_ps_errmsgs;
	BOOL	m_ps_managed;
	CString	m_ps_wait_interval;
	BOOL	m_ps_retain;
	CString	m_ps_q;
	BOOL	m_ps_local;
	BOOL	m_ps_not_own;
	BOOL	m_ps_suppress_reply;
	BOOL	m_ps_new_only;
	BOOL	m_ps_on_demand;
	CString	m_ps_topicName;
	CString	m_ps_sub_level;
	CString	m_ps_priority;
	BOOL	m_ps_group_sub;
	int		m_ps_wildcard;
	int		m_ps_correl_ascii;
	CString	m_ps_sub_id;
	CString	m_ps_expiry;
	BOOL	m_ps_set_correlid;
	CString	m_ps_user_data;
	BOOL	m_ps_any_userid;
	BOOL	m_ps_is_retained;
	CString	m_ps_appl_ident;
	CString	m_ps_acct_token;
	BOOL	m_ps_set_ident;
	CString	m_ps_selection;
	BOOL	m_ps_warn_no_pub;
	BOOL	m_ps_no_multicast;
	BOOL	m_ps_new_correl_id;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPS)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPS)
	afx_msg void OnDropdownPsSubname();
	afx_msg void OnDropdownPsQm();
	afx_msg void OnPsGet();
	afx_msg void OnPsGetSubs();
	afx_msg void OnPsGetTopics();
	afx_msg void OnPsPublish();
	afx_msg void OnPsCloseq();
	virtual BOOL OnInitDialog();
	afx_msg void OnPsSubscribe();
	afx_msg void OnSetfocusPsQm();
	afx_msg void OnDropdownPsQueueName();
	afx_msg void OnDropdownPsTopicName();
	afx_msg void OnPsReqPub();
	afx_msg void OnPsResume();
	afx_msg void OnPsClear();
	afx_msg void OnPsCorrelAscii();
	afx_msg void OnPsCorrelHex();
	afx_msg void OnEditchangePsSubname();
	afx_msg void OnSelchangePsSubname();
	afx_msg void OnSelendcancelPsSubname();
	afx_msg void OnEditchangePsTopicName();
	afx_msg void OnSelchangePsTopicName();
	afx_msg void OnSelendcancelPsTopicName();
	afx_msg void OnEditchangePsQm();
	afx_msg void OnSelchangePsQm();
	afx_msg void OnSelendcancelPsQm();
	afx_msg void OnEditchangePsQueueName();
	afx_msg void OnSelchangePsQueueName();
	afx_msg void OnSelendcancelPsQueueName();
	afx_msg void OnPsAlterSub();
	afx_msg void OnPsSaveMsgs();
	afx_msg void OnPsWriteMsgs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void SetSubCorrelId(MQBYTE24 * id);
	void setSubscriptionButtons();
	void loadEditBox();
	MyComboBox m_ps_qmComboBox;
	MyComboBox m_ps_qComboBox;
	MyComboBox m_ps_subComboBox;
	MyComboBox m_ps_topicNameComboBox;
	CIdEdit m_SubCorrelIdEdit;
	CIdEdit m_SubAcctTokenEdit;
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	CAPTPARMS	cparms;								// parameters area for capturing messages
	WRITEPARMS	wparms;								// parameters area for publishing messages
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PS_H__A1D82948_CC5E_4D71_961F_1F0C8E466C5F__INCLUDED_)
