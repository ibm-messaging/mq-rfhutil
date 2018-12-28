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

/* parmline.h header file used in main programs */
/* a common parameters area is used by all the  */
/* utilities.                                   */

#ifndef _parmline_
#define _parmline_

#define	MQMD_NO					0			/* ignore saved MQMDs in files */
#define MQMD_YES				1			/* use saved MQMDs if found at front of message */

#define	RFH_NO					0
#define RFH_V1					1
#define RFH_V2					2
#define RFH_XML					3
#define RFH_AUTO				4

#define RFHSTRIP				1
#define RFHNOSTRIP				0

#define TIMESTAMP_NO			0
#define TIMESTAMP_YES			1
#define TIMESTAMP_AUTO			2

#define TIMESTAMP_FORMAT_BIN	0
#define TIMESTAMP_FORMAT_HEX	1

#define SKIPDATAFILES			0
#define READDATAFILES			1

#define MAX_RFH_DATA_LEN	64

/* some default values for parameters */
#define TIME_OUT_DEF		120
#define DEF_SYNC			25
#define MAX_SYNC_ALLOW		1000

/* default buffer size is 16MB but will be adjusted to maximum length supported */
#define MAX_MESSAGE_LENGTH	256 * 65536

/* maximum field lengths */
#define MAX_TOPIC_LEN		1024
#define MAX_SUBPOINT_LEN	1024
#define MAX_FILTER_LEN		1024
#define MAX_PUBTIME_LEN		32
#define MAX_JMS_DEST		256
#define MAX_JMS_REPLY		256
#define MAX_JMS_CORRELID	32
#define MAX_JMS_GROUPID		32
#define MAX_JMS_EXPIRE		32
#define MAX_JMS_DELMODE		16
#define MAX_JMS_SEQ			16
#define MAX_JMS_PRIORITY	16

#define LATENCYHEADER		"<usr><ts>0000000000000000</ts></usr>"
#define LATENCYHEADEROFS	9

/**************************************************************/
/*                                                            */
/* Parameter values from parameter file.                      */
/*                                                            */
/**************************************************************/

