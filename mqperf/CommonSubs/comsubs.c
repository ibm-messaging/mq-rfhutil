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
/* comsubs.c - Common subroutines                                   */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#endif

/* definitions of 64-bit values for platform independence */
#include "int64defs.h"

#include "comsubs.h"

/* maximum number of bytes displayed on one line when dumping out data areas to the trace file */
#define		MAX_TRACE_BYTES_PER_LINE		32

static const unsigned char HEX_NUMBERS[] = "0123456789ABCDEF";
static const unsigned char BASE64ENCODE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*****************************************************************************/
static  const unsigned char aetab[] = {    /***** ASCII -----> EBCDIC *****/
                    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,   /* 00 */
                    0x08,0x09,0x25,0x0B,0x0C,0x0D,0x0E,0x0F,   /* 08 */
                    0x10,0x11,0x12,0x13,0xB6,0xB5,0x16,0x17,   /* 10 */
                    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,   /* 18 */
                    0x40,0x4F,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,   /* 20 */
                    0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,   /* 28 */
                    0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,   /* 30 */
                    0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,   /* 38 */
                    0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,   /* 40 */
                    0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,   /* 48 */
                    0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,   /* 50 */
                    0xE7,0xE8,0xE9,0x4A,0xE0,0x5A,0x5F,0x6D,   /* 58 */
                    0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,   /* 60 */
                    0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,   /* 68 */
                    0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,   /* 70 */
                    0xA7,0xA8,0xA9,0xC0,0xBB,0xD0,0xA1,0x41,   /* 78 */
                    0x68,0xDC,0x51,0x42,0x43,0x44,0x47,0x48,   /* 80 */
                    0x52,0x53,0x54,0x57,0x56,0x58,0x63,0x67,   /* 88 */
                    0x71,0x9C,0x9E,0xCB,0xCC,0xCD,0xDB,0xDD,   /* 90 */
                    0xDF,0xEC,0xFC,0x70,0xB1,0x80,0x41,0xB4,   /* 98 */
                    0x45,0x55,0xCE,0xDE,0x49,0x69,0x9A,0x9B,   /* A0 */
                    0xAB,0xAF,0xBA,0xB8,0xB7,0xAA,0x8A,0x8B,   /* A8 */
                    0x41,0x41,0x41,0x41,0x41,0x65,0x62,0x64,   /* B0 */
                    0x41,0x41,0x41,0x41,0x41,0xB0,0xB2,0x41,   /* B8 */
                    0x41,0x41,0x41,0x41,0x41,0x41,0x46,0x66,   /* C0 */
                    0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x9F,   /* C8 */
                    0x8C,0xAC,0x72,0x73,0x74,0xDA,0x75,0x76,   /* D0 */
                    0x77,0x41,0x41,0x41,0x41,0x6A,0x78,0x41,   /* D8 */
                    0xEE,0x59,0xEB,0xED,0xCF,0xEF,0xA0,0xAE,   /* E0 */
                    0x8E,0xFE,0xFB,0xFD,0x8D,0xAD,0xBC,0xBE,   /* E8 */
                    0xCA,0x8F,0xBF,0xB9,0xB6,0xB5,0x41,0x9D,   /* F0 */
                    0x90,0xBD,0xB3,0x41,0xFA,0xEA,0x41,0x41 }; /* F8 */

