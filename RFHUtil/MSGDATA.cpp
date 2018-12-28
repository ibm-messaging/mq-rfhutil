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

// MSGDATA.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "MSGDATA.h"
#include "rfhutilDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+3

/////////////////////////////////////////////////////////////////////////////
// MSGDATA property page

IMPLEMENT_DYNCREATE(MSGDATA, CPropertyPage)

MSGDATA::MSGDATA() : CPropertyPage(MSGDATA::IDD)
{
	//{{AFX_DATA_INIT(MSGDATA)
	m_msg_data = _T("");
	m_data_format = DATA_CHARACTER;
	m_character_format = CHAR_ASCII;
	m_numeric_format = NUMERIC_PC;
	m_pd_numeric_format = NUMERIC_PC;
	m_crlf = FALSE;
	m_edi = FALSE;
	m_BOM = FALSE;
	m_data_indent = FALSE;
	m_save_data_type = DATA_CHARACTER;
	m_called_copybook = 0;
	m_checkData = FALSE;
	curDisplayResolution = 0;
	//}}AFX_DATA_INIT

	// initialize the tab stop locations
	charTabStops[0] = 36;
	charTabStops[1] = 72;
	hexTabStops[0] = 36;
	hexTabStops[1] = 72;
	bothTabStops[0] = 36;
	bothTabStops[1] = 72;
	bothTabStops[2] = 108;
}

MSGDATA::~MSGDATA()
{
}

void MSGDATA::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MSGDATA)
	DDX_Text(pDX, IDC_MSG_DATA, m_msg_data);
	DDX_Radio(pDX, IDC_DATA_FORMAT, m_data_format);
	DDX_Radio(pDX, IDC_ASCII, m_character_format);
	DDX_Radio(pDX, IDC_NUMERIC_PC, m_numeric_format);
	DDX_Radio(pDX, IDC_PD_DATA_INTEL, m_pd_numeric_format);
	DDX_Check(pDX, IDC_CRLF, m_crlf);
	DDX_Check(pDX, IDC_EDI, m_edi);
	DDX_Check(pDX, IDC_DATA_BOM, m_BOM);
	DDX_Check(pDX, IDC_INDENT, m_data_indent);
	DDX_Check(pDX, IDC_DATA_VALIDATE, m_checkData);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(MSGDATA, CPropertyPage)
	//{{AFX_MSG_MAP(MSGDATA)
	ON_BN_CLICKED(IDC_ASCII, OnAscii)
	ON_BN_CLICKED(IDC_COBOL, OnCobol)
	ON_BN_CLICKED(IDC_HEX, OnHex)
	ON_BN_CLICKED(IDC_XML, OnXml)
	ON_BN_CLICKED(IDC_EBCDIC, OnEbcdic)
	ON_BN_CLICKED(IDC_BOTH, OnBoth)
	ON_BN_CLICKED(IDC_PARSED, OnParsed)
	ON_BN_CLICKED(IDC_COPYBOOK, OnCopybook)
	ON_BN_CLICKED(IDC_DATA_FORMAT, OnCharacter)
	ON_BN_CLICKED(IDC_HOST, OnHost)
	ON_BN_CLICKED(IDC_NUMERIC_PC, OnNumericPc)
	ON_BN_CLICKED(IDC_CRLF, OnCrlf)
	ON_BN_CLICKED(IDC_DATA_BRNEXT, OnDataBrnext)
	ON_BN_CLICKED(IDC_EDI, OnEdi)
	ON_BN_CLICKED(IDC_PD_DATA_INTEL, OnPdDataIntel)
	ON_BN_CLICKED(IDC_PD_DATA_HOST, OnPdDataHost)
	ON_BN_CLICKED(IDC_CHINESE, OnChinese)
	ON_BN_CLICKED(IDC_KOREAN, OnKorean)
	ON_BN_CLICKED(IDC_TRAD_CHINESE, OnTradChinese)
	ON_BN_CLICKED(IDC_JAPAN, OnJapan)
	ON_BN_CLICKED(IDC_INDENT, OnIndent)
	ON_BN_CLICKED(IDC_DATA_VALIDATE, OnDataValidate)
	ON_BN_CLICKED(IDC_DATA_BRPREV, OnDataBrprev)
	ON_BN_CLICKED(IDC_THAI, OnThai)
	ON_BN_CLICKED(IDC_JSON, OnJson)
	ON_BN_CLICKED(IDC_FIX, OnFix)
	ON_BN_CLICKED(IDC_RUSSIAN, OnRussian)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, MSGDATA::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, MSGDATA::GetToolTipText)
	ON_BN_CLICKED(IDC_DATA_BOM, &MSGDATA::OnBnClickedDataBom)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MSGDATA message handlers

