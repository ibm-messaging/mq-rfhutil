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

// Copybook.cpp: implementation of the CCopybook class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Copybook.h"
#include "comsubs.h"
#include "mqsubs.h"
#include "names.h"
#include "rfhutil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/**************************************************************/
/*                                                            */
/* This program takes a COBOL copy book as input. The         */
/* program first builds an in memory data structure to        */
/* represent the copy book.  Each variable definition is      */
/* entered into a variable table.  By default, the table      */
/* can handle up to 4096 variables - this can be expanded     */
/* by changing the MAX_VAR_COUNT definition and recompiling   */
/* the program.                                               */
/*                                                            */
/* The following COBOL keywords are recognized and used as    */
/*  indicated.                                                */
/*                                                            */
/*  PIC - The picture clause following is processed.  If the  */
/*   keyword IS is found after PICTURE, it is skipped and     */
/*   ignored.  The picture clause is used to determine the    */
/*   length of variables, and to a lesser extent their type.  */
/*   The USAGE of a variable is also used to determine the    */
/*   length and type of a variable.                           */
/*                                                            */
/*  BINARY - Indicated that the usage is BINARY.  The         */
/*   length of the item is adjusted to 2, 4 or 8 bytes,       */
/*   depending on the picture clause.                         */
/*                                                            */
/*  COMP - Treated the same as BINARY.                        */
/*                                                            */
/*  COMP-1 and COMP-2 - Floating point, with COMP-1 being     */
/*   4 bytes long and COMP-2 being 8 bytes long.              */
/*                                                            */
/*  COMP-3 - Packed decimal - the length is adjusted based    */
/*   on the picture clause.                                   */
/*                                                            */
/*  COMP-4 and COMP-5 - treated the same as BINARY.           */
/*                                                            */
/*  POINTER and PROCEDURE-POINTER - Length is set to 4 bytes. */
/*                                                            */
/*  OCCURS - The number of occurences is saved, and used to   */
/*   calculate the offsets of items which follow.  OCCURS     */
/*   clauses can be given for higher level items as well      */
/*   as primitive data types, and can be nested.              */
/*                                                            */
/*  REDEFINES - The variable that is redefined is located     */
/*   and a pointer is established.  Redefines can be for      */
/*   higher level items as well as primitive data types,      */
/*   and can be nested.                                       */
/*                                                            */
/*  JUST - The type of justification is noted.                */
/*                                                            */
/*  SYNC - The alignment boundary is saved.  This can be      */
/*   a 2, 4 or 8 byte boundary, depending on the USAGE of     */
/*   the item.                                                */
/*                                                            */
/*  The following COBOL keywords are ignored.                 */
/*                                                            */
/*   USAGE - the actual usage types, such as COMP or BINARY   */
/*    are recognized and used.                                */
/*                                                            */
/*   IS - optional in COBOL, safely ignored.                  */
/*                                                            */
/*   DISPLAY - ignored, since this is the default.            */
/*                                                            */
/*   TIMES - optional and ignored.                            */
/*                                                            */
/*   BLANK WHEN ZERO.                                         */
/*                                                            */
/*   VALUE - should not be specified anyway.                  */
/*                                                            */
/*   EXTERNAL - should not be specified.                      */
/*                                                            */
/*   SIGN.                                                    */
/*                                                            */
/*   GLOBAL.                                                  */
/*                                                            */
/**************************************************************/

/**************************************************************/
/*                                                            */
/* Handling of SYNC clauses                                   */
/*                                                            */
/*  SYNC or SYNCHRONIZED clauses are only supported on        */
/*   primitive data items.  They will align the current       */
/*   variable on a storage address boundary, depending on     */
/*   the data type and length.  This applies to binary,       */
/*   index and pointer data types, and they will be aligned   */
/*   on a boundary which is an even multiple of their data    */
/*   length.  Slack bytes will be inserted as required to     */
/*   align the item.  The slack bytes are not part of any     */
/*   primitive data item, but will be included in group       */
/*   items.                                                   */
/*                                                            */
/*  The slack bytes are inserted before a variable.  They     */
/*   also affect the alignment and length of group variables. */
/*   If the first variable in a group variable is aligned,    */
/*   then the group item will also be aligned.                */
/*                                                            */
/*  The SYNC clause also interacts with an OCCURS clause.     */
/*   If an OCCURS clause is used on a group level, then       */
/*   slack bytes may be required both at the beginning of     */
/*   the item and before each occurence of the group          */
/*   variable.  The total length of the group item will be    */
/*   rounded up to a boundary that is a multiple of the       */
/*   largest alignment boundary found with the group item.    */
/*   These slack bytes will be added to the end of the        */
/*   group variable.                                          */
/*                                                            */
/**************************************************************/

///////////////////////////////////////////////////////////////////
//
// COBOL Data Division keyword definitions
//
///////////////////////////////////////////////////////////////////

#define PIC_STR       "PIC"
#define PICTURE_STR   "PICTURE"
#define OCCURS_STR    "OCCURS"
#define SYNC_STR      "SYNC"
#define JUST_STR      "JUST"
#define COMP_STR      "COMP"
#define COMP1_STR     "COMP-1"
#define COMP2_STR     "COMP-2"
#define COMP3_STR     "COMP-3"
#define COMP4_STR     "COMP-4"
#define COMP5_STR     "COMP-5"
#define BINARY_STR    "BINARY"
#define PACKED_STR    "PACKED"
#define PACK_DEC_STR  "PACKED-DECIMAL"
#define POINTER_STR   "POINTER"
#define PROCEDURE_STR "PROCEDURE"
#define DISPLAY_STR   "DISPLAY"
#define INDEX_STR     "INDEX"
#define REDEFINES_STR "REDEFINES"
#define USAGE_STR     "USAGE"
#define BLANK_STR     "BLANK"
#define VALUE_STR     "VALUE"
#define FILLER_STR    "FILLER"
#define DEPENDING_STR "DEPENDING"
#define ON_STR        "ON"
#define TO_STR        "TO"
#define IS_STR        "IS"
#define SIGN_STR      "SIGN"
#define SEPARATE_STR  "SEPARATE"
#define LEADING_STR   "LEADING"
#define TRAILING_STR  "TRAILING"

///////////////////////////////////////////////////////////////////
//
//  Data type definitions - from USAGE clauses
//
///////////////////////////////////////////////////////////////////

#define USE_DISPLAY   0
#define USE_BINARY    1
#define USE_POINTER   2
#define USE_PACKED    3
#define USE_FLOAT1    4
#define USE_INDEX     5
#define USE_FLOAT2    8

/////////////////////////////////////////////////////////
//
// Defines and global variables
//
/////////////////////////////////////////////////////////

#define MAX_STMT_LEN  8192
#define MAX_LINE_LEN  255

// trace file environment variable
#define RFHUTIL_TRACE_COBOL "RFHUTIL_TRACE_COBOL"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCopybook::CCopybook()

{
	//////////////////////////////////////////////////
	//    Allocate and initialize variable areas
	//////////////////////////////////////////////////

	maxvarcount = MAX_VAR_COUNT;

	namObj = new Names(16384);
    fvar = (COPY_STRUCT *)rfhMalloc(sizeof(COPY_STRUCT) * (MAX_VAR_COUNT + 1), "FVAR    ");
    memset(fvar,'\0',(sizeof(COPY_STRUCT) * (MAX_VAR_COUNT + 1)));
	memset(offsetTable, 0, sizeof(offsetTable));
	maxvar=0;           // Number of fields found
	dependCount=0;
	linesText=0;
	bytesText=0;
	offsetError = FALSE;
	traceEnabled = FALSE;
}

CCopybook::~CCopybook()
{
	rfhFree(fvar);
	delete(namObj);
}

////////////////////////////////////////////////////////////////
//
// Routine to check for COBOL keywords.  This routine
// is needed to not be confused by keyword strings
// in variable names.  It will look for a proper
// delimiting character right after the keyword.
// Delimiting values can be a period, space or
// begin parentheses.
//
////////////////////////////////////////////////////////////////

char * CCopybook::findKeyword(char *cmdline, const char *keywd)

{
	char		*temptr;
	char		*prevptr;
	char		*endptr;
	char		*firstquote;
	char		*secondquote;
	char		traceInfo[512];		// work variable to build trace message

	temptr = cmdline;
	while (1)
	{
		// try to find the second string in the first string
		temptr = strstr(temptr, keywd);

		// Did we find it?
		if (temptr == NULL)
		{
			// Didn't find it = get out
			break;
		}
		else
		{
			// Point to the character after the keyword string
			endptr = temptr + strlen(keywd);

			// Point to the character before the keyword string
			prevptr = temptr - 1;

			// Make sure that the previous and next characters are delimiters
			if (((endptr[0] == ' ') || (endptr[0] == '.') ||
				 (endptr[0] == '(') || (endptr[0] == '\0')) && 
				 ((prevptr[0] == ' ') || (temptr == cmdline)))
			{
				// Lastly, check if we are in between quotes
				firstquote = strchr(cmdline, '\'');

				// Did we find a quote?
				if (firstquote != NULL)
				{
					// Yes, try to find a second quote
					secondquote = strchr(firstquote + 1, '\'');
				}

				// check if the key word is found in a literal
				if ((firstquote == NULL) || (secondquote == NULL) ||
					(temptr < firstquote) || (temptr > secondquote))
				{
					// We are not in a literal
					break;
				}
				else
				{
					// skip to the end of the second quote
					temptr = secondquote + 1;
				}
			}
			else
			{
				// move past the current string
				temptr++;
			}
		}
	}

 	if (traceEnabled)
	{
		// make sure there is something interesting to trace
		if (temptr != NULL)
		{
			// create the trace line
			sprintf(traceInfo, "Entering CCopybook::findKeyword() keywd=%s temptr=%.32s", keywd, temptr);

			// trace entry to findKeyword
			logTraceEntry(traceInfo);
		}
	}

	// Return the pointer value
	return temptr;
}

////////////////////////////////////////////////////////////////
//
// Routine to calculate offsets and handle occurs and
// redefines clauses, as well as sync clauses.  This routine
// will return the length of the item.  It will also set the
// offset to the end of the last variable
//
////////////////////////////////////////////////////////////////

int CCopybook::calcOffsets(int varnum, int startOfs, int *newOfs)

