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

#if !defined(AFX_JMS_H__AC90E404_5443_439A_9983_09B0592287CF__INCLUDED_)
#define AFX_JMS_H__AC90E404_5443_439A_9983_09B0592287CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// jms.h : header file
//

#include "DataArea.h"

/////////////////////////////////////////////////////////////////////////////
// jms dialog

class jms : public CPropertyPage
{
	DECLARE_DYNCREATE(jms)

// Construction
public:
	BOOL wasDataChanged();
	void setJMSMsgType(const char * domain);
	void clearJMSdata();
	char * getJmsArea(int ccsid, int encoding);
	int buildJmsArea(int ccsid, int encoding);
	void parseRFH2jms(unsigned char *rfhdata, int dataLen);
	void freeJmsArea();
	void setJmsArea(unsigned char *jmsData, int dataLen, int ccsid, int encoding);
	void updatePageData();
	DataArea* pDoc;
	jms();
	~jms();

// Dialog Data
	//{{AFX_DATA(jms)
	enum { IDD = IDD_JMS };
	CString	m_jms_dst;
	CString	m_jms_rto;
	int		m_jms_priority;
	CString	m_jms_expiration;
	int		m_jms_sequence;
	CString	m_jms_group_id;
	CString	m_jms_correl_id;
	int		m_jms_delivery_mode;
	int		m_jms_msg_type;
	CString	m_jms_timestamp;
	CString	m_jms_user_def;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(jms)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(jms)
	afx_msg void OnChangeJmsDst();
	afx_msg void OnChangeJmsCorrelId();
	afx_msg void OnChangeJmsDeliveryMode();
	afx_msg void OnChangeJmsExpiration();
	afx_msg void OnChangeJmsGroupId();
	afx_msg void OnChangeJmsPriority();
	afx_msg void OnChangeJmsRto();
	afx_msg void OnChangeJmsSequence();
	afx_msg void OnJmsBytes();
	afx_msg void OnJmsMap();
	afx_msg void OnJmsObject();
	afx_msg void OnJmsStream();
	afx_msg void OnJmsText();
	virtual BOOL OnInitDialog();
	afx_msg void OnJmsNone();
	afx_msg void OnChangeJmsTimestamp();
	afx_msg void OnChangeJmsUserDef();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	int m_RFH_jms_len;
	int jms_data_ccsid;
	int jms_data_encoding;
	unsigned char * rfh_jms_area;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	bool jmsDataChanged;
	char * parseRFH2misc(char * ptr,
						 char * endptr,
						 char * tempUser,
						 const char * endTag,
						 const int maxUser,
						 int * idx);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JMS_H__AC90E404_5443_439A_9983_09B0592287CF__INCLUDED_)
