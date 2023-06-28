// BuddyName.cpp : implementation file
//

#include "stdafx.h"
#include "Galaxy.h"
#include "BuddyName.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern char newBuddyName[32];
/////////////////////////////////////////////////////////////////////////////
// BuddyName dialog


BuddyName::BuddyName(CWnd* pParent /*=NULL*/)
	: CDialog(BuddyName::IDD, pParent)
{
	//{{AFX_DATA_INIT(BuddyName)
	m_buddynamestr = newBuddyName;
	//}}AFX_DATA_INIT
}


void BuddyName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(BuddyName)
	DDX_Control(pDX, IDC_BUDDYNAME, m_newbuddyname);
	DDX_Text(pDX, IDC_BUDDYNAME, m_buddynamestr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(BuddyName, CDialog)
	//{{AFX_MSG_MAP(BuddyName)
	ON_EN_CHANGE(IDC_BUDDYNAME, OnChangeBuddyname)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// BuddyName message handlers
BOOL BuddyName::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_newbuddyname.SetWindowText(newBuddyName);

	return TRUE;
}
void BuddyName::OnChangeBuddyname() 
{
	m_newbuddyname.GetWindowText(m_buddynamestr);	
}

void BuddyName::OnOK() 
{
	sprintf(newBuddyName, "%s", m_buddynamestr);

	CDialog::OnOK();
}

void BuddyName::OnCancel() 
{

	CDialog::OnCancel();
}
