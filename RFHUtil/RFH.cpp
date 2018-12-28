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

// RFH.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "mqsubs.h"
#include "xmlsubs.h"
#include "RFH.h"
#include "Usr.h"
#include "jms.h"
#include "PubSub.h"
#include "pscr.h"
#include "other.h"

// include for RFH2 constants, etc
#include <cmqpsc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+5

/////////////////////////////////////////////////////////////////////////////
// RFH property page

IMPLEMENT_DYNCREATE(RFH, CPropertyPage)

RFH::RFH() : CPropertyPage(RFH::IDD)
{
	//{{AFX_DATA_INIT(RFH)
	m_msg_domain = _T("");
	m_rfh_ccsid = _T("");
	m_rfh_charset = _T("");
	m_rfh_flags = _T("0");
	m_rfh_length = _T("");
	m_rfh_format = _T("");
	m_rfh1_format_name = _T("");
	m_rfh1_app_group = _T("");
	m_rfh1_charset = _T("");
	m_rfh1_flags = _T("");
	m_rfh1_format = _T("");
	m_msg_fmt = _T("");
	m_msg_set = _T("");
	m_msg_type = _T("");
	m_rfh1_length = _T("");
	m_rfh_encoding = NUMERIC_PC;
	m_rfh_pd_encoding = NUMERIC_PC;
	m_rfh_float_encoding = FLOAT_PC;
	m_rfh1_encoding = NUMERIC_PC;
	m_rfh1_pd_encoding = NUMERIC_PC;
	m_rfh1_float_encoding = FLOAT_PC;
	m_incl_jms = FALSE;
	m_incl_mcd = FALSE;
	m_incl_usr = FALSE;
	m_incl_pubsub = FALSE;
	m_incl_pscr = FALSE;
	m_incl_other = FALSE;
	m_incl_v1_mcd = FALSE;
	m_incl_v1_pubsub = FALSE;
	m_incl_v1_resp = FALSE;
	m_rfh1_include = FALSE;
	m_rfh2_include = FALSE;
	mcdDataChanged = false;
	//}}AFX_DATA_INIT

	mcdDataChanged = false;
	m_rfh_ccsid = "1208";
	m_rfh_data_len = 0;
	m_rfh_mcd_len = 0;
	m_rfh1_data_len = 0;
	rfh_data = NULL;
	rfh_mcd_area = NULL;
	rfh1_data = NULL;
	jmsData = NULL;
	pubsubData = NULL;
	pscrData = NULL;
	usrData = NULL;
	otherData = NULL;
	rfh_data_ccsid = -1;
	rfh_data_encoding = -1;
	rfh1_data_ccsid = -1;
	rfh1_data_encoding = -1;
	rfh_mcd_area_ccsid = -1;
	rfh_mcd_area_encoding = -1;
	pDoc = NULL;
	m_rfh1_data_changed = FALSE;
	m_rfh_data_changed = FALSE;
}

RFH::~RFH()
{
	// free any memory we might have acquired
	freeRfhArea();
	freeRfh1Area();
	freeMcdArea();
}

void RFH::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(RFH)
	DDX_Text(pDX, IDC_MSG_DOMAIN, m_msg_domain);
	DDX_Text(pDX, IDC_RFH_CCSID, m_rfh_ccsid);
	DDV_MaxChars(pDX, m_rfh_ccsid, 5);
	DDX_Text(pDX, IDC_RFH_CHARSET, m_rfh_charset);
	DDV_MaxChars(pDX, m_rfh_charset, 5);
	DDX_Text(pDX, IDC_RFH_FLAGS, m_rfh_flags);
	DDX_Text(pDX, IDC_RFH_LENGTH, m_rfh_length);
	DDX_Text(pDX, IDC_RFH_FORMAT, m_rfh_format);
	DDV_MaxChars(pDX, m_rfh_format, 8);
	DDX_Text(pDX, IDC_RFH1_FORMAT_NAME, m_rfh1_format_name);
	DDX_Text(pDX, IDC_RFH1_APP_GROUP, m_rfh1_app_group);
	DDX_Text(pDX, IDC_RFH1_CHARSET, m_rfh1_charset);
	DDV_MaxChars(pDX, m_rfh1_charset, 5);
	DDX_Text(pDX, IDC_RFH1_FLAGS, m_rfh1_flags);
	DDX_Text(pDX, IDC_RFH1_FORMAT, m_rfh1_format);
	DDV_MaxChars(pDX, m_rfh1_format, 8);
	DDX_Text(pDX, IDC_MSG_FMT, m_msg_fmt);
	DDX_Text(pDX, IDC_MSG_SET, m_msg_set);
	DDX_Text(pDX, IDC_MSG_TYPE, m_msg_type);
	DDX_Text(pDX, IDC_RFH1_LENGTH, m_rfh1_length);
	DDX_Radio(pDX, IDC_RFH_ENCODE_ASCII, m_rfh_encoding);
	DDX_Radio(pDX, IDC_RFH_PD_ENCODE_ASCII, m_rfh_pd_encoding);
	DDX_Radio(pDX, IDC_RFH_ENCODE_FLOAT_PC, m_rfh_float_encoding);
	DDX_Radio(pDX, IDC_RFH1_ENCODE_ASCII, m_rfh1_encoding);
	DDX_Radio(pDX, IDC_RFH1_PD_ENCODE_PC, m_rfh1_pd_encoding);
	DDX_Check(pDX, IDC_INCL_JMS, m_incl_jms);
	DDX_Check(pDX, IDC_INCL_MCD, m_incl_mcd);
	DDX_Check(pDX, IDC_INCL_USR, m_incl_usr);
	DDX_Check(pDX, IDC_INCL_PUBSUB, m_incl_pubsub);
	DDX_Check(pDX, IDC_INCL_PSCR, m_incl_pscr);
	DDX_Check(pDX, IDC_INCL_OTHER, m_incl_other);
	DDX_Check(pDX, IDC_INCL_V1_MCD, m_incl_v1_mcd);
	DDX_Check(pDX, IDC_INCL_V1_PUBSUB, m_incl_v1_pubsub);
	DDX_Check(pDX, IDC_INCL_V1_RESP, m_incl_v1_resp);
	DDX_Check(pDX, IDC_RFH1_INCLUDE_HEADER, m_rfh1_include);
	DDX_Check(pDX, IDC_RFH2_INCLUDE_HEADER, m_rfh2_include);
	DDX_Radio(pDX, IDC_RFH1_FLOAT_ENCODE_PC, m_rfh1_float_encoding);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(RFH, CPropertyPage)
	//{{AFX_MSG_MAP(RFH)
	ON_EN_CHANGE(IDC_MSG_DOMAIN, OnChangeMsgDomain)
	ON_EN_CHANGE(IDC_RFH_CHARSET, OnChangeRfhCharset)
	ON_EN_CHANGE(IDD_MSG_FMT, OnChangeMsgFmt)
	ON_EN_CHANGE(IDD_MSG_TYPE, OnChangeMsgType)
	ON_BN_CLICKED(IDC_RFH_ENCODE_ASCII, OnRfhEncodeAscii)
	ON_BN_CLICKED(IDC_RFH_ENCODE_EBCDIC, OnRfhEncodeEbcdic)
	ON_BN_CLICKED(IDC_INCL_JMS, OnInclJms)
	ON_BN_CLICKED(IDC_INCL_MCD, OnInclMcd)
	ON_BN_CLICKED(IDC_INCL_PUBSUB, OnInclPubsub)
	ON_BN_CLICKED(IDC_INCL_USR, OnInclUsr)
	ON_BN_CLICKED(IDC_INCL_PSCR, OnInclPscr)
	ON_BN_CLICKED(IDC_INCL_OTHER, OnInclOther)
	ON_CBN_EDITCHANGE(IDC_RFH_CCSID, OnEditchangeRfhCcsid)
	ON_BN_CLICKED(IDC_RFH_PD_ENCODE_ASCII, OnRfhPdEncodeAscii)
	ON_BN_CLICKED(IDC_RFH_PD_ENCODE_EBCDIC, OnRfhPdEncodeEbcdic)
	ON_BN_CLICKED(IDC_INCL_V1_MCD, OnInclV1Mcd)
	ON_BN_CLICKED(IDC_INCL_V1_PUBSUB, OnInclV1Pubsub)
	ON_EN_CHANGE(IDC_MSG_SET, OnChangeMsgSet)
	ON_EN_CHANGE(IDC_RFH1_APP_GROUP, OnChangeRfh1AppGroup)
	ON_EN_CHANGE(IDC_RFH1_FORMAT_NAME, OnChangeRfh1FormatName)
	ON_EN_CHANGE(IDC_RFH1_CHARSET, OnChangeRfh1Charset)
	ON_BN_CLICKED(IDC_RFH1_ENCODE_ASCII, OnRfh1EncodeAscii)
	ON_BN_CLICKED(IDC_RFH1_ENCODE_EBCDIC, OnRfh1EncodeEbcdic)
	ON_EN_CHANGE(IDC_RFH1_FORMAT, OnChangeRfh1Format)
	ON_BN_CLICKED(IDC_RFH1_PD_ENCODE_HOST, OnRfh1PdEncodeHost)
	ON_BN_CLICKED(IDC_RFH1_PD_ENCODE_PC, OnRfh1PdEncodePc)
	ON_EN_CHANGE(IDC_RFH_FORMAT, OnChangeRfhFormat)
	ON_BN_CLICKED(IDC_RFH2_INCLUDE_HEADER, OnRfh2IncludeHeader)
	ON_BN_CLICKED(IDC_RFH1_INCLUDE_HEADER, OnRfh1IncludeHeader)
	ON_BN_CLICKED(IDC_INCL_V1_RESP, OnInclV1Resp)
	ON_CBN_SELCHANGE(IDC_RFH_CCSID, OnSelchangeRfhCcsid)
	ON_CBN_SELENDOK(IDC_RFH_CCSID, OnSelendokRfhCcsid)
	ON_EN_CHANGE(IDC_MSG_FMT, OnChangeMsgFmt)
	ON_EN_CHANGE(IDC_MSG_TYPE, OnChangeMsgType)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, RFH::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, RFH::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// RFH message handlers

void RFH::setDisplay()

{
	// enable the appropriate RFH fields
	setV1();
	setV2();
}

void RFH::UpdatePageData()
{
	if (pDoc->traceEnabled)
	{
		// trace entry to UpdatePageData
		pDoc->logTraceEntry("Entering RFH::UpdatePageData()");
	}

	// the getMessage routine will update the variables from the document
	// by calling the updateAllViews routine, which will call the OnUpdate routine

	// update the form data from the instance variables
	UpdateData(FALSE);

	// set fields so they are properly enabled or disabled
	setDisplay();
}

BOOL RFH::OnSetActive() 
{
	// This dialog is about to become the active dialog,
	// so make sure the data is current
	if (pDoc->traceEnabled)
	{
		// trace entry to OnSetActive
		pDoc->logTraceEntry("Entering RFH::OnSetActive()");
	}

	// load the combo box for the ccsid code page
	((CComboBox *)GetDlgItem(IDC_RFH_CCSID))->ResetContent();
	((CComboBox *)GetDlgItem(IDC_RFH_CCSID))->AddString("1200");
	((CComboBox *)GetDlgItem(IDC_RFH_CCSID))->AddString("1208");
	((CComboBox *)GetDlgItem(IDC_RFH_CCSID))->AddString("13488");
	((CComboBox *)GetDlgItem(IDC_RFH_CCSID))->AddString("17584");

	UpdatePageData();
	
	// no header - select an option to add one
	((CButton *)GetDlgItem(IDC_RFH2_INCLUDE_HEADER))->SetFocus();

	// enable/disable the RFH fields for input
	setDisplay();

	BOOL ret = CPropertyPage::OnSetActive();
	
	// send a custom message to switch the focus to the QM combo box
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return ret;
}

BOOL RFH::OnKillActive() 
{
	// Make sure any data changes are reflected in the DataArea object
	jms		*jmsObj=(jms *)pDoc->jmsData;

	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering RFH::OnKillActive()");
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	if (mcdDataChanged)
	{
		// make sure we don't leave an old mcd area laying around
		freeMcdArea();

		// get rid of the RFH area as well since it has also changed
		freeRfhArea();

		// make sure the jms message type is up to date
		jmsObj->setJMSMsgType((LPCTSTR)m_msg_domain);
		
		mcdDataChanged = false;
	}

	if (m_rfh1_data_changed)
	{
		freeRfh1Area();
	}

	return CPropertyPage::OnKillActive();
}

///////////////////////////////////
//
// Routine to set the MQMD Format
// field and RFH fields to match 
// the setting of the RFH radio
// buttons
//
///////////////////////////////////

void RFH::OnRfhV2() 

{
	// Get the form data into the variables
	UpdateData(TRUE);

	if (m_rfh_ccsid.GetLength() == 0)
	{
		m_rfh_ccsid = "1208";
		mcdDataChanged = true;
	}

	UpdateData(FALSE);

	// enable the RFH fields
	setV2();
}

///////////////////////////////////
//
// Routine to enable or disable 
// editing of the individual RFH1
// fields.
//
///////////////////////////////////

void RFH::setV1()

