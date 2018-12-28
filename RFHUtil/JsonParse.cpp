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

// JsonParse.cpp: implementation of the CJsonParse class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"
#include "JsonParse.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CJsonParse::CJsonParse()
{
	// do some initialization
	numElements = 0;
	jsonElems = NULL;
}

CJsonParse::~CJsonParse()
{
	// was a parse tree allocated?
	if (jsonElems != NULL)
	{
		// free the parse tree
		rfhFree(jsonElems);
	}
}

int CJsonParse::parse(const char *jsonIn, int length)

{
	int				result=JPARSE_OK;	// result - TRUE means parse worked
	int				numBrackets=0;		// number of begin brackets found
	int				numEnds=0;			// number of end brackets found
	int				numColons=0;		// number of equal signs
	int				numCommas=0;		// number of commas
	int				numEscQuotes=0;		// number of escaped quotes
	int				idx;				// index within xml input
	size_t			memSize=0;			// size of memory to allocate
	const char *	ptr;				// work pointer
	CRfhutilApp *	app;				// pointer to application object to locate trace routines
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CJsonParse()::parse jsonIn=%8.8X length=%d", (unsigned int)jsonIn, length);

		// trace entry to CJsonParse::parse
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the input
			app->dumpTraceData("Json parse input", (unsigned char *) jsonIn, length);
		}
	}

	// point to the JSON data
	ptr = jsonIn;
	idx = 0;

	// count the number of begin brackets ('{') and colons (':')
	while ((idx < length) && (ptr[idx] != 0))
	{
		// check for double quotes, to ignore escaped characters
		if ('\"' == ptr[idx])
		{
			// find the trailing double quote
			while ((idx < length) && (ptr[idx] != 0) && (ptr[idx] != '\"'))
			{
				// check for escape character
				if (('\\' == ptr[idx]) && ('\"' == ptr[idx + 1]))
				{
					// found escaped double quote
					// skip both characters
					idx += 2;
					numEscQuotes++;
				}
				else
				{
					// move on to next character
					idx++;
				}
			}
		}
		else
		{
			if ('{' == ptr[idx])
			{
				// count the number of brackets and colons
				// should be an element
				numBrackets++;
			}
			else if ('}' == ptr[idx])
			{
				// count the number of equal signs
				// could be an attribute
				numEnds++;
			}
			else if (':' == ptr[idx])
			{
				// count the number of equal signs
				// could be an attribute
				numColons++;
			}
			else if (',' == ptr[idx])
			{
				// count the number of equal signs
				// could be an attribute
				numCommas++;
			}
		}

		// move on to the next character
		idx++;
	}

	// allocate storage for an array of elements including a NULL root element
	memSize = (numBrackets + numColons + numCommas + 2) * sizeof(JSONSTRUCT);
	jsonElems = (JSONSTRUCT *)rfhMalloc(memSize, "JSONELEM");

	// did the malloc work?
	if (NULL == jsonElems)
	{
		// failed - just get out
		return JPARSE_MALLOC_FAIL;
	}

	// initialize the storage
	memset(jsonElems, 0, memSize);

	// parse the JSON
	result = parseJSON(jsonIn, length);

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CJsonParse()::parse result=%d numBrackets=%d numEnds=%d numColons=%d numCommas=%d numElements=%d numEscQuotes=%d", result, numBrackets, numEnds, numColons, numCommas, numElements, numEscQuotes);

		// trace exit from CJsonParse::parse
		app->logTraceEntry(traceInfo);

		// check for verbose trace
		if (app->verboseTrace)
		{
			// dump out the table entries
			app->dumpTraceData("JSON parse tree", (unsigned char *) jsonElems, numElements * sizeof(JSONSTRUCT));

			// dump out the names as well
			dumpNames();
		}
	}

	return result;
}

int CJsonParse::parseJSON(const char *jsonIn, int length)

{
	int				result=JPARSE_OK;	// result - TRUE means parse worked
	int				root;				// root element
	const char *	ptr;				// working pointer
	const char *	endPtr;				// end of the data
	CRfhutilApp *	app;				// pointer to application object to locate trace routines
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CJsonParse()::parseJSON length=%d", length);

		// trace entry to CXMLParse()::parseJSON
		app->logTraceEntry(traceInfo);
	}

	// point to the JSON data to be parsed
	ptr = jsonIn;

	// calculate the end of the data
	endPtr = jsonIn + length;

	// find the first meaningful character
	ptr = skipWhiteSpace(ptr, endPtr);

	// make sure it is a begin bracket
	if ((ptr >= endPtr) || (ptr[0] != '{'))
	{
		// set an error message
		lastError  = "Parse error - First character not a brace";

		// either the input has no real data or the first character is not a brace character
		return JPARSE_INVALID_START_CHAR;
	}

	// create a root element
	root = insertEntry("root", 4, 0);

	// start the parse process, starting with the top level element
	parseLevel(ptr, endPtr, root);

	return result;
}

