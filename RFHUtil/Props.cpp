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

// Props.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "Props.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+12

/////////////////////////////////////////////////////////////////////////////
// CProps property page

IMPLEMENT_DYNCREATE(CProps, CPropertyPage)

CProps::CProps() : CPropertyPage(CProps::IDD)
{
	//{{AFX_DATA_INIT(CProps)
	m_properties = _T("");
	//}}AFX_DATA_INIT
	
	propertyCount = 0;				// initialize the number of user properties
	xml = NULL;						// initialize XML object pointer
}

CProps::~CProps()
{
	// is there an XML parser object that was instantiated?
	if (xml != NULL)
	{
		// delete the object
		delete(xml);
	}
}

void CProps::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProps)
	DDX_Text(pDX, IDC_PROPS_USER_PROPERTIES, m_properties);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProps, CPropertyPage)
	//{{AFX_MSG_MAP(CProps)
	ON_EN_CHANGE(IDC_PROPS_USER_PROPERTIES, OnChangePropsUserProperties)
	ON_BN_CLICKED(IDC_PROPS_MOVE_RFH_USR, OnPropsMoveRfhUsr)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, CProps::GetToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, CProps::GetToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProps message handlers

BOOL CProps::OnKillActive() 

{
	if (pDoc->traceEnabled)
	{
		// trace entry to OnKillActive
		pDoc->logTraceEntry("Entering CProps::OnKillActive()");
	}

	// get the data from the controls into the instance variables
	UpdateData(TRUE);

	// get the number of lines in the CEDIT control
	propertyCount = m_PropertiesEditBox.GetLineCount();

	return CPropertyPage::OnKillActive();
}

void CProps::UpdatePageData()

{
	// get the form data into the instance variables
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps::UpdatePageData()");

		// trace entry to UpdatePageData
		pDoc->logTraceEntry(traceInfo);
	}
}

BOOL CProps::OnSetActive() 

{
	// get the form data into the instance variables
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps::OnSetActive() m_properties.GetLength()=%d", m_properties.GetLength());

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

BOOL CProps::OnInitDialog() 

{
	CPropertyPage::OnInitDialog();
	// tool tips are provided and must be initialized
	EnableToolTips(TRUE);
		
	// use the special MyEdit subclass for the usr data edit box control
	m_PropertiesEditBox.SubclassDlgItem(IDC_PROPS_USER_PROPERTIES, this);

	return TRUE;  // return TRUE unless you set the focus to a control
}

LONG CProps::OnSetPageFocus(UINT wParam, LONG lParam)

{
	// set the focus to the queue manager name combo box
	m_PropertiesEditBox.SetFocus();

	return 0;
}

BOOL CProps::GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult)

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

			pResult = 0;
			return(TRUE);
		}
	}

	return(FALSE);
}

void CProps::appendProperty(const char * name, const char * value, const int valueLen, int type)

{
	char	data[16];
	char	traceInfo[640];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps::appendProperty() type=%d valueLen=%d name=%.256s value=%.256s", type, valueLen, name, value);

		// trace entry to appendProperty
		pDoc->logTraceEntry(traceInfo);

		// check for verbose trace
		if ((pDoc->verboseTrace) && (valueLen > 256))
		{
			// dump out the value
			pDoc->dumpTraceData("value", (unsigned char *)value, valueLen);
		}
	}

	// check if there is a previous property
	if (m_properties.GetLength() > 0)
	{
		m_properties += "\r\n";
	}

	// append the property name and value
	m_properties += name;

	// check the property type
	switch(type)
	{
	case MQTYPE_STRING:
		{
			m_properties += "=";
			m_properties += value;
			break;
		}
	case MQTYPE_INT8:
		{
			m_properties += "(dt=\'i1\')=";
			m_properties += value;
			break;
		}
	case MQTYPE_INT16:
		{
			m_properties += "(dt=\'i2\')=";
			m_properties += value;
			break;
		}
	case MQTYPE_INT32:
		{
			m_properties += "(dt=\'i4\')=";
			m_properties += value;

			break;
		}
	case MQTYPE_INT64:
		{
			m_properties += "(dt=\'i8\')=";
			m_properties += value;
			break;
		}
	case MQTYPE_BOOLEAN:
		{
			m_properties += "(dt=\'boolean\')=";
			m_properties += value;
			break;
		}
	case MQTYPE_BYTE_STRING:
		{
			// append the data as hex characters
			m_properties += "(dt=\'bin.hex\')=";
			m_properties += value;
			break;
		}
	case MQTYPE_FLOAT32:
		{
			m_properties += "(dt=\'r4\')=";
			m_properties += value;
			break;
		}
	case MQTYPE_FLOAT64:
		{
			m_properties += "(dt=\'r8\')=";
			m_properties += value;
			break;
		}
	case MQTYPE_NULL:
		{
			m_properties += "(xsi:nil=\'true\')=";
			break;
		}
	default:
		{
			// don't know what this is
			m_properties += "(dt=";

			// get the data type as an integer value and append it
			sprintf(data, "%d", type);
			m_properties += data;

			// close the parentheses and add the equal sign
			m_properties += ")=";
			m_properties += value;

			break;
		}
	}

	// count the number of properties appended
	propertyCount++;

	// get the instance variables into the form data
	UpdateData(FALSE);
}

