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

// CICS.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "mqsubs.h"
#include "CICS.h"
#include "MQMDPAGE.h"

#include "cmqc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+7

/////////////////////////////////////////////////////////////////////////////
// CCICS property page

IMPLEMENT_DYNCREATE(CCICS, CPropertyPage)

CCICS::CCICS() : CPropertyPage(CCICS::IDD)
{
	//{{AFX_DATA_INIT(CCICS)
	m_CIH_version = CIH_NONE;
	m_CIH_facility = _T("");
	m_CIH_status = CIH_END_NOSYNC;
	m_CIH_startcode = CIH_START_NONE;
	m_CIH_uow = CIH_UOW_ONLY;
	m_CIH_conversational = FALSE;
	m_CIH_format = _T("");
	m_CIH_program = _T("");
	m_CIH_next_tranid = _T("");
	m_CIH_cancel_code = _T("");
	m_CIH_comp_code = _T("");
	m_CIH_return_code = _T("");
	m_CIH_reason = _T("");
	m_CIH_cursor_pos = _T("");
	m_CIH_error_ofs = _T("");
	m_CIH_facility_like = _T("");
	m_CIH_aid = _T("");
	m_CIH_transid = _T("");
	m_CIH_remote_sys = _T("");
	m_CIH_remote_tran = _T("");
	m_CIH_reply_format = _T("");
	m_CIH_codepage = _T("");
	m_CIH_flags = _T("");
	m_CIH_wait_interval = _T("");
	m_CIH_data_length = _T("");
	m_CIH_abend_code = _T("");
	m_CIH_authenticator = _T("");
	m_CIH_keep_time = _T("");
	m_CIH_input_item = _T("");
	m_CIH_int_encode = NUMERIC_PC;
	m_CIH_pd_encode = NUMERIC_PC;
	m_CIH_float_encode = FLOAT_PC;
	m_CIH_function = _T("");
	m_CIH_link_type = CIH_LINK_PROG;
	m_cih_ads_msg_format = FALSE;
	m_cih_ads_receive = FALSE;
	m_cih_ads_send = FALSE;
	m_CIH_float_encode = -1;
	//}}AFX_DATA_INIT

	pDoc = NULL;
	m_save_CIH_version = CIH_NONE;
	cicsHeader = NULL;
	cicsCcsid = -1;
	cicsEncodeType = -1;
	cicsLength = 0;
	functionBoxInitialized = FALSE;
}

CCICS::~CCICS()
{
	// free any storage that we acquired
	freeCurrentHeader(0);
}

void CCICS::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCICS)
	DDX_Radio(pDX, IDC_CIH_NONE, m_CIH_version);
	DDX_Text(pDX, IDC_CIH_FACILITY, m_CIH_facility);
	DDV_MaxChars(pDX, m_CIH_facility, 16);
	DDX_Radio(pDX, IDC_CIH_END_NOSYNC, m_CIH_status);
	DDX_Radio(pDX, IDC_CIH_SC_NONE, m_CIH_startcode);
	DDX_Radio(pDX, IDC_CIH_UOW_ONLY, m_CIH_uow);
	DDX_Check(pDX, IDC_CIH_CONVERSATIONAL, m_CIH_conversational);
	DDX_Text(pDX, IDC_CIH_FORMAT, m_CIH_format);
	DDV_MaxChars(pDX, m_CIH_format, 8);
	DDX_Text(pDX, IDC_CIH_PROGRAM, m_CIH_program);
	DDV_MaxChars(pDX, m_CIH_program, 8);
	DDX_Text(pDX, IDC_CIH_NEXT_TRANID, m_CIH_next_tranid);
	DDV_MaxChars(pDX, m_CIH_next_tranid, 4);
	DDX_Text(pDX, IDC_CIH_CANCEL_CODE, m_CIH_cancel_code);
	DDV_MaxChars(pDX, m_CIH_cancel_code, 4);
	DDX_Text(pDX, IDC_CIH_COMP_CODE, m_CIH_comp_code);
	DDX_Text(pDX, IDC_CIH_RETURN_CODE, m_CIH_return_code);
	DDX_Text(pDX, IDC_CIH_REASON, m_CIH_reason);
	DDX_Text(pDX, IDC_CIH_CURSOR_POS, m_CIH_cursor_pos);
	DDV_MaxChars(pDX, m_CIH_cursor_pos, 5);
	DDX_Text(pDX, IDC_CIH_ERROR_OFFSET, m_CIH_error_ofs);
	DDV_MaxChars(pDX, m_CIH_error_ofs, 5);
	DDX_Text(pDX, IDC_CIH_FACILITY_LIKE, m_CIH_facility_like);
	DDV_MaxChars(pDX, m_CIH_facility_like, 4);
	DDX_Text(pDX, IDC_CIH_AID, m_CIH_aid);
	DDV_MaxChars(pDX, m_CIH_aid, 4);
	DDX_Text(pDX, IDC_CIH_TRANS_ID, m_CIH_transid);
	DDV_MaxChars(pDX, m_CIH_transid, 4);
	DDX_Text(pDX, IDC_CIH_REMOTE_SYS, m_CIH_remote_sys);
	DDV_MaxChars(pDX, m_CIH_remote_sys, 4);
	DDX_Text(pDX, IDC_CIH_REMOTE_TRAN, m_CIH_remote_tran);
	DDV_MaxChars(pDX, m_CIH_remote_tran, 4);
	DDX_Text(pDX, IDC_CIH_REPLY_FORMAT, m_CIH_reply_format);
	DDV_MaxChars(pDX, m_CIH_reply_format, 8);
	DDX_Text(pDX, IDC_CIH_CODEPAGE, m_CIH_codepage);
	DDV_MaxChars(pDX, m_CIH_codepage, 5);
	DDX_Text(pDX, IDC_CIH_FLAGS, m_CIH_flags);
	DDX_Text(pDX, IDC_CIH_WAIT_INTERVAL, m_CIH_wait_interval);
	DDX_Text(pDX, IDC_CIH_DATA_LENGTH, m_CIH_data_length);
	DDX_Text(pDX, IDC_CIH_ABEND_CODE, m_CIH_abend_code);
	DDV_MaxChars(pDX, m_CIH_abend_code, 4);
	DDX_Text(pDX, IDC_CIH_AUTHENTICATOR, m_CIH_authenticator);
	DDV_MaxChars(pDX, m_CIH_authenticator, 8);
	DDX_Text(pDX, IDC_CIH_KEEP_TIME, m_CIH_keep_time);
	DDX_Text(pDX, IDC_CIH_INPUT_ITEM, m_CIH_input_item);
	DDX_Radio(pDX, IDC_CIH_INT_ENCODE_PC, m_CIH_int_encode);
	DDX_Radio(pDX, IDC_CIH_PD_ENCODE_PC, m_CIH_pd_encode);
	DDX_CBString(pDX, IDC_CIH_FUNCTION, m_CIH_function);
	DDX_Radio(pDX, IDC_CIH_LINK_PROG, m_CIH_link_type);
	DDX_Check(pDX, IDC_CIH_ADS_MSG_FORMAT, m_cih_ads_msg_format);
	DDX_Check(pDX, IDC_CIH_ADS_RECEIVE, m_cih_ads_receive);
	DDX_Check(pDX, IDC_CIH_ADS_SEND, m_cih_ads_send);
	DDX_Radio(pDX, IDC_CIH_FLOAT_ENCODE_PC, m_CIH_float_encode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCICS, CPropertyPage)
	//{{AFX_MSG_MAP(CCICS)
	ON_BN_CLICKED(IDC_CIH_NONE, OnCihNone)
	ON_BN_CLICKED(IDC_CIH_V1, OnCihV1)
	ON_BN_CLICKED(IDC_CIH_V2, OnCihV2)
	ON_EN_CHANGE(IDC_CIH_ABEND_CODE, OnChangeCihAbendCode)
	ON_EN_CHANGE(IDC_CIH_AID, OnChangeCihAid)
	ON_EN_CHANGE(IDC_CIH_AUTHENTICATOR, OnChangeCihAuthenticator)
	ON_EN_CHANGE(IDC_CIH_CANCEL_CODE, OnChangeCihCancelCode)
	ON_EN_CHANGE(IDC_CIH_CODEPAGE, OnChangeCihCodepage)
	ON_EN_CHANGE(IDC_CIH_COMP_CODE, OnChangeCihCompCode)
	ON_BN_CLICKED(IDC_CIH_CONVERSATIONAL, OnCihConversational)
	ON_EN_CHANGE(IDC_CIH_CURSOR_POS, OnChangeCihCursorPos)
	ON_EN_CHANGE(IDC_CIH_DATA_LENGTH, OnChangeCihDataLength)
	ON_BN_CLICKED(IDC_CIH_END_BACKOUT, OnCihEndBackout)
	ON_BN_CLICKED(IDC_CIH_END_COMMIT, OnCihEndCommit)
	ON_BN_CLICKED(IDC_CIH_END_ENDTASK, OnCihEndEndtask)
	ON_BN_CLICKED(IDC_CIH_END_NOSYNC, OnCihEndNosync)
	ON_EN_CHANGE(IDC_CIH_ERROR_OFFSET, OnChangeCihErrorOffset)
	ON_EN_CHANGE(IDC_CIH_FACILITY, OnChangeCihFacility)
	ON_EN_CHANGE(IDC_CIH_FACILITY_LIKE, OnChangeCihFacilityLike)
	ON_EN_CHANGE(IDC_CIH_FLAGS, OnChangeCihFlags)
	ON_EN_CHANGE(IDC_CIH_FORMAT, OnChangeCihFormat)
	ON_BN_CLICKED(IDC_CIH_LINK_PROG, OnCihLinkProg)
	ON_BN_CLICKED(IDC_CIH_LINK_TRAN, OnCihLinkTran)
	ON_EN_CHANGE(IDC_CIH_NEXT_TRANID, OnChangeCihNextTranid)
	ON_EN_CHANGE(IDC_CIH_PROGRAM, OnChangeCihProgram)
	ON_EN_CHANGE(IDC_CIH_REASON, OnChangeCihReason)
	ON_EN_CHANGE(IDC_CIH_REMOTE_SYS, OnChangeCihRemoteSys)
	ON_EN_CHANGE(IDC_CIH_REMOTE_TRAN, OnChangeCihRemoteTran)
	ON_EN_CHANGE(IDC_CIH_REPLY_FORMAT, OnChangeCihReplyFormat)
	ON_EN_CHANGE(IDC_CIH_RETURN_CODE, OnChangeCihReturnCode)
	ON_BN_CLICKED(IDC_CIH_SC_DATA, OnCihScData)
	ON_BN_CLICKED(IDC_CIH_SC_NONE, OnCihScNone)
	ON_BN_CLICKED(IDC_CIH_SC_START, OnCihScStart)
	ON_BN_CLICKED(IDC_CIH_SC_TERM, OnCihScTerm)
	ON_EN_CHANGE(IDC_CIH_TRANS_ID, OnChangeCihTransId)
	ON_BN_CLICKED(IDC_CIH_UOW_COMMIT, OnCihUowCommit)
	ON_BN_CLICKED(IDC_CIH_UOW_CONT, OnCihUowCont)
	ON_BN_CLICKED(IDC_CIH_UOW_FIRST, OnCihUowFirst)
	ON_BN_CLICKED(IDC_CIH_UOW_LAST, OnCihUowLast)
	ON_BN_CLICKED(IDC_CIH_UOW_MIDDLE, OnCihUowMiddle)
	ON_BN_CLICKED(IDC_CIH_UOW_ONLY, OnCihUowOnly)
	ON_BN_CLICKED(IDC_CIH_UOW_ROLLBACK, OnCihUowRollback)
	ON_EN_CHANGE(IDC_CIH_WAIT_INTERVAL, OnChangeCihWaitInterval)
	ON_EN_CHANGE(IDC_CIH_KEEP_TIME, OnChangeCihKeepTime)
	ON_EN_CHANGE(IDC_CIH_INPUT_ITEM, OnChangeCihInputItem)
	ON_BN_CLICKED(IDC_CIH_INT_ENCODE_PC, OnCihIntEncodePc)
	ON_BN_CLICKED(IDC_CIH_INT_ENCODE_HOST, OnCihIntEncodeHost)
	ON_BN_CLICKED(IDC_CIH_PD_ENCODE_PC, OnCihPdEncodePc)
	ON_BN_CLICKED(IDC_CIH_PD_ENCODE_HOST, OnCihPdEncodeHost)
	ON_BN_CLICKED(IDC_CIH_ADS_SEND, OnCihAdsSend)
	ON_CBN_EDITCHANGE(IDC_CIH_FUNCTION, OnEditchangeCihFunction)
	ON_CBN_DROPDOWN(IDC_CIH_FUNCTION, OnDropdownCihFunction)
	ON_CBN_SELCHANGE(IDC_CIH_FUNCTION, OnSelchangeCihFunction)
	ON_CBN_SELENDOK(IDC_CIH_FUNCTION, OnSelendokCihFunction)
	ON_BN_CLICKED(IDC_CIH_ADS_MSG_FORMAT, OnCihAdsMsgFormat)
	ON_BN_CLICKED(IDC_CIH_ADS_RECEIVE, OnCihAdsReceive)
	ON_BN_CLICKED(IDC_CIH_FLOAT_ENCODE_PC, OnCihFloatEncodePc)
	ON_BN_CLICKED(IDC_CIH_FLOAT_ENCODE_HOST, OnCihFloatEncodeHost)
	ON_BN_CLICKED(IDC_CIH_FLOAT_ENCODE_390, OnCihFloatEncode390)
	ON_BN_CLICKED(IDC_CIH_FLOAT_ENCODE_TNS, OnCihFloatEncodeTns)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CCICS::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CCICS::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCICS message handlers

