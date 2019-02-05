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

// comsubs.c: common subroutines.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rfhutil.h"
#include "comsubs.h"

#define DUMP_FILE_NAME "c:\\rfhdump.txt"
#define NUMERIC_PC		0

	FILE	*df=NULL;				// debugging file

	BOOL	ccsidInstalled;			// result of checking if code page is installed
	int		ccsidInsCheck;			// code page to search for

	// memory usage statistics
	int64_t	totalBytes=0;			// number of bytes allocated
	int64_t	freeBytes=0;			// number of bytes freed
	int64_t	currBytes=0;			// current bytes allocated
	int64_t	maxBytes=0;				// high water mark
	int64_t	mallocReqs=0;			// number of malloc requests
	int64_t	freeReqs=0;				// number of free requests

//////////////////////////////////////////////
//
// GetTime routine to get high precision time
//
//////////////////////////////////////////////

MY_TIME_T GetTime()

{
	MY_TIME_T	count;
	if (!QueryPerformanceCounter((LARGE_INTEGER *)&count))
	{
		count = 0;
	}

	return(count);
}

/////////////////////////////////////////////////////
//
// DiffTime - difference in microseconds between two
//  high precision times
//
/////////////////////////////////////////////////////

double DiffTime(MY_TIME_T start, MY_TIME_T end)

{
//	unsigned long diffLong;
	__int64		diff;
	__int64		freq;
	double		usecs;

	/* calulate the difference in the high frequency counter */
	diff = (end - start) * 1000000;

	//diffLong = (unsigned long)(end-start);
	/* get the frequency*/
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

	/* calculate the result */

	//usecs = (double)diffLong*1000*1000/freq;
	usecs = (double)diff / freq;

	/* return the result */
	return(usecs);
}

/////////////////////////////////////////////////////
//
// formatTimeDiffSecs - format a number of seconds
//  resulting from a difference between two times
//
/////////////////////////////////////////////////////

void formatTimeDiffSecs(char * result, double time)

{
	int		secs=0;
	int		usecs=0;
	int		i;
	char	tempStr[8];

	result[0] = 0;

	/* calculate the average latency */
	secs = (int)(time / 1000000);
	usecs = (int)(time) % 1000000;

	/* get the microseconds as a string */
	sprintf(tempStr, "%6.2d", usecs);

	/* replace any leading blanks with zeros */
	i = 0;
	while ((i < (int)strlen(tempStr)) && (' ' == tempStr[i]))
	{
		tempStr[i] = '0';
		i++;
	}

	/* display the results */
	sprintf(result, "%d.%s", secs, tempStr);
}

///////////////////////////////////
//
// Return the value of an individual
// hexadecimal character
// the returned value should be
// between 0 and 15 inclusive.
//
///////////////////////////////////

char getHexCharValue(unsigned char charIn)

{
	char	ch;
	char	result=0;

	if ((charIn > '0') && (charIn <= '9'))
	{
		result = charIn & 15;
	}
	else
	{
		ch = toupper(charIn);
		if ((ch >= 'A') && (ch <= 'F'))
		{
			result = ch - 'A' + 10;
		}
	}

	return result;
}

///////////////////////////////////
//
// Translate from ASCII to Hex
//
///////////////////////////////////

void AsciiToHex(const unsigned char *dati, unsigned int pl, unsigned char *dato)

{
	unsigned int i;
	unsigned int buffer;
	char	ch;

	buffer = 0;
	i = 0;
	while (buffer < pl)
	{
		ch = (unsigned char) dati[buffer] >> 4;
		ch = HEX_NUMBERS[ch];
		dato[i++] = ch;

		ch = (unsigned char) dati[buffer] & 0x0F;
		ch = HEX_NUMBERS[ch];
		dato[i++] = ch;
		buffer++;
	}
}

///////////////////////////////////
//
// Translate from Hex to ASCII
//
///////////////////////////////////

void HexToAscii(unsigned char *dati, unsigned int pl, unsigned char *dato)

{
	unsigned int i;
	unsigned int buffer;
	char	ch;

	buffer = 0;
	i = 0;
	while (buffer < pl)
	{
		ch = getHexCharValue(dati[i++]) << 4;
		ch += getHexCharValue(dati[i++]);
		dato[buffer++] = ch;
	}
}

char fromHex(char firstchar, char secondchar)

{
	char	result=0;

	result = (charValue(firstchar) << 4) + charValue(secondchar);

	return result;
}

int checkIfHex(LPCTSTR dataptr, int datalen)

{
	char	ch;
	int		result=1;
	int		i=0;

	while (i < datalen)
	{
		ch = toupper(dataptr[i++]);

		if (ch > '9') 
		{
			// check if this is a valid hex letter
			if ((ch < 'A') || (ch > 'F'))
			{
				result = 0;
			}
		}
		else
		{
			// check if this is a number
			if (ch < '0')
			{
				result = 0;
			}
		}
	}

	return result;
}

int charValue(char input)

{
	int	result=0;

	// check if the first character is a number
	if ((input > '0') && (input <= '9'))
	{
		result += (input - '0');
	}

	if ((input >= 'A') && (input <= 'F'))
	{
		result += input - 'A' + 10;
	}

	return result;
}

int64_t my_atoi64(const char * valueptr)

{
	int64_t		result=0;
	int			i;
	int64_t		sign=1;

	for (i=0; i < (int)strlen(valueptr); i++)
	{
		if ((valueptr[i] >= '0') && (valueptr[i] <= '9'))
		{
			result = (result * 10) + ((int64_t)valueptr[i] - '0');
		}
		else
		{
			if ('-' == valueptr[i])
			{
				sign = -1;
			}
			else
			{
				printf("Non-numeric characters found in parameter value %s\n", valueptr);
				break;
			}
		}
	}

	/* handle negative values */
	result *= sign;

	return result;
}

///////////////////////////////////
//
// Translate from ASCII to EBCDIC
//
///////////////////////////////////

void AsciiToEbcdic(unsigned char *dati, unsigned int pl, unsigned char *dato)

{
	unsigned int	i;
	int				asciiCcsid=0;	// ASCII code page to use for EBCDIC to ASCII translations

	for(i=0; i<pl; i++)
	{
		dato[i] = aetab[dati[i]];
	}
}

void convertEbcdic(char *data, int len)

{
	unsigned char tempArea[256];

	if (len < sizeof(tempArea))
	{
		memcpy(tempArea, data, len);
		AsciiToEbcdic(tempArea, len, (unsigned char *)data);
	}
}

///////////////////////////////////
//
// Translate from EBCDIC to ASCII
//
///////////////////////////////////

void EbcdicToAscii(const unsigned char *dati, unsigned int pl, unsigned char *dato)

{
	unsigned int i;
	for(i=0;i<pl;i++)
	{
		dato[i]=eatab[dati[i]];
	}
}

///////////////////////////////////////
//
// Translate a single EBCDIC character
// to a specified ASCII character
//
// This routine will try the
// translation using Windows.
//  
// If the tranlation fails a zero will
// be returned.  Otherwise a 1 will be
// returned.
//
///////////////////////////////////////

int EbcdicCharToAsciiChar(int fromCcsid, int toCcsid, const char * input, char * output)

{
	int		tcount=0;
	char	defChar='.';
	wchar_t	ucsChar=0;

	// make sure that from and to code pages are provided
	if ((fromCcsid > 0) && (toCcsid > 0))
	{
		// start by trying to do a proper translation to UCS-2, using Windows
		tcount = MultiByteToWideChar(fromCcsid, 0, input, 1, &ucsChar, 1);

		// did the function succeed?
		if (tcount > 0)
		{
			// set the default character
			defChar = '.';

			// try to translate to the target code page
			tcount = WideCharToMultiByte(toCcsid, 0, &ucsChar, 1, output, 2, &defChar, NULL);
		}
	}

	// return the number of characters that were translated
	return tcount;
}

///////////////////////////////////////////////
//
// Routine to skip white space characters
//
///////////////////////////////////////////////

const char * skipWhiteSpace(const char *start, const char *end)

{
	// skip any white space
	while ((start < end) && (start[0] <= ' ') && (start[0] != 0))
	{
		// skip the white space character
		start++;
	}

	return start;
}

///////////////////////////////////////
//
// Routine to skip blanks in a string
//
///////////////////////////////////////

char * skipBlanks(const char *str)

{
   while ((*str <= ' ') && (*str != 0))
   {
      str++;
   } /* endwhile */

   return (char *)str;
}

///////////////////////////////////////
//
// Routine to find the next blank
// in a string
//
///////////////////////////////////////

char * findBlank(char *ptr)

{
	while (ptr[0] > ' ')
	{
		ptr++;
	}

	return ptr;
}

///////////////////////////////////////
//
// Routine to remove trailing blanks
// from a string
//
///////////////////////////////////////

void Rtrim(char *str)

{
	int	len;

	len = strlen(str);

	while ((len > 0) && (str[len - 1] <= ' '))
	{
		str[len - 1] = 0;
		len--;
	}
}

const char * skipQuotedString(const char *start, const char *eod)

{
	char	quotech;

	if (('\"' == start[0]) || ('\'' == start[0]))
	{
		// remember the type of quotes used
		quotech = start[0];

		// skip the initial delimiter
		start++;

		// find the trailing delimiter
		while ((start <= eod) && (start[0] != quotech))
		{
			start++;
		}

		// skip the trailing delimiter
		start++;
	}
	else
	{
		// not a quoted string, so find the next blank
		while ((start <= eod) && (start[0] > ' '))
		{
			start++;
		}
	}

	return start;
}

/**************************************************************/
/* Routine to reverse the bytes in a short variable           */
/**************************************************************/

short reverseBytes(short *var)

{
    return (((unsigned short) *var) >> 8) + ((((unsigned short) *var) & 255) << 8);
}

/**************************************************************/
/* Routine to reverse the bytes in an int variable            */
/**************************************************************/

int reverseBytes4(int var)

{
    return ((unsigned int) var >> 24) + (((unsigned int) var & 0x00ff0000) >> 8) +
           (((unsigned int) var & 0x0000ff00) << 8) + (((unsigned int) var & 255) <<24);
}

void reverseBytes24(unsigned char *in, unsigned char *out)

{
	int	u1;
	int	u2;
	int i;

	for (i = 0; i < 6; i++)
	{
		memcpy(&u1, in, 4);
		u2 = reverseBytes4(u1);
		memcpy(out, &u2, 4);
		in += 4;
		out += 4;
	}
}

void reverseBytes32(unsigned char *in, unsigned char *out)

{
	int	u1;
	int	u2;
	int	i;

	for (i = 0; i < 8; i++)
	{
		memcpy(&u1, in, 4);
		u2 = reverseBytes4(u1);
		memcpy(out, &u2, 4);
		in += 4;
		out += 4;
	}
}

//////////////////////////////////////////////////
//
// Routine to find a crlf sequence in the data
//
//////////////////////////////////////////////////

int findcrlf(const unsigned char *datain, const int maxchar)

{
	int count=maxchar;
	int i=0;

	while ((i < maxchar) && (datain[i] != '\r') && (datain[i] != '\n'))
	{
		i++;
	}

	if (i < maxchar)
	{
		count = i;
	}

	return count;
}

//////////////////////////////////////////////////
//
// Routine to find a crlf sequence in the data
//
//////////////////////////////////////////////////

int findcrlfW(const wchar_t *datain, const int maxchar)

{
	int count=maxchar;
	int i=0;

	while ((i < maxchar) && (datain[i] != L'\r') && (datain[i] != L'\n'))
	{
		i++;
	}

	if (i < maxchar)
	{
		count = i;
	}

	return count;
}

//////////////////////////////////////////////////
//
// Routine to round a data area to a multiple of 4
//
//////////////////////////////////////////////////

int roundLength(unsigned char *ptr)

{
	int		length;
	int		extra;

	length = strlen((char *)ptr);

	extra = length % 4;
	if (extra > 0)
	{
		extra = 4 - extra;
	}

	while (extra > 0)
	{
		ptr[length] = ' ';
		length++;
		ptr[length] = 0;
		extra--;
	}

	return length;
}

//////////////////////////////////////////////////
//
// Routine to round a UCS2 area to a multiple of 4
//
// returns the length of the string, which should
// be an even number of 16-bit characters
//
//////////////////////////////////////////////////

int roundLength2(unsigned char * ptr, const int encoding)

{
	int		length;

	length = wcslen((wchar_t *)ptr);

	if ((length % 2) > 0)
	{
		// check the encoding for little-endian
		if (NUMERIC_PC == encoding)
		{
			// little endian-space
			ptr[length * 2] = ' ';
			ptr[length * 2 + 1] = 0;
		}
		else
		{
			// big-endian space
			ptr[length * 2] = 0;
			ptr[length * 2 + 1] = ' ';
		}

		// create a new string terminator character
		ptr[length * 2 + 2] = 0;
		ptr[length * 2 + 3] = 0;

		// increment the length
		length++;
	}

	return length;
}

void formatDateTime(char *result, char *date, char *time)

{
	// format the date
	result[0] = date[0];
	result[1] = date[1];
	result[2] = date[2];
	result[3] = date[3];
	result[4] = '/';
	result[5] = date[4];
	result[6] = date[5];
	result[7] = '/';
	result[8] = date[6];
	result[9] = date[7];
	result[10] = ' ';
	result[11] = time[0];
	result[12] = time[1];
	result[13] = ':';
	result[14] = time[2];
	result[15] = time[3];
	result[16] = ':';
	result[17] = time[4];
	result[18] = time[5];
	result[19] = '.';
	result[20] = time[6];
	result[21] = time[7];
	result[22] = 0;
}

void setCharData(char *data, int size, LPCTSTR dataIn, int len)

{
	// initialize data to blanks
	memset(data, 32, size);

	// is there any data to copy?
	if ((len > 0) && (len <= size))
	{
		// copy the data
		memcpy(data, dataIn, len);
	}
}

void openDumpFile()

{
	// try to open the dump file for appending
	df = fopen(DUMP_FILE_NAME, "a+");
}

void closeDumpFile()

{
	// is the file open?
	if (df != NULL)
	{
		// file appears to be open
		// close the file
		fclose(df);

		// reinitialize the file handle
		df = NULL;
	}
}

void dumpStr(const char * str)

{
	fprintf(df, "%s\n", str);
}

void dumpData(char * data, int len)

{
	int		i;
	int		j;
	int		k;
	int		remaining;
	int		location=0;
	int		count;
	int		buffer=0;
	char	*offset;
	unsigned char	ch;
	char	*hexStr;
	char	asciiStr[32];
	char	tempLine[128];

	remaining = len;
	while (remaining > 0)
	{
		if (remaining > 16)
		{
			count = 16;
		}
		else
		{
			count = remaining;
		} // endif 

		// initialize the output strings and buffer offset 
		offset = data;
		i = 9;	// initialize variable offset 
		j = 0;	// initialize variable offset
		k = 0;	// initialize counter

		// add the offset to the beginning of the line
		sprintf(tempLine, "%8.8d", location);

		// initialize the next 19 bytes to blanks
		memset(tempLine + 8, ' ', 19);

		// calculate the next location
		location += 16;

		// point to the location for the hex part of the display
		hexStr = tempLine + 27;

		while (k < count)
		{
			// insert the next character into the output buffer
			// Make sure we have a character that can be displayed
			ch = (unsigned char) data[buffer];
			if ((ch > ' ') && (ch < 127))
			{
				// use the character, since it is printable
				tempLine[i++] = ch;
			}
			else
			{
				// replace with a period
				tempLine[i++] = '.';
			}

			// get the first part of the hex value
			ch >>= 4;
			hexStr[j++] = HEX_NUMBERS[ch];

			// get the second part of the hex value
			ch = (unsigned char) data[buffer];
			ch &= 0x0F;
			hexStr[j++] = HEX_NUMBERS[ch];

			// Go on to the next character 
			remaining--;
			buffer++;
			k++;   // increment counter 

			// Insert a blank for every fourth character 
			if ((k%4 == 0) && (k < count))
			{
				hexStr[j++] = ' ';
			} // endif 

			// Insert a blank for every eighth character in ASCII string 
			if (k%8 == 0)
			{
				asciiStr[i++] = ' ';
			} // endif 
		} // endwhile 

		// terminate the string
		hexStr[j] = 0;

		// write the line to the dump file
		fprintf(df, "%s\n", tempLine);
	}
}

/////////////////////////////////////////////////////////////////
//
// Routine to build a table that can be used to convert code
// points with no representation to periods.  This is used for
// character displays of single character display pages.  The
// windows locale is used to determine the current code page.
// The following code pages are recognized by this routine.
// Otherwise, code page 1252 is assumed.
//
/////////////////////////////////////////////////////////////////

void buildInvalidCharTable(char * invalidCharTable)

{
	int		i;
	int		cp=0;

	// build a table that maps all characters to the same character,
	// except the first 32 characters are mapped to periods
	for (i=0; i < 32; i++)
	{
		invalidCharTable[i] = '.';
	}

	// start by mapping all other characters to themselves
	for (i=32; i < 256; i++)
	{
		invalidCharTable[i] = i;
	}

	// never a valid display character
	invalidCharTable[127] = '.';	// 0x7F

	// see if we can get the current code page
	cp = GetACP();

	// set invalid code points to periods
	// this varies by code page
	switch (cp)
	{
	// US OEM 
	case 437:
	// Multi-lingual 
	case 850:
		{
			invalidCharTable[255] = '.';	// 0xFF
			break;
		}
	// Japanese 
	case 932:
		{
			invalidCharTable[128] = '.';	// 0x80
			invalidCharTable[160] = '.';	// 0xA0
			invalidCharTable[253] = '.';	// 0xFD
			invalidCharTable[254] = '.';	// 0xFE
			invalidCharTable[255] = '.';	// 0xFF
			break;
		}
	// Chinese (PRC, Singapore) 
	case 936:
		{
			invalidCharTable[255] = '.';	// 0xFF
			break;
		}
	// Korean 
	case 949:
		{
			invalidCharTable[128] = '.';	// 0x80
			invalidCharTable[255] = '.';	// 0xFF
			break;
		}
	// Chinese (traditional - Taiwan) 
	case 950:
		{
			invalidCharTable[128] = '.';	// 0x80
			invalidCharTable[255] = '.';	// 0xFF
			break;
		}
	// Latin II
	case 1250:
		{
			invalidCharTable[129] = '.';	// 0x81
			invalidCharTable[131] = '.';	// 0x83
			invalidCharTable[136] = '.';	// 0x88
			invalidCharTable[144] = '.';	// 0x90
			invalidCharTable[152] = '.';	// 0x98
			break;
		}
	// Cyrillic
	case 1251:
		{
			invalidCharTable[152] = '.';	// 0x98
			break;
		}
	// Latin I
	case 1252:
		{
			invalidCharTable[129] = '.';	// 0x81
			invalidCharTable[141] = '.';	// 0x8D
			invalidCharTable[143] = '.';	// 0x8F
			invalidCharTable[144] = '.';	// 0x90
			invalidCharTable[152] = '.';	// 0x98
			invalidCharTable[157] = '.';	// 0x9D
			break;
		}
	// Greek
	case 1253:
		{
			invalidCharTable[129] = '.';	// 0x81
			invalidCharTable[136] = '.';	// 0x88
			invalidCharTable[138] = '.';	// 0x8A
			invalidCharTable[138] = '.';	// 0x8C
			invalidCharTable[138] = '.';	// 0x8D
			invalidCharTable[138] = '.';	// 0x8E
			invalidCharTable[138] = '.';	// 0x8F
			invalidCharTable[144] = '.';	// 0x90
			invalidCharTable[152] = '.';	// 0x98
			invalidCharTable[154] = '.';	// 0x9A
			invalidCharTable[156] = '.';	// 0x9C
			invalidCharTable[157] = '.';	// 0x9D
			invalidCharTable[158] = '.';	// 0x9E
			invalidCharTable[159] = '.';	// 0x9F
			invalidCharTable[210] = '.';	// 0xD2
			invalidCharTable[255] = '.';	// 0xFF
			break;
		}
	// Turkish
	case 1254:
		{
			invalidCharTable[129] = '.';	// 0x81
			invalidCharTable[138] = '.';	// 0x8D
			invalidCharTable[138] = '.';	// 0x8E
			invalidCharTable[138] = '.';	// 0x8F
			invalidCharTable[144] = '.';	// 0x90
			invalidCharTable[152] = '.';	// 0x98
			invalidCharTable[154] = '.';	// 0x9A
			invalidCharTable[156] = '.';	// 0x9C
			invalidCharTable[157] = '.';	// 0x9D
			invalidCharTable[158] = '.';	// 0x9E
			break;
		}
	// Hebrew
	case 1255:
		{
			invalidCharTable[129] = '.';	// 0x81
			invalidCharTable[138] = '.';	// 0x8A
			invalidCharTable[138] = '.';	// 0x8C
			invalidCharTable[138] = '.';	// 0x8D
			invalidCharTable[138] = '.';	// 0x8E
			invalidCharTable[138] = '.';	// 0x8F
			invalidCharTable[144] = '.';	// 0x90
			invalidCharTable[154] = '.';	// 0x9A
			invalidCharTable[156] = '.';	// 0x9C
			invalidCharTable[157] = '.';	// 0x9D
			invalidCharTable[158] = '.';	// 0x9E
			invalidCharTable[159] = '.';	// 0x9F
			invalidCharTable[161] = '.';	// 0xA1
			invalidCharTable[170] = '.';	// 0xAA
			invalidCharTable[184] = '.';	// 0xB8
			invalidCharTable[186] = '.';	// 0xBA
			invalidCharTable[191] = '.';	// 0xBF
			invalidCharTable[215] = '.';	// 0xD7
			invalidCharTable[216] = '.';	// 0xD8
			invalidCharTable[217] = '.';	// 0xD9
			invalidCharTable[218] = '.';	// 0xDA
			invalidCharTable[219] = '.';	// 0xDB
			invalidCharTable[220] = '.';	// 0xDC
			invalidCharTable[221] = '.';	// 0xDD
			invalidCharTable[222] = '.';	// 0xDE
			invalidCharTable[223] = '.';	// 0xDF
			invalidCharTable[250] = '.';	// 0xFB
			invalidCharTable[251] = '.';	// 0xFC
			invalidCharTable[255] = '.';	// 0xFF
			break;
		}
	// Arabic
	case 1256:
		{
			invalidCharTable[138] = '.';	// 0x8A
			invalidCharTable[143] = '.';	// 0x8F
			invalidCharTable[144] = '.';	// 0x90
			invalidCharTable[152] = '.';	// 0x98
			invalidCharTable[159] = '.';	// 0x9F
			invalidCharTable[170] = '.';	// 0xAA
			invalidCharTable[192] = '.';	// 0xC0
			invalidCharTable[255] = '.';	// 0xFF
			break;
		}
	// Baltic
	case 1257:
		{
			invalidCharTable[129] = '.';	// 0x81
			invalidCharTable[131] = '.';	// 0x83
			invalidCharTable[136] = '.';	// 0x88
			invalidCharTable[138] = '.';	// 0x8A
			invalidCharTable[138] = '.';	// 0x8C
			invalidCharTable[138] = '.';	// 0x8D
			invalidCharTable[138] = '.';	// 0x8E
			invalidCharTable[138] = '.';	// 0x8F
			invalidCharTable[144] = '.';	// 0x90
			invalidCharTable[152] = '.';	// 0x98
			invalidCharTable[154] = '.';	// 0x9A
			invalidCharTable[156] = '.';	// 0x9C
			invalidCharTable[157] = '.';	// 0x9D
			invalidCharTable[158] = '.';	// 0x9E
			invalidCharTable[159] = '.';	// 0x9F
			invalidCharTable[161] = '.';	// 0xA1
			invalidCharTable[166] = '.';	// 0xA5
			invalidCharTable[180] = '.';	// 0xB4
			invalidCharTable[255] = '.';	// 0xFF
			break;
		}
	default:
		{
			// use 1252 as a default
			invalidCharTable[129] = '.';	// 0x81
			invalidCharTable[141] = '.';	// 0x8D
			invalidCharTable[143] = '.';	// 0x8F
			invalidCharTable[144] = '.';	// 0x90
			invalidCharTable[152] = '.';	// 0x98
			invalidCharTable[157] = '.';	// 0x9D
			break;
		}
	}
}

