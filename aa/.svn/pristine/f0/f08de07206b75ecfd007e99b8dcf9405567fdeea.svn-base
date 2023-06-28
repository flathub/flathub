// Help.cpp : implementation file
//

#include "stdafx.h"
#include "galaxy.h"
#include "Help.h"
#include "hyperlink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Help dialog


Help::Help()
	: CHyperLinkDlg(Help::IDD)
{
	//{{AFX_DATA_INIT(Help)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Help::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Help)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Help, CDialog)
	//{{AFX_MSG_MAP(Help)
	ON_BN_CLICKED(IDC_LINK1, OnLinks)
	ON_BN_CLICKED(IDC_LINK2, OnLinks)
	ON_BN_CLICKED(IDC_LINK3, OnLinks)
	ON_BN_CLICKED(IDC_LINK4, OnLinks)
	ON_BN_CLICKED(IDC_LINK5, OnLinks)
	ON_BN_CLICKED(IDC_LINK6, OnLinks)
	ON_BN_CLICKED(IDC_LINK7, OnLinks)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Help message handlers
BOOL Help::OnInitDialog() 
{
	CDialog::OnInitDialog();

	setURL(m_Link1,IDC_LINK1);
	setURL(m_Link2,IDC_LINK2);
	setURL(m_Link3,IDC_LINK3);
	setURL(m_Link4,IDC_LINK4);
	setURL(m_Link5,IDC_LINK5);
	setURL(m_Link6,IDC_LINK6);
	setURL(m_Link7,IDC_LINK7);

	return TRUE;
}

void Help::OnLinks() 
{
	// TODO: Add your control notification handler code here
	
}