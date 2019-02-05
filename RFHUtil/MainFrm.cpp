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

// MainFrm.cpp : implementation of the CMainFrame class
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "general.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_SIZING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	memset(strTitle, 0, sizeof(strTitle));
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)

{
	CRfhutilApp	*app = (CRfhutilApp	*) AfxGetApp();
	RECT	rect;

	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	//  the CREATESTRUCT cs
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

	cs.cx = 850;
	cs.cy = 450;


	// do not exceed a full screen
	if (cs.cx > rect.right)
	{
//		cs.cx = rect.right;
	}

	if (cs.cy > rect.bottom)
	{
//		cs.cy = rect.bottom;
	}

	cs.style &= (~FWS_ADDTOTITLE);
	cs.style |= WS_SYSMENU;

	sprintf(strTitle, "RfhUtil V%d.%d.%d.%d",
			app->getMajor(),
			app->getMinor(),
			app->getRevision(),
			app->getBuild());
	
#ifdef MQCLIENT
	strcat(strTitle, " (Client)");
#endif

	cs.lpszName = strTitle;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