{
	if (m_rfh1_include)
	{
		if (m_incl_v1_mcd)
		{
			((CEdit *)GetDlgItem(IDC_RFH1_APP_GROUP))->SetReadOnly(FALSE);
			((CEdit *)GetDlgItem(IDC_RFH1_FORMAT_NAME))->SetReadOnly(FALSE);
		}
		else
		{
			((CEdit *)GetDlgItem(IDC_RFH1_APP_GROUP))->SetReadOnly(TRUE);
			((CEdit *)GetDlgItem(IDC_RFH1_FORMAT_NAME))->SetReadOnly(TRUE);
		}

		// turn on the RFH1 fields
		((CEdit *)GetDlgItem(IDC_RFH1_FORMAT))->SetReadOnly(FALSE);
		((CEdit *)GetDlgItem(IDC_RFH1_CHARSET))->SetReadOnly(FALSE);
		((CButton *)GetDlgItem(IDC_INCL_V1_MCD))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_V1_PUBSUB))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_V1_RESP))->EnableWindow(TRUE);
	}
	else
	{
		// turn off the RFH1 fields
		((CEdit *)GetDlgItem(IDC_RFH1_APP_GROUP))->SetReadOnly(TRUE);
		((CEdit *)GetDlgItem(IDC_RFH1_FORMAT_NAME))->SetReadOnly(TRUE);
		((CEdit *)GetDlgItem(IDC_RFH1_FORMAT))->SetReadOnly(TRUE);
		((CEdit *)GetDlgItem(IDC_RFH1_CHARSET))->SetReadOnly(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_V1_MCD))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_INCL_V1_PUBSUB))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_INCL_V1_RESP))->EnableWindow(FALSE);
	}
}

///////////////////////////////////
//
// Routine to enable or disable 
// editing of the individual RFH2
// fields.
//
///////////////////////////////////

void RFH::setV2()

{
	if (m_rfh2_include)
	{
		// Do we only want the mcd folder in the rfh2?
		if (m_incl_mcd)
		{
			((CEdit *)GetDlgItem(IDC_MSG_DOMAIN))->SetReadOnly(FALSE);
			((CEdit *)GetDlgItem(IDC_MSG_SET))->SetReadOnly(FALSE);
			((CEdit *)GetDlgItem(IDC_MSG_TYPE))->SetReadOnly(FALSE);
			((CEdit *)GetDlgItem(IDC_MSG_FMT))->SetReadOnly(FALSE);
		}
		else
		{
			((CEdit *)GetDlgItem(IDC_MSG_DOMAIN))->SetReadOnly(TRUE);
			((CEdit *)GetDlgItem(IDC_MSG_SET))->SetReadOnly(TRUE);
			((CEdit *)GetDlgItem(IDC_MSG_TYPE))->SetReadOnly(TRUE);
			((CEdit *)GetDlgItem(IDC_MSG_FMT))->SetReadOnly(TRUE);
		}

		((CEdit *)GetDlgItem(IDC_RFH_FORMAT))->SetReadOnly(FALSE);
		((CEdit *)GetDlgItem(IDC_RFH_CHARSET))->SetReadOnly(FALSE);
		((CComboBox *)GetDlgItem(IDC_RFH_CCSID))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_MCD))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_JMS))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_USR))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_PUBSUB))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_PSCR))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_OTHER))->EnableWindow(TRUE);
	}
	else
	{
		// turn off the RFH2 fields
		((CEdit *)GetDlgItem(IDC_MSG_DOMAIN))->SetReadOnly(TRUE);
		((CEdit *)GetDlgItem(IDC_MSG_SET))->SetReadOnly(TRUE);
		((CEdit *)GetDlgItem(IDC_MSG_TYPE))->SetReadOnly(TRUE);
		((CEdit *)GetDlgItem(IDC_MSG_FMT))->SetReadOnly(TRUE);
		((CEdit *)GetDlgItem(IDC_RFH_CHARSET))->SetReadOnly(TRUE);
		((CEdit *)GetDlgItem(IDC_RFH_FORMAT))->SetReadOnly(TRUE);
		((CComboBox *)GetDlgItem(IDC_RFH_CCSID))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_INCL_MCD))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_INCL_JMS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_INCL_USR))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_INCL_PUBSUB))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_INCL_PSCR))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_INCL_OTHER))->EnableWindow(FALSE);
	}
}

void RFH::OnChangeRfhFormat() 

{
	// remember that the data on this page has been changed
	mcdDataChanged = true;
}

void RFH::OnChangeMsgDomain() 

{
	// remember that the data on this page has been changed
	mcdDataChanged = true;
}

void RFH::OnChangeMsgSet() 

{
	// remember that the data on this page has been changed
	mcdDataChanged = true;
}

void RFH::OnChangeMsgFmt() 

{
	// remember that the data on this page has been changed
	mcdDataChanged = true;
}

void RFH::OnChangeMsgType() 

{
	// remember that the data on this page has been changed
	mcdDataChanged = true;
}

void RFH::OnChangeRfhCharset()
 
{
	// remember that the data on this page has been changed
	m_rfh_data_changed = true;
	mcdDataChanged = true;
}

void RFH::OnRfhEncodeAscii()
 
{
	// remember that the data on this page has been changed
	m_rfh_data_changed = true;
	mcdDataChanged = true;
}

void RFH::OnRfhEncodeEbcdic() 

{
	// remember that the data on this page has been changed
	m_rfh_data_changed = true;
	mcdDataChanged = true;
}

void RFH::OnInclJms() 

{
	// remember that the data on this page has been changed
	m_rfh_data_changed = true;
	
	// update the the instance variables from the dialog data
	UpdateData(TRUE);
}

void RFH::OnInclMcd()
 
{
	// Get the form data into the variables
	UpdateData(TRUE);

	// remember that the data on this page has been changed
	mcdDataChanged = true;
	m_rfh_data_changed = true;

	// set fields so they are properly enabled or disabled
	setDisplay();
	
	// update the form data from the instance variables
	UpdateData(FALSE);
}

void RFH::OnInclPubsub()
 
{
	// remember that the data on this page has been changed
	m_rfh_data_changed = true;
	
	// update the the instance variables from the dialog data
	UpdateData(TRUE);
}

void RFH::OnInclUsr() 

{
	// remember that the data on this page has been changed
	m_rfh_data_changed = true;
	
	// update the the instance variables from the dialog data
	UpdateData(TRUE);
}

void RFH::OnInclPscr() 

{
	// remember that the data on this page has been changed
	m_rfh_data_changed = true;
	
	// update the the instance variables from the dialog data
	UpdateData(TRUE);
}

void RFH::OnInclOther() 

{
	// remember that the data on this page has been changed
	m_rfh_data_changed = true;
	
	// update the the instance variables from the dialog data
	UpdateData(TRUE);
}

void RFH::OnInclV1Mcd() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// get the latest data from the form
	UpdateData(TRUE);

	// set fields so they are properly enabled or disabled
	setDisplay();
}

void RFH::OnInclV1Pubsub() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// get the latest data from the form
	UpdateData(TRUE);
}

void RFH::OnInclV1Resp() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// get the latest data from the form
	UpdateData(TRUE);
}

void RFH::OnEditchangeRfhCcsid() 

{
	int		codepage;

	// remember that the data on this page has been changed
	mcdDataChanged = true;

	// Get the form data into the variables
	UpdateData(TRUE);

	codepage = atoi(m_rfh_ccsid);
	if ((codepage != 1200) && (codepage != 1208) && (codepage != 13488) && (codepage != 17584))
	{
		MessageBox("Code page must be 1200, 1208, 13488 or 17584", 
				   "Invalid value",
				   MB_ICONEXCLAMATION  | MB_OK );
	}

	// the ccsid of the variable part of the RFH2 has changed so release the current da
	freeRfhArea();

	// update the form data from the instance variables
	UpdateData(FALSE);
}

///////////////////////////////////////////
//
// The user has selected a value in the 
// drop down list.
//
// This routine will retrieve the selected
// text and then update the data in the
// edit box part of the control.  This 
// will then drive the Selchange routine
// below which will validate the new
// code page value.
//
///////////////////////////////////////////

void RFH::OnSelendokRfhCcsid() 

{
	CComboBox *	cb=(CComboBox *)GetDlgItem(IDC_RFH_CCSID);

	// remember that the data on this page has been changed
	mcdDataChanged = true;

	// get the ccsid that was selected
	int sel = cb->GetCurSel( );
	cb->GetLBText(sel, m_rfh_ccsid);

	// update the form data from the instance variables
	// this will result in the OnSelchangeRfhCcsid() routine being executed
	UpdateData(FALSE);
}

void RFH::OnSelchangeRfhCcsid() 

{
	CComboBox *	cb=(CComboBox *)GetDlgItem(IDC_RFH_CCSID);

	// remember that the data on this page has been changed
	mcdDataChanged = true;

	// get the ccsid that was selected
	int sel = cb->GetCurSel( );
	cb->GetLBText(sel, m_rfh_ccsid);

	// update the form data from the instance variables
	// this will result in the OnSelchangeRfhCcsid() routine being executed
	UpdateData(FALSE);
}

void RFH::OnRfhPdEncodeAscii() 

{
	// remember that the data on this page has been changed
	mcdDataChanged = true;
}

void RFH::OnRfhPdEncodeEbcdic() 

{
	// remember that the data on this page has been changed
	mcdDataChanged = true;
}

BOOL RFH::OnInitDialog()
 
{
	CPropertyPage::OnInitDialog();
	
	// Enable tool tips for this dialog
	EnableToolTips(TRUE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL RFH::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

BOOL RFH::PreTranslateMessage(MSG* pMsg) 

{
	// this routine is necessary to allow the tab
	// key to be used to move between fields
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
			if ((IDC_MSG_DOMAIN == id) || 
				(IDC_MSG_SET == id) || 
				(IDC_MSG_TYPE == id) || 
				(IDC_MSG_FMT == id) || 
				(IDC_RFH1_APP_GROUP == id) || 
				(IDC_RFH1_FORMAT_NAME == id) || 
				(IDC_RFH_FORMAT == id) || 
				(IDC_RFH1_FORMAT == id) || 
				(IDC_RFH_CHARSET == id) || 
				(IDC_RFH1_CHARSET == id))
			{
				processBackspace(curFocus);
				mcdDataChanged = true;
				m_rfh1_data_changed = TRUE;
				return TRUE;
			}
		}
	}

	if (IsDialogMessage(pMsg))
		return TRUE;
	else
		return CPropertyPage::PreTranslateMessage(pMsg);
}

void RFH::OnChangeRfh1AppGroup()
 
{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// make sure the instance variables are up to date
	UpdateData(TRUE);
}

void RFH::OnChangeRfh1FormatName() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// make sure the instance variables are up to date
	UpdateData(TRUE);
}

void RFH::OnChangeRfh1Charset() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// make sure the instance variables are up to date
	UpdateData(TRUE);
}

void RFH::OnRfh1EncodeAscii() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// make sure the instance variables are up to date
	UpdateData(TRUE);
}

void RFH::OnRfh1EncodeEbcdic() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// make sure the instance variables are up to date
	UpdateData(TRUE);
}

void RFH::OnChangeRfh1Format() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// make sure the instance variables are up to date
	UpdateData(TRUE);
}

void RFH::OnRfh1PdEncodeHost() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// make sure the instance variables are up to date
	UpdateData(TRUE);
}

void RFH::OnRfh1PdEncodePc() 

{
	// remember that the data on this page has been changed
	m_rfh1_data_changed = TRUE;

	// make sure the instance variables are up to date
	UpdateData(TRUE);
}

//////////////////////////////////////////////////////////////
//
// This routine sets the focus to a particular control.
// This is necessary for the tab key to be used to 
// move between controls..
// 
//////////////////////////////////////////////////////////////

LONG RFH::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the queue manager name combo box
	((CButton *)GetDlgItem(IDC_RFH2_INCLUDE_HEADER))->SetFocus();

	return 0;
}

/////////////////////////////////////////////////////
//
// Routine to parse an RFH V1 header.
//
// This routine will parse the fixed part of the
// header and pass any variable part to another
// routine for parsing.
//
/////////////////////////////////////////////////////

int RFH::parseRFH(unsigned char *rfhptr, int msglength, int ccsid, int encoding)

