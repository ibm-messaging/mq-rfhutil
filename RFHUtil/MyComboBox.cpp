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

// MyComboBox.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "MyComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////
// virtual key definition for Ctrl-V
// used to trap paste operations
//////////////////////////////////////
#define CTRL_V		22
#define VK_BACKSP	8

/////////////////////////////////////////////////////////////////////////////
// MyComboBox

MyComboBox::MyComboBox()
{
	// initialize auto completion variable
	m_bAutoComplete = TRUE;
}

MyComboBox::~MyComboBox()
{
}


BEGIN_MESSAGE_MAP(MyComboBox, CComboBox)
	//{{AFX_MSG_MAP(MyComboBox)
	ON_WM_KEYDOWN()
    ON_CONTROL_REFLECT(CBN_EDITUPDATE, OnEditUpdate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MyComboBox message handlers

void MyComboBox::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 

{
	// Handle the backspace key so it works in a ComboBox
	int		nStart=0;
	int		nEnd=0;
	CString value;

	// This routine is necessary to retain the caret location after
	// the delete is pressed.
	switch (nChar)
	{
	case VK_BACK:
		{
			// handle the backspace key
			// get the current location of the caret
			DWORD wSel = GetEditSel();
			nStart = HIWORD(wSel);
			nEnd = LOWORD(wSel);

			// check if any characters are selected
			if (nStart == nEnd)
			{
				// nothing selected so select the character to the left of the cursor
				nStart = nEnd - 1;
			}

			// check if the caret is not at the beginning of the string
			if (nEnd > 0)
			{
				// process the backspace character
				GetWindowText(value);
				value.Delete(nStart, nEnd - nStart);
				SetWindowText((LPCTSTR)value);

				// restore the caret position
				if (HIWORD(wSel) == LOWORD(wSel))
				{
					SetEditSel(nStart-1, nStart-1);
				}
				else
				{
					SetEditSel(nStart, nStart);
				}
			}

			break;
		}

	// the Final key has been appropriated for a different use
	case VK_FINAL:
		{
			// set the text in the edit box to the current selection
			int sel = GetCurSel();

			// is there something selected?
			if (sel != LB_ERR)
			{
				// get the text from the list box
				char text[1024];
				text[0] = 0;
				GetLBText(sel, (char *)&text);

				// make sure the pointer is valid
				SelectString(-1, (const char *)&text);
				SetWindowText(text);

				// get the edit control from the CComboBox control
				CWnd * pEditBox = FromHandle(m_hWnd);
				pEditBox = GetWindow(GW_CHILD);
				pEditBox->SetWindowText(text);

				// make sure the previous statement worked
				// the first window should be the list box
				if (pEditBox != NULL)
				{
				}
			}

			// close the drop down box
			ShowDropDown(FALSE);

			break;
		}

	case VK_ESCAPE:
		{
			// close the drop down box
			ShowDropDown(FALSE);

			break;
		}
	default:
		{
			CComboBox::OnKeyDown(nChar, nRepCnt, nFlags);
		}
	}
}

void MyComboBox::OnEditUpdate()

{
	int		nLength;
	int		nStart=0;
	int		nEnd=0;
	int		idx=0;
	CString	str;

	// check for backspace or delete key
	if (!m_bAutoComplete)
	{
		// don't do anything
		return;
	}

	// Get the text in the edit box
	GetWindowText(str);
	nLength = str.GetLength();
  
	// Currently selected range
	DWORD dwCurSel = GetEditSel();
	nStart = LOWORD(dwCurSel);
	nEnd   = HIWORD(dwCurSel);

	// check if any text is selected
	if (nStart > nEnd)
	{
		// truncate the selected text from the string we will use to search the edit box
		str.Delete(nStart, nLength - nStart);
	}

	// Search for, and select in, and string in the combo box that is prefixed
	// by the text in the edit box
	idx = SelectString(-1, str);
	if (idx == CB_ERR)
	{
		SetWindowText(str);				// Not found in the list so restore the original text and do not select any text 
		if (idx != CB_ERR)
		{
			SetEditSel(nStart, nEnd);	//restore cursor postion
		}
	}

	// Set the text selection as the additional text that we have added
	if ((nEnd < nLength) && (idx != CB_ERR))
	{
		SetEditSel(nStart, nEnd);
	}
	else
	{
		if (idx != CB_ERR)
		{
			// move cursor to the end of the text
			SetEditSel(nLength, -1);
		}
		else
		{
			// move cursor to the next character
			SetEditSel(nEnd, nEnd);
		}
	}
}


BOOL MyComboBox::PreTranslateMessage(MSG* pMsg) 

{
	int		nStart=0;
	int		nEnd=0;
	int		nVirtKey;
	CString value;

    // Check for backspace/delete. These will modify the text in
    // the edit box, causing the auto complete to just add back the text
    // the user has just tried to delete. 

    if (pMsg->message == WM_KEYDOWN)
    {
        m_bAutoComplete = TRUE;

		// get the key that was pressed
        nVirtKey = (int) pMsg->wParam;

		// check for delete key
        if (VK_DELETE == nVirtKey)
		{
			// turn off autocomplete
            m_bAutoComplete = FALSE;
		}
		else if (VK_BACK == nVirtKey)
		{
			// process backspace key
			// get the current location of the caret
			DWORD wSel = GetEditSel();
			nStart = HIWORD(wSel);
			nEnd = LOWORD(wSel);

			// check if any characters are selected
			if (nStart == nEnd)
			{
				// nothing selected so select the character to the left of the cursor
				nStart = nEnd - 1;
			}

			// check if the caret is not at the beginning of the string
			if (nEnd > 0)
			{
				// process the backspace character
				GetWindowText(value);
				value.Delete(nStart, nEnd - nStart);
				SetWindowText((LPCTSTR)value);

				// restore the caret position
				SetEditSel(nStart, nStart);
			}

			// indicate that the message was processed
			return TRUE;
		}
    }

	// process the message
    return CComboBox::PreTranslateMessage(pMsg);
}

UINT MyComboBox::OnGetDlgCode()

{
	return DLGC_WANTALLKEYS;
}


