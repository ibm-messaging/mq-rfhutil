#if !defined(AFX_MYPROPERTYSHEET_H__2DE663A9_33D6_4502_A9E3_177A2733256F__INCLUDED_)
#define AFX_MYPROPERTYSHEET_H_H__2DE663A9_33D6_4502_A9E3_177A2733256F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MyPropertySheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// MyPropertySheet window

class MyPropertySheet : public CPropertySheet
{
// Construction
public:
	virtual int DoModal();
	void BuildPropPageArray();
	MyPropertySheet();

protected:
    //virtual void BuildPropSheetArray();

protected:

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(MyPropertySheet)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~MyPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(MyPropertySheet)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEditUpdate();
	afx_msg UINT OnGetDlgCode();
	int CALLBACK PropSheetProc(HWND hWndDlg, UINT uMsg, LPARAM lParam);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	char	*m_pszFontFaceName;
	int		m_wFontSize;
	int		dpi;
	BOOL	m_bSystemFont;

//	void Attach(LPDLGTEMPLATE pTemplate);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MYPROPERTYSHEET_H__2DE663A9_33D6_4502_A9E3_177A2733256F__INCLUDED_)
