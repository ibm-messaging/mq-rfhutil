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

/********************************************************************/
/*                                                                  */
/*   comsubs.h - header file for comsubs.c                          */
/*                                                                  */
/********************************************************************/
#ifndef _CommonSubs_comsubs_h
#define _CommonSubs_comsubs_h

void Log(const char *szFormat, ...);
void LogNoCRLF(const char *szFormat, ...);
int openLog(const char * fileName);
void closeLog();
void dumpTraceData(const char * label, const unsigned char *data, unsigned int length);
char * skipBlanks(char *str);
char * findBlank(char *str);
void strupper(char *str);
void rtrim(char *str);
unsigned int iStrLen(const char * ptr);
int reverseBytes4(int var);
void AsciiToEbcdic(unsigned char *dato, const unsigned char *dati, size_t pl);
void EbcdicToAscii(const unsigned char *dati, size_t pl, unsigned char *dato);
void AsciiToHex(unsigned char *dato, const unsigned char *dati, const unsigned int pl);
void HexToAscii(char *dati, size_t pl, char *dato);
size_t Encode64(const unsigned char * input, size_t len, unsigned char * output, size_t maxLen, int padding);
size_t Decode64(const unsigned char * input, size_t len, unsigned char * output, size_t maxLen);
char * removeQuotes(char *dati);
int64_t reverseBytes8(int64_t in);
void reverseBytes24(unsigned char *in, unsigned char *out);
void reverseBytes32(unsigned char *in, unsigned char *out);
int64_t my_ato64(const char * valueptr);
void editHexParm(const char * parmName, 
				 char * parm, 
				 char * valueptr, 
				 size_t maxsize);
int checkHexParm(const char * parmName,
				  const char * parmConst,
				  char * parm, 
				  char * valueptr, 
				  int foundit,
				  size_t maxsize);
void editCharParm(const char * parmName, 
				 char * parm, 
				 char * valueptr, 
				 size_t maxsize);
int checkCharParm(const char * parmName,
				  const char * parmConst,
				  char * parm, 
				  char * valueptr, 
				  int * foundParm,
				  int foundit,
				  size_t maxsize);
void editYNParm(const char * parmName, 
				int * parm, 
				char * valueptr);
int checkYNParm(const char * parmName,
				const char * parmConst,
				int * parm, 
				char * valueptr, 
				int * foundParm,
				int foundit);
int checkIntParm(const char * parmName,
				 const char * parmConst,
				 int * value, 
				 char * valueptr, 
				 int * foundParm,
				 int foundit);

int checkI64Parm(const char * parmName,
				 const char * parmConst,
				 int64_t * value, 
				 char * valueptr, 
				 int * foundParm,
				 int foundit);

#endif