BOOL CCICS::PreCreateWindow(CREATESTRUCT& cs) 

{
	// settings for a property page dialog
	cs.style |= WS_TABSTOP;
	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	
	return CPropertyPage::PreCreateWindow(cs);
}

BOOL CCICS::OnInitDialog() 

{
	CPropertyPage::OnInitDialog();
	
	// tool tips are used and must be enabled
	EnableToolTips(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CCICS::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

int CCICS::parseCICSheader(unsigned char *cicsData, int dataLen, int ccsid, int encodeType)

{
	int				length=0;
	int				len=0;
	int				err=0;
	int				charType;
	int				maxLen=MQCIH_LENGTH_2;
	unsigned char	tempArea[32] = { 0 };
	MQCIH			tempCIH;
	CRfhutilApp*	app;
	char	traceInfo[512];		// work variable to build trace message

	if (dataLen < MQCIH_LENGTH_1)
	{
		// not enough data to be a CIH
		return 0;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::parseCICSheader() cicsData=%8.8X dataLen=%d ccsid=%d encodeType=%d", (unsigned int)&cicsData, dataLen, ccsid, encodeType);

		// trace entry to parseCICSheader
		pDoc->logTraceEntry(traceInfo);

		// check if there are not enough bytes in the input data
		if (maxLen > dataLen)
		{
			// make sure we don't go past the end of the buffer
			maxLen = dataLen;
		}

		// write the data to the trace file
		pDoc->dumpTraceData("cicsData", cicsData, maxLen);
	}

	// copy the data into the temporary work area
	len = MQCIH_LENGTH_1;
	if (dataLen >= MQCIH_LENGTH_2)
	{
		len = MQCIH_LENGTH_2;
	}

	memcpy(&tempCIH, cicsData, len);

	// figure out what kind of data we are dealing with
	charType = getCcsidType(ccsid);

	// check the code page of the CIH
	if (CHAR_EBCDIC == charType)
	{
		// translate the charater data to ascii
		EbcdicToAscii((unsigned char *)&tempCIH.StrucId, sizeof(tempCIH.StrucId), tempArea);
		memcpy(tempCIH.StrucId, tempArea, sizeof(tempCIH.StrucId));
		EbcdicToAscii((unsigned char *)&tempCIH.Format, sizeof(tempCIH.Format), tempArea);
		memcpy(tempCIH.Format, tempArea, sizeof(tempCIH.Format));
		EbcdicToAscii((unsigned char *)&tempCIH.Function, sizeof(tempCIH.Function), tempArea);
		memcpy(tempCIH.Function, tempArea, sizeof(tempCIH.Function));
		EbcdicToAscii((unsigned char *)&tempCIH.AbendCode, sizeof(tempCIH.AbendCode), tempArea);
		memcpy(tempCIH.AbendCode, tempArea, sizeof(tempCIH.AbendCode));
		EbcdicToAscii((unsigned char *)&tempCIH.Authenticator, sizeof(tempCIH.Authenticator), tempArea);
		memcpy(tempCIH.Authenticator, tempArea, sizeof(tempCIH.Authenticator));
		EbcdicToAscii((unsigned char *)&tempCIH.ReplyToFormat, sizeof(tempCIH.ReplyToFormat), tempArea);
		memcpy(tempCIH.ReplyToFormat, tempArea, sizeof(tempCIH.ReplyToFormat));
		EbcdicToAscii((unsigned char *)&tempCIH.RemoteSysId, sizeof(tempCIH.RemoteSysId), tempArea);
		memcpy(tempCIH.RemoteSysId, tempArea, sizeof(tempCIH.RemoteSysId));
		EbcdicToAscii((unsigned char *)&tempCIH.RemoteTransId, sizeof(tempCIH.RemoteTransId), tempArea);
		memcpy(tempCIH.RemoteTransId, tempArea, sizeof(tempCIH.RemoteTransId));
		EbcdicToAscii((unsigned char *)&tempCIH.TransactionId, sizeof(tempCIH.TransactionId), tempArea);
		memcpy(tempCIH.TransactionId, tempArea, sizeof(tempCIH.TransactionId));
		EbcdicToAscii((unsigned char *)&tempCIH.FacilityLike, sizeof(tempCIH.FacilityLike), tempArea);
		memcpy(tempCIH.FacilityLike, tempArea, sizeof(tempCIH.FacilityLike));
		EbcdicToAscii((unsigned char *)&tempCIH.AttentionId, sizeof(tempCIH.AttentionId), tempArea);
		memcpy(tempCIH.AttentionId, tempArea, sizeof(tempCIH.AttentionId));
		EbcdicToAscii((unsigned char *)&tempCIH.StartCode, sizeof(tempCIH.StartCode), tempArea);
		memcpy(tempCIH.StartCode, tempArea, sizeof(tempCIH.StartCode));
		EbcdicToAscii((unsigned char *)&tempCIH.CancelCode, sizeof(tempCIH.CancelCode), tempArea);
		memcpy(tempCIH.CancelCode, tempArea, sizeof(tempCIH.CancelCode));
		EbcdicToAscii((unsigned char *)&tempCIH.NextTransactionId, sizeof(tempCIH.NextTransactionId), tempArea);
		memcpy(tempCIH.NextTransactionId, tempArea, sizeof(tempCIH.NextTransactionId));
		EbcdicToAscii((unsigned char *)&tempCIH.Reserved1, sizeof(tempCIH.Reserved1), tempArea);
		memcpy(tempCIH.Reserved1, tempArea, sizeof(tempCIH.Reserved1));
		EbcdicToAscii((unsigned char *)&tempCIH.Reserved2, sizeof(tempCIH.Reserved2), tempArea);
		memcpy(tempCIH.Reserved2, tempArea, sizeof(tempCIH.Reserved2));
		EbcdicToAscii((unsigned char *)&tempCIH.Reserved3, sizeof(tempCIH.Reserved3), tempArea);
		memcpy(tempCIH.Reserved3, tempArea, sizeof(tempCIH.Reserved3));
	}

	// check the encoding of the data
	if (encodeType != NUMERIC_PC)
	{
		tempCIH.Version = reverseBytes4(tempCIH.Version);
		tempCIH.StrucLength = reverseBytes4(tempCIH.StrucLength);
		tempCIH.Encoding = reverseBytes4(tempCIH.Encoding);
		tempCIH.CodedCharSetId = reverseBytes4(tempCIH.CodedCharSetId);
		tempCIH.Flags = reverseBytes4(tempCIH.Flags);
		tempCIH.ReturnCode = reverseBytes4(tempCIH.ReturnCode);
		tempCIH.CompCode = reverseBytes4(tempCIH.CompCode);
		tempCIH.Reason = reverseBytes4(tempCIH.Reason);
		tempCIH.UOWControl = reverseBytes4(tempCIH.UOWControl);
		tempCIH.GetWaitInterval = reverseBytes4(tempCIH.GetWaitInterval);
		tempCIH.LinkType = reverseBytes4(tempCIH.LinkType);
		tempCIH.OutputDataLength = reverseBytes4(tempCIH.OutputDataLength);
		tempCIH.FacilityKeepTime = reverseBytes4(tempCIH.FacilityKeepTime);
		tempCIH.ADSDescriptor = reverseBytes4(tempCIH.ADSDescriptor);
		tempCIH.ConversationalTask = reverseBytes4(tempCIH.ConversationalTask);
		tempCIH.TaskEndStatus = reverseBytes4(tempCIH.TaskEndStatus);
	}

	if (MQCIH_VERSION_1 == tempCIH.Version)
	{
		// these fields need to be initialized
		tempCIH.CursorPosition = 0;
		tempCIH.ErrorOffset = 0;
		tempCIH.InputItem = 0;
		tempCIH.Reserved4 = 0;
	}

	// check the encoding of the data
	if (encodeType != NUMERIC_PC)
	{
		tempCIH.CursorPosition = reverseBytes4(tempCIH.CursorPosition);
		tempCIH.ErrorOffset = reverseBytes4(tempCIH.ErrorOffset);
		tempCIH.InputItem = reverseBytes4(tempCIH.InputItem);
		tempCIH.Reserved4 = reverseBytes4(tempCIH.Reserved4);
	}

	// check if we have a CICS header
	if ((memcmp(&tempCIH.StrucId, MQCIH_STRUC_ID, sizeof(tempCIH.StrucId)) != 0) ||
		((tempCIH.Version != MQCIH_VERSION_1) && (tempCIH.Version != MQCIH_VERSION_2)))
	{
		return 0;
	}

	m_CIH_program.Empty();
	if (tempCIH.Version == MQCIH_VERSION_1)
	{
		m_CIH_version = CIH_V1;
		length = MQCIH_LENGTH_1;

		if (MQCLT_PROGRAM == tempCIH.LinkType)
		{
			if (dataLen >= MQCIH_LENGTH_1 + CIH_MAX_PROG_NAME)
			{
				// get the program name
				if (CHAR_EBCDIC == charType)
				{
					EbcdicToAscii(cicsData + MQCIH_LENGTH_1, CIH_MAX_PROG_NAME, tempArea);
				}
				else
				{
					memcpy(tempArea, cicsData + MQCIH_LENGTH_1, CIH_MAX_PROG_NAME);
				}

				tempArea[CIH_MAX_PROG_NAME] = 0;
				m_CIH_program = tempArea;
				length += CIH_MAX_PROG_NAME;
			}
		}
	}
	else
	{
		m_CIH_version = CIH_V2;
		length = MQCIH_LENGTH_2;

		if (MQCLT_PROGRAM == tempCIH.LinkType)
		{
			if (dataLen >= MQCIH_LENGTH_1 + CIH_MAX_PROG_NAME)
			{
				// get the program name
				if (CHAR_EBCDIC == charType)
				{
					EbcdicToAscii(cicsData + MQCIH_LENGTH_2, CIH_MAX_PROG_NAME, tempArea);
				}
				else
				{
					memcpy(tempArea, cicsData + MQCIH_LENGTH_2, CIH_MAX_PROG_NAME);
				}

				tempArea[CIH_MAX_PROG_NAME] = 0;
				m_CIH_program = tempArea;
				length += CIH_MAX_PROG_NAME;
			}
		}
	}

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.Format, sizeof(tempCIH.Format));
	m_CIH_format = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.NextTransactionId, sizeof(tempCIH.NextTransactionId));
	m_CIH_next_tranid = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.CancelCode, sizeof(tempCIH.CancelCode));
	m_CIH_cancel_code = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.AbendCode, sizeof(tempCIH.AbendCode));
	m_CIH_abend_code = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.Authenticator, sizeof(tempCIH.Authenticator));
	m_CIH_authenticator = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.ReplyToFormat, sizeof(tempCIH.ReplyToFormat));
	m_CIH_reply_format = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.RemoteSysId, sizeof(tempCIH.RemoteSysId));
	m_CIH_remote_sys = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.RemoteTransId, sizeof(tempCIH.RemoteTransId));
	m_CIH_remote_tran = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.TransactionId, sizeof(tempCIH.TransactionId));
	m_CIH_transid = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.AttentionId, sizeof(tempCIH.AttentionId));
	m_CIH_aid = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.FacilityLike, sizeof(tempCIH.FacilityLike));
	m_CIH_facility_like = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempCIH.Function, sizeof(tempCIH.Function));
	m_CIH_function = tempArea;

	// capture the facility field in hex
	memset(tempArea, 0, sizeof(tempArea));
	AsciiToHex(tempCIH.Facility, sizeof(tempCIH.Facility), tempArea);
	m_CIH_facility = tempArea;

	m_CIH_startcode = CIH_START_NONE;
	if (memcmp(tempCIH.StartCode, MQCSC_START, sizeof(tempCIH.StartCode)) == 0)
	{
		m_CIH_startcode = CIH_START_S;
	}

	if (memcmp(tempCIH.StartCode, MQCSC_STARTDATA, sizeof(tempCIH.StartCode)) == 0)
	{
		m_CIH_startcode = CIH_START_SDATA;
	}

	if (memcmp(tempCIH.StartCode, MQCSC_TERMINPUT, sizeof(tempCIH.StartCode)) == 0)
	{
		m_CIH_startcode = CIH_START_TERM;
	}

	m_CIH_uow = CIH_UOW_ONLY;
	switch (tempCIH.UOWControl)
	{
	case MQCUOWC_ONLY:
		{
			m_CIH_uow = CIH_UOW_ONLY;
			break;
		}
	case MQCUOWC_CONTINUE:
		{
			m_CIH_uow = CIH_UOW_CONT;
			break;
		}
	case MQCUOWC_FIRST:
		{
			m_CIH_uow = CIH_UOW_FIRST;
			break;
		}
	case MQCUOWC_MIDDLE:
		{
			m_CIH_uow = CIH_UOW_MIDDLE;
			break;
		}
	case MQCUOWC_LAST:
		{
			m_CIH_uow = CIH_UOW_LAST;
			break;
		}
	case MQCUOWC_COMMIT:
		{
			m_CIH_uow = CIH_UOW_COMMIT;
			break;
		}
	case MQCUOWC_BACKOUT:
		{
			m_CIH_uow = CIH_UOW_BACKOUT;
			break;
		}
	}

	m_cih_ads_send = ((tempCIH.ADSDescriptor & MQCADSD_SEND) > 0);
	m_cih_ads_receive = ((tempCIH.ADSDescriptor & MQCADSD_RECV) > 0);
	m_cih_ads_msg_format = ((tempCIH.ADSDescriptor & MQCADSD_MSGFORMAT) > 0);

	if (MQCLT_TRANSACTION == tempCIH.LinkType)
	{
		m_CIH_link_type = CIH_LINK_TRAN;
	}
	else
	{
		if (MQCLT_PROGRAM == tempCIH.LinkType)
		{
			m_CIH_link_type = CIH_LINK_PROG;
		}
		else
		{
			m_CIH_link_type = -1;
		}
	}

	if (MQCCT_YES == tempCIH.ConversationalTask)
	{
		m_CIH_conversational = TRUE;
	}
	else
	{
		m_CIH_conversational = FALSE;
	}

	// get the encoding value
	m_CIH_int_encode = getIntEncode(tempCIH.Encoding);
	m_CIH_pd_encode = getPDEncode(tempCIH.Encoding);
	m_CIH_float_encode = getFloatEncode(tempCIH.Encoding);

	m_CIH_return_code.Format("%d", tempCIH.ReturnCode);
	m_CIH_comp_code.Format("%d", tempCIH.CompCode);
	m_CIH_reason.Format("%d", tempCIH.Reason);
	if ((tempCIH.CursorPosition < 100000) && (tempCIH.CursorPosition > -10000))
	{
		m_CIH_cursor_pos.Format("%d", tempCIH.CursorPosition);
	}
	else
	{
		err = 1;
		m_CIH_cursor_pos.Empty();
	}

	if ((tempCIH.ErrorOffset < 100000) && (tempCIH.ErrorOffset > -10000))
	{
		m_CIH_error_ofs.Format("%d", tempCIH.ErrorOffset);
	}
	else
	{
		err = 1;
		m_CIH_error_ofs.Empty();
	}

	if ((tempCIH.CodedCharSetId < 100000) && (tempCIH.CodedCharSetId > -10000))
	{
		m_CIH_codepage.Format("%d", tempCIH.CodedCharSetId);
	}
	else
	{
		err = 1;
		m_CIH_codepage.Empty();
	}

	m_CIH_flags.Format("%d", tempCIH.Flags);
	m_CIH_wait_interval.Format("%d", tempCIH.GetWaitInterval);
	m_CIH_data_length.Format("%d", tempCIH.OutputDataLength);
	m_CIH_keep_time.Format("%d", tempCIH.FacilityKeepTime);
	m_CIH_input_item.Format("%d", tempCIH.InputItem);

	// capture the task end value
	if (MQCTES_NOSYNC == tempCIH.TaskEndStatus)
	{
		m_CIH_status = CIH_END_NOSYNC;
	}
	else if (MQCTES_COMMIT == tempCIH.TaskEndStatus)
	{
		m_CIH_status = CIH_END_COMMIT;
	}
	else if (MQCTES_BACKOUT == tempCIH.TaskEndStatus)
	{
		m_CIH_status = CIH_END_BACKOUT;
	}
	else if (MQCTES_ENDTASK == tempCIH.TaskEndStatus)
	{
		m_CIH_status = CIH_END_ENDTASK;
	}
	else
	{
		// status is not recognized
		m_CIH_status = -1;
	}

	if (err > 0)
	{
		app = (CRfhutilApp *)AfxGetApp();
		app->pDocument.m_error_msg += "*** Errors parsing CICS header\r\n";
	}

	// update the form data from the instance variables
	UpdateData(FALSE);
	
	// save the current CICS header
	cicsHeader = (unsigned char *)rfhMalloc(length + 1, "CICSHDR ");
	memset(cicsHeader, 0, length + 1);
	memcpy(cicsHeader, cicsData, length);
	cicsLength = length;

	// remember the ccsid and encoding of this header
	cicsCcsid = ccsid;
	cicsEncodeType = encodeType;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCICS::parseCICSheader() length=%d m_CIH_version=%d m_CIH_format=%s m_CIH_program=%s", length, m_CIH_version, (LPCTSTR)m_CIH_format, (LPCTSTR)m_CIH_program);

		// trace exit from parseCICSheader
		pDoc->logTraceEntry(traceInfo);
	}

	return length;
}