static  const unsigned char eatab[] = {  /***** EBCDIC -----> ASCII *****/
                    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,   /* 00 */
                    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,   /* 08 */
                    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,   /* 10 */
                    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,   /* 18 */
                    0x20,0x21,0x22,0x23,0x24,0x0A,0x26,0x27,   /* 20 */
                    0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,   /* 28 */
                    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,   /* 30 */
                    0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,   /* 38 */
                    0x20,0xFF,0x83,0x84,0x85,0xA0,0xC6,0x86,   /* 40 */
                    0x87,0xA4,0x5B,0x2E,0x3C,0x28,0x2B,0x21,   /* 48 */
                    0x26,0x82,0x88,0x89,0x8A,0xA1,0x8C,0x8B,   /* 50 */
                    0x8D,0xE1,0x5D,0x24,0x2A,0x29,0x3B,0x5E,   /* 58 */
                    0x2D,0x2F,0xB6,0x8E,0xB7,0xB5,0xC7,0x8F,   /* 60 */
                    0x80,0xA5,0xDD,0x2C,0x25,0x5F,0x3E,0x3F,   /* 68 */
                    0x9B,0x90,0xD2,0xD3,0xD4,0xD6,0xD7,0xD8,   /* 70 */
                    0xDE,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,   /* 78 */
                    0x9D,0x61,0x62,0x63,0x64,0x65,0x66,0x67,   /* 80 */
                    0x68,0x69,0xAE,0xAF,0xD0,0xEC,0xE8,0xF1,   /* 88 */
                    0xF8,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,   /* 90 */
                    0x71,0x72,0xA6,0xA7,0x91,0xF7,0x92,0xCF,   /* 98 */
                    0xE6,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,   /* A0 */
                    0x79,0x7A,0xAD,0xA8,0xD1,0xED,0xE7,0xA9,   /* A8 */
                    0xBD,0x9C,0xBE,0xFA,0x9F,0xF5,0xF4,0xAC,   /* B0 */
                    0xAB,0xF3,0xAA,0x7C,0xEE,0xF9,0xEF,0xF2,   /* B8 */
                    0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,   /* C0 */
                    0x48,0x49,0xF0,0x93,0x94,0x95,0xA2,0xE4,   /* C8 */
                    0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,   /* D0 */
                    0x51,0x52,0xD5,0x96,0x81,0x97,0xA3,0x98,   /* D8 */
                    0x5C,0xE1,0x53,0x54,0x55,0x56,0x57,0x58,   /* E0 */
                    0x59,0x5A,0xFD,0xE2,0x99,0xE3,0xE0,0xE5,   /* E8 */
                    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,   /* F0 */
                    0x38,0x39,0xFC,0xEA,0x9A,0xEB,0xE9,0xFF};  /* F8 */

/* file handle used for logging */
	FILE *	logFile=NULL;

/**************************************************************/
/*                                                            */
/* This routine writes information to a log.  If no log       */
/* file name is provided then the information is written to   */
/* stdout.                                                    */
/*                                                            */
/**************************************************************/

void writeLog(const int addCRLF, const char * templine)

{
	time_t	ltime;					/* number of seconds since 1/1/70     */
	struct	tm *today;				/* today's date as a structure        */
	char	todaysDate[32];

	/* write message to stdout */
	if (1 == addCRLF)
	{
		printf("%s\n", templine);
	}
	else
	{
		printf("%s", templine);
	}

	if (logFile != NULL)
	{
		/* get a time stamp as well */
		memset(todaysDate, 0, sizeof(todaysDate));
		time(&ltime);
		today = localtime(&ltime);
		strftime(todaysDate, sizeof(todaysDate) - 1, "%H.%M.%S", today);

		/* write to the file */
		if (1 == addCRLF)
		{
			fprintf(logFile, "%s %s\n", todaysDate, templine);
		}
		else
		{
			fprintf(logFile, "%s %s", todaysDate, templine);
		}

		/* force the output to the disk */
		fflush(logFile);
	}
}

void Log(const char * szFormat, ...)

{
	va_list	list;
	char	templine[32768];		/* hopefully long enough */

	va_start(list, szFormat);

	/* create the line to be logged */
	vsprintf(templine, szFormat, list);

	writeLog(1, templine);

	va_end(list);
}

void LogNoCRLF(const char *szFormat, ...)

{
	va_list	list;
	char	templine[32768];		/* hopefully long enough */

	va_start(list, szFormat);

	/* create the line to be logged */
	vsprintf(templine, szFormat, list);

	writeLog(0, templine);

	va_end(list);
}

int openLog(const char * fileName)

{
	int	rc=0;

	/* check if the file is open */
	if (NULL == logFile)
	{
		logFile = fopen(fileName, "a");

		/* check if it worked */
		if (logFile != NULL)
		{
			/* write a line to indicate the start of the log */
			Log("Log file opened");
		}
		else
		{
			/* unable to open the file */
			rc = 2;
		}
	}
	else
	{
		/* already open - treat as error */
		rc = 1;
	}

	return rc;
}