/*void CProps::appendProperty(const char * name, const char * value, const int valueLen, int type)

{
	float	fp4;
	double	fp8;
	char	data[32];
	char	traceInfo[640];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps::appendProperty() type=%d name=%.512s", type, name);

		// trace entry to appendProperty
		pDoc->logTraceEntry(traceInfo);

		// check for verbose trace
		if (pDoc->verboseTrace)
		{
			// dump out the value
			pDoc->dumpTraceData("value", (unsigned char *)value, valueLen);
		}
	}

	// check if there is a previous property
	if (m_properties.GetLength() > 0)
	{
		m_properties += "\r\n";
	}

	// append the property name and value
	m_properties += name;

	// check the property type
	switch(type)
	{
	case MQTYPE_STRING:
		{
			m_properties += "=";
			m_properties += value;
			break;
		}
	case MQTYPE_INT8:
		{
			m_properties += "(dt=\"byte\")=";
			sprintf(data, "%u", value[0]);
			m_properties += data;
			break;
		}
	case MQTYPE_INT16:
		{
			m_properties += "(dt=\"short\")=";
			sprintf(data, "%u", (*(PMQINT16)value));
			m_properties += data;
			break;
		}
	case MQTYPE_INT32:
		{
			m_properties += "(dt=\"int\")=";
			sprintf(data, "%d", (*(PMQLONG)value));
			m_properties += data;
			break;
		}
	case MQTYPE_INT64:
		{
			m_properties += "(dt=\"I64\")=";
			sprintf(data, "%I64d", (*(PMQINT64)value));
			m_properties += data;
			break;
		}
	case MQTYPE_BOOLEAN:
		{
			m_properties += "(dt=\"boolean\")=";

			if (*(PMQBOOL)value)
			{
				m_properties += "1";
			}
			else
			{
				m_properties += "0";
			}

			break;
		}
	case MQTYPE_BYTE_STRING:
		{
			m_properties += "(dt=\"hex\")=X\'";

			// append the data as hex characters
			memset(data, 0, sizeof(data));
			for (int i=0; i < valueLen; i++)
			{
				// render the data as hex characters a byte at a time
				AsciiToHex((unsigned char *)value + i, 1, (unsigned char *)data);
				m_properties += data;
			}

			m_properties += "'";;
			break;
		}
	case MQTYPE_FLOAT32:
		{
			fp4 = (*(PMQFLOAT32)value);
			fp8 = fp4;
			m_properties += "(dt=\"float\")=";

			// check for zero
			if ((float)0.0 == fp8)
			{
				// limit the number of significant digits
				m_properties += "0.0";
			}
			else
			{
				sprintf(data, "%#.12G", fp4);
				m_properties += data;
			}

			break;
		}
	case MQTYPE_FLOAT64:
		{
			fp8 = (*(PMQFLOAT64)value);
			m_properties += "(dt=\"double\")=";

			// check for zero
			if ((double)0.0 == fp8)
			{
				// limit the number of significant digits
				m_properties += "0.0";
			}
			else
			{
				// normal formatting
				sprintf(data, "%#.17G", fp8);
				m_properties += data;
			}

			break;
		}
	case MQTYPE_NULL:
		{
			m_properties += "(dt=\"null\")=";
			break;
		}
	default:
		{
			// don't know what this is - treat it as hex
			m_properties += "(dt=";

			// get the data type as an integer value and append it
			sprintf(data, "%d", type);
			m_properties += data;

			// close the parentheses, add the equal sign and start the hex string
			m_properties += ")=x'";

			// append the data as hex characters
			for (int i=0; i < valueLen; i++)
			{
				sprintf(data, "%.2X", value + i);
				m_properties += data;
			}

			m_properties += "'";
			break;
		}
	}

	// count the number of properties appended
	propertyCount++;

	// get the instance variables into the form data
	UpdateData(FALSE);
}*/

