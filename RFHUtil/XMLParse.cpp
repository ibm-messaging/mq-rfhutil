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

// XMLParse.cpp: implementation of the CXMLParse class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "XMLParse.h"
#include "comsubs.h"
#include "xmlsubs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// defines for tag type
#define		TAG_NAME	0
#define		TAG_END		1
#define		TAG_NULL	2

// defines for XML processing
#define CDATA_START		"<![CDATA["
#define CDATA_END		"]]>"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLParse::CXMLParse()
{
	// do some initialization
	numElements = 0;
	xmlElems = NULL;
	lastErrorOffset = 0;
}

CXMLParse::~CXMLParse()

{
	// was a parse tree allocated?
	if (xmlElems != NULL)
	{
		// free the parse tree
		rfhFree(xmlElems);
	}
}

int CXMLParse::parse(const char * xmlIn, int length)

{
	int				result=PARSE_OK;	// result of the parse
	int				numBrackets=0;		// number of begin brackets found
	int				numEquals=0;		// number of equal signs
	int				idx;				// index within xml input
	bool			validData=true;		// indicate if invalid characters found in data
	const char *	ptr;				// work pointer
	CRfhutilApp *	app;				// pointer to application object to locate trace routines
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::parse length=%d", length);

		// trace entry to CXMLParse::parse
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the input
			app->dumpTraceData("XML parse input", (unsigned char *) xmlIn, length);
		}
	}

	// point to the XML data
	ptr = xmlIn;
	idx = 0;

	// count the number of begin brackets ('<')
	while ((idx < length) && validData)
	{
		// count the number of brackets and equal signs
		// should be an element or an attribute
		if ('<' == ptr[idx])
		{
			// count the number of begin brackets
			numBrackets++;
		}
		else if ('=' == ptr[idx])
		{
			// count the number of equal signs
			// could be an attribute
			numEquals++;
		}
		else if (0 == ptr[idx])
		{
			// invalid character found
			validData = false;
			lastErrorOffset = idx;
			result = PARSE_BIN_ZERO;
		}
		else if ((ptr[idx] < ' ') && (ptr[idx] != '\n') && (ptr[idx] != '\r') && (ptr[idx] != '\t'))
		{
			// invalid character found
			validData = false;
			lastErrorOffset = idx;
			result = PARSE_BAD_CHARS;
		}

		// data still valid?
		if (validData)
		{
			// move on to the next character
			idx++;
		}
	}

	// were any invalid characters found?
	if (!validData)
	{
		// is trace enabled?
		if (app->isTraceEnabled())
		{
			// create the trace line
			sprintf(traceInfo, "*****Error at idx=%d result=%d ptr[idx]=%d numBrackets=%d numEquals=%d", idx, result, ptr[idx], numBrackets, numEquals);

			// write error message to trace
			app->logTraceEntry(traceInfo);
		}

		// invalid characters found - just return
		return result;
	}

	// allocate storage for an array of elements
	xmlElems = (XMLSTRUCT *)rfhMalloc((numBrackets + numEquals + 1) * sizeof(XMLSTRUCT), "XMLELEMS");

	// did the malloc work?
	if (NULL == xmlElems)
	{
		// failed - just get out
		return PARSE_MALLOC_FAIL;
	}

	// initialize the storage
	memset(xmlElems, 0, (numBrackets + numEquals + 1) * sizeof(XMLSTRUCT));

	// parse the XML
	result = parseXML(xmlIn, length);

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::parse numBrackets=%d numEquals=%d numElements=%d result=%d", numBrackets, numEquals, numElements, result);

		// trace exit from CXMLParse()::parse
		app->logTraceEntry(traceInfo);
	
		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the table entries
			app->dumpTraceData("XML parse tree", (unsigned char *) xmlElems, numElements * sizeof(XMLSTRUCT));

			// dump out the names table as well
			dumpNames();
		}
	}

	return result;
}

const char * CXMLParse::processDTD(const char *ptr, const char *endPtr)

{
	// found DTD specification
	// skip the 8 characters at the beginning
	ptr += 8;

	// search for the end of the document type
	while ((ptr < endPtr) && (ptr[0] != '>'))
	{
		// check for quotations
		if (('\"' == ptr[0]) || ('\'' == ptr[0]))
		{
			// skip the contents within the quotes
			ptr = processQuote(ptr, endPtr, ptr[0]);
		}
		else
		{
			// skip the character
			ptr++;
		}
	}

	// skip the end bracket
	if ((ptr < endPtr) && ('>' == ptr[0]))
	{
		// skip the end bracket
		ptr++;
	}

	return ptr;
}

const char * CXMLParse::processQuote(const char *ptr, const char *endPtr, char quote)

{
	// skip the beginning quote mark
	ptr++;

	// search for the ending quotation mark
	while ((ptr < endPtr) && (ptr[0] != quote))
	{
		// skip the next character
		ptr++;
	}

	if ((ptr < endPtr) && (quote == ptr[0]))
	{
		// skip the trailing quotation mark
		ptr++;
	}

	return ptr;
}

const char * CXMLParse::skipComment(const char *ptr, const char *endPtr)

{
	// skip the beginning of the comment
	ptr += 3;

	// skip the comment
	while ((ptr + 2 < endPtr) && (memcmp(ptr, "-->", 3) != 0))
	{
		// skip the next character
		ptr++;
	}

	// was the end of comment found?
	if ((ptr + 2 < endPtr) && (memcmp(ptr, "-->", 3) == 0))
	{
		// skip the end of the comment
		ptr += 3;

		// find the next character
		ptr = skipWhiteSpace(ptr, endPtr);
	}

	return ptr;
}
int CXMLParse::parseXML(const char *xmlIn, int length)

