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

// rfhutilView.cpp : implementation of the CRfhutilView class
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"

#include "rfhutilDoc.h"
#include "rfhutilView.h"
#include "DataArea.h"
// goto dialog
#include "goto.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRfhutilView

IMPLEMENT_DYNCREATE(CRfhutilView, CCtrlView)

BEGIN_MESSAGE_MAP(CRfhutilView, CCtrlView)
	//{{AFX_MSG_MAP(CRfhutilView)
	ON_WM_SIZE()
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateEditSelectAll)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(IDM_READ_NOCRLF, OnReadNocrlf)
	ON_COMMAND(IDM_READ_IGNORE_HEADER, OnReadIgnoreHeader)
	ON_COMMAND(IDM_READ_UNIX, OnReadUnix)
	ON_UPDATE_COMMAND_UI(IDM_READ_DATAONLY, OnUpdateReadDataonly)
	ON_UPDATE_COMMAND_UI(IDM_READ_IGNORE_HEADER, OnUpdateReadIgnoreHeader)
	ON_UPDATE_COMMAND_UI(IDM_READ_NOCRLF, OnUpdateReadNocrlf)
	ON_UPDATE_COMMAND_UI(IDM_READ_UNIX, OnUpdateReadUnix)
	ON_COMMAND(IDM_WRITE_DATAONLY, OnWriteDataonly)
	ON_UPDATE_COMMAND_UI(IDM_WRITE_DATAONLY, OnUpdateWriteDataonly)
	ON_COMMAND(IDM_READ_SAVE_RFH, OnReadSaveRfh)
	ON_UPDATE_COMMAND_UI(IDM_READ_SAVE_RFH, OnUpdateReadSaveRfh)
	ON_COMMAND(IDM_VIEW_SYSTEM_QUEUES, OnViewSystemQueues)
	ON_UPDATE_COMMAND_UI(IDM_VIEW_SYSTEM_QUEUES, OnUpdateViewSystemQueues)
	ON_COMMAND(IDM_COPY_MSG_ID, OnCopyMsgId)
	ON_COMMAND(IDM_SAVE_GROUP_ID, OnSaveGroupId)
	ON_COMMAND(IDM_SAVE_CORREL_ID, OnSaveCorrelId)
	ON_COMMAND(IDM_RESTORE_MSG_ID, OnRestoreMsgId)
	ON_COMMAND(IDM_RESTORE_GROUP_ID, OnRestoreGroupId)
	ON_COMMAND(IDM_RESTORE_CORREL_ID, OnRestoreCorrelId)
	ON_COMMAND(IDM_SAVE_MSG_ID, OnSaveMsgId)
	ON_COMMAND(IDM_SAVE_CICS_FACILITY, OnSaveCicsFacility)
	ON_UPDATE_COMMAND_UI(IDM_SAVE_CICS_FACILITY, OnUpdateSaveCicsFacility)
	ON_COMMAND(IDM_RESTORE_CICS_FACILITY, OnRestoreCicsFacility)
	ON_UPDATE_COMMAND_UI(IDM_RESTORE_CICS_FACILITY, OnUpdateRestoreCicsFacility)
	ON_COMMAND(IDM_FILE_WRITE, OnFileWrite)
	ON_UPDATE_COMMAND_UI(IDM_FILE_WRITE, OnUpdateFileWrite)
	ON_COMMAND(IDC_OPEN, OnOpen)
	ON_UPDATE_COMMAND_UI(IDC_OPEN, OnUpdateOpen)
	ON_UPDATE_COMMAND_UI(IDM_READ_IGNORE_MQMD, OnUpdateReadIgnoreMqmd)
	ON_COMMAND(IDM_READ_IGNORE_MQMD, OnReadIgnoreMqmd)
	ON_COMMAND(IDM_WRITE_INCLUDE_MQMD, OnWriteIncludeMqmd)
	ON_UPDATE_COMMAND_UI(IDM_WRITE_INCLUDE_MQMD, OnUpdateWriteIncludeMqmd)
	ON_COMMAND(IDM_SEARCH_FIND, OnSearchFind)
	ON_UPDATE_COMMAND_UI(IDM_SEARCH_FIND, OnUpdateSearchFind)
	ON_COMMAND(IDM_SEARCH_FIND_HEX, OnSearchFindHex)
	ON_UPDATE_COMMAND_UI(IDM_SEARCH_FIND_HEX, OnUpdateSearchFindHex)
	ON_COMMAND(IDM_SEARCH_GOTO, OnSearchGoto)
	ON_UPDATE_COMMAND_UI(IDM_SEARCH_GOTO, OnUpdateSearchGoto)
	ON_COMMAND(IDM_VIEW_CLUSTER, OnViewCluster)
	ON_UPDATE_COMMAND_UI(IDM_VIEW_CLUSTER, OnUpdateViewCluster)
	ON_COMMAND(ID_BRNEXT, OnBrnext)
	ON_COMMAND(ID_BRPREV, OnBrprev)
	ON_COMMAND(IDM_RESTORE_TRANS_ID, OnRestoreTransId)
	ON_UPDATE_COMMAND_UI(IDM_RESTORE_TRANS_ID, OnUpdateRestoreTransId)
	ON_COMMAND(IDM_SAVE_IMS_TRANSID, OnSaveImsTransid)
	ON_UPDATE_COMMAND_UI(IDM_SAVE_IMS_TRANSID, OnUpdateSaveImsTransid)
	ON_UPDATE_COMMAND_UI(IDM_MQ_MQCLOSE, OnUpdateMqMqclose)
	ON_UPDATE_COMMAND_UI(IDM_MQ_MQOPEN_PUT, OnUpdateMqMqopenPut)
	ON_COMMAND(IDM_MQ_MQCLOSE, OnMqMqclose)
	ON_COMMAND(IDM_MQ_MQCONN, OnMqMqconn)
	ON_COMMAND(IDM_MQ_MQDISC, OnMqMqdisc)
	ON_COMMAND(IDM_MQ_MQOPEN_GET, OnMqMqopenGet)
	ON_COMMAND(IDM_MQ_MQOPEN_PUT, OnMqMqopenPut)
	ON_UPDATE_COMMAND_UI(IDM_MQ_MQCONN, OnUpdateMqMqconn)
	ON_UPDATE_COMMAND_UI(IDM_MQ_MQDISC, OnUpdateMqMqdisc)
	ON_UPDATE_COMMAND_UI(IDM_MQ_MQOPEN_GET, OnUpdateMqMqopenGet)
	ON_COMMAND(IDM_MQ_ENDBR, OnMqEndbr)
	ON_UPDATE_COMMAND_UI(IDM_MQ_ENDBR, OnUpdateMqEndbr)
	ON_COMMAND(IDM_MQ_MQOPEN_BROWSE, OnMqMqopenBrowse)
	ON_UPDATE_COMMAND_UI(IDM_MQ_MQOPEN_BROWSE, OnUpdateMqMqopenBrowse)
	ON_COMMAND(ID_BROWSEQ, OnBrowseq)
	ON_COMMAND(ID_DISPLAYQ, OnDisplayq)
	ON_COMMAND(ID_PURGEQ, OnPurgeq)
	ON_COMMAND(ID_LOADNAMES, OnLoadnames)
	ON_COMMAND(ID_SET_CONN_USER, OnSetConnUser)
	ON_COMMAND(ID_CLEARALL, OnClearall)
	ON_COMMAND(ID_CLOSEQ, OnCloseq)
	ON_COMMAND(ID_LOADQ, OnLoadq)
	ON_COMMAND(ID_READQ, OnReadq)
	ON_COMMAND(ID_SAVEQ, OnSaveq)
	ON_COMMAND(ID_WRITEQ, OnWriteq)
	ON_COMMAND(ID_STARTBR, OnStartbr)
	ON_COMMAND(ID_ENDBROWSE, OnEndbrowse)
	ON_COMMAND(ID_COPYBOOK, OnCopybook)
	ON_COMMAND(ID_COPYMSGID2CORRELID, OnCopymsgid2correlid)
	ON_COMMAND(ID_RESETIDS, OnResetids)
	ON_COMMAND(IDM_NEXT_TAB, OnNextTab)
	ON_COMMAND(IDM_PREV_TAB, OnPrevTab)
	ON_COMMAND(IDC_CLEAR_DATA, OnClearData)
	ON_UPDATE_COMMAND_UI(ID_EDIT_TOP, OnUpdateEditTop)
	ON_UPDATE_COMMAND_UI(ID_EDIT_BOTTOM, OnUpdateEditBottom)
	ON_COMMAND(ID_EDIT_BOTTOM, OnEditBottom)
	ON_COMMAND(ID_EDIT_TOP, OnEditTop)
	ON_COMMAND(ID_TRAD_CHINESE, OnTradChinese)
	ON_COMMAND(ID_SIMP_CHINESE, OnSimpChinese)
	ON_COMMAND(ID_ASCII, OnAscii)
	ON_COMMAND(ID_EBCDIC, OnEbcdic)
	ON_COMMAND(ID_KOREAN, OnKorean)
	ON_COMMAND(IDM_PS_GET_MSG, OnPsGet)
	ON_COMMAND(IDM_PS_GET_SUBS, OnPsGetSubs)
	ON_COMMAND(IDM_PS_PUBLISH, OnPsPublish)
	ON_COMMAND(IDM_PS_RESUME, OnPsResume)
	ON_COMMAND(IDM_PS_CLEAR_ALL, OnPsClear)
	ON_COMMAND(IDM_JSON, OnJson)
	ON_COMMAND(IDM_FIX, OnFix)
	ON_COMMAND(IDM_THAI, OnThai)
	ON_COMMAND(ID_PS_WRITE_MSGS, OnWriteMsgs)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelchangeSampletab)
	ON_NOTIFY_REFLECT(TCN_SELCHANGING, OnSelchangingSampletab)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE1 + 15, OnUpdateRecentFileMenu)
//	ON_REGISTERED_MESSAGE( findMessage, findHelper)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRfhutilView construction/destruction

