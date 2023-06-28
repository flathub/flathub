// SkinListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "SkinListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSkinListCtrl

CSkinListCtrl::CSkinListCtrl()
{
	g_MyClrBgHi = RGB(115,123,165);
	g_MyClrFgHi = RGB(229,229,229);
}

CSkinListCtrl::~CSkinListCtrl()
{
}


BEGIN_MESSAGE_MAP(CSkinListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CSkinListCtrl)
	ON_WM_NCCALCSIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_NOTIFY_REFLECT ( NM_CUSTOMDRAW, OnCustomDrawList )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkinListCtrl message handlers

void CSkinListCtrl::PreSubclassWindow() 
{
	//use our custom CHeaderCtrl as long as there
	//is a headerctrl object to subclass
	if(GetHeaderCtrl())
		m_SkinHeaderCtrl.SubclassWindow(GetHeaderCtrl()->m_hWnd);

	CListCtrl::PreSubclassWindow();
}


void CSkinListCtrl::OnCustomDrawList ( NMHDR* pNMHDR, LRESULT* pResult )
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	static bool bHighlighted = false;
	
    *pResult = CDRF_DODEFAULT;

    if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
        *pResult = CDRF_NOTIFYITEMDRAW;
	}
    else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
        int iRow = (int)pLVCD->nmcd.dwItemSpec;
		
		bHighlighted = IsRowHighlighted(m_hWnd, iRow);
		if (bHighlighted)
		{
			pLVCD->clrText   = g_MyClrFgHi; // Use my foreground hilite color
			pLVCD->clrTextBk = g_MyClrBgHi; // Use my background hilite color
			
			EnableHighlighting(m_hWnd, iRow, false);
		}
		
		*pResult = CDRF_DODEFAULT | CDRF_NOTIFYPOSTPAINT;
		
	}
	else if(CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage)
	{
	if (bHighlighted)
      {
        int  iRow = (int)pLVCD->nmcd.dwItemSpec;

        EnableHighlighting(m_hWnd, iRow, true);
      }

      *pResult = CDRF_DODEFAULT;

	}
}

void CSkinListCtrl::EnableHighlighting(HWND hWnd, int row, bool bHighlight)
{
  ListView_SetItemState(hWnd, row, bHighlight? 0xff: 0, LVIS_SELECTED);
}

bool CSkinListCtrl::IsRowSelected(HWND hWnd, int row)
{
  return ListView_GetItemState(hWnd, row, LVIS_SELECTED) != 0;
}

bool CSkinListCtrl::IsRowHighlighted(HWND hWnd, int row)
{
  return IsRowSelected(hWnd, row) /*&& (::GetFocus() == hWnd)*/;
}

void CSkinListCtrl::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp) 
{
	UpdateWindow();
	CListCtrl::OnNcCalcSize(bCalcValidRects, lpncsp);
}

BOOL CSkinListCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	m_SkinVerticleScrollbar.UpdateThumbPosition();
	m_SkinHorizontalScrollbar.UpdateThumbPosition();

	return CListCtrl::OnMouseWheel(nFlags, zDelta, pt);
}


void CSkinListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	m_SkinVerticleScrollbar.UpdateThumbPosition();
	m_SkinHorizontalScrollbar.UpdateThumbPosition();

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSkinListCtrl::Init()
{
	//another way to hide scrollbars
	InitializeFlatSB(m_hWnd);
	FlatSB_EnableScrollBar(m_hWnd, SB_BOTH, ESB_DISABLE_BOTH);

	CWnd* pParent = GetParent();

	//Create scrollbars at runtime
	m_SkinVerticleScrollbar.Create(NULL, WS_CHILD|SS_LEFT|SS_NOTIFY|WS_VISIBLE|WS_GROUP,CRect(0,0,0,0), pParent);
	m_SkinHorizontalScrollbar.Create(NULL, WS_CHILD|SS_LEFT|SS_NOTIFY|WS_VISIBLE|WS_GROUP,CRect(0,0,0,0), pParent);
	m_SkinVerticleScrollbar.pList = this;
	m_SkinHorizontalScrollbar.pList = this;

	//call this to position the scrollbars properly
	PositionScrollBars();
}

