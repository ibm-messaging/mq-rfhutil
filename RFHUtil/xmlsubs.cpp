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

// xmlsubs.c: XML subroutines.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "string.h"
#include "comsubs.h"
#include "xmlsubs.h"

#define CDATA_START		"<![CDATA["
#define CDATA_END		"]]>"
#define ENTITY_START	"<!ENTITY"

///////////////////////////////////////////////////
//
// Routine to handle a DTD
// the length of the DTD is returned
//
///////////////////////////////////////////////////

int processDTD(const char *dtd, const int remaining)

{
	const char * dtdend=dtd;

	if (memcmp(dtd, "<!DOCTYPE", 9) == 0)
	{
		// point to the end of the DTD header (<!DOCTYPE)
		dtdend = dtd + 9;

		while ((dtdend < dtd + remaining) && (dtdend[0] != '[') && (dtdend[0] != '>'))
		{
			dtdend++;
		}

		// did we find a bracket?
		if ('[' == dtdend[0])
		{
			while ((dtdend < dtd + remaining) && (dtdend[0] != ']'))
			{
				if (memcmp(dtdend, "<!ENTITY", 8) == 0)
				{
					dtdend = processEntity(dtdend, dtd + remaining);
				}
				else
				{
					if (memcmp(dtdend, "\"]\"", 3) != 0)
					{
						dtdend++;
					}
					else
					{
						dtdend += 3;
					}
				}
			}

			// skip the ending bracket delimiter
			dtdend++;

			// find the ending greater than sign
			while ((dtdend < dtd + remaining) && (dtdend[0] != '>'))
			{
				dtdend++;
			}
		}

		// skip the ending greater than sign
		dtdend++;
	}
	else
	{
		if (memcmp(dtd, "<\0!\0D\0O\0C\0T\0Y\0P\0E\0", 18) == 0)
		{
			// point to the end of the DTD header (<!DOCTYPE)
			dtdend = dtd + 18;

			// look for either a bracket or ending greater than sign
			while ((dtdend < (dtd + remaining)) && 
				   (memcmp(dtdend, "[\0", 2) != 0) && 
				   (memcmp(dtdend, ">\0", 2) != 0))
			{
				dtdend++;
			}

			// did we find a bracket?
			if ('[' == dtdend[0])
			{
				while ((dtdend < dtd + remaining) && (memcmp(dtdend, "]\0", 2) != 0))
				{
					dtdend++;
				}

				// skip the ending bracket
				dtdend += 2;

				// find the ending greater than sign
				while ((dtdend < dtd + remaining) && (memcmp(dtdend, ">\0", 2) != 0))
				{
					dtdend++;
				}
			}

			// skip the ending bracket
			dtdend += 2;
		}
	}

	return dtdend - dtd;
}

///////////////////////////////////////////////////
//
// Routine to handle an XML comment
// the length of the comment is returned
//
///////////////////////////////////////////////////

int processComment(const char *xml, const int remaining)

{
	const char * xmlend=xml;

	if (memcmp(xml, "<!--", 4) == 0)
	{
		// point to the end of the comment header (<!--)
		xmlend = xml + 4;

		// search for the end of the comment
		while ((xmlend < xml + remaining) && (memcmp(xmlend, "-->", 3) != 0))
		{
			xmlend++;
		}

		// skip the ending delimiter
		xmlend += 3;
	}
	else
	{
		// check for Unicode comment
		if (memcmp(xml, "<\0!\0-\0-\0", 8) == 0)
		{
			// point to the end of the comment header (<!--)
			xmlend = xml + 8;

			// search for the end of the comment
			while ((xmlend < xml + remaining) && (memcmp(xmlend, "-\0-\0>\0", 6) != 0))
			{
				xmlend++;
			}

			// skip the ending delimiter
			xmlend += 6;
		}
	}

	return xmlend - xml;
}

///////////////////////////////////////////////////
//
// Routine to handle a DTD Entity clause
// the address of the first character after
// the clause is returned.
//
///////////////////////////////////////////////////

