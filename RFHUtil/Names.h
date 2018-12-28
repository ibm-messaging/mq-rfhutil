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

// Names.h: interface for the Names class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NAMES_H__85708852_C98B_4132_8CB4_4D13E2B4866B__INCLUDED_)
#define AFX_NAMES_H__85708852_C98B_4132_8CB4_4D13E2B4866B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define INITNAMETABLESIZE	64 * 1024

#define NAMEINDEXMAX		16

typedef struct 
{
	int		firstName;
	int		lastName;
} NAMEINDEX;

class Names  
{
public:
	int findName(const char *name, int length);
	static const char * getNextName(const char * namePtr);
	const char * getNextNamePtr(const char * namePtr);
	int getReallocCount();
	int getInsertCount();
	int insertName(const char * name, int length);
	int getNextEntry(int ofs);
	int getFirstEntry();
	int getNameTableSize();
	void dumpNames(FILE *df);
	char * getNameAddr(int namePtr);
	int insertName(const char * name);
	int findName(const char * name);
	Names();
	Names(int initSize);
	virtual ~Names();

private:
	int calcHash(const unsigned char * name);
	void initialize(int initSize);
	char * AllocateTable(int size);
	void ReleaseTable();

	int reallocCount;
	int insertCount;
	int nameTableSize;
	int	nextEntry;
	char * nameTable;
	NAMEINDEX nameIdx[NAMEINDEXMAX];
};

#endif // !defined(AFX_NAMES_H__85708852_C98B_4132_8CB4_4D13E2B4866B__INCLUDED_)
