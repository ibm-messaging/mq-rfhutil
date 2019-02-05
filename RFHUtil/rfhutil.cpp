/*
Copyright (c) IBM Corporation 2000, 2019
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

// rfhutil.cpp : Defines the class behaviors for the application.

#include "stdafx.h"
#include "rfhutil.h"

#include "MainFrm.h"
#include "rfhutilDoc.h"
#include "rfhutilView.h"
#include "comsubs.h"

#include <locale.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// environment variable used to turn on trace
#define RFHUTIL_TRACE_FILE		"RFHUTIL_TRACE_FILE"
#define RFHUTIL_VERBOSE_TRACE	"RFHUTIL_VERBOSE_TRACE"
#define RFHUTIL_FONTS_TRACE		"RFHUTIL_FONTS_TRACE"
#define RFHUTIL_MAX_DISPLAY_TIME "RFHUTIL_MAX_DISPLAY_TIME"

// default maximum display queue wait time
#define MAX_DISPLAY_TIME_DEFAULT 15
#define MAX_DISPLAY_TIME_MIN 10

// environment variable used to turn on Audit log
#define RFHUTIL_AUDIT_FILE		"RFHUTIL_AUDIT_FILE"

// constants for offset of supported code pages by fonts
// the numbers are the bit that corresponds to supported
// by a particular font
#define	CP_LATIN1_1252		0
#define	CP_LATIN2_1250		1
#define	CP_CYRILLIC_1251	2
#define CP_HEBREW_1255		5
#define CP_THAI_874			16
#define CP_JAPAN_932		17
#define CP_SIMP_CH_936		18
#define CP_KOREA_949		19			// Korean unified Hangul
#define CP_TRAD_CH_950		20
#define CP_KOREA_1361		21			// Korean Johab
#define CP_RUSSIA_866		49
#define CP_NORDIC_865		50
#define CP_ARABIC_864		51
#define CP_LATIN_850		62
#define CP_US_437			63

// most recently used registry section names
#define RFHUTIL_MOST_RECENT_QMGR	"QMgr"
#define RFHUTIL_MOST_RECENT_QUEUE	"QName"
#define RFHUTIL_MOST_RECENT_REMOTE	"RemoteQMgr"

// extra information that is saved for client connections
#define RFHUTIL_MOST_RECENT_USE_CSP			"useCSP"
#define RFHUTIL_MOST_RECENT_USE_SSL			"useSSL"
#define RFHUTIL_MOST_RECENT_CIPHER_SPEC		"SSLCipherSpec"
#define RFHUTIL_MOST_RECENT_SSL_KEYR		"SSLKeyR"
#define RFHUTIL_MOST_RECENT_SSL_VALIDATE	"SSLValidate"
#define RFHUTIL_MOST_RECENT_SSL_RESET_COUNT	"SSLResetCount"
#define RFHUTIL_MOST_RECENT_CONN_USER		"connUser"
#define RFHUTIL_MOST_RECENT_CONN_PW			"connPW"
#define RFHUTIL_MOST_RECENT_CONN_SEC_EXIT	"SecurityExit"
#define RFHUTIL_MOST_RECENT_CONN_SEC_DATA	"SecurityData"
#define RFHUTIL_MOST_RECENT_CONN_LOC_ADDR	"LocalAddress"

// most recently used file path
#define RFHUTIL_MOST_RECENT_FILE_PATH		"FilePath"

// default fixed width font height for low res displays (dpi < 100)
#define DEFAULT_FIXED_FONT	14

// top level registry key for most recently used info
#ifdef MQCLIENT
#define RFHUTIL_SECTION_NAME "RFHUtilcLastUsed"
#define RFHUTIL_LAST_CLIENT_QM30 "RFHUTIL_LAST_CLIENT_QM30"*/
#else
#define RFHUTIL_SECTION_NAME "RFHUtilLastUsed"
#endif

// Registry entries for MQ
#define MQKEY				"SOFTWARE\\IBM\\MQSeries\\CurrentVersion"
#define MQKEY64				"SOFTWARE\\Wow6432Node\\IBM\\MQSeries\\CurrentVersion"
#define MQ_PRODUCT_TYPE		"ProductType"
#define MQ_SERVER_VERSION	"MQServerVersion"
#define MQ_SERVER_RELEASE	"MQServerRelease"
#define MQ_VRMF				"VRMF"

// maximum number of bytes displayed on one line when dumping out data areas to the trace file
#define		MAX_TRACE_BYTES_PER_LINE		32

	static char compileDate[] = "Compiled (" __DATE__ " at " __TIME__ ")";

/////////////////////////////////////////////////////////////////////////////
// CRfhutilApp

BEGIN_MESSAGE_MAP(CRfhutilApp, CWinApp)
	//{{AFX_MSG_MAP(CRfhutilApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRfhutilApp construction

