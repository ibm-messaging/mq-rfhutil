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

#if !defined(AFX_PUBSUB_H__5E5AAA80_5B98_4D91_8F43_B12B8CDBE9FD__INCLUDED_)
#define AFX_PUBSUB_H__5E5AAA80_5B98_4D91_8F43_B12B8CDBE9FD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PubSub.h : header file
//

#include "DataArea.h"

#include "MyComboBox.h"	// Added by ClassView

/////////////////////////////////////////////////////////////////////////////
// PubSub dialog

class PubSub : public CPropertyPage
{
	DECLARE_DYNCREATE(PubSub)

// Construction
public:
	void clearPubSubData();
	BOOL wasDataChanged();
	const char * getPubsubArea(int ccsid, int encoding);
	int buildPubsubArea(int ccsid, int encoding);
	int buildV1PubsubArea(char * tempBuf);
	char * parseV1Pubsub(char * rfhptr, int *found);
	void parseV1Command(char * rfhptr);
	void parseRFH2pubsub(unsigned char *rfhdata, int dataLen);
	void freePubsubArea();
	void setPubsubArea(unsigned char *pubsubData, int dataLen, int ccsid, int encoding);
	void updatePageData();
	void PubProcess();
	void PubClear();
	DataArea* pDoc;
	PubSub();
	~PubSub();

// Dialog Data
	//{{AFX_DATA(PubSub)
	enum { IDD = IDD_PUB };
	CString	m_ps_connect_q;
	CString	m_ps_connect_qm;
	CString	m_ps_errmsg;
	CString	m_ps_topic1;
	CString	m_ps_topic2;
	CString	m_ps_topic3;
	CString	m_ps_topic4;
	CString	m_ps_q;
	CString	m_ps_qm;
	CString	m_ps_filter;
	CString	m_ps_broker_qm;
	CString	m_ps_subpoint;
	CString	m_ps_pubtime;
	CString	m_ps_seqno;
	CString	m_ps_sub_data;
	CString	m_ps_sub_identity;
	CString	m_ps_sub_name;
	int		m_ps_persist;
	int		m_ps_reqtype;
	BOOL	m_inform_if_retained;
	BOOL	m_ps_isretained;
	BOOL	m_ps_correlasid;
	BOOL	m_ps_local;
	BOOL	m_ps_newonly;
	BOOL	m_ps_ondemand;
	BOOL	m_ps_deregall;
	BOOL	m_ps_retain;
	BOOL	m_ps_otheronly;
	BOOL	m_ps_var_user;
	BOOL	m_ps_noalter;
	BOOL	m_ps_locked;
	BOOL	m_ps_join_shared;
	BOOL	m_ps_join_excl;
	BOOL	m_ps_full_resp;
	BOOL	m_ps_add_name;
	BOOL	m_ps_incl_stream_name;
	BOOL	m_ps_leave_only;
	BOOL	m_ps_no_reg;
	BOOL	m_ps_direct_req;
	BOOL	m_ps_dups_ok;
	BOOL	m_ps_anon;
	CString	m_ps_other;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PubSub)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PubSub)
	afx_msg void OnRegister();
	afx_msg void OnReqPub();
	afx_msg void OnPublish();
	afx_msg void OnUnregister();
	afx_msg void OnDeletePub();
	afx_msg void OnPubProcess();
	afx_msg void OnPubClear();
	afx_msg void OnPubAspub();
	afx_msg void OnChangePubBrokerQm();
	afx_msg void OnPubCorrelasid();
	afx_msg void OnPubDeregall();
	afx_msg void OnChangePubFilter();
	afx_msg void OnPubInformRetain();
	afx_msg void OnPubLocal();
	afx_msg void OnPubOndemand();
	afx_msg void OnPubOtheronly();
	afx_msg void OnPubPerNone();
	afx_msg void OnChangePubPubtime();
	afx_msg void OnChangePubQ();
	afx_msg void OnChangePubQm();
	afx_msg void OnPubRetain();
	afx_msg void OnChangePubSeqno();
	afx_msg void OnChangePubSubpoint();
	afx_msg void OnChangePubTopic1();
	afx_msg void OnChangePubTopic2();
	afx_msg void OnChangePubTopic3();
	afx_msg void OnChangePubTopic4();
	afx_msg void OnPubasqueue();
	afx_msg void OnPubnonpers();
	afx_msg void OnPubpersist();
	afx_msg void OnSaveReqData();
	afx_msg void OnDropdownPsConnectQm();
	afx_msg void OnSelendcancelPsConnectQm();
	afx_msg void OnSelchangePsConnectQm();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangePsConnectQ();
	afx_msg void OnPubIsretained();
	afx_msg void OnRegPub();
	afx_msg void OnDeregPub();
	afx_msg void OnPubFullResp();
	afx_msg void OnPubJoinExcl();
	afx_msg void OnPubJoinShared();
	afx_msg void OnPubLocked();
	afx_msg void OnChangePubSubData();
	afx_msg void OnChangePubSubIdentity();
	afx_msg void OnChangePubSubName();
	afx_msg void OnPubNoalter();
	afx_msg void OnPubVarUser();
	afx_msg void OnPubAddName();
	afx_msg void OnPubDirectReq();
	afx_msg void OnPubDupsOk();
	afx_msg void OnPubInclStreamName();
	afx_msg void OnPubLeaveOnly();
	afx_msg void OnPubNoReg();
	afx_msg void OnPubAnon();
	afx_msg void OnChangePubOther();
	afx_msg void OnSetfocusPsConnectQm();
	afx_msg void OnSetfocusPubQm();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	MyComboBox m_ps_qmComboBox;
	char * buildV1DelOpt(char * ptr, const char *value);
	char * buildV1PubOpt(char * ptr, const char *value);
	char * buildV1RegOpt(char * ptr, const char *value);
	char * buildV1Opt(char * ptr, const char *value, const char * opt);
	void processPubsubOption(char *value);
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	int m_RFH_pubsub_len;
	int m_RFH_pubsub_ccsid;
	int m_RFH_pubsub_encoding;
	unsigned char * rfh_pubsub_area;
	void setUnregPub();
	void setRegPub();
	void setRFHV1();
	void setRFHV2();
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	void updateErrmsg();
	void setNone();
	void setDeletePub();
	void setUnregister();
	void setPublish();
	void setReqPub();
	void setRegister();
	void setConnectionQueue();
	void loadEditBox();
	void savePageData();
	bool pubsubDataChanged;
	char * parseRFH2misc(char * ptr,
						 char * endptr,
						 char * tempUser,
						 const char * endTag,
						 const int maxUser,
						 int * idx);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PUBSUB_H__5E5AAA80_5B98_4D91_8F43_B12B8CDBE9FD__INCLUDED_)