{
	MQRFH	tempRFH;
	char	tempEbcdic[MQ_FORMAT_LENGTH + 8];
	char	tempFormat[MQ_FORMAT_LENGTH + 8];
	char	*tempfield;
	int		rfhlength=0;
	int		i=0;
	int		charSet;
	char	traceInfo[512];		// work variable to build trace message

	// convert the ccsid to a character set type (ascii/ebcdic)
	charSet = getCcsidType(ccsid);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::parseRFH() msglength=%d ccsid=%d encoding=%d", msglength, ccsid, encoding);

		// trace entry to parseRFH
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("rfh fixed area", rfhptr, sizeof(MQRFH));
	}

	// return if there is not at least 32 bytes of data
	if (msglength < MQRFH_STRUC_LENGTH_FIXED)
	{
		// indicate there is no RFH header
		return rfhlength;
	}

	// get a copy of the first 32 bytes of the message
	memcpy(&tempRFH, rfhptr, sizeof(tempRFH));

	// check if we have to translate the StrucId and Format to EBCDIC
	// check for an RFH2 in EBCDIC
	if (charSet == CHAR_EBCDIC)
	{
		// translate the structure identifier to ascii
		EbcdicToAscii((unsigned char *) &tempRFH.StrucId, sizeof(tempRFH.StrucId), (unsigned char *)tempEbcdic);
		memcpy(&tempRFH.StrucId, tempEbcdic, sizeof(tempRFH.StrucId));

		// translate the format field to ascii
		EbcdicToAscii((unsigned char *) &tempRFH.Format, MQ_FORMAT_LENGTH, (unsigned char *)tempEbcdic);
		memcpy(&tempRFH.Format, tempEbcdic, MQ_FORMAT_LENGTH);
	}

	// check for host integer format for the RFH header
	convertRFHeader(&tempRFH, encoding);

	// check for an MQSI RFH structure at the beginning of the message
	// and that the message is long enough to have one
	if ((memcmp(tempRFH.StrucId, MQRFH_STRUC_ID, sizeof(tempRFH.StrucId)) == 0))
	{
		// Get the total length from the header
		rfhlength = tempRFH.StrucLength;

		m_rfh1_length.Format("%d", rfhlength);

		// check if the length is valid
		if ((rfhlength > MQRFH_STRUC_LENGTH_FIXED) && (rfhlength <= msglength))
		{
			// data seems to include an RFH header - try to parse it
			// set the RFH to yes
			m_rfh1_include = TRUE;

			// Go on, the length makes sense
			// Isolate the RFH header format name
			memset(tempFormat, 0, sizeof(tempFormat));
			memcpy(tempFormat, tempRFH.Format, sizeof(tempRFH.Format));
			m_rfh1_format = tempFormat;

			// get the code page of the next header or the user data
			m_rfh1_charset.Format("%d", tempRFH.CodedCharSetId);		// code page of RFH header

			pDoc->m_char_format = getCcsidType(tempRFH.CodedCharSetId);	// set data display format
			m_rfh1_flags.Format("%d", tempRFH.Flags);

			// get the encoding of the next header or the user data
			m_rfh1_encoding = getIntEncode(tempRFH.Encoding);

			// set the encoding type
			if ((tempRFH.Encoding & MQENC_INTEGER_REVERSED) > 0)
			{
				// normal PC little endian
				pDoc->m_numeric_format = NUMERIC_PC;
			}
			else
			{
				pDoc->m_numeric_format = NUMERIC_HOST;
			}

			m_rfh1_pd_encoding = getPDEncode(tempRFH.Encoding);
			pDoc->m_pd_numeric_format = m_rfh1_pd_encoding;
			m_rfh1_float_encoding = getFloatEncode(tempRFH.Encoding);
			pDoc->m_float_numeric_format = m_rfh1_float_encoding;

			// find out how much variable part there is
			i = rfhlength - MQRFH_STRUC_LENGTH_FIXED;

			// is there a variable part?
			if (i > 0)
			{
				// allocate an area for the variable part of the message
				tempfield = (char *)rfhMalloc(i + 33, "RFHTEMPF");
	
				// see if we can find an app group or msg type in the header
				memset(tempfield, 0, i +32);

				// Isolate the variable part of the RFH as a string
				// check if it is in EBCDIC or ASCII
				if (CHAR_EBCDIC == charSet)
				{
					EbcdicToAscii(rfhptr + MQRFH_STRUC_LENGTH_FIXED, i, (unsigned char *) tempfield);
				}
				else
				{
					memcpy(tempfield, rfhptr + MQRFH_STRUC_LENGTH_FIXED, i);
				}

				// parse the variable part of the rfh header
				parseRFH1VarArea(tempfield, i);

				// release the allocated memory
				rfhFree(tempfield);
			}
		}
		else
		{
			pDoc->m_error_msg = "***Invalid RFH Length field*** ";

			// set the length to zero
			rfhlength = 0;
		}
	}

	// save the current rfh data
	setRfh1Area(rfhptr, rfhlength, ccsid, encoding);

	// make sure the controls contain the same data that was just parsed
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting parseRFH - rfhlength=%d m_rfh1_charset=%s m_rfh1_encoding=%d", rfhlength, (LPCTSTR)m_rfh1_charset, m_rfh1_encoding);

		// trace exit from parseRFH
		pDoc->logTraceEntry(traceInfo);
	}

	// return the length of the RFH
	return rfhlength;
}

//////////////////////////////////////////////////
//
// Routine to build convert the fields in an RFH 
// V1 header to match the codepage and encoding
//
//////////////////////////////////////////////////

void RFH::convertRFHeader(MQRFH *tempRFH, int encodeType)

{
	// check if we have to change the encoding of the integers in the RFH header
	if (NUMERIC_HOST == encodeType)
	{
		// we need to reverse the integers in all of the RFH header fields
		tempRFH->Version        = reverseBytes4(tempRFH->Version);
		tempRFH->StrucLength    = reverseBytes4(tempRFH->StrucLength);
		tempRFH->Encoding       = reverseBytes4(tempRFH->Encoding);
		tempRFH->CodedCharSetId = reverseBytes4(tempRFH->CodedCharSetId);
		tempRFH->Flags          = reverseBytes4(tempRFH->Flags);
	}
}

//////////////////////////////////////////////////
//
// Routine to build convert the fields in an RFH2 
// header to match the codepage and encoding
//
//////////////////////////////////////////////////

void RFH::convertRFH2Header(MQRFH2 *tempRFH, int encodeType)

{
	// check if we have to change the encoding of the integers in the RFH header
	if (NUMERIC_HOST == encodeType)
	{
		// we need to reverse the integers in all of the RFH header fields
		tempRFH->Version        = reverseBytes4(tempRFH->Version);
		tempRFH->StrucLength    = reverseBytes4(tempRFH->StrucLength);
		tempRFH->Encoding       = reverseBytes4(tempRFH->Encoding);
		tempRFH->CodedCharSetId = reverseBytes4(tempRFH->CodedCharSetId);
		tempRFH->Flags          = reverseBytes4(tempRFH->Flags);
		tempRFH->NameValueCCSID = reverseBytes4(tempRFH->NameValueCCSID);
	}
}

//////////////////////////////////////////////////////////
//
// Routine to search for and parse the variable part of an
//  RFH1 header at the beginning of the message data
//
//////////////////////////////////////////////////////////

void RFH::parseRFH1VarArea(char *rfhptr, int msglength)

{
	int		found;
	int		oth;
	PubSub	*pubsubObj=(PubSub*)pubsubData;
	pscr	*pscrObj=(pscr*)pscrData;
	char	*endptr;
	char	*tempOther;
	char	ch;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::parseRFH1VarArea() msglength=%d", msglength);

		// trace entry to parseRFH1VarArea
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("RFH1 data", (unsigned char *)rfhptr, msglength);
	}

	// initialize the other field
	tempOther = (char *)rfhMalloc(msglength + 1, "RFHTEMPO");
	oth = 0;

	endptr = rfhptr + msglength;

	while ((rfhptr < endptr) && (rfhptr[0] != 0))
	{
		// find the beginning of the next item
		rfhptr = skipBlanks(rfhptr);

		if ((rfhptr < endptr) && (rfhptr[0] != 0))
		{
			found = 0;
			// find out what item this represents
			if (memcmp(rfhptr, MQNVS_APPL_TYPE, sizeof(MQNVS_APPL_TYPE) - 1) == 0)
			{
				found = 1;
				m_incl_v1_mcd = TRUE;
				rfhptr += sizeof(MQNVS_APPL_TYPE);
				rfhptr = skipBlanks(rfhptr);
				m_rfh1_app_group = rfhptr;
			}

			if ((0 == found) && 
				(memcmp(rfhptr, MQNVS_MSG_TYPE, sizeof(MQNVS_MSG_TYPE) - 1) == 0))
			{
				found = 1;
				m_incl_v1_mcd = TRUE;
				rfhptr += sizeof(MQNVS_MSG_TYPE);
				rfhptr = skipBlanks(rfhptr);
				m_rfh1_format_name = rfhptr;
			}

			if (0 == found)
			{
				rfhptr = pubsubObj->parseV1Pubsub(rfhptr, &found);

				if (1 == found)
				{
					m_incl_v1_pubsub = TRUE;
				}
			}

			if (0 == found)
			{
				rfhptr = pscrObj->parseV1Pscr(rfhptr, &found);

				if (1 == found)
				{
					m_incl_v1_resp = TRUE;
				}
			}

			// check for something we did not recognize
			if (0 == found)
			{
				// dump this into the other area on the pscr tab
				if (oth > 0)
				{
					// insert a blank if not at the beginning
					tempOther[oth++] = ' ';
				}

				// check for double quotes
				if ('\"' == rfhptr[0])
				{
					ch = '\"';
					tempOther[oth++] = '\"';
				}
				else
				{
					ch = ' ';
				}

				while ((rfhptr[0] >= ' ') && (rfhptr[0] != ch) && (oth < msglength))
				{
					tempOther[oth++] = rfhptr++[0];
				}

				if ('\"' == rfhptr[0])
				{
					tempOther[oth++] = rfhptr++[0];
				}
			}
		}
	}

	if (oth > 0)
	{
		tempOther[oth] = 0;
		m_incl_v1_resp = TRUE;
		pscrObj->setPscrOther(tempOther);
	}

	rfhFree(tempOther);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting parseRFH m_incl_v1_pubsub=%d m_incl_v1_resp=%d oth=%d", m_incl_v1_pubsub, m_incl_v1_resp, oth);

		// trace exit from parseRFH
		pDoc->logTraceEntry(traceInfo);
	}
}

void RFH::setRfh1Area(unsigned char *rfhData, int dataLen, int ccsid, int encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::setRfh1Area() rfh1_data=%8.8X m_rfh1_data_len=%d rfh1_data_ccsid=%d rfh1_data_encoding=%d", (unsigned int)rfh1_data, m_rfh1_data_len, rfh1_data_ccsid, rfh1_data_encoding);

		// trace entry to setRfh1Area
		pDoc->logTraceEntry(traceInfo);
	}

	// free any old area
	freeRfh1Area();

	if ((rfhData != NULL) && (dataLen > 0))
	{
		rfh1_data = (unsigned char *)rfhMalloc(dataLen + 1,"RFH1DATA");
		memcpy(rfh1_data, rfhData, dataLen);
		rfh1_data[dataLen] = 0;
		m_rfh1_data_len = dataLen;
		rfh1_data_ccsid = ccsid;
		rfh1_data_encoding = encoding;
		m_rfh1_data_changed = FALSE;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::setRfh1Area() rfh1_data=%8.8X m_rfh1_data_len=%d rfh1_data_ccsid=%d rfh1_data_encoding=%d", (unsigned int)rfh1_data, m_rfh1_data_len, rfh1_data_ccsid, rfh1_data_encoding);

		// trace entry to setRfh1Area
		pDoc->logTraceEntry(traceInfo);
	}
}

void RFH::freeRfh1Area()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::freeRfh1Area() rfh1_data=%8.8X m_rfh1_data_len=%d rfh1_data_ccsid=%d rfh1_data_encoding=%d", (unsigned int)rfh1_data, m_rfh1_data_len, rfh1_data_ccsid, rfh1_data_encoding);

		// trace entry to freeRfh1Area
		pDoc->logTraceEntry(traceInfo);
	}

	if (rfh1_data != NULL)
	{
		rfhFree(rfh1_data);
	}
		
	rfh1_data = NULL;
	m_rfh1_data_len = 0;
	rfh1_data_ccsid = -1;
	rfh1_data_encoding = -1;
}

char * RFH::parseRFH1String(char *ptr, char *value, int maxSize)

{
	int		i;
	char	ch;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::parseRFH1String() ptr=%s maxSize=%d", ptr, maxSize);

		// trace entry to parseRFH1String
		pDoc->logTraceEntry(traceInfo);
	}

	ptr = skipBlanks(ptr);

	if ('\"' == ptr[0])
	{
		ch = '\"';
	}
	else
	{
		ch = ' ';
	}

	i = 0;
	while ((ptr[0] >= ' ') && (ptr[0] != ch) && (i < maxSize))
	{
		value[i] = ptr[0];
		i++;
		ptr++;
	}

	if ('\"' == ptr[0])
	{
		ptr++;
	}

	value[i] = 0;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::parseRFH1String() ptr=%s value=%s", ptr, value);

		// trace exit from parseRFH1String
		pDoc->logTraceEntry(traceInfo);
	}

	return ptr;
}

//////////////////////////////////////////////////////////
//
// Routine to process an RFH2 header. This includes 
// processing the fixed part of the header and then 
// calling another routine to process the variable part.
//
//////////////////////////////////////////////////////////

int RFH::parseRFH2(unsigned char *rfhptr, int msglength, int ccsid, int encoding)

