// MyPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "rfhutil.h"
#include "MyPropertySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MyPropertySheet

MyPropertySheet::MyPropertySheet()

{
	CDC		*dc = NULL;
	HDC		hdc = NULL;

	m_wFontSize = 14;
	dpi = 0;
	m_pszFontFaceName = "Courier New";

	// try to get a device context
	dc = GetDC();

	if (dc != NULL)
	{
		hdc = dc->GetSafeHdc();
	}

	// check if it worked
	if (hdc != NULL)
	{
		// get the horizontal resolution as dpi
		dpi = GetDeviceCaps(hdc, LOGPIXELSX);

		// check for a higher resolution screen
		if (dpi > 100)
		{
			// use a larger font size
			m_wFontSize = (m_wFontSize * dpi) / 100;
		}
	}
}

MyPropertySheet::~MyPropertySheet()
{
}


BEGIN_MESSAGE_MAP(MyPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(MyPropertySheet)
	ON_WM_KEYDOWN()
    ON_CONTROL_REFLECT(CBN_EDITUPDATE, OnEditUpdate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MyPropertySheet callback routine

int CALLBACK MyPropertySheet::PropSheetProc(HWND hWndDlg, UINT uMsg, LPARAM lParam)
{
   switch(uMsg)
   {
   case PSCB_PRECREATE: // property sheet is being created
      {
           LPDLGTEMPLATE pResource = (LPDLGTEMPLATE)lParam;
           CDialogTemplate dlgTemplate(pResource);
           dlgTemplate.SetFont(m_pszFontFaceName, m_wFontSize);
           memmove((void*)lParam, dlgTemplate.m_hTemplate, dlgTemplate.m_dwTemplateSize);
      }
      break;
   }

   return 0;
}

/////////////////////////////////////////////////////////////////////////////
// MyPropertySheet message handlers

int MyPropertySheet::DoModal() 
{
    m_psh.dwFlags |= PSH_USECALLBACK;
    m_psh.pfnCallback = PropSheetProc;

    return CPropertySheet::DoModal();
}

void MyPropertySheet::BuildPropPageArray()
{
    CPropertySheet::BuildPropPageArray();

	PROPSHEETPAGE* ppsp = ( PROPSHEETPAGE* )m_psh.ppsp;
    for ( int nPage = 0; nPage < m_pages.GetSize(); nPage++ )
    {
        LPDLGTEMPLATE pTemplate = ( LPDLGTEMPLATE )ppsp[ nPage ].pResource;

		DialogTemplate dlgTemplate;
		DLGTEMPLATE dlgTemplate;
		dlgTemplate.Attach( pTemplate );
        dlgTemplate.SetFont(m_pszFontFaceName, m_wFontSize);
        dlgTemplate.Detach();
    }
}

/*int CALLBACK MyPropertySheet::PropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam )
{
    switch ( uMsg )
    {
        case PSCB_PRECREATE:
        {
            LPDLGTEMPLATE pTemplate = ( LPDLGTEMPLATE )lParam;

            CMyDialogTemplate dlgTemplate;
            dlgTemplate.Attach( pTemplate );
            dlgTemplate.SetFont( lpszFace, wSize );
            dlgTemplate.Detach();
            break;
        }
        default:
            break;
    }

    return 0;
}*/

/*void MyDialogTemplate::Attach( LPDLGTEMPLATE pTemplate )
{
    m_hTemplate      = ::GlobalHandle( pTemplate );
    m_dwTemplateSize = GetTemplateSize( pTemplate );
    m_bSystemFont    = false;
}*/
