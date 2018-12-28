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

#if !defined(AFX_DISPLAYQ_H__8BDAA27E_E25B_4D7A_9019_149AF788F244__INCLUDED_)
#define AFX_DISPLAYQ_H__8BDAA27E_E25B_4D7A_9019_149AF788F244__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DisplayQ.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDisplayQ dialog

class CDisplayQ : public CDialog
{
// Construction
public:
	int m_selectedLine;
	CDisplayQ(CWnd* pParent = NULL, const char * msgData=NULL, const char * dispName=NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDisplayQ)
	enum { IDD = IDD_DISPLAYQ };
	CString	m_msg_data;
	int		m_read_type;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDisplayQ)
	public:
	protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
private:
	void setSelectedLine();
	CString qName;
	void setFixedFont();
	CFont m_fixed_font;
	char strTitle[80];

	// Generated message map functions
	//{{AFX_MSG(CDisplayQ)
	afx_msg void OnDisplayqReadq();
	afx_msg void OnDisplayqBrowseMsg();
	afx_msg void OnDisplayqStartBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISPLAYQ_H__8BDAA27E_E25B_4D7A_9019_149AF788F244__INCLUDED_)
