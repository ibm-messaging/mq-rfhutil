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

#if !defined(AFX_DLQ_H__AB02BB6B_29A7_43CD_AE1C_F8F43CDA826D__INCLUDED_)
#define AFX_DLQ_H__AB02BB6B_29A7_43CD_AE1C_F8F43CDA826D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dlq.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlq dialog

class CDlq : public CPropertyPage
{
	DECLARE_DYNCREATE(CDlq)

// Construction
public:
	int getFloatEncoding();
	void prepareResend();
	void updateFields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding);
	void updatePageData();
	int getEncoding();
	int getPdEncoding();
	int getCcsid();
	void getFormat(char * mqformat);
	char * getDLQheader();
	void setNextFormat(char * nextFormat, int * charFormat, int * encoding);
	bool checkForDlq();
	void clearDLQheader();
	int getDLQlength();
	void freeCurrentHeader(const int callee);
	int createHeader(int charType, int encodeType);
	int parseDLQheader(unsigned char *dlqData, int dataLen, int ccsid, int encodeType);
	void UpdatePageData();
	CDlq();
	~CDlq();
	DataArea* pDoc;

// Dialog Data
	//{{AFX_DATA(CDlq)
	enum { IDD = IDD_DLQ };
	CString	m_dlq_codepage;
	CString	m_dlq_date_time;
	CString	m_dlq_format;
	CString	m_dlq_orig_queue;
	CString	m_dlq_orig_qm;
	int		m_dlq_encode;
	int		m_dlq_pd_encode;
	CString	m_dlq_put_appl_name;
	CString	m_dlq_put_appl_type;
	CString	m_dlq_reason;
	BOOL	m_dlq_include;
	int		m_dlq_float_encode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDlq)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDlq)
	afx_msg void OnChangeDlqCodepage();
	afx_msg void OnChangeDlqDateTime();
	afx_msg void OnChangeDlqFormat();
	afx_msg void OnDlqHost();
	afx_msg void OnChangeDlqOrigQ();
	afx_msg void OnChangeDlqOrigQm();
	afx_msg void OnDlqPc();
	afx_msg void OnDlqPdHost();
	afx_msg void OnDlqPdPc();
	afx_msg void OnChangeDlqPutApplName();
	afx_msg void OnEditchangeDlqPutApplType();
	afx_msg void OnChangeDlqReason();
	afx_msg void OnSetfocusDlqPutApplType();
	afx_msg void OnDlqPrepResend();
	afx_msg void OnDlqIncludeHeader();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int dlqEncodeType;
	int dlqCcsid;
	void resetDLQdata();
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	void enableDisplay();
	BOOL OnInitDialog();
	BOOL GetToolText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	void setPutApplType(int putType);
	BOOL putApplTypeInit;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);

	int dlqLength;
	unsigned char * dlqHeader;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLQ_H__AB02BB6B_29A7_43CD_AE1C_F8F43CDA826D__INCLUDED_)
