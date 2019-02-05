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

// PS.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "PS.h"
#include "comsubs.h"
#include "CapPubs.h"
#include "WritePubs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+13

// initialize value for accounting token = 64 zeros
#define INIT_ACCT_TOKEN "0000000000000000000000000000000000000000000000000000000000000000"

/////////////////////////////////////////////////////////////////////////////
// CPS property page

IMPLEMENT_DYNCREATE(CPS, CPropertyPage)

CPS::CPS() : CPropertyPage(CPS::IDD)
{
	//{{AFX_DATA_INIT(CPS)
	m_ps_broker_qm = _T("");
	m_ps_qm = _T("");
	m_ps_subname = _T("");
	m_ps_topic = _T("");
	m_ps_durable = FALSE;
	m_ps_remove = FALSE;
	m_ps_errmsgs = _T("");
	m_ps_managed = FALSE;
	m_ps_wait_interval = _T("0");
	m_ps_retain = FALSE;
	m_ps_q = _T("");
	m_ps_local = FALSE;
	m_ps_not_own = FALSE;
	m_ps_suppress_reply = FALSE;
	m_ps_new_only = FALSE;
	m_ps_on_demand = FALSE;
	m_ps_new_correl_id = FALSE;
	m_ps_topicName = _T("");
	m_ps_sub_level = _T("");
	m_ps_priority = _T("");
	m_ps_group_sub = FALSE;
	m_ps_wildcard = PS_WILDCARD_TOPIC;
	m_ps_correl_ascii = SUB_DISPLAY_HEX;
	m_ps_sub_id = _T("");
	m_ps_expiry = _T("");
	m_ps_set_correlid = FALSE;
	m_ps_user_data = _T("");
	m_ps_any_userid = FALSE;
	m_ps_is_retained = FALSE;
	m_ps_appl_ident = _T("");
	m_ps_acct_token = _T("");
	m_ps_set_ident = FALSE;
	m_ps_selection = _T("");
	m_ps_warn_no_pub = FALSE;
	m_ps_no_multicast = FALSE;
	m_ps_new_correl_id = FALSE;
	//}}AFX_DATA_INIT

	m_ps_acct_token = INIT_ACCT_TOKEN;
	memset(&cparms, 0, sizeof(cparms));
	memset(&wparms, 0, sizeof(wparms));
}

CPS::~CPS()
{
}

void CPS::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPS)
	DDX_Text(pDX, IDC_PS_BROKER_QM, m_ps_broker_qm);
	DDV_MaxChars(pDX, m_ps_broker_qm, 48);
	DDX_CBString(pDX, IDC_PS_QM, m_ps_qm);
	DDV_MaxChars(pDX, m_ps_qm, 255);
	DDX_CBString(pDX, IDC_PS_SUBNAME, m_ps_subname);
	DDV_MaxChars(pDX, m_ps_subname, 48);
	DDX_Text(pDX, IDC_PS_TOPIC, m_ps_topic);
	DDX_Check(pDX, IDC_PS_DURABLE, m_ps_durable);
	DDX_Check(pDX, IDC_PS_REMOVE, m_ps_remove);
	DDX_Text(pDX, IDC_PS_ERRORMSGS, m_ps_errmsgs);
	DDX_Check(pDX, IDC_PS_MANAGED, m_ps_managed);
	DDX_Text(pDX, IDC_PS_WAIT_INTERVAL, m_ps_wait_interval);
	DDV_MaxChars(pDX, m_ps_wait_interval, 5);
	DDX_Check(pDX, IDC_PS_RETAIN, m_ps_retain);
	DDX_CBString(pDX, IDC_PS_QUEUE_NAME, m_ps_q);
	DDV_MaxChars(pDX, m_ps_q, 48);
	DDX_Check(pDX, IDC_PS_LOCAL, m_ps_local);
	DDX_Check(pDX, IDC_PS_NOT_OWN, m_ps_not_own);
	DDX_Check(pDX, IDC_PS_SUPPRESS_REPLY, m_ps_suppress_reply);
	DDX_Check(pDX, IDC_PS_NEW_ONLY, m_ps_new_only);
	DDX_Check(pDX, IDC_PS_ON_DEMAND, m_ps_on_demand);
	DDX_CBString(pDX, IDC_PS_TOPIC_NAME, m_ps_topicName);
	DDV_MaxChars(pDX, m_ps_topicName, 48);
	DDX_Text(pDX, IDC_PS_SUB_LEVEL, m_ps_sub_level);
	DDV_MaxChars(pDX, m_ps_sub_level, 1);
	DDX_Text(pDX, IDC_PS_PRIORITY, m_ps_priority);
	DDV_MaxChars(pDX, m_ps_priority, 1);
	DDX_Check(pDX, IDC_PS_GROUP_SUB, m_ps_group_sub);
	DDX_Radio(pDX, IDC_PS_WILDCARD_TOPIC, m_ps_wildcard);
	DDX_Radio(pDX, IDC_PS_CORREL_ASCII, m_ps_correl_ascii);
	DDX_Text(pDX, IDC_PS_CORREL_ID, m_ps_sub_id);
	DDV_MaxChars(pDX, m_ps_sub_id, 48);
	DDX_Text(pDX, IDC_PS_EXPIRY, m_ps_expiry);
	DDV_MaxChars(pDX, m_ps_expiry, 10);
	DDX_Check(pDX, IDC_PS_SET_CORREL_ID, m_ps_set_correlid);
	DDX_Text(pDX, IDC_PS_USER_DATA, m_ps_user_data);
	DDX_Check(pDX, IDC_PS_ANY_USERID, m_ps_any_userid);
	DDX_Check(pDX, IDC_PS_IS_RETAINED, m_ps_is_retained);
	DDX_Text(pDX, IDC_PS_APPL_IDENT, m_ps_appl_ident);
	DDV_MaxChars(pDX, m_ps_appl_ident, 32);
	DDX_Text(pDX, IDC_PS_ACCT_TOKEN, m_ps_acct_token);
	DDV_MaxChars(pDX, m_ps_acct_token, 64);
	DDX_Check(pDX, IDC_PS_SET_IDENT, m_ps_set_ident);
	DDX_Text(pDX, IDC_PS_SELECTION, m_ps_selection);
	DDX_Check(pDX, IDC_PS_WARN_NO_PUBS, m_ps_warn_no_pub);
	DDX_Check(pDX, IDC_PS_NO_MULTICAST, m_ps_no_multicast);
	DDX_Check(pDX, IDC_PS_NEW_CORREL_ID, m_ps_new_correl_id);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPS, CPropertyPage)
	//{{AFX_MSG_MAP(CPS)
	ON_CBN_DROPDOWN(IDC_PS_SUBNAME, OnDropdownPsSubname)
	ON_CBN_DROPDOWN(IDC_PS_QM, OnDropdownPsQm)
	ON_BN_CLICKED(IDC_PS_GET, OnPsGet)
	ON_BN_CLICKED(IDC_PS_GET_SUBS, OnPsGetSubs)
	ON_BN_CLICKED(IDC_PS_GET_TOPICS, OnPsGetTopics)
	ON_BN_CLICKED(IDC_PS_PUBLISH, OnPsPublish)
	ON_BN_CLICKED(IDC_PS_CLOSEQ, OnPsCloseq)
	ON_BN_CLICKED(IDC_PS_SUBSCRIBE, OnPsSubscribe)
	ON_CBN_SETFOCUS(IDC_PS_QM, OnSetfocusPsQm)
	ON_CBN_DROPDOWN(IDC_PS_QUEUE_NAME, OnDropdownPsQueueName)
	ON_CBN_DROPDOWN(IDC_PS_TOPIC_NAME, OnDropdownPsTopicName)
	ON_BN_CLICKED(IDC_PS_REQ_PUB, OnPsReqPub)
	ON_BN_CLICKED(IDC_PS_RESUME, OnPsResume)
	ON_BN_CLICKED(IDC_PS_CLEAR, OnPsClear)
	ON_BN_CLICKED(IDC_PS_CORREL_ASCII, OnPsCorrelAscii)
	ON_BN_CLICKED(IDC_PS_CORREL_HEX, OnPsCorrelHex)
	ON_CBN_EDITCHANGE(IDC_PS_SUBNAME, OnEditchangePsSubname)
	ON_CBN_SELCHANGE(IDC_PS_SUBNAME, OnSelchangePsSubname)
	ON_CBN_SELENDCANCEL(IDC_PS_SUBNAME, OnSelendcancelPsSubname)
	ON_CBN_EDITCHANGE(IDC_PS_TOPIC_NAME, OnEditchangePsTopicName)
	ON_CBN_SELCHANGE(IDC_PS_TOPIC_NAME, OnSelchangePsTopicName)
	ON_CBN_SELENDCANCEL(IDC_PS_TOPIC_NAME, OnSelendcancelPsTopicName)
	ON_CBN_EDITCHANGE(IDC_PS_QM, OnEditchangePsQm)
	ON_CBN_SELCHANGE(IDC_PS_QM, OnSelchangePsQm)
	ON_CBN_SELENDCANCEL(IDC_PS_QM, OnSelendcancelPsQm)
	ON_CBN_EDITCHANGE(IDC_PS_QUEUE_NAME, OnEditchangePsQueueName)
	ON_CBN_SELCHANGE(IDC_PS_QUEUE_NAME, OnSelchangePsQueueName)
	ON_CBN_SELENDCANCEL(IDC_PS_QUEUE_NAME, OnSelendcancelPsQueueName)
	ON_BN_CLICKED(IDC_PS_ALTER_SUB, OnPsAlterSub)
	ON_BN_CLICKED(IDC_PS_SAVE_MSGS, OnPsSaveMsgs)
	ON_BN_CLICKED(IDC_PS_WRITE_MSGS, OnPsWriteMsgs)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CPS::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CPS::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPS message handlers