int CProps::GetPropertyCount()

{
	// get the form data into the instance variables
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps::GetPropertyCount() propertyCount=%d", propertyCount);

		// trace entry to GetPropertyCount
		pDoc->logTraceEntry(traceInfo);
	}

	return propertyCount;
}

void CProps::clearUserProperties()

{
	// get the form data into the instance variables
	char	traceInfo[128];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps::clearUserProperties() propertyCount=%d", propertyCount);

		// trace entry to clearUserProperties
		pDoc->logTraceEntry(traceInfo);
	}

	// Update the instance variables from the controls
	UpdateData (TRUE);

	// set the user property count to zero
	propertyCount = 0;
	
	// clear the actual properties
	m_properties.Empty();

	// update the controls from the instance variables
	UpdateData (FALSE);
}

void CProps::OnChangePropsUserProperties() 

{
	// Update the instance variables from the controls
	UpdateData (TRUE);

	if (m_properties.GetLength() == 0)
	{
		propertyCount = 0;
	}
	else
	{
		// get number of lines from the edit control
		propertyCount = m_PropertiesEditBox.GetLineCount();
	}
}

void CProps::getPropertyData(CString& data)

{
	// Update the instance variables from the controls
	UpdateData (TRUE);

	data = (LPCTSTR)m_properties;
}

/////////////////////////////////////////////////
//
// The move rfh usr button moves data from the
// RFH2 usr folder to the MQ user properties
// tab.  The display format is the same in 
// both cases, although there are more 
// restrictions on MQ user properties.  As a 
// result some items cannot be copied and 
// will be removed.
//
// The items that must be removed are as follows:
//  1) XML groups (structures)
//  2) Attributes other than dt (datatype).
//  3) Repeating value indicators.
//
/////////////////////////////////////////////////

void CProps::OnPropsMoveRfhUsr() 

{
	int			dataOK=0;			// check if the data can be copied
	CString		tempData;			// used to get the contents of the usr tab
	char		traceInfo[640];		// work variable to build trace message

	// Update the instance variables from the controls
	UpdateData (TRUE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps::OnPropsMoveRfhUsr() m_properties.GetLength()=%d", m_properties.GetLength());

		// trace entry to OnPropsMoveRfhUsr
		pDoc->logTraceEntry(traceInfo);
	}

	// get the data from the RFH usr tab
	pDoc->getRfhUsrData(tempData);

	// was there any data?
	if (tempData.GetLength() > 0)
	{
		// check the data for invalid constructs
		dataOK = checkPropData((LPCTSTR)tempData, (LPCTSTR)tempData + tempData.GetLength());

		if (1 == dataOK)
		{
			// check if a CRLF sequence should be appended first
			if (m_properties.GetLength() > 0)
			{
				m_properties += "\r\n";
			}

			// append the data from the RFH Usr tab
			m_properties += (LPCTSTR)tempData;

			// delete the data on the RFH usr tab
			pDoc->clearRfhUsrData();
		}
		else
		{
			// tell the user about the problem
			pDoc->m_error_msg = "Unsupported attributes or groups in data";
			pDoc->updateMsgText();
		}
	}

	// get the instance variables into the form data
	UpdateData(FALSE);

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CProps::OnPropsMoveRfhUsr() m_properties.GetLength()=%d tempData.GetLength()=%d dataOK=%d", m_properties.GetLength(), tempData.GetLength(), dataOK);

		// trace exit from OnPropsMoveRfhUsr
		pDoc->logTraceEntry(traceInfo);
	}
}

///////////////////////////////////////////////
//
// This routine will check the usr data for
// constructs that are allowed in the usr
// folder but not in MQ user properties.  This
// includes structures (periods in names) and
// attributes other than dt.
//
///////////////////////////////////////////////

int CProps::checkPropData(const char *start, const char *end)

