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

// Copybook.h: interface for the CCopybook class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COPYBOOK_H__2C727229_CD43_4D53_B941_84D54888547D__INCLUDED_)
#define AFX_COPYBOOK_H__2C727229_CD43_4D53_B941_84D54888547D__INCLUDED_

#include "Names.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////
// Variable table flags bit definitions
/////////////////////////////////////////////////////////////

#define SIGNED_NUMBER	1
#define SIGN_SEP_TRAIL	2
#define SIGN_SEP_LEAD	4

/////////////////////////////////////////////////////////////
// Variable table size definition
/////////////////////////////////////////////////////////////

#define MAX_VAR_COUNT  12288

/**************************************************************/
/* Variable table structure definition                        */
/*                                                            */
/* The variable table includes all the characteristics of     */
/* the variable, as follows:                                  */
/*                                                            */
/*  VarName - Variable name, up to 31 characters.             */
/*                                                            */
/*  VarLevel - The COBOL level, from 01 to 49.  Note that     */
/*   level 66, 77 and 88 items are ignored.                   */
/*                                                            */
/*  VarOffset - The offset into the records where this        */
/*   variable begins.                                         */
/*                                                            */
/*  VarLen - The length of this item.  For primitive items,   */
/*   the length is calculated from the picture and/or usage   */
/*   clauses.  For higher levels, the length is calculated    */
/*   from the lengths of the individual items contained in    */
/*   the higher level variable.                               */
/*                                                            */
/*  VarParent - This is a pointer to the next higher level    */
/*   item.  For example, if an 03 level is defined under      */
/*   an 02 level, the VarParent of the 03 level would point   */
/*   to the 02 variable as its parent.                        */
/*                                                            */
/*  VarChild - Pointer to the first child.  This field is     */
/*   used as an indicator that this variable is not a         */
/*   primitive data area.                                     */
/*                                                            */
/*  VarLastChild - Pointer to the last child.  This field is  */
/*   used to find the last variable that is included in       */
/*   the definition of this variable.                         */
/*                                                            */
/*  VarNext - A pointer to the next variable that is a child  */
/*   of the same parent.                                      */
/*                                                            */
/*  VarPrev - A pointer to the previous variable that is a    */
/*   child of the same parent.                                */
/*                                                            */
/*  VarNextLevel - A pointer to the next variable at the      */
/*   same level.  For example, all 02 level variables are     */
/*   chained together.  The anchor for this chain is the      */
/*   firstatlevel variable, and a chain ends with a zero      */
/*   pointer.                                                 */
/*                                                            */
/*  VarPrevLevel - A pointer to the previous variable at the  */
/*   same level.  The anchor for this chain is the            */
/*   nextatlevel variable, and a chain ends with a zero       */
/*   pointer.                                                 */
/*                                                            */
/*  VarRedefine - If a redefines clause was found, this is    */
/*   a pointer to the redefined variable.  If no redefines    */
/*   clause is found, then this pointer is zero.              */
/*                                                            */
/*  VarOccur - If an occurs clause was found for this         */
/*   variable, then the VarOccur will contain the number of   */
/*   times this variable occurs.  If no occurs clause is      */
/*   found, then this clause will contain a zero.             */
/*                                                            */
/*  VarOccurMax - The maximum number of occurences from a     */
/*   DEPENDING ON clause.                                     */
/*                                                            */
/*  VarOccurDepend - A pointer to the variable which has      */
/*   the number of occurences for an OCCURS DEPENDING ON      */
/*   clause.                                                  */
/*                                                            */
/*  VarSync - If a SYNCHRONIZED clause is found, then this    */
/*   field contains the alignment boundary for this field,    */
/*   which is determined by the USAGE clause.                 */
/*                                                            */
/*  VarEndSlack - If a SYNCHRONIZED clause is found within a  */
/*   group variable with an OCCURS clause, this field will    */
/*   hold the number of slack bytes at the end of the         */
/*   variable.                                                */
/*                                                            */
/*  VarType - The variable type is calculated based on the    */
/*   picture clause.  The type will be based on the first     */
/*   recognized character in the picture clause, such as      */
/*   an X or a 9.                                             */
/*                                                            */
/*  VarUsage - If a usage clause is found, then this          */
/*   entry is set to the particular usage, and the length     */
/*   of the data entry is adjusted to match the usage.        */
/*   The default usage is display.                            */
/*                                                            */
/*  VarJust - If a JUSTIFIED clause is found, then the        */
/*   type of justification is stored in this field,           */
/*   otherwise this field contains a binary zero.             */
/*                                                            */
/**************************************************************/

