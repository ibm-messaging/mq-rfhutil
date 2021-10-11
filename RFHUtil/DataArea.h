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

// DataArea.h: interface for the DataArea class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAAREA_H__6861C316_DF2F_4594_BB91_8743A5E7E765__INCLUDED_)
#define AFX_DATAAREA_H__6861C316_DF2F_4594_BB91_8743A5E7E765__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Copybook.h"
#include "Names.h"
#include "cmqc.h"
#include "cmqcfc.h"

// Defined constants
#define MAX_RFH_LENGTH		16 * 1024
#define MAX_FORMAT_NAME		128

#define Q_OPEN_NOT			0
#define Q_OPEN_READ			1
#define Q_OPEN_BROWSE		2
#define Q_OPEN_WRITE		3
#define Q_OPEN_READ_BROWSE	4

#define Q_CLOSE_NONE		0
#define Q_CLOSE_DELETE		1
#define Q_CLOSE_PURGE		2

#define ID_DISPLAY_ASCII	0
#define ID_DISPLAY_EBCDIC	1
#define ID_DISPLAY_HEX		2

#define READQ				0
#define BROWSEQ				1
#define STARTBR				2
#define NOREAD				3

#define MQHEADER_DLQ		0
#define MQHEADER_RFH1		1
#define MQHEADER_RFH2		2
#define MQHEADER_CICS		3
#define MQHEADER_IMS		4

#define LOAD_NAMES_SUBS		0
#define LOAD_NAMES_TOPICS	1
#define LOAD_NAMES_QUEUES	2

#define MQ_PROPS_AS_QUEUE	0
#define MQ_PROPS_NONE		1
#define MQ_PROPS_YES		2
#define MQ_PROPS_RFH2		3
#define MQ_PROPS_COMPAT		4

#define PS_WILDCARD_TOPIC	0
#define PS_WILDCARD_CHAR	1

#define PS_SUB_TYPE_CREATE	0
#define PS_SUB_TYPE_RESUME	1
#define PS_SUB_TYPE_ALTER	2

#define SUB_DISPLAY_ASCII	0
#define SUB_DISPLAY_HEX		1

#define MQ71_SERVER_INST	1
#define MQ71_CLIENT_INST	2

struct NAMESTRUCT
{
	char	id[4];
	void *	nextQM;
	int		count;
	int		ccsid;
	int		qmName;
	Names	*names;
};

typedef struct InstTable {
	void	*next;
	int		version;
	int		release;
	int		type;
	char	VRMF[16];
	char	installation[16];
	char	name[MQ_INSTALLATION_NAME_LENGTH + 8];
	char	filePath[512];
} InstTable;

typedef struct QmgrInstTable {
	void	*next;
	BOOL	isDefault;
	BOOL	isStandby;
	BOOL	isRunning;
	char	name[64];
	char	installation[MQ_INSTALLATION_NAME_LENGTH + 8];
	char	VRMF[16];
	char	filePath[512];
} QmgrInstTable;

typedef struct LocalSubParms {
	char		id[4];
	MQLONG      subType;
	MQLONG      destClass;
	MQLONG      durable;
	MQLONG      expiry;
	MQLONG      properties;
	MQLONG      priority;
	MQLONG      requestOnly;
	MQLONG      scope;
	MQLONG      subLevel;
	MQLONG      varUser;
	MQLONG      wildCard;
	MQCHAR48    Name;
	char        delim1;
	MQCHAR64    Desc;
	char        delim2;
	MQCHAR32    ApplIdentity;
	char        delim3;
	MQCHAR12    userID;
	char        delim4;
	MQCHAR12    creationDate;
	char        delim5;
	MQCHAR8     creationTime;
	char        delim6;
	MQCHAR12    alterationDate;
	char        delim7;
	MQCHAR8     alterationTime;
	char        delim8;
	MQCHAR48    destName;
	char        delim9;
	MQCHAR48    destQMName;
	char        delim10;
	MQBYTE		subID[MQ_SUB_IDENTITY_LENGTH];
} LocalSubParms;

typedef struct LocalTopicParms {
	char		id[4];
	MQLONG      type;
	MQLONG      status;
	MQLONG      attrs;
	MQLONG      sub;
	MQLONG      pub;
	MQCHAR	    Name[MQ_TOPIC_NAME_LENGTH + 4];
	MQCHAR	    topic[MQ_TOPIC_STR_LENGTH + 4];
} LocalTopicParms;