CRfhutilApp::CRfhutilApp()
{
	RECT	rect;
	int		defFontSize;
	int		i;
	char	traceInfo[256];
	LOGFONT	lf;

	// Place all significant initialization in InitInstance
	majorVersion = 0;
	minorVersion = 0;
	revisionVersion = 0;
	buildVersion = 0;
	consoleInputCP = 0;
	consoleOutputCP = 0;
	is64bit = FALSE;
	versionDone = false;
	captionSize = 0;
	menuSize = 0;
	m_mainWnd = NULL;
//	qNames = NULL;
	initQMname.Empty();
	initQname.Empty();
	initRemoteQMname.Empty();
	initSSLCipherSpec.Empty();
	initSSLKeyR.Empty();
	initConnUser.Empty();
	initConnPW.Empty();
	initFilePath.Empty();
	initSSLResetCount = 0;
	initUseSSL = FALSE;
	initSSLValidateClient = FALSE;
	initUseCSP = FALSE;
	m_enableRecentFileList = TRUE;
	auditFileName = NULL;
	numMonitors = 0;
	numDisplays = 0;
	height = 0;
	width = 0;
	dpi = 0;
	dpi2 = 0;
	technology = -1;
	horSize = 0;
	vertSize = 0;
	horRes = 0;
	vertRes = 0;
	horLogPixels = 0;
	vertLogPixels = 0;
	charWidthAvg = 0;
	charWidthMax = 0;
	charHeight = 0;
	scrHinch = 0;
	scrWinch = 0;
	m_fontFixedFound = FALSE;
	memset(fontFixedCharSets, 0, sizeof(fontFixedCharSets));

	japanOK = false;
	koreaOK = false;
	chinaBig5OK = false;
	chinaSimpOK = false;
	thaiOK = false;
	russianOK = false;

	// default values for write pubs dialog
	psWriteNewMsgid = TRUE;
	psWriteNewCorrelid = TRUE;
	psWriteUseTopic = TRUE;
	psWriteUseProps = TRUE;
	psWriteUseMQMD = TRUE;
	psWriteWriteOnce = FALSE;
	psWriteWaitTime = "";
	psWriteMsgCount = "";
	psWriteBatchSize = "1";

	// clear the type of MQ install
	MQServerVersion=0;
	MQServerRelease=0;
	memset(productType, 0, sizeof(productType));
	memset(VRMF, 0, sizeof(VRMF));

	// indicate free allocations not yet called
	freeAllocNotCalled = TRUE;

	// initialize trace file to NULL (no trace enabled)
	traceFile = NULL;
	verboseTrace = FALSE;

#ifdef MQCLIENT
	for (int i=0; i<RFHUTIL_LAST_CLIENT_MAX; i++)
	{
		lastClientQM[i].Empty();
	}
#endif

	// pcParam must point to RECT
	// get the height and width of the main screen
	if (SystemParametersInfoA(SPI_GETWORKAREA, NULL, &rect, 0))
	{
		// remember the size of the work area
		height = rect.bottom - rect.top;
		width = rect.right - rect.left;
	}

	// get the level of the local MQ installation
	getMQLevel();

	// initialize the trace file
	startTrace();

	// get the width of the primary display in pixels
	width = GetSystemMetrics(SM_CXFULLSCREEN);
	numDisplays = GetSystemMetrics(SM_CMONITORS);

	// get a device context to the main display
	HDC hDC = GetDC(0);

	if (hDC != NULL)
	{
		// get the horizontal resolution
		dpi = GetDeviceCaps(hDC, LOGPIXELSX);

		// enumerate fonts on system
		findFonts(hDC, ANSI_CHARSET);

		if (isTraceEnabled())
		{
			// put the information in the trace
			logTraceEntry("Enumerating Simplified Chinese fonts");
		}

		findFonts(hDC, GB2312_CHARSET);

		if (isTraceEnabled())
		{
			// put the information in the trace
			sprintf(traceInfo, "Simplified Chinese fontface is %s", (LPCTSTR)simpChFontface);
			logTraceEntry(traceInfo);
			logTraceEntry("Enumerating Korean fonts");
		}

		findFonts(hDC, HANGEUL_CHARSET);

		if (isTraceEnabled())
		{
			// put the information in the trace
			sprintf(traceInfo, "Korean fontface is %s", (LPCTSTR)koreaFontface);
			logTraceEntry(traceInfo);
			logTraceEntry("Enumerating Traditional Chinese fonts");
		}

		findFonts(hDC, CHINESEBIG5_CHARSET);

		if (isTraceEnabled())
		{
			// put the information in the trace
			sprintf(traceInfo, "Traditional Chinese fontface is %s", (LPCTSTR)tradChFontface);
			logTraceEntry(traceInfo);
			logTraceEntry("Enumerating Japanese fonts");
		}

		findFonts(hDC, SHIFTJIS_CHARSET);

		if (isTraceEnabled())
		{
			// put the information in the trace
			sprintf(traceInfo, "Japanese fontface is %s", (LPCTSTR)japanFontface);
			logTraceEntry(traceInfo);
			logTraceEntry("Enumerating Thai fonts");
		}

		findFonts(hDC, THAI_CHARSET);

		if (isTraceEnabled())
		{
			// put the information in the trace
			sprintf(traceInfo, "Thai fontface is %s", (LPCTSTR)thaiFontface);
			logTraceEntry(traceInfo);
			logTraceEntry("Enumerating Russian fonts");
		}

		findFonts(hDC, RUSSIAN_CHARSET);

		// release the device context
		ReleaseDC(0, hDC);
		hDC = NULL;
	}
	else
	{
		logTraceEntry("hDC is NULL");
	}

	// build a trace entry
	sprintf(traceInfo, "screen width=%d dpi=%d", width, dpi);

	// write the information to the trace file
	logTraceEntry(traceInfo);

	// get any environment overrides
	getEnvParms();

	// set the maximum display queue wait time to a default value
	maxDispTime = MAX_DISPLAY_TIME_DEFAULT;

#ifdef _DEBUG
	// enable memory leak detection
	afxMemDF |= checkAlwaysMemDF;
#endif

	// count the number of monitors on the system
	countMonitors();

	defFontSize = DEFAULT_FIXED_FONT;
	// check if the dpi has been set in rfhutil.cpp constructor
	if (dpi > 100)
	{
		// adjust the default font size for high res displays (e.g. 300 dpi vs 96 dpi)
		defFontSize = (defFontSize * dpi) / 96;
	}

	// create an alternate fixed font
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight = defFontSize;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
	lf.lfPitchAndFamily = FF_MODERN | FIXED_PITCH;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	strcpy(lf.lfFaceName, "Courier New");

	// create the default fixed font for ANSI character sets
	i = m_fixed_font.CreateFontIndirectA(&lf);

	// change LOGFONT for simplified Chinese
	lf.lfHeight = (5 *defFontSize) / 4;
	lf.lfCharSet = GB2312_CHARSET;
	lf.lfQuality = DRAFT_QUALITY;
	strcpy(lf.lfFaceName, (LPCTSTR)simpChFontface);

	// create a font for display of simplified Chinese characters
	i = m_china_font.CreateFontIndirectA(&lf);

	if (0 == i)
	{
		// try a different typeface
		strcpy(lf.lfFaceName, _T("gb2312"));
		i = m_china_font.CreateFontIndirectA(&lf);
	}

	if (i != 0)
	{
		chinaSimpOK = true;
	}

	// change LOGFONT for Korean
	lf.lfHeight = (5 * defFontSize) / 4;
	lf.lfCharSet = HANGEUL_CHARSET;
	strcpy(lf.lfFaceName, (LPCTSTR)koreaFontface);

	// create a font for display of Korean characters
	i = m_korea_font.CreateFontIndirectA(&lf);

	if (0 == i)
	{
		// try a different typeface
		strcpy(lf.lfFaceName, _T("korean"));
		i = m_korea_font.CreateFontIndirectA(&lf);
	}

	if (i != 0)
	{
		koreaOK = true;
	}

	// change LOGFONT for Japanese
	lf.lfHeight = (5 * defFontSize) / 4;
	lf.lfCharSet = SHIFTJIS_CHARSET;
	strcpy(lf.lfFaceName, (LPCTSTR)japanFontface);

	// create a font for display of Japanese characters
	i = m_japan_font.CreateFontIndirectA(&lf);

	if (0 == i)
	{
		// try a different typeface
		strcpy(lf.lfFaceName, _T("shiftjis"));
		i = m_japan_font.CreateFontIndirectA(&lf);
	}

	if (i != 0)
	{
		japanOK = true;
	}

	// change LOGFONT for Traditional Chinese
	lf.lfHeight = (5 * defFontSize) / 4;
	lf.lfCharSet = CHINESEBIG5_CHARSET;
	strcpy(lf.lfFaceName, (LPCTSTR)tradChFontface);

	// create a font for display of Traditional Chinese characters
	i = m_big5_font.CreateFontIndirectA(&lf);

	if (0 == i)
	{
		// try a different typeface
		strcpy(lf.lfFaceName, _T("big5"));
		i = m_big5_font.CreateFontIndirectA(&lf);
	}

	if (i != 0)
	{
		chinaBig5OK = true;
	}

	// change LOGFONT for Russian
	lf.lfHeight = defFontSize;
	lf.lfCharSet = RUSSIAN_CHARSET;
	strcpy(lf.lfFaceName, _T("Courier New"));

	// create a font for display of Russian characters
	i = m_russian_font.CreateFontIndirectA(&lf);

	if (i != 0)
	{
		russianOK = true;
	}

	// change LOGFONT for Thai
	lf.lfCharSet = THAI_CHARSET;
	lf.lfHeight = 2 * defFontSize;
	strcpy(lf.lfFaceName, (LPCTSTR)thaiFontface);

	// create a font for display of Thai characters
	i = m_thai_font.CreateFontIndirectA(&lf);

	if (0 == i)
	{
		// try a different typeface
		strcpy(lf.lfFaceName, _T("Lucida Sans Typewriter Regular"));
		lf.lfHeight = defFontSize;
		i = m_thai_font.CreateFontIndirectA(&lf);
	}

	if (i != 0)
	{
		// check if the support is supported and installed
//		if (IsValidLocale(MAKELCID(MAKELANGID(LANG_THAI, SUBLANG_NEUTRAL), SORT_DEFAULT), LCID_INSTALLED))
		{
			thaiOK = true;
		}
	}
}