const char * CJsonParse::parseLevel(const char *ptr, const char *endPtr, int parent)

{
	int				name;
	const char *	namePtr;				// pointer to element name
	const char *	endNamePtr;				// pointer to end of name
	const char *	valuePtr;				// pointer to value
	const char *	endValuePtr;			// pointer to end of value
	CRfhutilApp *	app;				// pointer to application object to locate trace routines
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CJsonParse()::parseLevel ptr=%8.8X endPtr=%8.8X parent=%d numElements=%d", (unsigned int)ptr, (unsigned int)endPtr, parent, numElements);

		// trace entry to CXMLParse()::parseLevel
		app->logTraceEntry(traceInfo);
	}

	// skip the begin brace
	ptr++;

	// continue until an ending brace character is found
	while ((ptr < endPtr) && (ptr[0] != '}'))
	{
		// find the beginning of the name
		ptr = skipWhiteSpace(ptr, endPtr);

		// remember the beginning of the name
		namePtr = ptr;
		endNamePtr = ptr;

		// check if the name is enclosed in double quotes
		if ((ptr < endPtr) && ('\"' == ptr[0]))
		{
			// skip the double quote character
			ptr++;
			namePtr++;
			endNamePtr++;

			// find the end of the name
			while ((ptr < endPtr) && (ptr[0] != '\"'))
			{
				// move on to the next chararcter in the name
				ptr++;
			}

			// point to the end of the name
			endNamePtr = ptr;

			// was the end quote found?
			if ((ptr < endPtr) && ('\"' == ptr[0]))
			{
				// skip the quote character
				ptr++;
			}
		}
		else
		{
			// just search for the end of the name
			while ((ptr < endPtr) && (ptr[0] > ' ') && (ptr[0] != ':') && (ptr[0] != '[') && (ptr[0] != ']') && (ptr[0] != '}'))
			{
				// move on to the next character
				ptr++;
			}

			// point to the end of the name
			endNamePtr = ptr;
		}

		// skip any white space after the name
		ptr = skipWhiteSpace(ptr, endPtr);

		// was the value found?
		if (ptr < endPtr)
		{
			// create an entry for the name
			name = insertEntry(namePtr, endNamePtr - namePtr, parent);

			// check for a value delimiter (colon)
			if (':' == ptr[0])
			{
				// skip the colon
				ptr++;

				// skip any white space in front of the value
				ptr = skipWhiteSpace(ptr, endPtr);

				// make sure there is a value
				if (ptr < endPtr)
				{
					// what kind of value is this?
					if ('{' == ptr[0])
					{
						// process the object by moving down one lewel
						ptr = parseLevel(ptr, endPtr, name);
					}
					else if ('[' == ptr[0])
					{
						// process the array
						ptr = processArray(ptr, endPtr, name, namePtr, endNamePtr-namePtr, parent);
					}
					else 
					{
						// remember the beginning of the value
						valuePtr = ptr;

						// process the value - either a string or a number
						// check for a string
						if ('\"' == ptr[0])
						{
							// this is a string value
							// skip the beginning quotation character
							ptr++;

							// look for the ending quotation character, skipping any escape characters
							while ((ptr < endPtr) && (ptr[0] != '\"'))
							{
								// check for an escape character
								if ('\\' == ptr[0])
								{
									// skip an extra character
									ptr++;
								}

								// skip over this character
								ptr++;
							}

							// was the ending quotation mark found?
							if ((ptr < endPtr) && ('\"' == ptr[0]))
							{
								// skip the trailing quotation mark
								ptr++;
							}

							// point to the end of the value
							endValuePtr = ptr;
						}
						else
						{
							// assume this is a number
							// the number can contain digits and a period and a plus or minus sign and the letter E (exponent)
							// the number is terminated when whitespace, a comma or a brace is found
							// check for a leading sign first
/*							if (('-' == ptr[0]) || ('+' == ptr[0]))
							{
								// skip the leading sign
								ptr++;
							}

							// find the end of the number
							while ((ptr < endPtr) && 
								   (ptr[0] > ' ') && 
								   (((ptr[0] >= '0') && (ptr[0] <= '9')) || ('.' == ptr[0]) || ('E' == ptr[0]) || ('e' == ptr[0])))
							{
								// move on to the next digit
								ptr++;
							}*/
							// this is a value that is not enclosed in quotes, so it should be a number
							// but just in case look for something that is whitespace or other characters
							// find the end of the number
							while ((ptr < endPtr) && 
								   (ptr[0] > ' ') && 
								   (ptr[0] != ',') && 
								   (ptr[0] != '[') && 
								   (ptr[0] != '}') && 
								   (ptr[0] != '{') && 
								   (ptr[0] != ']'))
							{
								// move on to the next digit
								ptr++;
							}

							// mark the end of the value
							endValuePtr = ptr;
						}

						// add the value to the element
						addValue(name, valuePtr, endValuePtr - valuePtr);

						// skip any whitespace characters
						ptr = skipWhiteSpace(ptr, endPtr);

						// look for the comma at the end of the value
						if ((ptr < endPtr) && (',' == ptr[0]))
						{
							// skip the comma and any white space
							ptr++;
							ptr = skipWhiteSpace(ptr, endPtr);
						}
					}
				}
			}
			else
			{
				// error - missing colon
			}
		}
	}

	// was a brace character found?
	if ((ptr < endPtr) && ('}' == ptr[0]))
	{
		// skip the trailing brace
		ptr++;
		ptr = skipWhiteSpace(ptr, endPtr);
	}

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CJsonParse()::parseLevel ptr=%8.8X numElements=%d name=%d namePtr=%8.8X endNamePtr=%8.8X", (unsigned int)ptr, numElements, name, (unsigned int)namePtr, (unsigned int)endNamePtr);

		// trace exit from CXMLParse()::parseLevel
		app->logTraceEntry(traceInfo);
	}

	// return the updated pointer
	return ptr;
}

