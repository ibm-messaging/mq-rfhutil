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

#if !defined(AFX_IMS_H__F77B1EFE_F98E_4795_BDF2_7B9EEB9FECCA__INCLUDED_)
#define AFX_IMS_H__F77B1EFE_F98E_4795_BDF2_7B9EEB9FECCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Ims.h : header file
//

#define IMS_NO_CONV			0
#define IMS_IN_CONV			1
#define IMS_ARCHITECT		2

#define IMS_COMMIT_FIRST	0
#define IMS_SEND_FIRST		1

#define	IMS_SEC_CHECK		0
#define IMS_SEC_FULL		1

/////////////////////////////////////////////////////////////////////////////
// CIms dialog

class CIms : public CPropertyPage
{
	DECLARE_DYNCREATE(CIms)

// Construction
public:
	int getFloatEncoding();
	void updateFields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding);
	void restoreTransInstanceId();
	void saveTransInstanceId();
	int buildIMSheader(unsigned char * header, int ccsid, int encodeType);
	void clearIMSheader();
	void UpdatePageData();
	void setNextFormat(char * nextFormat, int * charType, int * encoding);
	void getFormat(char * mqformat);
	void freeCurrentHeader(const int callee);
	int getPdEncoding();
	int getEncoding();
	int getCcsid();
	int getIMSlength();
	int parseIMSheader(unsigned char * imsData, int dataLen, int ccsid, int encodeType);
	void createHeader(int ccsid, int encodeType);
	CIms();
	~CIms();
	DataArea* pDoc;

// Dialog Data
	//{{AFX_DATA(CIms)
	enum { IDD = IDD_IMS };
	CString	m_IIH_codepage;
	CString	m_IIH_authenticator;
	int		m_IIH_commit_mode;
	CString	m_IIH_format;
	int		m_IIH_encode_int;
	int		m_IIH_encode_pd;
	int		m_IIH_encode_float;
	CString	m_IIH_lterm;
	CString	m_IIH_map_name;
	int		m_IIH_trans_state;
	CString	m_IIH_reply_format;
	int		m_IIH_security_scope;
	CString	m_IIH_trans_id;
	BOOL	m_IIH_include_header;
	BOOL	m_IIH_no_reply_format;
	BOOL	m_IIH_pass_expiration;
	BOOL	m_IIH_add_length;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIms)
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
	//{{AFX_MSG(CIms)
	virtual BOOL OnInitDialog();
	afx_msg void OnIihNoConversation();
	afx_msg void OnIihNoReplyFormat();
	afx_msg void OnIihPassExpiration();
	afx_msg void OnIihPdEncodeHost();
	afx_msg void OnIihPdEncodePc();
	afx_msg void OnChangeIihReplyFormat();
	afx_msg void OnIihSecCheck();
	afx_msg void OnIihSecFull();
	afx_msg void OnIihSendFirst();
	afx_msg void OnChangeIihTransId();
	afx_msg void OnIihArchitected();
	afx_msg void OnChangeIihAuthenticator();
	afx_msg void OnIihCommitFirst();
	afx_msg void OnChangeIihFormat();
	afx_msg void OnIihInConversation();
	afx_msg void OnIihIncludeHeader();
	afx_msg void OnIihIntEncodeHost();
	afx_msg void OnIihIntEncodePc();
	afx_msg void OnChangeIihLterm();
	afx_msg void OnChangeIihMapName();
	afx_msg void OnChangeIihCodepage();
	afx_msg void OnIihAddLength();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int imsEncodeType;
	int imsCcsid;
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	CString saveIMStransId;
	void enableDisplay();
	BOOL m_save_include_header;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	int imsLength;
	unsigned char * imsHeader;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMS_H__F77B1EFE_F98E_4795_BDF2_7B9EEB9FECCA__INCLUDED_)