///////////////////////////////////////////////////////////
//
// Routine to translate EBCDIC data to ASCII for display.
//
// The routine will attempt to do the translation using 
// windows calls if possible.  If the proper code pages
// are not installed, then the translation will default
// to a built-in translation table.
//
///////////////////////////////////////////////////////////

void translateEbcdicData(unsigned char *out, const unsigned char *in, unsigned int len, int ebcdicCP)

{
	unsigned int	i;
	int				tcount=0;			// number of characters successfully translated
	int				asciiCcsid=0;		// ASCII code page to use for EBCDIC to ASCII translations
	wchar_t			ucsChar[4];			// wide character area to translate one character
	unsigned char	tempTxt[8];			// area to translate one character
	char			defChar='.';		// character to be used for characters that cannot be translated
	char			invalidCharTable[256];

	// figure out what code page this system is using
	// and figure out what the valid code points are
	buildInvalidCharTable(invalidCharTable);

	// figure out what code page to translate to
	// we only need to do this once
	asciiCcsid = matchEbcdicToAscii(ebcdicCP);

	i = 0;
	while (i < len)
	{
		if ((asciiCcsid > 0) && (ebcdicCP > 0))
		{
			// start by trying to do a proper translation, using Windows
			tcount = MultiByteToWideChar(ebcdicCP, 0, (char *)in + i, 1, ucsChar, 1);

			// did the function succeed
			if (tcount > 0)
			{
				tcount = WideCharToMultiByte(asciiCcsid, 0, ucsChar, 1, (char *)tempTxt, 2, &defChar, NULL);
			}
		}

		// did we wind up with a single character properly translated by Windows?
		if ((asciiCcsid > 0) && (ebcdicCP > 0) && (1 == tcount))
		{
			// Windows did the translation
			// make sure we are generating a displayable character
			out[i] = invalidCharTable[tempTxt[0]];
		}
		else
		{
			// use the default translate table
			out[i] = invalidCharTable[eatab[in[i]]];
		}

		// move on to the next character
		i++;
	}
}

///////////////////////////////////////////////////////////
//
// Routine to take in an EBCDIC code page and determine
//  which Windows ASCII code page to use for translations.
//
///////////////////////////////////////////////////////////

int matchEbcdicToAscii(const int ccsid)

{
	int		result=0;
	BOOL	isInstalled=FALSE;

	// check if the code page is installed on this Windows system
	isInstalled = isCodePageInstalled(ccsid);

	switch (ccsid)
	{
	// English code pages
	case 037:
	case 500:
	case 1047:
	case 1140:
	case 1146:
	case 1148:
	case 20285:
	case 20924:
	{
			result = 1252;
			break;
		}
	// Turkish code pages
	case 1026:
		{
			result = 1254;
			break;
		}
	// German code pages
	case 1141:
	case 20273:
		{
			// check if we can do a proper translation using windows
			// this requires the EBCDIC code pages to be installed
			// on the local Windows system
			if (isInstalled)
			{
				// use the proper German code page
				result = 1250;
			}
			else
			{
				// use a standard LATIN 1 code page
				result = 1252;
			}

			break;
		}
	// Denmark/Norway code pages
	case 1142:
	case 20277:
		{
			result = 1252;
			break;
		}
	// Finland/Sweden code pages
	case 1143:
	case 20278:
		{
			result = 1252;
			break;
		}
	// Italian code pages
	case 1144:
	case 20280:
		{
			result = 1252;
			break;
		}
	// Spanish code pages
	case 1145:
	case 20284:
		{
			result = 1252;
			break;
		}
	// French code pages
	case 1147:
	case 20297:
		{
			result = 1252;
			break;
		}
	// Icelandic code pages
	case 1149:
	case 20871:
		{
			result = 1252;
			break;
		}
	// Hebrew code pages
	case 20424:
		{
			result = 1255;
			break;
		}
	// Arabic code pages
	case 20420:
		{
			result = 1256;
			break;
		}
	// Cyrillic code pages
	case 20880:
	case 21025:
		{
			result = 1251;
			break;
		}
	}

	return result;
}

char * parseRFH1String(char *ptr, char *value, int maxSize)

{
	int		i;
	char	ch;

	ptr = skipBlanks(ptr);

	if ('\"' == ptr[0])
	{
		ch = '\"';
	}
	else
	{
		ch = ' ';
	}

	i = 0;
	while ((ptr[0] >= ' ') && (ptr[0] != ch) && (i < maxSize))
	{
		value[i] = ptr[0];
		i++;
		ptr++;
	}

	if ('\"' == ptr[0])
	{
		ptr++;
	}

	value[i] = 0;

	return ptr;
}

BOOL CALLBACK EnumCodePagesProc(LPTSTR lpCodePageString)

{
	// check if this is the code page we are looking for
	if (atoi(lpCodePageString) == ccsidInsCheck)
	{
		// remember we found the one we wanted
		ccsidInstalled = TRUE;
	}

	// continue if not found yet
	return !ccsidInstalled;
}

BOOL isCodePageInstalled(const int ccsid)

{
	// initialize two global variables
	ccsidInstalled = FALSE;
	ccsidInsCheck = ccsid;

	// invoke the function, which will call back to the EnumCodePagesProc routine above
	BOOL bRet = EnumSystemCodePages(&EnumCodePagesProc, CP_INSTALLED);

	// return the results
	return ccsidInstalled;
}

void processBackspace(CWnd *wnd)

{
 	int		nStart=0;
	int		nEnd=0;
	DWORD	style;
	CString value;

	// check if the window is enabled and not read only
	if (!wnd->IsWindowEnabled())
	{
		return;
	}

	// check if the edit box is read only
	style = wnd->GetStyle();
	if ((style & ES_READONLY) > 0)
	{
		return;
	}

	// process backspace key
	// start by getting the current value
	wnd->GetWindowText(value);

	// get the current location of the caret
	DWORD wSel = ((CEdit *)wnd)->GetSel();
	nStart = HIWORD(wSel);
	nEnd = LOWORD(wSel);

	// make sure there is something in the edit control to start with
	// and that the cursor is not at offset 0
	if ((value.GetLength() > 0) && (nEnd > 0))
	{
		// check if any characters are selected
		if (nStart == nEnd)
		{
			// nothing selected so select the character to the left of the cursor
			nStart = nEnd - 1;

			// check if it is a CR or LF character
			if ((value[nStart] != '\n') && (value[nStart] != '\r'))
			{
				// delete the character
				value.Delete(nStart, 1);
			}
			else
			{
				// delete the CR or LF character
				value.Delete(nStart, 1);

				// check for a second character in front of this one
				if ((nStart > 0) && (((value[nStart-1] == '\n') || (value[nStart-1] == '\r'))))
				{
					// delete the CR or LF character
					value.Delete(nStart-1, 1);
				}
			}
		}
		else
		{
			// just delete the selected characters
			value.Delete(nStart, nEnd - nStart);
		}

		// update the text in the edit control
		wnd->SetWindowText((LPCTSTR)value);

		// point the caret at the correct location
		((CEdit *)wnd)->SetSel(nStart, nStart, TRUE);
	}
}

//////////////////////////////////
//
// Routine to check if a code 
// page representation is two
// byte unicode.  This routine
// returns true if the code page
// is two byte unicode and false
// otherwise.
//
//////////////////////////////////

BOOL isUCS2(int ccsid)

{
	BOOL	result=FALSE;

	if ((1200 == ccsid) ||
		(1201 == ccsid) ||
		(13488 == ccsid) ||
		(17584 == ccsid))
	{
		result = TRUE;
	}

	return result;
}

/////////////////////////////////////////////////
//
// Routine to translate data from UCS-2 to 
// multibyte (CP 1208)
//
/////////////////////////////////////////////////

int UCS2ToMultiByte(unsigned char * ucsptr, int ucslen, unsigned char * mbsptr, int mbslen)

{
	int		charCount=0;

	charCount = wcstombs((char *)mbsptr, (const wchar_t *)ucsptr, mbslen);

	if (0 == charCount)
	{
		// just copy the data if we cannot translate it
		// we will not recognize it
		memcpy(mbsptr, ucsptr, ucslen);
	}

	return charCount;
}

/////////////////////////////////////////////////
//
// Routine to translate data from multibyte 
// CP 1208) to UCS-2.
//
// Note that the ucslen and mbslen parameters
// are specified in number of bytes.
//
/////////////////////////////////////////////////

int MultiByteToUCS2(unsigned char * ucsptr, int ucslen, unsigned char * mbsptr, int mbslen)

{
	int		charCount=0;

	charCount = mbstowcs((wchar_t *)ucsptr, (char *)mbsptr, ucslen/2);

	if (0 == charCount)
	{
		// just copy the data if we cannot translate it
		// we will not recognize it
		memcpy(ucsptr, mbsptr, mbslen);
	}

	return charCount;
}

///////////////////////////////////////
//
// convert 1208 to wide character
//
///////////////////////////////////////

int char1208toWCS(wchar_t * ucsptr, const unsigned char * mbsptr)

{
	int			numBytes=0;
	wchar_t		result;

	numBytes = mbsCharCount(mbsptr);

	switch (numBytes)
	{
	case 1:
		{
			result = mbsptr[0] & 0x3f;
			break;
		}
	case 2:
		{
			result = ((mbsptr[0] & 0x1f) << 6) | (mbsptr[1] & 0x3f);
			break;
		}
	case 3:
		{
			result = ((mbsptr[0] & 0x0f) << 12) | ((mbsptr[1] & 0x3f) << 6) || (mbsptr[2] & 0x3f);
			break;
		}
	default:
		{
			// return a null character
			result = 0;
			break;
		}
	}

	// set the UCS character based on the unicode character input
	ucsptr[0] = result;

	return numBytes;
}

///////////////////////////////////////
//
// routine to return the number of
// bytes in a multibyte character
//
///////////////////////////////////////

int mbsCharCount(const unsigned char * mbsptr)

{
	int		numBytes=0;

	if ((mbsptr[0] & 0x80) == 0)
	{
		// high order bit is zero - just return length of 1
		numBytes = 1;
	}
	else if ((mbsptr[0] &0x40) == 0)
	{
		// continuation byte
		numBytes = 1;
	}
	else if ((mbsptr[0] & 0x20) == 0)
	{
		// two byte sequence 0x110vvvvv 10vvvvvv
		numBytes = 2;
	}
	else if ((mbsptr[0] & 0x10) == 0)
	{
		// three byte sequence 0x1110vvvv 10vvvvvv 10vvvvvv
		numBytes = 3;
	}
	else if ((mbsptr[0] & 0x08) == 0)
	{
		// four byte sequence 0x11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
		numBytes = 4;
	}
	else if ((mbsptr[0] & 0x04) == 0)
	{
		// five byte sequence 0x111110vv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
		numBytes = 5;
	}
	else if ((mbsptr[0] & 0x02) == 0)
	{
		// six byte sequence 0x1111110v 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
		numBytes = 6;
	}
	else
	{
		// the limit is supposed to be 3 bytes - not clear what is going on
		numBytes = 1;
	}

	return numBytes;
}

////////////////////////////////////////
//
// Convert wide char to code page 1252
//
////////////////////////////////////////

int WCSto1252(const wchar_t * ucsptr, unsigned char * mbsptr)

{
	int					numChars=1;
	unsigned char *		ptr=(unsigned char *)ucsptr;

	// is this just an ASCII character?
	if ((ucsptr[0] < 128) || (ucsptr[0] >= 0x00A0))
	{
		// just copy the ASCII character
		mbsptr[0] = ptr[0];
	}
	else
	{
		// check for other characters that can be displayed as is
		if (0x20AC == ucsptr[0])		// EURO SIGN
		{
			mbsptr[0] = 0x80;
		}
		else if (0x201A == ucsptr[0])	// SINGLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x82;
		}
		else if (0x0192 == ucsptr[0])	// LATIN SMALL LETTER F WITH HOOK
		{
			mbsptr[0] = 0x83;
		}
		else if (0x201E == ucsptr[0])	// DOUBLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x84;
		}
		else if (0x2026 == ucsptr[0])	// HORIZONTAL ELLIPSIS
		{
			mbsptr[0] = 0x85;
		}
		else if (0x2020 == ucsptr[0])	// DAGGER
		{
			mbsptr[0] = 0x86;
		}
		else if (0x2021 == ucsptr[0])	// DOUBLE DAGGER
		{
			mbsptr[0] = 0x87;
		}
		else if (0x02C6 == ucsptr[0])	// MODIFIER LETTER CIRCUMFLEX ACCENT
		{
			mbsptr[0] = 0x88;
		}
		else if (0x2030 == ucsptr[0])	// PER MILLE SIGN
		{
			mbsptr[0] = 0x89;
		}
		else if (0x0160 == ucsptr[0])	// LATIN CAPITAL LETTER S WITH CARON
		{
			mbsptr[0] = 0x8A;
		}
		else if (0x2039 == ucsptr[0])	// SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x8B;
		}
		else if (0x0152 == ucsptr[0])	// LATIN CAPITAL LIGATURE OE
		{
			mbsptr[0] = 0x8C;
		}
		else if (0x017D == ucsptr[0])	// LATIN CAPITAL LETTER Z WITH CARON
		{
			mbsptr[0] = 0x8E;
		}
		else if (0x2018 == ucsptr[0])	// LEFT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x91;
		}
		else if (0x2019 == ucsptr[0])	// RIGHT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x92;
		}
		else if (0x201C == ucsptr[0])	// LEFT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x93;
		}
		else if (0x201D == ucsptr[0])	// RIGHT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x94;
		}
		else if (0x2022 == ucsptr[0])	// BULLET
		{
			mbsptr[0] = 0x95;
		}
		else if (0x2013 == ucsptr[0])	// EN DASH
		{
			mbsptr[0] = 0x96;
		}
		else if (0x2014 == ucsptr[0])	// EM DASH
		{
			mbsptr[0] = 0x97;
		}
		else if (0x02DC == ucsptr[0])	// SMALL TILDE
		{
			mbsptr[0] = 0x98;
		}
		else if (0x2122 == ucsptr[0])	// TRADE MARK SIGN
		{
			mbsptr[0] = 0x99;
		}
		else if (0x0161 == ucsptr[0])	// LATIN SMALL LETTER S WITH CARON
		{
			mbsptr[0] = 0x9A;
		}
		else if (0x203A == ucsptr[0])	// SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x9B;
		}
		else if (0x0153 == ucsptr[0])	// LATIN SMALL LIGATURE OE
		{
			mbsptr[0] = 0x9C;
		}
		else if (0x017E == ucsptr[0])	// LATIN SMALL LETTER Z WITH CARON
		{
			mbsptr[0] = 0x9E;
		}
		else if (0x0178 == ucsptr[0])	// LATIN CAPITAL LETTER Y WITH DIAERESIS
		{
			mbsptr[0] = 0x9F;
		}
		else
		{
			// undefined character - substitute a period
			mbsptr[0] = '.';
		}
	}

	return numChars;
}

////////////////////////////////////////
//
// Convert wide char to code page 1250
//
////////////////////////////////////////

int WCSto1250(const wchar_t * ucsptr, unsigned char * mbsptr)

