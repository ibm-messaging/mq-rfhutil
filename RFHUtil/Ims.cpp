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

// Ims.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "Ims.h"
#include "comsubs.h"
#include "mqsubs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+8

/////////////////////////////////////////////////////////////////////////////
// CIms property page

IMPLEMENT_DYNCREATE(CIms, CPropertyPage)

CIms::CIms() : CPropertyPage(CIms::IDD)
{
	//{{AFX_DATA_INIT(CIms)
	m_IIH_codepage = _T("");
	m_IIH_authenticator = _T("");
	m_IIH_commit_mode = -1;
	m_IIH_format = _T("");
	m_IIH_encode_int = -1;
	m_IIH_lterm = _T("");
	m_IIH_map_name = _T("");
	m_IIH_trans_state = -1;
	m_IIH_encode_pd = -1;
	m_IIH_reply_format = _T("");
	m_IIH_security_scope = -1;
	m_IIH_trans_id = _T("");
	m_IIH_include_header = FALSE;
	m_IIH_no_reply_format = FALSE;
	m_IIH_pass_expiration = FALSE;
	m_IIH_add_length = FALSE;
	m_IIH_encode_float = -1;
	//}}AFX_DATA_INIT

	imsHeader = NULL;
	imsLength = 0;
	imsCcsid = -1;
	imsEncodeType = -1;
	pDoc = NULL;
	m_IIH_encode_int = NUMERIC_PC;
	m_IIH_encode_pd = NUMERIC_PC;
	m_IIH_commit_mode = IMS_COMMIT_FIRST;
	m_IIH_security_scope = IMS_SEC_CHECK;
	m_IIH_trans_state = IMS_NO_CONV;
	m_save_include_header = FALSE;
}

CIms::~CIms()
{
	freeCurrentHeader(0);
}

void CIms::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIms)
	DDX_Text(pDX, IDC_IIH_CODEPAGE, m_IIH_codepage);
	DDV_MaxChars(pDX, m_IIH_codepage, 5);
	DDX_Text(pDX, IDC_IIH_AUTHENTICATOR, m_IIH_authenticator);
	DDV_MaxChars(pDX, m_IIH_authenticator, 8);
	DDX_Radio(pDX, IDC_IIH_COMMIT_FIRST, m_IIH_commit_mode);
	DDX_Text(pDX, IDC_IIH_FORMAT, m_IIH_format);
	DDV_MaxChars(pDX, m_IIH_format, 8);
	DDX_Radio(pDX, IDC_IIH_INT_ENCODE_PC, m_IIH_encode_int);
	DDX_Text(pDX, IDC_IIH_LTERM, m_IIH_lterm);
	DDV_MaxChars(pDX, m_IIH_lterm, 8);
	DDX_Text(pDX, IDC_IIH_MAP_NAME, m_IIH_map_name);
	DDV_MaxChars(pDX, m_IIH_map_name, 8);
	DDX_Radio(pDX, IDC_IIH_NO_CONVERSATION, m_IIH_trans_state);
	DDX_Radio(pDX, IDC_IIH_PD_ENCODE_PC, m_IIH_encode_pd);
	DDX_Text(pDX, IDC_IIH_REPLY_FORMAT, m_IIH_reply_format);
	DDV_MaxChars(pDX, m_IIH_reply_format, 8);
	DDX_Radio(pDX, IDC_IIH_SEC_CHECK, m_IIH_security_scope);
	DDX_Text(pDX, IDC_IIH_TRANS_ID, m_IIH_trans_id);
	DDV_MaxChars(pDX, m_IIH_trans_id, 32);
	DDX_Check(pDX, IDC_IIH_INCLUDE_HEADER, m_IIH_include_header);
	DDX_Check(pDX, IDC_IIH_NO_REPLY_FORMAT, m_IIH_no_reply_format);
	DDX_Check(pDX, IDC_IIH_PASS_EXPIRATION, m_IIH_pass_expiration);
	DDX_Check(pDX, IDC_IIH_ADD_LENGTH, m_IIH_add_length);
	DDX_Radio(pDX, IDC_IIH_FLOAT_ENCODE_PC, m_IIH_encode_float);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIms, CPropertyPage)
	//{{AFX_MSG_MAP(CIms)
	ON_BN_CLICKED(IDC_IIH_NO_CONVERSATION, OnIihNoConversation)
	ON_BN_CLICKED(IDC_IIH_NO_REPLY_FORMAT, OnIihNoReplyFormat)
	ON_BN_CLICKED(IDC_IIH_PASS_EXPIRATION, OnIihPassExpiration)
	ON_BN_CLICKED(IDC_IIH_PD_ENCODE_HOST, OnIihPdEncodeHost)
	ON_BN_CLICKED(IDC_IIH_PD_ENCODE_PC, OnIihPdEncodePc)
	ON_EN_CHANGE(IDC_IIH_REPLY_FORMAT, OnChangeIihReplyFormat)
	ON_BN_CLICKED(IDC_IIH_SEC_CHECK, OnIihSecCheck)
	ON_BN_CLICKED(IDC_IIH_SEC_FULL, OnIihSecFull)
	ON_BN_CLICKED(IDC_IIH_SEND_FIRST, OnIihSendFirst)
	ON_EN_CHANGE(IDC_IIH_TRANS_ID, OnChangeIihTransId)
	ON_BN_CLICKED(IDC_IIH_ARCHITECTED, OnIihArchitected)
	ON_EN_CHANGE(IDC_IIH_AUTHENTICATOR, OnChangeIihAuthenticator)
	ON_BN_CLICKED(IDC_IIH_COMMIT_FIRST, OnIihCommitFirst)
	ON_EN_CHANGE(IDC_IIH_FORMAT, OnChangeIihFormat)
	ON_BN_CLICKED(IDC_IIH_IN_CONVERSATION, OnIihInConversation)
	ON_BN_CLICKED(IDC_IIH_INCLUDE_HEADER, OnIihIncludeHeader)
	ON_BN_CLICKED(IDC_IIH_INT_ENCODE_HOST, OnIihIntEncodeHost)
	ON_BN_CLICKED(IDC_IIH_INT_ENCODE_PC, OnIihIntEncodePc)
	ON_EN_CHANGE(IDC_IIH_LTERM, OnChangeIihLterm)
	ON_EN_CHANGE(IDC_IIH_MAP_NAME, OnChangeIihMapName)
	ON_EN_CHANGE(IDC_IIH_CODEPAGE, OnChangeIihCodepage)
	ON_BN_CLICKED(IDC_IIH_ADD_LENGTH, OnIihAddLength)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CIms::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CIms::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIms message handlers

