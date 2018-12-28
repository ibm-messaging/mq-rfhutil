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

// other.cpp : implementation file
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "other.h"
#include "comsubs.h"
#include "XMLParse.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+11

/////////////////////////////////////////////////////////////////////////////
// other property page

IMPLEMENT_DYNCREATE(other, CPropertyPage)

other::other() : CPropertyPage(other::IDD)
{
	//{{AFX_DATA_INIT(other)
	m_rfh_other_data = _T("");
	//}}AFX_DATA_INIT

	rfh_other_area = NULL;
	m_RFH_other_len = 0;
	otherCcsid = 0;
	otherEncoding = 0;
	otherDataChanged = false;
}

other::~other()
{
	freeOtherArea();
}

void other::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(other)
	DDX_Text(pDX, IDC_OTHER_RFH_DATA, m_rfh_other_data);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(other, CPropertyPage)
	//{{AFX_MSG_MAP(other)
	ON_EN_CHANGE(IDC_OTHER_RFH_DATA, OnChangeOtherRfhData)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, other::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, other::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// other message handlers

void other::OnChangeOtherRfhData() 
{
	// release the RFH other area and the RFH area, since they have been changed
	freeOtherArea();
	pDoc->freeRfhArea();
	otherDataChanged = true;

	// get the updated form data into the instance variables
	UpdateData(TRUE);
}

BOOL other::OnSetActive() 

{
	// get the form data into the instance variables
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering other::OnSetActive() m_rfh_other_data.GetLength()=%d", m_rfh_other_data.GetLength());

		// trace entry to OnSetActive
		pDoc->logTraceEntry(traceInfo);
	}

	// get the instance variables into the form data
	UpdateData(FALSE);

	// send a custom message to switch the focus to the edit box control
	// this allows the tab keys to work
	PostMessage(WM_SETPAGEFOCUS, 0, 0L);

	return CPropertyPage::OnSetActive();
}

BOOL other::OnKillActive() 

{
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering other::OnKillActive()");
	}

	// get the data from the controls into the instance variables
	UpdateData(TRUE);

	return CPropertyPage::OnKillActive();
}

BOOL other::OnInitDialog() 

{
	CPropertyPage::OnInitDialog();
	
	// tool tips are provided and must be initialized
	EnableToolTips(TRUE);
		
	// use the special MyEdit subclass for the usr data edit box control
	m_OtherEditBox.SubclassDlgItem(IDC_OTHER_RFH_DATA, this);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL other::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

void other::getSelection(int &firstChar, int &lastChar)

{
	firstChar = -1;
	lastChar = -1;

	((CEdit *)GetDlgItem(IDC_OTHER_RFH_DATA))->GetSel(firstChar, lastChar);
}

void other::updatePageData()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering other::updatePageData() rfh_other_area=%8.8X m_RFH_other_len=%d", (unsigned int)rfh_other_area, m_RFH_other_len);

		// trace entry to updatePageData
		pDoc->logTraceEntry(traceInfo);
	}

	otherDataChanged = false;

	// set the form data from the instance variables
	UpdateData(FALSE);
}

void other::freeOtherArea()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering other::freeOtherArea() rfh_other_area=%8.8X m_RFH_other_len=%d", (unsigned int)rfh_other_area, m_RFH_other_len);

		// trace entry to freeOtherArea
		pDoc->logTraceEntry(traceInfo);
	}

	if (rfh_other_area != NULL)
	{
		rfhFree(rfh_other_area);
	}
		
	rfh_other_area = NULL;
	m_RFH_other_len = 0;
}


///////////////////////////////////////////////////////
//
// Routine to parse the folders in an RFH2 header
// that are not recognized.
//
///////////////////////////////////////////////////////

void other::parseRFH2other(unsigned char *rfhdata, int dataLen, int ccsid)