{
	int			dataOK=1;			// return value - return 0 if invalid constructs found
	const char	*linePtr;			// pointer to beginning of input line
	const char	*endLine;			// end of current line
	const char	*ptr;				// pointer to next location in temporary data area

	// get a pointer to the data
	ptr = start;
	linePtr = start;

	// process the data a line at a time
	while ((linePtr < end) && (1 == dataOK))
	{
		// find the end of the line
		// this should be either a CR or LF character or a string termination (0)
		endLine = linePtr;
		while ((endLine < end) && (endLine[0] != 0) && (endLine[0] != '\r') && (endLine[0] != '\n'))
		{
			// move on to next character
			endLine++;
		}

		// check this line
		dataOK = checkLine(linePtr, endLine);

		// skip any CR or LF characters
		while ((endLine < end) && (('\r' == endLine[0]) || ('\n' == endLine[0])))
		{
			endLine++;
		}

		// move on ot the next line
		linePtr = endLine;
	}

	return dataOK;
}

///////////////////////////////////////////////
//
// This routine checks an individual line from
// the usr folder for constructs that are not
// supported by MQ user properties.  This
// includes names with periods and attributes
// other than dt.
//
///////////////////////////////////////////////

int CProps::checkLine(const char *start, const char *end)

{
	int		lineOK=1;			// result - 0 indicates unsupported construct found
	char	traceInfo[640];		// work variable to build trace message

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps::checkLine() start=%8.8X length=%d", (unsigned int)start, end - start);

		// trace entry to checkLine
		pDoc->logTraceEntry(traceInfo);

		// check for verbose trace
		if (pDoc->verboseTrace)
		{
			// write the input to the trace file
			pDoc->dumpTraceData("Usr data line", (unsigned char *)start, end - start);
		}
	}

	// search for a period to indicate an invalid name
	// or a parentheses in which case the attributes will be checked
	// or an equal sign indicating the end of the name
	while ((start < end) && (start[0] != '(') && (start[0] != '.') && (start[0] != 0))
	{
		// move on to the next character
		start++;
	}

	if ('.' == start[0])
	{
		// found a period in the name
		lineOK = 0;
	}
	else if ('(' == start[0])
	{
		// found attributes - check the attributes
		lineOK = checkLineAttrs(start, end);
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CProps::checkLine() start=%8.8X lineOK=%d", (unsigned int)start, lineOK);

		// trace exit from checkLine
		pDoc->logTraceEntry(traceInfo);
	}

	// return result - 1 if OK and 0 if unsupported construct found
	return lineOK;
}

///////////////////////////////////////////////
//
// This routine will check if the attributes
// and return a value of 1 if dt is the only
// atrribute.  A value of 0 will be returned
// if there are any attributes other than dt.
//
///////////////////////////////////////////////

int CProps::checkLineAttrs(const char *start, const char *end)