CRfhutilApp::~CRfhutilApp()
{
	char	traceInfo[128];

	// done with the font objects
	m_fixed_font.DeleteObject();
	m_china_font.DeleteObject();
	m_korea_font.DeleteObject();
	m_japan_font.DeleteObject();
	m_big5_font.DeleteObject();
	m_thai_font.DeleteObject();
	m_russian_font.DeleteObject();

	// check if the trace file was open
	if (traceFile != NULL)
	{
		// dump out the enum stats
		sprintf(traceInfo, "enumCallbacks=%d", enumCallbacks);
		logTraceEntry(traceInfo);
		fclose(traceFile);
	}
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRfhutilApp object

CRfhutilApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CRfhutilApp initialization

BOOL CRfhutilApp::InitInstance()

{
	// do initialization for some MFC routines
	int ret = CoInitialize(NULL);

	// is trace enabled?
	if (isTraceEnabled())
	{
		// tell if initialize did not work
		if (ret != S_OK)
		{
			// record the failure in the trace
			logTraceEntry("CoInitialize() failed in CRfhutilApp::InitInstance");
		}
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("IBM\\RFHUtil"));

	LoadStdProfileSettings(6);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CRfhutilDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CRfhutilView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// look for an initial queue manager and queue name on the command line
	processCmdLine();

	// check for the last used queue manager, queue name, etc from the registry
	GetLastUsedQNames();

	// search for MQ 7.1 installation
	pDocument.SearchForMQ71();

	// be graceful if the user specified a command line argument
	if (cmdInfo.m_strFileName.GetLength() > 0)
	{
		cmdInfo.m_strFileName.Empty();
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;
	}

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles();

	// get the audit file name if present
	auditFileName = getenv(RFHUTIL_AUDIT_FILE);

	// dump out the fonts that are in use
	dumpFontNames();

	// initialize the object that retrieves the queue names for the combo box
	pDocument.processLocalQMgrs();

	// check if a default queue manager was found
	if (pDocument.defaultQMname.GetLength() == 0)
	{
		// get the default Queue Manager name
		pDocument.getDefaultQM();
	}

	return TRUE;
}

BOOL CALLBACK countMonitorsCallback(HMONITOR hMon, HDC hdc, LPRECT rect, LPARAM parms)

{
	CRfhutilApp		*app = (CRfhutilApp *)AfxGetApp();
	MONITORINFOEX	monInfo;
	int				hPixels;

	// increase the monitor count
	app->numMonitors++;

	if (hMon != NULL)
	{
		// get information about the monitor
		monInfo.cbSize = sizeof(monInfo);

		if (GetMonitorInfo(hMon, &monInfo))
		{
			hPixels = monInfo.rcWork.right - monInfo.rcWork.left;
		}
	}

	// continue the process
	return true;
}

////////////////////////////////////
//
//  count the number of monitors
//  attached to the system
//
////////////////////////////////////

void CRfhutilApp::countMonitors()

{
	// start the enumeration, which will happen in the callback routint
	EnumDisplayMonitors(NULL, NULL, countMonitorsCallback, NULL);
}

////////////////////////////////////
//
// Write font names and types to
// the trace file.
//
////////////////////////////////////

void CRfhutilApp::dumpFontNames()

{
	LOGFONT	lf;
	HFONT	hFont;
	char	traceInfo[512];			// work variable to build trace message

	if (isTraceEnabled())
	{
		// create the trace line
		sprintf(traceInfo, "chinaBig5OK=%d chinaSimpOK=%d japanOK=%d koreaOK=%d thaiOK=%d russianOK=%d numMonitors=%d numDisplays=%d", chinaBig5OK, chinaSimpOK, japanOK, koreaOK, thaiOK, russianOK, numMonitors, numDisplays);

		// write data to trace
		logTraceEntry(traceInfo);

		// get info on fixed font
		if (m_fixed_font.GetLogFont(&lf))
		{
			// get the HFONT handle
			hFont = (HFONT)m_fixed_font;

			// create the trace line
			sprintf(traceInfo, "fixed font=%d height=%d width=%d pitch=%d face name %s", lf.lfCharSet, lf.lfHeight, lf.lfWidth, lf.lfPitchAndFamily, lf.lfFaceName);

			// write data to trace
			logTraceEntry(traceInfo);
		}

		// get info on chinese font
		if (m_china_font.GetLogFont(&lf))
		{
			// create the trace line
			sprintf(traceInfo, "china font is %d height=%d width=%d pitch=%d face name %s", lf.lfCharSet, lf.lfHeight, lf.lfWidth, lf.lfPitchAndFamily, lf.lfFaceName);

			// write data to trace
			logTraceEntry(traceInfo);
		}

		// get info on big5 font
		if (m_big5_font.GetLogFont(&lf))
		{
			// create the trace line
			sprintf(traceInfo, "big5 font is %d height=%d width=%d pitch=%d face name %s", lf.lfCharSet, lf.lfHeight, lf.lfWidth, lf.lfPitchAndFamily, lf.lfFaceName);

			// write data to trace
			logTraceEntry(traceInfo);
		}

		// get info on korean font
		if (m_korea_font.GetLogFont(&lf))
		{
			// create the trace line
			sprintf(traceInfo, "korean font is %d height=%d width=%d pitch=%d face name %s", lf.lfCharSet, lf.lfHeight, lf.lfWidth, lf.lfPitchAndFamily, lf.lfFaceName);

			// write data to trace
			logTraceEntry(traceInfo);
		}

		// get info on Japanese font
		if (m_japan_font.GetLogFont(&lf))
		{
			// create the trace line
			sprintf(traceInfo, "japanese font is %d height=%d width=%d pitch=%d face name %s", lf.lfCharSet, lf.lfHeight, lf.lfWidth, lf.lfPitchAndFamily, lf.lfFaceName);

			// write data to trace
			logTraceEntry(traceInfo);
		}

		// get info on Russian font
		if (m_russian_font.GetLogFont(&lf))
		{
			// create the trace line
			sprintf(traceInfo, "russian font is %d height=%d width=%d pitch=%d face name %s", lf.lfCharSet, lf.lfHeight, lf.lfWidth, lf.lfPitchAndFamily, lf.lfFaceName);

			// write data to trace
			logTraceEntry(traceInfo);
		}

		// get info on Thai font
		if (m_thai_font.GetLogFont(&lf))
		{
			// create the trace line
			sprintf(traceInfo, "thai font is %d height=%d width=%d pitch=%d face name %s", lf.lfCharSet, lf.lfHeight, lf.lfWidth, lf.lfPitchAndFamily, lf.lfFaceName);

			// write data to trace
			logTraceEntry(traceInfo);
		}
	}
}


//////////////////////////////////////////////////////////////
//
// routine to parse the command line parameters
// the first parameter is assumed to be a queue manager name
// and the second parameter is assumed to be a queue name
//
//////////////////////////////////////////////////////////////

void CRfhutilApp::processCmdLine()

{
	char	*ptr;

	// get a pointer to the first command
	ptr = strtok(this->m_lpCmdLine, " ");

	// loop until there are no more parameters or we have found both
	// a queue manager name and a queue name
	while (ptr != NULL)
	{
		// make sure the first character is not a dash
		if ('-' == ptr[0])
		{
			// check for keyword parameter
		}
		else
		{
			// check if we have already found a queue manager name
			if (initQMname.GetLength() == 0)
			{
				// first parameter is the initial queue manager name
				initQMname = ptr;
			}
			else
			{
				// check if we have found a queue name yet
				if (initQname.GetLength() == 0)
				{
					// second parameter is an initial queue name
					initQname = ptr;
				}
				else
				{
					if (initRemoteQMname.GetLength() == 0)
					{
						// set the remote queue manager name from the third parameter
						initRemoteQMname = ptr;
					}
				}
			}
		}

		// find the next parameter
		ptr = strtok(NULL, " ");
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CString	m_ver;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	CRfhutilApp	*app = (CRfhutilApp	*) AfxGetApp();
	char		strVersion[256];

	// build a string that we use if the about dialog is
	// displayed.  the string contains the build number and
	// version information that is defined in the version resource.
	sprintf(strVersion, "V%d.%d.%d Build %d Date %s",
			app->getMajor(),
			app->getMinor(),
			app->getRevision(),
			app->getBuild(),
			compileDate);

	//{{AFX_DATA_INIT(CAboutDlg)
	m_ver = _T("");
	//}}AFX_DATA_INIT

	m_ver = strVersion;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_ABOUT_VERSION, m_ver);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CRfhutilApp::OnAppAbout()
{
	CRfhutilApp	*app = (CRfhutilApp	*) AfxGetApp();
	char		strVersion[256];

	// build a string that we use if the about dialog is
	// displayed.  the string contains the build number and
	// version information that is defined in the version resource.
	sprintf(strVersion, "V%d.%d.%d Build %d Date %s",
			app->getMajor(),
			app->getMinor(),
			app->getRevision(),
			app->getBuild(),
			compileDate);

	CAboutDlg aboutDlg;

	// set the value to include the current version and compile date
	aboutDlg.m_ver = strVersion;

	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CRfhutilApp message handlers

void CRfhutilApp::getProductName(CString &prodName)
{
	getVersInfo();

	prodName = m_prodName;
}

void CRfhutilApp::getCompanyName(CString &compName)
{
	getVersInfo();

	compName = m_compName;
}

void CRfhutilApp::getVersInfo()

{
	DWORD	size;
	char	*m_versInfo;
	WORD	*nameInfo;
	char	*value;
	UINT	nameLen;
	char	blockName[256];
	char	szModuleName[_MAX_PATH];
	TCHAR	moduleName[_MAX_PATH];
	DWORD	handle = 0L;
	DWORD	verLS;
	DWORD	verMS;
	VS_FIXEDFILEINFO	*vf;

	if (!versionDone)
	{
		size = GetModuleFileName(NULL, szModuleName, sizeof(szModuleName));
		if (size > 0)
		{
			OemToChar(szModuleName, moduleName);
			size = ::GetFileVersionInfoSize(szModuleName, &handle);
			if (size > 0)
			{
				m_versInfo = (char *)rfhMalloc(size, "VERSINFO");

				if (GetFileVersionInfo(szModuleName, handle, size, m_versInfo))
				{
					versionDone = true;
					if (VerQueryValue(m_versInfo, "\\", (void **)&vf, (UINT *)&size))
					{
						// get the version info
						verLS = vf->dwProductVersionLS;
						verMS = vf->dwProductVersionMS;
						majorVersion = verMS >> 16;
						minorVersion = verMS & 0xffff;
						revisionVersion = verLS >> 16;
						buildVersion = verLS & 0xffff;

						// get the product name
						if (::VerQueryValue(m_versInfo,"\\VarFileInfo\\Translation", (void **)&nameInfo, &nameLen))
						{
							*(DWORD *)nameInfo = MAKELONG(HIWORD(*(DWORD *)nameInfo), LOWORD(*(DWORD *)nameInfo));

							//sprintf(blockName, "\\StringFileInfo\\%08x\\ProductName", *(LPWORD *)nameInfo);
							sprintf(blockName, "\\StringFileInfo\\%08x\\ProductName", *nameInfo);
							if (::VerQueryValue(m_versInfo, blockName, (void **)&value, (UINT *)&size))
							{
								m_prodName = value;
							}

							sprintf(blockName, "\\StringFileInfo\\%08x\\CompanyName", *nameInfo);
							if (::VerQueryValue(m_versInfo, blockName, (void **)&value, (UINT *)&size))
							{
								m_compName = value;
							}

							sprintf(blockName, "\\StringFileInfo\\%08x\\LegalCopyright", *nameInfo);
							if (::VerQueryValue(m_versInfo, blockName, (void **)&value, (UINT *)&size))
							{
								m_copyRight = value;
							}
						}
					}
				}

				rfhFree(m_versInfo);
			}
		}
	}
}


int CRfhutilApp::getMajor()

{
	getVersInfo();

	return majorVersion;
}

int CRfhutilApp::getMinor()

{
	getVersInfo();

	return minorVersion;
}

int CRfhutilApp::getRevision()

{
	getVersInfo();

	return revisionVersion;
}

int CRfhutilApp::getBuild()
{
	getVersInfo();

	return buildVersion;
}

void CRfhutilApp::getCopyright(CString &copyRight)
{
	getVersInfo();

	copyRight = m_copyRight;
}

int CRfhutilApp::ExitInstance()

{
	// clean up any open MQ resources
	// is there a browse active
	if (pDocument.browseActive)
	{
		// end it
		pDocument.endBrowse(TRUE);
	}
	else if (pDocument.isSubscriptionActive())
	{
		// close the subscription
		pDocument.closePSQueue(FALSE);
	}

	// make sure to disconnect from the queue manager (and close the queue)
	pDocument.discQM();

	// update the registry with the most recent queue manager and queue name, etc values
	SaveLastUsedQNames();

	// free any storage used in the DataArea object
	pDocument.freeAllocations();

	// write the storage statistics to the trace file
	// the storage statistics are maintained as global variables in comsubs.cpp
	rfhTraceStats();

	// clean up for MFC
	CoUninitialize();

	return 0;  
}

//////////////////////////////////////
//
// Create fixed width font
//
//////////////////////////////////////

int CRfhutilApp::createFixedFonts(CFont *font, const char *faceName, BYTE pitchFamily)

{
	int			rc;
	LOGFONT		lf;
	LONG		height=14;
	char		traceInfo[128];

	// check if the dpi is > 100
	if (dpi > 100)
	{
		height *= dpi;
		height = height / 100;
	}

	// create a fixed font
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight = height;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfPitchAndFamily = pitchFamily;
	//lf.lfPitchAndFamily = FIXED_PITCH;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	strcpy(lf.lfFaceName, "Courier New");

	// create the font
	rc = font->CreateFontIndirectA(&lf);

	// check if trace if enabled
	if (isTraceEnabled())
	{
		// build the trace line
		sprintf(traceInfo, "CRfhutilApp::createFixedFonts rc=%d height=%d face=%s", rc, height, lf.lfFaceName);

		// write the line to the trace file
		logTraceEntry(traceInfo);
	}

	return rc;
}

///////////////////////////////////////////////////////
//
// function to check 64 bit field for particular code
//  page, specified as offset
//
///////////////////////////////////////////////////////

BOOL CRfhutilApp::checkFontCharsForFont(const unsigned char * charFont, int fontOffset)

{
	int				byteOffset;
	int				bitOffset;
	int				mask;
	char			traceInfo[256];
	unsigned char	ch;
	unsigned char	hexCh[4];

	// calculate the byte offset
	byteOffset = fontOffset / 8;
	//byteOffset = 7 - fontOffset / 8;
	bitOffset = fontOffset % 8;
	mask = 128;
	mask = mask >> bitOffset;

	// get the character from the array
	ch = (charFont)[byteOffset];

	// check if trace is enabled
	if (isTraceEnabled() && verboseTrace)
	{

		// convert ch to hex for trace
		memset(hexCh, 0, sizeof(hexCh));
		AsciiToHex(&ch, 1, hexCh);

		// build a trace line
		sprintf(traceInfo, "CRfhutilApp::checkFontCharsForFont fontOffset=%d byteOffset=%d bitOffset=%d mask=%d ch=%s", fontOffset, byteOffset, bitOffset, mask, hexCh);

		// write to the trace log
		//logTraceEntry(traceInfo);
	}

	// check if the particular bit is on or not
	if ((ch & mask) > 0)
	{
		// bit is on, return true
		return TRUE;
	}
	else
	{
		// bit is off, return false
		return FALSE;
	}
}

///////////////////////////////////////////////////////
//
// Callback function used to enumerate installed fonts
//
///////////////////////////////////////////////////////

int CALLBACK EnumFixedFonts(
	const LOGFONT    *lf,
	const TEXTMETRIC *txtmetric,
	DWORD      FontType,
	LPARAM     lParam)

{
	// set a pointer to the application object
	// this parameter is set by the original call
	// to EnumFontFamiliesEx call below
	CRfhutilApp				*app = (CRfhutilApp *)lParam;
	const TEXTMETRIC		*tm = NULL;
	const NEWTEXTMETRICEX	*tt = NULL;
	char					fontCS[64];
	char					traceInfo[256];
	int						rc = 1;				// default to continue
	int						dpi = 0;
	BOOL					trueType=FALSE;
	BYTE					pitchFam = 0;

	app->enumCallbacks++;

	// get the pitch and family
	pitchFam = lf->lfPitchAndFamily;

	// check for Simplified Chinese fonts
	if (GB2312_CHARSET == lf->lfCharSet)
	{
		// check for SimHei font
		if (strcmp(lf->lfFaceName, "SimHei") == 0)
		{
			// select this font
			app->simpChFontface = lf->lfFaceName;

			// done with enumeration
			rc = 0;
		}
		// check for gb2312
		else if (strcmp(lf->lfFaceName, "gb2312") == 0)
		{
			app->simpChFontface = lf->lfFaceName;
		}
	}


	// check for Traditional Chinese fonts
	if (CHINESEBIG5_CHARSET == lf->lfCharSet)
	{
		// check for Microsoft JhengHei font
		if (strcmp(lf->lfFaceName, "Microsoft JhengHei") == 0)
		{
			// select this font
			app->tradChFontface = lf->lfFaceName;

			// done with enumeration
			rc = 0;
		}
		// check for big5
		else if (strcmp(lf->lfFaceName, "big5") == 0)
		{
			app->tradChFontface = lf->lfFaceName;
		}
	}

	// check for Japanese fonts
	if (SHIFTJIS_CHARSET == lf->lfCharSet)
	{
		// check for MS Gothic font
		if (strcmp(lf->lfFaceName, "MS Gothic") == 0)
		{
			// select this font
			app->japanFontface = lf->lfFaceName;

			// done with enumeration
			rc = 0;
		}
		// check for big5
		else if (strcmp(lf->lfFaceName, "shiftjis") == 0)
		{
			app->japanFontface = lf->lfFaceName;
		}
	}

	// check for Korean fonts
	if (HANGEUL_CHARSET == lf->lfCharSet)
	{
		// check for BatangChe font
		if (strcmp(lf->lfFaceName, "BatangChe") == 0)
		{
			// select this font
			app->koreaFontface = lf->lfFaceName;

			// done with enumeration
			rc = 0;
		}
		// check for korean
		else if (strcmp(lf->lfFaceName, "korean") == 0)
		{
			app->koreaFontface = lf->lfFaceName;
		}
	}

	// check for Thai fonts
	if (THAI_CHARSET == lf->lfCharSet)
	{
		// check for Browallial New font
		if (strcmp(lf->lfFaceName, "Browallia New") == 0)
		{
			// select this font
			app->thaiFontface = lf->lfFaceName;

			// done with enumeration
			rc = 0;
		}
		// check for AngSana New font
		else if (strcmp(lf->lfFaceName, "Angsana New") == 0)
		{
			app->thaiFontface = lf->lfFaceName;
		}
	}

	// check for fixed pitch fonts only
	//if ((pitchFam & FIXED_PITCH) == FIXED_PITCH)
	//{
		// check if verbose trace is enabled
		if (app->isTraceEnabled() && app->fontTrace)
		{
			// clear a work field
			memset(fontCS, 0, sizeof(fontCS));

			// build the trace line
			// get the name of the font
			strcpy(traceInfo, "faceName=");
			strcat(traceInfo, lf->lfFaceName);

			// check for true type font
			if ((FontType & TRUETYPE_FONTTYPE) == TRUETYPE_FONTTYPE)
			{
				// remember that this is a true type font
				trueType = TRUE;

				// add to trace info
				strcat(traceInfo, " TrueType");

				// set a point to a true type textmetric
				tt = (NEWTEXTMETRICEX *)txtmetric;
				memcpy(app->fontFixedCharSets, tt->ntmFontSig.fsCsb, sizeof(app->fontFixedCharSets));

				// get the field in hex for trace
				AsciiToHex((const unsigned char *)&(app->fontFixedCharSets), 8, (unsigned char *)fontCS);
				strcat(traceInfo, " ");
				strcat(traceInfo, fontCS);
			}
			else
			{
				// not a true type font
				// use regular text metric structure
				tm = (TEXTMETRICA *)txtmetric;

				// check for device font
				if ((FontType & DEVICE_FONTTYPE) == DEVICE_FONTTYPE)
				{
					// add to trace info
					strcat(traceInfo, " Device");
				}

				// check for raster font
				if ((FontType & RASTER_FONTTYPE) == RASTER_FONTTYPE)
				{
					// add to trace info
					strcat(traceInfo, " Raster");
				}

				// get the height of the font
				sprintf(fontCS, " height=%d, maxWidth=%d", tm->tmHeight, tm->tmMaxCharWidth);
				strcat(traceInfo, fontCS);
			}

			// append information from the logfont
			if ((pitchFam & 3) == FIXED_PITCH)
			{
				strcat(traceInfo, " Fixed");
			}

			// append information from the logfont
			if ((pitchFam & 0x0c) == FF_MODERN)
			{
				strcat(traceInfo, " FF Modern");
			}

			// check supported code pages for True Type fonts
			if (trueType)
			{
				// check for 437 US code page
				if (app->checkFontCharsForFont((unsigned char *)&(app->fontFixedCharSets), CP_US_437))
				{
					// show support
					strcat(traceInfo, " US 437");
				}

				// check for Simplified Chinese code page
				if (app->checkFontCharsForFont((unsigned char *)&(app->fontFixedCharSets), CP_SIMP_CH_936))
				{
					// show support
					strcat(traceInfo, " Simp Ch");
				}

				// check for Traditional Chinese code page
				if (app->checkFontCharsForFont((unsigned char *)&(app->fontFixedCharSets), CP_TRAD_CH_950))
				{
					// show support
					strcat(traceInfo, " Trad Ch");
				}

				// check for Korean
				if (app->checkFontCharsForFont((unsigned char *)&(app->fontFixedCharSets), CP_KOREA_949))
				{
					// show support
					strcat(traceInfo, " Korea");
				}

				// check for Korean 1361
				if (app->checkFontCharsForFont((unsigned char *)&(app->fontFixedCharSets), CP_KOREA_1361))
				{
					// show support
					strcat(traceInfo, " Korea2");
				}

				// check for Thai
				if (app->checkFontCharsForFont((unsigned char *)&(app->fontFixedCharSets), CP_THAI_874))
				{
					// show support
					strcat(traceInfo, " Thai");
				}

				// check for Russia
				if (app->checkFontCharsForFont((unsigned char *)&(app->fontFixedCharSets), CP_RUSSIA_866))
				{
					// show support
					strcat(traceInfo, " Russia");
				}

				// check for Cyrillic
				if (app->checkFontCharsForFont((unsigned char *)&(app->fontFixedCharSets), CP_CYRILLIC_1251))
				{
					// show support
					strcat(traceInfo, " Cyrillic");
				}

				// check for Japan
				if (app->checkFontCharsForFont((unsigned char *)&(app->fontFixedCharSets), CP_JAPAN_932))
				{
					// show support
					strcat(traceInfo, " Japan");
				}
			}

			// write the trace line to the trace file
			app->logTraceEntry(traceInfo);
		}
	//}

	// continue enumeration
	return rc;
}

// enumerate fixed fonts installed on the system
void CRfhutilApp::findFonts(HDC hdc, BYTE charSet)

{
	CRfhutilApp	*app = (CRfhutilApp *)AfxGetApp();
	LOGFONT		lf;
	int			rc = -1;
	char		traceInfo[128];

	memset(&lf, 0, sizeof(lf));
	enumCallbacks = 0;

	// prepare the logfont
	// note that the lfFaceName parameter is set
	// to a null string by the memset above
	lf.lfCharSet = charSet;
	lf.lfPitchAndFamily = FIXED_PITCH;

	rc = EnumFontFamiliesEx(hdc, &lf, EnumFixedFonts, (LPARAM)app, 0);

	// check if trace is enabled
	if (isTraceEnabled())
	{
		// prepare the trace information
		sprintf(traceInfo, "CRfhutilApp::findFonts() rc=%d enumCallbacks=%d", rc, enumCallbacks);

		// write the trace line to the trace
		logTraceEntry(traceInfo);
	}
}

void CRfhutilApp::SaveLastUsedQNames()

{
	LPCTSTR		sectionName=RFHUTIL_SECTION_NAME;
#ifdef MQCLIENT
	char		tempNum[16];
#endif

	// save the last used queue name in the regsitry
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_QMGR, (LPCTSTR)initQMname);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_QUEUE, (LPCTSTR)initQname);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_REMOTE, (LPCTSTR)initRemoteQMname);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_USER, (LPCTSTR)initConnUser);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_PW, (LPCTSTR)initConnPW);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_FILE_PATH, (LPCTSTR)initFilePath);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_SEC_EXIT, (LPCTSTR)initSecExit);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_SEC_DATA, (LPCTSTR)initSecData);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_LOC_ADDR, (LPCTSTR)initLocalAddress);

