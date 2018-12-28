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
/*   qsubs.h - header file for qsubs.c                              */
/*                                                                  */
/********************************************************************/
#ifndef _CommonSubs_qsubs_h
#define _CommonSubs_qsubs_h

void checkerror(const char *mqcalltype, MQLONG compcode, MQLONG reason, const char *resource);
void connect2QM(char * qmname, PMQHCONN qm, PMQLONG cc, PMQLONG reason);
void clientConnect2QM(char * qmname, PMQHCONN qm, int *maxMsgLen, PMQLONG cc, PMQLONG reason);
void formatTime(char *timeOut, char *timeIn);
void issueReply(MQHCONN qm, MQLONG	report, int *uow, char * msgId, PUTPARMS * parms);
void translateMQMD(void * mqmd, int datalen);
int checkMQMD(void * mqmdPtr, int datalen);
int checkAndXlateMQMD(void * mqmdPtr, int length);
void setContext(MQMD2 * mqmd);
#endif