{
	MQRFH2	tempRFH;
	char	tempEbcdic[MQ_FORMAT_LENGTH + 1];
	char	tempfield[MQ_FORMAT_LENGTH + 4];
	int		rfhlength=0;
	int		varlength=0;
	int		charSet;
	char	traceInfo[512];		// work variable to build trace message

	// convert the ccsid to a code page type (ascii/ebcdic)
	charSet = getCcsidType(ccsid);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::parseRFH2() msglength=%d ccsid=%d charSet=%d encoding=%d", msglength, ccsid, charSet, encoding);

		// trace entry to parseRFH2
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("rfh2 fixed area", rfhptr, sizeof(MQRFH2));
	}

	// convert the ccsid to a code page type (ascii/ebcdic)
	charSet = getCcsidType(ccsid);

	// get a copy of the first 36 bytes of the message
	memcpy(&tempRFH, rfhptr, sizeof(tempRFH));

	// check if we have to translate the StrucId and Format from EBCDIC to ASCII
	// check for an RFH2 in EBCDIC
	if (CHAR_EBCDIC == charSet)
	{
		// translate the structure identifier to ascii
		EbcdicToAscii((unsigned char *) &tempRFH.StrucId, sizeof(tempRFH.StrucId), (unsigned char *)tempEbcdic);
		memcpy(&tempRFH.StrucId, tempEbcdic, sizeof(tempRFH.StrucId));

		// translate the format field to ascii
		EbcdicToAscii((unsigned char *) &tempRFH.Format, MQ_FORMAT_LENGTH, (unsigned char *)tempEbcdic);
		memcpy(&tempRFH.Format, tempEbcdic, MQ_FORMAT_LENGTH);
	}

	// check for host integer format for the RFH header
	convertRFH2Header(&tempRFH, encoding);

	// check for an MQSI RFH structure at the beginning of the message
	// and that the message is long enough to have one
	if ((memcmp(tempRFH.StrucId, MQRFH_STRUC_ID, sizeof(tempRFH.StrucId)) == 0))
	{
		// Get the total length from the header
		rfhlength = tempRFH.StrucLength;

		// check if the length is valid
		if ((rfhlength >= MQRFH_STRUC_LENGTH_FIXED_2) && (rfhlength <= msglength))
		{
			// Go on, the length makes sense
			// data seems to include an RFH header - try to parse it
			// set the RFH to version 2
			m_rfh2_include = TRUE;
			m_rfh_length.Format("%d", rfhlength);

			// Isolate the RFH header format name, which is the format of the user data
			memset(tempfield, 0, sizeof(tempfield));
			memcpy(tempfield, tempRFH.Format, sizeof(tempRFH.Format));
			m_rfh_format = tempfield;
			m_rfh_charset.Format("%d", tempRFH.CodedCharSetId);			// format of user data
			pDoc->m_char_format = getCcsidType(tempRFH.CodedCharSetId);	// set data display format
			m_rfh_flags.Format("%d", tempRFH.Flags);
			m_rfh_ccsid.Format("%d", tempRFH.NameValueCCSID);			// format of body of RFH header

			m_rfh_encoding = getIntEncode(tempRFH.Encoding);
			if ((tempRFH.Encoding & MQENC_INTEGER_REVERSED) > 0)
			{
				// normal PC little endian
				pDoc->m_numeric_format = NUMERIC_PC;
			}
			else
			{
				pDoc->m_numeric_format = NUMERIC_HOST;
			}

			m_rfh_pd_encoding = getPDEncode(tempRFH.Encoding);
			pDoc->m_pd_numeric_format = m_rfh_pd_encoding;
			m_rfh_float_encoding = getFloatEncode(tempRFH.Encoding);
			pDoc->m_float_numeric_format = m_rfh_float_encoding;

			// save a copy of the rfh header 
			setRfhArea(rfhptr, rfhlength, ccsid, encoding);

			// parse the variable part of the rfh2
			if (rfhlength > MQRFH_STRUC_LENGTH_FIXED_2 + 4)
			{
				parseRFH2data(rfhptr + MQRFH_STRUC_LENGTH_FIXED_2, 
							  rfhlength - MQRFH_STRUC_LENGTH_FIXED_2, 
							  tempRFH.NameValueCCSID,
							  encoding);
			}
		}
		else
		{
			pDoc->m_error_msg = "***Invalid RFH Length field*** ";

			// set the length to zero
			rfhlength = 0;
		}
	}

	// update the form data from the instance variables
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting parseRFH2 - rfhlength=%d m_rfh_charset=%s m_rfh_encoding=%d", rfhlength, (LPCTSTR)m_rfh_charset, m_rfh_encoding);

		// trace exit from parseRFH2
		pDoc->logTraceEntry(traceInfo);
	}

	// return the length of the RFH
	return rfhlength;
}

//////////////////////////////////////////////////////////
//
// Routine to process the variable part of an RFH2 header.
//
//////////////////////////////////////////////////////////

void RFH::parseRFH2data(unsigned char *rfhData, int dataLen, int ccsid, int encoding)

{
	int				remaining=dataLen;
	int				segLen;
	int				saveOfs;
	int				bytesNeeded;
	Usr				*usrData=(Usr *)pDoc->usrData;
	jms				*jmsData=(jms *)pDoc->jmsData;
	PubSub			*pubData=(PubSub *)pDoc->pubData;
	pscr			*pscrData=(pscr *)pDoc->pscrData;
	other			*otherData=(other *)pDoc->otherData;
	wchar_t			*ucsPtr;
	unsigned char	*segPtr=rfhData;
	unsigned char	*ptr;				// pointer to the folder data after translation
	unsigned char	*freePtr=NULL;		// pointer to an area that we have allocated because the folder is longer than varData can hold
	unsigned char	saveWord[8];		// used to preserve and restore data so we can add a terminator to the original data
	unsigned char	folderName[512];	// name of the folder
	unsigned char	varData[4096];		// temporary work area to hold one folder at a time without having to malloc
	char			traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::parseRFH2data() dataLen=%d ccsid=%d encoding=%d", dataLen, ccsid, encoding);

		// trace entry to parseRFH2data
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("rfhData variable area", rfhData, dataLen);
	}

	while (remaining > 4)
	{
		// get the length of the first segment
		memcpy(&segLen, segPtr, 4);

		// check if we have to change the encoding of the integers in the RFH header
//		if (NUMERIC_HOST == encoding)
		// be more pragmatic in deciding whether to reverse the byte encoding
		if ((segLen < 0) || (segLen > remaining))
		{
			// need to reverse the bytes in the segment length field
			segLen = reverseBytes4(segLen);
		}

		// point to the actual data
		segPtr += 4;

		// remember to account for the 4 byte length field
		remaining -= 4;

		// check to make sure the length is valid
		if ((segLen > 0) && (segLen <= remaining))
		{
			// allow for the fact that the data may be UCS-2
			if (isUCS2(ccsid))
			{
				// the number of  bytes needed may be larger since 2 bytes may become 3 bytes
				bytesNeeded = (segLen * 2) + 2;
			}
			else
			{
				// just allow for a string delimiter
				bytesNeeded = segLen + 1;
			}

			// process the segment
			// Do not overrun our data area if the length is too big
			if (bytesNeeded < sizeof(varData))
			{
				ptr = varData;
				freePtr = NULL;
			}
			else
			{
				ptr = (unsigned char *)rfhMalloc(bytesNeeded + 16, "RFHVARDT");
				freePtr = ptr;
			}

			// remember the original length of the folder
			// this is necessary in case the data is translated from UCS2 to multi-byte
			saveOfs = segLen;

			// check if we need to translate the name value part of the RFH2
			if (getCcsidType(ccsid) == CHAR_EBCDIC)
			{
				// EBCDIC data is technically not allowed in the RFH2 variable area
				// but just in case it should be handled
				EbcdicToAscii(segPtr, segLen, ptr); 
			}
			else
			{
				if (isUCS2(ccsid))
				{
					// check if the data is big-endian
					if (NUMERIC_HOST == encoding)
					{
						// the data is big-endian UCS-2 so the individual characters
						// must have their bytes swapped before they can be translated
						// first allocate a temporary work area for the byte swapped data
						ucsPtr = (wchar_t *)rfhMalloc(segLen + 4, "RFHUCS  ");

						// make sure the malloc worked
						if (ucsPtr != NULL)
						{
							// initialize the storage we just acquired
							memset(ucsPtr, 0, segLen + 4);

							// copy the input data into the temporary work area
							memcpy(ucsPtr, segPtr, segLen);

							// next the individual bytes must be reversed
							int i=0;
							int j=segLen >> 1;			// each character is two bytes
							while (i < j)
							{
								// reverse the bytes in the UCS-2 character
								short s = ucsPtr[i];
								s = reverseBytes(&s);
								ucsPtr[i] = s;

								// move on to the next character
								i++;
							}

							// translate from UCS2 to multi-byte 
							// the multibyte data is in ptr and the original UCS data is at ucsPtr
							segLen = UCS2ToMultiByte((unsigned char *)ucsPtr, segLen, ptr, bytesNeeded);

							// release the storage we acquired
							rfhFree(ucsPtr);

							// check if the translation worked
							if (-1 == segLen)
							{
								if (pDoc->traceEnabled)
								{
									// create the trace line
									sprintf(traceInfo, "parseRFH2data - Unable to translate from UCS-2 segLen=%d remaining=%d", segLen, remaining);

									// trace error in parseRFH2data
									pDoc->logTraceEntry(traceInfo);
								}

								// indicate the data is invalid
								pDoc->m_error_msg += "Error translating RFH2 data from UCS-2\r\n";
							}
						}
						else
						{
							// indicate the malloc failure in the trace if enabled
							if (pDoc->traceEnabled)
							{
								// trace exit from parseRFH
								pDoc->logTraceEntry("*****Error malloc failed in RFH:parseRFH2data");
							}
						}
					}
					else
					{
						// the data must be properly terminated as a string
						// save the two bytes of data at the end of this segment
						memcpy(saveWord, segPtr + saveOfs, 2);

						// make sure the data we want to translate is properly terminated
						// this is safe because we always malloc extra bytes when we read a file or a message
						segPtr[saveOfs] = 0;
						segPtr[saveOfs + 1] = 0;

						// translate from UCS2 to multi-byte 
						segLen = UCS2ToMultiByte(segPtr, segLen, ptr, bytesNeeded);

						// restore the two bytes we temporarily changed
						memcpy(segPtr + saveOfs, saveWord, 2);
					}
				}
				else
				{
					// simply copy the data
					memcpy((char *)ptr, (char *)segPtr, segLen);

					// terminate the new string
					ptr[segLen] = 0;
				}
			}

			// find the open bracket and extract the folder name
			getFolderName(folderName, ptr, segLen);

			/* see what kind of data this is and process it */
			if (strcmp((char *)folderName, MQRFH2_MSG_CONTENT_FOLDER) == 0)
			{
				// save the original data
				setMcdArea(segPtr, saveOfs, ccsid, encoding);

				// process the mcd folder
				parseRFH2mcd(ptr, segLen);

				// check for a jms domain
				jmsData->setJMSMsgType((LPCTSTR)m_msg_domain);
			}
			else
			{
				if (strcmp((char *)folderName, MQRFH2_USER_FOLDER) == 0)
				{
					// remember we found the data
					m_incl_usr = TRUE;

					// capture the existing folder contents
					usrData->setUsrArea(segPtr, saveOfs, ccsid, encoding);

					// process the usr folder
					usrData->parseRFH2usr(ptr, segLen, ccsid, encoding);
				}
				else
				{
					if (strcmp((char *)folderName, "jms") == 0)
					{
						// remember we found the data
						m_incl_jms = TRUE;

						// save the original data
						jmsData->setJmsArea(segPtr, saveOfs, ccsid, encoding);

						// process the jms folder
						jmsData->parseRFH2jms(ptr, segLen);
					}
					else
					{
						if (strcmp((char *)folderName, MQRFH2_PUBSUB_CMD_FOLDER) == 0)
						{
							// remember we found the data
							m_incl_pubsub = TRUE;

							// save the raw data
							pubData->setPubsubArea(segPtr, saveOfs, ccsid, encoding);

							// process the publish/subscribe folder
							pubData->parseRFH2pubsub(ptr, segLen);
						}
						else
						{
							if (strcmp((char *)folderName, MQRFH2_PUBSUB_RESP_FOLDER) == 0)
							{
								// remember we found the data
								m_incl_pscr = TRUE;

								// process the publish/subscribe response folder (N.B.this will free any existing area)
								pscrData->parseRFH2pscr(ptr, segLen);

								// save the raw data
								pscrData->setPscrArea(segPtr, saveOfs, ccsid, encoding);
							}
							else
							{
								// check for translation errors and obviously invalid data
								if (segLen > 6)
								{
									// remember we found the data
									m_incl_other = TRUE;

									// process the unrecognized folder
									// the entire contents will be saved in raw format,
									// including the length
									// note that the raw data is not saved, since there
									// may be multiple unrecognized folders
									otherData->parseRFH2other(ptr, segLen, ccsid);
								}
							}
						}
					}
				}
			}

			// release any acquired areas
			if (freePtr != NULL)
			{
				rfhFree(freePtr);
				freePtr = NULL;
			}

			// check for a UCS-2 translation error
			if (-1 == segLen)
			{
				// error in translation - get out
				remaining = 0;
			}
			else
			{
				// move on to the next folder in the variable area
				segPtr += saveOfs;
				remaining -= saveOfs;
			}
		}
		else
		{
			// note the error in the trace
			if (pDoc->traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, "Invalid length in RFH2 variable area segLen=%d remaining=%d", segLen, remaining);

				// trace entry to parseRFH2data
				pDoc->logTraceEntry(traceInfo);
			}

			// indicate the data is invalid
			pDoc->m_error_msg += "Invalid length in RFH2 variable area - data skipped\r\n";

			// stop any further processing
			remaining = 0;
		}
	}

	if (pDoc->traceEnabled)
	{
		// trace exit from parseRFH
		pDoc->logTraceEntry("Exiting RFH:parseRFH2data");
	}
}

