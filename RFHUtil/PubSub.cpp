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

//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "PubSub.h"
#include "mqsubs.h"
#include "comsubs.h"
#include "xmlsubs.h"

// include for RFH2 constants, etc
#include <cmqpsc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MQQMKEY  "SOFTWARE\\IBM\\MQSeries\\CurrentVersion\\Configuration\\QueueManager"
#define MAX_ERR_MSG_LEN		4095

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+5
#define MAX_PUBSUB_NAME	4096

/////////////////////////////////////////////////////////////////////////////
// PubSub PropertyPage

IMPLEMENT_DYNCREATE(PubSub, CPropertyPage)

PubSub::PubSub() : CPropertyPage(PubSub::IDD)
{
	//{{AFX_DATA_INIT(PubSub)
	m_ps_connect_q = _T("");
	m_ps_connect_qm = _T("");
	m_ps_errmsg = _T("");
	m_ps_topic1 = _T("");
	m_ps_topic2 = _T("");
	m_ps_topic3 = _T("");
	m_ps_topic4 = _T("");
	m_ps_q = _T("");
	m_ps_qm = _T("");
	m_ps_filter = _T("");
	m_ps_broker_qm = _T("");
	m_ps_subpoint = _T("");
	m_ps_pubtime = _T("");
	m_ps_seqno = _T("");
	m_ps_sub_data = _T("");
	m_ps_sub_identity = _T("");
	m_ps_sub_name = _T("");
	m_ps_persist = PS_ASPUB;
	m_ps_reqtype = -1;
	m_inform_if_retained = FALSE;
	m_ps_isretained = FALSE;
	m_ps_correlasid = FALSE;
	m_ps_local = FALSE;
	m_ps_newonly = FALSE;
	m_ps_ondemand = FALSE;
	m_ps_deregall = FALSE;
	m_ps_retain = FALSE;
	m_ps_otheronly = FALSE;
	m_ps_var_user = FALSE;
	m_ps_noalter = FALSE;
	m_ps_locked = FALSE;
	m_ps_join_shared = FALSE;
	m_ps_join_excl = FALSE;
	m_ps_full_resp = FALSE;
	m_ps_add_name = FALSE;
	m_ps_incl_stream_name = FALSE;
	m_ps_leave_only = FALSE;
	m_ps_no_reg = FALSE;
	m_ps_direct_req = FALSE;
	m_ps_dups_ok = FALSE;
	m_ps_anon = FALSE;
	pubsubDataChanged = false;
	m_ps_other = _T("");
	//}}AFX_DATA_INIT

	pDoc = NULL;
	rfh_pubsub_area = NULL;
	m_RFH_pubsub_len = 0;
	m_RFH_pubsub_ccsid = -1;
	m_RFH_pubsub_encoding = -1;
}

PubSub::~PubSub()

{
	freePubsubArea();
}

void PubSub::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PubSub)
	DDX_Text(pDX, IDC_PS_CONNECT_Q, m_ps_connect_q);
	DDV_MaxChars(pDX, m_ps_connect_q, 48);
	DDX_CBString(pDX, IDC_PS_CONNECT_QM, m_ps_connect_qm);
	DDV_MaxChars(pDX, m_ps_connect_qm, 48);
	DDX_Text(pDX, IDC_PUB_ERR_MSG, m_ps_errmsg);
	DDX_Text(pDX, IDC_PUB_TOPIC1, m_ps_topic1);
	DDX_Text(pDX, IDC_PUB_TOPIC2, m_ps_topic2);
	DDX_Text(pDX, IDC_PUB_TOPIC3, m_ps_topic3);
	DDX_Text(pDX, IDC_PUB_TOPIC4, m_ps_topic4);
	DDX_Text(pDX, IDC_PUB_Q, m_ps_q);
	DDV_MaxChars(pDX, m_ps_q, 48);
	DDX_Text(pDX, IDC_PUB_QM, m_ps_qm);
	DDV_MaxChars(pDX, m_ps_qm, 48);
	DDX_Text(pDX, IDC_PUB_FILTER, m_ps_filter);
	DDX_Text(pDX, IDC_PUB_BROKER_QM, m_ps_broker_qm);
	DDV_MaxChars(pDX, m_ps_broker_qm, 48);
	DDX_Text(pDX, IDC_PUB_SUBPOINT, m_ps_subpoint);
	DDX_Text(pDX, IDC_PUB_PUBTIME, m_ps_pubtime);
	DDX_Text(pDX, IDC_PUB_SEQNO, m_ps_seqno);
	DDX_Text(pDX, IDC_PUB_SUB_DATA, m_ps_sub_data);
	DDX_Text(pDX, IDC_PUB_SUB_IDENTITY, m_ps_sub_identity);
	DDV_MaxChars(pDX, m_ps_sub_identity, 64);
	DDX_Text(pDX, IDC_PUB_SUB_NAME, m_ps_sub_name);
	DDX_Radio(pDX, IDC_PUB_ASPUB, m_ps_persist);
	DDX_Radio(pDX, IDC_REGISTER, m_ps_reqtype);
	DDX_Check(pDX, IDC_PUB_INFORM_RETAIN, m_inform_if_retained);
	DDX_Check(pDX, IDC_PUB_ISRETAINED, m_ps_isretained);
	DDX_Check(pDX, IDC_PUB_CORRELASID, m_ps_correlasid);
	DDX_Check(pDX, IDC_PUB_LOCAL, m_ps_local);
	DDX_Check(pDX, IDC_PUB_NEWONLY, m_ps_newonly);
	DDX_Check(pDX, IDC_PUB_ONDEMAND, m_ps_ondemand);
	DDX_Check(pDX, IDC_PUB_DEREGALL, m_ps_deregall);
	DDX_Check(pDX, IDC_PUB_RETAIN, m_ps_retain);
	DDX_Check(pDX, IDC_PUB_OTHERONLY, m_ps_otheronly);
	DDX_Check(pDX, IDC_PUB_VAR_USER, m_ps_var_user);
	DDX_Check(pDX, IDC_PUB_NOALTER, m_ps_noalter);
	DDX_Check(pDX, IDC_PUB_LOCKED, m_ps_locked);
	DDX_Check(pDX, IDC_PUB_JOIN_SHARED, m_ps_join_shared);
	DDX_Check(pDX, IDC_PUB_JOIN_EXCL, m_ps_join_excl);
	DDX_Check(pDX, IDC_PUB_FULL_RESP, m_ps_full_resp);
	DDX_Check(pDX, IDC_PUB_ADD_NAME, m_ps_add_name);
	DDX_Check(pDX, IDC_PUB_INCL_STREAM_NAME, m_ps_incl_stream_name);
	DDX_Check(pDX, IDC_PUB_LEAVE_ONLY, m_ps_leave_only);
	DDX_Check(pDX, IDC_PUB_NO_REG, m_ps_no_reg);
	DDX_Check(pDX, IDC_PUB_DIRECT_REQ, m_ps_direct_req);
	DDX_Check(pDX, IDC_PUB_DUPS_OK, m_ps_dups_ok);
	DDX_Check(pDX, IDC_PUB_ANON, m_ps_anon);
	DDX_Text(pDX, IDC_PUB_OTHER, m_ps_other);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(PubSub, CPropertyPage)
	//{{AFX_MSG_MAP(PubSub)
	ON_BN_CLICKED(IDC_REGISTER, OnRegister)
	ON_BN_CLICKED(IDC_REQ_PUB, OnReqPub)
	ON_BN_CLICKED(IDC_PUBLISH, OnPublish)
	ON_BN_CLICKED(IDC_UNREGISTER, OnUnregister)
	ON_BN_CLICKED(IDC_DELETE_PUB, OnDeletePub)
	ON_BN_CLICKED(IDC_PUB_PROCESS, OnPubProcess)
	ON_BN_CLICKED(IDC_PUB_CLEAR, OnPubClear)
	ON_BN_CLICKED(IDC_PUB_ASPUB, OnPubAspub)
	ON_EN_CHANGE(IDC_PUB_BROKER_QM, OnChangePubBrokerQm)
	ON_BN_CLICKED(IDC_PUB_CORRELASID, OnPubCorrelasid)
	ON_BN_CLICKED(IDC_PUB_DEREGALL, OnPubDeregall)
	ON_EN_CHANGE(IDC_PUB_FILTER, OnChangePubFilter)
	ON_BN_CLICKED(IDC_PUB_INFORM_RETAIN, OnPubInformRetain)
	ON_BN_CLICKED(IDC_PUB_LOCAL, OnPubLocal)
	ON_BN_CLICKED(IDC_PUB_ONDEMAND, OnPubOndemand)
	ON_BN_CLICKED(IDC_PUB_OTHERONLY, OnPubOtheronly)
	ON_EN_CHANGE(IDC_PUB_PUBTIME, OnChangePubPubtime)
	ON_EN_CHANGE(IDC_PUB_Q, OnChangePubQ)
	ON_EN_CHANGE(IDC_PUB_QM, OnChangePubQm)
	ON_BN_CLICKED(IDC_PUB_RETAIN, OnPubRetain)
	ON_EN_CHANGE(IDC_PUB_SEQNO, OnChangePubSeqno)
	ON_EN_CHANGE(IDC_PUB_SUBPOINT, OnChangePubSubpoint)
	ON_EN_CHANGE(IDC_PUB_TOPIC1, OnChangePubTopic1)
	ON_EN_CHANGE(IDC_PUB_TOPIC2, OnChangePubTopic2)
	ON_EN_CHANGE(IDC_PUB_TOPIC3, OnChangePubTopic3)
	ON_EN_CHANGE(IDC_PUB_TOPIC4, OnChangePubTopic4)
	ON_BN_CLICKED(IDC_PUBASQUEUE, OnPubasqueue)
	ON_BN_CLICKED(IDC_PUBNONPERS, OnPubnonpers)
	ON_BN_CLICKED(IDC_PUBPERSIST, OnPubpersist)
	ON_BN_CLICKED(IDC_SAVE_REQ_DATA, OnSaveReqData)
	ON_CBN_DROPDOWN(IDC_PS_CONNECT_QM, OnDropdownPsConnectQm)
	ON_CBN_SELENDCANCEL(IDC_PS_CONNECT_QM, OnSelendcancelPsConnectQm)
	ON_CBN_SELCHANGE(IDC_PS_CONNECT_QM, OnSelchangePsConnectQm)
	ON_EN_CHANGE(IDC_PS_CONNECT_Q, OnChangePsConnectQ)
	ON_BN_CLICKED(IDC_PUB_ISRETAINED, OnPubIsretained)
	ON_BN_CLICKED(IDC_REG_PUB, OnRegPub)
	ON_BN_CLICKED(IDC_DEREG_PUB, OnDeregPub)
	ON_BN_CLICKED(IDC_PUB_FULL_RESP, OnPubFullResp)
	ON_BN_CLICKED(IDC_PUB_JOIN_EXCL, OnPubJoinExcl)
	ON_BN_CLICKED(IDC_PUB_JOIN_SHARED, OnPubJoinShared)
	ON_BN_CLICKED(IDC_PUB_LOCKED, OnPubLocked)
	ON_EN_CHANGE(IDC_PUB_SUB_DATA, OnChangePubSubData)
	ON_EN_CHANGE(IDC_PUB_SUB_IDENTITY, OnChangePubSubIdentity)
	ON_EN_CHANGE(IDC_PUB_SUB_NAME, OnChangePubSubName)
	ON_BN_CLICKED(IDC_PUB_NOALTER, OnPubNoalter)
	ON_BN_CLICKED(IDC_PUB_VAR_USER, OnPubVarUser)
	ON_BN_CLICKED(IDC_PUB_ADD_NAME, OnPubAddName)
	ON_BN_CLICKED(IDC_PUB_DIRECT_REQ, OnPubDirectReq)
	ON_BN_CLICKED(IDC_PUB_DUPS_OK, OnPubDupsOk)
	ON_BN_CLICKED(IDC_PUB_INCL_STREAM_NAME, OnPubInclStreamName)
	ON_BN_CLICKED(IDC_PUB_LEAVE_ONLY, OnPubLeaveOnly)
	ON_BN_CLICKED(IDC_PUB_NO_REG, OnPubNoReg)
	ON_BN_CLICKED(IDC_PUB_ANON, OnPubAnon)
	ON_EN_CHANGE(IDC_PUB_OTHER, OnChangePubOther)
	ON_CBN_SETFOCUS(IDC_PS_CONNECT_QM, OnSetfocusPsConnectQm)
	ON_EN_SETFOCUS(IDC_PUB_QM, OnSetfocusPubQm)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, PubSub::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, PubSub::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PubSub message handlers

void PubSub::setRegister()

