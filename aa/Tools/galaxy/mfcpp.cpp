/*
 * Module ID: mfcpp.cpp
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
#include "stdafx.h"
#include "mfcpp.h"
#include "hyperlink.h"

/////////////////////////////////////////////////////////////////////////////
// CSubclassToolTipCtrl operations

/*
 * Function CSubclassToolTipCtrl::AddWindowTool
 */
BOOL CSubclassToolTipCtrl::AddWindowTool( HWND hWin, LPTSTR pszText )
{
	TOOLINFO ti;
	FillInToolInfo(ti,hWin,0);
	ti.uFlags  |= TTF_SUBCLASS;
	ti.hinst    = AfxGetInstanceHandle();
    ti.lpszText = pszText;

	return (BOOL)SendMessage(TTM_ADDTOOL,0,(LPARAM)&ti);
}

/*
 * Function CSubclassToolTipCtrl::AddRectTool
 */
BOOL CSubclassToolTipCtrl::AddRectTool( HWND hWin, LPTSTR pszText,
									    LPCRECT lpRect, UINT nIDTool )
{
	TOOLINFO ti;
	FillInToolInfo(ti,hWin,nIDTool);
	ti.uFlags  |= TTF_SUBCLASS;
	ti.hinst    = AfxGetInstanceHandle();
    ti.lpszText = pszText;
	::CopyRect(&ti.rect,lpRect);

	return (BOOL)SendMessage(TTM_ADDTOOL,0,(LPARAM)&ti);
}

// Implementation

/*
 * Function CSubclassToolTipCtrl::FillInToolInfo
 */