void RFH::setRfhArea(unsigned char *rfhData, int dataLen, int ccsid, int encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::setRfhArea() rfh_data=%8.8X m_rfh_data_len=%d rfh_data_ccsid=%d rfh_data_encoding=%d", (unsigned int)rfh_data, m_rfh_data_len, rfh_data_ccsid, rfh_data_encoding);

		// trace entry to setRfhArea
		pDoc->logTraceEntry(traceInfo);
	}

	// free any old area
	freeRfhArea();

	if ((rfhData != NULL) && (dataLen > 0))
	{
		// allocate storage for the RFH and copy the data to the new storage
		rfh_data = (unsigned char *)rfhMalloc(dataLen + 1, "RFHDATA ");
		memcpy(rfh_data, rfhData, dataLen);
		rfh_data[dataLen] = 0;

		// set the length, code page and encoding of the RFH
		m_rfh_data_len = dataLen;
		rfh_data_ccsid = ccsid;
		rfh_data_encoding = encoding;
		m_rfh_data_changed = FALSE;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::setRfhArea() rfh_data=%8.8X m_rfh_data_len=%d rfh_data_ccsid=%d rfh_data_encoding=%d", (unsigned int)rfh_data, m_rfh_data_len, rfh_data_ccsid, rfh_data_encoding);

		// trace exit from setRfhArea
		pDoc->logTraceEntry(traceInfo);
	}
}

void RFH::freeRfhArea()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::freeRfhArea() rfh_data=%8.8X m_rfh_data_len=%d rfh_data_ccsid=%d rfh_data_encoding=%d", (unsigned int)rfh_data, m_rfh_data_len, rfh_data_ccsid, rfh_data_encoding);

		// trace entry to freeRfhArea
		pDoc->logTraceEntry(traceInfo);
	}

	// check if there is a current area allocated
	if (rfh_data != NULL)
	{
		// release the storage
		rfhFree(rfh_data);
	}

	// initialize the data pointer, length, ccsid and encoding
	rfh_data = NULL;
	m_rfh_data_len = 0;
	rfh_data_ccsid = -1;
	rfh_data_encoding = -1;
}

///////////////////////////////////////////////////////
//
// Routine to find and isolate the folder name in 
// an RFH2 header.
//
///////////////////////////////////////////////////////

void RFH::getFolderName(unsigned char *folderName, unsigned char *rfhdata, int segLen)

{
	int		i;
	int		j;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getFolderName() rfhdata=%8.8s segLen=%d", rfhdata, segLen);

		// trace entry to getFolderName
		pDoc->logTraceEntry(traceInfo);
	}

	// initialize the folder name string to a zero length string
	folderName[0] = 0;

	// initialize the offset and find the beginning bracket
	i = 0;
	while ((i < segLen) && (rfhdata[i] != '<'))
	{
		i++;
	}

	if (i < segLen)
	{
		// skip the begin bracket
		i++;

		// skip any blanks before the tag name
		while ((i < segLen) && (' ' == rfhdata[i]))
		{
			i++;
		}

		/* get the folder name */
		j = 0;
		while ((i < segLen) && (rfhdata[i] > ' ') && (rfhdata[i] != '>') && (j < 255))
		{
			folderName[j] = rfhdata[i];
			i++;
			j++;
		}

		// terminate the folder name string
		folderName[j] = 0;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::getFolderName() folderName=%s", folderName);

		// trace exit from getFolderName
		pDoc->logTraceEntry(traceInfo);
	}
}

void RFH::setMcdArea(unsigned char *mcdData, int dataLen, int ccsid, int encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::setMcdArea() mcdData=%8.8X dataLen=%d ccsid=%d , int encoding=%d", (unsigned int)mcdData, dataLen, ccsid, encoding);

		// trace entry to setMcdArea
		pDoc->logTraceEntry(traceInfo);
	}

	// free any old area
	freeMcdArea();

	if ((mcdData != NULL) && (dataLen > 0))
	{
		// allocate storage and copy the data
		rfh_mcd_area = (unsigned char *)rfhMalloc(dataLen + 1, "RFHMCD  ");
		memcpy(rfh_mcd_area, mcdData, dataLen);
		rfh_mcd_area[dataLen] = 0;

		// set the length and ccsid
		m_rfh_mcd_len = dataLen;
		rfh_mcd_area_ccsid = ccsid;
		rfh_mcd_area_encoding = encoding;
	}
}

void RFH::freeMcdArea()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::freeMcdArea() rfh_mcd_area=%8.8X m_rfh_mcd_len=%d", (unsigned int)rfh_mcd_area, m_rfh_mcd_len);

		// trace entry to freeMcdArea
		pDoc->logTraceEntry(traceInfo);
	}

	if (rfh_mcd_area != NULL)
	{
		rfhFree(rfh_mcd_area);
	}

	// initialize the pointer and length to indicate no current memory is allocated
	rfh_mcd_area = NULL;
	m_rfh_mcd_len = 0;
}

///////////////////////////////////////////////////////
//
// Routine to parse the mcd folder in an RFH2 header.
// This folder is used by the message broker to 
// indicate the format of the user data.
//
///////////////////////////////////////////////////////

void RFH::parseRFH2mcd(unsigned char *rfhdata, int dataLen)

{
	int		found;
	bool	more;
	char	*ptr;
	char	*endptr;
	char	tempDomain[MAX_FORMAT_NAME + 1];
	char	tempMsgType[MAX_FORMAT_NAME + 1];
	char	tempMsgSet[MAX_FORMAT_NAME + 1];
	char	tempMsgFmt[MAX_FORMAT_NAME + 1];
	char	errmsg[64];
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::parseRFH2mcd() rfhdata=%8.8X dataLen=%d", (unsigned int)rfhdata, dataLen);

		// trace entry to parseRFH2mcd
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("mcd rfhdata", rfhdata, dataLen);
	}

	// remember we found the data
	m_incl_mcd = TRUE;
			
	// initialize domain, message set, message type and message fmt fields
	memset(tempDomain, 0, sizeof(tempDomain));
	memset(tempMsgSet, 0, sizeof(tempMsgSet));
	memset(tempMsgType, 0, sizeof(tempMsgType));
	memset(tempMsgFmt, 0, sizeof(tempMsgFmt));

	// Search for the fields in the mcd folder
	ptr = (char *)rfhdata + 5;

	// check for an empty folder as represented by <mcd/>
	// skip any blanks after the <mcd folder name - there really shouldn't be any - this is really paranoid
	ptr = skipBlanks(ptr);

	// is the next character a slash?
	if ('/' == ptr[0])
	{
		// the folder is empty so don't do anything else
		return;
	}

	// find the end of the mcd folder
	endptr = ptr;
	while ((endptr < ((char *)rfhdata + dataLen - 5)) && 
		   (memcmp(endptr, MQRFH2_MSG_CONTENT_FOLDER_E, sizeof(MQRFH2_MSG_CONTENT_FOLDER_E) - 1)))
	{
		endptr++;
	}

	// find the initial begin bracket
	more = true;
	while (more)
	{
		// find the next begin bracket
		while ((ptr < endptr) && (ptr[0] != '<') && (ptr[0] != 0))
		{
			ptr++;
		}

		// did we find what we were looking for?
		if ((ptr < endptr) && ('<' == ptr[0]))
		{
			if (memcmp(ptr, MQRFH2_MSG_CONTENT_FOLDER_E, sizeof(MQRFH2_MSG_CONTENT_FOLDER_E) - 1) == 0)
			{
				more = false;
			}
			else
			{
				// see if we recognize the tag
				// any unrecognized tags will be ignored
				found = 0;
				ptr = processTag(ptr, MQMCD_MSG_DOMAIN_B, tempDomain, sizeof(tempDomain), &found);
				ptr = processTag(ptr, MQMCD_MSG_SET_B, tempMsgSet, sizeof(tempMsgSet), &found);
				ptr = processTag(ptr, MQMCD_MSG_TYPE_B, tempMsgType, sizeof(tempMsgType), &found);
				ptr = processTag(ptr, MQMCD_MSG_FORMAT_B, tempMsgFmt, sizeof(tempMsgFmt), &found);

				if (0 == found)
				{
					sprintf(errmsg, "Invalid entry in msd folder %5.5s\r\n", ptr);
					pDoc->m_error_msg += errmsg;

					// break out of a potential loop if there is an invalid name in the data
					ptr++;
				}
			}
		}
		else
		{
			// no more brackets - time to leave
			more = false;
		}
	}

	// capture the domain name
	m_msg_domain = tempDomain;

	// capture the message set
	m_msg_set = tempMsgSet;

	// capture the message type
	m_msg_type = tempMsgType;

	// capture the message fmt
	m_msg_fmt = tempMsgFmt;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::parseRFH2mcd() m_msg_domain=%s m_msg_set=%s m_msg_type=%s m_msg_fmt=%s", tempDomain, tempMsgSet, tempMsgType, tempMsgFmt);

		// trace exit from parseRFH2mcd
		pDoc->logTraceEntry(traceInfo);
	}
}

//////////////////////////////////////////////////
//
// Routine to build an RFH2 header
//
//////////////////////////////////////////////////

int RFH::buildRFH2(int ccsid, int encoding)