{
	int					numChars=1;
	unsigned char *		ptr=(unsigned char *)ucsptr;

	// initialize to an undefined character (period)
	mbsptr[0] = '.';

	// is this just an ASCII character?
	if (ucsptr[0] < 128)
	{
		// just copy the ASCII character
		mbsptr[0] = ptr[0];
	}
	else
	{
		// check for other characters that can be displayed as is
		if (0x20AC == ucsptr[0])		// EURO SIGN
		{
			mbsptr[0] = 0x80;
		}
		else if (0x201A == ucsptr[0])	// SINGLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x82;
		}
		else if (0x201E == ucsptr[0])	// DOUBLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x84;
		}
		else if (0x2026 == ucsptr[0])	// HORIZONTAL ELLIPSIS
		{
			mbsptr[0] = 0x85;
		}
		else if (0x2020 == ucsptr[0])	// DAGGER
		{
			mbsptr[0] = 0x86;
		}
		else if (0x2021 == ucsptr[0])	// DOUBLE DAGGER
		{
			mbsptr[0] = 0x87;
		}
		else if (0x2030 == ucsptr[0])	// PER MILLE SIGN
		{
			mbsptr[0] = 0x89;
		}
		else if (0x0160 == ucsptr[0])	// LATIN CAPITAL LETTER S WITH CARON
		{
			mbsptr[0] = 0x8A;
		}
		else if (0x2039 == ucsptr[0])	// SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x8B;
		}
		else if (0x015A == ucsptr[0])	// LATIN CAPITAL LETTER S WITH ACUTE
		{
			mbsptr[0] = 0x8C;
		}
		else if (0x0164 == ucsptr[0])	// LATIN CAPITAL LETTER T WITH CARON
		{
			mbsptr[0] = 0x8D;
		}
		else if (0x017D == ucsptr[0])	// LATIN CAPITAL LETTER Z WITH CARON
		{
			mbsptr[0] = 0x8E;
		}
		else if (0x0179 == ucsptr[0])	// LATIN CAPITAL LETTER Z WITH ACUTE
		{
			mbsptr[0] = 0x8F;
		}
		else if (0x2018 == ucsptr[0])	// LEFT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x91;
		}
		else if (0x2019 == ucsptr[0])	// RIGHT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x92;
		}
		else if (0x201C == ucsptr[0])	// LEFT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x93;
		}
		else if (0x201D == ucsptr[0])	// RIGHT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x94;
		}
		else if (0x2022 == ucsptr[0])	// BULLET
		{
			mbsptr[0] = 0x95;
		}
		else if (0x2013 == ucsptr[0])	// EN DASH
		{
			mbsptr[0] = 0x96;
		}
		else if (0x2014 == ucsptr[0])	// EM DASH
		{
			mbsptr[0] = 0x97;
		}
		else if (0x2122 == ucsptr[0])	// TRADE MARK SIGN
		{
			mbsptr[0] = 0x99;
		}
		else if (0x0161 == ucsptr[0])	// LATIN SMALL LETTER S WITH CARON
		{
			mbsptr[0] = 0x9A;
		}
		else if (0x203A == ucsptr[0])	// SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x9B;
		}
		else if (0x015B == ucsptr[0])	// LATIN SMALL LETTER S WITH ACUTE
		{
			mbsptr[0] = 0x9C;
		}
		else if (0x0165 == ucsptr[0])	// LATIN SMALL LETTER T WITH CARON
		{
			mbsptr[0] = 0x9D;
		}
		else if (0x017E == ucsptr[0])	// LATIN SMALL LETTER Z WITH CARON
		{
			mbsptr[0] = 0x9E;
		}
		else if (0x017A == ucsptr[0])	// LATIN SMALL LETTER Z WITH ACUTE
		{
			mbsptr[0] = 0x9F;
		}
		else if (0x00A0 == ucsptr[0])	// NO-BREAK SPACE
		{
			mbsptr[0] = 0xA0;
		}
		else if (0x02C7 == ucsptr[0])	// CARON
		{
			mbsptr[0] = 0xA1;
		}
		else if (0x02D8 == ucsptr[0])	// BREVE
		{
			mbsptr[0] = 0xA2;
		}
		else if (0x0141 == ucsptr[0])	// LATIN CAPITAL LETTER L WITH STROKE
		{
			mbsptr[0] = 0xA3;
		}
		else if (0x00A4 == ucsptr[0])	// CURRENCY SIGN
		{
			mbsptr[0] = 0xA4;
		}
		else if (0x0104 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH OGONEK
		{
			mbsptr[0] = 0xA5;
		}
		else if (0x00A6 == ucsptr[0])	// BROKEN BAR
		{
			mbsptr[0] = 0xA6;
		}
		else if (0x00A7 == ucsptr[0])	// SECTION SIGN
		{
			mbsptr[0] = 0xA7;
		}
		else if (0x00A8 == ucsptr[0])	// DIAERESIS
		{
			mbsptr[0] = 0xA8;
		}
		else if (0x00A9 == ucsptr[0])	// COPYRIGHT SIGN
		{
			mbsptr[0] = 0xA9;
		}
		else if (0x015E == ucsptr[0])	// LATIN CAPITAL LETTER S WITH CEDILLA
		{
			mbsptr[0] = 0xAA;
		}
		else if (0x00AB == ucsptr[0])	// LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xAB;
		}
		else if (0x00AC == ucsptr[0])	// NOT SIGN
		{
			mbsptr[0] = 0xAC;
		}
		else if (0x00AD == ucsptr[0])	// SOFT HYPHEN
		{
			mbsptr[0] = 0xAD;
		}
		else if (0x00AE == ucsptr[0])	// REGISTERED SIGN
		{
			mbsptr[0] = 0xAE;
		}
		else if (0x017B == ucsptr[0])	// LATIN CAPITAL LETTER Z WITH DOT ABOVE
		{
			mbsptr[0] = 0xAF;
		}
		else if (0x00B0 == ucsptr[0])	// DEGREE SIGN
		{
			mbsptr[0] = 0xB0;
		}
		else if (0x00B1 == ucsptr[0])	// PLUS-MINUS SIGN
		{
			mbsptr[0] = 0xB1;
		}
		else if (0x02DB == ucsptr[0])	// OGONEK
		{
			mbsptr[0] = 0xB2;
		}
		else if (0x0142 == ucsptr[0])	// LATIN SMALL LETTER L WITH STROKE
		{
			mbsptr[0] = 0xB3;
		}
		else if (0x00B4 == ucsptr[0])	// ACUTE ACCENT
		{
			mbsptr[0] = 0xB4;
		}
		else if (0x00B5 == ucsptr[0])	// MICRO SIGN
		{
			mbsptr[0] = 0xB5;
		}
		else if (0x00B6 == ucsptr[0])	// PILCROW SIGN
		{
			mbsptr[0] = 0xB6;
		}
		else if (0x00B7 == ucsptr[0])	// MIDDLE DOT
		{
			mbsptr[0] = 0xB7;
		}
		else if (0x00B8 == ucsptr[0])	// CEDILLA
		{
			mbsptr[0] = 0xB8;
		}
		else if (0x0105 == ucsptr[0])	// LATIN SMALL LETTER A WITH OGONEK
		{
			mbsptr[0] = 0xB9;
		}
		else if (0x015F == ucsptr[0])	// LATIN SMALL LETTER S WITH CEDILLA
		{
			mbsptr[0] = 0xBA;
		}
		else if (0x00BB == ucsptr[0])	// RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xBB;
		}
		else if (0x013D == ucsptr[0])	// LATIN CAPITAL LETTER L WITH CARON
		{
			mbsptr[0] = 0xBC;
		}
		else if (0x02DD == ucsptr[0])	// DOUBLE ACUTE ACCENT
		{
			mbsptr[0] = 0xBD;
		}
		else if (0x013E == ucsptr[0])	// LATIN SMALL LETTER L WITH CARON
		{
			mbsptr[0] = 0xBE;
		}
		else if (0x017C == ucsptr[0])	// LATIN SMALL LETTER Z WITH DOT ABOVE
		{
			mbsptr[0] = 0xBF;
		}
		else if (0x0154 == ucsptr[0])	// LATIN CAPITAL LETTER R WITH ACUTE
		{
			mbsptr[0] = 0xC0;
		}
		else if (0x00C1 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH ACUTE
		{
			mbsptr[0] = 0xC1;
		}
		else if (0x00C2 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xC2;
		}
		else if (0x0102 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH BREVE
		{
			mbsptr[0] = 0xC3;
		}
		else if (0x00C4 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH DIAERESIS
		{
			mbsptr[0] = 0xC4;
		}
		else if (0x0139 == ucsptr[0])	// LATIN CAPITAL LETTER L WITH ACUTE
		{
			mbsptr[0] = 0xC5;
		}
		else if (0x0106 == ucsptr[0])	// LATIN CAPITAL LETTER C WITH ACUTE
		{
			mbsptr[0] = 0xC6;
		}
		else if (0x00C7 == ucsptr[0])	// LATIN CAPITAL LETTER C WITH CEDILLA
		{
			mbsptr[0] = 0xC7;
		}
		else if (0x010C == ucsptr[0])	// LATIN CAPITAL LETTER C WITH CARON
		{
			mbsptr[0] = 0xC8;
		}
		else if (0x00C9 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH ACUTE
		{
			mbsptr[0] = 0xC9;
		}
		else if (0x0118 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH OGONEK
		{
			mbsptr[0] = 0xCA;
		}
		else if (0x00CB == ucsptr[0])	// LATIN CAPITAL LETTER E WITH DIAERESIS
		{
			mbsptr[0] = 0xCB;
		}
		else if (0x011A == ucsptr[0])	// LATIN CAPITAL LETTER E WITH CARON
		{
			mbsptr[0] = 0xCC;
		}
		else if (0x00CD == ucsptr[0])	// LATIN CAPITAL LETTER I WITH ACUTE
		{
			mbsptr[0] = 0xCD;
		}
		else if (0x00CE == ucsptr[0])	// LATIN CAPITAL LETTER I WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xCE;
		}
		else if (0x010E == ucsptr[0])	// LATIN CAPITAL LETTER D WITH CARON
		{
			mbsptr[0] = 0xCF;
		}
		else if (0x0110 == ucsptr[0])	// LATIN CAPITAL LETTER D WITH STROKE
		{
			mbsptr[0] = 0xD0;
		}
		else if (0x0143 == ucsptr[0])	// LATIN CAPITAL LETTER N WITH ACUTE
		{
			mbsptr[0] = 0xD1;
		}
		else if (0x0147 == ucsptr[0])	// LATIN CAPITAL LETTER N WITH CARON
		{
			mbsptr[0] = 0xD2;
		}
		else if (0x00D3 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH ACUTE
		{
			mbsptr[0] = 0xD3;
		}
		else if (0x00D4 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xD4;
		}
		else if (0x0150 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
		{
			mbsptr[0] = 0xD5;
		}
		else if (0x00D6 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH DIAERESIS
		{
			mbsptr[0] = 0xD6;
		}
		else if (0x00D7 == ucsptr[0])	// MULTIPLICATION SIGN
		{
			mbsptr[0] = 0xD7;
		}
		else if (0x0158 == ucsptr[0])	// LATIN CAPITAL LETTER R WITH CARON
		{
			mbsptr[0] = 0xD8;
		}
		else if (0x016E == ucsptr[0])	// LATIN CAPITAL LETTER U WITH RING ABOVE
		{
			mbsptr[0] = 0xD9;
		}
		else if (0x00DA == ucsptr[0])	// LATIN CAPITAL LETTER U WITH ACUTE
		{
			mbsptr[0] = 0xDA;
		}
		else if (0x0170 == ucsptr[0])	// LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
		{
			mbsptr[0] = 0xDB;
		}
		else if (0x00DC == ucsptr[0])	// LATIN CAPITAL LETTER U WITH DIAERESIS
		{
			mbsptr[0] = 0xDC;
		}
		else if (0x00DD == ucsptr[0])	// LATIN CAPITAL LETTER Y WITH ACUTE
		{
			mbsptr[0] = 0xDD;
		}
		else if (0x0162 == ucsptr[0])	// LATIN CAPITAL LETTER T WITH CEDILLA
		{
			mbsptr[0] = 0xDE;
		}
		else if (0x00DF == ucsptr[0])	// LATIN SMALL LETTER SHARP S
		{
			mbsptr[0] = 0xDF;
		}
		else if (0x0155 == ucsptr[0])	// LATIN SMALL LETTER R WITH ACUTE
		{
			mbsptr[0] = 0xE0;
		}
		else if (0x00E1 == ucsptr[0])	// LATIN SMALL LETTER A WITH ACUTE
		{
			mbsptr[0] = 0xE1;
		}
		else if (0x00E2 == ucsptr[0])	// LATIN SMALL LETTER A WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xE2;
		}
		else if (0x0103 == ucsptr[0])	// LATIN SMALL LETTER A WITH BREVE
		{
			mbsptr[0] = 0xE3;
		}
		else if (0x00E4 == ucsptr[0])	// LATIN SMALL LETTER A WITH DIAERESIS
		{
			mbsptr[0] = 0xE4;
		}
		else if (0x013A == ucsptr[0])	// LATIN SMALL LETTER L WITH ACUTE
		{
			mbsptr[0] = 0xE5;
		}
		else if (0x0107 == ucsptr[0])	// LATIN SMALL LETTER C WITH ACUTE
		{
			mbsptr[0] = 0xE6;
		}
		else if (0x00E7 == ucsptr[0])	// LATIN SMALL LETTER C WITH CEDILLA
		{
			mbsptr[0] = 0xE7;
		}
		else if (0x010D == ucsptr[0])	// LATIN SMALL LETTER C WITH CARON
		{
			mbsptr[0] = 0xE8;
		}
		else if (0x00E9 == ucsptr[0])	// LATIN SMALL LETTER E WITH ACUTE
		{
			mbsptr[0] = 0xE9;
		}
		else if (0x0119 == ucsptr[0])	// LATIN SMALL LETTER E WITH OGONEK
		{
			mbsptr[0] = 0xEA;
		}
		else if (0x00EB == ucsptr[0])	// LATIN SMALL LETTER E WITH DIAERESIS
		{
			mbsptr[0] = 0xEB;
		}
		else if (0x011B == ucsptr[0])	// LATIN SMALL LETTER E WITH CARON
		{
			mbsptr[0] = 0xEC;
		}
		else if (0x00ED == ucsptr[0])	// LATIN SMALL LETTER I WITH ACUTE
		{
			mbsptr[0] = 0xED;
		}
		else if (0x00EE == ucsptr[0])	// LATIN SMALL LETTER I WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xEE;
		}
		else if (0x010F == ucsptr[0])	// LATIN SMALL LETTER D WITH CARON
		{
			mbsptr[0] = 0xEF;
		}
		else if (0x0111 == ucsptr[0])	// LATIN SMALL LETTER D WITH STROKE
		{
			mbsptr[0] = 0xF0;
		}
		else if (0x0144 == ucsptr[0])	// LATIN SMALL LETTER N WITH ACUTE
		{
			mbsptr[0] = 0xF1;
		}
		else if (0x0148 == ucsptr[0])	// LATIN SMALL LETTER N WITH CARON
		{
			mbsptr[0] = 0xF2;
		}
		else if (0x00F3 == ucsptr[0])	// LATIN SMALL LETTER O WITH ACUTE
		{
			mbsptr[0] = 0xF3;
		}
		else if (0x00F4 == ucsptr[0])	// LATIN SMALL LETTER O WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xF4;
		}
		else if (0x0151 == ucsptr[0])	// LATIN SMALL LETTER O WITH DOUBLE ACUTE
		{
			mbsptr[0] = 0xF5;
		}
		else if (0x00F6 == ucsptr[0])	// LATIN SMALL LETTER O WITH DIAERESIS
		{
			mbsptr[0] = 0xF6;
		}
		else if (0x00F7 == ucsptr[0])	// DIVISION SIGN
		{
			mbsptr[0] = 0xF7;
		}
		else if (0x0159 == ucsptr[0])	// LATIN SMALL LETTER R WITH CARON
		{
			mbsptr[0] = 0xF8;
		}
		else if (0x016F == ucsptr[0])	// LATIN SMALL LETTER U WITH RING ABOVE
		{
			mbsptr[0] = 0xF9;
		}
		else if (0x00FA == ucsptr[0])	// LATIN SMALL LETTER U WITH ACUTE
		{
			mbsptr[0] = 0xFA;
		}
		else if (0x0171 == ucsptr[0])	// LATIN SMALL LETTER U WITH DOUBLE ACUTE
		{
			mbsptr[0] = 0xFB;
		}
		else if (0x00FC == ucsptr[0])	// LATIN SMALL LETTER U WITH DIAERESIS
		{
			mbsptr[0] = 0xFC;
		}
		else if (0x00FD == ucsptr[0])	// LATIN SMALL LETTER Y WITH ACUTE
		{
			mbsptr[0] = 0xFD;
		}

		if (0x0163 == ucsptr[0])	// LATIN SMALL LETTER T WITH CEDILLA
		{
			mbsptr[0] = 0xFE;
		}
		else if (0x02D9 == ucsptr[0])	// DOT ABOVE
		{
			mbsptr[0] = 0xFF;
		}
	}

	return numChars;
}


////////////////////////////////////////
//
// Convert wide char to code page 1251
//
////////////////////////////////////////

int WCSto1251(const wchar_t * ucsptr, unsigned char * mbsptr)

{
	int					numChars=1;
	unsigned char *		ptr=(unsigned char *)ucsptr;

	// initialize to an undefined character (period)
	mbsptr[0] = '.';

	// is this just an ASCII character?
	if (ucsptr[0] < 128)
	{
		// just copy the ASCII character
		mbsptr[0] = ptr[0];
	}
	else
	{
		// check for other characters that can be displayed as is
		if (0x0402 == ucsptr[0])		// CYRILLIC CAPITAL LETTER DJE
		{
			mbsptr[0] = 0x80;
		}
		else if (0x0403 == ucsptr[0])	// CYRILLIC CAPITAL LETTER GJE
		{
			mbsptr[0] = 0x81;
		}
		else if (0x201A == ucsptr[0])	// SINGLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x82;
		}
		else if (0x0453 == ucsptr[0])	// CYRILLIC SMALL LETTER GJE
		{
			mbsptr[0] = 0x83;
		}
		else if (0x201E == ucsptr[0])	// DOUBLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x84;
		}
		else if (0x2026 == ucsptr[0])	// HORIZONTAL ELLIPSIS
		{
			mbsptr[0] = 0x85;
		}
		else if (0x2020 == ucsptr[0])	// DAGGER
		{
			mbsptr[0] = 0x86;
		}
		else if (0x2021 == ucsptr[0])	// DOUBLE DAGGER
		{
			mbsptr[0] = 0x87;
		}
		else if (0x20AC == ucsptr[0])	// EURO SIGN
		{
			mbsptr[0] = 0x88;
		}
		else if (0x2030 == ucsptr[0])	// PER MILLE SIGN
		{
			mbsptr[0] = 0x89;
		}
		else if (0x0409 == ucsptr[0])	// CYRILLIC CAPITAL LETTER LJE
		{
			mbsptr[0] = 0x8A;
		}
		else if (0x2039 == ucsptr[0])	// SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x8B;
		}
		else if (0x040A == ucsptr[0])	// CYRILLIC CAPITAL LETTER NJE
		{
			mbsptr[0] = 0x8C;
		}
		else if (0x040C == ucsptr[0])	// CYRILLIC CAPITAL LETTER KJE
		{
			mbsptr[0] = 0x8D;
		}
		else if (0x040B == ucsptr[0])	// CYRILLIC CAPITAL LETTER TSHE
		{
			mbsptr[0] = 0x8E;
		}
		else if (0x040F == ucsptr[0])	// CYRILLIC CAPITAL LETTER DZHE
		{
			mbsptr[0] = 0x8F;
		}
		else if (0x0452 == ucsptr[0])	// CYRILLIC SMALL LETTER DJE
		{
			mbsptr[0] = 0x90;
		}
		else if (0x2018 == ucsptr[0])	// LEFT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x91;
		}
		else if (0x2019 == ucsptr[0])	// RIGHT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x92;
		}
		else if (0x201C == ucsptr[0])	// LEFT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x93;
		}
		else if (0x201D == ucsptr[0])	// RIGHT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x94;
		}
		else if (0x2022 == ucsptr[0])	// BULLET
		{
			mbsptr[0] = 0x95;
		}
		else if (0x2013 == ucsptr[0])	// EN DASH
		{
			mbsptr[0] = 0x96;
		}
		else if (0x2014 == ucsptr[0])	// EM DASH
		{
			mbsptr[0] = 0x97;
		}
		else if (0x2122 == ucsptr[0])	// TRADE MARK SIGN
		{
			mbsptr[0] = 0x99;
		}
		else if (0x0459 == ucsptr[0])	// CYRILLIC SMALL LETTER LJE
		{
			mbsptr[0] = 0x9A;
		}
		else if (0x203A == ucsptr[0])	// SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x9B;
		}
		else if (0x045A == ucsptr[0])	// CYRILLIC SMALL LETTER NJE
		{
			mbsptr[0] = 0x9C;
		}
		else if (0x045C == ucsptr[0])	// CYRILLIC SMALL LETTER KJE
		{
			mbsptr[0] = 0x9D;
		}
		else if (0x045B == ucsptr[0])	// CYRILLIC SMALL LETTER TSHE
		{
			mbsptr[0] = 0x9E;
		}
		else if (0x045F == ucsptr[0])	// CYRILLIC SMALL LETTER DZHE
		{
			mbsptr[0] = 0x9F;
		}
		else if (0x00A0 == ucsptr[0])	// NO-BREAK SPACE
		{
			mbsptr[0] = 0xA0;
		}
		else if (0x040E == ucsptr[0])	// CYRILLIC CAPITAL LETTER SHORT U
		{
			mbsptr[0] = 0xA1;
		}
		else if (0x045E == ucsptr[0])	// CYRILLIC SMALL LETTER SHORT U
		{
			mbsptr[0] = 0xA2;
		}
		else if (0x0408 == ucsptr[0])	// CYRILLIC CAPITAL LETTER JE
		{
			mbsptr[0] = 0xA3;
		}
		else if (0x00A4 == ucsptr[0])	// CURRENCY SIGN
		{
			mbsptr[0] = 0xA4;
		}
		else if (0x0490 == ucsptr[0])	// CYRILLIC CAPITAL LETTER GHE WITH UPTURN
		{
			mbsptr[0] = 0xA5;
		}
		else if (0x00A6 == ucsptr[0])	// BROKEN BAR
		{
			mbsptr[0] = 0xA6;
		}
		else if (0x00A7 == ucsptr[0])	// SECTION SIGN
		{
			mbsptr[0] = 0xA7;
		}
		else if (0x0401 == ucsptr[0])	// CYRILLIC CAPITAL LETTER IO
		{
			mbsptr[0] = 0xA8;
		}
		else if (0x00A9 == ucsptr[0])	// COPYRIGHT SIGN
		{
			mbsptr[0] = 0xA9;
		}
		else if (0x0404 == ucsptr[0])	// CYRILLIC CAPITAL LETTER UKRAINIAN IE
		{
			mbsptr[0] = 0xAA;
		}
		else if (0x00AB == ucsptr[0])	// LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xAB;
		}
		else if (0x00AC == ucsptr[0])	// NOT SIGN
		{
			mbsptr[0] = 0xAC;
		}
		else if (0x00AD == ucsptr[0])	// SOFT HYPHEN
		{
			mbsptr[0] = 0xAD;
		}
		else if (0x00AE == ucsptr[0])	// REGISTERED SIGN
		{
			mbsptr[0] = 0xAE;
		}
		else if (0x0407 == ucsptr[0])	// CYRILLIC CAPITAL LETTER YI
		{
			mbsptr[0] = 0xAF;
		}
		else if (0x00B0 == ucsptr[0])	// DEGREE SIGN
		{
			mbsptr[0] = 0xB0;
		}
		else if (0x00B1 == ucsptr[0])	// PLUS-MINUS SIGN
		{
			mbsptr[0] = 0xB1;
		}
		else if (0x0406 == ucsptr[0])	// CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
		{
			mbsptr[0] = 0xB2;
		}
		else if (0x0456 == ucsptr[0])	// CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
		{
			mbsptr[0] = 0xB3;
		}
		else if (0x0491 == ucsptr[0])	// CYRILLIC SMALL LETTER GHE WITH UPTURN
		{
			mbsptr[0] = 0xB4;
		}
		else if (0x00B5 == ucsptr[0])	// MICRO SIGN
		{
			mbsptr[0] = 0xB5;
		}
		else if (0x00B6 == ucsptr[0])	// PILCROW SIGN
		{
			mbsptr[0] = 0xB6;
		}
		else if (0x00B7 == ucsptr[0])	// MIDDLE DOT
		{
			mbsptr[0] = 0xB7;
		}
		else if (0x0451 == ucsptr[0])	// CYRILLIC SMALL LETTER IO
		{
			mbsptr[0] = 0xB8;
		}
		else if (0x2116 == ucsptr[0])	// NUMERO SIGN
		{
			mbsptr[0] = 0xB9;
		}
		else if (0x0454 == ucsptr[0])	// CYRILLIC SMALL LETTER UKRAINIAN IE
		{
			mbsptr[0] = 0xBA;
		}
		else if (0x00BB == ucsptr[0])	// RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xBB;
		}
		else if (0x0458 == ucsptr[0])	// CYRILLIC SMALL LETTER JE
		{
			mbsptr[0] = 0xBC;
		}
		else if (0x0405 == ucsptr[0])	// CYRILLIC CAPITAL LETTER DZE
		{
			mbsptr[0] = 0xBD;
		}
		else if (0x0455 == ucsptr[0])	// CYRILLIC SMALL LETTER DZE
		{
			mbsptr[0] = 0xBE;
		}
		else if (0x0457 == ucsptr[0])	// CYRILLIC SMALL LETTER YI
		{
			mbsptr[0] = 0xBF;
		}
		else if (0x0410 == ucsptr[0])	// CYRILLIC CAPITAL LETTER A
		{
			mbsptr[0] = 0xC0;
		}
		else if (0x0411 == ucsptr[0])	// CYRILLIC CAPITAL LETTER BE
		{
			mbsptr[0] = 0xC1;
		}
		else if (0x0412 == ucsptr[0])	// CYRILLIC CAPITAL LETTER VE
		{
			mbsptr[0] = 0xC2;
		}
		else if (0x0413 == ucsptr[0])	// CYRILLIC CAPITAL LETTER GHE
		{
			mbsptr[0] = 0xC3;
		}
		else if (0x0414 == ucsptr[0])	// CYRILLIC CAPITAL LETTER DE
		{
			mbsptr[0] = 0xC4;
		}
		else if (0x0415 == ucsptr[0])	// CYRILLIC CAPITAL LETTER IE
		{
			mbsptr[0] = 0xC5;
		}
		else if (0x0416 == ucsptr[0])	// CYRILLIC CAPITAL LETTER ZHE
		{
			mbsptr[0] = 0xC6;
		}
		else if (0x0417 == ucsptr[0])	// CYRILLIC CAPITAL LETTER ZE
		{
			mbsptr[0] = 0xC7;
		}
		else if (0x0418 == ucsptr[0])	// CYRILLIC CAPITAL LETTER I
		{
			mbsptr[0] = 0xC8;
		}
		else if (0x0419 == ucsptr[0])	// CYRILLIC CAPITAL LETTER SHORT I
		{
			mbsptr[0] = 0xC9;
		}
		else if (0x041A == ucsptr[0])	// CYRILLIC CAPITAL LETTER KA
		{
			mbsptr[0] = 0xCA;
		}
		else if (0x041B == ucsptr[0])	// CYRILLIC CAPITAL LETTER EL
		{
			mbsptr[0] = 0xCB;
		}
		else if (0x041C == ucsptr[0])	// CYRILLIC CAPITAL LETTER EM
		{
			mbsptr[0] = 0xCC;
		}
		else if (0x041D == ucsptr[0])	// CYRILLIC CAPITAL LETTER EN
		{
			mbsptr[0] = 0xCD;
		}
		else if (0x041E == ucsptr[0])	// CYRILLIC CAPITAL LETTER O
		{
			mbsptr[0] = 0xCE;
		}
		else if (0x041F == ucsptr[0])	// CYRILLIC CAPITAL LETTER PE
		{
			mbsptr[0] = 0xCF;
		}
		else if (0x0420 == ucsptr[0])	// CYRILLIC CAPITAL LETTER ER
		{
			mbsptr[0] = 0xD0;
		}
		else if (0x0421 == ucsptr[0])	// CYRILLIC CAPITAL LETTER ES
		{
			mbsptr[0] = 0xD1;
		}
		else if (0x0422 == ucsptr[0])	// CYRILLIC CAPITAL LETTER TE
		{
			mbsptr[0] = 0xD2;
		}
		else if (0x0423 == ucsptr[0])	// CYRILLIC CAPITAL LETTER U
		{
			mbsptr[0] = 0xD3;
		}
		else if (0x0424 == ucsptr[0])	// CYRILLIC CAPITAL LETTER EF
		{
			mbsptr[0] = 0xD4;
		}
		else if (0x0425 == ucsptr[0])	// CYRILLIC CAPITAL LETTER HA
		{
			mbsptr[0] = 0xD5;
		}
		else if (0x0426 == ucsptr[0])	// CYRILLIC CAPITAL LETTER TSE
		{
			mbsptr[0] = 0xD6;
		}
		else if (0x0427 == ucsptr[0])	// CYRILLIC CAPITAL LETTER CHE
		{
			mbsptr[0] = 0xD7;
		}
		else if (0x0428 == ucsptr[0])	// CYRILLIC CAPITAL LETTER SHA
		{
			mbsptr[0] = 0xD8;
		}
		else if (0x0429 == ucsptr[0])	// CYRILLIC CAPITAL LETTER SHCHA
		{
			mbsptr[0] = 0xD9;
		}
		else if (0x042A == ucsptr[0])	// CYRILLIC CAPITAL LETTER HARD SIGN
		{
			mbsptr[0] = 0xDA;
		}
		else if (0x042B == ucsptr[0])	// CYRILLIC CAPITAL LETTER YERU
		{
			mbsptr[0] = 0xDB;
		}
		else if (0x042C == ucsptr[0])	// CYRILLIC CAPITAL LETTER SOFT SIGN
		{
			mbsptr[0] = 0xDC;
		}
		else if (0x042D == ucsptr[0])	// CYRILLIC CAPITAL LETTER E
		{
			mbsptr[0] = 0xDD;
		}
		else if (0x042E == ucsptr[0])	// CYRILLIC CAPITAL LETTER YU
		{
			mbsptr[0] = 0xDE;
		}
		else if (0x042F == ucsptr[0])	// CYRILLIC CAPITAL LETTER YA
		{
			mbsptr[0] = 0xDF;
		}
		else if (0x0430 == ucsptr[0])	// CYRILLIC SMALL LETTER A
		{
			mbsptr[0] = 0xE0;
		}
		else if (0x0431 == ucsptr[0])	// CYRILLIC SMALL LETTER BE
		{
			mbsptr[0] = 0xE1;
		}
		else if (0x0432 == ucsptr[0])	// CYRILLIC SMALL LETTER VE
		{
			mbsptr[0] = 0xE2;
		}
		else if (0x0433 == ucsptr[0])	// CYRILLIC SMALL LETTER GHE
		{
			mbsptr[0] = 0xE3;
		}
		else if (0x0434 == ucsptr[0])	// CYRILLIC SMALL LETTER DE
		{
			mbsptr[0] = 0xE4;
		}
		else if (0x0435 == ucsptr[0])	// CYRILLIC SMALL LETTER IE
		{
			mbsptr[0] = 0xE5;
		}
		else if (0x0436 == ucsptr[0])	// CYRILLIC SMALL LETTER ZHE
		{
			mbsptr[0] = 0xE6;
		}
		else if (0x0437 == ucsptr[0])	// CYRILLIC SMALL LETTER ZE
		{
			mbsptr[0] = 0xE7;
		}
		else if (0x0438 == ucsptr[0])	// CYRILLIC SMALL LETTER I
		{
			mbsptr[0] = 0xE8;
		}
		else if (0x0439 == ucsptr[0])	// CYRILLIC SMALL LETTER SHORT I
		{
			mbsptr[0] = 0xE9;
		}
		else if (0x043A == ucsptr[0])	// CYRILLIC SMALL LETTER KA
		{
			mbsptr[0] = 0xEA;
		}
		else if (0x043B == ucsptr[0])	// CYRILLIC SMALL LETTER EL
		{
			mbsptr[0] = 0xEB;
		}
		else if (0x043C == ucsptr[0])	// CYRILLIC SMALL LETTER EM
		{
			mbsptr[0] = 0xEC;
		}
		else if (0x043D == ucsptr[0])	// CYRILLIC SMALL LETTER EN
		{
			mbsptr[0] = 0xED;
		}
		else if (0x043E == ucsptr[0])	// CYRILLIC SMALL LETTER O
		{
			mbsptr[0] = 0xEE;
		}
		else if (0x043F == ucsptr[0])	// CYRILLIC SMALL LETTER PE
		{
			mbsptr[0] = 0xEF;
		}
		else if (0x0440 == ucsptr[0])	// CYRILLIC SMALL LETTER ER
		{
			mbsptr[0] = 0xF0;
		}
		else if (0x0441 == ucsptr[0])	// CYRILLIC SMALL LETTER ES
		{
			mbsptr[0] = 0xF1;
		}
		else if (0x0442 == ucsptr[0])	// CYRILLIC SMALL LETTER TE
		{
			mbsptr[0] = 0xF2;
		}
		else if (0x0443 == ucsptr[0])	// CYRILLIC SMALL LETTER U
		{
			mbsptr[0] = 0xF3;
		}
		else if (0x0444 == ucsptr[0])	// CYRILLIC SMALL LETTER EF
		{
			mbsptr[0] = 0xF4;
		}
		else if (0x0445 == ucsptr[0])	// CYRILLIC SMALL LETTER HA
		{
			mbsptr[0] = 0xF5;
		}
		else if (0x0446 == ucsptr[0])	// CYRILLIC SMALL LETTER TSE
		{
			mbsptr[0] = 0xF6;
		}
		else if (0x0447 == ucsptr[0])	// CYRILLIC SMALL LETTER CHE
		{
			mbsptr[0] = 0xF7;
		}
		else if (0x0448 == ucsptr[0])	// CYRILLIC SMALL LETTER SHA
		{
			mbsptr[0] = 0xF8;
		}
		else if (0x0449 == ucsptr[0])	// CYRILLIC SMALL LETTER SHCHA
		{
			mbsptr[0] = 0xF9;
		}

		if (0x044A == ucsptr[0])	// CYRILLIC SMALL LETTER HARD SIGN
		{
			mbsptr[0] = 0xFA;
		}
		else if (0x044B == ucsptr[0])	// CYRILLIC SMALL LETTER YERU
		{
			mbsptr[0] = 0xFB;
		}
		else if (0x044C == ucsptr[0])	// CYRILLIC SMALL LETTER SOFT SIGN
		{
			mbsptr[0] = 0xFC;
		}
		else if (0x044D == ucsptr[0])	// CYRILLIC SMALL LETTER E
		{
			mbsptr[0] = 0xFD;
		}
		else if (0x044E == ucsptr[0])	// CYRILLIC SMALL LETTER YU
		{
			mbsptr[0] = 0xFE;
		}
		else if (0x044F == ucsptr[0])	// CYRILLIC SMALL LETTER YA
		{
			mbsptr[0] = 0xFF;
		}
	}

	return numChars;
}