BOOL CPS::OnKillActive() 

{
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering CPS::OnKillActive()");
	}

	// get the data from the controls into the instance variables
	UpdateData(TRUE);

	// remember the last queue used
	pDoc->m_QM_name= (LPCTSTR)m_ps_qm;
	pDoc->m_Q_name = (LPCTSTR)m_ps_q;

	return CPropertyPage::OnKillActive();
}

void CPS::UpdatePageData()

{
	char	traceInfo[256];		// work variable to build trace message

	// user has selected another dialog (tab)
	if (pDoc->traceEnabled)
	{
		// trace entry to updatePageData
		pDoc->logTraceEntry("Entering CPS::updatePageData()");
	}

	// check if there are any error messages to display
	if (pDoc->m_error_msg.GetLength() > 0)
	{
		pDoc->updateMsgText();
	}

	// update the console log
	m_ps_errmsgs = pDoc->m_msg_text;

	// update the controls from the instance variables
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CPS::updatePageData() m_ps_durable=%d, m_ps_managed=%d, m_ps_remove=%d, m_ps_connect_qm=%s, m_ps_broker_qm=%s", m_ps_durable, m_ps_managed, m_ps_remove, (LPCTSTR)m_ps_qm, (LPCTSTR)m_ps_broker_qm);

		// trace exit from updatePageData
		pDoc->logTraceEntry(traceInfo);
	}
}

BOOL CPS::OnSetActive() 

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CPS::OnSetActive() m_ps_broker_qm=%s", (LPCTSTR)m_ps_broker_qm);

		// trace entry to OnSetActive
		pDoc->logTraceEntry(traceInfo);
	}

#ifndef MQCLIENT
	m_ps_qmComboBox.LimitText(MQ_Q_NAME_LENGTH);
#endif

	// get the current values from the shared DataArea object
	m_ps_qm = (LPCTSTR)pDoc->m_QM_name;
	m_ps_q = (LPCTSTR)pDoc->m_Q_name;

	// update the error message edit box
	UpdatePageData();

	// check if user properties are supported
	if (pDoc->pubsubSupported)
	{
		// enable and disable the appropriate buttons
		setSubscriptionButtons();
	}
	else
	{
		// disable the pubsub buttons
		((CButton *)GetDlgItem(IDC_PS_GET))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_GET_SUBS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_GET_TOPICS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_PUBLISH))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_CLOSEQ))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_SUBSCRIBE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_REQ_PUB))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_RESUME))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_ALTER_SUB))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_SAVE_MSGS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_WRITE_MSGS))->EnableWindow(FALSE);
	}

	// send a custom message to switch the focus to the edit box control
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return CPropertyPage::OnSetActive();
}

void CPS::OnDropdownPsSubname() 

{
	int			added=0;
	int			skipped=0;
	char		*ptr;				// pointer used to search for ampersand
	const char	*namePtr=NULL;
	char		sName[MQ_SUB_NAME_LENGTH+MQ_Q_MGR_NAME_LENGTH+MQ_Q_NAME_LENGTH+8];	// saved subscription name - consists of character D for durable followed by subname;
	char		traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CPS::OnDropdownPsSubname() ");

		// trace entry to OnDropdownPsSubname
		pDoc->logTraceEntry(traceInfo);
	}

	// clear the current subscription names
	m_ps_subComboBox.ResetContent();

	// see if we can find a list of queue names for the queue manager
	if (m_ps_qm.GetLength() > 0)
	{
		namePtr = pDoc->getSubNamesListPtr((LPCTSTR)m_ps_qm);
	}
	else
	{
		namePtr = pDoc->getSubNamesListPtr((LPCTSTR)pDoc->defaultQMname);
	}

	if (namePtr != NULL)
	{
		while (namePtr[0] != 0)
		{
			// check the first character - only load durable subscriptions
			if ('D' == namePtr[0])
			{
				// add the name, skipping the first character which is a D or durable or N for non-durable
				// and the second character is an M for managed or P for provided
				// the + 2 is to skip these two characters
				// isolate the rest of the name
				strcpy(sName, namePtr + 2);

				// search for an ampersand ("&") character in the string
				ptr = strchr(sName, '&');
				if (ptr != NULL)
				{
					// terminate the string at the end of the subscription name
					ptr[0] = 0;
				}

				m_ps_subComboBox.AddString(sName);
				added++;						// in case trace is turned on

				if (pDoc->traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " CPS::OnDropdownPsSubname() adding=%s", sName);

					// trace entry to updateIdFields
					pDoc->logTraceEntry(traceInfo);
				}
			}
			else
			{
				// keep track of the number of non-durable subscriptions
				skipped++;
			}

			namePtr = Names::getNextName(namePtr);
		}
	}
					
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CPS::OnDropdownPsSubname() added=%d skipped=%d namePtr=%8.8X", added, skipped, (unsigned int)namePtr);

		// trace exit from OnDropdownPsSubname
		pDoc->logTraceEntry(traceInfo);
	}
}

void CPS::OnDropdownPsQm() 

{
	// Load the list of known queue managers into the 
	// drop down list
	loadEditBox();	
}

void CPS::OnDropdownPsQueueName() 
{
	int			added=0;
	int			skipped=0;
	const char 	*qNamePtr=NULL;
	char		traceInfo[512];		// work variable to build trace message

	m_ps_qComboBox.ResetContent();

	// see if we can find a list of queue names for the queue manager
	if (m_ps_qm.GetLength() > 0)
	{
		qNamePtr = pDoc->getQueueNamesListPtr((LPCTSTR)m_ps_qm);
	}
	else
	{
		qNamePtr = pDoc->getQueueNamesListPtr((LPCTSTR)pDoc->defaultQMname);
	}

	if (qNamePtr != NULL)
	{
		while (qNamePtr[0] != 0)
		{
			// check for cluster queue
			if ((pDoc->m_show_system_queues) || (memcmp(qNamePtr + 1, "SYSTEM.", 7) != 0))
			{
				//cb->AddString(qNamePtr + 1);
				m_ps_qComboBox.AddString(qNamePtr + 1);
				added++;						// in case trace is turned on

				if (pDoc->traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " CPS::OnDropdownPsQueueName() adding=%s", qNamePtr);

					// trace entry to updateIdFields
					pDoc->logTraceEntry(traceInfo);
				}
			}
			else
			{
				skipped++;					// in case trace is turned on

				if (pDoc->traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " CPS::OnDropdownPsQueueName() ignoring=%s", qNamePtr);

					// trace entry to updateIdFields
					pDoc->logTraceEntry(traceInfo);
				}
			}

			qNamePtr = Names::getNextName(qNamePtr);
		}
	}
					
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CPS::OnDropdownPsQueueName() added=%d skipped=%d qNamePtr=%8.8X", added, skipped, (unsigned int)qNamePtr);

		// trace entry to updateIdFields
		pDoc->logTraceEntry(traceInfo);
	}
}

//////////////////////////////////////////////////
//
// Routine to process a subscription request
//
//////////////////////////////////////////////////

void CPS::OnPsSubscribe() 