void CSkinListCtrl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	m_SkinVerticleScrollbar.UpdateThumbPosition();
	m_SkinHorizontalScrollbar.UpdateThumbPosition();

	CListCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
}

BOOL CSkinListCtrl::OnEraseBkgnd(CDC* pDC) 
{
	m_SkinVerticleScrollbar.UpdateThumbPosition();
	m_SkinHorizontalScrollbar.UpdateThumbPosition();
	return FALSE;
	//return CListCtrl::OnEraseBkgnd(pDC);
}


void CSkinListCtrl::OnPaint() 
{
	CPaintDC dc(this);
	CRect rect;
	GetClientRect(&rect);
	CMemDC memDC(&dc, rect);
	
	//funky code to allow use to double buffer
	//the onpaint calls for flicker free drawing
	//of the list items

	CRect headerRect;
	GetDlgItem(0)->GetWindowRect(&headerRect);
	ScreenToClient(&headerRect);
	dc.ExcludeClipRect(&headerRect);
	   
	   
	CRect clip;
	memDC.GetClipBox(&clip);
	memDC.FillSolidRect(clip, RGB(76,85,118));
	   
	SetTextBkColor(RGB(76,85,118));
	   
	m_SkinVerticleScrollbar.UpdateThumbPosition();
	m_SkinHorizontalScrollbar.UpdateThumbPosition();
	   
	   
	DefWindowProc(WM_PAINT, (WPARAM)memDC->m_hDC, (LPARAM)0);
}

void CSkinListCtrl::PositionScrollBars()
{
	//Thanks goes to mindows for this function
	//he posted on the message forums. He modified
	//it a bit based on the original init function,
	//and now I have modified his version a tiny bit ;)
	//The pParent->ScreenToClient that you did made it
	//possible for me to make the scrollbars position
	//properly based on any dialog size/borders/titlebar etc... :D

	CWnd* pParent = GetParent();
	
	CRect windowRect;
	GetWindowRect(&windowRect);

	
	int nTitleBarHeight = 0;
	
	if(pParent->GetStyle() & WS_CAPTION)
		nTitleBarHeight = GetSystemMetrics(SM_CYSIZE);
	
	
	int nDialogFrameHeight = 0;
	int nDialogFrameWidth = 0;
	if((pParent->GetStyle() & WS_BORDER))
	{
		nDialogFrameHeight = GetSystemMetrics(SM_CYDLGFRAME);
		nDialogFrameWidth = GetSystemMetrics(SM_CYDLGFRAME);
	}
	
	if(pParent->GetStyle() & WS_THICKFRAME)
	{
		nDialogFrameHeight+=1;
		nDialogFrameWidth+=1;
	}
	
	pParent->ScreenToClient(&windowRect);

	windowRect.top+=nTitleBarHeight+nDialogFrameHeight;
	windowRect.bottom+=nTitleBarHeight+nDialogFrameHeight;
	windowRect.left +=nDialogFrameWidth;
	windowRect.right+=nDialogFrameWidth;

	CRect vBar(windowRect.right-nDialogFrameWidth,windowRect.top-nTitleBarHeight-nDialogFrameHeight,windowRect.right+12-nDialogFrameWidth,windowRect.bottom+12-nTitleBarHeight-nDialogFrameHeight);
	CRect hBar(windowRect.left-nDialogFrameWidth,windowRect.bottom-nTitleBarHeight-nDialogFrameHeight,windowRect.right+1-nDialogFrameWidth,windowRect.bottom+12-nTitleBarHeight-nDialogFrameHeight);
	
	m_SkinVerticleScrollbar.SetWindowPos(NULL,vBar.left,vBar.top,vBar.Width(),vBar.Height(),SWP_NOZORDER);
	m_SkinHorizontalScrollbar.SetWindowPos(NULL,hBar.left,hBar.top,hBar.Width(),hBar.Height(),SWP_NOZORDER);
	
	m_SkinHorizontalScrollbar.UpdateThumbPosition();
	m_SkinVerticleScrollbar.UpdateThumbPosition();
}
