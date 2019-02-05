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

// Names.cpp: implementation of the Names class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include "comsubs.h"
#include "Names.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// length of the prefix area
// this consists of an offset in the table and a 4-byte integer string length
#define PREFIX_SIZE		8

#define DUMP_FILE_NAME "c:\\namedump.txt"

/////////////////////////////////////////////////////////////////
//
// Internal format of the names table
//
// The names table contains a number of null specified strings.
// Each unique string is denoted with a unique integer, which
// is the offset within memory to the string.  This allows the
// main table to be reallocated to allow for growth without
// invalidating the string handle.  
//
// Each string in the table has an 8-byte prefix.  The prefix 
// consists of a 4-byte forward pointer and a 4-byte string
// length field.  
//
/////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#pragma warning(suppress: 26495)
Names::Names()

{
	initialize(INITNAMETABLESIZE);
}

#pragma warning(suppress: 26495)
Names::Names(int initSize)

{
	initialize(initSize);
}

void Names::initialize(int initSize)

{
	// initialize the internal variables, in case the malloc fails
	nameTable = NULL;
	nameTableSize = 0;
	nextEntry = 1;
	insertCount=0;
	reallocCount=0;
	
	// try to allocate an initial name table
	nameTable = AllocateTable(initSize);
	if (nameTable != NULL)
	{
		// remember how much memory we allocated
		nameTableSize = initSize;
	}

	// initialize the names index entries
	memset(nameIdx, 0, NAMEINDEXMAX * sizeof(NAMEINDEX));
}

Names::~Names()

{
	// check if we allocated a name table
	ReleaseTable();

	nameTableSize = 0;
}

int Names::calcHash(const unsigned char * name)

{
	// calculate a hash based on the first character of the name
	return ((name[0] & 15) + (name[0] >> 4)) & 15;
}

/////////////////////////////////////////////////////////////////
//
// Look up a name in the name table, and if found, return the 
//  offset within the table.  If not found, return zero.  
//  Note that offset zero will return a pointer to a null string.
//
/////////////////////////////////////////////////////////////////

int Names::findName(const char *name, int length)

{
	int				idx;
	int				offset;
	char *			ptr;
	char *			endptr;

	// calculate the name hash
	idx = calcHash((const unsigned char *)name);
	
	// get the offset of the first entry
	offset = nameIdx[idx].firstName;

	// check if there are any names for this hash value
	if (0 == offset)
	{
		// no entries - just return a pointer to a zero length string
		return 0;
	}

	// get an anchor to the first entry for this name
	ptr = nameTable + offset;

	/* point to the beginning of the name table */
	endptr = nameTable + nextEntry;

	/* search for the name in the current table */
	while ((offset > 0) && (ptr < endptr) && ((memcmp(ptr + PREFIX_SIZE, name, length) != 0) || ((int)strlen(ptr + PREFIX_SIZE) != length)))
	{
		// get the next offset
		offset = (*(int *)ptr);
		ptr = nameTable + offset;
	}

	if ((ptr < endptr) && (offset > 0))
	{
		return ptr - nameTable + PREFIX_SIZE;
	}
	else
	{
		return 0;
	}
}

/////////////////////////////////////////////////////////////////
//
// Look up a name in the name table, and if found, return the 
//  offset within the table.  If not found, return zero.  
//  Note that offset zero will return a pointer to a null string.
//
/////////////////////////////////////////////////////////////////

int Names::findName(const char *name)

{
	// use the common routine
	return findName(name, strlen(name));
}

/////////////////////////////////////////////////////////////////
//
// Insert a name that is specified as a null-terminated string.
// This routine just calls the normal insertName routine.
//
/////////////////////////////////////////////////////////////////

int Names::insertName(const char *name)

{
	// get the length of the name and insert the name
	return insertName(name, strlen(name));
}

/////////////////////////////////////////////////////////////////
//
// Insert a name in the name table, first checking to see if it
//  already exists.  In either case return the offset within
//  the table as a unique key to the name.  The advantage of
//  using an offset is that the table itself can be expanded
//  and relocated without the caller knowing.
//
/////////////////////////////////////////////////////////////////

int Names::insertName(const char *name, int length)

