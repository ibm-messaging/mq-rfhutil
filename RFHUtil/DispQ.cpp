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

// DispQ.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "DispQ.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDispQ dialog

#define MAX_DISPLAY_MSGS	50000

CDispQ::CDispQ(CWnd* pParent /*=NULL*/)
	: CDialog(CDispQ::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDispQ)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// default to first line of display
	m_selectedLine = 0;

	// set the previous message id to a default value
	memcpy(m_prev_msg_id, MQMI_NONE, MQ_MSG_ID_LENGTH);

	// set a window title
	memset(strTitle, 0, sizeof(strTitle));

	// initialize the message id pointer
	m_msg_id_table = NULL;
	m_msg_id_count = 0;
}


CDispQ::~CDispQ()

{
}

void CDispQ::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDispQ)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDispQ, CDialog)
	//{{AFX_MSG_MAP(CDispQ)
	ON_BN_CLICKED(IDC_DISPQ_CLOSE, OnDispqClose)
	ON_BN_CLICKED(IDC_DISPQ_READQ, OnDispqReadq)
	ON_BN_CLICKED(IDC_DISPQ_START_BROWSE, OnDispqStartBrowse)
	ON_BN_CLICKED(IDC_DISPQ_BROWSEQ, OnDispqBrowseq)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_DISPQ_MSGS, OnItemchangedDispqMsgs)
	//}}AFX_MSG_MAP
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchangedDispqMsgs)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDispQ message handlers

BOOL CDispQ::PreCreateWindow(CREATESTRUCT& cs) 

{
	// initialize the window title
	if( !CDialog::PreCreateWindow(cs) )
		return FALSE;

	cs.lpszName = strTitle;

	return TRUE;
}

BOOL CDispQ::OnInitDialog() 

{
	MY_TIME_T	startTime=0;			// starting time
	MY_TIME_T	currTime=-1;			// current value of high resolution timer
	MY_TIME_T	endTime=0;				// time when should give up reading messages because it is taking too long
	MY_TIME_T	lastMsgTime=0;			// time when previous message was read
	__int64		freq=1;					// frequency of high resolution timer (ticks per second)
	MQLONG		rc=MQRC_NONE;			// MQ reason code
	MQLONG		msgLen=0;				// message length
	CListCtrl	*lc=NULL;				// pointer to list control
	MQMD2		mqmd={MQMD2_DEFAULT};	// mqmd V2 so that group id, etc are received in the MQMD
	double		elapsed;				// total time to get messages
	int			maxDisplayTime;			// maximum number of seconds to wait while reading messages from the queue
	char		elapsedStr[32];			// work area for elapsed time
	char		lastMsgTimeStr[32];		// work area for elapsed time to read last message
	char		remainingStr[32];		// work area for remaining time
	char		traceInfo[512];			// work variable to build trace message


	// get the maximum display time setting
	maxDisplayTime = ((CRfhutilApp *)AfxGetApp())->maxDispTime;

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CDispQ::OnInitDialog() QMname=%s Queue=%s maxDisplayTime=%d", (LPCTSTR)m_qmgr_name, (LPCTSTR)m_q_name, maxDisplayTime);

		// trace entry to OnInitDialog
		pDoc->logTraceEntry(traceInfo);
	}

	CDialog::OnInitDialog();

	// initialize work area
	memset(lastMsgTimeStr, 0, sizeof(lastMsgTimeStr));
	
	// set the window style
