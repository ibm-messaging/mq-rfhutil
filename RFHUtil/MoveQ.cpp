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

// MoveQ.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "MoveQ.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MoveQ dialog


MoveQ::MoveQ(CWnd* pParent /*=NULL*/)
	: CDialog(MoveQ::IDD, pParent)
{
	//{{AFX_DATA_INIT(MoveQ)
	m_max_count = _T("");
	m_new_queue_name = _T("");
	m_remove_dlq = FALSE;
	m_start_msg = _T("");
	m_pass_all = TRUE;
	//}}AFX_DATA_INIT

	// initialize counts of the number of queue names added and skipped
	added = 0;
	skipped = 0;
	pDoc = NULL;
	m_hAccel = NULL;
}


void MoveQ::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MoveQ)
	DDX_Text(pDX, IDC_MOVEQ_MAX_COUNT, m_max_count);
	DDX_CBString(pDX, IDC_MOVEQ_QUEUE_NAME, m_new_queue_name);
	DDX_Check(pDX, IDC_MOVEQ_REMOVE_DLQ, m_remove_dlq);
	DDX_Text(pDX, IDC_MOVEQ_START_MSG, m_start_msg);
	DDX_Check(pDX, IDC_MOVEQ_PASS_ALL, m_pass_all);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(MoveQ, CDialog)
	//{{AFX_MSG_MAP(MoveQ)
	ON_COMMAND(IDC_MOVEQ_REMOVE_DLQ_KEY, OnMoveqRemoveDlqKey)
	ON_UPDATE_COMMAND_UI(IDC_MOVEQ_REMOVE_DLQ_KEY, OnUpdateMoveqRemoveDlqKey)
	ON_COMMAND(IDC_MOVEQ_PASS_ALL_KEY, OnMoveqPassAllKey)
	ON_UPDATE_COMMAND_UI(IDC_MOVEQ_PASS_ALL_KEY, OnUpdateMoveqPassAllKey)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, MoveQ::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, MoveQ::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MoveQ message handlers

BOOL MoveQ::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

BOOL MoveQ::OnInitDialog()

{
	const char *	qNamePtr=NULL;
	CComboBox		*cb=NULL;
	char			traceInfo[512];

	CDialog::OnInitDialog();
	
	// tool tips are provided for this dialog and must be initialized
	EnableToolTips(TRUE);

	// load the accelerator key table for this dialog
	m_hAccel = ::LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MOVEQ_ACCELERATOR));

	// load the combo box with queue names for the current queue manager
	// load the queue names into the Combobox control
	cb = (CComboBox *)GetDlgItem(IDC_MOVEQ_QUEUE_NAME);

	// check if it worked
	if (cb != NULL)
	{
		cb->ResetContent();

		// see if we can find a list of queue names for the queue manager
		if (pDoc->m_QM_name.GetLength() > 0)
		{
			qNamePtr = pDoc->getQueueNamesListPtr((LPCTSTR)pDoc->m_QM_name);
		}
		else
		{
			qNamePtr = pDoc->getQueueNamesListPtr((LPCTSTR)pDoc->defaultQMname);
		}

		if (qNamePtr != NULL)
		{
			while (qNamePtr[0] != 0)
			{
				if ((pDoc->m_show_system_queues) || (memcmp(qNamePtr + 1, "SYSTEM.", 7) != 0))
				{
					cb->AddString(qNamePtr + 1);
					added++;						// in case trace is turned on

					if (pDoc->traceEnabled)
					{
						// create the trace line
						sprintf(traceInfo, " MoveQ::OnInitDialog() adding=%s", qNamePtr);

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
						sprintf(traceInfo, " MoveQ::OnInitDialog ignoring=%s", qNamePtr);

						// trace entry to updateIdFields
						pDoc->logTraceEntry(traceInfo);
					}
				}

				qNamePtr = Names::getNextName(qNamePtr);
			}
		}
	}
					

	return TRUE;
}

BOOL MoveQ::PreTranslateMessage(MSG* pMsg) 

{
	if ((WM_KEYFIRST <= pMsg->message) && (pMsg->message <= WM_KEYLAST))
	{
		// check for an accelerator key
		if (m_hAccel && ::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
		{
			// accelerator key was handled so return TRUE
			return TRUE;
		}
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

void MoveQ::OnMoveqRemoveDlqKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_remove_dlq)
	{
		m_remove_dlq = FALSE;
	}
	else
	{
		m_remove_dlq = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void MoveQ::OnUpdateMoveqRemoveDlqKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}

void MoveQ::OnMoveqPassAllKey() 

{
	// get the data from the controls into the instance variables
	UpdateData (TRUE);

	// process the accelerator key
	if (m_pass_all)
	{
		m_pass_all = FALSE;
	}
	else
	{
		m_pass_all = TRUE;
	}

	// Update the controls
	UpdateData (FALSE);
}

void MoveQ::OnUpdateMoveqPassAllKey(CCmdUI* pCmdUI) 

{
	// enable the accelerator key
	pCmdUI->Enable(TRUE);
}
