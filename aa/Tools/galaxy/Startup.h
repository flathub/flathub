#if !defined(AFX_STARTUP_H__2F88380B_C0B2_4C5F_8B5A_2FAE8E653D2F__INCLUDED_)
#define AFX_STARTUP_H__2F88380B_C0B2_4C5F_8B5A_2FAE8E653D2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Startup.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Startup dialog

class Startup : public CDialog
{
// Construction
public:
	Startup(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Startup)
	enum { IDD = IDD_STARTUP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Startup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Startup)
	afx_msg void OnRefreshProgress2(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STARTUP_H__2F88380B_C0B2_4C5F_8B5A_2FAE8E653D2F__INCLUDED_)
