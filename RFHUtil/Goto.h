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

#if !defined(AFX_GOTO_H__CAC00049_1BF9_4CF6_B69F_5CB2680301A0__INCLUDED_)
#define AFX_GOTO_H__CAC00049_1BF9_4CF6_B69F_5CB2680301A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Goto.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGoto dialog

class CGoto : public CDialog
{
// Construction
public:
	int getOffset();
	CGoto(CWnd* pParent, int maxOffset);
	CGoto(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGoto)
	enum { IDD = IDD_GOTO };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGoto)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGoto)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	int offset;
	int maxOffset;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GOTO_H__CAC00049_1BF9_4CF6_B69F_5CB2680301A0__INCLUDED_)