const char * processEntity(const char *dtdend, const char *eod)

{
	char	parsetype;

	// make sure we have found an entity 
	if (memcmp(dtdend, ENTITY_START, sizeof(ENTITY_START) - 1) == 0)
	{
		// skip the entity definition
		dtdend += 8;

		// skip any blanks
		dtdend = skipBlanks((char *) dtdend);

		// check for parameter entity
		if ('%' == dtdend[0])
		{
			// skip the leading percent sign
			dtdend++;
			dtdend = skipBlanks((char *) dtdend);
		}

		// skip the name of the entity
		while ((dtdend < eod) && (dtdend[0] > ' '))
		{
			dtdend++;
		}

		// skip any intervening blanks
		dtdend = skipBlanks((char *) dtdend);

		// we're now past the entity name and need to 
		// figure out the type of entity
		if ((memcmp(dtdend, "SYSTEM", 6) == 0) || (memcmp(dtdend, "PUBLIC", 6) == 0))
		{
			// remember the type of entity
			parsetype = dtdend[0];

			// skip the entity type
			dtdend += 6;

			// skip any intervening blanks
			dtdend = skipBlanks((char *) dtdend);

			// skip the first attribute value
			dtdend = skipQuotedString(dtdend, eod);

			// find the next non-blank character
			dtdend = skipBlanks((char *) dtdend);

			// check if we have a public entity
			if ('P' == parsetype)
			{
				// skip the second quoted string
				dtdend = skipQuotedString(dtdend, eod);

				// find the next non-blank character
				dtdend = skipBlanks((char *) dtdend);
			}
		}
		else
		{
			// we should have a string in quotes
			// skip this item
			dtdend = skipQuotedString(dtdend, eod);
		}

		// finally, find the last greater than sign
		while ((dtdend < eod) && (dtdend[0] != '>'))
		{
			dtdend++;
		}

		// finally, skip the last greater than sign
		dtdend++;
	}

	return dtdend;
}

int removeUnneededXML(unsigned char *dataIn, unsigned int len, unsigned char *tempData)

{
	// at this point, get the original data into a temporary 
	// buffer, removing any data that is not valid ascii 
	// characters
	int					i=0;
	unsigned int		j=0;			// work variable
	int					foundBegin=0;

	while (j < len)
	{
		if ((dataIn[j] >= ' ') && (dataIn[j] < 128))
		{
			// we found something worth saving
			foundBegin = 1;

			// check for comments, which will be removed
			if ((memcmp(dataIn + j, "<!--", 4) == 0) ||
				(memcmp(dataIn + j, "<\0!\0-\0-\0", 8) == 0))
			{
				// skip the comment
				j += processComment((char *) dataIn + j, len - j);
			}
			else
			{
				// check for a DTD definition, which we will remove
				if ((memcmp(dataIn + j, "<!DOCTYPE", 9) == 0) ||
					(memcmp(dataIn + j, "<\0!\0D\0O\0C\0T\0Y\0P\0E\0", 18) == 0))
				{
					// skip the beginning of the DTD
					j += processDTD((char *) dataIn + j, len - j);
				}
				else
				{
					// valid character, move it to the output
					tempData[i++] = dataIn[j++];
				}
			}
		}
		else
		{
			// skip any leading blanks and other whitespace
			if (1 == foundBegin)
			{
				// skip any characters that are less than a blank, except for
				// newlines, carriage returns and tabs
				if ((dataIn[j] < ' ')  && 
					(dataIn[j] != '\r') && 
					(dataIn[j] != '\n') && 
					(dataIn[j] != '\t'))
				{
					j++;
				}
				else
				{
					tempData[i++] = dataIn[j++];
				}
			}
			else
			{
				j++;
			}
		}
	}

	// terminate the string
	tempData[i] = 0;

	// strip any trailing characters
	while ((i > 0) && (tempData[i - 1] <= ' '))
	{
		tempData[i] = 0;
		i--;
	}

	return i + 1;
}