BOOL CIms::PreTranslateMessage(MSG *pMsg)

{
	// Necessary to allow the tab key to be used to move between fields
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
			if ((IDC_IIH_CODEPAGE == id) || 
				(IDC_IIH_AUTHENTICATOR == id) || 
				(IDC_IIH_FORMAT == id) || 
				(IDC_IIH_LTERM == id) || 
				(IDC_IIH_MAP_NAME == id) || 
				(IDC_IIH_REPLY_FORMAT == id) || 
				(IDC_IIH_TRANS_ID == id))
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

BOOL CIms::OnInitDialog() 

{
	CPropertyPage::OnInitDialog();
	
	// enable tool tips for this dialog	
	EnableToolTips(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CIms::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

void CIms::freeCurrentHeader(const int callee)

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CIms::freeCurrentHeader() callee=%d imsHeader=%8.8X imsLength=%d imsCcsid=%d imsEncodeType=%d", callee, (unsigned int)imsHeader, imsLength, imsCcsid, imsEncodeType);

		// trace entry to freeCurrentHeader
		pDoc->logTraceEntry(traceInfo);
	}

	// free the current area
	if (imsHeader != NULL)
	{
		rfhFree(imsHeader);
	}

	// reset the pointer, length, character set and encoding fields
	imsHeader = NULL;
	imsLength = 0;
	imsCcsid = -1;
	imsEncodeType = -1;
}

void CIms::createHeader(int ccsid, int encodeType)

{
	int				charType;
	MQIIH			iih={MQIIH_DEFAULT};
	char			traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CIms::createHeader() ccsid=%d encodeType=%d", ccsid, encodeType);

		// trace entry to createHeader
		pDoc->logTraceEntry(traceInfo);
	}


	// is there a header?
	if (!m_IIH_include_header)
	{
		// no IMS header for this message
		return;
	}

	// get rid of any existing header
	freeCurrentHeader(10);

	// get the form data into the instance variables
	UpdateData(TRUE);
	
	if (m_IIH_codepage.GetLength() > 0)
	{
		iih.CodedCharSetId = atoi((LPCTSTR)m_IIH_codepage);
	}

	iih.Encoding = getEncodingValue(m_IIH_encode_int, m_IIH_encode_pd, m_IIH_encode_float);

	// set character fields
	setCharData(iih.Format, sizeof(iih.Format), (LPCTSTR)m_IIH_format, m_IIH_format.GetLength());
	setCharData(iih.LTermOverride, sizeof(iih.LTermOverride), (LPCTSTR)m_IIH_lterm, m_IIH_lterm.GetLength());
	setCharData(iih.MFSMapName, sizeof(iih.MFSMapName), (LPCTSTR)m_IIH_map_name, m_IIH_map_name.GetLength());
	setCharData(iih.ReplyToFormat, sizeof(iih.ReplyToFormat), (LPCTSTR)m_IIH_reply_format, m_IIH_reply_format.GetLength());
	setCharData(iih.Authenticator, sizeof(iih.Authenticator), (LPCTSTR)m_IIH_authenticator, m_IIH_authenticator.GetLength());

	if ((m_IIH_trans_id.GetLength() > 0) && (m_IIH_trans_id.GetLength() <= (2 * sizeof(iih.TranInstanceId))))
	{
		// set the transaction instance id
		HexToAscii((unsigned char *)(LPCTSTR)m_IIH_trans_id, m_IIH_trans_id.GetLength() / 2, iih.TranInstanceId);
	}

	switch (m_IIH_trans_state)
	{
	case IMS_NO_CONV:
		{
			iih.TranState = MQITS_NOT_IN_CONVERSATION;
			break;
		}
	case IMS_IN_CONV:
		{
			iih.TranState = MQITS_IN_CONVERSATION;
			break;
		}
	case IMS_ARCHITECT:
		{
			iih.TranState = MQITS_ARCHITECTED;
			break;
		}
	default:
		{
			break;
		}
	}

	if (IMS_SEND_FIRST == m_IIH_commit_mode)
	{
		iih.CommitMode = MQICM_SEND_THEN_COMMIT;
	}

	if (IMS_SEC_FULL == m_IIH_security_scope)
	{
		iih.SecurityScope = MQISS_FULL;
	}

	// check for flags
	if (m_IIH_pass_expiration)
	{
		iih.Flags |= MQIIH_PASS_EXPIRATION;
	}

	if (m_IIH_no_reply_format)
	{
		iih.Flags |= MQIIH_REPLY_FORMAT_NONE;
	}

	imsLength = MQIIH_LENGTH_1;

	// figure out what type of data we are dealing with
	charType = getCcsidType(ccsid);

	// check the code page of the header
	if (CHAR_EBCDIC == charType)
	{
		// translate the charater data to EBCDIC
		convertEbcdic(iih.StrucId, sizeof(iih.StrucId));
		convertEbcdic(iih.Format, sizeof(iih.Format));
		convertEbcdic(iih.LTermOverride, sizeof(iih.LTermOverride));
		convertEbcdic(iih.MFSMapName, sizeof(iih.MFSMapName));
		convertEbcdic(iih.ReplyToFormat, sizeof(iih.ReplyToFormat));
		convertEbcdic(iih.Authenticator, sizeof(iih.Authenticator));
		convertEbcdic(&iih.TranState, sizeof(iih.TranState));
		convertEbcdic(&iih.CommitMode, sizeof(iih.CommitMode));
		convertEbcdic(&iih.SecurityScope, sizeof(iih.SecurityScope));
		convertEbcdic(&iih.Reserved, sizeof(iih.Reserved));
	}

	// check the encoding of the data
	if (encodeType != NUMERIC_PC)
	{
		// change to PC encoding
		iih.Version = reverseBytes4(iih.Version);
		iih.StrucLength = reverseBytes4(iih.StrucLength);
		iih.Encoding = reverseBytes4(iih.Encoding);
		iih.CodedCharSetId = reverseBytes4(iih.CodedCharSetId);
		iih.Flags = reverseBytes4(iih.Flags);
	}

	// update the form from the instance variables
	UpdateData(FALSE);

	// allocate a buffer
	imsHeader = (unsigned char *)rfhMalloc(imsLength, "IMSHDR  ");
	memcpy(imsHeader, &iih, imsLength);

	// remember the ccsid and encoding of this header
	imsCcsid = ccsid;
	imsEncodeType = encodeType;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CIms::createHeader() imsLength=%d imsCcsid=%d imsEncodeType=%d", imsLength, imsCcsid, imsEncodeType);

		// trace exit from createHeader
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("IMS header", imsHeader, imsLength);
	}
}