{
	MQRFH2	tempRFH = {MQRFH2_DEFAULT};
	char	tempFormat[MQ_FORMAT_LENGTH + 1];
	unsigned char	*ptr;
	unsigned char	*tempbuf;
	int		rfhlength=MQRFH_STRUC_LENGTH_FIXED_2;
	unsigned int	varlength=0;
	unsigned int	worklen;
	// pointers to the individual variable segments of the RFH
	const char	*rfh_jms_area=NULL;
	const char	*rfh_pubsub_area=NULL;
	const char	*rfh_pscr_area=NULL;
	const char	*rfh_usr_area=NULL;
	const char	*rfh_other_area=NULL;
	// lengths of the individual variable segments of the RFH
	int		m_RFH_jms_len=0;
	int		m_RFH_pubsub_len=0;
	int		m_RFH_pscr_len=0;
	int		m_RFH_usr_len=0;
	int		m_RFH_other_len=0;
	int		rfhccsid;
	// pointers to the other RFH objects
	Usr		*usrObj=(Usr *)pDoc->usrData;
	jms		*jmsObj=(jms *)pDoc->jmsData;
	PubSub	*pubObj=(PubSub *)pDoc->pubData;
	pscr	*pscrObj=(pscr *)pDoc->pscrData;
	other	*otherObj=(other *)pDoc->otherData;

	// working storage
	char	tempEbcdicfield[MQRFH_STRUC_LENGTH_FIXED_2 + 4];
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::buildRFH2() m_rfh_data_len=%d ccsid=%d encoding=%d m_rfh_ccsid=%s m_rfh_data_changed=%d mcdDataChanged=%d", m_rfh_data_len, ccsid, encoding, (LPCTSTR)m_rfh_ccsid, m_rfh_data_changed, mcdDataChanged);

		// trace entry to buildRFH2
		pDoc->logTraceEntry(traceInfo);
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	// check if any of the subfolders have been modified or the ccsid or encoding has been changed
	if (m_rfh_data_changed || mcdDataChanged || usrObj->wasDataChanged() || jmsObj->wasDataChanged() || pubObj->wasDataChanged() || pscrObj->wasDataChanged() || otherObj->wasDataChanged())
	{
		freeRfhArea();
	} else if ((ccsid != rfh_data_ccsid) || (encoding != rfh_data_encoding))
	{
		freeRfhArea();
	}

	if (rfh_data != NULL)
	{
		if (pDoc->traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Exiting RFH::buildRFH2() reusing existing area m_rfh_data_len=%d", m_rfh_data_len);

			// trace exit from buildRFH
			pDoc->logTraceEntry(traceInfo);

			// write the data to the trace file
			pDoc->dumpTraceData("V2 rfhdata", rfh_data, m_rfh_data_len);
		}

		// no need to build the RFH2
		return m_rfh_data_len;
	}

	// get the code page for the variable parts
	rfhccsid = atoi((LPCTSTR)m_rfh_ccsid);

	// first, build the variable part of the RFH
	if (m_incl_mcd)
	{
		buildMcdArea(rfhccsid, encoding);
		rfhlength += m_rfh_mcd_len + 4;
	}

	if (m_incl_jms)
	{
		// check if we already have a jms area built
		m_RFH_jms_len = jmsObj->buildJmsArea(rfhccsid, encoding);
		if (m_RFH_jms_len > 0)
		{
			rfh_jms_area = jmsObj->getJmsArea(rfhccsid, encoding);
			rfhlength += m_RFH_jms_len + 4;
		}
	}

	if (m_incl_usr)
	{
		m_RFH_usr_len = usrObj->buildUsrArea(rfhccsid, encoding);

		if (m_RFH_usr_len > 0)
		{
			rfh_usr_area = usrObj->getUsrArea();
			rfhlength += m_RFH_usr_len + 4;
		}
	}

	if (m_incl_other)
	{
		m_RFH_other_len = otherObj->buildOtherArea(rfhccsid, encoding);

		if (m_RFH_other_len > 0)
		{
			rfh_other_area = otherObj->getOtherArea();
			rfhlength += m_RFH_other_len;
		}
	}

	if (m_incl_pubsub)
	{
		m_RFH_pubsub_len = pubObj->buildPubsubArea(rfhccsid, encoding);
		
		if (m_RFH_pubsub_len > 0)
		{
			rfhlength += m_RFH_pubsub_len + 4;
			rfh_pubsub_area = pubObj->getPubsubArea(rfhccsid, encoding);
		}
	}

	if (m_incl_pscr)
	{
		m_RFH_pscr_len = pscrObj->buildPscrArea(rfhccsid, encoding);

		if (m_RFH_pscr_len > 0)
		{
			rfh_pscr_area = pscrObj->getPscrArea();
			rfhlength += m_RFH_pscr_len + 4;
		}
	}

	// allocate storage for the RFH2 header
	tempbuf = (unsigned char *)rfhMalloc(rfhlength + 16, "RFHTBUF ");

	// next, build the fixed part of the rfh
	// we build it after the variable part, 
	// so that we can set the total length
	tempRFH.StrucLength = rfhlength;
	if (atoi((LPCTSTR)m_rfh_charset) != 0)
	{
		tempRFH.CodedCharSetId = atoi((LPCTSTR)m_rfh_charset);
	}

	tempRFH.NameValueCCSID = atoi((LPCTSTR)m_rfh_ccsid);
	tempRFH.Encoding = getEncodingValue(m_rfh_encoding, m_rfh_pd_encoding, m_rfh_float_encoding);

	// initialize the format to blanks
	memset(tempFormat, ' ', MQ_FORMAT_LENGTH);
	if (m_rfh_format.GetLength() > 0)
	{
		memcpy(tempFormat, (LPCTSTR)m_rfh_format, m_rfh_format.GetLength());
	}

	// set the RFH format field
	memcpy(tempRFH.Format, tempFormat, MQ_FORMAT_LENGTH);

	// set the flags, which are normally zero
	tempRFH.Flags = atoi((LPCTSTR)m_rfh_flags);

	// check if we have to translate the StrucId and Format to EBCDIC
	if (getCcsidType(ccsid) == CHAR_EBCDIC)
	{
		// translate the structure id to EBCDIC
		AsciiToEbcdic((unsigned char *) tempRFH.StrucId, sizeof(tempRFH.StrucId), (unsigned char *) tempEbcdicfield);
		memcpy(tempRFH.StrucId, tempEbcdicfield, sizeof(tempRFH.StrucId));

		// translate the data format to EBCDIC
		AsciiToEbcdic((unsigned char *) tempRFH.Format, sizeof(tempRFH.Format), (unsigned char *) tempEbcdicfield);
		memcpy(tempRFH.Format, tempEbcdicfield, sizeof(tempRFH.Format));
	}

	// check if we need to convert the integer fields
	convertRFH2Header(&tempRFH, encoding);

	// finally, copy the fixed and variable parts of the data into the buffer
	memcpy(tempbuf, tempRFH.StrucId, MQRFH_STRUC_LENGTH_FIXED_2);

	ptr = tempbuf + MQRFH_STRUC_LENGTH_FIXED_2;
	if ((m_incl_mcd) && (m_rfh_mcd_len > 0))
	{
		worklen = m_rfh_mcd_len;
		if (NUMERIC_HOST == encoding)
		{
			worklen = reverseBytes4(worklen);
		}

		memcpy(ptr, (const void *) &worklen, 4);
		memcpy(ptr + 4, rfh_mcd_area, m_rfh_mcd_len);
		ptr += m_rfh_mcd_len + 4;
	}

	if ((m_incl_pscr) && (m_RFH_pscr_len > 0))
	{
		worklen = m_RFH_pscr_len;
		if (NUMERIC_HOST == encoding)
		{
			worklen = reverseBytes4(worklen);
		}

		memcpy(ptr, (const void *) &worklen, 4);
		memcpy(ptr + 4, rfh_pscr_area, m_RFH_pscr_len);
		ptr += m_RFH_pscr_len + 4;
	}

	if ((m_incl_pubsub) && (m_RFH_pubsub_len > 0))
	{
		worklen = m_RFH_pubsub_len;
		if (NUMERIC_HOST == encoding)
		{
			worklen = reverseBytes4(worklen);
		}

		memcpy(ptr, (const void *) &worklen, 4);
		memcpy(ptr + 4, rfh_pubsub_area, m_RFH_pubsub_len);
		ptr += m_RFH_pubsub_len + 4;
	}

	if ((m_incl_jms) && (m_RFH_jms_len > 0))
	{
		worklen = m_RFH_jms_len;
		if (NUMERIC_HOST == encoding)
		{
			worklen = reverseBytes4(worklen);
		}

		memcpy(ptr, (const void *) &worklen, 4);
		memcpy(ptr + 4, rfh_jms_area, m_RFH_jms_len);
		ptr += m_RFH_jms_len + 4;
	}

	if ((m_incl_usr) && (m_RFH_usr_len > 0))
	{
		worklen = m_RFH_usr_len;
		if (NUMERIC_HOST == encoding)
		{
			worklen = reverseBytes4(worklen);
		}

		memcpy(ptr, (const void *) &worklen, 4);
		memcpy(ptr + 4, rfh_usr_area, m_RFH_usr_len);
		ptr += m_RFH_usr_len + 4;
	}

	if ((m_incl_other) && (m_RFH_other_len > 0))
	{
		memcpy(ptr, rfh_other_area, m_RFH_other_len);
		ptr += m_RFH_other_len;
	}

	// save the location and length of the entire RFH
	setRfhArea(tempbuf, rfhlength, ccsid, encoding);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::buildRFH2() rfhlength=%d", rfhlength);

		// trace exit from buildRFH2
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("V2 rfhdata", tempbuf, rfhlength);
	}

	// return the length of the RFH that was added
	return rfhlength;
}

//////////////////////////////////////////////////
//
// Routine to build an mcd folder for the RFH header
//
//////////////////////////////////////////////////

int RFH::buildMcdArea(int ccsid, int encoding)

{
	int				i;
	int				j;
	wchar_t			*ucsPtr;
	unsigned char	*tempPtr;
	char			tempfield[MAX_RFH_LENGTH + 4];
	char			tempValue[MAX_RFH_LENGTH + 4];
	char			traceInfo[512];					// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering Entering RFH::buildMcdArea() ccsid=%d encoding=%d", ccsid, encoding);

		// trace entry to buildMcdArea
		pDoc->logTraceEntry(traceInfo);
	}

	// Has the mcd data been changed?
	if (mcdDataChanged || (ccsid != rfh_mcd_area_ccsid) || (rfh_mcd_area_encoding != encoding))
	{
		// data has changed - free the current mcd area
		freeMcdArea();
	}

	// does the mcd area already exist?
	if (NULL == rfh_mcd_area)
	{
		memset(tempfield, 0, sizeof(tempfield));

		// Add the first part of the XML headers
		strcpy(tempfield, MQRFH2_MSG_CONTENT_FOLDER_B);

		if (m_msg_domain.GetLength() > 0)
		{
			strcat(tempfield, MQMCD_MSG_DOMAIN_B);

			// get the message domain name and append it to the XML message
			replaceChars(m_msg_domain, tempValue);
			strcat(tempfield, tempValue);

			// Add the next part of the XML headers
			strcat(tempfield, MQMCD_MSG_DOMAIN_E);
		}

		if (m_msg_set.GetLength() > 0)
		{
			strcat(tempfield, MQMCD_MSG_SET_B);

			// get the message set name and append it to the XML message
			replaceChars(m_msg_set, tempValue);
			strcat(tempfield, tempValue);

			// Add the next part of the XML headers
			strcat(tempfield, MQMCD_MSG_SET_E);
		}

		if (m_msg_type.GetLength() > 0)
		{
			strcat(tempfield, MQMCD_MSG_TYPE_B);

			// get the message type name and append it to the XML message
			replaceChars(m_msg_type, tempValue);
			strcat(tempfield, tempValue);

			// Add the next part of the XML headers
			strcat(tempfield, MQMCD_MSG_TYPE_E);
		}

		if (m_msg_fmt.GetLength() > 0)
		{
			strcat(tempfield, MQMCD_MSG_FORMAT_B);

			// get the message fmt name and append it to the XML message
			replaceChars(m_msg_fmt, tempValue);
			strcat(tempfield, tempValue);

			// Add the last part of the XML headers
			strcat(tempfield, MQMCD_MSG_FORMAT_E);
		}

		strcat(tempfield, MQRFH2_MSG_CONTENT_FOLDER_E);

		// check if the data must be converted to UCS-2
		if (isUCS2(ccsid))
		{
			// convert the data to UCS-2
			// allocate a temporary area to hold the UCS-2 data
			tempPtr = (unsigned char *)rfhMalloc(2 * strlen(tempfield) + 16, "RFHTPTR2");

			if (tempPtr != 0)
			{
				// translate the data to UCS-2
				MultiByteToUCS2(tempPtr, 2 * strlen(tempfield) + 2, (unsigned char *)tempfield, strlen(tempfield));

				// set the length of the new mcd in bytes (each 16-bit character is 2 bytes)
				m_rfh_mcd_len = roundLength2(tempPtr, NUMERIC_PC) * 2;

				// check if the data has to be reversed
				if (encoding != NUMERIC_PC)
				{
					// reverse the order of the bytes
					i = 0;
					j = m_rfh_mcd_len >> 1;
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
				setMcdArea(tempPtr, m_rfh_mcd_len, ccsid, encoding);

				// release the storage we acquired
				rfhFree(tempPtr);
			}
			else
			{
				if (pDoc->traceEnabled)
				{
					// malloc failed - write entry to trace
					pDoc->logTraceEntry("RFH::buildMcdArea() malloc failed");
				}
			}
		}
		else
		{
			// get the length of the mcd segment
			m_rfh_mcd_len = roundLength((unsigned char *)tempfield);

			// save the results
			setMcdArea((unsigned char *)tempfield, m_rfh_mcd_len, ccsid, encoding);
		}
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::buildMcdArea() m_rfh_mcd_len=%d ccsid=%d", m_rfh_mcd_len, ccsid);

		// trace exit from buildMcdArea
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("mcd data", (unsigned char *)rfh_mcd_area, m_rfh_mcd_len);
	}

	// return the length of the RFH that was added
	return m_rfh_mcd_len;
}

//////////////////////////////////////////////////
//
// Routine to all the fields in the RFH2
//
//////////////////////////////////////////////////

void RFH::clearRFH()

{
	Usr		*usrObj=(Usr *)pDoc->usrData;
	jms		*jmsObj=(jms *)pDoc->jmsData;
	PubSub	*pubObj=(PubSub *)pDoc->pubData;
	pscr	*pscrObj=(pscr *)pDoc->pscrData;
	other	*otherObj=(other *)pDoc->otherData;

	if ((pDoc != NULL) && pDoc->traceEnabled)
	{
		// trace entry to clearRFH
		pDoc->logTraceEntry("Entering RFH::clearRFH()");
	}

	m_rfh1_include = FALSE;
	m_rfh2_include = FALSE;
	m_incl_jms = FALSE;
	m_incl_mcd = FALSE;
	m_incl_usr = FALSE;
	m_incl_pubsub = FALSE;
	m_incl_pscr = FALSE;
	m_incl_other = FALSE;
	m_incl_v1_mcd = FALSE;
	m_incl_v1_pubsub = FALSE;
	m_incl_v1_resp = FALSE;
	m_msg_domain.Empty();
	m_rfh_ccsid = "1208";
	m_rfh_charset.Empty();
	m_rfh_flags = _T("0");
	m_rfh_length.Empty();
	m_rfh_encoding = NUMERIC_PC;
	m_rfh_pd_encoding = NUMERIC_PC;
	m_rfh_format.Empty();
	m_rfh1_format_name.Empty();
	m_rfh1_app_group.Empty();
	m_rfh1_charset.Empty();
	m_rfh1_flags = _T("0");
	m_rfh1_format.Empty();
	m_msg_fmt.Empty();
	m_msg_set.Empty();
	m_msg_type.Empty();
	m_rfh1_encoding = NUMERIC_PC;
	m_rfh1_pd_encoding = NUMERIC_PC;
	m_rfh1_float_encoding = FLOAT_PC;
	m_rfh1_length.Empty();
	mcdDataChanged = false;

	// release any storage areas
	freeRfhArea();
	freeMcdArea();
	freeRfh1Area();

	// clear the RFH jms data
	jmsObj->clearJMSdata();

	// clear the Pubsub data
	pubObj->clearPubSubData();

	// clear the PSCR data
	pscrObj->clearPSCRdata();

	// clear the RFH usr data
	usrObj->clearUsrData();

	// clear the RFH other folders
	otherObj->clearOtherData();
}

//////////////////////////////////////////////////
//
// Routine to build an RFH header
//
//////////////////////////////////////////////////

int RFH::buildRFH(int ccsid, int encoding)