void closeLog()

{
	if (logFile != NULL)
	{
		fclose(logFile);
		logFile = NULL;
	}
}

void dumpTraceData(const char * label, const unsigned char *data, unsigned int length)

{
	int						i;
	int						j;
	int						k;
	int						remaining;
	int						count;
	int						offset=0;
	const unsigned char	*	ptr=data;
	const unsigned char	*	asciiptr=data;
	char					traceLine[256];


	/* check if a label line was provided */
	if (label != NULL)
	{
		/* create a label line with the address included */
		sprintf(traceLine, "%.192s at %p", label, data);
	}
	else
	{
		/* create a label line with the address included */
		sprintf(traceLine, "memory at %p", data);
	}

	/* write the lable line to the console */
	Log(traceLine);

	/* check if the data pointer is NULL */
	if ((NULL == data) || (0 == length))
	{
		/* make sure we don't blow up */
		Log("data is NULL or zero length");
		return;
	}

	/* is the length more than 32K? */
	if (length > 32 * 1024)
	{
		/* build a trace line */
		sprintf(traceLine, "Dumping out first 32K bytes of %d", length);

		/* tell the user if the trace is being limited */
		Log(traceLine);

		/* limit the data to 32K */
		length = 32 * 1024;
	}

	/* get the number of bytes to output */
	remaining = length;
	ptr = data;
	while (remaining > 0)
	{
		/* get the number of bytes to output on this line */
		count = remaining;
		if (count > MAX_TRACE_BYTES_PER_LINE)
		{
			count = 32;
		}

		/* build the ascii part of the line */
		/* initialize the line to a 4 character offset plus 37 blanks (32 characters of data plus 5 blank delimiters) */
		sprintf(traceLine, "%4.4X", offset);
		offset += 32;
		memset(traceLine + 4, ' ', 37);

		/* process the individual characters */
		i = 0;
		j = 5;
		k = 0;
		while (i < count)
		{
			if ((asciiptr[0] >= ' ') && (asciiptr[0] < 127))
			{
				traceLine[j] = asciiptr[0];
			}
			else
			{
				traceLine[j] = '.';
			}

			/* move on to the next character */
			i++;
			j++;
			asciiptr++;

			/* check if this is the fourth character */
			k++;
			if (8 == k)
			{
				/* reset the counter */
				k = 0;

				/* skip a blank character */
				j++;
			}
		}

		/* build the hex part of the line */
		j = 41;							/* pointer to the next character location to be added to the line */
		k = count >> 2;					/* get the count of whole 4-byte segments */
		for (i=0; i<k; i++)
		{
			traceLine[j++] = ' ';		/* insert a blank every 4 bytes to make it easier to read */

			/* get the next 4 bytes as hex characters */
			AsciiToHex((unsigned char *)traceLine + j, ptr, 4);
			j += 8;									/* 4 bytes becomes 8 characters */
			ptr += 4;								/* advance the input pointer 4 bytes */
		}

		/* check if there is an odd number of bytes left */
		k = count % 4;
		if (k > 0)
		{
			traceLine[j++] = ' ';
			AsciiToHex((unsigned char *)traceLine + j, ptr, k);
			j += k * 2;								/* each byte becomes two hex characters on output */
			ptr += k;								/* advance the input pointer */
		}

		/* properly terminate the string */
		traceLine[j] = 0;

		/* write the trace entry to the console */
		Log("%s", traceLine);

		/* decrement the remaining bytes */
		remaining -= count;
	}
}

/**************************************************************/
/*                                                            */
/* This routine skips blanks in a string                      */
/*                                                            */
/**************************************************************/

char * skipBlanks(char *str)

{
   while (*str == ' ')
   {
      str++;
   } /* endwhile */

   return str;
}

/**************************************************************/
/*                                                            */
/* This routine finds the next blank in a string              */
/*                                                            */
/**************************************************************/

char * findBlank(char *str)

{
   while ((str[0] != ' ') && (str[0] != 0))
   {
      str++;
   } /* endwhile */

   return str;
}

/**************************************************************/
/*                                                            */
/* This routine translates a string to upper case.            */
/*                                                            */
/**************************************************************/