{
	int			child;
	int			length=0;
	int			occur;
	int			tempOfs;            // temporary work area for offset
	int			delta=0;            // working variable for sync clauses
	int			slack=0;			// slack bytes at end of each occurence except last one
	int			firstChild=0;		// first child of element
	int			offset;             // Offset within the copybook area
	int			RC=0;
	char		traceInfo[512];		// work variable to build trace message
	
	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::calcOffsets() startOfs=%d newOfs=%d varnum=%d VarName=%s", startOfs, (*newOfs), varnum, namObj->getNameAddr(fvar[varnum].VarName));

		// trace entry to calcOffsets
		logTraceEntry(traceInfo);
	}

	// check for alignment
	if ((fvar[varnum].VarSync) > 1)
	{
		// calculate the number of slack bytes required at the beginning of this variable
		delta = fvar[varnum].VarSync - (startOfs % fvar[varnum].VarSync);
		delta = delta % fvar[varnum].VarSync;
	}

	// align the variable on the appropriate boundary (2, 4 or 8 byte boundary)
	startOfs += delta;

	// Calculate the length of any items, and then
	// the starting position of any items, for each
	// child of this variable.

	// Does this element have children?
	if (0 == fvar[varnum].VarChild)
	{
		// no children, just set the offset and return
		fvar[varnum].VarOffset = startOfs;
		length = fvar[varnum].VarLen;

		if (fvar[varnum].VarOccurMax > 0)
		{
			occur = fvar[varnum].VarOccur;
		}
		else
		{
			//occur = 1;
			//if (fvar[varnum].VarOccur > 1)
			//{
				occur = fvar[varnum].VarOccur;
			//}
		}

		if (0 == fvar[varnum].VarOccurDepend)
		{
			// there is no occurs depending on clause
			(*newOfs) = startOfs + length;
		}
		else
		{
			(*newOfs) = startOfs + (length * fvar[varnum].VarOccur) + (delta * (fvar[varnum].VarOccur - 1));
		}
	}
	else
	{
		// get the beginning offset
		offset = startOfs;

		// get the first child
		firstChild = fvar[varnum].VarChild;
		child = firstChild;

		while (child > 0)
		{
			if (fvar[child].VarOccurMax > 0)
			{
				occur = fvar[child].VarOccur;
			}
			else
			{
				occur = 1;
				if (fvar[child].VarOccur > 1)
				{
					occur = fvar[child].VarOccur;
				}
			}

			// check if this variable is a redefine of another
			if (0 == fvar[child].VarRedefine)
			{
				// not a redefine - use the new offset
				length = calcOffsets(child, offset, &offset);
				offset += (occur - 1) * fvar[child].VarLen;
			}
			else
			{
				// this is a redefinition - use the previous offset
				tempOfs = fvar[fvar[child].VarRedefine].VarOffset;
				length = calcOffsets(child, tempOfs, &tempOfs);
			}

			child = fvar[child].VarNext;
		}

		// set the total length, including any slack bytes
		fvar[varnum].VarLen = offset - startOfs;
		fvar[varnum].VarOffset = fvar[fvar[varnum].VarChild].VarOffset;

		// check if there is an occurs without a redefine and a sync clause
		if ((0 == fvar[varnum].VarRedefine) && (fvar[varnum].VarSync > 1) && (fvar[varnum].VarOccur > 1))
		{
			// calculate the number of slack bytes
			// calculate the number of slack bytes after each occurence
			slack = (fvar[firstChild].VarSync - (fvar[varnum].VarLen % fvar[firstChild].VarSync)) % fvar[firstChild].VarSync;

			// add the appropriate number of slack bytes
			offset += slack * (fvar[varnum].VarOccur - 1);

			// remember the number of slack bytes
			fvar[varnum].VarEndSlack = slack;
		}

		(*newOfs) = offset;
	}
				// Does this variable need to be aligned?
				// if so, then align the variable and
				// then propagate the alignment to any
				// parents if necessary.  Alignment of
				// parents is only necessary if this is
				// the first child.
				// These slack bytes are added before the
				// variable.  Other slack bytes may be
				// added after a group variable with an
				// OCCURS clause.  These will be added in
				// the routine above which handles
				// changes in level
//				if (fvar[maxvar].VarSync > 1)
//				{
					// change the offset to align the variable,
					//  if not already aligned
//					if ((fvar[maxvar].VarOffset%align) > 0)
//					{
						// calculate the number of slack bytes required
//						delta = align - (fvar[maxvar].VarOffset%align);

						// change the offset to align the variable
//						fvar[maxvar].VarOffset  += delta;

						// increase the current offset to match
//						offset += delta;

						// Next, if this is the first child, propagate
						// the alignment up to the parent
//						tempvar = fvar[maxvar].VarParent;
//						tempchild = maxvar;
//						while (tempvar > 0)
//						{
//							if (fvar[tempvar].VarChild == tempchild)
//							{
								// remember the alignment level
//								fvar[tempvar].VarSync = align;

								// change the offset to align the variable
//								fvar[tempvar].VarOffset = fvar[tempchild].VarOffset;
//							}
//							else
//							{
//								fvar[tempvar].VarLen += delta;
//							}

							// move up to the next higher variable and see
							// if we are still the first child
//							tempchild = tempvar;
//							tempvar = fvar[tempchild].VarParent;
//						}
//					}
//				}
				// Reset the offset for any REDEFINES clauses
//				if ((redefines != NULL) && (redefvar > 0))
//				{
//					// Reset the offset for a new redefines
//					offset = fvar[redefvar].VarOffset;
//				} // endif

	// set the length in the 0 variable
//	if (1 == fvar[1].VarLevel)
//	{
//		fvar[1].VarLen = offset;
//	}

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::calcOffsets() Total length of area in bytes %d newofs %d delta=%d slack=%d VarName=%s", length, (*newOfs), delta, slack, namObj->getNameAddr(fvar[varnum].VarName));

		// trace exit from calcOffsets
		logTraceEntry(traceInfo);
	}

	return length;
}

////////////////////////////////////////////////////////////////
//
// Routine to process sync clauses and propagate them up to all
// of the parent elements where the child is the first child of
// the parent and the alignment should also be aligned.
//
// SYNC clauses will align data on 1, 2, 4 or 8-byte boundaries.
//
// This processing should not be required since SYNC clauses
// are only allowed on elemental items and not on group items.
//
////////////////////////////////////////////////////////////////

void CCopybook::processSync()

{
	int		varnum;
	int		parent;
	int		child;

	for (varnum = 1; varnum <= maxvar; varnum++)
	{
		// get a pointer to the first child of this element
		child = varnum;
		parent = fvar[varnum].VarParent;

		// is the element the first child and does it have a SYNC clause?
		while ((parent > 0) && (fvar[child].VarSync > 0) && (fvar[parent].VarChild == child))
		{
			// propagate the synch clause down
			fvar[parent].VarSync = fvar[child].VarSync;

			// move up to the next level
			child = parent;
			parent = fvar[varnum].VarParent;
		}
	}
}

////////////////////////////////////////////////////////////////
//
// Routine to read the COBOL copy book file and build a
// logical tree structure in memory.  The logical tree
// will only contain lengths of elementary items and will
// not have offsets calculated.  The offsets will be
// calculated in a later pass
//
////////////////////////////////////////////////////////////////

int CCopybook::buildTree(const char * fileName)

{
	int		depth=0;            // Number of levels below root
	int		length=0;           // Length of data item
	int		level;              // Level number of current line
	int		parent=0;           // Current parent element
	int		redefLevel=0;       // Level number of redefines clause
	int		i;                  // Work variable
	int		oldvarcount;        // Used to expand fvar table
	int		levelOnes=0;        // Number of 01 level variables found
	COPY_STRUCT * oldfvar;   // Used to expand fvar table
	char	*cline;             // Current line pointer
	char	*endLevel;          // Pointer to end of level number
	char	*varname;           // Pointer to variable name
	char	*redefines;         // Location of REDEFINES
	char	*pic;               // Location of PIC
	char	*occurs;            // Location of OCCURS
	char	*sync;              // Location of SYNC
	char	*just;              // Location of JUST
	char	*comp;              // Location of COMP
	char	*binary;            // Location of BINARY
	char	*packed;            // Location of PACKED
	char	*packdec;           // Location of PACKED-DECIMAL
	char	*display;           // Location of DISPLAY
	char	*index;             // Location of INDEX
	char	*pointer;           // Location of POINTER
	char	*procptr;           // Location of PROCEDURE-POINTER
	char	*usage;             // Location of USAGE
	char	*blank;             // Location of BLANK
	char	*value;             // Location of VALUE
	char	*sign;              // Location of SIGN
	char	*separate;          // Location of SEPARATE
	char	*leading;           // Location of LEADING
	char	*trailing;          // Location of TRAILING
	char	use;                // Type of USAGE
	char	datatype=0;         // type of data field
	char	*redefineName;      // redefines target
	char	cobolStmt[MAX_STMT_LEN + MAX_LINE_LEN + 1];  // current commarea line
	int		firstatlevel[50];   // keep track of first variable at level
	int		lastatlevel[50];    // keep track of last variable at level
	int		occuratlevel[50];   // keep track of OCCURS at level
	int		savelast;           // working variable
	int		redefvar=0;         // working variable
	int		lastlevel=0;        // remember level of previous variable
	int		eof;                // End of File indicator
	int		RC=0;
	char	traceInfo[512];		// work variable to build trace message
	
 //
 // File variables and counters
 //

    FILE        *copybookfile; // input COBOL program
    int         inCounter=0;   // input line counter
    int         outCounter=0;  // output line counter
	int			inFile=0;      // Lines within current file

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::buildTree() fileName=%s", (LPCTSTR)fileName);

		// trace entry to buildTree
		logTraceEntry(traceInfo);
	}

   for (i=0; i < 50; i++)
    {
       firstatlevel[i] = 0;
       lastatlevel[i]  = 0;
       occuratlevel[i] = 0;
    } // end for

/////////////////////////////////////////
// Open the copy book and message file
/////////////////////////////////////////

	if ((copybookfile = fopen(fileName, "r")) == NULL)
	{
 		if (traceEnabled)
		{
			// create the trace line
			sprintf(traceInfo, "Open failed for fileName=%s", (LPCTSTR)fileName);

			// trace entry to buildTree
			logTraceEntry(traceInfo);
		}

		return 100;
	}

