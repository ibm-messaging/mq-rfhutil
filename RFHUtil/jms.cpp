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

// jms.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "xmlsubs.h"
#include "jms.h"

// include for RFH2 constants, etc
#include <cmqpsc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+6
#define MAX_JMS_NAME	512

#define RFH2_JMS_B	 "<jms>"
#define RFH2_JMS_E	 "</jms>"
#define RFH2_JMS_DST_B	"<Dst>"
#define RFH2_JMS_DST_E "</Dst>"
#define RFH2_JMS_RTO_B	"<Rto>"
#define RFH2_JMS_RTO_E "</Rto>"
#define RFH2_JMS_EXP_B	"<Exp>"
#define RFH2_JMS_EXP_E "</Exp>"
#define RFH2_JMS_CID_B	"<Cid>"
#define RFH2_JMS_CID_E "</Cid>"
#define RFH2_JMS_GID_B	"<Gid>"
#define RFH2_JMS_GID_E "</Gid>"
#define RFH2_JMS_SEQ_B	"<Seq>"
#define RFH2_JMS_SEQ_E "</Seq>"
#define RFH2_JMS_TMS_B	"<Tms>"
#define RFH2_JMS_TMS_E "</Tms>"
#define RFH2_JMS_PRI_B	"<Pri>"
#define RFH2_JMS_PRI_E "</Pri>"
#define RFH2_JMS_DLV_B	"<Dlv>"
#define RFH2_JMS_DLV_E "</Dlv>"

/////////////////////////////////////////////////////////////////////////////
// jms property page

IMPLEMENT_DYNCREATE(jms, CPropertyPage)

jms::jms() : CPropertyPage(jms::IDD)
{
	//{{AFX_DATA_INIT(jms)
	m_jms_dst = _T("");
	m_jms_rto = _T("");
	m_jms_priority = 0;
	m_jms_expiration = _T("");
	m_jms_sequence = 0;
	m_jms_group_id = _T("");
	m_jms_correl_id = _T("");
	m_jms_delivery_mode = 0;
	m_jms_msg_type = -1;
	jmsDataChanged = false;
	m_jms_timestamp = _T("");
	m_jms_user_def = _T("");
	//}}AFX_DATA_INIT

	m_RFH_jms_len = 0;
	jms_data_ccsid = -1;
	jms_data_encoding = -1;
	rfh_jms_area = NULL;
	jmsDataChanged = false;
	pDoc = NULL;
}

jms::~jms()
{
	freeJmsArea();
}

void jms::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(jms)
	DDX_Text(pDX, IDC_JMS_DST, m_jms_dst);
	DDV_MaxChars(pDX, m_jms_dst, 255);
	DDX_Text(pDX, IDC_JMS_RTO, m_jms_rto);
	DDV_MaxChars(pDX, m_jms_rto, 255);
	DDX_Text(pDX, IDC_JMS_PRIORITY, m_jms_priority);
	DDX_Text(pDX, IDC_JMS_EXPIRATION, m_jms_expiration);
	DDV_MaxChars(pDX, m_jms_expiration, 20);
	DDX_Text(pDX, IDC_JMS_SEQUENCE, m_jms_sequence);
	DDX_Text(pDX, IDC_JMS_GROUP_ID, m_jms_group_id);
	DDV_MaxChars(pDX, m_jms_group_id, 255);
	DDX_Text(pDX, IDC_JMS_CORREL_ID, m_jms_correl_id);
	DDV_MaxChars(pDX, m_jms_correl_id, 255);
	DDX_Text(pDX, IDC_JMS_DELIVERY_MODE, m_jms_delivery_mode);
	DDX_Radio(pDX, IDC_JMS_TEXT, m_jms_msg_type);
	DDX_Text(pDX, IDC_JMS_TIMESTAMP, m_jms_timestamp);
	DDV_MaxChars(pDX, m_jms_timestamp, 20);
	DDX_Text(pDX, IDC_JMS_USER_DEF, m_jms_user_def);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(jms, CPropertyPage)
	//{{AFX_MSG_MAP(jms)
	ON_EN_CHANGE(IDC_JMS_DST, OnChangeJmsDst)
	ON_EN_CHANGE(IDC_JMS_CORREL_ID, OnChangeJmsCorrelId)
	ON_EN_CHANGE(IDC_JMS_DELIVERY_MODE, OnChangeJmsDeliveryMode)
	ON_EN_CHANGE(IDC_JMS_EXPIRATION, OnChangeJmsExpiration)
	ON_EN_CHANGE(IDC_JMS_GROUP_ID, OnChangeJmsGroupId)
	ON_EN_CHANGE(IDC_JMS_PRIORITY, OnChangeJmsPriority)
	ON_EN_CHANGE(IDC_JMS_RTO, OnChangeJmsRto)
	ON_EN_CHANGE(IDC_JMS_SEQUENCE, OnChangeJmsSequence)
	ON_BN_CLICKED(IDC_JMS_BYTES, OnJmsBytes)
	ON_BN_CLICKED(IDC_JMS_MAP, OnJmsMap)
	ON_BN_CLICKED(IDC_JMS_OBJECT, OnJmsObject)
	ON_BN_CLICKED(IDC_JMS_STREAM, OnJmsStream)
	ON_BN_CLICKED(IDC_JMS_TEXT, OnJmsText)
	ON_BN_CLICKED(IDC_JMS_NONE, OnJmsNone)
	ON_EN_CHANGE(IDC_JMS_TIMESTAMP, OnChangeJmsTimestamp)
	ON_EN_CHANGE(IDC_JMS_USER_DEF, OnChangeJmsUserDef)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, jms::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, jms::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// jms message handlers

