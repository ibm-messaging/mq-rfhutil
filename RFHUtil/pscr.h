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

#if !defined(AFX_PSCR_H__48EEF4B9_76C2_4B9F_AFCF_CE24FEDEDFD2__INCLUDED_)
#define AFX_PSCR_H__48EEF4B9_76C2_4B9F_AFCF_CE24FEDEDFD2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// pscr.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// pscr dialog

#include "DataArea.h"

class pscr : public CPropertyPage
{
	DECLARE_DYNCREATE(pscr)

// Construction
public:
	void clearPSCRdata();
	const char * getPscrArea();
	BOOL wasDataChanged();
	int buildPscrArea(int ccsid, int encoding);
	int buildV1PubResp(char * tempBuf);
	void setPscrOther(char  *tempOther);
	char * parseV1Pscr(char * rfhptr, int *found);
	void parseRFH2pscr(unsigned char *rfhdata, int dataLen);
	void freePscrArea();
	void setPscrArea(unsigned char *pscrData, int dataLen, int ccsid, int encoding);
	void updatePageData();
	pscr();
	~pscr();
	DataArea* pDoc;

// Dialog Data
	//{{AFX_DATA(pscr)
	enum { IDD = IDD_PSCR };
	int		m_pscr_reply;
	CString	m_pscr_reason1;
	CString	m_pscr_reason2;
	CString	m_pscr_response1;
	CString	m_pscr_response2;
	CString	m_pscr_other;
	CString	m_pscr_error_id;
	CString	m_pscr_error_pos;
	CString	m_pscr_parm_id;
	CString	m_pscr_user_id;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(pscr)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(pscr)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeReason1();
	afx_msg void OnChangeReason2();
	afx_msg void OnChangeResponse1Mesg();
	afx_msg void OnChangeResponse2Mesg();
	afx_msg void OnPscrWarn();
	afx_msg void OnPscrError();
	afx_msg void OnPscrReplyOk();
	afx_msg void OnChangePscrOther();
	afx_msg void OnChangePscrErrorId();
	afx_msg void OnChangePscrErrorPos();
	afx_msg void OnChangePscrParmId();
	afx_msg void OnChangePscrUserId();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	int m_RFH_pscr_len;
	int m_RFH_pscr_ccsid;
	int m_RFH_pscr_encoding;
	unsigned char * rfh_pscr_area;
	bool pscrDataChanged;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	char * processPscrResponse(char * ptr, 
							   char *endptr, 
							   char * reason, 
							   char * other,
							   int maxReason,
							   int maxOther);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PSCR_H__48EEF4B9_76C2_4B9F_AFCF_CE24FEDEDFD2__INCLUDED_)