{
	int		attrOK=1;			// result
	char	quote;				// quote character
	char	traceInfo[640];		// work variable to build trace message

	// skip the begin parentheses and any whitespace characters
	start++;
	start = skipWhiteSpace(start, end);

	// check for dt followed by an equal sign or a space
	if ((memcmp(start, "xsi:nil=", 3) == 0) || (memcmp(start, "xsi:nil ", 3) == 0))
	{
		// xsi:nil attribute found - skip it and any whitespace following it
		start += 7;
		start = skipWhiteSpace(start, end);

		// check for an equal sign
		if ((start < end) && ('=' == start[0]))
		{
			// skip any white space in front of the value
			start = skipWhiteSpace(start + 1, end);

			// skip the value, looking for a blank or a close parentheses
			while ((start < end) && (start[0] > ' ') && (start[0] != ')'));
			{
				// continue to search for the end of the attribute
				start++;
			}

			// make sure a close parentheses was found
			if ((start < end) && (')' == start[0]))
			{
				// all is well - skip the closing parentheses and any whitespace after it
				start = skipWhiteSpace(start + 1, end);
			}
			else
			{
				// either there is another attribute or the line is invalid
				attrOK = 0;
			}
		}
		else
		{
			// no equal sign - report the error
			attrOK = 0;
		}
	}
	// check for dt followed by an equal sign or a space
	else if ((memcmp(start, "dt=", 3) != 0) && (memcmp(start, "dt ", 3) != 0))
	{
		// not a dt attribute 
		attrOK = 0;
	}
	else
	{
		// this is a dt attribute
		// skip the attribute name
		start += 2;

		// skip any white space
		start = skipWhiteSpace(start, end);

		// make sure the next character is an equal sign
		if ((start < end) && ('=' == start[0]))
		{
			// skip the equal sign
			start++;

			// skip any white space after the equal sign
			start = skipWhiteSpace(start, end);

			// check for end of line
			if (start < end)
			{
				// check for a quote character
				quote = start[0];
				if (('\'' == quote) || ('\"' == quote))
				{
					// skip the value by looking for the closing quotation
					start++;
					while ((start < end) && (start[0] != 0) && (start[0] != quote))
					{
						// keep looking
						start++;
					}

					// check if the closing quotation was found
					if ((start < end) && (start[0] == quote))
					{
						// see what comes after the quote
						start++;

						// skip whitespace
						start = skipWhiteSpace(start, end);

						// is the next character a close parenthese?
						if ((start < end) && (')' == start[0]))
						{
							// dt is the only attribute
							// skip the ending parentheses and any whitespace after it
							start++;
							start = skipWhiteSpace(start, end);
						}
						else
						{
							// must be another attribute
							attrOK = 0;
						}
					}
					else
					{
						// ending quotation not found
						attrOK = 0;
					}
				}
				else
				{
					// no quotes - use white space as a delimiter
					while ((start < end) && (start[0] > ' ') && (start[0] != ')') && (start[0] != '='))
					{
						// search for the end of the value
						start++;
					}

					// skip any whitespace after the value
					start = skipWhiteSpace(start, end);

					// check if a close parenthese was found
					if ((start < end) && (')' == start[0]))
					{
						// skip the ending parentheses and any whitespace after it
						start++;
						start = skipWhiteSpace(start, end);
					}
					else
					{
						// must be another attribue
						attrOK = 0;
					}
				}
			}
			else
			{
				// not valid
				attrOK = 0;
			}
		}
		else
		{
			// not an equal sign - not valid
			attrOK = 0;
		}
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CProps::checkLineAttrs() start=%8.8X attrOK=%d", (unsigned int)start, attrOK);

		// trace exit from checkLineAttrs
		pDoc->logTraceEntry(traceInfo);
	}

	// return 0 if an attribute other than dt is found
	return attrOK;
}

///////////////////////////////////////////////
//
// This routine will search for a property that
// indicates a publication is a retained 
// publication.  This is set by MQ V7 publish
// and subscribe.
//
///////////////////////////////////////////////

void CProps::checkForRetained(BOOL *isRetained)

{
	int		ofs;
	int		ofs2;
	int		ofs3;
	int		ofs4;
	int		ofs5;
	int		ofs6;
	int		ofs7;
	int		ofs8;
	char	traceInfo[640];		// work variable to build trace message

	// Update the instance variables from the controls
	UpdateData (TRUE);

	// initialize the is retained value to false
	(*isRetained) = FALSE;

	// were any user properties found?
	if (m_properties.GetLength() > 0)
	{
		ofs = m_properties.Find("MQIsRetained(dt=\"boolean\")=1");
		ofs2 = m_properties.Find("MQIsRetained(dt=\"bool\")=1");
		ofs3 = m_properties.Find("MQIsRetained(dt=\"boolean\")=TRUE");
		ofs4 = m_properties.Find("MQIsRetained(dt=\"bool\")=TRUE");
		ofs5 = m_properties.Find("MQIsRetained(dt=\'boolean\')=1");
		ofs6 = m_properties.Find("MQIsRetained(dt=\'bool\')=1");
		ofs7 = m_properties.Find("MQIsRetained(dt=\'boolean\')=TRUE");
		ofs8 = m_properties.Find("MQIsRetained(dt=\'bool\')=TRUE");

		if ((ofs >= 0) || (ofs2 >= 0) || (ofs3 >= 0) || (ofs4 >= 0) || (ofs5 >= 0) || (ofs6 >= 0) || (ofs7 >= 0) || (ofs8 >= 0))
		{
			// found the property
			(*isRetained) = TRUE;
		}
	}

	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exit CProps::checkForRetained() ofs=%d ofs2=%d ofs3=%d ofs4=%d ofs5=%d ofs6=%d ofs7=%d ofs8=%d isRetained=%d m_properties.GetLength()=%d m_properties=%.512s", ofs, ofs2, ofs3, ofs4, ofs5, ofs6, ofs7, ofs8, (*isRetained), m_properties.GetLength(), (LPCTSTR)m_properties);

		// trace exit from checkForRetained
		pDoc->logTraceEntry(traceInfo);
	}
}