{
	// Enable and disable the appropriate controls
	if (pDoc->traceEnabled)
	{
		// trace entry to setRegister
		pDoc->logTraceEntry("Entering PubSub::setRegister()");
	}

	((CEdit *)GetDlgItem(IDC_PUB_TOPIC1))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC2))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC3))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC4))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUBPOINT))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_FILTER))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_PUBTIME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SEQNO))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_BROKER_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_Q))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_NAME))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_IDENTITY))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_DATA))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_OTHER))->SetReadOnly(FALSE);
	//((CListBox *)GetDlgItem(IDC_PS_CONNECT_QM))->EnableWindow(TRUE);
	m_ps_qmComboBox.EnableWindow(TRUE);
	((CEdit *)GetDlgItem(IDC_PS_CONNECT_Q))->SetReadOnly(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_LOCAL))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_NEWONLY))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_OTHERONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ONDEMAND))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_CORRELASID))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DEREGALL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INFORM_RETAIN))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_ISRETAINED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_FULL_RESP))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_SHARED))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_EXCL))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_ADD_NAME))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_NOALTER))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_VAR_USER))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DIRECT_REQ))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DUPS_OK))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_INCL_STREAM_NAME))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_LEAVE_ONLY))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_NO_REG))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LOCKED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ANON))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBPERSIST))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBNONPERS))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_ASPUB))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBASQUEUE))->EnableWindow(TRUE);
}

void PubSub::OnRegister()
 
{
	// Handle setting of the Register subscription radio button
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnRegister() GetDlgItem(IDC_REGISTER))->GetCheck()=%d", ((CButton *)GetDlgItem(IDC_REGISTER))->GetCheck());

		// trace entry to OnRegister
		pDoc->logTraceEntry(traceInfo);
	}

	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_REGISTER))->GetCheck() == 1)
	{
		// handle a register subscription request
		setRegister();
		setConnectionQueue();
		setRFHV2();
	}
}

void PubSub::setReqPub()

{
	// Enable and disable the appropriate controls
	if (pDoc->traceEnabled)
	{
		// trace entry to setReqPub
		pDoc->logTraceEntry("Entering PubSub::setReqPub()");
	}

	((CEdit *)GetDlgItem(IDC_PUB_TOPIC1))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC2))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC3))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC4))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUBPOINT))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_FILTER))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_PUBTIME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SEQNO))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_BROKER_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_Q))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_NAME))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_IDENTITY))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_DATA))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_OTHER))->SetReadOnly(FALSE);
	//((CListBox *)GetDlgItem(IDC_PS_CONNECT_QM))->EnableWindow(TRUE);
	m_ps_qmComboBox.EnableWindow(TRUE);
	((CEdit *)GetDlgItem(IDC_PS_CONNECT_Q))->SetReadOnly(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_LOCAL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NEWONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_OTHERONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ONDEMAND))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_CORRELASID))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DEREGALL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INFORM_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ISRETAINED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_FULL_RESP))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_SHARED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_EXCL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ADD_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NOALTER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_VAR_USER))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DIRECT_REQ))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DUPS_OK))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INCL_STREAM_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LEAVE_ONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NO_REG))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ANON))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LOCKED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBPERSIST))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBNONPERS))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_ASPUB))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBASQUEUE))->EnableWindow(TRUE);
}

void PubSub::OnReqPub() 

{
	// Handle setting of the Request publication radio button
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnReqPub() GetDlgItem(IDC_REQ_PUB))->GetCheck()=%d", ((CButton *)GetDlgItem(IDC_REQ_PUB))->GetCheck());

		// trace entry to OnReqPub
		pDoc->logTraceEntry(traceInfo);
	}

	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_REQ_PUB))->GetCheck() == 1)
	{
		// handle a request publication request
		setReqPub();
		setConnectionQueue();
		setRFHV2();
	}
}

void PubSub::setPublish()

{
	// Enable and disable the appropriate controls
	if (pDoc->traceEnabled)
	{
		// trace entry to setPublish
		pDoc->logTraceEntry("Entering PubSub::setPublish()");
	}

	((CEdit *)GetDlgItem(IDC_PUB_TOPIC1))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC2))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC3))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC4))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUBPOINT))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_FILTER))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_PUBTIME))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SEQNO))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_BROKER_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_Q))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_NAME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_IDENTITY))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_DATA))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_OTHER))->SetReadOnly(FALSE);
	//((CListBox *)GetDlgItem(IDC_PS_CONNECT_QM))->EnableWindow(TRUE);
	m_ps_qmComboBox.EnableWindow(TRUE);
	((CEdit *)GetDlgItem(IDC_PS_CONNECT_Q))->SetReadOnly(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LOCAL))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_NEWONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_OTHERONLY))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_ONDEMAND))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_RETAIN))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_CORRELASID))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DEREGALL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INFORM_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ISRETAINED))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_FULL_RESP))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_SHARED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_EXCL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ADD_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NOALTER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_VAR_USER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DIRECT_REQ))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DUPS_OK))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INCL_STREAM_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LEAVE_ONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NO_REG))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_LOCKED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ANON))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBPERSIST))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBNONPERS))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_ASPUB))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBASQUEUE))->EnableWindow(TRUE);
}

void PubSub::OnPublish() 

{
	// Handle setting of the publish radio button
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnPublish() GetDlgItem(IDC_PUBLISH))->GetCheck()=%d", ((CButton *)GetDlgItem(IDC_PUBLISH))->GetCheck());

		// trace entry to OnPublish
		pDoc->logTraceEntry(traceInfo);
	}

	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_PUBLISH))->GetCheck() == 1)
	{
		// handle a publish request
		setPublish();
		setRFHV2();
	}
}

void PubSub::setUnregister()

{
	// Enable and disable the appropriate controls
	if (pDoc->traceEnabled)
	{
		// trace entry to setUnregister
		pDoc->logTraceEntry("Entering PubSub::setUnregister()");
	}

	((CEdit *)GetDlgItem(IDC_PUB_TOPIC1))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC2))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC3))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC4))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUBPOINT))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_FILTER))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_PUBTIME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SEQNO))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_BROKER_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_Q))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_NAME))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_IDENTITY))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_DATA))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_OTHER))->SetReadOnly(FALSE);
	//((CListBox *)GetDlgItem(IDC_PS_CONNECT_QM))->EnableWindow(TRUE);
	m_ps_qmComboBox.EnableWindow(TRUE);
	((CEdit *)GetDlgItem(IDC_PS_CONNECT_Q))->SetReadOnly(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_LOCAL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NEWONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_OTHERONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ONDEMAND))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_CORRELASID))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DEREGALL))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_INFORM_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ISRETAINED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_FULL_RESP))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_SHARED))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_EXCL))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_ADD_NAME))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_NOALTER))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_VAR_USER))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DIRECT_REQ))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DUPS_OK))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INCL_STREAM_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LEAVE_ONLY))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_NO_REG))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ANON))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LOCKED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBPERSIST))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBNONPERS))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_ASPUB))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBASQUEUE))->EnableWindow(TRUE);
}

void PubSub::OnUnregister() 

{
	// Handle setting of the unsubscribe radio button
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnUnregister() GetDlgItem(IDC_UNREGISTER))->GetCheck()=%d", ((CButton *)GetDlgItem(IDC_UNREGISTER))->GetCheck());

		// trace entry to OnUnregister
		pDoc->logTraceEntry(traceInfo);
	}

	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_UNREGISTER))->GetCheck() == 1)
	{
		// handle an unsubscribe request
		setUnregister();
		setConnectionQueue();
		setRFHV2();
	}
}

void PubSub::setDeletePub()

{
	// Enable and disable the appropriate controls
	if (pDoc->traceEnabled)
	{
		// trace entry to setDeletePub
		pDoc->logTraceEntry("Entering PubSub::setDeletePub()");
	}

	((CEdit *)GetDlgItem(IDC_PUB_TOPIC1))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC2))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC3))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC4))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUBPOINT))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_FILTER))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_PUBTIME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SEQNO))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_BROKER_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_Q))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_NAME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_IDENTITY))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_DATA))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_OTHER))->SetReadOnly(FALSE);
	//((CListBox *)GetDlgItem(IDC_PS_CONNECT_QM))->EnableWindow(TRUE);
	m_ps_qmComboBox.EnableWindow(TRUE);
	((CEdit *)GetDlgItem(IDC_PS_CONNECT_Q))->SetReadOnly(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LOCAL))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_NEWONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_OTHERONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ONDEMAND))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_CORRELASID))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DEREGALL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INFORM_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ISRETAINED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_FULL_RESP))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_SHARED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_EXCL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ADD_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NOALTER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_VAR_USER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DIRECT_REQ))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DUPS_OK))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INCL_STREAM_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LEAVE_ONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NO_REG))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ANON))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LOCKED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBPERSIST))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBNONPERS))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ASPUB))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBASQUEUE))->EnableWindow(FALSE);
}

void PubSub::OnDeletePub() 

{
	// Handle setting of the delete retained publication radio button
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnDeletePub() GetDlgItem(IDC_DELETE_PUB))->GetCheck()=%d", ((CButton *)GetDlgItem(IDC_DELETE_PUB))->GetCheck());

		// trace entry to OnDeletePub
		pDoc->logTraceEntry(traceInfo);
	}

	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_DELETE_PUB))->GetCheck() == 1)
	{
		// handle a delete publication request
		setDeletePub();
		setRFHV2();
	}
}

void PubSub::setRegPub()

{
	// Enable and disable the appropriate controls
	if (pDoc->traceEnabled)
	{
		// trace entry to setRegPub
		pDoc->logTraceEntry("Entering PubSub::setRegPub()");
	}

	((CEdit *)GetDlgItem(IDC_PUB_TOPIC1))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC2))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC3))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC4))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUBPOINT))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_FILTER))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_PUBTIME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SEQNO))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_BROKER_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_Q))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_NAME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_IDENTITY))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_DATA))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_OTHER))->SetReadOnly(FALSE);
	//((CListBox *)GetDlgItem(IDC_PS_CONNECT_QM))->EnableWindow(TRUE);
	m_ps_qmComboBox.EnableWindow(TRUE);
	((CEdit *)GetDlgItem(IDC_PS_CONNECT_Q))->SetReadOnly(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_LOCAL))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_NEWONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_OTHERONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ONDEMAND))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_CORRELASID))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DEREGALL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INFORM_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ISRETAINED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_FULL_RESP))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_SHARED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_EXCL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ADD_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NOALTER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_VAR_USER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DIRECT_REQ))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DUPS_OK))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INCL_STREAM_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LEAVE_ONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NO_REG))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LOCKED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ANON))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUBPERSIST))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBNONPERS))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ASPUB))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBASQUEUE))->EnableWindow(FALSE);
}

void PubSub::OnRegPub() 

{
	// Handle setting of the register publisher radio button
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnRegPub() GetDlgItem(IDC_REG_PUB))->GetCheck()=%d", ((CButton *)GetDlgItem(IDC_REG_PUB))->GetCheck());

		// trace entry to OnRegPub
		pDoc->logTraceEntry(traceInfo);
	}

	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_REG_PUB))->GetCheck() == 1)
	{
		// handle a register publisher request - only supported for RFH V1 and the MQ broker
		setRegPub();
		setConnectionQueue();
		setRFHV1();
	}
}

void PubSub::setUnregPub()

{
	// Enable and disable the appropriate controls
	if (pDoc->traceEnabled)
	{
		// trace entry to setUnregPub
		pDoc->logTraceEntry("Entering PubSub::setUnregPub()");
	}

	((CEdit *)GetDlgItem(IDC_PUB_TOPIC1))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC2))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC3))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC4))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUBPOINT))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_FILTER))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_PUBTIME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SEQNO))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_BROKER_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_QM))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_Q))->SetReadOnly(FALSE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_NAME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_IDENTITY))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_DATA))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_OTHER))->SetReadOnly(FALSE);
	//((CListBox *)GetDlgItem(IDC_PS_CONNECT_QM))->EnableWindow(TRUE);
	m_ps_qmComboBox.EnableWindow(TRUE);
	((CEdit *)GetDlgItem(IDC_PS_CONNECT_Q))->SetReadOnly(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_LOCAL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NEWONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_OTHERONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ONDEMAND))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_CORRELASID))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_DEREGALL))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_INFORM_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ISRETAINED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_FULL_RESP))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_SHARED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_EXCL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ADD_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NOALTER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_VAR_USER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DIRECT_REQ))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DUPS_OK))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INCL_STREAM_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LEAVE_ONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NO_REG))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LOCKED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ANON))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBPERSIST))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBNONPERS))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ASPUB))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBASQUEUE))->EnableWindow(FALSE);
}

void PubSub::OnDeregPub() 

