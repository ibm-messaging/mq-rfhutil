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

#if !defined(AFX_CICS_H__31FB8FAD_214B_4132_B21C_8BA0BEB7EAF3__INCLUDED_)
#define AFX_CICS_H__31FB8FAD_214B_4132_B21C_8BA0BEB7EAF3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CICS.h : header file
//

#define CIH_NONE		0
#define CIH_V1			1
#define CIH_V2			2

#define CIH_END_NOSYNC	0
#define CIH_END_COMMIT	1
#define CIH_END_BACKOUT	2
#define CIH_END_ENDTASK	3

#define CIH_UOW_ONLY	0
#define CIH_UOW_CONT	1
#define CIH_UOW_FIRST	2
#define CIH_UOW_MIDDLE	3
#define CIH_UOW_LAST	4
#define CIH_UOW_COMMIT	5
#define CIH_UOW_BACKOUT	6

#define CIH_START_NONE	0
#define CIH_START_S		1
#define CIH_START_SDATA	2
#define CIH_START_TERM	3

#define CIH_LINK_PROG	0
#define CIH_LINK_TRAN	1

#define CIH_MAX_PROG_NAME	8
/////////////////////////////////////////////////////////////////////////////
// CCICS dialog

class CCICS : public CPropertyPage
{
	DECLARE_DYNCREATE(CCICS)

// Construction
public:
	int getFloatEncoding();
	void updateFields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding);
	void UpdatePageData();
	int getPdEncoding();
	int getEncoding();
	int getCcsid();
	void getFormat(char * mqformat);
	void setNextFormat(char * nextFormat, int * charFormat, int * encoding);
	void restoreFacility();
	void saveFacility();
	void clearCICSheader();
	int getCICSlength();
	int buildCICSheader(unsigned char *, int ccsid, int encodeType);
	void freeCurrentHeader(const int callee);
	int parseCICSheader(unsigned char * cicsData, int dataLen, int dataType, int encodeType);
	CCICS();
	~CCICS();
	DataArea* pDoc;

// Dialog Data
	//{{AFX_DATA(CCICS)
	enum { IDD = IDD_CICS };
	int		m_CIH_version;
	CString	m_CIH_facility;
	int		m_CIH_status;
	int		m_CIH_startcode;
	int		m_CIH_uow;
	BOOL	m_CIH_conversational;
	CString	m_CIH_format;
	CString	m_CIH_program;
	CString	m_CIH_next_tranid;
	CString	m_CIH_cancel_code;
	CString	m_CIH_comp_code;
	CString	m_CIH_return_code;
	CString	m_CIH_reason;
	CString	m_CIH_cursor_pos;
	CString	m_CIH_error_ofs;
	CString	m_CIH_facility_like;
	CString	m_CIH_aid;
	CString	m_CIH_transid;
	CString	m_CIH_remote_sys;
	CString	m_CIH_remote_tran;
	CString	m_CIH_reply_format;
	CString	m_CIH_codepage;
	CString	m_CIH_flags;
	CString	m_CIH_wait_interval;
	CString	m_CIH_data_length;
	CString	m_CIH_abend_code;
	CString	m_CIH_authenticator;
	CString	m_CIH_keep_time;
	CString	m_CIH_input_item;
	int		m_CIH_int_encode;
	int		m_CIH_pd_encode;
	int		m_CIH_float_encode;
	CString	m_CIH_function;
	int		m_CIH_link_type;
	BOOL	m_cih_ads_msg_format;
	BOOL	m_cih_ads_receive;
	BOOL	m_cih_ads_send;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCICS)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCICS)
	afx_msg void OnCihNone();
	afx_msg void OnCihV1();
	afx_msg void OnCihV2();
	afx_msg void OnChangeCihAbendCode();
	afx_msg void OnChangeCihAid();
	afx_msg void OnChangeCihAuthenticator();
	afx_msg void OnChangeCihCancelCode();
	afx_msg void OnChangeCihCodepage();
	afx_msg void OnChangeCihCompCode();
	afx_msg void OnCihConversational();
	afx_msg void OnChangeCihCursorPos();
	afx_msg void OnChangeCihDataLength();
	afx_msg void OnCihEndBackout();
	afx_msg void OnCihEndCommit();
	afx_msg void OnCihEndEndtask();
	afx_msg void OnCihEndNosync();
	afx_msg void OnChangeCihErrorOffset();
	afx_msg void OnChangeCihFacility();
	afx_msg void OnChangeCihFacilityLike();
	afx_msg void OnChangeCihFlags();
	afx_msg void OnChangeCihFormat();
	afx_msg void OnCihLinkNone();
	afx_msg void OnCihLinkProg();
	afx_msg void OnCihLinkTran();
	afx_msg void OnChangeCihNextTranid();
	afx_msg void OnChangeCihProgram();
	afx_msg void OnChangeCihReason();
	afx_msg void OnChangeCihRemoteSys();
	afx_msg void OnChangeCihRemoteTran();
	afx_msg void OnChangeCihReplyFormat();
	afx_msg void OnChangeCihReturnCode();
	afx_msg void OnCihScData();
	afx_msg void OnCihScNone();
	afx_msg void OnCihScStart();
	afx_msg void OnCihScTerm();
	afx_msg void OnChangeCihTransId();
	afx_msg void OnCihUowCommit();
	afx_msg void OnCihUowCont();
	afx_msg void OnCihUowFirst();
	afx_msg void OnCihUowLast();
	afx_msg void OnCihUowMiddle();
	afx_msg void OnCihUowOnly();
	afx_msg void OnCihUowRollback();
	afx_msg void OnChangeCihWaitInterval();
	afx_msg void OnChangeCihKeepTime();
	afx_msg void OnChangeCihInputItem();
	afx_msg void OnCihIntEncodePc();
	afx_msg void OnCihIntEncodeHost();
	afx_msg void OnCihPdEncodePc();
	afx_msg void OnCihPdEncodeHost();
	afx_msg void OnCihAdsNone();
	afx_msg void OnCihAdsSend();
	afx_msg void OnCihAdsMsgFmt();
	afx_msg void OnEditchangeCihFunction();
	afx_msg void OnDropdownCihFunction();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeCihFunction();
	afx_msg void OnSelendokCihFunction();
	afx_msg void OnCihAdsMsgFormat();
	afx_msg void OnCihAdsReceive();
	afx_msg void OnCihRemoveHeader();
	afx_msg void OnCihInsertHeader();
	afx_msg void OnCihFloatEncodePc();
	afx_msg void OnCihFloatEncodeHost();
	afx_msg void OnCihFloatEncode390();
	afx_msg void OnCihFloatEncodeTns();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void dlgItemAddString(const int dlgItem, const char *itemText);
	int conv2EBCDIC(char * data, int len);
	int cicsEncodeType;
	int cicsCcsid;
	void resetCICSheader();
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	void enableDisplay();
	int m_save_CIH_version;
	CString saveCICSfacility;
	BOOL functionBoxInitialized;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	void createHeader(int charType, int encodeType);
	int cicsLength;
	unsigned char * cicsHeader;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CICS_H__31FB8FAD_214B_4132_B21C_8BA0BEB7EAF3__INCLUDED_)
