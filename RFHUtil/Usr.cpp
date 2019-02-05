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

// Usr.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "xmlsubs.h"
#include "Usr.h"
#include "XMLParse.h"

// include for RFH2 constants, etc
#include <cmqpsc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define RFH2_USR_BEGIN	"<usr>"
#define RFH2_USR_END	"</usr>"

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+10

/////////////////////////////////////////////////////////////////////////////
// Usr dialog


Usr::Usr() :CPropertyPage(Usr::IDD)
{
	//{{AFX_DATA_INIT(Usr)
	m_rfh_usr_data = _T("");
	//}}AFX_DATA_INIT

	rfh_usr_area = NULL;
	m_RFH_usr_len = 0;
	m_rfh_usr_ccsid = -1;
	m_rfh_usr_encoding = -1;
	usrDataChanged = false;
	pDoc = NULL;
}

Usr::~Usr()
{
	freeUsrArea();
}

void Usr::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Usr)
	DDX_Text(pDX, IDC_RFH_USR_DATA, m_rfh_usr_data);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Usr, CPropertyPage)
	//{{AFX_MSG_MAP(Usr)
	ON_EN_CHANGE(IDC_RFH_USR_DATA, OnChangeRfhUsrData)
	ON_EN_SETFOCUS(IDC_RFH_USR_DATA, OnSetfocusRfhUsrData)
	ON_BN_CLICKED(IDC_RFH_FROM_USER, OnRfhFromUser)
	ON_EN_UPDATE(IDC_RFH_USR_DATA, OnUpdateRfhUsrData)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, Usr::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, Usr::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Usr message handlers

BOOL Usr::OnSetActive() 

{
	// The dialog is about to receive the focus
	// make sure the dialog display reflects the current instance variables
	if (pDoc->traceEnabled)
	{
		// trace entry to OnSetActive
		pDoc->logTraceEntry("Entering Usr::OnSetActive()");
	}

	usrDataChanged = false;

	updatePageData();
	
	// send a custom message to switch the focus to the edit box control
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return CPropertyPage::OnSetActive();
}

BOOL Usr::OnKillActive() 

{
	// The dialog is about to lose the focus
	// make sure the instance variables are up to date
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering Usr::OnKillActive()");
	}

	// get the form data into the instance variables
	UpdateData(TRUE);

	return CPropertyPage::OnKillActive();
}

void Usr::updatePageData()

{
	if (pDoc->traceEnabled)
	{
		// trace entry to updatePageData
		pDoc->logTraceEntry("Entering Usr::updatePageData()");
	}

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void Usr::OnChangeRfhUsrData() 
{
	// User has changed the data so invalidate the current RFH area
	usrDataChanged = true;

	// delete the current RFH and usr areas
	freeUsrArea();
	pDoc->freeRfhArea();

	// get the form data into the instance variables
	UpdateData(TRUE);
}

void Usr::OnSetfocusRfhUsrData() 
{
	// remove the automatic selection of the data
	((CEdit *)GetDlgItem(IDC_RFH_USR_DATA))->SetSel(-1);
}

BOOL Usr::OnInitDialog() 

{
	CPropertyPage::OnInitDialog();
	
	// enable tool tips for this dialog
	EnableToolTips(TRUE);
	
	// use the special MyEdit subclass for the usr data edit box control
	m_DataEditBox.SubclassDlgItem(IDC_RFH_USR_DATA, this);

	return TRUE;  // return TRUE unless you set the focus to a control	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL Usr::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pTTTStruct;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pTTTStruct;
	CString strTipText;

	UINT nID = pTTTStruct->idFrom;

	if (((pTTTStruct->code == TTN_NEEDTEXTA) && (pTTTA->uFlags & TTF_IDISHWND)) ||
		((pTTTStruct->code == TTN_NEEDTEXTW) && (pTTTW->uFlags & TTF_IDISHWND)))
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

			pResult = 0;
			return(TRUE);
		}
	}

	return(FALSE);
}

void Usr::getSelection(int &firstChar, int &lastChar)

