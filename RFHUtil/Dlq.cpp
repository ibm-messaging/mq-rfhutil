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

// Dlq.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "rfhutil.h"
#include "Dlq.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "mqsubs.h"
#include "cmqc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+9

/////////////////////////////////////////////////////////////////////////////
// CDlq property page

IMPLEMENT_DYNCREATE(CDlq, CPropertyPage)

CDlq::CDlq() : CPropertyPage(CDlq::IDD)

{
	//{{AFX_DATA_INIT(CDlq)
	m_dlq_codepage = _T("");
	m_dlq_date_time = _T("");
	m_dlq_format = _T("");
	m_dlq_orig_queue = _T("");
	m_dlq_orig_qm = _T("");
	m_dlq_encode = -1;
	m_dlq_pd_encode = -1;
	m_dlq_put_appl_name = _T("");
	m_dlq_put_appl_type = _T("");
	m_dlq_reason = _T("");
	m_dlq_include = FALSE;
	m_dlq_float_encode = -1;
	//}}AFX_DATA_INIT

	pDoc = NULL;
	putApplTypeInit = TRUE;
	dlqHeader = NULL;
	dlqLength = 0;
	dlqCcsid = -1;
	dlqEncodeType = -1;
}

CDlq::~CDlq()

{
	// release any storage we have acquired
	freeCurrentHeader(0);
}

void CDlq::DoDataExchange(CDataExchange* pDX)

{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlq)
	DDX_Text(pDX, IDC_DLQ_CODEPAGE, m_dlq_codepage);
	DDV_MaxChars(pDX, m_dlq_codepage, 5);
	DDX_Text(pDX, IDC_DLQ_DATE_TIME, m_dlq_date_time);
	DDX_Text(pDX, IDC_DLQ_FORMAT, m_dlq_format);
	DDV_MaxChars(pDX, m_dlq_format, 8);
	DDX_Text(pDX, IDC_DLQ_ORIG_Q, m_dlq_orig_queue);
	DDV_MaxChars(pDX, m_dlq_orig_queue, 48);
	DDX_Text(pDX, IDC_DLQ_ORIG_QM, m_dlq_orig_qm);
	DDV_MaxChars(pDX, m_dlq_orig_qm, 48);
	DDX_Radio(pDX, IDC_DLQ_PC, m_dlq_encode);
	DDX_Radio(pDX, IDC_DLQ_PD_PC, m_dlq_pd_encode);
	DDX_Text(pDX, IDC_DLQ_PUT_APPL_NAME, m_dlq_put_appl_name);
	DDV_MaxChars(pDX, m_dlq_put_appl_name, 28);
	DDX_CBString(pDX, IDC_DLQ_PUT_APPL_TYPE, m_dlq_put_appl_type);
	DDX_Text(pDX, IDC_DLQ_REASON, m_dlq_reason);
	DDX_Check(pDX, IDC_DLQ_INCLUDE_HEADER, m_dlq_include);
	DDX_Radio(pDX, IDC_DLQ_FLOAT_PC, m_dlq_float_encode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlq, CPropertyPage)
	//{{AFX_MSG_MAP(CDlq)
	ON_EN_CHANGE(IDC_DLQ_CODEPAGE, OnChangeDlqCodepage)
	ON_EN_CHANGE(IDC_DLQ_DATE_TIME, OnChangeDlqDateTime)
	ON_EN_CHANGE(IDC_DLQ_FORMAT, OnChangeDlqFormat)
	ON_BN_CLICKED(IDC_DLQ_HOST, OnDlqHost)
	ON_EN_CHANGE(IDC_DLQ_ORIG_Q, OnChangeDlqOrigQ)
	ON_EN_CHANGE(IDC_DLQ_ORIG_QM, OnChangeDlqOrigQm)
	ON_BN_CLICKED(IDC_DLQ_PC, OnDlqPc)
	ON_BN_CLICKED(IDC_DLQ_PD_HOST, OnDlqPdHost)
	ON_BN_CLICKED(IDC_DLQ_PD_PC, OnDlqPdPc)
	ON_EN_CHANGE(IDC_DLQ_PUT_APPL_NAME, OnChangeDlqPutApplName)
	ON_CBN_EDITCHANGE(IDC_DLQ_PUT_APPL_TYPE, OnEditchangeDlqPutApplType)
	ON_EN_CHANGE(IDC_DLQ_REASON, OnChangeDlqReason)
	ON_CBN_SETFOCUS(IDC_DLQ_PUT_APPL_TYPE, OnSetfocusDlqPutApplType)
	ON_BN_CLICKED(IDC_DLQ_PREP_RESEND, OnDlqPrepResend)
	ON_BN_CLICKED(IDC_DLQ_INCLUDE_HEADER, OnDlqIncludeHeader)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX( TTN_NEEDTEXT, 0, CDlq::GetToolText )
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CDlq::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CDlq::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlq message handlers

