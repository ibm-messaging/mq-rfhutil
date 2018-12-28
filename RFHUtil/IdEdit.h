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

#if !defined(AFX_IDEDIT_H__8F139E9E_D743_447C_A099_5AC606264909__INCLUDED_)
#define AFX_IDEDIT_H__8F139E9E_D743_447C_A099_5AC606264909__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IdEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIdEdit window

class CIdEdit : public CEdit
{
// Construction
public:
	CIdEdit();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIdEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	void setMaxLength(const int maxLength);
	void SetHexOnly(BOOL hexOnly);
	BOOL GetOvertype();
	void SetOvertype(BOOL overType);
	virtual ~CIdEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CIdEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEditPaste();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	int maxHexLen;
	int maxDataLen;
	void editHex();
	void checkLength(int maxLen, char pad);
	void editContents();
	BOOL m_bHexOnly;
	BOOL m_bOvertype;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IDEDIT_H__8F139E9E_D743_447C_A099_5AC606264909__INCLUDED_)