CRfhutilView::CRfhutilView()
	:CCtrlView(_T("SysTabControl32"), AFX_WS_DEFAULT_VIEW | WS_TABSTOP | WS_GROUP)
{
	CRfhutilApp*	app;
	HDC				hDC = NULL;
	LOGFONT			lf;

	// clear the logical font structure
	memset(&lf, 0, sizeof(lf));

	// initialize various variables
	m_bInitialized = false;
	currentSelection = PAGE_MAIN;
	findValue.Empty();
	lastFindValue.Empty();
	lastFindDType = 0;
	findDialog = NULL;
	hexFindDialog = NULL;
	printerFont = NULL;
	dpi = 0;
	m_fontSize = 14;
	lastChar = 0;
	firstChar = 0;
	prtLoc = 0;
	prtLen = 0;
	prtData = NULL;
	maxLines = 0;
	maxChars = 0;

	// register our two special windows messages that are used for
	// the find dialogs, which run as modeless dialogs
	findMessage = ::RegisterWindowMessage(FINDMSGSTRING);
	hexFindMessage = ::RegisterWindowMessage(HEX_FINDER_DIALOG);

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// set a pointer to the dataarea object in each dialog object
	m_general.pDoc = &app->pDocument;
	m_ps.pDoc = &app->pDocument;
	m_jms.pDoc = &app->pDocument;
	m_mqmd.pDoc = &app->pDocument;
	m_props.pDoc = &app->pDocument;
	m_data.pDoc = &app->pDocument;
	m_other.pDoc = &app->pDocument;
	m_pscr.pDoc = &app->pDocument;
	m_pubsub.pDoc = &app->pDocument;
	m_rfh.pDoc = &app->pDocument;
	m_usr.pDoc = &app->pDocument;
	m_cics.pDoc = &app->pDocument;
	m_ims.pDoc = &app->pDocument;
	m_dlq.pDoc = &app->pDocument;

	// set a pointer to some dialog objects in the dataarea object
	app->pDocument.cicsData = ((CObject *)&m_cics);
	app->pDocument.imsData = ((CObject *)&m_ims);
	app->pDocument.dlqData = ((CObject *)&m_dlq);
	app->pDocument.rfhData = ((CObject *)&m_rfh);
	app->pDocument.jmsData = ((CObject *)&m_jms);
	app->pDocument.pubData = ((CObject *)&m_pubsub);
	app->pDocument.pscrData = ((CObject *)&m_pscr);
	app->pDocument.usrData = ((CObject *)&m_usr);
	app->pDocument.otherData = ((CObject *)&m_other);
	app->pDocument.mqmdData = ((CObject *)&m_mqmd);
	app->pDocument.propData = ((CObject *)&m_props);
	app->pDocument.psData = ((CObject *)&m_ps);

	// set the pointers for the rfh object to use
	m_rfh.jmsData = (CObject *)&m_jms;
	m_rfh.pubsubData = (CObject *)&m_pubsub;
	m_rfh.pscrData = (CObject *)&m_pscr;
	m_rfh.usrData = (CObject *)&m_usr;
	m_rfh.otherData = (CObject *)&m_other;

	// get the device characteristics
	// get the device context handle
	hDC = GetDC()->GetSafeHdc();
	if (hDC != NULL)
	{
		// Get the display device characteristics
		app->technology = GetDeviceCaps(hDC, TECHNOLOGY);
		app->horSize = GetDeviceCaps(hDC, HORZSIZE);
		app->vertSize = GetDeviceCaps(hDC, VERTSIZE);
		app->horRes = GetDeviceCaps(hDC, HORZRES);
		app->vertRes = GetDeviceCaps(hDC, VERTRES);
		app->horLogPixels = GetDeviceCaps(hDC, LOGPIXELSX);
		app->vertLogPixels = GetDeviceCaps(hDC, LOGPIXELSY);
		dpi = app->vertLogPixels;
	}

	// check if dpi is higher resolution
	if (dpi > 125)
	{
		// increase the size of the tab font
		m_fontSize = (m_fontSize * dpi) / 100;
	}

	// create a font for the tabs to use
	lf.lfHeight = m_fontSize;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfPitchAndFamily = DEFAULT_PITCH;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	//strcpy(lf.lfFaceName, "Courier New");

	m_tab_font.CreateFontIndirectA(&lf);
}

CRfhutilView::~CRfhutilView()
{
}

BOOL CRfhutilView::PreCreateWindow(CREATESTRUCT& cs)
{
	// Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	::InitCommonControls ();

	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	return CCtrlView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CRfhutilView printing

BOOL CRfhutilView::OnPreparePrinting(CPrintInfo* pInfo)

{
	// start at the beginning of the file
	prtLoc = 0;

	// figure out what we want to print
	switch (currentSelection)
	{
	case PAGE_DATA:
		{
			prtData = m_data.m_msg_data.GetBuffer(10);
			prtLen = m_data.m_msg_data.GetLength();

			// check for a selection
			m_data.getSelection(firstChar, lastChar);
			if (firstChar == lastChar)
			{
				// no text is currently selected
				firstChar = 0;
				pInfo->m_pPD->m_pd.Flags |= PD_NOSELECTION;
			}
			else
			{
				// enable selections in the print dialog
				pInfo->m_pPD->m_pd.Flags |= PD_SELECTION;
				pInfo->m_pPD->m_pd.Flags &= pInfo->m_pPD->m_pd.Flags - PD_NOSELECTION;
			}

			break;
		}
	case PAGE_USR:
		{
			prtData = m_usr.m_rfh_usr_data.GetBuffer(10);
			prtLen = m_usr.m_rfh_usr_data.GetLength();

			// check for a selection
			m_usr.getSelection(firstChar, lastChar);
			if (firstChar == lastChar)
			{
				// no text is currently selected
				firstChar = 0;
				pInfo->m_pPD->m_pd.Flags |= PD_NOSELECTION;
			}
			else
			{
				// enable selections in the print dialog
				pInfo->m_pPD->m_pd.Flags |= PD_SELECTION;
				pInfo->m_pPD->m_pd.Flags &= pInfo->m_pPD->m_pd.Flags - PD_NOSELECTION;
			}

			break;
		}
	case PAGE_OTHER:
		{
			prtData = m_other.m_rfh_other_data.GetBuffer(10);
			prtLen = m_other.m_rfh_other_data.GetLength();

			// check for a selection
			m_other.getSelection(firstChar, lastChar);
			if (firstChar == lastChar)
			{
				// no text is currently selected
				firstChar = 0;
				pInfo->m_pPD->m_pd.Flags |= PD_NOSELECTION;
			}
			else
			{
				// enable selections in the print dialog
				pInfo->m_pPD->m_pd.Flags |= PD_SELECTION;
				pInfo->m_pPD->m_pd.Flags &= pInfo->m_pPD->m_pd.Flags - PD_NOSELECTION;
			}

			break;
		}
	default:
		{
			pInfo->m_bContinuePrinting = FALSE;
			break;
		}
	}

	// remove trailing blanks, etc
	while ((prtLen > 0) && (prtData[prtLen] <= ' '))
	{
		prtLen--;
	}

	pInfo->SetMinPage(1);
	pInfo->m_bContinuePrinting = TRUE;
	pInfo->m_pPD->m_pd.Flags |= PD_NOPAGENUMS;

	if (pInfo->m_bPreview)
	{
		pInfo->SetMaxPage(1);
	}

	// default preparation
	return DoPreparePrinting(pInfo);
}

void CRfhutilView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();
	LOGFONT		lf;
	DataArea	*pDoc;

	// initialization before printing
	BeginWaitCursor();

	// get a pointer to the DataArea object
	pDoc = &(app->pDocument);

	// check if we are printing the whole file or just the selected text
	if ((pInfo->m_pPD->m_pd.Flags & PD_SELECTION) > 0)
	{
		if (lastChar <= prtLen)
		{
			prtLen = lastChar;
			prtLoc = firstChar;
		}
	}

	printerFont = new CFont;

	// set the mapping mode to lometric
	pDC->SetMapMode(MM_LOMETRIC);

	// figure out which font to use and get the logical font characteristics
	switch (m_data.m_character_format)
	{
	case CHAR_CHINESE:
		{
			if (app->m_china_font.GetLogFont(&lf))
			{
				if (!pInfo->m_bPreview)
				{
					lf.lfHeight = 140;
				}
					
				printerFont->CreatePointFontIndirect(&lf, pDC);
//				pDC->SelectObject(m_data.m_china_font);
				pDC->SelectObject(printerFont);
			}
			else
				pInfo->m_bContinuePrinting = FALSE;

			break;
		}
	case CHAR_KOREAN:
		{
			if (app->m_korea_font.GetLogFont(&lf))
			{
				lf.lfHeight = 140;
				printerFont->CreatePointFontIndirect(&lf, pDC);
//				pDC->SelectObject(m_data.m_korea_font);
				pDC->SelectObject(printerFont);
			}
			else
				pInfo->m_bContinuePrinting = FALSE;

			break;
		}
	case CHAR_TRAD_CHIN:
		{
			if (app->m_big5_font.GetLogFont(&lf))
			{
				lf.lfHeight = 140;
				printerFont->CreatePointFontIndirect(&lf, pDC);
//				pDC->SelectObject(m_data.m_big5_font);
				pDC->SelectObject(printerFont);
			}
			else
				pInfo->m_bContinuePrinting = FALSE;

			break;
		}
	case CHAR_JAPANESE:
		{
			if (app->m_japan_font.GetLogFont(&lf))
			{
				lf.lfHeight = 140;
				printerFont->CreatePointFontIndirect(&lf, pDC);
//				pDC->SelectObject(m_data.m_japan_font);
				pDC->SelectObject(printerFont);
			}
			else
				pInfo->m_bContinuePrinting = FALSE;

			break;
		}
	default:
		{
			if (app->m_fixed_font.GetLogFont(&lf))
			{
				if (!pInfo->m_bPreview)
				{
					lf.lfHeight = -120;
					printerFont->CreatePointFontIndirect(&lf, pDC);
				}
				else
				{
					printerFont->CreateFontIndirect(&lf);
				}

//				pDC->SelectObject(m_data.m_fixed_font);
				pDC->SelectObject(printerFont);
			}
			else
				pInfo->m_bContinuePrinting = FALSE;

			break;
		}
	}
}

void CRfhutilView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	// cleanup after printing
	delete printerFont;

	EndWaitCursor();
}

/////////////////////////////////////////////////////////////////////////////
// CRfhutilView diagnostics

#ifdef _DEBUG
void CRfhutilView::AssertValid() const
{
	CCtrlView::AssertValid();
}

void CRfhutilView::Dump(CDumpContext& dc) const
{
	CCtrlView::Dump(dc);
}

