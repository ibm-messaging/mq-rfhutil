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

//
// rfhutilView.h : interface of the CRfhutilView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RFHUTILVIEW_H__97A9D604_785C_43C1_8EDC_63437C4593E0__INCLUDED_)
#define AFX_RFHUTILVIEW_H__97A9D604_785C_43C1_8EDC_63437C4593E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "General.h"
#include "jms.h"
#include "MQMDPAGE.h"
#include "MSGDATA.h"
#include "other.h"
#include "pscr.h"
#include "PubSub.h"
#include "RFH.h"
#include "Usr.h"
#include "cics.h"
#include "ims.h"
#include "dlq.h"
#include "Props.h"
#include "PS.h"
#include "HexFind.h"

class CRfhutilView : public CCtrlView
{
protected: // create from serialization only
	CRfhutilView();
	DECLARE_DYNCREATE(CRfhutilView)

// Attributes
public:
	CRfhutilDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRfhutilView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);
	virtual void OnUpdateRecentFileMenu(CCmdUI* pCmdUI);
	//}}AFX_VIRTUAL

// Implementation
public:
	void UpdateAllFields();
	void editIDField(int id);
	virtual ~CRfhutilView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CTabCtrl&	GetTabCtrl ();

private:
	void OnPsClear();
	void OnPsResume();
	void OnPsPublish();
	void OnPsGetSubs();
	void OnPsGet();
	void SetActive(int sel);
	void KillActive(int sel);
	void getQnames();
	CString remoteQM;
	CString qName;
	CString qmName;
	int lastFindDType;
	void hexFindHelper(UINT wParam, LONG lParam);
	CString lastFindValue;
	CString findValue;
	UINT findMessage;
	UINT hexFindMessage;
	CFindReplaceDialog* findDialog;
	CHexFind * hexFindDialog;
	void updatePageData();
	void updateIdView();
	int lastChar;
	int firstChar;
	CFont *printerFont;
	int getLinesPerPage(CDC* pDC);
	int GetSizeInPicas(int nPicas, CDC* pDC);
	int prtLoc;
	int maxLines;
	int maxChars;
	int prtLen;
	int dpi;
	int m_fontSize;
	char * prtData;
	int currentSelection;
	BOOL GetToolTipText(UINT id, NMHDR * pTTTStruct, LRESULT * pResult );
	bool m_bInitialized;
	CFont m_tab_font;

