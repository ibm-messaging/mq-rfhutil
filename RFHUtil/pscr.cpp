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

// pscr.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "pscr.h"
#include "comsubs.h"
#include "xmlsubs.h"

// include for RFH2 constants, etc
#include <cmqpsc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+6

/////////////////////////////////////////////////////////////////////////////
// pscr property page

IMPLEMENT_DYNCREATE(pscr, CPropertyPage)

pscr::pscr() : CPropertyPage(pscr::IDD)
{
	//{{AFX_DATA_INIT(pscr)
	m_pscr_reply = -1;
	m_pscr_reason1 = _T("");
	m_pscr_reason2 = _T("");
	m_pscr_response1 = _T("");
	m_pscr_response2 = _T("");
	m_pscr_other = _T("");
	m_pscr_error_id = _T("");
	m_pscr_error_pos = _T("");
	m_pscr_parm_id = _T("");
	m_pscr_user_id = _T("");
	//}}AFX_DATA_INIT

	m_RFH_pscr_len = 0;
	m_RFH_pscr_ccsid = -1;
	m_RFH_pscr_encoding = -1;
	rfh_pscr_area = NULL;
	pscrDataChanged = 0;
}

pscr::~pscr()
{
	freePscrArea();
}

void pscr::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(pscr)
	DDX_Radio(pDX, IDC_PSCR_REPLY_OK, m_pscr_reply);
	DDX_Text(pDX, IDC_REASON1, m_pscr_reason1);
	DDX_Text(pDX, IDC_REASON2, m_pscr_reason2);
	DDX_Text(pDX, IDC_RESPONSE1_MESG, m_pscr_response1);
	DDX_Text(pDX, IDC_RESPONSE2_MESG, m_pscr_response2);
	DDX_Text(pDX, IDC_PSCR_OTHER, m_pscr_other);
	DDX_Text(pDX, IDC_PSCR_ERROR_ID, m_pscr_error_id);
	DDV_MaxChars(pDX, m_pscr_error_id, 8);
	DDX_Text(pDX, IDC_PSCR_ERROR_POS, m_pscr_error_pos);
	DDV_MaxChars(pDX, m_pscr_error_pos, 8);
	DDX_Text(pDX, IDC_PSCR_PARM_ID, m_pscr_parm_id);
	DDX_Text(pDX, IDC_PSCR_USER_ID, m_pscr_user_id);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(pscr, CPropertyPage)
	//{{AFX_MSG_MAP(pscr)
	ON_EN_CHANGE(IDC_REASON1, OnChangeReason1)
	ON_EN_CHANGE(IDC_REASON2, OnChangeReason2)
	ON_EN_CHANGE(IDC_RESPONSE1_MESG, OnChangeResponse1Mesg)
	ON_EN_CHANGE(IDC_RESPONSE2_MESG, OnChangeResponse2Mesg)
	ON_BN_CLICKED(IDC_PSCR_WARN, OnPscrWarn)
	ON_BN_CLICKED(IDC_PSCR_ERROR, OnPscrError)
	ON_BN_CLICKED(IDC_PSCR_REPLY_OK, OnPscrReplyOk)
	ON_EN_CHANGE(IDC_PSCR_OTHER, OnChangePscrOther)
	ON_EN_CHANGE(IDC_PSCR_ERROR_ID, OnChangePscrErrorId)
	ON_EN_CHANGE(IDC_PSCR_ERROR_POS, OnChangePscrErrorPos)
	ON_EN_CHANGE(IDC_PSCR_PARM_ID, OnChangePscrParmId)
	ON_EN_CHANGE(IDC_PSCR_USER_ID, OnChangePscrUserId)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, pscr::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, pscr::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// pscr message handlers