////////////////////////////////////
//
// Routine to do a simple check if
// the data could possibly be XML
//
// Return Codes
//
//  1 - No begin bracket found
//  2 - Number of end brackets not same as begin brackets
//  3 - End bracket found before begin bracket
//
////////////////////////////////////

int checkIfXml(const unsigned char *data, const int len, char **errmsg)

{
	int	offset=0;
	int	begin=0;
	int	rc=0;
	int	end=0;
	int inCdata=0;
	int	bracketLevel=0;

	while (offset < len)
	{
		if (0 == inCdata)
		{
			// check for beginning of CDATA section
			if (memcmp(data + offset, "<![CDATA", 8) == 0)
			{
				// remember we are in CDATA
				inCdata = 1;
				offset += 8;
			}
			else
			{
				// check for the beginning of a comment
				if ((memcmp(data + offset, "<!--", 4) == 0) ||
					(memcmp(data + offset, "<\0!\0-\0-\0", 8) == 0))
				{
					// ignore the comment
					offset += processComment((char *) data + offset, len - offset);
				}
				else
				{
					// check for a DTD
					if ((memcmp(data + offset, "<!DOCTYPE", 9) == 0) ||
						(memcmp(data + offset, "<\0!\0D\0O\0C\0T\0Y\0P\0E\0", 18) == 0))
					{
						// skip the DTD
						offset += processDTD((const char *) data + offset, len - offset);
					}
					else
					{
						if (data[offset] == '<')
						{
							begin++;
							bracketLevel++;
						}

						if (data[offset] == '>')
						{
							// check if we have a previous begin bracket
							// otherwise, the end bracket may be in a data area,
							// which could be OK
							if (bracketLevel > 0)
							{
								end++;
								bracketLevel--;
							}

							if (begin == 0)
							{
								// end bracket before first begin bracket 
								rc = 3;
							}
						}

						offset++;
					}
				}
			}
		}
		else
		{
			// look for end of CDATA section
			while ((offset < len) && (memcmp(data + offset, CDATA_END, 3) != 0))
			{
				offset++;
			}

			// skip the ending sequence
			inCdata = 0;
			offset += 3;
		}
	}

	if (begin != end)
	{
		rc = 2;
	}

	if (begin == 0)
	{
		rc = 1;
	}

	switch (rc)
	{
	case 1:
		{
			*errmsg = "*****No beginning bracket found\r\n";
			break;
		}
	case 2:
		{
			*errmsg = "*****Number of end brackets does not match begin brackets\r\n";
			break;
		}
	case 3:
		{
			*errmsg = "*****End bracket before first begin bracket\r\n";
			break;
		}
	}

	return rc;
}

////////////////////////////////////////////////////
//
// Routine to remove a variable name from the
// end of a parsed name string
//
////////////////////////////////////////////////////

int removeVarName(char *xmlVarName, char *VarName)

{
	int		rc = 0;
	int		templen;
	int		templen2;
	char	*tempPtr;

	// get the length of the variable part
	templen = strlen(VarName);
	templen2 = strlen(xmlVarName);
	if (templen > 0)
	{
		templen--;
	}

	tempPtr = xmlVarName + templen2 - templen;
	if (strcmp(tempPtr, VarName + 1) == 0)
	{
		if (tempPtr > xmlVarName)
		{
			tempPtr--;
			if ('.' == tempPtr[0])
			{
				tempPtr[0] = 0;
			}
			else
			{
				// indicate name break not at correct point
				rc = 1;
			}
		}
	}
	else
	{
		// indicate names do not match
		rc = 1;
	}

	return rc;
}

///////////////////////////////////
//
// Remove any Carriage return or line
//  feed characters, returning the
//  length of the data after the
//  characters are removed.
//
///////////////////////////////////

int removeCrLf(unsigned char *data, unsigned int len)

{
	unsigned int i=0;
	unsigned int j=0;
	char	ch;

	while (i < len)
	{
		ch = data[i++];

		if ((ch != '\n') && (ch != '\r'))
		{
			data[j++] = ch;
		}
	}

	data[j] = 0;
	return j;
}