int CIms::parseIMSheader(unsigned char * imsData, int dataLen, int ccsid, int encodeType)

{
	int				dataType;
	unsigned char	tempArea[64] = { 0 };
	MQIIH			iih;
	char			traceInfo[128];		// work variable to build trace message

	if (dataLen < MQIIH_LENGTH_1)
	{
		// not enough data to be a IIH
		return 0;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CIms::parseIMSheader() imsData=%8.8X dataLen=%d ccsid=%d encodeType=%d", (unsigned int)imsData, dataLen, ccsid, encodeType);

		// trace entry to parseIMSheader
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("imsData", imsData, MQIIH_LENGTH_1);
	}

	// copy the data into the temporary work area
	memcpy(&iih, imsData, MQIIH_LENGTH_1);

	// figure out what type of data this is
	dataType = getCcsidType(ccsid);

	// check the code page of the header
	if (CHAR_EBCDIC == dataType)
	{
		// translate the charater data to ascii
		EbcdicToAscii((unsigned char *)&iih.StrucId, sizeof(iih.StrucId), tempArea);
		memcpy(iih.StrucId, tempArea, sizeof(iih.StrucId));
		EbcdicToAscii((unsigned char *)&iih.Format, sizeof(iih.Format), tempArea);
		memcpy(iih.Format, tempArea, sizeof(iih.Format));
		EbcdicToAscii((unsigned char *)&iih.LTermOverride, sizeof(iih.LTermOverride), tempArea);
		memcpy(iih.LTermOverride, tempArea, sizeof(iih.LTermOverride));
		EbcdicToAscii((unsigned char *)&iih.MFSMapName, sizeof(iih.MFSMapName), tempArea);
		memcpy(iih.MFSMapName, tempArea, sizeof(iih.MFSMapName));
		EbcdicToAscii((unsigned char *)&iih.ReplyToFormat, sizeof(iih.ReplyToFormat), tempArea);
		memcpy(iih.ReplyToFormat, tempArea, sizeof(iih.ReplyToFormat));
		EbcdicToAscii((unsigned char *)&iih.Authenticator, sizeof(iih.Authenticator), tempArea);
		memcpy(iih.Authenticator, tempArea, sizeof(iih.Authenticator));
		iih.TranState = eatab[(unsigned char)iih.TranState];
		iih.CommitMode = eatab[(unsigned char)iih.CommitMode];
		iih.SecurityScope = eatab[(unsigned char)iih.SecurityScope];
		iih.Reserved = eatab[(unsigned char)iih.Reserved];
	}

	// check the encoding of the data
	if (encodeType != NUMERIC_PC)
	{
		// change to PC encoding
		iih.Version = reverseBytes4(iih.Version);
		iih.StrucLength = reverseBytes4(iih.StrucLength);
		iih.Encoding = reverseBytes4(iih.Encoding);
		iih.CodedCharSetId = reverseBytes4(iih.CodedCharSetId);
		iih.Flags = reverseBytes4(iih.Flags);
	}

	// check if we have a IMS header
	if ((memcmp(&iih.StrucId, MQIIH_STRUC_ID, sizeof(iih.StrucId)) != 0) || (iih.Version != MQIIH_VERSION_1))
	{
		return 0;
	}

	// capture the character fields
	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, iih.Format, sizeof(iih.Format));
	m_IIH_format = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, iih.LTermOverride, sizeof(iih.LTermOverride));
	m_IIH_lterm = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, iih.MFSMapName, sizeof(iih.MFSMapName));
	m_IIH_map_name = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, iih.ReplyToFormat, sizeof(iih.ReplyToFormat));
	m_IIH_reply_format = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, iih.Authenticator, sizeof(iih.Authenticator));
	m_IIH_authenticator = tempArea;

	// capture the transaction instance id field in hex
	memset(tempArea, 0, sizeof(tempArea));
	AsciiToHex(iih.TranInstanceId, sizeof(iih.TranInstanceId), tempArea);
	m_IIH_trans_id = tempArea;

	// get the encoding value
	m_IIH_encode_int = getIntEncode(iih.Encoding);
	m_IIH_encode_pd = getPDEncode(iih.Encoding);
	m_IIH_encode_float = getFloatEncode(iih.Encoding);

	// get the integer fields
	m_IIH_codepage.Format("%d", iih.CodedCharSetId);

	// capture the transaction state
	switch (iih.TranState)
	{
	case MQITS_NOT_IN_CONVERSATION:
		{
			m_IIH_trans_state = IMS_NO_CONV;
			break;
		}
	case MQITS_IN_CONVERSATION:
		{
			m_IIH_trans_state = IMS_IN_CONV;
			break;
		}
	case MQITS_ARCHITECTED:
		{
			m_IIH_trans_state = IMS_ARCHITECT;
			break;
		}
	default:
		{
			m_IIH_trans_state = -1;
			break;
		}
	}

	// capture the commit mode
	switch (iih.CommitMode)
	{
	case MQICM_COMMIT_THEN_SEND:
		{
			m_IIH_commit_mode = IMS_COMMIT_FIRST;
			break;
		}
	case MQICM_SEND_THEN_COMMIT:
		{
			m_IIH_commit_mode = IMS_SEND_FIRST;
			break;
		}
	default:
		{
			m_IIH_commit_mode = -1;
			break;
		}
	}

	// capture the security scope
	switch (iih.SecurityScope)
	{
	case MQISS_CHECK:
		{
			m_IIH_security_scope = IMS_SEC_CHECK;
			break;
		}
	case MQISS_FULL:
		{
			m_IIH_security_scope = IMS_SEC_FULL;
			break;
		}
	default:
		{
			m_IIH_security_scope = -1;
			break;
		}
	}

	// capture the flags
	m_IIH_pass_expiration = (iih.Flags & MQIIH_PASS_EXPIRATION) > 0;
	m_IIH_no_reply_format = (iih.Flags & MQIIH_REPLY_FORMAT_NONE) > 0;

	// indicate we have an IMS header
	m_IIH_include_header = TRUE;

	// this option is for output and must be selected manually
	m_IIH_add_length = FALSE;

	// save the current IMS header
	imsHeader = (unsigned char *)rfhMalloc(MQIIH_LENGTH_1 + 1, "IMSHDR2 ");
	memset(imsHeader, 0, MQIIH_LENGTH_1 + 1);
	memcpy(imsHeader, imsData, MQIIH_LENGTH_1);
	imsLength = MQIIH_LENGTH_1;

	// remember the code page and encoding
	imsCcsid = ccsid;
	imsEncodeType = encodeType;

	// Update the data in the controls
	UpdateData (FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CIms::parseIMSheader() imsLength=%d", imsLength);

		// trace exit from parseIMSheader
		pDoc->logTraceEntry(traceInfo);
	}

	return MQIIH_LENGTH_1;
}