int CCICS::buildCICSheader(unsigned char * header, int ccsid, int encodeType)

{
	int		progLen;
	int		len=0;
	char	tempProg[CIH_MAX_PROG_NAME];
	char	traceInfo[512];		// work variable to build trace message

	// get the form data into the instance variables
	UpdateData(TRUE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::buildCICSheader() ccsid=%d encodeType=%d m_CIH_version=%d", ccsid, encodeType, m_CIH_version);

		// trace entry to buildCICSheader
		pDoc->logTraceEntry(traceInfo);
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	// is there a header?
	if (CIH_NONE == m_CIH_version)
	{
		return 0;
	}

	// check if the header exists and is of the correct character type and encoding
	if ((cicsHeader != NULL) && ((cicsCcsid != ccsid) || (cicsEncodeType != encodeType)))
	{
		// release the storage held by the current header
		freeCurrentHeader(10);
	}

	// does the header already exist?
	if (NULL == cicsHeader)
	{
		// does not exist, so create the header
		createHeader(ccsid, encodeType);
	}

	// set the length to be returned
	len = cicsLength;

	// copy the header to the requested data area
	if (cicsHeader != NULL) {
		memcpy(header, cicsHeader, cicsLength);
	}

	// only include the program name for prog type requests (default)
	if ((cicsHeader != NULL) && (CIH_LINK_TRAN != m_CIH_link_type))
	{
		// build the program name
		// initialize the workarea to spaces
		memset(tempProg, 32, sizeof(tempProg));

		// get the length of the input field
		progLen = m_CIH_program.GetLength();
		if (progLen > CIH_MAX_PROG_NAME)
		{
			// Make sure it is not too long
			progLen = CIH_MAX_PROG_NAME;
		}

		if (progLen > 0)
		{
			// get the program name
			memcpy(tempProg, (LPCTSTR)m_CIH_program, progLen);
		}

		if (CHAR_EBCDIC == getCcsidType(atoi((LPCTSTR)m_CIH_codepage)))
		{
			AsciiToEbcdic((unsigned char *)tempProg, CIH_MAX_PROG_NAME, header + cicsLength);
		}
		else
		{
			memcpy(header + cicsLength, tempProg, CIH_MAX_PROG_NAME);
		}

		len += CIH_MAX_PROG_NAME;
	}

	// Update the data in the controls
	UpdateData (FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCICS::buildCICSheader() len=%d", len);

		// trace exit from buildCICSheader
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("CICS header", cicsHeader, len);
	}

	return len;
}

int CCICS::getCICSlength()

{
	int		length=0;
	char	traceInfo[128];		// work variable to build trace message

	switch (m_CIH_version)
	{
	case CIH_V1:
		{
			length = MQCIH_LENGTH_1;
			break;
		}
	case CIH_V2:
		{
			length = MQCIH_LENGTH_2;
			break;
		}
	}

	// check if this is a prog or tran type of request
	if ((CIH_LINK_PROG == m_CIH_link_type) && (m_CIH_version != CIH_NONE))
	{
		length += CIH_MAX_PROG_NAME;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCICS::getCICSlength() length=%d m_CIH_link_type=%d", length, m_CIH_link_type);

		// trace exit from getIMSlength
		pDoc->logTraceEntry(traceInfo);
	}

	return length;
}

void CCICS::resetCICSheader()

{
	if ((pDoc != NULL) && pDoc->traceEnabled)
	{
		// trace entry to resetCICSheader
		pDoc->logTraceEntry("Entering CCICS::resetCICSheader()");
	}

	if (cicsHeader != NULL)
	{
		rfhFree(cicsHeader);
		cicsHeader = NULL;
	}

	cicsLength = 0;

	// reset the individual fields
	m_CIH_abend_code.Empty();
	m_cih_ads_send = FALSE;
	m_cih_ads_receive = FALSE;
	m_cih_ads_msg_format = FALSE;
	m_CIH_aid.Empty();
	m_CIH_authenticator.Empty();
	m_CIH_cancel_code.Empty();
	m_CIH_codepage.Empty();
	m_CIH_comp_code.Empty();
	m_CIH_conversational = FALSE;
	m_CIH_cursor_pos.Empty();
	m_CIH_data_length.Empty();
	m_CIH_error_ofs.Empty();
	m_CIH_facility.Empty();
	m_CIH_facility_like.Empty();
	m_CIH_flags.Empty();
	m_CIH_format.Empty();
	m_CIH_function.Empty();
	m_CIH_input_item.Empty();
	m_CIH_int_encode = -1;
	m_CIH_keep_time.Empty();
	m_CIH_link_type = CIH_LINK_PROG;
	m_CIH_next_tranid.Empty();
	m_CIH_pd_encode = NUMERIC_HOST;
	m_CIH_float_encode = FLOAT_PC;
	m_CIH_program.Empty();
	m_CIH_reason.Empty();
	m_CIH_remote_sys.Empty();
	m_CIH_remote_tran.Empty();
	m_CIH_reply_format.Empty();
	m_CIH_return_code.Empty();
	m_CIH_startcode = CIH_START_NONE;
	m_CIH_status = CIH_END_NOSYNC;
	m_CIH_transid.Empty();
	m_CIH_uow = CIH_UOW_ONLY;
	m_CIH_version = CIH_NONE;
	m_CIH_wait_interval.Empty();
}

void CCICS::clearCICSheader()

{
	resetCICSheader();

	// Update the data in the controls
	UpdateData (FALSE);
}

void CCICS::OnCihNone() 

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::OnCihNone() m_CIH_format=%s m_CIH_codepage=%s m_CIH_int_encode=%d m_CIH_pd_encode=%d m_CIH_float_encode=%d GetDlgItem(IDC_CIH_NONE))->GetCheck()=%d", (LPCTSTR)m_CIH_format, (LPCTSTR)m_CIH_codepage, m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode, ((CButton *)GetDlgItem(IDC_CIH_NONE))->GetCheck());

		// trace entry to OnCihNone
		pDoc->logTraceEntry(traceInfo);
	}

	// check if we are just setting the focus to this button
	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_CIH_NONE))->GetCheck() == 1)
	{
		// remove the CICS header from the current header chain
		// make sure we have current info
		UpdateData(TRUE);

		// remove the header from the chain
		pDoc->removeMQheader(MQHEADER_CICS, (LPCTSTR)m_CIH_format, atoi((LPCTSTR)m_CIH_codepage), m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode);

		// disable most of the controls
		enableDisplay();

		// remember the current version
		m_save_CIH_version = m_CIH_version;
	}
}