typedef struct {
	/* global error switch */
	int			err;

	/* verbose switch */
	int			verbose;

	/* silent mode indicator - do not issue normal operational messages */
	int			silent;

	/* total memory used               */
	size_t		memUsed;		

	/* Counters and statistics         */
	int			fileCount;				/* number of files read            */
	int			mesgCount;				/* number of messages found        */

	/* name of the parameters file */
	char		parmFilename[512];

	/* delimiter value, if specified */
	int			iDelimiterLen;
	int			delimiterIsHex;
	size_t		delimiterLen;
	char		delimiter[32];

	/* Universal parameters */
	int			persist;
	int			priority;
	int			batchSize;				/* number of messages in a unit of work */
	int			saveBatchSize;
	int			saveThinkTime;
	int			qdepth;
	int			qmax;
	int			sleeptime;
	int			tune;
	int			maxtime;				/* maximum number of seconds for MQTimes3 to wait for first message */
	int			saveMQMD;
	int			readOnly;
	int			indivFiles;
	int			subLevel;				/* subscription level */

	/* indicators if message id, correlation id or group id is to be used by mqcapture */
	int			msgidSet;
	int			correlidSet;
	int			groupidSet;

	/* Queue and queue manager names - universal */
#ifdef MQCLIENT
	char		qmname[512];
	char		saveQMname[512];
#else
	char		qmname[MQ_Q_MGR_NAME_LENGTH + 4];
	char		saveQMname[MQ_Q_MGR_NAME_LENGTH + 4];
#endif
	char		qname[MQ_Q_NAME_LENGTH + 4];
	char		replyQname[MQ_Q_NAME_LENGTH + 4];			/* used by MQTimes3 for mandatory replies */
	char		replyQMname[MQ_Q_MGR_NAME_LENGTH + 4];		/* used by MQTimes3 for mandatory replies */
	char		saveQname[MQ_Q_NAME_LENGTH + 4];
	char		saveQReply[MQ_Q_NAME_LENGTH + 4];
	char		remoteQM[MQ_Q_MGR_NAME_LENGTH + 4];

	/* MQMD parameters - per message */
	int			useMQMD;
	int			ignoreMQMD;
	int			newMsgId;
	int			logicalOrder;
	int			codepage;
	int			encoding;
	int			expiry;
	int			msgtype;
	int			feedback;
	int			report;
	int			GetByCorrelId;			/* used by MQLatency */
	int			acctTokenSet;
	int			inGroup;
	int			lastGroup;
	int			fileAsGroup;
	int			formatSet;

	/* MQMD parameters - per message */
	char		msgid[MQ_GROUP_ID_LENGTH + 4];
	char		correlid[MQ_CORREL_ID_LENGTH + 4];
	char		groupid[MQ_GROUP_ID_LENGTH + 4];
	char		msgformat[MQ_FORMAT_LENGTH + 4];
	char		replyQ[MQ_Q_NAME_LENGTH + 4];
	char		replyQM[MQ_Q_MGR_NAME_LENGTH + 4];
	char		userId[MQ_USER_ID_LENGTH + 4];
	char		accountingToken[MQ_ACCOUNTING_TOKEN_LENGTH + 4];


	/* selector - used by mqcapture */
	char		selector[MQ_SELECTOR_LENGTH + 4];	/* Additional message filter */


	/* field to remember the current group id */
	char		saveGroupId[MQ_GROUP_ID_LENGTH + 4];

	/* parameters used by mqcapture */
	int			striprfh;			/* whether to strip MQ headers before saving the data */
	int			addTimeStamp;		/* insert a timestamp at the end of the file name */
	int			appendFile;			/* append messages to output file */

	/* think time after message is written (in milliseconds) */
	int			thinkTime;

	/* Report progress after this number of messages */
	int			reportEvery;				/* used by MQPut2 */
	int			reportEverySecond;
	int			reportInterval;				/* used by MQTimes3 */

	/* Maximum time to wait for a reply message - default is 5 seconds */
	int			maxWaitTime;
	int			waitTime;					/* wait time in seconds for MQGET - used by mqcapture */

	/* set time stamp - used by MQPut2 and MQTimes2 or MQTimes3 */
	int			setTimeStamp;
	int			timeStampOffset;
	int			timeStampInAccountingToken;
	int			timeStampInGroupId;
	int			timeStampInCorrelId;
	int			timeStampUserProp;

	/* fields used by mqreply */
	int			resendRFHusr;
	int			resendRFHjms;
	int			resendRFHmcd;
	int			resendRFHpsc;
	int			resendRFHpscr;
	int			resendRFH;

	/* Indication that set context must be used on MQOpen */
	int			foundMQMD;				/* found at least one MQMD in file */

	/* write each message once, ignoring the count parameter */
	int			writeOnce;	

	/* switch to determine if reply queue should be purged before sending any messages */
	int			purgeQ;

	/* RFH fields */
	char		rfhdomain[MAX_RFH_DATA_LEN + 4];
	char		rfhset[MAX_RFH_DATA_LEN + 4];
	char		rfhtype[MAX_RFH_DATA_LEN + 4];
	char		rfhfmt[MAX_RFH_DATA_LEN + 4];
	char		rfhappgroup[MAX_RFH_DATA_LEN + 4];
	char		rfhformat[MAX_RFH_DATA_LEN + 4];
	char		rfh_PscReplyQ[MQ_Q_NAME_LENGTH + 4];
	char		rfh_PscReplyQM[MQ_Q_MGR_NAME_LENGTH + 4];
	char		rfh_topic1[MAX_TOPIC_LEN];
	char		rfh_topic2[MAX_TOPIC_LEN];
	char		rfh_topic3[MAX_TOPIC_LEN];
	char		rfh_topic4[MAX_TOPIC_LEN];
	char		rfh_subpoint[MAX_SUBPOINT_LEN];
	char		rfh_filter[MAX_FILTER_LEN];
	char		rfh_pubtime[MAX_PUBTIME_LEN];
	char		rfh_jms_dest[MAX_JMS_DEST];
	char		rfh_jms_reply[MAX_JMS_REPLY];
	char		rfh_jms_correlid[MAX_JMS_CORRELID];
	char		rfh_jms_groupid[MAX_JMS_GROUPID];
	char		rfh_jms_expire[MAX_JMS_EXPIRE];
	char		rfh_jms_delmode[MAX_JMS_DELMODE];
	char		rfh_jms_seq[MAX_JMS_SEQ];
	char		rfh_jms_priority[MAX_JMS_PRIORITY];

	int			rfhccsid;
	int			rfhencoding;
	int			nameValueCCSID;
	int			rfh_psc_seqno;
	int			rfh_psc_reqtype;
	int			rfh_jms_reqtype;
	int			rfh_psc_local;
	int			rfh_psc_newonly;
	int			rfh_psc_otheronly;
	int			rfh_psc_ondemand;
	int			rfh_psc_retainpub;
	int			rfh_psc_isretainpub;
	int			rfh_psc_correlid;
	int			rfh_psc_deregall;
	int			rfh_psc_infretain;
	int			rfh_psc_pers_type;

	/* pointers to various RFH areas */
	char *			rfh_usr;
	char *			rfh_jms;
	char *			rfh_mcd;
	char *			rfh_psc;
	char *			rfh_pscr;
	unsigned int	max_usr;
	unsigned int	max_jms;
	unsigned int	max_mcd;
	unsigned int	max_psc;
	unsigned int	max_pscr;

	/* counters and statistics */
	/* number of bytes written */
	int64_t			totcount;
	int64_t			saveTotcount;
	int64_t			msgwritten;
	int64_t			byteswritten;
	int64_t			noMatches;
	int64_t			replyCount;

	/* used by mqtimes3 */
	size_t			fileSizePAN;
	size_t			fileSizeNAN;
	char			*fileDataPAN;
	char			*fileDataNAN;

	/* indicate already tried to handle 2068 error on MQINQ */
	int				reopenInq;

	/* total number of bytes read */
	size_t			totMsgLen;	

	/* number of messages read */
	int64_t			msgsRead;

	/* publish level and other publication options */
	int				pubLevel;
	int				retained;
	int				local;
	int				notOwn;
	int				suppressReply;
	int				warnNoSub;

	/* receive options */
	int				maxmsglen;
	int				drainQ;				/* drain messages from queue when starting */

	/* reply to queue handle */
	MQHOBJ			hReplyQ;

	/* message id from last message that was written */
	/* used for get by correl id */
	MQBYTE24		savedMsgId;

	/* topic fields for pub/sub support */
	char			topicStr[MQ_TOPIC_STR_LENGTH + 8];
	char			saveTopicStr[MQ_TOPIC_STR_LENGTH + 8];
	char			topicName[MQ_TOPIC_NAME_LENGTH + 8];
	char			saveTopicName[MQ_TOPIC_NAME_LENGTH + 8];

	/* RFH parameters - per message */
	int				rfh;
	int				rfhlength;
	unsigned char	rfhdata[128 * 1024];

	/* name of the output file - used by capture programs */
	char			outputFilename[756];

	/* name of the log file */
	char			logFileName[756];

	/* reply data - used by MQReply */
	int				useInputAsReply;
	char			replyFilename[512];
} PUTPARMS;

int processParmLine(char * ptr, PUTPARMS * parms);
int processFirstUsrLine(char * ptr, PUTPARMS * parms, int readFiles);
int processUsrLine(char * ptr, int foundit, PUTPARMS * parms, int readFiles);
void processArgs(int argc, char **argv, PUTPARMS *parms);
void initializeParms(PUTPARMS *parms, size_t parmSize);
void processOverrides(PUTPARMS *parms);
void getRFHUsrTimeStamp(char *msg, int msgLen, MY_TIME_T *startTime);

#endif	/* _parmline_ */