{
	int				result=PARSE_OK;	// result - TRUE means parse worked
	int				nameLen;			// length of the name
	int				len;				// work variable
	int				parent=0;			// current parent element
	int				currElem = 0;			// current element
	int				currAttr = 0;			// current attribute
	int				numElems=0;			// number of elements
	int				numAttrs=0;			// number of attributes
	int				attrType;			// element type (attribute and type of quote used)
	const char *	ptr;				// working pointer
	const char *	endPtr;				// end of the data
	const char *	name;				// pointer to element name
	const char *	endName;			// pointer to end of name
	const char *	attrPtr;			// pointer to attributes
	const char *	attrName;			// pointer to beginning of attribute name
	const char *	attrNameEnd;		// end of attribute name
	const char *	attrValue;			// pointer to beginning of attribute value
	const char *	attrValueEnd;		// end of attribute value
	const char *	cdata;				// start of cdata section
	const char *	cdataEnd;			// end of cdata section
	const char *	valuePtr;			// pointer to beginning of the value
	const char *	namePtr;			// pointer to name in names table
	char			endQuote;			// attribute value delimiter
	CRfhutilApp *	app;
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::parseXML length=%d", length);

		// trace entry to CXMLParse()::parseXML
		app->logTraceEntry(traceInfo);
	}

	// point to the XML data to be parsed
	ptr = xmlIn;

	// check for trailing white space
	while ((length > 0) && (ptr[length - 1] <= ' '))
	{
		// ignore the white space at the end
		length--;
	}

	// calculate the end of the data
	endPtr = xmlIn + length;

	// loop through the input (until the pointer reaches the end of the input)
	while ((ptr < endPtr) && (PARSE_OK == result))
	{
		// skip blanks
		ptr = skipWhiteSpace(ptr, endPtr);

		// check for a begin bracket
		if ((ptr[0] != '<') && (PARSE_OK == result))
		{
			// indicate parsing error
			result = PARSE_START_BRACKET;
			lastErrorOffset = ptr - xmlIn;
		}
		else
		{
			ptr++;
		}

		// skip white space
		ptr = skipWhiteSpace(ptr, endPtr);

		// check for certain XML constructs such as XML ID
		while ((ptr < endPtr) && (('?' == ptr[0]) || ('!' == ptr[0])))
		{
			if ('?' == ptr[0])
			{
				// remove the XML ID
				// skip the question mark
				ptr++;

				// find the next question mark and end bracket combination
				while ((ptr < endPtr) && ((ptr[0] != '?') || (ptr[1] != '>')))
				{
					// skip character
					ptr++;
				}

				if ((ptr < endPtr) && (memcmp(ptr, "?>", 2) == 0))
				{
					// skip the last two characters
					ptr += 2;
				}
			}
			else if ('!' == ptr[0])
			{
				// check for DOCTYPE
				if ((ptr + 8 < endPtr) && (memcmp(ptr, "!DOCTYPE", 8) == 0))
				{
					// skip the declaration
					ptr = processDTD(ptr, endPtr);
				}
				else if ((ptr + 5 < endPtr) && (memcmp(ptr, "!--", 3) == 0))
				{
					// found XML comment - skip the comment
					ptr = skipComment(ptr, endPtr);
				}
			}
			
			// skip any white space
			ptr = skipWhiteSpace(ptr, endPtr);

			// check for a bracket
			if (ptr[0] != '<')
			{
				result = PARSE_START_BRACKET;
				lastErrorOffset = ptr - xmlIn;
			}
			else
			{
				ptr++;
			}
		}

		// skip any blanks
		ptr = skipWhiteSpace(ptr, endPtr);

		// get the beginning of the name
		name = ptr;
		endName = ptr;

		// check for an end tag
		if ('/' == ptr[0])
		{
			// skip the end tag indicator so the next loop does not stop too soon
			ptr++;
		}

		// find the end of the name pointer
		while ((ptr < endPtr) && (ptr[0] != '/') && (ptr[0] != '>'))
		{
			ptr++;
			endName++;
		}

		// check for an end tag
		if ('/' == name[0])
		{
			// this is an end tag
			// check that it matches the current element
			// get a pointer to the name of the current parent
			namePtr = names.getNameAddr(xmlElems[parent].name);

			// is verbose trace enabled?
			if (app->isTraceEnabled() && app->verboseTrace)
			{
				// create the trace line
				sprintf(traceInfo, " end tag name=%s parent=%d", namePtr, parent);

				// add to trace
				app->logTraceEntry(traceInfo);
			}

			// compare the names
			if (memcmp(name + 1, namePtr, ptr - name - 1) != 0)
			{
				// parsing error
				lastError  = "Parse error - name mismatch";
				lastErrorOffset = ptr - xmlIn;
				result = PARSE_TAG_MISMATCH;

				// is verbose trace enabled?
				if (app->isTraceEnabled())
				{
					// create the trace line
					sprintf(traceInfo, " tag mismatch name=%.*s namePtr=%s length=%d parent=%d lastErrorOffset=%d", ptr-name-1, name + 1, namePtr, ptr-name-1, parent, lastErrorOffset);

					// add to trace
					app->logTraceEntry(traceInfo);
				}
			}

			// move the parent pointer
			parent = xmlElems[parent].parent;

			// skip the end bracket
			ptr++;
		}
		else
		{
			// check for attributes in the name
			attrPtr = name;
			while ((attrPtr < endName) && ((unsigned char)attrPtr[0] > ' '))
			{
				// move on to the next character in the name
				attrPtr++;
			}

			// calculate the length of the name
			nameLen = attrPtr - name;

			// was there a nmae?
			if (nameLen > 0)
			{
				// create an entry for this name
				currElem = insertEntry(name, nameLen, parent, TYPE_ELEM);

				// is verbose trace enabled?
				if (app->isTraceEnabled() && app->verboseTrace)
				{
					// create the trace line
					sprintf(traceInfo, " inserted currElem=%d parent=%d name=%.*s", currElem, parent, nameLen, name);

					// add to trace
					app->logTraceEntry(traceInfo);
				}

				// count the number of elements
				numElems++;

				// was there a space in the name?
				while (attrPtr < endName)
				{
					// check for attributes in the name
					// skip the blanks
					attrPtr = skipWhiteSpace(attrPtr, endName);

					// remember the start of the attribute name
					attrName = attrPtr;

					// find the end of the attribute name
					while ((attrPtr < endName) && (attrPtr[0] != '='))
					{
						// move on to the next character in the attribute string
						attrPtr++;
					}

					// check if an equal sign was found
					if (attrPtr < endName)
					{
						// point to the end of the attribute name
						attrNameEnd = attrPtr;

						// skip the equal sign
						attrPtr++;

						// skip any white space after the equal sign
						attrPtr = skipWhiteSpace(attrPtr, endName);

						// point to the attribute value
						attrValue = attrPtr;

						// figure out what delimits the attribute value
						if ('\'' == attrValue[0])
						{
							// will look for a single quote mark
							endQuote = '\'';

							// skip the single quote 
							attrValueEnd = attrValue + 1;

							// set the attribute type
							attrType = TYPE_ATTR | TYPE_SINGLE;
						}
						else if ('\"' == attrValue[0])
						{
							// will look for a double quotation mark
							endQuote = '\"';

							// skip the double quote 
							attrValueEnd = attrValue + 1;

							// set the attribute type
							attrType = TYPE_ATTR | TYPE_DOUBLE;
						}
						else
						{
							// no quotation - just look for a blank
							endQuote = ' ';
							attrValueEnd = attrValue;

							// set the attribute type
							attrType = TYPE_ATTR;
						}

						// search for the end of the value
						while ((attrValueEnd < endName) && (attrValueEnd[0] != endQuote))
						{
							// move on to the next character
							attrValueEnd++;
						}

						// what kind of delimiter was used?
						if ((attrValueEnd < endName) && (endQuote != ' '))
						{
							// include the closing quotation in the value
							attrValueEnd++;
						}

						// insert the attribute entry into the table
						currAttr = insertEntry(attrName, attrNameEnd - attrName, currElem, attrType);

						// count the number of attributes
						numAttrs++;

						// is verbose trace enabled?
						if (app->isTraceEnabled() && app->verboseTrace)
						{
							// create the trace line
							sprintf(traceInfo, " attribute inserted currAttr=%d attrType=%d parent=%d numAttrs=%d name=%.*s", currAttr, attrType, currElem, numAttrs, attrNameEnd - attrName, attrName);

							// add to trace
							app->logTraceEntry(traceInfo);
						}

						// check for a value
						if (attrValueEnd > attrValue)
						{
							// before the value is saved  the PCDATA must be converted to character data
							// add a value to the attribute
							convertAddValue(currAttr, attrValue, attrValueEnd - attrValue);
						}

						// move on to the next attribute
						attrPtr = attrValueEnd;
					}
				}
			}

			// already at the end of the input?
			if (ptr < endPtr)
			{
				// look for a value for this element
				if ('/' == ptr[0])
				{
					// no value in this case
					ptr++;

					// find the end of the tag
					while ((ptr < endPtr) && (ptr[0] != '>'))
					{
						ptr++;
					}

					// check if an end bracket was found
					if ((ptr < endPtr) && ('>' == ptr[0]))
					{
						// skip the end bracket
						ptr++;
					}

					// skip any trailing white space
					ptr = skipWhiteSpace(ptr, endPtr);
				}
				else if ('>' == ptr[0])
				{
					// change the parent pointer to the current element
					parent = currElem;

					// skip the ending character of the tag
					ptr++;

					// check for cdata
					cdata = ptr;
					cdata = skipWhiteSpace(cdata, endPtr);

					// check if a cdata section was found
					if ((cdata + sizeof(CDATA_START) < endPtr) && (memcmp(cdata, CDATA_START, sizeof(CDATA_START) - 1) == 0))
					{
						// found a cdata section
						// find the end of the CDATA section
						cdataEnd = cdata + sizeof(CDATA_START) - 1;
						while ((cdataEnd + sizeof(CDATA_END) - 1 < endPtr) && (memcmp(cdataEnd, CDATA_END, sizeof(CDATA_END) - 1) != 0))
						{
							// go on to the next character
							cdataEnd++;
						}

						// was the end of the cdata section found?
						if ((cdataEnd + sizeof(CDATA_END) - 1 < endPtr) && (memcmp(cdataEnd, CDATA_END, sizeof(CDATA_END) - 1) == 0))
						{
							// increment the cdata end pointer to the end of the cdata section
							cdataEnd += sizeof(CDATA_END) - 1;
						}

						// update the pointer
						ptr = cdataEnd;

						// get the length of the cdata
						len = cdataEnd - cdata;

						// update the value of the current element
						addValue(currElem, cdata, len);

						// is verbose trace enabled?
						if (app->isTraceEnabled() && app->verboseTrace)
						{
							// limit to 64 bytes of data
							if (len > 64)
							{
								// limit to 64 characters
								len = 64;
							}

							// create the trace line
							sprintf(traceInfo, " cdata currElem=%d len=%d cdata=%.*s", currElem, cdataEnd - cdata, len, cdata);

							// add to trace
							app->logTraceEntry(traceInfo);
						}
					}
					else
					{
						// skip any white space at the beginning of the value
						ptr = skipWhiteSpace(ptr, endPtr);

						// remember the beginning of the value
						valuePtr = ptr;

						// search for the end of the value
						while ((ptr < endPtr) && (ptr[0] != '<'))
						{
							ptr++;
						}

						// check if a value was found
						if (ptr > valuePtr)
						{
							// get the data length
							len = ptr - valuePtr;

							// add the value to the current element
							convertAddValue(currElem, valuePtr, len);

							// is verbose trace enabled?
							if (app->isTraceEnabled() && app->verboseTrace)
							{
								// limit to 64 bytes of data
								if (len > 64)
								{
									// limit to 64 characters
									len = 64;
								}

								// create the trace line
								sprintf(traceInfo, " value currElem=%d len=%d value=%.*s", currElem, ptr - valuePtr, len, valuePtr);

								// add to trace
								app->logTraceEntry(traceInfo);
							}
						}
					}
				}
			}
		}

		// check for verbose trace
		if (app->isTraceEnabled() && app->verboseTrace)
		{
			// dump out the table entries
			app->dumpTraceData("XML parse tree", (unsigned char *) xmlElems, numElements * sizeof(XMLSTRUCT));

			// dump out the names table as well
			dumpNames();
		}
	}

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::parseXML numElems=%d numAttrs=%d numElements=%d result=%d", numElems, numAttrs, numElements, result);

		// trace exit from CXMLParse()::parseXML
		app->logTraceEntry(traceInfo);
	}

	return result;
}

