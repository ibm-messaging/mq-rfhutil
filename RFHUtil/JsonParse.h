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

// JsonParse.h: interface for the CJsonParse class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_JSONPARSE_H__9ACBEA50_327A_4A3F_A2CF_34B19523643D__INCLUDED_)
#define AFX_JSONPARSE_H__9ACBEA50_327A_4A3F_A2CF_34B19523643D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// return codes
#define JPARSE_OK					0
#define JPARSE_MALLOC_FAIL			1
#define JPARSE_INVALID_START_CHAR	200

struct JSONSTRUCT
{
	int		parent;
	int		firstChild;
	int		lastChild;
	int		nextSib;
	int		prevSib;
	int		value;
	int		name;
	int		filler;				// unused
};

class CJsonParse  
{
public:
	void buildParsedArea(char * output, int maxLen, BOOL includeRoot);
	void getLastError(char *errtxt, int maxLen);
	int getNextSibling(int elem);
	int getFirstChild(int elem);
	const char * getElemValue(int elem);
	const char * getElemName(int elem);
	int parse(const char *jsonIn, int length);
	CJsonParse();
	virtual ~CJsonParse();

private:
	const char * processArray(const char * ptr, const char * endPtr, int name, const char * namePtr, int nameLen, int parent);
	char * processElement(int elem, char *ptr, char *endPtr, BOOL includeRoot);
	const char * parseLevel(const char * ptr, const char * endPtr, int parent);
	void dumpNames();
	char * appendName(int elem, char *ptr, char *endPtr, BOOL includeRoot);
	void addValue(int elem, const char *value, int valueLen);
	char * addName(int elem, char * ptr, char * endPtr, BOOL includeRoot);
	int insertEntry(const char * name, int nameLen, int parent);
	int parseJSON(const char *jsonIn, int length);
	JSONSTRUCT * jsonElems;
	int numElements;
	CString lastError;
	Names	names;
};

#endif // !defined(AFX_JSONPARSE_H__9ACBEA50_327A_4A3F_A2CF_34B19523643D__INCLUDED_)
