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

#if !defined(AFX_RFH_H__232B777A_64B8_41AE_9107_8409A84074B5__INCLUDED_)
#define AFX_RFH_H__232B777A_64B8_41AE_9107_8409A84074B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RFH.h : header file
//

#include "DataArea.h"

/////////////////////////////////////////////////////////////////////////////
// RFH dialog

class RFH : public CPropertyPage
{
	DECLARE_DYNCREATE(RFH)

// Construction
public:
	int getRFH2FloatEncoding();
	int getRFH1FloatEncoding();
	void selectUsrFolder(BOOL selectUsr);
	void setNextFormatRfh2(char * nextFormat, int * charFormat, int * encoding);
	void setNextFormatRfh1(char * nextFormat, int * charFormat, int * encoding);
	void updateRFH1Fields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding);
	void updateRFH2Fields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding);
	void setRFHV2();
	void setRFHV1();
	void setPubSubVersion();
	int buildRFH2header(unsigned char *header, int ccsid, int encoding);
	int buildRFH1header(unsigned char * header, int ccsid, int encoding);
	int getRFH2length();
	int getRFH1length();
	int getRFH2PdEncoding();
	int getRFH1PdEncoding();
	int getRFH2Encoding();
	int getRFH1Encoding();
	int getRFH2Ccsid();
	int getRFH1Ccsid();
	void getRFH2Format(char * mqformat);
	void getRFH1Format(char * mqformat);
	void setJMSdomain(const char * domain);
	int buildRFH(int ccsid, int encoding);
	void clearRFH();
	int buildRFH2(int ccsid, int encoding);
	void freeMcdArea();
	void freeRfhArea();
	int parseRFH2(unsigned char *rfhptr, int msglength, int ccsid, int encoding);
	void freeRfh1Area();
	int parseRFH(unsigned char *rfhptr, int msglength, int ccsid, int encoding);
	void UpdatePageData();
	CObject*	jmsData;
	CObject*	pubsubData;
	CObject*	pscrData;
	CObject*	usrData;
	CObject*	otherData;
	DataArea*	pDoc;
	RFH();
	~RFH();

// Dialog Data
	//{{AFX_DATA(RFH)
	enum { IDD = IDD_RFH };
	CString	m_msg_domain;
	CString	m_rfh_ccsid;
	CString	m_rfh_charset;
	CString	m_rfh_flags;
	CString	m_rfh_length;
	CString	m_rfh_format;
	CString	m_rfh1_format_name;
	CString	m_rfh1_app_group;
	CString	m_rfh1_charset;
	CString	m_rfh1_flags;
	CString	m_rfh1_format;
	CString	m_msg_fmt;
	CString	m_msg_set;
	CString	m_msg_type;
	CString	m_rfh1_length;
	int		m_rfh_encoding;
	int		m_rfh_pd_encoding;
	int		m_rfh_float_encoding;
	int		m_rfh1_encoding;
	int		m_rfh1_pd_encoding;
	BOOL	m_incl_jms;
	BOOL	m_incl_mcd;
	BOOL	m_incl_usr;
	BOOL	m_incl_pubsub;
	BOOL	m_incl_pscr;
	BOOL	m_incl_other;
	BOOL	m_incl_v1_mcd;
	BOOL	m_incl_v1_pubsub;
	BOOL	m_incl_v1_resp;
	BOOL	m_rfh1_include;
	BOOL	m_rfh2_include;
	int		m_rfh1_float_encoding;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(RFH)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(RFH)
	afx_msg void OnRfhV2();
	afx_msg void OnChangeMsgDomain();
	afx_msg void OnChangeRfhCharset();
	afx_msg void OnChangeMsgFmt();
	afx_msg void OnChangeMsgType();
	afx_msg void OnRfhEncodeAscii();
	afx_msg void OnRfhEncodeEbcdic();
	afx_msg void OnInclJms();
	afx_msg void OnInclMcd();
	afx_msg void OnInclPubsub();
	afx_msg void OnInclUsr();
	afx_msg void OnInclPscr();
	afx_msg void OnInclOther();
	afx_msg void OnEditchangeRfhCcsid();
	afx_msg void OnRfhPdEncodeAscii();
	afx_msg void OnRfhPdEncodeEbcdic();
	virtual BOOL OnInitDialog();
	afx_msg void OnInclV1Mcd();
	afx_msg void OnInclV1Pubsub();
	afx_msg void OnChangeMsgSet();
	afx_msg void OnChangeRfh1AppGroup();
	afx_msg void OnChangeRfh1FormatName();
	afx_msg void OnChangeRfh1Charset();
	afx_msg void OnRfh1EncodeAscii();
	afx_msg void OnRfh1EncodeEbcdic();
	afx_msg void OnChangeRfh1Format();
	afx_msg void OnRfh1PdEncodeHost();
	afx_msg void OnRfh1PdEncodePc();
	afx_msg void OnChangeRfhFormat();
	afx_msg void OnRfh2IncludeHeader();
	afx_msg void OnRfh1IncludeHeader();
	afx_msg void OnInclV1Resp();
	afx_msg void OnSelchangeRfhCcsid();
	afx_msg void OnSelendokRfhCcsid();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int rfh_mcd_area_ccsid;
	int rfh_mcd_area_encoding;
	int rfh_data_encoding;
	int rfh_data_ccsid;
	int rfh1_data_encoding;
	int rfh1_data_ccsid;
	int buildMcdArea(int ccsid, int encoding);
	void parseRFH2mcd(unsigned char *rfhdata, int dataLen);
	void setMcdArea(unsigned char *mcdData, int dataLen, int ccsid, int encoding);
	void getFolderName(unsigned char *folderName, unsigned char *rfhdata, int segLen);
	void setRfhArea(unsigned char *rfhData, int dataLen, int ccsid, int encoding);
	void parseRFH2data(unsigned char *rfhData, int dataLen, int ccsid, int encoding);
	char * parseRFH1String(char *ptr, char *value, int maxSize);
	void setRfh1Area(unsigned char *rfhData, int dataLen, int ccsid, int encoding);
	void parseRFH1VarArea(char *rfhptr, int msglength);
	void convertRFH2Header(MQRFH2 *tempRFH, int encodeType);
	void convertRFHeader(MQRFH *tempRFH, int encodeType);
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	int m_rfh_data_len;
	int m_rfh1_data_len;
	int m_rfh_mcd_len;
	unsigned char * rfh_data;
	unsigned char * rfh1_data;
	unsigned char * rfh_mcd_area;
	BOOL m_rfh1_data_changed;
	BOOL m_rfh_data_changed;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	bool mcdDataChanged;
	void setDisplay();
	void setV2();
	void setV1();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RFH_H__232B777A_64B8_41AE_9107_8409A84074B5__INCLUDED_)