/////////////////////////////////////////////////////////////
//
// Remove the dead letter header from the message
//
/////////////////////////////////////////////////////////////

void CDlq::UpdatePageData()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to UpdatePageData
		pDoc->logTraceEntry("Entering CDlq::UpdatePageData()");
	}

	// update the form data from the instance variables
	UpdateData(FALSE);

	// enable/disable controls
	enableDisplay();
}

BOOL CDlq::OnSetActive() 

{
	// this dialog has been selected and is becoming active
	if (pDoc->traceEnabled)
	{
		// trace entry to OnSetActive
		pDoc->logTraceEntry("Entering CDlq::OnSetActive()");
	}

	// update the form data from the instance variables
	UpdateData(FALSE);

	// disable/disable the appropriate controls
	enableDisplay();

	BOOL ret = CPropertyPage::OnSetActive();
	
	// send a custom message to switch the focus to the QM combo box
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return ret;
}

BOOL CDlq::OnKillActive() 

{
	// this dialog has lost the focus
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering CDlq::OnKillActive()");
	}

	return CPropertyPage::OnKillActive();
}

int CDlq::parseDLQheader(unsigned char *dlqData, int dataLen, int ccsid, int encodeType)

{
	int				length=0;
	int				len=0;
	int				err=0;
	int				dataType;
	unsigned char	tempArea[64];
	char			tempDate[32];
	char			tempDt[16];
	char			tempTm[16];
	MQDLH			tempDLQ;
	char	traceInfo[512];		// work variable to build trace message

	if (dataLen < sizeof(MQDLH))
	{
		// not enough data to be a DLQ header
		return 0;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CDlq::parseDLQheader() dataLen=%d ccsid=%d encodeType=%d", dataLen, ccsid, encodeType);

		// trace entry to parseDLQheader
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("dlqData", dlqData, sizeof(MQDLH));
	}

	// copy the data into the temporary work area
	memcpy(&tempDLQ, dlqData, sizeof(MQDLH));

	// get the type of data
	dataType = getCcsidType(ccsid);

	// check the code page of the CIH
	if (CHAR_EBCDIC == dataType)
	{
		// translate the charater data to ascii
		EbcdicToAscii((unsigned char *)&tempDLQ.StrucId, sizeof(tempDLQ.StrucId), tempArea);
		memcpy(tempDLQ.StrucId, tempArea, sizeof(tempDLQ.StrucId));
		EbcdicToAscii((unsigned char *)&tempDLQ.DestQName, sizeof(tempDLQ.DestQName), tempArea);
		memcpy(tempDLQ.DestQName, tempArea, sizeof(tempDLQ.DestQName));
		EbcdicToAscii((unsigned char *)&tempDLQ.DestQMgrName, sizeof(tempDLQ.DestQMgrName), tempArea);
		memcpy(tempDLQ.DestQMgrName, tempArea, sizeof(tempDLQ.DestQMgrName));
		EbcdicToAscii((unsigned char *)&tempDLQ.Format, sizeof(tempDLQ.Format), tempArea);
		memcpy(tempDLQ.Format, tempArea, sizeof(tempDLQ.Format));
		EbcdicToAscii((unsigned char *)&tempDLQ.PutApplName, sizeof(tempDLQ.PutApplName), tempArea);
		memcpy(tempDLQ.PutApplName, tempArea, sizeof(tempDLQ.PutApplName));
		EbcdicToAscii((unsigned char *)&tempDLQ.PutDate, sizeof(tempDLQ.PutDate), tempArea);
		memcpy(tempDLQ.PutDate, tempArea, sizeof(tempDLQ.PutDate));
		EbcdicToAscii((unsigned char *)&tempDLQ.PutTime, sizeof(tempDLQ.PutTime), tempArea);
		memcpy(tempDLQ.PutTime, tempArea, sizeof(tempDLQ.PutTime));
	}

	// check the encoding of the data
	if (encodeType != NUMERIC_PC)
	{
		tempDLQ.Version			= reverseBytes4(tempDLQ.Version);
		tempDLQ.Reason			= reverseBytes4(tempDLQ.Reason);
		tempDLQ.Encoding		= reverseBytes4(tempDLQ.Encoding);
		tempDLQ.CodedCharSetId	= reverseBytes4(tempDLQ.CodedCharSetId);
		tempDLQ.PutApplType		= reverseBytes4(tempDLQ.PutApplType);
	}

	// check if we have a dead letter queue header
	if ((memcmp(&tempDLQ.StrucId, MQDLH_STRUC_ID, sizeof(tempDLQ.StrucId)) != 0) ||(tempDLQ.Version != MQDLH_VERSION_1))
	{
		return 0;
	}

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempDLQ.DestQName, sizeof(tempDLQ.DestQName));
	Rtrim((char *)&tempArea);
	m_dlq_orig_queue = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempDLQ.DestQMgrName, sizeof(tempDLQ.DestQMgrName));
	Rtrim((char *)&tempArea);
	m_dlq_orig_qm = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempDLQ.Format, sizeof(tempDLQ.Format));
	m_dlq_format = tempArea;

	memset(tempArea, 0, sizeof(tempArea));
	memcpy(tempArea, tempDLQ.PutApplName, sizeof(tempDLQ.PutApplName));
	m_dlq_put_appl_name = tempArea;

	// process the date and time
	memset(tempDt, 0, sizeof(tempDt));
	memcpy(tempDt, &tempDLQ.PutDate, 8);
	memset(tempTm, 0, sizeof(tempTm));
	memcpy(tempTm, &tempDLQ.PutTime, 8);
	formatDateTime(tempDate, tempDt, tempTm);
	m_dlq_date_time = tempDate;

	// process the integer items
	// get the encoding value
	m_dlq_encode = getIntEncode(tempDLQ.Encoding);
	m_dlq_pd_encode = getPDEncode(tempDLQ.Encoding);
	m_dlq_float_encode = getFloatEncode(tempDLQ.Encoding);

	m_dlq_reason.Format("%d", tempDLQ.Reason);
	m_dlq_codepage.Format("%d", tempDLQ.CodedCharSetId);

	// set the put application type
	setPutApplType(tempDLQ.PutApplType);

	// remember we have a dead letter queue header
	m_dlq_include = TRUE;

	// update the form data from the instance variables
	UpdateData(FALSE);

	dlqLength = sizeof(MQDLH);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CDlq::parseDLQheader() m_dlq_format=%s m_dlq_reason=%s m_dlq_codepage=%s", (LPCTSTR)m_dlq_format, (LPCTSTR)m_dlq_reason, (LPCTSTR)m_dlq_codepage);

		// trace exit from parseDLQheader
		pDoc->logTraceEntry(traceInfo);
	}

	return dlqLength;
}