//	ModifyStyle(0, (DWORD)WS_TABSTOP | WS_GROUP);

	// set the dialog title
	sprintf(strTitle, "Display messages in Queue %s", (LPCTSTR)m_q_name);
	SetWindowText(strTitle);

	// Enable tool tips for this dialog
	EnableToolTips(TRUE);
	
	// get a pointer to the list control
	lc = (CListCtrl *)GetDlgItem(IDC_DISPQ_MSGS);

	// make sure it worked
	if (lc != NULL)
	{
		// set the columns
		lc->InsertColumn(0, "Pos", LVCFMT_LEFT, 50, 0);
		lc->InsertColumn(1, "Length", LVCFMT_LEFT, 90, 0);
		lc->InsertColumn(2, "Format", LVCFMT_LEFT, 80, 0);
		lc->InsertColumn(3, "GS", LVCFMT_LEFT, 30, 0);
		lc->InsertColumn(4, "User Id", LVCFMT_LEFT, 90, 0);
		lc->InsertColumn(5, "Put Date/Time", LVCFMT_LEFT, 130, 0);
		lc->InsertColumn(6, "Application", LVCFMT_LEFT, 200, 0);
		lc->InsertColumn(7, "Type", LVCFMT_LEFT, 40, 0);
		lc->InsertColumn(8, "Pr", LVCFMT_LEFT, 30, 0);
		lc->InsertColumn(9, "CCSID", LVCFMT_LEFT, 50, 0);
		lc->InsertColumn(10, "Encoding", LVCFMT_LEFT, 50, 0);
		lc->InsertColumn(11, "Reply To Queue", LVCFMT_LEFT, 250, 0);
		lc->InsertColumn(12, "Reply To QMgr", LVCFMT_LEFT, 250, 0);
		lc->InsertColumn(13, "Message ID", LVCFMT_LEFT, 315, 0);
		lc->InsertColumn(14, "Correlation ID", LVCFMT_LEFT, 315, 0);
		lc->InsertColumn(15, "Group ID", LVCFMT_LEFT, 315, 0);

		// set the extended style to allow selection
		lc->SetExtendedStyle( lc->GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE);

		// get the current starting time
		startTime = GetTime();
		if (QueryPerformanceCounter((LARGE_INTEGER *)&startTime))
		{
			// get the current timer setting
			currTime = startTime;
			lastMsgTime = startTime;

			// get the fregquency of the high resolution timer
			QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

			// calculate the ending time to limit the total time to 15 seconds
			endTime = startTime + (maxDisplayTime * freq);
		}
		else
		{
			startTime = 0;
		}

		// start a browse operation to retrive the message data
		rc = pDoc->startMsgDisplay((LPCTSTR)m_qmgr_name, (LPCTSTR)m_q_name, &mqmd, &msgLen);

		if (MQRC_NONE == rc)
		{
			// allocate storage for up to 50000 message ids
			m_msg_id_table = (char *)rfhMalloc((MAX_DISPLAY_MSGS + 1) * MQ_MSG_ID_LENGTH, "MSGIDTAB");

			if (NULL == m_msg_id_table)
			{
				// indicate error
				pDoc->m_error_msg = "Memory allocation failed";
				pDoc->updateMsgText();
			}
			else
			{
				// insert the results into the list control
				insertMessage(lc, &mqmd, msgLen);

				// read the next message
				while ((MQRC_NONE == rc) && (currTime < endTime) && (m_msg_id_count <= MAX_DISPLAY_MSGS))
				{
					// get the next message from the queue
					rc = pDoc->getNextMsgDisplay(&mqmd, &msgLen);

					// check if it worked
					if (MQRC_NONE == rc)
					{
						// insert the next row
						insertMessage(lc, &mqmd, msgLen);

						// check if the current time value should be updated
						// only check every 50 messages to minimize overhead
						if (((m_msg_id_count %50) == 0) || (m_msg_id_count < 50))
						{
							// get the current value of the high performance timer to limit queries to 15 seconds
							// this amount will be used to determine if the maximum time limit has been exceeded
							// this is done to prevent the operation from taking an excessive amount of time, since
							// the user cannot do anything else once a display operation is started
							QueryPerformanceCounter((LARGE_INTEGER *)&currTime);
						}

						// check if verbose trace is on
						if (pDoc->traceEnabled && pDoc->verboseTrace)
						{
							// get the performance counter every message rather than once every 50 messages
							QueryPerformanceCounter((LARGE_INTEGER *)&currTime);

							// calculate the time interval from the beginning
							elapsed = DiffTime(startTime, currTime);
							formatTimeDiffSecs(elapsedStr, elapsed);

							// calculate the time interval since the last message
							elapsed = DiffTime(lastMsgTime, currTime);
							formatTimeDiffSecs(lastMsgTimeStr, elapsed);

							// update the time of the last message
							lastMsgTime = currTime;

							// calculate the remaining time
							elapsed = DiffTime(currTime, endTime);
							formatTimeDiffSecs(remainingStr, elapsed);

							// create the trace line
							sprintf(traceInfo, "CDispQ::OnInitDialog() total elapsed=%s since last msg=%s remaining=%s m_msg_id_count=%d currTime=%I64d endTime=%I64d", elapsedStr, lastMsgTimeStr, remainingStr, m_msg_id_count, currTime, endTime);

							// trace exit from OnInitDialog
							pDoc->logTraceEntry(traceInfo);
						}
					}
				}

				// end the browse operation if necessary by closing the queue
				pDoc->endBrowse(true);
			}
		}
		else
		{
			// no messages were retrieved - indicate that nothing is selected
			m_sel = -1;
			m_selCount = -1;
		}
	}

	if (pDoc->traceEnabled)
	{
		// calculate the time interval
		elapsed = DiffTime(startTime, currTime);
		formatTimeDiffSecs(elapsedStr, elapsed);

		// create the trace line
		sprintf(traceInfo, "Exiting CDispQ::OnInitDialog() elapsed=%s m_sel=%d m_selCount=%d m_msg_id_count=%d startTime=%I64d currTime=%I64d endTime=%I64d currTime-startTime=%I64d endTime-startTime=%I64d", elapsedStr, m_sel, m_selCount, m_msg_id_count, startTime, currTime, endTime, currTime-startTime, endTime-startTime);

		// trace exit from OnInitDialog
		pDoc->logTraceEntry(traceInfo);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDispQ::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

void CDispQ::insertMessage(CListCtrl *lc, MQMD2 *mqmd, int msgLen)

{
	MQLONG		flags=0;
	char		tempStr[64];		// label for row within list control

	// insert the row into the list oontrol
	sprintf(tempStr, "Item%d", m_msg_id_count);
	lc->InsertItem(m_msg_id_count, tempStr);

	// insert the column text
	sprintf(tempStr, "%d", m_msg_id_count + 1);
	lc->SetItemText(m_msg_id_count, 0, tempStr);

	sprintf(tempStr, "%d", msgLen);
	lc->SetItemText(m_msg_id_count, 1, tempStr);

	memset(tempStr, 0, sizeof(tempStr));
	memcpy(tempStr, mqmd->Format, MQ_FORMAT_LENGTH);
	Rtrim(tempStr);
	lc->SetItemText(m_msg_id_count, 2, tempStr);

	// get the message flags
	flags = mqmd->MsgFlags;
	strcpy(tempStr, "  ");
	if ((flags & MQMF_MSG_IN_GROUP) > 0)
	{
		// message is part of a message group
		tempStr[0] = 'G';
	}

	if ((flags & MQMF_LAST_MSG_IN_GROUP) > 0)
	{
		// message is last message in a message group
		tempStr[0] = 'L';
	}

	if ((flags & MQMF_SEGMENT) > 0)
	{
		// message is segmented
		tempStr[1] = 'S';
	}

	if ((flags & MQMF_LAST_SEGMENT) > 0)
	{
		// message is the last message in a segmented message
		tempStr[1] = 'L';
	}

	lc->SetItemText(m_msg_id_count, 3, tempStr);

	memset(tempStr, 0, sizeof(tempStr));
	memcpy(tempStr, mqmd->UserIdentifier, MQ_USER_ID_LENGTH);
	Rtrim(tempStr);
	lc->SetItemText(m_msg_id_count, 4, tempStr);

	// format the date and time
	formatDateTime(tempStr, mqmd->PutDate, mqmd->PutTime);
	lc->SetItemText(m_msg_id_count, 5, tempStr);

	memset(tempStr, 0, sizeof(tempStr));
	memcpy(tempStr, mqmd->PutApplName, MQ_PUT_APPL_NAME_LENGTH);
	Rtrim(tempStr);
	lc->SetItemText(m_msg_id_count, 6, tempStr);

	sprintf(tempStr, "%d", mqmd->MsgType);
	lc->SetItemText(m_msg_id_count, 7, tempStr);

	sprintf(tempStr, "%d", mqmd->Priority);
	lc->SetItemText(m_msg_id_count, 8, tempStr);

	sprintf(tempStr, "%d", mqmd->CodedCharSetId);
	lc->SetItemText(m_msg_id_count, 9, tempStr);

	sprintf(tempStr, "%d", mqmd->Encoding);
	lc->SetItemText(m_msg_id_count, 10, tempStr);

	memset(tempStr, 0, sizeof(tempStr));
	memcpy(tempStr, mqmd->ReplyToQ, MQ_Q_NAME_LENGTH);
	Rtrim(tempStr);
	lc->SetItemText(m_msg_id_count, 11, tempStr);

	memset(tempStr, 0, sizeof(tempStr));
	memcpy(tempStr, mqmd->ReplyToQMgr, MQ_Q_MGR_NAME_LENGTH);
	Rtrim(tempStr);
	lc->SetItemText(m_msg_id_count, 12, tempStr);

	// convert the message id to hex
	memset(tempStr, 0, sizeof(tempStr));
	AsciiToHex((const unsigned char *)&mqmd->MsgId, MQ_MSG_ID_LENGTH, (unsigned char *)&tempStr);
	lc->SetItemText(m_msg_id_count, 13, tempStr);

	// convert the correlation id to hex
	memset(tempStr, 0, sizeof(tempStr));
	AsciiToHex((const unsigned char *)&mqmd->CorrelId, MQ_CORREL_ID_LENGTH, (unsigned char *)&tempStr);
	lc->SetItemText(m_msg_id_count, 14, tempStr);

	// convert the group id to hex
	memset(tempStr, 0, sizeof(tempStr));
	AsciiToHex((const unsigned char *)&mqmd->GroupId, MQ_GROUP_ID_LENGTH, (unsigned char *)&tempStr);
	lc->SetItemText(m_msg_id_count, 15, tempStr);

	// copy the message id from the MQMD into the table
	memcpy(m_msg_id_table + (m_msg_id_count * MQ_MSG_ID_LENGTH), mqmd->MsgId, MQ_MSG_ID_LENGTH);
	m_msg_id_count++;
}

void CDispQ::OnDispqClose() 

{
	// free any acquired storage
	freeMsgIdTable();

	CDialog::OnCancel();
}

void CDispQ::OnCancel() 
{
	// free any acquired storage
	freeMsgIdTable();

	CDialog::OnCancel();
}

void CDispQ::OnDispqReadq() 

{
	m_read_type = READQ;

	// Update the message id display in the proper format
	setMsgId(FALSE);
	
	CDialog::OnOK();
}

void CDispQ::OnDispqStartBrowse() 

{
	m_read_type = STARTBR;
	
	// Update the message id display in the proper format
	setMsgId(TRUE);

	CDialog::OnOK();
}

void CDispQ::OnDispqBrowseq() 

{
	m_read_type = BROWSEQ;
	
	// Update the message id display in the proper format
	setMsgId(FALSE);

	CDialog::OnOK();
}

void CDispQ::OnItemchangedDispqMsgs(NMHDR* pNMHDR, LRESULT* pResult) 

{
	CListCtrl			*lc=NULL;					// pointer to list control

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
	// get a pointer to the list control
	lc = ((CListCtrl *)GetDlgItem(IDC_DISPQ_MSGS));

	// make sure it worked
	if (lc != NULL)
	{
		// get the selection mark
		m_sel = lc->GetSelectionMark();
		m_selCount = lc->GetSelectedCount();
	}
	
	*pResult = 0;
}


void CDispQ::freeMsgIdTable()

{
	// was any storage acquired?
	if (m_msg_id_table != NULL)
	{
		// free the acquired storage
		rfhFree(m_msg_id_table);
		m_msg_id_table = NULL;
	}
}

void CDispQ::setMsgId(BOOL setPrev)

{
	CListCtrl			*lc=NULL;					// pointer to list control

	// get a pointer to the list control
	lc = ((CListCtrl *)GetDlgItem(IDC_DISPQ_MSGS));

	if (lc != NULL)
	{
		// see if anything is selected
		m_sel = lc->GetSelectionMark();
		m_selCount = lc->GetSelectedCount();
	}

	// check if a message id table was acquired
	if (m_msg_id_table != NULL)
	{
		// check if anything was selected
		if ((-1 == m_sel) || (0 == m_selCount))
		{
			// default to the first message
			m_sel = 0;
		}

		// copy the corresponding message id to the message id that will be passed back
		memcpy(m_msg_id, m_msg_id_table + (MQ_MSG_ID_LENGTH * m_sel), MQ_MSG_ID_LENGTH);

		// check if a browse operation is starting
		if (setPrev)
		{
			// check if the browse is starting at the first message
			if (m_sel > 0)
			{
				// copy the message id of the previous message
				memcpy(m_prev_msg_id, m_msg_id_table + (MQ_MSG_ID_LENGTH * (m_sel - 1)), MQ_MSG_ID_LENGTH);
			}
		}

		// release the message id table
		freeMsgIdTable();
	}
}