{
	// Handle setting of the unregister publisher radio button
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnDeregPub() GetDlgItem(IDC_DEREG_PUB))->GetCheck()=%d", ((CButton *)GetDlgItem(IDC_DEREG_PUB))->GetCheck());

		// trace entry to OnDeregPub
		pDoc->logTraceEntry(traceInfo);
	}

	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_DEREG_PUB))->GetCheck() == 1)
	{
		// handle an unregister publisher request - only supported by MQ broker and RFH V1
		setUnregPub();
		setConnectionQueue();
		setRFHV1();
	}
}

void PubSub::setNone()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to setNone
		pDoc->logTraceEntry("Entering PubSub::setNone()");
	}

	((CEdit *)GetDlgItem(IDC_PUB_TOPIC1))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC2))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC3))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_TOPIC4))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_FILTER))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUBPOINT))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_NAME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_IDENTITY))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_DATA))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_PUBTIME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SEQNO))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_BROKER_QM))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_QM))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_Q))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_NAME))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_IDENTITY))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_SUB_DATA))->SetReadOnly(TRUE);
	((CEdit *)GetDlgItem(IDC_PUB_OTHER))->SetReadOnly(TRUE);
	//((CListBox *)GetDlgItem(IDC_PS_CONNECT_QM))->EnableWindow(FALSE);
	m_ps_qmComboBox.EnableWindow(FALSE);
	((CEdit *)GetDlgItem(IDC_PS_CONNECT_Q))->SetReadOnly(TRUE);
	((CButton *)GetDlgItem(IDC_PUB_LOCAL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NEWONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_OTHERONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ONDEMAND))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_CORRELASID))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DEREGALL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INFORM_RETAIN))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ISRETAINED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_FULL_RESP))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_SHARED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_JOIN_EXCL))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ADD_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NOALTER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_VAR_USER))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DIRECT_REQ))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_DUPS_OK))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_INCL_STREAM_NAME))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LEAVE_ONLY))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_NO_REG))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_LOCKED))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ANON))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBPERSIST))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBNONPERS))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUB_ASPUB))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_PUBASQUEUE))->EnableWindow(FALSE);
}

void PubSub::OnSaveReqData() 

{
	int		rc;

	// get the form data into the instance variables
	UpdateData(TRUE);
	
	savePageData();

	if (m_ps_reqtype != -1)
	{
		// invoke standard dialog to choose file name
		CFileDialog fd(FALSE, NULL, NULL, OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT);
		rc = fd.DoModal();

		//	Save the copy book file name 
		if (rc == IDOK)
		{
			// set the full file name in the DataArea object
			strcpy(pDoc->fileName, fd.GetPathName( ));

			// drive the document file processing
			pDoc->WriteFile(fd.GetPathName());
			updateErrmsg();
		}
	}

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void PubSub::OnPubProcess() 

{
	// Handle setting of the unregister publisher radio button
	char	traceInfo[256];		// work variable to build trace message

	// get the form data into the instance variables
	UpdateData(TRUE);
	
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnPubProcess() m_ps_reqtype=%d m_ps_connect_qm=%s m_ps_broker_qm=%s m_ps_connect_q=%s", m_ps_reqtype, (LPCTSTR)m_ps_connect_qm, (LPCTSTR)m_ps_broker_qm, (LPCTSTR)m_ps_connect_q);

		// trace entry to OnPubProcess
		pDoc->logTraceEntry(traceInfo);
	}

	// makd sure the data in the main dialog reflects any changes
	savePageData();

	if (m_ps_reqtype != -1)
	{
		// force on the switch to include the psc folder in the rfh header
		pDoc->setPubSubVersion();

		// put a message to the selected queue
		pDoc->putMessage(m_ps_connect_qm, m_ps_broker_qm, m_ps_connect_q);

		// update the message area with the results of the put
		updateErrmsg();
	}

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void PubSub::savePageData()

{
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::savePageData() pubsubDataChanged=%d m_ps_connect_qm=%s m_ps_broker_qm=%s m_ps_connect_q=%s", pubsubDataChanged, (LPCTSTR)m_ps_connect_qm, (LPCTSTR)m_ps_broker_qm, (LPCTSTR)m_ps_connect_q);

		// trace entry to savePageData
		pDoc->logTraceEntry(traceInfo);
	}

	// get the form data into the instance variables
	UpdateData(TRUE);
	
	// Update the control variables from the form
	if (pubsubDataChanged)
	{
		// these three lines are necessary so that the same fields on the main tab will be updated
		pDoc->m_QM_name = m_ps_connect_qm;
		pDoc->m_Q_name = m_ps_connect_q;
		pDoc->m_remote_QM = m_ps_broker_qm;

		pDoc->freeRfh1Area();
		pDoc->freeRfhArea();
		freePubsubArea();

		pubsubDataChanged = false;
	}

	if (pDoc->traceEnabled)
	{
		// trace exit from savePageData
		pDoc->logTraceEntry("Exiting PubSub::savePageData()");
	}
}

void PubSub::OnPubClear() 

{
	// clear the data in all of the controls
	if (pDoc->traceEnabled)
	{
		// trace entry to OnPubClear
		pDoc->logTraceEntry("Entering PubSub::OnPubClear()");
	}

	m_ps_topic1 = _T("");
	m_ps_topic2 = _T("");
	m_ps_topic3 = _T("");
	m_ps_topic4 = _T("");
	m_ps_q = _T("");
	m_ps_qm = _T("");
	m_ps_broker_qm = _T("");
	m_ps_connect_qm = _T("");
	m_ps_connect_q = _T("");
	m_ps_sub_data = _T("");
	m_ps_sub_identity = _T("");
	m_ps_sub_name = _T("");
	m_ps_other = _T("");
	m_ps_correlasid = FALSE;
	m_ps_retain = FALSE;
	m_ps_local = FALSE;
	m_ps_newonly = FALSE;
	m_ps_ondemand = FALSE;
	m_ps_otheronly = FALSE;
	m_ps_persist = PS_PERSIST_NONE;
	m_ps_reqtype = -1;
	m_ps_filter = _T("");
	m_ps_subpoint = _T("");
	m_ps_deregall = FALSE;
	m_ps_pubtime = _T("");
	m_ps_seqno = _T("");
	m_inform_if_retained = FALSE;
	m_ps_var_user = FALSE;
	m_ps_noalter = FALSE;
	m_ps_locked = FALSE;
	m_ps_join_shared = FALSE;
	m_ps_join_excl = FALSE;
	m_ps_full_resp = FALSE;
	m_ps_add_name = FALSE;
	m_ps_incl_stream_name = FALSE;
	m_ps_leave_only = FALSE;
	m_ps_no_reg = FALSE;
	m_ps_direct_req = FALSE;
	m_ps_dups_ok = FALSE;
	m_ps_anon = FALSE;

	setNone();
	pubsubDataChanged = true;

	// update the form data from the instance variables
	UpdateData(FALSE);
}

BOOL PubSub::OnKillActive() 
{
	char	traceInfo[512];		// work variable to build trace message

	// user has selected another dialog (tab)
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnKillActive() pubsubDataChanged=%d", pubsubDataChanged);

		// trace entry to OnKillActive
		pDoc->logTraceEntry(traceInfo);
	}

	// update the instance variables from the form
	UpdateData(TRUE);
	
	// update the data in the main dialog and in the RFH dialog
	savePageData();

	return CPropertyPage::OnKillActive();
}

BOOL PubSub::OnSetActive() 

{
	// user has selected this tab
	if (pDoc->traceEnabled)
	{
		// trace entry to OnSetActive
		pDoc->logTraceEntry("Entering PubSub::OnSetActive()");
	}

	// make sure the form reflects the latest data
	updatePageData();

	BOOL ret = CPropertyPage::OnSetActive();
	
	// send a custom message to switch the focus to the QM combo box
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return ret;
}

void PubSub::OnPubAspub() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubBrokerQm() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubCorrelasid() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubDeregall() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubFilter() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubInformRetain() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubLocal() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubOndemand() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubOtheronly() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubPerNone() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubPubtime() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubQ() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubQm() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubRetain() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubSeqno() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubSubpoint() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubTopic1() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubTopic2() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubTopic3() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubTopic4() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubasqueue() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubnonpers() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubpersist() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnDropdownPsConnectQm() 

{
	// load the queue manager names into the combo box drop down list
	loadEditBox();	
}

void PubSub::loadEditBox()

{
	// populate the drop down list of queue manager names
	// this routine is located in the main application class
	pDoc->loadQMComboBox((CComboBox *)&m_ps_qmComboBox);
}

void PubSub::OnSelendcancelPsConnectQm() 
{
	// restore the previous setting of the combo box
	//((CComboBox *)GetDlgItem(IDC_PS_CONNECT_QM))->SetWindowText(m_ps_connect_qm);
	m_ps_qmComboBox.SetWindowText((LPCTSTR)m_ps_connect_qm);
}

void PubSub::OnSelchangePsConnectQm() 

{
	int		index;
	int		len;
	char	qmName[256];

	UpdateData (TRUE);

	qmName[0] = 0;

	//index = ((CComboBox *)GetDlgItem(IDC_PS_CONNECT_QM))->GetCurSel();
	index = m_ps_qmComboBox.GetCurSel();
	//len = ((CComboBox *)GetDlgItem(IDC_PS_CONNECT_QM))->GetLBText(index, qmName);
	len = m_ps_qmComboBox.GetLBText(index, qmName);
	if (len > 0)
	{
		m_ps_connect_qm = qmName;
	}
	else
	{
		m_ps_connect_qm.Empty();
	}

	// set the value in the edit box
	//((CComboBox *)GetDlgItem(IDC_PS_CONNECT_QM))->SetWindowText(m_ps_connect_qm);
	m_ps_qmComboBox.SetWindowText((LPCTSTR)m_ps_connect_qm);

	// save the QM name in the document so the Show Cluster menu works in RFHUtilView
	pDoc->m_QM_name = m_ps_connect_qm;

	// Update the controls
	UpdateData (FALSE);
}

void PubSub::setConnectionQueue()
{
	// Update the control variables, to make sure we have the queue name and QM names correctly
	UpdateData (TRUE);

	m_ps_connect_q = "SYSTEM.BROKER.CONTROL.QUEUE";

	// update the queue name on the Main tab as well if the focus is changed
	pubsubDataChanged = true;

	// Update the controls
	UpdateData (FALSE);
}

void PubSub::updateErrmsg()

{
	if (pDoc->m_error_msg.GetLength() > 0)
	{
		pDoc->updateMsgText();
	}

	m_ps_errmsg = pDoc->m_msg_text;
}

BOOL PubSub::OnInitDialog() 

{
	CPropertyPage::OnInitDialog();
	
	// tool tips are used in this dialog and must be initialized
	EnableToolTips(TRUE);
	
	// use the special MyComboBox subclass for the queue manager combo box
	m_ps_qmComboBox.SubclassDlgItem(IDC_PS_CONNECT_QM, this);
	loadEditBox();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL PubSub::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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
			else
			{
//				pTTTW->lpszText = MAKEINTRESOURCE(nID);
//				pTTTW->hinst = AfxGetResourceHandle();
			}

			pResult = 0;
			return(TRUE);
		}
	}

	return(FALSE);
}

BOOL PubSub::PreTranslateMessage(MSG* pMsg) 

{
	// necessary override to allow tool tips to work
	if (pMsg->message != WM_KEYDOWN)
		return CPropertyPage::PreTranslateMessage(pMsg);

	// check for a backspace key
	if (VK_BACK == pMsg->wParam)
	{
		CWnd * curFocus = GetFocus();
		if (curFocus != NULL)
		{
			int id = curFocus->GetDlgCtrlID();

			// check if this is an edit box control
			if ((IDC_PUB_TOPIC1 == id) || 
				(IDC_PUB_TOPIC2 == id) || 
				(IDC_PUB_TOPIC3 == id) || 
				(IDC_PUB_TOPIC4 == id) || 
				(IDC_PUB_SUBPOINT == id) || 
				(IDC_PUB_SUB_NAME == id) || 
				(IDC_PUB_SUB_IDENTITY == id) || 
				(IDC_PUB_SUB_DATA == id) || 
				(IDC_PS_CONNECT_Q == id) || 
				(IDC_PUB_BROKER_QM == id) || 
				(IDC_PUB_QM == id) || 
				(IDC_PUB_Q == id) || 
				(IDC_PUB_PUBTIME == id) || 
				(IDC_PUB_OTHER == id) || 
				(IDC_PUB_SEQNO == id))
			{
				processBackspace(curFocus);
				pubsubDataChanged = true;
				return TRUE;
			}
		}
	}

	if (IsDialogMessage(pMsg))
		return TRUE;
	else
		return CPropertyPage::PreTranslateMessage(pMsg);
}

void PubSub::setRFHV1()

{
	pDoc->setRFHV1();
	freePubsubArea();
}

void PubSub::setRFHV2()

{
	pDoc->setRFHV2();
}