int CXMLParse::insertEntry(const char * name, int nameLen, int parent, int type)

{
	int				nameOfs=0;
	int				lastChild;
	CRfhutilApp *	app;
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled() && app->verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::insertEntry nameLen=%d parent=%d type=%d name=%.*s", nameLen, parent, type, nameLen, name);

		// trace entry to CXMLParse()::insertEntry
		app->logTraceEntry(traceInfo);
	}

	// check for a name
	if (nameLen > 0)
	{
		nameOfs = names.insertName(name, nameLen);
	}

	// capture the name
	xmlElems[numElements].name = nameOfs;

	// set the parent
	xmlElems[numElements].parent = parent;

	// set the type
	xmlElems[numElements].type = type;

	// get the last child of the parent
	lastChild = xmlElems[parent].lastChild;

	// insert as the new last child
	xmlElems[parent].lastChild = numElements;

	// are there any children of this parent?
	if (0 == lastChild)
	{
		// first child of this parent
		xmlElems[parent].firstChild = numElements;
	}
	else
	{
		// the last child is now the previous sibling of this element
		xmlElems[numElements].prevSib = lastChild;

		// the previous last child now has a next sibling
		xmlElems[lastChild].nextSib = numElements;
	}

	// move on to the next element
	numElements++;

	if (app->isTraceEnabled() && app->verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::insertEntry numElements=%d", numElements);

		// trace exit from CXMLParse()::insertEntry
		app->logTraceEntry(traceInfo);
	}

	return numElements - 1;
}

void CXMLParse::addValue(int elem, const char * value, int valueLen)

{
	int				valueOfs=0;
	CRfhutilApp *	app;
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled() && app->verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::addValue elem=%d valueLen=%d value=%.*s", elem, valueLen, valueLen, value);

		// trace entry to CXMLParse()::addValue
		app->logTraceEntry(traceInfo);
	}

	if (valueLen > 0)
	{
		valueOfs = names.insertName(value, valueLen);
	}

	xmlElems[elem].value = valueOfs;

	if (app->isTraceEnabled() && app->verboseTrace)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::addValue valueOfs=%d", valueOfs);

		// trace exit from CXMLParse()::addValue
		app->logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////
//
// Routine to remove escape sequences
// and then add the value to the element.
//
/////////////////////////////////////////

void CXMLParse::convertAddValue(int elem, const char *value, int length)

{
	char	newValue[2048];
	char	*ptr;

	// check if there are escape sequences in the value
	if (foundEscape(value, value + length) & 0)
	{
		// have to remove the escape sequences from the value
		// check if the value will fit in the newValue area
		if (length < sizeof(newValue) - 1)
		{
			// use the newValue area
			// copy the data and terminate the string
			memcpy(newValue, value, length);
			newValue[length] = 0;

			// remove the escape sequences
			removeEscSeq(newValue);

			// add the value
			addValue(elem, newValue, strlen(newValue));
		}
		else
		{
			// have to allocate storage
			ptr = (char *)rfhMalloc(length + 8, "XMLPTR  ");

			// copy the data and terminate the string
			memcpy(ptr, value, length);
			ptr[length] = 0;

			// remove any escape sequences from the data
			removeEscSeq(ptr);

			// add the value
			addValue(elem, ptr, strlen(ptr));

			// free the storage
			rfhFree(ptr);
		}
	}
	else
	{
		// no escape sequences in the value - just add it
		addValue(elem, value, length);
	}
}

void CXMLParse::getLastError(char *errtxt, int maxLen)

{
	int		len;

	// figure out if the string will fit in memory
	len = lastError.GetLength();
	if (len > maxLen - 1)
	{
		len = maxLen - 1;
	}

	// copy the error string
	memcpy(errtxt, (LPCTSTR)lastError, len);
	errtxt[len] = 0;
}

/////////////////////////////////////////////////////////
//
// Create a multi-line string representing the parsed
// XML contained in the XML object. 
//
/////////////////////////////////////////////////////////

void CXMLParse::buildParsedArea(char * output, int maxLen, BOOL includeRoot)

{
	char			*ptr;
	CRfhutilApp		*app;
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::buildParsedArea maxLen=%d includeRoot=%d", maxLen, includeRoot);

		// trace entry to CXMLParse()::buildParsedArea
		app->logTraceEntry(traceInfo);
	}

	// initialize the output data to a null string
	output[0] = 0;

	// check if there is any XML data
	if (0 == numElements)
	{
		// just return a null string
		return;
	}

	// start the process
	ptr = processElement(0, output, output + maxLen, includeRoot);

	// terminate the string
	ptr[0] = 0;

	// remove any trailing CR or LF characters
	ptr--;
	while ((ptr > output) && (('\r' == ptr[0]) || ('\n' == ptr[0])))
	{
		ptr[0] = 0;
		ptr--;
	}

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::buildParsedArea length=%d", ptr-output);

		// trace exit from CXMLParse()::buildParsedArea
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the table entries
			app->dumpTraceData("parsed data", (unsigned char *)output, strlen(output));
		}
	}
}