////////////////////////////////////////
//
// Convert wide char to code page 1253
//
////////////////////////////////////////

int WCSto1253(const wchar_t * ucsptr, unsigned char * mbsptr)

{
	int					numChars=1;
	unsigned char *		ptr=(unsigned char *)ucsptr;

	// is this just an ASCII character?
	if (ucsptr[0] < 128)
	{
		// just copy the ASCII character
		mbsptr[0] = ptr[0];
	}
	else
	{
		// check for other characters that can be displayed as is
		if (0x20AC == ucsptr[0])		// EURO SIGN
		{
			mbsptr[0] = 0x80;
		}
		else if (0x201A == ucsptr[0])	// SINGLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x82;
		}
		else if (0x0192 == ucsptr[0])	// LATIN SMALL LETTER F WITH HOOK
		{
			mbsptr[0] = 0x83;
		}
		else if (0x201E == ucsptr[0])	// DOUBLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x84;
		}
		else if (0x2026 == ucsptr[0])	// HORIZONTAL ELLIPSIS
		{
			mbsptr[0] = 0x85;
		}
		else if (0x2020 == ucsptr[0])	// DAGGER
		{
			mbsptr[0] = 0x86;
		}
		else if (0x2021 == ucsptr[0])	// DOUBLE DAGGER
		{
			mbsptr[0] = 0x87;
		}
		else if (0x2030 == ucsptr[0])	// PER MILLE SIGN
		{
			mbsptr[0] = 0x89;
		}
		else if (0x2039 == ucsptr[0])	// SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x8B;
		}
		else if (0x2018 == ucsptr[0])	// LEFT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x91;
		}
		else if (0x2019 == ucsptr[0])	// RIGHT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x92;
		}
		else if (0x201C == ucsptr[0])	// LEFT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x93;
		}
		else if (0x201D == ucsptr[0])	// RIGHT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x94;
		}
		else if (0x2022 == ucsptr[0])	// BULLET
		{
			mbsptr[0] = 0x95;
		}
		else if (0x2013 == ucsptr[0])	// EN DASH
		{
			mbsptr[0] = 0x96;
		}
		else if (0x2014 == ucsptr[0])	// EM DASH
		{
			mbsptr[0] = 0x97;
		}
		else if (0x2122 == ucsptr[0])	// TRADE MARK SIGN
		{
			mbsptr[0] = 0x99;
		}
		else if (0x203A == ucsptr[0])	// SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x9B;
		}
		else if (0x00A0 == ucsptr[0])	// NO-BREAK SPACE
		{
			mbsptr[0] = 0xA0;
		}
		else if (0x0385 == ucsptr[0])	// GREEK DIALYTIKA TONOS
		{
			mbsptr[0] = 0xA1;
		}
		else if (0x0386 == ucsptr[0])	// GREEK CAPITAL LETTER ALPHA WITH TONOS
		{
			mbsptr[0] = 0xA2;
		}
		else if (0x00A3 == ucsptr[0])	// POUND SIGN
		{
			mbsptr[0] = 0xA3;
		}
		else if (0x00A4 == ucsptr[0])	// CURRENCY SIGN
		{
			mbsptr[0] = 0xA4;
		}
		else if (0x00A5 == ucsptr[0])	// YEN SIGN
		{
			mbsptr[0] = 0xA5;
		}
		else if (0x00A6 == ucsptr[0])	// BROKEN BAR
		{
			mbsptr[0] = 0xA6;
		}
		else if (0x00A7 == ucsptr[0])	// SECTION SIGN
		{
			mbsptr[0] = 0xA7;
		}
		else if (0x00A8 == ucsptr[0])	// DIAERESIS
		{
			mbsptr[0] = 0xA8;
		}
		else if (0x00A9 == ucsptr[0])	// COPYRIGHT SIGN
		{
			mbsptr[0] = 0xA9;
		}
		else if (0x00AB == ucsptr[0])	// LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xAB;
		}
		else if (0x00AC == ucsptr[0])	// NOT SIGN
		{
			mbsptr[0] = 0xAC;
		}
		else if (0x00AD == ucsptr[0])	// SOFT HYPHEN
		{
			mbsptr[0] = 0xAD;
		}
		else if (0x00AE == ucsptr[0])	// REGISTERED SIGN
		{
			mbsptr[0] = 0xAE;
		}
		else if (0x2015 == ucsptr[0])	// HORIZONTAL BAR
		{
			mbsptr[0] = 0xAF;
		}
		else if (0x00B0 == ucsptr[0])	// DEGREE SIGN
		{
			mbsptr[0] = 0xB0;
		}
		else if (0x00B1 == ucsptr[0])	// PLUS-MINUS SIGN
		{
			mbsptr[0] = 0xB1;
		}
		else if (0x00B2 == ucsptr[0])	// SUPERSCRIPT TWO
		{
			mbsptr[0] = 0xB2;
		}
		else if (0x00B3 == ucsptr[0])	// SUPERSCRIPT THREE
		{
			mbsptr[0] = 0xB3;
		}
		else if (0x0384 == ucsptr[0])	// GREEK TONOS
		{
			mbsptr[0] = 0xB4;
		}
		else if (0x00B5 == ucsptr[0])	// MICRO SIGN
		{
			mbsptr[0] = 0xB5;
		}
		else if (0x00B6 == ucsptr[0])	// PILCROW SIGN
		{
			mbsptr[0] = 0xB6;
		}
		else if (0x00B7 == ucsptr[0])	// MIDDLE DOT
		{
			mbsptr[0] = 0xB7;
		}
		else if (0x0388 == ucsptr[0])	// GREEK CAPITAL LETTER EPSILON WITH TONOS
		{
			mbsptr[0] = 0xB8;
		}
		else if (0x0389 == ucsptr[0])	// GREEK CAPITAL LETTER ETA WITH TONOS
		{
			mbsptr[0] = 0xB9;
		}
		else if (0x038A == ucsptr[0])	// GREEK CAPITAL LETTER IOTA WITH TONOS
		{
			mbsptr[0] = 0xBA;
		}
		else if (0x00BB == ucsptr[0])	// RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xBB;
		}
		else if (0x038C == ucsptr[0])	// GREEK CAPITAL LETTER OMICRON WITH TONOS
		{
			mbsptr[0] = 0xBC;
		}
		else if (0x00BD == ucsptr[0])	// VULGAR FRACTION ONE HALF
		{
			mbsptr[0] = 0xBD;
		}
		else if (0x038E == ucsptr[0])	// GREEK CAPITAL LETTER UPSILON WITH TONOS
		{
			mbsptr[0] = 0xBE;
		}
		else if (0x038F == ucsptr[0])	// GREEK CAPITAL LETTER OMEGA WITH TONOS
		{
			mbsptr[0] = 0xBF;
		}
		else if (0x0390 == ucsptr[0])	// GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
		{
			mbsptr[0] = 0xC0;
		}
		else if (0x0391 == ucsptr[0])	// GREEK CAPITAL LETTER ALPHA
		{
			mbsptr[0] = 0xC1;
		}
		else if (0x0392 == ucsptr[0])	// GREEK CAPITAL LETTER BETA
		{
			mbsptr[0] = 0xC2;
		}
		else if (0x0393 == ucsptr[0])	// GREEK CAPITAL LETTER GAMMA
		{
			mbsptr[0] = 0xC3;
		}
		else if (0x0394 == ucsptr[0])	// GREEK CAPITAL LETTER DELTA
		{
			mbsptr[0] = 0xC4;
		}
		else if (0x0395 == ucsptr[0])	// GREEK CAPITAL LETTER EPSILON
		{
			mbsptr[0] = 0xC5;
		}
		else if (0x0396 == ucsptr[0])	// GREEK CAPITAL LETTER ZETA
		{
			mbsptr[0] = 0xC6;
		}
		else if (0x0397 == ucsptr[0])	// GREEK CAPITAL LETTER ETA
		{
			mbsptr[0] = 0xC7;
		}
		else if (0x0398 == ucsptr[0])	// GREEK CAPITAL LETTER THETA
		{
			mbsptr[0] = 0xC8;
		}
		else if (0x0399 == ucsptr[0])	// GREEK CAPITAL LETTER IOTA
		{
			mbsptr[0] = 0xC9;
		}
		else if (0x039A == ucsptr[0])	// GREEK CAPITAL LETTER KAPPA
		{
			mbsptr[0] = 0xCA;
		}
		else if (0x039B == ucsptr[0])	// GREEK CAPITAL LETTER LAMDA
		{
			mbsptr[0] = 0xCB;
		}
		else if (0x039C == ucsptr[0])	// GREEK CAPITAL LETTER MU
		{
			mbsptr[0] = 0xCC;
		}
		else if (0x039D == ucsptr[0])	// GREEK CAPITAL LETTER NU
		{
			mbsptr[0] = 0xCD;
		}
		else if (0x039E == ucsptr[0])	// GREEK CAPITAL LETTER XI
		{
			mbsptr[0] = 0xCE;
		}
		else if (0x039F == ucsptr[0])	// GREEK CAPITAL LETTER OMICRON
		{
			mbsptr[0] = 0xCF;
		}
		else if (0x03A0 == ucsptr[0])	// GREEK CAPITAL LETTER PI
		{
			mbsptr[0] = 0xD0;
		}
		else if (0x03A1 == ucsptr[0])	// GREEK CAPITAL LETTER RHO
		{
			mbsptr[0] = 0xD1;
		}
		else if (0x03A3 == ucsptr[0])	// GREEK CAPITAL LETTER SIGMA
		{
			mbsptr[0] = 0xD3;
		}
		else if (0x03A4 == ucsptr[0])	// GREEK CAPITAL LETTER TAU
		{
			mbsptr[0] = 0xD4;
		}
		else if (0x03A5 == ucsptr[0])	// GREEK CAPITAL LETTER UPSILON
		{
			mbsptr[0] = 0xD5;
		}
		else if (0x03A6 == ucsptr[0])	// GREEK CAPITAL LETTER PHI
		{
			mbsptr[0] = 0xD6;
		}
		else if (0x03A7 == ucsptr[0])	// GREEK CAPITAL LETTER CHI
		{
			mbsptr[0] = 0xD7;
		}
		else if (0x03A8 == ucsptr[0])	// GREEK CAPITAL LETTER PSI
		{
			mbsptr[0] = 0xD8;
		}
		else if (0x03A9 == ucsptr[0])	// GREEK CAPITAL LETTER OMEGA
		{
			mbsptr[0] = 0xD9;
		}
		else if (0x03AA == ucsptr[0])	// GREEK CAPITAL LETTER IOTA WITH DIALYTIKA
		{
			mbsptr[0] = 0xDA;
		}
		else if (0x03AB == ucsptr[0])	// GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA
		{
			mbsptr[0] = 0xDB;
		}
		else if (0x03AC == ucsptr[0])	// GREEK SMALL LETTER ALPHA WITH TONOS
		{
			mbsptr[0] = 0xDC;
		}
		else if (0x03AD == ucsptr[0])	// GREEK SMALL LETTER EPSILON WITH TONOS
		{
			mbsptr[0] = 0xDD;
		}
		else if (0x03AE == ucsptr[0])	// GREEK SMALL LETTER ETA WITH TONOS
		{
			mbsptr[0] = 0xDE;
		}
		else if (0x03AF == ucsptr[0])	// GREEK SMALL LETTER IOTA WITH TONOS
		{
			mbsptr[0] = 0xDF;
		}
		else if (0x03B0 == ucsptr[0])	// GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
		{
			mbsptr[0] = 0xE0;
		}
		else if (0x03B1 == ucsptr[0])	// GREEK SMALL LETTER ALPHA
		{
			mbsptr[0] = 0xE1;
		}
		else if (0x03B2 == ucsptr[0])	// GREEK SMALL LETTER BETA
		{
			mbsptr[0] = 0xE2;
		}
		else if (0x03B3 == ucsptr[0])	// GREEK SMALL LETTER GAMMA
		{
			mbsptr[0] = 0xE3;
		}
		else if (0x03B4 == ucsptr[0])	// GREEK SMALL LETTER DELTA
		{
			mbsptr[0] = 0xE4;
		}
		else if (0x03B5 == ucsptr[0])	// GREEK SMALL LETTER EPSILON
		{
			mbsptr[0] = 0xE5;
		}
		else if (0x03B6 == ucsptr[0])	// GREEK SMALL LETTER ZETA
		{
			mbsptr[0] = 0xE6;
		}
		else if (0x03B7 == ucsptr[0])	// GREEK SMALL LETTER ETA
		{
			mbsptr[0] = 0xE7;
		}
		else if (0x03B8 == ucsptr[0])	// GREEK SMALL LETTER THETA
		{
			mbsptr[0] = 0xE8;
		}
		else if (0x03B9 == ucsptr[0])	// GREEK SMALL LETTER IOTA
		{
			mbsptr[0] = 0xE9;
		}
		else if (0x03BA == ucsptr[0])	// GREEK SMALL LETTER KAPPA
		{
			mbsptr[0] = 0xEA;
		}
		else if (0x03BB == ucsptr[0])	// GREEK SMALL LETTER LAMDA
		{
			mbsptr[0] = 0xEB;
		}
		else if (0x03BC == ucsptr[0])	// GREEK SMALL LETTER MU
		{
			mbsptr[0] = 0xEC;
		}
		else if (0x03BD == ucsptr[0])	// GREEK SMALL LETTER NU
		{
			mbsptr[0] = 0xED;
		}
		else if (0x03BE == ucsptr[0])	// GREEK SMALL LETTER XI
		{
			mbsptr[0] = 0xEE;
		}
		else if (0x03BF == ucsptr[0])	// GREEK SMALL LETTER OMICRON
		{
			mbsptr[0] = 0xEF;
		}
		else if (0x03C0 == ucsptr[0])	// GREEK SMALL LETTER PI
		{
			mbsptr[0] = 0xF0;
		}
		else if (0x03C1 == ucsptr[0])	// GREEK SMALL LETTER RHO
		{
			mbsptr[0] = 0xF1;
		}
		else if (0x03C2 == ucsptr[0])	// GREEK SMALL LETTER FINAL SIGMA
		{
			mbsptr[0] = 0xF2;
		}
		else if (0x03C3 == ucsptr[0])	// GREEK SMALL LETTER SIGMA
		{
			mbsptr[0] = 0xF3;
		}
		else if (0x03C4 == ucsptr[0])	// GREEK SMALL LETTER TAU
		{
			mbsptr[0] = 0xF4;
		}
		else if (0x03C5 == ucsptr[0])	// GREEK SMALL LETTER UPSILON
		{
			mbsptr[0] = 0xF5;
		}
		else if (0x03C6 == ucsptr[0])	// GREEK SMALL LETTER PHI
		{
			mbsptr[0] = 0xF6;
		}
		else if (0x03C7 == ucsptr[0])	// GREEK SMALL LETTER CHI
		{
			mbsptr[0] = 0xF7;
		}
		else if (0x03C8 == ucsptr[0])	// GREEK SMALL LETTER PSI
		{
			mbsptr[0] = 0xF8;
		}
		else if (0x03C9 == ucsptr[0])	// GREEK SMALL LETTER OMEGA
		{
			mbsptr[0] = 0xF9;
		}
		else if (0x03CA == ucsptr[0])	// GREEK SMALL LETTER IOTA WITH DIALYTIKA
		{
			mbsptr[0] = 0xFA;
		}
		else if (0x03CB == ucsptr[0])	// GREEK SMALL LETTER UPSILON WITH DIALYTIKA
		{
			mbsptr[0] = 0xFB;
		}
		else if (0x03CC == ucsptr[0])	// GREEK SMALL LETTER OMICRON WITH TONOS
		{
			mbsptr[0] = 0xFC;
		}
		else if (0x03CD == ucsptr[0])	// GREEK SMALL LETTER UPSILON WITH TONOS
		{
			mbsptr[0] = 0xFD;
		}
		else if (0x03CE == ucsptr[0])	// GREEK SMALL LETTER OMEGA WITH TONOS
		{
			mbsptr[0] = 0xFE;
		}
		else
		{
			// undefined character - substitute a period
			mbsptr[0] = '.';
		}
	}

	return numChars;
}