#ifdef MQCLIENT
	sprintf(tempNum, "%d", initUseCSP);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_USE_CSP, tempNum);
	sprintf(tempNum, "%d", initUseSSL);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_USE_SSL, tempNum);
	sprintf(tempNum, "%d", initSSLValidateClient);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_SSL_VALIDATE, tempNum);
	sprintf(tempNum, "%d", initSSLResetCount);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_SSL_RESET_COUNT, tempNum);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_CIPHER_SPEC, (LPCTSTR)initSSLCipherSpec);
	WriteProfileString(sectionName, RFHUTIL_MOST_RECENT_SSL_KEYR, (LPCTSTR)initSSLKeyR);
	writeQMgr2Registry(sectionName);
#endif
}

void CRfhutilApp::GetLastUsedQNames()

{
	LPCTSTR		sectionName=RFHUTIL_SECTION_NAME;
	CString		tempNum;
	char		traceInfo[512];		// work variable to build trace message

	// check if an initial queue manager name was specified on the command line
	if (initQMname.GetLength() == 0)
	{
		// not specified - see if we saved the last one in the registry
		initQMname = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_QMGR, NULL);
	}


	// check if an initial queue name was specified on the command line
	if (initQname.GetLength() == 0)
	{
		// not specified - see if we saved the last one in the registry
		initQname = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_QUEUE, NULL);
	}

	if (initRemoteQMname.GetLength() == 0)
	{
		// see if there is a saved value in the registry
		initRemoteQMname = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_REMOTE, NULL);
	}

	// get the connection user id and password
	initConnUser = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_USER, NULL);
	initConnPW = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_PW, NULL);

	// get the most recently used file path
	initFilePath = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_FILE_PATH, NULL);