void strupper(char *str)

{
	size_t	i=0;
	size_t	len=strlen(str);

	while (i < len)
	{
		str[i] = toupper(str[i]);
		i++;
	}
}

/**************************************************************/
/*                                                            */
/* This routine removes trailing blanks.                      */
/*                                                            */
/**************************************************************/

void rtrim(char *str)

{
	size_t	len=strlen(str);

	while ((len > 0) && (str[len - 1] <= ' '))
	{
		str[len - 1] = 0;
		len--;
	}
}

/**************************************************************/
/*                                                            */
/* Routine to reverse the bytes in an int variable            */
/*                                                            */
/**************************************************************/

int reverseBytes4(int var)

{
    return ((unsigned int) var >> 24) + (((unsigned int) var & 0x00ff0000) >> 8) +
           (((unsigned int) var & 0x0000ff00) << 8) + (((unsigned int) var & 255) <<24);
}

/**************************************************************/
/*                                                            */
/* Routine to reverse the bytes in a 64-bit integer variable  */
/*                                                            */
/**************************************************************/

int64_t reverseBytes8(int64_t in)

{
	int64_t		result=0;
	char		*c_in;
	char		*out;

	c_in = (char *)&in;
	out = (char *)&result;
	out[0] = c_in[7];
	out[1] = c_in[6];
	out[2] = c_in[5];
	out[3] = c_in[4];
	out[4] = c_in[3];
	out[5] = c_in[2];
	out[6] = c_in[1];
	out[7] = c_in[0];

	return result;
}

/**************************************************************/
/*                                                            */
/*Translate from ASCII to EBCDIC                              */
/*                                                            */
/**************************************************************/

void AsciiToEbcdic(unsigned char *dato, const unsigned char *dati, size_t pl)

{
	size_t	i;

	for(i=0; i<pl; i++)
	{
		dato[i] = aetab[dati[i]];
	}
}

/**************************************************************/
/*                                                            */
/* Translate from EBCDIC to ASCII                             */
/*                                                            */
/**************************************************************/

void EbcdicToAscii(const unsigned char *dati, size_t pl, unsigned char *dato)

{
	size_t	i;

	for(i=0;i<pl;i++)
	{
		dato[i]=eatab[dati[i]];
	}
}

/**************************************************************/
/*                                                            */
/* Return the value of an individual                          */
/* hexadecimal character                                      */
/* the returned value should be                               */
/* between 0 and 15 inclusive.                                */
/*                                                            */
/**************************************************************/

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

/**************************************************************/
/*                                                            */
/*Translate from ASCII to Hex                                 */
/*                                                            */
/**************************************************************/


void AsciiToHex(unsigned char *dato, const unsigned char *dati, const unsigned int pl)

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

/**************************************************************/
/*                                                            */
/* Translate from Hex to ASCII                                */
/*                                                            */
/**************************************************************/

void HexToAscii(char *dati, size_t pl, char *dato)

{
	size_t	i;
	int		buffer;
	char	ch;

	buffer = 0;
	i = 0;
	while (i < pl)
	{
		ch = getHexCharValue((unsigned char) dati[i++]) << 4;
		ch += getHexCharValue((unsigned char) dati[i++]);
		dato[buffer++] = ch;
	}
}

/**************************************************************/
/*                                                            */
/* Convert binary data to base 64.                            */
/*                                                            */
/**************************************************************/

size_t Encode64(const unsigned char * input, size_t len, unsigned char * output, size_t maxLen, int padding)

