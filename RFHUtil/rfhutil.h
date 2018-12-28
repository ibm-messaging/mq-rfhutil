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


// rfhutil.h : main header file for the RFHUTIL application
//

#if !defined(AFX_RFHUTIL_H__96F2439F_AE02_4650_8A3E_EE7F0EC7E72C__INCLUDED_)
#define AFX_RFHUTIL_H__96F2439F_AE02_4650_8A3E_EE7F0EC7E72C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <afxmt.h>
#include "resource.h"       // main symbols
#include "DataArea.h"	// Added by ClassView

#define MAX_SAVED_QNAMES 20

// Header level definitions
// tab definitions
#define PAGE_MAIN		0
#define PAGE_DATA		1
#define PAGE_MQMD		2
#define PAGE_V7PUBSUB	3
#define PAGE_V7PROPS	4
#define PAGE_RFH		5
#define PAGE_PUBSUB		6
#define PAGE_PSCR		7
#define PAGE_JMS		8
#define PAGE_USR		9
#define PAGE_OTHER		10
#define PAGE_CICS		11
#define PAGE_IMS		12
#define PAGE_DLQ		13

// Display format types
#define DATA_CHARACTER	0
#define	DATA_HEX		1
#define	DATA_BOTH		2
#define	DATA_XML		3
#define	DATA_PARSED		4
#define	DATA_COBOL		5
#define	DATA_JSON		6
#define DATA_FIX		7

// Character display types
#define CHAR_ASCII		0
#define CHAR_EBCDIC		1
#define CHAR_CHINESE	2
#define CHAR_KOREAN		3
#define CHAR_TRAD_CHIN	4
#define CHAR_JAPANESE	5
#define CHAR_THAI		6
#define CHAR_RUSSIAN	7
#define CHAR_UNICODE	8

#define IGNORE_CRLF		0
#define HONOR_CRLF		1

#define NUMERIC_PC		0
#define NUMERIC_HOST	1

#define FLOAT_PC		0
#define FLOAT_UNIX		1
#define FLOAT_390		2
#define FLOAT_TNS		3

#define BIND_AS_Q		0
#define BIND_ON_OPEN	1
#define BIND_NOT_FIXED	2
#define BIND_GROUP		3

#define MQPERSIST_NO	0
#define MQPERSIST_YES	1
#define MQPERSIST_AS_Q	2

#define REPORT_NONE		0
#define REPORT_YES		1
#define REPORT_DATA		2
#define REPORT_FULL		3

#define GROUP_NO		0
#define GROUP_YES		1
#define GROUP_LAST		2

#define SEGMENT_NO		0
#define SEGMENT_YES		1
#define SEGMENT_LAST	2

#define PS_PERSIST_NONE	0
#define PS_ASPUB		0
#define PS_PERSIST		1
#define PS_NONPERS		2
#define PS_ASQUEUE		3

#define PS_REQ_REGISTER		0
#define PS_REQ_UNREG		1
#define PS_REQ_PUBLISH		2
#define PS_REQ_REQPUB		3
#define PS_REQ_DELPUB		4
#define PS_REQ_REG_PUB		5
#define PS_REQ_UNREG_PUB	6

#define PSCR_COMP_OK	0
#define PSCR_COMP_WARN	1
#define PSCR_COMP_ERR	2

// JMS message types
#define JMS_TEXT		0
#define JMS_BYTES		1
#define JMS_STREAM		2
#define JMS_OBJECT		3
#define JMS_MAP			4
#define JMS_NONE		5

// code page defaults
#define RFH_NAMEVALUE_CCSID_DEFAULT 1208
#define DEF_EBCDIC_CCSID			500
#define DEF_ASCII_CCSID				437

// encoding types
#define DEF_PC_ENCODING				546
#define DEF_HOST_ENCODING			785
#define DEF_UNIX_ENCODING			273

#define SAVE_DELIM_CHAR	0
#define SAVE_DELIM_HEX	1

#define SAVE_REMOVE_MSGS_NO		0
#define SAVE_REMOVE_MSGS_YES	1

#define SAVE_FILE_SAME_FILE		0
#define SAVE_FILE_DIFF_FILES	1

// maximum number of saved client entries in the registry
#define RFHUTIL_LAST_CLIENT_MAX	30

/////////////////////////////////////////////////////////////////////////////
// CRfhutilApp:
// See rfhutil.cpp for the implementation of this class
//

class CRfhutilApp : public CWinApp
{
public:
	BOOL is64bit;
	CString psWriteBatchSize;
	CString psWriteMsgCount;
	CString psWriteWaitTime;
	BOOL psWriteWriteOnce;
	BOOL psWriteUseMQMD;
	BOOL psWriteUseProps;
	BOOL psWriteUseTopic;
	BOOL psWriteNewCorrelid;
	BOOL psWriteNewMsgid;
	BOOL psWriteWarnNoSub;
	CString psWriteFileName;

	CCriticalSection	m_free_alloc;
	BOOL				freeAllocNotCalled;