int CIms::getIMSlength()

{
	int		len=0;
	char	traceInfo[128];		// work variable to build trace message

	if (m_IIH_include_header)
	{
		len = MQIIH_LENGTH_1;

		if (m_IIH_add_length)
		{
			len += 4;
		}
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CIms::getIMSlength() len=%d", len);

		// trace exit from getIMSlength
		pDoc->logTraceEntry(traceInfo);
	}

	return len;
}

int CIms::getCcsid()

{
	// make sure we have current info
	UpdateData(TRUE);

	return atoi((LPCTSTR)m_IIH_codepage);
}

int CIms::getEncoding()

{
	// make sure we have current info
	UpdateData(TRUE);

	return m_IIH_encode_int;
}

int CIms::getPdEncoding()

{
	// make sure we have current info
	UpdateData(TRUE);

	return m_IIH_encode_pd;
}

int CIms::getFloatEncoding()

{
	// make sure we have current info
	UpdateData(TRUE);

	return m_IIH_encode_float;
}

void CIms::getFormat(char *mqformat)

{
	memset(mqformat, ' ', MQ_FORMAT_LENGTH + 1);
	if (m_IIH_format.GetLength() > 0)
	{
		memcpy(mqformat, (LPCTSTR)m_IIH_format, m_IIH_format.GetLength());
	}

	// terminate the string
	mqformat[MQ_FORMAT_LENGTH] = 0;
}