#ifdef MQCLIENT
	// if this is the client version check for saved CSP values in the registry
	tempNum.Empty();
	tempNum = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_USE_CSP, NULL);
	if (tempNum.GetLength() > 0)
	{
		initUseCSP = atoi((LPCTSTR)tempNum);
	}

	// if this is the client version check for saved SSL values in the registry
	tempNum.Empty();
	tempNum = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_USE_SSL, NULL);
	if (tempNum.GetLength() > 0)
	{
		initUseSSL = atoi((LPCTSTR)tempNum);
	}

	tempNum.Empty();
	tempNum = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_SSL_VALIDATE, NULL);
	if (tempNum.GetLength() > 0)
	{
		initSSLValidateClient = atoi((LPCTSTR)tempNum);
	}

	tempNum.Empty();
	tempNum = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_SSL_RESET_COUNT, NULL);
	if (tempNum.GetLength() > 0)
	{
		initSSLResetCount = atoi((LPCTSTR)tempNum);
	}

	initSSLCipherSpec = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_CIPHER_SPEC, NULL);
	initSSLKeyR = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_SSL_KEYR, NULL);

	// get the security exit name and data
	initSecExit = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_SEC_EXIT, NULL);
	initSecData = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_SEC_DATA, NULL);

	// get the local address
	initLocalAddress = GetProfileString(sectionName, RFHUTIL_MOST_RECENT_CONN_LOC_ADDR, NULL);

	// get the most recently used queue managers
	GetLastUsedQMgrs(sectionName);