BOOL pscr::OnKillActive() 
{
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering pscr::OnKillActive()");
	}

	UpdateData(TRUE);
	
	if (pscrDataChanged)
	{
		// release the current complete RFH
		freePscrArea();
		pDoc->freeRfhArea();
	}

	// update the form data from the instance variables
	UpdateData(FALSE);
	
	return CPropertyPage::OnKillActive();
}

BOOL pscr::OnSetActive() 

{
	if (pDoc->traceEnabled)
	{
		// trace entry to OnSetActive
		pDoc->logTraceEntry("Entering pscr::OnSetActive()");
	}

	// Update the data in the controls from the instance variables
	updatePageData();
	
	BOOL ret = CPropertyPage::OnSetActive();
	
	// send a custom message to switch the focus to the reply type
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return ret;
}

BOOL pscr::OnInitDialog() 

{
	CPropertyPage::OnInitDialog();
	
	// tool tips are provided and must be initialized
	EnableToolTips(TRUE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL pscr::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)
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

void pscr::OnChangeReason1()

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnChangeReason2()

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnChangeResponse1Mesg()

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnChangeResponse2Mesg() 

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnPscrWarn()

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnPscrError()

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnPscrReplyOk()

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnChangePscrOther() 

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::updatePageData()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to updatePageData
		pDoc->logTraceEntry("Entering pscr::updatePageData()");
	}

	UpdateData (TRUE);

	// the data in the controls has just been refreshed
	pscrDataChanged = false;

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void pscr::OnChangePscrErrorId() 


{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnChangePscrErrorPos() 

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnChangePscrParmId() 

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

void pscr::OnChangePscrUserId() 

{
	// data in the controls has been changed by the user
	pscrDataChanged = true;
}

LONG pscr::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the appropriate response type
	switch (m_pscr_reply)
	{
	case PSCR_COMP_OK:
		{
			((CButton *)GetDlgItem(IDC_PSCR_REPLY_OK))->SetFocus();
			break;
		}
	case PSCR_COMP_WARN:
		{
			((CButton *)GetDlgItem(IDC_PSCR_WARN))->SetFocus();
			break;
		}
	case PSCR_COMP_ERR:
		{
			((CButton *)GetDlgItem(IDC_PSCR_ERROR))->SetFocus();
			break;
		}
	default:
		{
			((CButton *)GetDlgItem(IDC_PSCR_REPLY_OK))->SetFocus();
			break;
		}
	}

	return 0;
}

BOOL pscr::PreCreateWindow(CREATESTRUCT& cs) 

{
	// initialize the dialog as a property page
	cs.style |= WS_TABSTOP;
	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	
	return CPropertyPage::PreCreateWindow(cs);
}

BOOL pscr::PreTranslateMessage(MSG* pMsg) 

{
	//
	// This routine is necessary for the tab key to work correctly
	//

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
			if ((IDC_REASON1 == id) || 
				(IDC_REASON2 == id) || 
				(IDC_RESPONSE1_MESG == id) || 
				(IDC_RESPONSE2_MESG == id) || 
				(IDC_PSCR_OTHER == id) || 
				(IDC_PSCR_ERROR_ID == id) || 
				(IDC_PSCR_ERROR_POS == id) || 
				(IDC_PSCR_PARM_ID == id) || 
				(IDC_PSCR_USER_ID == id))
			{
				processBackspace(curFocus);
				pscrDataChanged = true;
				return TRUE;
			}
		}
	}

	if (IsDialogMessage(pMsg))
		return TRUE;
	else
		return CPropertyPage::PreTranslateMessage(pMsg);
}