int CDlq::createHeader(int ccsid, int encodeType)

{
	int				len;
	int				charType;
	char			*ptr;
	MQDLH			tempDLQ={MQDLH_DEFAULT};
	char			tempDate[32];
	char			tempDt[16];
	char			tempTm[16];
	CString			tempFormat;
	CString			tempQName;
	char			traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CDlq::createHeader() m_dlq_format=%s ccsid=%d encodeType=%d", (LPCTSTR)m_dlq_format, ccsid, encodeType);

		// trace entry to createHeader
		pDoc->logTraceEntry(traceInfo);
	}

	// check if a DLQ already exists and has the right ccsid and encoding type
	if ((dlqHeader != NULL) && (dlqCcsid == ccsid) && (dlqEncodeType == encodeType))
	{
		// the DLQ header already exists so reuse it
		if (pDoc->traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Reusing existing header dlqHeader=%8.8X dlqLength=%d", (unsigned int)dlqHeader, dlqLength);

			// trace entry to createHeader
			pDoc->logTraceEntry(traceInfo);

			// write the data to the trace file
			pDoc->dumpTraceData("DLQ header", dlqHeader, dlqLength);
		}

		// just need to return
		return dlqLength;
	}

	// release any current DLQ header
	freeCurrentHeader(1);

	// start to build the new header in ASCII
	tempDLQ.Reason = atoi((LPCTSTR)m_dlq_reason);
	tempDLQ.PutApplType = atoi((LPCTSTR)m_dlq_put_appl_type);
	tempDLQ.CodedCharSetId = atoi((LPCTSTR)m_dlq_codepage);
	tempDLQ.Encoding = getEncodingValue(m_dlq_encode, m_dlq_pd_encode, m_dlq_float_encode);

	// set the format
	setCharData(tempDLQ.Format, sizeof(tempDLQ.Format), (LPCTSTR)m_dlq_format, m_dlq_format.GetLength());

	// get the queue and queue manager names
	setCharData(tempDLQ.DestQName, sizeof(tempDLQ.DestQName), (LPCTSTR)m_dlq_orig_queue, m_dlq_orig_queue.GetLength());
	setCharData(tempDLQ.DestQMgrName, sizeof(tempDLQ.DestQMgrName), (LPCTSTR)m_dlq_orig_qm, m_dlq_orig_qm.GetLength());

	// set the put application name
	setCharData(tempDLQ.PutApplName, sizeof(tempDLQ.PutApplName), (LPCTSTR)m_dlq_put_appl_name, m_dlq_put_appl_name.GetLength());

	// get the data and time
	len = m_dlq_date_time.GetLength();
	if (len > 0)
	{
		if (len > sizeof(tempDate) - 1)
		{
			len = sizeof(tempDate) - 1;
		}
		

		// get a copy of the date
		memset(tempDate, 0, sizeof(tempDate));
		memcpy(tempDate, (LPCTSTR)m_dlq_date_time, len);

		pDoc->dumpTraceData("DLQ DateTime", (const unsigned char *)tempDate, len);

		// If copied from the MQMD, the displayed put date/time should look like
		//  tempDate = "2020/12/31 01:02:03.44"
		// Move the date to the PutDate field - removing the slashes
		ptr = tempDt;
		memcpy(ptr, tempDate, 4);			// copy the year
		memcpy(ptr + 4, tempDate + 5, 2);	// copy the month, skipping the slash
		memcpy(ptr + 6, tempDate + 8, 2);	// copy the day, skipping the slash
		memcpy(tempDLQ.PutDate, tempDt, sizeof(tempDLQ.PutDate));

		// move the time to the PutTime field - removing the colons
		ptr = tempTm;
		memcpy(ptr, tempDate + 11, 2);		// copy the hours
		memcpy(ptr + 2, tempDate + 14, 2);	// copy the minutes, skipping the colon
		memcpy(ptr + 4, tempDate + 17, 2);	// copy the seconds, skipping the colon
		memcpy(ptr + 6, tempDate + 20, 2);	// copy the hundredths, skipping the period
		memcpy(tempDLQ.PutTime, tempTm, sizeof(tempDLQ.PutTime));
	}

	// get the character type
	charType = getCcsidType(ccsid);

	// check if we need to translate to EBCDIC
	if (CHAR_EBCDIC == charType)
	{
		// translate all charater data to EBCDIC
		convertEbcdic(tempDLQ.StrucId, sizeof(tempDLQ.StrucId));
		convertEbcdic(tempDLQ.DestQName, sizeof(tempDLQ.DestQName));
		convertEbcdic(tempDLQ.DestQMgrName, sizeof(tempDLQ.DestQMgrName));
		convertEbcdic(tempDLQ.Format, sizeof(tempDLQ.Format));
		convertEbcdic(tempDLQ.PutApplName, sizeof(tempDLQ.PutApplName));
		convertEbcdic(tempDLQ.PutDate, sizeof(tempDLQ.PutDate));
		convertEbcdic(tempDLQ.PutTime, sizeof(tempDLQ.PutTime));
	}

	// check if we need to convert to big-Endian
	if (encodeType != NUMERIC_PC)
	{
		tempDLQ.Version			= reverseBytes4(tempDLQ.Version);
		tempDLQ.Reason			= reverseBytes4(tempDLQ.Reason);
		tempDLQ.Encoding		= reverseBytes4(tempDLQ.Encoding);
		tempDLQ.CodedCharSetId	= reverseBytes4(tempDLQ.CodedCharSetId);
		tempDLQ.PutApplType		= reverseBytes4(tempDLQ.PutApplType);
	}

	// update the form from the instance variables
	UpdateData(FALSE);

	// allocate enough storage
	dlqHeader = (unsigned char *)rfhMalloc(sizeof(MQDLH), "DLQHDR  ");

	// check if the allocate worked
	if (NULL == dlqHeader)
	{
		dlqLength = 0;
	}
	else
	{
		memcpy(dlqHeader, (void *)&tempDLQ, sizeof(MQDLH));
		dlqLength = sizeof(MQDLH);

		// remember the ccsid and encoding of this header
		dlqCcsid = ccsid;
		dlqEncodeType = encodeType;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CDlq::createHeader() dlqHeader=%8.8X dlqLength=%d dlqCcsid=%d dlqEncodeType=%d", (unsigned int)dlqHeader, dlqLength, dlqCcsid, dlqEncodeType);

		// trace exit from createHeader
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("DLQ header", dlqHeader, dlqLength);
	}

	// return the length of the acquired area
	return dlqLength;
}