typedef struct LocalQueueParms {
	char		id[4];
	MQLONG      QType;
	MQLONG      InhibitPut;
	MQLONG      DefPriority;
	MQLONG      DefPersistence;
	MQLONG      InhibitGet;
	MQLONG      MaxQDepth;
	MQLONG      MaxMsgLength;
	MQLONG      BackoutThreshold;
	MQLONG      Shareability;
	MQLONG      DefInputOpenOption;
	MQLONG      HardenGetBackout;
	MQLONG      MsgDeliverySequence;
	MQLONG      RetentionInterval;
	MQLONG      DefinitionType;
	MQLONG      Usage;
	MQLONG      OpenInputCount;
	MQLONG      OpenOutputCount;
	MQLONG      CurrentQDepth;
	MQLONG      TriggerControl;
	MQLONG      TriggerType;
	MQLONG      TriggerMsgPriority;
	MQLONG      TriggerDepth;
	MQLONG      Scope;
	MQLONG      QDepthHighLimit;
	MQLONG      QDepthLowLimit;
	MQLONG      QDepthMaxEvent;
	MQLONG      QDepthHighEvent;
	MQLONG      QDepthLowEvent;
	MQLONG      QServiceInterval;
	MQLONG      QServiceIntervalEvent;
	MQCHAR48    QName;
	char        delim1;
	MQCHAR64    QDesc;
	char        delim2;
	MQCHAR48    ProcessName;
	char        delim3;
	MQCHAR48    BackoutReqQName;
	char        delim4;
	MQCHAR48    InitiationQName;
	char        delim5;
	MQCHAR12    CreationDate;
	char        delim6;
	MQCHAR8     CreationTime;
	char        delim7;
	MQCHAR64    TriggerData;
	char        delim8;
	MQCHAR48    remoteQName;
	char        delim9;
	MQCHAR48    remoteQMName;
	char        delim10;
	MQCHAR48    clusterName;
	char        delim11;
	MQCHAR48    clusterQMName;
	char        delim12;
	MQCHAR48    repositoryName;
	char        delim13;
} LocalQueueParms;

typedef struct CAPTPARMS
{
	_int64			msgCount;				// maximum number of messages to capture
	_int64			count;					// number of messages processed
	_int64			totalBytes;				// total number of bytes in saved messages
	LPVOID			pDocument;				// pointer to DataArea object
	HWND			m_hWnd;					// window handle to dialog window
	FILE			*outputFile;			// output file
	int				fileLen;				// starting length of output file
	int				delimLen;
	int				propDelimLen;
	int				maxTime;				// maximum number of seconds to run
	int				allocCount;				// number of buffers that were allocated besides default
	int				freeCount;				// number of buffers that were freed
	int	volatile	err;					// error indicator
	int volatile	stop;					// flag to indicate user has pressed stop button.
	bool			includeMQMD;			// include the MQMD with the data
	bool			removeHeaders;			// remove any MQ headers
	bool			appendFile;				// append data to existing file
	bool			inclProps;				// capture user properties
	bool			inclTopic;				// capture topic property
	char			errMsg[64];
	char			delimiter[64];			// delimiter between messages
	char			propDelim[64];			// delimiter between properties and message data
} CAPTPARMS;

typedef struct WRITEPARMS
{
	_int64			msgCount;				// maximum number of messages to publish
	_int64			count;					// number of messages published
	_int64			noMatchCount;			// number of messages published that did not match a subscriber
	_int64			totalBytes;				// total number of bytes written
	LPVOID			pDocument;				// pointer to DataArea object
	MQHOBJ			hTopic;					// handle to publish messages on
	HWND			m_hWnd;					// window handle to dialog window
	const char *	topicName;				// pointer to topic name
	LPCTSTR			QMname;					// pointer to queue manager name
	unsigned char *	fileData;				// pointer to the data in the file
	MQMD2 *			mqmd;					// pointer to MQMD or NULL
	const char *	userProp;				// pointer to user properties or NULL
	int				fileSize;				// length of the data in the file
	int				delimLen;				// length of the delimiter
	int				propDelimLen;			// length of the properties delimiter
	int				waitTime;				// number of milliseconds to wait between units of work
	int				allocCount;				// number of buffers that were allocated besides default
	int				freeCount;				// number of buffers that were freed
	size_t			batchSize;				// number of messages to publish in a single unit of work
	size_t			uow;					// number of messages in current unit of work
	size_t			batch;					// number of messages in current batch
	int				pubLevel;				// pub level
	int				msgSize;				// message size in bytes
	int				propLen;				// length of properties area in bytes
	unsigned char *	msgData;				// pointer to message data
	int	volatile	err;					// error indicator
	int volatile	stop;					// flag to indicate user has pressed stop button.
	BOOL			onePass;				// stop after one pass through the file
	BOOL			new_msg_id;				// create new message id
	BOOL			new_correl_id;			// create new correlation id
	BOOL			retained;				// retained publication
	BOOL			local;					// local publication - do not propagate to other cluster members
	BOOL			suppressReply;			// do not publish reply to queue mananger and queue
	BOOL			warnNoSub;				// warn if no subscriptions match the topic of published message
	BOOL			useMQMD;
	BOOL			useTopic;				// use the topic that is stored as a user property if it exists
	BOOL			useProps;				// use any user properties that are stored with the message
	bool			removeHeaders;
	char			errMsg[64];
	char			delimiter[64];			// delimiter between messages
	char			propDelim[64];			// delimiter between properties and message data
	char			topic[MQ_TOPIC_STR_LENGTH + 8];			// topic string
	char			resTopicStr[MQ_TOPIC_STR_LENGTH + 8];	// full topic string after open
} WRITEPARMS;

