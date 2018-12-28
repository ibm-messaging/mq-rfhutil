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

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include <ctype.h>
#include <time.h>

#ifdef WIN32
#include "windows.h"
#else
#include <sys/time.h>
#endif

/* include for 64-bit integer definitions */
#include "int64defs.h"

/* high precision timer subroutines */
#include "timesubs.h"
#include "comsubs.h"

#ifdef _WIN32
	/* Results of QueryPerformanceFrequency - done only once to reduce overhead */
	static __int64	freq=0;
#endif

void InitializeTimer()

{
#ifdef WIN32
	/* get the counts per second as a LARGE_INTEGER if running under windows */
	if (!QueryPerformanceFrequency((LARGE_INTEGER *)&freq))
	{
		/* didn't work - set to zero */
		freq = 0;

		/* tell what we are doing about it */
		Log("*****WARNING! - No performance counter on hardware");
	}
#endif
}

/*********************************************************/
/* GetTime - get high precision time.                    */
/*********************************************************/

void GetTime(MY_TIME_T *tv)
{
#ifdef _WIN32
	LARGE_INTEGER	count;

	if (!QueryPerformanceCounter(&count))
	{
		(*tv) = 0;
	}

	(*tv) = count.QuadPart;
#else
	gettimeofday(tv, 0);
#endif
}

/* clear the time to zero */
void clearTime(MY_TIME_T *time)

{
#ifdef _WIN32
	(*time) = 0;
#else
	(*time).tv_sec = 0;
	(*time).tv_usec = 0;
#endif
}

/* return the time in microseconds */
int64_t timeToMicroSecs(MY_TIME_T time)
{
	int64_t result=0;

#ifdef WIN32
	if (freq > 0)
	{
		result = (time * 1000 * 1000) / freq;
	}
#else
	result = time.tv_sec;
	result = result * 1000 * 1000;
	result = result + time.tv_usec;
#endif

	return result;
}

/*********************************************************/
/* DiffTime - difference in microseconds between two     */
/*  high precision times.                                */
/*********************************************************/

int64_t DiffTime(MY_TIME_T start, MY_TIME_T end)
{
#ifdef _WIN32
	if (0 == freq)
	{
		InitializeTimer();
	}

	if (freq > 0)
	{
		return ((end - start) * 1000 * 1000) / freq;
	}
	else
	{
		/* unable to get frequency - don't divide by zero */
		return 0;
	}
#else
	int64_t	val;

	/* is the ending microsecond value less than the starting microsecond value? */
	if (end.tv_usec < start.tv_usec)
	{
		/* borrow a second (1000000 microsecs) from the end time */
		val = 1000000 + end.tv_usec - start.tv_usec;

		/* account for the borrowed second */
		end.tv_sec--;
	}
	else
	{
		/* just subtract the two microsecond value */
		val = end.tv_usec - start.tv_usec;
	}

	val += (end.tv_sec - start.tv_sec)*1000000;

 /* return the difference in microseconds */
	return val;
#endif
}

/*********************************************************/
/* formatTimeDiff - format a number of microseconds      */
/*  resulting from a difference between two times.       */
/*********************************************************/

void formatTimeDiff(char * result, int64_t diff)

{
	int64_t	secs=0;
	int64_t	usecs=0;
	int		i;
	int		slen;

	result[0] = 0;

	/* divide the time into seconds and microseconds */
	secs = diff / 1000000;
	usecs = diff % 1000000;

#ifdef _WIN32
	/* format the results */
	sprintf(result, "%I64d.%6.6I64d", secs, usecs);
#else
	/* format the results */
	sprintf(result, "%lld.%6.6lld", secs, usecs);
#endif

	/* replace any blanks with zeros */
	slen = strlen(result);
	for (i = 0; i < slen; i++)
	{
		if (' ' == result[i])
		{
			result[i] = '0';
		}
	}
}

/*********************************************************/
/*                                                       */
/* Perform a timer check and then exit.                  */
/*                                                       */
/*********************************************************/

void performTimerCheck()

