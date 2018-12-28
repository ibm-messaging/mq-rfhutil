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
#if !defined(AFX_PROPS_H__C32D8BBC_DED0_4942_9075_429AF4D1C909__INCLUDED_)
#define AFX_PROPS_H__C32D8BBC_DED0_4942_9075_429AF4D1C909__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Props.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProps dialog

#include "DataArea.h"
#include "MyEdit.h"
#include "XMLParse.h"

class CProps : public CPropertyPage
{
	DECLARE_DYNCREATE(CProps)

// Construction
public:
	int getNextProp(int elem);
	int getPropType(int elem);
	const char * getPropValue(int elem);
	const char * getPropName(int elem);
	int getFirstProperty();
	void checkForRetained(BOOL * isRetained);
	void getPropertyData(CString& data);
	void clearUserProperties();
	int GetPropertyCount();
	int propTypeToInt(const char * ptr);
	void appendProperty(const char * name, const char * value, const int valueLen, int type);
	void UpdatePageData();
	CProps();
	~CProps();

	DataArea* pDoc;

// Dialog Data
	//{{AFX_DATA(CProps)
	enum { IDD = IDD_V7PROPS };
	CString	m_properties;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CProps)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CProps)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangePropsUserProperties();
	afx_msg void OnPropsMoveRfhUsr();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CXMLParse * xml;
	int checkLineAttrs(const char *start, const char *end);
	int checkLine(const char * start, const char * end);
	int checkPropData(const char * start, const char * end);
	int propertyCount;
	LONG OnSetPageFocus(UINT wParam, LONG lParam);
	MyEdit m_PropertiesEditBox;
	BOOL GetToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPS_H__C32D8BBC_DED0_4942_9075_429AF4D1C909__INCLUDED_)