void MSGDATA::OnAscii() 
{
	// Update the ASCII/EBCDIC data type in the document
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering MSGDATA::OnAscii() pDoc->m_char_format=%d", pDoc->m_char_format);

		// trace entry to OnAscii
		pDoc->logTraceEntry(traceInfo);
	}

	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	pDoc->m_char_format = CHAR_ASCII;

	// make sure we have the right font
	setFixedFont();

	// update the data display
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exit from MSGDATA::OnAscii() pDoc->m_char_format=%d", pDoc->m_char_format);

		// trace exit from OnAscii
		pDoc->logTraceEntry(traceInfo);
	}
}

//////////////////////////////////////////////////////////////////
//
// The COBOL file display option has been selected by the user
// A dialog box will be displayed if the user has not previously
// selected a COBOL copy book to use for the format operation
//
//////////////////////////////////////////////////////////////////

void MSGDATA::OnCobol() 

{
	int	rc=0;
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// do we already have a copy book name?
	if (0 == pDoc->m_copybook_file_name.GetLength())
	{
		// No copybook name, ask the user for one
		rc = pDoc->readCopybookFile ();

		if (rc == IDCANCEL)
		{
			// User cancelled out and no file name, so revert to previous display
			// restore the previous data format
			m_data_format = m_save_data_type;

			// Update the controls
			UpdateData (FALSE);
		}
	}

	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

///////////////////////////////////////////////////////////
//
// The hex display option has been selected by the user.
// The current message data will be displayed in a 
// hex format.
//
///////////////////////////////////////////////////////////

void MSGDATA::OnHex() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// try to set tab stops
	cedit->SetTabStops(2, (int *)&hexTabStops);

	// update the data in the message data CEdit control
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

///////////////////////////////////////////////////////////
//
// The XML display option has been selected by the user.
// The current message data will be parsed and formatted
// as XML.  
//
///////////////////////////////////////////////////////////

void MSGDATA::OnXml() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// update the data in the message data CEdit control
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnJson() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// update the data in the message data CEdit control
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnFix() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// update the data in the message data CEdit control
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::updateMsgdata()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering MSGDATA::updateMsgdata() m_character_format=%d pDoc->m_char_format=%d m_numeric_format=%d pDoc->m_numeric_format=%d", m_character_format, pDoc->m_char_format, m_numeric_format, pDoc->m_numeric_format);

		// trace entry to updateMsgdata
		pDoc->logTraceEntry(traceInfo);

		// create the trace line
		sprintf(traceInfo, " m_pd_numeric_format=%d pDoc->m_pd_numeric_format=%d m_data_format=%d m_save_data_type=%d", m_pd_numeric_format, pDoc->m_pd_numeric_format, m_data_format, m_save_data_type);

		// trace entry to updateMsgdata
		pDoc->logTraceEntry(traceInfo);
	}

	BeginWaitCursor();

	// Update the variables from the controls
	UpdateData(TRUE);

	// make sure our encoding and code page settings are up to date
	// they are changed whenever a file or message is read
	m_numeric_format = pDoc->m_numeric_format;
	m_pd_numeric_format = pDoc->m_pd_numeric_format;
	m_character_format = pDoc->m_char_format;

	// make sure we have the right font loaded
	setDisplayFont();

	switch (m_data_format)
	{
	case DATA_CHARACTER:
		{
			// Data is in ascii characters
			pDoc->getCharacterData(m_character_format, m_crlf, m_edi, m_BOM, m_data_indent);
			m_msg_data = pDoc->m_data_ascii;

			break;
		}
	case DATA_HEX:
		{
			// Data is in hex
			pDoc->getHexData();
			m_msg_data = pDoc->m_data_hex;
			break;
		}
	case DATA_BOTH:
		{
			// Data is in ascii and hex
			pDoc->getBothData(m_character_format, m_BOM);
			m_msg_data = pDoc->m_data_both;
			break;
		}
	case DATA_XML:
		{
			// Data is in XML
			pDoc->getXmlData(m_character_format);
			m_msg_data = pDoc->m_data_xml;
			break;
		}
	case DATA_PARSED:
		{
			// Data is in Parsed XML
			pDoc->getParsedData(m_character_format);
			m_msg_data = pDoc->m_data_parsed;
			break;
		}
	case DATA_COBOL:
		{
			// Data is in COBOL copybook format
			m_msg_data = pDoc->getCobolData(m_character_format, m_numeric_format, m_pd_numeric_format, m_data_indent, m_checkData);
			break;
		}
	case DATA_JSON:
		{
			// Data is in JSON format
			pDoc->getJsonData(m_character_format);
			m_msg_data = pDoc->m_data_json;
			break;
		}
	case DATA_FIX:
		{
			// Data is in Fix format
			pDoc->getFixData(m_character_format);
			m_msg_data = pDoc->m_data_fix;
			break;
		}
	}

	// remember the current display type
	m_save_data_type = m_data_format;

	// get the source of the data
	((CStatic *)GetDlgItem(IDC_STATIC_MESSAGE_DATA))->SetWindowText((LPCTSTR)pDoc->fileSource);

	// check if we need to enable the browse next button
	if (1 == pDoc->browseActive)
	{
		// browse is active, set the button appropriately
		((CButton *)GetDlgItem(IDC_DATA_BRNEXT))->EnableWindow(TRUE);

		// check if we should enable the browse prev button
		if (1 == pDoc->browsePrevActive)
		{
			// browse is active, set the button appropriately
			((CButton *)GetDlgItem(IDC_DATA_BRPREV))->EnableWindow(TRUE);
		}
		else
		{
			((CButton *)GetDlgItem(IDC_DATA_BRPREV))->EnableWindow(FALSE);
		}
	}
	else
	{
		// browse is not active, set the button appropriately
		((CButton *)GetDlgItem(IDC_DATA_BRNEXT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_DATA_BRPREV))->EnableWindow(FALSE);
	}

	// Update the controls from the instance data
	UpdateData(FALSE);

	EndWaitCursor();

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exit from MSGDATA::updateMsgdata() m_character_format=%d pDoc->m_char_format=%d", m_character_format, pDoc->m_char_format);

		// trace exit from updateMsgdata
		pDoc->logTraceEntry(traceInfo);
	}
}