///////////////////////////////////////////////////
//
// Process an element and all of its children
//
///////////////////////////////////////////////////

char * CXMLParse::processElement(int elem, char *ptr, char *endPtr, BOOL includeRoot)

{
	int				currElem;
	int				len;
	char			*namePtr;			// pointer to name as a string
	char			*valuePtr;
	CRfhutilApp		*app;				// pointer to application object to locate trace routines
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if ((app->isTraceEnabled()) && (app->verboseTrace))
	{
		// get a pointer to the name and value
		namePtr = names.getNameAddr(xmlElems[elem].name);
		valuePtr = names.getNameAddr(xmlElems[elem].value);

		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::processElement elem=%d firstChild=%d lastChild=%d prevSib=%d nextSib=%d name=%.64s value=%.64s",
				elem, xmlElems[elem].firstChild, xmlElems[elem].lastChild, xmlElems[elem].prevSib, xmlElems[elem].nextSib, namePtr, valuePtr);

		// trace entry to CXMLParse()::processElement
		app->logTraceEntry(traceInfo);
	}

	// check for attributes
	currElem = xmlElems[elem].firstChild;
	while ((currElem > 0) && ((xmlElems[currElem].type & TYPE_ATTR) != TYPE_ATTR))
	{
		// move on to the next sibling
		currElem = xmlElems[currElem].nextSib;
	}

	// check for no children or a value or has attributes
	if ((xmlElems[elem].value > 0) || ((0 == xmlElems[elem].firstChild) && (0 == xmlElems[elem].lastChild)) || (currElem > 0))
	{
		// should the root name be included?
		if ((elem != 0) || includeRoot)
		{
			// process any attributes
			ptr = processAttrs(elem, ptr, endPtr, includeRoot);

			// check if the element has a value
			if (xmlElems[elem].value > 0)
			{
				// need to output this element
				// append the complete name
				ptr = addName(elem, ptr, endPtr, includeRoot);

				// get a pointer to the value
				valuePtr = names.getNameAddr(xmlElems[elem].value);

				// get the length of the value
				len = strlen(valuePtr);

				// check if the value will fit
				if (ptr + len + 2 < endPtr)
				{
					// check for the root element
					// insert an equal sign
					ptr[0] = '=';
					ptr++;

					// insert the value
					strcpy(ptr, valuePtr);

					// update the pointer
					ptr += len;

					// insert a CRLF sequence
					ptr++[0] = '\r';
					ptr++[0] = '\n';
				}
			}
		}
	}

	// process any children of this element
	currElem = xmlElems[elem].firstChild;
	while (currElem > 0)
	{
		// skip attributes
		if ((xmlElems[currElem].type & TYPE_ATTR) != TYPE_ATTR)
		{
			// process the child element
			ptr = processElement(currElem, ptr, endPtr, includeRoot);
		}

		// move on to the next child
		currElem = xmlElems[currElem].nextSib;
	}

	return ptr;
}

//////////////////////////////////////////////////////////
//
// This routine will process attributes for a particular
// element.  It will write them out with the full name
// with (XML.attr) in front of the attribute name.  It 
// will then append the value of the attribute after an
// equal sign.  This should result in a display that
// looks like the following:
//
//  a.b.(XML.attr)c=value
//
//////////////////////////////////////////////////////////

char * CXMLParse::processAttrs(int elem, char *ptr, char *endPtr, BOOL includeRoot)

{
	int				foundAttr=0;		// found at least one attribute
	int				child;				// children of main element
	int				valueLen;			// length of the value
	char			*namePtr;			// pointer to name
	char			*valuePtr;			// pointer to value
	CRfhutilApp		*app;				// pointer to application object to locate trace routines
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if ((app->isTraceEnabled()) && (app->verboseTrace))
	{
		// get a pointer to the name and value
		namePtr = names.getNameAddr(xmlElems[elem].name);
		valuePtr = names.getNameAddr(xmlElems[elem].value);

		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::processAttrs elem=%d ptr=%8.8X endptr=%8.8X length=%d", elem, (unsigned int)ptr, (unsigned int)endPtr, endPtr-ptr);

		// trace entry to CXMLParse()::processAttrs
		app->logTraceEntry(traceInfo);
	}

	// process all children of this parent
	child = xmlElems[elem].firstChild;
	while ((child > 0) && (ptr < endPtr))
	{
		// is this an attribute?
		if ((xmlElems[child].type & TYPE_ATTR) == TYPE_ATTR)
		{
			// found an attribute
			// insert it
			// append the attribute name
			ptr = addName(child, ptr, endPtr, includeRoot);

			// get a pointer to the attribute value
			valuePtr = names.getNameAddr(xmlElems[child].value);
			valueLen = strlen(valuePtr);

			// make sure the name will fit
			if (ptr + valueLen + 5 < endPtr)
			{
				// insert an equal sign
				ptr[0] = '=';
				ptr++;

				// check if the value already has the quotes
				if (('\'' == valuePtr[0]) || ('\"' == valuePtr[0]))
				{
					// just copy the value
					strcpy(ptr, valuePtr);
					ptr += valueLen;
				}
				else
				{
					// insert a double quote
					ptr[0] = '\"';
					ptr++;

					// copy the value
					strcpy(ptr, valuePtr);
					ptr += valueLen;

					// insert the ending double quote
					ptr[0] = '\"';
					ptr++;
				}

				// insert a CRLF sequence
				ptr++[0] = '\r';
				ptr++[0] = '\n';
			}
		}

		// move on to the next child
		child = xmlElems[child].nextSib;
	}

	return ptr;
}

char * CXMLParse::addName(int elem, char * ptr, char * endPtr, BOOL includeRoot)

{
	// has the root element been reached?
	if (0 == elem)
	{
		// should the root name be included?
		if (includeRoot)
		{
			// append the name to the output
			ptr = appendName(elem, ptr, endPtr, includeRoot);
		}
	}
	else if (elem > 0)
	{
		// not the root
		// add the higher level first
		ptr = addName(xmlElems[elem].parent, ptr, endPtr, includeRoot);

		// append the name to the output
		ptr = appendName(elem, ptr, endPtr, includeRoot);
	}

	// return the next location in the output
	return ptr;
}

char * CXMLParse::appendName(int elem, char *ptr, char *endPtr, BOOL includeRoot)

{
	int			len;
	int			sibCount;
	int			sib;
	char *		namePtr;

	if (ptr < endPtr)
	{
		if ((elem != 0) && ((xmlElems[elem].parent > 0) || includeRoot))
		{
			// insert a period
			ptr[0] = '.';
			ptr++;
		}

		// get a pointer to the name
		namePtr = names.getNameAddr(xmlElems[elem].name);

		// is this an attribute?
		if ((xmlElems[elem].type & TYPE_ATTR) == TYPE_ATTR)
		{
			// insert (XML.attr)
			if (ptr + sizeof("(XML.attr)") < endPtr)
			{
				strcpy(ptr, "(XML.attr)");

				// update the pointer
				ptr += sizeof("(XML.attr)") - 1;
			}
		}

		// is there room for the name
		len = strlen(namePtr);
		if (ptr + len < endPtr)
		{
			// insert this name
			strcpy(ptr, namePtr);

			// account for the name
			ptr += len;
		}

		// is this an element or an attribute?
		if ((xmlElems[elem].type & TYPE_ATTR) != TYPE_ATTR)
		{
			// check if there are more than one element
			// check previous siblings for a match
			sibCount = 0;

			// check previous siblings first
			sib = xmlElems[elem].prevSib;
			while (sib != 0)
			{
				// is the name the same?
				if ((xmlElems[sib].name == xmlElems[elem].name) && ((xmlElems[sib].type & TYPE_ATTR) != TYPE_ATTR))
				{
					// found a previous sibling with the same name
					sibCount++;
				}

				// move on to the previous sibling
				sib = xmlElems[sib].prevSib;
			}

			// were previous siblings found?
			if (sibCount > 0)
			{
				// insert an index
				sprintf(ptr, "[%d]", sibCount + 1);

				// skip the inserted characters
				while (ptr[0] != ']')
				{
					// move on to the next character
					ptr++;
				}

				// skip the ending bracket
				ptr++;
			}
			else
			{
				// check following siblings for a match
				// this is done to insert a [1] if there are more siblings
				sib = xmlElems[elem].nextSib;
				while (sib > 0)
				{
					if ((xmlElems[elem].name == xmlElems[sib].name) && ((xmlElems[sib].type & TYPE_ATTR) != TYPE_ATTR))
					{
						// found sibling with the same name
						sibCount++;

						// end the loop
						sib = 0;
					}
					else
					{
						// move on to the next sibling
						sib = xmlElems[sib].nextSib;
					}
				}

				// were any siblings with the same name found?
				if (sibCount > 0)
				{
					// insert an index of 1
					strcpy(ptr, "[1]");
					ptr += 3;
				}
			}
		}
	}

	// return the next location in the output
	return ptr;
}