CRfhutilDoc* CRfhutilView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRfhutilDoc)));
	return (CRfhutilDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRfhutilView message handlers

CTabCtrl& CRfhutilView::GetTabCtrl()

{
	return (*((CTabCtrl *) this));
}

void CRfhutilView::OnInitialUpdate() 

{
	CRfhutilApp		*app=(CRfhutilApp *)AfxGetApp();

// Do not call the base class OnInitialUpdate
//	CCtrlView::OnInitialUpdate();
	GetTabCtrl().ModifyStyleEx(0,WS_EX_CONTROLPARENT);
	LONG style = GetTabCtrl().GetStyle();
	GetTabCtrl().ModifyStyle(0, WS_TABSTOP | WS_GROUP);
	LONG style2 = GetTabCtrl().GetStyle();

	GetTabCtrl().SetFont(&m_tab_font, 0);
	
	if (m_bInitialized == true)
	{
		return;
	}

	// get the main frame window
	app->m_mainWnd = GetTabCtrl().GetParent();

	// create the property pages
	m_general.Create(IDD_MAIN, this);
	m_ps.Create(IDD_V7PUBSUB, this);
	m_props.Create(IDD_V7PROPS, this);
	m_jms.Create(IDD_JMS, this);
	m_mqmd.Create(IDD_MQMD, this);
	m_data.Create(IDD_DATA, this);
	m_other.Create(IDD_OTHER, this);
	m_pscr.Create(IDD_PSCR, this);
	m_rfh.Create(IDD_RFH, this);
	m_pubsub.Create(IDD_PUB, this);
	m_usr.Create(IDD_USR, this);
	m_cics.Create(IDD_CICS, this);
	m_ims.Create(IDD_IMS, this);
	m_dlq.Create(IDD_DLQ, this);

	// try to set the font for the data display
	m_data.SetFont(&(app->m_fixed_font));
	m_usr.SetFont(&(app->m_fixed_font));

	m_general.ModifyStyleEx(0, WS_EX_CONTROLPARENT);
	m_general.ModifyStyle(0, WS_TABSTOP | WS_GROUP);

	// add the property pages to the tab control
	TCITEM TabItem;
	memset (&TabItem, '\0', sizeof (TCITEM));
	TabItem.mask = TCIF_TEXT | TCIF_PARAM;

	TabItem.lParam = (long) &m_general;
	TabItem.pszText = "Main";
	BOOL result = GetTabCtrl().InsertItem (0, &TabItem);

	TabItem.pszText = "Data";
	TabItem.lParam = (long) &m_data;
	result = GetTabCtrl().InsertItem (1, &TabItem);

	TabItem.pszText = "MQMD";
	TabItem.lParam = (long) &m_mqmd;
	result = GetTabCtrl().InsertItem (2, &TabItem);

	TabItem.lParam = (long) &m_ps;
	TabItem.pszText = "PS";
	result = GetTabCtrl().InsertItem (3, &TabItem);

	TabItem.pszText = "Usr Prop";
	TabItem.lParam = (long) &m_props;
	result = GetTabCtrl().InsertItem (4, &TabItem);

	TabItem.pszText = "RFH";
	TabItem.lParam = (long) &m_rfh;
	result = GetTabCtrl().InsertItem (5, &TabItem);

	TabItem.pszText = "PubSub";
	TabItem.lParam = (long) &m_pubsub;
	result = GetTabCtrl().InsertItem (6, &TabItem);

	TabItem.pszText = "pscr";
	TabItem.lParam = (long) &m_pscr;
	result = GetTabCtrl().InsertItem (7, &TabItem);

	TabItem.pszText = "jms";
	TabItem.lParam = (long) &m_jms;
	result = GetTabCtrl().InsertItem (8, &TabItem);

	TabItem.pszText = "usr";
	TabItem.lParam = (long) &m_usr;
	result = GetTabCtrl().InsertItem (9, &TabItem);

	TabItem.pszText = "other";
	TabItem.lParam = (long) &m_other;
	result = GetTabCtrl().InsertItem (10, &TabItem);

	TabItem.pszText = "CICS";
	TabItem.lParam = (long) &m_cics;
	result = GetTabCtrl().InsertItem (11, &TabItem);

	TabItem.pszText = "IMS";
	TabItem.lParam = (long) &m_ims;
	result = GetTabCtrl().InsertItem (12, &TabItem);

	TabItem.pszText = "DLQ";
	TabItem.lParam = (long) &m_dlq;
	result = GetTabCtrl().InsertItem (13, &TabItem);

	GetTabCtrl().SetCurSel(0);
	GetTabCtrl().ShowWindow (SW_NORMAL);

	CRect rcItem, rc;
	GetTabCtrl().GetItemRect (0, rcItem);
	GetTabCtrl().GetClientRect (rc);
	rc.top = rcItem.bottom + 4;
	m_general.MoveWindow (rc, FALSE);
	m_data.MoveWindow (rc, FALSE);
	m_mqmd.MoveWindow (rc, FALSE);
	m_ps.MoveWindow (rc, FALSE);
	m_props.MoveWindow (rc, FALSE);
	m_rfh.MoveWindow (rc, FALSE);
	m_pubsub.MoveWindow (rc, FALSE);
	m_pscr.MoveWindow (rc, FALSE);
	m_jms.MoveWindow (rc, FALSE);
	m_usr.MoveWindow (rc, FALSE);
	m_other.MoveWindow (rc, FALSE);
	m_cics.MoveWindow (rc, FALSE);
	m_ims.MoveWindow (rc, FALSE);
	m_dlq.MoveWindow (rc, FALSE);

	EnableToolTips(TRUE);   // enable tool tips for view

  	m_bInitialized = true;
}

void CRfhutilView::OnSize(UINT nType, int cx, int cy) 

{
	CCtrlView::OnSize(nType, cx, cy);
	
	// handle sizing requests
	CRect rcItem, rc;
	GetTabCtrl().GetItemRect (0, rcItem);
	GetTabCtrl().GetClientRect (rc);
	rc.top = rcItem.bottom + 4;

	if (m_general.m_hWnd != 0)
		m_general.MoveWindow (rc, FALSE);

	if (m_ps.m_hWnd != 0)
		m_ps.MoveWindow (rc, FALSE);

	if (m_jms.m_hWnd != 0)
		m_jms.MoveWindow (rc, FALSE);

	if (m_mqmd.m_hWnd != 0)
		m_mqmd.MoveWindow (rc, FALSE);

	if (m_props.m_hWnd != 0)
		m_props.MoveWindow (rc, FALSE);

	if (m_data.m_hWnd != 0)
		m_data.MoveWindow (rc, FALSE);

	if (m_other.m_hWnd != 0)
		m_other.MoveWindow (rc, FALSE);

	if (m_pscr.m_hWnd != 0)
		m_pscr.MoveWindow (rc, FALSE);

	if (m_pubsub.m_hWnd != 0)
		m_pubsub.MoveWindow (rc, FALSE);

	if (m_rfh.m_hWnd != 0)
		m_rfh.MoveWindow (rc, FALSE);

	if (m_usr.m_hWnd != 0)
		m_usr.MoveWindow (rc, FALSE);

	if (m_cics.m_hWnd != 0)
		m_cics.MoveWindow (rc, FALSE);

	if (m_ims.m_hWnd != 0)
		m_ims.MoveWindow (rc, FALSE);

	if (m_dlq.m_hWnd != 0)
		m_dlq.MoveWindow (rc, FALSE);
}

void CRfhutilView::OnSelchangingSampletab (NMHDR* pNMHDR, LRESULT* pResult)

{
	// notify the dialog that it is losing focus
	TCITEM		ti;
	CWnd*		cDlg=NULL;
	int			result;
	int			sel;

	// check if a find, findhex or goto dialog has been displayed
	if ((NULL == findDialog) && (NULL == hexFindDialog))
	{
		// nothing displayed, so continue with the change
		sel = GetTabCtrl().GetCurSel();
		ti.mask = TCIF_PARAM;
		result = GetTabCtrl().GetItem (sel, &ti);

		if (result)
		{
			cDlg = (CWnd *) ti.lParam;
			if (cDlg)
				cDlg->ShowWindow (SW_HIDE);

			KillActive(sel);
		}
	}

	// indicate it was handled
	*pResult = 0;
}

void CRfhutilView::OnSelchangeSampletab (NMHDR* pNMHDR, LRESULT* pResult)

{
	// notify dialog that it is becoming active
	TC_ITEM		ti;
	CWnd*		cDlg;
	int			result;
	int			sel;

	// check if a find, findhex or goto dialog has been displayed
	if ((NULL == findDialog) && (NULL == hexFindDialog))
	{
		sel = GetTabCtrl().GetCurSel();
		ti.mask = TCIF_PARAM;
		result = GetTabCtrl().GetItem (sel, &ti);

		if (result)
		{
			cDlg = (CWnd *) ti.lParam;
			if (cDlg)
				cDlg->ShowWindow (SW_NORMAL);

			SetActive(sel);
		}

		// remember the current selection
		currentSelection = sel;
	}

	*pResult = 0;
}

BOOL CRfhutilView::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

{

	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pTTTStruct;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pTTTStruct;
	CString strTipText;

	UINT nID = pTTTStruct->idFrom;

	if (pTTTStruct->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pTTTStruct->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
//	if (pTTT->uFlags & TTF_IDISHWND)
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
		if(nID != 0)
		{
			strTipText.Format("Control ID = %d", nID);
//			lstrcpyn(pTTT->szText, strTipText, sizeof(pTTT->szText));

			if (pTTTStruct->code == TTN_NEEDTEXTA) {
				(void)lstrcpyn(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
			} 
			else {
				// Convert based on number of multibyte chars, not the size (bytes) of the buffer
				_mbstowcsz(pTTTW->szText, strTipText, sizeof(pTTTW->szText) / sizeof(pTTTW->szText[0]));
			}
			pResult = 0;
//			pTTT->lpszText = MAKEINTRESOURCE(nID);
//			pTTT->hinst = AfxGetResourceHandle();
			return(TRUE);
		}
	}

	return(FALSE);
}

void CRfhutilView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
	// handle print requests
	bool		prtDone=false;
	bool		pageDone=false;
	int			i=0;
	int			chars;
	SIZE		sz;
	RECT		rect;
	TEXTMETRIC	tm;
	int			cyChar;
	int			cxChar;
	int			cyRes;
	int			cxRes;
	int			marginX;
	int			marginY;
	int			logPixelsX;
	int			logPixelsY;
	int			pageX;
	int			pageY;
	int			horzsize;
	int			horzres;
	int			vertsize;
	int			vertres;
	int			maxPageY;
	int			mapmode;
	int			lines=0;
	int			maxlines=30;
	int			maxLineWidthLU;
	double		maxLine;
	POINT		pt;
	char		pline[1024];

	CCtrlView::OnPrint(pDC, pInfo);

	pDC->GetTextMetrics(&tm);
	pt.x = tm.tmAveCharWidth;
	pt.y = tm.tmHeight + tm.tmExternalLeading;
	pDC->LPtoDP(&pt);
	cyRes = abs(pt.y);
	cxRes = abs(pt.x);
	cyChar = tm.tmHeight + tm.tmExternalLeading;
	cxChar = tm.tmAveCharWidth;

	mapmode = pDC->GetMapMode();

	marginX = GetDeviceCaps(pDC->m_hDC, PHYSICALOFFSETX);
	marginY = GetDeviceCaps(pDC->m_hDC, PHYSICALOFFSETY);
//	pt.x = marginX;
//	pt.y = marginY;
//	pDC->LPtoDP(&pt);
//	marginX = abs(pt.x);
//	marginY = abs(pt.y);
	pageX = GetDeviceCaps(pDC->m_hDC, PHYSICALWIDTH);
	pageY = GetDeviceCaps(pDC->m_hDC, PHYSICALHEIGHT);
	horzsize = GetDeviceCaps(pDC->m_hDC, HORZSIZE);
	horzres = GetDeviceCaps(pDC->m_hDC, HORZRES);
	logPixelsX = GetDeviceCaps(pDC->m_hDC, LOGPIXELSX);
	vertsize = GetDeviceCaps(pDC->m_hDC, VERTSIZE);
	vertres = GetDeviceCaps(pDC->m_hDC, VERTRES);
	logPixelsY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);

	maxLineWidthLU = 5 * 72;
	if (logPixelsX > 0)
	{
		maxLine = horzres;
		maxLine = maxLine / logPixelsX;
		maxLine = maxLine * 72;
		maxLineWidthLU = (int)maxLine;
//		maxLineWidthLU = ((horzres / logPixelsX) * 72) + ((horzres % logPixelsX)
	}

	maxPageY = (2 * marginY) - pageY;

	pDC->GetClipBox(&rect);
	pDC->LPtoDP(&rect);
	rect.left = marginX;
	rect.top = 0;
	rect.right = pageX - marginX;
	rect.bottom = rect.top - cyChar;

	if (cyRes > 0)
	{
		maxlines = (vertres - (2 * marginY)) / cyRes;
	}

	pt = pDC->GetCurrentPosition();
	while ((!prtDone) && (!pageDone))
	{
		// capture up to 256 characters
		i = 0;
		int j=prtLoc;
		while ((i < 256) && (j < prtLen) && (prtData[j] >= ' '))
		{
			pline[i++] = prtData[j++];
		}

		pline[i] = 0;

		// get the number of characters that actually fit
		chars = 0;
//		int ret = GetTextExtentExPoint(pDC->m_hDC, pline, i, GetSizeInPicas(5*72, pDC), &chars, NULL, &sz);
		int ret = GetTextExtentExPoint(pDC->m_hDC, pline, i, GetSizeInPicas(maxLineWidthLU, pDC), &chars, NULL, &sz);

		if (chars > 0)
		{
			// limit the number of characters to the number that actually fit
			pline[chars] = 0;
			pDC->DrawText(pline, chars, &rect, DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_TOP);
//			pDC->TextOut(rect.left, rect.top, pline, chars);
			prtLoc += chars;

			if (pInfo->m_bPreview)
			{
				rect.top += cyChar;
				rect.bottom += cyChar;
			}
			else
			{
				rect.top -= cyChar;
				rect.bottom -= cyChar;
			}

			lines++;
			if (lines == maxlines)
//			if (rect.bottom <= maxPageY)
			{
				pageDone = true;
			}
		}

		while ((prtData[prtLoc] < ' ') && (prtLoc < prtLen))
		{
			prtLoc++;
		}

		if ((prtLoc >= prtLen) || (pInfo->m_bPreview))
		{
			prtDone = true;
			pInfo->m_bContinuePrinting = FALSE;
		}
	}
/*	i = 0;
	pline[0] = 0;
	while ((!prtDone) && (!pageDone))
	{
		if (prtLoc < prtLen)
		{
			if (prtData[prtLoc] < ' ')
			{
				// skip any control characters
				foundLF = false;
				while ((prtLoc < prtLen) && (prtData[prtLoc] < ' '))
				{
					if ('\n' == prtData[prtLoc])
					{
						foundLF = true;
					}

					prtLoc++;
				}

				if (foundLF)
				{
					// print the current line
					pline[i] = 0;
					pDC->TextOut(0, 0, pline, i);
					lines++;
					pline[0] = 0;
					i = 0;
				}
			}
			else
			{
				pline[i++] = prtData[prtLoc++];

				if (i >= maxChars)
				{
					// print the current line
					pline[i] = 0;
					pDC->TextOut(0, 0, pline, i);
					lines++;
					pline[0] = 0;
					i = 0;
				}
			}

			if (lines >= maxLines)
			{
				pageDone = true;
			}
		}
		else
		{
			prtDone = true;
		}
	}

	if (i > 0)
	{
		// print the current line
		pline[i] = 0;
		pDC->TextOut(0, 0, pline, i);
	}
*/
//	pDC->EndPage();
}