{
	int				rfhlength=0;
	int				extra;
	unsigned int	varlength=0;
	MQRFH			tempRFH = {MQRFH_DEFAULT};
	char			tempEbcdicfield[MAX_RFH_LENGTH + 4];
	char			tempfield[MAX_RFH_LENGTH + 4];
	char			tempFormat[MQ_FORMAT_LENGTH + 4];
	unsigned char	tempbuf[MAX_RFH_LENGTH + 4];
	char			traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::buildRFH() ccsid=%d encoding=%d m_rfh1_data_changed=%d", ccsid, encoding, m_rfh1_data_changed);

		// trace entry to buildRFH
		pDoc->logTraceEntry(traceInfo);
	}

	// check if the existing data has been changed
	if (m_rfh1_data_changed || ((PubSub *)pubsubData)->wasDataChanged() || ((pscr *)pscrData)->wasDataChanged())
	{
		// something was changed so release any previous RFH1
		freeRfh1Area();
	}

	// check if the appropriate header is already built
	if ((rfh1_data != NULL) && (rfh1_data_ccsid == ccsid) && (rfh1_data_encoding == encoding))
	{
		if (pDoc->traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Exiting RFH::buildRFH() reusing existing area m_rfh1_data_len=%d", m_rfh1_data_len);

			// trace exit from buildRFH
			pDoc->logTraceEntry(traceInfo);

			// write the data to the trace file
			pDoc->dumpTraceData("V1 rfhdata", rfh1_data, m_rfh1_data_len);
		}

		return m_rfh1_data_len;
	}

	// first, build the variable part of the RFH
	memset(tempfield, 0, sizeof(tempfield));
	tempbuf[0] = 0;

	if (m_incl_v1_mcd)
	{
		if (m_rfh1_app_group.GetLength() > 0)
		{
			strcpy(tempfield, MQNVS_APPL_TYPE);

			// get the application group name
			strcat(tempfield,(LPCTSTR) m_rfh1_app_group);

			// truncate any trailing blanks
			Rtrim(tempfield);
		}

		if (m_rfh1_format_name.GetLength() > 0)
		{
			// check if we are at the beginning of the variable part
			if (tempfield[0] != 0)
			{
				// add a blank delimiter
				strcat(tempfield, " ");
			}

			// next, copy in the message type name
			strcat(tempfield, MQNVS_MSG_TYPE);

			// get the message type into a string
			strcat(tempfield,(LPCTSTR) m_rfh1_format_name);

			// truncate any trailing blanks
			Rtrim(tempfield);
		}
	}

	if (m_incl_v1_pubsub)
	{
		if (strlen(tempfield) > 0)
		{
			strcat(tempfield, " ");
		}

		((PubSub *)pubsubData)->buildV1PubsubArea(tempfield + strlen(tempfield));

		// truncate any trailing blanks, since we may not have added anything aside from the first blank
		Rtrim(tempfield);
	}

	if (m_incl_v1_resp)
	{
		if (strlen(tempfield) > 0)
		{
			strcat(tempfield, " ");
		}

		((pscr *)pscrData)->buildV1PubResp(tempfield + strlen(tempfield));

		// truncate any trailing blanks, since we may not have added anything aside from the first blank
		Rtrim(tempfield);
	}

	// Round it up to a multiple of 4 if necessary
	extra = strlen(tempfield) % 4;
	if (extra > 0)
	{
		extra = 4 - extra;

		while (extra > 0)
		{
			strcat(tempfield, " ");
			extra--;
		}
	}

	// calculate the length of the variable part of the rfh
	varlength = strlen(tempfield);

	// calulate the length of the RFH
	rfhlength = varlength + MQRFH_STRUC_LENGTH_FIXED;

	// next, build the fixed part of the rfh
	tempRFH.StrucLength = rfhlength;
	tempRFH.Encoding = getEncodingValue(m_rfh1_encoding, m_rfh1_pd_encoding, m_rfh1_float_encoding);
	if (m_rfh1_charset.GetLength() > 0)
	{
		tempRFH.CodedCharSetId = getRFH1Ccsid();
	}
	else
	{
		tempRFH.CodedCharSetId = DEF_ASCII_CCSID;
	}

	// initialize the format to blanks
	memset(tempFormat, ' ', MQ_FORMAT_LENGTH);
	if (m_rfh1_format.GetLength() > 0)
	{
		memcpy(tempFormat, (LPCTSTR)m_rfh1_format, m_rfh1_format.GetLength());
	}

	// set the RFH format field
	memcpy(tempRFH.Format, tempFormat, MQ_FORMAT_LENGTH);

	// set the flags, which are normally zero
	tempRFH.Flags = atoi((LPCTSTR)m_rfh_flags);

	// check if we have to translate the StrucId and Format to EBCDIC
	if (getCcsidType(ccsid) == CHAR_EBCDIC)
	{
		// translate the structure id to EBCDIC
		AsciiToEbcdic((unsigned char *) tempRFH.StrucId, sizeof(tempRFH.StrucId), (unsigned char *) tempEbcdicfield);
		memcpy(tempRFH.StrucId, tempEbcdicfield, sizeof(tempRFH.StrucId));

		// translate the data format to EBCDIC
		AsciiToEbcdic((unsigned char *) tempRFH.Format, sizeof(tempRFH.Format), (unsigned char *) tempEbcdicfield);
		memcpy(tempRFH.Format, tempEbcdicfield, sizeof(tempRFH.Format));
	}

	// check if we need to convert the integer fields
	convertRFHeader(&tempRFH, encoding);

	// finally, copy the fixed and variable parts of the data into the buffer
	memcpy(tempbuf, tempRFH.StrucId, MQRFH_STRUC_LENGTH_FIXED);

	// check if we need to translate the variable part of the RFH
	if (getCcsidType(ccsid) == CHAR_EBCDIC)
	{
		AsciiToEbcdic((unsigned char *) tempfield, varlength, tempbuf + MQRFH_STRUC_LENGTH_FIXED);
	}
	else
	{
		memcpy(tempbuf + MQRFH_STRUC_LENGTH_FIXED, tempfield, varlength);
	}

	// save the result
	setRfh1Area(tempbuf, rfhlength, ccsid, encoding);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::buildRFH() rfhlength=%d", rfhlength);

		// trace exit from buildRFH
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("V1 rfhdata", tempbuf, rfhlength);
	}

	// return the length of the RFH that was added
	return rfhlength;
}

////////////////////////////////////////////
//
// This routine is called from the DataArea
// object.  The routine is used to change
// the domain when a selection is made on 
// the JMS tab.
//
////////////////////////////////////////////

void RFH::setJMSdomain(const char * domain)

{
	char	traceInfo[512];		// work variable to build trace message

	// make sure we have current info
	UpdateData(TRUE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::setJMSdomain() domain=%s m_msg_domain=%s", domain, (LPCTSTR)m_msg_domain);

		// trace entry to setJMSdomain
		pDoc->logTraceEntry(traceInfo);
	}

	// Is JMS_NONE being selected?
	if ((domain != NULL) && (strcmp(domain, MQMCD_DOMAIN_JMS_NONE) == 0))
	{
		// check if the domain is one of the JMS types
		if ((strcmp(m_msg_domain, MQMCD_DOMAIN_JMS_TEXT) == 0) ||
			(strcmp(m_msg_domain, MQMCD_DOMAIN_JMS_BYTES) == 0) ||
			(strcmp(m_msg_domain, MQMCD_DOMAIN_JMS_MAP) == 0) ||
			(strcmp(m_msg_domain, MQMCD_DOMAIN_JMS_OBJECT) == 0) ||
			(strcmp(m_msg_domain, MQMCD_DOMAIN_JMS_STREAM) == 0))
		{
			// remove the current JMS header from the mcd
			m_msg_domain = "";
			m_incl_mcd = FALSE;
		}

		// remove the include rfh2 selection if there are no other folders selected
		if ((FALSE == m_incl_mcd) &&
			(FALSE == m_incl_jms) &&
			(FALSE == m_incl_pubsub) &&
			(FALSE == m_incl_pscr) &&
			(FALSE == m_incl_usr) &&
			(FALSE == m_incl_other))
		{
			// remove the include RFH2 header selection since there are no folders specified
			m_rfh2_include=FALSE;
		}
	}
	else
	{
		// set the message domain to the type of JMS message
		m_msg_domain = domain;

		// the RFH2 header must now include an mcd folder
		m_incl_mcd = TRUE;

		// make sure the include RFH2 header is also selected
		m_rfh2_include=TRUE;
	}

	// release the current data areas
	freeRfhArea();
	freeMcdArea();

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void RFH::updateRFH1Fields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::updateRFH1Fields() m_rfh1_format=%s m_rfh1_charset=%s m_rfh1_encoding=%d m_rfh1_pd_encoding=%d m_rfh1_float_encoding=%d", (LPCTSTR)m_rfh1_format, (LPCTSTR)m_rfh1_charset, m_rfh1_encoding, m_rfh1_pd_encoding, m_rfh1_float_encoding);

		// trace entry to updateRFH1Fields
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure we have current info
	UpdateData(TRUE);

	// update the format
	m_rfh1_format = newFormat;
	m_rfh1_charset.Format("%d", newCcsid);
	m_rfh1_encoding = newEncoding;
	m_rfh1_pd_encoding = newPdEncoding;
	m_rfh1_float_encoding = newFloatEncoding;

	// release the current saved area if any
	freeRfhArea();

	// update the dialog controls
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::updateRFH1Fields() m_rfh1_format=%s m_rfh1_charset=%s m_rfh1_encoding=%d m_rfh1_pd_encoding=%d m_rfh1_float_encoding=%d", (LPCTSTR)m_rfh1_format, (LPCTSTR)m_rfh1_charset, m_rfh1_encoding, m_rfh1_pd_encoding, m_rfh1_float_encoding);

		// trace exit from updateFields
		pDoc->logTraceEntry(traceInfo);
	}
}

void RFH::updateRFH2Fields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::updateRFH2Fields() m_rfh_format=%s m_rfh_charset=%s m_rfh_encoding=%d m_rfh_pd_encoding=%d m_rfh_float_encoding=%d", (LPCTSTR)m_rfh_format, (LPCTSTR)m_rfh_charset, m_rfh_encoding, m_rfh_pd_encoding, m_rfh_float_encoding);

		// trace entry to updateRFH2Fields
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure we have current info
	UpdateData(TRUE);

	// update the fields
	m_rfh_format = newFormat;
	m_rfh_charset.Format("%d", newCcsid);
	m_rfh_encoding = newEncoding;
	m_rfh_pd_encoding = newPdEncoding;
	m_rfh_float_encoding = newFloatEncoding;

	// release the current saved area if any
	freeRfhArea();

	// update the dialog controls
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::updateRFH2Fields() m_rfh_format=%s m_rfh_charset=%s m_rfh_encoding=%d m_rfh_pd_encoding=%d m_rfh_float_encoding=%d", (LPCTSTR)m_rfh_format, (LPCTSTR)m_rfh_charset, m_rfh_encoding, m_rfh_pd_encoding, m_rfh_float_encoding);

		// trace exit from updateFields
		pDoc->logTraceEntry(traceInfo);
	}
}

void RFH::getRFH1Format(char *mqformat)

{
	int		maxLen;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH1Format() m_rfh1_format=%s", (LPCTSTR)m_rfh1_format);

		// trace entry to getRFH1Format
		pDoc->logTraceEntry(traceInfo);
	}

	memset(mqformat, ' ', MQ_FORMAT_LENGTH);

	// terminate the string
	mqformat[MQ_FORMAT_LENGTH] = 0;

	// get the length
	maxLen = m_rfh1_format.GetLength();

	if (maxLen > MQ_FORMAT_LENGTH)
	{
		maxLen = MQ_FORMAT_LENGTH;
	}

	if (maxLen > 0)
	{
		memcpy(mqformat, (LPCTSTR)m_rfh1_format, maxLen);
	}
}

void RFH::getRFH2Format(char *mqformat)

{
	int		maxLen;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH2Format() m_rfh_format=%s", (LPCTSTR)m_rfh_format);

		// trace entry to getRFH2Format
		pDoc->logTraceEntry(traceInfo);
	}

	memset(mqformat, ' ', MQ_FORMAT_LENGTH);

	// terminate the string
	mqformat[MQ_FORMAT_LENGTH] = 0;

	// get the length
	maxLen = m_rfh_format.GetLength();

	if (maxLen > MQ_FORMAT_LENGTH)
	{
		maxLen = MQ_FORMAT_LENGTH;
	}

	if (maxLen > 0)
	{
		memcpy(mqformat, (LPCTSTR)m_rfh_format, maxLen);
	}
}

int RFH::getRFH1Ccsid()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH1Ccsid() m_rfh1_charset=%s", (LPCTSTR)m_rfh1_charset);

		// trace entry to getRFH1Ccsid
		pDoc->logTraceEntry(traceInfo);
	}

	return atoi((LPCTSTR)m_rfh1_charset);
}

int RFH::getRFH2Ccsid()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH2Ccsid() m_rfh_charset=%s", (LPCTSTR)m_rfh_charset);

		// trace entry to getRFH2Ccsid
		pDoc->logTraceEntry(traceInfo);
	}

	return atoi((LPCTSTR)m_rfh_charset);
}

int RFH::getRFH1Encoding()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH1Encoding() m_rfh1_encoding=%d", m_rfh1_encoding);

		// trace entry to getRFH1Encoding
		pDoc->logTraceEntry(traceInfo);
	}

	return m_rfh1_encoding;
}

int RFH::getRFH2Encoding()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH2Encoding() m_rfh_encoding=%d", m_rfh_encoding);

		// trace entry to getRFH2Encoding
		pDoc->logTraceEntry(traceInfo);
	}

	return m_rfh_encoding;
}

int RFH::getRFH1PdEncoding()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH1PdEncoding() m_rfh1_pd_encoding=%d", m_rfh1_pd_encoding);

		// trace entry to getRFH1PdEncoding
		pDoc->logTraceEntry(traceInfo);
	}

	return m_rfh1_pd_encoding;
}

int RFH::getRFH2PdEncoding()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH2PdEncoding() m_rfh_pd_encoding=%d", m_rfh_pd_encoding);

		// trace entry to getRFH2PdEncoding
		pDoc->logTraceEntry(traceInfo);
	}

	return m_rfh_pd_encoding;
}

int RFH::getRFH1FloatEncoding()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH1FloatEncoding() m_rfh1_float_encoding=%d", m_rfh1_float_encoding);

		// trace entry to getRFH1PdEncoding
		pDoc->logTraceEntry(traceInfo);
	}

	return m_rfh1_float_encoding;
}