///////////////////////////////////////////////////////////////////
//
// Routine to replace certain escape sequences with their
// respective characters, as follows:
//
// &amp;	&
// &quot;	"
// &gt;		>
// &lt;		<
// &apos;	'
// &#nnn;
//
///////////////////////////////////////////////////////////////////

void removeEscSeq(char *valStr)

{
	int	i=0;
	int	j=0;
	int	len;
	int	templen;
	char *ptr;
	char ch=' ';
	char tempSeq[65];

	len = strlen(valStr);

	// check for a zero length string
	if (0 == len)
	{
		// nothing to do so return immediately
		// this allows this routine to be called using a literal null string ("")
		return;
	}

	while (i < len)
	{
		if (valStr[i] == '&')
		{
			// get the next characters (max of 64)
			templen = len - i;
			if (templen > (sizeof(tempSeq) - 1))
			{
				templen = sizeof(tempSeq) - 1;
			}

			memcpy(tempSeq, valStr + i + 1, templen);
			tempSeq[templen] = 0;

			ptr = strchr(tempSeq + 1, ';');
							
			// check if we found a semicolon delimiter
			if (ptr != NULL)
			{
				// get rid of the semicolon
				ptr[0] = 0;
			}

			// translate to upper case
			_strupr(tempSeq);

			// check for any of the escape sequences
			if ((strcmp(tempSeq, "LT") == 0) || (strcmp(tempSeq, "GT") == 0))
			{
				// skip the escape sequence
				i += 4;
				if (strcmp(tempSeq, "LT") == 0)
				{
					ch = '<';
				}
				else
				{
					ch = '>';
				}
			}
			else
			{
				if (strcmp(tempSeq, "AMP") == 0)
				{
					// skip the escape sequence
					i += 5;
					ch = '&';
				}
				else
				{
					if ((strcmp(tempSeq, "APOS") == 0) || (strcmp(tempSeq, "QUOT") == 0))
					{
						// skip the escape sequence
						i += 6;

						// replace the character
						if (strcmp(tempSeq, "APOS") == 0)
						{
							ch = '\'';
						}
						else
						{
							ch = '"';
						}
					}
					else
					{
						if ('#' == tempSeq[0])
						{
							// arbitrary character
							// there can be from 1 to 13 digits
							// we will look for a trailing semi-colon 
							// and remove it, and then convert the
							// remaining characters to an integer

							// did we find the semicolon?
							if (ptr != NULL)
							{
								if (tempSeq[1] != 'X')
								{
									// get the integer value
									ch = atoi(tempSeq + 1);
								}
								else
								{
									// hex value
									ptr = tempSeq + 2;

									// skip any leading zeros
									while ('0' == ptr[0])
									{
										ptr++;
									}

									// did we find anything besides zeros?
									if (((ptr[0] >= '0') && (ptr[0] <= '9')) ||
										((ptr[0] >= 'A') && (ptr[0] <= 'F')))
									{
										// process the first digit
										if ((ptr[0] >= '0') && (ptr[0] <= '9'))
										{
											ch = ptr[0] - '0';
										}
										else
										{
											if ((ptr[0] >= 'A') && (ptr[0] <= 'F'))
											{
												ch = ptr[0] - 'A' + 10;
											}
										}

										ptr++;

										// get the next digit
										if ((ptr[0] >= '0') && (ptr[0] <= '9'))
										{
											ch = (ch << 4) + ptr[0] - '0';
										}
										else
										{
											if ((ptr[0] >= 'A') && (ptr[0] <= 'F'))
											{
												ch = (ch << 4) + ptr[0] - 'A' + 10;
											}
										}
									}
								}

								// check for a value less than a blank
								if (ch < 32)
								{
									// replace with a blank
									ch = ' ';
								}

								// skip the number of characters we used
								i += strlen(tempSeq) + 2;
							}
							else
							{
								// didn't find semicolon - leave alone
								ch = '#';
								i++;
							}
						}
						else
						{
							// didn't recognize, leave it alone
							ch = '&';
							i++;
						}
					}
				}
			}

			// replace any escape sequences with the appropriate character
			valStr[j++] = ch;
		}
		else
		{
			if ((valStr[i] != '\r') && (valStr[i] != '\n') && (valStr[i] != '\t'))
			{
				valStr[j++] = valStr[i++];
			}
			else
			{
				valStr[j++] = '.';
				i++;
			}
		}
	}

	valStr[j] = 0;
}