{
	MQLONG			cc=MQCC_OK;									// results of subscribe call
	CRfhutilApp *	app;										// pointer to MFC application object
	char			qName[MQ_Q_NAME_LENGTH+8];					// work area to retrieve the name of the managed queue
	char			topicName[MQ_TOPIC_NAME_LENGTH+8];			// work area to retrieve the name of the topic
	char			applIdent[MQ_APPL_IDENTITY_DATA_LENGTH+8];	// application identity to use for msgs published for this subscription
	char			acctToken[MQ_ACCOUNTING_TOKEN_LENGTH+8];	// accounting token to use for msgs published for this subscription

	char			*topic = (char *)rfhMalloc(MQ_TOPIC_STR_LENGTH + 8, "PSONPSTP");				// work area to retrieve the topic string
	char			*userData = (char *)rfhMalloc(MQ_TOPIC_STR_LENGTH + 8, "PSONPSUD");			// work area for user data - for now make same length as topic string since there is no defined constant
	char			*selection = (char *)rfhMalloc(MQ_SELECTOR_LENGTH+8, "PSONPSSL");			// selection string

	SUBPARMS		parms;										// parameters area to pass to subscribe function

	// User has pressed the Subscribe button
	// initialize some variables
	memset(qName, 0, sizeof(qName));
	memset(topicName, 0, sizeof(topicName));
	memset(applIdent, 0, sizeof(applIdent));
	memset(acctToken, 0, sizeof(acctToken));
	memset(&parms, 0, sizeof(parms));
	parms.correlId = &m_correlid;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);
	
	// change the cursor to an hourglass
	app->BeginWaitCursor();

	// Create a subscription
	// get the queue name, topic name and topic string
	strcpy(qName, (LPCTSTR)m_ps_q);
	strcpy(topicName, (LPCTSTR)m_ps_topicName);
	strcpy(topic, (LPCTSTR)m_ps_topic);

	// is there any user data?
	parms.userDataLength = m_ps_user_data.GetLength();
	if (parms.userDataLength > 0)
	{
		strcpy(userData, (LPCTSTR)m_ps_user_data);
	}

	// convert the level to an integer
	if (m_ps_sub_level.GetLength() > 0)
	{
		parms.level = atoi((LPCTSTR)m_ps_sub_level);
	}

	if (m_ps_priority.GetLength() > 0)
	{
		parms.priority = atoi((LPCTSTR)m_ps_priority);
	}

	if (m_ps_expiry.GetLength() > 0)
	{
		parms.expiry = atoi((LPCTSTR)m_ps_expiry);
	}

	// check if set identity is selected
	if (m_ps_set_ident)
	{
		// set the identity fields for this subscription
		memcpy(applIdent, (LPCTSTR)m_ps_appl_ident, m_ps_appl_ident.GetLength());
		HexToAscii((unsigned char *)(LPCTSTR)m_ps_acct_token, m_ps_acct_token.GetLength() / 2, (unsigned char *)&acctToken);
	}

	// set the wildcard option
	parms.wildcard = m_ps_wildcard;

	// set the selection string
	memset(selection, 0, sizeof(selection));
	strncpy(selection, (LPCTSTR)m_ps_selection, MQ_SELECTOR_LENGTH);

	// set some other pointers
	parms.subType = PS_SUB_TYPE_CREATE;
	parms.QMname = (LPCTSTR)m_ps_qm;
	parms.remoteQM = (LPCTSTR)m_ps_broker_qm;
	parms.queue = (LPCTSTR)m_ps_q;
	parms.managedQName = qName;
	parms.topic = (LPCTSTR)m_ps_topic;
	parms.topicName = (LPCTSTR)m_ps_topicName;
	parms.resumeTopic = topic;
	parms.topicName = topicName;
	parms.subName = (LPCTSTR)m_ps_subname;
	parms.selection = selection;
	parms.maxSelLen = MQ_SELECTOR_LENGTH;
	parms.applIdent = applIdent;
	parms.acctToken = acctToken;
	parms.userData = userData;
	parms.userDataBufSize = MQ_TOPIC_STR_LENGTH;
	parms.resumeTopicName = topicName;
	parms.durable = m_ps_durable;
	parms.managed = m_ps_managed;
	parms.onDemand = m_ps_on_demand;
	parms.newOnly = m_ps_new_only;
	parms.local = m_ps_local;
	parms.groupSub = m_ps_group_sub;
	parms.anyUserId = m_ps_any_userid;
	parms.setIdent = m_ps_set_ident;
	parms.setCorrelId = m_ps_set_correlid;

	// process the subscription
	cc = pDoc->subscribe(&parms);

	// return to a normal cursor
	app->EndWaitCursor();

	// check if the subcription was successful
	if (MQCC_OK == cc)
	{
		// check if this was a managed subscription and if a queue name was returned
		if (m_ps_managed && (strlen(qName) > 0))
		{
			// update the queue name field with the name of the managed queue
			m_ps_q = qName;
		}

		// check if a topic string was returned
		if (strlen(topic) > 0)
		{
			// get the real topic string
			m_ps_topic = topic;

			// clear the topic name
			m_ps_topicName.Empty();
		}
		else
		{
			// check if a topic name was returned
			if (strlen(topicName) > 0)
			{
				m_ps_topicName = topicName;
			}
		}

		// Clear the file name, since we read the data from a queue
		pDoc->fileName[0] = 0;
	}
	
	// process error message and refresh the controls on the screen
	UpdatePageData();

	// set the windows title to the subscription name
	if ((app->m_mainWnd != NULL) && (m_ps_subname.GetLength() > 0))
	{
		app->m_mainWnd->SetWindowText((LPCTSTR)m_ps_subname);
	}

	// set the buttons to match the subscription status
	setSubscriptionButtons();

	if (topic) rfhFree(topic);
	if (userData) rfhFree(userData);
	if (selection) rfhFree(selection);

	return;
}

void CPS::OnPsResume() 

{
	MQLONG			cc=MQCC_OK;									// results of subscribe call
	int				userDataLength=0;							// length of user data area
	CRfhutilApp *	app;										// pointer to MFC application object
	char			qName[MQ_Q_NAME_LENGTH + 8] = { 0 };					// work area to retrieve the name of the managed queue
	char			topicName[MQ_TOPIC_NAME_LENGTH + 8] = { 0 };			// work area to retrieve the name of the topic
	char			applIdent[MQ_APPL_IDENTITY_DATA_LENGTH+8] = { 0 };	// application identity to use for msgs published for this subscription
	char			acctToken[MQ_ACCOUNTING_TOKEN_LENGTH+8] = { 0 };	// accounting token to use for msgs published for this subscription
	char			tempToken[2*MQ_ACCOUNTING_TOKEN_LENGTH+8] = { 0 };	// work area for binary to hex conversion

	char			*topic = (char *)rfhMalloc(MQ_TOPIC_STR_LENGTH + 8, "PSONRSTP");				// work area to retrieve the topic string
	char			*userData = (char *)rfhMalloc(MQ_TOPIC_STR_LENGTH + 8, "PSONRSUD");			// work area for user data - for now make same length as topic string since there is no defined constant
	char			*selection = (char *)rfhMalloc(MQ_SELECTOR_LENGTH + 8, "PSONRSSL");			// selection string

	SUBPARMS		parms;										// parameters area to pass to subscribe function

	// User has pressed the Resume button
	// initialize stack variables
	memset(qName, 0, sizeof(qName));
	memset(topicName, 0, sizeof(topicName));	
	memset(applIdent, 0, sizeof(applIdent));
	memset(acctToken, 0, sizeof(acctToken));
	memset(&parms, 0, sizeof(parms));
	parms.correlId = &m_correlid;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);

	// change the cursor to an hourglass
	app->BeginWaitCursor();

	// set some other pointers
	parms.subType = PS_SUB_TYPE_RESUME;
	parms.QMname = (LPCTSTR)m_ps_qm;
	parms.remoteQM = (LPCTSTR)m_ps_broker_qm;
	parms.queue = (LPCTSTR)m_ps_q;
	parms.managedQName = qName;
	parms.topic = (LPCTSTR)m_ps_topic;
	parms.topicName = (LPCTSTR)m_ps_topicName;
	parms.resumeTopic = topic;
	parms.topicName = topicName;
	parms.subName = (LPCTSTR)m_ps_subname;
	parms.selection = selection;
	parms.maxSelLen = MQ_SELECTOR_LENGTH;
	parms.applIdent = applIdent;
	parms.acctToken = acctToken;
	parms.userData = userData;
	parms.userDataLength = 0;
	parms.userDataBufSize = MQ_TOPIC_STR_LENGTH;
	parms.resumeTopicName = topicName;

	// process the subscription
	cc = pDoc->subscribe(&parms);

	// return to a normal cursor
	app->EndWaitCursor();

	// check if the resume subcription request was successful
	if (MQCC_OK == cc)
	{
		// the subscription must be durable for resume to work
		m_ps_durable = TRUE;

		// check if this was a managed subscription and if a queue name was returned
		if (m_ps_managed && (strlen(qName) > 0))
		{
			// update the queue name field with the name of the managed queue
			m_ps_q = qName;
		}

		// set the topic name and topic string based on the subscription results
		m_ps_topicName = topicName;
		m_ps_topic = topic;
		m_ps_sub_level.Format("%d", parms.level);
		m_ps_priority.Format("%d", parms.priority);

		// check if a subscription expiration was specified
		if (parms.expiry > 0)
		{
			// set the subscription expiration
			m_ps_expiry.Format("%d", parms.expiry);
		}
		else
		{
			m_ps_expiry.Empty();
		}

		// set the wildcard option
		m_ps_wildcard = parms.wildcard;

		// set the subscription level
		if (parms.level > 0)
		{
			// set the subscription level
			m_ps_sub_level.Format("%d", parms.level);
		}
		else
		{
			// clear the subscription level field
			m_ps_sub_level.Empty();
		}

		// set the priority
		if (parms.priority > 0)
		{
			// set the subscription priority
			m_ps_priority.Format("%d", parms.priority);
		}
		else
		{
			// clear the subscription priority field
			m_ps_priority.Empty();
		}

		// check if user data was returned
		if (parms.userDataLength > 0)
		{
			// set the user data field
			m_ps_user_data = userData;
		}
		else
		{
			// clear the user data
			m_ps_user_data.Empty();
		}

		// is there a selection string?
		if (parms.selection[0] != 0)
		{
			//  capture it
			m_ps_selection = parms.selection;
		}
		else
		{
			// no selection string - clear the field
			m_ps_selection.Empty();
		}

		// is the identity data being specified?
		if (parms.setIdent)
		{
			// get the identity data
			m_ps_appl_ident = parms.applIdent;
			memset(tempToken, 0, sizeof(tempToken));
			AsciiToHex((unsigned char *)&(parms.acctToken), MQ_ACCOUNTING_TOKEN_LENGTH, (unsigned char *)&tempToken);
			m_ps_acct_token = tempToken;
		}

		// capture the correlation id for this subscription
		SetSubCorrelId(parms.correlId);

		// set boolean fields
		m_ps_managed = parms.managed;
		m_ps_on_demand = parms.onDemand;
		m_ps_new_only = parms.newOnly;
		m_ps_local = parms.local;
		m_ps_group_sub = parms.groupSub;
		m_ps_any_userid = parms.anyUserId;
		m_ps_set_ident = parms.setIdent;

		// Clear the file name, since we read the data from a queue
		pDoc->fileName[0] = 0;
	}
	
	// process error message and refresh the controls on the screen
	UpdatePageData();

	// set the windows title to the subscription name
	if ((app->m_mainWnd != NULL) && (m_ps_subname.GetLength() > 0))
	{
		app->m_mainWnd->SetWindowText((LPCTSTR)m_ps_subname);
	}

	// set the buttons to match the subscription status
	setSubscriptionButtons();

	if (topic) rfhFree(topic);
	if (userData) rfhFree(userData);
	if (selection) rfhFree(selection);

	return;
}	