void CDlq::freeCurrentHeader(const int callee)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CDlq::freeCurrentHeader() callee=%d dlqHeader=%8.8X dlqLength=%d dlqCcsid=%d dlqEncodeType=%d", callee, (unsigned int)dlqHeader, dlqLength, dlqCcsid, dlqEncodeType);

		// trace entry to freeCurrentHeader
		pDoc->logTraceEntry(traceInfo);
	}

	if (dlqHeader != NULL)
	{
		rfhFree(dlqHeader);
		dlqHeader = NULL;
	}

	dlqLength = 0;
	dlqCcsid = -1;
	dlqEncodeType = -1;
}

BOOL CDlq::GetToolText(UINT id, NMHDR *pNMHDR, LRESULT *pResult)

{
    TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
    UINT nID =pNMHDR->idFrom;
    if (pTTT->uFlags & TTF_IDISHWND)
    {
        // idFrom is actually the HWND of the tool
        nID = ::GetDlgCtrlID((HWND)nID);
        if(nID)
        {
            pTTT->lpszText = MAKEINTRESOURCE(nID);
            pTTT->hinst = AfxGetResourceHandle();
            return(TRUE);
        }
    }
    return(FALSE);
}

BOOL CDlq::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

void CDlq::OnChangeDlqCodepage() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	// discard the header that the DLQ points to since the code page has changed
	pDoc->freeHeader(m_dlq_format);

	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_CODEPAGE);
}