int CJsonParse::insertEntry(const char *name, int nameLen, int parent)

{
	int				nameOfs=0;
	int				lastChild;
	CRfhutilApp *	app;				// pointer to application object to locate trace routines
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// check for a name
	if (nameLen > 0)
	{
		nameOfs = names.insertName(name, nameLen);
	}

	// capture the name
	jsonElems[numElements].name = nameOfs;

	// set the parent
	jsonElems[numElements].parent = parent;

	// get the last child of the parent
	lastChild = jsonElems[parent].lastChild;

	// insert as the new last child
	jsonElems[parent].lastChild = numElements;

	// are there any children of this parent?
	if (0 == lastChild)
	{
		// first child of this parent
		jsonElems[parent].firstChild = numElements;
	}
	else
	{
		// the last child is now the previous sibling of this element
		jsonElems[numElements].prevSib = lastChild;

		// the previous last child now has a next sibling
		jsonElems[lastChild].nextSib = numElements;
	}

	// move on to the next element
	numElements++;

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CJsonParse()::insertEntry numElements=%d parent=%d name=%s", numElements, parent, names.getNameAddr(nameOfs));

		// trace exit from CXMLParse()::insertEntry
		app->logTraceEntry(traceInfo);
	}

	return numElements - 1;
}

char * CJsonParse::addName(int elem, char *ptr, char *endPtr, BOOL includeRoot)

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
		ptr = addName(jsonElems[elem].parent, ptr, endPtr, includeRoot);

		// append the name to the output
		ptr = appendName(elem, ptr, endPtr, includeRoot);
	}

	// return the next location in the output
	return ptr;
}

void CJsonParse::addValue(int elem, const char *value, int valueLen)

{
	int		valueOfs=0;

	if (valueLen > 0)
	{
		valueOfs = names.insertName(value, valueLen);
	}

	jsonElems[elem].value = valueOfs;
}

char * CJsonParse::appendName(int elem, char *ptr, char *endPtr, BOOL includeRoot)

