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

// rfhutilDoc.h : interface of the CRfhutilDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RFHUTILDOC_H__2646814C_B0BD_4428_AF0A_170DDAA64A35__INCLUDED_)
#define AFX_RFHUTILDOC_H__2646814C_B0BD_4428_AF0A_170DDAA64A35__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CRfhutilDoc : public CDocument
{
protected: // create from serialization only
	CRfhutilDoc();
	DECLARE_DYNCREATE(CRfhutilDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRfhutilDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRfhutilDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CRfhutilDoc)
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditPaste();
	afx_msg void OnFileMruFile1();
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL checkIDforCopyOnly(int id);
	BOOL checkIDforPaste(int id);
	void getRecentFile(int fileNum, char * fileName, const int size);
	void openFile(LPCTSTR lpszPathName);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RFHUTILDOC_H__2646814C_B0BD_4428_AF0A_170DDAA64A35__INCLUDED_)