void CDlq::OnChangeDlqDateTime() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_DATE_TIME);
}

void CDlq::OnChangeDlqFormat() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_FORMAT);
}

void CDlq::OnDlqHost() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	// discard the header that the DLQ points to since the encoding has changed
	pDoc->freeHeader(m_dlq_format);

	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_HOST);
}

void CDlq::OnChangeDlqOrigQ() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_ORIG_Q);
}

void CDlq::OnChangeDlqOrigQm() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_ORIG_QM);
}

void CDlq::OnDlqPc() 

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	// discard the header that the DLQ points to since the encoding has changed
	pDoc->freeHeader(m_dlq_format);

	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_PC);
}

void CDlq::OnDlqPdHost() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_PD_HOST);
}

void CDlq::OnDlqPdPc() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_PD_PC);
}

void CDlq::OnChangeDlqPutApplName() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_PUT_APPL_NAME);
}

void CDlq::OnEditchangeDlqPutApplType() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_PUT_APPL_TYPE);
}

void CDlq::OnChangeDlqReason() 

{
	// discard the current header since data has been changed
	freeCurrentHeader(IDC_DLQ_REASON);
}

void CDlq::OnSetfocusDlqPutApplType() 

{
	// populate the combo box drop down list
	if (putApplTypeInit)
	{
		putApplTypeInit = FALSE;
		// insert the known types into the dropdown list	
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_NO_CONTEXT));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_CICS));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_ZOS));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_IMS));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_OS2));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_DOS));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_UNIX));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_QMGR));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_OS400));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_WINDOWS));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_CICS_VSE));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_WINDOWS_NT));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_VMS));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_GUARDIAN));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_VOS));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_IMS_BRIDGE));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_XCF));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_CICS_BRIDGE));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_NOTES_AGENT));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_USER));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_BROKER));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_JAVA));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_DQM));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_CHANNEL_INITIATOR));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_WLM));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_BATCH));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_RRS_BATCH));
		((CComboBox *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->AddString(getApplTypeDisplay(MQAT_SIB));
	}
}