BOOL CIms::OnKillActive() 

{
	// The dialog is about to lose the focus
	// make sure the instance variables are up to date
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering CIms::OnKillActive()");
	}

	// get the current data from the form variables
	UpdateData (TRUE);

	return CPropertyPage::OnKillActive();
}

BOOL CIms::OnSetActive() 

{
	// The dialog is about to receive the focus
	if (pDoc->traceEnabled)
	{
		// trace entry to OnSetActive
		pDoc->logTraceEntry("Entering CIms::OnSetActive()");
	}

	// make sure the dialog display reflects the current instance variables
	// update the form variables
	UpdateData(FALSE);

	// enable the appropriate fields
	enableDisplay();

	// Does this message include an IMS header?
	if (!m_IIH_include_header)
	{
		// no IMS header - set the focus to the include check box
		((CButton *)GetDlgItem(IDC_IIH_INCLUDE_HEADER))->SetFocus();
	}

	BOOL ret = CPropertyPage::OnSetActive();
	
	// send a custom message to switch the focus to the QM combo box
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return ret;
}

void CIms::setNextFormat(char * nextFormat, int *charFormat, int *encoding)

{
	strcpy(nextFormat, (LPCTSTR)m_IIH_format);
	// since the ccsid and encoding fields are defined as ignored,
	// do not change the current character format or encoding 
//	(*charFormat) = getCcsidType(atoi((LPCTSTR)m_IIH_codepage));
//	(*encoding) = m_IIH_encode_int;
}