{
	int				length=dataLen;
	int				result;
	char			*ptr;
	char			*dataStart;
	char			*dataEnd;
	CXMLParse		*xml;				// XML parser object
	char			traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering other::parseRFH2other() rfhdata=%8.8X dataLen=%d m_rfh_other_data.GetLength()=%d", (unsigned int)rfhdata, dataLen, m_rfh_other_data.GetLength());

		// trace entry to parseRFH2other
		pDoc->logTraceEntry(traceInfo);
	}

	// check if a copy of the input should be saved
	if (rfh_other_area != NULL)
	{
		// this must be another unrecognized folder
		// make sure that the other area is not saved, since the area consists of multiple areas
		freeOtherArea();
	}
	else if (m_rfh_other_data.GetLength() == 0)
	{
		// this is the first unrecognized folder - it can be saved safely
		// if another unrecognized folder is found then this will be freed above
		// allocate storage
		rfh_other_area = (unsigned char *)rfhMalloc(dataLen + 1, "OTHAREA ");

		// check if it worked
		if (rfh_other_area != NULL)
		{
			// copy the storage and add a terminating character
			memcpy(rfh_other_area, rfhdata, dataLen);
			rfh_other_area[dataLen] = 0;
		}
	}

	// find the end of the data 
	while ((length > 0) && (rfhdata[length - 1] != '>'))
	{
		length--;
	}

	// trim the input data
	Rtrim((char *)rfhdata);

	// calculate the end of the data
	dataEnd = (char *)rfhdata + length;

	// search for the usr tag in the data
	dataStart = (char *)rfhdata;
	while ((dataStart < dataEnd) && (dataStart[0] != '<'))
	{
		dataStart++;
		length--;
	}

	// create an XML parser object to parse the input
	xml = new CXMLParse();

	if (NULL == xml)
	{
		// tell the user about the error
		pDoc->m_error_msg = "Error creating parser object in parseOther";

		if (pDoc->traceEnabled)
		{
			// trace exit from parseRFH2usr
			pDoc->logTraceEntry(" Unable to create parser object in parseRFH2other");
		}

		return;
	}

	// try to parse the input data
	result = xml->parse(dataStart, length);

	// check if the parse worked
	if (result != PARSE_OK)
	{
		// error parsing rfh2 folder
		pDoc->m_error_msg.Format("Error parsing RFH2 folder offset=%d", xml->lastErrorOffset);
		pDoc->updateMsgText();

		if (pDoc->traceEnabled)
		{
			// explain the error
			sprintf(traceInfo, "Error parsing RFH2 folder - %d offset=%d", result, xml->lastErrorOffset);

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
	ptr = (char *)rfhMalloc(length * 16 + 8192, "OTHAREA2");

	// make sure the malloc worked
	if (ptr != NULL)
	{
		// tell the user what happened
		// create the parsed area - include the root name in the output since this is not one of the recognized folders
		xml->buildParsedArea(ptr, length * 16 + 8000, TRUE);

		// check if this is not the first folder
		if (m_rfh_other_data.GetLength() > 0)
		{
			// put the folder on the next line of the edit control
			m_rfh_other_data += "\r\n";
		}

		// update the usr data display
		m_rfh_other_data += ptr;

		// free the acquired storage
		rfhFree(ptr);
	}
	else
	{
		// error - memory allocation failed
		pDoc->m_error_msg = "Memory allocation failed";

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
	
	// get the instance variables into the form
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting other::parseRFH2other() m_rfh_other_data.GetLength()=%d", m_rfh_other_data.GetLength());

		// trace exit from parseRFH2other
		pDoc->logTraceEntry(traceInfo);
	}
}

BOOL other::wasDataChanged()

{
	return otherDataChanged;
}

char * other::findEndOfLine(char *inPtr, char *endPtr)

{
	// look for a CR or LF or the end of the string
	while ((inPtr < endPtr) && (inPtr[0] != 0) && (inPtr[0] != '\r') && (inPtr[0] != '\r'))
	{
		// move on to the next character
		inPtr++;
	}

	return inPtr;
}

int other::processFolder(char *input, int length, char *output, int maxLen, int ccsid, int encoding, BOOL ucs)

{
	int				i;					// working variable
	int				len=0;				// length of UCS-2 data
	int				result;
	int				charCount=0;
	char			*tempPtr;
	char			*allocPtr=NULL;
	wchar_t			*ucsptr;
	CXMLParse		*xml;				// XML parser object
	char			traceInfo[512];		// work variable to build trace message
	char			tempData[2048];

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering other::processFolder() length=%d maxLen=%d ucs=%d input=%.256s", length, maxLen, ucs, input);

		// trace entry to processFolder
		pDoc->logTraceEntry(traceInfo);
	}

	// create an XML parser object to parse the input
	xml = new CXMLParse();

	if (NULL == xml)
	{
		// tell the user about the error
		pDoc->m_error_msg = "Error creating parser object in processFolder";

		if (pDoc->traceEnabled)
		{
			// trace exit from parseRFH2usr
			pDoc->logTraceEntry(" Unable to create parser object in other::processFolder");
		}

		return 0;
	}

	// try to parse the input data
	result = xml->parseRootText(input, length);

	// check if the parse worked
	if (result != PARSE_OK)
	{
		// error parsing usr area
		pDoc->m_error_msg.Format("Error parsing other folder (%d) offset=%d", result, xml->lastErrorOffset);
		pDoc->updateMsgText();

		if (pDoc->traceEnabled)
		{
			// explain the error
			sprintf(traceInfo, "Error parsing other folder - %d offset=%d", result, xml->lastErrorOffset);

			// trace exit from parseRFH2usr
			pDoc->logTraceEntry(traceInfo);

			// dump out the usr area
			pDoc->dumpTraceData("RFH Other Data", (unsigned char *)input, length);
		}

		// delete the xml object
		delete(xml);

		return 0;
	}

	// now create the XML from the parse tree
	result = xml->createXML(output, maxLen);

	// check the result
	if (PARSE_OK == result)
	{
		// the output is built
		// is the output supposed to be ucs-2?
		if (ucs)
		{
			// convert the result to UCS-2 characters
			// check if the result will fit (UCS-2 requires 2 bytes per character)
			len = strlen(output);
			if (len << 1 < maxLen)
			{
				// allocate a workarea for the ASCII characters
				// will the data fit in 2048 bytes?
				if (len < sizeof(tempData) - 1)
				{
					// point to the workarea on the stack
					tempPtr = tempData;
				}
				else
				{
					// too big - have to use malloc
					tempPtr = (char *)rfhMalloc(len + 1, "OTHTMPTR");
					allocPtr = tempPtr;
				}

				// make sure the pointer is valid
				if (tempPtr != NULL)
				{
					// copy the characters to the temporary area
					memcpy(tempPtr, output, len);
					tempPtr[len] = 0;

					// translate the current characters to UCS2
					// the -1 indicates a null terminated string
					charCount = MultiByteToWideChar(CP_OEMCP, (DWORD)0, tempPtr, -1, (LPWSTR)output, maxLen >> 1);

					// check if an area was allocated
					if (allocPtr != NULL)
					{
						// release the acquired storage
						rfhFree(allocPtr);
					}

					// check for big-endian encoding
					if (encoding != NUMERIC_PC)
					{
						// reverse the order of the characters
						ucsptr = (wchar_t *)output;
						for (i=0; i < charCount; i++)
						{
							ucsptr[i] = reverseBytes((short *)(ucsptr + i));
						}
					}

					// get the number of bytes in the output
					len = roundLength2((unsigned char *)output, encoding);

					// change the length to bytes from UCS characters
					len <<= 1;
				}

			}
		}
		else
		{
			// round the length to a multiple of 4
			len = roundLength((unsigned char *)output);
		}
	}

	// delete the xml object
	delete(xml);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting other::processFolder() len=%d charCount=%d allocPtr=%8.8X", len, charCount, (unsigned int)allocPtr);

		// trace exit from processFolder
		pDoc->logTraceEntry(traceInfo);

		// write the folder data to the trace file
		pDoc->dumpTraceData("folder data", (unsigned char *)output, len);
	}

	// return the number of bytes in the folder
	return	len;
}