{
	size_t					charsOut=0;			/* number of base64 bytes in output */
	size_t					remaining=len;		/* number of bytes remaining */
	const unsigned char *	in=input;			/* pointer to next input bye */
	unsigned char *			out=output;			/* pointer to next output byte */
	int						index=0;			/* index into encode table */
	int						index2=0;			/* index into encode table */

	/* Loop through the input data 3 bytes at a time */
	while ((remaining > 0) && (charsOut < maxLen))
	{
		/* always do at least the first byte */
		/* get the first 6 bits of the first input byte and shift the binary data 2 characters to the right */
		index = in[0] >> 2;

		/* insert into the output */
		out++[0] = BASE64ENCODE[index];
		charsOut++;

		/* get the last two bits in the first byte */
		index = (in[0] & 0x00000003) << 4;

		/* move on to the next byte of input */
		in++;
		remaining--;

		/* is there a second input byte? */
		if (remaining > 0)
		{
			/* get the first 4 bits of the second byte of input */
			index |= (in[0] >> 4); 
		}

		/* make sure not to overrun the output buffer */
		if (charsOut < maxLen)
		{
			/* insert into the output */
			out++[0] = BASE64ENCODE[index];
			charsOut++;
		}

		/* is there any remaining input? */
		if (remaining > 0)
		{
			/* get the next 4 bits of input */
			index = (in[0] & 0x0000000F) << 2;

			/* move on to the next byte of input */
			in++;
			remaining--;

			/* check if there is a third byte of data */
			if (remaining > 0)
			{
				/* get the first two bits of the third byte of data */
				index |= (in[0] & 0x000000C0) >> 6;
			}

			/* make sure not to overrun the output buffer */
			if (charsOut < maxLen)
			{
				/* insert into the output */
				out++[0] = BASE64ENCODE[index];
				charsOut++;
			}

			if (remaining > 0)
			{
				/* get the last 6 bits of input */
				index = (in[0] & 0x0000003F);

				/* make sure not to overrun the output buffer */
				if (charsOut < maxLen)
				{
					/* insert into the output */
					out++[0] = BASE64ENCODE[index];
					charsOut++;
				}

				/* move past this input byte */
				in++;
				remaining--;
			}
			else
			{
				/* check if padding characters to be used */
				if ((padding != 0) && (charsOut < maxLen))
				{
					/* insert the padding character */
					out++[0] = '=';

					/* count the padding character */
					charsOut++;
				}
			}
		}
		else
		{
			/* check if padding characters to be used */
			if ((padding != 0) && (charsOut + 1 < maxLen))
			{
				/* insert two padding characters */
				out++[0] = '=';
				out++[0] = '=';

				/* count the padding characters */
				charsOut += 2;
			}
		}
	}

	return charsOut;
}

/**************************************************************/
/*                                                            */
/* Convert character to 6-bit binary value (0-63)             */
/*                                                            */
/*  N.B. Returns zero for any unrecognized character,         */
/*  including a padding character ("=").                      */
/*                                                            */
/**************************************************************/

int	GetBase64Value(const unsigned char * nextByte)

{
	int		value=0;		/* set default value, which includes invalid characters or padding characters */

	/* check for capital letter */
	if ((nextByte[0] >= 'A') && (nextByte[0] <= 'Z'))
	{
		/* set the value */
		value = nextByte[0] - 'A';
	}

	/* check for capital letter */
	if ((nextByte[0] >= 'a') && (nextByte[0] <= 'z'))
	{
		/* set the value */
		value = nextByte[0] - 'a' + 26;
	}

	/* check for number */
	if ((nextByte[0] >= '0') && (nextByte[0] <= '9'))
	{
		/* set the value */
		value = nextByte[0] - '0' + 52;
	}

	/* check for special characters */
	if ('+' == nextByte[0])
	{
		/* value is 62 */
		value = 62;
	}
	else if ('/' == nextByte[0])
	{
		/* value is 63 */
		value = 63;
	}

	return value;
}

/**************************************************************/
/*                                                            */
/* Convert base 64 to binary data.                            */
/*                                                            */
/**************************************************************/

size_t Decode64(const unsigned char * input, size_t len, unsigned char * output, size_t maxLen)