///////////////////////////////////////////////////////////////
//
//  This routine will read the copy book file and generate
//  an in-memory parse tree structure
//
///////////////////////////////////////////////////////////////

	// Read the next line(s) in the source file. This routine
	// returns a complete COBOL source statement, with the
	// ending period removed.  It also removes comments and
	// and blank lines.  It returns a 1 if end of file is
	// reached.  Leading and trailing blanks are removed
	while (1)
	{
		// Get the next COBOL line
		eof = buildNextLine(cobolStmt, MAX_STMT_LEN, copybookfile, &inCounter, &inFile);

		// Check for end of file
		if (1 == eof)
		{
			break;
		}

		// find the end of the level number
		endLevel = cobolStmt;
		endLevel++;
		while ((endLevel[0] != ' ') && (endLevel[0] != 0) && (0 == eof))
		{
			endLevel++;
		} // end while

		if ((endLevel[0] != 0) && (0 == eof))
		{
			// terminate the level number string
			endLevel[0] = 0;

			// convert the number to a level number
			level = atoi(cobolStmt);

			// check for a level 01
			if (1 == level)
			{
				levelOnes++;
			}

			// Ignore if not a level 01-49
			if ((level > 0) && (level <= 49) && (0 == eof))
			{
				// remember we found this level
				offsetTable[level] = 1;

				// point to the rest of the line
				cline = endLevel + 1;

				// point to variable name
				cline = skipBlanks(cline);

 				if (traceEnabled)
				{
					// create the trace line
					sprintf(traceInfo, " cline=%.256s", cline);

					// trace the current line
					logTraceEntry(traceInfo);
				}

				// find the end of the variable name
				varname = cline;
				while ((cline[0] != ' ') && (cline[0] != '\0') &&
					   (cline[0] != '.'))
				{
					cline++;
				} // end while

				// Check for a COBOL keyword instead of a variable name
				// In COBOL, it is legitimate to leave off the variable
				// name and let it default to FILLER.  The purpose of
				// this subroutine is to recognize if we have a COBOL
				// keyword immediately following the level, which
				// indicates that the variable name is defaulting to
				// FILLER.  If this is the case, then the pointer is
				// changed, and we reset the end of string
				if ((memcmp(varname, REDEFINES_STR, sizeof(REDEFINES_STR) - 1) == 0) ||
					(memcmp(varname, PIC_STR, sizeof(PIC_STR) - 1) == 0) ||
					(memcmp(varname, PICTURE_STR, sizeof(PICTURE_STR) - 1) == 0))
				{
					cline = varname;		// restore the pointer
					varname = FILLER_STR;
				} // end if
				else
				{
					// Terminate the variable name to produce a string
					if (cline[0] != 0)
					{
						cline[0] = 0;
						cline++;
					}
				}

				// find the first keyword
				cline = skipBlanks(cline);

				// check if there is any need to check for keywords
				// this is done for performance reasons.
				if (0 == cline[0])
				{
					// nothing on this line
					occurs    = NULL;
					sync      = NULL;
					just      = NULL;
					binary    = NULL;
					packed    = NULL;
					packdec   = NULL;
					display   = NULL;
					index     = NULL;
					pointer   = NULL;
					procptr   = NULL;
					redefines = NULL;
					usage     = NULL;
					blank     = NULL;
					value     = NULL;
					sign      = NULL;
					separate  = NULL;
					leading   = NULL;
					trailing  = NULL;
					pic       = NULL;
					comp      = NULL;
				}
				else
				{
					// search for various keywords
					occurs    = findKeyword(cline, OCCURS_STR);
					sync      = findKeyword(cline, SYNC_STR);
					just      = findKeyword(cline, JUST_STR);
					binary    = findKeyword(cline, BINARY_STR);
					packed    = findKeyword(cline, PACKED_STR);
					packdec   = findKeyword(cline, PACK_DEC_STR);
					display   = findKeyword(cline, DISPLAY_STR);
					index     = findKeyword(cline, INDEX_STR);
					pointer   = findKeyword(cline, POINTER_STR);
					procptr   = findKeyword(cline, PROCEDURE_STR);
					redefines = findKeyword(cline, REDEFINES_STR);
					usage     = findKeyword(cline, USAGE_STR);
					blank     = findKeyword(cline, BLANK_STR);
					value     = findKeyword(cline, VALUE_STR);
					sign      = findKeyword(cline, SIGN_STR);
					separate  = findKeyword(cline, SEPARATE_STR);
					leading   = findKeyword(cline, LEADING_STR);
					trailing  = findKeyword(cline, TRAILING_STR);
					pic       = findKeyword(cline, PIC_STR);
					comp      = findKeyword(cline, COMP_STR);

					// check for multiple level 01 variables without a redefines
					if ((1 == level) && (levelOnes > 1) && (NULL == redefines))
					{
						// second level one found - stop processing the copy book
						break;
					}

					// check for variations on COMP and PIC
					if (NULL == pic)
					{
						pic = findKeyword(cline, PICTURE_STR);
					}

					if (NULL == comp)
					{
						comp = findKeyword(cline, COMP1_STR);
					}

					if (NULL == comp)
					{
						comp = findKeyword(cline, COMP2_STR);
					}

					if (NULL == comp)
					{
						comp = findKeyword(cline, COMP3_STR);
					}

					if (NULL == comp)
					{
						comp = findKeyword(cline, COMP4_STR);
					}

					if (NULL == comp)
					{
						comp = findKeyword(cline, COMP5_STR);
					}
				}

				if (redefLevel >= level)
				{
					redefLevel = 0;
				}

				// count the number of fields found
				maxvar++;
				if (maxvar == maxvarcount)
				{
					// allocate a larger table
					oldfvar = fvar;
					oldvarcount = maxvarcount;
					maxvarcount *= 2;
					fvar = (COPY_STRUCT *)rfhMalloc(sizeof(COPY_STRUCT) * (maxvarcount + 1), "FVAR2   ");
					memset(fvar, 0, sizeof(COPY_STRUCT) * (maxvarcount + 1));
					memcpy(fvar, oldfvar, sizeof(COPY_STRUCT) * (oldvarcount + 1));
					rfhFree(oldfvar);
				}

				// insert the field into the variable table
				fvar[maxvar].VarName = namObj->insertName(varname);
				fvar[maxvar].VarLevel  = level;

				// check for a redefines clause
				if (redefines != NULL)
				{
					// get the variable name in the redefines clause
					redefineName = processRedefine(redefines);

					// check if it is the last at the same level
					redefvar = lastatlevel[level];
					while ((strcmp(namObj->getNameAddr(fvar[redefvar].VarName), redefineName) != 0) &&
						   (redefvar > 0))
					{
						// Not last at same level - try to find it anyway
						redefvar = fvar[redefvar].VarPrev;
					} // endif

					if (redefvar == 0)
					{
						// indicate an error - could not locate the target
						RC = 99;

 						if (traceEnabled)
						{
							// create the trace line
							sprintf(traceInfo, "CCopybook::buildTree() Invalid redefines - level=%d varname=%s redefineName=%s", level, varname, redefineName);

							// trace error in buildTree
							logTraceEntry(traceInfo);
						}
					} // endif

					if ((level < redefLevel) || (redefLevel == 0))
					{
						redefLevel = level;
					}

					// update the redefines pointer if necessary
					fvar[maxvar].VarRedefine = redefvar;
				} // endif

				// Insert into the level pointers
				if (firstatlevel[level] == 0)
				{
					// first at level, create chain
					firstatlevel[level] = maxvar;
					lastatlevel[level]  = maxvar;
				}
				else
				{
					// update the backwards chain pointers
					savelast = lastatlevel[level];
					lastatlevel[level] = maxvar;
					fvar[maxvar].VarPrevLevel = savelast;

					// update the forward level chain pointers
					fvar[savelast].VarNextLevel = maxvar;
					fvar[maxvar].VarNextLevel = 0;
				} // endif

				// the following code looks out for the
				// situation where a level is less than
				// the previous level, but the parent
				// is still the same.
				// For example, consider:
				// 10 PARENT_VAR
				//  15 FIRST_CHILD PIC X
				//  12 SECOND-CHILD PIC X
				//
				// Both FIRST-CHILD and SECOND-CHILD
				// are children of parent.
				if ((level < lastlevel) && (level > fvar[parent].VarLevel) && (lastlevel > 0))
				{
					lastlevel = level;
				}

				// did the level increase or decrease?
				if (level > lastlevel)
				{
					// level is greater than previous variable
					// This indicates that we have moved down
					// in the hierarchy.  Occurs and redefines
					// will stay the same, and the previous
					// variable is the new parent.
					depth++;
					parent = maxvar - 1;

					// Remember that this is the first and
					// last child of the parent.
					fvar[parent].VarChild = maxvar;
					fvar[parent].VarLastChild = maxvar;

					// Remember the parent variable
					fvar[maxvar].VarParent = parent;

					// Set the current level
					lastlevel = level;
				}
				else
				{
					// one or more redefines and/or occurs
					// clauses are now finished.  For the
					// occurs clauses, we need to calculate
					// a new offset, respecting the alignment
					// of the occurs clause.  If a redefines
					// clause has ended, the lengths of the
					// two areas must be checked, and the
					// offset adjusted if necessary.  Lastly,
					// we must find the new parent, if any.
					if (level == lastlevel)
					{
						// this is the last child of the
						// parent of the previous element
						fvar[maxvar].VarPrev = fvar[parent].VarLastChild;
						fvar[fvar[parent].VarLastChild].VarNext = maxvar;
						fvar[maxvar - 1].VarNextLevel = maxvar;
						fvar[maxvar].VarPrevLevel = maxvar - 1;
						fvar[parent].VarLastChild = maxvar;
					}
					else
					{
						// Level is less than previous variable
						// This indicates that we have moved up
						// the hierarchy. First, find the
						// previous sibling.
						depth--;
						while ((parent > 0) && (fvar[parent].VarLevel > level))
						{
							parent = fvar[parent].VarParent;
							depth--;
						}

						if (0 == parent)
						{
							RC = 97;
						}
						else
						{
							// move up to the parent of this element
							parent = fvar[parent].VarParent;

							// insert this element as the last child of the parent
							fvar[fvar[parent].VarLastChild].VarNext = maxvar;
							fvar[maxvar].VarPrev = fvar[parent].VarLastChild;
							fvar[parent].VarLastChild = maxvar;
						}
					}
				} // endif

				// save the current variable's parent
				fvar[maxvar].VarParent = parent;
				fvar[maxvar].VarDepth = depth;

				// Set the current level
				lastlevel = level;

				// Check for OCCURS clause
				if (occurs != NULL)
				{
					// Fill in the occurs count
					handleOccurs(occurs, &dependCount);

					if ((fvar[maxvar].VarOccur > 1) || (fvar[maxvar].VarOccurDepend > 0))
					{
						// remember the occurs at this level
						occuratlevel[level] = maxvar;
					}
					else
					{
						fvar[maxvar].VarOccur = 0;
					}
				} // endif

				// handle the picture clause
				datatype = 0;
				if (pic != NULL)
				{
					length = handlePic(pic, &datatype, &fvar[maxvar].VarFlags);

					// check for a sign separate clause
					if ((sign != NULL) && (separate != NULL))
					{
						// account for the sign character
						length++;

						if (trailing != NULL)
						{
							fvar[maxvar].VarFlags |= SIGN_SEP_TRAIL;
						}
						else
						{
							fvar[maxvar].VarFlags |= SIGN_SEP_LEAD;
						}
					}
				}
				else
				{
					length = 0;
				}

				// handle the usage clause keywords
				// set the default to no usage type
				use = 0;

				// check for USAGE DISPLAY
				if (display != NULL)
				{
					// set the usage
					use = USE_DISPLAY;
				}

				// check for USAGE BINARY
				if (binary != NULL)
				{
					// set the usage
					use = USE_BINARY;

					// calculate the actual length of the item
					length = handleBinaryLength(length);
				}

				// check for USAGE PACKED or PACKED-DECIMAL
				if ((packed != NULL) || (packdec != NULL))
				{
					// set the usage
					use = USE_PACKED;

					// calculate the actual length of the item
					length = handlePackedLength(length);
				}

				// check for one of the USAGE COMP keywords
				if (comp != NULL)
				{
					use = handleComp(comp, &length);
				}

				// check for USAGE INDEX
				if (index != NULL)
				{
					// set the usage and the length
					length = 4;
					use = USE_INDEX;
				}

				// check for USAGE POINTER or PROCEDURE-POINTER
				if ((pointer != NULL) || (procptr != NULL))
				{
					// set the usage and the length
					length = 4;
					use = USE_POINTER;
				}

				// Set the fields in the variable record
				fvar[maxvar].VarLen    = length;
				fvar[maxvar].VarType   = datatype;
				fvar[maxvar].VarUsage  = use;

				// calculate the alignment of this variable, in
				// case there is a sync clause either for this
				// variable or for a parent of this variable

				// Check for a SYNC or SYNCHRONIZED clause
				// If none was found for this variable, check
				// for one at a higher level

				if (sync != NULL)
				{
					syncCount++;

					// get the boundary to align to
					switch (use)
					{
					case USE_POINTER:
					case USE_INDEX:
					case USE_BINARY:
					case USE_FLOAT1:
					case USE_FLOAT2:
						{
							// align on 2, 4 or 8 byte boundary
							fvar[maxvar].VarSync = length;
							break;
						}

					default:
						{
							// align on single byte boundary
							break;
						}
					}
				}
			}

			// increment the line counter for the output program
			outCounter++;
 				
			if (traceEnabled)
			{
				// create the trace line
				sprintf(traceInfo, " outCounter=%d maxvar=%d level=%d redefLevel=%d length=%d datatype=%d use=%d", outCounter, maxvar, level, redefLevel, length, datatype, use);

				// trace the current line
				logTraceEntry(traceInfo);
			}
		} // endif

	} // endwhile

 	if (traceEnabled)
	{
		// print all the variables, starting with the first variable
		logTraceEntry("\nDump of Variable table");
		dumpVar(0, fvar, maxvar);

		logTraceEntry("First at Level table");
		for (i=1; i < 50; i++)
		{
			if (firstatlevel[i] > 0)
			{
				sprintf(traceInfo,"  level %.2d - first(%d)", i, firstatlevel[i]);
				logTraceEntry(traceInfo);
			}
		}

		logTraceEntry("Last at Level table");
		for (i=1; i < 50; i++)
		{
			if (lastatlevel[i] > 0)
			{
				sprintf(traceInfo,"  level %.2d -  last(%d)",i,lastatlevel[i]);
				logTraceEntry(traceInfo);
			}
		}

		logTraceEntry("Occurs table");
		for (i=1; i < 50; i++)
		{
			if (occuratlevel[i] > 0)
			{
				sprintf(traceInfo,"  level %.2d - occurs(%d)",i,occuratlevel[i]);
				logTraceEntry(traceInfo);
			}
		}
	}

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::buildTree() RC=%d Input lines read=%d Output lines written=%d variables found=%d", RC, inCounter, outCounter, maxvar);

		// trace exit from buildTree
		logTraceEntry(traceInfo);
	}

	// close the file
	fclose(copybookfile);

	return RC;
}

///////////////////////////////////////////
//
// Public routine that is called to
// read and process a COBOL copy book
//
// The real work is done in the buildTree
// method.
//
///////////////////////////////////////////

int CCopybook::parseCopyBook(const char * fileName)