BOOL jms::OnKillActive() 
{
	// this dialog is becoming inactive
	// make sure any changes are reflected in the instance variables
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering jms::OnKillActive()");
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	return CPropertyPage::OnKillActive();
}

BOOL jms::OnSetActive() 

{
	// this dialog is becoming active
	// make sure the dialog reflects the contents of the instance variables
	if (pDoc->traceEnabled)
	{
		// trace entry to OnSetActive
		pDoc->logTraceEntry("Entering jms::OnSetActive()");
	}

	updatePageData();
	
	BOOL ret = CPropertyPage::OnSetActive();
	
	// send a custom message to switch the focus to the QM combo box
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return ret;
}

void jms::OnChangeJmsDst() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnChangeJmsCorrelId() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnChangeJmsDeliveryMode() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnChangeJmsExpiration() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnChangeJmsGroupId() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnChangeJmsPriority() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnChangeJmsRto() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnChangeJmsSequence() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnChangeJmsTimestamp() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnChangeJmsUserDef() 

{
	// Remember that the user has changed the data and the data area must be rebuilt
	jmsDataChanged = true;
}

void jms::OnJmsBytes() 

{
	// change the format name
	pDoc->setJMSdomain(MQMCD_DOMAIN_JMS_BYTES);
}

void jms::OnJmsMap() 

{
	// change the format name
	pDoc->setJMSdomain(MQMCD_DOMAIN_JMS_MAP);
}

void jms::OnJmsObject() 

{
	// change the format name
	pDoc->setJMSdomain(MQMCD_DOMAIN_JMS_OBJECT);
}

void jms::OnJmsStream() 

{
	// change the format name
	pDoc->setJMSdomain(MQMCD_DOMAIN_JMS_STREAM);
}

void jms::OnJmsText() 

{
	// check if any of the radio buttons have been selected
	if (((CButton *)GetDlgItem(IDC_JMS_TEXT))->GetCheck() == 1)
	{
		// change the format name
		pDoc->setJMSdomain(MQMCD_DOMAIN_JMS_TEXT);
	}
}

void jms::OnJmsNone()
 
{
	// change the format name
	pDoc->setJMSdomain(MQMCD_DOMAIN_JMS_NONE);
}

BOOL jms::OnInitDialog()
 