////////////////////////////////////////////////////////
//
// This routine will render the display in the edit
// control as XML.  Each line contains a name and
// value pair, with the folder name followed by the
// tag name and separated by a period.  Attributes
// will be preceded by a (XML.attr) sequence.  If a
// tag occurs more than once then the later occurences
// will include the index number enclosed in square
// brackets.
//
// The attributes (if any) should be displayed before
// the children.
//
////////////////////////////////////////////////////////

int other::buildOtherArea(int ccsid, int encoding)

{
	int		i;
	int		len;
	int		allocLen;
	int		lines=1;
	int		folderLen;
	int		inputLen;
	int		tempLen;
	int		folderNameLen;		// length of the folder name
	char	*lenPtr;
	char	*ptr;
	char	*folderName;		// pointer to folder name
	char	*input;				// pointer to copy of contents of the CEdit control
	char	*inPtr;
	char	*endLine;			// end of current line
	char	*endPtr;			// end of input area
	char	*endOutput;			// end of output area
	wchar_t	*ucsptr;
	BOOL	ucs;
	BOOL	sameFolder;
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering other::buildOtherArea() rfh_other_area=%8.8X m_RFH_other_len=%d", (unsigned int)rfh_other_area, m_RFH_other_len);

		// trace entry to buildOtherArea
		pDoc->logTraceEntry(traceInfo);
	}

	// check if we already have the usr area built
	if ((rfh_other_area != NULL) && (ccsid == otherCcsid) && (encoding == otherEncoding))
	{
		// already built - just return the current value
		return m_RFH_other_len;
	}

	if (rfh_other_area != NULL)
	{
		// free the current area
		freeOtherArea();
	}

	// build the other area
	// unlike the other rfh areas, this area will include the length fields, since there may be several distinct
	// folders together in the other area
	// start by calculating the total length of the area
	// The total length will be the length of the data in the edit control plus 2 extra bytes for each CR/LF sequence
	// In order to simplify the calulation the area that will be allocated will be twice the length of the string.
	len = m_rfh_other_data.GetLength();

	// calculate the number of lines in the data
	// each line represents a different variable and possibly a different folder
	i = 0;
	while ((i < len) && (i != -1))
	{
		// find the next new line character
		i = m_rfh_other_data.Find('\n', i);

		// was a new line character found?
		if (i != -1)
		{
			// count the number of entries
			lines++;

			// move on to the next character position
			i++;
		}
	}

	// allow for UCS-2 characters and for 4 byte length field plus three rounding characters per folder
	allocLen = (len * 4) + (6 * lines) + 16;
	rfh_other_area = (unsigned char *)rfhMalloc(allocLen, "RFHOTHER");
	endOutput = (char *)rfh_other_area + allocLen;

	// make a copy of the contents of the CString
	inputLen = m_rfh_other_data.GetLength();
	input = (char *)rfhMalloc(inputLen + 2, "OTHINPUT");
	endPtr = (char *)input + inputLen;

	// check if the malloc worked
	if ((rfh_other_area != NULL) && (input != NULL))
	{
		// initialize the area to nulls
		memset(rfh_other_area, 0, allocLen);
		memcpy(input, (LPCTSTR)m_rfh_other_data, inputLen);
		input[inputLen] = 0;

		// remember the code page and encoding that was used to build this area
		otherCcsid = ccsid;
		otherEncoding = encoding;

		// is the code page UCS-2 (2 bytes per character)?
		ucs = isUCS2(ccsid);

		// initialize some variables
		lenPtr = (char *)rfh_other_area;		// length field in output
		ptr = (char *)rfh_other_area + 4;		// output pointer
		ucsptr = (wchar_t *)ptr;				// ucs-2 output pointer
		m_RFH_other_len = 0;					// total length of rfh_other_area

		// now start to move the data into the data area
		i = 0;
		folderLen = 0;

		// find the first root name
		inPtr = skipBlanks(input);

		// process the folders
		while (inPtr < endPtr)
		{
			// find the end of the folderName
			folderName = inPtr;
			while ((inPtr < endPtr) && (inPtr[0] != 0) && (inPtr[0] > ' ') && (inPtr[0] != '.') && (inPtr[0] != '=') && (inPtr[0] != '\r') && (inPtr[0] != '\n'))
			{
				// skip the folder name
				inPtr++;
			}

			// calculate the length
			folderNameLen = inPtr - folderName;

			// look for a CR or LF character or a binary zero
			inPtr = findEndOfLine(inPtr, endPtr);
			
			// find the end of the line
			endLine = inPtr;

			// find the rest of the lines in this folder
			// this involves finding the beginning of the next line
			// and checking if the folder name is the same
			sameFolder = TRUE;
			while ((inPtr < endPtr) && (inPtr[0] != 0) && sameFolder)
			{
				// find the beginning of the next line
				while ((inPtr < endPtr) && (inPtr[0] != 0) && (inPtr[0] <= ' '))
				{
					// skip the blank or CR/LF character
					inPtr++;
				}

				// check if another line was found
				if ((inPtr < endPtr) && (inPtr[0] != 0))
				{
					// check if the folder name is the same
					if ((memcmp(inPtr, folderName, folderNameLen) == 0) && ('.' == inPtr[folderNameLen]))
					{
						// skip past the folder name
						inPtr += folderNameLen + 1;

						// find the end of the line and continue the loop
						// look for a CR or LF character or a binary zero
						inPtr = findEndOfLine(inPtr, endPtr);

						// reset the end line pointer
						endLine = inPtr;
					}
					else
					{
						// drop out of the loop
						sameFolder = FALSE;

						// terminate the string for the XML parser
						endLine[0] = 0;
					}
				}
			}

			folderLen = processFolder(folderName, inPtr - folderName, ptr, endOutput - ptr, ccsid, encoding, ucs);

			// set the length
			// insert the length of the current folder using the proper encoding
			if (NUMERIC_PC == encoding)
			{
				// get the length in front of the folder
				memcpy(lenPtr, &folderLen, 4);
			}
			else
			{
				// reverse the bytes and then copy the result
				tempLen = reverseBytes4(folderLen);
				memcpy(lenPtr, &tempLen, 4);
			}

			// advance the output pointers 
			lenPtr += folderLen + 4;
			ptr += folderLen + 4;
			m_RFH_other_len += folderLen + 4;
		}
	}

	// check if the input area was allocated
	if (input != NULL)
	{
		// release the storage
		rfhFree(input);
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting other::buildOtherArea() rfh_other_area=%8.8X m_RFH_other_len=%d allocLen=%d", (unsigned int)rfh_other_area, m_RFH_other_len, allocLen);

		// trace exit from buildOtherArea
		pDoc->logTraceEntry(traceInfo);

		// write the data to the trace file
		pDoc->dumpTraceData("other data", rfh_other_area, m_RFH_other_len);
	}

	return m_RFH_other_len;
}


const char * other::getOtherArea()

{
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering other::getOtherArea() rfh_other_area=%8.8X m_RFH_other_len=%d", (unsigned int)rfh_other_area, m_RFH_other_len);

		// trace entry to getOtherArea
		pDoc->logTraceEntry(traceInfo);
	}

	return (char *)rfh_other_area;
}

void other::clearOtherData()

{
	char	traceInfo[512];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering other::clearOtherData() m_rfh_other_data.GetLength()=%d", m_rfh_other_data.GetLength());

		// trace entry to clearOtherData
		pDoc->logTraceEntry(traceInfo);
	}

	m_rfh_other_data.Empty();
	otherDataChanged = false;

	// get the instance variables into the form data
	UpdateData(FALSE);
}

LONG other::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the queue manager name combo box
	m_OtherEditBox.SetFocus();

	return 0;
}
