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
/*   rfhsubs.h - Common subroutines for processing RFH headers.     */
/*                                                                  */
/********************************************************************/
#ifndef _CommonSubs_rfhsubs_h
#define _CommonSubs_rfhsubs_h

#define USR_AREA_SIZE		2048

#define RFH2_MCD_BEGIN		"<mcd>"
#define RFH2_MCD_END		"</mcd>"
#define RFH2_MSD_BEGIN		"<Msd>"
#define RFH2_MSD_END		"</Msd>"
#define RFH2_SET_BEGIN		"<Set>"
#define RFH2_SET_END		"</Set>"
#define RFH2_TYPE_BEGIN		"<Type>"
#define RFH2_TYPE_END		"</Type>"
#define RFH2_FMT_BEGIN		"<Fmt>"
#define RFH2_FMT_END		"</Fmt>"

#define RFH_JMS_TEXT		1
#define RFH_JMS_BYTES		2
#define RFH_JMS_STREAM		3
#define RFH_JMS_OBJECT		4
#define RFH_JMS_MAP			5
#define RFH_JMS_NONE		6

#define RFH2_JMS_BEGIN "<jms>"
#define RFH2_JMS_END "</jms>"

#define RFH_JMS_REQTYPE		"RFH_JMS_REQTYPE"
#define RFH_JMS_DEST		"RFH_JMS_DEST"
#define RFH_JMS_REPLY		"RFH_JMS_REPLY"
#define RFH_JMS_CORRELID	"RFH_JMS_CORRELID"
#define RFH_JMS_GROUPID		"RFH_JMS_GROUPID"
#define RFH_JMS_PRIORITY	"RFH_JMS_PRIORITY"
#define RFH_JMS_EXPIRE		"RFH_JMS_EXPIRE"
#define RFH_JMS_DELMODE		"RFH_JMS_DELMODE"
#define RFH_JMS_SEQ			"RFH_JMS_SEQ"

#define RFH_PSC_SUB			1
#define RFH_PSC_UNSUB		2
#define RFH_PSC_PUB			3
#define RFH_PSC_REQPUB		4
#define RFH_PSC_DELPUB		5

#define RFH_PSC_PERS			1
#define RFH_PSC_NON_PERS		2
#define RFH_PSC_PERS_PUB		3
#define RFH_PSC_PERS_QUEUE		4

#define RFH2_PSC_BEGIN "<psc>"
#define RFH2_PSC_END "</psc>"

#define RFH_PSC_REQTYPE		"RFH_PSC_REQTYPE"
#define RFH_PSC_TOPIC1		"RFH_PSC_TOPIC1"
#define RFH_PSC_TOPIC2		"RFH_PSC_TOPIC2"
#define RFH_PSC_TOPIC3		"RFH_PSC_TOPIC3"
#define RFH_PSC_TOPIC4		"RFH_PSC_TOPIC4"
#define RFH_PSC_SUBPOINT	"RFH_PSC_SUBPOINT"
#define RFH_PSC_FILTER		"RFH_PSC_FILTER"
#define RFH_PSC_REPLYQM		"RFH_PSC_REPLYQM"
#define RFH_PSC_REPLYQ		"RFH_PSC_REPLYQ"
#define RFH_PSC_PUBTIME		"RFH_PSC_PUBTIME"
#define RFH_PSC_SEQNO		"RFH_PSC_SEQNO"
#define RFH_PSC_LOCAL		"RFH_PSC_LOCAL"
#define RFH_PSC_NEWONLY		"RFH_PSC_NEWONLY"
#define RFH_PSC_OTHERONLY	"RFH_PSC_OTHERONLY"
#define RFH_PSC_ONDEMAND	"RFH_PSC_ONDEMAND"
#define RFH_PSC_RETAINPUB	"RFH_PSC_RETAINPUB"
#define RFH_PSC_ISRETAINPUB	"RFH_PSC_ISRETAINPUB"
#define RFH_PSC_CORRELID	"RFH_PSC_CORRELID"
#define RFH_PSC_DEREGALL	"RFH_PSC_DEREGALL"
#define RFH_PSC_INFRETAIN	"RFH_PSC_INFRETAIN"