void MSGDATA::setDisplayFont()

{
	switch (pDoc->m_char_format)
	{
	case CHAR_ASCII:
	case CHAR_EBCDIC:
		{
			setFixedFont();
			break;
		}
	case CHAR_CHINESE:
		{
			setChinaFont();
			break;
		}
	case CHAR_KOREAN:
		{
			setKoreaFont();
			break;
		}
	case CHAR_TRAD_CHIN:
		{
			setBig5Font();
			break;
		}
	case CHAR_JAPANESE:
		{
			setJapanFont();
			break;
		}
	case CHAR_THAI:
		{
			setThaiFont();
			break;
		}
	case CHAR_RUSSIAN:
		{
			setRussianFont();
			break;
		}
	default:
		{
			setFixedFont();
			break;
		}
	}
}

BOOL MSGDATA::OnSetActive() 

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();

	if (pDoc->traceEnabled)
	{
		// trace entry to OnEbcdic
		pDoc->logTraceEntry("Entering MSGDATA::OnSetActive()");
	}

	// select the correct font for display
	setDisplayFont();

	// get the data to display
	updateMsgdata();

	// set off any radio buttons for which the appropriate font 
	// cannot be found on the system
	if (!app->chinaBig5OK)
	{
		((CButton *)GetDlgItem(IDC_TRAD_CHINESE))->EnableWindow(FALSE);
	}

	if (!app->chinaSimpOK)
	{
		((CButton *)GetDlgItem(IDC_CHINESE))->EnableWindow(FALSE);
	}
	
	if (!app->japanOK)
	{
		((CButton *)GetDlgItem(IDC_JAPAN))->EnableWindow(FALSE);
	}

	if (!app->koreaOK)
	{
		((CButton *)GetDlgItem(IDC_KOREAN))->EnableWindow(FALSE);
	}
	
	if (!app->thaiOK)
	{
		((CButton *)GetDlgItem(IDC_THAI))->EnableWindow(FALSE);
	}
	
	if (!app->russianOK)
	{
		((CButton *)GetDlgItem(IDC_RUSSIAN))->EnableWindow(FALSE);
	}
	
	BOOL ret = CPropertyPage::OnSetActive();
	
	// send a custom message to switch the focus to the QM combo box
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return ret;
}