void CCICS::freeCurrentHeader(const int callee)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::freeCurrentHeader() callee=%d cicsHeader=%8.8X cicsLength=%d cicsCcsid=%d cicsEncodeType=%d", callee, (unsigned int)&cicsHeader, cicsLength, cicsCcsid, cicsEncodeType);

		// trace entry to freeCurrentHeader
		pDoc->logTraceEntry(traceInfo);
	}

	// free the current area
	if (cicsHeader != NULL)
	{
		rfhFree(cicsHeader);
		cicsHeader = NULL;
	}

	cicsCcsid = -1;
	cicsEncodeType = -1;
	cicsLength = 0;
}

void CCICS::createHeader(int ccsid, int encodeType)

{
	int				workLen;
	int				ads=MQCADSD_NONE;
	int				charType;
	MQCIH			tempCIH={MQCIH_DEFAULT};
	unsigned char	tempArea[32];
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::createHeader() ccsid=%d encodeType=%d m_CIH_version=%d", ccsid, encodeType, m_CIH_version);

		// trace entry to createHeader
		pDoc->logTraceEntry(traceInfo);
	}

	// is there a header?
	if (CIH_NONE == m_CIH_version)
	{
		return;
	}

	// get the form data into the instance variables
	UpdateData(TRUE);
	
	if (CIH_V1 == m_CIH_version)
	{
		tempCIH.Version = MQCIH_VERSION_1;
		workLen = MQCIH_LENGTH_1;
	}
	else
	{
		tempCIH.Version = MQCIH_VERSION_2;
		workLen = MQCIH_LENGTH_2;
	}

	tempCIH.StrucLength = workLen;
	tempCIH.CodedCharSetId = atoi((LPCTSTR)m_CIH_codepage);

	// set integer fields
	tempCIH.Flags = atoi((LPCTSTR)m_CIH_flags);
	tempCIH.ReturnCode = atoi((LPCTSTR)m_CIH_return_code);
	tempCIH.CompCode = atoi((LPCTSTR)m_CIH_comp_code);
	tempCIH.Reason = atoi((LPCTSTR)m_CIH_reason);
	tempCIH.GetWaitInterval = atoi((LPCTSTR)m_CIH_wait_interval);
	tempCIH.OutputDataLength = atoi((LPCTSTR)m_CIH_data_length);
	tempCIH.CursorPosition = atoi((LPCTSTR)m_CIH_cursor_pos);
	tempCIH.ErrorOffset = atoi((LPCTSTR)m_CIH_error_ofs);
	tempCIH.InputItem = atoi((LPCTSTR)m_CIH_input_item);
	tempCIH.FacilityKeepTime = atoi((LPCTSTR)m_CIH_keep_time);

	// set the encoding
	// N.B. the getEncodingValue routine will default to the little-endian if the encoding was not set
	tempCIH.Encoding = getEncodingValue(m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode);

	// set character data fields
	setCharData(tempCIH.Format, sizeof(tempCIH.Format), (LPCTSTR)m_CIH_format, m_CIH_format.GetLength());
	setCharData(tempCIH.AbendCode, sizeof(tempCIH.AbendCode), (LPCTSTR)m_CIH_abend_code, m_CIH_abend_code.GetLength());
	setCharData(tempCIH.Authenticator, sizeof(tempCIH.Authenticator), (LPCTSTR)m_CIH_authenticator, m_CIH_authenticator.GetLength());
	setCharData(tempCIH.ReplyToFormat, sizeof(tempCIH.ReplyToFormat), (LPCTSTR)m_CIH_reply_format, m_CIH_reply_format.GetLength());
	setCharData(tempCIH.RemoteSysId, sizeof(tempCIH.RemoteSysId), (LPCTSTR)m_CIH_remote_sys, m_CIH_remote_sys.GetLength());
	setCharData(tempCIH.RemoteTransId, sizeof(tempCIH.RemoteTransId), (LPCTSTR)m_CIH_remote_tran, m_CIH_remote_tran.GetLength());
	setCharData(tempCIH.TransactionId, sizeof(tempCIH.TransactionId), (LPCTSTR)m_CIH_transid, m_CIH_transid.GetLength());
	setCharData(tempCIH.FacilityLike, sizeof(tempCIH.FacilityLike), (LPCTSTR)m_CIH_facility_like, m_CIH_facility_like.GetLength());
	setCharData(tempCIH.AttentionId, sizeof(tempCIH.AttentionId), (LPCTSTR)m_CIH_aid, m_CIH_aid.GetLength());
	setCharData(tempCIH.CancelCode, sizeof(tempCIH.CancelCode), (LPCTSTR)m_CIH_cancel_code, m_CIH_cancel_code.GetLength());
	setCharData(tempCIH.NextTransactionId, sizeof(tempCIH.NextTransactionId), (LPCTSTR)m_CIH_next_tranid, m_CIH_next_tranid.GetLength());
	setCharData(tempCIH.Function, sizeof(tempCIH.Function), (LPCTSTR)m_CIH_function, m_CIH_function.GetLength());

	// set the facility value
	// initialize the temporary work area to 16 character zeros
	memset(tempArea, '0', sizeof(tempCIH.Facility) * 2);

	// make sure we don't overflow the work area
	// N.B. the work area is twice the expected length so this should not be a problem
	if (m_CIH_facility.GetLength() < sizeof(tempArea))
	{
		// copy the facility field into the work area, replacing the character zeros
		memcpy(tempArea, (LPCTSTR)m_CIH_facility, m_CIH_facility.GetLength());
	}

	// set the facility translating from hex to binary
	HexToAscii(tempArea, sizeof(tempCIH.Facility), tempCIH.Facility);

	// get the task end value
	switch (m_CIH_status)
	{
	case CIH_END_NOSYNC:
		{
			tempCIH.TaskEndStatus = MQCTES_NOSYNC;
			break;
		}
	case CIH_END_COMMIT:
		{
			tempCIH.TaskEndStatus = MQCTES_COMMIT;
			break;
		}
	case CIH_END_BACKOUT:
		{
			tempCIH.TaskEndStatus = MQCTES_BACKOUT;
			break;
		}
	case CIH_END_ENDTASK:
		{
			tempCIH.TaskEndStatus = MQCTES_ENDTASK;
			break;
		}
	}

	tempCIH.LinkType = MQCLT_PROGRAM;
	if (CIH_LINK_TRAN == m_CIH_link_type)
	{
		tempCIH.LinkType = MQCLT_TRANSACTION;
	}

	switch (m_CIH_uow)
	{
	case CIH_UOW_ONLY:
		{
			tempCIH.UOWControl = MQCUOWC_ONLY;
			break;
		}
	case CIH_UOW_CONT:
		{
			tempCIH.UOWControl = MQCUOWC_CONTINUE;
			break;
		}
	case CIH_UOW_FIRST:
		{
			tempCIH.UOWControl = MQCUOWC_FIRST;
			break;
		}
	case CIH_UOW_MIDDLE:
		{
			tempCIH.UOWControl = MQCUOWC_MIDDLE;
			break;
		}
	case CIH_UOW_LAST:
		{
			tempCIH.UOWControl = MQCUOWC_LAST;
			break;
		}
	case CIH_UOW_COMMIT:
		{
			tempCIH.UOWControl = MQCUOWC_COMMIT;
			break;
		}
	case CIH_UOW_BACKOUT:
		{
			tempCIH.UOWControl = MQCUOWC_BACKOUT;
			break;
		}
	}

	// set the start code
	switch (m_CIH_startcode)
	{
	case CIH_START_NONE:
		{
			memcpy(tempCIH.StartCode, MQCSC_NONE, 4);
			break;
		}
	case CIH_START_S:
		{
			memcpy(tempCIH.StartCode, MQCSC_START, 4);
			break;
		}
	case CIH_START_SDATA:
		{
			memcpy(tempCIH.StartCode, MQCSC_STARTDATA, 4);
			break;
		}
	case CIH_START_TERM:
		{
			memcpy(tempCIH.StartCode, MQCSC_TERMINPUT, 4);
			break;
		}
	default:
		{
			memcpy(tempCIH.StartCode, MQCSC_NONE, 4);
			break;
		}
	}

	// set the ADS descriptor field
	if (m_cih_ads_send)
	{
		ads |= MQCADSD_SEND;
	}

	if (m_cih_ads_receive)
	{
		ads |= MQCADSD_RECV;
	}

	if (m_cih_ads_msg_format)
	{
		ads |= MQCADSD_MSGFORMAT;
	}

	tempCIH.ADSDescriptor = ads;

	if (m_CIH_conversational)
	{
		tempCIH.ConversationalTask = MQCCT_YES;
	}
	else
	{
		tempCIH.ConversationalTask = MQCCT_NO;
	}

	// figure out what kind of data we are using
	charType = getCcsidType(ccsid);

	// check if we need to translate to EBCDIC
	if (CHAR_EBCDIC == charType)
	{
		// translate all charater data to EBCDIC
		convertEbcdic(tempCIH.StrucId, sizeof(tempCIH.StrucId));
		convertEbcdic(tempCIH.Format, sizeof(tempCIH.Format));
		convertEbcdic(tempCIH.Function, sizeof(tempCIH.Function));
		convertEbcdic(tempCIH.AbendCode, sizeof(tempCIH.AbendCode));
		convertEbcdic(tempCIH.Authenticator, sizeof(tempCIH.Authenticator));
		convertEbcdic(tempCIH.ReplyToFormat, sizeof(tempCIH.ReplyToFormat));
		convertEbcdic(tempCIH.RemoteSysId, sizeof(tempCIH.RemoteSysId));
		convertEbcdic(tempCIH.RemoteTransId, sizeof(tempCIH.RemoteTransId));
		convertEbcdic(tempCIH.TransactionId, sizeof(tempCIH.TransactionId));
		convertEbcdic(tempCIH.FacilityLike, sizeof(tempCIH.FacilityLike));
		convertEbcdic(tempCIH.AttentionId, sizeof(tempCIH.AttentionId));
		convertEbcdic(tempCIH.StartCode, sizeof(tempCIH.StartCode));
		convertEbcdic(tempCIH.CancelCode, sizeof(tempCIH.CancelCode));
		convertEbcdic(tempCIH.NextTransactionId, sizeof(tempCIH.NextTransactionId));
		convertEbcdic(tempCIH.Reserved1, sizeof(tempCIH.Reserved1));
		convertEbcdic(tempCIH.Reserved2, sizeof(tempCIH.Reserved2));
		convertEbcdic(tempCIH.Reserved3, sizeof(tempCIH.Reserved3));
	}

	// check if we need to convert to big-Endian
	if (encodeType != NUMERIC_PC)
	{
		tempCIH.Version = reverseBytes4(tempCIH.Version);
		tempCIH.StrucLength = reverseBytes4(tempCIH.StrucLength);
		tempCIH.Encoding = reverseBytes4(tempCIH.Encoding);
		tempCIH.CodedCharSetId = reverseBytes4(tempCIH.CodedCharSetId);
		tempCIH.Flags = reverseBytes4(tempCIH.Flags);
		tempCIH.ReturnCode = reverseBytes4(tempCIH.ReturnCode);
		tempCIH.CompCode = reverseBytes4(tempCIH.CompCode);
		tempCIH.Reason = reverseBytes4(tempCIH.Reason);
		tempCIH.UOWControl = reverseBytes4(tempCIH.UOWControl);
		tempCIH.GetWaitInterval = reverseBytes4(tempCIH.GetWaitInterval);
		tempCIH.LinkType = reverseBytes4(tempCIH.LinkType);
		tempCIH.OutputDataLength = reverseBytes4(tempCIH.OutputDataLength);
		tempCIH.FacilityKeepTime = reverseBytes4(tempCIH.FacilityKeepTime);
		tempCIH.ADSDescriptor = reverseBytes4(tempCIH.ADSDescriptor);
		tempCIH.ConversationalTask = reverseBytes4(tempCIH.ConversationalTask);
		tempCIH.TaskEndStatus = reverseBytes4(tempCIH.TaskEndStatus);
		tempCIH.CursorPosition = reverseBytes4(tempCIH.CursorPosition);
		tempCIH.ErrorOffset = reverseBytes4(tempCIH.ErrorOffset);
		tempCIH.InputItem = reverseBytes4(tempCIH.InputItem);
		tempCIH.Reserved4 = reverseBytes4(tempCIH.Reserved4);
	}

	// update the form from the instance variables
	UpdateData(FALSE);

	// allocate enough storage
	cicsHeader = (unsigned char *)rfhMalloc(MQCIH_LENGTH_2 + CIH_MAX_PROG_NAME, "CICSHDR2");
	memset(cicsHeader, 0, MQCIH_LENGTH_2 + CIH_MAX_PROG_NAME);
	memcpy(cicsHeader, (void *)&tempCIH, workLen);
	cicsLength = workLen;

	// remember the ccsid and encoding of this header
	cicsCcsid = ccsid;
	cicsEncodeType = encodeType;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCICS::createHeader() cicsLength=%d", cicsLength);

		// trace exit from createHeader
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("CICS header", cicsHeader, cicsLength);
	}
}