#endif

	if (isTraceEnabled())
	{
#ifdef MQCLIENT
		// create trace entry for the SSL entries
		sprintf(traceInfo, "Exiting CRfhutilApp::GetLastUsedQNames() initUseCSP=%d initUseSSL=%d, initSSLCipherSpec=%s initSSLValidateClient=%d initSSLKeyR=%s initSSLResetCount=%d", initUseCSP, initUseSSL, (LPCTSTR)initSSLCipherSpec, initSSLValidateClient, (LPCTSTR)initSSLKeyR, initSSLResetCount);

		// write the data to the trace
		logTraceEntry(traceInfo);

		if ((initSecExit.GetLength() > 0) || (initSecData.GetLength() > 0))
		{
			// create trace entry for the security exit and data entries
			sprintf(traceInfo, "Exiting CRfhutilApp::GetLastUsedQNames() initSecExit=%.128s, initSecData=%.32s", (LPCTSTR)initSecExit, (LPCTSTR)initSecData);

			// write the data to the trace
			logTraceEntry(traceInfo);
		}
#endif
		// create trace entry
		sprintf(traceInfo, "Exiting CRfhutilApp::GetLastUsedQNames() initQMname=%s, initQname=%s initRemoteQMname=%s initConnUser=%s initConnPW=%s", (LPCTSTR)initQMname, (LPCTSTR)initQname, (LPCTSTR)initRemoteQMname, (LPCTSTR)initConnUser, (LPCTSTR)initConnPW);

		// trace exit from GetLastUsedQNames
		logTraceEntry(traceInfo);
	}
}

void CRfhutilApp::OnOpenRecFile(UINT idx)

{
	char	traceInfo[512];		// work variable to build trace message

	if (isTraceEnabled())
	{
		// create trace entry
		sprintf(traceInfo, "Entering CRfhutilApp::OnOpenRecFile() idx=%d", idx);

		// trace entry to OnOpenRecFile
		logTraceEntry(traceInfo);
	}

	TRACE("Entered CRfhutilApp::OnOpenRecFile idx %d\n", idx);
	OnOpenRecentFile(ID_FILE_MRU_FILE1 + idx);
}

CDocument* CRfhutilApp::OpenDocumentFile(LPCTSTR lpszFileName)

{
	char	fileName[512];
	char	traceInfo[512];		// work variable to build trace message

	if (isTraceEnabled())
	{
		// create trace entry
		sprintf(traceInfo, "Entering CRfhutilApp::OpenDocumentFile() fileName=%s", lpszFileName);

		// trace entry to OpenDocumentFile
		logTraceEntry(traceInfo);
	}

	// drive the document file processing
	strcpy(fileName, lpszFileName);
	pDocument.ReadFileData(fileName);

	// we need to update the page being viewed, since we have read new data
	pDocument.updateMsgText();

	// guard against the lpszFileName pointer becoming invalid, which seems to happen at random times
	return CWinApp::OpenDocumentFile(fileName);
}

void CRfhutilApp::createAuditRecord(LPCTSTR qMgr, LPCTSTR qName, LPCTSTR activity, MQLONG reason)

{
	time_t	ltime;				// number of seconds since 1/1/70
	struct	tm *today;			// today's date as a structure
	char	todaysDate[32];
	FILE *	auditFile;

	// check if trace is enabled
	if (NULL == auditFileName)
	{
		// audit is not enabled - return immediately
		return;
	}

	// get a time stamp
	memset(todaysDate, 0, sizeof(todaysDate));
	time(&ltime);
	today = localtime(&ltime);
	strftime(todaysDate, sizeof(todaysDate) - 1, "%y-%m-%d:%H.%M.%S", today);

	// try to open the audit file for output
	auditFile = fopen(auditFileName, "a");

	// check if it worked
	if (auditFile != NULL)
	{
		// write the entry to the audit file
		fprintf(auditFile, "%s %s %s on %s - result=%d\n", todaysDate, activity, qName, qMgr, reason);
		fflush(auditFile);

		// close the audit file
		fclose(auditFile);
	}
}

void CRfhutilApp::logTraceEntry(LPCTSTR traceInfo)

{
	time_t	ltime;				// number of seconds since 1/1/70
	struct	tm *today;			// today's date as a structure
	char	todaysDate[32];

	// check if trace is enabled
	if (NULL == traceFile)
	{
		// trace is not enabled - return immediately
		return;
	}

	// get a time stamp
	memset(todaysDate, 0, sizeof(todaysDate));
	time(&ltime);
	today = localtime(&ltime);
	strftime(todaysDate, sizeof(todaysDate) - 1, "%y-%m-%d:%H.%M.%S", today);

	// write the trace entry to the trace file
	fprintf(traceFile, "%s %s\n", todaysDate, traceInfo);
	fflush(traceFile);
}

BOOL CRfhutilApp::isTraceEnabled()

{
	return traceFile != NULL;
}

///////////////////////////////////////////////////////////
//
// This routine will write an arbitrary area of storage
// to the trace file in hex.
//
///////////////////////////////////////////////////////////

void CRfhutilApp::dumpTraceData(const char * label, const unsigned char *data, unsigned int length)

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

	// check if trace is enabled
	if (NULL == traceFile)
	{
		// trace is not enabled - return immediately
		return;
	}

	// check if a label line was provided
	if (label != NULL)
	{
		// create a label line with the address included
		sprintf(traceLine, "%.192s at %8.8X", label, (unsigned int)data);
	}
	else
	{
		// create a label line with the address included
		sprintf(traceLine, "memory at %8.8X", (unsigned int)data);
	}

	// write the lable line to the trace
	logTraceEntry(traceLine);

	// check if the data pointer is NULL
	if ((NULL == data) || (0 == length))
	{
		// make sure we don't blow up
		logTraceEntry("data is NULL or zero length");
		return;
	}

	// check for verbose trace
	if (!verboseTrace)
	{
		// not verbose trace
		// is the length more than 8K?
		if (length > 8 * 1024)
		{
			// build a trace line
			sprintf(traceLine, "Dumping out first 8K bytes of %d", length);

			// tell the user if the trace is being limited
			logTraceEntry(traceLine);

			// limit the data to 8K
			length = 8 * 1024;
		}
	}

	// get the number of bytes to output
	remaining = length;
	ptr = data;
	while (remaining > 0)
	{
		// get the number of bytes to output on this line
		count = remaining;
		if (count > MAX_TRACE_BYTES_PER_LINE)
		{
			count = 32;
		}

		// build the ascii part of the line
		// initialize the line to a 4 character offset plus 37 blanks (32 characters of data plus 5 blank delimiters)
		sprintf(traceLine, "%4.4X", offset);
		offset += 32;
		memset(traceLine + 4, ' ', 37);

		// process the individual characters
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

			// move on to the next character
			i++;
			j++;
			asciiptr++;

			// check if this is the fourth character
			k++;
			if (8 == k)
			{
				// reset the counter
				k = 0;

				// skip a blank character
				j++;
			}
		}

		// build the hex part of the line
		j = 41;							// pointer to the next character location to be added to the line
		k = count >> 2;					// get the count of whole 4-byte segments
		for (i=0; i<k; i++)
		{
			traceLine[j++] = ' ';		// insert a blank every 4 bytes to make it easier to read

			// get the next 4 bytes as hex characters
			AsciiToHex(ptr, 4, (unsigned char *)traceLine + j);
			j += 8;									// 4 bytes becomes 8 characters
			ptr += 4;								// advance the input pointer 4 bytes
		}

		// check if there is an odd number of bytes left
		k = count % 4;
		if (k > 0)
		{
			traceLine[j++] = ' ';
			AsciiToHex(ptr, k, (unsigned char *)traceLine + j);
			j += k * 2;								// each byte becomes two hex characters on output
			ptr += k;								// advance the input pointer
		}

		// properly terminate the string
		traceLine[j] = 0;

		//  write trace line to the trace file directly - no need for timestamps
		if (traceFile != NULL)
		{
			// write the trace entry to the trace file
			fprintf(traceFile, "%s\n", traceLine);
			fflush(traceFile);
		}

		// decrement the remaining bytes
		remaining -= count;
	}
}