void MSGDATA::updateFixedFont(LONG height)

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();

	int		i;
	char	traceInfo[512];			// work variable to build trace message

	// create the default fixed font for ANSI character sets
	i = app->createFixedFonts(&(app->m_fixed_font), "Courier New", FF_MODERN | FIXED_PITCH);

	if (pDoc->traceEnabled)
	{
		// create a trace entry
		sprintf(traceInfo, "MSGDATA::updateFixedFont height=%d i=%d", 14 * (app->dpi), i);

		// write the entry to the trace
		pDoc->logTraceEntry(traceInfo);
	}
}

void MSGDATA::setFixedFont()

{
	CRfhutilApp		*app = (CRfhutilApp *)AfxGetApp();

	// Update the ASCII/EBCDIC data type in the document
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// check if we have created the right size font
	if (curDisplayResolution != app->horLogPixels)
	{
		// calculate the right sized font
		if (app->horLogPixels > 200)
		{
			updateFixedFont(-40);
		}

		// remember so we won't do this every time
		curDisplayResolution = app->horLogPixels;
	}

	// Set a fixed font for the message data edit box
	cedit->SetFont(&app->m_fixed_font, TRUE);

	// trace the result
	traceFont(cedit, &(app->m_fixed_font));
}

void MSGDATA::OnEbcdic() 

{
	// User has asked the data be treated as EBCDIC data
	// It will be translated to ASCII before it is displayed
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering MSGDATA::OnEbcdic() pDoc->m_char_format=%d", pDoc->m_char_format);

		// trace entry to OnEbcdic
		pDoc->logTraceEntry(traceInfo);
	}

	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// Update the ASCII/EBCDIC data type in the document
	pDoc->m_char_format = CHAR_EBCDIC;

	// use fixed font to display the data
	setFixedFont();

	// update the message display
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exit from MSGDATA::OnEbcdic() pDoc->m_char_format=%d", pDoc->m_char_format);

		// trace exit from OnAscii
		pDoc->logTraceEntry(traceInfo);
	}
}

void MSGDATA::OnBoth() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// try to set tab stops
	cedit->SetTabStops(3, (int *)&bothTabStops);

	// update the data in the message data CEdit control
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnParsed() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// update the data in the message data CEdit control
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnCopybook() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);
	int	rc;

	// give the user a chance to change the copy book name
	rc = pDoc->readCopybookFile ();

	// redisplay the data, possibly with a different copy book
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnCharacter() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// try to set tab stops
	cedit->SetTabStops(2, (int *)&charTabStops);

	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnHost() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// Update the Numeric data type in the document
	pDoc->m_numeric_format = NUMERIC_HOST;

	// packed data format only affects COBOL displays
	if (DATA_COBOL == m_data_format)
	{
		updateMsgdata();
	}

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnNumericPc() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// Update the Numeric data type in the document
	pDoc->m_numeric_format = NUMERIC_PC;

	// data format only affects COBOL displays
	if (DATA_COBOL == m_data_format)
	{
		// update the data in the message data CEdit control
		updateMsgdata();
	}

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnCrlf() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// update the data in the message data CEdit control
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnDataBrnext() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// start the wait cursor
	((CRfhutilApp *)AfxGetApp())->BeginWaitCursor();

	// Read the next message
	pDoc->browseNext(false);

	// end the wait cursor
	((CRfhutilApp *)AfxGetApp())->EndWaitCursor();

	// update the message text in the general tab
	pDoc->updateMsgText();

	// refresh the variables on the screen
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnDataBrprev() 