{
	// initialize the variables in case the call does not work
	firstChar = -1;
	lastChar = -1;

	// get the current selection
	((CEdit *)GetDlgItem(IDC_RFH_USR_DATA))->GetSel(firstChar, lastChar);
}

///////////////////////////////////////////////////////
//
// Routine to parse the usr folder in an RFH2 header.
// This folder is used for miscellaneous fields that
// do not fit in one of the other architected folders.
// It is used by JMS to carry user properties, for 
// example.
//
// Each individual tag will be placed on a separate
// line as a name/value pair in the format of 
// name=value.
//
///////////////////////////////////////////////////////

void Usr::parseRFH2usr(unsigned char *rfhdata, int dataLen, int ccsid, int encoding)

{
	int			length=dataLen;
	int			result;
	int			i;					// looping work variable
	char		*ptr;
	const char	*errMsg;
	char		*dataStart;
	char		*dataEnd;
	char		*tempData=NULL;		// pointer to allocated storage to hold usr area
	CXMLParse	*xml;				// XML parser object
	char		errtxt[256];		// work area to build error message
	char		traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering Usr::parseRFH2usr() rfhdata=%8.8X (%.32s) dataLen=%d ccsid=%d encoding=%d", (unsigned int)rfhdata, rfhdata, dataLen, ccsid, encoding);

		// trace entry to parseRFH2usr
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("usr rfhdata", rfhdata, dataLen);
	}

	// clear the current contents of the edit box
	m_rfh_usr_data.Empty();
	
	// find the end of the data 
	while ((length > 0) && (rfhdata[length - 1] != '>'))
	{
		length--;
	}

	// calculate the end of the data
	dataEnd = (char *)rfhdata + length;

	// search for the usr tag in the data
	dataStart = (char *)rfhdata;
	while ((dataStart < dataEnd) && (dataStart[0] != '<'))
	{
		dataStart++;
		length--;
	}

	// allocate storage for a usr area that can be modified
	tempData = (char *)rfhMalloc(dataEnd - dataStart + 16, "USR_TEMP");

	// did the malloc work?
	if (NULL == tempData)
	{
		// error parsing usr area
		pDoc->m_error_msg = "Error parsing usr area - memory allocation failed";
		pDoc->updateMsgText();

		if (pDoc->traceEnabled)
		{
			// explain the error
			sprintf(traceInfo, "Memory allocation USR_TEMP failed - %d", dataEnd - dataStart + 16);

			// trace exit from parseRFH2usr
			pDoc->logTraceEntry(traceInfo);
		}

		return;
	}

	// clear the allocated data
	memset(tempData, 0, dataEnd - dataStart + 16);

	// copy the data to the temporary data
	// replace any CR or LF or binary zero characters in the data
	for (i=0; i < dataEnd - dataStart; i++)
	{
		if ((dataStart[i] != '\n') && (dataStart[i] != '\r') && (dataStart[i] != 0))
		{
			tempData[i] = dataStart[i];
		}
		else
		{
			tempData[i] = ' ';
		}
	}

	// create an XML parser object to parse the input
	xml = new CXMLParse();

	if (NULL == xml)
	{
		// tell the user about the error
		pDoc->m_error_msg = "Error creating usr parser object";

		if (pDoc->traceEnabled)
		{
			// trace exit from parseRFH2usr
			pDoc->logTraceEntry(" Unable to create parser object");
		}

		// release the memory that was acquired
		rfhFree(tempData);

		return;
	}

	// try to parse the input data
	result = xml->parse(tempData, length);

	// release the memory that was acquired
	rfhFree(tempData);

	// check if the parse worked
	if (result != PARSE_OK)
	{
		// error parsing usr area
		// create and display an error message
		errMsg = xml->getErrorMsg(result);
		sprintf(errtxt, "XML parse error in usr area - %s at offset %d", errMsg, xml->lastErrorOffset);
		pDoc->m_error_msg = errtxt;
		pDoc->updateMsgText();

		if (pDoc->traceEnabled)
		{
			// explain the error
			sprintf(traceInfo, "**Error parsing usr area - %d", result);

			// trace exit from parseRFH2usr
			pDoc->logTraceEntry(traceInfo);

			// dump out the usr area
			pDoc->dumpTraceData("RFH Usr Data", (unsigned char *)rfhdata, length);
		}

		// delete the xml object
		delete(xml);

		return;
	}

	// allocate storage for a temporary data to parse the data into
	ptr = (char *)rfhMalloc(length * 16 + 8192, "USRPTR  ");

	// make sure the malloc worked
	if (ptr != NULL)
	{
		// tell the user what happened
		// create the parsed area - do not include the root name (usr) in the output
		xml->buildParsedArea(ptr, length * 16 + 8000, FALSE);

		// update the usr data display
		m_rfh_usr_data = ptr;

		// free the acquired storage
		rfhFree(ptr);
	}
	else
	{
		// error parsing usr area
		pDoc->m_error_msg = "Error parsing usr area";
		pDoc->updateMsgText();

		if (pDoc->traceEnabled)
		{
			// explain the error
			sprintf(traceInfo, "Memory allocation failed - %d", length * 16 + 8192);

			// trace exit from parseRFH2usr
			pDoc->logTraceEntry(traceInfo);
		}
	}

	// delete the parser object
	delete(xml);
	
	// update the form data from the instance variables
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting parseRFH2usr - length=%d", length);

		// trace exit from parseRFH2usr
		pDoc->logTraceEntry(traceInfo);
	}
}