void CRfhutilView::OnUpdateFilePrint(CCmdUI* pCmdUI) 

{
	// determine if the print menu item should be enabled
	switch (currentSelection)
	{
	case PAGE_DATA:
		{
			if (((CRfhutilApp *)AfxGetApp())->pDocument.fileSize > 0)
				pCmdUI->Enable(TRUE);
			else
				pCmdUI->Enable(FALSE);

			pCmdUI->SetText("&Print Data\tCtrl+P");
			break;
		}
	case PAGE_USR:
		{
			if (m_usr.m_rfh_usr_data.GetLength() > 0)
				pCmdUI->Enable(TRUE);
			else
				pCmdUI->Enable(FALSE);

			pCmdUI->SetText("&Prt Usr Data\tCtrl+P");
			break;
		}
	case PAGE_OTHER:
		{
			if (m_other.m_rfh_other_data.GetLength() > 0)
				pCmdUI->Enable(TRUE);
			else
				pCmdUI->Enable(FALSE);

			pCmdUI->SetText("&Prt Other Data\tCtrl+P");
			break;
		}
	default:
		{
			pCmdUI->SetText("&Print");
			pCmdUI->Enable(FALSE);
		}
	}
}

void CRfhutilView::OnUpdateFilePageSetup(CCmdUI* pCmdUI) 

{
	// enable the menu item
	pCmdUI->Enable(TRUE);
}

void CRfhutilView::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
	// determine if the edit cut menu item should be active
	// there is nothing that supports cut on the message data page
	if (PAGE_DATA == currentSelection)
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->Enable(TRUE);
}

void CRfhutilView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	CRfhutilApp		*app=(CRfhutilApp *)AfxGetApp();
	char			traceInfo[128];

	if (PAGE_DATA == currentSelection)
	{
		// nothing on this page supports paste
		pCmdUI->Enable(FALSE);
	}
	else
	{
		pCmdUI->Enable(::IsClipboardFormatAvailable(CF_TEXT));
	}

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilView::OnUpdateEditPaste id=%d format=%d currentSelection=%d", pCmdUI->m_nID, ::IsClipboardFormatAvailable(CF_TEXT), currentSelection);
		app->logTraceEntry(traceInfo);
	}
}

void CRfhutilView::OnUpdateEditSelectAll(CCmdUI* pCmdUI) 

{
	// enable the menu item
	pCmdUI->Enable(TRUE);
}

void CRfhutilView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	// enable the menu item
	pCmdUI->Enable(TRUE);
}

int CRfhutilView::GetSizeInPicas(int nPicas, CDC* pDC)

{
	POINT	pt;
	POINT ptOrg = {0, 0};

	pt.y = ::GetDeviceCaps(pDC->m_hDC, LOGPIXELSX) * nPicas;
	pt.x = 72;			// 72 points per inch

	DPtoLP(pDC->m_hDC, &pt, 1);
	DPtoLP(pDC->m_hDC, &ptOrg, 1);
	return (abs(pt.y - ptOrg.y));
}

int CRfhutilView::getLinesPerPage(CDC* pDC)

{
	int		linesPerPage=0;

	return linesPerPage;
}

void CRfhutilView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) 

{
	// prepare the device context for print requests
	CCtrlView::OnPrepareDC(pDC, pInfo);

	if (prtLoc >= prtLen)
	{
		pInfo->m_bContinuePrinting = FALSE;
	}
	else
	{
		// set the mapping mode to lometric
		pDC->SetMapMode(MM_LOMETRIC);
		pInfo->m_bContinuePrinting = TRUE;
		pInfo->m_strPageDesc = "%d";
	}
}

void CRfhutilView::OnFilePageSetup() 

{
	// not implemented
}

void CRfhutilView::OnFilePrintPreview()
 
{
	// not implemented
}

void CRfhutilView::OnUpdateFilePrintPreview(CCmdUI* pCmdUI) 

{
	switch (currentSelection)
	{
	case PAGE_DATA:
		{
			if (((CRfhutilApp *)AfxGetApp())->pDocument.fileSize > 0)
				pCmdUI->Enable(TRUE);
			else
				pCmdUI->Enable(FALSE);

			break;
		}
	case PAGE_USR:
		{
			if (m_usr.m_rfh_usr_data.GetLength() > 0)
				pCmdUI->Enable(TRUE);
			else
				pCmdUI->Enable(FALSE);

			break;
		}
	case PAGE_OTHER:
		{
			if (m_other.m_rfh_other_data.GetLength() > 0)
				pCmdUI->Enable(TRUE);
			else
				pCmdUI->Enable(FALSE);

			break;
		}
	default:
		{
			pCmdUI->Enable(FALSE);
		}
	}
}

void CRfhutilView::OnReadNocrlf() 
{
	DataArea*	pDoc;

	// capture the new setting
	pDoc = &((CRfhutilApp *)AfxGetApp())->pDocument;
	if (pDoc->m_read_nocrlf)
	{
		pDoc->m_read_nocrlf = FALSE;
	}
	else
	{
		pDoc->m_read_nocrlf = TRUE;
	}
}

void CRfhutilView::OnReadIgnoreHeader() 
{
	DataArea*	pDoc;

	// capture the new setting
	pDoc = &((CRfhutilApp *)AfxGetApp())->pDocument;
	if (pDoc->m_read_ignore_header)
	{
		pDoc->m_read_ignore_header = FALSE;
	}
	else
	{
		pDoc->m_read_ignore_header = TRUE;
	}
}

void CRfhutilView::OnReadUnix() 
{
	DataArea*	pDoc;

	// capture the new setting
	pDoc = &((CRfhutilApp *)AfxGetApp())->pDocument;
	if (pDoc->m_read_unix)
	{
		pDoc->m_read_unix = FALSE;
	}
	else
	{
		pDoc->m_read_unix = TRUE;
	}
}

void CRfhutilView::OnUpdateReadDataonly(CCmdUI* pCmdUI) 
{
	// Enable the menu option and set the check mark if selected
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_dataonly);
}

void CRfhutilView::OnUpdateReadIgnoreHeader(CCmdUI* pCmdUI) 
{
	// Enable the menu option and set the check mark if selected
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_read_ignore_header);
}

void CRfhutilView::OnUpdateReadNocrlf(CCmdUI* pCmdUI) 
{
	// Enable the menu option and set the check mark if selected
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_read_nocrlf);
}

void CRfhutilView::OnUpdateReadUnix(CCmdUI* pCmdUI) 
{
	// Enable the menu option and set the check mark if selected
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_read_unix);
}

void CRfhutilView::OnWriteDataonly() 
{
	// handle the write data only menu item
	DataArea*	pDoc;

	// capture the current setting
	pDoc = &((CRfhutilApp *)AfxGetApp())->pDocument;
	if (pDoc->m_dataonly)
	{
		pDoc->m_dataonly = FALSE;
	}
	else
	{
		pDoc->m_dataonly = TRUE;
	}
}

void CRfhutilView::OnUpdateWriteDataonly(CCmdUI* pCmdUI) 

{
	// Enable the menu option and set the check mark if selected
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_dataonly);
}

void CRfhutilView::OnReadSaveRfh() 

{
	DataArea*	pDoc;

	// capture the new setting
	pDoc = &((CRfhutilApp *)AfxGetApp())->pDocument;
	if (pDoc->m_save_rfh)
	{
		pDoc->m_save_rfh = FALSE;
	}
	else
	{
		pDoc->m_save_rfh = TRUE;
	}
}

void CRfhutilView::OnUpdateReadSaveRfh(CCmdUI* pCmdUI) 

{
	// Enable the menu option and set the check mark if selected
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_save_rfh);
}

void CRfhutilView::updatePageData()

