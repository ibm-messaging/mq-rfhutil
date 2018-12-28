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
/*   putparms.h - Input parameter file processing subroutines       */
/*                                                                  */
/********************************************************************/

#ifndef _CommonSubs_putparms_h
#define _CommonSubs_putparms_h
#define INIT_COUNT	32			/* number of messages written initially */

#define MIN_SLEEP	0			/* 0 millisecond */
#define MAX_SLEEP	10000		/* 10 seconds */
#define DEF_SLEEP	10			/* Default sleep time (10 milliseconds) */
#define MIN_THINK	0			/* 0 millisecond */
#define MAX_THINK	30000		/* 30 seconds */

typedef struct {
	void*			nextfile;
	char			*dataptr;
	void			*mqmdptr;
	const char		*userDataPtr;
	char			*userPropLatencyPtr;
	size_t			rfhlen;
	size_t			length;
	int				hasMQMD;
	int				hasRFH;
	int				useFileRFH;
	int				newMsgId;
	int				Codepage;
	int				Encoding;
	int				Expiry;
	int				Persist;
	int				Priority;
	int				Report;
	int				Msgtype;
	int				Feedback;
	int				GetByCorrelId;			/* used by MQLatency */
	int				CorrelIdSet;
	int				GroupIdSet;
	int				AcctTokenSet;
	int				inGroup;
	int				lastGroup;
	int				FormatSet;
	int				setTimeStamp;
	int				timeStampOffset;
	int				timeStampInAccountingToken;
	int				timeStampInGroupId;
	int				timeStampInCorrelId;
	int				timeStampUserProp;
	int				thinkTime;
	char			*acqStorAddr;
	char			CorrelId[MQ_CORREL_ID_LENGTH + 8];
	char			GroupId[MQ_GROUP_ID_LENGTH + 8];
	char			Format[MQ_FORMAT_LENGTH + 8];
	char			ReplyQ[MQ_Q_NAME_LENGTH + 8];
	char			ReplyQM[MQ_Q_MGR_NAME_LENGTH + 8];
	char			UserId[MQ_USER_ID_LENGTH + 4];
	char			AccountingToken[MQ_ACCOUNTING_TOKEN_LENGTH + 8];
}	FILEPTR;

FILEPTR * processParmFile(char * parmFileName, PUTPARMS * parms, int readFiles);
int readFileData(const char *filename, size_t *length, char ** dataptr, PUTPARMS * parms);
const char * scanForDelim(const char * msgdata, const size_t datalen, PUTPARMS *parms);
void createNextFileName(const char *fileName, char *newFileName, int fileCount);
void appendTimeStamp(PUTPARMS * parms);
#endif