////////////////////////////////////////////
//
// Create a parse tree from formatted text.
//
// Input is in name=value format string.
// Each level in the name is separated by
// a period and each name=value pair is on
// a separate line.
//
// An attribute name is preceded by the
// characters (XML.attr).  If there is more
// than one occurence of an element then
// there will be a square bracket to
// indicate the index of this occurence.
//
////////////////////////////////////////////

int CXMLParse::parseFormattedText(const char *input, const char * rootName)

{
	int				result=PARSE_OK;	// return code
	int				numPeriods=0;		// number of periods
	int				numLines=1;			// number of NL characters
	int				numParens=0;		// number of parentheses
	int				parent;
	const char *	endLine;
	const char *	ptr;
	CRfhutilApp *	app;				// pointer to application object
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::parseFormattedText strlen(input)=%d rootName=%s", strlen(input), rootName);

		// trace entry to CXMLParse()::parseFormattedText
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the input
			app->dumpTraceData("Formatted input", (unsigned char *) input, strlen(input));
		}
	}

	// calculate the number of possible table entries
	ptr = input;
	while (ptr[0] != 0)
	{
		if ('.' == ptr[0])
		{
			// count the number of periods
			numPeriods++;
		}
		else if ('\n' == ptr[0])
		{
			// count the number of lines
			numLines++;
		}
		else if ('(' == ptr[0])
		{
			// count the number of equal signs (attributes)
			numParens++;
		}

		// move on to the next character
		ptr++;
	}

	// allocate storage for the parse tree
	xmlElems = (XMLSTRUCT *)rfhMalloc((numPeriods + numLines + numParens + 1) * sizeof(XMLSTRUCT), "XMLELEM2");

	// check if the malloc worked
	if (NULL == xmlElems)
	{
		// failed - just get out
		return PARSE_MALLOC_FAIL;
	}

	// initialize the storage
	memset(xmlElems, 0, (numPeriods + numLines + numParens + 1) * sizeof(XMLSTRUCT));

	// create a root element
	parent = insertEntry(rootName, strlen(rootName), 0, TYPE_ELEM);

	// enter the main loop 
	ptr = input;
	while (ptr[0] != 0)
	{
		// find the end of the line
		endLine = ptr;
		while ((endLine[0] != 0) && (endLine[0] != '\r') && (endLine[0] != '\n'))
		{
			endLine++;
		}

		// now process the line
		processLine(ptr, endLine);

		// move on to the next line
		ptr = endLine;
		while ((ptr[0] != 0) && (('\r' == ptr[0]) || ('\n' == ptr[0])))
		{
			// skip the CRLF sequence
			ptr++;
		}
	}

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::parseFormattedText numPeriods=%d numLines=%d numParens=%d numElements=%d result=%d", numPeriods, numLines, numParens, numElements, result);

		// trace exit from CXMLParse()::parseFormattedText
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the table entries
			app->dumpTraceData("XML parse tree", (unsigned char *) xmlElems, numElements * sizeof(XMLSTRUCT));

			// dump out the names as well
			dumpNames();
		}
	}

	return result;
}

int CXMLParse::parseRootText(const char *input, int length)

{
	int				result=PARSE_OK;	// return code
	int				numPeriods=0;		// number of periods
	int				numLines=1;			// number of NL characters
	const char *	endLine;
	const char *	ptr;	
	CRfhutilApp *	app;
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::parseRootText length=%d", length);

		// trace entry to CXMLParse()::parseRootText
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the input
			app->dumpTraceData("Parsed input", (unsigned char *) input, length);
		}
	}

	// calculate the number of possible table entries
	ptr = input;
	while (ptr[0] != 0)
	{
		if ('.' == ptr[0])
		{
			// count the number of periods
			numPeriods++;
		}
		else if ('\n' == ptr[0])
		{
			numLines++;
		}

		// move on to the next character
		ptr++;
	}

	// allocate storage for the parse tree
	xmlElems = (XMLSTRUCT *)rfhMalloc((numPeriods + numLines + 1) * sizeof(XMLSTRUCT), "XMLELEM3");

	// check if the malloc worked
	if (NULL == xmlElems)
	{
		// failed - just get out
		return PARSE_MALLOC_FAIL;
	}

	// initialize the storage
	memset(xmlElems, 0, (numPeriods + numLines + 1) * sizeof(XMLSTRUCT));

	// enter the main loop 
	ptr = input;
	while (ptr[0] != 0)
	{
		// find the end of the line
		endLine = ptr;
		while ((endLine[0] != 0) && (endLine[0] != '\r') && (endLine[0] != '\n'))
		{
			endLine++;
		}

		// now process the line
		processLine(ptr, endLine);

		// move on to the next line
		ptr = endLine;
		while ((ptr[0] != 0) && (('\r' == ptr[0]) || ('\n' == ptr[0])))
		{
			// skip the CRLF sequence
			ptr++;
		}
	}

	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::parseRootText numPeriods=%d numLines=%d numElements=%d result=%d", numPeriods, numLines, numElements, result);

		// trace exit from CXMLParse()::parseRootText
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the table entries
			app->dumpTraceData("XML parse tree", (unsigned char *) xmlElems, numElements * sizeof(XMLSTRUCT));

			// dump out the names as well
			dumpNames();
		}
	}

	return result;
}

////////////////////////////////////////////
//
// Generate XML from the parse tree.
//
// An output area for the XML is provided.
//
////////////////////////////////////////////

int CXMLParse::createXML(char *output, int maxLen)

{
	int				result=PARSE_OK;
	int				parent=0;
	char			*lastChar;
	CRfhutilApp *	app;
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::createXML maxLen=%d", maxLen);

		// trace entry to CXMLParse()::createXML
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the parse tree
			app->dumpTraceData("XML parse tree", (unsigned char *) xmlElems, numElements * sizeof(XMLSTRUCT));

			// dump out the names as well
			dumpNames();
		}
	}

	// initialize the output
	output[0] = 0;

	// check if there are any elements
	if (0 == numElements)
	{
		// nothing to create
		// simply return a zero length string
		return PARSE_NO_DATA;
	}

	lastChar = buildXML(0, output, output+maxLen);

	// terminate the string
	lastChar[0] = 0;

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::createXML strlen(output)=%d", strlen(output));

		// trace exit from CXMLParse()::createXML
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the input
			app->dumpTraceData("Output", (unsigned char *) output, strlen(output));
		}
	}

	return result;
}

//////////////////////////////////////////////////
//
// This routine will parse one line of a name and
// value representation of XML data.  This format
// is used to display the data in various folders.
//
//////////////////////////////////////////////////

void CXMLParse::processLine(const char *ptr, const char *endLine)