void CPS::OnPsAlterSub() 

{
	MQLONG		cc=MQCC_OK;									// results of subscribe call
	int			userDataLength=0;							// length of user data area
	char		qName[MQ_Q_NAME_LENGTH+8] = { 0 };					// work area to retrieve the name of the managed queue
	char		topicName[MQ_TOPIC_NAME_LENGTH+8] = { 0 };			// work area to retrieve the name of the topic
	char		applIdent[MQ_APPL_IDENTITY_DATA_LENGTH+8] = { 0 };	// application identity to use for msgs published for this subscription
	char		acctToken[MQ_ACCOUNTING_TOKEN_LENGTH+8] = { 0 };	// accounting token to use for msgs published for this subscription
	char		tempToken[2*MQ_ACCOUNTING_TOKEN_LENGTH+8] = { 0 };	// work area for binary to hex conversion
	char			*topic = (char *)rfhMalloc(MQ_TOPIC_STR_LENGTH + 8, "PSONPSTP");				// work area to retrieve the topic string
	char			*userData = (char *)rfhMalloc(MQ_TOPIC_STR_LENGTH + 8, "PSONPSUD");			// work area for user data - for now make same length as topic string since there is no defined constant
	char			*selection = (char *)rfhMalloc(MQ_SELECTOR_LENGTH + 8, "PSONPSSL");			// selection string
	SUBPARMS	parms;										// parameters area to pass to subscribe function

	// User has pressed the alter button
	// initialize the stack variables - rfhMalloc has already initialised heap-allocated buffers
	memset(qName, 0, sizeof(qName));
	memset(topicName, 0, sizeof(topicName));	
	memset(applIdent, 0, sizeof(applIdent));
	memset(acctToken, 0, sizeof(acctToken));
	memset(&parms, 0, sizeof(parms));
	parms.correlId = &m_correlid;

	// Create a subscription
	CRfhutilApp *	app;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);
	
	// change the cursor to an hourglass
	app->BeginWaitCursor();

	// start by closing the current subscription without removing it
	pDoc->closePSQueue(FALSE); 

	// initialize the user data work area
	userData[0] = 0;
	
	// is there any user data?
	userDataLength = m_ps_user_data.GetLength();
	if (userDataLength > 0)
	{
		strcpy(parms.userData, (LPCTSTR)m_ps_user_data);
	}

	// convert the level to an integer
	if (m_ps_sub_level.GetLength() > 0)
	{
		parms.level = atoi((LPCTSTR)m_ps_sub_level);
	}

	if (m_ps_priority.GetLength() > 0)
	{
		parms.priority = atoi((LPCTSTR)m_ps_priority);
	}

	if (m_ps_expiry.GetLength() > 0)
	{
		parms.expiry = atoi((LPCTSTR)m_ps_expiry);
	}

	// check if set identity is selected
	if (m_ps_set_ident)
	{
		// set the identity fields for this subscription
		memcpy(parms.applIdent, (LPCTSTR)m_ps_appl_ident, m_ps_appl_ident.GetLength());
		HexToAscii((unsigned char *)(LPCTSTR)m_ps_acct_token, m_ps_acct_token.GetLength() / 2, (unsigned char *)&parms.acctToken);
	}

	// set the wildcard option
	parms.wildcard = m_ps_wildcard;

	// set the selection string
	strncpy(selection, (LPCTSTR)m_ps_selection, MQ_SELECTOR_LENGTH);

	// set some other pointers
	parms.subType = PS_SUB_TYPE_ALTER;
	parms.QMname = (LPCTSTR)m_ps_qm;
	parms.remoteQM = (LPCTSTR)m_ps_broker_qm;
	parms.queue = (LPCTSTR)m_ps_q;
	parms.managedQName = qName;
	parms.topic = (LPCTSTR)m_ps_topic;
	parms.topicName = (LPCTSTR)m_ps_topicName;
	parms.resumeTopic = topic;
	parms.topicName = topicName;
	parms.subName = (LPCTSTR)m_ps_subname;
	parms.selection = selection;
	parms.maxSelLen = MQ_SELECTOR_LENGTH;
	parms.applIdent = applIdent;
	parms.acctToken = acctToken;
	parms.userData = userData;
	parms.userDataLength = userDataLength;
	parms.userDataBufSize = MQ_TOPIC_STR_LENGTH;
	parms.resumeTopicName = topicName;
	parms.durable = m_ps_durable;
	parms.managed = m_ps_managed;
	parms.onDemand = m_ps_on_demand;
	parms.newOnly = m_ps_new_only;
	parms.local = m_ps_local;
	parms.groupSub = m_ps_group_sub;
	parms.anyUserId = m_ps_any_userid;
	parms.setIdent = m_ps_set_ident;
	parms.setCorrelId = m_ps_set_correlid;

	// process the subscription
	cc = pDoc->subscribe(&parms);

	// return to a normal cursor
	app->EndWaitCursor();

	// check if the alter subcription request was successful
	if (MQCC_OK == cc)
	{
		// update the queue name field with the name of the managed queue
//		m_ps_q = qName;

		// set the topic name and topic string based on the subscription results
//		m_ps_topicName = topicName;
//		m_ps_topic = topic;

		// check if a subscription expiration was specified
		if (parms.expiry > 0)
		{
			// set the subscription expiration
			m_ps_expiry.Format("%d", parms.expiry);
		}
		else
		{
			m_ps_expiry.Empty();
		}

		// set the wildcard option
		m_ps_wildcard = parms.wildcard;

		// set the subscription level
		if (parms.level > 0)
		{
			// set the subscription level
			m_ps_sub_level.Format("%d", parms.level);
		}
		else
		{
			// clear the subscription level field
			m_ps_sub_level.Empty();
		}

		// set the priority
		if (parms.priority > 0)
		{
			// set the subscription priority
			m_ps_priority.Format("%d", parms.priority);
		}
		else
		{
			// clear the subscription priority field
			m_ps_priority.Empty();
		}

		// check if user data was returned
		if (parms.userDataLength > 0)
		{
			// set the user data field
			m_ps_user_data = userData;
		}
		else
		{
			// clear the user data
			m_ps_user_data.Empty();
		}

		// is the identity data being specified?
		if (m_ps_set_ident)
		{
			// get the identity data
			m_ps_appl_ident = applIdent;
			memset(tempToken, 0, sizeof(tempToken));
			AsciiToHex((unsigned char *)&acctToken, MQ_ACCOUNTING_TOKEN_LENGTH, (unsigned char *)&tempToken);
			m_ps_acct_token = tempToken;
		}

		// capture the correlation id for this subscription
		SetSubCorrelId(parms.correlId);
	}
	
	// process error message and refresh the controls on the screen
	UpdatePageData();

	// set the windows title to the subscription name
	if ((app->m_mainWnd != NULL) && (m_ps_subname.GetLength() > 0))
	{
		app->m_mainWnd->SetWindowText((LPCTSTR)m_ps_subname);
	}

	// set the buttons to match the subscription status
	setSubscriptionButtons();

	if (topic) rfhFree(topic);
	if (userData) rfhFree(userData);
	if (selection) rfhFree(selection);

	return;
}

/////////////////////////////////////////////////////////
//
// Routine to get a message using a subscription request
//
/////////////////////////////////////////////////////////

void CPS::OnPsGet() 

{
	// User has pressed the Subscribe button
	// Create a subscription
	int				waitTime=0;
	CRfhutilApp *	app;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);

	// check if there is a wait time
	if (m_ps_wait_interval.GetLength() > 0)
	{
		// convert the wait time to seconds
		waitTime = atoi((LPCTSTR)m_ps_wait_interval) * 1000;
	}
	
	// change the cursor to an hourglass
	app->BeginWaitCursor();

	// read a message based on the subscription request
	pDoc->getPSMessage((LPCTSTR)m_ps_subname, waitTime, &m_ps_is_retained);

	// return to a normal cursor
	app->EndWaitCursor();

	// Clear the file name, since we read the data from a queue
	pDoc->fileName[0] = 0;
	
	// process error message and refresh the controls on the screen
	UpdatePageData();

	// set the buttons to match the subscription status
	setSubscriptionButtons();

	// set the windows title to the subscription name
	if (app->m_mainWnd != NULL)
	{
		app->m_mainWnd->SetWindowText((LPCTSTR)m_ps_subname);
	}
}