void PubSub::OnChangePsConnectQ()
 
{
	// remember the new setting of the combo box
	//((CComboBox *)GetDlgItem(IDC_PS_CONNECT_QM))->GetWindowText(m_ps_connect_qm);
	m_ps_qmComboBox.GetWindowText(m_ps_connect_qm);

	// make sure the QM name in the document tracks the QM name in this dialog
	// save the QM name in the document so the Show Cluster menu works in RFHUtilView
	pDoc->m_QM_name = m_ps_connect_qm;

	pubsubDataChanged = true;
}

void PubSub::OnPubIsretained() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

/////////////////////////////////////////////////
//
// This routine is called to indicate that the
// instance variables may have changed and
// that the dialog needs to be refreshed.
//
// It is called either when the pubsub page is
// selected by the user or called when a menu
// item is selected that could have changed the
// RFH2 data (such as reading a file).
//
/////////////////////////////////////////////////

void PubSub::updatePageData()

{
	char	traceInfo[256];		// work variable to build trace message

	// user has selected another dialog (tab)
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::updatePageData() m_ps_reqtype=%d, m_ps_connect_qm=%s, m_ps_connect_q=%s", m_ps_reqtype, (LPCTSTR)m_ps_connect_qm, (LPCTSTR)m_ps_connect_q);

		// trace entry to updatePageData
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure the queue manager and queue names reflect the values in the main page
	m_ps_connect_qm = pDoc->m_QM_name;
	m_ps_connect_q = pDoc->m_Q_name;
	m_ps_broker_qm = pDoc->m_remote_QM;

	// check if there are any error messages to display
	if (pDoc->m_error_msg.GetLength() > 0)
	{
		pDoc->updateMsgText();
	}

	m_ps_errmsg = pDoc->m_msg_text;

	// update the form data from the instance variables
	UpdateData(FALSE);

	switch (m_ps_reqtype)
	{
	case PS_REQ_REGISTER:
		{
			setRegister();
			break;
		}
	case PS_REQ_UNREG:
		{
			setUnregister();
			break;
		}
	case PS_REQ_PUBLISH:
		{
			setPublish();
			break;
		}
	case PS_REQ_REQPUB:
		{
			setReqPub();
			break;
		}
	case PS_REQ_DELPUB:
		{
			setDeletePub();
			break;
		}
	case PS_REQ_REG_PUB:
		{
			setRegPub();
			break;
		}
	case PS_REQ_UNREG_PUB:
		{
			setUnregPub();
			break;
		}
	default:
		{
			setNone();
			break;
		}
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting PubSub::updatePageData() m_ps_reqtype=%d, m_ps_connect_qm=%s, m_ps_connect_q=%s", m_ps_reqtype, (LPCTSTR)m_ps_connect_qm, (LPCTSTR)m_ps_connect_q);

		// trace exit from updatePageData
		pDoc->logTraceEntry(traceInfo);
	}
}

void PubSub::OnPubFullResp() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubJoinExcl() 

{
	// turn off join shared
	// Update the control variables from the form
	UpdateData (TRUE);
	
	// Pubsub Join Exclusive and shared are mutually exclusive
	// only one can be selected at a time
	m_ps_join_shared = FALSE;

	// data in the controls has been changed by the user
	pubsubDataChanged = true;

	UpdateData (FALSE);
}

void PubSub::OnPubJoinShared() 

{
	// turn off join exclusive
	// Update the control variables from the form
	UpdateData (TRUE);
	
	// Pubsub Join Exclusive and shared are mutually exclusive
	// only one can be selected at a time
	m_ps_join_excl = FALSE;

	// data in the controls has been changed by the user
	pubsubDataChanged = true;

	UpdateData (FALSE);
}

void PubSub::OnPubLocked() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubSubData() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubSubIdentity() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubSubName() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubNoalter() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubVarUser() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubAddName() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubDirectReq() 
{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubDupsOk() 
{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubInclStreamName() 
{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubLeaveOnly() 
{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubNoReg() 
{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnPubAnon() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnChangePubOther() 

{
	// data in the controls has been changed by the user
	pubsubDataChanged = true;
}

void PubSub::OnSetfocusPsConnectQm() 
{
	// set the maximum length to match either a client connection string or a queue manager name (direct connection)
#ifdef MQCLIENT
	//((CComboBox *)GetDlgItem(IDC_PS_CONNECT_QM))->LimitText(255);
	m_ps_qmComboBox.LimitText(255);
#else
	//((CComboBox *)GetDlgItem(IDC_PS_CONNECT_QM))->LimitText(MQ_Q_NAME_LENGTH);
	m_ps_qmComboBox.LimitText(MQ_Q_NAME_LENGTH);
#endif
}

void PubSub::OnSetfocusPubQm() 
{
	// set the maximum length to match either a client connection string or a queue manager name (direct connection)
#ifdef MQCLIENT
	((CComboBox *)GetDlgItem(IDC_PUB_QM))->LimitText(255);
#else
	((CComboBox *)GetDlgItem(IDC_PUB_QM))->LimitText(MQ_Q_NAME_LENGTH);
#endif
}

void PubSub::PubClear()

{
	OnPubClear();
}

void PubSub::PubProcess()

{
	OnPubProcess();
}

LONG PubSub::OnSetPageFocus(UINT wParam, LONG lParam)

{
	char	traceInfo[256];		// work variable to build trace message

	// user has selected another dialog (tab)
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::OnSetPageFocus() m_ps_reqtype=%d", m_ps_reqtype);

		// trace entry to OnSetPageFocus
		pDoc->logTraceEntry(traceInfo);
	}

	// set the focus to the appropriate radio button
	switch (m_ps_reqtype)
	{
	case PS_REQ_UNREG:
		{
			((CButton *)GetDlgItem(IDC_UNREGISTER))->SetFocus();
			break;
		}
	case PS_REQ_PUBLISH:
		{
			((CButton *)GetDlgItem(IDC_PUBLISH))->SetFocus();
			break;
		}
	case PS_REQ_REQPUB:
		{
			((CButton *)GetDlgItem(IDC_REQ_PUB))->SetFocus();
			break;
		}
	case PS_REQ_DELPUB:
		{
			((CButton *)GetDlgItem(IDC_DELETE_PUB))->SetFocus();
			break;
		}
	case PS_REQ_REG_PUB:
		{
			((CButton *)GetDlgItem(IDC_REG_PUB))->SetFocus();
			break;
		}
	case PS_REQ_UNREG_PUB:
		{
			((CButton *)GetDlgItem(IDC_DEREG_PUB))->SetFocus();
			break;
		}
	default:
		{
			((CButton *)GetDlgItem(IDC_REGISTER))->SetFocus();
			break;
		}
	}

	return 0;
}

void PubSub::setPubsubArea(unsigned char *pubsubData, int dataLen, int ccsid, int encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::setPubsubArea() rfh_pubsub_area=%8.8X m_RFH_pubsub_len=%d ccsid=%d encoding=%d", (unsigned int)rfh_pubsub_area, m_RFH_pubsub_len, ccsid, encoding);

		// trace entry to setPubsubArea
		pDoc->logTraceEntry(traceInfo);
	}

	// free any old area
	freePubsubArea();

	if ((pubsubData != NULL) && (dataLen > 0))
	{
		// allocate storage for the pubsub folder
		rfh_pubsub_area = (unsigned char *)rfhMalloc(dataLen + 5, "PUBAREA ");

		// copy the data to the allocated area and terminate it as a safeguard
		memcpy(rfh_pubsub_area, pubsubData, dataLen);
		rfh_pubsub_area[dataLen] = 0;
		rfh_pubsub_area[dataLen+1] = 0;			// in case of UCS-2 data

		// remember the length of the pubsub folder
		m_RFH_pubsub_len = dataLen;

		// remember the ccsid and encoding that were used to build this folder
		m_RFH_pubsub_ccsid = ccsid;
		m_RFH_pubsub_encoding = encoding;

		// indicate that the data is current
		pubsubDataChanged = false;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting PubSub::setPubsubArea() rfh_pubsub_area=%8.8X m_RFH_pubsub_len=%d", (unsigned int)rfh_pubsub_area, m_RFH_pubsub_len);

		// trace exit from setPubsubArea
		pDoc->logTraceEntry(traceInfo);
	}
}

void PubSub::freePubsubArea()

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::freePubsubArea() rfh_pubsub_area=%8.8X m_RFH_pubsub_len=%d m_RFH_pubsub_ccsid=%d m_RFH_pubsub_encoding=%d", (unsigned int)rfh_pubsub_area, m_RFH_pubsub_len, m_RFH_pubsub_ccsid, m_RFH_pubsub_encoding);

		// trace entry to freePubsubArea
		pDoc->logTraceEntry(traceInfo);
	}

	if (rfh_pubsub_area != NULL)
	{
		rfhFree(rfh_pubsub_area);
	}
		
	rfh_pubsub_area = NULL;
	m_RFH_pubsub_len = 0;
	m_RFH_pubsub_ccsid = -1;
	m_RFH_pubsub_encoding = -1;
}

const char * PubSub::getPubsubArea(int ccsid, int encoding)

{
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting from PubSub::getPubsubArea() rfh_pubsub_area=%8.8X m_RFH_pubsub_len=%d m_RFH_pubsub_ccsid=%d m_RFH_pubsub_encoding=%d", (unsigned int)rfh_pubsub_area, m_RFH_pubsub_len, m_RFH_pubsub_ccsid, m_RFH_pubsub_encoding);

		// trace entry to freePubsubArea
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure what we are returning is valid
	if ((NULL == rfh_pubsub_area) || pubsubDataChanged || (ccsid != m_RFH_pubsub_ccsid) || (encoding != m_RFH_pubsub_encoding))
	{
		buildPubsubArea(ccsid, encoding);
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting from PubSub::getPubsubArea() rfh_pubsub_area=%8.8X m_RFH_pubsub_len=%d m_RFH_pubsub_ccsid=%d m_RFH_pubsub_encoding=%d", (unsigned int)rfh_pubsub_area, m_RFH_pubsub_len, m_RFH_pubsub_ccsid, m_RFH_pubsub_encoding);

		// trace entry to freePubsubArea
		pDoc->logTraceEntry(traceInfo);
	}

	return (char *)rfh_pubsub_area;
}

///////////////////////////////////////////////////////
//
// Routine to parse the psc folder in an RFH2 header.
// This folder is used for publish and subscribe
// requests to the broker.
//
///////////////////////////////////////////////////////

void PubSub::parseRFH2pubsub(unsigned char *rfhdata, int dataLen)

{
	bool	more;
	int		found;
	int		idx=0;
	char	*ptr;
	char	*endptr;
	char	tempCommand[32];
	char	tempRegOpt[32];
	char	tempPubOpt[32];
	char	tempDelOpt[32];
	char	tempTopic1[MAX_PUBSUB_NAME];
	char	tempTopic2[MAX_PUBSUB_NAME];
	char	tempTopic3[MAX_PUBSUB_NAME];
	char	tempTopic4[MAX_PUBSUB_NAME];
	char	tempSubPoint[MAX_PUBSUB_NAME];
	char	tempFilter[MAX_PUBSUB_NAME];
	char	tempUser[16384];
	char	tempQMgrName[64];
	char	tempQName[64];
	char	tempPubTime[64];
	char	tempSeqNum[32];
	char	traceInfo[512];		// work variable to build trace message

	// make sure we have current info
	UpdateData(TRUE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::parseRFH2pubsub() rfhdata=%8.8X dataLen=%d", (unsigned int)rfhdata, dataLen);

		// trace entry to parseRFH2pubsub
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("pubsub rfhdata", rfhdata, dataLen);
	}

	// reset the current values
	m_ps_local = FALSE;
	m_ps_correlasid = FALSE;
	m_ps_newonly = FALSE;
	m_ps_ondemand = FALSE;
	m_ps_deregall = FALSE;
	m_ps_retain = FALSE;
	m_ps_otheronly = FALSE;
	m_inform_if_retained = FALSE;
	m_ps_persist = PS_PERSIST_NONE;

	// initialize fields
	memset(tempCommand, 0, sizeof(tempCommand));
	memset(tempRegOpt, 0, sizeof(tempRegOpt));
	memset(tempPubOpt, 0, sizeof(tempPubOpt));
	memset(tempDelOpt, 0, sizeof(tempDelOpt));
	memset(tempTopic1, 0, sizeof(tempTopic1));
	memset(tempTopic2, 0, sizeof(tempTopic2));
	memset(tempTopic3, 0, sizeof(tempTopic3));
	memset(tempTopic4, 0, sizeof(tempTopic4));
	memset(tempSubPoint, 0, sizeof(tempSubPoint));
	memset(tempFilter, 0, sizeof(tempFilter));
	memset(tempQMgrName, 0, sizeof(tempQMgrName));
	memset(tempQName, 0, sizeof(tempQName));
	memset(tempPubTime, 0, sizeof(tempPubTime));
	memset(tempSeqNum, 0, sizeof(tempSeqNum));
	memset(tempUser, 0, sizeof(tempUser));

	// Search for the RFH pub/sub fields in the pub/sub folder
	ptr = (char *)rfhdata + 5;

	// find the end of the pscr folder
	endptr = ptr;
	while ((endptr < ((char *)rfhdata + dataLen)) && 
		   (memcmp(endptr, MQRFH2_PUBSUB_CMD_FOLDER_E, sizeof(MQRFH2_PUBSUB_CMD_FOLDER_E) - 1)))
	{
		endptr++;
	}

	// find the initial begin bracket
	more = true;
	while (more)
	{
		// find the next begin bracket
		while ((ptr < endptr) && (ptr[0] != '<'))
		{
			ptr++;
		}

		// did we find what we were looking for?
		if ((ptr < endptr) && ('<' == ptr[0]))
		{
			if (strcmp(ptr, MQRFH2_PUBSUB_CMD_FOLDER_E) == 0)
			{
				more = false;
			}
			else
			{
				// check for an end tag
				if ('/' == ptr[1])
				{
					ptr += 4;
				}
				else
				{
					// see if we recognize the tag
					// check for a command
					found = 0;
					ptr = processTag(ptr, MQPSC_COMMAND_B, tempCommand, sizeof(tempCommand), &found);
					ptr = processTag(ptr, MQPSC_REGISTRATION_OPTION_B, tempRegOpt, sizeof(tempRegOpt), &found);
					ptr = processTag(ptr, MQPSC_PUBLICATION_OPTION_B, tempPubOpt, sizeof(tempPubOpt), &found);
					ptr = processTag(ptr, MQPSC_DELETE_OPTION_B, tempDelOpt, sizeof(tempDelOpt), &found);
					ptr = processTag(ptr, MQPSC_SUBSCRIPTION_POINT_B, tempSubPoint, sizeof(tempSubPoint), &found);
					ptr = processTag(ptr, MQPSC_FILTER_B, tempFilter, sizeof(tempFilter), &found);
					ptr = processTag(ptr, MQPSC_Q_MGR_NAME_B, tempQMgrName, sizeof(tempCommand), &found);
					ptr = processTag(ptr, MQPSC_Q_NAME_B, tempQName, sizeof(tempQName), &found);
					ptr = processTag(ptr, MQPSC_PUBLISH_TIMESTAMP_B, tempPubTime, sizeof(tempPubTime), &found);
					ptr = processTag(ptr, MQPSC_SEQUENCE_NUMBER_B, tempSeqNum, sizeof(tempSeqNum), &found);

					// process any registration, publication or delete options right away,
					// since they may be repeated many times
					processPubsubOption(tempRegOpt);
					processPubsubOption(tempPubOpt);
					processPubsubOption(tempDelOpt);

					if (0 == tempTopic1[0])
					{
						ptr = processTag(ptr, MQPSC_TOPIC_B, tempTopic1, sizeof(tempTopic1), &found);
					}
					else
					{
						if (0 == tempTopic2[0])
						{
							ptr = processTag(ptr, MQPSC_TOPIC_B, tempTopic2, sizeof(tempTopic2), &found);
						}
						else
						{
							if (0 == tempTopic3[0])
							{
								ptr = processTag(ptr, MQPSC_TOPIC_B, tempTopic3, sizeof(tempTopic3), &found);
							}
							else
							{
								ptr = processTag(ptr, MQPSC_TOPIC_B, tempTopic4, sizeof(tempTopic4), &found);
							}
						}
					}

					// move on to the next item
					// we can skip at least 3 characters assuming a valid tag name
					if (!found)
					{
						ptr = parseRFH2misc(ptr, endptr, tempUser, MQRFH2_PUBSUB_CMD_FOLDER_E, sizeof(tempUser) - 1, &idx);
					}
				}
			}
		}
		else
		{
			// no more brackets - time to leave
			more = false;
		}
	}

	// check for the command type
	m_ps_reqtype = -1;
	if (strcmp(tempCommand, MQPSC_DELETE_PUBLICATION) == 0)
	{
		m_ps_reqtype = PS_REQ_DELPUB;
	}

	if (strcmp(tempCommand, MQPSC_DEREGISTER_SUBSCRIBER) == 0)
	{
		m_ps_reqtype = PS_REQ_UNREG;
	}

	if (strcmp(tempCommand, MQPSC_PUBLISH) == 0)
	{
		m_ps_reqtype = PS_REQ_PUBLISH;
	}

	if (strcmp(tempCommand, MQPSC_REGISTER_SUBSCRIBER) == 0)
	{
		m_ps_reqtype = PS_REQ_REGISTER;
	}

	if (strcmp(tempCommand, MQPSC_REQUEST_UPDATE) == 0)
	{
		m_ps_reqtype = PS_REQ_REQPUB;
	}

	// capture the first four topic strings
	m_ps_topic1 = tempTopic1;
	m_ps_topic2 = tempTopic2;
	m_ps_topic3 = tempTopic3;
	m_ps_topic4 = tempTopic4;

	// capture the SubPoint type
	m_ps_subpoint = tempSubPoint;

	// capture the Filter type
	m_ps_filter = tempFilter;

	// capture the broker QM name
	m_ps_broker_qm = tempQMgrName;
	m_ps_qm = tempQMgrName;

	// capture the broker Q name
	m_ps_q = tempQName;

	// capture the publication time
	m_ps_pubtime = tempPubTime;

	// capture the sequence number
	m_ps_seqno = tempSeqNum;

	// capture any miscellaneous fields
	m_ps_other = tempUser;

	// no user changes yet
	pubsubDataChanged = false;

	// Update the data in the controls
	UpdateData (FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting PubSub::parseRFH2pubsub() tempCommand=%s", tempCommand);

		// trace exit from parseRFH2pubsub
		pDoc->logTraceEntry(traceInfo);
	}
}

void PubSub::processPubsubOption(char * value)

{
	int		found=0;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::processPubsubOption value=%s", value);

		// trace entry to processPubsubOption
		pDoc->logTraceEntry(traceInfo);
	}

	if (value[0] != 0)
	{
		// search for the Local option
		if (strcmp(value, MQPSC_LOCAL) == 0)
		{
			m_ps_local = TRUE;
			found = 1;
		}

		if (strcmp(value, MQPSC_CORREL_ID_AS_IDENTITY) == 0)
		{
			m_ps_correlasid = TRUE;
			found = 1;
		}

		if (strcmp(value, MQPSC_NEW_PUBS_ONLY) == 0)
		{
			m_ps_newonly = TRUE;
			found = 1;
		}

		if (strcmp(value, MQPSC_PUB_ON_REQUEST_ONLY) == 0)
		{
			m_ps_ondemand = TRUE;
			found = 1;
		}

		if (strcmp(value, MQPSC_DEREGISTER_ALL) == 0)
		{
			m_ps_deregall = TRUE;
			found = 1;
		}

		if (strcmp(value, MQPSC_RETAIN_PUB) == 0)
		{
			m_ps_retain = TRUE;
			found = 1;
		}

		if (strcmp(value, MQPSC_OTHER_SUBS_ONLY) == 0)
		{
			m_ps_otheronly = TRUE;
			found = 1;
		}

		if (strcmp(value, MQPSC_INFORM_IF_RETAINED) == 0)
		{
			m_inform_if_retained = TRUE;
			found = 1;
		}

		if (strcmp(value, MQPSC_IS_RETAINED_PUB) == 0)
		{
			m_ps_isretained = TRUE;
			found = 1;
		}

		if (strcmp(value, MQPSC_NON_PERSISTENT) == 0)
		{
			m_ps_persist = PS_NONPERS;
			found = 1;
		}

		if (strcmp(value, MQPSC_PERSISTENT) == 0)
		{
			m_ps_persist = PS_PERSIST;
			found = 1;
		}

		if (strcmp(value, MQPSC_PERSISTENT_AS_PUBLISH) == 0)
		{
			m_ps_persist = PS_ASPUB;
			found = 1;
		}

		if (strcmp(value, MQPSC_PERSISTENT_AS_Q) == 0)
		{
			m_ps_persist = PS_ASQUEUE;
			found = 1;
		}

		value[0] = 0;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting PubSub::processPubsubOption() found=%d", found);

		// trace exit from processPubsubOption
		pDoc->logTraceEntry(traceInfo);
	}
}