{
	int			len;
	int			sibCount;
	int			sib;
	char *		namePtr;

	if (ptr < endPtr)
	{
		if ((elem != 0) && ((jsonElems[elem].parent > 0) || includeRoot))
		{
			// insert a period
			ptr[0] = '.';
			ptr++;
		}

		// get a pointer to the name
		namePtr = names.getNameAddr(jsonElems[elem].name);

		// is there room for the name
		len = strlen(namePtr);
		if (ptr + len < endPtr)
		{
			// insert this name
			strcpy(ptr, namePtr);

			// account for the name
			ptr += len;
		}

		// check if there are more than one element
		// check previous siblings for a match
		sibCount = 0;

		// check previous siblings first
		sib = jsonElems[elem].prevSib;
		while (sib != 0)
		{
			// is the name the same?
			if (jsonElems[sib].name == jsonElems[elem].name)
			{
				// found a previous sibling with the same name
				sibCount++;
			}

			// move on to the previous sibling
			sib = jsonElems[sib].prevSib;
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
			sib = jsonElems[elem].nextSib;
			while (sib > 0)
			{
				if (jsonElems[elem].name == jsonElems[sib].name)
				{
					// found sibling with the same name
					sibCount++;

					// end the loop
					sib = 0;
				}
				else
				{
					// move on to the next sibling
					sib = jsonElems[sib].nextSib;
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

	// return the next location in the output
	return ptr;
}

const char * CJsonParse::getElemName(int elem)

{
	const char *	name="";

	// check if there is a parse tree
	if ((elem < numElements) && (elem >= 0) && (jsonElems != NULL))
	{
		// point to the name as a string
		name = names.getNameAddr(jsonElems[elem].name);
	}

	return name;
}

const char * CJsonParse::getElemValue(int elem)

{
	const char *	value="";

	// check if there is a parse tree
	if ((elem < numElements) && (elem >= 0) && (jsonElems != NULL))
	{
		// point to the value as a string
		value = names.getNameAddr(jsonElems[elem].value);
	}

	return value;
}

int CJsonParse::getFirstChild(int elem)

{
	int		firstChild=0;

	// check if there is a parse tree
	if ((elem < numElements) && (elem >= 0) && (jsonElems != NULL))
	{
		// get the first child of the given element
		firstChild = jsonElems[elem].firstChild;
	}

	return firstChild;
}

int CJsonParse::getNextSibling(int elem)

{
	int		nextSib=0;

	// check if there is a parse tree
	if ((elem < numElements) && (elem >= 0) && (jsonElems != NULL))
	{
		// get the next sibling of the given element
		nextSib = jsonElems[elem].nextSib;
	}

	return nextSib;
}

////////////////////////////////////////////////
//
// Dump out the contents of the names table
// which contains all string values, including
// both variable names and values.
//
// This routine is only used for debugging
// purposes.
//
////////////////////////////////////////////////

void CJsonParse::dumpNames()

{
	const char *	startName;			// beginning of names table
	CRfhutilApp *	app;				// pointer to application object
	const char *	ptr;
	char			traceInfo[512];

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// dump out the names table
		startName = names.getNameAddr(0);
		ptr = startName + 1;
		while (ptr[0] != 0)
		{
			// build a trace line
			sprintf(traceInfo, "%8.8X %.384s", ptr - startName, ptr);

			// write the trace line
			app->logTraceEntry(traceInfo);

			// move on to the next name
			ptr += strlen(ptr) + 9;
		}
	}
}

void CJsonParse::getLastError(char *errtxt, int maxLen)

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

void CJsonParse::buildParsedArea(char *output, int maxLen, BOOL includeRoot)

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
		sprintf(traceInfo, "Entering CJsonParse()::buildParsedArea maxLen=%d includeRoot=%d", maxLen, includeRoot);

		// trace entry to CJsonParse()::buildParsedArea
		app->logTraceEntry(traceInfo);
	}

	// initialize the output data to a null string
	output[0] = 0;

	// check if there is any data
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
		sprintf(traceInfo, "Exiting CJsonParse()::buildParsedArea length=%d", ptr-output);

		// trace exit from CJsonParse()::buildParsedArea
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

char * CJsonParse::processElement(int elem, char *ptr, char *endPtr, BOOL includeRoot)

{
	int				currElem;
	int				len;				// length of the value string
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
		namePtr = names.getNameAddr(jsonElems[elem].name);
		valuePtr = names.getNameAddr(jsonElems[elem].value);

		// create the trace line
		sprintf(traceInfo, "Entering CJsonParse()::processElement elem=%d parent=%d firstChild=%d lastChild=%d nextSib=%d prevSib=%d name=%.64s value=%.64s",
				elem, jsonElems[elem].parent, jsonElems[elem].firstChild, jsonElems[elem].lastChild, jsonElems[elem].nextSib, jsonElems[elem].prevSib, namePtr, valuePtr);

		// trace entry to CJsonParse()::processElement
		app->logTraceEntry(traceInfo);
	}

	// point to the first element
	currElem = jsonElems[elem].firstChild;

	// check for a value
	if (jsonElems[elem].value > 0)
	{
		// need to output this element
		ptr = addName(elem, ptr, endPtr, includeRoot);

		// insert an equal sign
		ptr[0] = '=';
		ptr++;

		// get a pointer to the value
		valuePtr = names.getNameAddr(jsonElems[elem].value);

		// get the length of the value
		len = strlen(valuePtr);

		// check if the value will fit
		if (ptr + len + 2 < endPtr)
		{
			// insert the value
			strcpy(ptr, valuePtr);

			// update the pointer
			ptr += len;

			// insert a CRLF sequence
			ptr++[0] = '\r';
			ptr++[0] = '\n';
		}
	}

	// process any children of this element
	currElem = jsonElems[elem].firstChild;
	while (currElem > 0)
	{
		// process the child element
		ptr = processElement(currElem, ptr, endPtr, includeRoot);

		// move on to the next child
		currElem = jsonElems[currElem].nextSib;
	}

	return ptr;
}

const char * CJsonParse::processArray(const char *ptr, const char *endPtr, int name, const char *namePtr, int nameLen, int parent)

{
	int				numValues=0;		// counter for trace output - number of values found
	int				firstTime=1;		// remember the first element
	int				name2;
	const char *	valuePtr;			// pointer to beginning of value
	const char *	endValuePtr;		// pointer to end of value
	CRfhutilApp		*app;				// pointer to application object to locate trace routines
	char			traceInfo[512];		// work area for trace line

	// get a pointer to the application object
	app = (CRfhutilApp *)AfxGetApp();

	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering CJsonParse()::processArray ptr=%8.8X name=%d namePtr=%s",
				(unsigned int)ptr, name, names.getNameAddr(jsonElems[name].name));

		// trace entry to CJsonParse()::processArray
		app->logTraceEntry(traceInfo);
	}

	// initialize the element number
	name2 = name;

	// skip the initial begin bracket
	ptr++;
	ptr = skipWhiteSpace(ptr, endPtr);

	while ((ptr < endPtr) && (ptr[0] != ']'))
	{
		// what kind of value is this?
		if ('\"' == ptr[0])
		{
			// normal string value
			// remember where the string starts
			valuePtr = ptr;

			// skip the beginning quote
			ptr++;

			// find the end of the value
			while ((ptr < endPtr) && (ptr[0] != '\"'))
			{
				// move on to the next character
				ptr++;
			}

			// was the closing quote found?
			if ((ptr < endPtr) && ('\"' == ptr[0]))
			{
				// skip the trailing quote
				ptr++;
			}

			// remember the end of the value
			endValuePtr = ptr;

			// capture the value
			// is this the first value?
			if (0 == firstTime)
			{
				// not the first - need to create another element
				name2 = insertEntry(namePtr, nameLen, parent);
			}
			else
			{
				// only skip once
				firstTime = 0;
			}

			// add the value to the element
			addValue(name2, valuePtr, endValuePtr - valuePtr);
		}
		else if ('{' == ptr[0])
		{
			// each occurence is an object
			// is this the first occurence?
			if (name != name2)
			{
				// not the first one - need to create a new element
				name2 = insertEntry(namePtr, nameLen, parent);
			}

			// process the object
			ptr = parseLevel(ptr, endPtr, name2);

			// check for a comma
			if (',' == ptr[0])
			{
				// skip the comma and any following whitespace
				ptr++;
				ptr = skipWhiteSpace(ptr, endPtr);
			}
		}
		else 
		{
			// hopefully this is a number
			// remember where the string starts
			valuePtr = ptr;

			// find the end of the value - either whitespace or a comma
			while ((ptr < endPtr) && (ptr[0] != ',') && (ptr[0] > ' '))
			{
				// move on to the next character
				ptr++;
			}

			// skip any whitespace
			ptr = skipWhiteSpace(ptr, endPtr);

			// was the closing comma found?
			if ((ptr < endPtr) && (',' == ptr[0]))
			{
				// skip the trailing quote
				ptr++;
			}

			// remember the end of the value
			endValuePtr = ptr;

			// capture the value
			// is this the first value?
			if (name2 != name)
			{
				// not the first - need to create another element
				name2 = insertEntry(namePtr, nameLen, parent);
			}

			// add the value to the element
			addValue(name2, valuePtr, endValuePtr - valuePtr);
		}

		// check for a comma
		if (',' == ptr[0])
		{
			// move on to the next character
			ptr++;
			ptr = skipWhiteSpace(ptr, endPtr);
		}
	}

	// was the ending square bracket found?
	if ((ptr < endPtr) && (']' == ptr[0]))
	{
		// skip the ending square bracket
		ptr++;
		ptr = skipWhiteSpace(ptr, endPtr);
	}

	// remember the first element
	// the first value will be associated with this element
	// new elements will be created for subsequent values
	name2 = name;

	// process each individual value
	// is trace enabled?
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CJsonParse()::processArray ptr=%8.8X numValues=%d", (unsigned int)ptr, numValues);

		// trace exit from CJsonParse()::processArray
		app->logTraceEntry(traceInfo);
	}

	// return the updated pointer
	return ptr;
}