{
	int			i;                  // Work variable
	int			length=0;           // Length of root variable
	int			ofs;                // Number of spaces to offset
	int			lastpeer;			// pointer to last peer of root element
	int			RC=0;               // Return code from program
	char		*envPtr;			// pointer to environment variable
	CRfhutilApp	*app=(CRfhutilApp *)AfxGetApp();
	char		traceInfo[512];		// work variable to build trace message

   // check if trace is enabled
 	if (app->isTraceEnabled())
	{
		// check if the environment variable for COBOL trace is set
		envPtr = getenv(RFHUTIL_TRACE_COBOL);
		if ((envPtr != NULL) && app->isTraceEnabled())
		{
			// enable tracing of COBOL copybook processing
			traceEnabled = TRUE;
		}
	}

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::parseCopyBook() fileName=%s", (LPCTSTR)fileName);

		// trace entry to parseCopyBook
		logTraceEntry(traceInfo);
	}

	// initialize global variable used to remember if any synchronzie clauses were found
	syncCount = 0;

	// clear the offsets table for a new copy book
	memset(offsetTable, 0, sizeof(offsetTable));

	// read the copybook and build an in-memory tree
	RC = buildTree(fileName);

	// check if the copybook was read
	if (RC != 0)
	{
		return RC;
	}

	// at this point, we will attempt to cover up for what
	// are invalid COBOL copy books.  We will make sure that
	// all the children of the root are chained together
	lastpeer = fvar[1].VarLastChild;
	i = lastpeer + 1;

	while (i < maxvar)		
	{
		// add this one to the chain
		if (0 == fvar[i].VarParent)
		{
			fvar[i].VarPrev = lastpeer;
			fvar[lastpeer].VarNext = i;
			lastpeer = i;
		}

		i++;
	}

	// check if we found any sync clauses
	if (syncCount > 0)
	{
	   processSync();

		if (traceEnabled)
		{
			// print all the variables, starting with the first variable
			logTraceEntry("\nDump of Variable table after processSync");
			dumpVar(0, fvar, maxvar);
		}
	}

	// Calculate offsets of all variables and lengths
	// of compound items
	calcOffsets(1, 0, &length);
	fvar[1].VarLen = length;

	// remember the size of the copy book
	copyBookSize = length;

	if (traceEnabled)
	{
		// print all the variables, starting with the first variable
		logTraceEntry("\nDump of Variable table after calcOffsets");
		dumpVar(0, fvar, maxvar);
	}

	// we have now parsed the entire copy book
	// build the offsets for each level that we 
	// will use when we display the data
	// no offset for level 1
	offsetTable[0] = 0;
	offsetTable[1] = 0;
	i = 2;

	// offset each successive level that actually exists
	// in the copy book
	ofs = 0;
	while (i < sizeof(offsetTable))
	{
		if (offsetTable[i] > 0)
		{
			offsetTable[i] = ofs++;

			// maximum of 10 for offset
			if (ofs > 10)
			{
				ofs = 10;
			}
		}

		i++;
	}

	// Finished - return normally
	return RC;
}

int CCopybook::displayDat(char *printArea, 
						  const int maxArea, 
						  const int firstvar, 
						  const int lastvar,
						  const unsigned char *dataIn, 
						  const unsigned char *endData, 
						  const int occurCnt,
						  const int occurOfs,
						  const int charFormat, 
						  const int numFormat,
						  const int pdNumFormat,
						  const int ccsid, 
						  const int indent,
						  const int checkData)

{
	int			i;
	int			j;
	int			rc=0;
	int			iteration;
	int			occurCount;
	int			newCount;
	int			dataOffset;
	int			parent;
	char		*printLoc;
	char		*redefname;
	char		traceInfo[512];		// work variable to build trace message

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::displayDat() firstvar=%d, lastvar=%d, occurCnt=%d, occurOfs=%d firstvar=%s lastvar=%s", 
			firstvar, lastvar, occurCnt, occurOfs, namObj->getNameAddr(fvar[firstvar].VarName), namObj->getNameAddr(fvar[lastvar].VarName));

		// trace entry to displayDat
		logTraceEntry(traceInfo);
	}

	// do not suppress the first offset error message
	// this switch will suppress repeated messages on adjacent lines of output
	offsetError = FALSE;

	// point to the data area
	printLoc = printArea + strlen(printArea);

	i = firstvar;
	while ((i > 0) && (0 == rc))
	{
		// if this variable is a redefines, get the name of the redefined variable
		redefname = NULL;
		if (fvar[i].VarRedefine > 0)
		{
			redefname = namObj->getNameAddr(fvar[fvar[i].VarRedefine].VarName);
		}

		// get the occurs count
		occurCount = fvar[i].VarOccur;

		if ((0 == occurCount) && (0 == fvar[i].VarOccurDepend))
		{
			// make sure it is at least 1
			occurCount = 1;
		}

		// check for an occurs depending on clause
		if (fvar[i].VarOccurDepend > 0)
		{
			// check if we are past the end of the data area
			if ((dataIn + fvar[i].VarOffset + fvar[i].VarLen) > endData)
			{
				// try to prevent runaway loops if the data is 
				// past the end of the input area
				newCount = 0;
				occurCount = 0;
			}
			else
			{
				// get the current occurs depending on count
				//newCount = fvar[fvar[i].VarOccurDepend].VarCurrentValue;
				newCount = getVarValue(fvar[i].VarOccurDepend, dataIn, numFormat, pdNumFormat);
				iteration = 1;

				// prevent runaway loops
				if ((newCount > fvar[i].VarOccurMax) && (fvar[i].VarOccurMax > 0))
				{
					newCount = fvar[i].VarOccurMax;
				}
			}

			// check if the count is different
			if (newCount != occurCount)
			{
				// we need to change the length of the parents
				parent = fvar[i].VarParent;
				while (parent != 0)
				{
					fvar[parent].VarLen += (newCount - occurCount) * fvar[i].VarLen;
					parent = fvar[parent].VarParent;
				}

				// set the new number of occurences
				occurCount = newCount;
			}
		}
		else
		{
			// get the iteration number, which should be zero if 
			// the item only occurs once
			if (occurCount > 1)
			{
				iteration = 1;
			}
			else
			{
				if (occurCnt > 0)
				{
					iteration = occurCnt;
				}
				else
				{
					iteration = 0;
				}
			}
		}

		// remember the extra offset to add to each line
		dataOffset = 0;

		j=1;
		if (fvar[i].VarChild == 0)
		{
			// The occurs is on an elementary item.  No need for 
			// anything fancy 

			while (j <= occurCount)
			{
				if ((printLoc - printArea) > (maxArea - 4096))
				{
					strcat(printLoc, "***** Exceeded memory buffer\r\n");
					j = occurCount;
					i = lastvar;
					rc = 1;			// break out of all the loops
				}
				else
				{
					varReportLine(printLoc, 
								  i, 
								  dataIn + fvar[i].VarOffset + dataOffset,
								  endData,
								  redefname, 
								  iteration, 
								  occurOfs + (j - 1) * (fvar[i].VarLen + fvar[i].VarEndSlack), 
								  charFormat, 
								  numFormat,
								  pdNumFormat,
								  ccsid,
								  indent,
								  checkData);

					// wrote a line of output - do not suppress error messages
					offsetError = FALSE;
				}

				// check if this is a redefines and the redefine is larger than the original variable
				if ((1 == j) && (fvar[i].VarRedefine > 0))
				{
					// check the lengths
					if (fvar[i].VarLen > fvar[fvar[i].VarRedefine].VarLen)
					{
						// insert an error message if a larger area redefines a smaller one
						sprintf(printLoc + strlen(printLoc), "***** Redefine larger than original definition\r\n");
					}
				}

				// move on to the next offset
				dataOffset += fvar[i].VarLen + fvar[i].VarEndSlack;

				// move on to the next print location in the print buffer
				printLoc += strlen(printLoc);

				// do the next occur
				iteration++;
				j++;
			}
		}
		else
		{
			while (j <= occurCount)
			{
				// output the parent variable to the display
				varReportLine(printLoc, 
							  i, 
							  dataIn + fvar[i].VarOffset + dataOffset, 
							  endData,
							  redefname, 
							  iteration, 
							  occurOfs + (j - 1) * (fvar[i].VarLen + fvar[i].VarEndSlack), 
							  charFormat, 
							  numFormat,
							  pdNumFormat,
							  ccsid,
							  indent,
							  checkData);


				// check if this is a redefines and the redefine is larger than the original variable
				if ((1 == j) && (fvar[i].VarRedefine > 0))
				{
					// check the lengths
					if (fvar[i].VarLen > fvar[fvar[i].VarRedefine].VarLen)
					{
						// insert an error message if a larger area redefines a smaller one
						sprintf(printLoc + strlen(printLoc), "***** Redefine larger than original definition\r\n");
					}
				}

				// wrote a line of output - do not suppress error messages
				offsetError = FALSE;

				if (occurCount > 0)
				{
					// now, write out the structure
					rc = displayDat(printArea + strlen(printArea), 
									maxArea - strlen(printArea), 
									fvar[i].VarChild, 
									fvar[i].VarLastChild, 
									dataIn + dataOffset, 
									endData,
									iteration,
									occurOfs + dataOffset,
									charFormat, 
									numFormat,
									pdNumFormat,
									ccsid,
									indent,
									checkData);
				}

				// move on to the next offset
				dataOffset += fvar[i].VarLen + fvar[i].VarEndSlack;
				printLoc += strlen(printLoc);

				// do the next occur
				iteration++;
				j++;
			}

			// we have done all the children - don't do them again
			dataOffset += fvar[i].VarLen;
//			i = fvar[i].VarLastChild;
		}

		if ((dataIn + fvar[i].VarOffset + fvar[i].VarLen) > endData)
		{
			if (!offsetError)
			{
				offsetError = TRUE;
				strcat(printLoc, "***** RFHUtil formating error - Copy book offset exceeds data area\r\n");
			}

			// break out of this level
			i = lastvar;
		}

		i = fvar[i].VarNext;
   }

/////////////////////////////////
// Finished - return normally
/////////////////////////////////

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::displayDat() rc=%d", rc);

		// trace exit from displayDat
		logTraceEntry(traceInfo);
	}

	return rc;
}

void CCopybook::increaseOffsets(const int varnum, const int extraLen)

{
	int		i=varnum;

	while (i != 0)
	{
		if (fvar[i].VarChild > 0)
		{
			increaseOffsets(fvar[i].VarChild, extraLen);
		}

		fvar[i].VarOffset += extraLen;

		i = fvar[i].VarNext;
	}
}

