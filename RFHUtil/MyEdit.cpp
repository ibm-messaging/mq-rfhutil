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

// MyEdit.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "MyEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MyEdit

#define VK_ENTER	13

MyEdit::MyEdit()
{
}

MyEdit::~MyEdit()
{
}


BEGIN_MESSAGE_MAP(MyEdit, CEdit)
	//{{AFX_MSG_MAP(MyEdit)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MyEdit message handlers

// Check for backspace and enter. 
// The backspace key will erase the last character, moving up a line if necessary
// The Enter key will insert a CR/LF sequence

BOOL MyEdit::PreTranslateMessage(MSG* pMsg) 

{
 	int		nStart=0;
	int		nEnd=0;
	int		nLine=0;
	int		nVirtKey;
	CString value;

    if (pMsg->message == WM_KEYDOWN)
    {
		// get the key that was pressed
        nVirtKey = (int) pMsg->wParam;

		// check for enter or backspace keys
        if ((nVirtKey == VK_ENTER) || (nVirtKey == VK_BACK))
		{
			// get the current location of the caret
			DWORD wSel = GetSel();
			nStart = HIWORD(wSel);
			nEnd = LOWORD(wSel);

			// check what line we are on
			nLine = LineFromChar(nStart);

			if (nVirtKey == VK_ENTER)
			{
				// insert a CR/LF into and move to the next line
				// get the current value
				GetWindowText(value);

				// insert a CR/LF pair
				value.Insert(nStart, "\r\n");

				// update the data in the control
				SetWindowText((LPCTSTR)value);

				// set the selection to the beginning of the line that was just inserted
				SetSel(nStart + 2, nStart + 2, TRUE);
			}
			else if (nVirtKey == VK_BACK)
			{
				// process backspace key
				// start by getting the current value
				GetWindowText(value);

				// make sure there is something in the edit control to start with
				// and that the cursor is not at offset 0
				if ((value.GetLength() > 0) && (nEnd > 0))
				{
					// check if any characters are selected
					if (nStart == nEnd)
					{
						// nothing selected so select the character to the left of the cursor
						nStart = nEnd - 1;

						// check if it is a CR or LF character
						if ((value[nStart] != '\n') && (value[nStart] != '\r'))
						{
							// delete the character
							value.Delete(nStart, 1);
						}
						else
						{
							// delete the CR or LF character
							value.Delete(nStart, 1);

							// check for a second character in front of this one
							if ((nStart > 0) && (((value[nStart-1] == '\n') || (value[nStart-1] == '\r'))))
							{
								// delete the CR or LF character
								value.Delete(nStart-1, 1);
							}
						}
					}
					else
					{
						// just delete the selected characters
						value.Delete(nStart, nEnd - nStart);
					}

					// update the text in the edit control
					SetWindowText((LPCTSTR)value);

					// point the caret at the correct location
					SetSel(nStart, nStart, TRUE);
				}

				return TRUE;
			}
		}
    }
	
	return CEdit::PreTranslateMessage(pMsg);
}