// Generated message map functions
protected:
	//{{AFX_MSG(CRfhutilView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFilePageSetup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditSelectAll(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnFilePageSetup();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnUpdateFilePrintPreview(CCmdUI* pCmdUI);
	afx_msg void OnReadNocrlf();
	afx_msg void OnReadIgnoreHeader();
	afx_msg void OnReadUnix();
	afx_msg void OnUpdateReadDataonly(CCmdUI* pCmdUI);
	afx_msg void OnUpdateReadIgnoreHeader(CCmdUI* pCmdUI);
	afx_msg void OnUpdateReadNocrlf(CCmdUI* pCmdUI);
	afx_msg void OnUpdateReadUnix(CCmdUI* pCmdUI);
	afx_msg void OnWriteDataonly();
	afx_msg void OnUpdateWriteDataonly(CCmdUI* pCmdUI);
	afx_msg void OnReadSaveRfh();
	afx_msg void OnUpdateReadSaveRfh(CCmdUI* pCmdUI);
	afx_msg void OnViewSystemQueues();
	afx_msg void OnUpdateViewSystemQueues(CCmdUI* pCmdUI);
	afx_msg void OnCopyMsgId();
	afx_msg void OnSaveGroupId();
	afx_msg void OnSaveCorrelId();
	afx_msg void OnRestoreMsgId();
	afx_msg void OnRestoreGroupId();
	afx_msg void OnRestoreCorrelId();
	afx_msg void OnSaveMsgId();
	afx_msg void OnSaveCicsFacility();
	afx_msg void OnUpdateSaveCicsFacility(CCmdUI* pCmdUI);
	afx_msg void OnRestoreCicsFacility();
	afx_msg void OnUpdateRestoreCicsFacility(CCmdUI* pCmdUI);
	afx_msg void OnFileWrite();
	afx_msg void OnUpdateFileWrite(CCmdUI* pCmdUI);
	afx_msg void OnOpen();
	afx_msg void OnUpdateOpen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateReadIgnoreMqmd(CCmdUI* pCmdUI);
	afx_msg void OnReadIgnoreMqmd();
	afx_msg void OnWriteIncludeMqmd();
	afx_msg void OnUpdateWriteIncludeMqmd(CCmdUI* pCmdUI);
	afx_msg void OnSearchFind();
	afx_msg void OnUpdateSearchFind(CCmdUI* pCmdUI);
	afx_msg void OnSearchFindHex();
	afx_msg void OnUpdateSearchFindHex(CCmdUI* pCmdUI);
	afx_msg void OnSearchGoto();
	afx_msg void OnUpdateSearchGoto(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSearchNext(CCmdUI* pCmdUI);
	afx_msg void OnViewCluster();
	afx_msg void OnUpdateViewCluster(CCmdUI* pCmdUI);
	afx_msg void OnBrnext();
	afx_msg void OnBrprev();
	afx_msg void OnRestoreTransId();
	afx_msg void OnUpdateRestoreTransId(CCmdUI* pCmdUI);
	afx_msg void OnSaveImsTransid();
	afx_msg void OnUpdateSaveImsTransid(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMqMqclose(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMqMqopenPut(CCmdUI* pCmdUI);
	afx_msg void OnMqMqclose();
	afx_msg void OnMqMqconn();
	afx_msg void OnMqMqdisc();
	afx_msg void OnMqMqopenGet();
	afx_msg void OnMqMqopenPut();
	afx_msg void OnUpdateMqMqconn(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMqMqdisc(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMqMqopenGet(CCmdUI* pCmdUI);
	afx_msg void OnMqEndbr();
	afx_msg void OnUpdateMqEndbr(CCmdUI* pCmdUI);
	afx_msg void OnMqMqopenBrowse();
	afx_msg void OnUpdateMqMqopenBrowse(CCmdUI* pCmdUI);
	afx_msg void OnBrowseq();
	afx_msg void OnDisplayq();
	afx_msg void OnPurgeq();
	afx_msg void OnLoadnames();
	afx_msg void OnSetConnUser();
	afx_msg void OnClearall();
	afx_msg void OnCloseq();
	afx_msg void OnLoadq();
	afx_msg void OnReadq();
	afx_msg void OnSaveq();
	afx_msg void OnWriteq();
	afx_msg void OnStartbr();
	afx_msg void OnEndbrowse();
	afx_msg void OnCopybook();
	afx_msg void OnCopymsgid2correlid();
	afx_msg void OnResetids();
	afx_msg void OnNextTab();
	afx_msg void OnPrevTab();
	afx_msg void OnClearData();
	afx_msg void OnUpdateEditTop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditBottom(CCmdUI* pCmdUI);
	afx_msg void OnEditBottom();
	afx_msg void OnEditTop();
	afx_msg void OnTradChinese();
	afx_msg void OnSimpChinese();
	afx_msg void OnAscii();
	afx_msg void OnEbcdic();
	afx_msg void OnKorean();
	afx_msg void OnJson();
	afx_msg void OnFix();
	afx_msg void OnThai();
	afx_msg void OnWriteMsgs();
	//}}AFX_MSG
	afx_msg void findHelper(UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
	General		m_general;
	jms			m_jms;
	MQMDPAGE	m_mqmd;
	MSGDATA		m_data;
	other		m_other;
	pscr		m_pscr;
	PubSub		m_pubsub;
	RFH			m_rfh;
	Usr			m_usr;
	CCICS		m_cics;
	CIms		m_ims;
	CDlq		m_dlq;
	CProps		m_props;
	CPS			m_ps;
	void OnSelchangingSampletab (NMHDR* pNMHDR, LRESULT* pResult);
	void OnSelchangeSampletab (NMHDR* pNMHDR, LRESULT* pResult);
};

#ifndef _DEBUG  // debug version in rfhutilView.cpp
inline CRfhutilDoc* CRfhutilView::GetDocument()
   { return (CRfhutilDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RFHUTILVIEW_H__97A9D604_785C_43C1_8EDC_63437C4593E0__INCLUDED_)