void CCopybook::updateLength(const int firstVar,
							 const unsigned char *dataIn, 
							 const unsigned int dataLen,
							 const int numFormat,
							 const int pdNumFormat,
							 char *printArea)
{
	int		i=firstVar;
	int		occ;
	int		extraLen;
	int		j;
	int		tempParent;

	// note that the second check is necessary to prevent running past
	// the end of the buffer
	while (i != 0)
	{
		// check if this variable has any children
		// if so, check them
		if (fvar[i].VarChild != 0)
		{
			updateLength(fvar[i].VarChild, dataIn, dataLen, numFormat, pdNumFormat, printArea);
		}

		// search for any variables that have an occurs depending on clause
		if (fvar[i].VarOccurDepend != 0)
		{
			// check if the value of the occurs variable is within the data area
			if ((fvar[fvar[i].VarOccurDepend].VarOffset + fvar[fvar[i].VarOccurDepend].VarLen) < (int) dataLen)
			{
				occ = getVarValue(fvar[i].VarOccurDepend, dataIn, numFormat, pdNumFormat);

				// don't allow a negative occurs value
				if (occ < 0)
				{
					occ = 0;
				}
			}
			else
			{
				occ = 0;
				sprintf(printArea + strlen(printArea), "***** RFHUtil format error - Occurs value past end of input buffer\n");
			}

			// check if the value is greater than the maximum allowed, assuming one was found
			if ((occ > fvar[i].VarOccurMax) && (fvar[i].VarOccurMax > 0))
			{
				sprintf(printArea + strlen(printArea), "***** RFHUtil format error - Occurs value exceeds maximum - occurCount=%d maximum=%d\r\n", 
						occ,fvar[i].VarOccurMax);
				occ = fvar[i].VarOccurMax;
			}

			if (occ < fvar[i].VarOccur)
			{
				sprintf(printArea + strlen(printArea), "***** RFHUtil format error - Occurs value below minimum - value=%d minimum=%d\r\n", 
						occ,fvar[i].VarOccur);
			}

			extraLen = (occ - fvar[i].VarOccur) * fvar[i].VarLen;
			if (extraLen != 0)
			{
				// greater, we need to update the offsets of any later peers
				// and their children of this variable and the same for each
				// of the parent chain of this variable.
				tempParent = i;
				while (tempParent != 0)
				{
					increaseOffsets(fvar[tempParent].VarNext, extraLen);

					// move on to the next level
					tempParent = fvar[tempParent].VarParent;

					// check if this parent also has an occurs depending on clause
					//if ((fvar[tempParent].VarFlags & OCCURS_VAR) > 0)
					if (fvar[tempParent].VarOccurDepend > 0)
					{
						// stop now - we will catch this one later
						tempParent = 0;
					}
				}
			}

			// increase the lengths of the parent fields
			j = fvar[i].VarParent;
			while (j != 0)
			{
				fvar[j].VarLen += extraLen;

				// check if this variable also has an occurs depending on clause
				//if ((fvar[j].VarFlags & OCCURS_VAR) > 0)
				if (fvar[j].VarOccurDepend > 0)
				{
					// stop now - we will catch this one later
					j = 0;
				}
				else
				{
					j = fvar[j].VarParent;
				}
			}

			// make it look like a regular occurs clause
			fvar[i].VarOccur = occ;
//			fvar[i].VarOccurMax = 0;
		}

		i = fvar[i].VarNext;
	}
}

int CCopybook::formatData(char *printArea, 
						  const int maxArea,
						  const unsigned char *dataIn, 
						  const unsigned int dataLen, 
						  const int charFormat, 
						  const int numFormat,
						  const int pdNumFormat,
						  const int ccsid,
						  const int indent,
						  const int checkData)

{
	int RC=0;
	int extra=0;	// extra amount from occurs depending on value
	int				tempFvarLen=0;
	COPY_STRUCT *	tempFvar;
	char		traceInfo[512];		// work variable to build trace message

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::formatData() dataIn=0x%x dataLen=%d", (unsigned int)dataIn, dataLen);

		// trace entry to formatData
		logTraceEntry(traceInfo);
	}

	linesText = 0;
	bytesText = 0;

	// initialize the return string
	printArea[0] = 0;
	if ((dataIn == NULL) || (dataLen == 0))
	{
		// no data - nothing to do
		return 0;
	}

	// check if we need to fix up the copybook 
	if (dependCount > 0)
	{
		// we need to fix up the copybook offsets
		// we will use a temporary copy of the variable area
		// first, allocate a new area to hold a copy of 
		// the original copybook structure and copy the
		// original structure into this area
		tempFvarLen = (maxvar + 2) * sizeof(COPY_STRUCT);
		tempFvar = (COPY_STRUCT *)rfhMalloc(tempFvarLen, "TEMPFVAR");
		memset(tempFvar, 0, tempFvarLen);
		memcpy(tempFvar, fvar, tempFvarLen);

		// now fix up the variable area, so that any 
		// occurs depending on clauses appear as normal
		// occurs clauses
		updateLength(1, dataIn, dataLen, numFormat, pdNumFormat, printArea);

		copyBookSize = fvar[1].VarLen;
	}

	displayDat(printArea, 
			   maxArea, 
			   1, 
			   maxvar, 
			   dataIn, 
			   dataIn + dataLen,
			   0,
			   0,
			   charFormat, 
			   numFormat,
			   pdNumFormat,
			   ccsid,
			   indent,
			   checkData);

	// append the size of the data area and the copy book
	sprintf(printArea + strlen(printArea), "Copy book size %d, Data area size %d", 
			copyBookSize, dataLen);

	// check if the sizes match
	if ((int)dataLen != copyBookSize)
	{
		sprintf(printArea + strlen(printArea), "\r\n***** RFHUtil error - Data length does not match copybook");
		RC = 1;
	}

	// restore the original variable table and free the acquired storage
	if (dependCount > 0)
	{
		memcpy(fvar, tempFvar, tempFvarLen);
		rfhFree(tempFvar);
		copyBookSize = fvar[1].VarLen;
	}

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::formatData() RC=%d", RC);

		// trace exit from formatData
		logTraceEntry(traceInfo);
	}

	return RC;
}

////////////////////////////////////////////////////
//
// Build the next line by reading one or more lines
// from the input file and concatenating to form
// a single logical line in memory
//
////////////////////////////////////////////////////

int CCopybook::buildNextLine(char *line, const int maxSize, FILE *commareafile, int *inCounter, int *inFileCount)

{
	int			i;
	int			len;
	int			remaining = maxSize;
	char		*cline;					// Current line pointer
	char		cobolLine[MAX_LINE_LEN];
	char		traceInfo[512];			// work variable to build trace message

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::buildNextLine() line=0x%8.8x maxSize=%d inCounter=%d inFileCount=%d", (unsigned int)line, maxSize, (*inCounter), (*inFileCount));

		// trace entry to buildNextLine
		logTraceEntry(traceInfo);
	}

	// start with a null statement
	line[0] = 0;

	// loop until either a complete statement is built or
	// end of file is reached.  
	while (1)
	{
		if (fgets(cobolLine, MAX_LINE_LEN, commareafile) == NULL)
		{
			// Indicate end of file has been reached
			line[0] = '\0';
			return 1;
		}

		// increment the source line counters
		(*inCounter)++;
		(*inFileCount)++;

		// get rid of any information in columns 73-80
		cobolLine[72] = 0;

		// get rid of the newline character in the line, if present
		len = strlen(cobolLine) - 1;
		if (cobolLine[len] == '\n')
		{
			cobolLine[len--] = '\0';
		}

	  // get rid of any trailing tabs, etc
//	  while (cobolLine[len] < ' ')
//	  {
//         cobolLine[len--] = '\0';
//	  }

		// check that the line is long enough and not a comment
		if ((strlen(cobolLine) > 7) && (cobolLine[6] != '*') &&
		  (cobolLine[6] != '/') && (cobolLine[0] != '*'))
		{
			// start at column 8 unless a continuation line
			if (cobolLine[6] == '-')
			{
				cline = cobolLine + 11;
			}
			else
			{
				cline = cobolLine + 7;
			} // endif

			// skip leading blanks
			cline = skipBlanks(cline);

			// Check if the line is blank
			if (strlen(cline) > 0)
			{
				//skip trailing blanks
				Rtrim(cline);
				i = strlen(cline);

				if (i > 0)
				{
					if (i < remaining)
					{
						// concatenate this to the output line
						strcat(line, cline);
						remaining -= i;
					}
					else
					{
						// issue error message - statement too large
						// stop processing - something is wrong
						i = strlen(line);
						return 1;
					} // endif

					// check if we have found a period at the end
					i = strlen(line) - 1;

					if ('.' == line[i])
					{
						// remove the period
						line[i] = 0;

						// return with the statement
						break;
					}
					else
					{
						// Insert a blank as a delimiter
						strcat(line," ");
					} // endif
				} // endif
			} // endif
		} // endif
	} // endwhile

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::buildNextLine() inCounter=%d inFileCount=%d", (*inCounter), (*inFileCount));

		// trace exit from buildNextLine
		logTraceEntry(traceInfo);
	}

   return 0;
}

//////////////////////////////////////////////////////
//
// This routine finds the target variable name of a
// redefines clause and moves it into
//
//////////////////////////////////////////////////////

char * CCopybook::processRedefine(char * redefines)

{
	char * nameEnd;
	char		traceInfo[512];		// work variable to build trace message

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::processRedefine() redefines=%.16s", redefines);

		// trace entry to processRedefine
		logTraceEntry(traceInfo);
	}

	// search for a delimiter after redefines 
	redefines = findBlank(redefines);

	// find the variable name that is redefined
	redefines = skipBlanks(redefines);

	// find the end of the variable name 
	nameEnd = findBlank(redefines);

	// did we find anything?
	if (nameEnd != NULL)
	{
		// truncate the variable name
		nameEnd[0] = 0;
	}

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::processRedefine() redefines=%.16s", redefines);

		// trace exit from processRedefine
		logTraceEntry(traceInfo);
	}

	return redefines;
}

//////////////////////////////////////////////////////
//
// This routine adjusts the length of the picture
// clause to match a binary number's real length
//
//////////////////////////////////////////////////////

int CCopybook::handleBinaryLength(int piclength)

{
   int len;

   // adjust the length based on the picture
   if (piclength < 5)
   {
      // handle pic 9(1) to 9(4)
      len = 2;
   }
   else
   {
      // handle pic 9(5) or greater
      len = 4;
   }

   return len;
}

///////////////////////////////////////////////////////////////
//
// This routine adjusts the length of a usage PACKED
// clause mask element.  It runs after the picture clause
// if any, has been parsed
//
///////////////////////////////////////////////////////////////

int CCopybook::handlePackedLength(int length)

{
   // divide the length by 2 to account for packed, and allow for the sign byte
   // set the usage and the length
   return (length / 2) + 1;
}

///////////////////////////////////////////////////////////////
//
// This routine handles a usage COMP clause mask element
// It runs after the picture clause, if any, has been
// parsed.  It will adjust or set the length from the
// picture clause as needed
//
///////////////////////////////////////////////////////////////

char CCopybook::handleComp(char *usage, int *length)

{
	char	*loc;
	char	use;
	char	traceInfo[512];		// work variable to build trace message

	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::handleComp() usage=%.16s", usage);

		// trace entry to handleComp
		logTraceEntry(traceInfo);
	}

   // step past the keyword COMP
   loc = usage + strlen(COMP_STR);

   // set if just plain comp
   use = USE_BINARY;

   // next, check if there is a specific type of comp.
   // note that the keyword can be COMP or COMPUTATIONAL
   while ((loc[0] != ' ') && (loc[0] != '-') &&
          (loc[0] != '\0') && (loc[0] != '.'))
   {
       loc++;
   }

   // check if we found a dash
   if (loc[0] == '-')
   {
      switch (loc[1])
      {
         case '1':
            // set the usage and the length
            use = USE_FLOAT1;
            *length = 4;
            break;

         case '2':
            // set the usage and the length
            use = USE_FLOAT2;
            *length = 8;
            break;

         case '3':
            // set the usage
            use = USE_PACKED;

            // adjust the length
            *length = handlePackedLength(*length);

            break;

         case '4':
            // set the usage
            use = USE_BINARY;

            // calculate the actual length of the item
            (*length) = handleBinaryLength(*length);

            break;

         case '5':
            // set the usage
            use = USE_BINARY;

            // calculate the actual length of the item
            (*length) = handleBinaryLength(*length);

            break;

         default:
            break;

      }
   }
   else
   {
      // no dash - must be plain COMP item
      // calculate the actual length of the item
      (*length) = handleBinaryLength(*length);
   }

  	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::handleComp() use=%d length=%d", use, (*length));

		// trace exit from handleComp
		logTraceEntry(traceInfo);
	}

	return use;
}

////////////////////////////////////////////////////////////////
//
// This routine handles an OCCURS clause
//
////////////////////////////////////////////////////////////////

void CCopybook::handleOccurs(char *occurs, int *dependCount)