typedef struct {
   int  VarLevel;
   int  VarOffset;
   int  VarLen;
   int  VarParent;
   int  VarChild;
   int  VarLastChild;
   int	VarNext;
   int	VarPrev;
   int  VarNextLevel;
   int  VarPrevLevel;
   int  VarRedefine;
   int  VarOccur;
   int	VarOccurMax;
   int	VarOccurDepend;
   int  VarSync;
   int  VarEndSlack;
   int	VarExtraOffset;
   int  VarExtraLength;
   int	VarDepth;
   char VarType;
   char VarUsage;
   char VarJust;
   char VarFlags;
   int	VarName;
} COPY_STRUCT;

class CCopybook : public CObject  
{
public:
	int getMaxLines();
	int getMaxVars();
	unsigned int copyBookSize;
	void resetCopybook();
	static void buildHex(char *dataText, int len, const unsigned char *msgdata, const int numFormat);
	int formatData(char *printArea, 
				   const int maxArea, 
				   const unsigned char *dataIn, 
				   const unsigned int dataLen, 
				   const int charFormat, 
				   const int numFormat,
				   const int pdNumFormat,
				   const int ccsid,
				   const int indent,
				   const int checkData);
	int parseCopyBook(const char * fileName);
	CCopybook();
	virtual ~CCopybook();

private:
	int cicsEncodeType;
	BOOL offsetError;
	BOOL traceEnabled;
	Names *namObj;
	COPY_STRUCT * fvar;
	int maxvar;
	int indent;
	int bytesText;
	int linesText;
	int syncCount;
	int maxvarcount;
	int dependCount;
	int getMaxChildren(int varnum);
	int calcOffsets(int varnum, int startOfs, int *newOfs);
	int buildTree(const char * fileName);
	char offsetTable[50];
	BOOL checkNumber(const unsigned char *dataloc, const int len);
	void processSync();
	void logTraceEntry(const char *traceInfo);
	void increaseOffsets(const int varnum, const int extraLen);
	char * processRedefine(char *redefines);
	int buildNextLine(char *line, const int maxSize, FILE *commareafile, int *inCounter, int *inFileCount);
	int handlePic(char *pic, char *type, char *flags);
	void handleOccurs(char *occurs, int *dependCount);
	char handleComp(char *usage, int *length);
	int getVarValue(int varnum, const unsigned char *dataIn, const int numFormat, const int pdNumFormat);
	void calcExtraOffsets(unsigned char *dataIn, const int numFormat, const int pdNumFormat);
	int extractValue(COPY_STRUCT depVar, const unsigned char* dataIn, int numFormat, int pdNumFormat);
	void dumpVar(int varnum, COPY_STRUCT *fvar, int count);
	void updateLength(const int firstVar,
					  const unsigned char *dataIn, 
					  const unsigned int dataLen,
					  const int numFormat,
					  const int pdNumFormat,
					  char *printArea);
	void finishOcc(COPY_STRUCT *fvar, 
				   int occuratlevel[50], 
				   int lastlevel, 
				   const int level, 
				   int *offset);
	void varReportLine(char *textArea, 
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
					   const int checkData);
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
							  const int checkData);
	static int checkASCII(const unsigned char *msgData, int len);
	static int checkPacked(const unsigned char *dataloc, const int len, const int numFormat);
	static char * handleElement(char *pic, char *type, int *len);
	static int handlePackedLength(int length);
	static int handleBinaryLength(int piclength);
	static void getPackedNumber(char * dataArea, const unsigned char *dataIn, int length, int pcdata);
	static int getBinaryValue(const unsigned char *dataIn, int length, int numFormat);
	static int getDecimalValue(const unsigned char *dataIn, int length);
	static int getPackedValue(const unsigned char* dataIn, int length, int numFormat);
protected:
	char * findKeyword(char *cmdline, const char *keywd);
};

#endif // !defined(AFX_COPYBOOK_H__2C727229_CD43_4D53_B941_84D54888547D__INCLUDED_)
