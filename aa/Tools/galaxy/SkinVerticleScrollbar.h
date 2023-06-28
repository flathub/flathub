#if !defined(AFX_SKINVERTICLESCROLLBAR_H__B382B86C_A9B6_4F61_A03D_53C27C76DF9E__INCLUDED_)
#define AFX_SKINVERTICLESCROLLBAR_H__B382B86C_A9B6_4F61_A03D_53C27C76DF9E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SkinVerticleScrollbar.h : header file
//
#include "memdc.h"
/////////////////////////////////////////////////////////////////////////////
// CSkinVerticleScrollbar window

class CSkinVerticleScrollbar : public CStatic
{
// Construction
public:
	CSkinVerticleScrollbar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinVerticleScrollbar)
	//}}AFX_VIRTUAL

// Implementation
public:
	CListCtrl* pList;
	void LimitThumbPosition();
	void Draw();
	void UpdateThumbPosition();
	bool bMouseDownArrowUp, bMouseDownArrowDown;
	bool bDragging;
	bool bMouseDown;

	int nThumbTop;
	double dbThumbInterval;

	void ScrollDown();
	void ScrollUp();
	void PageUp();
	void PageDown();
	virtual ~CSkinVerticleScrollbar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSkinVerticleScrollbar)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINVERTICLESCROLLBAR_H__B382B86C_A9B6_4F61_A03D_53C27C76DF9E__INCLUDED_)