int CDlq::getDLQlength()

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CDlq::getDLQlength() dlqLength=%d", dlqLength);

		// trace exit from getIMSlength
		pDoc->logTraceEntry(traceInfo);
	}

	return dlqLength;
}

BOOL CDlq::PreTranslateMessage(MSG* pMsg) 

{
	// required override to allow tooltips to work
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
			if ((IDC_DLQ_CODEPAGE == id) || 
				(IDC_DLQ_DATE_TIME == id) || 
				(IDC_DLQ_FORMAT == id) || 
				(IDC_DLQ_ORIG_Q == id) || 
				(IDC_DLQ_ORIG_QM == id) || 
				(IDC_DLQ_PUT_APPL_NAME == id) || 
				(IDC_DLQ_REASON == id))
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

void CDlq::setPutApplType(int putType)

{
	const char *	ptr;
	char			tempType[16];

	ptr = getApplTypeDisplay(putType);

	if (strlen(ptr) == 0)
	{
		// check for user type
		if (putType > 0)
		{
			sprintf(tempType, "%d", putType);
			ptr = tempType;
		}
		else
		{
			ptr = "-1 Other/Unknown";
		}
	}

	m_dlq_put_appl_type = ptr;
}

void CDlq::resetDLQdata()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to resetDLQdata
		pDoc->logTraceEntry("Entering CDlq::resetDLQdata()");
	}

	m_dlq_codepage.Empty();
	m_dlq_date_time.Empty();
	m_dlq_encode = -1;
	m_dlq_format.Empty();
	m_dlq_orig_qm.Empty();
	m_dlq_orig_queue.Empty();
	m_dlq_pd_encode = -1;
	m_dlq_put_appl_name.Empty();
	m_dlq_put_appl_type.Empty();
	m_dlq_reason.Empty();
	m_dlq_include = FALSE;;
}

void CDlq::clearDLQheader()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CDlq::clearDLQheader() m_dlq_format=%s m_dlq_codepage=%s m_dlq_encode=%d m_dlq_pd_encode=%d", (LPCTSTR)m_dlq_format, (LPCTSTR)m_dlq_codepage, m_dlq_encode, m_dlq_pd_encode);

		// trace entry to clearDLQheader
		pDoc->logTraceEntry(traceInfo);
	}

	// discard the current header since data has been changed
	freeCurrentHeader(10);
	resetDLQdata();
}

bool CDlq::checkForDlq()

