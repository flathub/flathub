/*
 * Module ID: mfcpp.h
 * Title    : MFC++: Extend MFC classes.
 *
 * Author   : Olivier Langlois <olivier@olivierlanglois.net>
 * Date     : December 12, 2005
 *
 * For details on CStrechyStatusBar class, go to:
 * http://www.olivierlanglois.net/clover.html
 *
 * For details on CSubclassToolTipCtrl and CHyperLinkDlg classes, go to:
 * http://www.olivierlanglois.net/hyperlinkdemo.htm
 *
 * For details on CMinMaxFrame class, go to:
 * http://www.olivierlanglois.net/minmaxdemo.html
 *
 * Revision :
 *
 * 001        03-Feb-2006 - Olivier Langlois
 *            - Added CMinMaxFrame class
 */

#ifndef   _MFCPP_H_
#define   _MFCPP_H_

#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls

/*
 * Defines
 */
#define URLMAXLENGTH 256

/*
 * Forward declaration
 */
class CHyperLink;

/*
 * class CStrechyStatusBar
 */
class CStrechyStatusBar : public CStatusBar
{
protected:
/******************************************************************************
 *
 * Name      : AddWindowTool
 *
 * Purpose   : This function can be called from your derived class OnCreate()
 *             function once the status bar indicators have been installed.
 *             For an example, see cloverstatusbar.cpp in
 *             http://www.olivierlanglois.net/clover.html
 *
 * Parameters: None
 *
 * Return value : None
 *
 ****************************************************************************/
	void MakeStrechy(void)
	{
		UINT nID, nStyle;
		int  cxWidth;
		/*
		 * Set the first strechy indicator width to its minimum to
		 * make sure that the right side indicators do not disapear when
		 * the status bar width is reduced.
		 */
		GetPaneInfo(0, nID, nStyle, cxWidth);
		SetPaneInfo(0, nID, nStyle, 1);
	}
};

/*
 * class CSubclassToolTipCtrl
 */
class CSubclassToolTipCtrl : public CToolTipCtrl
{
// Operations
public:
/******************************************************************************
 *
 * Name      : AddWindowTool
 *
 * Purpose   : Add a window tool by using the Tooltip subclass feature
 *
 * Parameters:
 *     hWin    (HWND)    Tool window
 *     pszText (LPTSTR)  Tip text (can also be a string resource ID).
 *
 * Return value : Returns TRUE if successful, or FALSE otherwise.
 *
 ****************************************************************************/
	BOOL AddWindowTool( HWND hWin, LPTSTR pszText );

/******************************************************************************
 *
 * Name      : AddRectTool
 *
 * Purpose   : Add a rect tool by using the Tooltip subclass feature
 *
 * Parameters:
 *     hWin    (HWND)    Tool window parent
 *     pszText (LPTSTR)  Tip text (can also be a string resource ID).
 *     lpRect  (LPCRECT) Tool rect
 *     nIDTool (UINT)    User defined Tool ID
 *
 * Return value : Returns TRUE if successful, or FALSE otherwise.
 *
 ****************************************************************************/
	BOOL AddRectTool( HWND hWin, LPTSTR pszText, LPCRECT lpRect, UINT nIDTool );

// Implementation
	void FillInToolInfo(TOOLINFO& ti, HWND hWnd, UINT nIDTool) const;
};

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

#endif /* _MFCPP_H_ */