void pscr::setPscrArea(unsigned char *pscrData, int dataLen, int ccsid, int encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering pscr::setPscrArea() rfh_pscr_area=%8.8X m_RFH_pscr_len=%d ccsid=%d encoding=%d", (unsigned int)rfh_pscr_area, m_RFH_pscr_len, ccsid, encoding);

		// trace entry to setPscrArea
		pDoc->logTraceEntry(traceInfo);
	}

	// free any old area
	freePscrArea();

	if ((pscrData != NULL) && (dataLen > 0))
	{
		// allocate storage for the pscr folder
		rfh_pscr_area = (unsigned char *)rfhMalloc(dataLen + 5, "PSCRAREA");

		// copy the data to the allocated area and terminate it as a safeguard
		memcpy(rfh_pscr_area, pscrData, dataLen);
		rfh_pscr_area[dataLen] = 0;
		rfh_pscr_area[dataLen + 1] = 0;		// allow for UCS-2 data

		// remember the length of the pscr folder
		m_RFH_pscr_len = dataLen;

		// remember the ccsid and encoding that were used to build this folder
		m_RFH_pscr_ccsid = ccsid;
		m_RFH_pscr_encoding = encoding;

		// indicate that the data is current
		pscrDataChanged = false;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting pscr::setPscrArea() rfh_pscr_area=%8.8X m_RFH_pscr_len=%d", (unsigned int)rfh_pscr_area, m_RFH_pscr_len);

		// trace exit from setPscrArea
		pDoc->logTraceEntry(traceInfo);
	}
}

void pscr::freePscrArea()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering pscr::freePscrArea() rfh_pscr_area=%8.8X m_RFH_pscr_len=%d", (unsigned int)rfh_pscr_area, m_RFH_pscr_len);

		// trace entry to freePscrArea
		pDoc->logTraceEntry(traceInfo);
	}

	if (rfh_pscr_area != NULL)
	{
		rfhFree(rfh_pscr_area);
	}

	rfh_pscr_area = NULL;
	m_RFH_pscr_len = 0;
	m_RFH_pscr_ccsid = -1;
	m_RFH_pscr_encoding = -1;
}

///////////////////////////////////////////////////////
//
// Routine to parse the pscr folder in an RFH2 header.
// This folder is used for publish and subscribe
// responses from the broker.
//
///////////////////////////////////////////////////////

void pscr::parseRFH2pscr(unsigned char *rfhdata, int dataLen)

