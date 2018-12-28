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

// rfhutilDoc.cpp : implementation of the CRfhutilDoc class
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"

#include "rfhutilDoc.h"
#include "rfhutilView.h"

#ifdef _DEBUG
#depne new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MRU_FILE_KEY	"Software\\IBM\\RFHUtil\\RFHUtil\\Recent File List"

/////////////////////////////////////////////////////////////////////////////
// CRfhutilDoc

IMPLEMENT_DYNCREATE(CRfhutilDoc, CDocument)

BEGIN_MESSAGE_MAP(CRfhutilDoc, CDocument)
	//{{AFX_MSG_MAP(CRfhutilDoc)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_FILE_MRU_FILE1, OnFileMruFile1)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRfhutilDoc construction/destruction

CRfhutilDoc::CRfhutilDoc()

{
}

CRfhutilDoc::~CRfhutilDoc()

{
}

BOOL CRfhutilDoc::OnNewDocument()

{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CRfhutilDoc serialization

void CRfhutilDoc::Serialize(CArchive& ar)

{
	CRfhutilApp		*app;
	CFile			*file;

	file = ar.GetFile();
	app = (CRfhutilApp *)AfxGetApp();

	if (ar.IsStoring())
	{
		// add storing code here
		// not used in RFHUtil
	}
	else
	{
		// read in the file
		app->pDocument.ReadFileData((LPCTSTR)file->GetFilePath());
		app->pDocument.updateMsgText();

		// update the current view
		UpdateAllViews(NULL);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRfhutilDoc diagnostics

#ifdef _DEBUG
void CRfhutilDoc::AssertValid() const

{
	CDocument::AssertValid();
}

void CRfhutilDoc::Dump(CDumpContext& dc) const

{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRfhutilDoc commands

void CRfhutilDoc::OnEditCopy() 

{
	int				id=0;
	int				found=0;
	CWnd			*pWnd;
	CRfhutilApp		*app=(CRfhutilApp *)AfxGetApp();
	char			traceInfo[128];

	// Get the main window
	pWnd = app->GetMainWnd();
	if (pWnd != 0)
	{
		// find the first child window
		pWnd = pWnd->GetFocus();
		if (pWnd != 0)
		{
			id = pWnd->GetDlgCtrlID();

			if (checkIDforPaste(id) || checkIDforCopyOnly(id))
			{
				// perform the copy to the clipboard
				((CEdit *)pWnd)->Copy();
				found = 1;
			}
		}
	}

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilDoc::OnEditCopy id=%d found=%d", id, found);
		app->logTraceEntry(traceInfo);
	}
}

void CRfhutilDoc::OnEditCut() 

{
	int				id=0;
	int				found=0;
	CWnd			*pWnd;
	CRfhutilApp		*app=(CRfhutilApp *)AfxGetApp();
	char			traceInfo[128];

	// Get the main window
	pWnd = app->GetMainWnd();
	if (pWnd != 0)
	{
		// find the first child window
		pWnd = pWnd->GetFocus();
		if (pWnd != 0)
		{
			id = pWnd->GetDlgCtrlID();

			if (checkIDforPaste(id))
			{
				// perform the cut operation, adding the text to the clipboard
				((CEdit *)pWnd)->Cut();
				found = 2;
			}
			else if (checkIDforCopyOnly(id))
			{
				// Treat as copy for read only controls
				((CEdit *)pWnd)->Copy();
				found = 1;
			}
		}
	}

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilDoc::OnEditCut id=%d found=%d", id, found);
		app->logTraceEntry(traceInfo);
	}
}

void CRfhutilDoc::OnEditPaste() 

{
	int				id=0;
	int				found=0;
	CWnd			*pWnd;
	CRfhutilApp		*app=(CRfhutilApp *)AfxGetApp();
	char			traceInfo[128];

	// get the main frame window
	pWnd = app->GetMainWnd();
	if (pWnd != 0)
	{
		// find the first child window
		pWnd = pWnd->GetFocus();
		if (pWnd != 0)
		{
			// get the dialog id
			id = pWnd->GetDlgCtrlID();

			// figure out which control the user is trying to paste into
			if (checkIDforPaste(id))
			{
				// the paste is allowed, so perform the paste operation
				((CEdit *)pWnd)->Paste();
				found = 4;

				// check if we have pasted into a message id, correlation id or group id field
				if ((IDC_CORREL_ID == id) || (IDC_GROUP_ID == id) || (IDC_MSG_ID == id))
				{
					// need to check if we pasted in valid data and if the length is too long
					// the mqmd class will be called
					// to find the mqmd class we must first find the view class
					POSITION pos = GetFirstViewPosition();
					CRfhutilView *view = (CRfhutilView*) GetNextView(pos);
					if (view != NULL)
					{
						view->editIDField(id);
						found = 5;
					}
				}
			}
		}
	}

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// build the trace line and add the line to the trace file
		sprintf(traceInfo, "Entered CRfhutilDoc::OnEditPaste id=%d found=%d", id, found);
		app->logTraceEntry(traceInfo);
	}
}

void CRfhutilDoc::OnEditSelectAll() 

{
	CWnd	*pWnd;
	int		id;

	// handle a select all (Ctrl + A or use the Edit->Select menu option
	pWnd = ((CRfhutilApp *)AfxGetApp())->GetMainWnd();
	if (pWnd != 0)
	{
		// find the first child window
		pWnd = pWnd->GetFocus();
		if (pWnd != 0)
		{
			id = pWnd->GetDlgCtrlID();

			if (checkIDforPaste(id) || checkIDforCopyOnly(id))
			{
				((CEdit *)pWnd)->SetSel(0, -1, TRUE);
			}
		}
	}
}

BOOL CRfhutilDoc::OnOpenDocument(LPCTSTR lpszPathName)
 
{
	CRfhutilApp		*app;
	char			fileName[512];
	char			traceInfo[512];

	strcpy(fileName, lpszPathName);

	app = (CRfhutilApp *)AfxGetApp();
	if (app->isTraceEnabled())
	{
		sprintf(traceInfo, "Entered CRfhutilDoc::OnOpenDocument PathName %s", fileName);
		app->logTraceEntry(traceInfo);
	}

//	if (!CDocument::OnOpenDocument(lpszPathName))
//		return FALSE;

	openFile(fileName);
	return TRUE;
}

void CRfhutilDoc::getRecentFile(int fileNum, char *fileName, const int size)

{
	int				ret;
	unsigned long	len;
	unsigned long	nameValueType=0;
	HKEY			regkey;
	CRfhutilApp		*app;
	char			keyName[256];
	char			traceInfo[756];

	app = (CRfhutilApp *)AfxGetApp();
	if (app->isTraceEnabled())
	{
		sprintf(traceInfo, "Entered CRfhutilDoc::getRecentFile fileNum %d fileName %s size %d", fileNum, fileName, size);
		app->logTraceEntry(traceInfo);
	}

	// initialize the file name
	fileName[0] = 0;
	len = size - 1;
	sprintf(keyName, "File%d", fileNum);

	// open the registry key that we will enumerate
	ret = RegOpenKeyEx(HKEY_CURRENT_USER,
					   MRU_FILE_KEY,
					   0,
					   KEY_QUERY_VALUE,
					   &regkey);

	if (ERROR_SUCCESS == ret)
	{
		ret = RegQueryValueEx(regkey,
							 keyName,
							 0,
							 &nameValueType,
							 (unsigned char *)fileName,
							 &len);

		if (ret != ERROR_SUCCESS)
		{
			memset(fileName, 0, size);
		}

		/* close the key */
		ret = RegCloseKey(regkey);
	}

	if (app->isTraceEnabled())
	{
		sprintf(traceInfo, "CRfhutilDoc::getRecentFile returned fileName %s", fileName);
		app->logTraceEntry(traceInfo);
	}
}

////////////////////////////////////////////////
//
// This routine handles forces a reread of the
// last file read if the recent file list menu
// entry is selected.
//
////////////////////////////////////////////////

void CRfhutilDoc::OnFileMruFile1() 

{
	// Special handling for the first file name in the most recently used list
	CRfhutilApp *	app;
	char			traceInfo[512];
	char			fileName[512];

	app = (CRfhutilApp *)AfxGetApp();
	if (app->isTraceEnabled())
	{
		app->logTraceEntry("Entered CRfhutilDoc::OnFileMruFile1");
	}

	memset(fileName, 0, sizeof(fileName));

	if (0 == app->pDocument.lastFileRead[0])
	{
		getRecentFile(1, fileName, sizeof(fileName));

		if (app->isTraceEnabled())
		{
			sprintf(traceInfo, "getRecentFile returned fileName %s", fileName);
			app->logTraceEntry(traceInfo);
		}

		if (fileName[0] != 0)
		{
			app->pDocument.ReadFileData(fileName);
		}
	}
	else
	{
		if (strlen(app->pDocument.lastFileRead) > 0)
		{
			strcpy(fileName, app->pDocument.lastFileRead);
			if (app->isTraceEnabled())
			{
				sprintf(traceInfo, "Using app->pDocument.lastFileRead %s", fileName);
				app->logTraceEntry(traceInfo);
			}

			app->pDocument.ReadFileData(fileName);
		}
	}

	((CRfhutilApp *)AfxGetApp())->pDocument.updateMsgText();
	UpdateAllViews(NULL);
}

void CRfhutilDoc::openFile(LPCTSTR lpszPathName)

{
	CRfhutilApp *	app;
	char			traceInfo[512];

	app = (CRfhutilApp *)AfxGetApp();
	if (app->isTraceEnabled())
	{
		sprintf(traceInfo, "Entered CRfhutilDoc::openFile for %s", lpszPathName);
		app->logTraceEntry(traceInfo);
	}
//	((CRfhutilApp *)AfxGetApp())->pDocument.ReadFile(lpszPathName);

//	((CRfhutilApp *)AfxGetApp())->pDocument.updateMsgText();
	UpdateAllViews(NULL);
}

BOOL CRfhutilDoc::checkIDforPaste(int id)

{
	BOOL	result=FALSE;

	if ((1001 == id) ||
		(IDC_MSG_DATA == id) ||
		(IDC_REMOTE_QM == id) ||
		(IDC_Q_NAME == id) ||
		(IDC_RFH_FORMAT == id) ||
		(IDC_RFH_CHARSET == id) ||
		(IDC_RFH_CCSID == id) ||
		(IDC_RFH1_APP_GROUP == id) ||
		(IDC_RFH1_FORMAT_NAME == id) ||
		(IDC_MSG_DOMAIN == id) ||
		(IDC_MSG_SET == id) ||
		(IDC_MSG_TYPE == id) ||
		(IDC_MSG_FMT == id) ||
		(IDC_MQMD_FORMAT == id) ||
		(IDC_EXPIRY == id) ||
		(IDC_MSGTYPE == id) ||
		(IDC_CORREL_ID == id) ||
		(IDC_GROUP_ID == id) ||
		(IDC_SEQNO == id) ||
		(IDC_MQMD_CCSID == id) ||
		(IDC_REPLY_QM == id) ||
		(IDC_REPLY_QUEUE == id) ||
		(IDC_PROPS_USER_PROPERTIES == id) || 
		(IDC_PS_QM == id) || 
		(IDC_PS_QUEUE_NAME == id) || 
		(IDC_PS_TOPIC_NAME == id) || 
		(IDC_PS_TOPIC == id) || 
		(IDC_PS_USER_DATA == id) || 
		(IDC_PS_CORREL_ID == id) || 
		(IDC_PS_BROKER_QM == id) || 
		(IDC_PS_SUBNAME == id) || 
		(IDC_PS_SELECTION == id) || 
		(IDC_PS_CORREL_ID == id) || 
		(IDC_PS_APPL_IDENT == id) || 
		(IDC_PS_EXPIRY == id) || 
		(IDC_PS_PRIORITY == id) || 
		(IDC_PS_SUB_LEVEL == id) || 
		(IDC_PS_WAIT_INTERVAL == id) || 
		(IDC_JMS_DST == id) ||
		(IDC_JMS_RTO == id) ||
		(IDC_JMS_CORREL_ID == id) ||
		(IDC_JMS_GROUP_ID == id) ||
		(IDC_JMS_SEQUENCE == id) ||
		(IDC_JMS_PRIORITY == id) ||
		(IDC_JMS_EXPIRATION == id) ||
		(IDC_JMS_DELIVERY_MODE == id) ||
		(IDC_JMS_TIMESTAMP == id) ||
		(IDC_JMS_USER_DEF == id) ||
		(IDC_OTHER_RFH_DATA == id) ||
		(IDC_RFH_USR_DATA == id) ||
		(IDC_PUB_TOPIC1 == id) ||
		(IDC_PUB_TOPIC2 == id) ||
		(IDC_PUB_TOPIC3 == id) ||
		(IDC_PUB_TOPIC4 == id) ||
		(IDC_PUB_SUBPOINT == id) ||
		(IDC_PUB_FILTER == id) ||
		(IDC_PUB_SUB_NAME == id) ||
		(IDC_PUB_SUB_IDENTITY == id) ||
		(IDC_PUB_SUB_DATA == id) ||
		(IDC_PS_CONNECT_QM == id) ||
		(IDC_PS_CONNECT_Q == id) ||
		(IDC_PUB_BROKER_QM == id) ||
		(IDC_PUB_QM == id) ||
		(IDC_PUB_Q == id) ||
		(IDC_PSCR_ERROR_ID == id) ||
		(IDC_PSCR_ERROR_POS == id) ||
		(IDC_PSCR_PARM_ID == id) ||
		(IDC_PSCR_OTHER == id) ||
		(IDC_REASON1 == id) ||
		(IDC_RESPONSE1_MESG == id) ||
		(IDC_REASON2 == id) ||
		(IDC_RESPONSE2_MESG == id) ||
		(IDC_CIH_FACILITY == id) ||
		(IDC_CIH_FORMAT == id) ||
		(IDC_CIH_PROGRAM == id) ||
		(IDC_CIH_NEXT_TRANID == id) ||
		(IDC_CIH_CANCEL_CODE == id) ||
		(IDC_CIH_CURSOR_POS == id) ||
		(IDC_CIH_ERROR_OFFSET == id) ||
		(IDC_CIH_FACILITY_LIKE == id) ||
		(IDC_CIH_AID == id) ||
		(IDC_CIH_TRANS_ID == id) ||
		(IDC_CIH_REMOTE_SYS == id) ||
		(IDC_CIH_REMOTE_TRAN == id) ||
		(IDC_CIH_REPLY_FORMAT == id) ||
		(IDC_CIH_CODEPAGE == id) ||
		(IDC_CIH_WAIT_INTERVAL == id) ||
		(IDC_CIH_DATA_LENGTH == id) ||
		(IDC_CIH_ABEND_CODE == id) ||
		(IDC_CIH_AUTHENTICATOR == id) ||
		(IDC_CIH_KEEP_TIME == id) ||
		(IDC_CIH_FUNCTION == id) ||
		(IDC_DLQ_REASON == id) ||
		(IDC_DLQ_ORIG_QM == id) ||
		(IDC_DLQ_ORIG_Q == id) ||
		(IDC_DLQ_FORMAT == id) ||
		(IDC_DLQ_CODEPAGE == id) ||
		(IDC_DLQ_PUT_APPL_NAME == id) ||
		(IDC_DLQ_DATE_TIME == id) ||
		(IDC_LOADQ_DELIMITER == id) ||
		(IDC_LOADQ_FILENAME == id) ||
		(IDC_LOADQ_MAX_COUNT == id) ||
		(IDC_LOADQ_FORMAT == id) ||
		(IDC_LOADQ_CCSID == id) ||
		(IDC_SAVEMSGS_FILENAME == id) ||
		(IDC_SAVEMSGS_DELIMITER == id) ||
		(IDC_DISPMSGS_END_COUNT == id) ||
		(IDC_DISPMSGS_START == id) ||
		(IDC_LOADQ_DELIMITER == id) ||
		(IDC_LOADQ_MAX_COUNT == id) ||
		(IDC_LOADQ_FORMAT == id) ||
		(IDC_LOADQ_CCSID == id) ||
		(IDC_CAPTURE_FILENAME == id) ||
		(IDC_CAPTURE_DELIMITER == id) ||
		(IDC_CAPTURE_PROP_DELIM == id) ||
		(IDC_CAPTURE_END_COUNT == id) ||
		(IDC_CAPTURE_MAX_WAIT == id) ||
		(IDC_WRITEPUBS_FILENAME == id) ||
		(IDC_WRITEPUBS_DELIMITER == id) ||
		(IDC_WRITEPUBS_PROP_DELIM == id) ||
		(IDC_WRITEPUBS_END_COUNT == id) ||
		(IDC_WRITEPUBS_WAIT == id) ||
		(IDC_WRITEPUBS_BATCHSIZE == id) ||
		(IDC_IIH_FORMAT == id) ||
		(IDC_IIH_MAP_NAME == id) ||
		(IDC_IIH_REPLY_FORMAT == id) ||
		(IDC_IIH_LTERM == id) ||
		(IDC_IIH_AUTHENTICATOR == id) ||
		(IDC_IIH_CODEPAGE == id) ||
		(IDC_IIH_TRANS_ID == id))
	{
		result = TRUE;
	}

	return result;
}

BOOL CRfhutilDoc::checkIDforCopyOnly(int id)

{
	BOOL	result=FALSE;

	if ((IDC_MSG_DATA == id) ||
		(IDC_COPYBOOK_FILE_NAME == id) ||
		(IDC_FILE_NAME == id) ||
		(IDC_ERRMSG == id) ||
		(IDC_USER_ID == id) ||
		(IDC_APPL_TYPE == id) ||
		(IDC_PUT_DATE_TIME == id) ||
		(IDC_MSG_ID == id) ||
		(IDC_APPL_IDENTITY == id) ||
		(IDC_APPL_ORIGIN == id) ||
		(IDC_APPL_NAME == id) ||
		(IDC_ACCOUNT_TOKEN == id) ||
		(IDC_PS_ERRORMSGS == id) ||
		(IDC_PUB_ERR_MSG == id))
	{
		result = TRUE;
	}

	return result;
}

void CRfhutilDoc::OnFileSave() 

{
	int			rc;
	CRfhutilApp	*app=(CRfhutilApp *)AfxGetApp();

	// check if trace is active
	if (app->isTraceEnabled())
	{
		// insert an entry into the trace file
		app->logTraceEntry("Entered CRfhutilDoc::OnFileSave()");
	}

	// find the current view
	POSITION pos = GetFirstViewPosition();
	CView* pFirstView = GetNextView( pos );

	// execute the kill active method for the view
	// this will ensure that the instance variables reflect the current data
	((CRfhutilView *)pFirstView)->UpdateAllFields();

	// invoke standard dialog to choose file name
	CFileDialog fd(FALSE, NULL, NULL, OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT);
	rc = fd.DoModal();

	//	Save the copy book file name 
	if (rc == IDOK)
	{
		// set the full file name in the DataArea object
		strcpy(app->pDocument.fileName, fd.GetPathName( ));

		// drive the document file processing
		app->BeginWaitCursor();
		app->pDocument.WriteFile(fd.GetPathName());
		app->EndWaitCursor();
	}

	// reflect the latest information in the display
	UpdateAllViews(NULL);
}