{
	if (m_dlq_include)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CDlq::setNextFormat(char * nextFormat, int * charFormat, int * encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CDlq::setNextFormat() nextFormat=%s charFormat=%d encoding=%d", nextFormat, (*charFormat), (*encoding));

		// trace entry to setNextFormat
		pDoc->logTraceEntry(traceInfo);
	}

	strcpy(nextFormat, (LPCTSTR)m_dlq_format);
	(*charFormat) = getCcsidType(atoi((LPCTSTR)m_dlq_codepage));
	(*encoding) = m_dlq_encode;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CDlq::setNextFormat() nextFormat=%s charFormat=%d encoding=%d", nextFormat, (*charFormat), (*encoding));

		// trace exit from setNextFormat
		pDoc->logTraceEntry(traceInfo);
	}
}

char * CDlq::getDLQheader()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CDlq::getDLQheader() dlqHeader=%8.8X dlqLength=%d dlqCcsid=%d dlqEncodeType=%d", (unsigned int)dlqHeader, dlqLength, dlqCcsid, dlqEncodeType);

		// trace entry to getDLQheader
		pDoc->logTraceEntry(traceInfo);
	}

	return (char *)dlqHeader;
}

// routine to return the current setting of the format field

void CDlq::getFormat(char * mqformat)

{
	// make sure we have current info
	UpdateData(TRUE);

	memset(mqformat, ' ', MQ_FORMAT_LENGTH);
	if (m_dlq_format.GetLength() > 0)
	{
		memcpy(mqformat, (LPCTSTR)m_dlq_format, m_dlq_format.GetLength());
	}

	// terminate the string
	mqformat[MQ_FORMAT_LENGTH] = 0;
}

int CDlq::getCcsid()

{
	// make sure we have current info
	UpdateData(TRUE);

	// update the dialog controls
	UpdateData(FALSE);

	return atoi((LPCTSTR)m_dlq_codepage);
}

int CDlq::getEncoding()

{
	// make sure we have current info
	UpdateData(TRUE);

	// update the dialog controls
	UpdateData(FALSE);

	return m_dlq_encode;
}

int CDlq::getPdEncoding()

{
	// make sure we have current info
	UpdateData(TRUE);

	return m_dlq_pd_encode;
}

int CDlq::getFloatEncoding()

{
	// make sure we have current info
	UpdateData(TRUE);

	return m_dlq_float_encode;
}

void CDlq::updateFields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CDlq::updateFields() m_dlq_format=%s m_dlq_codepage=%s m_dlq_encode=%d m_dlq_pd_encode=%d", (LPCTSTR)m_dlq_format, (LPCTSTR)m_dlq_codepage, m_dlq_encode, m_dlq_pd_encode);

		// trace entry to updateFields
		pDoc->logTraceEntry(traceInfo);
	}

	// make sure we have current info
	UpdateData(TRUE);

	m_dlq_format = newFormat;
	m_dlq_codepage.Format("%d", newCcsid);
	m_dlq_encode = newEncoding;
	m_dlq_pd_encode = newPdEncoding;
	m_dlq_float_encode = newFloatEncoding;

	// release any saved area since the header has changed
	freeCurrentHeader(11);

	// update the dialog controls
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CDlq::updateFields() m_dlq_format=%s m_dlq_codepage=%s m_dlq_encode=%d m_dlq_pd_encode=%d", (LPCTSTR)m_dlq_format, (LPCTSTR)m_dlq_codepage, m_dlq_encode, m_dlq_pd_encode);

		// trace exit from updateFields
		pDoc->logTraceEntry(traceInfo);
	}
}

void CDlq::OnDlqPrepResend()
 
{
	// make sure we have current info in the instance variables
	UpdateData(TRUE);

	// prepare to resubmit the data
	// remove the dead letter header
	m_dlq_include = FALSE;

	// update the dialog controls from the instance variables
	UpdateData(FALSE);

	// update the original format, code page and encoding
	pDoc->removeMQheader(MQHEADER_DLQ, (LPCTSTR)m_dlq_format, atoi((LPCTSTR)m_dlq_codepage), m_dlq_encode, m_dlq_pd_encode, m_dlq_float_encode);

	// make sure we have current info in the instance variables
	UpdateData(TRUE);

	// update the queue name on the main screen
	pDoc->m_Q_name = (LPCTSTR)m_dlq_orig_queue;

	// check if this queue is on the currently selected queue manager
	if (pDoc->m_QM_name.Compare((LPCTSTR)m_dlq_orig_qm) != 0)
	{
		// update the remote queue manager name
		pDoc->m_remote_QM = (LPCTSTR)m_dlq_orig_qm;
	}

	// update the dialog controls from the instance variables
	UpdateData(FALSE);

	// disable most controls
	enableDisplay();
}

