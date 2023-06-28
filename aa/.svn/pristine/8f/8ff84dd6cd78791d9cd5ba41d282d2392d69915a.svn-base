#if !defined(AFX_HELP_H__D858810D_873F_4CFD_AA6C_3F10E71C5CE1__INCLUDED_)
#define AFX_HELP_H__D858810D_873F_4CFD_AA6C_3F10E71C5CE1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Help.h : header file
//
#include "hyperlink.h"
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Help dialog

class Help : public CHyperLinkDlg
{
// Construction
public:
	Help();   // standard constructor

// Dialog Data
	//{{AFX_DATA(Help)
	enum { IDD = IDD_HELPBOX };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Help)
	protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Help)
	afx_msg void OnLinks();
	//}}AFX_MSG


private:

	CDemoLink	m_Link1;
	CDemoLink	m_Link2;
	CDemoLink	m_Link3;
	CDemoLink	m_Link4;
	CDemoLink	m_Link5;
	CDemoLink	m_Link6;
	CDemoLink	m_Link7;

	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HELP_H__D858810D_873F_4CFD_AA6C_3F10E71C5CE1__INCLUDED_)
