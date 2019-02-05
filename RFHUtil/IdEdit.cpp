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

// IdEdit.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "IdEdit.h"

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

/////////////////////////////
// Maximum lengths for data
/////////////////////////////
#define MAXDATALEN		24
#define MAXDATALENHEX	48

/////////////////////////////////////////////////////////////////////////////
// CIdEdit

CIdEdit::CIdEdit()
{
	m_bOvertype = TRUE;
	m_bHexOnly = FALSE;
	maxDataLen = MAXDATALEN;
	maxHexLen = MAXDATALENHEX;
}

CIdEdit::~CIdEdit()
{
}


BEGIN_MESSAGE_MAP(CIdEdit, CEdit)
	//{{AFX_MSG_MAP(CIdEdit)
	ON_WM_CHAR()
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIdEdit message handlers

void CIdEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 

{
	int		nBegin=0;
	int		nEnd=0;

	// This routine is necessary to retain the caret location after
	// the delete is pressed.
	switch (nChar)
	{
	case VK_DELETE:
		{
			// get the current location of the caret
			GetSel(nBegin, nEnd);

			// process the key
			CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

			// restore the selection
			//SetSel(nBegin, nEnd);
			SetSel(nBegin, nBegin);

			break;
		}
	case VK_INSERT:
		{
			// toggle the overtype indicator
			m_bOvertype = !m_bOvertype;
			break;
		}
	case VK_BACKSP:
		{
			// handle the backspace key
			// get the current location of the caret
			GetSel(nBegin, nEnd);

			// check if the caret is not at the beginning of the data
			if (nEnd > 0)
			{
				// check if any characters are selected
				if (nBegin == nEnd)
				{
					// nothing selected so select the character to the left of the cursor
					SetSel(nEnd - 1, nEnd);
				}

				// process the backspace character
				ReplaceSel("");

				// restore the caret position
				if (nBegin == nEnd)
				{
					SetSel(nBegin-1, nBegin-1);
				}
				else
				{
					SetSel(nBegin, nBegin);
				}
			}

			break;
		}
	default:
		{
			CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
		}
	}
}

void CIdEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 

{
	char	ch;
	int		err=0;
	int		len;
	char	s[4];
	CString	data;
	CString	newData;
	CString clipBoardData;
	char	currentText[64];

	// get the input character without change, in case of insert mode
	ch = nChar;

	// check for ctrl+V (Edit Paste) 
	switch (nChar)
	{
	case CTRL_V:
		{
			// handle paste operation here
			// is there text in the clipboard?
			if (::IsClipboardFormatAvailable(CF_TEXT))
			{
				// check if the control is read-only
				DWORD dwStyle(::GetWindowLong(GetSafeHwnd(), GWL_STYLE));
				if (ES_READONLY == (dwStyle & ES_READONLY))
				{
					::MessageBeep((UINT) -1);
				}
				else 
				{
					// check if clipboard can be opened
					if (OpenClipboard())
					{
						HANDLE hData = ::GetClipboardData(CF_TEXT);

						// make sure the handle is valid
						if (hData != NULL)
						{
							char * ptr = (char *)GlobalLock(hData);

							// make sure the pointer is valid
							if (ptr != NULL)
							{
								clipBoardData = ptr;
								GlobalUnlock(hData);

								// check how long the data to be inserted is
								int maxBytes = clipBoardData.GetLength();

								// limit to maximum of 48 bytes
								if (maxBytes > 48)
								{
									// truncate the clip board data
									clipBoardData = clipBoardData.Left(48);
									maxBytes = 48;
								}

								// get the current text in the buffer
								memset(currentText, 0, sizeof(currentText));
								CEdit::GetWindowText(currentText, sizeof(currentText) - 1);

								// check if anything is selected
								// get the current location of the caret
								WORD wBeg = LOWORD(GetSel());
								WORD wEnd = HIWORD(GetSel());

								// check if there is anything selected
								if (wBeg == wEnd)
								{
									// nothing is selected
									// replace from the cursor position to the end
									wEnd = 48;
								}

								// start replacing characters with text from the clipboard
								int idx=0;
								while ((idx < maxBytes) && (wBeg < wEnd))
								{
#pragma warning(suppress: 6386)
									currentText[wBeg++] = clipBoardData[idx++];
								}

								// replace the old text with the updated text
								CEdit::SetWindowText(currentText);
							}
						}

						// close the clipboard handle
						CloseClipboard();
					}
					else
					{
						// check the contents of the clipboard
						// perform the paste operation
						CEdit::OnChar(nChar, nRepCnt, nFlags);

						// now edit the result so that we have valid results
						editContents();
					}
				}
			}

			return;
			break;
		}
	case VK_BACKSP:
		{
			// handle the backspace key
			// get the current location of the caret
			WORD wPos = LOWORD(GetSel());

			// check if the caret is not at the beginning of the data
			if (wPos > 0)
			{
				// process the backspace character
				CEdit::OnChar(nChar, nRepCnt, nFlags);

				// restore the caret position
				SetSel(wPos-1, wPos-1);
			}

			return;
			break;
		}
	default:
		{
			// check if the hex only option is selected and that this is a normal character
			if (m_bHexOnly && (ch >= ' '))
			{
				// get the input in upper case
				ch = toupper(nChar);

				err = 1;
				// make sure that the character is a valid hex character
				if (((ch >= '0') && (ch <= '9')) || ((ch >= 'A') && (ch <= 'F')))
				{
					// character is valid hex character
					err = 0;
				}

				// check what key was pressed
				if (1 == err)
				{
					// invalid hex data
					// issue beep and reject the character by returning without processing it
					Beep(300,200);
					return;
				}
			}
		}
	}

	// check if we have a normal character
	if (ch >= ' ')
	{
		// check for overtype mode
		if (m_bOvertype)
		{
			// select the character to the right of the caret
			// this character will then be replaced with the character that was typed
			WORD wPos = LOWORD(GetSel());
			SetSel(wPos, wPos+1);
		}
		else
		{
			// get the current location of the caret
			WORD wPos = LOWORD(GetSel());

			// get the current length of the data
			len = GetWindowTextLength();

			// check if this is an valid character and that the caret is not at the end of the text
			if (wPos < (len - 1))
			{
				// to handle insert mode the last character must be deleted
				// check if we need to delete the last character in the control
				if ((24 == len) || (48 == len))
				{
					// get the current contents of the control and delete the last character
					GetWindowText(data);

					// is the caret at the beginning of the data?
					if (wPos > 0)
					{
						// not at the beginning, so capture the preceding data
						newData = data.Left(wPos);
					}
					else
					{
						newData.Empty();
					}

					// insert the new character
					s[0] = ch;
					s[1] = 0;
					newData += s;

					// is there data after the caret?
					if (wPos < len - 1)
					{
						newData += data.Mid(wPos, len - wPos - 1);
					}

					int j = newData.GetLength();
					// restore the window text
					SetWindowText((LPCTSTR)newData);

					// set the caret to the next location
					SetSel(wPos + 1, wPos + 1);

					// the keystroke has been handled, so return
					return;
				}
			}
		}
	}
	
	// check if the hex only option is selected
	if (m_bHexOnly)
	{
		// force to upper case letters only
		CEdit::OnChar(ch, nRepCnt, nFlags);
	}
	else
	{
		// allow any characters
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}
}