{
	CPropertyPage::OnInitDialog();
	
	// Enable tool tips support for this dialog
	EnableToolTips(TRUE);
	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL jms::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

BOOL jms::PreTranslateMessage(MSG* pMsg) 

{
	// necessary to allow the tab key to be used to move between fields
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
			if ((IDC_JMS_DST == id) || 
				(IDC_JMS_RTO == id) || 
				(IDC_JMS_PRIORITY == id) || 
				(IDC_JMS_EXPIRATION == id) || 
				(IDC_JMS_SEQUENCE == id) || 
				(IDC_JMS_GROUP_ID == id) || 
				(IDC_JMS_CORREL_ID == id) || 
				(IDC_JMS_DELIVERY_MODE == id) || 
				(IDC_JMS_TIMESTAMP == id) || 
				(IDC_PUB_BROKER_QM == id) || 
				(IDC_PUB_QM == id) || 
				(IDC_PUB_Q == id) || 
				(IDC_PUB_PUBTIME == id) || 
				(IDC_PUB_OTHER == id) || 
				(IDC_JMS_USER_DEF == id))
			{
				processBackspace(curFocus);
				jmsDataChanged = true;
				return TRUE;
			}
		}
	}

	if (IsDialogMessage(pMsg))
		return TRUE;
	else
		return CPropertyPage::PreTranslateMessage(pMsg);
}

void jms::updatePageData()

{
	// update the form data from the instance variables
	if (pDoc->traceEnabled)
	{
		// trace entry to updatePageData
		pDoc->logTraceEntry("Entering jms::updatePageData()");
	}

	UpdateData(FALSE);
}

LONG jms::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the queue manager name combo box
	((CButton *)GetDlgItem(IDC_JMS_TEXT))->SetFocus();

	return 0;
}

void jms::setJmsArea(unsigned char *jmsData, int dataLen, int ccsid, int encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering jms::setJmsArea() rfh_jms_area=%8.8X m_RFH_jms_len=%d", (unsigned int)rfh_jms_area, m_RFH_jms_len);

		// trace entry to setJmsArea
		pDoc->logTraceEntry(traceInfo);
	}

	// free any old area
	freeJmsArea();

	if ((jmsData != NULL) && (dataLen > 0))
	{
		// allocate storage for the jms folder
		rfh_jms_area = (unsigned char *)rfhMalloc(dataLen + 5, "JMSHDR  ");

		// copy the data to the allocated area and terminate it as a safeguard
		memcpy(rfh_jms_area, jmsData, dataLen);
		rfh_jms_area[dataLen] = 0;
		rfh_jms_area[dataLen + 1] = 0;		// allow for UCS-2 data

		// remember the length of the jms folder
		m_RFH_jms_len = dataLen;

		// remember the ccsid and encoding that were used to build this folder
		jms_data_ccsid = ccsid;
		jms_data_encoding = encoding;

		// indicate that the data is current
		jmsDataChanged = false;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting jms::setJmsArea() rfh_jms_area=%8.8X m_RFH_jms_len=%d jms_data_ccsid=%d jms_data_encoding=%d", (unsigned int)rfh_jms_area, m_RFH_jms_len, jms_data_ccsid, jms_data_encoding);

		// trace exit from setJmsArea
		pDoc->logTraceEntry(traceInfo);
	}
}

void jms::freeJmsArea()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering jms::freeJmsArea() rfh_jms_area=%8.8X m_RFH_jms_len=%d", (unsigned int)rfh_jms_area, m_RFH_jms_len);

		// trace entry to freeJmsArea
		pDoc->logTraceEntry(traceInfo);
	}

	if (rfh_jms_area != NULL)
	{
		rfhFree(rfh_jms_area);
	}
		
	rfh_jms_area = NULL;
	m_RFH_jms_len = 0;
	jms_data_ccsid = -1;
	jms_data_encoding = -1;
}

///////////////////////////////////////////////////////
//
// Routine to parse the jms folder in an RFH2 header.
// This folder is used for additional fields used
// in JMS messages.
//
///////////////////////////////////////////////////////

void jms::parseRFH2jms(unsigned char *rfhdata, int dataLen)