//////////////////////////////////////////////////
//
// Routine to close a subscription request
//
//////////////////////////////////////////////////

void CPS::OnPsCloseq() 

{
	// User has pressed the Subscribe button
	// Create a subscription
	CRfhutilApp *	app;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);

	// change the cursor to an hourglass
	app->BeginWaitCursor();

	// process the close request
	pDoc->closePSQueue(m_ps_remove); 

	// return to a normal cursor
	app->EndWaitCursor();

	// process error message and refresh the controls on the screen
	UpdatePageData();

	// set the buttons to match the subscription status
	setSubscriptionButtons();
}

//////////////////////////////////////////////////
//
// Routine to publish a message
//
//////////////////////////////////////////////////

void CPS::OnPsPublish() 

{
	int		pubLevel=0;

	// User has pressed the Subscribe button
	// Create a subscription
	CRfhutilApp *	app;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);

	// change the cursor to an hourglass
	app->BeginWaitCursor();

	// convert the publication level to an integer
	pubLevel = atoi((LPCTSTR)m_ps_sub_level);

	// process the subscription
	pDoc->publishMsg((LPCTSTR)m_ps_qm, 
					 (LPCTSTR)m_ps_topic,
					 (LPCTSTR)m_ps_topicName,
					 pubLevel,
					 m_ps_retain,
					 m_ps_local,
					 m_ps_not_own,
					 m_ps_suppress_reply,
					 m_ps_warn_no_pub,
					 m_ps_new_correl_id,
					 m_ps_no_multicast);

	// done with the wait cursor
	// return to a normal cursor
	app->EndWaitCursor();

	// process error message and refresh the controls on the screen
	UpdatePageData();
}

void CPS::OnPsGetSubs() 

{
	CRfhutilApp *	app;
	char			traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CPS::OnPsGetSubs() ps_qm=%s broker_qm=%s", (LPCTSTR)m_ps_qm, (LPCTSTR)m_ps_broker_qm);

		// trace entry to OnPsGetSubs
		pDoc->logTraceEntry(traceInfo);
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	// put the current values into the shared DataArea object
	pDoc->m_QM_name = (LPCTSTR)m_ps_qm;
	pDoc->m_remote_QM = (LPCTSTR)m_ps_broker_qm;

	// set the cursor to an hourglass since this might take a while
	app = (CRfhutilApp *)AfxGetApp();
	app->BeginWaitCursor();

	// try to get the subscription names
	pDoc->loadNames((LPCTSTR)m_ps_qm, (LPCTSTR)m_ps_broker_qm, LOAD_NAMES_SUBS, 0, FALSE, FALSE);

	// done with the wait cursor
	// return to a normal cursor
	((CRfhutilApp *)AfxGetApp())->EndWaitCursor();

	// process error message and refresh the controls on the screen
	UpdatePageData();
}

void CPS::OnPsGetTopics() 

{
	CRfhutilApp *	app;
	char			traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CPS::OnPsGetTopics() ps_qm=%s broker_qm=%s", (LPCTSTR)m_ps_qm, (LPCTSTR)m_ps_broker_qm);

		// trace entry to OnPsGetTopics
		pDoc->logTraceEntry(traceInfo);
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	// put the current values into the shared DataArea object
	pDoc->m_QM_name = (LPCTSTR)m_ps_qm;
	pDoc->m_remote_QM = (LPCTSTR)m_ps_broker_qm;

	// set the cursor to an hourglass since this might take a while
	app = (CRfhutilApp *)AfxGetApp();
	app->BeginWaitCursor();

	// try to get the subscription names
	pDoc->loadNames((LPCTSTR)m_ps_qm, (LPCTSTR)m_ps_broker_qm, LOAD_NAMES_TOPICS, 0, FALSE, FALSE);

	// done with the wait cursor
	// return to a normal cursor
	((CRfhutilApp *)AfxGetApp())->EndWaitCursor();

	// process error message and refresh the controls on the screen
	UpdatePageData();
}

void CPS::loadEditBox()

{
	// populate the drop down list of queue manager names
	// this routine is located in the main application class
	// since it is also used in the pubsub class
	pDoc->loadQMComboBox((CComboBox *)&m_ps_qmComboBox);
}

BOOL CPS::OnInitDialog() 

{
	CPropertyPage::OnInitDialog();
	
	// tool tips are provided and must be initialized
	EnableToolTips(TRUE);
		
	// use the special MyComboBox subclass for the queue manager and queue name combo boxes
	m_ps_qmComboBox.SubclassDlgItem(IDC_PS_QM, this);
	m_ps_qComboBox.SubclassDlgItem(IDC_PS_QUEUE_NAME, this);
	m_ps_subComboBox.SubclassDlgItem(IDC_PS_SUBNAME, this);
	m_ps_topicNameComboBox.SubclassDlgItem(IDC_PS_TOPIC_NAME, this);

	// use the special ID control for the  correlation id
	// use the special CIdEdit subclass for the subscription correlation id edit box
	m_SubCorrelIdEdit.SubclassDlgItem(IDC_PS_CORREL_ID, this);

	// use the special ID control for the accounting token
	// change the maximum length and force it to hex mode
	m_SubAcctTokenEdit.setMaxLength(MQ_ACCOUNTING_TOKEN_LENGTH);
	m_SubAcctTokenEdit.SetHexOnly(TRUE);
	m_SubAcctTokenEdit.SetOvertype(TRUE);
	m_SubAcctTokenEdit.SubclassDlgItem(IDC_PS_ACCT_TOKEN, this);

	// initialize the id fields to overtype
	m_SubCorrelIdEdit.SetOvertype(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CPS::OnSetfocusPsQm() 

{
	// the focus is moving to the Queue Manager dialog box
	// The maximum size of the text must be set depending
	// on whether this is a client implementation or
	// normal server bindings are used.  If server bindings
	// are used then the name of the queue manager is limited
	// to 48 characters.  Otherwise the channel/TCP/hostname
	// combination can be up to 255 characters in length
#ifdef MQCLIENT
	m_ps_qmComboBox.LimitText(255);
#else
	m_ps_qmComboBox.LimitText(MQ_Q_NAME_LENGTH);
#endif
}

//////////////////////////////////////////////////////
//
// custom message handler to force the focus to the
// queue manager combo box control
//
//////////////////////////////////////////////////////

LONG CPS::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the queue manager name combo box
	//((CComboBox *)GetDlgItem(IDC_QM))->SetFocus();
	m_ps_qmComboBox.SetFocus();

	return 0;
}

BOOL CPS::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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


BOOL CPS::PreTranslateMessage(MSG* pMsg) 

{
	//
	// This routine is necessary for the tab key and tooltips to work correctly
	//

	if (pMsg->message != WM_KEYDOWN)
		return CPropertyPage::PreTranslateMessage(pMsg);

	// check for backspace key
	if (VK_BACK == pMsg->wParam)
	{
		CWnd * curFocus = GetFocus();
		if (curFocus != NULL)
		{
			int id = curFocus->GetDlgCtrlID();

			// check if this is an edit box control
			if ((IDC_PS_QM == id) || 
				(IDC_PS_BROKER_QM == id) || 
				(IDC_PS_SUBNAME == id) || 
				(IDC_PS_TOPIC == id) || 
				(IDC_PS_TOPIC_NAME == id) || 
				(IDC_PS_QUEUE_NAME == id) ||
				(IDC_PS_APPL_IDENT == id) ||
				(IDC_PS_CORREL_ID == id) ||
				(IDC_PS_SELECTION == id) ||
				(IDC_PS_USER_DATA == id) ||
				(IDC_PS_WAIT_INTERVAL == id))
			{
				processBackspace(curFocus);
				return TRUE;
			}
		}
	}

	if (IsDialogMessage(pMsg))
		return TRUE;
	else
		return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL CPS::PreCreateWindow(CREATESTRUCT& cs) 

{
	// Adjust the window style 
	cs.style |= WS_TABSTOP;
	cs.dwExStyle |= WS_EX_CONTROLPARENT;

	return CPropertyPage::PreCreateWindow(cs);
}

void CPS::OnDropdownPsTopicName() 

{
	int			added=0;
	const char	*namePtr=NULL;
	char		traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CPS::OnDropdownPsTopicName() ");

		// trace entry to OnDropdownPsTopicName
		pDoc->logTraceEntry(traceInfo);
	}

	// clear the current subscription names
	m_ps_topicNameComboBox.ResetContent();

	// see if we can find a list of queue names for the queue manager
	if (m_ps_qm.GetLength() > 0)
	{
		namePtr = pDoc->getTopicNamesListPtr((LPCTSTR)m_ps_qm);
	}
	else
	{
		namePtr = pDoc->getTopicNamesListPtr((LPCTSTR)pDoc->defaultQMname);
	}

	if (namePtr != NULL)
	{
		while (namePtr[0] != 0)
		{
			// add the name, skipping the first character which is a D or durable or N for non-durable
			m_ps_topicNameComboBox.AddString(namePtr);
			added++;						// in case trace is turned on

			if (pDoc->traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, " CPS::OnDropdownPsTopicName() adding=%s", namePtr);

				// trace entry to updateIdFields
				pDoc->logTraceEntry(traceInfo);
			}

			namePtr = Names::getNextName(namePtr);
		}
	}
					
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CPS::OnDropdownPsTopicName() added=%d namePtr=%8.8X", added, (unsigned int)namePtr);

		// trace exit from OnDropdownPsTopicName
		pDoc->logTraceEntry(traceInfo);
	}
}