char * findEndBracket(char *tempptr)

{
	while ((tempptr[0] != '>') && (tempptr[0] != 0))
	{
		tempptr++;
	}

	return tempptr;
}

void findXmlValue(const char *ptr, const char *srch, char *result, const int maxlen)

{
	char *tempptr;
	int	i;

	tempptr = strstr((char *)ptr, srch);
	if (tempptr != NULL)
	{
		// Found beginning of XML, now isolate the value
		tempptr += strlen(srch);

		// find the ending bracket
		tempptr = findEndBracket(tempptr);

		// skip the end bracket
		tempptr++;

		// start to gather the data
		i = 0;
		while ((tempptr[0] != '<') && (i < maxlen - 1) &&
				(tempptr[0] >= ' '))
		{
			result[i++] = tempptr++[0];
		}

		// terminate the string
		result[i] = 0;

		// now, set the variable
		removeEscSeq(result);
	}
}

//////////////////////////////////////////////////
//
// Routine to find a specific character in the data
//
//////////////////////////////////////////////////

int findDelim(const unsigned char *datain, 
			  const int maxchar, 
			  const unsigned char delim,
			  const unsigned char escape)

{
	int		count=maxchar;
	int		i=0;
	bool	done=false;

	do
	{
		while ((i < maxchar) && (datain[i] != delim))
		{
			i++;
		}

		if (i == maxchar)
		{
			done = true;
		}
		else
		{
			if (datain[i - 1] == escape)
			{
				i++;
			}
			else
			{
				done = true;
			}
		}
	} while (!done);

	if (i < maxchar)
	{
		count = i;
		count++;
	}

	return count;
}

//////////////////////////////////////////////////
//
// Routine to find a specific character in the data
//
//////////////////////////////////////////////////

int findDelimW(const wchar_t *datain, 
			   const int maxchar, 
			   const wchar_t delim,
			   const wchar_t escape)

{
	int		count=maxchar;
	int		i=0;
	bool	done=false;

	do
	{
		while ((i < maxchar) && (datain[i] != delim))
		{
			i++;
		}

		if (i >= maxchar)
		{
			done = true;
		}
		else
		{
			if (datain[i - 1] == escape)
			{
				i++;
			}
			else
			{
				done = true;
			}
		}
	} while (!done);

	if (i < maxchar)
	{
		count = i;
		count++;
	}

	return count;
}

///////////////////////////////////////////////////////////////////
//
// Routine to replace certain special characters with their
// respective escape sequences, as follows:
//
// & &amp;
// " &quot;
// < &gt;
// > &lt;
// ' &apos;
//
///////////////////////////////////////////////////////////////////

int insertEscChars(char * output, const char * input, int length)

{
	char	ch;
	int		i=0;
	int		j=0;

	while (i < length)
	{
		ch = input[i];
		switch (ch)
		{
		case '&':
			{
				output[j++] = '&';
				output[j++] = 'a';
				output[j++] = 'm';
				output[j++] = 'p';
				output[j++] = ';';
				break;
			}
		case '<':
			{
				output[j++] = '&';
				output[j++] = 'l';
				output[j++] = 't';
				output[j++] = ';';
				break;
			}
		case '>':
			{
				output[j++] = '&';
				output[j++] = 'g';
				output[j++] = 't';
				output[j++] = ';';
				break;
			}
		case '"':
			{
				output[j++] = '&';
				output[j++] = 'q';
				output[j++] = 'u';
				output[j++] = 'o';
				output[j++] = 't';
				output[j++] = ';';
				break;
			}
		case '\'':
			{
				output[j++] = '&';
				output[j++] = 'a';
				output[j++] = 'p';
				output[j++] = 'o';
				output[j++] = 's';
				output[j++] = ';';
				break;
			}
		default:
			{
				output[j++] = ch;
			}
		}

		i++;
	}

	// terminate the result string
	output[j] = 0;

	return j;
}