void CCICS::OnCihV1() 

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::OnCihV1() m_CIH_format=%s m_save_CIH_version=%d m_CIH_codepage=%s m_CIH_int_encode=%d m_CIH_pd_encode=%d m_CIH_float_encode=%d GetDlgItem(IDC_CIH_V1)->GetCheck()=%d", (LPCTSTR)m_CIH_format, m_save_CIH_version, (LPCTSTR)m_CIH_codepage, m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode, ((CButton *)GetDlgItem(IDC_CIH_V1))->GetCheck());

		// trace entry to OnCihV1
		pDoc->logTraceEntry(traceInfo);
	}

	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_CIH_V1))->GetCheck() == 1)
	{
		// user has added a version 1 header
		if (cicsLength != MQCIH_LENGTH_1)
		{
			freeCurrentHeader(IDC_CIH_V1);
		}

		// make sure we have current info
		UpdateData(TRUE);

		// check what the previous value was
		if (CIH_NONE == m_save_CIH_version)
		{
			// insert the header into the chain
			// N.B. this will call the updateFields routine in this class which will change the code page
			// and encoding to reflect the current values in the header that precedes the new CIH
			pDoc->insertMQheader(MQHEADER_CICS, atoi((LPCTSTR)m_CIH_codepage), m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode);
		}

		// disable most of the controls
		enableDisplay();

		// remember the current version
		m_save_CIH_version = m_CIH_version;
	}
}

