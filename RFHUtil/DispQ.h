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

#if !defined(AFX_DISPQ_H__93FE7C53_3B2B_4B92_B34F_677D46A591FF__INCLUDED_)
#define AFX_DISPQ_H__93FE7C53_3B2B_4B92_B34F_677D46A591FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DispQ.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDispQ dialog

class CDispQ : public CDialog
{
// Construction
public:
	int m_msg_id_count;
	MQBYTE24 m_msg_id;
	MQBYTE24 m_prev_msg_id;
	CString m_q_name;
	CString m_qmgr_name;
	int m_selCount;
	int m_sel;
	int m_selectedLine;
	int	m_read_type;
	DataArea* pDoc;

	CDispQ(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDispQ();

// Dialog Data
	//{{AFX_DATA(CDispQ)
	enum { IDD = IDD_DISP_Q };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDispQ)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnCancel();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDispQ)
	afx_msg void OnDispqClose();
	afx_msg void OnDispqReadq();
	afx_msg void OnDispqStartBrowse();
	afx_msg void OnDispqBrowseq();
	afx_msg void OnItemchangedDispqMsgs(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void insertMessage(CListCtrl * lc, MQMD2 * mqmd, int msgLen);
	void setMsgId(BOOL setPrev);
	void freeMsgIdTable();
	char * m_msg_id_table;
	char strTitle[96];
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISPQ_H__93FE7C53_3B2B_4B92_B34F_677D46A591FF__INCLUDED_)