{
	int		rc;

	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// start the wait cursor
	((CRfhutilApp *)AfxGetApp())->BeginWaitCursor();

	// Read the previous message
	rc = pDoc->browsePrev();

	// end the wait cursor
	((CRfhutilApp *)AfxGetApp())->EndWaitCursor();

	// update the message text in the general tab
	pDoc->updateMsgText();

	// refresh the variables on the screen
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnEdi()

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit = (CEdit *)GetDlgItem(IDC_MSG_DATA);

	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnBOM()

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit = (CEdit *)GetDlgItem(IDC_MSG_DATA);

	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnPdDataIntel()
{
	// Update the Numeric data type in the document
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	pDoc->m_pd_numeric_format = NUMERIC_PC;

	// Is the current data display format COBOL?
	if (DATA_COBOL == m_data_format)
	{
		// Display format is COBOL, so must update the data to reflect
		// the new choice for data encoding
		updateMsgdata();
	}

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnPdDataHost() 

{
	// Update the Numeric data type in the document
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	pDoc->m_pd_numeric_format = NUMERIC_HOST;

	// packed data format only affects COBOL displays
	if (DATA_COBOL == m_data_format)
	{
		// Display format is COBOL, so must update the data to reflect
		// the new choice for data encoding
		updateMsgdata();
	}

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

BOOL MSGDATA::OnInitDialog() 

{
	CPropertyPage::OnInitDialog();
	
	// set the windowCControl style
	ModifyStyle(0, (DWORD)WS_TABSTOP | WS_GROUP);

	// Enable tool tips for this dialog
	EnableToolTips(TRUE);

	// disable the byte order mark (BOM) check box
	((CWnd *)GetDlgItem(IDC_DATA_BOM))->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL MSGDATA::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pTTTStruct;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pTTTStruct;
	CString strTipText;

	UINT nID = pTTTStruct->idFrom;

	if (pTTTStruct->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pTTTStruct->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
		if(nID != 0)
		{
			if (pTTTStruct->code == TTN_NEEDTEXTA)
			{
				pTTTA->lpszText = MAKEINTRESOURCE(nID);
				pTTTA->hinst = AfxGetResourceHandle();
			}
			else
			{
//				pTTTW->lpszText = MAKEINTRESOURCE(nID);
//				pTTTW->hinst = AfxGetResourceHandle();
			}
//			strTipText.Format("Control ID = %d", nID);

//			if (pTTTStruct->code == TTN_NEEDTEXTA)
//				lstrcpyn(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
//			else
//				_mbstowcsz(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
			pResult = 0;
			return(TRUE);
		}
	}

	return(FALSE);
}

void MSGDATA::OnChinese() 

{
	// Update the ASCII/EBCDIC data type in the document
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	pDoc->m_char_format = CHAR_CHINESE;

	// set the font in the edit box to chinese
	setChinaFont();

	// update the display area 
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnKorean() 

{
	// Update the display data type in the document
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	pDoc->m_char_format = CHAR_KOREAN;

	// set the font in the edit box to Korean
	setKoreaFont();

	// update the display area 
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnTradChinese() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// Update the display data type in the document
	pDoc->m_char_format = CHAR_TRAD_CHIN;

	// set the font to traditional chinese
	setBig5Font();

	// update the display area 
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnJapan() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// Update the display data type in the document
	pDoc->m_char_format = CHAR_JAPANESE;

	// set the appropriate font
	setJapanFont();

	// update the display area 
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnThai() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// Update the display data type in the document
	pDoc->m_char_format = CHAR_THAI;

	// set the appropriate font
	setThaiFont();

	// update the display area 
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnRussian() 

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// Update the display data type in the document
	pDoc->m_char_format = CHAR_RUSSIAN;

	// set the appropriate font
	setRussianFont();

	// update the display area 
	updateMsgdata();

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::setChinaFont()

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();

	setFont(&(app->m_china_font));
}

void MSGDATA::setKoreaFont()

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();

	setFont(&(app->m_korea_font));
}

void MSGDATA::setBig5Font()

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();

	setFont(&(app->m_big5_font));
}

void MSGDATA::setJapanFont()

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();

	setFont(&(app->m_japan_font));
}

void MSGDATA::setRussianFont()

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();

	setFont(&(app->m_russian_font));
}

void MSGDATA::setThaiFont()

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();

	setFont(&(app->m_thai_font));
}

void MSGDATA::setFont(CFont *font)

{
	CEdit *ce;

	// get a pointer to the edit box
	ce = (CEdit *)GetDlgItem(IDC_MSG_DATA);

	// Set a font for the message data edit box
	ce->SetFont(font,TRUE);

	// trace the result
	traceFont(ce, font);
}