{
	int				elem;
	int				parent=0;
	const char *	name;				// pointer to beginning of name
	const char *	endName;			// pointer to first character after name (might be white space)
	const char *	valuePtr;			// pointer to value
	CRfhutilApp *	app;
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::processLine endLine-ptr=%d numElements=%d", endLine-ptr, numElements);

		// trace entry to CXMLParse()::processLine
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the parse tree
			app->dumpTraceData("input line", (unsigned char *) ptr, endLine - ptr);
		}
	}

	// find the first part of the name
	name = ptr;

	// find the end of the name
	endName = name;

	// check for a period
	if ('.' == name[0])
	{
		// move past the period
		name++;
	}

	// skip any blanks
	name = skipWhiteSpace(name, endLine);

	// search for the end of the whole name
	while ((endName < endLine) && (endName[0] != '=') && (endName[0] > ' '))
	{
		endName++;
	}

	// process the name, returning the final element number that is assigned
	// this processing may insert multiple levels of name into the table
	elem = processLongName(name, endName, parent);

	// check for a value
	valuePtr = endName;
	
	// search for an equal sign
	while ((valuePtr < endLine) && (valuePtr[0] != '='))
	{
		// move on to the next character
		valuePtr++;
	}

	if ((valuePtr < endLine) && ('=' == valuePtr[0]))
	{
		// skip the equal sign
		valuePtr++;
	}

	// check for a value
	if (valuePtr < endLine)
	{
		// the rest of the line should be the value
		addValue(elem, valuePtr, endLine - valuePtr);
	}

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::processLine numElements=%d", numElements);

		// trace exit from CXMLParse()::processLine
		app->logTraceEntry(traceInfo);
	}
}

int CXMLParse::processLongName(const char *name, const char *endName, int parent)

{
	int			elem=0;
	int			segments=0;
	int			attrs=0;
	const char	*ptr;
	CRfhutilApp *	app;
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::processLongName endName-name=%d numElements=%d", endName-name, numElements);

		// trace entry to CXMLParse()::processLongName
		app->logTraceEntry(traceInfo);
	}

	// loop through all the parts of the name
	while (name < endName)
	{
		// point to the beginning of the name
		ptr = name;

		// find the end of the first part of the name
		while ((ptr < endName) && (ptr[0] != '.') && (ptr[0] != '=') && (ptr[0] != '['))
		{
			// move on to the next character
			ptr++;
		}

		// check if a name was found
		if ((ptr > name) && (name[0] > ' '))
		{
			// found a name
			// increment the trace count
			segments++;

			// check for an attribute
			if (('(' == name[0]) && (ptr > name + 3) && (memcmp(name, "(XML.attr)", 10) == 0))
			{
				// looks like an attribute - increment the trace count
				attrs++;

				// skip the (XML.attr) name prefix
				name += 10;
				ptr += 6;

				// skip the attribute name
				while ((ptr < endName) && (ptr[0] != '.') && (ptr[0] != '='))
				{
					// move on to the next character
					ptr++;
				}

				// get rid of any white space (just in case)
				name = skipBlanks(name);

				// the name pointer should now be pointing to the attribute name
				// create an entry for this name
				elem = insertEntry(name, ptr-name, parent, TYPE_ATTR | TYPE_SINGLE);
			}
			else
			{
				// not an attribute
				elem = findOrInsertName(name, ptr, parent, TYPE_ELEM);

				// update the parent
				parent = elem;
			}
		}

		// move past this segment
		name = ptr;

		// move on to the next name
		while ((name < endName) && (name[0] <= ' '))
		{
			// move on to the next character
			name++;
		}
		
		// check for index
		if ('[' == name[0])
		{
			// move past the begin bracket
			name++;

			// search for the closing bracket
			while ((name < endName) && (name[0] != ']'))
			{
				name++;
			}

			// check if a closing bracket was found
			if ((name < endName) && (']' == name[0]))
			{
				// skip the closing bracket
				name++;
			}
		}

		// check for a period or blanks
		while ((name < endName) && ('.' == name[0]) || (' ' == name[0]))
		{
			name++;
		}
	}

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::processLongName numElements=%d elem=%d parent=%d segments=%d attrs=%d", numElements, elem, parent, segments, attrs);

		// trace exit from CXMLParse()::processLongName
		app->logTraceEntry(traceInfo);
	}

	// return the final element that was added
	return elem;
}

///////////////////////////////////////////////
//
// This routine will look for a child that of
// a parent element that matches the name
// that is pointed to.  If the appropriate
// element is found then the element index 
// will be returned.  If no element is found
// then the element will be inserted as the
// last child of the parent element.
//
///////////////////////////////////////////////

int CXMLParse::findOrInsertName(const char *name, const char *endName, int parent, int type)

{
	int			i;						// work variable
	int			occ;					// offset of last child found with same name
	int			child;					// index of children of parent
	int			occurence=0;			// occurence of this item
	int			nameOfs=0;				// offset of name in the strings table
	int			inserted=0;				// set to 1 if name was inserted into the table - only used for trace
	bool		notDone;				// working variable to see if loop is finished
	const char	*occPtr;				// pointer to occurence number
	char		occNumber[16] = { 0 };			// occurence as ascii characters
	CRfhutilApp *	app;				// pointer to application object - used for trace
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::findOrInsertName endName-name=%d numElements=%d parent=%d type=%d name=%.*s", endName-name, numElements, parent, type, endName-name, name);

		// trace entry to CXMLParse()::findOrInsertName
		app->logTraceEntry(traceInfo);
	}

	// check for root element or an attribute or no children yet
	if ((-1 == parent) || ((type & TYPE_ATTR) == TYPE_ATTR) || (0 == xmlElems[parent].firstChild))
	{
		// check for root element
		if (-1 == parent)
		{
			// process the root element differently, since it has no parent
			if (0 == numElements)
			{
				// root element not inserted yet so insert it
				child = insertEntry(name, endName-name, 0, TYPE_ELEM);

				// set trace indicator
				inserted=1;
			}
			else
			{
				// no need to insert anything
				child = 0;
			}
		}
		else
		{
			// always insert an attribute or first element of a parent
			child = insertEntry(name, endName-name, parent, type);

			// set trace indicator
			inserted=1;
		}

		// is trace enabled?
		if (app->isTraceEnabled())
		{
			// create the trace line
			sprintf(traceInfo, "Exiting CXMLParse()::findOrInsertName numElements=%d child=%d inserted=%d", numElements, child, inserted);

			// trace exit from CXMLParse()::findOrInsertName
			app->logTraceEntry(traceInfo);
		}

		// return the inserted entry
		return child;
	}

	// check for a specific occurence
	occPtr = skipBlanks(endName);

	if ('[' == occPtr[0])
	{
		// skip the bracket
		occPtr++;

		// skip any whitespace
		occPtr = skipBlanks(occPtr);

		// clear the memory
		memset(occNumber, 0, sizeof(occNumber));

		// isolate the occurence in a work area
		i = 0;
		while ((occPtr[0] >= '0') && (occPtr[0] <= '9') && (i < sizeof(occNumber)))
		{
			occNumber[i++] = occPtr[0];
			occPtr++;
		}

		// convert the string to an integer
		occurence = atoi(occNumber);

		// skip the closing bracket
		occPtr++;
	}

	// check if this is the lowest level name segment
	if (occPtr[0] != '.')
	{
		// no more lower levels - always add this name
		child = insertEntry(name, endName-name, parent, type);

		// set trace indicator
		inserted=1;
	}
	else
	{
		// get the name into the strings table
		nameOfs = names.findName(name, endName - name);

		// see if this parent already has this occurence of this child of type element
		child = xmlElems[parent].firstChild;
		occ = 0;
		notDone = true;
		while ((child > 0) && notDone)
		{
			// ignore attributes of the same name
			if ((xmlElems[child].name == nameOfs) && ((xmlElems[child].type & TYPE_ATTR) != TYPE_ATTR))
			{
				// check if this is the proper occurence
				if (occ >= occurence)
				{
					// found the right entry
					notDone = false;
				}
				else
				{
					// increment the number of occurences that have been found so far
					occ++;

					// move on to the next child
					child = xmlElems[child].nextSib;
				}
			}
			else
			{
				// move on to the next child
				child = xmlElems[child].nextSib;
			}
		}

		// was the element found?
		if (0 == child)
		{
			// not found so it must be inserted
			child = insertEntry(name, endName-name, parent, type);

			// count for trace
			inserted++;
		}
	}

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::findOrInsertName numElements=%d child=%d occurence=%d nameOfs=%d inserted=%d", numElements, child, occurence, nameOfs, inserted);

		// trace exit from CXMLParse()::findOrInsertName
		app->logTraceEntry(traceInfo);
	}

	return child;
}

