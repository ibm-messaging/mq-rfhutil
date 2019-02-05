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

// HexFind.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "rfhutil.h"
#include "HexFind.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHexFind dialog


CHexFind::CHexFind(CWnd* pParent /*=NULL*/)
	: CDialog(CHexFind::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHexFind)
	m_hex_value = _T("");
	m_direction = -1;
	//}}AFX_DATA_INIT

	parent = pParent;
	terminating = FALSE;
	findPressed = FALSE;
	m_direction = SEARCH_DOWN;		// down is default
	helperMsg = NULL;
}


void CHexFind::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHexFind)
	DDX_Text(pDX, IDC_HEX_VALUE, m_hex_value);
	DDV_MaxChars(pDX, m_hex_value, 16);
	DDX_Radio(pDX, IDC_HEXFIND_UP, m_direction);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHexFind, CDialog)
	//{{AFX_MSG_MAP(CHexFind)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CHexFind::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CHexFind::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHexFind message handlers

BOOL CHexFind::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

void CHexFind::OnOK() 

{
	// handler for the enter key and the OK button
	findPressed = TRUE;

	// reset the previous error message
	SetDlgItemText(IDC_HEX_EDIT_ERRMSG, "");

	// validate the data
	if (hexDatavalid())
	{
		::SendMessage(parent->m_hWnd, helperMsg, 0, 0);
	}
	else
	{
		Beep(500, 500);
	}
}

void CHexFind::OnCancel() 
{
	// handler for the Esc key and the cancel button
	terminating = TRUE;
	findPressed = FALSE;

	::SendMessage(parent->m_hWnd, helperMsg, 0, 0);

	delete(this);
}

BOOL CHexFind::OnInitDialog()

{
	CEdit	*cedit;

	// register the windows message for the modeless dialog
	helperMsg = ::RegisterWindowMessage( HEX_FINDER_DIALOG );
	
	// set the variables in the control, to initialize the radio buttons
	EnableToolTips(TRUE);
	UpdateData(FALSE);

	cedit = (CEdit *)GetDlgItem(IDC_HEX_VALUE);
	cedit->SetFocus();
	return FALSE;

//	return TRUE;
}

int CHexFind::getHexValue(unsigned char * value, int maxLen)

{
	int		len;
	LPTSTR		ptr;

	UpdateData(TRUE);

	len = m_hex_value.GetLength();		// get the number of hex digits
	len >>= 1;							// divide by 2

	if (len > maxLen)
	{
		// make sure we do not overflow the buffer
		len = maxLen;
	}

	ptr = m_hex_value.GetBuffer(16);
	HexToAscii((unsigned char *)ptr, len, value);
	m_hex_value.ReleaseBuffer();

	UpdateData(FALSE);

	return len;
}

void CHexFind::HexToAscii(unsigned char *dati, unsigned int pl, unsigned char *dato)

{
	unsigned int i;
	unsigned int buffer;
	char	ch;

	buffer = 0;
	i = 0;
	while (buffer < pl)
	{
		ch = getHexCharValue(dati[i++]) << 4;
		ch += getHexCharValue(dati[i++]);
		dato[buffer++] = ch;
	}
}

char CHexFind::getHexCharValue(unsigned char charIn)

{
	char	ch;
	char	result=0;

	if ((charIn > '0') && (charIn <= '9'))
	{
		result = charIn & 15;
	}
	else
	{
		ch = toupper(charIn);
		if ((ch >= 'A') && (ch <= 'F'))
		{
			result = ch - 'A' + 10;
		}
	}

	return result;
}

BOOL CHexFind::isTerminating()

{
	return terminating;
}

BOOL CHexFind::FindNext()

{
	return findPressed;
}

BOOL CHexFind::hexDatavalid()

{
	int		len;
	int		i;
	char	*ptr;
	BOOL	result=TRUE;

	// get the data into the variables
	UpdateData(TRUE);

	len = m_hex_value.GetLength();

	if (0 == len)
	{
		SetDlgItemText(IDC_HEX_EDIT_ERRMSG, "No data entered");
		return FALSE;
	}

	// check if the data length is a multiple of two
	if ((len & 1) == 0)
	{
		if (len > 0)
		{
			// get a pointer to the data
			ptr = m_hex_value.GetBuffer(16);

			// check each character
			i = 0;
			while (result && (i < len))
			{
				if (((ptr[i] >= '0') && (ptr[i] <='9')) || ((ptr[i] >= 'A') && (ptr[i] <= 'F')))
				{
					i++;
				}
				else
				{
					result = FALSE;
				}
			}

			if (!result)
			{
				SetDlgItemText(IDC_HEX_EDIT_ERRMSG, "Invalid hex character(s)");
			}

			// done with the pointer
			m_hex_value.ReleaseBuffer();
		}
		else
		{
			// no data to process
			SetDlgItemText(IDC_HEX_EDIT_ERRMSG, "Please enter some data");
			result = FALSE;
		}
	}
	else
	{
		// odd number of characters
		SetDlgItemText(IDC_HEX_EDIT_ERRMSG, "Odd number of characters");
		result = FALSE;
	}

	return result;
}

BOOL CHexFind::SearchDown()

{
	UpdateData(TRUE);

	return m_direction;
}