void MSGDATA::getSelection(int &firstChar, int &lastChar)

{
	firstChar = -1;
	lastChar = -1;

	((CEdit *)GetDlgItem(IDC_MSG_DATA))->GetSel(firstChar, lastChar);
}

void MSGDATA::OnIndent() 

{
	// Update the indentation in the display
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	if ((DATA_COBOL == m_data_format) || (DATA_CHARACTER == m_data_format))
	{
		updateMsgdata();
	}

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::OnDataValidate() 
{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	if (DATA_COBOL == m_data_format)
	{
		// Update the indentation in the display
		updateMsgdata();
	}

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::moveDisplay(int line, int ofs, int count)

{
	int		charIndex;
	CPoint	point;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering MSGDATA::moveDisplay() line=%d ofs=%d count=%d", line, ofs, count);

		// trace entry to moveDisplay
		pDoc->logTraceEntry(traceInfo);
	}

	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	charIndex = cedit->LineIndex(line);
	cedit->LineScroll(line - cedit->GetFirstVisibleLine());
	cedit->SetSel(charIndex + ofs, charIndex + ofs + count);
	point = cedit->PosFromChar(charIndex + ofs);
	cedit->SetCaretPos(point);
	cedit->SetFocus();

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting MSGDATA::moveDisplay() charIndex=%d charIndex+ofs=%d", charIndex, charIndex + ofs);

		// trace exit from moveDisplay
		pDoc->logTraceEntry(traceInfo);
	}
}


void MSGDATA::EndBr()

{
	// Get a direct pointer to the message data CEdit control
	CEdit	*cedit=(CEdit *)GetDlgItem(IDC_MSG_DATA);

	// Read the next message
	pDoc->endBrowse(false);

	// update the message text in the general tab
	pDoc->updateMsgText();

	// browse is not active, set the button appropriately
	((CButton *)GetDlgItem(IDC_DATA_BRNEXT))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_DATA_BRPREV))->EnableWindow(FALSE);

	// return the focus to the edit display so the scrolling keys work
	cedit->SetFocus();
}

void MSGDATA::ReadCopybook()

{
	OnCopybook();
}

//////////////////////////////////////////////////////
//
// custom message handler to force the focus to the
// queue manager combo box control
//
//////////////////////////////////////////////////////

LONG MSGDATA::OnSetPageFocus(UINT wParam, LONG lParam)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering MSGDATA::OnSetPageFocus() wParam=%d lParam=%d", wParam, lParam);

		// trace entry to OnSetPageFocus
		pDoc->logTraceEntry(traceInfo);
	}

	// set the focus to the queue manager name combo box
	((CEdit *)GetDlgItem(IDC_MSG_DATA))->SetFocus();

	return 0;
}

BOOL MSGDATA::PreCreateWindow(CREATESTRUCT& cs) 

{
	// Set the appropriate windows styles
	cs.style |= WS_TABSTOP;
	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	
	return CPropertyPage::PreCreateWindow(cs);
}

BOOL MSGDATA::PreTranslateMessage(MSG* pMsg) 

{
	//
	// This routine is necessary for the tab key to work correctly
	//

	if (pMsg->message != WM_KEYDOWN)
		return CPropertyPage::PreTranslateMessage(pMsg);

	if (IsDialogMessage(pMsg))
		return TRUE;
	else
		return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL MSGDATA::OnKillActive() 

{
	// moving to a different page
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering MSGDATA::OnKillActive()");
	}

	// get the current data from the controls into the instance variables
	// this really does nothing as there are not controls on this page which alter any data
	// the controls only relate to the display format
	UpdateData(TRUE);
	
	return CPropertyPage::OnKillActive();
}

void MSGDATA::setCharacter()