void replaceChars(const CString &value, char *valStr)

{
	int j = insertEscChars(valStr, (LPCTSTR)value, value.GetLength());
}

////////////////////////////////////////////////////
//
// Routine to extract an attribute name/value pair
//
////////////////////////////////////////////////////

char * extractAttribute(char *VarName, char *VarValue, char *Ptr)

{
	char	*tempValue = NULL;
	char	*tempVarName;
	char	*tempVarValue;
	char	quotechar;

	// initialize the return variables to nulls
	VarName[0] = 0;
	VarValue[0] = 0;

	// check if we can find the beginning of a variable name
	// by skipping any blanks or newlines, etc
	while ((Ptr[0] > 0) && (Ptr[0] <= ' '))
	{
		Ptr++;
	}

	// did we find one?
	if (Ptr[0] > 0)
	{
		// skip the first character of the name to look for the equal sign
		tempValue = Ptr;
		tempVarName = VarName;
		while ((tempValue[0] > 0) && (tempValue[0] != '='))
		{
			tempVarName++[0] = tempValue++[0];
		}

		// terminate the variable name string
		tempVarName[0] = 0;

		// get rid of any trailing blanks on the attribute name
		Rtrim(VarName);

		if (tempValue[0] == '=')
		{
			// found an equal sign
			// terminate the attribute name
			//tempValue[0] = 0;
			tempValue++;

			// skip any blanks
			tempValue = skipBlanks(tempValue);

			tempVarValue = VarValue;

			// now, extract the value, looking for one of
			// several things, namely:
			// A blank, if the value does not start with single or double quotes
			// A NL character, if the value does not start with a quote character
			// A matching quote character, not followed by another quote character
			if ((tempValue[0] == '"') || (tempValue[0] == '\''))
			{
				// Remember what kind of quote we found
				quotechar = tempValue[0];

				//skip the leading double quote
				tempValue++;

				while (1)
				{
					if ((tempValue[0] == quotechar) && (tempValue[1] != quotechar))
					{
						break;
					}

					if (tempValue[0] == quotechar)
					{
						// skip the second double quote
						tempValue++;
					}

					// keep building the value string
					if (tempValue[0] >= ' ')
					{
						tempVarValue++[0] = tempValue++[0];
					}
					else
					{
						tempVarValue++[0] = ' ';
						tempValue++;
					}
				}

				// skip the trailing double quote or other character
				tempValue++;

				// terminate the string properly
				tempVarValue[0] = 0;
			}
			else
			{
				while ((tempValue[0] > ' ') && (tempValue[0] != ','))
				{
					tempVarValue++[0] = tempValue++[0];
				}

				// terminate the value string
				tempVarValue[0] = 0;
			}

			// convert any Escape sequences
			removeEscSeq(VarValue);

			// skip any trailing commas or blanks
			while ((tempValue[0] > 0) && ((tempValue[0] <= ' ') || (tempValue[0] == ',')))
			{
				tempValue++;
			}
		}
	}

	return tempValue;
}

////////////////////////////////////////////////////
//
// Routine to parse any attribute values that
//  come with an XML element
//
////////////////////////////////////////////////////

void processAttributes(char *xmlVarName, char *varName, char *tempVarAttr, char *xmlAttrStr)