////////////////////////////////////////
//
// Convert wide char to code page 1254
//
////////////////////////////////////////

int WCSto1254(const wchar_t * ucsptr, unsigned char * mbsptr)

{
	int					numChars=1;
	unsigned char *		ptr=(unsigned char *)ucsptr;

	// is this just an ASCII character?
	if (ucsptr[0] < 128)
	{
		// just copy the ASCII character
		mbsptr[0] = ptr[0];
	}
	else
	{
		// check for other characters that can be displayed as is
		if (0x20AC == ucsptr[0])		// EURO SIGN
		{
			mbsptr[0] = 0x80;
		}
		else if (0x201A == ucsptr[0])	// SINGLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x82;
		}
		else if (0x0192 == ucsptr[0])	// LATIN SMALL LETTER F WITH HOOK
		{
			mbsptr[0] = 0x83;
		}
		else if (0x201E == ucsptr[0])	// DOUBLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x84;
		}
		else if (0x2026 == ucsptr[0])	// HORIZONTAL ELLIPSIS
		{
			mbsptr[0] = 0x85;
		}
		else if (0x2020 == ucsptr[0])	// DAGGER
		{
			mbsptr[0] = 0x86;
		}
		else if (0x2021 == ucsptr[0])	// DOUBLE DAGGER
		{
			mbsptr[0] = 0x87;
		}
		else if (0x02C6 == ucsptr[0])	// MODIFIER LETTER CIRCUMFLEX ACCENT
		{
			mbsptr[0] = 0x88;
		}
		else if (0x2030 == ucsptr[0])	// PER MILLE SIGN
		{
			mbsptr[0] = 0x89;
		}
		else if (0x0160 == ucsptr[0])	// LATIN CAPITAL LETTER S WITH CARON
		{
			mbsptr[0] = 0x8A;
		}
		else if (0x2039 == ucsptr[0])	// SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x8B;
		}
		else if (0x0152 == ucsptr[0])	// LATIN CAPITAL LIGATURE OE
		{
			mbsptr[0] = 0x8C;
		}
		else if (0x2018 == ucsptr[0])	// LEFT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x91;
		}
		else if (0x2019 == ucsptr[0])	// RIGHT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x92;
		}
		else if (0x201C == ucsptr[0])	// LEFT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x93;
		}
		else if (0x201D == ucsptr[0])	// RIGHT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x94;
		}
		else if (0x2022 == ucsptr[0])	// BULLET
		{
			mbsptr[0] = 0x95;
		}
		else if (0x2013 == ucsptr[0])	// EN DASH
		{
			mbsptr[0] = 0x96;
		}
		else if (0x2014 == ucsptr[0])	// EM DASH
		{
			mbsptr[0] = 0x97;
		}
		else if (0x02DC == ucsptr[0])	// SMALL TILDE
		{
			mbsptr[0] = 0x98;
		}
		else if (0x2122 == ucsptr[0])	// TRADE MARK SIGN
		{
			mbsptr[0] = 0x99;
		}
		else if (0x0161 == ucsptr[0])	// LATIN SMALL LETTER S WITH CARON
		{
			mbsptr[0] = 0x9A;
		}
		else if (0x203A == ucsptr[0])	// SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x9B;
		}
		else if (0x0153 == ucsptr[0])	// LATIN SMALL LIGATURE OE
		{
			mbsptr[0] = 0x9C;
		}
		else if (0x0178 == ucsptr[0])	// LATIN CAPITAL LETTER Y WITH DIAERESIS
		{
			mbsptr[0] = 0x9F;
		}
		else if (0x00A0 == ucsptr[0])	// NO-BREAK SPACE
		{
			mbsptr[0] = 0xA0;
		}
		else if (0x00A1 == ucsptr[0])	// INVERTED EXCLAMATION MARK
		{
			mbsptr[0] = 0xA1;
		}
		else if (0x00A2 == ucsptr[0])	// CENT SIGN
		{
			mbsptr[0] = 0xA2;
		}
		else if (0x00A3 == ucsptr[0])	// POUND SIGN
		{
			mbsptr[0] = 0xA3;
		}
		else if (0x00A4 == ucsptr[0])	// CURRENCY SIGN
		{
			mbsptr[0] = 0xA4;
		}
		else if (0x00A5 == ucsptr[0])	// YEN SIGN
		{
			mbsptr[0] = 0xA5;
		}
		else if (0x00A6 == ucsptr[0])	// BROKEN BAR
		{
			mbsptr[0] = 0xA6;
		}
		else if (0x00A7 == ucsptr[0])	// SECTION SIGN
		{
			mbsptr[0] = 0xA7;
		}
		else if (0x00A8 == ucsptr[0])	// DIAERESIS
		{
			mbsptr[0] = 0xA8;
		}
		else if (0x00A9 == ucsptr[0])	// COPYRIGHT SIGN
		{
			mbsptr[0] = 0xA9;
		}
		else if (0x00AA == ucsptr[0])	// FEMININE ORDINAL INDICATOR
		{
			mbsptr[0] = 0xAA;
		}
		else if (0x00AB == ucsptr[0])	// LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xAB;
		}
		else if (0x00AC == ucsptr[0])	// NOT SIGN
		{
			mbsptr[0] = 0xAC;
		}
		else if (0x00AD == ucsptr[0])	// SOFT HYPHEN
		{
			mbsptr[0] = 0xAD;
		}
		else if (0x00AE == ucsptr[0])	// REGISTERED SIGN
		{
			mbsptr[0] = 0xAE;
		}
		else if (0x00AF == ucsptr[0])	// MACRON
		{
			mbsptr[0] = 0xAF;
		}
		else if (0x00B0 == ucsptr[0])	// DEGREE SIGN
		{
			mbsptr[0] = 0xB0;
		}
		else if (0x00B1 == ucsptr[0])	// PLUS-MINUS SIGN
		{
			mbsptr[0] = 0xB1;
		}
		else if (0x00B2 == ucsptr[0])	// SUPERSCRIPT TWO
		{
			mbsptr[0] = 0xB2;
		}
		else if (0x00B3 == ucsptr[0])	// SUPERSCRIPT THREE
		{
			mbsptr[0] = 0xB3;
		}
		else if (0x00B4 == ucsptr[0])	// ACUTE ACCENT
		{
			mbsptr[0] = 0xB4;
		}
		else if (0x00B5 == ucsptr[0])	// MICRO SIGN
		{
			mbsptr[0] = 0xB5;
		}
		else if (0x00B6 == ucsptr[0])	// PILCROW SIGN
		{
			mbsptr[0] = 0xB6;
		}
		else if (0x00B7 == ucsptr[0])	// MIDDLE DOT
		{
			mbsptr[0] = 0xB7;
		}
		else if (0x00B8 == ucsptr[0])	// CEDILLA
		{
			mbsptr[0] = 0xB8;
		}
		else if (0x00B9 == ucsptr[0])	// SUPERSCRIPT ONE
		{
			mbsptr[0] = 0xB9;
		}
		else if (0x00BA == ucsptr[0])	// MASCULINE ORDINAL INDICATOR
		{
			mbsptr[0] = 0xBA;
		}
		else if (0x00BB == ucsptr[0])	// RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xBB;
		}
		else if (0x00BC == ucsptr[0])	// VULGAR FRACTION ONE QUARTER
		{
			mbsptr[0] = 0xBC;
		}
		else if (0x00BD == ucsptr[0])	// VULGAR FRACTION ONE HALF
		{
			mbsptr[0] = 0xBD;
		}
		else if (0x00BE == ucsptr[0])	// VULGAR FRACTION THREE QUARTERS
		{
			mbsptr[0] = 0xBE;
		}
		else if (0x00BF == ucsptr[0])	// INVERTED QUESTION MARK
		{
			mbsptr[0] = 0xBF;
		}
		else if (0x00C0 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH GRAVE
		{
			mbsptr[0] = 0xC0;
		}
		else if (0x00C1 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH ACUTE
		{
			mbsptr[0] = 0xC1;
		}
		else if (0x00C2 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xC2;
		}
		else if (0x00C3 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH TILDE
		{
			mbsptr[0] = 0xC3;
		}
		else if (0x00C4 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH DIAERESIS
		{
			mbsptr[0] = 0xC4;
		}
		else if (0x00C5 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH RING ABOVE
		{
			mbsptr[0] = 0xC5;
		}
		else if (0x00C6 == ucsptr[0])	// LATIN CAPITAL LETTER AE
		{
			mbsptr[0] = 0xC6;
		}
		else if (0x00C7 == ucsptr[0])	// LATIN CAPITAL LETTER C WITH CEDILLA
		{
			mbsptr[0] = 0xC7;
		}
		else if (0x00C8 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH GRAVE
		{
			mbsptr[0] = 0xC8;
		}
		else if (0x00C9 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH ACUTE
		{
			mbsptr[0] = 0xC9;
		}
		else if (0x00CA == ucsptr[0])	// LATIN CAPITAL LETTER E WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xCA;
		}
		else if (0x00CB == ucsptr[0])	// LATIN CAPITAL LETTER E WITH DIAERESIS
		{
			mbsptr[0] = 0xCB;
		}
		else if (0x00CC == ucsptr[0])	// LATIN CAPITAL LETTER I WITH GRAVE
		{
			mbsptr[0] = 0xCC;
		}
		else if (0x00CD == ucsptr[0])	// LATIN CAPITAL LETTER I WITH ACUTE
		{
			mbsptr[0] = 0xCD;
		}
		else if (0x00CE == ucsptr[0])	// LATIN CAPITAL LETTER I WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xCE;
		}
		else if (0x00CF == ucsptr[0])	// LATIN CAPITAL LETTER I WITH DIAERESIS
		{
			mbsptr[0] = 0xCF;
		}
		else if (0x011E == ucsptr[0])	// LATIN CAPITAL LETTER G WITH BREVE
		{
			mbsptr[0] = 0xD0;
		}
		else if (0x00D1 == ucsptr[0])	// LATIN CAPITAL LETTER N WITH TILDE
		{
			mbsptr[0] = 0xD1;
		}
		else if (0x00D2 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH GRAVE
		{
			mbsptr[0] = 0xD2;
		}
		else if (0x00D3 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH ACUTE
		{
			mbsptr[0] = 0xD3;
		}
		else if (0x00D4 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xD4;
		}
		else if (0x00D5 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH TILDE
		{
			mbsptr[0] = 0xD5;
		}
		else if (0x00D6 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH DIAERESIS
		{
			mbsptr[0] = 0xD6;
		}
		else if (0x00D7 == ucsptr[0])	// MULTIPLICATION SIGN
		{
			mbsptr[0] = 0xD7;
		}
		else if (0x00D8 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH STROKE
		{
			mbsptr[0] = 0xD8;
		}
		else if (0x00D9 == ucsptr[0])	// LATIN CAPITAL LETTER U WITH GRAVE
		{
			mbsptr[0] = 0xD9;
		}
		else if (0x00DA == ucsptr[0])	// LATIN CAPITAL LETTER U WITH ACUTE
		{
			mbsptr[0] = 0xDA;
		}
		else if (0x00DB == ucsptr[0])	// LATIN CAPITAL LETTER U WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xDB;
		}
		else if (0x00DC == ucsptr[0])	// LATIN CAPITAL LETTER U WITH DIAERESIS
		{
			mbsptr[0] = 0xDC;
		}
		else if (0x0130 == ucsptr[0])	// LATIN CAPITAL LETTER I WITH DOT ABOVE
		{
			mbsptr[0] = 0xDD;
		}
		else if (0x015E == ucsptr[0])	// LATIN CAPITAL LETTER S WITH CEDILLA
		{
			mbsptr[0] = 0xDE;
		}
		else if (0x00DF == ucsptr[0])	// LATIN SMALL LETTER SHARP S
		{
			mbsptr[0] = 0xDF;
		}
		else if (0x00E0 == ucsptr[0])	// LATIN SMALL LETTER A WITH GRAVE
		{
			mbsptr[0] = 0xE0;
		}
		else if (0x00E1 == ucsptr[0])	// LATIN SMALL LETTER A WITH ACUTE
		{
			mbsptr[0] = 0xE1;
		}
		else if (0x00E2 == ucsptr[0])	// LATIN SMALL LETTER A WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xE2;
		}
		else if (0x00E3 == ucsptr[0])	// LATIN SMALL LETTER A WITH TILDE
		{
			mbsptr[0] = 0xE3;
		}
		else if (0x00E4 == ucsptr[0])	// LATIN SMALL LETTER A WITH DIAERESIS
		{
			mbsptr[0] = 0xE4;
		}
		else if (0x00E5 == ucsptr[0])	// LATIN SMALL LETTER A WITH RING ABOVE
		{
			mbsptr[0] = 0xE5;
		}
		else if (0x00E6 == ucsptr[0])	// LATIN SMALL LETTER AE
		{
			mbsptr[0] = 0xE6;
		}
		else if (0x00E7 == ucsptr[0])	// LATIN SMALL LETTER C WITH CEDILLA
		{
			mbsptr[0] = 0xE7;
		}
		else if (0x00E8 == ucsptr[0])	// LATIN SMALL LETTER E WITH GRAVE
		{
			mbsptr[0] = 0xE8;
		}
		else if (0x00E9 == ucsptr[0])	// LATIN SMALL LETTER E WITH ACUTE
		{
			mbsptr[0] = 0xE9;
		}
		else if (0x00EA == ucsptr[0])	// LATIN SMALL LETTER E WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xEA;
		}
		else if (0x00EB == ucsptr[0])	// LATIN SMALL LETTER E WITH DIAERESIS
		{
			mbsptr[0] = 0xEB;
		}
		else if (0x00EC == ucsptr[0])	// LATIN SMALL LETTER I WITH GRAVE
		{
			mbsptr[0] = 0xEC;
		}
		else if (0x00ED == ucsptr[0])	// LATIN SMALL LETTER I WITH ACUTE
		{
			mbsptr[0] = 0xED;
		}
		else if (0x00EE == ucsptr[0])	// LATIN SMALL LETTER I WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xEE;
		}
		else if (0x00EF == ucsptr[0])	// LATIN SMALL LETTER I WITH DIAERESIS
		{
			mbsptr[0] = 0xEF;
		}
		else if (0x011F == ucsptr[0])	// LATIN SMALL LETTER G WITH BREVE
		{
			mbsptr[0] = 0xF0;
		}
		else if (0x00F1 == ucsptr[0])	// LATIN SMALL LETTER N WITH TILDE
		{
			mbsptr[0] = 0xF1;
		}
		else if (0x00F2 == ucsptr[0])	// LATIN SMALL LETTER O WITH GRAVE
		{
			mbsptr[0] = 0xF2;
		}
		else if (0x00F3 == ucsptr[0])	// LATIN SMALL LETTER O WITH ACUTE
		{
			mbsptr[0] = 0xF3;
		}
		else if (0x00F4 == ucsptr[0])	// LATIN SMALL LETTER O WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xF4;
		}
		else if (0x00F5 == ucsptr[0])	// LATIN SMALL LETTER O WITH TILDE
		{
			mbsptr[0] = 0xF5;
		}
		else if (0x00F6 == ucsptr[0])	// LATIN SMALL LETTER O WITH DIAERESIS
		{
			mbsptr[0] = 0xF6;
		}
		else if (0x00F7 == ucsptr[0])	// DIVISION SIGN
		{
			mbsptr[0] = 0xF7;
		}
		else if (0x00F8 == ucsptr[0])	// LATIN SMALL LETTER O WITH STROKE
		{
			mbsptr[0] = 0xF8;
		}
		else if (0x00F9 == ucsptr[0])	// LATIN SMALL LETTER U WITH GRAVE
		{
			mbsptr[0] = 0xF9;
		}
		else if (0x00FA == ucsptr[0])	// LATIN SMALL LETTER U WITH ACUTE
		{
			mbsptr[0] = 0xFA;
		}
		else if (0x00FB == ucsptr[0])	// LATIN SMALL LETTER U WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xFB;
		}
		else if (0x00FC == ucsptr[0])	// LATIN SMALL LETTER U WITH DIAERESIS
		{
			mbsptr[0] = 0xFC;
		}
		else if (0x0131 == ucsptr[0])	// LATIN SMALL LETTER DOTLESS I
		{
			mbsptr[0] = 0xFD;
		}
		else if (0x015F == ucsptr[0])	// LATIN SMALL LETTER S WITH CEDILLA
		{
			mbsptr[0] = 0xFE;
		}
		else if (0x00FF == ucsptr[0])	// LATIN SMALL LETTER Y WITH DIAERESIS
		{
			mbsptr[0] = 0xFF;
		}
		else
		{
			// undefined character - substitute a period
			mbsptr[0] = '.';
		}
	}

	return numChars;
}

////////////////////////////////////////
//
// Convert wide char to code page 1255
//
////////////////////////////////////////

int WCSto1255(const wchar_t * ucsptr, unsigned char * mbsptr)

{
	int					numChars=1;
	unsigned char *		ptr=(unsigned char *)ucsptr;

	// is this just an ASCII character?
	if (ucsptr[0] < 128)
	{
		// just copy the ASCII character
		mbsptr[0] = ptr[0];
	}
	else
	{
		// check for other characters that can be displayed as is
		if (0x20AC == ucsptr[0])		// EURO SIGN
		{
			mbsptr[0] = 0x80;
		}
		else if (0x201A == ucsptr[0])	// SINGLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x82;
		}
		else if (0x0192 == ucsptr[0])	// LATIN SMALL LETTER F WITH HOOK
		{
			mbsptr[0] = 0x83;
		}
		else if (0x201E == ucsptr[0])	// DOUBLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x84;
		}
		else if (0x2026 == ucsptr[0])	// HORIZONTAL ELLIPSIS
		{
			mbsptr[0] = 0x85;
		}
		else if (0x2020 == ucsptr[0])	// DAGGER
		{
			mbsptr[0] = 0x86;
		}
		else if (0x2021 == ucsptr[0])	// DOUBLE DAGGER
		{
			mbsptr[0] = 0x87;
		}
		else if (0x2030 == ucsptr[0])	// PER MILLE SIGN
		{
			mbsptr[0] = 0x89;
		}
		else if (0x2039 == ucsptr[0])	// SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x8B;
		}
		else if (0x2018 == ucsptr[0])	// LEFT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x91;
		}
		else if (0x2019 == ucsptr[0])	// RIGHT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x92;
		}
		else if (0x201C == ucsptr[0])	// LEFT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x93;
		}
		else if (0x201D == ucsptr[0])	// RIGHT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x94;
		}
		else if (0x2022 == ucsptr[0])	// BULLET
		{
			mbsptr[0] = 0x95;
		}
		else if (0x2013 == ucsptr[0])	// EN DASH
		{
			mbsptr[0] = 0x96;
		}
		else if (0x2014 == ucsptr[0])	// EM DASH
		{
			mbsptr[0] = 0x97;
		}
		else if (0x02DC == ucsptr[0])	// SMALL TILDE
		{
			mbsptr[0] = 0x98;
		}
		else if (0x2122 == ucsptr[0])	// TRADE MARK SIGN
		{
			mbsptr[0] = 0x99;
		}
		else if (0x203A == ucsptr[0])	// SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x9B;
		}
		else if (0x00A0 == ucsptr[0])	// NO-BREAK SPACE
		{
			mbsptr[0] = 0xA0;
		}
		else if (0x00A1 == ucsptr[0])	// INVERTED EXCLAMATION MARK
		{
			mbsptr[0] = 0xA1;
		}
		else if (0x00A2 == ucsptr[0])	// CENT SIGN
		{
			mbsptr[0] = 0xA2;
		}
		else if (0x00A3 == ucsptr[0])	// POUND SIGN
		{
			mbsptr[0] = 0xA3;
		}
		else if (0x20AA == ucsptr[0])	// NEW SHEQEL SIGN
		{
			mbsptr[0] = 0xA4;
		}
		else if (0x00A5 == ucsptr[0])	// YEN SIGN
		{
			mbsptr[0] = 0xA5;
		}
		else if (0x00A6 == ucsptr[0])	// BROKEN BAR
		{
			mbsptr[0] = 0xA6;
		}
		else if (0x00A7 == ucsptr[0])	// SECTION SIGN
		{
			mbsptr[0] = 0xA7;
		}
		else if (0x00A8 == ucsptr[0])	// DIAERESIS
		{
			mbsptr[0] = 0xA8;
		}
		else if (0x00A9 == ucsptr[0])	// COPYRIGHT SIGN
		{
			mbsptr[0] = 0xA9;
		}
		else if (0x00D7 == ucsptr[0])	// MULTIPLICATION SIGN
		{
			mbsptr[0] = 0xAA;
		}
		else if (0x00AB == ucsptr[0])	// LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xAB;
		}
		else if (0x00AC == ucsptr[0])	// NOT SIGN
		{
			mbsptr[0] = 0xAC;
		}
		else if (0x00AD == ucsptr[0])	// SOFT HYPHEN
		{
			mbsptr[0] = 0xAD;
		}
		else if (0x00AE == ucsptr[0])	// REGISTERED SIGN
		{
			mbsptr[0] = 0xAE;
		}
		else if (0x00AF == ucsptr[0])	// MACRON
		{
			mbsptr[0] = 0xAF;
		}
		else if (0x00B0 == ucsptr[0])	// DEGREE SIGN
		{
			mbsptr[0] = 0xB0;
		}
		else if (0x00B1 == ucsptr[0])	// PLUS-MINUS SIGN
		{
			mbsptr[0] = 0xB1;
		}
		else if (0x00B2 == ucsptr[0])	// SUPERSCRIPT TWO
		{
			mbsptr[0] = 0xB2;
		}
		else if (0x00B3 == ucsptr[0])	// SUPERSCRIPT THREE
		{
			mbsptr[0] = 0xB3;
		}
		else if (0x00B4 == ucsptr[0])	// ACUTE ACCENT
		{
			mbsptr[0] = 0xB4;
		}
		else if (0x00B5 == ucsptr[0])	// MICRO SIGN
		{
			mbsptr[0] = 0xB5;
		}
		else if (0x00B6 == ucsptr[0])	// PILCROW SIGN
		{
			mbsptr[0] = 0xB6;
		}
		else if (0x00B7 == ucsptr[0])	// MIDDLE DOT
		{
			mbsptr[0] = 0xB7;
		}
		else if (0x00B8 == ucsptr[0])	// CEDILLA
		{
			mbsptr[0] = 0xB8;
		}
		else if (0x00B9 == ucsptr[0])	// SUPERSCRIPT ONE
		{
			mbsptr[0] = 0xB9;
		}
		else if (0x00F7 == ucsptr[0])	// DIVISION SIGN
		{
			mbsptr[0] = 0xBA;
		}
		else if (0x00BB == ucsptr[0])	// RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xBB;
		}
		else if (0x00BC == ucsptr[0])	// VULGAR FRACTION ONE QUARTER
		{
			mbsptr[0] = 0xBC;
		}
		else if (0x00BD == ucsptr[0])	// VULGAR FRACTION ONE HALF
		{
			mbsptr[0] = 0xBD;
		}
		else if (0x00BE == ucsptr[0])	// VULGAR FRACTION THREE QUARTERS
		{
			mbsptr[0] = 0xBE;
		}
		else if (0x00BF == ucsptr[0])	// INVERTED QUESTION MARK
		{
			mbsptr[0] = 0xBF;
		}
		else if (0x05B0 == ucsptr[0])	// HEBREW POINT SHEVA
		{
			mbsptr[0] = 0xC0;
		}
		else if (0x05B1 == ucsptr[0])	// HEBREW POINT HATAF SEGOL
		{
			mbsptr[0] = 0xC1;
		}
		else if (0x05B2 == ucsptr[0])	// HEBREW POINT HATAF PATAH
		{
			mbsptr[0] = 0xC2;
		}
		else if (0x05B3 == ucsptr[0])	// HEBREW POINT HATAF QAMATS
		{
			mbsptr[0] = 0xC3;
		}
		else if (0x05B4 == ucsptr[0])	// HEBREW POINT HIRIQ
		{
			mbsptr[0] = 0xC4;
		}
		else if (0x05B5 == ucsptr[0])	// HEBREW POINT TSERE
		{
			mbsptr[0] = 0xC5;
		}
		else if (0x05B6 == ucsptr[0])	// HEBREW POINT SEGOL
		{
			mbsptr[0] = 0xC6;
		}
		else if (0x05B7 == ucsptr[0])	// HEBREW POINT PATAH
		{
			mbsptr[0] = 0xC7;
		}
		else if (0x05B8 == ucsptr[0])	// HEBREW POINT QAMATS
		{
			mbsptr[0] = 0xC8;
		}
		else if (0x05B9 == ucsptr[0])	// HEBREW POINT HOLAM
		{
			mbsptr[0] = 0xC9;
		}
		else if (0x05BB == ucsptr[0])	// HEBREW POINT QUBUTS
		{
			mbsptr[0] = 0xCB;
		}
		else if (0x05BC == ucsptr[0])	// HEBREW POINT DAGESH OR MAPIQ
		{
			mbsptr[0] = 0xCC;
		}
		else if (0x05BD == ucsptr[0])	// HEBREW POINT METEG
		{
			mbsptr[0] = 0xCD;
		}
		else if (0x05BE == ucsptr[0])	// HEBREW PUNCTUATION MAQAF
		{
			mbsptr[0] = 0xCE;
		}
		else if (0x05BF == ucsptr[0])	// HEBREW POINT RAFE
		{
			mbsptr[0] = 0xCF;
		}
		else if (0x05C0 == ucsptr[0])	// HEBREW PUNCTUATION PASEQ
		{
			mbsptr[0] = 0xD0;
		}
		else if (0x05C1 == ucsptr[0])	// HEBREW POINT SHIN DOT
		{
			mbsptr[0] = 0xD1;
		}
		else if (0x05C2 == ucsptr[0])	// HEBREW POINT SIN DOT
		{
			mbsptr[0] = 0xD2;
		}
		else if (0x05C3 == ucsptr[0])	// HEBREW PUNCTUATION SOF PASUQ
		{
			mbsptr[0] = 0xD3;
		}
		else if (0x05F0 == ucsptr[0])	// HEBREW LIGATURE YIDDISH DOUBLE VAV
		{
			mbsptr[0] = 0xD4;
		}
		else if (0x05F1 == ucsptr[0])	// HEBREW LIGATURE YIDDISH VAV YOD
		{
			mbsptr[0] = 0xD5;
		}
		else if (0x05F2 == ucsptr[0])	// HEBREW LIGATURE YIDDISH DOUBLE YOD
		{
			mbsptr[0] = 0xD6;
		}
		else if (0x05F3 == ucsptr[0])	// HEBREW PUNCTUATION GERESH
		{
			mbsptr[0] = 0xD7;
		}
		else if (0x05F4 == ucsptr[0])	// HEBREW PUNCTUATION GERSHAYIM
		{
			mbsptr[0] = 0xD8;
		}
		else if (0x05D0 == ucsptr[0])	// HEBREW LETTER ALEF
		{
			mbsptr[0] = 0xE0;
		}
		else if (0x05D1 == ucsptr[0])	// HEBREW LETTER BET
		{
			mbsptr[0] = 0xE1;
		}
		else if (0x05D2 == ucsptr[0])	// HEBREW LETTER GIMEL
		{
			mbsptr[0] = 0xE2;
		}
		else if (0x05D3 == ucsptr[0])	// HEBREW LETTER DALET
		{
			mbsptr[0] = 0xE3;
		}
		else if (0x05D4 == ucsptr[0])	// HEBREW LETTER HE
		{
			mbsptr[0] = 0xE4;
		}
		else if (0x05D5 == ucsptr[0])	// HEBREW LETTER VAV
		{
			mbsptr[0] = 0xE5;
		}
		else if (0x05D6 == ucsptr[0])	// HEBREW LETTER ZAYIN
		{
			mbsptr[0] = 0xE6;
		}
		else if (0x05D7 == ucsptr[0])	// HEBREW LETTER HET
		{
			mbsptr[0] = 0xE7;
		}
		else if (0x05D8 == ucsptr[0])	// HEBREW LETTER TET
		{
			mbsptr[0] = 0xE8;
		}
		else if (0x05D9 == ucsptr[0])	// HEBREW LETTER YOD
		{
			mbsptr[0] = 0xE9;
		}
		else if (0x05DA == ucsptr[0])	// HEBREW LETTER FINAL KAF
		{
			mbsptr[0] = 0xEA;
		}
		else if (0x05DB == ucsptr[0])	// HEBREW LETTER KAF
		{
			mbsptr[0] = 0xEB;
		}
		else if (0x05DC == ucsptr[0])	// HEBREW LETTER LAMED
		{
			mbsptr[0] = 0xEC;
		}
		else if (0x05DD == ucsptr[0])	// HEBREW LETTER FINAL MEM
		{
			mbsptr[0] = 0xED;
		}
		else if (0x05DE == ucsptr[0])	// HEBREW LETTER MEM
		{
			mbsptr[0] = 0xEE;
		}
		else if (0x05DF == ucsptr[0])	// HEBREW LETTER FINAL NUN
		{
			mbsptr[0] = 0xEF;
		}
		else if (0x05E0 == ucsptr[0])	// HEBREW LETTER NUN
		{
			mbsptr[0] = 0xF0;
		}
		else if (0x05E1 == ucsptr[0])	// HEBREW LETTER SAMEKH
		{
			mbsptr[0] = 0xF1;
		}
		else if (0x05E2 == ucsptr[0])	// HEBREW LETTER AYIN
		{
			mbsptr[0] = 0xF2;
		}
		else if (0x05E3 == ucsptr[0])	// HEBREW LETTER FINAL PE
		{
			mbsptr[0] = 0xF3;
		}
		else if (0x05E4 == ucsptr[0])	// HEBREW LETTER PE
		{
			mbsptr[0] = 0xF4;
		}
		else if (0x05E5 == ucsptr[0])	// HEBREW LETTER FINAL TSADI
		{
			mbsptr[0] = 0xF5;
		}
		else if (0x05E6 == ucsptr[0])	// HEBREW LETTER TSADI
		{
			mbsptr[0] = 0xF6;
		}
		else if (0x05E7 == ucsptr[0])	// HEBREW LETTER QOF
		{
			mbsptr[0] = 0xF7;
		}
		else if (0x05E8 == ucsptr[0])	// HEBREW LETTER RESH
		{
			mbsptr[0] = 0xF8;
		}
		else if (0x05E9 == ucsptr[0])	// HEBREW LETTER SHIN
		{
			mbsptr[0] = 0xF9;
		}
		else if (0x05EA == ucsptr[0])	// HEBREW LETTER TAV
		{
			mbsptr[0] = 0xFA;
		}
		else if (0x200E == ucsptr[0])	// LEFT-TO-RIGHT MARK
		{
			mbsptr[0] = 0xFD;
		}
		else if (0x200F == ucsptr[0])	// RIGHT-TO-LEFT MARK
		{
			mbsptr[0] = 0xFE;
		}
		else
		{
			// undefined character - substitute a period
			mbsptr[0] = '.';
		}
	}

	return numChars;
}