typedef struct LOADPARMS
{
	LPCTSTR fileName;
	LPCTSTR qmName;
	LPCTSTR RemoteQM;
	LPCTSTR qName;
	int startFileNumber;
	int maxCount;
	int waitTime;
	int batchSize;
	int filesType;
	int	defaultCcsid;
	int defaultEncoding;
	bool removeMQMD;
	bool removeHdrs;
	bool useSetAll;
	bool newMsgId;
	bool persistent;
	bool writeOnce;
	bool ignoreProps;
	bool singleFile;
	const char *format;
	const char *delimiter;
	const char *propDelim;
	int delimLen;
	int propDelimLen;
} LOADPARMS;

typedef struct SAVEPARMS
{
	LPCTSTR fileName;
	LPCTSTR qmName;
	LPCTSTR qName;
	int startCount;
	int endCount;
	int filesType;
	int browseMsgs;
	bool includeMQMD;
	bool removeHdrs;
	bool appendFile;
	const char *delimiter;
	const char *propDelim;
	int delimLen;
	int propDelimLen;
	int	maxfileSize;
} SAVEPARMS;

typedef struct SUBPARMS
{
	int subType;
	int userDataLength;
	int userDataBufSize;
	MQLONG maxSelLen;
	MQLONG level;
	MQLONG wildcard;
	MQLONG priority;
	MQLONG expiry;
	LPCTSTR QMname;
	LPCTSTR remoteQM;
	LPCTSTR queue;
	LPCTSTR topic;
	LPCTSTR topicName;
	LPCTSTR subName;
	char * resumeTopicName;
	char * resumeTopic;
	char * selection;
	char * applIdent;
	char * acctToken;
	char * userData;
	char * managedQName;
	MQBYTE24 * correlId;
	BOOL durable;
	BOOL managed;
	BOOL onDemand;
	BOOL newOnly;
	BOOL local;
	BOOL groupSub;
	BOOL anyUserId;
	BOOL setIdent;
	BOOL setCorrelId;
} SUBPARMS;

class DataArea : public CObject
{
public:
	DataArea();
	virtual ~DataArea();
// Attributes
public:

	// Variables related to the current file/message data
	char			fileName[512];
	char			lastFileRead[512];
	UINT			fileSize;
	int				fileCcsid;
	unsigned char	*fileData;
	CString			fileSource;

