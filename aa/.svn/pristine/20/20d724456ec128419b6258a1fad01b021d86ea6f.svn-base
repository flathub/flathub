#if !defined(AFX_PLAYERPROFILE_H__3E1FD51E_6213_437D_9A2B_96C43A072E75__INCLUDED_)
#define AFX_PLAYERPROFILE_H__3E1FD51E_6213_437D_9A2B_96C43A072E75__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PlayerProfile.h : header file
//
#include "hyperlink.h"

/////////////////////////////////////////////////////////////////////////////
// PlayerProfile dialog

/*
 * class CMinMaxFrame
 *
 * Note: This class assumes that the associated frame has a menu and the
 * following Window Styles:
 *
 * - WS_BORDER
 * - WS_CAPTION
 * - WS_THICKFRAME
 *
 * This condition should always be met since the MFC AppWizard
 * generated code is using WS_OVERLAPPEDWINDOW that includes all 3 styles
 * to create the frame window.
 *
 * Possible enhancements:
 * - Support more than 1 toolbar with a list
 * - Support maximum size
 * - Override PreCreateWindow() function to make sure that the 3 mandatory
 *   window style flags are always present.
 * - Use documented MFC features as the current solution might not work
 *   anymore with future MFC releases.
 */

#include "minmaxlogic.h"

#define DEFAULTMINCLIENTSIZE 350

class CMinMaxFrame : public CFrameWnd
{
public:
	CMinMaxFrame( LONG minX = DEFAULTMINCLIENTSIZE,
		          LONG minY = DEFAULTMINCLIENTSIZE );

/******************************************************************************
 *
 * Name      : setClientMin
 *
 * Purpose   : Recompute the minimum frame size from the newly provided minimum
 *             client area size. It can be called anytime by the user.
 *
 * Parameters:
 *     x       (LONG) Minimum client horizontal size.
 *     y       (LONG) Minumum client vertical size.
 *
 * Return value : None.
 *
 ****************************************************************************/
	void setClientMin(LONG x, LONG y )
	{
		m_MinMaxLogic.setClientMin(x,y);
	}

/******************************************************************************
 *
 * Name      : setToolBar
 *
 * Purpose   : Register the toolbar to monitor for adjusting the minimum frame
 *             size to respect the requested the minimum client area size.
 *
 * Note      : Currently only 1 toolbar is supported but more could be
 *             supported with the help of a toolbar list.
 *
 * Parameters:
 *     pTB     (CToolBar *) Toolbar to register.
 *
 * Return value : None.
 *
 ****************************************************************************/
	void setToolBar( CToolBar *pTB )
	{
		m_pTB = pTB;
		if( pTB )
		{
			m_MinMaxLogic.m_tbPos = TBFLOAT;
		}
		else
		{
			m_MinMaxLogic.m_tbPos = TBNOTCREATED;
		}
	}

/******************************************************************************
 *
 * Name      : setStatusBar
 *
 * Purpose   : Register the status bar to monitor for adjusting the minimum
 *             frame size to respect the requested the minimum client area
 *             size.
 *
 * Parameters:
 *     pST     (CStatusBar *) Status bar to register.
 *
 * Return value : None.
 *
 ****************************************************************************/
	void setStatusBar( CStatusBar *pST )
	{
		// Compute the status bar height
		if( pST )
		{
			m_MinMaxLogic.m_sbHeight = pST->CalcFixedLayout(TRUE,TRUE).cy;
		}
		else
		{
			m_MinMaxLogic.m_sbHeight = 0;
		}
	}

// Overrides
/******************************************************************************
 *
 * Name      : RecalcLayout
 *
 * Purpose   : This function is called by the MFC framework whenever a
 *             toolbar status is changing (is attached or detached to/from
 *             the frame). It is used as a hook to maintain this class
 *             internal state concerning the toolbar position and size.
 *             It should not be called directly.
 *
 * Parameters:
 *     bNotify (BOOL) Not used.
 *
 * Return value : None.
 *
 ****************************************************************************/
	virtual void RecalcLayout(BOOL bNotify = TRUE);
protected:
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg BOOL OnBarCheck(UINT nID);
	DECLARE_MESSAGE_MAP()
private:
	CMinMaxLogic m_MinMaxLogic;
	CToolBar    *m_pTB;

	// TB Functions
	void triggerGetMinMaxInfoMsg(void);
	int getTBSize(int pos);
	int findDockSide(void);
};

class PlayerProfile : public CHyperLinkDlg
{
// Construction
public:
	PlayerProfile();   // standard constructor

// Dialog Data
	//{{AFX_DATA(PlayerProfile)
	enum { IDD = IDD_SETPROFILE };
	CComboBox	m_ircselectserver;
	CEdit	m_gamepathctrl;
	CButton	m_joinstartupctrl;
	CEdit	m_playernamectrl;
	CEdit	m_playeremailctrl;
	CString	m_playeremailstr;
	CString	m_playernamestr;
	BOOL	m_joinstartup;
	CString	m_gamepathstr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PlayerProfile)
	protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PlayerProfile)
	afx_msg void OnChangePlayername();
	afx_msg void OnChangePlayeremail();
	virtual void OnOK();
	afx_msg void OnJoinatstartup();
	afx_msg void OnChangeGamepath();
	afx_msg void OnSelchangeIrcserver();
	afx_msg void OnPrivacy();
	//}}AFX_MSG
private:

CDemoLink	m_PrivacyLink;

	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PLAYERPROFILE_H__3E1FD51E_6213_437D_9A2B_96C43A072E75__INCLUDED_)