#define RFH2_PSC_COMMAND_BEGIN	"<Command>"
#define RFH2_PSC_COMMAND_END	"</Command>"
#define RFH2_PSC_TOPIC_BEGIN	"<Topic>"
#define RFH2_PSC_TOPIC_END		"</Topic>"
#define RFH2_PSC_SUBPOINT_BEGIN	"<SubPoint>"
#define RFH2_PSC_SUBPOINT_END	"</SubPoint>"
#define RFH2_PSC_FILTER_BEGIN	"<Filter>"
#define RFH2_PSC_FILTER_END	"</Filter>"
#define RFH2_PSC_QMGRNAME_BEGIN	"<QMgrName>"
#define RFH2_PSC_QMGRNAME_END	"</QMgrName>"
#define RFH2_PSC_QNAME_BEGIN	"<QName>"
#define RFH2_PSC_QNAME_END		"</QName>"
#define RFH2_PSC_PUBTIME_BEGIN	"<PubTime>"
#define RFH2_PSC_PUBTIME_END	"</PubTime>"
#define RFH2_PSC_SEQNUM_BEGIN	"<SeqNum>"
#define RFH2_PSC_SEQNUM_END		"</SeqNum>"
#define RFH2_PSC_REGOPT_BEGIN	"<RegOpt>"
#define RFH2_PSC_REGOPT_END		"</RegOpt>"
#define RFH2_PSC_PUBOPT_BEGIN	"<PubOpt>"
#define RFH2_PSC_PUBOPT_END		"</PubOpt>"
#define RFH2_PSC_DELOPT_BEGIN	"<DelOpt>"
#define RFH2_PSC_DELOPT_END		"</DelOpt>"
#define RFH2_PSC_LOCAL			"Local"
#define RFH2_PSC_NEWONLY		"NewPubsOnly"
#define RFH2_PSC_OTHERONLY		"OtherSubsOnly"
#define RFH2_PSC_ONDEMAND		"PubOnReqOnly"
#define RFH2_PSC_RETAINPUB		"RetainPub"
#define RFH2_PSC_ISRETAINPUB	"IsRetainedPub"
#define RFH2_PSC_CORRELASID		"CorrelAsId"
#define RFH2_PSC_DEREGALL		"DeregAll"
#define RFH2_PSC_INFORMIFRET	"InformIfRet"
#define RFH2_PSC_PERS			"Pers"
#define RFH2_PSC_NON_PERS		"NonPers"
#define RFH2_PSC_PERSASPUB		"PersAsPub"
#define RFH2_PSC_PERSASQUEUE	"PersAsQueue"

#define PSC_REGSUB		"RegSub"
#define PSC_DEREGSUB	"DeregSub"
#define PSC_PUBLISH		"Publish"
#define PSC_REQUPDATE	"ReqUpdate"
#define PSC_DELETEPUB	"DeletePub"

#define RFH2_USR_BEG	"<usr>"
#define RFH2_USR_END	"</usr>"

#define RFH2_PSCR_BEGIN	"<pscr>"
#define RFH2_PSCR_END	"</pscr>"

int isRFH(unsigned char * data, size_t length, size_t *rfhLen);
void translateRFH(unsigned char * data, size_t length);
unsigned int checkRFH(const char * msgdata, const size_t datalen, MQMD2 *mqmd, PUTPARMS * parms);
char * checkForRFH(char * msgdata, MQMD2 * msgdesc);
void releaseRFH(PUTPARMS * parms);
int buildRFH1(char * rfhdata, PUTPARMS *parms);
int buildRFH2(char * rfhdata, PUTPARMS *parms, int xmlOnly);
void createRFH(PUTPARMS *parms);
void rebuildMCD(PUTPARMS * parms);
void rebuildJMS(PUTPARMS * parms);
void rebuildPSC(PUTPARMS * parms);
void resetMCD(PUTPARMS * parms);
void resetPSC(PUTPARMS * parms);
void resetPSCR(PUTPARMS * parms);
void resetJMS(PUTPARMS * parms);
void resetUSR(PUTPARMS * parms);
void appendMCD(char * data, PUTPARMS * parms);
void appendPSC(char * data, PUTPARMS * parms);
void appendPSCR(char * data, PUTPARMS * parms);
void appendJMS(char * data, PUTPARMS * parms);
void appendUSR(char * data, PUTPARMS * parms);
#endif