{
	char	*tempPtr;
	char	*tempVarName = (char *)rfhMalloc(8192,"XMLSUBVN");
	char	*tempVarValue = (char *)rfhMalloc(8192,"XMLSUBVV");
	char	tempName[4];

	// initialize a pointer to the beginning of the attribute list
	tempPtr = tempVarAttr;

	// Find the first attribute name and value
	tempPtr = extractAttribute(tempVarName, tempVarValue, tempPtr);

	while ((tempVarName[0] != 0) && (tempVarName[0] != '/'))
	{
		// check for a data type attribute
		tempName[0] = tempVarName[0];
		tempName[1] = tempVarName[1];
		tempName[2] = tempVarName[2];
		tempName[3] = 0;
		_strupr(tempName);

		// Is this a data type attribute or a user attribute?
		if (strcmp(tempName, "DT") != 0)
		{
			// build the variable name from two names
			// check if there are any higher level names
			if (strlen(xmlVarName) > 0)
			{
				strcat(xmlAttrStr, xmlVarName);
				strcat(xmlAttrStr, ".");
			}

			// check for the variable name
			if ((varName != NULL) && (varName[0] != 0))
			{
				strcat(xmlAttrStr, varName);
				strcat(xmlAttrStr, ".");
			}

			strcat(xmlAttrStr, "(XML.attr)");
			strcat(xmlAttrStr, tempVarName);
			strcat(xmlAttrStr, "='");
			strcat(xmlAttrStr, tempVarValue);
			strcat(xmlAttrStr, "'");
			strcat(xmlAttrStr, "\r\n");
		}

		tempPtr = extractAttribute(tempVarName, tempVarValue, tempPtr);
	}

	if (tempVarName)
		rfhFree(tempVarName);
	if (tempVarValue)
		rfhFree(tempVarValue);
}

int setEscChar(unsigned char *bufferin, unsigned char *bufferout)

{
	int				length=1;
	int				i;
	int				j;
	unsigned char	tempBuf[16];
	unsigned char	ch;

	// initialize the character to an ampersand in case we don't recognize the sequence
	ch = '&';

	// get the data in a temporary buffer
	memcpy(tempBuf, bufferin + 1, 5);
	tempBuf[5] = 0;
	_strupr((char *)tempBuf);

	if (memcmp(tempBuf, "LT;", 3) == 0)
	{
		ch = '<';
		length = 4;
	}

	if (memcmp(tempBuf, "GT;", 3) == 0)
	{
		ch = '>';
		length = 4;
	}

	if (memcmp(tempBuf, "AMP;", 4) == 0)
	{
		ch = '&';
		length = 5;
	}

	if (memcmp(tempBuf, "APOS;", 5) == 0)
	{
		ch = '\'';
		length = 6;
	}

	if (memcmp(tempBuf, "QUOT;", 5) == 0)
	{
		ch = '"';
		length = 6;
	}

	if ('#' == bufferin[0])
	{
		// arbitrary character
		// there can be from 1 to 13 digits
		// we will look for a trailing semi-colon 
		// and remove it, and then convert the
		// remaining characters to an integer
		// first, isolate the number
		i = 1;
		j = 0;
		while ((j < sizeof(tempBuf) - 1) && 
			   (bufferin[i] != 0) &&
			   (bufferin[i] != ';'))
		{
			tempBuf[j++] = bufferin[i++];
		}

		// terminate the string
		tempBuf[j] = 0;

		// did we find the semicolon in the original data?
		if (';' == bufferin[i])
		{
			// set the length of the total item
			length = i + 1;

			// check for a hex value
			if (tempBuf[0] !='X')
			{
				// not hex - get the integer value
				ch = atoi((char *)tempBuf);
			}
			else
			{
				// hex value
				// skip any leading zeros
				j = 0;
				while ('0' == tempBuf[j])
				{
					j++;
				}

				// did we find anything besides zeros?
				if (((tempBuf[j] >= '0') && (tempBuf[j] <= '9')) ||
					((tempBuf[j] >= 'A') && (tempBuf[j] <= 'F')))
				{
					// process the first digit
					ch = getHexCharValue(tempBuf[j]);
					j++;

					// get the next digit
					if (((tempBuf[j] >= '0') && (tempBuf[j] <= '9')) ||
						((tempBuf[j] >= 'A') && (tempBuf[j] <= 'F')))
					{
						ch <<= 4;
						ch |= getHexCharValue(tempBuf[j]);
					}
				}
				else
				{
					ch = ' ';
				}
			}

			// check for a value less than a blank
			if (ch < 32)
			{
				// replace with a blank
				ch = ' ';
			}
		}
	}

	// insert the appropriate character in the output buffer
	bufferout[0] = ch;

	return length;
}