void CIms::updateFields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CIms::updateFields() m_IIH_format=%s m_IIH_codepage=%s m_IIH_encode_int=%d m_IIH_encode_pd=%d m_IIH_encode_float=%d", (LPCTSTR)m_IIH_format, (LPCTSTR)m_IIH_codepage, m_IIH_encode_int, m_IIH_encode_pd, m_IIH_encode_float);

		// trace entry to updateFields
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure we have current info
	UpdateData(TRUE);

	m_IIH_format = newFormat;
	m_IIH_codepage.Format("%d", newCcsid);
	m_IIH_encode_int = newEncoding;
	m_IIH_encode_pd = newPdEncoding;
	m_IIH_encode_float = newFloatEncoding;

	// release any saved area since the header has changed
	freeCurrentHeader(11);

	// update the dialog controls
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CIms::updateFields() m_IIH_format=%s m_IIH_codepage=%s m_IIH_encode_int=%d m_IIH_encode_pd=%d m_IIH_encode_float=%d", (LPCTSTR)m_IIH_format, (LPCTSTR)m_IIH_codepage, m_IIH_encode_int, m_IIH_encode_pd, m_IIH_encode_float);

		// trace exit from updateFields
		pDoc->logTraceEntry(traceInfo);
	}
}

////////////////////////////////////////////////////
//
// This routine is called if the data is modified
// while this page is being displayed
//
////////////////////////////////////////////////////

void CIms::UpdatePageData()

{
	// update the form data from the instance variables
	if (pDoc->traceEnabled)
	{
		// trace entry to UpdatePageData
		pDoc->logTraceEntry("Entering CIms::UpdatePageData()");
	}

	// update the dialog controls
	UpdateData(FALSE);

	// enable the appropriate fields
	enableDisplay();
}

void CIms::clearIMSheader()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to clearIMSheader
		pDoc->logTraceEntry("Entering CIms::clearIMSheader()");
	}

	// initialize all variables
	m_IIH_include_header = FALSE;
	m_IIH_add_length = FALSE;
	m_IIH_authenticator.Empty();
	m_IIH_codepage.Empty();
	m_IIH_format.Empty();
	m_IIH_lterm.Empty();
	m_IIH_map_name.Empty();
	m_IIH_reply_format.Empty();
	m_IIH_trans_id.Empty();
	m_IIH_commit_mode = IMS_COMMIT_FIRST;
	m_IIH_encode_int = NUMERIC_PC;
	m_IIH_encode_pd = NUMERIC_PC;
	m_IIH_encode_float = FLOAT_PC;
	m_IIH_no_reply_format = FALSE;
	m_IIH_pass_expiration = FALSE;
	m_IIH_security_scope = IMS_SEC_CHECK;
	m_IIH_trans_state = IMS_NO_CONV;

	// Update the data in the controls
	UpdateData (FALSE);
}