///////////////////////////////////////////////////////////
//
// This routine will save the last 30 queue managers that
// have been accessed.
//
///////////////////////////////////////////////////////////

void CRfhutilApp::saveClientQM(const char *qm)

{
#ifdef MQCLIENT
	int			i;
	int			j;
	char		traceInfo[512];		// work variable to build trace message

	if (isTraceEnabled())
	{
		// create trace entry
		sprintf(traceInfo, "Entering CRfhutilApp::saveClientQM() qm=%s", qm);

		// trace entry into saveClientQM
		logTraceEntry(traceInfo);

		// check for verbose trace
		if (verboseTrace)
		{
			for (i=0; i<RFHUTIL_LAST_CLIENT_MAX; i++)
			{
				if (lastClientQM[i].GetLength() > 0)
				{
					// create a trace line
					sprintf(traceInfo, " lastClientQM[%d]=%s", i, (LPCTSTR)lastClientQM[i]);

					// write the trace entry to the trace log
					logTraceEntry(traceInfo);
				}
			}
		}
	}

	// check if the qm name is null
	if ((NULL == qm) || (0 == qm[0]))
	{
		return;
	}

	// check if this is the most recent queue manager already
	if (lastClientQM[0].Compare(qm) == 0)
	{
		// return immediately since nothing to do
		return;
	}

	// check if this queue manager is already in the list
	// N.B. do not check the last one since that would drop off anyway
	// the first entry has already been checked
	for (i=1; i<RFHUTIL_LAST_CLIENT_MAX-1; i++)
	{
		if (lastClientQM[i].Compare(qm) == 0)
		{
			break;
		}
	}

	// check for verbose trace
	if (isTraceEnabled() && verboseTrace)
	{
		// create a trace line
		sprintf(traceInfo, " found qmgr at location i=%d lastClientQM[%d]=%s lastClientQM[%d]=%s", i, i, (LPCTSTR)lastClientQM[i], i-1, (LPCTSTR)lastClientQM[i-1]);

		// write the trace entry to the trace log
		logTraceEntry(traceInfo);
	}

	// move the earlier queue managers down one in the list
	// if the entry was not found then it will move all the entries down one
	for (j = i; j > 0; j--)
	{
		lastClientQM[j] = lastClientQM[j - 1];
	}

	// set the first entry
	lastClientQM[0] = qm;

	// now update the registry with the least recently used queue managers
	writeQMgr2Registry(RFHUTIL_SECTION_NAME);

	// check for verbose trace
	if (isTraceEnabled())
	{
		// trace exit from
		logTraceEntry("Exiting CRfhutilApp::saveClientQM()");
	}
#endif
}

///////////////////////////////////////////////////////////
//
// This routine will write the last 30 queue managers that
// have been accessed to the windows registry.
//
///////////////////////////////////////////////////////////

void CRfhutilApp::writeQMgr2Registry(const char *sectionName)

{
#ifdef MQCLIENT
	int		i;
	int		j=0;
	char	regKeyStr[64];
	char	traceInfo[512];		// work variable to build trace message

		// check for verbose trace
		if (isTraceEnabled() && verboseTrace)
		{
			// create trace entry
			sprintf(traceInfo, "Entering CRfhutilApp::writeQMgr2Registry() sectionName=%s", sectionName);

			// trace entry into writeQMgr2Registry
			logTraceEntry(traceInfo);
		}

	// clear the memory key string
	memset(regKeyStr, 0, sizeof(regKeyStr));

	// write out all of the registry entries
	for (i=0; i<RFHUTIL_LAST_CLIENT_MAX; i++)
	{
		if (lastClientQM[i].GetLength() > 0)
		{
			// create the registry key
			sprintf(regKeyStr, "RFHUTIL_LAST_CLIENT_QM%d", j);
			j++;

			// write the registry key
			WriteProfileString(sectionName, regKeyStr, lastClientQM[i]);

			// check for verbose trace
			if (isTraceEnabled() && verboseTrace)
			{
				// create a trace line
				sprintf(traceInfo, " lastClientQM[%d]=%s written to %s", i, (LPCTSTR)lastClientQM[i], regKeyStr);

				// write the trace entry to the trace log
				logTraceEntry(traceInfo);
			}
		}
	}

	// clear the rest of the keys
	while (j < RFHUTIL_LAST_CLIENT_MAX)
	{
		// create the registry key
		sprintf(regKeyStr, "RFHUTIL_LAST_CLIENT_QM%d", j);
		j++;

		// write the registry key
		WriteProfileString(sectionName, regKeyStr, "");
	}
#endif
}

///////////////////////////////////////////////////////////
//
// This routine will read the last 30 queue managers that
// have been accessed from the windows registry.
//
///////////////////////////////////////////////////////////

void CRfhutilApp::GetLastUsedQMgrs(const char *sectionName)

{
#ifdef MQCLIENT
	int		i;
	char	regKeyStr[64];

	for (i=0; i<RFHUTIL_LAST_CLIENT_MAX; i++)
	{
		// create the registry key
		sprintf(regKeyStr, "RFHUTIL_LAST_CLIENT_QM%d", i);

		// read the registry key
		lastClientQM[i] = GetProfileString(sectionName, regKeyStr, NULL);
	}
#endif
}

/////////////////////////////////////////////////////////////
//
// Save the current recent file list and replace the
// current pointer with a NULL.  This is to temporarily
// suppress the recent file menu when a non-modal pop-up is
// displayed.  The find and hex find dialogs are non-modal.
//
/////////////////////////////////////////////////////////////

void CRfhutilApp::saveRecentFileList()

{
	// do not enable recent file menu
	m_enableRecentFileList = FALSE;
}

/////////////////////////////////////////////////////////////
//
// Restore the recent file list that was previously saved.
//
/////////////////////////////////////////////////////////////

void CRfhutilApp::restoreRecentFileList()

{
	// enable the recent file menu
	m_enableRecentFileList = TRUE;
}

BOOL CRfhutilApp::isRecentFileMenuEnabled()

{
	// indicate if the recent file menu is enabled
	return m_enableRecentFileList;
}

void CRfhutilApp::setMostRecentFilePath(LPCTSTR fileName)

{
	int		i;
	char	path[512];
	char	traceInfo[768];

	if (isTraceEnabled())
	{
		// create trace entry
		sprintf(traceInfo, "Entering CRfhutilApp::setMostRecentFilePath() initFilePath=%s", (LPCTSTR)initFilePath);

		// trace entry to setMostRecentFilePath
		logTraceEntry(traceInfo);
	}

	if (strlen(fileName) < sizeof(path))
	{
		// update the most recent path variable
		strcpy(path, fileName);

		// figure out how long the full path and file name is
		i = strlen(path) - 1;
		while ((i >= 0) && (path[i] != '\\'))
		{
			i--;
		}

		if (i >= 0)
		{
			// remove the file name
			path[i + 1] = 0;

			// update the initial file path
			initFilePath = path;
		}
	}

	if (isTraceEnabled())
	{
		// create trace entry
		sprintf(traceInfo, "Exiting CRfhutilApp::setMostRecentFilePath() initFilePath=%s", (LPCTSTR)initFilePath);

		// trace exit from setMostRecentFilePath
		logTraceEntry(traceInfo);
	}
}