///////////////////////////////////////////////////////
//
// Routine to parse the folders in an RFH2 header
// that are not recognized.
//
///////////////////////////////////////////////////////

char * PubSub::parseRFH2misc(char * ptr,
							 char * endptr,
							 char * tempUser,
							 const char * endTag,
							 const int maxUser,
							 int * index)

{
	int		i;
	int		idx;
	bool	notdone;
	char	*tempptr;
	char	tempTag[8192];
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::parseRFH2misc ptr=%8.8X index=%d", (unsigned int)ptr, (*index));

		// trace entry to parseRFH2misc
		pDoc->logTraceEntry(traceInfo);
	}

	// get the current index value in the temporary area
	idx = (*index);

	// This routine captures things that the program does
	// not recognize in raw XML format.  It captures a 
	// complete pair of tags, including any data in between
	// capture the begin bracket
	tempUser[idx++] = ptr++[0];

	// start building the ending tag name
	tempTag[0] = '<';
	tempTag[1] = '/';

	// capture the rest of the tag
	i = 2;
	while ((idx < maxUser) &&
		   (ptr[0] != '>') &&
		   (ptr[0] != 0) &&
		   (i < sizeof(tempTag) - 1) &&
		   (ptr < endptr) &&
		   (strcmp(ptr, endTag) != 0))
	{
		tempUser[idx++] = ptr[0];
		tempTag[i++] = ptr[0];
		ptr++;
	}

	// finish the end tag name and
	// turn the it into a string
	tempTag[i++] = '>';
	tempTag[i] = 0;

	// check for a blank in the tag name
	// this indicates attributes that are
	// not part of the ending tag name
	tempptr = strchr(tempTag, ' ');
	if (tempptr != NULL)
	{
		tempptr[0] = '>';
		tempptr[1] = 0;
		i = strlen(tempTag);
	}

	// now copy characters until we find the end tag
	notdone = true;
	while ((idx < maxUser) &&
		   (ptr < (endptr - i + 1)) &&
		   (notdone) &&
		   (ptr[0] != 0) &&
		   (strcmp(ptr, endTag) != 0))
	{
		if (memcmp(ptr, tempTag, i) == 0)
		{
			// found the end tag - capture the
			// data and skip over the end tag
			notdone = false;
			strcat(tempUser + idx, tempTag);
			idx += strlen(tempTag);
			ptr += strlen(tempTag);
		}
		else
		{
			tempUser[idx++] = ptr[0];
			ptr++;
		}
	}

	(*index) = idx;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting PubSub::parseRFH2misc ptr=%8.8X index=%d", (unsigned int)ptr, (*index));

		// trace exit from parseRFH2misc
		pDoc->logTraceEntry(traceInfo);
	}

	return ptr;
}


void PubSub::parseV1Command(char * rfhptr)

{
	int		found=0;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::parseV1Command rfhptr=%.8s", rfhptr);

		// trace entry to parseV1Command
		pDoc->logTraceEntry(traceInfo);
	}

	m_ps_reqtype = -1;
	if (memcmp(rfhptr, MQPS_DELETE_PUBLICATION, sizeof(MQPS_DELETE_PUBLICATION)-1) == 0)
	{
		m_ps_reqtype = PS_REQ_DELPUB;
		found = 1;
	}

	if (memcmp(rfhptr, MQPS_DEREGISTER_SUBSCRIBER, sizeof(MQPS_DEREGISTER_SUBSCRIBER)-1) == 0)
	{
		m_ps_reqtype = PS_REQ_UNREG;
		found = 1;
	}

	if (memcmp(rfhptr, MQPS_PUBLISH, sizeof(MQPS_PUBLISH)-1) == 0)
	{
		m_ps_reqtype = PS_REQ_PUBLISH;
		found = 1;
	}

	if (memcmp(rfhptr, MQPS_REGISTER_SUBSCRIBER, sizeof(MQPS_REGISTER_SUBSCRIBER)-1) == 0)
	{
		m_ps_reqtype = PS_REQ_REGISTER;
		found = 1;
	}

	if (memcmp(rfhptr, MQPS_REQUEST_UPDATE, sizeof(MQPS_REQUEST_UPDATE)-1) == 0)
	{
		m_ps_reqtype = PS_REQ_REQPUB;
		found = 1;
	}

	if (memcmp(rfhptr, MQPS_DEREGISTER_PUBLISHER, sizeof(MQPS_DEREGISTER_PUBLISHER)-1) == 0)
	{
		m_ps_reqtype = PS_REQ_UNREG_PUB;
		found = 1;
	}

	if (memcmp(rfhptr, MQPS_REGISTER_PUBLISHER, sizeof(MQPS_REGISTER_PUBLISHER)-1) == 0)
	{
		m_ps_reqtype = PS_REQ_REG_PUB;
		found = 1;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting PubSub::parseV1Command() found=%d", found);

		// trace exit from parseV1Command
		pDoc->logTraceEntry(traceInfo);
	}
}