{
	int				idx;
	int				namePtr;
	char			*ptr;
	char			*newTablePtr=NULL;

	// was a name provided?
	if ((0 == name[0]) | (0 == length))
	{
		// return a pointer to a null string
		return 0;
	}

	// see if the name is already in the name table
	namePtr = findName(name, length);

	// did we find the entry?
	if (0 == nameTable[namePtr])
	{
		// didn't find the entry, so insert the name
		// check if this will overflow the table
		if ((nextEntry + length + 16) > nameTableSize)
		{
			// check if our initial allocation failed
			if (0 == nameTableSize)
			{
				// make sure we can handle the case where the initial allocation failed
				// if we can't malloc 256 bytes we are in real trouble
				nameTable = AllocateTable(256);

				// did the malloc work?
				if (nameTable != NULL)
				{
					// set the table size
					nameTableSize = 256;
				}
			}

			// allocate a new names table that is twice the size
			newTablePtr = AllocateTable((nameTableSize * 2) + 36);

			if (newTablePtr != NULL)
			{
				// initialize the new table and copy the old one
				memcpy(newTablePtr, nameTable, nameTableSize);

				// release the current name table storage
				ReleaseTable();

				// update the size and pointer
				nameTableSize *= 2;
				nameTable = newTablePtr;

				// update statistics
				reallocCount++;
			}
		}

		// insert the new value into the table
		// calculate the name hash
		idx = calcHash((const unsigned char *)name);
	
		// check if this is the first entry for this hash
		if (0 == nameIdx[idx].firstName)
		{
			// first entry in table
			nameIdx[idx].firstName = nextEntry;
			nameIdx[idx].lastName = nextEntry;
		}
		else
		{
			// get a pointer to the previous entry for this hash value
			ptr = nameTable + nameIdx[idx].lastName;

			// not the first entry
			nameIdx[idx].lastName = nextEntry;

			// update the pointer in the previous entry
			memcpy(ptr, &(nextEntry), 4);
		}

		// copy the name into the table
		memcpy(nameTable + nextEntry + PREFIX_SIZE, name, length);

		// set the length
		memcpy(nameTable + nextEntry + PREFIX_SIZE - 4, &length, 4);

		// remember where we stored this name
		namePtr = nextEntry + PREFIX_SIZE;

		// move on to the next location in the table
		nextEntry += length + PREFIX_SIZE + 1;

		// keep statistics for diagnostic purposes
		insertCount++;
	}

	return namePtr;
}

/////////////////////////////////////////////////////////////////
//
// Get a pointer to the name within the name table.  The pointer
//  is only guaranteed to be good until more names are inserted
//  into the table.  The input is a key that was returned when
//  the name was inserted into the table.
//
/////////////////////////////////////////////////////////////////

char * Names::getNameAddr(int namePtr)

{
	return nameTable + namePtr;
}

/////////////////////////////////////////////////////////////////
//
// Return the key of the first item in the table.
//
/////////////////////////////////////////////////////////////////

int Names::getFirstEntry()

{
	// check if there are any entries in the table
	if (1 == nextEntry)
	{
		// return the first entry
		return 0;
	}

	// skip the first byte plus the forward pointer and length
	return PREFIX_SIZE + 1;
}

/////////////////////////////////////////////////////////////////
//
// Return the key of the next item in the table after a given item.
//
/////////////////////////////////////////////////////////////////

int Names::getNextEntry(int ofs)

{
	int		length;

	if (ofs >= nextEntry)
	{
		// past the end of the table - return a pointer to a null string at the beginning of the table
		// this is an attempt to be graceful in case of an internal error
		return 0;
	}
	else if (0 == ofs)
	{
		// return the first entry in the table
		return getFirstEntry();
	}
	else
	{
		// advance the offset to the next item in the table
		length = (int)(*(int *)(nameTable + ofs - 4));

		// make sure the length makes sense
		if ((length <= 0) || (length > nextEntry))
		{
			// return an offset to a zero length string
			return 0;
		}

		return ofs + length + 9;
	}
}

const char * Names::getNextName(const char *namePtr)