{
	bool	more;
	int		found;
	int		idx;
	char	*ptr;
	char	*endptr;
	char	tempRto[MAX_JMS_NAME] = { 0 };
	char	tempDst[MAX_JMS_NAME] = { 0 };
	char	tempCid[MAX_JMS_NAME] = { 0 };
	char	tempGid[MAX_JMS_NAME] = { 0 };
	char	tempSeq[MAX_JMS_NAME] = { 0 };
	char	tempExp[MAX_JMS_NAME] = { 0 };
	char	tempPri[MAX_JMS_NAME] = { 0 };
	char	tempDlv[MAX_JMS_NAME] = { 0 };
	char	tempTms[MAX_JMS_NAME] = { 0 };
	char	*tempUser;
	size_t  tempUserLen = 16384;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering jms::parseRFH2jms() rfhdata=%8.8X dataLen=%d", (unsigned int)rfhdata, dataLen);

		// trace entry to parseRFH2jms
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("jms rfhdata", rfhdata, dataLen);
	}

	// initialize fields
	idx = 0;
	tempUser = (char*)rfhMalloc(tempUserLen, "JMSTPUSR");
	memset(tempUser, 0, tempUserLen);

	// Search for the fields in the jms folder
	ptr = (char *)rfhdata + 5;

	// find the end of the jms folder
	endptr = ptr;
	while ((endptr < ((char *)rfhdata + dataLen - 5)) && 
		   (memcmp(endptr, RFH2_JMS_E, sizeof(RFH2_JMS_E) - 1)))
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
			if (memcmp(ptr, RFH2_JMS_E, sizeof(RFH2_JMS_E) - 1) == 0)
			{
				more = false;
			}
			else
			{
				// see if we recognize the tag
				found = 0;
				ptr = processTag(ptr, RFH2_JMS_DST_B, tempDst, sizeof(tempDst), &found);
				ptr = processTag(ptr, RFH2_JMS_RTO_B, tempRto, sizeof(tempRto), &found);
				ptr = processTag(ptr, RFH2_JMS_EXP_B, tempExp, sizeof(tempExp), &found);
				ptr = processTag(ptr, RFH2_JMS_CID_B, tempCid, sizeof(tempCid), &found);
				ptr = processTag(ptr, RFH2_JMS_GID_B, tempGid, sizeof(tempGid), &found);
				ptr = processTag(ptr, RFH2_JMS_SEQ_B, tempSeq, sizeof(tempSeq), &found);
				ptr = processTag(ptr, RFH2_JMS_TMS_B, tempTms, sizeof(tempTms), &found);
				ptr = processTag(ptr, RFH2_JMS_PRI_B, tempPri, sizeof(tempPri), &found);
				ptr = processTag(ptr, RFH2_JMS_DLV_B, tempDlv, sizeof(tempDlv), &found);

				if (0 == found)
				{
					ptr = parseRFH2misc(ptr, endptr, tempUser, RFH2_JMS_E, sizeof(tempUser) - 1, &idx);
				}
			}
		}
		else
		{
			// no more brackets - time to leave
			more = false;
		}
	}

	// capture the destination
	m_jms_dst = tempDst;

	// capture the reply to queue name
	m_jms_rto = tempRto;

	// capture the expiration time
	m_jms_expiration = tempExp;

	// capture the correlation id
	m_jms_correl_id = tempCid;

	// capture the group id
	m_jms_group_id = tempGid;

	// capture the group sequence number
	if (strlen(tempSeq) > 0)
	{
		m_jms_sequence = atoi(tempSeq);
	}

	// capture the timestamp
	m_jms_timestamp = tempTms;

	// capture the priority
	if (strlen(tempPri) > 0)
	{
		m_jms_priority = atoi(tempPri);
	}

	// capture the delivery mode
	if (strlen(tempDlv) > 0)
	{
		m_jms_delivery_mode = atoi(tempDlv);
	}

	if (idx > 0)
	{
		tempUser[idx] = 0;
		m_jms_user_def = tempUser;
	}

	// get the instance variables into the form data
	UpdateData(FALSE);

	if (tempUser) {
		rfhFree(tempUser);
	}

	if (pDoc->traceEnabled)
	{
		// trace exit from parseRFH2jms
		pDoc->logTraceEntry("Exiting parseRFH2jms");
	}
}

///////////////////////////////////////////////////////
//
// Routine to parse the folders in an RFH2 header
// that are not recognized.
//
///////////////////////////////////////////////////////

char * jms::parseRFH2misc(char * ptr,
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
		sprintf(traceInfo, "Entering jms::parseRFH2misc ptr=%8.8X index=%d", (unsigned int)ptr, (*index));

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
		sprintf(traceInfo, "Exiting jms::parseRFH2misc ptr=%8.8X index=%d", (unsigned int)ptr, (*index));

		// trace exit from parseRFH2misc
		pDoc->logTraceEntry(traceInfo);
	}

	return ptr;
}

//////////////////////////////////////////////////
//
// Routine to build a jms folder for the RFH header
//
//////////////////////////////////////////////////

int jms::buildJmsArea(int ccsid, int encoding)