/////////////////////////////////
//
// Routine to create an XML
// parse tree from the user
// properties and return the 
// number of the first child 
// element of the root
//
/////////////////////////////////

int CProps::getFirstProperty()

{
	int		elem=0;				// value to return
	int		parseResult=0;		// parse return code
	char	traceInfo[512];		// work area for trace line

	// is trace enabled?
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CProps()::getFirstProperty m_properties.GetLength()=%d xml=%8.8X", m_properties.GetLength(), (unsigned int)xml);

		// trace entry to getFirstProperty
		pDoc->logTraceEntry(traceInfo);

		// check for verbose trace
		if (pDoc->verboseTrace)
		{
			// dump out the input
			pDoc->dumpTraceData("m_properties", (const unsigned char *)(LPCTSTR)m_properties, m_properties.GetLength());
		}
	}

	// is there already an XML object?
	if (xml != NULL)
	{
		// delete the current XML object
		delete(xml);
	}

	// instantiate a new XML parser object
	xml = new CXMLParse();

	// make sure it worked
	if (xml != NULL)
	{
		// parse the current properties
		parseResult = xml->parseFormattedText((LPCTSTR)m_properties, "usr");

		// check the return code from the parse
		if (PARSE_OK == parseResult)
		{
			// parse was successful
			// get the first child of the root
			elem = xml->getFirstChild(0);
		}
	}

	// is trace enabled?
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CProps()::getFirstProperty elem=%d parseResult=%d xml=%8.8X", elem, parseResult, (unsigned int)xml);

		// trace exit from getFirstProperty
		pDoc->logTraceEntry(traceInfo);
	}

	// return the first child element
	return elem;
}

const char * CProps::getPropName(int elem)

{
	const char *	name="";
	char			traceInfo[512];		// work area for trace line

	// make sure the element is not the root
	if ((elem > 0) && (xml != NULL))
	{
		// get the name of the element
		name = xml->getElemName(elem);
	}

	// is trace enabled?
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CProps()::getPropName elem=%d name=%.256s", elem, name);

		// trace exit from getPropName
		pDoc->logTraceEntry(traceInfo);
	}

	return name;
}

const char * CProps::getPropValue(int elem)

{
	const char *	value="";
	char			traceInfo[512];		// work area for trace line

	// make sure the element is not the root
	if ((elem > 0) && (xml != NULL))
	{
		// get the value of the element
		value = xml->getElemValue(elem);
	}

	// is trace enabled?
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CProps()::getPropValue elem=%d value=%.256s", elem, value);

		// trace exit from getPropValue
		pDoc->logTraceEntry(traceInfo);
	}

	return value;
}

int CProps::getPropType(int elem)

{
	int			firstChild=0;
	int			xmlType=0;
	const char	*attrName="";
	const char	*attrValue="";
	MQLONG		type=MQTYPE_STRING;
	char		traceInfo[512];		// work area for trace line

	// make sure the element is not the root
	if ((elem > 0) && (xml != NULL))
	{
		// get the first child of the element
		firstChild = xml->getFirstChild(elem);

		// check if there is a child
		if (firstChild > 0)
		{
			// get the element type and make sure it is an attribute
			xmlType = xml->getElemType(firstChild);

			// check if it is an attribute
			if (TYPE_ATTR == xmlType)
			{
				// get the name and value of the attribute
				attrName = xml->getElemName(firstChild);
				attrValue = xml->getElemValue(firstChild);

				// check if the attribute name is dt
				if (strcmp(attrName, "dt") == 0)
				{
					// found a data type (dt) attribute
					// check the value and return the type if it is recognized
					// the default of string is returned if the type is not recognized
					type = propTypeToInt(attrValue);
				}
				else if (strcmp(attrName, "xsi:nil") == 0)
				{
					// set the type to NULL
					type = MQTYPE_NULL;
				}
			}
		}
	}

	// is trace enabled?
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CProps()::getPropType elem=%d type=%d firstChild=%d xmlType=%d attrName=%.128s attrValue=%.128s", elem, type, firstChild, xmlType, attrName, attrValue);

		// trace exit from getPropType
		pDoc->logTraceEntry(traceInfo);
	}

	return type;
}