	CString m_error_msg;
	CString m_copybook_file_name;
	CString	m_security_exit;
	CString	m_security_data;
	CString m_ssl_keyr;
	CString m_ssl_cipher;
	CString m_conn_password;
	CString m_conn_userid;
	CString m_local_address;
	CString m_msg_text;
	CString m_queue_type;
	CString m_remote_QM;
	CString m_Q_name;
	CString m_QM_name;
	CString m_resolvedQname;
	CString m_user_id;
	CString m_reply_queue;
	CString m_reply_qm;
	CString	currentQ;
	CString	currentQM;
	CString	currentRemoteQM;
	CString	currentUserid;
	CString m_ps_broker_qm;
	CString m_filter;
	CString m_prop_delim;
	BOOL foundMQ;
	BOOL pubsubSupported;
	BOOL propertiesSupported;
	BOOL m_ps_remove;
	BOOL m_ssl_validate;
	BOOL m_use_ssl;
	BOOL traceEnabled;
	BOOL m_alt_userid;
	BOOL m_complete_msg;
	BOOL m_convert;
	BOOL m_new_correl_id;
	BOOL m_new_msg_id;
	BOOL m_setUserID;
	BOOL m_set_all;
	BOOL m_get_by_msgid;
	BOOL m_get_by_correlid;
	BOOL m_get_by_groupid;
	BOOL m_show_cluster_queues;
	BOOL m_readfile_ascii;
	BOOL m_read_nocrlf;
	BOOL m_read_unix;
	BOOL m_read_ignore_header;
	BOOL m_dataonly;
	BOOL m_save_rfh;
	BOOL m_all_avail;
	BOOL m_show_system_queues;
	BOOL m_logical_order;
	BOOL m_report_nan;
	BOOL m_report_pan;
	BOOL m_report_activity;
	BOOL m_report_pass_msgid;
	BOOL m_report_pass_correlid;
	BOOL m_report_discard;
	BOOL m_report_expire_discard;
	BOOL m_write_include_MQMD;
	BOOL m_read_ignore_MQMD;
	BOOL verboseTrace;
	BOOL fontTrace;
	int m_report_coa;					// report options - Confirm on Arrival
	int m_report_cod;					// report options - Confirm on Arrival
	int m_report_except;				// report options - Exception when message processed
	int m_report_expire;				// report options - Message expired
	int m_ssl_reset_count;				// number of bytes transferred before SSL key is reset
	int m_char_format;					// format of data in buffer (ASCII, EBCDIC, Chinese/Korean/Japanese)
	int m_pd_numeric_format;			// encoding of decimal data
	int m_float_numeric_format;			// encoding of floating point data
	int m_numeric_format;				// encoding of binary and floating point data
	int fileIntFormat;
	int m_backout_count;				// backout count from MQMD
	int	Qopen;							// indicator if queue is currently open
	int	currentConnect;					// indicator if connected to a queue manager
	int	m_bind_option;
	int browseActive;					// indication that a browse is active
	int browsePrevActive;				// indication that a browse previous is now allowed (must have browsed at least 2 messages)
	int browseCount;					// number of messages browsed in current operation
	int m_mq_props;
	int m_close_option;
	int m_file_codepage;				// code page of data within a file
	MQLONG			m_q_depth;			// depth of the last queue that was accessed
	MQLONG			level;				// queue manager level
	MQLONG			platform;			// queue manager platform type
	MQLONG			MQCcsid;			// ccsid of queue manager
	unsigned char * m_data_ascii;
	unsigned char * m_data_hex;
	unsigned char * m_data_both;
	unsigned char * m_data_parsed;
	unsigned char * m_data_xml;
	unsigned char * m_data_json;
	unsigned char * m_data_fix;
	wchar_t * m_data_ascii_UCS;
	MQBYTE24 m_save_message_id;

// Implementation
public:
	const char * getMQ71serverInstPath();
	BOOL checkForMQ();
	void freeAllocations();
	MQLONG pubWriteMsgCommit(WRITEPARMS *parms);
	MQLONG pubWriteMsgClose(WRITEPARMS *parms);
	MQLONG pubWriteMsgOpen(WRITEPARMS *parms);
	MQLONG pubWriteMsg(WRITEPARMS * parms);
	int getJsonOfs();
	int getJsonLineNumber(LPCTSTR value, int dir);
	int getFixOfs();
	int getFixLineNumber(LPCTSTR value, int dir);
	void getFixData(const int charFormat);
	void getJsonData(const int charFormat);
	void loadQMComboBox(CComboBox * cb);
	void selectUsrFolder();
	void SearchForMQ71();
	void getNameValueData(const unsigned char * input, int inputLen, unsigned char * output, int maxLength, int ccsid, const int charFormat);
	int getQueueNames(const char * qmName);
	int processLocalQMgrs();
	const char * getNextQMname(const char * name);
	const char * getFirstQMname();
	void clearRfhUsrData();
	void clearUserProperties();
	void getRfhUsrData(CString& data);
	void getUserPropertyData(CString& data);
	CString defaultQMname;
	void getDefaultQM();
	int loadMQdll();
	BOOL isSubscriptionActive();
	void ReqPub(const char * QMname, const char * brokerQM, const char * topic, const char * subName);
	const char * getTopicNamesListPtr(const char *QMname);
	const char * getSubNamesListPtr(const char * QMname);
	const char * getQueueNamesListPtr(const char *QMname);
	int loadNames(const char * QMname, const char * brokerQM, int nameType, MQLONG qType, BOOL inclCluster, BOOL resetList);
	void publishMsg(LPCTSTR QMname,
					const char *topic,
					const char * topicName,
					int pubLevel,
					BOOL retained,
					BOOL local,
					BOOL notOwn,
					BOOL suppressReply,
					BOOL warnNoSub,
					BOOL newCorrelId,
					BOOL noMulticast);
	void closePSQueue(BOOL removeSub);
	void getPSMessage(const char * subName, int waitTime, BOOL * isRetained);
	int subscribe(SUBPARMS * parms);
	MQLONG openAdminQ(LPCTSTR QMname, LPCTSTR remoteQM, MQLONG * reason);
	MQLONG closeAdminQ(MQLONG * reason);
	MQLONG getReplyMsg(MQLONG MQParm, char * UserMsg, char * errtxt, MQLONG ReadBufferLen, MQLONG * bytesRead, MQLONG * reason, MQLONG * ccsid, MQLONG * encoding);
	MQLONG PutAdminMsg(MQCHAR8 MsgFormat, char * UserMsg, MQLONG UserMsgLen, MQLONG ccsid, char * errtxt, const char * replyQName, MQLONG * rc);
	void closeReplyQ();
	MQLONG OpenTempQ(LPCTSTR QMname, LPCTSTR qName, MQLONG openOpts, char *realQname, MQLONG *ret);
	void WriteDataFile();
	void ReadDataFile();
	int MQEBC2UCS(wchar_t *out, const unsigned char *in, int len, int ccsidIn);
	void dumpTraceData(const char *label, const unsigned char *data, unsigned int length);
	void DeleteContents();
	void setRFHV1();
	void setRFHV2();
	void setPubSubVersion();
	void freeRfh1Area();
	void freeRfhArea();
	void setJMSdomain(const char *domain);
	void explicitCloseQ();
	void explicitDiscQM();
	void logTraceEntry(const char * traceInfo);
	void setBrowsePrevMsg(char * prevMsgId);
	BOOL isConnectionActive();
	unsigned char * scanForDelim(unsigned char * msgData, int msgLen, const char * delimiter, int delimLen);
	void loadMsgs(LOADPARMS * parms);
	void saveMsgs(SAVEPARMS * parms);
	int captureMsg(CAPTPARMS *parms);
	void moveMessages(const char * qmName, const char * qName, const char * newQName, int start, int count, BOOL removeDlq, BOOL passAll);
	void freeHeader(const char * format);
	void removeMQheader(int hdrType, const char * format, int ccsid, int encoding, int pdEncoding, int floatEncoding);
	void insertMQheader(int hdrType, int ccsid, int encoding, int pdEncoding, int floatEncoding);
	int startMsgDisplay(const char * QMname, const char * Queue, MQMD2 * mqmd, MQLONG *msgLen);
	int getNextMsgDisplay(MQMD2 * mqmd, MQLONG *msgLen);
	void setMsgId(char * msgId);
	int checkForUnicodeMarker(unsigned char * data);
	void getHdrCcsid(const char * format, int *ccsid, int *encoding);
	unsigned char * getTranslatedData(int * length, const int charFormat, const int ccsid);
	int getParsedOfs();
	int getXMLofs();
	void resetParsedFind();
	int getParsedLineNumber(LPCTSTR value, int dir);
	int curDataLineNumber;
	void resetHexFind();
	int findNextHex(unsigned char * value, int length, int upDown);
	void resetXMLfind();
	int getXMLLineNumber(LPCTSTR value, int upDown);
	int getLineOffset(int ofs, int dataFormat, int crlf, int edi, int BOM);
	int getLineNumber(int ofs, int dataFormat, int crlf, int edi, int BOM);
	int findNext(unsigned char * value, int len, int upDown);
	void resetFind();
	CObject * cicsData;
	CObject * imsData;
	CObject * dlqData;
	CObject * rfhData;
	CObject * jmsData;
	CObject * pubData;
	CObject * pscrData;
	CObject * usrData;
	CObject * otherData;
	CObject * mqmdData;
	CObject * propData;
	CObject * psData;
	BOOL isQueueOpen();
	void closeQ(MQLONG options);
	void discQM();
	void printData(CEdit * cedit);
	void updateMsgText();
	void resetDisplayData();
	void resetDataArea();
	void ClearData();
	void clearFileData();
	void getParsedData(const int charFormat);
	int readCopybookFile();
	CString getCobolData(const int charFormat,
						 const int numFormat,
						 const int pdNumFormat,
						 const int indent,
						 const int checkData);
	void setErrorMsg(MQLONG cc, MQLONG rc, const char * operation);
	bool				connected;
	MQLONG				characterSet;
	int readFirstMessage(bool silent, LPCTSTR qName, int line);
	int startBrowse(LPCTSTR QMname, LPCTSTR Queue, bool silent, int line);
	int browseNext(bool silent);
	int browsePrev();
	void endBrowse(bool silent);
	bool checkConnection(LPCTSTR QMname);
	bool connect2QM(LPCTSTR qm);
	bool explicitConnect(LPCTSTR qmName);
	bool openQ(LPCTSTR Queue, LPCTSTR RemoteQM, int openType, BOOL passAll);
	bool explicitOpen(LPCTSTR Queue, LPCTSTR RemoteQM, int openType);
	int getQueueDepth(LPCTSTR QMname, LPCTSTR Queue);
	void getMessage(LPCTSTR QMname, LPCTSTR Queue, int reqType);
	void purgeQueue(LPCTSTR QMname, LPCTSTR Queue, CEdit * depthBox);
	void putMessage(LPCTSTR QMname, LPCTSTR RemoteQM, LPCTSTR Queue);
	void WriteFile(LPCTSTR fname);
	void ReadFileData(LPCTSTR fname);
	void getCharacterData(const int charFormat, const int crlf, const int edi, const int BOM, BOOL indent);
	void getHexData();
	void getBothData(const int charFormat, const int BOM);
	void getXmlData(const int charFormat);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	BOOL foundMQ71;
	BOOL is64bit;
	char highestLevelInstallationName[MQ_INSTALLATION_NAME_LENGTH + 8];
	void loadMQreportError(const char * mqmPath, const char * loadMQreportError);
	void releaseQMgrEntries();
	QmgrInstTable * CreateQMgrInstEntry(const char * dspmqOutput, const char * qmgrName);
	void releaseInstallEntries();
	void CreateInstallEntry(const char * name, const char * filepath, const char * VRMF, int version, int release, const char * installation, const char * package);
	void processMQcomponents();
	void processMQinstallation(HKEY regkey);
	void insertTraceString(CComboBox *cb, const char * str);
	FILE * openOutputFile(LPCTSTR fname, LPTSTR errMsg);
	BOOL storageFreed;
	int pubWriteMsgAddProps(WRITEPARMS *parms, MQHMSG hMsg);
	int msgIdCount;
	int ocount;
	int ccount;
	int rcount;
	int acount;
	int mcount;
	int lcount;
	int MQCcsidLen;
	int MQUcsCcsid;
	int MQCcsidOut;
	int MQCcsidIn;
	int m_save_browse_count;
	int saveEdi;
	int saveBOM;
	int saveParsedFormat;
	int saveXMLFormat;
	int saveBothFormat;
	int saveCharCrlf;
	int saveCharFormat;
	int saveJsonFormat;
	int	saveFixFormat;
	int findParsedOfs;
	int findXMLofs;
	int findJsonOfs;
	int findFixOfs;
	int findParsedOffset;
	int findParsedLine;
	int findJsonLine;
	int findFixLine;
	int findXMLline;
	int findXMLOffset;
	int findJsonOffset;
	int findFixOffset;
	int findStartOffset;
	int findHexOffset;
	MQLONG maxMsgLenQM;				// maximum message length for this QMgr
	MQLONG maxMsgLenQ;				// maximum message length for this Queue
	MQLONG maxUOW;					// maximum uncommitted messages in a single UOW
	char * m_msg_id_table;
	char invalidCharTable[256];
	char * defaultMQbuffer;			// default buffer to avoid extra mallocs
	unsigned char * MQCcsidPtr;
	wchar_t * MQUcsPtr;
	NAMESTRUCT *subNamesRoot;
	NAMESTRUCT *topicNamesRoot;
	NAMESTRUCT *queueNamesRoot;
	InstTable *firstInstallation;
	QmgrInstTable * firstQmgr;
	int highestMQVersion;
	int highestMQRelease;
	char higestVRMF[16];
	char MQ71Path[512];
	bool segmentActive;
	bool groupActive;
	BOOL invalidCharTableNotBuilt;
	BOOL firstError;
	BOOL saveIndentOn;
	BOOL m_save_set_all_setting;
	BOOL m_save_set_user_setting;
	TCHAR szPrinterName[104];
	char currentFilter[MQ_SELECTOR_LENGTH];
	char QueueManagerRealName[MQ_Q_MGR_NAME_LENGTH + 8];
	MQHCONN		qm;
	MQHOBJ		q;
	MQHOBJ		hReplyQ;				// handle to reply queue for PCF messages
	MQHOBJ		hAdminQ;				// handle to put PCF messages to Admin queue
	MQHOBJ		subQ;					// object handle used for MQGET
	MQHOBJ		hSub;					// object handle for subscription
	CCopybook *m_copybook;
	CString transmissionQueue;
	CString qmTempName;

