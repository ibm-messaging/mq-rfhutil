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

#if !defined(AFX_HEXFIND_H__A25F167C_1B66_4FE8_BAAD_BCF3F84BB0CA__INCLUDED_)
#define AFX_HEXFIND_H__A25F167C_1B66_4FE8_BAAD_BCF3F84BB0CA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HexFind.h : header file
//
#include <afxwin.h>
#include <afxdlgs.h>

#define HEX_FINDER_DIALOG	"hex finder dialog"

#define SEARCH_UP			0
#define SEARCH_DOWN			1

/////////////////////////////////////////////////////////////////////////////
// CHexFind dialog

class CHexFind : public CDialog
{
// Construction
public:
	BOOL SearchDown();
	BOOL FindNext();
	BOOL isTerminating();
	int getHexValue(unsigned char * value, int maxLen);
	CHexFind(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CHexFind)
	enum { IDD = IDD_HEXFIND };
	CString	m_hex_value;
	int		m_direction;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHexFind)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHexFind)
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
private:
	BOOL hexDatavalid();
	BOOL terminating;
	BOOL findPressed;
	CWnd * parent;
	char getHexCharValue(unsigned char charIn);
	void HexToAscii(unsigned char *dati, unsigned int pl, unsigned char *dato);
	UINT helperMsg;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HEXFIND_H__A25F167C_1B66_4FE8_BAAD_BCF3F84BB0CA__INCLUDED_)