{
	int  count=0;
	int  i;
	char *countend;
	char *depend;
	char *dependend;
	char *tobegin;
	char *toend;
	char *onbegin;
	char		traceInfo[512];		// work variable to build trace message

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::handleOccurs() occurs=%.16s", occurs);

		// trace entry to handleOccurs
		logTraceEntry(traceInfo);
	}

	// find the delimiting blank
	occurs = findBlank(occurs);

	// Make sure we don't run past the end of the statement
	if (occurs[0] != 0)
	{
		// find the count
		occurs = skipBlanks(occurs);

		// find the end of the count and add a delimiter, unless it is the end of the depends clause
		countend = findBlank(occurs);

		// check for just a depending on clause
		if (memcmp(occurs, DEPENDING_STR, sizeof(DEPENDING_STR) - 1) != 0)
		{
			if (countend[0] == ' ')
			{
				countend++[0] = 0;
			}

			// Convert the count to an integer
			fvar[maxvar].VarOccur = atoi(occurs);
		}
		else
		{
			// move the pointer back to the depending word
			countend = occurs;
		}

		// Now, check if we have a depending on clause
		depend = findKeyword(countend, DEPENDING_STR);

		if (depend != NULL)
		{
			// we may have a depending on clause
			// find the next number after the first count
			// ignoring the keyword TO if present

			// check for a TO clause
			tobegin = findKeyword(countend, TO_STR);

			if (tobegin != NULL)
			{
				// we have a TO clause
				// skip past it to find the maximum occurs number
				countend = tobegin + sizeof(TO_STR) - 1;

				// skip any intervening blanks
				tobegin = skipBlanks(countend);

				// make sure we are not at the end of the data
				if (tobegin[0] > 0)
				{
					toend = findBlank(tobegin);
					toend[0] = 0;

					fvar[maxvar].VarOccurMax = atoi(tobegin);
				}
			}

			 // finally, isolate the name of the depending on clause
			 depend += sizeof(DEPENDING_STR) - 1;
			 onbegin = findKeyword(depend, ON_STR);
			 if (onbegin != NULL)
			 {
				 depend = onbegin + sizeof(ON_STR) - 1;
			 }

			 // skip any intervening blanks
			 depend = skipBlanks(depend);

			 // make sure we didn't run past the end of the line
			 if (depend[0] > 0)
			 {
				 // we now have a pointer to the beginning of the variable name
				 // find the end of the variable name
				dependend = findBlank(depend);
				dependend[0] = 0;

				// find the target of the depending on clause
				i = maxvar - 1;
				while ((i > 0) && (strcmp(namObj->getNameAddr(fvar[i].VarName), depend) != 0))
				{
					i--;
				}

				// remember the occurs depending on variable name
				fvar[maxvar].VarOccurDepend = i;

				// remember that we have occurs depending on clauses in the copybook
				(*dependCount)++;
			 }
		 }
	} // endif

  	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::handleOccurs() dependCount=%d", (*dependCount));

		// trace exit from handleOccurs
		logTraceEntry(traceInfo);
	}
}

/////////////////////////////////////////////////////////////////////
//
// This routine handles a picture clause mask element.  It sets
//  the picture type as appropriate, and updates the picture
//  length.  It handles counts following a picture element, as
//  in X(9) or 9(4).  The location pointer within the picture is
//  updated and returned to the calling routine
//
/////////////////////////////////////////////////////////////////////

char * CCopybook::handleElement(char *pic, char *type, int *len)

{
	char	*loc;
	int		length=1;
	char	ch;

	// Set the type if it has not been previously set
	if (0 == type[0])
	{
		ch = toupper(pic[0]);

	   if (('Z' == ch) || ('-' == ch) || ('9' == ch) ||
		   ('$' == ch) || ('*' == ch) || ('+' == ch))
	   {
		   type[0] = '9';
	   }
	   else
	   {
		   if (('G' == ch) || ('N' == ch) || 
			   ('B' == ch) || ('X' == ch))
		   {
			   type[0] = 'X';
		   }
		   else
		   {
			   if ('A' == ch)
			   {
				   type[0] = 'A';
			   }
		   }
	   }
	}

	// get location of the next character
	loc = pic + 1;

	if (loc[0] == '(')
	{
		// find the end of the count
		while((loc[0] != ')') && (loc[0] != '\0'))
		{
			loc++;
		}

		// move on past the close parentheses, if found
		if (loc[0] == ')')
		{
			// terminate the string
			loc[0] = 0;

			loc++;
		}

		// get the count of the number of characters
		length = atoi(pic + 2);
	}

	// Check for DBCS character
	if ((type[0] == 'G') || (type[0] == 'N'))
	{
		// two bytes per character
		length = length << 1;
	} // endif

	// update the length
	*len = (*len) +length;

	// Return the new location pointer
	return loc;
}

/////////////////////////////////////////////////////////////////////
//
// This routine handles a picture clause
//
// The following characters in a picture clause occupy one
//  position:
//
//    A - Alphabetic Character
//    B - Blank Character
//    X - Alphameric Character
//    9 - Numeric Character
//    . - Period
//    , - Comma
//    * - Asterisk (Check protect character)
//    $ - Dollar Sign (Currency Symbol)
//    / - Slash, used as delimiter in dates
//
//  The following characters occupy no space
//
//    E - Exponent in floating point number
//    P - Decimal point scaling position
//    S - Indicates signed number
//    V - Decimal point assumed position
//
//  The following characters or sequences occupy two positions
//
//    CR - Indicates credit balance
//    DB - Indicates debit balance
//    N  - DBCS Character
//    G  - DBCS Character
//
//  This routine will attempt to determine the type of data
//  within a field, for purposes of justification, translation
//  etc
//
//  This routine does not handle mixed type fields, to the extent
//  they are legal within COBOL.  It will set the field type
//  to match the first recognized type character, and will only
//  set the type to A (Alphabetic), 9 (Numeric) or X (Alphameric)
//
/////////////////////////////////////////////////////////////////////

int CCopybook::handlePic(char *pic, char *type, char *flags)

{
	char		*val;
	char		savetype=0;
	int			length=0;
	int			i;
	char		picInfo[32];
	char		traceInfo[512];		// work variable to build trace message

	// point past the PIC keyword
	val = pic + 3;

	// find the end of the PIC keyword
	val = findBlank(val);

	// find the picture mask
	val = skipBlanks(val);

 	if (traceEnabled)
	{
		// get the picture info as a properly terminated string
		i = 0;
		while ((val[i] > ' ') && (i < sizeof(picInfo) - 1))
		{
			picInfo[i] = val[i];
			i++;
		}

		// terminate the sring
		picInfo[i] = 0;

		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::handlePic() pic=%s", picInfo);

		// trace entry to handlePic
		logTraceEntry(traceInfo);
	}

	// process the mask
	while (val[0] > ' ')
	{
		switch (toupper(val[0]))
		{
		// These characters take a space and set the variable
		//  type.
		case 'A':	// Alphabetic character
		case 'X':	// Alphameric character
		case 'N':	// DBCS Character
		case 'G':	// DBCS Character
		case '9':	// Numeric character
		case 'Z':	// Numeric character, with leading zero suppression.
		case '-':	// minus sign
		case '+':	// Constant plus sign
		case '*':	// Check protection character
		case '$':	// Currency symbol
			{
				val = handleElement(val, &savetype, &length);
				break;
			}

		// These characters take a space but do not set the
		//  variable type.
		case '0':      // Constant zero
		case '/':      // Constant slash, used in dates
		case '.':      // Constant period
		case ',':      // Constant comma
		case 'B':      // Blank space
		case 'C':      // First character of CR
		case 'D':      // First character of DB
		case 'E':      // Exponent delimiter
		case 'R':      // Second character of CR
			{
				if (('D' == toupper(val[0])) && ('B' == toupper(val[1])))
				{
					val++;
					length++;
				}

				if (('C' == toupper(val[0])) && ('R' == toupper(val[1])))
				{
					val++;
					length++;
				}

				val++;
				length++;

				break;
			}

		// These characters do not take a space
		case 'S':      // Signed number
			{
				flags[0] |= SIGNED_NUMBER;
				val++;
				break;
			}

		// These characters do not take a space and are ignored
		//  as are any unrecognized characters
		case 'P':      // Decimal precision
		case 'V':      // Assumed Decimal point location
		default:
			{
				val++;
				break;
			}
		}
	}

	type[0] = savetype;

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::handlePic() length=%d type=%c flags=%c", length, savetype, (*flags));

		// trace exit from handlePic
		logTraceEntry(traceInfo);
	}

	// return the length of the picture clause
	return length;
}

//////////////////////////////////////////////////////////
//
//  Subroutine to convert the data to a hexidecimal
//   representation
//
//////////////////////////////////////////////////////////

void CCopybook::buildHex(char *dataText, int len, const unsigned char *msgdata, const int numFormat)

{
	int	i;
	int	j=0;
	char ch;

	for (i=0; i < len; i++)
	{
		if (numFormat != NUMERIC_PC)
		{
			ch = (unsigned char) msgdata[i] >> 4;
		}
		else
		{
			ch = (unsigned char) msgdata[len - i - 1] >> 4;
		}

		dataText[j++] = HEX_NUMBERS[ch];

		if (numFormat != NUMERIC_PC)
		{
			ch = (unsigned char) msgdata[i] & 0x0F;
		}
		else
		{
			ch = (unsigned char) msgdata[len - i - 1] & 0x0F;
		}

		dataText[j++] = HEX_NUMBERS[ch];
	}

	// terminate the string
	dataText[j] = 0;
}

////////////////////////////////////////////
//
// Routine to check if the data is numeric
//
////////////////////////////////////////////

BOOL CCopybook::checkNumber(const unsigned char *dataloc, const int len)

{
	BOOL	valid=TRUE;
	int		i;

	i = 0;
	while (valid && (i < len))
	{
		if ((dataloc[i] < '0') || (dataloc[i] > '9'))
		{
			valid = FALSE;
		}

		i++;
	}

	return valid;
}

///////////////////////////////////////////////////
//
// Routine to check for valid packed decimal data
//
///////////////////////////////////////////////////

int CCopybook::checkPacked(const unsigned char *dataloc, const int len, const int numFormat)

{
	int	rc=0;
	int i;
	char ch;
	char ch2;

	if (len > 0)
	{
		for (i=0; i < len - 1; i++)
		{
			// check each nibble to be sure it is a valid number
			if (NUMERIC_PC == numFormat)
			{
				ch  = dataloc[len - i - 1] >> 4;
				ch2 = dataloc[len - i - 1] & 0x0F;
			}
			else
			{
				ch = dataloc[i] >> 4;
				ch2 = dataloc[i] & 0x0F;
			}

			if (ch > 9)
			{
				rc = 1;
			}

			if (ch2 > 9)
			{
				rc = 1;
			}
		}

		// check the sign byte
		if (NUMERIC_PC == numFormat)
		{
			ch  = dataloc[0] >> 4;
			ch2 = dataloc[0] & 0x0F;
		}
		else
		{
			ch  = dataloc[len - 1] >> 4;
			ch2 = dataloc[len - 1] & 0x0F;
		}

		if (ch > 9)
		{
			rc = 1;
		}

		if (ch2 < 10)
		{
			rc = 1;
		}
	}

	return rc;
}

////////////////////////////////////////////////
//
// Routine to check for valid ASCII characters
//
////////////////////////////////////////////////

int CCopybook::checkASCII(const unsigned char *msgData, int len)

{
	int	RC=0;
	int	i;

	for (i=0; i < len; i++)
	{
		// check for characters outside of the normal ASCII range of blank (' ') to 127
		if ((msgData[i] < ' ') || (msgData[i] > 127))
		{
			RC = 1;
		}
	}

	return RC;
}

////////////////////////////////////////////////
//
// Extract a value from a packed decimal field
//
// The input data is in packed decimal format,
// and the output is a character string consisting
// of numbers.
//
// The pcData field is set to a 0 if the data is
// Intel format and 1 if the data is host format.
//
// The length field is the number of bytes that the
// packed decimal number occupies in storage.  The
// resulting decimal number will be 2 * length - 1
// bytes long, plus a trailing null character.
//
// If the number is negative, then a minus sign 
// will be prepended.
//
////////////////////////////////////////////////

void CCopybook::getPackedNumber(char *dataArea, const unsigned char *dataIn, int length, int pcdata)