	BOOL CreateChildProcess(HANDLE hChildStdoutWr);
	const char * getNamesListPtr(const char *QMname, NAMESTRUCT * namePtr);
	const char * findPropEnd(const char * propPtr, const char * endPtr);
	void releaseMsgIdTable();
	void resetJsonFind();
	void resetFixFind();
	void connectionLostCleanup();
	void listClientTable(char *tabdata, int filelen, CComboBox *cb);
	void dumpIndFontName(CFont * cf, const char * name);
	int getPropType(const char * propPtr, const char * endProp);
	void resetNames(NAMESTRUCT * namePtr, const char * QMname);
	void processQueuesReply(char * msg, NAMESTRUCT * namesPtr, const char * replyQName, MQLONG ccsid, MQLONG encoding);
	int appendPropsToBuffer(const char * namePtr, const char * valuePtr, int len, int propType, char * buffer, int bufLen, int bufOfs);
	void buildPCFRequestQueues(char * pAdminMsg, MQLONG qType, BOOL inclCluster);
	void buildPCFRequestTopics(char * pAdminMsg);
	void buildPCFRequestSubs(char * pAdminMsg);
	void processTopicReply(char * msg, NAMESTRUCT * namesPtr, MQLONG ccsid, MQLONG encoding);
	void processSubNamesReply(char * msg, NAMESTRUCT * namesPtr, MQLONG ccsid, MQLONG encoding);
	NAMESTRUCT * findNameInList(const char * QMname, NAMESTRUCT * namePtr);
	NAMESTRUCT * allocateNameStruct(const char * QMname, const char * nameType);
	NAMESTRUCT * getSubNamesPtr(const char * QMname, BOOL reset);
	NAMESTRUCT * getTopicNamesPtr(const char * QMname, BOOL reset);
	NAMESTRUCT * getQueueNamesPtr(const char * QMname, BOOL reset);
	int getListOfNames(NAMESTRUCT * namesPtr, const char * brokerQM, const char * replyQM, const char * replyQName, int nameType, MQLONG qType, BOOL inclCluster);
	int extractMsgProps(MQHMSG hMsg, int propDelimLen, const char *propDelim, char * buffer, int bufLen);
	bool openSubQ(const char * Queue, const char * remoteQM);
	bool processSaveMsgsCC(MQLONG cc, MQLONG rc, const char * operation);
	int findMsgProperties(MQHMSG hMsg, unsigned char * propData, int propLen);
	int setMsgProps(MQHMSG hMsg);
	int getQueueType(MQHOBJ queue);
	int processMQGet(const char * getType, MQHOBJ handle, int options, int match, int waitTime,  MQMD2 * mqmd, MQLONG * rc);
	int setInitialBufferSize();
	void getCurrentDepth();
	void assignMQMDccsid(const char * tempId, const char * headerName, const int ccsid, int encoding);
	int assignMQMDEncoding(int encoding, const int tempVer, const int headerValue);
	int getDefaultEncoding(int encoding);
	int getDefaultCcsid(int ccsid);
	int MQEBC2ASC(unsigned char * out, const unsigned char * in, int len, int ccsidIn, int ccsidOut);
	int readFileChunk(FILE * input, int *remaining, const int offset, unsigned char * buffer, bool firstTime);
	int nameToType(char * namePtr);
	int xlateEbcdicChar(int fromCcsid, int toCcsid, const char * input, char * output);

// implementation
	void findMQInstallDirectory(unsigned char * dirName, unsigned long dirNameLen);
	int setMQoptions(int options);
	int findcrlf(const unsigned char *datain, const int maxchar, const int charSet);
	void getCharDataUcs(const int charFormat, const int crlf, const int edi);
	BOOL isLeadChar(unsigned char ch, const int charFormat);
	void appendError(const char * errtxt);
	int stripMQhdrs(unsigned char *msgData, int msgLen, char *format, int *ccsid, int *encoding);
	int checkForMQhdrs(unsigned char * msgData, int msgLen, char * format, int * ccsid, int * encoding);
	int savemsgsRemoveHeaders(char * msgData, MQMD2 * mqmd, int msgLen, int ccsid, int encoding, char * format);
	void createNextFileName(const char * fileName, char *newFileName, int fileCount);
	int findPrevMsgId(const char * Qname);
	int checkDataForHeader();
	int parseMsgHeaders();
	int buildHeaders(unsigned char * tempbuf, int dlqLen, int cicsLen, int imsLen, int rfh1Len, int rfh2Len);
	void setWindowTitle(const char * queue);
	void rollbackUOW();
	void commitUOW(bool silent);
	void beginUOW(bool silent);
	bool unitOfWorkActive;
	void changeUnixFile();
	void checkForEBCDIC();
	bool PrintPage (CEdit * cedit,  PRINTDLG &pd);
	void setQueueType(int queuetype);
	void getFolderName(unsigned char * folderName, unsigned char * rfhdata, int segLen);
	void processMessage(MQMD2 * mqmd);
	void setOfs(CString& currentData, const int currentLevel);
	void MQParmCpy( char *target, char *source, int length );
	void convertPCFHeader(MQCFH *pPCFHeader, MQLONG encoding);
	void convertIntHeader(MQCFIN * pPCFInteger, MQLONG encoding);
	void convertIntListHeader(MQCFIL *pPCFInteger, MQLONG encoding);
	void convertStringHeader(MQCFST *pPCFString, MQLONG ccsid, MQLONG encoding);
	void convertStrListHeader(MQCFSL *pPCFString, MQLONG ccsid, MQLONG encoding);
	void convertByteStringHeader(MQCFBS * pPCFByteString, int ccsid, int encoding);

