// Startup.cpp : implementation file
//

#include "stdafx.h"
#include "Galaxy.h"
#include "Startup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Startup dialog


Startup::Startup(CWnd* pParent /*=NULL*/)
	: CDialog(Startup::IDD, pParent)
{
	//{{AFX_DATA_INIT(Startup)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Startup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Startup)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Startup, CDialog)
	//{{AFX_MSG_MAP(Startup)
	ON_NOTIFY(NM_OUTOFMEMORY, IDC_PROGRESS2, OnRefreshProgress2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Startup message handlers

void Startup::OnRefreshProgress2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}