int CIms::buildIMSheader(unsigned char * header, int ccsid, int encodeType)

{
	short	len;
	int		hdrLen=imsLength;
	char	traceInfo[256];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CIms::buildIMSheader() ccsid=%d encodeType=%d m_IIH_include_header=%d", ccsid, encodeType, m_IIH_include_header);

		// trace entry to buildIMSheader
		pDoc->logTraceEntry(traceInfo);
	}

	// is there a header?
	if (!m_IIH_include_header)
	{
		return 0;
	}

	// check if the header exists and is of the correct character type and encoding
	if ((imsHeader != NULL) && ((imsCcsid != ccsid) || (imsEncodeType != encodeType)))
	{
		// release the storage held by the current header
		freeCurrentHeader(12);
	}

	if (NULL == imsHeader)
	{
		createHeader(ccsid, encodeType);
	}

	if (imsHeader != NULL) {
		memcpy(header, imsHeader, imsLength);
	}

	// check if we are supposed to add an LLBB field after the header
	if (m_IIH_add_length)
	{
		// get the length of the data area, including the LLBB field
		len = pDoc->fileSize + 4;

		// check if we need to reverse the bytes
		if (encodeType != NUMERIC_PC)
		{
			len = reverseBytes(&len);
		}

		// insert the length bytes
		memset(header + imsLength, 0, 4);			// clear the LLBB field
		memcpy(header + imsLength, &len, 2);		// copy in the LL field

		// add the 4 bytes to the header length
		hdrLen += 4;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CIms::buildIMSheader() hdrLen=%d m_IIH_add_length=%d ccsid=%d encodeType=%d", hdrLen, m_IIH_add_length, ccsid, encodeType);

		// trace exit from buildIMSheader
		pDoc->logTraceEntry(traceInfo);
	}

	return hdrLen;
}

void CIms::OnIihNoConversation() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_NO_CONVERSATION);	
}

void CIms::OnIihNoReplyFormat() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_NO_REPLY_FORMAT);	
}

void CIms::OnIihPassExpiration() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_PASS_EXPIRATION);	
}

void CIms::OnIihPdEncodeHost() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_PD_ENCODE_HOST);	
}

void CIms::OnIihPdEncodePc() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_PD_ENCODE_PC);	
}

void CIms::OnChangeIihReplyFormat() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_REPLY_FORMAT);	
}

void CIms::OnIihSecCheck() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_SEC_CHECK);	
}

void CIms::OnIihSecFull() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_SEC_FULL);	
}

void CIms::OnIihSendFirst() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_SEND_FIRST);	
}

void CIms::OnChangeIihTransId()
 
{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_TRANS_ID);	
}

void CIms::OnIihArchitected()
 
{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_ARCHITECTED);	
}

void CIms::OnChangeIihAuthenticator() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_AUTHENTICATOR);	
}

void CIms::OnIihCommitFirst() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_COMMIT_FIRST);	
}

void CIms::OnChangeIihFormat() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_FORMAT);	
}

void CIms::OnIihInConversation() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_IN_CONVERSATION);	
}

void CIms::OnIihIncludeHeader() 

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CIms::OnIihIncludeHeader() m_IIH_include_header=%d m_IIH_format=%s m_IIH_codepage=%s m_IIH_encode_int=%d m_IIH_encode_pd=%d m_IIH_encode_float=%d", m_IIH_include_header, (LPCTSTR)m_IIH_format, (LPCTSTR)m_IIH_codepage, m_IIH_encode_int, m_IIH_encode_pd, m_IIH_encode_float);

		// trace entry to OnIihIncludeHeader
		pDoc->logTraceEntry(traceInfo);
	}

	// get the form data into the instance variables to be sure we have the current values
	UpdateData(TRUE);

	// figure out what we just did
	if (m_IIH_include_header)
	{
		// we have a header - must have just added it
		pDoc->insertMQheader(MQHEADER_IMS, atoi((LPCTSTR)m_IIH_codepage), m_IIH_encode_int, m_IIH_encode_pd, m_IIH_encode_float);
	}
	else
	{
		// no more header - must have removed it
		pDoc->removeMQheader(MQHEADER_IMS, (LPCTSTR)m_IIH_format, atoi((LPCTSTR)m_IIH_codepage), m_IIH_encode_int, m_IIH_encode_pd, m_IIH_encode_float);
	}

	// enable/disable controls to match
	enableDisplay();
}