{
#ifdef WIN32
	MY_TIME_T	time1;
	MY_TIME_T	time2;
	int64_t		totalLatency;

	/* get the time and sleep for ten msec */
	GetTime(&time1);
	Sleep(10);

	/* calculate and display the time */
	GetTime(&time2);
	totalLatency = DiffTime(time1, time2);
	Log("\n10 millisecond sleep time is %I64d microseconds", totalLatency);
	
	/* get the time and sleep for one hundred millisecs */
	GetTime(&time1);
	Sleep(100);

	/* calculate and display the time */
	GetTime(&time2);
	totalLatency = DiffTime(time1, time2);
	Log("\n100 millisecond sleep time is %I64d microseconds", totalLatency);
	
	/* get the time and sleep for one second */
	GetTime(&time1);
	Sleep(1000);

	/* calculate and display the time */
	GetTime(&time2);
	totalLatency = DiffTime(time1, time2);
	Log("\nOne second sleep time is %I64d microseconds", totalLatency);
	
	/* get the time and sleep for two seconds */
	GetTime(&time1);
	Sleep(2000);

	/* calculate and display the time */
	GetTime(&time2);
	totalLatency = DiffTime(time1, time2);
	Log("\nTwo second sleep time is %I64d microseconds", totalLatency);
#else
	MY_TIME_T	time1;
	MY_TIME_T	time2;
	int64_t		totalLatency;

	/* get the time and sleep for ten msec */
	GetTime(&time1);
	usleep(10000);

	/* calculate and display the time */
	GetTime(&time2);
	totalLatency = DiffTime(time1, time2);
	Log("\n10 millisecond sleep time is %lld microseconds", totalLatency);
	
	/* get the time and sleep for one hundred millisecs */
	GetTime(&time1);
	usleep(100000);

	/* calculate and display the time */
	GetTime(&time2);
	totalLatency = DiffTime(time1, time2);
	Log("\n100 millisecond sleep time is %lld microseconds", totalLatency);
	
	/* get the time and sleep for one second */
	GetTime(&time1);
	usleep(1000000);

	/* calculate and display the time */
	GetTime(&time2);
	totalLatency = DiffTime(time1, time2);
	Log("\nOne second sleep time is %lld microseconds", totalLatency);
	
	/* get the time and sleep for two seconds */
	GetTime(&time1);
	usleep(2000000);

	/* calculate and display the time */
	GetTime(&time2);
	totalLatency = DiffTime(time1, time2);
	Log("\nTwo second sleep time is %lld microseconds", totalLatency);
#endif
}

/**************************************************************/
/*                                                            */
/* Subroutine to format a time.                               */
/*                                                            */
/* The input time should be a time_h value obtained from      */
/*  a time function call.  This call gives the number of      */
/*  seconds since January 1, 1970 as a 32-bit long integer.   */
/*                                                            */
/* The time is formatted as hhmmss.  There are no separators. */
/*                                                            */
/**************************************************************/

void formatTimeSecsNoColons(char *timeOut, time_t timeIn)

{
	char	tempTime[32];

	strcpy(tempTime, ctime(&timeIn));
	memcpy(timeOut, tempTime + 11, 2);
	memcpy(timeOut+2, tempTime + 14, 2);
	memcpy(timeOut+4, tempTime + 17, 2);
	timeOut[6] = 0;
}

/**************************************************************/
/*                                                            */
/* Subroutine to format a time.                               */
/*                                                            */
/* The input time should be a time_h value obtained from      */
/*  a time function call.  This call gives the number of      */
/*  seconds since January 1, 1970 as a 32-bit long integer.   */
/*                                                            */
/* The time is formatted as hh:mm:ss with colons as           */
/*  separators.                                               */
/*                                                            */
/**************************************************************/

void formatTimeSecs(char *timeOut, time_t timeIn)

{
	char	tempTime[32];

	strcpy(tempTime, ctime(&timeIn));
	memcpy(timeOut, tempTime + 11, 8);
	timeOut[8] = 0;
}

/*********************************************************/
/* formatTimeDiffSecs - format a number of seconds       */
/*  resulting from a difference between two times.       */
/*********************************************************/

void formatTimeDiffSecs(char * result, int64_t time)

{
	int64_t	secs=0;
	int64_t	usecs=0;
	size_t	i;
	char	tempStr[8];

	result[0] = 0;

	/* break the time into seconds and microseconds */
	secs = time / 1000000;
	usecs = time % 1000000;

	/* get the microseconds as a string */
	sprintf(tempStr, FMTI64_66, usecs);

	/* replace any leading blanks with zeros */
	i = 0;
	while ((i < strlen(tempStr)) && (' ' == tempStr[i]))
	{
		tempStr[i] = '0';
		i++;
	}

	/* display the results */
	sprintf(result, FMTI64 ".%s", secs, tempStr);
}

int getSecs(int time)

{
	int		hh;
	int		mm;
	int		ss;

	ss = time % 100;
	mm = ((time - ss) % 10000) / 100;
	hh = (time - mm - ss) / 10000;

	return (hh * 3600) + (mm * 60) + ss;
}