{
	char			traceInfo[128];
	CRfhutilApp *	app = (CRfhutilApp *)AfxGetApp();

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilView::updatePageData currentSelection=%d", currentSelection);
		app->logTraceEntry(traceInfo);
	}

	switch (currentSelection)
	{
	case PAGE_MAIN:
		{
			m_general.UpdatePageData();
			break;
		}
	case PAGE_V7PUBSUB:
		{
			m_ps.UpdatePageData();
			break;
		}
	case PAGE_DATA:
		{
			m_data.setDisplayFont();
			m_data.updateMsgdata();
			break;
		}
	case PAGE_MQMD:
		{
			m_mqmd.UpdatePageData();
			break;
		}
	case PAGE_V7PROPS:
		{
			m_props.UpdatePageData();
			break;
		}
	case PAGE_RFH:
		{
//			m_rfh.updateOldVersion(((CRfhutilApp *)AfxGetApp())->pDocument.m_rfh_version);
			m_rfh.UpdatePageData();
			break;
		}
	case PAGE_PUBSUB:
		{
			m_pubsub.updatePageData();
			break;
		}
	case PAGE_PSCR:
		{
			m_pscr.updatePageData();
			break;
		}
	case PAGE_JMS:
		{
			m_jms.updatePageData();
			break;
		}
	case PAGE_USR:
		{
			m_usr.updatePageData();
			break;
		}
	case PAGE_OTHER:
		{
			m_other.updatePageData();
			break;
		}
	case PAGE_CICS:
		{
			m_cics.UpdatePageData();
			break;
		}
	case PAGE_IMS:
		{
			m_ims.UpdatePageData();
			break;
		}
	case PAGE_DLQ:
		{
			m_dlq.UpdatePageData();
			break;
		}
	}
}

void CRfhutilView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 

{
	// notify the current dialog that the data may have changed, so it can refresh
	updatePageData();
}

void CRfhutilView::OnViewSystemQueues() 

{
	DataArea*	pDoc;

	// capture the new value
	pDoc = &((CRfhutilApp *)AfxGetApp())->pDocument;
	if (pDoc->m_show_system_queues)
	{
		pDoc->m_show_system_queues = FALSE;
	}
	else
	{
		pDoc->m_show_system_queues = TRUE;
	}
}

void CRfhutilView::OnUpdateViewSystemQueues(CCmdUI* pCmdUI) 

{
	// Enable the menu option and set the check mark if selected
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_show_system_queues);
}

void CRfhutilView::OnCopyMsgId() 

{
	// copy the message id to the correlation id
	m_mqmd.CopyMsgIdToCorrelId();
	updateIdView();
}

void CRfhutilView::OnSaveMsgId() 
{
	// save the current value of the message id
	m_mqmd.saveMsgId();
	updateIdView();
}

void CRfhutilView::OnSaveGroupId() 

{
	// save the current value of the group id
	m_mqmd.saveGroupId();
	updateIdView();
}

void CRfhutilView::OnSaveCorrelId() 
{
	// save the current value of the correlation id
	m_mqmd.saveCorrelId();
	updateIdView();
}

void CRfhutilView::OnRestoreMsgId() 
{
	// restore the message id from the last saved value
	m_mqmd.restoreMsgId();
	updateIdView();
}

void CRfhutilView::OnRestoreGroupId() 
{
	// restore the group id from the last saved value
	m_mqmd.restoreGroupId();
	updateIdView();
}

void CRfhutilView::OnRestoreCorrelId() 
{
	// restore the correlation id from the last saved value
	m_mqmd.restoreCorrelId();
	updateIdView();
}

void CRfhutilView::updateIdView()

{
	if (PAGE_MQMD == currentSelection)
	{
		m_mqmd.UpdatePageData();
	}
}

void CRfhutilView::OnSaveCicsFacility() 

{
	// save the current value of the CICS facility field
	m_cics.saveFacility();
}

void CRfhutilView::OnUpdateSaveCicsFacility(CCmdUI* pCmdUI) 

{
	// Enable the menu item
	pCmdUI->Enable(TRUE);
}

void CRfhutilView::OnRestoreCicsFacility() 

{
	// restore the facility from the last saved value
	m_cics.restoreFacility();
}

void CRfhutilView::OnUpdateRestoreCicsFacility(CCmdUI* pCmdUI) 

{
	// Enable the menu item
	pCmdUI->Enable(TRUE);
}

void CRfhutilView::OnRestoreTransId() 

{
	// restore the transaction id from the last saved value
	m_ims.restoreTransInstanceId();	
}

void CRfhutilView::OnUpdateRestoreTransId(CCmdUI* pCmdUI) 

{
	// Enable the menu item
	pCmdUI->Enable(TRUE);
}

void CRfhutilView::OnSaveImsTransid() 

{
	// save the current value of the IMS transaction id
	m_ims.saveTransInstanceId();	
}

void CRfhutilView::OnUpdateSaveImsTransid(CCmdUI* pCmdUI) 

{
	// Enable the menu item
	pCmdUI->Enable(TRUE);
}

void CRfhutilView::OnFileWrite() 

{
	// handle file writes from the menu
	CRfhutilApp *	app = (CRfhutilApp *)AfxGetApp();

	// Is trace on?
	if (app->isTraceEnabled())
	{
		// add the line to the trace file
		app->logTraceEntry("Entered CRfhutilView::OnFileWrite");
	}

	// Write the current data to a file
	app->pDocument.WriteDataFile();

	// force the current tab to refresh its data
	updatePageData();
}

void CRfhutilView::OnUpdateFileWrite(CCmdUI* pCmdUI) 

{
	// Make sure that no find or hex find dialog has been displayed
	if ((NULL == findDialog) && (NULL == hexFindDialog))
	{
		// Enable the menu item
		pCmdUI->Enable(TRUE);
	}
	else
	{
		// Disable the menu item since you can't have two pop-ups at the same time
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnOpen() 

{
	// handle file open requests from the menu
	CRfhutilApp *	app = (CRfhutilApp *)AfxGetApp();

	// Is trace on?
	if (app->isTraceEnabled())
	{
		// add the line to the trace file
		app->logTraceEntry("Entered CRfhutilView::OnOpen");
	}

	// Read the contents of a file into memory
	app->pDocument.ReadDataFile();

	// force the current tab to refresh its data
	updatePageData();
}

void CRfhutilView::OnUpdateOpen(CCmdUI* pCmdUI) 

{
	// check if a queue browse operation is in progress
	// also make sure that no find or hex find dialog has been displayed
	if ((((CRfhutilApp *)AfxGetApp())->pDocument.browseActive) || (findDialog != NULL) || (hexFindDialog != NULL))
	{
		// do not allow file read when a queue browse is active
		pCmdUI->Enable(FALSE);
	}
	else
	{
		// enable the menu option
		pCmdUI->Enable(TRUE);
	}
}

void CRfhutilView::OnUpdateReadIgnoreMqmd(CCmdUI* pCmdUI) 

{
	// Enable the menu option and set the check mark if selected
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_read_ignore_MQMD);
}

void CRfhutilView::OnReadIgnoreMqmd() 

{
	// Reverse the current setting
	((CRfhutilApp *)AfxGetApp())->pDocument.m_read_ignore_MQMD = !((CRfhutilApp *)AfxGetApp())->pDocument.m_read_ignore_MQMD;
}

void CRfhutilView::OnWriteIncludeMqmd() 

{
	// Reverse the current setting
	((CRfhutilApp *)AfxGetApp())->pDocument.m_write_include_MQMD = !((CRfhutilApp *)AfxGetApp())->pDocument.m_write_include_MQMD;
}

void CRfhutilView::OnUpdateWriteIncludeMqmd(CCmdUI* pCmdUI) 

{
	// Enable the menu option and set the check mark if selected
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_write_include_MQMD);
}

void CRfhutilView::OnSearchFind() 

{
	// initiate a find operation
	// the find operation uses a modeless dialog that remains active until it is cancelled
	// a special user message is used to notify the drive the actual searches
	if (NULL == findDialog)
	{
		findDialog = new CFindReplaceDialog();
		findDialog->Create(TRUE, findValue, "", FR_HIDEWHOLEWORD | FR_HIDEMATCHCASE | FR_DOWN, this );

		findDialog->m_fr.lStructSize = sizeof(FINDREPLACE);
		findDialog->m_fr.hwndOwner = this->m_hWnd;

		// disable the most recent file menu for now
		// it will be restored when the dialog is closed
		((CRfhutilApp *)AfxGetApp())->saveRecentFileList();
	}
}

