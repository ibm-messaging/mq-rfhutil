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

// DisplayQ.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "rfhutil.h"
#include "DisplayQ.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisplayQ dialog


CDisplayQ::CDisplayQ(CWnd* pParent, const char * msgData, const char * dispName)
	: CDialog(CDisplayQ::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDisplayQ)
	m_msg_data = _T("");
	m_read_type = 1;
	//}}AFX_DATA_INIT

	m_msg_data = msgData;
	m_selectedLine = -1;
	if (dispName != NULL)
	{
		sprintf(strTitle, "Display Queue Contents (%s)", dispName);
	}
	else
	{
		strcpy(strTitle, "Display Queue Contents");
	}

	// create a fixed font for the data display edit box to use
	int i= m_fixed_font.CreateFont (-12,
									  0,
									  0,
									  0,
									  FW_NORMAL,
									  0,
									  0,
									  0,
									  ANSI_CHARSET,
									  3,
									  2,
									  1,
									  FF_MODERN | FIXED_PITCH,
									  "Courier New");
}


void CDisplayQ::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDisplayQ)
	DDX_Text(pDX, IDC_DISPLAYQ_MSGS, m_msg_data);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDisplayQ, CDialog)
	//{{AFX_MSG_MAP(CDisplayQ)
	ON_BN_CLICKED(IDC_DISPLAYQ_READQ, OnDisplayqReadq)
	ON_BN_CLICKED(IDC_DISPLAYQ_BROWSE_MSG, OnDisplayqBrowseMsg)
	ON_BN_CLICKED(IDC_DISPLAYQ_START_BROWSE, OnDisplayqStartBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisplayQ message handlers

void CDisplayQ::setFixedFont()

{
	// Set a fixed font for the message data edit box
	((CEdit *)GetDlgItem(IDC_DISPLAYQ_MSGS))->SetFont(&m_fixed_font,TRUE);
	((CStatic *)GetDlgItem(IDC_DISPLAYQ_HEADER))->SetFont(&m_fixed_font,TRUE);
}

BOOL CDisplayQ::OnInitDialog()

{
	CDialog::OnInitDialog();

	// use a fixed font
	setFixedFont();

	// set initial selection
	((CEdit *)GetDlgItem(IDC_DISPLAYQ_MSGS))->SetSel(-1, 0);

	this->SetWindowText(strTitle);

#ifdef SAFEMODE
	((CButton *)GetDlgItem(IDC_DISPLAYQ_READQ))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_DISPLAYQ_READQ))->ShowWindow(SW_HIDE);
#endif

	return TRUE;
}

BOOL CDisplayQ::PreCreateWindow(CREATESTRUCT& cs) 

{
	// initialize the window title
	if( !CDialog::PreCreateWindow(cs) )
		return FALSE;

	cs.lpszName = strTitle;

	return TRUE;
}

void CDisplayQ::OnDisplayqReadq() 

{
	setSelectedLine();

	m_read_type = READQ;

	CDialog::OnOK();
}

void CDisplayQ::OnDisplayqBrowseMsg() 

{
	setSelectedLine();

	m_read_type = BROWSEQ;

	CDialog::OnOK();
}

void CDisplayQ::OnDisplayqStartBrowse() 

{
	setSelectedLine();

	m_read_type = STARTBR;

	CDialog::OnOK();
}

void CDisplayQ::setSelectedLine()

{
	int				startChar=0;
	int				endChar=0;
	CEdit			*edit;

	// Update the control variables to capture the data from the dialog
	UpdateData (TRUE);

	// get a pointer to the dialog item
	edit = (CEdit *)GetDlgItem(IDC_DISPLAYQ_MSGS);

	if (edit != NULL)
	{
		edit->GetSel(startChar, endChar);
		if (startChar >= 0)
		{
			m_selectedLine = edit->LineFromChar(startChar);
		}
	}
}