void Usr::setUsrArea(unsigned char *usrData, int dataLen, int ccsid, int encoding)

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering Usr::setUsrArea() rfh_usr_area=%8.8X m_RFH_usr_len=%d m_rfh_usr_ccsid=%d m_rfh_usr_encoding=%d", (unsigned int)rfh_usr_area, m_RFH_usr_len, m_rfh_usr_ccsid, m_rfh_usr_encoding);

		// trace entry to setUsrArea
		pDoc->logTraceEntry(traceInfo);
	}

	// free any old area
	freeUsrArea();

	if ((usrData != NULL) && (dataLen > 0))
	{
		// allocate storage for the usr folder
		rfh_usr_area = (unsigned char *)rfhMalloc(dataLen + 5, "USRAREA ");		// allow for terminating characters

		// copy the data to the allocated area and terminate it as a safeguard
		memcpy(rfh_usr_area, usrData, dataLen);
		rfh_usr_area[dataLen] = 0;
		rfh_usr_area[dataLen + 1] = 0;		// in case of UCS-2 data

		// remember the length of the usr folder
		m_RFH_usr_len = dataLen;

		// remember the ccsid and encoding that were used to build this folder
		m_rfh_usr_ccsid = ccsid;
		m_rfh_usr_encoding = encoding;
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting Usr::setUsrArea() rfh_usr_area=%8.8X m_RFH_usr_len=%d m_rfh_usr_ccsid=%d m_rfh_usr_encoding=%d", (unsigned int)rfh_usr_area, m_RFH_usr_len, m_rfh_usr_ccsid, m_rfh_usr_encoding);

		// trace exit from setUsrArea
		pDoc->logTraceEntry(traceInfo);
	}
}

void Usr::freeUsrArea()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering Usr::freeUsrArea() rfh_usr_area=%8.8X m_RFH_usr_len=%d", (unsigned int)rfh_usr_area, m_RFH_usr_len);

		// trace entry to freeUsrArea
		pDoc->logTraceEntry(traceInfo);
	}

	if (rfh_usr_area != NULL)
	{
		rfhFree(rfh_usr_area);
	}
		
	rfh_usr_area = NULL;
	m_RFH_usr_len = 0;
	m_rfh_usr_ccsid = -1;
	m_rfh_usr_encoding = -1;
}

int Usr::buildUsrArea(int ccsid, int encoding)

