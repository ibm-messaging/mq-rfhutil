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

// XMLParse.h: interface for the CXMLParse class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLPARSE_H__267E97FC_5EFA_42CD_A566_C36E99434214__INCLUDED_)
#define AFX_XMLPARSE_H__267E97FC_5EFA_42CD_A566_C36E99434214__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// defines for type of entry
#define		TYPE_ELEM	0
#define		TYPE_ATTR	1
#define		TYPE_SINGLE	16
#define		TYPE_DOUBLE	32

// return codes
#define PARSE_OK					0
#define PARSE_MALLOC_FAIL			1
#define PARSE_OBJ_CREATE_FAIL		2
#define PARSE_NO_DATA				3
#define PARSE_TAG_MISMATCH			100
#define PARSE_WRONG_LEVEL			101
#define PARSE_START_BRACKET			102
#define PARSE_INVALID_START_CHAR	200
#define PARSE_BIN_ZERO				201
#define PARSE_BAD_CHARS				202

struct XMLSTRUCT
{
	int		parent;
	int		firstChild;
	int		lastChild;
	int		nextSib;
	int		prevSib;
	int		type;
	int		value;
	int		name;
};

class CXMLParse 
{
public:
	int lastErrorOffset;
	const char * getErrorMsg(int returnCode);
	int getElemType(int elem);
	const char * getElemValue(int elem);
	const char * getElemName(int elem);
	int getNextSibling(int elem);
	int getFirstChild(int elem);
	int parseRootText(const char * input, int length);
	int createXML(char * output, int maxLen);
	int parseFormattedText(const char * input, const char * rootName);
	int parse(const char * xmlIn, int length);
	void buildParsedArea(char * output, int maxLen, BOOL includeRoot);
	void getLastError(char * errtxt, int maxLen);
	CXMLParse();
	virtual ~CXMLParse();

private:
	int findOrInsertName(const char * name, const char * endName, int parent, int type);
	int processLongName(const char * name, const char * endName, int parent);
	const char * skipComment(const char *ptr, const char *endPtr);
	const char * processQuote(const char *ptr, const char *endPtr, char quote);
	const char * processDTD(const char *ptr, const char *endPtr);
	void dumpNames();
	void convertAddValue(int elem, const char * value, int length);
	BOOL foundEscape(const char * start, const char * end);
	int copyXMLData(char * output, char * endOut, const char * valuePtr, int valueLen);
	char * processAttrs(int elem, char * ptr, char * endPtr, BOOL includeRoot);
	const char * processLineAttrs(int elem, const char * ptr, const char * endLine);
	char * appendName(int elem, char * ptr, char * endPtr, BOOL includeRoot);
	char * buildXMLattrs(int elem, char *output, char *endOut);
	char * buildXML(int elem, char * output, char * endOut);
	void processLine(const char * ptr, const char * endLine);
	char * addName(int elem, char * ptr, char * endPtr, BOOL includeRoot);
	char * processElement(int elem, char * ptr, char * endPtr, BOOL includeRoot);
	void addValue(int elem, const char * value, int valueLen);
	int insertEntry(const char * name, int nameLen, int parent, int type);
	int parseXML(const char * xmlIn, int length);

	XMLSTRUCT * xmlElems;
	int numElements;
	CString lastError;
	Names	names;
};

#endif // !defined(AFX_XMLPARSE_H__267E97FC_5EFA_42CD_A566_C36E99434214__INCLUDED_)