{
	bool	more;
	int		oth=0;
	int		found;
	char	*ptr;
	char	*endptr;
	char	tempReply[32];
	char	tempReason1[32];
	char	tempReason2[32];
	char	tempResponse1[8192];
	char	tempResponse2[8192];
	char	tempOther[8192];
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering pscr::parseRFH2pscr() rfhdata=%8.8X dataLen=%d", (unsigned int)rfhdata, dataLen);

		// trace entry to parseRFH2pscr
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("pscr rfhdata", rfhdata, dataLen);
	}

	// reset the current values
	clearPSCRdata();

	// initialize fields
	memset(tempReply, 0, sizeof(tempReply));
	memset(tempReason1, 0, sizeof(tempReason1));
	memset(tempReason2, 0, sizeof(tempReason2));
	memset(tempResponse1, 0, sizeof(tempResponse1));
	memset(tempResponse2, 0, sizeof(tempResponse2));
	memset(tempOther, 0, sizeof(tempOther));

	// Search for the RFH pub/sub fields in the pub/sub folder
	ptr = (char *)rfhdata + 6;		// skip the pscr tag

	// find the end of the pscr folder
	endptr = ptr;
	while ((endptr < ((char *)rfhdata + dataLen - 6)) && 
		   (memcmp(endptr, MQRFH2_PUBSUB_RESP_FOLDER_E, sizeof(MQRFH2_PUBSUB_RESP_FOLDER_E) - 1)))
	{
		endptr++;
	}

	// find the initial begin bracket
	more = true;
	while (more)
	{
		while ((ptr < endptr) && (ptr[0] != '<'))
		{
			ptr++;
		}

		// did we find what we were looking for?
		if ((ptr < endptr) && ('<' == ptr[0]))
		{
			if (memcmp(ptr, MQRFH2_PUBSUB_RESP_FOLDER_E, sizeof(MQRFH2_PUBSUB_RESP_FOLDER_E) - 1) == 0)
			{
				more = false;
			}
			else
			{
				if (memcmp(ptr, MQPSCR_RESPONSE_B, sizeof(MQPSCR_RESPONSE_B) - 1) == 0)
				{
					if ((0 == tempReason1[0]) && (0 == tempResponse1[0]))
					{
						ptr = processPscrResponse(ptr, 
												  endptr, 
												  tempReason1, 
												  tempResponse1,
												  sizeof(tempReason1),
												  sizeof(tempResponse1));
					}
					else
					{
						if ((0 == tempReason2[0]) && (0 == tempResponse2[0]))
						{
							ptr = processPscrResponse(ptr, 
													  endptr, 
													  tempReason2, 
													  tempResponse2,
													  sizeof(tempReason1),
													  sizeof(tempResponse1));
						}
						else
						{
							// more than 2 responses - this is more than we have room for, so give up
							while ((ptr < endptr) && 
								   (memcmp(ptr, MQPSCR_RESPONSE_E, sizeof(MQPSCR_RESPONSE_E) - 1) != 0))
							{
								ptr++;
							}

							if ((ptr < endptr) && 
								(memcmp(ptr, MQPSCR_RESPONSE_E, sizeof(MQPSCR_RESPONSE_E) - 1) == 0))
							{
								ptr += sizeof(MQPSCR_RESPONSE_E) - 1;
							}
						}
					}
				}
				else
				{
					if (memcmp(ptr, MQPSCR_COMPLETION_B, sizeof(MQPSCR_COMPLETION_B) - 1) == 0)
					{
						// process the command tag
						found = 0;
						ptr = processTag(ptr, MQPSCR_COMPLETION_B , tempReply, sizeof(tempReply), &found);
					}
					else
					{
						// make sure we haven't exceeded the capacity of our temporary variable
						if (oth < (sizeof(tempOther) - 1))
						{
							// capture any data that is not in the completion or response tags
							do
							{
								tempOther[oth] = ptr[0];
								ptr++;
								oth++;
							} while ((ptr < endptr) && (ptr[0] != '<') && 
									 (oth < (sizeof(tempOther) - 1)));
						}
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

	m_pscr_reply = -1;
	if (strcmp(tempReply, MQPSCR_OK) == 0)
	{
		m_pscr_reply = PSCR_COMP_OK;
	}
	else
	{
		if (strcmp(tempReply, MQPSCR_WARNING) == 0)
		{
			m_pscr_reply = PSCR_COMP_WARN;
		}
		else
		{
			if (strcmp(tempReply, MQPSCR_ERROR) == 0)
			{
				m_pscr_reply = PSCR_COMP_ERR;
			}
		}
	}

	m_pscr_reason1 = tempReason1;
	m_pscr_reason2 = tempReason2;
	m_pscr_response1 = tempResponse1;
	m_pscr_response2 = tempResponse2;
	m_pscr_other = tempOther;

	// update the form data from the instance variables
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting parseRFH2pscr - m_pscr_reply=%d", m_pscr_reply);

		// trace exit from parseRFH2pscr
		pDoc->logTraceEntry(traceInfo);
	}
}

char * pscr::processPscrResponse(char *ptr, 
								 char *endptr, 
								 char *reason, 
								 char *other,
								 int maxReason,
								 int maxOther)

{
	int		j;
	int		found;
	bool	more;
	char	*endresp;

	// Skip the response tag
	ptr += sizeof(MQPSCR_RESPONSE_B) - 1;

	// find the end of the response folder
	endresp = ptr;
	while ((endresp < endptr - 10) && 
		   (memcmp(endresp, MQPSCR_RESPONSE_E, sizeof(MQPSCR_RESPONSE_E) - 1) != 0))
	{
		endresp++;
	}

	j = 0;
	more = true;
	while (more)
	{
		// find the beginning of the next tag
		while ((ptr < endresp) && (ptr[0] != '<'))
		{
			ptr++;
		}

		// check for a reason tag
		found = 0;
		ptr = processTag(ptr, MQPSCR_REASON_B, reason, maxReason, &found);
		if (0 == found)
		{
			// this is some other tag, so we will add it to the response
			do
			{
				other[j++] = ptr++[0];
			} while ((j < maxOther) && (ptr < endptr) && (ptr[0] != '<'));
		}

		if (ptr >= endresp)
		{
			more = false;
		}
	}

	// terminate the other field
	other[j] = 0;

	if ((endresp < endptr) && 
		(memcmp(endresp, MQPSCR_RESPONSE_E, sizeof(MQPSCR_RESPONSE_E) - 1) == 0))
	{
		endresp += sizeof(MQPSCR_RESPONSE_E) - 1;
	}

	return endresp;
}


char * pscr::parseV1Pscr(char *rfhptr, int *found)

{
	char	tempval[8192];
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering pscr::parseV1Pscr() rfhptr=%8.8X %.32s", (unsigned int)rfhptr, rfhptr);

		// trace entry to parseV1Pscr
		pDoc->logTraceEntry(traceInfo);
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_COMP_CODE, sizeof(MQPS_COMP_CODE) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_COMP_CODE);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_pscr_reply = atoi(tempval);
	}

	// N.B. must check for Reason Text first since the check for Reason will also match
	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_REASON_TEXT, sizeof(MQPS_REASON_TEXT) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_REASON_TEXT);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);

		if (m_pscr_response1.GetLength() == 0)
		{
			m_pscr_response1 = tempval;
		}
		else
		{
			if (m_pscr_response2.GetLength() == 0)
			{
				m_pscr_response2 = tempval;
			}
		}
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_REASON, sizeof(MQPS_REASON) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_REASON);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);

		if (m_pscr_reason1.GetLength() == 0)
		{
			m_pscr_reason1 = tempval;
		}
		else
		{
			if (m_pscr_reason2.GetLength() == 0)
			{
				m_pscr_reason2 = tempval;
			}
		}
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_ERROR_ID, sizeof(MQPS_ERROR_ID) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_ERROR_ID);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_pscr_error_id = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_ERROR_POS, sizeof(MQPS_ERROR_POS) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_ERROR_POS);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_pscr_error_pos = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_PARAMETER_ID, sizeof(MQPS_PARAMETER_ID) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_PARAMETER_ID);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_pscr_parm_id = tempval;
	}

	if ((0 == (*found)) && 
		(memcmp(rfhptr, MQPS_USER_ID, sizeof(MQPS_USER_ID) - 1) == 0))
	{
		(*found) = 1;
		rfhptr += sizeof(MQPS_USER_ID);
		rfhptr = parseRFH1String(rfhptr, tempval, sizeof(tempval) - 1);
		m_pscr_user_id = tempval;
	}

	// update the form data from the instance variables
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting parseV1Pscr - rfhptr=%8.8X m_pscr_reply=%d found=%d", (unsigned int)rfhptr, m_pscr_reply, (*found));

		// trace exit from parseV1Pscr
		pDoc->logTraceEntry(traceInfo);
	}

	return rfhptr;
}

