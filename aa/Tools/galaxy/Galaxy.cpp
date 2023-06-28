// Galaxy.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Galaxy.h"
#include "GalaxyDlg.h"
#include "PollServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern void Open_Winsock(void);
extern void Close_Winsock(void);
extern void IRCDisconnect(void);

/////////////////////////////////////////////////////////////////////////////
// CGalaxyApp

BEGIN_MESSAGE_MAP(CGalaxyApp, CWinApp)
	//{{AFX_MSG_MAP(CGalaxyApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGalaxyApp construction

CGalaxyApp::CGalaxyApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGalaxyApp object

CGalaxyApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CGalaxyApp initialization

BOOL CGalaxyApp::InitInstance()
{

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	
	Open_Winsock();

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CGalaxyDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();

	if (nResponse == IDOK)
	{
		IRCDisconnect();
		Close_Winsock();
		exit(0);
	}

	IRCDisconnect();
	Close_Winsock();
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