{
	int				i;
	int				j;
	wchar_t			*ucsPtr;
	unsigned char	*tempPtr;
	char			*tempField;
	char			*tempValue;
	const size_t    bufLen = MAX_RFH_LENGTH + 4;
	char			traceInfo[128];					// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering jms::buildJmsArea() ccsid=%d encoding=%d", ccsid, encoding);

		// trace entry to buildJmsArea
		pDoc->logTraceEntry(traceInfo);
	}

	tempField = (char *)rfhMalloc(bufLen, "TMPFIELD");
	tempValue = (char *)rfhMalloc(bufLen, "TMPVALUE");
	memset(tempField, 0, bufLen);
	memset(tempValue, 0, bufLen);

	// Has the jms data been changed?
	if (jmsDataChanged || (jms_data_ccsid != ccsid) || (jms_data_encoding != encoding))
	{
		// data has changed - free the current jms area
		freeJmsArea();
	}

	if (NULL == rfh_jms_area)
	{
		// Add the first part of the XML headers
		strcpy(tempField, RFH2_JMS_B);

		if (m_jms_dst.GetLength() > 0)
		{
			strcat(tempField, RFH2_JMS_DST_B);
			replaceChars(m_jms_dst, tempValue);
			strcat(tempField, tempValue);
			strcat(tempField, RFH2_JMS_DST_E);
		}

		if (m_jms_rto.GetLength() > 0)
		{
			strcat(tempField, RFH2_JMS_RTO_B);
			replaceChars(m_jms_rto, tempValue);
			strcat(tempField, tempValue);
			strcat(tempField, RFH2_JMS_RTO_E);
		}

		if (m_jms_priority > 0)
		{
			strcat(tempField, RFH2_JMS_PRI_B);
			sprintf(tempValue, "%d", m_jms_priority);
			strcat(tempField, tempValue);
			strcat(tempField, RFH2_JMS_PRI_E);
		}

		if (m_jms_delivery_mode > 0)
		{
			strcat(tempField, RFH2_JMS_DLV_B);
			sprintf(tempValue, "%d", m_jms_delivery_mode);
			strcat(tempField, tempValue);
			strcat(tempField, RFH2_JMS_DLV_E);
		}

		if (m_jms_expiration.GetLength() > 0)
		{
			strcat(tempField, RFH2_JMS_EXP_B);
			replaceChars(m_jms_expiration, tempValue);
			strcat(tempField, tempValue);
			strcat(tempField, RFH2_JMS_EXP_E);
		}

		if (m_jms_correl_id.GetLength() > 0)
		{
			strcat(tempField, RFH2_JMS_CID_B);
			replaceChars(m_jms_correl_id, tempValue);
			strcat(tempField, tempValue);
			strcat(tempField, RFH2_JMS_CID_E);
		}

		if (m_jms_group_id.GetLength() > 0)
		{
			strcat(tempField, RFH2_JMS_GID_B);
			replaceChars(m_jms_group_id, tempValue);
			strcat(tempField, tempValue);
			strcat(tempField, RFH2_JMS_GID_E);
		}

		if (m_jms_sequence > 0)
		{
			strcat(tempField, RFH2_JMS_SEQ_B);
			sprintf(tempValue, "%d", m_jms_sequence);
			strcat(tempField, tempValue);
			strcat(tempField, RFH2_JMS_SEQ_E);
		}

		if (m_jms_timestamp.GetLength() > 0)
		{
			strcat(tempField, RFH2_JMS_TMS_B);
			replaceChars(m_jms_timestamp, tempValue);
			strcat(tempField, tempValue);
			strcat(tempField, RFH2_JMS_TMS_E);
		}

		if (m_jms_user_def.GetLength() > 0)
		{
			strcat(tempField, m_jms_user_def);
		}

		// Add the last part of the XML headers
		strcat(tempField, RFH2_JMS_E);

		// check if the data must be converted to UCS-2
		if (isUCS2(ccsid))
		{
			// convert the data to UCS-2
			// allocate a temporary area to hold the UCS-2 data
			tempPtr = (unsigned char *)rfhMalloc(2 * strlen(tempField) + 16, "TEMPPTRJ");

			if (tempPtr != 0)
			{
				// translate the data to UCS-2
				MultiByteToUCS2(tempPtr, 2 * strlen(tempField) + 2, (unsigned char *)tempField, strlen(tempField));

				// set the length of the new jms in bytes (each 16-bit character is 2 bytes)
				m_RFH_jms_len = roundLength2(tempPtr, NUMERIC_PC) * 2;

				// check if the data has to be reversed
				if (encoding != NUMERIC_PC)
				{
					// reverse the order of the bytes
					i = 0;
					j = m_RFH_jms_len >> 1;
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
				setJmsArea(tempPtr, m_RFH_jms_len, ccsid, encoding);

				// release the storage we acquired
				rfhFree(tempPtr);
			}
			else
			{
				if (pDoc->traceEnabled)
				{
					// malloc failed - write entry to trace
					pDoc->logTraceEntry("jms::buildJmsArea() malloc failed");
				}
			}
		}
		else
		{
			// Round it up to a multiple of 4 if necessary
			m_RFH_jms_len = roundLength((unsigned char *)tempField);

			// save the results
			setJmsArea((unsigned char *)tempField, m_RFH_jms_len, ccsid, encoding);
		}
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting jms::buildJmsArea() m_RFH_jms_len=%d", m_RFH_jms_len);

		// trace exit from buildJmsArea
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("jms data", (unsigned char *)rfh_jms_area, m_RFH_jms_len);
	}

	if (tempField) {
		rfhFree(tempField);
	}
	if (tempValue) {
		rfhFree(tempValue);
	}

	return m_RFH_jms_len;
}