////////////////////////////////////////
//
// Convert wide char to code page 1256
//
////////////////////////////////////////

int WCSto1256(const wchar_t * ucsptr, unsigned char * mbsptr)

{
	int					numChars=1;
	unsigned char *		ptr=(unsigned char *)ucsptr;

	// initialize to an undefined character (period)
	mbsptr[0] = '.';

	// is this just an ASCII character?
	if (ucsptr[0] < 128)
	{
		// just copy the ASCII character
		mbsptr[0] = ptr[0];
	}
	else
	{
		// check for other characters that can be displayed as is
		if (0x20AC == ucsptr[0])		// EURO SIGN
		{
			mbsptr[0] = 0x80;
		}
		if (0x067E == ucsptr[0])		// ARABIC LETTER PEH
		{
			mbsptr[0] = 0x81;
		}
		else if (0x201A == ucsptr[0])	// SINGLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x82;
		}
		else if (0x0192 == ucsptr[0])	// LATIN SMALL LETTER F WITH HOOK
		{
			mbsptr[0] = 0x83;
		}
		else if (0x201E == ucsptr[0])	// DOUBLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x84;
		}
		else if (0x2026 == ucsptr[0])	// HORIZONTAL ELLIPSIS
		{
			mbsptr[0] = 0x85;
		}
		else if (0x2020 == ucsptr[0])	// DAGGER
		{
			mbsptr[0] = 0x86;
		}
		else if (0x2021 == ucsptr[0])	// DOUBLE DAGGER
		{
			mbsptr[0] = 0x87;
		}
		else if (0x02C6 == ucsptr[0])	// MODIFIER LETTER CIRCUMFLEX ACCENT
		{
			mbsptr[0] = 0x88;
		}
		else if (0x2030 == ucsptr[0])	// PER MILLE SIGN
		{
			mbsptr[0] = 0x89;
		}
		else if (0x0679 == ucsptr[0])	// ARABIC LETTER TTEH
		{
			mbsptr[0] = 0x8A;
		}
		else if (0x2039 == ucsptr[0])	// SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x8B;
		}
		else if (0x0152 == ucsptr[0])	// LATIN CAPITAL LIGATURE OE
		{
			mbsptr[0] = 0x8C;
		}
		else if (0x0686 == ucsptr[0])	// ARABIC LETTER TCHEH
		{
			mbsptr[0] = 0x8D;
		}
		else if (0x0698 == ucsptr[0])	// ARABIC LETTER JEH
		{
			mbsptr[0] = 0x8E;
		}
		else if (0x0688 == ucsptr[0])	// ARABIC LETTER DDAL
		{
			mbsptr[0] = 0x8F;
		}
		else if (0x06AF == ucsptr[0])	// ARABIC LETTER GAF
		{
			mbsptr[0] = 0x90;
		}
		else if (0x2018 == ucsptr[0])	// LEFT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x91;
		}
		else if (0x2019 == ucsptr[0])	// RIGHT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x92;
		}
		else if (0x201C == ucsptr[0])	// LEFT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x93;
		}
		else if (0x201D == ucsptr[0])	// RIGHT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x94;
		}
		else if (0x2022 == ucsptr[0])	// BULLET
		{
			mbsptr[0] = 0x95;
		}
		else if (0x2013 == ucsptr[0])	// EN DASH
		{
			mbsptr[0] = 0x96;
		}
		else if (0x2014 == ucsptr[0])	// EM DASH
		{
			mbsptr[0] = 0x97;
		}
		else if (0x06A9 == ucsptr[0])	// ARABIC LETTER KEHEH
		{
			mbsptr[0] = 0x98;
		}
		else if (0x2122 == ucsptr[0])	// TRADE MARK SIGN
		{
			mbsptr[0] = 0x99;
		}
		else if (0x0691 == ucsptr[0])	// ARABIC LETTER RREH
		{
			mbsptr[0] = 0x9A;
		}
		else if (0x203A == ucsptr[0])	// SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x9B;
		}
		else if (0x0153 == ucsptr[0])	// LATIN SMALL LIGATURE OE
		{
			mbsptr[0] = 0x9C;
		}
		else if (0x200C == ucsptr[0])	// ZERO WIDTH NON-JOINER
		{
			mbsptr[0] = 0x9D;
		}
		else if (0x200D == ucsptr[0])	// ZERO WIDTH JOINER
		{
			mbsptr[0] = 0x9E;
		}
		else if (0x06BA == ucsptr[0])	// ARABIC LETTER NOON GHUNNA
		{
			mbsptr[0] = 0x9F;
		}
		else if (0x00A0 == ucsptr[0])	// NO-BREAK SPACE
		{
			mbsptr[0] = 0xA0;
		}
		else if (0x060C == ucsptr[0])	// ARABIC COMMA
		{
			mbsptr[0] = 0xA1;
		}
		else if (0x00A2 == ucsptr[0])	// CENT SIGN
		{
			mbsptr[0] = 0xA2;
		}
		else if (0x00A3 == ucsptr[0])	// POUND SIGN
		{
			mbsptr[0] = 0xA3;
		}
		else if (0x00A4 == ucsptr[0])	// CURRENCY SIGN
		{
			mbsptr[0] = 0xA4;
		}
		else if (0x00A5 == ucsptr[0])	// YEN SIGN
		{
			mbsptr[0] = 0xA5;
		}
		else if (0x00A6 == ucsptr[0])	// BROKEN BAR
		{
			mbsptr[0] = 0xA6;
		}
		else if (0x00A7 == ucsptr[0])	// SECTION SIGN
		{
			mbsptr[0] = 0xA7;
		}
		else if (0x00A8 == ucsptr[0])	// DIAERESIS
		{
			mbsptr[0] = 0xA8;
		}
		else if (0x00A9 == ucsptr[0])	// COPYRIGHT SIGN
		{
			mbsptr[0] = 0xA9;
		}
		else if (0x06BE == ucsptr[0])	// ARABIC LETTER HEH DOACHASHMEE
		{
			mbsptr[0] = 0xAA;
		}
		else if (0x00AB == ucsptr[0])	// LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xAB;
		}
		else if (0x00AC == ucsptr[0])	// NOT SIGN
		{
			mbsptr[0] = 0xAC;
		}
		else if (0x00AD == ucsptr[0])	// SOFT HYPHEN
		{
			mbsptr[0] = 0xAD;
		}
		else if (0x00AE == ucsptr[0])	// REGISTERED SIGN
		{
			mbsptr[0] = 0xAE;
		}
		else if (0x00AF == ucsptr[0])	// MACRON
		{
			mbsptr[0] = 0xAF;
		}
		else if (0x00B0 == ucsptr[0])	// DEGREE SIGN
		{
			mbsptr[0] = 0xB0;
		}
		else if (0x00B1 == ucsptr[0])	// PLUS-MINUS SIGN
		{
			mbsptr[0] = 0xB1;
		}
		else if (0x00B2 == ucsptr[0])	// SUPERSCRIPT TWO
		{
			mbsptr[0] = 0xB2;
		}
		else if (0x00B3 == ucsptr[0])	// SUPERSCRIPT THREE
		{
			mbsptr[0] = 0xB3;
		}
		else if (0x00B4 == ucsptr[0])	// ACUTE ACCENT
		{
			mbsptr[0] = 0xB4;
		}
		else if (0x00B5 == ucsptr[0])	// MICRO SIGN
		{
			mbsptr[0] = 0xB5;
		}
		else if (0x00B6 == ucsptr[0])	// PILCROW SIGN
		{
			mbsptr[0] = 0xB6;
		}
		else if (0x00B7 == ucsptr[0])	// MIDDLE DOT
		{
			mbsptr[0] = 0xB7;
		}
		else if (0x00B8 == ucsptr[0])	// CEDILLA
		{
			mbsptr[0] = 0xB8;
		}
		else if (0x00B9 == ucsptr[0])	// SUPERSCRIPT ONE
		{
			mbsptr[0] = 0xB9;
		}
		else if (0x061B == ucsptr[0])	// ARABIC SEMICOLON
		{
			mbsptr[0] = 0xBA;
		}
		else if (0x00BB == ucsptr[0])	// RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xBB;
		}
		else if (0x00BC == ucsptr[0])	// VULGAR FRACTION ONE QUARTER
		{
			mbsptr[0] = 0xBC;
		}
		else if (0x00BD == ucsptr[0])	// VULGAR FRACTION ONE HALF
		{
			mbsptr[0] = 0xBD;
		}
		else if (0x00BE == ucsptr[0])	// VULGAR FRACTION THREE QUARTERS
		{
			mbsptr[0] = 0xBE;
		}
		else if (0x061F == ucsptr[0])	// ARABIC QUESTION MARK
		{
			mbsptr[0] = 0xBF;
		}
		else if (0x06C1 == ucsptr[0])	// ARABIC LETTER HEH GOAL
		{
			mbsptr[0] = 0xC0;
		}
		else if (0x0621 == ucsptr[0])	// ARABIC LETTER HAMZA
		{
			mbsptr[0] = 0xC1;
		}
		else if (0x0622 == ucsptr[0])	// ARABIC LETTER ALEF WITH MADDA ABOVE
		{
			mbsptr[0] = 0xC2;
		}
		else if (0x0623 == ucsptr[0])	// ARABIC LETTER ALEF WITH HAMZA ABOVE
		{
			mbsptr[0] = 0xC3;
		}
		else if (0x0624 == ucsptr[0])	// ARABIC LETTER WAW WITH HAMZA ABOVE
		{
			mbsptr[0] = 0xC4;
		}
		else if (0x0625 == ucsptr[0])	// ARABIC LETTER ALEF WITH HAMZA BELOW
		{
			mbsptr[0] = 0xC5;
		}
		else if (0x0626 == ucsptr[0])	// ARABIC LETTER YEH WITH HAMZA ABOVE
		{
			mbsptr[0] = 0xC6;
		}
		else if (0x0627 == ucsptr[0])	// ARABIC LETTER ALEF
		{
			mbsptr[0] = 0xC7;
		}
		else if (0x0628 == ucsptr[0])	// ARABIC LETTER BEH
		{
			mbsptr[0] = 0xC8;
		}
		else if (0x0629 == ucsptr[0])	// ARABIC LETTER TEH MARBUTA
		{
			mbsptr[0] = 0xC9;
		}
		else if (0x062A == ucsptr[0])	// ARABIC LETTER TEH
		{
			mbsptr[0] = 0xCA;
		}
		else if (0x062B == ucsptr[0])	// ARABIC LETTER THEH
		{
			mbsptr[0] = 0xCB;
		}
		else if (0x062C == ucsptr[0])	// ARABIC LETTER JEEM
		{
			mbsptr[0] = 0xCC;
		}
		else if (0x062D == ucsptr[0])	// ARABIC LETTER HAH
		{
			mbsptr[0] = 0xCD;
		}
		else if (0x062E == ucsptr[0])	// ARABIC LETTER KHAH
		{
			mbsptr[0] = 0xCE;
		}
		else if (0x062F == ucsptr[0])	// ARABIC LETTER DAL
		{
			mbsptr[0] = 0xCF;
		}
		else if (0x0630 == ucsptr[0])	// ARABIC LETTER THAL
		{
			mbsptr[0] = 0xD0;
		}
		else if (0x0631 == ucsptr[0])	// ARABIC LETTER REH
		{
			mbsptr[0] = 0xD1;
		}
		else if (0x0632 == ucsptr[0])	// ARABIC LETTER ZAIN
		{
			mbsptr[0] = 0xD2;
		}
		else if (0x0633 == ucsptr[0])	// ARABIC LETTER SEEN
		{
			mbsptr[0] = 0xD3;
		}
		else if (0x0634 == ucsptr[0])	// ARABIC LETTER SHEEN
		{
			mbsptr[0] = 0xD4;
		}
		else if (0x0635 == ucsptr[0])	// ARABIC LETTER SAD
		{
			mbsptr[0] = 0xD5;
		}
		else if (0x0636 == ucsptr[0])	// ARABIC LETTER DAD
		{
			mbsptr[0] = 0xD6;
		}
		else if (0x00D7 == ucsptr[0])	// MULTIPLICATION SIGN
		{
			mbsptr[0] = 0xD7;
		}
		else if (0x0637 == ucsptr[0])	// ARABIC LETTER TAH
		{
			mbsptr[0] = 0xD8;
		}
		else if (0x0638 == ucsptr[0])	// ARABIC LETTER ZAH
		{
			mbsptr[0] = 0xD9;
		}
		else if (0x0639 == ucsptr[0])	// ARABIC LETTER AIN
		{
			mbsptr[0] = 0xDA;
		}
		else if (0x063A == ucsptr[0])	// ARABIC LETTER GHAIN
		{
			mbsptr[0] = 0xDB;
		}
		else if (0x0640 == ucsptr[0])	// ARABIC TATWEEL
		{
			mbsptr[0] = 0xDC;
		}
		else if (0x0641 == ucsptr[0])	// ARABIC LETTER FEH
		{
			mbsptr[0] = 0xDD;
		}
		else if (0x0642 == ucsptr[0])	// ARABIC LETTER QAF
		{
			mbsptr[0] = 0xDE;
		}
		else if (0x0643 == ucsptr[0])	// ARABIC LETTER KAF
		{
			mbsptr[0] = 0xDF;
		}
		else if (0x00E0 == ucsptr[0])	// LATIN SMALL LETTER A WITH GRAVE
		{
			mbsptr[0] = 0xE0;
		}
		else if (0x0644 == ucsptr[0])	// ARABIC LETTER LAM
		{
			mbsptr[0] = 0xE1;
		}
		else if (0x00E2 == ucsptr[0])	// LATIN SMALL LETTER A WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xE2;
		}
		else if (0x0645 == ucsptr[0])	// ARABIC LETTER MEEM
		{
			mbsptr[0] = 0xE3;
		}
		else if (0x0646 == ucsptr[0])	// ARABIC LETTER NOON
		{
			mbsptr[0] = 0xE4;
		}
		else if (0x0647 == ucsptr[0])	// ARABIC LETTER HEH
		{
			mbsptr[0] = 0xE5;
		}
		else if (0x0648 == ucsptr[0])	// ARABIC LETTER WAW
		{
			mbsptr[0] = 0xE6;
		}
		else if (0x00E7 == ucsptr[0])	// LATIN SMALL LETTER C WITH CEDILLA
		{
			mbsptr[0] = 0xE7;
		}
		else if (0x00E8 == ucsptr[0])	// LATIN SMALL LETTER E WITH GRAVE
		{
			mbsptr[0] = 0xE8;
		}
		else if (0x00E9 == ucsptr[0])	// LATIN SMALL LETTER E WITH ACUTE
		{
			mbsptr[0] = 0xE9;
		}
		else if (0x00EA == ucsptr[0])	// LATIN SMALL LETTER E WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xEA;
		}
		else if (0x00EB == ucsptr[0])	// LATIN SMALL LETTER E WITH DIAERESIS
		{
			mbsptr[0] = 0xEB;
		}
		else if (0x0649 == ucsptr[0])	// ARABIC LETTER ALEF MAKSURA
		{
			mbsptr[0] = 0xEC;
		}
		else if (0x064A == ucsptr[0])	// ARABIC LETTER YEH
		{
			mbsptr[0] = 0xED;
		}
		else if (0x00EE == ucsptr[0])	// LATIN SMALL LETTER I WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xEE;
		}
		else if (0x00EF == ucsptr[0])	// LATIN SMALL LETTER I WITH DIAERESIS
		{
			mbsptr[0] = 0xEF;
		}
		else if (0x064B == ucsptr[0])	// ARABIC FATHATAN
		{
			mbsptr[0] = 0xF0;
		}
		else if (0x064C == ucsptr[0])	// ARABIC DAMMATAN
		{
			mbsptr[0] = 0xF1;
		}
		else if (0x064D == ucsptr[0])	// ARABIC KASRATAN
		{
			mbsptr[0] = 0xF2;
		}
		else if (0x064E == ucsptr[0])	// ARABIC FATHA
		{
			mbsptr[0] = 0xF3;
		}
		else if (0x00F4 == ucsptr[0])	// LATIN SMALL LETTER O WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xF4;
		}
		else if (0x064F == ucsptr[0])	// ARABIC DAMMA
		{
			mbsptr[0] = 0xF5;
		}
		else if (0x0650 == ucsptr[0])	// ARABIC KASRA
		{
			mbsptr[0] = 0xF6;
		}
		else if (0x00F7 == ucsptr[0])	// DIVISION SIGN
		{
			mbsptr[0] = 0xF7;
		}
		else if (0x0651 == ucsptr[0])	// ARABIC SHADDA
		{
			mbsptr[0] = 0xF8;
		}
		else if (0x00F9 == ucsptr[0])	// LATIN SMALL LETTER U WITH GRAVE
		{
			mbsptr[0] = 0xF9;
		}
		
		if (0x0652 == ucsptr[0])	// ARABIC SUKUN
		{
			mbsptr[0] = 0xFA;
		}
		else if (0x00FB == ucsptr[0])	// LATIN SMALL LETTER U WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xFB;
		}
		else if (0x00FC == ucsptr[0])	// LATIN SMALL LETTER U WITH DIAERESIS
		{
			mbsptr[0] = 0xFC;
		}
		else if (0x200E == ucsptr[0])	// LEFT-TO-RIGHT MARK
		{
			mbsptr[0] = 0xFD;
		}
		else if (0x200F == ucsptr[0])	// RIGHT-TO-LEFT MARK
		{
			mbsptr[0] = 0xFE;
		}
		else if (0x06D2 == ucsptr[0])	// ARABIC LETTER YEH BARREE
		{
			mbsptr[0] = 0xFF;
		}
	}

	return numChars;
}

