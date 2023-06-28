// UpdateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Galaxy.h"
#include "UpdateDlg.h"

#include "fce.h"
#include "keycode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern int updateprogress;
/////////////////////////////////////////////////////////////////////////////
// UpdateDlg dialog


UpdateDlg::UpdateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(UpdateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(UpdateDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void UpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(UpdateDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
		DDX_Control(pDX, IDC_UPDATEPROGRESS, m_updateprogress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(UpdateDlg, CDialog)
	//{{AFX_MSG_MAP(UpdateDlg)
	ON_BN_CLICKED(IDC_BUTTON1, DownloadUpdates)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL UpdateDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_updateprogress.SetRange(1, 5);
	m_updateprogress.SetPos(1);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// UpdateDlg message handlers

void UpdateDlg::DownloadUpdates() { //put all updated files on the client machine

	int error;

	//first back up old versions of exe(do this here since it's on the same dir level)
	remove("old_crx.exe");
	rename("crx.exe", "old_crx.exe");

	// Initialize FCE (look in KEYCODE.H for FCE_KEY_CODE)
	fceAttach(1, FCE_KEY_CODE);

	fceSetInteger(0, FCE_SET_PASSIVE, 1);
	fceSetInteger(0, FCE_SET_CONNECT_WAIT, 1000);
    fceSetInteger(0, FCE_SET_MAX_RESPONSE_WAIT, 1000);
	
	// Connect to FTP server
	error = fceConnect(0,"web.planethosting.gamespy.com","cor","eatshit");
	if(error < 0) {
		AfxMessageBox("Failed to connect to host!");
		rename("old_crx.exe", "crx.exe");
		goto exit;
	}

	fceSetMode(0,'B'); //set to binary

	//change to correct dir
	error = fceSetServerDir (0, "cor.planetquake.gamespy.com/codered/files");
	if(error < 0) {
		fceClose(0);
		AfxMessageBox("Failed to locate file!");
		rename("old_crx.exe", "crx.exe");
		goto exit;
	}
	m_updateprogress.SetPos(2);

	error = fceGetFile(0, "crx.exe");
	if(error < 0) {
		fceClose(0);
		AfxMessageBox("Failed to download file!");
		rename("old_crx.exe", "crx.exe");
		goto exit;
	}	
	m_updateprogress.SetPos(3);

	error = fceGetFile(0, "gamex86.dll");
	if(error < 0) {
		fceClose(0);
		AfxMessageBox("Failed to download game dll!");	
		goto exit;
	}
	m_updateprogress.SetPos(4);

	error = fceGetFile(0, "version.txt");
	if(error < 0) {
		fceClose(0);
		AfxMessageBox("Failed to update revision file!");
		goto exit;
	}
	
	//move the game dll to the right places(only get here if there is success)
	remove("arena/old_gamex86.dll");
	rename("arena/gamex86.dll", "arena/old_gamex86.dll");
	rename("gamex86.dll", "arena/gamex86.dll");
	m_updateprogress.SetPos(5);

	fceClose(0);
exit:
	fceRelease();	

}