void CSubclassToolTipCtrl::FillInToolInfo(TOOLINFO& ti, HWND hWnd, UINT nIDTool) const
{
	::ZeroMemory(&ti, sizeof(TOOLINFO));
	ti.cbSize   = sizeof(TOOLINFO);
	if (nIDTool == 0)
	{
		ti.hwnd = ::GetParent(hWnd);
		ti.uFlags = TTF_IDISHWND;
		ti.uId = (UINT)hWnd;
	}
	else
	{
		ti.hwnd = hWnd;
		ti.uFlags = 0;
		ti.uId = nIDTool;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHyperLinkDlg operations

/*
 * Function CHyperLinkDlg::setURL
 */
void CHyperLinkDlg::setURL(CHyperLink &ctr, int id)
{
	TCHAR buffer[URLMAXLENGTH];
	int nLen = ::LoadString(AfxGetResourceHandle(), id, buffer, URLMAXLENGTH);
	if( !nLen )
	{
		lstrcpy( buffer, __TEXT(""));
	}
    ctr.ConvertStaticToHyperlink(GetSafeHwnd(),id,buffer);
}

/////////////////////////////////////////////////////////////////////////////
// CMinMaxFrame operations

/*
 * It might be more appropriate to install the OnBarCheck handlers in derived
 * classes as this class cannot know in advance which bars will be used.
 *
 * (To investigate)
 */
BEGIN_MESSAGE_MAP(CMinMaxFrame, CFrameWnd)
	ON_WM_GETMINMAXINFO()
	ON_COMMAND_EX(ID_VIEW_TOOLBAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnBarCheck)
END_MESSAGE_MAP()

/*
 * Constructor/Destructor
 */
CMinMaxFrame::CMinMaxFrame( LONG minX, LONG minY )
: m_MinMaxLogic(minX,minY), m_pTB(NULL)
{
}

/*-----------------------------------------------------------------------------
 * Public functions
 */

/*
 * CMinMaxFrame::RecalcLayout function
 *
 * Purpose   : This function is called by the MFC framework whenever a
 *             toolbar status is changing (is attached or detached to/from
 *             the frame). It is used as a hook to maintain this class
 *             internal state concerning the toolbar position and size.
 *             It should not be called directly.
 */
void CMinMaxFrame::RecalcLayout(BOOL bNotify)
{
	CFrameWnd::RecalcLayout(bNotify);

	// TODO: Add your specialized code here and/or call the base class
	if( m_MinMaxLogic.m_tbPos != TBNOTCREATED )
	{
		if( !m_pTB->IsFloating() )
		{
			int newPos = findDockSide();
			if( m_MinMaxLogic.m_tbPos != newPos )
			{
				m_MinMaxLogic.m_tbPos  = newPos;
				m_MinMaxLogic.m_tbSize = getTBSize(m_MinMaxLogic.m_tbPos);

				triggerGetMinMaxInfoMsg();
			}
		}
		else
		{
			m_MinMaxLogic.m_tbPos  = TBFLOAT;
			m_MinMaxLogic.m_tbSize = 0;
		}
	}
}

/*=============================================================================
 * Protected functions
 */

/*
 * CMinMaxFrame::OnGetMinMaxInfo function
 *
 * Purpose   : WM_GETMINMAXINFO message handler. There is nothing
 *             specific to MFC so it directly pass the control to the
 *             CMinMaxLogic WM_GETMINMAXINFO message handler.
 */
void CMinMaxFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	// TODO: Add your message handler code here and/or call default
    m_MinMaxLogic.OnGetMinMaxInfo(lpMMI);
}

/*
 * CMinMaxFrame::OnBarCheck function
 *
 * Purpose   : MFC defined message handler. It is called whenever a toolbar
 *             or a status bar visibility state change. It is used to trigger
 *             a WM_GETMINMAXINFO since the minimum frame size to maintain a
 *             minimum client area size has changed.
 */
BOOL CMinMaxFrame::OnBarCheck(UINT nID)
{
	BOOL res = CFrameWnd::OnBarCheck(nID);

	// TODO: Add your command handler code here
	if( nID == ID_VIEW_STATUS_BAR )
	{
		m_MinMaxLogic.m_sbVisible = !m_MinMaxLogic.m_sbVisible;
		if( m_MinMaxLogic.m_sbVisible )
		{
			triggerGetMinMaxInfoMsg();
		}
	}
	else if( nID == ID_VIEW_TOOLBAR )
	{
		m_MinMaxLogic.m_tbVisible = !m_MinMaxLogic.m_tbVisible;
		if( m_MinMaxLogic.m_tbVisible )
		{
			triggerGetMinMaxInfoMsg();
		}
	}

	return res;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Private functions
 */

/*
 * CMinMaxFrame::triggerGetMinMaxInfoMsg function
 */
void CMinMaxFrame::triggerGetMinMaxInfoMsg()
{
	/*
	 * Trigger a WM_MINMAXINFO message by calling the function MoveWindow()
	 * with the current frame size. The purpose of generating a call to the
	 * WM_GETMINMAXINFO handler is to verify that the new client area size
	 * still respect the minimum size.
     */
    RECT wRect;
	GetWindowRect(&wRect);
	MoveWindow(&wRect);
}

/******************************************************************************
 * TB Functions
 */

/*
 * CMinMaxFrame::findDockSide function
 *
 * Note: This function is using AFXPRIV. It might not be working anymore
 *       with a future MFC version.
 */
#include "afxpriv.h"

int CMinMaxFrame::findDockSide()
{
	// dwDockBarMap
	static const DWORD dwDockBarMap[4] =
	{
		AFX_IDW_DOCKBAR_TOP,
		AFX_IDW_DOCKBAR_BOTTOM,
		AFX_IDW_DOCKBAR_LEFT,
		AFX_IDW_DOCKBAR_RIGHT
	};

	int res = TBFLOAT;

	for( int i = 0; i < 4; i++ )
	{
		CDockBar *pDock = (CDockBar *)GetControlBar(dwDockBarMap[i]);
		if( pDock != NULL )
		{
			if( pDock->FindBar(m_pTB) != -1 )
			{
				res = i;
				break;
			}
		}
	}
	return res;
}

/*
 * CMinMaxFrame::getTBSize function
 *
 * Purpose   : Returns the horizontal or the vertical toolbar size based on the
 *             toolbar position.
 */
int CMinMaxFrame::getTBSize(int pos)
{
	int res;

	CSize cbSize = m_pTB->CalcFixedLayout(FALSE,
		                               (pos==TBTOP||pos==TBBOTTOM)?TRUE:FALSE);
    if( pos == TBTOP || pos == TBBOTTOM )
	{
		res = cbSize.cy;
	}
	else
	{
		res = cbSize.cx;
	}

	return res;
}
