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

#ifndef _CommonSubs_timesubs_h
#define _CommonSubs_timesubs_h

#ifndef WIN32
#include <sys/time.h>
#endif

/**********************************************************/
/* MY_TIME_T                                              */
/* The definition of this data type is platform specific. */
/**********************************************************/
#ifdef WIN32
typedef unsigned __int64 MY_TIME_T;
#else
typedef struct timeval MY_TIME_T;
#endif

void GetTime(MY_TIME_T *tv);
void clearTime(MY_TIME_T *time);
int64_t timeToMicroSecs(MY_TIME_T time);
int64_t DiffTime(MY_TIME_T start, MY_TIME_T end);
void formatTimeDiff(char * result, int64_t diff);
void performTimerCheck();
void formatTimeSecsNoColons(char * timeOut, time_t timeIn);
void formatTimeSecs(char * timeOut, time_t timeIn);
void formatTimeDiffSecs(char * result, int64_t time);
void InitializeTimer();
int getSecs(int time);
#endif
