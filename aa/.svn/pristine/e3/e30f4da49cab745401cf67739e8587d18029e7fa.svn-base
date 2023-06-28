/*
 * Module ID: minmaxlogic.cpp
 * Title    : CMinMaxLogic class definition.
 *
 * For details on CMinMaxLogic class, go to:
 * http://www.olivierlanglois.net/minmaxdemo.html
 *
 * Author   : Olivier Langlois <olivier@olivierlanglois.net>
 * Date     : February 03, 2006
 */
#include "stdafx.h"
#include "minmaxlogic.h"
#include <windows.h>

/******************************************************************************
 * Functions definition for CMinMaxLogic
 */

/*
 * Constructor/Destructor
 */
CMinMaxLogic::CMinMaxLogic(LONG x, LONG y)
{
	setClientMin(x,y);
	m_sbVisible   = TRUE;
	m_tbVisible   = TRUE;
	m_sbHeight    = 0;
	m_tbPos       = TBNOTCREATED;
	m_tbSize      = 0;
}

CMinMaxLogic::~CMinMaxLogic(void)
{
}

/*-----------------------------------------------------------------------------
 * Public functions
 */

/*
 * CMinMaxLogic::setClientMin function
 *
 * Uses GetSystemMetrics() to obtain
 * - the sizing border size   (SM_CXFRAME,SM_CYFRAME)
 * - the 3-D border dimension (SM_CXEDGE ,SM_CYEDGE)
 * - the caption size         (SM_CYCAPTION)
 * - the menu size            (SM_CYMENU)
 */
void CMinMaxLogic::setClientMin(LONG x, LONG y )
{
	m_cxMin = x;
	m_cyMin = y;
	m_fxMin = 2*(GetSystemMetrics(SM_CXFRAME)+
		      GetSystemMetrics(SM_CXEDGE))   +
		      m_cxMin;
	m_fyMin = 2*(GetSystemMetrics(SM_CYFRAME)+
		      GetSystemMetrics(SM_CYEDGE))   +
              GetSystemMetrics(SM_CYCAPTION) +
              GetSystemMetrics(SM_CYMENU)    +
			  m_cyMin;
}

/*
 * CMinMaxLogic::OnGetMinMaxInfo function
 *
 * Note: Substract a nonunderstood fuzzy factor (2,4) based on the
 *       toolbar position.
 */
void CMinMaxLogic::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	lpMMI->ptMinTrackSize.x = m_fxMin;
	lpMMI->ptMinTrackSize.y = m_fyMin;
	if( m_sbVisible )
	{
		lpMMI->ptMinTrackSize.y += m_sbHeight;
	}

	if( m_tbVisible )
	{
	  if( m_tbPos == TBTOP || m_tbPos == TBBOTTOM )
	  {
		lpMMI->ptMinTrackSize.y += m_tbSize - 2;
	  }
	  else if( m_tbPos == TBLEFT || m_tbPos == TBRIGHT )
	  {
		lpMMI->ptMinTrackSize.x += m_tbSize - 4;
	  }
	}
}