void CCICS::OnCihV2() 

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::OnCihV2() m_CIH_format=%s m_save_CIH_version=%d m_CIH_codepage=%s m_CIH_int_encode=%d m_CIH_pd_encode=%d m_CIH_float_encode=%d GetDlgItem(IDC_CIH_V2)->GetCheck()=%d", (LPCTSTR)m_CIH_format, m_save_CIH_version, (LPCTSTR)m_CIH_codepage, m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode, ((CButton *)GetDlgItem(IDC_CIH_V2))->GetCheck());

		// trace entry to OnCihV2
		pDoc->logTraceEntry(traceInfo);
	}

	// check if we are just setting the focus to this button
	// check if this is a set focus event or a real one
	// This check is necessary since we set the focus to this
	// control in the SetActive handler.
	if (((CButton *)GetDlgItem(IDC_CIH_V2))->GetCheck() == 1)
	{
		if (cicsLength != MQCIH_LENGTH_2)
		{
			freeCurrentHeader(IDC_CIH_V2);
		}

		// make sure we have current info
		UpdateData(TRUE);

		// check what the previous value was
		if (CIH_NONE == m_save_CIH_version)
		{
			// insert the header into the chain
			// N.B. this will call the updateFields routine in this class which will change the code page
			// and encoding to reflect the current values in the header that precedes the new CIH
			pDoc->insertMQheader(MQHEADER_CICS, atoi((LPCTSTR)m_CIH_codepage), m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode);
		}

		// enable most of the controls
		enableDisplay();

		// remember the current version
		m_save_CIH_version = m_CIH_version;
	}
}

void CCICS::OnChangeCihAbendCode() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_ABEND_CODE);
}

void CCICS::OnChangeCihAid() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_AID);
}

void CCICS::OnChangeCihAuthenticator() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_AUTHENTICATOR);
}

void CCICS::OnChangeCihCancelCode() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_CANCEL_CODE);
}

void CCICS::OnChangeCihCodepage() 

{
	// delete any existing header data areas
	// get the form data into the instance variables
	UpdateData(TRUE);

	pDoc->freeHeader(m_CIH_format);

	// update the form data from the instance variables
	UpdateData(FALSE);
	
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_CODEPAGE);
}