int CProps::getNextProp(int elem)

{
	int			nextSib=0;
	char		traceInfo[512];		// work area for trace line

	// make sure the element is not the root
	if ((elem > 0) && (xml != NULL))
	{
		// get the value of the element
		nextSib = xml->getNextSibling(elem);
	}

	// check for no more siblings
	if (0 == nextSib)
	{
		// done with the XML parser object
		delete(xml);

		// reset the pointer
		xml = NULL;
	}

	// is trace enabled?
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CProps()::getNextProp elem=%d nextSib=%d xml=%8.8X", elem, nextSib, (unsigned int)xml);

		// trace exit from getNextProp
		pDoc->logTraceEntry(traceInfo);
	}

	return nextSib;
}

int CProps::propTypeToInt(const char * ptr)

{
	int			type=MQTYPE_STRING;
	int			len;
	char		typeStr[32];
	char		traceInfo[512];		// work area for trace line

	// get the type value so the quotes can be removed
	memset(typeStr, 0, sizeof(typeStr));
	len = strlen(ptr);

	// check if the value will fit in the typeStr area
	// if it will not fit then it cannot match any valid type
	if (len < sizeof(typeStr) - 1)
	{
		if (('\'' == ptr[0]) || ('\"' == ptr[0]))
		{
			// skip the first quote
			ptr++;
			len--;
		}

		// copy the string into the work area
		strcpy(typeStr, ptr);

		// check for a quote on the end of the string
		if (('\'' == typeStr[len-1]) || ('\"' == typeStr[len-1]))
		{
			// remove the trailing quote
			typeStr[len-1] = 0;
		}

		// convert the string to upper case to avoid upper/lower case problems
		_strlwr(typeStr);

		// check the data type
		if (strcmp(typeStr, "byte") == 0)
		{
			type = MQTYPE_INT8;
		}
		else if (strcmp(typeStr, "i1") == 0)
		{
			type = MQTYPE_INT8;
		}
		else if (strcmp(typeStr, "short") == 0)
		{
			type = MQTYPE_INT16;
		}
		else if (strcmp(typeStr, "i2") == 0)
		{
			type = MQTYPE_INT16;
		}
		else if (strcmp(typeStr, "int") == 0)
		{
			type = MQTYPE_INT32;
		}
		else if (strcmp(typeStr, "i4") == 0)
		{
			type = MQTYPE_INT32;
		}
		else if (strcmp(typeStr, "i64") == 0)
		{
			type = MQTYPE_INT64;
		}
		else if (strcmp(typeStr, "i8") == 0)
		{
			type = MQTYPE_INT64;
		}
		else if (strcmp(typeStr, "long") == 0)
		{
			type = MQTYPE_LONG;
		}
		else if (strcmp(typeStr, "bin.hex") == 0)
		{
			type = MQTYPE_BYTE_STRING;
		}
		else if (strcmp(typeStr, "bool") == 0)
		{
			type = MQTYPE_BOOLEAN;
		}
		else if (strcmp(typeStr, "boolean") == 0)
		{
			type = MQTYPE_BOOLEAN;
		}
		else if (strcmp(typeStr, "float") == 0)
		{
			type = MQTYPE_FLOAT32;
		}
		else if (strcmp(typeStr, "r4") == 0)
		{
			type = MQTYPE_FLOAT32;
		}
		else if (strcmp(typeStr, "double") == 0)
		{
			type = MQTYPE_FLOAT64;
		}
		else if (strcmp(typeStr, "r8") == 0)
		{
			type = MQTYPE_FLOAT64;
		}
		else if (strcmp(typeStr, "null") == 0)
		{
			type = MQTYPE_NULL;
		}
		else if (strcmp(typeStr, "string") == 0)
		{
			type = MQTYPE_STRING;
		}
		else if (strcmp(typeStr, "str") == 0)
		{
			type = MQTYPE_STRING;
		}
		else
		{
			// check if this is an unknown property type which is rendered as an number
			if ((typeStr[0] >= '0') && (typeStr[0] <= '9'))
			{
				type = atoi(typeStr);
			}
		}
	}

	// is trace enabled?
	if (pDoc->traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CProps()::propTypeToInt type=%d len=%d typeStr=%s ptr=%.128s", type, len, typeStr, ptr);

		// trace exit from propTypeToInt
		pDoc->logTraceEntry(traceInfo);
	}

	return type;
}