///////////////////////////////////////
//
// Request publication button has been
// selected by the user.
//
///////////////////////////////////////

void CPS::OnPsReqPub() 

{
	// User has pressed the Subscribe button
	// Create a subscription
	CRfhutilApp *	app;

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// get the form data into the instance variables
	UpdateData(TRUE);
	
	// change the cursor to an hourglass
	app->BeginWaitCursor();

	// process the subscription
	pDoc->ReqPub((LPCTSTR)m_ps_qm, 
				 (LPCTSTR)m_ps_broker_qm,
				 (LPCTSTR)m_ps_topic,
				 (LPCTSTR)m_ps_subname);

	// return to a normal cursor
	app->EndWaitCursor();

	// Clear the file name, since we read the data from a queue
	pDoc->fileName[0] = 0;
	
	// process error message and refresh the controls on the screen
	UpdatePageData();

	// set the windows title to the subscription name
	if ((app->m_mainWnd != NULL) && (m_ps_subname.GetLength() > 0))
	{
		app->m_mainWnd->SetWindowText((LPCTSTR)m_ps_subname);
	}
}

void CPS::setSubscriptionButtons()

{
	// determine if a subscription is active 
	if (pDoc->isSubscriptionActive())
	{
		// subscription is active - set the buttons appropriately
		((CButton *)GetDlgItem(IDC_PS_SUBSCRIBE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_RESUME))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_PUBLISH))->EnableWindow(FALSE);
		// only durable subscriptions can be altered
		((CButton *)GetDlgItem(IDC_PS_ALTER_SUB))->EnableWindow(m_ps_durable);
		((CButton *)GetDlgItem(IDC_PS_GET))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_REQ_PUB))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_SAVE_MSGS))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_WRITE_MSGS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_CLOSEQ))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_CLEAR))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_MANAGED))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_DURABLE))->EnableWindow(FALSE);
//		((CButton *)GetDlgItem(IDC_PS_NOT_OWN))->EnableWindow(FALSE);
//		((CButton *)GetDlgItem(IDC_PS_SUPPRESS_REPLY))->EnableWindow(FALSE);
//		((CButton *)GetDlgItem(IDC_PS_ON_DEMAND))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_NEW_ONLY))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_GROUP_SUB))->EnableWindow(FALSE);
//		((CButton *)GetDlgItem(IDC_PS_SET_CORREL_ID))->EnableWindow(FALSE);
//		((CButton *)GetDlgItem(IDC_PS_ANY_USERID))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_PS_TOPIC))->SetReadOnly(TRUE);
	}
	else
	{
		// subscription is not active - set the buttons appropriately
		((CButton *)GetDlgItem(IDC_PS_SUBSCRIBE))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_RESUME))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_ALTER_SUB))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_PUBLISH))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_GET))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_REQ_PUB))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_SAVE_MSGS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_WRITE_MSGS))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_CLOSEQ))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_PS_CLEAR))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_MANAGED))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_DURABLE))->EnableWindow(TRUE);
//		((CButton *)GetDlgItem(IDC_PS_NOT_OWN))->EnableWindow(TRUE);
//		((CButton *)GetDlgItem(IDC_PS_SUPPRESS_REPLY))->EnableWindow(TRUE);
//		((CButton *)GetDlgItem(IDC_PS_ON_DEMAND))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_NEW_ONLY))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_PS_GROUP_SUB))->EnableWindow(TRUE);
//		((CButton *)GetDlgItem(IDC_PS_SET_CORREL_ID))->EnableWindow(TRUE);
//		((CButton *)GetDlgItem(IDC_PS_ANY_USERID))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_PS_TOPIC))->SetReadOnly(FALSE);
	}
}

void CPS::OnPsClear() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);
	
	// clear out all of the PS fields
	m_ps_user_data.Empty();
	m_ps_broker_qm.Empty();
	m_ps_topicName.Empty();
	m_ps_topic.Empty();
	m_ps_expiry.Empty();
	m_ps_priority.Empty();
	m_ps_sub_level.Empty();
	m_ps_subname.Empty();
	m_ps_wait_interval.Empty();
	m_ps_appl_ident.Empty();
	m_ps_durable = FALSE;
	m_ps_managed = FALSE;
	m_ps_group_sub = FALSE;
	m_ps_local = FALSE;
	m_ps_new_only = FALSE;
	m_ps_not_own = FALSE;
	m_ps_on_demand = FALSE;
	m_ps_remove = FALSE;
	m_ps_retain = FALSE;
	m_ps_warn_no_pub = FALSE;
	m_ps_set_correlid = FALSE;
	m_ps_suppress_reply = FALSE;
	m_ps_set_ident=FALSE;
	m_ps_any_userid = FALSE;
	m_ps_is_retained = FALSE;
	m_ps_correl_ascii = SUB_DISPLAY_HEX;
	m_ps_wildcard = PS_WILDCARD_TOPIC;
	m_ps_warn_no_pub = FALSE;

	// revert to overtype mode
	m_SubCorrelIdEdit.SetOvertype(TRUE);
	m_SubAcctTokenEdit.SetOvertype(TRUE);

	// set the varible to 64 zeros
	m_ps_acct_token = INIT_ACCT_TOKEN;

	// clear the subscription correlation id
	memset(m_correlid, 0, sizeof(m_correlid));
	SetSubCorrelId(&m_correlid);


	// update the form data from the instance variables
	UpdateData(FALSE);
}

void CPS::SetSubCorrelId(MQBYTE24 *id)

{
	int				i;
	unsigned char	tempStr[2 * MQ_CORREL_ID_LENGTH + 16] = { 0 };
	char			traceInfo[512];								// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		memset(tempStr, 0, sizeof(tempStr));
		AsciiToHex((unsigned char *)id, MQ_CORREL_ID_LENGTH, tempStr);
		sprintf(traceInfo, "Entering CPS::SetSubCorrelId() m_id_disp_ascii=%d id=%s", m_ps_correl_ascii, tempStr);

		// trace entry to SetSubCorrelId
		pDoc->logTraceEntry(traceInfo);
	}

	switch (m_ps_correl_ascii)
	{
	case SUB_DISPLAY_ASCII:
		// return value in ASCII
		// get the value into a temporary area
		memcpy(tempStr, id, MQ_CORREL_ID_LENGTH);

		// terminate the string
		tempStr[MQ_CORREL_ID_LENGTH] = 0;

		// replace any values less than a blank with blanks
		for (i=0; i<MQ_CORREL_ID_LENGTH; i++)
		{
			if (tempStr[i] < ' ')
			{
				tempStr[i] = ' ';
			}
		}

		break;

/*	case ID_DISPLAY_EBCDIC:
		// get a copy of the correlId and translate value to ASCII
		EbcdicToAscii((unsigned char *)id, MQ_CORREL_ID_LENGTH, tempStr);

		// terminate the string
		tempStr[MQ_CORREL_ID_LENGTH] = 0;

		// replace any values less than a blank with blanks
		for (i=0; i<MQ_CORREL_ID_LENGTH; i++)
		{
			if (tempStr[i] < ' ')
			{
				tempStr[i] = ' ';
			}
		}

		break;*/

	case SUB_DISPLAY_HEX:
		// return value in hex
		AsciiToHex((unsigned char *)id, MQ_CORREL_ID_LENGTH, tempStr);

		// terminate the string
		tempStr[2 * MQ_CORREL_ID_LENGTH] = 0;

		break;
	}

	// set the instance variable
	m_ps_sub_id = tempStr;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CPS:SetSubCorrelId() tempStr=%s", tempStr);

		// trace exit from SetSubCorrelId
		pDoc->logTraceEntry(traceInfo);
	}
}

void CPS::OnPsCorrelAscii() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);
	
	SetSubCorrelId(&m_correlid);

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void CPS::OnPsCorrelHex() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);
	
	SetSubCorrelId(&m_correlid);

	// update the form data from the instance variables
	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////
//
// Called whenever the user changes the value in the 
// combo box using the keyboard.
//
/////////////////////////////////////////////////////////

void CPS::OnEditchangePsSubname() 

{
	// Update the control variables, to make sure the subscription name is correct
//	UpdateData (TRUE);

	// get the current offset of the cursor
	// remember the new setting of the combo box
	((CComboBox *)GetDlgItem(IDC_PS_SUBNAME))->GetWindowText(m_ps_subname);

	// Update the controls
//	UpdateData (FALSE);
}