char * PubSub::parseV1Pubsub(char * rfhptr, int *found)

{
	char	tempval[8192];
	char	traceInfo[512];		// work variable to build trace message

	// make sure we have current info
	UpdateData(TRUE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::parseV1Pubsub() rfhptr=%8.8X %.32s", (unsigned int)rfhptr, rfhptr);

		// trace entry to parseV1Pubsub
		pDoc->logTraceEntry(traceInfo);
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_COMMAND, sizeof(MQPS_COMMAND) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_COMMAND);
		rfhptr = skipBlanks(rfhptr);

		// parse the actual request type
		parseV1Command(rfhptr);

		// skip the request type
		rfhptr = findBlank(rfhptr);
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_PUBLISH_TIMESTAMP, sizeof(MQPS_PUBLISH_TIMESTAMP) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_PUBLISH_TIMESTAMP);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_ps_pubtime = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_Q_MGR_NAME, sizeof(MQPS_Q_MGR_NAME) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_Q_MGR_NAME);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_ps_qm = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_Q_NAME, sizeof(MQPS_Q_NAME) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_Q_NAME);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_ps_q = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_SEQUENCE_NUMBER, sizeof(MQPS_SEQUENCE_NUMBER) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_SEQUENCE_NUMBER);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_ps_seqno = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_STREAM_NAME, sizeof(MQPS_STREAM_NAME) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_STREAM_NAME);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_ps_subpoint = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_SUBSCRIPTION_NAME, sizeof(MQPS_SUBSCRIPTION_NAME) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_SUBSCRIPTION_NAME);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_ps_sub_name = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_SUBSCRIPTION_IDENTITY, sizeof(MQPS_SUBSCRIPTION_IDENTITY) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_SUBSCRIPTION_IDENTITY);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_ps_sub_identity = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_SUBSCRIPTION_USER_DATA, sizeof(MQPS_SUBSCRIPTION_USER_DATA) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_SUBSCRIPTION_USER_DATA);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_ps_sub_data = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_TOPIC, sizeof(MQPS_TOPIC) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_TOPIC);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		if (m_ps_topic1.GetLength() == 0)
		{
			m_ps_topic1 = tempval;
		}
		else
		{
			if (m_ps_topic2.GetLength() == 0)
			{
				m_ps_topic2 = tempval;
			}
			else
			{
				if (m_ps_topic3.GetLength() == 0)
				{
					m_ps_topic3 = tempval;
				}
				else
				{
					if (m_ps_topic4.GetLength() == 0)
					{
						m_ps_topic4 = tempval;
					}
				}
			}
		}
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_DELETE_OPTIONS, sizeof(MQPS_DELETE_OPTIONS) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_DELETE_OPTIONS);
		rfhptr = skipBlanks(rfhptr);

		if (memcmp(rfhptr, MQPS_LOCAL, sizeof(MQPS_LOCAL) - 1) == 0)
		{
			m_ps_local = TRUE;
		}

		rfhptr = findBlank(rfhptr);
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_PUBLICATION_OPTIONS, sizeof(MQPS_PUBLICATION_OPTIONS) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_PUBLICATION_OPTIONS) - 1;
		rfhptr = skipBlanks(rfhptr);

		if (memcmp(rfhptr, MQPS_LOCAL, sizeof(MQPS_LOCAL) - 1) == 0)
		{
			m_ps_local = TRUE;
		}

		if (memcmp(rfhptr, MQPS_CORREL_ID_AS_IDENTITY, sizeof(MQPS_CORREL_ID_AS_IDENTITY) - 1) == 0)
		{
			m_ps_correlasid = TRUE;
		}

		if (memcmp(rfhptr, MQPS_VARIABLE_USER_ID, sizeof(MQPS_VARIABLE_USER_ID) - 1) == 0)
		{
			m_ps_var_user = TRUE;
		}

		if (memcmp(rfhptr, MQPS_RETAIN_PUBLICATION, sizeof(MQPS_RETAIN_PUBLICATION) - 1) == 0)
		{
			m_ps_retain = TRUE;
		}

		if (memcmp(rfhptr, MQPS_IS_RETAINED_PUBLICATION, sizeof(MQPS_IS_RETAINED_PUBLICATION) - 1) == 0)
		{
			m_ps_isretained = TRUE;
		}

		if (memcmp(rfhptr, MQPS_OTHER_SUBSCRIBERS_ONLY, sizeof(MQPS_OTHER_SUBSCRIBERS_ONLY) - 1) == 0)
		{
			m_ps_otheronly = TRUE;
		}

		if (memcmp(rfhptr, MQPS_NO_REGISTRATION, sizeof(MQPS_NO_REGISTRATION) - 1) == 0)
		{
			m_ps_no_reg = TRUE;
		}

		rfhptr = findBlank(rfhptr);
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_REGISTRATION_OPTIONS, sizeof(MQPS_REGISTRATION_OPTIONS) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_REGISTRATION_OPTIONS) - 1;
		rfhptr = skipBlanks(rfhptr);

		if (memcmp(rfhptr, MQPS_LOCAL, sizeof(MQPS_LOCAL) - 1) == 0)
		{
			m_ps_local = TRUE;
		}

		if (memcmp(rfhptr, MQPS_NEW_PUBLICATIONS_ONLY, sizeof(MQPS_NEW_PUBLICATIONS_ONLY) - 1) == 0)
		{
			m_ps_newonly = TRUE;
		}

		if (memcmp(rfhptr, MQPS_PUBLISH_ON_REQUEST_ONLY, sizeof(MQPS_PUBLISH_ON_REQUEST_ONLY) - 1) == 0)
		{
			m_ps_ondemand = TRUE;
		}

		if (memcmp(rfhptr, MQPS_INFORM_IF_RETAINED, sizeof(MQPS_INFORM_IF_RETAINED) - 1) == 0)
		{
			m_inform_if_retained = TRUE;
		}

		if (memcmp(rfhptr, MQPS_CORREL_ID_AS_IDENTITY, sizeof(MQPS_CORREL_ID_AS_IDENTITY) - 1) == 0)
		{
			m_ps_correlasid = TRUE;
		}

		if (memcmp(rfhptr, MQPS_ADD_NAME, sizeof(MQPS_ADD_NAME) - 1) == 0)
		{
			m_ps_add_name = TRUE;
		}

		if (memcmp(rfhptr, MQPS_FULL_RESPONSE, sizeof(MQPS_FULL_RESPONSE) - 1) == 0)
		{
			m_ps_full_resp = TRUE;
		}

		if (memcmp(rfhptr, MQPS_JOIN_EXCLUSIVE, sizeof(MQPS_JOIN_EXCLUSIVE) - 1) == 0)
		{
			m_ps_join_excl = TRUE;
		}

		if (memcmp(rfhptr, MQPS_JOIN_SHARED, sizeof(MQPS_JOIN_SHARED) - 1) == 0)
		{
			m_ps_join_shared = TRUE;
		}

		if (memcmp(rfhptr, MQPS_NO_ALTERATION, sizeof(MQPS_NO_ALTERATION) - 1) == 0)
		{
			m_ps_noalter = TRUE;
		}

		if (memcmp(rfhptr, MQPS_LOCKED, sizeof(MQPS_LOCKED) - 1) == 0)
		{
			m_ps_locked = TRUE;
		}

		if (memcmp(rfhptr, MQPS_VARIABLE_USER_ID, sizeof(MQPS_VARIABLE_USER_ID) - 1) == 0)
		{
			m_ps_var_user = TRUE;
		}

		if (memcmp(rfhptr, MQPS_DIRECT_REQUESTS, sizeof(MQPS_DIRECT_REQUESTS) - 1) == 0)
		{
			m_ps_direct_req = TRUE;
		}

		if (memcmp(rfhptr, MQPS_DUPLICATES_OK, sizeof(MQPS_DUPLICATES_OK) - 1) == 0)
		{
			m_ps_dups_ok = TRUE;
		}

		if (memcmp(rfhptr, MQPS_INCLUDE_STREAM_NAME, sizeof(MQPS_INCLUDE_STREAM_NAME) - 1) == 0)
		{
			m_ps_incl_stream_name = TRUE;
		}

		if (memcmp(rfhptr, MQPS_DEREGISTER_ALL, sizeof(MQPS_DEREGISTER_ALL) - 1) == 0)
		{
			m_ps_deregall = TRUE;
		}

		if (memcmp(rfhptr, MQPS_ANONYMOUS, sizeof(MQPS_ANONYMOUS) - 1) == 0)
		{
			m_ps_anon = TRUE;
		}

		if (memcmp(rfhptr, MQPS_LEAVE_ONLY, sizeof(MQPS_LEAVE_ONLY) - 1) == 0)
		{
			m_ps_leave_only = TRUE;
		}

		if (memcmp(rfhptr, MQPS_NO_REGISTRATION, sizeof(MQPS_NO_REGISTRATION) - 1) == 0)
		{
			m_ps_no_reg = TRUE;
		}

		if (memcmp(rfhptr, MQPS_PERSISTENT, sizeof(MQPS_PERSISTENT) - 1) == 0)
		{
			m_ps_persist = PS_PERSIST;
		}

		if (memcmp(rfhptr, MQPS_NON_PERSISTENT, sizeof(MQPS_NON_PERSISTENT) - 1) == 0)
		{
			m_ps_persist = PS_NONPERS;
		}

		if (memcmp(rfhptr, MQPS_PERSISTENT_AS_Q, sizeof(MQPS_PERSISTENT_AS_Q) - 1) == 0)
		{
			m_ps_persist = PS_ASQUEUE;
		}

		rfhptr = findBlank(rfhptr);
	}

	// Update the controls
	UpdateData (FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting PubSub::parseV1Pubsub rfhptr=%8.8X found=%d", (unsigned int)rfhptr, (*found));

		// trace exit from parseV1Pubsub
		pDoc->logTraceEntry(traceInfo);
	}

	return rfhptr;
}


//////////////////////////////////////////////////
//
// Routine to add pubsub fields to the RFH1 header
//
//////////////////////////////////////////////////

int PubSub::buildV1PubsubArea(char *tempBuf)