void CRfhutilView::OnUpdateSearchFind(CCmdUI* pCmdUI) 
{
	int		dType;

	// Check if the find option should be enabled
	if ((PAGE_DATA == currentSelection) && 
		(NULL == hexFindDialog) && 
		(NULL == findDialog) && 
		(((CRfhutilApp *)AfxGetApp())->pDocument.fileSize > 0))
	{
		// check the kind of display
		dType = m_data.m_data_format;
		if ((DATA_CHARACTER == dType) ||
			(DATA_HEX == dType) ||
			(DATA_BOTH == dType) ||
			(DATA_XML == dType) ||
			(DATA_PARSED == dType) ||
			(DATA_JSON == dType) ||
			(DATA_FIX == dType))
		{
			pCmdUI->Enable(TRUE);
		}
		else
		{
			pCmdUI->Enable(FALSE);
		}
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnSearchFindHex() 
{
	// initiate a hexadecimal find operation
	// the find operation uses a modeless dialog that remains active until it is cancelled
	// a special user message is used to notify the drive the actual searches
	if (NULL == hexFindDialog)
	{
		hexFindDialog = new CHexFind( this );
		hexFindDialog->Create(IDD_HEXFIND,  this );

		// disable the most recent file menu for now
		// it will be restored when the dialog is closed
		((CRfhutilApp *)AfxGetApp())->saveRecentFileList();
	}
}

void CRfhutilView::OnUpdateSearchFindHex(CCmdUI* pCmdUI) 
{
	int		dType;

	// Check if the hex find option should be enabled
	if ((PAGE_DATA == currentSelection) && 
		(NULL == hexFindDialog) && 
		(NULL == findDialog) && 
		(((CRfhutilApp *)AfxGetApp())->pDocument.fileSize > 0))
	{
		// check the kind of display
		dType = m_data.m_data_format;
		if ((DATA_CHARACTER == dType) ||
			(DATA_HEX == dType) ||
			(DATA_BOTH == dType))
		{
			pCmdUI->Enable(TRUE);
		}
		else
		{
			pCmdUI->Enable(FALSE);
		}
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnSearchGoto() 
{
	int			ret;
	int			offset;
	int			dType;
	int			line;
	int			ofs;
	int			crlf;
	int			edi;
	int			BOM;
	int			charCount;
	CGoto		gt(this, ((CRfhutilApp *)AfxGetApp())->pDocument.fileSize-1);
	char		traceInfo[256];

	CRfhutilApp *	app = (CRfhutilApp *)AfxGetApp();

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilView::OnSearchGoto dType=%d", m_data.m_data_format);
		app->logTraceEntry(traceInfo);
	}

	ret = gt.DoModal();

	if (IDOK == ret)
	{
		offset = gt.getOffset() + 1;
		dType = m_data.m_data_format;
		crlf = m_data.m_crlf;
		edi = m_data.m_edi;
		BOM = m_data.m_BOM,
		line = app->pDocument.getLineNumber(offset, dType, crlf, edi, BOM);
		ofs = app->pDocument.getLineOffset(offset, dType, crlf, edi, BOM);

		if (DATA_HEX == dType)
		{
			charCount = 2;
		}
		else
		{
			charCount = 1;
		}

		// now see if we can change the data edit box
		m_data.moveDisplay(line, ofs, charCount);

		// check if trace is enabled
		if (app->isTraceEnabled())
		{
			// build the trace line and add the line to the trace file
			sprintf(traceInfo, "Exiting CRfhutilView::OnSearchGoto offset=%d line=%d ofs=%d charCount=%d", offset, line, ofs, charCount);
			app->logTraceEntry(traceInfo);
		}
	}
}

void CRfhutilView::OnUpdateSearchGoto(CCmdUI* pCmdUI) 
{
	int		dType;

	// make sure the tab is appropriate for a goto offset function
	// and the find or hex find is not active
	if ((PAGE_DATA == currentSelection) && 
		(NULL == hexFindDialog) && 
		(NULL == findDialog) && 
		(((CRfhutilApp *)AfxGetApp())->pDocument.fileSize > 0))
	{
		// check the kind of display
		dType = m_data.m_data_format;
		if ((DATA_CHARACTER == dType) ||
			(DATA_HEX == dType) ||
			(DATA_BOTH == dType))
		{
			// appropriate for goto function
			pCmdUI->Enable(TRUE);
		}
		else
		{
			// not appropriate display type - don't allow a goto
			pCmdUI->Enable(FALSE);
		}
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::findHelper(UINT wParam, LONG lParam)

{
	int		offset = 0;
	int		line = 0;
	int		ofs = 0;
	int		dType = 0;
	int		crlf = 0;
	int		edi = 0;
	int		BOM = 0;
	int		charCount = 0;
	char	traceInfo[256];

	CRfhutilApp *	app = (CRfhutilApp *)AfxGetApp();

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilView::findHelper dType=%d lastFindDType=%d lastFindValue=%s", m_data.m_data_format, lastFindDType, (LPCTSTR)lastFindValue);
		app->logTraceEntry(traceInfo);
	}

	if (findDialog->IsTerminating())
	{
		findDialog = NULL;

		// restore the recent file list
		((CRfhutilApp *)AfxGetApp())->restoreRecentFileList();

		// check if trace is enabled
		if (app->isTraceEnabled())
		{
			app->logTraceEntry("Find dialog terminating");
		}
	}
	else
	{
		if (findDialog->FindNext())
		{
			// get the current display format
			dType = m_data.m_data_format;

			// check if the user changed the string
			if ((lastFindValue.Compare(findDialog->GetFindString()) != 0) || (lastFindDType != dType))
			{
				// reset the current find next locations
				app->pDocument.resetXMLfind();
				app->pDocument.resetParsedFind();
				app->pDocument.resetFind();
				lastFindValue = findDialog->GetFindString();
				lastFindDType = dType;
			}

			if ((DATA_CHARACTER == dType) ||
				(DATA_HEX == dType) ||
				(DATA_BOTH == dType))
			{
				// get the current display options that affect a search
				crlf = m_data.m_crlf;
				edi = m_data.m_edi;
				BOM = m_data.m_BOM;

				offset = app->pDocument.findNext((unsigned char *)(LPCTSTR)findDialog->GetFindString(), 
												 findDialog->GetFindString().GetLength(),
												 findDialog->SearchDown());

				if (-1 == offset)
				{
					// set value for trace line
					charCount = 0;
					line = 0;
					ofs = 0;

					// not found, move to beginning of document
					m_data.moveDisplay(0, 0, 0);
				}
				else
				{
					line = app->pDocument.getLineNumber(offset, dType, crlf, edi, BOM);
					ofs = app->pDocument.getLineOffset(offset, dType, crlf, edi, BOM);

					if (DATA_HEX == dType)
					{
						charCount = 2;
					}
					else
					{
						charCount = 1;
					}

					// now see if we can change the data edit box
					m_data.moveDisplay(line, ofs, charCount);
				}
			}
			else
			{
				// set value for trace line
				charCount = 1;

				// handle XML and parsed differently
				if (DATA_XML == dType)
				{
					// search the formatted data
					line = app->pDocument.getXMLLineNumber((LPCTSTR)findDialog->GetFindString(), findDialog->SearchDown());
					ofs = app->pDocument.getXMLofs();
					m_data.moveDisplay(line, ofs, 1);
				}
				else if (DATA_PARSED == dType)
				{
					// search the formatted data
					line = app->pDocument.getParsedLineNumber((LPCTSTR)findDialog->GetFindString(), findDialog->SearchDown());
					ofs = app->pDocument.getParsedOfs();
					m_data.moveDisplay(line, ofs, 1);
				}
				else if (DATA_JSON == dType)
				{
					// search the formatted data
					line = app->pDocument.getJsonLineNumber((LPCTSTR)findDialog->GetFindString(), findDialog->SearchDown());
					ofs = app->pDocument.getJsonOfs();
					m_data.moveDisplay(line, ofs, 1);
				}
				else if (DATA_FIX == dType)
				{
					// search the formatted data
					line = app->pDocument.getFixLineNumber((LPCTSTR)findDialog->GetFindString(), findDialog->SearchDown());
					ofs = app->pDocument.getFixOfs();
					m_data.moveDisplay(line, ofs, 1);
				}
			}
		}

		// check if trace is enabled
		if (app->isTraceEnabled())
		{
			// build the trace line and add the line to the trace file
			sprintf(traceInfo, "Exiting CRfhutilView::findHelper offset=%d line=%d ofs=%d charCount=%d lastFindValue=%s", offset, line, ofs, charCount, (LPCTSTR)lastFindValue);
			app->logTraceEntry(traceInfo);
		}
	}
}

LRESULT CRfhutilView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 

{
	// handle the special user messages generated by the find dialogs
	if (findMessage == message)
	{
		findHelper(wParam, lParam);
	}
	
	if (hexFindMessage == message)
	{
		hexFindHelper(wParam, lParam);
	}

	return CCtrlView::WindowProc(message, wParam, lParam);
}

void CRfhutilView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType) 
{
	CCtrlView::CalcWindowRect(lpClientRect, nAdjustType);
}

void CRfhutilView::hexFindHelper(UINT wParam, LONG lParam)

{
	int				dType;
	int				crlf;
	int				edi;
	int				BOM;
	int				len=0;
	int				offset=0;
	int				line=0;
	int				ofs=0;
	int				charCount=0;
	unsigned char	hexData[32];
	char			traceInfo[256];

	CRfhutilApp *	app = (CRfhutilApp *)AfxGetApp();

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilView::hexFindHelper dType=%d", m_data.m_data_format);
		app->logTraceEntry(traceInfo);
	}

	if (hexFindDialog->isTerminating())
	{
		hexFindDialog = NULL;

		// restore the recent file list
		((CRfhutilApp *)AfxGetApp())->restoreRecentFileList();

		// check if trace is enabled
		if (app->isTraceEnabled())
		{
			app->logTraceEntry("Hex find dialog terminating");
		}
	}
	else
	{
		CRfhutilApp *	app = (CRfhutilApp *)AfxGetApp();

		if (hexFindDialog->FindNext())
		{
			// check the kind of display
			// ignore for XML and COBOL formatting
			dType = m_data.m_data_format;
			crlf = m_data.m_crlf;
			edi = m_data.m_edi;
			BOM = m_data.m_BOM;
			if ((DATA_CHARACTER == dType) ||
				(DATA_HEX == dType) ||
				(DATA_BOTH == dType))
			{
				// get the data
				memset(hexData, 0, sizeof(hexData));
				len = hexFindDialog->getHexValue(hexData, sizeof(hexData));
				offset = app->pDocument.findNext(hexData, len, hexFindDialog->SearchDown());
				if (offset != -1)
				{
					line = app->pDocument.getLineNumber(offset, dType, crlf, edi, BOM);
					ofs = app->pDocument.getLineOffset(offset, dType, crlf, edi, BOM);

					if (DATA_HEX == dType)
					{
						charCount = 2;
					}
					else
					{
						charCount = 1;
					}

					// now see if we can change the data edit box
					m_data.moveDisplay(line, ofs, charCount);
				}
				else
				{
					// move to the first line of data
					m_data.moveDisplay(0, 0, 0);
				}

				// check if trace is enabled
				if (app->isTraceEnabled())
				{
					// build the trace line and add the line to the trace file
					sprintf(traceInfo, "Exiting CRfhutilView::hexFindHelper offset=%d line=%d ofs=%d charCount=%d len=%d hexData=%s", offset, line, ofs, charCount, len, hexData);
					app->logTraceEntry(traceInfo);
				}
			}
		}
	}
}

void CRfhutilView::OnViewCluster() 

{
	// handle the view cluster queues menu item
	// this does not function since PCF messages do not return cluster queue entries
	DataArea*	pDoc;
	CRfhutilApp * app;
	int			rc;
	char		errtxt[256];

	// initialize the error message area
	memset(errtxt, 0, sizeof(errtxt));

	// get a pointer to the data object
	app = (CRfhutilApp *)AfxGetApp();
	pDoc = &(app)->pDocument;

	// reverse the current setting
	if (pDoc->m_show_cluster_queues)
	{
		pDoc->m_show_cluster_queues = FALSE;
	}
	else
	{
		pDoc->m_show_cluster_queues = TRUE;
	}

	// reload the queue names
	app->BeginWaitCursor();
	rc = pDoc->getQueueNames((LPCTSTR)(pDoc->m_QM_name));
	app->EndWaitCursor();

	// check if we had an error
	if (rc != MQRC_NONE)
	{
		// did we get an error message back?
		if (errtxt[0] != 0)
		{
			pDoc->m_error_msg = errtxt;
			pDoc->m_error_msg += "\r\n";
		}

		// report the error
		sprintf(errtxt, "Error getting queue names - reason %d", rc);
		pDoc->m_error_msg += errtxt;
		pDoc->updateMsgText();
		updatePageData();
	}

	// finally check if the main page is the current 
}

void CRfhutilView::OnUpdateViewCluster(CCmdUI* pCmdUI) 

{
	// enable the menu item and set the check mark to reflect the current setting
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(((CRfhutilApp *)AfxGetApp())->pDocument.m_show_cluster_queues);
}

void CRfhutilView::OnBrnext() 

{
	// handle a browse next request
	DataArea*	pDoc;

	if ((PAGE_MAIN == currentSelection) || (PAGE_DATA == currentSelection) || (PAGE_USR == currentSelection))
	{
		CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

		// find the dataarea object
		pDoc = &(app)->pDocument;

		// check if a browse operation has been started
		if (pDoc->browseActive)
		{
			pDoc->browseNext(false);

			if (PAGE_DATA == currentSelection)
			{
				// now see if we can change the data edit box
				m_data.setDisplayFont();
				m_data.updateMsgdata();
			}

			if (PAGE_USR == currentSelection)
			{
				// update the data edit box
				m_usr.updatePageData();
			}

			// update the error text box on the main dialog
			m_general.UpdatePageData();
		}
	}
}

void CRfhutilView::OnBrprev() 

{
	// handle a browse previous request
	DataArea*	pDoc;

	if ((PAGE_MAIN == currentSelection) || (PAGE_DATA == currentSelection) || (PAGE_USR == currentSelection))
	{
		CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

		// get a pointer to the dataarea object
		pDoc = &(app)->pDocument;

		// check if a browse previous is allowed
		if (pDoc->browsePrevActive)
		{
			pDoc->browsePrev();

			if (PAGE_DATA == currentSelection)
			{
				// now see if we can change the data edit box
				m_data.setDisplayFont();
				m_data.updateMsgdata();
			}

			if (PAGE_USR == currentSelection)
			{
				// update the data edit box
				m_usr.updatePageData();
			}

			// update the error text box on the main dialog
			m_general.UpdatePageData();
		}
	}
}

void CRfhutilView::OnUpdateMqMqconn(CCmdUI* pCmdUI) 

{
	// enable the menu item if appropriate
	DataArea*	pDoc;

	// get a pointer to the dataarea object
	pDoc = &((CRfhutilApp *)AfxGetApp())->pDocument;

	// check if a queue is open
	if ((PAGE_MAIN == currentSelection) && !pDoc->isConnectionActive() && !pDoc->browseActive)
	{
		pCmdUI->Enable(TRUE);
		getQnames();
	} else {
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnUpdateMqMqdisc(CCmdUI* pCmdUI) 

{
	// enable the menu item if appropriate
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;

	// check for a queue manager connection and that a queue is not open
	if (pDoc->isConnectionActive() && !pDoc->browseActive)
	{
		pCmdUI->Enable(TRUE);
	} else {
		// must connect first
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnUpdateMqMqopenGet(CCmdUI* pCmdUI) 

{
	// enable the menu item if appropriate
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;

	// check for a queue manager connection
	if ((PAGE_MAIN == currentSelection) && pDoc->isConnectionActive() && !pDoc->browseActive)
	{
		// check if a queue is open
		if (pDoc->isQueueOpen())
		{
			pCmdUI->Enable(FALSE);
		} else {
			getQnames();
			pCmdUI->Enable(TRUE);
		}
	} else {
		// must connect first
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnUpdateMqMqopenPut(CCmdUI* pCmdUI) 

{
	// enable the menu item if appropriate
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;

	// check for a queue manager connection
	if ((PAGE_MAIN == currentSelection) && pDoc->isConnectionActive() && !pDoc->browseActive)
	{
		// check if a queue is open
		if (pDoc->isQueueOpen())
		{
			pCmdUI->Enable(FALSE);
		} else {
			getQnames();
			pCmdUI->Enable(TRUE);
		}
	} else {
		// must connect first
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnUpdateMqMqopenBrowse(CCmdUI* pCmdUI) 

{
	// enable the menu item if appropriate
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;

	// check for a queue manager connection
	if ((PAGE_MAIN == currentSelection) && pDoc->isConnectionActive() && !pDoc->browseActive)
	{
		// check if a queue is open
		if (pDoc->isQueueOpen())
		{
			pCmdUI->Enable(FALSE);
		} else {
			getQnames();
			pCmdUI->Enable(TRUE);
		}
	} else {
		// must connect first
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnUpdateMqMqclose(CCmdUI* pCmdUI) 

{
	// enable the menu item if appropriate
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;

	// check if a queue is open
	if (pDoc->isQueueOpen() && !pDoc->browseActive)
	{
		pCmdUI->Enable(TRUE);
	} else {
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnUpdateMqEndbr(CCmdUI* pCmdUI) 

{
	// enable the menu item if appropriate
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;

	// check if a browse is active
	if (((PAGE_MAIN == currentSelection) || (PAGE_DATA == currentSelection)) &&pDoc->browseActive)
	{
		pCmdUI->Enable(TRUE);
	} else {
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::getQnames()

{
	m_general.getQMname(qmName);
	m_general.getQname(qName);
	m_general.getRemoteQM(remoteQM);
}

void CRfhutilView::OnMqMqconn() 

{
	// process an MQCONN request to the specified queue manager
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;
	pDoc->explicitConnect((LPCTSTR)qmName);

	// update the button status and display
	if (PAGE_MAIN == currentSelection)
	{
		m_general.UpdatePageData();
	}
}

void CRfhutilView::OnMqMqdisc()
 
{
	// process an MQDISC request to the specified queue manager
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;

	// check if a queue is open
	if (pDoc->isQueueOpen())
	{
		// first close the queue
		pDoc->explicitCloseQ();
	}

	// disconnect from the queue manager
	pDoc->explicitDiscQM();

	// update the button status and display
	if (PAGE_MAIN == currentSelection)
	{
		m_general.UpdatePageData();
	}
}

void CRfhutilView::OnMqMqopenGet() 

{
	// process an MQGET request to the specified queue
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;
	pDoc->explicitOpen((LPCTSTR)qName, (LPCTSTR)remoteQM, Q_OPEN_READ);

	// update the button status and display
	if (PAGE_MAIN == currentSelection)
	{
		m_general.UpdatePageData();
	}
}

void CRfhutilView::OnMqMqopenPut() 

{
	// process an MQPUT request to the specified queue
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;
	pDoc->explicitOpen((LPCTSTR)qName, (LPCTSTR)remoteQM, Q_OPEN_WRITE);

	// update the button status and display
	if (PAGE_MAIN == currentSelection)
	{
		m_general.UpdatePageData();
	}
}

void CRfhutilView::OnMqMqopenBrowse() 

{
	// process an MQGET (browse) request to the specified queue
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;
	pDoc->explicitOpen((LPCTSTR)qName, (LPCTSTR)remoteQM, Q_OPEN_BROWSE);

	// update the button status and display
	if (PAGE_MAIN == currentSelection)
	{
		m_general.UpdatePageData();
	}
}

void CRfhutilView::OnMqMqclose() 

{
	// process an MQCLOSE request to the specified queue
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;
	pDoc->explicitCloseQ();

	// update the button status and display
	if (PAGE_MAIN == currentSelection)
	{
		m_general.UpdatePageData();
	}
}

void CRfhutilView::OnMqEndbr() 

{
	// process an end browse request to the specified queue
	DataArea*	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// get a pointer to the dataarea object
	pDoc = &(app)->pDocument;
	pDoc->endBrowse(true);

	// update the button status and display
	if (PAGE_MAIN == currentSelection)
	{
		m_general.UpdatePageData();
	}
	else if (PAGE_DATA == currentSelection)
	{
		// update the button status
		m_data.setDisplayFont();
		m_data.updateMsgdata();
	}
}

void CRfhutilView::OnBrowseq() 

{
	// process an MQGET (browse) request to the specified queue 
	// ignore the key stroke unless the main page is selected
	if (PAGE_MAIN == currentSelection)
	{
		m_general.BrowseQ();
	}
	else if (PAGE_DATA == currentSelection)
	{
		m_data.setBoth();
	}
}

void CRfhutilView::OnDisplayq() 

{
	// display the contents of a queue
	// ignore the key stroke unless the main page is selected
	if (PAGE_MAIN == currentSelection)
	{
		m_general.DisplayQ();
	}
	else if (PAGE_DATA == currentSelection)
	{
		m_data.togglePDEncoding();
	}	
}

void CRfhutilView::OnPurgeq() 

{
	// purge (delete all) the contents of a queue
	// ignore the key stroke unless the main page is selected
	if (PAGE_MAIN == currentSelection)
	{
		m_general.PurgeQ();
	}
	else if (PAGE_DATA == currentSelection)
	{
		m_data.setEBCDIC();
	}
}

void CRfhutilView::OnLoadnames() 

{
	// load a queue with messages saved in one or more files
	// ignore the key stroke unless the main page is selected
	if (PAGE_MAIN == currentSelection)
	{
		m_general.LoadNames();
	}
	else if (PAGE_DATA == currentSelection)
	{
		m_data.setHex();
	}	
}

void CRfhutilView::OnSetConnUser() 

{
	CRfhutilApp		*app=(CRfhutilApp *)AfxGetApp();
	char			traceInfo[128];

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilView::OnSetConnUser currentSelection=%d", currentSelection);
		app->logTraceEntry(traceInfo);
	}

	// allow an alternate user id to be set
	// ignore the key stroke unless the main page is selected
	if (PAGE_MAIN == currentSelection)
	{
		m_general.SetUserId();
	}	
	else if (PAGE_DATA == currentSelection)
	{
		m_data.toggleIntEncoding();
	}	
}

void CRfhutilView::OnClearall() 

{
	// clear the current message data and headers
	if (PAGE_MAIN == currentSelection)
	{
		m_general.ClearAll();
	}
	else if (PAGE_PUBSUB == currentSelection)
	{
		m_pubsub.PubClear();
	}
}

void CRfhutilView::OnCloseq() 

{
	// close the queue and disconnect from the queue manager
	if (PAGE_MAIN == currentSelection)
	{
		m_general.CloseQ();
	}	
	else if (PAGE_DATA == currentSelection)
	{
		m_data.setCobol();
	}
}

void CRfhutilView::OnLoadq() 

{
	// load a queue with messages saved in one or more files
	if (PAGE_MAIN == currentSelection)
	{
		m_general.LoadQ();
	}	
	else if (PAGE_DATA == currentSelection)
	{
		m_data.setXML();
	}
}

void CRfhutilView::OnReadq() 

{
	// read a message from a queue
	if (PAGE_MAIN == currentSelection)
	{
		m_general.ReadQ();
	}	
	else if (PAGE_DATA == currentSelection)
	{
		m_data.setParsed();
	}
}

void CRfhutilView::OnSaveq() 

{
	// save the contents of a queue to one or more files
	if (PAGE_MAIN == currentSelection)
	{
		m_general.SaveQ();
	} else if (PAGE_DATA == currentSelection)
	{
		m_data.ReadCopybook();
	}
}

void CRfhutilView::OnWriteq() 

{
	// write a message to the specified queue
	if (PAGE_MAIN == currentSelection)
	{
		m_general.WriteQ();
	}
	else if (PAGE_PUBSUB == currentSelection)
	{
		m_pubsub.PubProcess();
	}
	else if (PAGE_DLQ == currentSelection)
	{
		m_dlq.prepareResend();
	}
}

void CRfhutilView::OnStartbr() 

{
	// start a browse operaton on the specified queue, reading the first message
	if (PAGE_MAIN == currentSelection)
	{
		m_general.StartBr();
	}	
	else if (PAGE_DATA == currentSelection)
	{
		m_data.setCharacter();
	}	
}

void CRfhutilView::OnEndbrowse() 

{
	// end a previously initiated browse operation
	if (PAGE_MAIN == currentSelection)
	{
		m_general.EndBr();
	} 
	else if (PAGE_DATA == currentSelection)
	{
		m_data.EndBr();
	}
	else if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.SaveMsgs();
	}
}

void CRfhutilView::OnCopybook() 
{
	// read a new COBOL copybook file to be used for message displays in COBOL format
	if (PAGE_MAIN == currentSelection)
	{
		m_general.ClearAll();
	} 
	else if (PAGE_DATA == currentSelection)
	{
		m_data.ReadCopybook();
	}
}

void CRfhutilView::OnCopymsgid2correlid() 
{
	// copy the message id to the correlation id
	if (PAGE_MQMD == currentSelection)
	{
		m_mqmd.CopyMsgIdToCorrelId();
	}
	else if (PAGE_MQMD == currentSelection)
	{
		m_mqmd.CopyMsgIdToCorrelId();
	}
}

void CRfhutilView::OnResetids()
 
{
	// reset the current message ids to binary zeros
	if (PAGE_MAIN == currentSelection)
	{
		m_general.ClearData();
	}
	else if (PAGE_DATA == currentSelection)
	{
		m_data.setJapanese();
	}
	else if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.CloseQ();
	}
}

void CRfhutilView::OnNextTab() 

{
	// move to the next tab
	TCITEM		ti;
	CWnd*		cDlg=NULL;
	int			result;
	int			sel;

	// get the currently selected tab
	sel = GetTabCtrl().GetCurSel();

	// get the currently selected dialog
	ti.mask = TCIF_PARAM;
	result = GetTabCtrl().GetItem (sel, &ti);

	if (result)
	{
		cDlg = (CWnd *) ti.lParam;
		if (cDlg)
			cDlg->ShowWindow (SW_HIDE);

		KillActive(sel);
	}

	if (PAGE_DLQ == sel)
	{
		sel = 0;
	}
	else
	{
		sel++;
	}

	// change the selection
	GetTabCtrl().SetCurSel(sel);

	// remember the selection
	this->currentSelection = sel;

	// get the new dialog
	ti.mask = TCIF_PARAM;
	result = GetTabCtrl().GetItem (sel, &ti);

	if (result)
	{
		cDlg = (CWnd *) ti.lParam;
		if (cDlg)
			cDlg->ShowWindow (SW_SHOW);

		SetActive(sel);
	}
}

void CRfhutilView::OnPrevTab() 

{
	// move to the previous tab
	TCITEM		ti;
	CWnd*		cDlg=NULL;
	int			result;

	int			sel;

	// get the currently selected tab
	sel = GetTabCtrl().GetCurSel();

	// get the currently selected dialog
	ti.mask = TCIF_PARAM;
	result = GetTabCtrl().GetItem (sel, &ti);

	if (result)
	{
		cDlg = (CWnd *) ti.lParam;
		if (cDlg)
			cDlg->ShowWindow (SW_HIDE);

		KillActive(sel);
	}

	if (0 == sel)
	{
		sel = PAGE_DLQ;
	}
	else
	{
		sel--;
	}

	// change the selection
	GetTabCtrl().SetCurSel(sel);

	// remember the selection
	this->currentSelection = sel;

	// get the new dialog
	ti.mask = TCIF_PARAM;
	result = GetTabCtrl().GetItem (sel, &ti);

	if (result)
	{
		// display the new dialog
		cDlg = (CWnd *) ti.lParam;
		if (cDlg)
			cDlg->ShowWindow (SW_SHOW);

		SetActive(sel);
	}
}

void CRfhutilView::KillActive(int sel)

{
	CEdit			*cedit;
	CRfhutilApp		*app=(CRfhutilApp *)AfxGetApp();
	char			traceInfo[128];

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilView::KillActive sel=%d", sel);
		app->logTraceEntry(traceInfo);
	}

	switch (sel)
	{
	case PAGE_MAIN:
		{
			m_general.OnKillActive();
			break;
		}
	case PAGE_V7PUBSUB:
		{
			m_ps.OnKillActive();
			break;
		}
	case PAGE_DATA:
		{
			// remember the current visible line
			cedit = (CEdit *)m_data.GetDlgItem(IDC_MSG_DATA);
			((CRfhutilApp *)AfxGetApp())->pDocument.curDataLineNumber = cedit->GetFirstVisibleLine();

			m_data.OnKillActive();
			break;
		}
	case PAGE_MQMD:
		{
			m_mqmd.OnKillActive();
			break;
		}
	case PAGE_V7PROPS:
		{
			m_props.OnKillActive();
			break;
		}
	case PAGE_RFH:
		{
			m_rfh.OnKillActive();
			break;
		}
	case PAGE_PUBSUB:
		{
			m_pubsub.OnKillActive();
			break;
		}
	case PAGE_PSCR:
		{
			m_pscr.OnKillActive();
			break;
		}
	case PAGE_JMS:
		{
			m_jms.OnKillActive();
			break;
		}
	case PAGE_USR:
		{
			m_usr.OnKillActive();
			break;
		}
	case PAGE_OTHER:
		{
			m_other.OnKillActive();
			break;
		}
	case PAGE_CICS:
		{
			m_cics.OnKillActive();
			break;
		}
	case PAGE_IMS:
		{
			m_ims.OnKillActive();
			break;
		}
	case PAGE_DLQ:
		{
			m_dlq.OnKillActive();
			break;
		}
	}
}

void CRfhutilView::SetActive(int sel)

{
	CEdit		*cedit;
	int			line;
	CRfhutilApp		*app=(CRfhutilApp *)AfxGetApp();
	char			traceInfo[128];

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilView::SetActive sel=%d", sel);
		app->logTraceEntry(traceInfo);
	}

	switch (sel)
	{
	case PAGE_MAIN:
		{
			m_general.OnSetActive();
			break;
		}
	case PAGE_V7PUBSUB:
		{
			m_ps.OnSetActive();
			break;
		}
	case PAGE_DATA:
		{
			m_data.OnSetActive();

			// restore the current visible line
			line = ((CRfhutilApp *)AfxGetApp())->pDocument.curDataLineNumber;
			cedit = (CEdit *)m_data.GetDlgItem(IDC_MSG_DATA);
			cedit->SetSel(0, 0);
			cedit->LineScroll(line);
			cedit->SetFocus();

			break;
		}
	case PAGE_MQMD:
		{
			m_mqmd.OnSetActive();
			break;
		}
	case PAGE_V7PROPS:
		{
			m_props.OnSetActive();
			break;
		}
	case PAGE_RFH:
		{
			m_rfh.OnSetActive();
			break;
		}
	case PAGE_PUBSUB:
		{
			m_pubsub.OnSetActive();
			break;
		}
	case PAGE_PSCR:
		{
			m_pscr.OnSetActive();
			break;
		}
	case PAGE_JMS:
		{
			m_jms.OnSetActive();
			break;
		}
	case PAGE_USR:
		{
			m_usr.OnSetActive();
			break;
		}
	case PAGE_OTHER:
		{
			m_other.OnSetActive();
			break;
		}
	case PAGE_CICS:
		{
			m_cics.OnSetActive();
			break;
		}
	case PAGE_IMS:
		{
			m_ims.OnSetActive();
			break;
		}
	case PAGE_DLQ:
		{
			m_dlq.OnSetActive();
			break;
		}
	}
}

////////////////////////////////////////////////////////
//
// This is a courtesy function to allow access to 
// a function in the protected member m_mqmd without
// having to make the variable public
//
////////////////////////////////////////////////////////

void CRfhutilView::editIDField(int id)

{
	// pass on the request to the appropriate class
	m_mqmd.editIDField(id);
}

void CRfhutilView::OnClearData()
 
{
	DataArea *	pDoc;
	CRfhutilApp * app = (CRfhutilApp *)AfxGetApp();

	// clear the data and update the data length
	if (PAGE_MAIN == currentSelection)
	{
		m_general.ClearData();
	}
	else if (PAGE_DATA == currentSelection)
	{
		// get a pointer to the DataArea object
		pDoc = &(app->pDocument);

		// Reset the message data contents
		pDoc->clearFileData();

		// update the display contents
		updatePageData();
	}
	else if (PAGE_PUBSUB == currentSelection)
	{
		// clear the data on the pub/sub tab
		m_pubsub.clearPubSubData();
	}
	else
	{
		// get a pointer to the DataArea object
		pDoc = &(app->pDocument);

		// just clear the message data contents
		pDoc->clearFileData();
	}
}

void CRfhutilView::OnUpdateEditTop(CCmdUI* pCmdUI) 

{
	// Check what the current page is
	if (PAGE_DATA == currentSelection)
	{
		// enable the menu item
		pCmdUI->Enable(TRUE);
	}
	else
	{
		// disable the menu item
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnUpdateRecentFileMenu(CCmdUI* pCmdUI) 

{
	if (((CRfhutilApp *)AfxGetApp())->isRecentFileMenuEnabled())
	{
		// enable the menu item
		pCmdUI->Enable(TRUE);

		pCmdUI->ContinueRouting();
	}
	else
	{
		// disable the menu item
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnUpdateEditBottom(CCmdUI* pCmdUI) 

{
	// Check what the current page is
	if (PAGE_DATA == currentSelection)
	{
		// enable the menu item
		pCmdUI->Enable(TRUE);
	}
	else
	{
		// disable the menu item
		pCmdUI->Enable(FALSE);
	}
}

void CRfhutilView::OnEditBottom() 

{
	int		lastLine;
	int		curLine;
	CEdit	*cedit=NULL;

	// Check what the current page is
	if (PAGE_DATA == currentSelection)
	{
		cedit = (CEdit *)m_data.GetDlgItem(IDC_MSG_DATA);
		if (cedit != NULL)
		{
			// figure out how many lines of text there are and what the current line is
			lastLine = cedit->GetLineCount();
			curLine = cedit->GetFirstVisibleLine();

			// make the last line visible
			cedit->LineScroll(lastLine - curLine);
		}
	}
}

void CRfhutilView::OnEditTop() 

{
	int		curLine;
	CEdit	*cedit=NULL;

	// Check what the current page is
	if (PAGE_DATA == currentSelection)
	{
		cedit = (CEdit *)m_data.GetDlgItem(IDC_MSG_DATA);
		if (cedit != NULL)
		{
			curLine = cedit->GetFirstVisibleLine();

			// make the first line visible
			cedit->LineScroll(-curLine);
		}
	}
}

void CRfhutilView::OnTradChinese() 

{
	if (PAGE_DATA == currentSelection)
	{
		m_data.setTradChinese();
	}
	else if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.GetTopics();
	}
}

void CRfhutilView::OnSimpChinese() 

{
	if (PAGE_DATA == currentSelection)
	{
		m_data.setSimpChinese();
	}
	else if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.Subscribe();
	}
}

void CRfhutilView::OnAscii() 

{
	if (PAGE_DATA == currentSelection)
	{
		m_data.setAscii();
	}
	else if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.AlterSub();
	}
}

void CRfhutilView::OnEbcdic() 

{
	if (PAGE_DATA == currentSelection)
	{
		m_data.setEBCDIC();
	}
	else if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.ReqPub();
	}
}

void CRfhutilView::OnKorean() 

{
	if (PAGE_DATA == currentSelection)
	{
		m_data.setKorean();
	}
}

void CRfhutilView::UpdateAllFields()

{
	KillActive(currentSelection);
}

void CRfhutilView::OnPsGet()

{
	if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.GetMsg();
	}
}

void CRfhutilView::OnPsGetSubs()

{
	if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.GetSubs();
	}
}

void CRfhutilView::OnPsPublish()

{
	if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.Publish();
	}
}

void CRfhutilView::OnPsResume()

{
	if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.Resume();
	}
}

void CRfhutilView::OnPsClear()

{
	if (PAGE_V7PUBSUB == currentSelection)
	{
		m_ps.ClearAll();
	}
}

void CRfhutilView::OnJson() 

{
	if (PAGE_DATA == currentSelection)
	{
		m_data.setJson();
	}
}

void CRfhutilView::OnFix() 

{
	if (PAGE_DATA == currentSelection)
	{
		m_data.setFix();
	}
}

void CRfhutilView::OnThai() 

{
	if (PAGE_DATA == currentSelection)
	{
		m_data.setThai();
	}
}

void CRfhutilView::OnWriteMsgs()

{
	if (PAGE_V7PUBSUB == currentSelection)
	{
		// invoke the proper function for this accelerator
		m_ps.WriteMsgs();
	}
}
