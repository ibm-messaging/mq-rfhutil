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

#if !defined(AFX_OTHER_H__38B9C4B4_5B9B_4326_9C65_20B91437F74A__INCLUDED_)
#define AFX_OTHER_H__38B9C4B4_5B9B_4326_9C65_20B91437F74A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// other.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// other dialog

#include "DataArea.h"
#include "MyEdit.h"

class other : public CPropertyPage
{
	DECLARE_DYNCREATE(other)

// Construction
public:
	void clearOtherData();
	const char * getOtherArea();
	int buildOtherArea(int ccsid, int encoding);
	BOOL wasDataChanged();
	void parseRFH2other(unsigned char *rfhdata, int dataLen, int ccsid);
	void freeOtherArea();
	void updatePageData();
	void getSelection(int &firstChar, int &lastChar);
	DataArea* pDoc;
	other();
	~other();

// Dialog Data
	//{{AFX_DATA(other)
	enum { IDD = IDD_OTHER };
	CString	m_rfh_other_data;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(other)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(other)
	afx_msg void OnChangeOtherRfhData();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int processFolder(char *input, int length, char *output, int maxLen, int ccsid, int encoding, BOOL ucs);
	char * findEndOfLine(char * inPtr, char * endPtr);
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	int otherEncoding;
	int otherCcsid;
	unsigned char * rfh_other_area;
	int m_RFH_other_len;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	bool otherDataChanged;
	MyEdit m_OtherEditBox;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OTHER_H__38B9C4B4_5B9B_4326_9C65_20B91437F74A__INCLUDED_)