void CCICS::OnChangeCihCompCode() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_COMP_CODE);
}

void CCICS::OnCihConversational() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_CONVERSATIONAL);
}

void CCICS::OnChangeCihCursorPos() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_CURSOR_POS);
}

void CCICS::OnChangeCihDataLength() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_DATA_LENGTH);
}

void CCICS::OnCihEndBackout() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_END_BACKOUT);
}

void CCICS::OnCihEndCommit() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_END_COMMIT);
}

void CCICS::OnCihEndEndtask() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_END_ENDTASK);
}

void CCICS::OnCihEndNosync() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_END_NOSYNC);
}

void CCICS::OnChangeCihErrorOffset() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_ERROR_OFFSET);
}

void CCICS::OnChangeCihFacility() 

{
	freeCurrentHeader(IDC_CIH_FACILITY);
}

void CCICS::OnChangeCihFacilityLike() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_FACILITY_LIKE);
}

void CCICS::OnChangeCihFlags() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_FLAGS);
}

void CCICS::OnChangeCihFormat() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_FORMAT);
}

void CCICS::OnCihLinkProg() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_LINK_PROG);
}

void CCICS::OnCihLinkTran() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_LINK_TRAN);
}

void CCICS::OnChangeCihNextTranid() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_NEXT_TRANID);
}

void CCICS::OnChangeCihProgram() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_PROGRAM);
}

void CCICS::OnChangeCihReason() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_REASON);
}

void CCICS::OnChangeCihRemoteSys() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_REMOTE_SYS);
}

void CCICS::OnChangeCihRemoteTran() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_REMOTE_TRAN);
}

void CCICS::OnChangeCihReplyFormat() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_REPLY_FORMAT);
}

void CCICS::OnChangeCihReturnCode() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_RETURN_CODE);
}

void CCICS::OnCihScData() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_SC_DATA);
}

void CCICS::OnCihScNone() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_SC_NONE);
}

void CCICS::OnCihScStart() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_SC_START);
}

void CCICS::OnCihScTerm() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_SC_TERM);
}

void CCICS::OnChangeCihTransId() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_TRANS_ID);
}

void CCICS::OnCihUowCommit() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_UOW_COMMIT);
}

void CCICS::OnCihUowCont() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_UOW_CONT);
}

void CCICS::OnCihUowFirst() 

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)pDoc->mqmdData;
	MQBYTE24	cid;

	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_UOW_FIRST);

	// set the correlation id to match
	memcpy(cid, MQCI_NEW_SESSION, sizeof(cid));
	mqmdObj->changeCorrelId(&cid);
}

void CCICS::OnCihUowLast() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_UOW_LAST);
}

void CCICS::OnCihUowMiddle() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_UOW_MIDDLE);
}

void CCICS::OnCihUowOnly() 

{
	MQMDPAGE	*mqmdObj=(MQMDPAGE *)pDoc->mqmdData;
	MQBYTE24	cid;

	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_UOW_ONLY);

	// set the correlation id to match
	memcpy(cid, MQCI_NEW_SESSION, sizeof(cid));
	mqmdObj->changeCorrelId(&cid);
}

void CCICS::OnCihUowRollback() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_UOW_ROLLBACK);
}

void CCICS::OnChangeCihWaitInterval() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_WAIT_INTERVAL);
}

void CCICS::OnChangeCihKeepTime() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_KEEP_TIME);
}

void CCICS::OnChangeCihInputItem() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_INPUT_ITEM);
}

void CCICS::OnCihIntEncodePc() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	// Update the encoding in the previous MQ header
	pDoc->freeHeader(m_CIH_format);

	// update the form data from the instance variables
	UpdateData(FALSE);
	
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_INT_ENCODE_PC);
}

void CCICS::OnCihIntEncodeHost() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	// Update the encoding in the previous MQ header
	pDoc->freeHeader(m_CIH_format);

	// update the form data from the instance variables
	UpdateData(FALSE);
	
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_INT_ENCODE_HOST);
}

void CCICS::OnCihPdEncodePc() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_PD_ENCODE_PC);
}

void CCICS::OnCihPdEncodeHost() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_PD_ENCODE_HOST);
}

void CCICS::OnCihFloatEncodePc() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_FLOAT_ENCODE_PC);
}

void CCICS::OnCihFloatEncodeHost() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_FLOAT_ENCODE_HOST);
}

void CCICS::OnCihFloatEncode390() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_FLOAT_ENCODE_390);
}

void CCICS::OnCihFloatEncodeTns() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_FLOAT_ENCODE_TNS);
}

void CCICS::OnCihAdsSend() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_ADS_SEND);
}

void CCICS::OnCihAdsMsgFmt() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_ADS_MSG_FORMAT);
}

void CCICS::OnEditchangeCihFunction() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_FUNCTION);
}

void CCICS::dlgItemAddString(const int dlgItem, const char *itemText)

{
#ifdef _UNICODE
	wchar_t	str[1024];

	// clear working storage
	memset(str, 0, sizeof(str));

	// convert the string to unicode
	mbstowcs(str, itemText, sizeof(str));

	// add the selection to the dialog box
	((CComboBox *)GetDlgItem(dlgItem))->AddString(str);
#else
	// add the selection to the dialog box
	((CComboBox *)GetDlgItem(dlgItem))->AddString(itemText);
#endif
}

void CCICS::OnDropdownCihFunction() 

{
	// insert the options into the function combo box
	if (!functionBoxInitialized)
	{
		functionBoxInitialized = TRUE;
		((CComboBox *)GetDlgItem(IDC_CIH_FUNCTION))->ResetContent();

		// insert the items into the drop down menu
		dlgItemAddString(IDC_CIH_FUNCTION, MQCFUNC_NONE);
		dlgItemAddString(IDC_CIH_FUNCTION, MQCFUNC_MQCONN);
		dlgItemAddString(IDC_CIH_FUNCTION, MQCFUNC_MQGET);
		dlgItemAddString(IDC_CIH_FUNCTION, MQCFUNC_MQINQ);
		dlgItemAddString(IDC_CIH_FUNCTION, MQCFUNC_MQOPEN);
		dlgItemAddString(IDC_CIH_FUNCTION, MQCFUNC_MQPUT);
		dlgItemAddString(IDC_CIH_FUNCTION, MQCFUNC_MQPUT1);
	}
}

BOOL CCICS::PreTranslateMessage(MSG* pMsg) 

{
	// this routine provides the support for tool tips
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
			if ((IDC_CIH_FACILITY == id) || 
				(IDC_CIH_FORMAT == id) || 
				(IDC_CIH_PROGRAM == id) || 
				(IDC_CIH_NEXT_TRANID == id) || 
				(IDC_CIH_CANCEL_CODE == id) || 
				(IDC_CIH_COMP_CODE == id) || 
				(IDC_CIH_RETURN_CODE == id) || 
				(IDC_CIH_REASON == id) || 
				(IDC_CIH_CURSOR_POS == id) || 
				(IDC_CIH_ERROR_OFFSET == id) || 
				(IDC_CIH_FACILITY_LIKE == id) || 
				(IDC_CIH_AID == id) || 
				(IDC_CIH_TRANS_ID == id) || 
				(IDC_CIH_REMOTE_SYS == id) || 
				(IDC_CIH_REMOTE_TRAN == id) || 
				(IDC_CIH_REPLY_FORMAT == id) || 
				(IDC_CIH_CODEPAGE == id) || 
				(IDC_CIH_FLAGS == id) || 
				(IDC_CIH_WAIT_INTERVAL == id) || 
				(IDC_CIH_DATA_LENGTH == id) || 
				(IDC_CIH_ABEND_CODE == id) || 
				(IDC_CIH_AUTHENTICATOR == id) || 
				(IDC_CIH_KEEP_TIME == id) || 
				(IDC_CIH_INPUT_ITEM == id))
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

BOOL CCICS::OnKillActive() 

{
	// moving to a different page
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering CCICS::OnKillActive()");
	}

	// get the current data from the controls into the instance variables
	UpdateData (TRUE);

	return CPropertyPage::OnKillActive();
}

BOOL CCICS::OnSetActive() 

{
	// the CICS page has been selected
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::OnSetActive() m_CIH_version=%d m_save_CIH_version=%d", m_CIH_version, m_save_CIH_version);

		// trace entry to OnSetActive
		pDoc->logTraceEntry(traceInfo);
	}

	m_save_CIH_version = m_CIH_version;

	// update the controls from the instance variables
	UpdateData(FALSE);

	// enable/disable the controls
	enableDisplay();

	BOOL ret = CPropertyPage::OnSetActive();
	
	// send a custom message to switch the focus to the radio button
	// this allows the tab key to move between controls on the dialog page
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return ret;
}

void CCICS::OnSelchangeCihFunction() 

{
	CString		tempVar;

	// get the form data into the instance variables
	UpdateData(TRUE);

	tempVar = m_CIH_function;

	// update the form data from the instance variables
	UpdateData(FALSE);

	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_FUNCTION);
}

void CCICS::OnSelendokCihFunction() 