char * jms::getJmsArea(int ccsid, int encoding)

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "jms::getJmsArea() returning rfh_jms_area=%8.8X m_RFH_jms_len=%d", (unsigned int)rfh_jms_area, m_RFH_jms_len);

		// trace exit from getJmsArea
		pDoc->logTraceEntry(traceInfo);
	}

	if (jmsDataChanged || (jms_data_ccsid != ccsid) || (jms_data_encoding != encoding))
	{
		// rebuild the jms data area
		buildJmsArea(ccsid, encoding);
	}

	return (char *)rfh_jms_area;
}

void jms::clearJMSdata()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to clearJMSdata
		pDoc->logTraceEntry("Entering jms::clearJMSdata()");
	}

	// clear the jms folder fields
	m_jms_dst.Empty();
	m_jms_rto.Empty();
	m_jms_timestamp.Empty();
	m_jms_priority = 0;
	m_jms_expiration.Empty();
	m_jms_sequence = 0;
	m_jms_group_id.Empty();
	m_jms_correl_id.Empty();
	m_jms_delivery_mode = 0;
	m_jms_user_def.Empty();
	m_jms_msg_type = -1;
	jmsDataChanged = false;

	// get the instance variables into the form data
	UpdateData(FALSE);
}

void jms::setJMSMsgType(const char *domain)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering jms::setJMSMsgType domain=%s m_jms_msg_type=%d", domain, m_jms_msg_type);

		// trace entry to setJMSMsgType
		pDoc->logTraceEntry(traceInfo);
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	if (strcmp(domain, MQMCD_DOMAIN_JMS_NONE) == 0)
	{
		m_jms_msg_type = JMS_NONE;
	}
	else if (strcmp(domain, MQMCD_DOMAIN_JMS_TEXT) == 0)
	{
		m_jms_msg_type = JMS_TEXT;
	}
	else if (strcmp(domain, MQMCD_DOMAIN_JMS_BYTES) == 0)
	{
		m_jms_msg_type = JMS_BYTES;
	}
	else if (strcmp(domain, MQMCD_DOMAIN_JMS_STREAM) == 0)
	{
		m_jms_msg_type = JMS_STREAM;
	}
	else if (strcmp(domain, MQMCD_DOMAIN_JMS_MAP) == 0)
	{
		m_jms_msg_type = JMS_MAP;
	}
	else if (strcmp(domain, MQMCD_DOMAIN_JMS_OBJECT) == 0)
	{
		m_jms_msg_type = JMS_OBJECT;
	}
	else
	{
		m_jms_msg_type = -1;
	}

	// update the form data from the instance variables
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting jms::setJMSMsgType m_jms_msg_type=%d", m_jms_msg_type);

		// trace exit from setJMSMsgType
		pDoc->logTraceEntry(traceInfo);
	}
}

BOOL jms::wasDataChanged()

{
	return jmsDataChanged;
}