{
	int				i;
	int				j;
	int				maxLen;
	wchar_t			*ucsPtr;
	unsigned char	*newArea;
	unsigned char	*tempPtr;
	CXMLParse		*xml;				// XML parser object
	char			traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering Usr::buildUsrArea() rfh_usr_area=%8.8X m_RFH_usr_len=%d ccsid=%d encoding=%d", (unsigned int)rfh_usr_area, m_RFH_usr_len, ccsid, encoding);

		// trace entry to buildUsrArea
		pDoc->logTraceEntry(traceInfo);
	}

	// check if we already have the usr area built
	if ((rfh_usr_area != NULL) && (ccsid == m_rfh_usr_ccsid) && (encoding == m_rfh_usr_encoding))
	{
		// already built - just return the current value
		return m_RFH_usr_len;
	}

	// check if the first character is a begin bracket
	if (m_rfh_usr_data[0] != '<')
	{
		// have to build the usr area
		// create an XML parser object to parse the input
		xml = new CXMLParse();

		if (NULL == xml)
		{
			// tell the user about the error
			pDoc->m_error_msg = "Error creating usr parser object";

			if (pDoc->traceEnabled)
			{
				// trace exit from parseRFH2usr
				pDoc->logTraceEntry(" Unable to create parser object");
			}

			return 0;
		}

		// create a parse tree from the input data
		xml->parseFormattedText((LPCTSTR)m_rfh_usr_data, "usr");

		// allocate storage, including allowing for the fact that escape sequences may be needed
		// and names appear twice as beginning and ending tags
		maxLen = 6*m_rfh_usr_data.GetLength() + 4096;
		newArea = (unsigned char *)rfhMalloc(maxLen, "USRNEWA ");

		// make sure the malloc worked
		if (NULL == newArea)
		{
			// try to tell the user about the erro
			pDoc->m_error_msg = "memory allocation failed in buildUsrArea";
			pDoc->updateMsgText();

			// delete the XML parser object
			delete(xml);

			// malloc failed - try to be graceful
			return 0;
		}

		// now build the XML output
		xml->createXML((char *)newArea, maxLen - 1);

		// check if the data must be converted to UCS-2
		if (isUCS2(ccsid))
		{
			// convert the data to UCS-2
			// allocate a temporary area to hold the UCS-2 data
			tempPtr = (unsigned char *)rfhMalloc(2 * strlen((char *)newArea) + 16, "USRTPTR ");

			if (tempPtr != 0)
			{
				// translate the data to UCS-2
				MultiByteToUCS2(tempPtr, 2 * strlen((char *)newArea) + 2, newArea, strlen((char *)newArea));

				// set the length of the new mcd in bytes (each 16-bit character is 2 bytes)
				m_RFH_usr_len = roundLength2(tempPtr, NUMERIC_PC) * 2;

				// check if the data has to be reversed
				if (encoding != NUMERIC_PC)
				{
					// reverse the order of the bytes
					i = 0;
					j = m_RFH_usr_len >> 1;
					ucsPtr = (wchar_t *)tempPtr;
					while (i < j)
					{
						// reverse the bytes in the UCS-2 character
						short s = ucsPtr[i];
						s = reverseBytes(&s);
						ucsPtr[i] = s;

						// move on to the next character
						i++;
					}
				}

				// save the results
				setUsrArea(tempPtr, m_RFH_usr_len, ccsid, encoding);

				// release the storage we acquired
				rfhFree(tempPtr);
			}
			else
			{
				if (pDoc->traceEnabled)
				{
					// malloc failed - write entry to trace
					pDoc->logTraceEntry("Usr::buildUsrArea() malloc failed");
				}
			}
		}
		else
		{
			// round the area up to a multiple of 4 characters
			roundLength((unsigned char *)newArea);

			// set the rfh usr area pointer
			setUsrArea(newArea, strlen((char *)newArea), ccsid, encoding);
		}

		// delete the XML parser object
		delete(xml);
	}
	else
	{
		// must be old format
		// assume that the area contains valid XML
		// allocate storage
		newArea = (unsigned char *)rfhMalloc(m_rfh_usr_data.GetLength() + 64, "USRNEWA2");

		// create the output
		strcpy((char *)newArea, RFH2_USR_BEGIN);
		strcpy((char *)newArea + sizeof(RFH2_USR_BEGIN) - 1, (LPCTSTR)m_rfh_usr_data);
		strcpy((char *)newArea + m_rfh_usr_data.GetLength() + sizeof(RFH2_USR_BEGIN) - 1, RFH2_USR_END);
			
		// round the area up to a multiple of 4 characters
		roundLength((unsigned char *)newArea);

		// set the rfh usr area pointer
		setUsrArea(newArea, strlen((char *)newArea), ccsid, encoding);
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting Usr::buildUsrArea() rfh_usr_area=%8.8X m_RFH_usr_len=%d", (unsigned int)rfh_usr_area, m_RFH_usr_len);

		// trace exit from buildUsrArea
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("usr data", rfh_usr_area, m_RFH_usr_len);
	}

	return m_RFH_usr_len;
}