void CDlq::updatePageData()

{
	// some of the instance variables may have changed
	// therefore, refresh the dialog controls
	UpdateData(FALSE);
}

BOOL CDlq::OnInitDialog()

{
	CPropertyPage::OnInitDialog();
	
	// tool tips must be enabled	
	EnableToolTips(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDlq::OnDlqIncludeHeader() 

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CDlq::OnDlqIncludeHeader() m_dlq_include=%d m_dlq_format=%s m_dlq_codepage=%s m_dlq_encode=%d m_dlq_pd_encode=%d", m_dlq_include, (LPCTSTR)m_dlq_format, (LPCTSTR)m_dlq_codepage, m_dlq_encode, m_dlq_pd_encode);

		// trace entry to OnDlqIncludeHeader
		pDoc->logTraceEntry(traceInfo);
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	// update the form data from the instance variables
	UpdateData(FALSE);

	// figure out what we just did
	if (m_dlq_include)
	{
		// we have a header - must have just added it
		pDoc->insertMQheader(MQHEADER_DLQ, atoi((LPCTSTR)m_dlq_codepage), m_dlq_encode, m_dlq_pd_encode, m_dlq_float_encode);
	}
	else
	{
		// no more header - must have removed it
		pDoc->removeMQheader(MQHEADER_DLQ, (LPCTSTR)m_dlq_format, atoi((LPCTSTR)m_dlq_codepage), m_dlq_encode, m_dlq_pd_encode, m_dlq_float_encode);
	}

	// enable/disable controls to match
	enableDisplay();
}

void CDlq::enableDisplay()

{
	if (m_dlq_include)
	{
		// including dead letter queue
		// enable the controls
		((CButton *)GetDlgItem(IDC_DLQ_PD_PC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_DLQ_PD_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_DLQ_FLOAT_PC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_DLQ_FLOAT_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_DLQ_FLOAT_390))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_DLQ_FLOAT_TNS))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_DLQ_PC))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_DLQ_HOST))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_DLQ_PREP_RESEND))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_DLQ_REASON))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_DLQ_ORIG_QM))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_DLQ_ORIG_Q))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_DLQ_FORMAT))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_DLQ_CODEPAGE))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_DLQ_PUT_APPL_NAME))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_DLQ_DATE_TIME))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->EnableWindow(TRUE);
	}
	else
	{
		// no dead letter queue
		// disable the controls
		((CButton *)GetDlgItem(IDC_DLQ_PD_PC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_DLQ_PD_HOST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_DLQ_FLOAT_PC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_DLQ_FLOAT_HOST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_DLQ_FLOAT_390))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_DLQ_FLOAT_TNS))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_DLQ_PC))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_DLQ_HOST))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_DLQ_PREP_RESEND))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_DLQ_REASON))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_DLQ_ORIG_QM))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_DLQ_ORIG_Q))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_DLQ_FORMAT))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_DLQ_CODEPAGE))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_DLQ_PUT_APPL_NAME))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_DLQ_DATE_TIME))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_DLQ_PUT_APPL_TYPE))->EnableWindow(FALSE);
	}
}

LONG CDlq::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the queue manager name combo box
	((CButton *)GetDlgItem(IDC_DLQ_INCLUDE_HEADER))->SetFocus();

	return 0;
}

BOOL CDlq::PreCreateWindow(CREATESTRUCT& cs) 

{
	// set window style to match a property page dialog
	cs.style |= WS_TABSTOP;
	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	
	return CPropertyPage::PreCreateWindow(cs);
}

void CDlq::prepareResend()

{
	OnDlqPrepResend();
}