{
	int		len;

	// get the length of this sring
	len = strlen(namePtr);

	// is this already at the end of the table?
	if (0 == len)
	{
		// just return the input pointer
		return namePtr;
	}
	else
	{
		// move to the next string in the table
		return namePtr + len + PREFIX_SIZE + 1;
	}
}
/////////////////////////////////////////////////////////////////
//
// Return the length of the name table.
//  This is primarily for diagnostic purposes
//
/////////////////////////////////////////////////////////////////

int Names::getNameTableSize()

{
	return nameTableSize;
}

/////////////////////////////////////////////////////////////////
//
// Return the number of insertions into the table.
//  This is primarily for diagnostic purposes
//
/////////////////////////////////////////////////////////////////

int Names::getInsertCount()

{
	return insertCount;
}

/////////////////////////////////////////////////////////////////
//
// Return the number of times the table was reallocated.
//  This is primarily for diagnostic purposes
//
/////////////////////////////////////////////////////////////////

int Names::getReallocCount()

{
	return reallocCount;
}

/////////////////////////////////////////////////////////////////
//
// Diagnostic function to dump out the names table values
//
/////////////////////////////////////////////////////////////////

void Names::dumpNames(FILE *df)

{
	int			i;
	int			i1;
	int			slen;
	char		*ptr;
	SYSTEMTIME	stime;

	// check if a file was passed
	if (NULL == df)
	{
		// try to open the dump file for appending
		df = fopen(DUMP_FILE_NAME, "a+");
	}

	if (df != NULL)
	{
		// get the current time
		GetSystemTime(&stime);

		// format the date and time
		fprintf(df, "Dump of Names object on %d-%d-%d %d:%2.2d:%2.2d.%3.3d\n",
				stime.wYear,
				stime.wMonth,
				stime.wDay,
				stime.wHour,
				stime.wMinute,
				stime.wSecond,
				stime.wMilliseconds);

		fprintf(df, "nameTableSize %d\n", nameTableSize);
		fprintf(df, "nextEntry %d\n", nextEntry);
		fprintf(df, "nameTable %8.8x\n\n", (unsigned int)nameTable);

		// dump out the index entries
		for (i=0; i<NAMEINDEXMAX; i++)
		{
			fprintf(df, "idx=%d first=%8.8d last=%8.8d\n", i, nameIdx[i].firstName, nameIdx[i].lastName);
		}

		// print out the individual entries in the table
		fprintf(df, "Name entries follow\n");
		fflush(df);

		ptr = nameTable + 1;
		while ((ptr[PREFIX_SIZE] != 0) && ((ptr - nameTable) < nameTableSize))
		{
			// get the next name offset and length of this string
			i1 = (int)(*(int *)ptr);
			slen = (int)(*(int *)(ptr + PREFIX_SIZE - 4));

			// print the line to the dump file
			fprintf(df, "%8.8d %8.8u %8.8u %s\n", (ptr - nameTable), i1, slen, ptr + PREFIX_SIZE);
			fflush(df);

			// move on to the next string
			// the length of the entry is the length of the string
			// plus the length of the prefix plus the string terminator
			ptr += slen + PREFIX_SIZE + 1;
		}

		// print out the total size of the string table
		fprintf(df, "total size %d\n\n", ptr - nameTable);
		fflush(df);
	}
}


char * Names::AllocateTable(int size)

{
	char * ptr;

	// allocate the storage
	ptr = (char *)rfhMalloc(size + 4, "NAMESTBL");

	// check if the malloc worked
	if (ptr != NULL)
	{
		// initialize the storage
		memset(ptr, 0, size);

		// add an eye-catcher to help catch memory leaks
		memcpy(ptr, "NAME", 4);
		ptr += 4;
	}

	return ptr;
}

void Names::ReleaseTable()

{
	if (nameTable != NULL)
	{
		// free the current storage
		rfhFree(nameTable - 4);
	}
}

const char * Names::getNextNamePtr(const char *namePtr)

{
	int		len;

	// check if the name pointer is within the table
	if ((namePtr <= nameTable) || (namePtr > (nameTable + nextEntry)))
	{
		// return a pointer to a zero length string
		return nameTable;
	}

	// get the length of this item
	len = (int)(*(namePtr - 4));

	// check if the length makes sense
	if ((len < 0) || (len + namePtr > nameTable + nextEntry))
	{
		// not a valid length
		return nameTable;
	}

	return namePtr + len + PREFIX_SIZE + 1;
}