{
	size_t	charsOut=0;
	size_t	remaining=len;
	int		value=0;
	int		padding;

	/* loop through taking four characters of output at a time */
	while ((remaining > 0) && (charsOut <= maxLen))
	{
		/* clear the padding indicator */
		padding = 0;

		/* get the next input character value */
		value = GetBase64Value(input) << 18;

		/* move on to the next input byte */
		input++;
		remaining--;

		/* check for remaining input bytes */
		if (remaining > 0)
		{
			/* get the next part of the 24-bit binary value */
			value |= GetBase64Value(input) << 12;

			/* move on to the next input byte */
			input++;
			remaining--;

			/* check for remaining input bytes */
			if (remaining > 0)
			{
				/* check for a padding character */
				if ('=' == input[0])
				{
					/* remember there are two padding characters */
					padding = 2;
				}

				/* get the next part of the 24-bit binary value */
				value |= GetBase64Value(input) << 6;

				/* move on to the next input byte */
				input++;
				remaining--;

				/* check for remaining input bytes */
				if (remaining > 0)
				{
					/* check for a padding character */
					/* first padding character? */
					if (('=' == input[0]) && (0 == padding))
					{
						padding = 1;
					}

					/* get the next part of the 24-bit binary value */
					value |= GetBase64Value(input);

					/* move on to the next input byte */
					input++;
					remaining--;
				}
			}
		}

		/* move the binary value to the output */
		if (charsOut < maxLen)
		{
			output[charsOut] = (value & 0x00ff0000) >> 16;

			/* move on to the next output value */
			charsOut++;
		}

		/* move the binary value to the output */
		if (charsOut < maxLen)
		{
			output[charsOut] = (value & 0x0000ff00) >> 8;

			/* check for two padding characters */
			if (padding < 2)
			{
				/* move on to the next output value */
				charsOut++;
			}
		}

		/* move the binary value to the output */
		if (charsOut < maxLen)
		{
			output[charsOut] = value & 0x000000ff;

			/* check for a padding character */
			if (0 == padding)
			{
				/* move on to the next output value */
				charsOut++;
			}
		}
	}

	return charsOut;
}

/**************************************************************/
/*                                                            */
/* Remove the leading and trailing quotes if present.         */
/*                                                            */
/**************************************************************/

char * removeQuotes(char *dati)

{
	char	ch;

	/* check for a leading quote */
	if ((dati[0] == '"') || (dati[0] == (char)148))
	{
		ch = dati[0];

		/* found leading quote, skip it */
		dati++;

		/* make sure the last character is a matching quote */
		if (dati[strlen(dati) - 1] == ch)
		{
			dati[strlen(dati) - 1] = 0;
		}
		else
		{
			printf("***** missing trailing quote for value %s\n", dati);
		}
	}

	return dati;
}

/**************************************************************/
/*                                                            */
/* Process a parameter line that contains a parameter of type */
/* hex string.                                                */
/*                                                            */
/**************************************************************/

void editHexParm(const char * parmName, 
				 char * parm, 
				 char * valueptr, 
				 size_t maxsize)

{
	size_t	len;

	len = strlen(valueptr);

	/* check if the parameter is too long */
	if (len > 2 * maxsize)
	{
		/* truncate the data */
		valueptr[2 * maxsize] = 0;

		/* issue an error message */
		printf("*****Error - parameter value length %d exceeds maximum %d for %s and was truncated\n", len, maxsize, parmName);

		len = 2 * maxsize;
	}

	/* make sure the result is an even number of bytes */
	if ((len & 1) > 0)
	{
		/* odd number of bytes, issue error and return */
		printf("***** %s truncated - odd number of bytes (%d) for hex value - %s\n", 
				parmName, len, valueptr);

		/* truncate the last character */
		len--;
	}

	memset(parm, 0, maxsize);
	HexToAscii(valueptr, len, parm);
}

int checkHexParm(const char * parmName,
				  const char * parmConst,
				  char * parm, 
				  char * valueptr, 
				  int foundit,
				  size_t maxsize)

{
	if ((0 == foundit) && (strcmp(parmName, parmConst) == 0))
	{
		editHexParm(parmName, parm, valueptr, maxsize);
		foundit = 1;
	}

	return foundit;
}

/**************************************************************/
/*                                                            */
/* Process a parameter line that contains a parameter of type */
/* string.                                                    */
/*                                                            */
/**************************************************************/

void editCharParm(const char * parmName, 
				 char * parm, 
				 char * valueptr, 
				 size_t maxsize)