/////////////////////////////////////////////////////////
//
// Called whenever a dropdown list item is selected by 
// the user.
//
// When a subscription name is selected using the drop
// down list then the durable and managed options are
// automatically set as well as the queue and queue
// manager names.  This information is stored along with
// the subscription name in the names object.  The first
// two characters are indicators for durable and managed
// subscriptions.  The queue and queue manager names
// are stored at the end of the name, with ampersand
// characters between the subscription, queue and queue
// manager names.
//
/////////////////////////////////////////////////////////

void CPS::OnSelchangePsSubname() 

{
	// Subscription name is about to be changed by the user
	int			index;
	int			len;
	int			found=0;
	char		*ptr;				// pointer used to search for ampersand
	char		*ptr2;				// pointer used to search for ampersand
	const char	*namePtr=NULL;
	char		subName[256];
	char		sName[MQ_SUB_NAME_LENGTH+MQ_Q_MGR_NAME_LENGTH+MQ_Q_NAME_LENGTH+8];	// saved subscription name - consists of character D for durable followed by subname;

	UpdateData (TRUE);

	index = ((CComboBox *)GetDlgItem(IDC_PS_SUBNAME))->GetCurSel();
	subName[0] = 0;
	len = ((CComboBox *)GetDlgItem(IDC_PS_SUBNAME))->GetLBText(index, subName);
	if (len > 0)
	{
		m_ps_subname = subName;
	}
	else
	{
		m_ps_subname.Empty();
	}

	// try to find the type of destination
	// this requires trying to match the sub names list with the name that was selected
	// see if we can find a list of queue names for the queue manager
	if (m_ps_qm.GetLength() > 0)
	{
		namePtr = pDoc->getSubNamesListPtr((LPCTSTR)m_ps_qm);
	}
	else
	{
		namePtr = pDoc->getSubNamesListPtr((LPCTSTR)pDoc->defaultQMname);
	}

	if (namePtr != NULL)
	{
		// append an ampersand ("&") to the name in to a work area
		// this makes sure the comparison gets the right subscription
		strcat(subName, "&");
		len = strlen(subName);
		
		while ((namePtr[0] != 0) && (0 == found))
		{
			// check if this is the right subscription
			if (memcmp(namePtr + 2, subName, len) == 0)
			{
				// stop the loop
				found = 1;

				// set the managed check box
				if ('M' == namePtr[1])
				{
					// set managed subscription on
					m_ps_managed = TRUE;
				}
				else if ('P' == namePtr[1])
				{
					// turn off the managed subscription indicator
					m_ps_managed = FALSE;
				}

				// set the durable option
				if ('D' == namePtr[0])
				{
					// set durable sub on
					m_ps_durable = TRUE;
				}
				else if ('N' == namePtr[0])
				{
					// turn off durable sub
					m_ps_durable = FALSE;
				}

				// get the queue name and queue manager name
				strcpy(sName, namePtr + 2);

				// find the queue name
				// search for an ampersand ("&") character in the string
				ptr = strchr(sName, '&');
				if (ptr != NULL)
				{
					// terminate the string at the end of the subscription name
					ptr[0] = 0;

					// the queue name starts with the next character
					ptr++;

					// find the end of the queue name
					ptr2 = strchr(ptr, '&');

					if (ptr2 != NULL)
					{
						// terminate the queue name
						ptr2[0] = 0;

						// point to the remote QMgr name (may be null)
						ptr2++;

						// set the queue name
						m_ps_q = ptr;

						// is the queue manager the same as the connection QMgr?
						if (m_ps_qm.GetLength() > 0)
						{
							if (m_ps_qm.Compare(ptr2) == 0)
							{
								// clear the remote queue manager name
								m_ps_broker_qm.Empty();
							}
							else
							{
								// set the queue manager name
								m_ps_broker_qm = ptr2;
							}
						}
						else
						{
							// default queue manager name
							if (pDoc->defaultQMname.Compare(ptr2) == 0)
							{
								// clear the remote queue manager name
								pDoc->defaultQMname.Empty();
							}
							else
							{
								// set the queue manager name
								m_ps_broker_qm = ptr2;
							}
						}
					}
				}
			}

			// go on to the next name
			namePtr += strlen(namePtr) + 1;
		}
	}

	// set the value in the edit box
	((CComboBox *)GetDlgItem(IDC_PS_SUBNAME))->SetWindowText(m_ps_subname);

	// Update the controls
	UpdateData (FALSE);
}

/////////////////////////////////////////////////////////
//
// Called whenever a dropdown box selection is cancelled
// The previous value of the edit box is restored
//
/////////////////////////////////////////////////////////

void CPS::OnSelendcancelPsSubname() 

{
	// restore the previous setting of the combo box
	((CComboBox *)GetDlgItem(IDC_PS_SUBNAME))->SetWindowText(m_ps_subname);
}

/////////////////////////////////////////////////////////
//
// Called whenever the user changes the value in the 
// combo box using the keyboard.
//
/////////////////////////////////////////////////////////

void CPS::OnEditchangePsTopicName() 

{
	// Update the control variables, to make sure the topic name is correct
//	UpdateData (TRUE);

	// remember the new setting of the combo box
	((CComboBox *)GetDlgItem(IDC_PS_TOPIC_NAME))->GetWindowText(m_ps_topicName);

	// Update the controls
//	UpdateData (FALSE);
}

/////////////////////////////////////////////////////////
//
// Called whenever a dropdown box item is selected by 
// the user
//
/////////////////////////////////////////////////////////

void CPS::OnSelchangePsTopicName() 

{
	// Topic name is about to be changed by the user
	int		index;
	int		len;
	char	topName[256];

	UpdateData (TRUE);

	index = ((CComboBox *)GetDlgItem(IDC_PS_TOPIC_NAME))->GetCurSel();
	topName[0] = 0;
	len = ((CComboBox *)GetDlgItem(IDC_PS_TOPIC_NAME))->GetLBText(index, topName);
	if (len > 0)
	{
		m_ps_topicName = topName;
	}
	else
	{
		m_ps_topicName.Empty();
	}

	// set the value in the edit box
	((CComboBox *)GetDlgItem(IDC_PS_TOPIC_NAME))->SetWindowText(m_ps_topicName);

	// Update the controls
	UpdateData (FALSE);
}

/////////////////////////////////////////////////////////
//
// Called whenever a dropdown box selection is cancelled
// The previous value of the edit box is restored
//
/////////////////////////////////////////////////////////

void CPS::OnSelendcancelPsTopicName() 

{
	// restore the previous setting of the combo box
	((CComboBox *)GetDlgItem(IDC_PS_TOPIC_NAME))->SetWindowText(m_ps_topicName);
}

/////////////////////////////////////////////////////////
//
// Called whenever the user changes the value in the 
// combo box using the keyboard.
//
/////////////////////////////////////////////////////////

void CPS::OnEditchangePsQm() 

{
	// Update the control variables, to make sure the queue manager name is correct
//	UpdateData (TRUE);

	// remember the new setting of the combo box
	((CComboBox *)GetDlgItem(IDC_PS_QM))->GetWindowText(m_ps_qm);

	// Update the controls
//	UpdateData (FALSE);
}

/////////////////////////////////////////////////////////
//
// Called whenever a dropdown box item is selected by 
// the user
//
/////////////////////////////////////////////////////////

void CPS::OnSelchangePsQm() 

{
	// Queue manager name changed by the user
	int		index;
	int		len;
	char	qmName[256];

	UpdateData (TRUE);

	index = ((CComboBox *)GetDlgItem(IDC_PS_QM))->GetCurSel();
	qmName[0] = 0;
	len = ((CComboBox *)GetDlgItem(IDC_PS_QM))->GetLBText(index, qmName);
	if (len > 0)
	{
		m_ps_qm = qmName;
	}
	else
	{
		m_ps_qm.Empty();
	}

	// set the value in the edit box
	((CComboBox *)GetDlgItem(IDC_PS_QM))->SetWindowText(m_ps_qm);

	// Update the controls
	UpdateData (FALSE);
}

/////////////////////////////////////////////////////////
//
// Called whenever a dropdown box selection is cancelled
// The previous value of the edit box is restored
//
/////////////////////////////////////////////////////////

void CPS::OnSelendcancelPsQm() 

{
	// restore the previous setting of the combo box
	((CComboBox *)GetDlgItem(IDC_PS_QM))->SetWindowText(m_ps_qm);
}

/////////////////////////////////////////////////////////
//
// Called whenever the user changes the value in the 
// combo box using the keyboard.
//
/////////////////////////////////////////////////////////

void CPS::OnEditchangePsQueueName() 

{
	// Update the control variables, to make sure the topic name is correct
//	UpdateData (TRUE);

	// remember the new setting of the combo box
	((CComboBox *)GetDlgItem(IDC_PS_QUEUE_NAME))->GetWindowText(m_ps_q);

	// Update the controls
//	UpdateData (FALSE);
}

/////////////////////////////////////////////////////////
//
// Called whenever a dropdown box item is selected by 
// the user
//
/////////////////////////////////////////////////////////

void CPS::OnSelchangePsQueueName() 