{
	char	*ptr;
	char	traceInfo[512];		// work variable to build trace message

	// make sure we have current info
	UpdateData(TRUE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::buildV1PubsubArea() tempBuf=%8.8X m_ps_reqtype=%d", (unsigned int)tempBuf, m_ps_reqtype);

		// trace entry to buildV1PubsubArea
		pDoc->logTraceEntry(traceInfo);
	}

	// reject any attempts if the command is not set
	if (-1 == m_ps_reqtype)
	{
		return 0;
	}

	// Insert the command string
	strcpy(tempBuf, MQPS_COMMAND);
	strcat(tempBuf, " ");
	ptr = tempBuf + strlen(tempBuf);

	switch (m_ps_reqtype)
	{
	case PS_REQ_REGISTER:
		{
			strcat(ptr, MQPS_REGISTER_SUBSCRIBER);
			break;
		}
	case PS_REQ_UNREG:
		{
			strcat(ptr, MQPS_DEREGISTER_SUBSCRIBER);
			break;
		}
	case PS_REQ_PUBLISH:
		{
			strcat(ptr, MQPS_PUBLISH);
			break;
		}
	case PS_REQ_DELPUB:
		{
			strcat(ptr, MQPS_DELETE_PUBLICATION);
			break;
		}
	case PS_REQ_REQPUB:
		{
			strcat(ptr, MQPS_REQUEST_UPDATE);
			break;
		}
	case PS_REQ_REG_PUB:
		{
			strcat(ptr, MQPS_REGISTER_PUBLISHER);
			break;
		}
	case PS_REQ_UNREG_PUB:
		{
			strcat(ptr, MQPS_DEREGISTER_PUBLISHER);
			break;
		}
	}

	// move past this item
	ptr += strlen(ptr);

	// fill in the first topic
	if (m_ps_topic1.GetLength() > 0)
	{
		ptr = buildV1String(ptr, MQPS_TOPIC, m_ps_topic1, ptr - tempBuf);
	}

	if (m_ps_reqtype != PS_REQ_REQPUB)
	{
		if (m_ps_topic2.GetLength() > 0)
		{
			ptr = buildV1String(ptr, MQPS_TOPIC, m_ps_topic2, ptr - tempBuf);
		}

		if (m_ps_topic3.GetLength() > 0)
		{
			ptr = buildV1String(ptr, MQPS_TOPIC, m_ps_topic3, ptr - tempBuf);
		}

		if (m_ps_topic4.GetLength() > 0)
		{
			ptr = buildV1String(ptr, MQPS_TOPIC, m_ps_topic4, ptr - tempBuf);
		}
	}

	// check if the queue or queue manager name is specified
	if (m_ps_qm.GetLength() > 0)
	{
		ptr = buildV1String(ptr, MQPS_Q_MGR_NAME, m_ps_qm, ptr - tempBuf);
	}

	// check if the queue or queue manager name is specified
	if (m_ps_q.GetLength() > 0)
	{
		ptr = buildV1String(ptr, MQPS_Q_NAME, m_ps_q, ptr - tempBuf);
	}

	// build the rest of the command
	switch (m_ps_reqtype)
	{
	case PS_REQ_REGISTER:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_STREAM_NAME, m_ps_subpoint, ptr - tempBuf);
			}

			// handle the subscription name, identity and user data
			// check if the subscription name is specified
			if (m_ps_sub_name.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_SUBSCRIPTION_NAME, m_ps_sub_name, ptr - tempBuf);
			}

			// check if the subscription identity is specified
			if (m_ps_sub_identity.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_SUBSCRIPTION_IDENTITY, m_ps_sub_identity, ptr - tempBuf);
			}

			// check if the subscription identity is specified
			if (m_ps_sub_data.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_SUBSCRIPTION_USER_DATA, m_ps_sub_data, ptr - tempBuf);
			}

			if (m_ps_local)
			{
				ptr = buildV1RegOpt(ptr, MQPS_LOCAL);
			}

			if (m_ps_newonly)
			{
				ptr = buildV1RegOpt(ptr, MQPS_NEW_PUBLICATIONS_ONLY);
			}

			if (m_ps_ondemand)
			{
				ptr = buildV1RegOpt(ptr, MQPS_PUBLISH_ON_REQUEST_ONLY);
			}

			if (m_ps_correlasid)
			{
				ptr = buildV1RegOpt(ptr, MQPS_CORREL_ID_AS_IDENTITY);
			}

			if (m_inform_if_retained)
			{
				ptr = buildV1RegOpt(ptr, MQPS_INFORM_IF_RETAINED);
			}

			if (m_ps_add_name)
			{
				ptr = buildV1RegOpt(ptr, MQPS_ADD_NAME);
			}

			if (m_ps_full_resp)
			{
				ptr = buildV1RegOpt(ptr, MQPS_FULL_RESPONSE);
			}

			if (m_ps_join_excl)
			{
				ptr = buildV1RegOpt(ptr, MQPS_JOIN_EXCLUSIVE);
			}

			if (m_ps_join_shared)
			{
				ptr = buildV1RegOpt(ptr, MQPS_JOIN_SHARED);
			}

			if (m_ps_noalter)
			{
				ptr = buildV1RegOpt(ptr, MQPS_NO_ALTERATION);
			}

			if (m_ps_locked)
			{
				ptr = buildV1RegOpt(ptr, MQPS_LOCKED);
			}

			if (m_ps_var_user)
			{
				ptr = buildV1RegOpt(ptr, MQPS_VARIABLE_USER_ID);
			}

			if (m_ps_incl_stream_name)
			{
				ptr = buildV1RegOpt(ptr, MQPS_INCLUDE_STREAM_NAME);
			}

			if (m_ps_no_reg)
			{
				ptr = buildV1RegOpt(ptr, MQPS_NO_REGISTRATION);
			}

			if (m_ps_persist > 0)
			{
				strcat(ptr, " ");							// don't forget to include a delimiter
				strcat(ptr, MQPS_REGISTRATION_OPTIONS);		// set the field name
				strcat(ptr, " ");							// include the delimiter

				switch (m_ps_persist)						// figure out which option to include
				{
				case PS_PERSIST:
					{
						strcat(ptr, MQPS_PERSISTENT);		// use persistent messages for this subscription
						break;
					}
				case PS_NONPERS:
					{
						strcat(ptr, MQPS_NON_PERSISTENT);	// use non-persistent messages for this subscription
						break;
					}
				case PS_ASQUEUE:
					{
						strcat(ptr, MQPS_PERSISTENT_AS_Q);	// use the persistence specified on the queue for this subscription
						break;
					}
				default:
					{
						strcat(ptr, MQPS_PERSISTENT_AS_PUBLISH);	// set a sensible default just in case - should not ever get here
						break;
					}
				}

				ptr += strlen(ptr) + 1;						// update the pointer to the next available byte
			}

			break;
		}
	case PS_REQ_UNREG:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_STREAM_NAME, m_ps_subpoint, ptr - tempBuf);
			}

			// handle the subscription name and identity
			// check if the subscription name is specified
			if (m_ps_sub_name.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_SUBSCRIPTION_NAME, m_ps_sub_name, ptr - tempBuf);
			}

			// check if the subscription identity is specified
			if (m_ps_sub_identity.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_SUBSCRIPTION_IDENTITY, m_ps_sub_identity, ptr - tempBuf);
			}

			if (m_ps_deregall)
			{
				ptr = buildV1RegOpt(ptr, MQPS_DEREGISTER_ALL);
			}

			if (m_ps_correlasid)
			{
				ptr = buildV1RegOpt(ptr, MQPS_CORREL_ID_AS_IDENTITY);
			}

			if (m_ps_leave_only)
			{
				ptr = buildV1RegOpt(ptr, MQPS_LEAVE_ONLY);
			}

			if (m_ps_var_user)
			{
				ptr = buildV1RegOpt(ptr, MQPS_VARIABLE_USER_ID);
			}

			if (m_ps_full_resp)
			{
				ptr = buildV1RegOpt(ptr, MQPS_FULL_RESPONSE);
			}

			break;
		}
	case PS_REQ_PUBLISH:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_STREAM_NAME, m_ps_subpoint, ptr - tempBuf);
			}

			if (m_ps_retain)
			{
				ptr = buildV1PubOpt(ptr, MQPS_RETAIN_PUBLICATION);
			}

			if (m_ps_isretained)
			{
				ptr = buildV1PubOpt(ptr, MQPS_IS_RETAINED_PUBLICATION);
			}

			if (m_ps_local)
			{
				// note that this is a registration option on a publish command
				ptr = buildV1RegOpt(ptr, MQPS_LOCAL);
			}

			if (m_ps_anon)
			{
				// note that this is a registration option on a publish command
				ptr = buildV1RegOpt(ptr, MQPS_ANONYMOUS);
			}

			if (m_ps_direct_req)
			{
				// note that this is a registration option on a publish command
				ptr = buildV1RegOpt(ptr, MQPS_DIRECT_REQUESTS);
			}

			if (m_ps_otheronly)
			{
				ptr = buildV1PubOpt(ptr, MQPS_OTHER_SUBSCRIBERS_ONLY);
			}

			if (m_ps_correlasid)
			{
				ptr = buildV1PubOpt(ptr, MQPS_CORREL_ID_AS_IDENTITY);
			}

			if (m_ps_no_reg)
			{
				ptr = buildV1PubOpt(ptr, MQPS_NO_REGISTRATION);
			}

			if (m_ps_pubtime.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_PUBLISH_TIMESTAMP, m_ps_pubtime, ptr - tempBuf);
			}

			if (m_ps_seqno.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_SEQUENCE_NUMBER, m_ps_seqno, ptr - tempBuf);
			}

			break;
		}
	case PS_REQ_DELPUB:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_STREAM_NAME, m_ps_subpoint, ptr - tempBuf);
			}

			if (m_ps_local)
			{
				ptr = buildV1DelOpt(ptr, MQPS_LOCAL);
			}

			break;
		}
	case PS_REQ_REQPUB:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_STREAM_NAME, m_ps_subpoint, ptr - tempBuf);
			}

			// check if the subscription name is specified
			if (m_ps_sub_name.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_SUBSCRIPTION_NAME, m_ps_sub_name, ptr - tempBuf);
			}

			if (m_ps_correlasid)
			{
			// note that this is a registration option on a publish command
				ptr = buildV1RegOpt(ptr, MQPS_CORREL_ID_AS_IDENTITY);
			}

			if (m_ps_var_user)
			{
				// note that this is a registration option on a publish command
				ptr = buildV1RegOpt(ptr, MQPS_VARIABLE_USER_ID);
			}

			break;
		}
	case PS_REQ_REG_PUB:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_STREAM_NAME, m_ps_subpoint, ptr - tempBuf);
			}

			if (m_ps_local)
			{
				ptr = buildV1RegOpt(ptr, MQPS_LOCAL);
			}

			if (m_ps_correlasid)
			{
				ptr = buildV1RegOpt(ptr, MQPS_CORREL_ID_AS_IDENTITY);
			}

			if (m_ps_direct_req)
			{
				ptr = buildV1RegOpt(ptr, MQPS_DIRECT_REQUESTS);
			}

			if (m_ps_anon)
			{
				ptr = buildV1RegOpt(ptr, MQPS_ANONYMOUS);
			}

			break;
		}
	case PS_REQ_UNREG_PUB:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				ptr = buildV1String(ptr, MQPS_STREAM_NAME, m_ps_subpoint, ptr - tempBuf);
			}

			if (m_ps_deregall)
			{
				ptr = buildV1RegOpt(ptr, MQPS_DEREGISTER_ALL);
			}

			if (m_ps_correlasid)
			{
				ptr = buildV1RegOpt(ptr, MQPS_CORREL_ID_AS_IDENTITY);
			}

			break;
		}
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting PubSub::buildV1PubsubArea length=%d", ptr - tempBuf);

		// trace exit from buildV1PubsubArea
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("pubsub data", (unsigned char *)tempBuf, ptr - tempBuf);
	}

	return ptr - tempBuf;
}

///////////////////////////////////////////////////////
//
// Routine to add option fields to the RFH1 header
//
///////////////////////////////////////////////////////

char * PubSub::buildV1Opt(char *ptr, const char *value, const char *opt)

{
	strcat(ptr, " ");
	strcat(ptr, opt);
	strcat(ptr, " ");
	strcat(ptr, value);
	ptr += strlen(ptr);

	return ptr;
}

///////////////////////////////////////////////////////
//
// Routine to add pubsub register subscription fields
// to the RFH1 header
//
///////////////////////////////////////////////////////

char * PubSub::buildV1RegOpt(char *ptr, const char *value)

{
	ptr = buildV1Opt(ptr, value, MQPS_REGISTRATION_OPTIONS);

	return ptr;
}

//////////////////////////////////////////////////
//
// Routine to add pubsub publish fields
// to the RFH1 header
//
//////////////////////////////////////////////////

char * PubSub::buildV1PubOpt(char *ptr, const char *value)

{
	ptr = buildV1Opt(ptr, value, MQPS_PUBLICATION_OPTIONS);

	return ptr;
}

//////////////////////////////////////////////////
//
// Routine to add pubsub delete publication fields
// to the RFH1 header
//
//////////////////////////////////////////////////

char * PubSub::buildV1DelOpt(char *ptr, const char *value)

{
	ptr = buildV1Opt(ptr, value, MQPS_DELETE_OPTIONS);

	return ptr;
}

//////////////////////////////////////////////////
//
// Routine to build a pubsub folder for the RFH header
//
//////////////////////////////////////////////////

int PubSub::buildPubsubArea(int ccsid, int encoding)