{
	int		i;
	char	*dataloc;

	// check for zero or negative length and exit if true
	if (length <= 0)
	{
		// return a null string as the result
		dataArea[0] = 0;
		return;
	}

	// get a pointer to the output area
	dataloc = dataArea;

	if (NUMERIC_PC == pcdata)
	{
		// check if the number is negative
		if (((dataIn[0] & 15) == 13) || ((dataIn[0] & 15) == 11))
		{
			// prepend a minus sign
			dataloc[0] = '-';
			dataloc++;
		}

		// handle little endian (Intel) numbers
		// get the first half byte as an ASCII number 0 through 9
		dataloc[2 * (length - 1)] = (dataIn[0] >> 4) + 48;

		i = 1;
		while (i < length)
		{
			dataloc[2 * (length - i) - 2] = (dataIn[i] >> 4) + 48;		// get the left half-byte as an ASCII number 0 through 9
			dataloc[(2 * (length - i)) - 1] = (dataIn[i] & 15) + 48;	// get the right half-byte as an ASCII number 0 through 9
			i++;
		}
	}
	else
	{
		// check if the number is negative
		if (((dataIn[length - 1] & 15) == 13) || ((dataIn[length - 1] & 15) == 11))
		{
			// prepend a minus sign
			dataloc[0] = '-';
			dataloc++;
		}

		// handle big endian (host) numbers
		i = 0;
		while (i < (length - 1))
		{
			dataloc[2 * i] = (dataIn[i] >> 4) + 48;						// get the left half-byte as an ASCII number 0 through 9
			dataloc[(2 * i) + 1] = (dataIn[i] & 15) + 48;				// get the right half-byte as an ASCII number 0 through 9
			i++;
		}

		// get the last half byte
		dataloc[2 * (length - 1)] = (dataIn[length - 1] >> 4) + 48;		// 48 is an ASCII zero
	}

	// terminate the result string
	dataloc[(2 * length) - 1] = 0;

	return;
}

///////////////////////////////////////////////////////
//
// Routine to print a line for an individual variable.
//
///////////////////////////////////////////////////////

void CCopybook::varReportLine(char *textArea, 
							  const int varnum, 
							  const unsigned char *msgdata,
							  const unsigned char *endData,
							  char *redefname, 
							  int iteration,
							  int occOffset,
							  const int charFormat, 
							  const int numFormat,
							  const int pdNumFormat,
							  const int ccsid, 
							  const int indent,
							  const int checkData)

{
	int				i1;
	int				i2;
	int				j;
	int				len;
	int				offset;
	int				depth;
	char			*ptr;
	char			*extra;
	char			occ[10];
	char			type[10];
	char			lineOfs[32];
	char			dataText[160];
	unsigned char	tempText[160];

	linesText++;

	textArea[0] = 0;               

	// get the number of the occurs into a string
	//if (fvar->VarOccur > 0)
	if (iteration > 0)
	{
		sprintf(occ, "%d", iteration);
	}
	else
	{
		// set the string value to a null
		occ[0] = 0;
	}

	strcpy(type, "     ");
	memset(dataText, 0, sizeof(dataText));
	dataText[0] = 0;
	len = fvar[varnum].VarLen;
	offset = fvar[varnum].VarOffset;

	if ((fvar[varnum].VarChild == 0) && (fvar[varnum].VarLevel > 1) && (fvar[varnum].VarType > 0))
	{
		// get some of the data value into a string
		switch (fvar[varnum].VarUsage)
		{
		case USE_DISPLAY:
			{
				// get the length but no more than 128 bytes 
				if (len > 128)
				{
					len = 128;

					// Indicate there is additional data that is not shown
					extra = "...";
				}
				else
				{
					extra = "";
				}

				switch (fvar[varnum].VarType)
				{
					case '9':
						{
							// Picture 999
							if ((1 == checkData) && (len < sizeof(tempText) - 1))
							{
								memset(tempText, 0, sizeof(tempText));
								if (CHAR_EBCDIC == charFormat)
								{
									translateEbcdicData(tempText, msgdata, len, ccsid);
									strcat((char *)tempText, extra);
								}
								else
								{
									memcpy(tempText, msgdata, len);
									strcat((char *)tempText, extra);
								}

								if (!checkNumber(tempText, fvar[varnum].VarLen))
								{
									sprintf(textArea,"***** RFHUtil format error - Invalid numeric data in next field\r\n");
								}
							}

							strcpy(type, "NUMB ");
							break;
						}
					case 'A':
						{
							// Picture AAA
							strcpy(type, "ALPHA");
							break;
						}
					default:
						{
							// Picture XXX 
							strcpy(type, "CHAR ");
							break;
						}
				}

				// make sure we don't go past the end of the data
				if (msgdata + len > endData)
				{
					// limit the length to the available data
					len = endData - msgdata;

					// but never less than zero
					if (len < 0)
					{
						len = 0;
					}
				}

				// clear the temporary data area
				memset(tempText, 0, sizeof(tempText));
				if (len > 0)
				{
					if (CHAR_EBCDIC == charFormat)
					{
						translateEbcdicData(tempText, msgdata, len, ccsid);

						// check for signed numbers
						if ((fvar[varnum].VarFlags & SIGNED_NUMBER) > 0)
						{
							// check for positive and negative values
							i1 = msgdata[len - 1] & 240;
							i2 = msgdata[len - 1] & 15;

							// first, check for C0-C9
							if (192 == i1)
							{
								// correct the last character
								tempText[len - 1] = '0' + i2;
							}
							else
							{
								// check for either B0 or D0
								if ((176 == i1) || (208 == i1))
								{
									// append minus sign
									tempText[len - 1] = '0' + i2;
									tempText[len++] = '-';
									tempText[len] = 0;
								}
							}
						}
					}
					else
					{
						if (CHAR_ASCII == charFormat)
						{
							memcpy(tempText, msgdata, len);
						}
						else
						{
							// multi-byte message - make sure we get
							// complete multibyte characters
							j = 0;
							while (j < len)
							{
								j += _mbclen(msgdata + j);
							}

							len = j;
							memcpy(tempText, msgdata, len);
						}
					}
				}

				// replace any binary zeros with blanks to avoid spurious hex displays
				for (j=0; j < len; j++)
				{
					// check for binary zero
					if (0 == tempText[j])
					{
						// convert to a space character
						tempText[j] = ' ';
					}
				}

				// check if the data is ascii
				if ((checkASCII(tempText,len) == 0) || (charFormat > CHAR_EBCDIC))
				{
					// data is ASCII
					for (j=0; j < len; j++)
					{
						dataText[j] = tempText[j];
					}

					dataText[len] = 0;
				}
				else
				{
					// data is not ASCII - display 16 bytes in hex
					// maximum of 16 bytes
					if (len > 16)
					{
						len = 16;
					}

					// display the data as hex - use NUMERIC_HOST so that the order is not reversed
					buildHex(dataText, len, tempText, NUMERIC_HOST);
				}

				break;
			}
		case USE_PACKED:
			{
				// packed decimal
				strcpy(type, "PD   ");

				if (msgdata + len > endData)
				{
					dataText[0] = 0;
				}
				else
				{
					// check if the packed decimal data is valid 
					if (checkPacked(msgdata, len, pdNumFormat) == 1)
					{
						sprintf(textArea,"***** RFHUtil format error - Invalid Packed Decimal data in next field\r\n");
						buildHex(dataText, len, msgdata, pdNumFormat);
					}
					else
					{
						getPackedNumber(dataText, msgdata, len, pdNumFormat);

						// terminate the string 
						dataText[2 * len] = 0;
					}
				}

				break;
			}
		case USE_BINARY:
			{
				// binary
				strcpy(type, "INT  ");
				if (msgdata + len > endData)
				{
					dataText[0] = 0;
				}
				else
				{
					if (len > 4)
					{
						len = 4;
					}

					buildHex(dataText, len, msgdata, numFormat);
				}

				break;
			}
		case USE_INDEX:
			{
				// index pointer
				strcpy(type, "IDX  ");

				if (msgdata + len > endData)
				{
					dataText[0] = 0;
				}
				else
				{
					buildHex(dataText, len, msgdata, numFormat);
				}

				break;
			}
		case USE_POINTER:
			{
				// pointer
				strcpy(type, "PTR  ");

				if (msgdata + len > endData)
				{
					dataText[0] = 0;
				}
				else
				{
					buildHex(dataText, len, msgdata, numFormat);
				}

				break;
			}
		case USE_FLOAT1:
			{
				// floating point 4 byte
				strcpy(type, "FLT4 ");

				if (msgdata + len > endData)
				{
					dataText[0] = 0;
				}
				else
				{
					buildHex(dataText, len, msgdata, numFormat);
				}

				break;
			}
		case USE_FLOAT2:
			{
				// floating point 8 byte
				strcpy(type, "FLT8 ");

				if (msgdata + len > endData)
				{
					dataText[0] = 0;
				}
				else
				{
					buildHex(dataText, len, msgdata, numFormat);
				}

				break;
			}
		}
	}

	// calculate the offset of this line
	lineOfs[0] = 0;
	if (indent != 0)
	{
		depth = fvar[varnum].VarDepth - 1;
		while (depth > 0)
		{
			strcat(lineOfs, " ");
			depth--;
		}
	}
//	j = offsetTable[fvar[varnum].VarLevel];
//	while (strlen(lineOfs) < (unsigned int) j)
//	{
//		strcat(lineOfs, " ");
//	}

	ptr = textArea + strlen(textArea);

	// reduce the number of spaces that we need, 
	// since most fields do not have a redefinition name
	if (redefname != NULL)
	{
		// print the results to the message file
		sprintf(ptr,"%s%.2d %5d %5d %s %3.3s %-30.30s %-32.32s %s\r\n",
				lineOfs,
			    fvar[varnum].VarLevel,
				offset + occOffset,
				fvar[varnum].VarLen,
				type,
				occ,
				namObj->getNameAddr(fvar[varnum].VarName),
				dataText,
				redefname);
	}
	else
	{
		if (dataText[0] >= ' ')
		{
			// print the results to the message file
			sprintf(ptr,"%s%.2d %5d %5d %s %3.3s %-30.30s %s\r\n",
					lineOfs,
				    fvar[varnum].VarLevel,
					offset + occOffset,
					fvar[varnum].VarLen,
					type,
					occ,
					namObj->getNameAddr(fvar[varnum].VarName),
					dataText);
		}
		else
		{
			// print the results to the message file
			sprintf(ptr,"%s%.2d %5d %5d %s %3.3s %s\r\n",
					lineOfs,
				    fvar[varnum].VarLevel,
					offset + occOffset,
					fvar[varnum].VarLen,
					type,
					occ,
					namObj->getNameAddr(fvar[varnum].VarName));
		}
	}

	bytesText += strlen(ptr);
}

////////////////////////////////////////////////////////////////
//
// Subroutine to finish the processing of any occurs or
//  redefines clauses.  This happens when the level of a
//  variable is less than the previous level, or when end
//  of file has been reached
//
////////////////////////////////////////////////////////////////