const char * CXMLParse::processLineAttrs(int elem, const char *ptr, const char *endLine)

{
	int			count=0;		// counter used for trace purposes
	int			attr=0;			// index of the attribut in the parse tree
	int			attrType;		// attribute type
	const char	*attrName;		// pointer to attribute name
	const char	*attrNameEnd;	// end of name
	const char	*attrValue;		// pointer to attribute value
	const char	*attrValueEnd;	// end of value
	const char	*equalSign;		// equal sign between name and value
	char		quote;			// which parentheses to look for 
	CRfhutilApp *	app;
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CXMLParse()::processLineAttrs endLine-ptr=%d numElements=%d", endLine-ptr, numElements);

		// trace entry to CXMLParse()::processLineAttrs
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the parse tree
			app->dumpTraceData("attr line", (unsigned char *) ptr, endLine - ptr);
		}
	}

	// get a pointer to the first attribute, skipping the open parentheses
	attrName = ptr + 1;

	// continue until the last attribute is found
	while ((attrName < endLine) && (attrName[0] != 0) && (attrName[0] != '=') && (attrName[0] != ')'))
	{
		// skip any whitespace before the name
		attrName = skipWhiteSpace(attrName, endLine);

		// find the end of the name
		attrNameEnd = attrName;
		while ((attrNameEnd < endLine) && (attrNameEnd[0] != '=') && (attrNameEnd[0] != ')') && (attrNameEnd[0] > ' ') && (attrNameEnd[0] != 0))
		{
			// find the end of the name
			attrNameEnd++;
		}

		// find the value next
		attrValue = attrNameEnd;

		// skip any whitespace after the name
		attrValue = skipWhiteSpace(attrValue, endLine);

		// point to the beginning of the value
		equalSign = attrValue;

		// check if the equal sign was found
		if ((attrValue < endLine) && ('=' == attrValue[0]))
		{
			// increase the attribute count
			count++;

			// skip the equal sign and any whitespace after the equal sign
			attrValue = skipWhiteSpace(attrValue + 1, endLine);

			// check what type of quotes are used
			if ('\'' == attrValue[0])
			{
				// set the attribute type
				attrType = TYPE_ATTR | TYPE_SINGLE;
			}
			else if ('\"' == attrValue[0])
			{
				// set the attribute type
				attrType = TYPE_ATTR | TYPE_DOUBLE;
			}
			else
			{
				// set the attribute type
				attrType = TYPE_ATTR;
			}

			// append the attribute as a child of the parent
			attr = insertEntry(attrName, attrNameEnd - attrName, elem, attrType);

			// get the beginning of the value
			attrValueEnd = attrValue;

			// find the end of the value clause
			if ((attrValueEnd < endLine) && (('\"' == attrValueEnd[0]) || ('\'' == attrValueEnd[0])))
			{
				// get the proper quote character
				quote = attrValueEnd[0];
				attrValueEnd++;

				// search for the ending quote
				while ((attrValueEnd < endLine) && (attrValueEnd[0] != quote) && (attrValueEnd[0] != 0))
				{
					// go on to the next character
					attrValueEnd++;
				}

				// check if the ending quote was found
				if ((attrValueEnd < endLine) && (attrValueEnd[0] == quote))
				{
					// skip the trailing quote
					attrValueEnd++;
				}
			}
			else
			{
				// not a proper value since there are no quotes
				// check for a value immediately after the equal sign
				if (('=' == equalSign[0]) && (equalSign[1] > ' '))
				{
					// looks like a value right after the equal sign
					// assume the value ends with the first whitespace character
					attrValue = equalSign + 1;
					attrValueEnd = attrValue;
					while ((attrValueEnd < endLine) && (attrValueEnd[0] > ' ') && (attrValueEnd[0] != ')') && (attrValueEnd[0] != 0) && (attrValueEnd[0] != '='))
					{
						// find the end of the value
						attrValueEnd++;
					}
				}
			}

			// add the value
			addValue(attr, attrValue, attrValueEnd - attrValue);

			// move on to the next attribute
			attrName = attrValueEnd;
		}
		else
		{
			// not a valid attribute - skip it
			attrName = attrValue;
		}

		// skip any whitespace before the name
		while ((attrName < endLine) && (attrName[0] <= ' '))
		{
			// skip whitespace
			attrName++;
		}
	}

	if ((attrName < endLine) && (')' == attrName[0]))
	{
		// skip the closing parentheses
		attrName++;
	}

	// skip any whitespace
	while ((attrName < endLine) && (attrName[0] <= ' ') && (attrName[0] != 0))
	{
		// skip the whitespace character
		attrName++;
	}

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::processLineAttrs numElements=%d count=%d attr=%d", numElements, count, attr);

		// trace exit from CXMLParse()::processLineAttrs
		app->logTraceEntry(traceInfo);
	}

	return attrName;
}

char * CXMLParse::buildXML(int elem, char *output, char *endOut)

{
	int			nameLen;
	int			valueLen;
	int			child;
	const char	*namePtr;
	const char	*valuePtr;

	// check if there is room in the output
	if (output < endOut)
	{
		// add the begin bracket
		output[0] = '<';
		output++;

		// get a pointer to the name
		namePtr = names.getNameAddr(xmlElems[elem].name);

		// get the length of the name
		nameLen = strlen(namePtr);

		// check if the name will fit in the output area
		if (output + nameLen < endOut)
		{
			// append the name
			memcpy(output, namePtr, nameLen);
			output += nameLen;

			// process any attributes for this element
			output = buildXMLattrs(xmlElems[elem].firstChild, output, endOut);

			// check if the bracket will fit
			if (output < endOut)
			{
				// append the end bracket
				output[0] = '>';
				output++;

				// check if there is a value
				if (xmlElems[elem].value > 0)
				{
					// get a pointer to the value and the length of the value
					valuePtr = names.getNameAddr(xmlElems[elem].value);
					valueLen = strlen(valuePtr);

					// check if the value will fit
					if (output + valueLen < endOut)
					{
						// copy the value to the output
						output += copyXMLData(output, endOut, valuePtr, valueLen);
					}
				}

				// now process any child elements
				child = xmlElems[elem].firstChild;
				while (child > 0)
				{
					// check if this is an element or attribute
					if (TYPE_ELEM == xmlElems[child].type)
					{
						// process this element
						output = buildXML(child, output, endOut);
					}

					// move on to the next child
					child = xmlElems[child].nextSib;
				}

				// make sure the closing tag will fit
				if (output + nameLen + 3 < endOut)
				{
					// append the closing tag
					output[0] = '<';
					output[1] = '/';
					memcpy(output + 2, namePtr, nameLen);
					output += nameLen + 2;

					// append the closing bracket
					output[0] = '>';
					output++;
				}
			}
		}
	}

	return output;
}

////////////////////////////////////////////////
//
// routine to copy character data and produce
// parsed character data (PCDATA)
//
////////////////////////////////////////////////

int CXMLParse::copyXMLData(char *output, char *endOut, const char *valuePtr, int valueLen)