////////////////////////////////////////////
//
// Routine to get the level of the installed
// MQ on this system.
//
// This is necessary to recognize cases
// where an MQ 5.3 client is connected
// to MQ V6.  Certain functions must not
// be used if the client is not at the
// same level as the queue manager.
//
// This routine will directly access the
// Windows registry to get the client level
// since no approved way to do this is
// provided by MQ.
//
////////////////////////////////////////////

void CRfhutilApp::getMQLevel()

{
	int				ret;
	HKEY			regkey;
	DWORD			keyValueSize;
	DWORD			version=0;
	DWORD			release=0;
	unsigned long	valueType=0;
	const char *	keyName;

	MQServerVersion=0;
	MQServerRelease=0;

	// what type of OS is this?
	if (is64bit)
	{
		// 64-bit OS
		keyName = MQKEY64;
	}
	else
	{
		// 32-bit OS
		keyName = MQKEY;
	}

	// open the MQSeries/CurrentVersion registry key
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					   keyName,
					   0,
					   KEY_QUERY_VALUE,
					   &regkey);

	if (ERROR_SUCCESS == ret)
	{
		// get the installation type - can be server or client
		keyValueSize = sizeof(productType);
		ret = RegQueryValueEx(regkey,
							  MQ_PRODUCT_TYPE,
							  NULL,
							  &valueType,
							  (unsigned char *)&productType,
							  &keyValueSize);

		// get the release and fix pack level
		keyValueSize = sizeof(VRMF);
		ret = RegQueryValueEx(regkey,
							  MQ_VRMF,
							  NULL,
							  &valueType,
							  (unsigned char *)&VRMF,
							  &keyValueSize);

		// get the server version
		keyValueSize = sizeof(version);
		ret = RegQueryValueEx(regkey,
							  MQ_SERVER_VERSION,
							  NULL,
							  &valueType,
							  (unsigned char *)&version,
							  &keyValueSize);

		// did it work?
		if (ERROR_SUCCESS == ret)
		{
			// copy the version to a public variable
			MQServerVersion = version;
		}

		// get the server release
		keyValueSize = sizeof(release);
		ret = RegQueryValueEx(regkey,
							  MQ_SERVER_RELEASE,
							  NULL,
							  &valueType,
							  (unsigned char *)&release,
							  &keyValueSize);

		// did it work?
		if (ERROR_SUCCESS == ret)
		{
			// copy the release to a public variable
			MQServerRelease = release;
		}

		// close the key
		ret = RegCloseKey(regkey);
	}
}

void CRfhutilApp::startTrace()

{
	char			traceInfo[256];
	const char *	traceFileName;
	const char *	traceTypePtr;
	const char *	fontTracePtr;
	const char *	processorArchitecture;
	const char *	processorArchitew6432;
	char			strOEM[8];
	char			strANSI[8];
	char			strLANG[8];
	char			strCOUNTRY[8];
	char			strVersion[256];


	// check for a trace file name in the environment
	// if the RFHUTIL_TRACE_FILE environment variable is set to a valid file name
	// then the trace file will be opened and tracing will be enabled
	traceFileName = getenv(RFHUTIL_TRACE_FILE);

	// is the environment variable set?
	if (traceFileName != NULL)
	{
		// try to open the trace file
		traceFile = fopen(traceFileName, "a");

		// check if the open worked
		if (traceFile != NULL)
		{
			fprintf(traceFile, "\n");
#ifdef MQCLIENT
			logTraceEntry("RFHUtilc started");
#else
			logTraceEntry("RFHUtil started");
#endif

			// build a string that we use if the about dialog is
			// displayed.  the string contains the build number and
			// version information that is defined in the version resource.
			sprintf(strVersion, "V%d.%d.%d Build %d Date %s",
					getMajor(),
					getMinor(),
					getRevision(),
					getBuild(),
					compileDate);

			// put the version of RFHUtil in the trace file
			logTraceEntry(strVersion);

			// initialize the data areas in case the get locales fail
			memset(strOEM, 0, sizeof(strOEM));
			memset(strANSI, 0, sizeof(strANSI));
			memset(strLANG, 0, sizeof(strLANG));
			memset(strCOUNTRY, 0, sizeof(strCOUNTRY));

			// try to get the current locale information and add it to the trace
			int rc = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTCODEPAGE, strOEM, sizeof(strOEM));
			int rc2 = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE, strANSI, sizeof(strANSI));
			int rc3 = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTLANGUAGE, strLANG, sizeof(strLANG));
			int rc4 = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTCOUNTRY, strCOUNTRY, sizeof(strCOUNTRY));

			// get the console code pages
			consoleInputCP = GetConsoleCP();
			consoleOutputCP = GetConsoleOutputCP();

			// get the OS type (32-bit or 64-bit)
			processorArchitecture = getenv("PROCESSOR_ARCHITECTURE");

			// make sure it worked
			if (processorArchitecture != NULL)
			{
				if (strcmp(processorArchitecture, "AMD64") == 0)
				{
					// 64-bit system
					is64bit = TRUE;
				}
				else if (strcmp(processorArchitecture, "x86") == 0)
				{
					// check for 32-bit program on 64-bit OS
					processorArchitew6432 = getenv("PROCESSOR_ARCHITEW6432");

					// check original architecture
					if ((processorArchitew6432 != NULL) && strcmp(processorArchitew6432, "AMD64") == 0)
					{
						// 64-bit system
						is64bit = TRUE;
					}
				}
			}

			// format the trace information even if any of the get locales failed
			sprintf(traceInfo, "Default OEM code page is %s ANSI %s Language %s Country %s Console Codepages(Input=%d Output=%d) is64bit=%d", strOEM, strANSI, strLANG, strCOUNTRY, consoleInputCP, consoleOutputCP, is64bit);

			// capture the locale in the trace file
			logTraceEntry(traceInfo);

			// save the locale settings
			oem = atoi(strOEM);
			ansi = atoi(strANSI);
			lang = atoi(strLANG);
			country = atoi(strCOUNTRY);

			// record the screen sizes in the trace
			sprintf(traceInfo, "Screen workarea height=%d width=%d", height, width);
			logTraceEntry(traceInfo);

			// log the MQ install type and version info to the trace file
			sprintf(traceInfo, "MQ Install type is %s Version %d Release %d VRMF %s", productType, MQServerVersion, MQServerRelease, VRMF);

			// capture the locale in the trace file
			logTraceEntry(traceInfo);

			pDocument.traceEnabled = TRUE;

			// check for verbose and fonts trace
			pDocument.verboseTrace = FALSE;
			pDocument.fontTrace = FALSE;
			traceTypePtr = getenv(RFHUTIL_VERBOSE_TRACE);
			fontTracePtr = getenv(RFHUTIL_FONTS_TRACE);

			if ((traceTypePtr != NULL) && (traceTypePtr[0] != 'N') && (traceTypePtr[0] != 'n') && (traceTypePtr[0] != '0'))
			{
				verboseTrace = TRUE;
				pDocument.verboseTrace = TRUE;
			}

			if ((fontTracePtr != NULL) && (fontTracePtr[0] != 'N') && (fontTracePtr[0] != 'n') && (fontTracePtr[0] != '0'))
			{
				fontTrace = TRUE;
				pDocument.fontTrace = TRUE;
			}

			// trace the verbose setting
			sprintf(traceInfo, "verboseTrace=%d", verboseTrace);

			// capture the locale in the trace file
			logTraceEntry(traceInfo);
		}
	}
}

void CRfhutilApp::getEnvParms()

{
	const char *	maxDispTimePtr;

	// check for an override for the maximum wait time for the display queue
	// if the RFHUTIL_MAX_DISPLAY_TIME environment variable is set to a valid number > 10
	// then the maximum wait time will be changed from the default of 15 seconds
	// the setting is in seconds
	maxDispTimePtr = getenv(RFHUTIL_MAX_DISPLAY_TIME);

	// is the environment variable set?
	if (maxDispTimePtr != NULL)
	{
		// try to override the default setting
		maxDispTime = atoi(maxDispTimePtr);

		// make sure it is set to at least 10 seconds
		if (maxDispTime < MAX_DISPLAY_TIME_MIN)
		{
			// ensure the wait is at least 10 seconds
			maxDispTime = MAX_DISPLAY_TIME_MIN;
		}
	}
}