void pscr::setPscrOther(char *tempOther)

{
	m_pscr_other = tempOther;

	// update the form data from the instance variables
	UpdateData(FALSE);
}

//////////////////////////////////////////////////
//
// Routine to add pubsub reply fields to the RFH1 header
//
//////////////////////////////////////////////////

int pscr::buildV1PubResp(char *tempBuf)

{
	char	*ptr;
	int		comp=-1;
	char	tempVal[4];
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering pscr::buildV1PubResp()");

		// trace entry to buildV1PubResp
		pDoc->logTraceEntry(traceInfo);
	}

	switch (m_pscr_reply)
	{
	case PSCR_COMP_OK:
		{
			comp = MQCC_OK;
			break;
		}
	case PSCR_COMP_WARN:
		{
			comp = MQCC_WARNING;
			break;
		}
	case PSCR_COMP_ERR:
		{
			comp = MQCC_FAILED;
			break;
		}
	}

	ptr = tempBuf;
	if (comp != -1)
	{
		memset(tempVal, 0, sizeof(tempVal));
		sprintf(tempVal, "%d", comp);
		ptr = buildV1String(ptr, MQPS_COMP_CODE, tempVal, 0);
	}

	if (m_pscr_reason1.GetLength() > 0)
	{
		ptr = buildV1String(ptr, MQPS_REASON, m_pscr_reason1, ptr-tempBuf);
	}

	if (m_pscr_response1.GetLength() > 0)
	{
		ptr = buildV1String(ptr, MQPS_REASON_TEXT, m_pscr_response1, ptr-tempBuf);
	}

	if (m_pscr_error_id.GetLength() > 0)
	{
		ptr = buildV1String(ptr, MQPS_ERROR_ID, m_pscr_error_id, ptr-tempBuf);
	}


	if (m_pscr_error_pos.GetLength() > 0)
	{
		ptr = buildV1String(ptr, MQPS_ERROR_POS, m_pscr_error_pos, ptr-tempBuf);
	}

	if (m_pscr_parm_id.GetLength() > 0)
	{
		ptr = buildV1String(ptr, MQPS_PARAMETER_ID, m_pscr_parm_id, ptr-tempBuf);
	}

	if (m_pscr_user_id.GetLength() > 0)
	{
		ptr = buildV1String(ptr, MQPS_USER_ID, m_pscr_user_id, ptr-tempBuf);
	}

	if (m_pscr_other.GetLength() > 0)
	{
		if (ptr > tempBuf)
		{
			strcat(ptr, " ");
		}

		strcat(ptr, (LPCTSTR) m_pscr_other);
		ptr += m_pscr_other.GetLength();
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting pscr::buildV1PubResp() length=%d", ptr - tempBuf);

		// trace entry to buildV1PubResp
		pDoc->logTraceEntry(traceInfo);

		// was anything built?
		if (ptr > tempBuf)
		{
			// write the data to the trace file
			pDoc->dumpTraceData("pscr V1 rfhdata", (unsigned char *)tempBuf, ptr - tempBuf);
		}
	}

	return (ptr - tempBuf);
}