{
	size_t	len;

	len = strlen(valueptr);

	/* check if the parameter is too long */
	if (len > (maxsize - 1))
	{
		/* truncate the data */
		valueptr[maxsize - 1] = 0;

		/* issue an error message */
		printf("*****Error - parameter value length %d exceeds maximum %d for %s and was truncated\n", len, maxsize, parmName);
	}

	/* check for previous setting */
	if (parm[0] > ' ')
	{
		/* issue warning message */
		printf("Parameter %s value was previously set and has been reset to %s\n", parmName, valueptr);
	}

	/* copy the parameter value to the variable */
	memset(parm, 0, maxsize);
	strcpy(parm, valueptr);
}

int checkCharParm(const char * parmName,
				  const char * parmConst,
				  char * parm, 
				  char * valueptr, 
				  int * foundParm,
				  int foundit,
				  size_t maxsize)

{
	if ((0 == foundit) && (strcmp(parmName, parmConst) == 0))
	{
		editCharParm(parmName, parm, valueptr, maxsize);

		/* check if want to remember found this one */
		if (foundParm != NULL)
		{
			(*foundParm) = 1;
		}

		foundit = 1;
	}

	return foundit;
}

/**************************************************************/
/*                                                            */
/* Process a parameter line that contains a parameter of type */
/* string.                                                    */
/*                                                            */
/**************************************************************/

void editYNParm(const char * parmName, 
				int * parm, 
				char * valueptr)

{
	if (('Y' == valueptr[0]) || ('y' == valueptr[0]) || ('1' == valueptr[0]))
	{
		(*parm) = 1;
	}
	else
	{
		(*parm) = 0;

		if (('N' != valueptr[0]) && ('n' != valueptr[0]) && ('0' != valueptr[0]))
		{
			/* invalid specification - issue error message */
			printf("***** Invalid specification for Yes/No parameter %s - set to No\n", parmName);
		}
	}
}

int checkYNParm(const char * parmName,
				const char * parmConst,
				int * parm, 
				char * valueptr, 
				int * foundParm,
				int foundit)

{
	if ((0 == foundit) && (strcmp(parmName, parmConst) == 0))
	{
		editYNParm(parmName, parm, valueptr);

		/* check if we want to remember we found this one */
		if (foundParm != NULL)
		{
			(*foundParm) = 1;
		}

		foundit = 1;
	}

	return foundit;
}

int checkIntParm(const char * parmName,
				 const char * parmConst,
				 int * value, 
				 char * valueptr, 
				 int * foundParm,
				 int foundit)

{
	if ((0 == foundit) && (strcmp(parmName, parmConst) == 0))
	{
		(*value) = atoi(valueptr);

		/* check if want to remember found this one */
		if (foundParm != NULL)
		{
			(*foundParm) = 1;
		}

		foundit = 1;
	}

	return foundit;
}

/**************************************************************/
/*                                                            */
/* Subroutine to check the length of a string and return it   */
/*  an unsigned integer.                                      */
/*                                                            */
/**************************************************************/

unsigned int iStrLen(const char * ptr)

{
	unsigned int	i=0;

	/* check for the end of the string */
	while (ptr[i] != 0)
	{
		/* move on to the next character */
		i++;
	}

	/* return the length of the string */
	return i;
}

int64_t my_ato64(const char * valueptr)

{
	int64_t		result=0;
	size_t		i;

	for (i=0; i < strlen(valueptr); i++)
	{
		/* check for a number */
		if ((valueptr[i] >= '0') && (valueptr[i] <= '9'))
		{
			/* multiply the previous result by 10 and add the new value */
			result = (result * 10) + (int)(valueptr[i] - '0');
		}
		else
		{
			/* warn that value is not numeric */
			printf("Non-numeric characters found in parameter value %s\n", valueptr);
			break;
		}
	}

	return result;
}

int checkI64Parm(const char * parmName,
				 const char * parmConst,
				 int64_t * value, 
				 char * valueptr, 
				 int * foundParm,
				 int foundit)

{
	if ((0 == foundit) && (strcmp(parmName, parmConst) == 0))
	{
		/* set the value */
		(*value) = my_ato64(valueptr);

		/* check if remember found this one */
		if (foundParm != NULL)
		{
			(*foundParm) = 1;
		}

		/* parameters matched */
		foundit = 1;
	}

	/* return match status */
	return foundit;
}

void reverseBytes24(unsigned char *in, unsigned char *out)

{
	int	u1;
	int	u2;
	int	i;

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
