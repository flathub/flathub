#if !defined(AFX_SKINLISTCTRL_H__D65A645A_8C29_4A93_B453_ED9C92807426__INCLUDED_)
#define AFX_SKINLISTCTRL_H__D65A645A_8C29_4A93_B453_ED9C92807426__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SkinListCtrl.h : header file
//
#include "SkinHeaderCtrl.h"
#include "SkinHorizontalScrollbar.h"
#include "SkinVerticleScrollbar.h"

/////////////////////////////////////////////////////////////////////////////
// CSkinListCtrl window

class CSkinListCtrl : public CListCtrl
{
// Construction
public:
	CSkinListCtrl();
	CSkinHeaderCtrl m_SkinHeaderCtrl;
	COLORREF g_MyClrFgHi;
	COLORREF g_MyClrBgHi;
	void EnableHighlighting(HWND hWnd, int row, bool bHighlight);
	bool IsRowSelected(HWND hWnd, int row);
	bool IsRowHighlighted(HWND hWnd, int row);

	CSkinVerticleScrollbar m_SkinVerticleScrollbar;
	CSkinHorizontalScrollbar m_SkinHorizontalScrollbar;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinListCtrl)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	void PositionScrollBars();
	void Init();
	virtual ~CSkinListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSkinListCtrl)
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg void OnCustomDrawList ( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINLISTCTRL_H__D65A645A_8C29_4A93_B453_ED9C92807426__INCLUDED_)