{
	// Queue name changed by the user
	int		index;
	int		len;
	char	qName[256];

	UpdateData (TRUE);

	index = ((CComboBox *)GetDlgItem(IDC_PS_QUEUE_NAME))->GetCurSel();
	qName[0] = 0;
	len = ((CComboBox *)GetDlgItem(IDC_PS_QUEUE_NAME))->GetLBText(index, qName);
	if (len > 0)
	{
		m_ps_q = qName;
	}
	else
	{
		m_ps_q.Empty();
	}

	// set the value in the edit box
	((CComboBox *)GetDlgItem(IDC_PS_QUEUE_NAME))->SetWindowText(m_ps_q);

	// Update the controls
	UpdateData (FALSE);
}

/////////////////////////////////////////////////////////
//
// Called whenever a dropdown box selection is cancelled
// The previous value of the edit box is restored
//
/////////////////////////////////////////////////////////

void CPS::OnSelendcancelPsQueueName() 

{
	// restore the previous setting of the combo box
	((CComboBox *)GetDlgItem(IDC_PS_QUEUE_NAME))->SetWindowText(m_ps_q);
}

//////////////////////////////////////////////
//
// Routines to handle the accelerator keys
//
//////////////////////////////////////////////

void CPS::AlterSub()

{
	// check if pubsub is supported on this level of MQ
	if (pDoc->pubsubSupported && !(pDoc->isSubscriptionActive()))
	{
		// handle Alt + A
		OnPsAlterSub();
	}
}

void CPS::CloseQ()

{
	// check if pubsub is supported on this level of MQ
	if (pDoc->pubsubSupported && pDoc->isSubscriptionActive())
	{
		// handle Alt + C
		OnPsCloseq();
	}
}

void CPS::ReqPub()

{
	// check if pubsub is supported on this level of MQ
	if (pDoc->pubsubSupported && pDoc->isSubscriptionActive())
	{
		// handle Alt + E
		OnPsReqPub();
	}
}

void CPS::GetMsg()

{
	// check if pubsub is supported on this level of MQ
	if (pDoc->pubsubSupported && pDoc->isSubscriptionActive())
	{
		// handle Alt + G
		OnPsGet();
	}
}

void CPS::ClearAll()

{
	// handle Alt + L
	OnPsClear();
}

void CPS::GetSubs()

{
	// check if pubsub is supported on this level of MQ
	if (pDoc->pubsubSupported)
	{
		// handle Alt + N
		OnPsGetSubs();
	}
}

void CPS::Publish()

{
	// check if pubsub is supported on this level of MQ
	if (pDoc->pubsubSupported && !(pDoc->isSubscriptionActive()))
	{
		// handle Alt + P
		OnPsPublish();
	}
}

void CPS::Resume()

{
	// check if pubsub is supported on this level of MQ
	if (pDoc->pubsubSupported && !(pDoc->isSubscriptionActive()))
	{
		// handle Alt + R
		OnPsResume();
	}
}

void CPS::Subscribe()

{
	// check if pubsub is supported on this level of MQ
	if (pDoc->pubsubSupported && !(pDoc->isSubscriptionActive()))
	{
		// handle Alt + S
		OnPsSubscribe();
	}
}

void CPS::GetTopics()

{
	// check if pubsub is supported on this level of MQ
	if (pDoc->pubsubSupported)
	{
		// handle Alt + T
		OnPsGetTopics();
	}
}

void CPS::OnPsSaveMsgs() 

{
	_int64			msgCount=0;
	_int64			msgBytes=0;
	int				rc;
	bool			includeMQMD=false;
	bool			browseMsgs=true;
	bool			removeHdrs=false;
	bool			appendFile=false;
	CString			fileName;
	CCapPubs		dlg;
	char			traceInfo[512];		// work variable to build trace message

	// Update the control variables, to make sure we have the queue name correctly
	UpdateData (TRUE);
	
	// user has selected capture messages button
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CPS::OnPsSaveMsgs() m_ps_connect_qm=%s", (LPCTSTR)pDoc->currentQM);

		// trace entry to OnPsSaveMsgs
		pDoc->logTraceEntry(traceInfo);
	}

	// initialize the parameters area
	memset(&cparms, 0, sizeof(cparms));

	// set the pointer to the DataArea object
	cparms.pDocument = pDoc;
	dlg.parms = &cparms;

	// initialize some dialog fields from current selections
	dlg.m_incl_headers = !pDoc->m_dataonly;
	dlg.m_incl_mqmd = pDoc->m_write_include_MQMD;
	dlg.pDoc = pDoc;

	// run the dialog
	rc = dlg.DoModal();

	// did the user press the Close button or did the dialog close automatically?
	if (IDOK == rc)
	{
		// extract the data from the dialog
		fileName = dlg.m_filename;

		if (dlg.m_incl_mqmd)
		{
			includeMQMD = true;
		}

		// understand if MQ headers are to be stripped before messages are saved
		removeHdrs = !dlg.m_incl_headers;

		// get the file append option from the dialog
		appendFile = (dlg.m_append_file == TRUE);

		// get the maximum message count and total bytes captured
		msgCount = dlg.parms->count;
		msgBytes = dlg.parms->totalBytes;

		// generate a message to tell the user what happened
		pDoc->m_error_msg.Format("%s%I64d messages (%I64d bytes) captured to file %.192s", dlg.parms->errMsg, msgCount, msgBytes, (LPCTSTR)fileName);

		// update the controls including the message area
		UpdatePageData();
	}
	
	// record results of message capture
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CPS::OnPsSaveMsgs() rc=%d msgCount=%I64d msgBytes=%I64d includeMQMD=%d removeHdrs=%d appendFile=%d fileName=%.256s", rc, msgCount, msgBytes, includeMQMD, removeHdrs, appendFile, (LPCTSTR)fileName);

		// trace exit from OnPsSaveMsgs
		pDoc->logTraceEntry("Exiting CPS::OnPsSaveMsgs()");
	}
}

void CPS::OnPsWriteMsgs() 

{
	_int64			msgCount=0;
	_int64			noMatchCount=0;
	_int64			msgBytes=0;
	int				rc=0;
	CString			fileName;
	WritePubs		dlg;
	char			tempTxt[32];		// work variable for building no match message
	char			traceInfo[512];		// work variable to build trace message

	// Update the control variables, to make sure we have the queue name correctly
	UpdateData (TRUE);
	
	// user has selected capture messages button
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CPS::OnPsWriteMsgs() m_ps_connect_qm=%s", (LPCTSTR)pDoc->currentQM);

		// trace entry to OnPsWriteMsgs
		pDoc->logTraceEntry(traceInfo);
	}

	// initialize the parameters area
	memset(&wparms, 0, sizeof(wparms));
	memset(tempTxt, 0, sizeof(tempTxt));

	// set the pointer to the DataArea object
	wparms.pDocument = pDoc;
	dlg.parms = &wparms;

	// set a pointer to the DataArea object
	dlg.pDoc = pDoc;

	// set some parameters in the dialog
	strcpy(dlg.parms->topic, (LPCTSTR)m_ps_topic);
	dlg.parms->topicName = (LPCTSTR)m_ps_topicName;
	dlg.parms->QMname = (LPCTSTR)m_ps_qm;

	// set some publish options
	dlg.parms->pubLevel = atoi((LPCTSTR)m_ps_sub_level);
	dlg.parms->retained = m_ps_retain;
	dlg.parms->suppressReply = m_ps_suppress_reply;
	dlg.parms->warnNoSub = m_ps_warn_no_pub;
	dlg.parms->local = m_ps_local;

	// run the dialog
	rc = dlg.DoModal();

	// did the user press the Close button or did the dialog close automatically?
	if (IDOK == rc)
	{
		// extract the data from the dialog
		fileName = dlg.m_file_name;

		// get the maximum message count and total bytes written
		msgCount = dlg.parms->count;
		msgBytes = dlg.parms->totalBytes;
		noMatchCount = dlg.parms->noMatchCount;

		// were there any warnings about no matching subscribers?
		if (noMatchCount > 0)
		{
			sprintf(tempTxt, "(%I64d had no subscribers) ", noMatchCount);
		}

		// generate a message to tell the user what happened
		pDoc->m_error_msg.Format("%s%I64d messages %I64d bytes %spublished from file %.192s", dlg.parms->errMsg, msgCount, msgBytes, tempTxt, (LPCTSTR)fileName);

		// update the controls including the message area
		UpdatePageData();
	}
	
	// record results of message capture
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CPS::OnPsWriteMsgs() rc=%d msgCount=%I64d noMatchCount=%I64d msgBytes=%I64d fileName=%.256s", rc, msgCount, noMatchCount, msgBytes, (LPCTSTR)fileName);

		// trace exit from OnPsSaveMsgs
		pDoc->logTraceEntry(traceInfo);
	}
}

void CPS::SaveMsgs()

{
	// support for accelerator key - alt + V
	// check if there is a subscription - otherwise ignore the request
	if (pDoc->pubsubSupported && pDoc->isSubscriptionActive())
	{
		OnPsSaveMsgs();
	}
}

void CPS::WriteMsgs()

{
	// support for accelerator key - alt + M
	// check if there is a subscription - ignore the request if there is
	if (pDoc->pubsubSupported && !(pDoc->isSubscriptionActive()))
	{
		OnPsWriteMsgs();
	}
}
