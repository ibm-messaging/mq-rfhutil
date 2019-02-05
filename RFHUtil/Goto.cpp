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

// Goto.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "rfhutil.h"
#include "Goto.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGoto dialog


CGoto::CGoto(CWnd* pParent /*=NULL*/)
	: CDialog(CGoto::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGoto)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	maxOffset = 0;
	offset = 0;
}


void CGoto::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGoto)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGoto, CDialog)
	//{{AFX_MSG_MAP(CGoto)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CGoto::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CGoto::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoto message handlers

BOOL CGoto::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

void CGoto::OnOK() 

{
	// Handler for the enter key or OK button
	char			strOfs[32];
	unsigned int	i;
	BOOL			valid=TRUE;

	// reset the error message
	SetDlgItemText(IDC_GOTO_ERRMSG, "");

	// validate the data is numeric and within range
	memset(strOfs, 0, sizeof(strOfs));
	GetDlgItemText(IDC_GOTO_OFFSET, strOfs, sizeof(strOfs));
	
	i = 0;
	while ((i < strlen(strOfs)) && valid)
	{
		if ((strOfs[i] >= '0') && (strOfs[i] <= '9'))
		{
			i++;
		}
		else
		{
			valid = FALSE;
		}
	}
	
	if (valid)
	{
		// save the result in an instance variable for later retrieval
		offset = atoi(strOfs);

		if ((offset >= 0) && (offset <= maxOffset))
		{
			offset--;

			CDialog::OnOK();
		}
		else
		{
			SetDlgItemText(IDC_GOTO_ERRMSG, "Offset too large or < 0");
		}
	}
	else
	{
		SetDlgItemText(IDC_GOTO_ERRMSG, "Offset must be a number");
	}
}

CGoto::CGoto(CWnd *pParent, int maxOffset)
		: CDialog(CGoto::IDD, pParent)
{
	this->maxOffset = maxOffset;
	this->offset = 0;
}

int CGoto::getOffset()

{
	return offset;
}

BOOL CGoto::OnInitDialog() 
{
	CString	tempStr;
	CEdit	*cedit;

	CDialog::OnInitDialog();
	
	// tool tips are provided so the support must be initialized
	EnableToolTips(TRUE);

	tempStr.Format("Enter offset (0 - %d)", maxOffset);
	SetDlgItemText(IDC_GOTO_LABEL, tempStr);
	
	// set the focus to a control so the tab key will move between controls
	cedit = (CEdit *)GetDlgItem(IDC_GOTO_OFFSET);
	cedit->SetFocus();
	return FALSE;

//	return TRUE;  // return TRUE unless you set the focus to a control
}