int RFH::getRFH2FloatEncoding()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::getRFH2FloatEncoding() m_rfh_float_encoding=%d", m_rfh_float_encoding);

		// trace entry to getRFH2PdEncoding
		pDoc->logTraceEntry(traceInfo);
	}

	return m_rfh_float_encoding;
}

void RFH::OnRfh2IncludeHeader() 

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::OnRfh2IncludeHeader() m_rfh2_include=%d", m_rfh2_include);

		// trace entry to OnRfh2IncludeHeader
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure we have current info
	UpdateData(TRUE);

	if (m_rfh2_include)
	{
		// insert the rfh2 header
		pDoc->insertMQheader(MQHEADER_RFH2, getRFH2Ccsid(), m_rfh_encoding, m_rfh_pd_encoding, m_rfh_float_encoding);
	}
	else
	{
		// unchain the rfh2 header
		pDoc->removeMQheader(MQHEADER_RFH2, (LPCTSTR)m_rfh_format, getRFH2Ccsid(), m_rfh_encoding, m_rfh_pd_encoding, m_rfh_float_encoding);
	}

	if (m_rfh_ccsid.GetLength() == 0)
	{
		m_rfh_ccsid = "1208";
		mcdDataChanged = true;
	}

	UpdateData(FALSE);

	// enable or disable the check boxes for the individual folders
	setV2();
}

void RFH::OnRfh1IncludeHeader() 

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::OnRfh1IncludeHeader() m_rfh1_include=%d", m_rfh1_include);

		// trace entry to OnRfh1IncludeHeader
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure we have current info
	UpdateData(TRUE);

	if (m_rfh1_include)
	{
		// insert the rfh1 header
		pDoc->insertMQheader(MQHEADER_RFH1, getRFH1Ccsid(), m_rfh1_encoding, m_rfh1_pd_encoding, m_rfh1_float_encoding);
	}
	else
	{
		// unchain the rfh1 header
		pDoc->removeMQheader(MQHEADER_RFH1, (LPCTSTR)m_rfh1_format, getRFH1Ccsid(), m_rfh1_encoding, m_rfh1_pd_encoding, m_rfh1_float_encoding);
	}

	// enable or disable the check boxes for the individual folders
	setV1();
}

int RFH::getRFH1length()

{
	return m_rfh1_data_len;
}

int RFH::getRFH2length()

{
	return m_rfh_data_len;
}

int RFH::buildRFH1header(unsigned char *header, int ccsid, int encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::buildRFH1header() m_rfh1_include=%d ccsid=%d rfh1_data_ccsid=%d encoding=%d rfh1_data_encoding=%d", m_rfh1_include, ccsid, rfh1_data_ccsid, encoding, rfh1_data_encoding);

		// trace entry to buildRFH1header
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure we have current info
	UpdateData(TRUE);

	// is there a header?
	if (!m_rfh1_include)
	{
		return 0;
	}

	if ((NULL == rfh1_data) || (ccsid != rfh1_data_ccsid) || (encoding != rfh1_data_encoding))
	{
		buildRFH(ccsid, encoding);
	}

	memcpy(header, rfh1_data, m_rfh1_data_len);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::buildRFH1header() m_rfh1_data_len=%d", m_rfh1_data_len);

		// trace exit from buildRFH1header
		pDoc->logTraceEntry(traceInfo);
	}

	return m_rfh1_data_len;
}

int RFH::buildRFH2header(unsigned char *header, int ccsid, int encoding)

{
	// is there a header?
	char	traceInfo[512];		// work variable to build trace message
	// pointers to the other RFH objects
	Usr		*usrObj=(Usr *)pDoc->usrData;
	jms		*jmsObj=(jms *)pDoc->jmsData;
	PubSub	*pubObj=(PubSub *)pDoc->pubData;
	pscr	*pscrObj=(pscr *)pDoc->pscrData;
	other	*otherObj=(other *)pDoc->otherData;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::buildRFH2header() m_rfh2_include=%d ccsid=%d rfh_data_ccsid=%d encoding=%d rfh_data_encoding=%d", m_rfh2_include, ccsid, rfh_data_ccsid, encoding, rfh_data_encoding);

		// trace entry to buildRFH2header
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure we have current info
	UpdateData(TRUE);

	// check if an RFH2 has been requested
	if (!m_rfh2_include)
	{
		// do not need an RFH2 - return zero length
		return 0;
	}

	// check if the existing data has been changed
	if (m_rfh_data_changed || mcdDataChanged || usrObj->wasDataChanged() || jmsObj->wasDataChanged() || pubObj->wasDataChanged() || pscrObj->wasDataChanged() || otherObj->wasDataChanged())
	{
		// something was changed so release any previous RFH1
		freeRfhArea();
	}

	// check if we already have one built and if it matches the current settings
	if ((NULL == rfh_data) || (ccsid != rfh_data_ccsid) || (encoding != rfh_data_encoding))
	{
		// need to build one
		buildRFH2(ccsid, encoding);
	}

	// copy the RFH header to the message
	memcpy(header, rfh_data, m_rfh_data_len);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::buildRFH2header() m_rfh_data_len=%d", m_rfh_data_len);

		// trace exit from buildRFH2header
		pDoc->logTraceEntry(traceInfo);
	}

	return m_rfh_data_len;
}

void RFH::setPubSubVersion()

{
	// is there a header?
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::setPubSubVersion() m_rfh2_include=%d m_incl_pubsub=%d m_rfh1_include=%d m_incl_v1_pubsub=%d", m_rfh2_include, m_incl_pubsub, m_rfh1_include, m_incl_v1_pubsub);

		// trace entry to setPubSubVersion
		pDoc->logTraceEntry(traceInfo);
	}

	if (m_rfh2_include)
	{
		m_incl_pubsub = TRUE;
	}
	else if (m_rfh1_include)
	{
		m_incl_v1_pubsub = TRUE;
	}
	else
	{
		// no RFH header selected - use an RFH V2
		m_rfh2_include = TRUE;
		m_incl_pubsub = TRUE;
	}

	// update the dialog controls from the instance variables
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::setPubSubVersion() m_rfh2_include=%d m_incl_pubsub=%d m_rfh1_include=%d m_incl_v1_pubsub=%d", m_rfh2_include, m_incl_pubsub, m_rfh1_include, m_incl_v1_pubsub);

		// trace exit from setPubSubVersion
		pDoc->logTraceEntry(traceInfo);
	}
}

//////////////////////////////////////////////////////////////
//
// This routine is called from the pubsub screen when a 
// register publisher or unregister publisher operation is
// selected.  It will ensure that an RFH1 header is used
// for the pubsub request.
//
//////////////////////////////////////////////////////////////

void RFH::setRFHV1()

{
	// is there a header?
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::setRFHV1() m_rfh1_include=%d m_incl_v1_pubsub=%d", m_rfh1_include, m_incl_v1_pubsub);

		// trace entry to setRFHV1
		pDoc->logTraceEntry(traceInfo);
	}

	// check if there is already an RFH1 header with pubsub selected.
	if (m_rfh1_include && m_incl_v1_pubsub)
	{
		// the right selections have been made - no need to change anything
		return;
	}

	// is an RFH1 already selected?
	if (!m_rfh1_include)
	{
		// not selected - need to insert the header
		m_rfh1_include = TRUE;
		m_incl_v1_pubsub = TRUE;
		pDoc->insertMQheader(MQHEADER_RFH1, DEF_ASCII_CCSID, NUMERIC_PC, NUMERIC_PC, FLOAT_PC);
	}

	// check if there is an RFH2 header selected
	if (m_rfh2_include)
	{
		// remove the pubsub option
		m_incl_pubsub = false;

		// free the current rfh header
		freeRfhArea();

		// check if any of the folders are selected
		if (!m_incl_jms && !m_incl_mcd && !m_incl_other && !m_incl_pscr && !m_incl_usr)
		{
			// no more folders selected - remove the RFH2 header
			m_rfh2_include = FALSE;

			// unchain the rfh2 header
			pDoc->removeMQheader(MQHEADER_RFH2, (LPCTSTR)m_rfh_format, getRFH2Ccsid(), m_rfh_encoding, m_rfh_pd_encoding, m_rfh_float_encoding);
		}
	}

	// update the dialog controls from the instance variables
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::setRFHV1() m_rfh1_include=%d m_incl_v1_pubsub=%d", m_rfh1_include, m_incl_v1_pubsub);

		// trace exit from setRFHV1
		pDoc->logTraceEntry(traceInfo);
	}
}

//////////////////////////////////////////////////////////////
//
// This routine is called from the pubsub screen when an 
// operation that does not require an RFH V1 is
// selected.  It will ensure that at least one RFH header
// is selected with the pubsub option.
//
//////////////////////////////////////////////////////////////

void RFH::setRFHV2()

{
	// is there a header?
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::setRFHV2() m_rfh2_include=%d m_incl_pubsub=%d", m_rfh2_include, m_incl_pubsub);

		// trace entry to setRFHV2
		pDoc->logTraceEntry(traceInfo);
	}

	// check if either an RFH1 or RFH2 header is already selected 
	if ((m_rfh1_include && m_incl_v1_pubsub) || (m_rfh2_include && m_incl_pubsub))
	{
		// already selected, so nothing to do - just return
		return;
	}

	// check if an RFH2 is already selected
	if (m_rfh2_include)
	{
		// yes - just force on the pubsub folder
		m_incl_pubsub = TRUE;

		// force on the check box as well
		((CButton *)GetDlgItem(IDC_INCL_PUBSUB))->SetCheck(TRUE);

		// get rid of any saved header
		freeRfhArea();
	} 
	else if (m_rfh1_include)
	{
		// use the rfh1 that is already selected
		m_incl_v1_pubsub = TRUE;

		// force on the check box as well
		((CButton *)GetDlgItem(IDC_INCL_V1_PUBSUB))->SetCheck(TRUE);

		// get rid of any saved header
		freeRfh1Area();
	}
	else
	{
		// no RFH currently selected - insert an RFH2 header
		m_rfh2_include = TRUE;
		m_incl_pubsub = TRUE;

		// force on the check boxes as well
		((CButton *)GetDlgItem(IDC_RFH2_INCLUDE_HEADER))->SetCheck(TRUE);
		((CButton *)GetDlgItem(IDC_INCL_PUBSUB))->SetCheck(TRUE);

		// insert the rfh2 header
		m_rfh_charset.Format("%d", DEF_ASCII_CCSID);
		m_rfh_encoding = NUMERIC_PC;
		m_rfh_ccsid = "1208";
		pDoc->insertMQheader(MQHEADER_RFH2, DEF_ASCII_CCSID, NUMERIC_PC, NUMERIC_PC, FLOAT_PC);
	}

	// update the dialog controls from the instance variables
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::setRFHV2() m_rfh2_include=%d m_incl_pubsub=%d m_incl_v1_pubsub=%d", m_rfh2_include, m_incl_pubsub, m_incl_v1_pubsub);

		// trace exit from setRFHV2
		pDoc->logTraceEntry(traceInfo);
	}
}

void RFH::setNextFormatRfh1(char *nextFormat, int *charFormat, int *encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::setNextFormatRfh1() nextFormat=%s charFormat=%d encoding=%d", nextFormat, (*charFormat), (*encoding));

		// trace entry to setNextFormatRfh1
		pDoc->logTraceEntry(traceInfo);
	}

	strcpy(nextFormat, (LPCTSTR)m_rfh1_format);
	(*charFormat) = getCcsidType(atoi((LPCTSTR)m_rfh1_charset));
	(*encoding) = m_rfh1_encoding;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::setNextFormatRfh1() nextFormat=%s charFormat=%d encoding=%d", nextFormat, (*charFormat), (*encoding));

		// trace exit from setNextFormatRfh1
		pDoc->logTraceEntry(traceInfo);
	}
}

void RFH::setNextFormatRfh2(char *nextFormat, int *charFormat, int *encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::setNextFormatRfh2() nextFormat=%s charFormat=%d encoding=%d", nextFormat, (*charFormat), (*encoding));

		// trace entry to setNextFormatRfh2
		pDoc->logTraceEntry(traceInfo);
	}

	strcpy(nextFormat, (LPCTSTR)m_rfh_format);
	(*charFormat) = getCcsidType(atoi((LPCTSTR)m_rfh_charset));
	(*encoding) = m_rfh_encoding;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting RFH::setNextFormatRfh2() nextFormat=%s charFormat=%d encoding=%d", nextFormat, (*charFormat), (*encoding));

		// trace exit from setNextFormatRfh2
		pDoc->logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////
//
// This routine is used when the data
// in the usr folder is transferred
// to MQ user properties or to the
// usr folder from MQ user properties
//
/////////////////////////////////////

void RFH::selectUsrFolder(BOOL selectUsr)

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering RFH::selectUsrFolder() selectUsr=%d", selectUsr);

		// trace entry to selectUsrFolder
		pDoc->logTraceEntry(traceInfo);
	}

	// discard any saved RFH2 data
	freeRfhArea();

	if (selectUsr)
	{
		// select the usr folder and force the RFH2 to be selected
		m_incl_usr = TRUE;
		m_rfh2_include=TRUE;
	}
	else
	{
		// deselect the usr folder
		m_incl_usr = FALSE;
	}

	// update the dialog controls from the instance variables
	UpdateData(FALSE);
}