void CIms::OnIihIntEncodeHost() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	// this change could affect the next header 
	pDoc->freeHeader(m_IIH_format);

	// update the form data from the instance variables
	UpdateData(FALSE);
	
	freeCurrentHeader(IDC_IIH_INT_ENCODE_HOST);	
}

void CIms::OnIihIntEncodePc() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	// this change could affect the next header 
	pDoc->freeHeader(m_IIH_format);

	// update the form data from the instance variables
	UpdateData(FALSE);
	
	freeCurrentHeader(IDC_IIH_INT_ENCODE_PC);	
}

void CIms::OnChangeIihLterm() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_LTERM);	
}

void CIms::OnChangeIihMapName() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_MAP_NAME);	
}

void CIms::OnChangeIihCodepage() 

{
	// User has made a change so the current copy in memory 
	// is no longer valid and must be rebuilt
	freeCurrentHeader(IDC_IIH_CODEPAGE);	
}

////////////////////////////////////////////////
//
// Routine to enable/disable fields depending
// on whether an IMS header is included
//
////////////////////////////////////////////////

void CIms::enableDisplay()

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CIms::enableDisplay() m_IIH_include_header=%d", m_IIH_include_header);

		// trace entry to enableDisplay
		pDoc->logTraceEntry(traceInfo);
	}

	if (m_IIH_include_header)
	{
		((CButton *)GetDlgItem(IDC_IIH_PD_ENCODE_PC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_PD_ENCODE_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_INT_ENCODE_PC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_INT_ENCODE_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_FLOAT_ENCODE_PC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_FLOAT_ENCODE_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_FLOAT_ENCODE_390))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_FLOAT_ENCODE_TNS))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_PD_ENCODE_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_NO_CONVERSATION))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_IN_CONVERSATION))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_ARCHITECTED))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_COMMIT_FIRST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_SEND_FIRST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_SEC_CHECK))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_SEC_FULL))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_PASS_EXPIRATION))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_NO_REPLY_FORMAT))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_IIH_ADD_LENGTH))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_IIH_FORMAT))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_IIH_MAP_NAME))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_IIH_REPLY_FORMAT))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_IIH_LTERM))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_IIH_AUTHENTICATOR))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_IIH_TRANS_ID))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_IIH_CODEPAGE))->EnableWindow(TRUE);
	}
	else
	{
		((CButton *)GetDlgItem(IDC_IIH_PD_ENCODE_PC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_PD_ENCODE_HOST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_INT_ENCODE_PC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_INT_ENCODE_HOST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_FLOAT_ENCODE_PC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_FLOAT_ENCODE_HOST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_FLOAT_ENCODE_390))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_FLOAT_ENCODE_TNS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_NO_CONVERSATION))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_IN_CONVERSATION))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_ARCHITECTED))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_COMMIT_FIRST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_SEND_FIRST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_SEC_CHECK))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_SEC_FULL))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_PASS_EXPIRATION))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_NO_REPLY_FORMAT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_IIH_ADD_LENGTH))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_IIH_FORMAT))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_IIH_MAP_NAME))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_IIH_REPLY_FORMAT))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_IIH_LTERM))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_IIH_AUTHENTICATOR))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_IIH_TRANS_ID))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_IIH_CODEPAGE))->EnableWindow(FALSE);
	}
}

void CIms::OnIihAddLength() 

{
	// This field does not affect the current header
	// It determines if RFHUtil should insert an LLBB field
	// in front of the data or not
	// If the LLBB field is already part of the data then
	// this should not be done.  
	// There is no need to free the current header since 
	// it has not been changed.
}

void CIms::saveTransInstanceId()

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	// save the current transaction id
	saveIMStransId = m_IIH_trans_id;
}

void CIms::restoreTransInstanceId()

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	m_IIH_trans_id = saveIMStransId;

	// update the form data from the instance variables
	UpdateData(FALSE);
}

LONG CIms::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the queue manager name combo box
	((CButton *)GetDlgItem(IDC_IIH_INCLUDE_HEADER))->SetFocus();

	return 0;
}

BOOL CIms::PreCreateWindow(CREATESTRUCT& cs) 
{
	// set the appropriate windows styles for this dialog
	cs.style |= WS_TABSTOP;
	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	
	return CPropertyPage::PreCreateWindow(cs);
}
