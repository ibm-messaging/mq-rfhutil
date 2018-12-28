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

#if !defined(AFX_MSGDATA_H__EC6C6267_015D_4939_8594_CE8A13BB071F__INCLUDED_)
#define AFX_MSGDATA_H__EC6C6267_015D_4939_8594_CE8A13BB071F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MSGDATA.h : header file
//

#include "DataArea.h"

/////////////////////////////////////////////////////////////////////////////
// MSGDATA dialog

class MSGDATA : public CPropertyPage
{
	DECLARE_DYNCREATE(MSGDATA)

// Construction
public:
	void setFix();
	void setJson();
	void setThai();
	void togglePDEncoding();
	void toggleIntEncoding();
	void setJapanese();
	void setKorean();
	void setSimpChinese();
	void setTradChinese();
	void setAscii();
	void setCobol();
	void setParsed();
	void setXML();
	void setBoth();
	void setHex();
	void setEBCDIC();
	void setCharacter();
	void ReadCopybook();
	void EndBr();
	void setDisplayFont();
	void moveDisplay(int line, int ofs, int count);
	void getSelection(int& firstChar, int& lastChar);
	DataArea* pDoc;
	void updateMsgdata();
	MSGDATA();
	~MSGDATA();

// Dialog Data
	//{{AFX_DATA(MSGDATA)
	enum { IDD = IDD_DATA };
	CString	m_msg_data;
	int		m_data_format;
	int		m_character_format;
	int		m_numeric_format;
	int		m_pd_numeric_format;
	BOOL	m_crlf;
	BOOL	m_edi;
	BOOL	m_BOM;
	BOOL	m_data_indent;
	BOOL	m_checkData;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(MSGDATA)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(MSGDATA)
	afx_msg void OnAscii();
	afx_msg void OnCobol();
	afx_msg void OnHex();
	afx_msg void OnXml();
	afx_msg void OnEbcdic();
	afx_msg void OnBoth();
	afx_msg void OnParsed();
	afx_msg void OnCopybook();
	afx_msg void OnCharacter();
	afx_msg void OnHost();
	afx_msg void OnNumericPc();
	afx_msg void OnCrlf();
	afx_msg void OnDataBrnext();
	afx_msg void OnEdi();
	afx_msg void OnBOM();
	afx_msg void OnPdDataIntel();
	afx_msg void OnPdDataHost();
	virtual BOOL OnInitDialog();
	afx_msg void OnChinese();
	afx_msg void OnKorean();
	afx_msg void OnTradChinese();
	afx_msg void OnJapan();
	afx_msg void OnIndent();
	afx_msg void OnDataValidate();
	afx_msg void OnDataBrprev();
	afx_msg void OnThai();
	afx_msg void OnJson();
	afx_msg void OnFix();
	afx_msg void OnRussian();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void setRussianFont();
	int bothTabStops[3];
	int charTabStops[2];
	int hexTabStops[2];
	void setFont(CFont * font);
	void traceFont(CEdit * cedit, CFont *font);
	void setThaiFont();
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	void updateFixedFont(LONG height);
	void setFixedFont();
	void setJapanFont();
	void setBig5Font();
	void setKoreaFont();
	void setChinaFont();
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
	int m_called_copybook;
	int m_save_data_type;
	int	curDisplayResolution;
public:
	afx_msg void OnBnClickedDataBom();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSGDATA_H__EC6C6267_015D_4939_8594_CE8A13BB071F__INCLUDED_)
