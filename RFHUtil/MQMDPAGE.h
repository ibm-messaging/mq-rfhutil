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

#if !defined(AFX_MQMD_H__550D3129_A3D4_4DAE_8341_48322F93EA72__INCLUDED_)
#define AFX_MQMD_H__550D3129_A3D4_4DAE_8341_48322F93EA72__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MQMDPAGE.h : header file
//

#include "DataArea.h"
#include "IdEdit.h"	// Added by ClassView
#include "MyComboBox.h"	// Added by ClassView

/////////////////////////////////////////////////////////////////////////////
// MQMDPAGE dialog

class MQMDPAGE : public CPropertyPage
{
	DECLARE_DYNCREATE(MQMDPAGE)

// Construction
public:
	int getFloatEncoding();
	void setGroupId(MQMD2 * mqmd);
	void setCorrelId(MQMD2 * mqmd);
	void setMsgId(MQMD2 * mqmd);
	void setUserId(MQMD2 * mqmd);
	void setMsgId(char * msgId);
	void editIDField(int id);
	void updateFields(const char *newFormat, int newCcsid, int newEncoding, int newPdEncoding, int newFloatEncoding);
	int getPdEncoding();
	int getEncoding();
	int getCcsid();
	void getFormat(char * mqformat);
	void restoreGroupId();
	void restoreCorrelId();
	void restoreMsgId();
	void saveGroupId();
	void saveCorrelId();
	void saveMsgId();
	void changeCorrelId(MQBYTE24 * correlid);
	MQBYTE24 m_message_id;
	MQBYTE24 m_correlid;
	MQBYTE24 m_group_id;
//	void extractMQMD(MQMD *mqmd, ImqMessage &msg);
	int getMQMD(unsigned char *msgData, int msgLen, int *ccsid, int * encoding);
	void setEncoding(int encoding, int pdEncoding, int floatEncoding);
	void setCcsid(int ccsid);
	void setFormat(const char * fmt);
	bool getSegmentActive();
	bool setGroupActive(BOOL m_logical_order);
	void updateMQMDafterPut(MQMD2 * mqmd, MQLONG msgLen);
	void setMessageMQMD(MQMD2 * mqmd, BOOL m_setUserID, BOOL m_set_all);
	void buildMQMD(MQMD2 * mqmd, BOOL m_setUserID, BOOL m_set_all);
	void clearMQMD();
	void processMessageMQMD(MQMD2 * mqmd);
	int processMQMDE(MQMDE * mqmde, int size, int ccsid, int encoding);
	int processMQMD(const unsigned char * data, int size);
	void CopyMsgIdToCorrelId();
	void ResetIds();
	void UpdatePageData();
	DataArea* pDoc;
	MQMDPAGE();
	~MQMDPAGE();

// Dialog Data
	//{{AFX_DATA(MQMDPAGE)
	enum { IDD = IDD_MQMD };
	CString	m_mqmd_correl_id;
	CString	m_mqmd_ccsid;
	CString	m_mqmd_format;
	CString	m_msg_id;
	int		m_mqmd_persist;
	CString	m_reply_qm;
	CString	m_reply_queue;
	CString	m_mqmd_appl_id;
	CString	m_mqmd_appl_origin;
	CString	m_mqmd_appl_name;
	CString	m_mqmd_appl_type;
	int		m_mqmd_encoding;
	int		m_mqmd_float_encoding;
	CString	m_mqmd_userid;
	CString	m_mqmd_group_id;
	CString	m_mqmd_backout_count;
	int		m_id_disp_ascii;
	CString	m_mqmd_seq_no;
	CString	m_mqmd_account_token;
	CString	m_mqmd_date_time;
	CString	m_mqmd_expiry;
	CString	m_mqmd_msgtype;
	int		m_mqmd_pd_encoding;
	int		m_report_coa;
	int		m_report_cod;
	int		m_report_except;
	int		m_report_expire;
	BOOL	m_mqmd_segment_last;
	BOOL	m_mqmd_segment_yes;
	CString	m_mqmd_offset;
	CString	m_mqmd_orig_len;
	CString	m_mqmd_feedback;
	CString	m_mqmd_priority;
	BOOL	m_mqmd_group_last;
	BOOL	m_mqmd_group_yes;
	BOOL	m_segment_allowed;
	BOOL	m_report_nan;
	BOOL	m_report_pan;
	BOOL	m_report_activity;
	BOOL	m_report_discard;
	BOOL	m_report_pass_msgid;
	BOOL	m_report_pass_correlid;
	BOOL	m_report_expire_discard;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(MQMDPAGE)
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
	//{{AFX_MSG(MQMDPAGE)
	afx_msg void OnCopyMsgid();
	afx_msg void OnResetIds();
	afx_msg void OnIdDispAscii();
	afx_msg void OnIdDispEbcdic();
	afx_msg void OnHex();
	afx_msg void OnUpdateGroupId();
	afx_msg void OnUpdateCorrelId();
	afx_msg void OnSetfocusMsgtype();
	afx_msg void OnSelchangeMsgtype();
	afx_msg void OnKillfocusMsgtype();
	virtual BOOL OnInitDialog();
	afx_msg void OnUpdateMqmdCcsid();
	afx_msg void OnMqmdPc();
	afx_msg void OnMqmdHost();
	afx_msg void OnMqmdUnix();
	afx_msg void OnSelchangeApplType();
	afx_msg void OnSetfocusApplType();
	afx_msg void OnSetfocusCorrelId();
	afx_msg void OnKillfocusCorrelId();
	afx_msg void OnChangeCorrelId();
	afx_msg void OnSetfocusGroupId();
	afx_msg void OnKillfocusGroupId();
	afx_msg void OnSetfocusMsgId();
	afx_msg void OnKillfocusMsgId();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void setMsgId(int dispType);
	CIdEdit m_MsgIdEdit;
	CIdEdit m_CorrelIdEdit;
	CIdEdit m_GroupIdEdit;
	MyComboBox m_mqmd_format_cb;
	int getPersistence();
	int buildFlags();
	int buildReportOptions();
	void setGroupId();
	void setCorrelId(int dispType);
	void SetId(MQBYTE24 * id, int idType);
	void processFlags(int flags);
	void processReportOptions(int report);
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	void setPutApplType(int putType);
	BOOL putApplTypeInit;
	int getPutApplType(const char *applType);
	void setMsgType(int msgType);
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	CString saveMsgType;
	CString OrigGroupid;
	CString OrigCorrelid;
	CString OrigMsgid;
	int msgTypeInit;
	int prev_disp_type;
	void setMaxFieldLengths(int dispType);
	void updateIdFields();
	MQBYTE24 m_saved_group_id;
	MQBYTE24 m_saved_correlid;
	MQBYTE24 m_saved_msg_id;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MQMD_H__550D3129_A3D4_4DAE_8341_48322F93EA72__INCLUDED_)
