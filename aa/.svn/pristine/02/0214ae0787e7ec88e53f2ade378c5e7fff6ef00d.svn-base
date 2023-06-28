// GalaxyDlg.h : header file
//

#if !defined(AFX_GALAXYDLG_H__D1CCCB45_C467_497E_A7A7_FF237A06A6FA__INCLUDED_)
#define AFX_GALAXYDLG_H__D1CCCB45_C467_497E_A7A7_FF237A06A6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SkinListCtrl.h"
/////////////////////////////////////////////////////////////////////////////
// CGalaxyDlg dialog

class CGalaxyDlg : public CDialog
{

// Construction
public:
	CGalaxyDlg(CWnd* pParent = NULL);	// standard constructor
// Dialog Data
	//{{AFX_DATA(CGalaxyDlg)
	enum { IDD = IDD_GALAXY_DIALOG };
	CEdit	m_news;
	CBitmapButton	m_showusers;
	CBitmapButton	m_delbuddy;
	CBitmapButton	m_addbuddy;
	CSkinListCtrl	m_buddylist;
	CEdit	m_playernum;
	CEdit	m_status2;
	CBitmapButton	m_disconnect;
	CBitmapButton	m_joinchannel;
	CBitmapButton	m_sendtext;
	CSkinListCtrl	m_chatthread;
	CEdit	m_chatstring;
	CEdit	m_status;
	CBitmapButton	m_launchbmp;
	CBitmapButton	m_joinbmp;
	CBitmapButton	m_refreshbmp;
	CProgressCtrl	m_refreshprogress;
	CSkinListCtrl	m_serverrules;
	CSkinListCtrl	m_playerinfo;
	CSkinListCtrl	m_serverinfo;
	CImageList  m_ImageList;
	CString	m_sendstring;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGalaxyDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	static CString TextRetriever(int nIndex);
	// Generated message map functions
	//{{AFX_MSG(CGalaxyDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchangeList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRefresh();
	afx_msg void Do_Refresh();
	afx_msg void LookUpStats();
	afx_msg void LookUpPlayerStats(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void Check_Buddies();
	afx_msg void Configure();
	afx_msg void OnChangeEdit2();
	afx_msg void OnChangeEdit3();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void analizeLine(char Line[1000]);
	afx_msg void OnButton4();
	afx_msg void OnButton5();
	afx_msg void OnJoin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnJoin2();
	afx_msg void OnLaunch();
	afx_msg void OnButton6();
	afx_msg void OnAddbuddy();
	afx_msg void OnDelbuddy();
	afx_msg void OnSelchangeBuddylist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPlayersort();
	afx_msg void OnPingsort();
	afx_msg void OnQuickAddbuddy();
	afx_msg void OnShowusers();
	afx_msg void GetNews();
	afx_msg void About();
	afx_msg void HelpLinks();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GALAXYDLG_H__D1CCCB45_C467_497E_A7A7_FF237A06A6FA__INCLUDED_)