	// MQI replacement functions

//////////////////////////////////////////////////////////////////
//  MQCONNX Function -- Connect to Queue Manager
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQCONNX) (
	PMQCHAR		pQMgrName,		// Name of queue manager
	PMQCNO		pConnectOpts,	// Options that control the action of MQCONNX
	PMQHCONN	pHconn,			// Connection handle
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQDISC Function -- Disconnect Queue Manager
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQDISC) (
   PMQHCONN		pHconn,		// Connection handle
	PMQLONG		pCompCode,	// Completion code
	PMQLONG		pReason);	// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQBACK Function -- Back Out Changes
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQBACK) (
	MQHCONN		Hconn,		// Connection handle
	PMQLONG		pCompCode,	// Completion code
	PMQLONG		pReason);	// Reason code qualifying CompCode


//////////////////////////////////////////////////////////////////
//  MQBEGIN Function -- Begin Unit of Work
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQBEGIN) (
	MQHCONN		Hconn,			// Connection handle
	PMQVOID		pBeginOptions,	// Options that control the action of MQBEGIN
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQCMIT Function -- Commit Changes
//////////////////////////////////////////////////////////////////


typedef void (MQENTRY* XMQCMIT) (
	MQHCONN		Hconn,		// Connection handle
	PMQLONG		pCompCode,	// Completion code
	PMQLONG		pReason);	// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQOPEN Function -- Open Object
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQOPEN) (
	MQHCONN		Hconn,			// Connection handle
	PMQVOID		pObjDesc,		// Object descriptor
	MQLONG		Options,		// Options that control the action of MQOPEN
	PMQHOBJ		pHobj,			// Object handle
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQCLOSE Function -- Close Object
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQCLOSE) (
	MQHCONN		Hconn,		// Connection handle
	PMQHOBJ		pHobj,		// Object handle
	MQLONG		Options,	// Options that control the action of MQCLOSE
	PMQLONG		pCompCode,	// Completion code
	PMQLONG		pReason);	// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQGET Function -- Get Message
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQGET) (
	MQHCONN		Hconn,			// Connection handle
	MQHOBJ		Hobj,			// Object handle
	PMQVOID		pMsgDesc,		// Message descriptor
	PMQVOID		pGetMsgOpts,	// Options that control the action of MQGET
	MQLONG		BufferLength,	// Length in bytes of the Buffer area
	PMQVOID		pBuffer,		// Area to contain the message data
	PMQLONG		pDataLength,	// Length of the message
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode


//////////////////////////////////////////////////////////////////
//  MQPUT Function -- Put Message
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQPUT) (
	MQHCONN		Hconn,			// Connection handle
	MQHOBJ		Hobj,			// Object handle
	PMQVOID		pMsgDesc,		// Message descriptor
	PMQVOID		pPutMsgOpts,	// Options that control the action of MQPUT
	MQLONG		BufferLength,	// Length of the message in Buffer
	PMQVOID		pBuffer,		// Message data
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQINQ Function -- Inquire Object Attributes
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQINQ) (
	MQHCONN		Hconn,			// Connection handle
	MQHOBJ		Hobj,			// Object handle
	MQLONG		SelectorCount,	// Count of selectors
	PMQLONG		pSelectors,		// Array of attribute selectors
	MQLONG		IntAttrCount,	// Count of integer attributes
	PMQLONG		pIntAttrs,		// Array of integer attributes
	MQLONG		CharAttrLength,	// Length of character attributes buffer
	PMQCHAR		pCharAttrs,		// Character attributes
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQSUB Function -- Subscribe to topic
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQSUB) (
	MQHCONN		Hconn,			// Connection handle
	PMQVOID		pSubDesc,		// Subscription descriptor
	PMQHOBJ		pHobj,			// Object handle for queue
	PMQHOBJ		pHsub,			// Subscription object handle
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQSUBRQ Function -- Subscription Request
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQSUBRQ) (
	MQHCONN		Hconn,			// Connection handle
	MQHOBJ		Hsub,			// Subscription handle
	MQLONG		Action,			// Action requested on the subscription
	PMQVOID		pSubRqOpts,		// Subscription Request Options
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQCRTMH Function -- Create Message Handle
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQCRTMH) (
	MQHCONN		Hconn,			// Connection handle
	PMQVOID		pCrtMsgHOpts,	// Options that control the action of MQCRTMH
	PMQHMSG		pHmsg,			// Message handle
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQDLTMH Function -- Delete Message Handle
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQDLTMH) (
	MQHCONN		Hconn,			// Connection handle
	PMQHMSG		pHmsg,			// Message handle
	PMQVOID		pDltMsgHOpts,	// Options that control the action of MQDLTMH
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQINQMP Function -- Inquire Message Property
//////////////////////////////////////////////////////////////////

typedef void (MQENTRY* XMQINQMP) (
	MQHCONN		Hconn,			// Connection handle
	MQHMSG		Hmsg,			// Message handle
	PMQVOID		pInqPropOpts,	// Options that control the action of MQINQMP
	PMQVOID		pName,			// Property name
	PMQVOID		pPropDesc,		// Property descriptor
	PMQLONG		pType,			// Property data type
	MQLONG		ValueLength,	// Length in bytes of the Value area
	PMQVOID		pValue,			// Property value
	PMQLONG		pDataLength,	// Length of the property value
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

//////////////////////////////////////////////////////////////////
//  MQSETMP Function -- Set Message Property
//////////////////////////////////////////////////////////////////


typedef void (MQENTRY* XMQSETMP) (
	MQHCONN		Hconn,			// Connection handle
	MQHMSG		Hmsg,			// Message handle
	PMQVOID		pSetPropOpts,	// Options that control the action of MQSETMP
	PMQVOID		pName,			// Property name
	PMQVOID		pPropDesc,		// Property descriptor
	MQLONG		Type,			// Property data type
	MQLONG		ValueLength,	// Length of the Value area
	PMQVOID		pValue,			// Property value
	PMQLONG		pCompCode,		// Completion code
	PMQLONG		pReason);		// Reason code qualifying CompCode

	// MQ entrypoints
	HINSTANCE	mqmdll;
	XMQCONNX	XMQConnX;
	XMQDISC		XMQDisc;
	XMQBEGIN	XMQBegin;
	XMQCMIT		XMQCmit;
	XMQBACK		XMQBack;
	XMQOPEN		XMQOpen;
	XMQCLOSE	XMQClose;
	XMQGET		XMQGet;
	XMQPUT		XMQPut;
	XMQINQ		XMQInq;
	XMQSUB		XMQSub;
	XMQSUBRQ	XMQSubRq;
	XMQCRTMH	XMQCrtMh;
	XMQDLTMH	XMQDltMh;
	XMQINQMP	XMQInqMp;
	XMQSETMP	XMQSetMp;
};

#endif // !defined(AFX_DATAAREA_H__6861C316_DF2F_4594_BB91_8743A5E7E765__INCLUDED_)