void CCopybook::finishOcc(COPY_STRUCT *fvar, 
						  int occuratlevel[], 
						  int lastlevel, 
						  const int level, 
						  int *offset) 
{
	int		i;                  // Work variable
	int		tempparent;         // working variable for sync clauses
	int		tempvar;            // temporary work variable
	int		delta;              // working variable for sync clauses
//	int		addoffset;          // working variable for sync clauses
	bool	foundRedef=false;	// check if a redefines has been found

	// Finish any OCCURS and REDEFINES clauses
	for (i = lastlevel; i >= level; i--)
	{
		if (occuratlevel[i] > 0)
		{
			// Get the occurs at variable
			tempvar = occuratlevel[i];

			// see if we need to add in any slack bytes
			// These slack bytes are only needed for
			// group variables which have a variable
			// with a SYNC clause within them, and
			// which contain an OCCURS clause
			// initialize the number of slack bytes required
			delta = 0;

			// check if slack bytes are required
			if ((fvar[tempvar].VarSync > 1) && (fvar[tempvar].VarOccur > 1))
			{
				// calculate if slack bytes are needed
				// delta = (fvar[tempvar].VarOffset + fvar[tempvar].VarLen) % fvar[tempvar].VarSync;
				delta = (fvar[tempvar].VarLen) % fvar[tempvar].VarSync;
				if (delta > 0)
				{
					// compute the number of slack bytes
					delta = fvar[tempvar].VarSync - delta;

					// add the slack bytes to the group length
					fvar[tempvar].VarLen += delta;

					// Remember the number of ending slack bytes
					//fvar[tempvar].VarEndSlack = delta;

					// calculate the total number of slack bytes
					//delta = delta * (fvar[tempvar].VarOccur - 1);

					// add the extra slack to the current offset
					(*offset) += delta;
				}
			}

			// add the additional occurences to the
			// current offset
//			addoffset = fvar[tempvar].VarLen * (fvar[tempvar].VarOccur - 1);
//			(*offset) += addoffset;

			// Next, add the occurences to any parents
			// Get the first parent
			tempparent = fvar[tempvar].VarParent;

			// Process all parent variables
			while (tempparent > 0)
			{
				// first add in any added slack bytes
				fvar[tempparent].VarLen += delta;

				// Next, add the additional occurences
//				fvar[tempparent].VarLen += addoffset;

				// Get the next parent variable
				tempparent = fvar[tempparent].VarParent;
			}

			// clear the occursatlevel pointer
			occuratlevel[i] = 0;
		} // endif
	} // endfor
}

////////////////////////////////////////////////////////////////
//
// Routine to write out the contents of a variable table
// entry, primarily for debugging purposes.  A pointer to
// the first entry to be printed and the number of variables
// to be printed are passed as input, as well as a pointer
// to the listing file
//
////////////////////////////////////////////////////////////////

void CCopybook::dumpVar(int varnum, COPY_STRUCT *fvar, int count)

{
	int			i;
	char		traceInfo[512];		// work variable to build trace message

 	if (traceEnabled)
	{

		for (i=varnum; i <= varnum + count; i++)
		{
			// print out the variable in the listing file
			// create the trace line
			sprintf(traceInfo,"%.2d %-32s offset(%.5d) length(%.5d) type(%d) usage(%d) occ(%d) occmax(%d) occvar(%d) redefine(%d) sync(%d) endSlack{%d}",
						i,
						namObj->getNameAddr(fvar[i].VarName),
						fvar[i].VarOffset,
						fvar[i].VarLen,
						fvar[i].VarType,
						fvar[i].VarUsage,
						fvar[i].VarOccur,
						fvar[i].VarOccurMax,
						fvar[i].VarOccurDepend,
						fvar[i].VarRedefine,
						fvar[i].VarSync,
						fvar[i].VarEndSlack);
			// write trace entry
			logTraceEntry(traceInfo);

			sprintf(traceInfo,"     next(%d) prev(%d) nextLevel(%d) prevLevel(%d) parent(%d) child(%d) lastchild(%d) level(%d) depth(%d) flags(0x%2.2x)",
						fvar[i].VarNext,
						fvar[i].VarPrev,
						fvar[i].VarNextLevel,
						fvar[i].VarPrevLevel,
						fvar[i].VarParent,
						fvar[i].VarChild,
						fvar[i].VarLastChild,
						fvar[i].VarLevel,
						fvar[i].VarDepth,
						fvar[i].VarFlags);
			// write trace entry
			logTraceEntry(traceInfo);

			if (fvar[i].VarOccur > 0)
			{
				sprintf(traceInfo, "     occurs(%d) MaxOccurs(%d) DependingVar(%d)",
							fvar[i].VarOccur,
							fvar[i].VarOccurMax,
							fvar[i].VarOccurDepend);
				// write trace entry
				logTraceEntry(traceInfo);
			}
		}
	}
}

///////////////////////////////////////////
//
// Erase the previous copy book variables
//
///////////////////////////////////////////

void CCopybook::resetCopybook()

{
	char		traceInfo[512];		// work variable to build trace message

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Entering CCopybook::resetCopybook() maxvar=%d dependCount=%d", maxvar, dependCount);

		// trace entry to findKeyword
		logTraceEntry(traceInfo);
	}

    memset(fvar,'\0',(sizeof(COPY_STRUCT) * (MAX_VAR_COUNT + 1)));
	maxvar=0;           // Number of fields found
	dependCount=0;
}

/////////////////////////////////////////////////////
//
// Extract a value for an OCCURS DEPENDING ON clause
//
/////////////////////////////////////////////////////

int CCopybook::extractValue(COPY_STRUCT depVar, const unsigned char* dataIn, int numFormat, int pdNumFormat)

{
	int		dependCount=0;
	int		offset;
	int		length=0;
	char	tempNum[64];
	char	traceInfo[512];

	offset = depVar.VarOffset;
	length = depVar.VarLen;
	switch (depVar.VarUsage)
	{
	case USE_PACKED:
		{
			tempNum[0] = 0;
			getPackedNumber(tempNum, dataIn + depVar.VarOffset, depVar.VarLen, pdNumFormat);
			dependCount = atoi(tempNum);
			break;
		}
	case USE_DISPLAY:
		{
			dependCount = getDecimalValue(dataIn + depVar.VarOffset, depVar.VarLen);
			break;
		}
	default:
		{
			dependCount = getBinaryValue(dataIn + depVar.VarOffset, depVar.VarLen, numFormat);
			break;
		}
	}

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::extractValue() dependCount=%d length=%d offset=%d numFormat=%d pdNumFormat=%d", dependCount, length, offset, numFormat, pdNumFormat);

		// trace entry to findKeyword
		logTraceEntry(traceInfo);
	}

	return dependCount;
}

////////////////////////////////////////////////
//
// Extract a value from a packed decimal field
//
////////////////////////////////////////////////

int CCopybook::getPackedValue(const unsigned char *dataIn, int length, int numFormat)

{
	int	value=0;
	int	i;
	int	sign=1;

	// check that the length is valid
	if (length <= 0)
	{
		return 0;
	}

	if (NUMERIC_PC == numFormat)
	{
		// little endian (3C000000 = 3)
		// check the sign
		if ((dataIn[0] & 1) > 0)
		{
			sign = -1;
		}

		i = length - 1;
		while (i > 0)
		{
			value *= 10;
			value += (dataIn[i] >> 4);
			value *= 10;
			value += (dataIn[i] & 15);

			i--;
		}

		// get the first character
		value *= 10;
		value += dataIn[0] >> 4;
	}
	else
	{
		// big endian  (0000003C = 3)
		i = 0;
		while (i < (length - 1))
		{
			// get the first half byte
			value *= 10;
			value += (dataIn[i] >> 4);

			// get the second half byte
			value *= 10;
			value += dataIn[i] & 15;

			// go on to the next byte
			i++;
		}

		// get the last half byte
		value *= 10;
		value += dataIn[length - 1] >> 4;

		// check if the number is negative
		if ((dataIn[length - 1] & 1) > 0)
		{
			sign = -1;
		}
	}

	// change the sign if necessary
	value = value * sign;

	return value;
}

////////////////////////////////////////////////
//
// Extract a value from a display decimal field
// and return it as an integer.
//
////////////////////////////////////////////////

int CCopybook::getDecimalValue(const unsigned char *dataIn, int length)

{
	int	value=0;
	char	tempData[32];

	if (length <= 0)
	{
		return 0;
	}

	// make sure we don't try too long a number
	if (length > 31)
	{
		length = 31;
	}

	// capture the data into a string
	memcpy(tempData, dataIn, length);
	tempData[length] = 0;

	// convert the string to an integer
	value = atoi(tempData);

	return value;
}

////////////////////////////////////////////////
//
// Extract a value from a binary field
//
////////////////////////////////////////////////

int CCopybook::getBinaryValue(const unsigned char *dataIn, int length, int numFormat)

{
	int		value=0;
	char *	ptr;

	if (length <= 0)
	{
		return 0;
	}

	if (length > 4)
	{
		length = 4;
	}

	// check if this looks like big Endian or little Endian
	if (NUMERIC_PC == numFormat)
	{
		// little Endian
		memcpy(&value, dataIn, length);
	}
	else
	{
		// big Endian
		ptr = (char *)&value;
		memcpy(ptr + 4 - length, dataIn, length);
		value = reverseBytes4(value);
	}

	return value;
}

////////////////////////////////////////////////
//
// This routine will handle the fix ups
// that are necessary to handle multiple
// occurs depending on clauses, either 
// nested or back to back.
//
////////////////////////////////////////////////

void CCopybook::calcExtraOffsets(unsigned char *dataIn, const int numFormat, const int pdNumFormat)

{
	int		i;
	int		j;
	int		occ;
	int		fixup;

	i = 0;
	while (i < maxvar)
	{
		// check for an occurs depending on clause
		if (fvar[i].VarOccurDepend > 0)
		{
			// found one - now check if it has any peers and do the
			// fixup that is necessary
			// check if this variable has any peers
			j = fvar[i].VarNext;
			if (j > 0)
			{
				occ = getVarValue(i, dataIn, numFormat, pdNumFormat);
				fixup = (occ - fvar[i].VarOccur) * fvar[i].VarLen;
				while (j < maxvar)
				{
					fvar[j].VarExtraOffset += fixup;
					j++;
				}
			}
		}

		i++;
	}
}

////////////////////////////////////////////////
//
// This routine will calculate the value of
// a particular variable in the buffer, 
// depending on the type of the variable
// and the buffer contents and return the
// value as an integer.
//
////////////////////////////////////////////////

int CCopybook::getVarValue(int varnum, const unsigned char *dataIn, const int numFormat, const int pdNumFormat)

{
	int		value=0;
	int		len;
	const unsigned char	*ptr;
	char	workArea[256];
	char	traceInfo[512];

	// initialize the workarea
	memset(workArea, 0, sizeof(workArea));

	// get the length and location of the data
	len = fvar[varnum].VarLen;
	ptr = dataIn + fvar[varnum].VarOffset;
	switch (fvar[varnum].VarUsage)
	{
	case USE_DISPLAY:
		{
			memcpy(workArea, ptr, len);
			workArea[len] = 0;
			value = atoi(workArea);
			break;
		}
	case USE_BINARY:
		{
			value = getBinaryValue(ptr, len, numFormat);
			// get the value as hex characters in the workArea
			AsciiToHex(ptr, len, (unsigned char *)workArea);
			break;
		}
	case USE_PACKED:
		{
			value = getPackedValue(ptr, len, pdNumFormat);
			// get the value as hex characters in the workArea
			AsciiToHex(ptr, len, (unsigned char *)workArea);
			break;
		}
	}

 	if (traceEnabled)
	{
		// create the trace line
		sprintf(traceInfo, "Exiting CCopybook::getVarValue() value=%d varnum=%d len=%d numFormat=%d pdNumFormat=%d dataIn=%s", value, varnum, len, numFormat, pdNumFormat, workArea);

		// trace entry to findKeyword
		logTraceEntry(traceInfo);
	}

	return value;
}

int CCopybook::getMaxVars()

{
	return maxvar;
}

int CCopybook::getMaxLines()

{
	return getMaxChildren(1);
}

int CCopybook::getMaxChildren(int varnum)

{
	int		maxLines=0;
	int		child;
	int		occur=1;

	if (fvar[varnum].VarOccurMax > 0)
	{
		occur = fvar[varnum].VarOccurMax;
	}
	else
	{
		if (fvar[varnum].VarOccur > 1)
		{
			occur = fvar[varnum].VarOccur;
		}
		else
		{
			if (fvar[varnum].VarOccurDepend > 0)
			{
				// handle the case of an occurs depending on without
				// a minimum or maximum - therefore make sure we can 
				// handle at least 10 occurences
				occur = 10;
			}
		}
	}

	child = fvar[varnum].VarChild;
	if (0 == child)
	{
		maxLines = occur;
	}
	else
	{
		while (child > 0)
		{
			maxLines += getMaxChildren(child);
			child = fvar[child].VarNext;
		}

		maxLines++;
		maxLines *= occur;
	}

	return maxLines;
}

void CCopybook::logTraceEntry(const char *traceInfo)

{
	((CRfhutilApp *)AfxGetApp())->logTraceEntry(traceInfo);
}