	int maxDispTime;
	int	oem;
	int	ansi;
	int	lang;
	int country;
	int	initSSLResetCount;
	UINT consoleInputCP;
	UINT consoleOutputCP;
	char productType[16];
	char VRMF[32];
	BOOL isRecentFileMenuEnabled();
	BOOL initUseCSP;
	BOOL verboseTrace;
	BOOL fontTrace;
	BOOL	initUseSSL;
	CString initQname;
	CString initQMname;
	CString initRemoteQMname;
	CString	initSSLCipherSpec;
	CString initSSLKeyR;
	CString initConnUser;
	CString initConnPW;
	CString initFilePath;
	CString initSecData;
	CString initSecExit;
	CString initLocalAddress;
	BOOL	initSSLValidateClient;
	CWnd * m_mainWnd;
	DataArea pDocument;

	const char * auditFileName;
	int MQServerVersion;
	int MQServerRelease;

	// display window characteristics
	int	horSize;
	int	vertSize;
	int	horRes;
	int	vertRes;
	int	horLogPixels;
	int	vertLogPixels;
	int	technology;
	int dpi;						// dpi of primary monitor
	int dpi2;						// dpi of second monitor, if found
	int enumCallbacks;
	LONG	height;
	LONG	width;
	LONG	charWidthAvg;
	LONG	charWidthMax;
	LONG	charHeight;
	int		scrWinch;			// screen width in tenths of an inch
	int		scrHinch;			// screen height in tenths of an inch
	int		numMonitors;		// number of monitors enumerated by EnumDisplayMonitors
	int		numDisplays;		// number of displays from GetSystemMetrics(SM_CMONITORS)
	char	fontFixedCharSets[8];
	BOOL	m_fontFixedFound;
	bool japanOK;						// able to create Japanese font
	bool thaiOK;						// able to create Japanese font
	bool koreaOK;						// able to create Korean font
	bool chinaBig5OK;					// able to create Traditional Chinese font
	bool chinaSimpOK;					// able to create Simplified Chinese font
	bool russianOK;						// able to create Russian font
	CFont m_fixed_font;					// fixed length font to display data so that columns line up
	CFont m_china_font;					// Simplified Chinese font
	CFont m_big5_font;					// Traditional Chinese font
	CFont m_korea_font;					// Korean font
	CFont m_japan_font;					// Japanese font
	CFont m_thai_font;					// Thai font
	CFont m_russian_font;				// Russian font
	// fonts to use on a secondary monitor, which may have a different resolution than the primary
	CFont m_fixed_font2;				// fixed length font to display data so that columns line up
	CFont m_china_font2;				// Simplified Chinese font
	CFont m_big5_font2;					// Traditional Chinese font
	CFont m_korea_font2;				// Korean font
	CFont m_japan_font2;				// Japanese font
	CFont m_thai_font2;					// Thai font
	CFont m_russian_font2;				// Russian font
	CString thaiFontface;				// thai font family
	CString simpChFontface;				// simplified Chinese font family
	CString tradChFontface;				// traditional Chinese font family
	CString koreaFontface;				// Korean font family
	CString japanFontface;				// Japanese font family
	BYTE fontPitchFamily;				// font pitch and family

#ifdef MQCLIENT
	CString lastClientQM[RFHUTIL_LAST_CLIENT_MAX];
#endif

	int createFixedFonts(CFont *font, const char *faceName, BYTE pitchFamily);
	BOOL checkFontCharsForFont(const unsigned char * charFont, int fontOffset);
	void restoreRecentFileList();
	void saveRecentFileList();
	void setMostRecentFilePath(LPCTSTR fileName);
	void saveClientQM(const char * qm);
	void dumpTraceData(const char * label, const unsigned char * data, unsigned int length);
	void createAuditRecord(LPCTSTR qMgr, LPCTSTR qName, LPCTSTR activity, MQLONG reason);
	BOOL isTraceEnabled();
	void logTraceEntry(LPCTSTR traceInfo);
	void getCopyright(CString &copyRight);
	int getBuild();
	int getRevision();
	int getMinor();
	int getMajor();
	int versMajor;
	void getCompanyName(CString & compName);
	void getProductName(CString & prodName);
	void OnOpenRecFile(UINT idx);
	CRfhutilApp();
	~CRfhutilApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRfhutilApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CRfhutilApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool versionDone;
	int buildVersion;
	int revisionVersion;
	int minorVersion;
	int majorVersion;
	int cy;
	int cx;
	int menuSize;
	int captionSize;
	BOOL m_enableRecentFileList;
	FILE * traceFile;
	CString m_copyRight;
	CString m_compName;
	CString m_prodName;

	void getEnvParms();
	void startTrace();
	void getMQLevel();
	void GetLastUsedQMgrs(const char * sectionName);
	void writeQMgr2Registry(const char * sectionName);
	void GetLastUsedQNames();
	void SaveLastUsedQNames();
	void processCmdLine();
	void getVersInfo();
	void findFonts(HDC hdc, BYTE charSet);
	void dumpFontNames();
	void countMonitors();						// count the number of monitors on the system
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RFHUTIL_H__96F2439F_AE02_4650_8A3E_EE7F0EC7E72C__INCLUDED_)