{
	int				i=0;				// index variable
	char			*ptr;				// pointer to the next output location
	CRfhutilApp *	app;				// pointer to MFC application object
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// initialize the output pointer
	ptr = output;

	// copy the data from the input to the output
	while ((ptr < endOut) && (i < valueLen))
	{
		switch (valuePtr[i])
		{
		case '&':
			{
				if (ptr + 5 < endOut)
				{
					strcpy(ptr, "&amp;");
					ptr += 5;
				}

				break;
			}
		case '<':
			{
				if (ptr + 4 < endOut)
				{
					strcpy(ptr, "&lt;");
					ptr += 4;
				}

				break;
			}
		case '>':
			{
				if (ptr + 4 < endOut)
				{
					strcpy(ptr, "&gt;");
					ptr += 4;
				}

				break;
			}
		case '\'':
			{
				if (ptr + 6 < endOut)
				{
					strcpy(ptr, "&apos;");
					ptr += 6;
				}

				break;
			}
		case '\"':
			{
				if (ptr + 6 < endOut)
				{
					strcpy(ptr, "&quot;");
					ptr += 6;
				}

				break;
			}
		default:
			{
				ptr++[0] = valuePtr[i];
				break;
			}
		}

		// increment the input index variable
		i++;
	}

	// terminate the string
	ptr[0] = 0;

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CXMLParse()::copyXMLData returned %d valueLen=%d endOut-output=%d output=%.256s", ptr - output, valueLen, endOut-output, output);

		// trace exit from CXMLParse()::copyXMLData
		app->logTraceEntry(traceInfo);
	}

	// return the number of bytes used in the output area
	return ptr - output;
}

char * CXMLParse::buildXMLattrs(int elem, char *output, char *endOut)

{
	int			nameLen;
	int			valueLen;
	const char	*name;
	const char	*value;
	char		quote;

	while (elem > 0)
	{
		// is this an attribute?
		if ((xmlElems[elem].type & TYPE_ATTR) == TYPE_ATTR)
		{
			// found an attribute
			// get the name of the attribute and the length
			name = names.getNameAddr(xmlElems[elem].name);
			nameLen = strlen(name);

			// get the value and the length of the value
			value = names.getNameAddr(xmlElems[elem].value);
			valueLen = strlen(value);

			// check if there is room for the attribute in the output area
			if (output + nameLen + valueLen + 4 < endOut)
			{
				// create the output
				// insert a blank
				output[0] = ' ';
				output++;

				// copy the name
				memcpy(output, name, nameLen);
				output += nameLen;

				// insert an equal sign and quote
				output++[0] = '=';

				// check for a quote in the value
				if ((value[0] != '\'') && (value[0] != '\"'))
				{
					// check the type of quotes
					if ((xmlElems[elem].type & TYPE_DOUBLE) == TYPE_DOUBLE)
					{
						quote = '\"';
					}
					else
					{
						// use single quotes
						quote = '\'';
					}

					// value does not have quotes
					output++[0] = quote;

					// copy the attribute data
					output += copyXMLData(output, endOut-1, value, valueLen);

					// insert the trailing quote
					output++[0] = quote;
				}
				else
				{
					// copy the quote from the value
					output++[0] = value[0];

					// check for the same quote value at the end of the data
					if (value[0] == value[valueLen - 1])
					{
						// append the value
						output += copyXMLData(output, endOut-1, value + 1, valueLen - 2);

						// copy the final character from the value
						output++[0] = value[valueLen - 1];
					}
					else
					{
						// the quote is missing from the end of the data
						// use the one from the beginninf of the value
						output += copyXMLData(output, endOut-1, value + 1, valueLen - 1);

						// copy the quote from the first character of the value
						output++[0] = value[0];
					}
				}
			}
		}

		// move on to the next element
		elem = xmlElems[elem].nextSib;
	}

	return output;
}

int CXMLParse::getFirstChild(int elem)

{
	int		firstChild=0;

	// check if there is a parse tree
	if ((elem < numElements) && (elem >= 0) && (xmlElems != NULL))
	{
		// get the first child of the given element
		firstChild = xmlElems[elem].firstChild;
	}

	return firstChild;
}

int CXMLParse::getNextSibling(int elem)

{
	int		nextSib=0;

	// check if there is a parse tree
	if ((elem < numElements) && (elem >= 0) && (xmlElems != NULL))
	{
		// get the next sibling of the given element
		nextSib = xmlElems[elem].nextSib;
	}

	return nextSib;
}

const char * CXMLParse::getElemName(int elem)

{
	const char *	name="";

	// check if there is a parse tree
	if ((elem < numElements) && (elem >= 0) && (xmlElems != NULL))
	{
		// point to the name as a string
		name = names.getNameAddr(xmlElems[elem].name);
	}

	return name;
}

const char * CXMLParse::getElemValue(int elem)

{
	const char *	value="";

	// check if there is a parse tree
	if ((elem < numElements) && (elem >= 0) && (xmlElems != NULL))
	{
		// point to the value as a string
		value = names.getNameAddr(xmlElems[elem].value);
	}

	return value;
}

int CXMLParse::getElemType(int elem)

{
	int		type=0;

	// check if there is a parse tree
	if ((elem < numElements) && (elem >= 0) && (xmlElems != NULL))
	{
		// get the element type
		type = xmlElems[elem].type & TYPE_ATTR;
	}

	return type;
}

///////////////////////////////////////
//
// This routine looks for an ampersand
// in the data.  If it is found then
// there are probably escape sequences
// in the data.
//
///////////////////////////////////////

BOOL CXMLParse::foundEscape(const char *start, const char *end)

{
	BOOL	found=FALSE;

	// loop through the value searching for an ampersand
	while ((start < end) && !found)
	{
		// is this character an ampersand?
		if ('&' == start[0])
		{
			// set the result and end the loop
			found = TRUE;
		}
		else
		{
			// not an ampersand - move on to the next character
			start++;
		}
	}

	return found;
}

void CXMLParse::dumpNames()

{
	int				ofs;
	CRfhutilApp *	app;				// pointer to application object
	const char *	ptr;
	char			traceInfo[512];

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// dump out the names table
		ofs = names.getFirstEntry();
		ptr = names.getNameAddr(ofs);
		while (ptr[0] != 0)
		{
			// build a trace line
			sprintf(traceInfo, "%8.8X %.384s", ofs, ptr);

			// write the trace line
			app->logTraceEntry(traceInfo);

			// move on to the next name
			ofs = names.getNextEntry(ofs);
			ptr = names.getNameAddr(ofs);
		}
	}
}

const char * CXMLParse::getErrorMsg(int returnCode)

{
	const char *	ptr = "Unknown error";

	// set a pointer to the error message
	switch (returnCode)
	{
	case PARSE_OK:
		{
			ptr = "";
			break;
		}
	case PARSE_MALLOC_FAIL:
		{
			ptr = "Memory allocation failed";
			break;
		}
	case PARSE_OBJ_CREATE_FAIL:
		{
			ptr = "Object creation failed";
			break;
		}
	case PARSE_NO_DATA:
		{
			ptr = "No data found";
			break;
		}
	case PARSE_TAG_MISMATCH:
		{
			ptr = "End tag does not match";
			break;
		}
	case PARSE_WRONG_LEVEL:
		{
			ptr = "Nesting error";
			break;
		}
	case PARSE_START_BRACKET:
		{
			ptr = "Start bracket not found";
			break;
		}
	case PARSE_INVALID_START_CHAR:
		{
			ptr = "Invalid character before begin bracket";
			break;
		}
	case PARSE_BIN_ZERO:
		{
			ptr = "Null character found in data";
			break;
		}
	case PARSE_BAD_CHARS:
		{
			ptr = "Invalid characters in data";
			break;
		}
	}

	return ptr;
}