void CIdEdit::SetOvertype(BOOL overType)

{
	m_bOvertype = overType;
}

BOOL CIdEdit::GetOvertype()

{
	return m_bOvertype;
}

void CIdEdit::SetHexOnly(BOOL hexOnly)

{
	m_bHexOnly = hexOnly;
}

BOOL CIdEdit::PreTranslateMessage(MSG* pMsg) 

{
	// is the hex only option selected?
	if (m_bHexOnly)
	{
		// make sure this is an ordinary character
		if (WM_CHAR == pMsg->message)
		{
			// translate the character to upper case
			pMsg->wParam = toupper(pMsg->wParam);
		}
	}
	
	return CEdit::PreTranslateMessage(pMsg);
}

void CIdEdit::OnEditPaste() 

{
	CEdit::Paste();
}

void CIdEdit::editContents()

{
	// check if we are in hex mode
	if (m_bHexOnly)
	{
		// first check the length
		checkLength(maxHexLen, '0');

		// only allow hex characters
		editHex();
	}
	else
	{
		checkLength(maxDataLen, ' ');
	}
}

void CIdEdit::checkLength(int maxLen, char pad)

{
	int		len=GetWindowTextLength();
	char	data[64];

	// this routine handles the case where the user changes the overall length of the data in the control
	// this can happen if the user inserts more text then is selected or selects more text that is inserted
	// from the clipboard
	// check if the length exceeds the maximum length
	if (len > maxLen)
	{
		// truncate the data to maximum length
		GetWindowText(data, maxLen + 1);
		data[maxLen] = 0;
		SetWindowText(data);
	}

	// pad the text with blanks
	if (len < maxLen)
	{
		// get the data
		memset(data, 0, sizeof(data));
		GetWindowText(data, maxLen);

		// pad the data with blanks
		while (len < maxLen)
		{
			// change a binary zero to a padding character
			data[len - 1] = pad;
			len++;
		}

		// update the current window text
		SetWindowText(data);
	}
}

void CIdEdit::editHex()

{
	char	data[64];
	char	ch;

	// get the current data
	memset(data, 0, sizeof(data));
	GetWindowText(data, maxHexLen+1);

	// check each individual character
	for (int i=0; i<maxHexLen; i++)
	{
		ch = toupper(data[i]);
		if (((ch >= '0') && (ch <= '9')) || ((ch >= 'A') && (ch <= 'F')))
		{
			// data is valid, just make sure it is upper case
			data[i] = ch;
		}
		else
		{
			// data is invalid - replace with a zero
			data[i] = '0';
		}
	}

	// finally replace the data with the updated data
	SetWindowText(data);
}


void CIdEdit::setMaxLength(const int maxLength)

{
	// set the new maximum length
	maxDataLen = maxLength;
	maxHexLen = maxLength << 1;
}