////////////////////////////////////////
//
// Convert wide char to code page 1257
//
////////////////////////////////////////

int WCSto1257(const wchar_t * ucsptr, unsigned char * mbsptr)

{
	int					numChars=1;
	unsigned char *		ptr=(unsigned char *)ucsptr;

	// initialize to an undefined character (period)
	mbsptr[0] = '.';

	// is this just an ASCII character?
	if (ucsptr[0] < 128)
	{
		// just copy the ASCII character
		mbsptr[0] = ptr[0];
	}
	else
	{
		// check for other characters that can be displayed as is
		if (0x20AC == ucsptr[0])		// EURO SIGN
		{
			mbsptr[0] = 0x80;
		}
		else if (0x201A == ucsptr[0])	// SINGLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x82;
		}
		else if (0x201E == ucsptr[0])	// DOUBLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x84;
		}
		else if (0x2026 == ucsptr[0])	// HORIZONTAL ELLIPSIS
		{
			mbsptr[0] = 0x85;
		}
		else if (0x2020 == ucsptr[0])	// DAGGER
		{
			mbsptr[0] = 0x86;
		}
		else if (0x2021 == ucsptr[0])	// DOUBLE DAGGER
		{
			mbsptr[0] = 0x87;
		}
		else if (0x2030 == ucsptr[0])	// PER MILLE SIGN
		{
			mbsptr[0] = 0x89;
		}
		else if (0x2039 == ucsptr[0])	// SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x8B;
		}
		else if (0x00A8 == ucsptr[0])	// DIAERESIS
		{
			mbsptr[0] = 0x8D;
		}
		else if (0x02C7 == ucsptr[0])	// CARON
		{
			mbsptr[0] = 0x8E;
		}
		else if (0x00B8 == ucsptr[0])	// CEDILLA
		{
			mbsptr[0] = 0x8F;
		}
		else if (0x2018 == ucsptr[0])	// LEFT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x91;
		}
		else if (0x2019 == ucsptr[0])	// RIGHT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x92;
		}
		else if (0x201C == ucsptr[0])	// LEFT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x93;
		}
		else if (0x201D == ucsptr[0])	// RIGHT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x94;
		}
		else if (0x2022 == ucsptr[0])	// BULLET
		{
			mbsptr[0] = 0x95;
		}
		else if (0x2013 == ucsptr[0])	// EN DASH
		{
			mbsptr[0] = 0x96;
		}
		else if (0x2014 == ucsptr[0])	// EM DASH
		{
			mbsptr[0] = 0x97;
		}
		else if (0x2122 == ucsptr[0])	// TRADE MARK SIGN
		{
			mbsptr[0] = 0x99;
		}
		else if (0x203A == ucsptr[0])	// SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x9B;
		}
		else if (0x00AF == ucsptr[0])	// MACRON
		{
			mbsptr[0] = 0x9D;
		}
		else if (0x02DB == ucsptr[0])	// OGONEK
		{
			mbsptr[0] = 0x9E;
		}
		else if (0x00A0 == ucsptr[0])	// NO-BREAK SPACE
		{
			mbsptr[0] = 0xA0;
		}
		else if (0x00A2 == ucsptr[0])	// CENT SIGN
		{
			mbsptr[0] = 0xA2;
		}
		else if (0x00A3 == ucsptr[0])	// POUND SIGN
		{
			mbsptr[0] = 0xA3;
		}
		else if (0x00A4 == ucsptr[0])	// CURRENCY SIGN
		{
			mbsptr[0] = 0xA4;
		}
		else if (0x00A6 == ucsptr[0])	// BROKEN BAR
		{
			mbsptr[0] = 0xA6;
		}
		else if (0x00A7 == ucsptr[0])	// SECTION SIGN
		{
			mbsptr[0] = 0xA7;
		}
		else if (0x00D8 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH STROKE
		{
			mbsptr[0] = 0xA8;
		}
		else if (0x00A9 == ucsptr[0])	// COPYRIGHT SIGN
		{
			mbsptr[0] = 0xA9;
		}
		else if (0x0156 == ucsptr[0])	// LATIN CAPITAL LETTER R WITH CEDILLA
		{
			mbsptr[0] = 0xAA;
		}
		else if (0x00AB == ucsptr[0])	// LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xAB;
		}
		else if (0x00AC == ucsptr[0])	// NOT SIGN
		{
			mbsptr[0] = 0xAC;
		}
		else if (0x00AD == ucsptr[0])	// SOFT HYPHEN
		{
			mbsptr[0] = 0xAD;
		}
		else if (0x00AE == ucsptr[0])	// REGISTERED SIGN
		{
			mbsptr[0] = 0xAE;
		}
		else if (0x00C6 == ucsptr[0])	// LATIN CAPITAL LETTER AE
		{
			mbsptr[0] = 0xAF;
		}
		else if (0x00B0 == ucsptr[0])	// DEGREE SIGN
		{
			mbsptr[0] = 0xB0;
		}
		else if (0x00B1 == ucsptr[0])	// PLUS-MINUS SIGN
		{
			mbsptr[0] = 0xB1;
		}
		else if (0x00B2 == ucsptr[0])	// SUPERSCRIPT TWO
		{
			mbsptr[0] = 0xB2;
		}
		else if (0x00B3 == ucsptr[0])	// SUPERSCRIPT THREE
		{
			mbsptr[0] = 0xB3;
		}
		else if (0x00B4 == ucsptr[0])	// ACUTE ACCENT
		{
			mbsptr[0] = 0xB4;
		}
		else if (0x00B5 == ucsptr[0])	// MICRO SIGN
		{
			mbsptr[0] = 0xB5;
		}
		else if (0x00B6 == ucsptr[0])	// PILCROW SIGN
		{
			mbsptr[0] = 0xB6;
		}
		else if (0x00B7 == ucsptr[0])	// MIDDLE DOT
		{
			mbsptr[0] = 0xB7;
		}
		else if (0x00F8 == ucsptr[0])	// LATIN SMALL LETTER O WITH STROKE
		{
			mbsptr[0] = 0xB8;
		}
		else if (0x00B9 == ucsptr[0])	// SUPERSCRIPT ONE
		{
			mbsptr[0] = 0xB9;
		}
		else if (0x0157 == ucsptr[0])	// LATIN SMALL LETTER R WITH CEDILLA
		{
			mbsptr[0] = 0xBA;
		}
		else if (0x00BB == ucsptr[0])	// RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xBB;
		}
		else if (0x00BC == ucsptr[0])	// VULGAR FRACTION ONE QUARTER
		{
			mbsptr[0] = 0xBC;
		}
		else if (0x00BD == ucsptr[0])	// VULGAR FRACTION ONE HALF
		{
			mbsptr[0] = 0xBD;
		}
		else if (0x00BE == ucsptr[0])	// VULGAR FRACTION THREE QUARTERS
		{
			mbsptr[0] = 0xBE;
		}
		else if (0x00E6 == ucsptr[0])	// LATIN SMALL LETTER AE
		{
			mbsptr[0] = 0xBF;
		}
		else if (0x0104 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH OGONEK
		{
			mbsptr[0] = 0xC0;
		}
		else if (0x012E == ucsptr[0])	// LATIN CAPITAL LETTER I WITH OGONEK
		{
			mbsptr[0] = 0xC1;
		}
		else if (0x0100 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH MACRON
		{
			mbsptr[0] = 0xC2;
		}
		else if (0x0106 == ucsptr[0])	// LATIN CAPITAL LETTER C WITH ACUTE
		{
			mbsptr[0] = 0xC3;
		}
		else if (0x00C4 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH DIAERESIS
		{
			mbsptr[0] = 0xC4;
		}
		else if (0x00C5 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH RING ABOVE
		{
			mbsptr[0] = 0xC5;
		}
		else if (0x0118 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH OGONEK
		{
			mbsptr[0] = 0xC6;
		}
		else if (0x0112 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH MACRON
		{
			mbsptr[0] = 0xC7;
		}
		else if (0x010C == ucsptr[0])	// LATIN CAPITAL LETTER C WITH CARON
		{
			mbsptr[0] = 0xC8;
		}
		else if (0x00C9 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH ACUTE
		{
			mbsptr[0] = 0xC9;
		}
		else if (0x0179 == ucsptr[0])	// LATIN CAPITAL LETTER Z WITH ACUTE
		{
			mbsptr[0] = 0xCA;
		}
		else if (0x0116 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH DOT ABOVE
		{
			mbsptr[0] = 0xCB;
		}
		else if (0x0122 == ucsptr[0])	// LATIN CAPITAL LETTER G WITH CEDILLA
		{
			mbsptr[0] = 0xCC;
		}
		else if (0x0136 == ucsptr[0])	// LATIN CAPITAL LETTER K WITH CEDILLA
		{
			mbsptr[0] = 0xCD;
		}
		else if (0x012A == ucsptr[0])	// LATIN CAPITAL LETTER I WITH MACRON
		{
			mbsptr[0] = 0xCE;
		}
		else if (0x013B == ucsptr[0])	// LATIN CAPITAL LETTER L WITH CEDILLA
		{
			mbsptr[0] = 0xCF;
		}
		else if (0x0160 == ucsptr[0])	// LATIN CAPITAL LETTER S WITH CARON
		{
			mbsptr[0] = 0xD0;
		}
		else if (0x0143 == ucsptr[0])	// LATIN CAPITAL LETTER N WITH ACUTE
		{
			mbsptr[0] = 0xD1;
		}
		else if (0x0145 == ucsptr[0])	// LATIN CAPITAL LETTER N WITH CEDILLA
		{
			mbsptr[0] = 0xD2;
		}
		else if (0x00D3 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH ACUTE
		{
			mbsptr[0] = 0xD3;
		}
		else if (0x014C == ucsptr[0])	// LATIN CAPITAL LETTER O WITH MACRON
		{
			mbsptr[0] = 0xD4;
		}
		else if (0x00D5 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH TILDE
		{
			mbsptr[0] = 0xD5;
		}
		else if (0x00D6 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH DIAERESIS
		{
			mbsptr[0] = 0xD6;
		}
		else if (0x00D7 == ucsptr[0])	// MULTIPLICATION SIGN
		{
			mbsptr[0] = 0xD7;
		}
		else if (0x0172 == ucsptr[0])	// LATIN CAPITAL LETTER U WITH OGONEK
		{
			mbsptr[0] = 0xD8;
		}
		else if (0x0141 == ucsptr[0])	// LATIN CAPITAL LETTER L WITH STROKE
		{
			mbsptr[0] = 0xD9;
		}
		else if (0x015A == ucsptr[0])	// LATIN CAPITAL LETTER S WITH ACUTE
		{
			mbsptr[0] = 0xDA;
		}
		else if (0x016A == ucsptr[0])	// LATIN CAPITAL LETTER U WITH MACRON
		{
			mbsptr[0] = 0xDB;
		}
		else if (0x00DC == ucsptr[0])	// LATIN CAPITAL LETTER U WITH DIAERESIS
		{
			mbsptr[0] = 0xDC;
		}
		else if (0x017B == ucsptr[0])	// LATIN CAPITAL LETTER Z WITH DOT ABOVE
		{
			mbsptr[0] = 0xDD;
		}
		else if (0x017D == ucsptr[0])	// LATIN CAPITAL LETTER Z WITH CARON
		{
			mbsptr[0] = 0xDE;
		}
		else if (0x00DF == ucsptr[0])	// LATIN SMALL LETTER SHARP S
		{
			mbsptr[0] = 0xDF;
		}
		else if (0x0105 == ucsptr[0])	// LATIN SMALL LETTER A WITH OGONEK
		{
			mbsptr[0] = 0xE0;
		}
		else if (0x012F == ucsptr[0])	// LATIN SMALL LETTER I WITH OGONEK
		{
			mbsptr[0] = 0xE1;
		}
		else if (0x0101 == ucsptr[0])	// LATIN SMALL LETTER A WITH MACRON
		{
			mbsptr[0] = 0xE2;
		}
		else if (0x0107 == ucsptr[0])	// LATIN SMALL LETTER C WITH ACUTE
		{
			mbsptr[0] = 0xE3;
		}
		else if (0x00E4 == ucsptr[0])	// LATIN SMALL LETTER A WITH DIAERESIS
		{
			mbsptr[0] = 0xE4;
		}
		else if (0x00E5 == ucsptr[0])	// LATIN SMALL LETTER A WITH RING ABOVE
		{
			mbsptr[0] = 0xE5;
		}
		else if (0x0119 == ucsptr[0])	// LATIN SMALL LETTER E WITH OGONEK
		{
			mbsptr[0] = 0xE6;
		}
		else if (0x0113 == ucsptr[0])	// LATIN SMALL LETTER E WITH MACRON
		{
			mbsptr[0] = 0xE7;
		}
		else if (0x010D == ucsptr[0])	// LATIN SMALL LETTER C WITH CARON
		{
			mbsptr[0] = 0xE8;
		}
		else if (0x00E9 == ucsptr[0])	// LATIN SMALL LETTER E WITH ACUTE
		{
			mbsptr[0] = 0xE9;
		}
		else if (0x017A == ucsptr[0])	// LATIN SMALL LETTER Z WITH ACUTE
		{
			mbsptr[0] = 0xEA;
		}
		else if (0x0117 == ucsptr[0])	// LATIN SMALL LETTER E WITH DOT ABOVE
		{
			mbsptr[0] = 0xEB;
		}
		else if (0x0123 == ucsptr[0])	// LATIN SMALL LETTER G WITH CEDILLA
		{
			mbsptr[0] = 0xEC;
		}
		else if (0x0137 == ucsptr[0])	// LATIN SMALL LETTER K WITH CEDILLA
		{
			mbsptr[0] = 0xED;
		}
		else if (0x012B == ucsptr[0])	// LATIN SMALL LETTER I WITH MACRON
		{
			mbsptr[0] = 0xEE;
		}
		else if (0x013C == ucsptr[0])	// LATIN SMALL LETTER L WITH CEDILLA
		{
			mbsptr[0] = 0xEF;
		}
		else if (0x0161 == ucsptr[0])	// LATIN SMALL LETTER S WITH CARON
		{
			mbsptr[0] = 0xF0;
		}
		else if (0x0144 == ucsptr[0])	// LATIN SMALL LETTER N WITH ACUTE
		{
			mbsptr[0] = 0xF1;
		}
		else if (0x0146 == ucsptr[0])	// LATIN SMALL LETTER N WITH CEDILLA
		{
			mbsptr[0] = 0xF2;
		}
		else if (0x00F3 == ucsptr[0])	// LATIN SMALL LETTER O WITH ACUTE
		{
			mbsptr[0] = 0xF3;
		}
		else if (0x014D == ucsptr[0])	// LATIN SMALL LETTER O WITH MACRON
		{
			mbsptr[0] = 0xF4;
		}
		else if (0x00F5 == ucsptr[0])	// LATIN SMALL LETTER O WITH TILDE
		{
			mbsptr[0] = 0xF5;
		}
		else if (0x00F6 == ucsptr[0])	// LATIN SMALL LETTER O WITH DIAERESIS
		{
			mbsptr[0] = 0xF6;
		}
		else if (0x00F7 == ucsptr[0])	// DIVISION SIGN
		{
			mbsptr[0] = 0xF7;
		}
		else if (0x0173 == ucsptr[0])	// LATIN SMALL LETTER U WITH OGONEK
		{
			mbsptr[0] = 0xF8;
		}
		else if (0x0142 == ucsptr[0])	// LATIN SMALL LETTER L WITH STROKE
		{
			mbsptr[0] = 0xF9;
		}
		else if (0x015B == ucsptr[0])	// LATIN SMALL LETTER S WITH ACUTE
		{
			mbsptr[0] = 0xFA;
		}
		else if (0x016B == ucsptr[0])	// LATIN SMALL LETTER U WITH MACRON
		{
			mbsptr[0] = 0xFB;
		}
		else if (0x00FC == ucsptr[0])	// LATIN SMALL LETTER U WITH DIAERESIS
		{
			mbsptr[0] = 0xFC;
		}
		else if (0x017C == ucsptr[0])	// LATIN SMALL LETTER Z WITH DOT ABOVE
		{
			mbsptr[0] = 0xFD;
		}
		else if (0x017E == ucsptr[0])	// LATIN SMALL LETTER Z WITH CARON
		{
			mbsptr[0] = 0xFE;
		}
		else if (0x02D9 == ucsptr[0])	// DOT ABOVE
		{
			mbsptr[0] = 0xFF;
		}
	}

	return numChars;
}

////////////////////////////////////////
//
// Convert wide char to code page 1258
//
////////////////////////////////////////

int WCSto1258(const wchar_t * ucsptr, unsigned char * mbsptr)

{
	int					numChars=1;
	unsigned char *		ptr=(unsigned char *)ucsptr;

	// initialize to an undefined character (period)
	mbsptr[0] = '.';

	// is this just an ASCII character?
	if (ucsptr[0] < 128)
	{
		// just copy the ASCII character
		mbsptr[0] = ptr[0];
	}
	else
	{
		// check for other characters that can be displayed as is
		if (0x20AC == ucsptr[0])		// EURO SIGN
		{
			mbsptr[0] = 0x80;
		}
		else if (0x201A == ucsptr[0])	// SINGLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x82;
		}
		else if (0x0192 == ucsptr[0])	// LATIN SMALL LETTER F WITH HOOK
		{
			mbsptr[0] = 0x83;
		}
		else if (0x201E == ucsptr[0])	// DOUBLE LOW-9 QUOTATION MARK
		{
			mbsptr[0] = 0x84;
		}
		else if (0x2026 == ucsptr[0])	// HORIZONTAL ELLIPSIS
		{
			mbsptr[0] = 0x85;
		}
		else if (0x2020 == ucsptr[0])	// DAGGER
		{
			mbsptr[0] = 0x86;
		}
		else if (0x2021 == ucsptr[0])	// DOUBLE DAGGER
		{
			mbsptr[0] = 0x87;
		}
		else if (0x02C6 == ucsptr[0])	// MODIFIER LETTER CIRCUMFLEX ACCENT
		{
			mbsptr[0] = 0x88;
		}
		else if (0x2030 == ucsptr[0])	// PER MILLE SIGN
		{
			mbsptr[0] = 0x89;
		}
		else if (0x2039 == ucsptr[0])	// SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x8B;
		}
		else if (0x0152 == ucsptr[0])	// LATIN CAPITAL LIGATURE OE
		{
			mbsptr[0] = 0x8C;
		}
		else if (0x2018 == ucsptr[0])	// LEFT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x91;
		}
		else if (0x2019 == ucsptr[0])	// RIGHT SINGLE QUOTATION MARK
		{
			mbsptr[0] = 0x92;
		}
		else if (0x201C == ucsptr[0])	// LEFT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x93;
		}
		else if (0x201D == ucsptr[0])	// RIGHT DOUBLE QUOTATION MARK
		{
			mbsptr[0] = 0x94;
		}
		else if (0x2022 == ucsptr[0])	// BULLET
		{
			mbsptr[0] = 0x95;
		}
		else if (0x2013 == ucsptr[0])	// EN DASH
		{
			mbsptr[0] = 0x96;
		}
		else if (0x2014 == ucsptr[0])	// EM DASH
		{
			mbsptr[0] = 0x97;
		}
		else if (0x02DC == ucsptr[0])	// SMALL TILDE
		{
			mbsptr[0] = 0x98;
		}
		else if (0x2122 == ucsptr[0])	// TRADE MARK SIGN
		{
			mbsptr[0] = 0x99;
		}
		else if (0x203A == ucsptr[0])	// SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0x9B;
		}
		else if (0x0153 == ucsptr[0])	// LATIN SMALL LIGATURE OE
		{
			mbsptr[0] = 0x9C;
		}
		else if (0x0178 == ucsptr[0])	// LATIN CAPITAL LETTER Y WITH DIAERESIS
		{
			mbsptr[0] = 0x9F;
		}
		else if (0x00A0 == ucsptr[0])	// NO-BREAK SPACE
		{
			mbsptr[0] = 0xA0;
		}
		else if (0x00A1 == ucsptr[0])	// INVERTED EXCLAMATION MARK
		{
			mbsptr[0] = 0xA1;
		}
		else if (0x00A2 == ucsptr[0])	// CENT SIGN
		{
			mbsptr[0] = 0xA2;
		}
		else if (0x00A3 == ucsptr[0])	// POUND SIGN
		{
			mbsptr[0] = 0xA3;
		}
		else if (0x00A4 == ucsptr[0])	// CURRENCY SIGN
		{
			mbsptr[0] = 0xA4;
		}
		else if (0x00A5 == ucsptr[0])	// YEN SIGN
		{
			mbsptr[0] = 0xA5;
		}
		else if (0x00A6 == ucsptr[0])	// BROKEN BAR
		{
			mbsptr[0] = 0xA6;
		}
		else if (0x00A7 == ucsptr[0])	// SECTION SIGN
		{
			mbsptr[0] = 0xA7;
		}
		else if (0x00A8 == ucsptr[0])	// DIAERESIS
		{
			mbsptr[0] = 0xA8;
		}
		else if (0x00A9 == ucsptr[0])	// COPYRIGHT SIGN
		{
			mbsptr[0] = 0xA9;
		}
		else if (0x00AA == ucsptr[0])	// FEMININE ORDINAL INDICATOR
		{
			mbsptr[0] = 0xAA;
		}
		else if (0x00AB == ucsptr[0])	// LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xAB;
		}
		else if (0x00AC == ucsptr[0])	// NOT SIGN
		{
			mbsptr[0] = 0xAC;
		}
		else if (0x00AD == ucsptr[0])	// SOFT HYPHEN
		{
			mbsptr[0] = 0xAD;
		}
		else if (0x00AE == ucsptr[0])	// REGISTERED SIGN
		{
			mbsptr[0] = 0xAE;
		}
		else if (0x00AF == ucsptr[0])	// MACRON
		{
			mbsptr[0] = 0xAF;
		}
		else if (0x00B0 == ucsptr[0])	// DEGREE SIGN
		{
			mbsptr[0] = 0xB0;
		}
		else if (0x00B1 == ucsptr[0])	// PLUS-MINUS SIGN
		{
			mbsptr[0] = 0xB1;
		}
		else if (0x00B2 == ucsptr[0])	// SUPERSCRIPT TWO
		{
			mbsptr[0] = 0xB2;
		}
		else if (0x00B3 == ucsptr[0])	// SUPERSCRIPT THREE
		{
			mbsptr[0] = 0xB3;
		}
		else if (0x00B4 == ucsptr[0])	// ACUTE ACCENT
		{
			mbsptr[0] = 0xB4;
		}
		else if (0x00B5 == ucsptr[0])	// MICRO SIGN
		{
			mbsptr[0] = 0xB5;
		}
		else if (0x00B6 == ucsptr[0])	// PILCROW SIGN
		{
			mbsptr[0] = 0xB6;
		}
		else if (0x00B7 == ucsptr[0])	// MIDDLE DOT
		{
			mbsptr[0] = 0xB7;
		}
		else if (0x00B8 == ucsptr[0])	// CEDILLA
		{
			mbsptr[0] = 0xB8;
		}
		else if (0x00B9 == ucsptr[0])	// SUPERSCRIPT ONE
		{
			mbsptr[0] = 0xB9;
		}
		else if (0x00BA == ucsptr[0])	// MASCULINE ORDINAL INDICATOR
		{
			mbsptr[0] = 0xBA;
		}
		else if (0x00BB == ucsptr[0])	// RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		{
			mbsptr[0] = 0xBB;
		}
		else if (0x00BC == ucsptr[0])	// VULGAR FRACTION ONE QUARTER
		{
			mbsptr[0] = 0xBC;
		}
		else if (0x00BD == ucsptr[0])	// VULGAR FRACTION ONE HALF
		{
			mbsptr[0] = 0xBD;
		}
		else if (0x00BE == ucsptr[0])	// VULGAR FRACTION THREE QUARTERS
		{
			mbsptr[0] = 0xBE;
		}
		else if (0x00BF == ucsptr[0])	// INVERTED QUESTION MARK
		{
			mbsptr[0] = 0xBF;
		}
		else if (0x00C0 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH GRAVE
		{
			mbsptr[0] = 0xC0;
		}
		else if (0x00C1 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH ACUTE
		{
			mbsptr[0] = 0xC1;
		}
		else if (0x00C2 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xC2;
		}
		else if (0x0102 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH BREVE
		{
			mbsptr[0] = 0xC3;
		}
		else if (0x00C4 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH DIAERESIS
		{
			mbsptr[0] = 0xC4;
		}
		else if (0x00C5 == ucsptr[0])	// LATIN CAPITAL LETTER A WITH RING ABOVE
		{
			mbsptr[0] = 0xC5;
		}
		else if (0x00C6 == ucsptr[0])	// LATIN CAPITAL LETTER AE
		{
			mbsptr[0] = 0xC6;
		}
		else if (0x00C7 == ucsptr[0])	// LATIN CAPITAL LETTER C WITH CEDILLA
		{
			mbsptr[0] = 0xC7;
		}
		else if (0x00C8 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH GRAVE
		{
			mbsptr[0] = 0xC8;
		}
		else if (0x00C9 == ucsptr[0])	// LATIN CAPITAL LETTER E WITH ACUTE
		{
			mbsptr[0] = 0xC9;
		}
		else if (0x00CA == ucsptr[0])	// LATIN CAPITAL LETTER E WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xCA;
		}
		else if (0x00CB == ucsptr[0])	// LATIN CAPITAL LETTER E WITH DOT ABOVE
		{
			mbsptr[0] = 0xCB;
		}
		else if (0x0300 == ucsptr[0])	// COMBINING GRAVE ACCENT
		{
			mbsptr[0] = 0xCC;
		}
		else if (0x00CD == ucsptr[0])	// LATIN CAPITAL LETTER I WITH ACUTE
		{
			mbsptr[0] = 0xCD;
		}
		else if (0x00CE == ucsptr[0])	// LATIN CAPITAL LETTER I WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xCE;
		}
		else if (0x00CF == ucsptr[0])	// LATIN CAPITAL LETTER I WITH DIAERESIS
		{
			mbsptr[0] = 0xCF;
		}
		else if (0x0110 == ucsptr[0])	// LATIN CAPITAL LETTER D WITH STROKE
		{
			mbsptr[0] = 0xD0;
		}
		else if (0x00D1 == ucsptr[0])	// LATIN CAPITAL LETTER N WITH TILDE
		{
			mbsptr[0] = 0xD1;
		}
		else if (0x0309 == ucsptr[0])	// COMBINING HOOK ABOVE
		{
			mbsptr[0] = 0xD2;
		}
		else if (0x00D3 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH ACUTE
		{
			mbsptr[0] = 0xD3;
		}
		else if (0x00D4 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xD4;
		}
		else if (0x01A0 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH HORN
		{
			mbsptr[0] = 0xD5;
		}
		else if (0x00D6 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH DIAERESIS
		{
			mbsptr[0] = 0xD6;
		}
		else if (0x00D7 == ucsptr[0])	// MULTIPLICATION SIGN
		{
			mbsptr[0] = 0xD7;
		}
		else if (0x00D8 == ucsptr[0])	// LATIN CAPITAL LETTER O WITH STROKE
		{
			mbsptr[0] = 0xD8;
		}
		else if (0x00D9 == ucsptr[0])	// LATIN CAPITAL LETTER U WITH GRAVE
		{
			mbsptr[0] = 0xD9;
		}
		else if (0x00DA == ucsptr[0])	// LATIN CAPITAL LETTER U WITH ACUTE
		{
			mbsptr[0] = 0xDA;
		}
		else if (0x00DB == ucsptr[0])	// LATIN CAPITAL LETTER U WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xDB;
		}
		else if (0x00DC == ucsptr[0])	// LATIN CAPITAL LETTER U WITH DIAERESIS
		{
			mbsptr[0] = 0xDC;
		}
		else if (0x01AF == ucsptr[0])	// LATIN CAPITAL LETTER U WITH HORN
		{
			mbsptr[0] = 0xDD;
		}
		else if (0x0303 == ucsptr[0])	// COMBINING TILDE
		{
			mbsptr[0] = 0xDE;
		}
		else if (0x00DF == ucsptr[0])	// LATIN SMALL LETTER SHARP S
		{
			mbsptr[0] = 0xDF;
		}
		else if (0x00E0 == ucsptr[0])	// LATIN SMALL LETTER A WITH GRAVE
		{
			mbsptr[0] = 0xE0;
		}
		else if (0x00E1 == ucsptr[0])	// LATIN SMALL LETTER A WITH ACUTE
		{
			mbsptr[0] = 0xE1;
		}
		else if (0x00E2 == ucsptr[0])	// LATIN SMALL LETTER A WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xE2;
		}
		else if (0x0103 == ucsptr[0])	// LATIN SMALL LETTER A WITH BREVE
		{
			mbsptr[0] = 0xE3;
		}
		else if (0x00E4 == ucsptr[0])	// LATIN SMALL LETTER A WITH DIAERESIS
		{
			mbsptr[0] = 0xE4;
		}
		else if (0x00E5 == ucsptr[0])	// LATIN SMALL LETTER A WITH RING ABOVE
		{
			mbsptr[0] = 0xE5;
		}
		else if (0x00E6 == ucsptr[0])	// LATIN SMALL LETTER AE
		{
			mbsptr[0] = 0xE6;
		}
		else if (0x00E7 == ucsptr[0])	// LATIN SMALL LETTER C WITH CEDILLA
		{
			mbsptr[0] = 0xE7;
		}
		else if (0x00E8 == ucsptr[0])	// LATIN SMALL LETTER E WITH GRAVE
		{
			mbsptr[0] = 0xE8;
		}
		else if (0x00E9 == ucsptr[0])	// LATIN SMALL LETTER E WITH ACUTE
		{
			mbsptr[0] = 0xE9;
		}
		else if (0x00EA == ucsptr[0])	// LATIN SMALL LETTER E WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xEA;
		}
		else if (0x00EB == ucsptr[0])	// LATIN SMALL LETTER E WITH DIAERESIS
		{
			mbsptr[0] = 0xEB;
		}
		else if (0x0301 == ucsptr[0])	// COMBINING ACUTE ACCENT
		{
			mbsptr[0] = 0xEC;
		}
		else if (0x00ED == ucsptr[0])	// LATIN SMALL LETTER I WITH ACUTE
		{
			mbsptr[0] = 0xED;
		}
		else if (0x00EE == ucsptr[0])	// LATIN SMALL LETTER I WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xEE;
		}
		else if (0x00EF == ucsptr[0])	// LATIN SMALL LETTER I WITH DIAERESIS
		{
			mbsptr[0] = 0xEF;
		}
		else if (0x0111 == ucsptr[0])	// LATIN SMALL LETTER D WITH STROKE
		{
			mbsptr[0] = 0xF0;
		}
		else if (0x00F1 == ucsptr[0])	// LATIN SMALL LETTER N WITH TILDE
		{
			mbsptr[0] = 0xF1;
		}
		else if (0x0323 == ucsptr[0])	// COMBINING DOT BELOW
		{
			mbsptr[0] = 0xF2;
		}
		else if (0x00F3 == ucsptr[0])	// LATIN SMALL LETTER O WITH ACUTE
		{
			mbsptr[0] = 0xF3;
		}
		else if (0x00F4 == ucsptr[0])	// LATIN SMALL LETTER O WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xF4;
		}
		else if (0x01A1 == ucsptr[0])	// LATIN SMALL LETTER O WITH HORN
		{
			mbsptr[0] = 0xF5;
		}
		else if (0x00F6 == ucsptr[0])	// LATIN SMALL LETTER O WITH DIAERESIS
		{
			mbsptr[0] = 0xF6;
		}
		else if (0x00F7 == ucsptr[0])	// DIVISION SIGN
		{
			mbsptr[0] = 0xF7;
		}
		else if (0x00F8 == ucsptr[0])	// LATIN SMALL LETTER O WITH STROKE
		{
			mbsptr[0] = 0xF8;
		}
		else if (0x00F9 == ucsptr[0])	// LATIN SMALL LETTER U WITH GRAVE
		{
			mbsptr[0] = 0xF9;
		}
		else if (0x00FA == ucsptr[0])	// LATIN SMALL LETTER U WITH ACUTE
		{
			mbsptr[0] = 0xFA;
		}
		else if (0x00FB == ucsptr[0])	// LATIN SMALL LETTER U WITH CIRCUMFLEX
		{
			mbsptr[0] = 0xFB;
		}
		else if (0x00FC == ucsptr[0])	// LATIN SMALL LETTER U WITH DIAERESIS
		{
			mbsptr[0] = 0xFC;
		}
		else if (0x20AB == ucsptr[0])	// LATIN SMALL LETTER U WITH HORN
		{
			mbsptr[0] = 0xFD;
		}
		else if (0x017E == ucsptr[0])	// DONG SIGN
		{
			mbsptr[0] = 0xFE;
		}
		else if (0x00FF == ucsptr[0])	// LATIN SMALL LETTER Y WITH DIAERESIS
		{
			mbsptr[0] = 0xFF;
		}
	}

	return numChars;
}

void * rfhMalloc(size_t length, const char * tag)

{
	char *			ptr;
	CRfhutilApp *	app;				// pointer to the application object
	char			traceInfo[512];		// work variable to build trace message
	char            tmpTag[9] = { 0 };

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// allocate the memory plus an additional 16 bytes
	ptr = (char *)malloc(length + 16);
	
	// did the malloc work?
	if (NULL == ptr)
	{
		// check if trace is enabled
		if (app->isTraceEnabled())
		{
			// create the trace line
			sprintf(traceInfo, "rfhMalloc() - malloc failed %8.8s length=%d mallocReqs=%I64d totalBytes=%I64d", tag, length, mallocReqs, totalBytes);

			// trace entry to rfhMalloc
			app->logTraceEntry(traceInfo);
		}

		// malloc failed - just return
		return ptr;
	}

	// keep track of total bytes allocated
	totalBytes += length;
	currBytes += length;
	mallocReqs++;

	// check for a new high water mark
	if (currBytes > maxBytes)
	{
		// note new high
		maxBytes = currBytes;
	}

	
	// set a tag at the front of the data. Compiler/Analyser gives bogus warning so suppress it
	memset(ptr, 0, 16 + length); // Fill buffer with 0
	// Make sure tag has 8 bytes available */
	memset(tmpTag, ' ', 8);
	strncpy(tmpTag, tag, 8);
	memcpy(ptr, tmpTag, 8);
	memcpy(ptr + 8, &length, sizeof(size_t));

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering rfhMalloc() %8.8s ptr=%8.8X length=%d mallocReqs=%I64d totalBytes=%I64d", tag, (unsigned int)ptr, length, mallocReqs, totalBytes);

		// trace entry to rfhMalloc
		app->logTraceEntry(traceInfo);
	}

	return ptr + 16;
}

void rfhFree(void * ptr)

{
	char *	realPtr;
	CRfhutilApp *	app;
	size_t			len=0;				// variable to retrieve allocated length
	char			tagData[16];
	char			traceInfo[512];		// work variable to build trace message

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// keep track of the number of free requests
	freeReqs++;

	// make sure the pointer is valid
	if (ptr != NULL)
	{
		// initialize the tag data area
		memset(tagData, 0, sizeof(tagData));

		// the real address is 16 bytes earlier
		realPtr = (char *)ptr;
		realPtr = realPtr - 16;

		// capture the tag and original length
		memcpy(tagData, realPtr, 8);
		memcpy(&len, realPtr + 8, sizeof(size_t));

		// calculate total bytes freed and current allocated
		freeBytes += len;
		currBytes -= len;

		// release the storage
		free(realPtr);

		// check if trace is enabled
		if (app->isTraceEnabled())
		{
			// create the trace line
			sprintf(traceInfo, "Entering rfhFree() %8.8s ptr=%8.8X realPtr=%8.8X len=%d freeReqs=%I64d freeBytes=%I64d currBytes=%I64d", tagData, (unsigned int)ptr, (unsigned int)realPtr, len, freeReqs, freeBytes, currBytes);

			// trace entry to rfhMalloc
			app->logTraceEntry(traceInfo);
		}
	}
}

void rfhTraceStats()

{
	CRfhutilApp *	app;
	char			traceInfo[512];		// work variable to build trace message

	// find the application object
	app = (CRfhutilApp *)AfxGetApp();

	// check if trace is enabled
	if (app->isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "Entering rfhTraceStats() mallocReqs=%I64d totalBytes=%I64d freeReqs=%I64d freeBytes=%I64d currBytes=%I64d maxBytes=%I64d", mallocReqs, totalBytes, freeReqs, freeBytes, currBytes, maxBytes);

		// trace entry to rfhMalloc
		app->logTraceEntry(traceInfo);
	}
}