const char * Usr::getUsrArea()

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting Usr::getUsrArea() rfh_usr_area=%8.8X m_RFH_usr_len=%d", (unsigned int)rfh_usr_area, m_RFH_usr_len);

		// trace exit from buildUsrArea
		pDoc->logTraceEntry(traceInfo);
	}

	return (char *)rfh_usr_area;
}

BOOL Usr::wasDataChanged()

{
	return usrDataChanged;
}

void Usr::clearUsrData()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering Usr::clearUsrData() m_rfh_usr_data.GetLength()=%d", m_rfh_usr_data.GetLength());

		// trace entry to clearUsrData
		pDoc->logTraceEntry(traceInfo);
	}

	m_rfh_usr_data.Empty();
	freeUsrArea();

	// get the instance variables into the form data
	UpdateData(FALSE);
}

LONG Usr::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the queue manager name combo box
	m_DataEditBox.SetFocus();

	return 0;
}

void Usr::OnRfhFromUser() 

{
	CString	tempData;
	char	traceInfo[640];		// work variable to build trace message

	// get the form data into the instance variables
	UpdateData(TRUE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering Usr::OnRfhFromUser() m_rfh_usr_data.GetLength()=%d data=%.512s", m_rfh_usr_data.GetLength(), (LPCTSTR)m_rfh_usr_data);

		// trace entry to OnRfhFromUser
		pDoc->logTraceEntry(traceInfo);
	}

	// get the user property data
	pDoc->getUserPropertyData(tempData);

	// were there any user properties?
	if (tempData.GetLength() > 0)
	{
		// check if a CRLF sequence is needed
		if (m_rfh_usr_data.GetLength() > 0)
		{
			m_rfh_usr_data += "\r\n";
		}

		// append the user properties
		m_rfh_usr_data += (LPCTSTR)tempData;

		// clear the user properties
		pDoc->clearUserProperties();

		// make sure RFH2 header and usr folder are selected
		pDoc->selectUsrFolder();
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting Usr::OnRfhFromUser() m_rfh_usr_data.GetLength()=%d data=%.512s", m_rfh_usr_data.GetLength(), (LPCTSTR)m_rfh_usr_data);

		// trace exit from OnRfhFromUser
		pDoc->logTraceEntry(traceInfo);
	}

	// update the form data from the instance variables
	UpdateData(FALSE);
}

void Usr::getUsrData(CString &data)

{
	// get the form data into the instance variables
	UpdateData(TRUE);

	data = (LPCTSTR)m_rfh_usr_data;
}

////////////////////////////////////////////
//
// MQ user properties can be passed in the
// usr folder in an RFH2 header.  This 
// routine will search the usr folder for
// a property with a name of MQIsRetained.
//
////////////////////////////////////////////

void Usr::checkForRetained(BOOL *isRetained)

{
	// set the is retained value to false
	(*isRetained) = FALSE;

	// check if there is actually user data
	if (m_rfh_usr_data.GetLength() > 0)
	{
		// search for the string
		int ofs = m_rfh_usr_data.Find("MQIsRetained(dt=\"boolean\")=1");
		int ofs2 = m_rfh_usr_data.Find("MQIsRetained(dt=\"bool\")=1");

		if ((ofs >= 0) || (ofs2 > 0))
		{
			// found the MQIsRetained property
			(*isRetained) = TRUE;
		}
	}
}

void Usr::OnUpdateRfhUsrData() 

{
	// indicate data was changed
	usrDataChanged = true;
}