//////////////////////////////////////////////////
//
// Routine to build a pscr folder for the RFH header
//
//////////////////////////////////////////////////

int pscr::buildPscrArea(int ccsid, int encoding)

{
	int				i;
	int				j;
	int				extra;
	unsigned char	*tempPtr;
	wchar_t			*ucsPtr;
	char			spaces[4];
	CString			xml;
	char			traceInfo[512];		// work variable to build trace message

	// make sure we have current info
	UpdateData(TRUE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering pscr::buildPscrArea() ccsid=%d encoding=%d", ccsid, encoding);

		// trace entry to buildPscrArea
		pDoc->logTraceEntry(traceInfo);
	}

	// Has the jms data been changed?
	if (pscrDataChanged || (m_RFH_pscr_ccsid != ccsid) || (m_RFH_pscr_encoding != encoding))
	{
		// data has changed - free the current jms area
		freePscrArea();
	}

	if (NULL == rfh_pscr_area)
	{
		// initialize the string to nulls
		xml.Empty();

		// reject any attempts if the command is not set
		if (-1 == m_pscr_reply)
		{
			return 0;
		}

		// build the first part of the XML text
		xml = MQRFH2_PUBSUB_RESP_FOLDER_B;
		xml += MQPSCR_COMPLETION_B;

		switch (m_pscr_reply)
		{
		case PSCR_COMP_OK:
			{
				xml += MQPSCR_OK;
				break;
			}
		case PSCR_COMP_WARN:
			{
				xml += MQPSCR_WARNING;
				break;
			}
		case PSCR_COMP_ERR:
			{
				xml += MQPSCR_ERROR;
				break;
			}
		}

		xml += MQPSCR_COMPLETION_E;

		if ((m_pscr_response1.GetLength() > 0) || 
			(m_pscr_reason1.GetLength() > 0))
		{
			xml += MQPSCR_RESPONSE_B;

			if (m_pscr_reason1.GetLength() > 0)
			{
				xml += MQPSCR_REASON_B;
				xml += m_pscr_reason1;
				xml += MQPSCR_REASON_E;
			}

			xml += m_pscr_response1;
			xml += MQPSCR_RESPONSE_E;
		}

		if ((m_pscr_response2.GetLength() > 0) || 
			(m_pscr_reason2.GetLength() > 0))
		{
			xml += MQPSCR_RESPONSE_B;

			if (m_pscr_reason2.GetLength() > 0)
			{
				xml += MQPSCR_REASON_B;
				xml += m_pscr_reason2;
				xml += MQPSCR_REASON_E;
			}

			xml += m_pscr_response2;
			xml += MQPSCR_RESPONSE_E;
		}

		if (m_pscr_other.GetLength() > 0)
		{
			xml += m_pscr_other;
		}

		xml += MQRFH2_PUBSUB_RESP_FOLDER_E;
	}

	// check if the data must be converted to UCS-2
	if (isUCS2(ccsid))
	{
		// convert the data to UCS-2
		// allocate a temporary area to hold the UCS-2 data
		tempPtr = (unsigned char *)rfhMalloc(2 * xml.GetLength() + 16, "PSCRTPTR");

		if (tempPtr != 0)
		{
			// translate the data to UCS-2
			MultiByteToUCS2(tempPtr, 2 * xml.GetLength() + 2, (unsigned char *)(LPCTSTR)xml, 2 * xml.GetLength());

			// set the length of the new mcd in bytes (each 16-bit character is 2 bytes)
			m_RFH_pscr_len = roundLength2(tempPtr, NUMERIC_PC) * 2;

			// check if the data has to be reversed
			if (encoding != NUMERIC_PC)
			{
				// reverse the order of the bytes
				i = 0;
				j = m_RFH_pscr_len >> 1;
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
			setPscrArea(tempPtr, m_RFH_pscr_len, ccsid, encoding);

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
		setPscrArea((unsigned char *)(LPCTSTR)xml, xml.GetLength(), ccsid, encoding);
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting pscr::buildPscrArea m_RFH_pscr_len=%d", m_RFH_pscr_len);

		// trace exit from buildPscrArea
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("pubsub data", rfh_pscr_area, m_RFH_pscr_len);
	}

	return m_RFH_pscr_len;
}


BOOL pscr::wasDataChanged()

{
	return pscrDataChanged;
}

const char * pscr::getPscrArea()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering other::getPscrArea() rfh_pscr_area=%8.8X m_RFH_pscr_len=%d", (unsigned int)rfh_pscr_area, m_RFH_pscr_len);

		// trace entry to getPscrArea
		pDoc->logTraceEntry(traceInfo);
	}

	return (char *)rfh_pscr_area;
}

void pscr::clearPSCRdata()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to clearPSCRdata
		pDoc->logTraceEntry("Entering pscr::clearPSCRdata()");
	}

	// reset the current values
	m_pscr_reply = -1;
	m_pscr_reason1.Empty();
	m_pscr_reason2.Empty();
	m_pscr_response1.Empty();
	m_pscr_response2.Empty();
	m_pscr_other.Empty();
	m_pscr_user_id.Empty();
	m_pscr_error_id.Empty();
	m_pscr_error_pos.Empty();
	m_pscr_parm_id.Empty();

	// get rid of any saved areas
	freePscrArea();

	// update the form data from the instance variables
	UpdateData(FALSE);
}
