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

#if !defined(AFX_USR_H__555E0652_1B5F_40CE_8380_93705F416B6D__INCLUDED_)
#define AFX_USR_H__555E0652_1B5F_40CE_8380_93705F416B6D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Usr.h : header file
//

#include "DataArea.h"

// include for RFH2 constants, etc

#include <cmqpsc.h>
#include "MyEdit.h"	// Added by ClassView

/////////////////////////////////////////////////////////////////////////////
// Usr dialog

class Usr : public CPropertyPage
{
// Construction
public:
	void checkForRetained(BOOL *isRetained);
	void getUsrData(CString& data);
	void clearUsrData();
	BOOL wasDataChanged();
	const char * getUsrArea();
	int buildUsrArea(int ccsid, int encoding);
	void setUsrArea(unsigned char *usrData, int dataLen, int ccsid, int encoding);
	void freeUsrArea();
	void parseRFH2usr(unsigned char *rfhdata, int dataLen, int ccsid, int encoding);
	void getSelection(int &firstChar, int &lastChar);
	void updatePageData();
	DataArea* pDoc;
	Usr();   // standard constructor
	~Usr();

// Dialog Data
	//{{AFX_DATA(Usr)
	enum { IDD = IDD_USR };
	CString	m_rfh_usr_data;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Usr)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Usr)
	afx_msg void OnChangeRfhUsrData();
	afx_msg void OnSetfocusRfhUsrData();
	virtual BOOL OnInitDialog();
	afx_msg void OnRfhFromUser();
	afx_msg void OnUpdateRfhUsrData();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	MyEdit m_DataEditBox;
	int m_RFH_usr_len;
	int m_rfh_usr_encoding;
	int m_rfh_usr_ccsid;
	unsigned char * rfh_usr_area;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	bool usrDataChanged;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USR_H__555E0652_1B5F_40CE_8380_93705F416B6D__INCLUDED_)