char * processTag(char *ptr, const char *tagName, char *dataArea, int maxLen, int *found)

{
	int		i;
	int		len;
	char	*retPtr=ptr;

	if (1 == (*found) || (strlen(tagName) < 3))
	{
		return ptr;
	}

	// skip any blanks
	ptr = skipBlanks(ptr);

	// make sure the first character is a begin bracket
	if (ptr[0] != tagName[0])
	{
		// first character does not match
		return ptr;
	}

	// skip the beginning bracket and any intervening blanks
	ptr = skipBlanks(ptr + 1);

	// get the length of the tag name
	len = strlen(tagName);

	// now check if we have found the next part of the tag name
	if (memcmp(ptr, tagName + 1, len - 2) != 0) 
	{
		// not the right folder so return
		return retPtr;
	}

	// skip the tag name characters
	ptr += len - 2;

	// check what the first character after the folder name is
	// if it is not whitespace, a slash or a greater than sign we have not found the tag name
	// first check for a slash character
	if (ptr[0] <= ' ')
	{
		// whitespace character found
		// skip the white space character
		ptr = skipBlanks(ptr);

		// continue until we find the ending bracket or a slash
		while ((ptr[0] > 0) && (ptr[0] != '>') && (ptr[0] != '/'))
		{
			// skip this character
			ptr++;
		}

		// check for a slash, indicating an empty folder
		if ('/' == ptr[0])
		{
			// found a slash - skip the slash and exit
			ptr++;
			return ptr;
		}
		else if ('>' == ptr[0]) // check for an end bracket
		{
			// end of the name tag
			// skip the bracket
			ptr++;
		}
	}
	else if ('/' == ptr[0])
	{
		// found a slash - skip the slash and exit
		ptr++;
		(*found) = 1;
		return ptr;
	}
	else if ('>' == ptr[0])
	{
		// skip the ending bracket
		ptr++;
	}
	else
	{
		// at this point the next character is not whitespace, a slash or an ending bracket
		// this means that the tag name has more characters than we are looking for
		// therefore this is not a match so return.
		return retPtr;
	}

	// skip any blanks at the beginning of the value
	ptr = skipBlanks(ptr);

	// indicate that we found the tag
	(*found) = 1;

	// capture the value, begin careful to not exceed the maximum length
	// of the allocated area
	i = 0;
	while ((ptr[0] != '<') && (i < maxLen))
	{
		dataArea[i] = ptr[0];
		ptr++;
		i++;
	}

	// terminate the value string
	dataArea[i] = 0;

	// remove any escape characters
	removeEscSeq(dataArea);

	// find the end of the terminating tag
	while ((ptr[0] != 0) && (ptr[0] != '>'))
	{
		ptr++;
	}

	// did we find an end bracket?
	if ('>' == ptr[0])
	{
		// skip the end bracket
		ptr++;
	}

	return ptr;
}

///////////////////////////////////////////////////////
//
// Routine to add field to the RFH1 header
//  this includes checking for embedded blanks and
//  enclosing the string in double quotes if necessary.
//
///////////////////////////////////////////////////////

char * buildV1String(char *ptr, const char *name, LPCTSTR value, int curLen)

{
	char *	blankptr;

	// check if the value has any embedded blanks
	blankptr = strchr((char *)value, ' ');

	if (curLen != 0)
	{
		strcat(ptr, " ");
	}

	strcat(ptr, name);
	strcat(ptr, " ");

	if (blankptr != NULL)
	{
		// double quotes are required
		strcat(ptr, "\"");
	}

	// insert the value
	strcat(ptr, value);

	if (blankptr != NULL)
	{
		// double quotes are required
		strcat(ptr, "\"");
	}

	ptr += strlen(ptr);

	return ptr;
}