{
	m_data_format = DATA_CHARACTER;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setHex()

{
	// update the instance variables from the controls
	//((CButton *)GetDlgItem(IDC_HEX))->SetCheck(TRUE);
	m_data_format = DATA_HEX;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setBoth()

{
	m_data_format = DATA_BOTH;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setXML()

{
	m_data_format = DATA_XML;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setParsed()

{
	m_data_format = DATA_PARSED;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setCobol()

{
	int	rc=0;

	m_data_format = DATA_COBOL;
	UpdateData (FALSE);

	// do we already have a copy book name?
	if (0 == pDoc->m_copybook_file_name.GetLength())
	{
		// No copybook name, ask the user for one
		rc = pDoc->readCopybookFile ();

		if (rc == IDCANCEL)
		{
			// User cancelled out and no file name, so revert to previous display
			// get the previous data format
			m_data_format = m_save_data_type;

			// Update the controls
			UpdateData (FALSE);
		}
	}

	updateMsgdata();
}

void MSGDATA::setJson()

{
	m_data_format = DATA_JSON;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setFix()

{
	m_data_format = DATA_FIX;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setAscii()

{
	m_character_format = CHAR_ASCII;
	pDoc->m_char_format = CHAR_ASCII;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setEBCDIC()

{
	m_character_format = CHAR_EBCDIC;
	pDoc->m_char_format = CHAR_EBCDIC;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setTradChinese()

{
	m_character_format = CHAR_TRAD_CHIN;
	pDoc->m_char_format = CHAR_TRAD_CHIN;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setSimpChinese()

{
	m_character_format = CHAR_CHINESE;
	pDoc->m_char_format = CHAR_CHINESE;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setKorean()

{
	m_character_format = CHAR_KOREAN;
	pDoc->m_char_format = CHAR_KOREAN;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setJapanese()

{
	m_character_format = CHAR_JAPANESE;
	pDoc->m_char_format = CHAR_JAPANESE;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::setThai()

{
	m_character_format = CHAR_THAI;
	pDoc->m_char_format = CHAR_THAI;
	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::toggleIntEncoding()

{
	// check the current setting of the packed decimal encoding
	if (NUMERIC_PC == m_numeric_format)
	{
		m_numeric_format = NUMERIC_HOST;
		pDoc->m_numeric_format = NUMERIC_HOST;
	}
	else
	{
		m_numeric_format = NUMERIC_PC;
		pDoc->m_numeric_format = NUMERIC_PC;
	}

	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::togglePDEncoding()

{
	// check the current setting of the packed decimal encoding
	if (NUMERIC_PC == m_pd_numeric_format)
	{
		m_pd_numeric_format = NUMERIC_HOST;
		pDoc->m_pd_numeric_format = NUMERIC_HOST;
	}
	else
	{
		m_pd_numeric_format = NUMERIC_PC;
		pDoc->m_pd_numeric_format = NUMERIC_PC;
	}

	UpdateData (FALSE);
	updateMsgdata();
}

void MSGDATA::traceFont(CEdit *cedit, CFont *font)

{
	UINT				rc=0;
	DWORD				err=-1;
	CDC					*pDC=NULL;
	OUTLINETEXTMETRIC	*otm;
	char				*family;
	char				*face;
	char				*style;
	char				*fullName;
	char				fontInfo[16384];
	char				traceInfo[16384];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// try to get a pointer to the device context object
		pDC = cedit->GetDC();

		if (pDC != NULL)
		{
			// set the font to use
			pDC->SelectObject(font);

			// try to retrieve the text metrics of the selected font
			rc = pDC->GetOutlineTextMetrics(sizeof(fontInfo), (OUTLINETEXTMETRIC *)fontInfo);

			// check if it worked
			if (rc != 0)
			{
				// set the font that was used
				// point to the returned text metrics
				otm = (OUTLINETEXTMETRIC *)&fontInfo;

				// initialize the pointers 
				family = (char *)&fontInfo + (int)(otm->otmpFamilyName);
				face = (char *)&fontInfo + (int)(otm->otmpFaceName);
				style = (char *)&fontInfo + (int)(otm->otmpStyleName);
				fullName = (char *)&fontInfo + (int)(otm->otmpFullName);

				// create the trace line
				sprintf(traceInfo, " Font selected charSet=%d pitch=%d family=%s face=%s style=%s fullName=%s", otm->otmTextMetrics.tmCharSet, otm->otmTextMetrics.tmPitchAndFamily, family, face, style, fullName);

				// trace entry to OnAscii
				pDoc->logTraceEntry(traceInfo);
			}
			else
			{
				err = GetLastError();
			}

			ReleaseDC(pDC);
		}
	}
}


void MSGDATA::OnBnClickedDataBom()
{
	// TODO: Add your control notification handler code here
}