{
	int				i;
	int				j;
	int				extra;
	wchar_t			*ucsPtr;
	CString			xml;
	unsigned char	*tempPtr;
	char			spaces[4];
	char			tempBuf[8192];
	char			traceInfo[256];		// work variable to build trace message

	// make sure we have current info in the instance variables
	UpdateData(TRUE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering PubSub::buildPubsubArea m_ps_reqtype=%d ccsid=%d encoding=%d", m_ps_reqtype, ccsid, encoding);

		// trace entry to buildPubsubArea
		pDoc->logTraceEntry(traceInfo);
	}

	// initialize the string to nulls
	xml.Empty();

	// reject any attempts if the command is not set
	if ((-1 == m_ps_reqtype) || 
		(PS_REQ_REG_PUB == m_ps_reqtype) ||
		(PS_REQ_UNREG_PUB == m_ps_reqtype))
	{
		return 0;
	}

	// build the first part of the XML text
	xml = MQRFH2_PUBSUB_CMD_FOLDER_B;
	xml += MQPSC_COMMAND_B;

	// insert the command string
	switch (m_ps_reqtype)
	{
	case PS_REQ_REGISTER:
		{
			xml += MQPSC_REGISTER_SUBSCRIBER;
			break;
		}
	case PS_REQ_UNREG:
		{
			xml += MQPSC_DEREGISTER_SUBSCRIBER;
			break;
		}
	case PS_REQ_PUBLISH:
		{
			xml += MQPSC_PUBLISH;
			break;
		}
	case PS_REQ_DELPUB:
		{
			xml += MQPSC_DELETE_PUBLICATION;
			break;
		}
	case PS_REQ_REQPUB:
		{
			xml += MQPSC_REQUEST_UPDATE;
			break;
		}
	}

	// finish the command type
	xml += MQPSC_COMMAND_E;

	// fill in the first topic
	if (m_ps_topic1.GetLength() > 0)
	{
		xml += MQPSC_TOPIC_B;
		replaceChars(m_ps_topic1, tempBuf);
		xml += tempBuf;
		xml += MQPSC_TOPIC_E;
	}

	if (m_ps_reqtype != PS_REQ_REQPUB)
	{
		if (m_ps_topic2.GetLength() > 0)
		{
			xml += MQPSC_TOPIC_B;
			replaceChars(m_ps_topic2, tempBuf);
			xml += tempBuf;
			xml += MQPSC_TOPIC_E;
		}

		if (m_ps_topic3.GetLength() > 0)
		{
			xml += MQPSC_TOPIC_B;
			replaceChars(m_ps_topic3, tempBuf);
			xml += tempBuf;
			xml += MQPSC_TOPIC_E;
		}

		if (m_ps_topic4.GetLength() > 0)
		{
			xml += MQPSC_TOPIC_B;
			replaceChars(m_ps_topic4, tempBuf);
			xml += tempBuf;
			xml += MQPSC_TOPIC_E;
		}
	}

	// build the rest of the command
	switch (m_ps_reqtype)
	{
	case PS_REQ_REGISTER:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_POINT_B;
				replaceChars(m_ps_subpoint, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_POINT_E;
			}

			if (m_ps_filter.GetLength() > 0)
			{
				xml += MQPSC_FILTER_B;
				replaceChars(m_ps_filter, tempBuf);
				xml += tempBuf;
				xml += MQPSC_FILTER_E;
			}

			if (m_ps_sub_name.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_NAME_B;
				replaceChars(m_ps_sub_name, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_NAME_E;
			}

			if (m_ps_sub_identity.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_IDENTITY_B;
				replaceChars(m_ps_sub_identity, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_IDENTITY_E;
			}

			if (m_ps_sub_data.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_USER_DATA_B;
				replaceChars(m_ps_sub_data, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_USER_DATA_E;
			}

			if (m_ps_local)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_LOCAL;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_newonly)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_NEW_PUBS_ONLY;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_ondemand)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_PUB_ON_REQUEST_ONLY;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_correlasid)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_CORREL_ID_AS_IDENTITY;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_inform_if_retained)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_INFORM_IF_RETAINED;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_add_name)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_ADD_NAME;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_full_resp)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_FULL_RESPONSE;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_join_excl)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_JOIN_EXCLUSIVE;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_join_shared)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_JOIN_SHARED;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_noalter)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_NO_ALTERATION;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_var_user)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_VARIABLE_USER_ID;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_persist > 0)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;

				switch (m_ps_persist)
				{
				case PS_PERSIST:
					{
						xml += MQPSC_PERSISTENT;
						break;
					}
				case PS_NONPERS:
					{
						xml += MQPSC_NON_PERSISTENT;
						break;
					}
				case PS_ASQUEUE:
					{
						xml += MQPSC_PERSISTENT_AS_Q;
						break;
					}
				default:
					{
						xml += MQPSC_PERSISTENT_AS_PUBLISH;
						break;
					}
				}

				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			break;
		}
	case PS_REQ_UNREG:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_POINT_B;
				replaceChars(m_ps_subpoint, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_POINT_E;
			}

			if (m_ps_sub_name.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_NAME_B;
				replaceChars(m_ps_sub_name, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_NAME_E;
			}

			if (m_ps_sub_identity.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_IDENTITY_B;
				replaceChars(m_ps_sub_identity, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_IDENTITY_E;
			}

			if (m_ps_filter.GetLength() > 0)
			{
				xml += MQPSC_FILTER_B;
				replaceChars(m_ps_filter, tempBuf);
				xml += tempBuf;
				xml += MQPSC_FILTER_E;
			}

			if (m_ps_deregall)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_DEREGISTER_ALL;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_correlasid)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_CORREL_ID_AS_IDENTITY;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_leave_only)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_LEAVE_ONLY;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_full_resp)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_FULL_RESPONSE;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_var_user)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_VARIABLE_USER_ID;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			break;
		}
	case PS_REQ_PUBLISH:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_POINT_B;
				replaceChars(m_ps_subpoint, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_POINT_E;
			}

			if (m_ps_retain)
			{
				xml += MQPSC_PUBLICATION_OPTION_B;
				xml += MQPSC_RETAIN_PUB;
				xml += MQPSC_PUBLICATION_OPTION_E;
			}

			if (m_ps_isretained)
			{
				xml += MQPSC_PUBLICATION_OPTION_B;
				xml += MQPSC_IS_RETAINED_PUB;
				xml += MQPSC_PUBLICATION_OPTION_E;
			}

			if (m_ps_local)
			{
				xml += MQPSC_PUBLICATION_OPTION_B;
				xml += MQPSC_LOCAL;
				xml += MQPSC_PUBLICATION_OPTION_E;
			}

			if (m_ps_otheronly)
			{
				xml += MQPSC_PUBLICATION_OPTION_B;
				xml += MQPSC_OTHER_SUBS_ONLY;
				xml += MQPSC_PUBLICATION_OPTION_E;
			}

			if (m_ps_correlasid)
			{
				xml += MQPSC_PUBLICATION_OPTION_B;
				xml += MQPSC_CORREL_ID_AS_IDENTITY;
				xml += MQPSC_PUBLICATION_OPTION_E;
			}

			if (m_ps_pubtime.GetLength() > 0)
			{
				xml += MQPSC_PUBLISH_TIMESTAMP_B;
				replaceChars(m_ps_pubtime, tempBuf);
				xml += tempBuf;
				xml += MQPSC_PUBLISH_TIMESTAMP_E;
			}

			if (m_ps_seqno.GetLength() > 0)
			{
				xml += MQPSC_SEQUENCE_NUMBER_B;
				replaceChars(m_ps_seqno, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SEQUENCE_NUMBER_E;
			}

			break;
		}
	case PS_REQ_DELPUB:
		{
			if (m_ps_local)
			{
				xml += MQPSC_DELETE_OPTION_B;
				xml += MQPSC_LOCAL;
				xml += MQPSC_DELETE_OPTION_E;
			}

			break;
		}
	case PS_REQ_REQPUB:
		{
			if (m_ps_subpoint.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_POINT_B;
				replaceChars(m_ps_subpoint, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_POINT_E;
			}

			if (m_ps_filter.GetLength() > 0)
			{
				xml += MQPSC_FILTER_B;
				replaceChars(m_ps_filter, tempBuf);
				xml += tempBuf;
				xml += MQPSC_FILTER_E;
			}

			if (m_ps_sub_name.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_NAME_B;
				replaceChars(m_ps_sub_name, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_NAME_E;
			}

			if (m_ps_sub_identity.GetLength() > 0)
			{
				xml += MQPSC_SUBSCRIPTION_IDENTITY_B;
				replaceChars(m_ps_sub_identity, tempBuf);
				xml += tempBuf;
				xml += MQPSC_SUBSCRIPTION_IDENTITY_E;
			}

			if (m_ps_correlasid)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_CORREL_ID_AS_IDENTITY;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			if (m_ps_var_user)
			{
				xml += MQPSC_REGISTRATION_OPTION_B;
				xml += MQPSC_VARIABLE_USER_ID;
				xml += MQPSC_REGISTRATION_OPTION_E;
			}

			break;
		}
	}

	// check if the queue or queue manager name is specified
	if (m_ps_qm.GetLength() > 0)
	{
		xml += MQPSC_Q_MGR_NAME_B;
		replaceChars(m_ps_qm, tempBuf);
		xml += tempBuf;
		xml += MQPSC_Q_MGR_NAME_E;
	}

	// check if the queue or queue manager name is specified
	if (m_ps_q.GetLength() > 0)
	{
		xml += MQPSC_Q_NAME_B;
		replaceChars(m_ps_q, tempBuf);
		xml += tempBuf;
		xml += MQPSC_Q_NAME_E;
	}

	// append any other fields we don't recognize
	xml += m_ps_other;

	// finish the psc folder
	xml += MQRFH2_PUBSUB_CMD_FOLDER_E;

	// check if the data must be converted to UCS-2
	if (isUCS2(ccsid))
	{
		// convert the data to UCS-2
		// allocate a temporary area to hold the UCS-2 data
		tempPtr = (unsigned char *)rfhMalloc(2 * xml.GetLength() + 16, "PUBTPTR ");

		if (tempPtr != 0)
		{
			// translate the data to UCS-2
			MultiByteToUCS2(tempPtr, 2 * xml.GetLength() + 2, (unsigned char *)(LPCTSTR)xml, 2 * xml.GetLength());

			// set the length of the new mcd in bytes (each 16-bit character is 2 bytes)
			m_RFH_pubsub_len = roundLength2(tempPtr, NUMERIC_PC) * 2;

			// check if the data has to be reversed
			if (encoding != NUMERIC_PC)
			{
				// reverse the order of the bytes
				i = 0;
				j = m_RFH_pubsub_len >> 1;
				ucsPtr = (wchar_t *)tempPtr;
				while (i < j)
				{
					// reverse the bytes in the UCS-2 character
					short s = ucsPtr[i];
					s = reverseBytes(&s);
					ucsPtr[i] = s;

					// move on to the next character
					i++;
				}
			}

			// save the results
			setPubsubArea(tempPtr, m_RFH_pubsub_len, ccsid, encoding);

			// release the storage we acquired
			rfhFree(tempPtr);
		}
		else
		{
			if (pDoc->traceEnabled)
			{
				// malloc failed - write entry to trace
				pDoc->logTraceEntry("Usr::buildUsrArea() malloc failed");
			}
		}
	}
	else
	{
		// Round it up to a multiple of 4 if necessary
		extra = xml.GetLength() % 4;
		if (extra > 0)
		{
			extra = 4 - extra;
			spaces[0] = 0;

			while (extra > 0)
			{
				strcat(spaces, " ");
				extra--;
			}
			
			xml += spaces;
		}

		// save the results
		setPubsubArea((unsigned char *)(LPCTSTR)xml, xml.GetLength(), ccsid, encoding);
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting PubSub::buildPubsubArea m_RFH_pubsub_len=%d", m_RFH_pubsub_len);

		// trace exit from buildPubsubArea
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("pubsub data", rfh_pubsub_area, m_RFH_pubsub_len);
	}

	return m_RFH_pubsub_len;
}

BOOL PubSub::wasDataChanged()

{
	return pubsubDataChanged;
}

void PubSub::clearPubSubData()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to clearPubSubData
		pDoc->logTraceEntry("Entering PubSub::clearPubSubData()");
	}

	freePubsubArea();

	m_ps_connect_q = _T("");
	m_ps_connect_qm = _T("");
	m_ps_errmsg = _T("");
	m_ps_topic1 = _T("");
	m_ps_topic2 = _T("");
	m_ps_topic3 = _T("");
	m_ps_topic4 = _T("");
	m_ps_q = _T("");
	m_ps_qm = _T("");
	m_ps_filter = _T("");
	m_ps_broker_qm = _T("");
	m_ps_subpoint = _T("");
	m_ps_pubtime = _T("");
	m_ps_seqno = _T("");
	m_ps_sub_data = _T("");
	m_ps_sub_identity = _T("");
	m_ps_sub_name = _T("");
	m_ps_persist = PS_ASPUB;
	m_ps_reqtype = -1;
	m_inform_if_retained = FALSE;
	m_ps_isretained = FALSE;
	m_ps_correlasid = FALSE;
	m_ps_local = FALSE;
	m_ps_newonly = FALSE;
	m_ps_ondemand = FALSE;
	m_ps_deregall = FALSE;
	m_ps_retain = FALSE;
	m_ps_otheronly = FALSE;
	m_ps_var_user = FALSE;
	m_ps_noalter = FALSE;
	m_ps_locked = FALSE;
	m_ps_join_shared = FALSE;
	m_ps_join_excl = FALSE;
	m_ps_full_resp = FALSE;
	m_ps_add_name = FALSE;
	m_ps_incl_stream_name = FALSE;
	m_ps_leave_only = FALSE;
	m_ps_no_reg = FALSE;
	m_ps_direct_req = FALSE;
	m_ps_dups_ok = FALSE;
	m_ps_anon = FALSE;
	pubsubDataChanged = false;
	m_ps_other = _T("");

	// get the instance variables into the form data
	UpdateData(FALSE);
}