{
	CString		tempVar;

	// get the form data into the instance variables
	UpdateData(TRUE);

	tempVar = m_CIH_function;

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void CCICS::OnCihAdsMsgFormat() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_ADS_MSG_FORMAT);
}

void CCICS::OnCihAdsReceive() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_CIH_ADS_RECEIVE);
}

void CCICS::saveFacility()

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	saveCICSfacility = m_CIH_facility;

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void CCICS::restoreFacility()

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	m_CIH_facility = saveCICSfacility;

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void CCICS::setNextFormat(char * nextFormat, int * charFormat, int * encoding)

{
	strcpy(nextFormat, (LPCTSTR)m_CIH_format);
	// since the ccsid and encoding of the CICS header is defined as ignored,
	// do not update the character format and encoding values
//	(*charFormat) = getCcsidType(atoi((LPCTSTR)m_CIH_codepage));
//	(*encoding) = m_CIH_int_encode;
}

// routine to return the current setting of the format field

void CCICS::getFormat(char * mqformat)

{
	// make sure we have current info
	UpdateData(TRUE);

	// routine to return the current setting of the MQ format field
	memset(mqformat, ' ', MQ_FORMAT_LENGTH + 1);
	if (m_CIH_format.GetLength() > 0)
	{
		memcpy(mqformat, (LPCTSTR)m_CIH_format, m_CIH_format.GetLength());
	}

	// terminate the string
	mqformat[MQ_FORMAT_LENGTH] = 0;
}

int CCICS::getCcsid()

{
	// make sure we have current info
	UpdateData(TRUE);

	return atoi((LPCTSTR)m_CIH_codepage);
}

int CCICS::getEncoding()

{
	// make sure we have current info
	UpdateData(TRUE);

	return m_CIH_int_encode;
}

int CCICS::getPdEncoding()

{
	// make sure we have current info
	UpdateData(TRUE);

	return m_CIH_pd_encode;
}

int CCICS::getFloatEncoding()

{
	// make sure we have current info
	UpdateData(TRUE);

	return m_CIH_float_encode;
}

void CCICS::updateFields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::updateFields() m_CIH_format=%s m_CIH_codepage=%s m_CIH_int_encode=%d m_CIH_pd_encode=%d m_CIH_float_encode=%d", (LPCTSTR)m_CIH_format, (LPCTSTR)m_CIH_codepage, m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode);

		// trace entry to updateFields
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure we have current info from the dialog
	UpdateData(TRUE);

	m_CIH_format = newFormat;
	m_CIH_codepage.Format("%d", newCcsid);
	m_CIH_int_encode = newEncoding;
	m_CIH_pd_encode = newPdEncoding;
	m_CIH_float_encode = newFloatEncoding;

	// release any saved area since the header has changed
	freeCurrentHeader(12);

	// update the dialog controls with the new values
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCICS::updateFields() m_CIH_format=%s m_CIH_codepage=%s m_CIH_int_encode=%d m_CIH_pd_encode=%d m_CIH_float_encode=%d", (LPCTSTR)m_CIH_format, (LPCTSTR)m_CIH_codepage, m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode);

		// trace exit from updateFields
		pDoc->logTraceEntry(traceInfo);
	}
}

void CCICS::UpdatePageData()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to UpdatePageData
		pDoc->logTraceEntry("Entering CCICS::UpdatePageData()");
	}

	// remember the previous CIH version
	m_save_CIH_version = m_CIH_version;

	// update the form data from the instance variables
	UpdateData(FALSE);

	// enable/disable the controls
	enableDisplay();
}

void CCICS::OnCihInsertHeader() 

{
	if (pDoc->traceEnabled)
	{
		// trace entry to OnCihRemoveHeader
		pDoc->logTraceEntry("Entering CCICS::OnCihInsertHeader()");
	}

	// make sure we have current info
	UpdateData(TRUE);

	// the cics header was added
	pDoc->insertMQheader(MQHEADER_CICS, atoi((LPCTSTR)m_CIH_codepage), m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode);

	// update the dialog controls
	UpdateData(FALSE);
}

void CCICS::OnCihRemoveHeader() 

{
	if (pDoc->traceEnabled)
	{
		// trace entry to OnCihRemoveHeader
		pDoc->logTraceEntry("Entering CCICS::OnCihRemoveHeader()");
	}

	// make sure we have current info
	UpdateData(TRUE);

	// the header was removed
	this->m_CIH_version = CIH_NONE;
	pDoc->removeMQheader(MQHEADER_CICS, (LPCTSTR)m_CIH_format, atoi((LPCTSTR)m_CIH_codepage), m_CIH_int_encode, m_CIH_pd_encode, m_CIH_float_encode);

	// update the dialog controls
	UpdateData(FALSE);
}


void CCICS::enableDisplay()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCICS::enableDisplay() m_CIH_version=%d", m_CIH_version);

		// trace entry to enableDisplay
		pDoc->logTraceEntry(traceInfo);
	}

	// figure out if a CICS header has been selected
	if (CIH_NONE == m_CIH_version)
	{
		// no header - disable the controls
		((CButton *)GetDlgItem(IDC_CIH_PD_ENCODE_PC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_PD_ENCODE_HOST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_INT_ENCODE_PC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_INT_ENCODE_HOST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_FLOAT_ENCODE_PC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_FLOAT_ENCODE_HOST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_FLOAT_ENCODE_390))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_FLOAT_ENCODE_TNS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_ONLY))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_CONT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_FIRST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_MIDDLE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_LAST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_COMMIT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_ROLLBACK))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_SC_NONE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_SC_START))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_SC_DATA))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_SC_TERM))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_END_NOSYNC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_END_COMMIT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_END_BACKOUT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_END_ENDTASK))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_CONVERSATIONAL))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_LINK_PROG))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_LINK_TRAN))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_ADS_SEND))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_ADS_RECEIVE))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CIH_ADS_MSG_FORMAT))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_FACILITY))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_FORMAT))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_PROGRAM))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_NEXT_TRANID))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_RETURN_CODE))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_COMP_CODE))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_REASON))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_CANCEL_CODE))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_CURSOR_POS))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_ERROR_OFFSET))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_FACILITY_LIKE))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_AID))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_TRANS_ID))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_REMOTE_SYS))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_REMOTE_TRAN))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_REPLY_FORMAT))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_CODEPAGE))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_FLAGS))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_WAIT_INTERVAL))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_DATA_LENGTH))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_ABEND_CODE))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_AUTHENTICATOR))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_KEEP_TIME))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_INPUT_ITEM))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_CIH_FUNCTION))->EnableWindow(FALSE);
	}
	else
	{
		// have a header - enable the controls
		((CButton *)GetDlgItem(IDC_CIH_PD_ENCODE_PC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_PD_ENCODE_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_INT_ENCODE_PC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_INT_ENCODE_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_FLOAT_ENCODE_PC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_FLOAT_ENCODE_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_FLOAT_ENCODE_390))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_FLOAT_ENCODE_TNS))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_ONLY))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_CONT))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_FIRST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_MIDDLE))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_LAST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_COMMIT))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_UOW_ROLLBACK))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_SC_NONE))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_SC_START))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_SC_DATA))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_SC_TERM))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_END_NOSYNC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_END_COMMIT))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_END_BACKOUT))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_END_ENDTASK))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_CONVERSATIONAL))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_LINK_PROG))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_LINK_TRAN))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_ADS_SEND))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_ADS_RECEIVE))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CIH_ADS_MSG_FORMAT))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_FACILITY))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_FORMAT))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_PROGRAM))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_NEXT_TRANID))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_RETURN_CODE))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_COMP_CODE))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_REASON))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_CANCEL_CODE))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_CURSOR_POS))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_ERROR_OFFSET))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_FACILITY_LIKE))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_AID))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_TRANS_ID))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_REMOTE_SYS))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_REMOTE_TRAN))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_REPLY_FORMAT))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_CODEPAGE))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_FLAGS))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_WAIT_INTERVAL))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_DATA_LENGTH))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_ABEND_CODE))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_AUTHENTICATOR))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_KEEP_TIME))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_INPUT_ITEM))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_CIH_FUNCTION))->EnableWindow(TRUE);
	}
}

LONG CCICS::OnSetPageFocus(UINT wParam, LONG lParam)

{
	if (pDoc->traceEnabled)
	{
		// trace entry to OnSetPageFocus
		pDoc->logTraceEntry("Entering CCICS::OnSetPageFocus()");
	}

	// set the focus to the CICS radio button
	switch (m_CIH_version)
	{
	case CIH_NONE:
		{
			((CButton *)GetDlgItem(IDC_CIH_NONE))->SetFocus();
			break;
		}
	case CIH_V1:
		{
			((CButton *)GetDlgItem(IDC_CIH_V1))->SetFocus();
			break;
		}
	case CIH_V2:
		{
			((CButton *)GetDlgItem(IDC_CIH_V2))->SetFocus();
			break;
		}
	}

	return 0;
}

int CCICS::conv2EBCDIC(char *data, int len)

{
	int		count=0;
	int		asciiCcsid=0;

	// get the appropriate ASCII code page
//	asciiCcsid = matchEbcdicToAscii(fileCcsid);

	// return the number of characters that were translated
	return count;
}
