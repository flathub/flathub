// GalaxyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "winsock.h"
#include "time.h"
#include <iostream>
#include <iomanip> 
#include <fstream>
#include <wininet.h>
#include "Galaxy.h"
#include "GalaxyDlg.h"
#include "PollServer.h"
#include "userinfo.h"
#include "socket.h"
#include "functions.h"
#include "PlayerProfile.h"
#include "help.h"
#include "BuddyName.h"
#include "Startup.h"
#include "UpdateDlg.h"
#include <shellAPI.h>
#include <shlobj.h>
#include <direct.h>
#include  <io.h>
#include <mmsystem.h>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
 
#define MAXSERVERS 256

extern SERVERINFO servers[MAXSERVERS];

extern unsigned int numServers;
extern unsigned int totalPlayers;
extern void GetServerList(void);
extern void PingServers (SERVERINFO *server, CProgressCtrl *m_refreshprogress);
extern bool CheckVersion(void);

//globals
SERVERLIST serverstring[MAXSERVERS];
SERVERLIST serverlist[MAXSERVERS];

int refreshed;
int liveServers;
char Server[32];
char smensaje[1000];

int messagecount;

char CRXbuff[MAX_PATH];
char CRXPath[MAX_PATH];
char szStartFolder[MAX_PATH];

void handle_error(void);           // menejador de errores 
char mensaje[200];				   // variable de todo uso.
char servidor[100];
cUser user;
bool joinflg;
bool connectedToServer = false;
bool connectedToChannel = false;
bool canJoin = false;
int	lastPing = 0;

char currBuddyName[32];
char newBuddyName[32];
PLAYERINFO players[64];

cSocket sockete;

int count = 0;

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGalaxyDlg dialog

CGalaxyDlg::CGalaxyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGalaxyDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGalaxyDlg)
	m_sendstring = "";
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGalaxyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGalaxyDlg)
	DDX_Control(pDX, IDC_NEWS, m_news);
	DDX_Control(pDX, IDC_SHOWUSERS, m_showusers);
	DDX_Control(pDX, IDC_DELBUDDY, m_delbuddy);
	DDX_Control(pDX, IDC_ADDBUDDY, m_addbuddy);
	DDX_Control(pDX, IDC_BUDDYLIST, m_buddylist);
	DDX_Control(pDX, IDC_EDIT2, m_playernum);
	DDX_Control(pDX, IDC_STATUS2, m_status2);
	DDX_Control(pDX, IDC_BUTTON6, m_disconnect);
	DDX_Control(pDX, IDC_BUTTON4, m_joinchannel);
	DDX_Control(pDX, IDC_BUTTON5, m_sendtext);
	DDX_Control(pDX, IDC_LIST2, m_chatthread);
	DDX_Control(pDX, IDC_EDIT3, m_chatstring);
	DDX_Control(pDX, IDC_EDIT1, m_status);
	DDX_Control(pDX, IDC_BUTTON3, m_launchbmp);
	DDX_Control(pDX, IDC_BUTTON2, m_joinbmp);
	DDX_Control(pDX, IDC_BUTTON1, m_refreshbmp);
	DDX_Control(pDX, IDC_PROGRESS1, m_refreshprogress);
	DDX_Control(pDX, IDC_LIST4, m_serverrules);
	DDX_Control(pDX, IDC_LIST3, m_playerinfo);
	DDX_Control(pDX, IDC_LIST1, m_serverinfo);
	DDX_Text(pDX, IDC_EDIT3, m_sendstring);
	DDV_MaxChars(pDX, m_sendstring, 1024);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGalaxyDlg, CDialog)
	//{{AFX_MSG_MAP(CGalaxyDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CLICK, IDC_LIST1, OnSelchangeList1)
	ON_BN_CLICKED(IDC_BUTTON1, OnRefresh)
	ON_BN_CLICKED(ID_MENU_CONFIGURE, Configure)
	ON_BN_CLICKED(ID_MENU_HELP, HelpLinks)
	ON_BN_CLICKED(ID_MENU_ABOUT, About)
	ON_BN_CLICKED(ID_STATS_LOOKUPSTATS, LookUpStats)
	ON_EN_CHANGE(IDC_EDIT3, OnChangeEdit3)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON4, OnButton4)
	ON_BN_CLICKED(IDC_BUTTON5, OnButton5)
	ON_BN_CLICKED(IDC_BUTTON2, OnJoin2)
	ON_BN_CLICKED(IDC_BUTTON3, OnLaunch)
	ON_BN_CLICKED(IDC_BUTTON6, OnButton6)
	ON_BN_CLICKED(IDC_ADDBUDDY, OnAddbuddy)
	ON_BN_CLICKED(IDC_DELBUDDY, OnDelbuddy)
	ON_NOTIFY(NM_CLICK, IDC_BUDDYLIST, OnSelchangeBuddylist)
	ON_BN_CLICKED(IDC_PLAYERSORT, OnPlayersort)
	ON_BN_CLICKED(IDC_PINGSORT, OnPingsort)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST3, LookUpPlayerStats) 
	ON_BN_CLICKED(ID_SERVERTOOLS_REFRESH, OnRefresh)
	ON_BN_CLICKED(ID_SERVERTOOLS_JOIN, OnJoin2)
	ON_BN_CLICKED(ID_SERVERTOOLS_LAUNCHALIENARENA, OnLaunch)
	ON_BN_CLICKED(ID_CHAT_JOINCHATCHANNEL, OnButton5)
	ON_BN_CLICKED(ID_CHAT_LEAVECHATCHANNEL, OnButton6)
	ON_BN_CLICKED(IDC_SHOWUSERS, OnShowusers)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnJoin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int Sys_Milliseconds (void)
{
	static int		base;
	static bool	initialized = false;
	int timeofday;

	if (!initialized)
	{	// let base retain 16 bits of effectively random data
		base = timeGetTime() & 0xffff0000;
		initialized = true;
	}
	timeofday = timeGetTime() - base;

	return timeofday;
}

UINT RecvThreadProc(LPVOID pParam)
{

	while(1) 
	{
		Sleep(1000); //try not to eat up CPU
		sockete.getData();	
		//if an error - break - will have to set something up

		if(!connectedToChannel && canJoin) 
		{
			sockete.sendData("JOIN #alienarena\n\r");
			connectedToChannel = true;
			AfxMessageBox("Joining #alienarena");
		}
		
		if(Sys_Milliseconds() - lastPing > (200 * 1000))
		{
			//send a pong if no ping received in 200 seconds as a desperation attempt to keep us connected to server
			sockete.sendData("PONG\n\r");
			lastPing = Sys_Milliseconds();
		}
	}
	return 0;
}

response SkipWords(char msg[250],int skip)
{
	response res;
	int ichar=0;

	int counter=0;
	memset((res.word),'\0',200*1);
	for (int t=0; t<256;t++)
	{
		if (msg[t]==' ') counter++;
		if (counter>=skip) {
			res.word[0][ichar]=msg[t];
			ichar++;
		};
	}
	return res;
}

response GetWords(char msg[200])
{
	response res;
	int word=0;
	int ichar=0;

	memset((res.word),'\0',200*50);

	for (int t=0; t<200;t++)
	{
		res.word[word][ichar] = msg[t];
		ichar++;
		if (msg[t]==' ') {
			ichar=0;
			word++; 
		};
	}
	res.words = word;
	return res;
};
/////////////////////////////////////////////////////////////////////////////
// CGalaxyDlg message handlers

BOOL CGalaxyDlg::OnInitDialog()
{

	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here

	//server list
	m_serverinfo.Init();
	
	m_serverinfo.SetBkColor(RGB(76,85,118));
	m_serverinfo.SetTextColor(RGB(222,222,222));
	
	m_serverinfo.InsertColumn(0, "SERVERNAME", LVCFMT_LEFT, 200);
	m_serverinfo.InsertColumn(1, "ADDRESS", LVCFMT_LEFT, 110);
	m_serverinfo.InsertColumn(2, "MAP", LVCFMT_LEFT, 100);
	m_serverinfo.InsertColumn(3, "PLAYERS", LVCFMT_LEFT, 50);
	m_serverinfo.InsertColumn(4, "PING", LVCFMT_LEFT, 50);

	ListView_SetExtendedListViewStyle(m_serverinfo.m_hWnd, LVS_EX_FULLROWSELECT  | LVS_EX_HEADERDRAGDROP);
	
	LVITEM lvi;
	lvi.mask =  LVIF_IMAGE | LVIF_TEXT;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = (LPTSTR)(LPCTSTR)("Click Refresh to see servers!");
	lvi.iImage = 2;	
	m_serverinfo.InsertItem(&lvi);
	
	//player list
	m_playerinfo.Init();
	
	m_playerinfo.SetBkColor(RGB(76,85,118));
	m_playerinfo.SetTextColor(RGB(222,222,222));
	
	m_playerinfo.InsertColumn(0, "PLAYER", LVCFMT_LEFT, 100);
	m_playerinfo.InsertColumn(1, "SCORE", LVCFMT_LEFT, 40);
	m_playerinfo.InsertColumn(2, "PING", LVCFMT_LEFT, 40);

	ListView_SetExtendedListViewStyle(m_playerinfo.m_hWnd, LVS_EX_FULLROWSELECT  | LVS_EX_HEADERDRAGDROP);
	
	//serverinfo
	m_serverrules.Init();
	
	m_serverrules.SetBkColor(RGB(76,85,118));
	m_serverrules.SetTextColor(RGB(222,222,222));
	
	m_serverrules.InsertColumn(0, "RULE", LVCFMT_LEFT, 60);
	m_serverrules.InsertColumn(1, "VALUE", LVCFMT_LEFT, 200);

	ListView_SetExtendedListViewStyle(m_serverrules.m_hWnd, LVS_EX_FULLROWSELECT  | LVS_EX_HEADERDRAGDROP);
	
	//buddy list
	m_buddylist.Init();
	
	m_buddylist.SetBkColor(RGB(76,85,118));
	m_buddylist.SetTextColor(RGB(222,222,222));
	
	m_buddylist.InsertColumn(0, "PLAYER: GREEN=ONLINE  RED = OFFLINE", LVCFMT_LEFT, 180);

	ListView_SetExtendedListViewStyle(m_buddylist.m_hWnd, LVS_EX_FULLROWSELECT  | LVS_EX_HEADERDRAGDROP);
	

	//chat window
	messagecount = 0;
	m_chatthread.Init();
	
	m_chatthread.SetBkColor(RGB(76,85,118));
	m_chatthread.SetTextColor(RGB(0,222,0));
	
	m_chatthread.InsertColumn(0, "IRC CHAT WINDOW", LVCFMT_LEFT, 510);

	ListView_SetExtendedListViewStyle(m_chatthread.m_hWnd, LVS_EX_FULLROWSELECT  | LVS_EX_HEADERDRAGDROP);
	
	BOOL   bRetValue = FALSE;
	HICON  hIcon = NULL;

	// Create image list
	bRetValue = m_ImageList.Create(16, 16,
								   ILC_COLOR32 | ILC_MASK,
								   5, 1);
	ASSERT(bRetValue == TRUE);

	// Add some icons for listboxes
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
	m_ImageList.Add(hIcon);
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON2);
	m_ImageList.Add(hIcon);
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON3);
	m_ImageList.Add(hIcon);
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON4);
	m_ImageList.Add(hIcon);
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON5);
	m_ImageList.Add(hIcon);
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON6);
	m_ImageList.Add(hIcon);
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON7);
	m_ImageList.Add(hIcon);
	hIcon = AfxGetApp()->LoadIcon(IDI_ICON8);
	m_ImageList.Add(hIcon);

	// Associate image list to list box
	m_serverinfo.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_playerinfo.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_buddylist.SetImageList(&m_ImageList, LVSIL_SMALL);

	//load bitmaps for buttons
	m_refreshbmp.LoadBitmaps(IDB_BITMAP4, IDB_BITMAP3, IDB_BITMAP1);
	m_joinbmp.LoadBitmaps(IDB_BITMAP7, IDB_BITMAP5, IDB_BITMAP6);
	m_launchbmp.LoadBitmaps(IDB_BITMAP8, IDB_BITMAP10, IDB_BITMAP9);
	m_joinchannel.LoadBitmaps(IDB_BITMAP13, IDB_BITMAP11, IDB_BITMAP12);
	m_disconnect.LoadBitmaps(IDB_BITMAP14, IDB_BITMAP16, IDB_BITMAP15);
	m_sendtext.LoadBitmaps(IDB_BITMAP17, IDB_BITMAP19, IDB_BITMAP18);
	m_addbuddy.LoadBitmaps(IDB_BITMAP22, IDB_BITMAP20, IDB_BITMAP21);
	m_delbuddy.LoadBitmaps(IDB_BITMAP25, IDB_BITMAP23, IDB_BITMAP24);
	m_showusers.LoadBitmaps(IDB_BITMAP26, IDB_BITMAP28, IDB_BITMAP27);
	SetDefID(IDC_BUTTON5); //set default button to send text

	//put in a default status message
	m_status.SetWindowText("Refresh server list!");

	refreshed = false;

	//read in user data from .ini - if not found, ie, first time running, we
	//want to bring up a dialog to force them to choose an identity.
	char defName[32];

	sprintf(defName, "Player");

	GetPrivateProfileString("Galaxy", "name", defName, user.nick, 32, "galaxy.ini");
	GetPrivateProfileString("Galaxy", "email", "email@email.com", user.email, 100, "galaxy.ini");
	GetPrivateProfileString("Galaxy", "exe", "C:/Alien Arena 7_66/", CRXPath, MAX_PATH, "galaxy.ini");
	GetPrivateProfileString("Galaxy", "chatstart", "true", user.joinatstart, 12, "galaxy.ini");
	GetPrivateProfileString("Galaxy", "server", "irc.planetarena.org", servidor, 100, "galaxy.ini");
	//set the join flag for the dialog bool
	if(!_tcscmp(user.joinatstart, "true"))
		joinflg = true;
	else
		joinflg = false;

	//force a player name change the first time program is run, otherwise, continue happily
	if(!strcmp(user.nick, "Player")) {
		PlayerProfile ppDlg;
		ppDlg.DoModal();
	}
	//get daily news
	m_news.SetWindowText("Newsfeed: Welcome to Alien Arena!");
	GetNews();
	//initialize IRC, log into channel, and start streaming to edit box

	//verify date of client files
	BOOL current = CheckVersion();
	if(!current) {
		//start a new dialog
		UpdateDlg ppDlg;
		ppDlg.DoModal();

	}

	//set up data thread
	AfxBeginThread(RecvThreadProc, GetSafeHwnd(),
	               THREAD_PRIORITY_IDLE);

	SetTimer(100,1000,NULL); //parse data if it's there

	strcpy(user.ident, user.nick);

	if(!strcmp(user.joinatstart, "true")) {
		WSASetLastError(0);
		sockete.init(6667,servidor);
		sockete.connectf();

		Sleep(1000); //just a short sleep to allow the socket to open all the way

		sprintf(mensaje,"USER %s %s: %s %s  \n\r", user.nick , user.email , user.ident , user.ident );
		sockete.sendData(mensaje);
		sprintf(mensaje,"NICK %s\n\r", user.nick);
		sockete.sendData(mensaje);
		connectedToChannel = false;
		m_status2.SetWindowText("connected to server");
		connectedToServer = true;

		sockete.getData();
	}	
	else {
		m_status2.SetWindowText("Disconnected from chat channel...");
		connectedToServer = false;
	}
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGalaxyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGalaxyDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGalaxyDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

//when someone selects a server in the list(single click)
void CGalaxyDlg::OnSelchangeList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	char info[128];
	int i, j;
	char server_ip[16];
	SOCKADDR_IN addr;
	LVITEM lvi;

	if(!refreshed)
		return;

	i = m_serverinfo.GetNextItem(-1,LVNI_SELECTED);
	j = serverlist[i].real_pos;

	//nuke player list
	for(i=0; i<64; i++) {
		players[i].playername[0] = 0;
		players[i].orgname[0] = 0;
	}
	//destroy old list
	m_playerinfo.DeleteAllItems();
	m_playerinfo.SetRedraw(FALSE);

	for (i=0; i<servers[j].curClients; i++) {

		strcpy(players[i].playername, servers[j].players[i].playername);
		strcpy(players[i].orgname, servers[j].players[i].playername); //fix me, no longer needed

		strcpy(players[i].playername, servers[j].players[i].playername);
		
		//add info to list
		//add the sorted list into the window 
		
		lvi.mask =  LVIF_IMAGE | LVIF_TEXT;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.pszText = (LPTSTR)(LPCTSTR)(servers[j].players[i].playername);
		if(servers[j].players[i].ping > 0)
			lvi.iImage = 1;
		else
			lvi.iImage = 6; //bot
		m_playerinfo.InsertItem(&lvi);

		sprintf(info, "%i",  servers[j].players[i].score);
		m_playerinfo.SetItemText(i, 1, info);
		sprintf(info, "%i", servers[j].players[i].ping);
		m_playerinfo.SetItemText(i, 2, info);

	}
	m_playerinfo.SetRedraw(TRUE);

	//do server info
	//convert the ip to a string
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.S_un.S_addr = servers[j].ip;
	sprintf (server_ip, "%s", inet_ntoa(addr.sin_addr));
	sprintf(Server, "%s:%i", server_ip, servers[j].port);

	//destroy old list
	m_serverrules.DeleteAllItems();
	m_serverrules.SetRedraw(FALSE);

	m_serverrules.InsertItem(0,  "Admin");
	m_serverrules.SetItemText(0, 1, servers[j].szAdmin);

	m_serverrules.InsertItem(0,  "Website");
	m_serverrules.SetItemText(0, 1, servers[j].szWebSite);

	m_serverrules.InsertItem(0,  "Fraglimit");
	m_serverrules.SetItemText(0, 1, servers[j].szFragLimit);

	m_serverrules.InsertItem(0,  "Timelimit");
	m_serverrules.SetItemText(0, 1, servers[j].szTimeLimit);

	m_serverrules.InsertItem(0,  "Version");
	m_serverrules.SetItemText(0, 1, servers[j].szVersion);
	
	m_serverrules.SetRedraw(TRUE);

	*pResult = 0;
}

void CGalaxyDlg::Do_Refresh() //refresh server list
{
	int i, j, k;
	char server_ip[16];
	char txt[128];
	SOCKADDR_IN addr;
	int lowestPing;
	LVITEM lvi;

	//destroy list
	m_serverinfo.DeleteAllItems();

	//nuke data array
	for(i = 0; i < MAXSERVERS; i++) {
		serverlist[i].name[0] = serverstring[i].name[0] = 0;
		serverlist[i].map[0] = serverstring[i].map[0] = 0;
		serverlist[i].ipstring[0] = serverstring[i].ipstring[0] = 0;
		serverlist[i].real_pos = serverstring[i].real_pos = 0;
		serverlist[i].ping = serverstring[i].ping = 0;
		serverlist[i].players = serverstring[i].players = 0;
		serverlist[i].sorted = serverstring[i].sorted = false;
		serverlist[i].ip = 0;
		servers[i].szHostName[0] = 0; //nuke out the server list
		servers[i].szAdmin[0] = 0;
		servers[i].szWebSite[0] = 0;
		servers[i].ip = 0;
	}

	liveServers = 0;
	totalPlayers = 0;

	//call to master and get server list
	GetServerList();

	if(!numServers) {
		m_status.SetWindowText("Unable to reach a master server!");
		refreshed = true;
		return;
	}

	m_status.SetWindowText("Pinging servers in list...");

	PingServers(servers, &m_refreshprogress);

	j = 0;
	for(i = 0; i < numServers; i++){

		//convert the ip to a string
		memset(&addr, 0, sizeof(addr));
		addr.sin_addr.S_un.S_addr = servers[i].ip;
		sprintf (server_ip, "%s", inet_ntoa(addr.sin_addr));

	
		if(servers[i].szHostName[0]) {
			strcpy(serverstring[j].name, servers[i].szHostName);
			strcpy(serverstring[j].map, servers[i].szMapName);
			sprintf(serverstring[j].ipstring, "%s:%i", server_ip, servers[i].port);
			serverstring[j].ping = servers[i].ping; //for sorting by ping
			serverstring[j].players = servers[i].curClients; //for sorting by player total
			serverstring[j].real_pos = i;
			serverstring[i].sorted = false;
			liveServers++;
			j++;
		}
	}

	//sort the list, by ping, or by players
	int x = 0;
	while(liveServers) {
		//find the next unsorted ping value
		for(i=0; i<liveServers; i++) {
			if(!serverstring[i].sorted) {//got one
				lowestPing = serverstring[i].ping;
				break;
			}
		}
		for(i=0; i<liveServers; i++) { 
			
			if(!serverstring[i].sorted && serverstring[i].ping <= lowestPing) {
				lowestPing = serverstring[i].ping;
				k = i;
			}
		}
		//this is the lowest ping, put it in the array
		strcpy(serverlist[x].name, serverstring[k].name);
		strcpy(serverlist[x].map, serverstring[k].map);
		strcpy(serverlist[x].ipstring, serverstring[k].ipstring);
		serverlist[x].ping = serverstring[k].ping;
		serverlist[x].real_pos = serverstring[k].real_pos; //this is the wrong way to do it!
		serverlist[x].players = serverstring[k].players;
		serverstring[k].sorted = serverlist[x].sorted = true; //it was the lowest, it's inserted now
		x++;
		if(x>255 || x>=liveServers) //we have sorted all servers and don't overrun array size
			break;
	}
	//add the sorted list into the window 
	
	m_serverinfo.DeleteAllItems();
	m_serverinfo.SetRedraw(FALSE);
	for(i=0; i<x; i++)
	{
			
		lvi.mask =  LVIF_IMAGE | LVIF_TEXT;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.pszText = (LPTSTR)(LPCTSTR)(serverlist[i].name);
		if(!strcmp("noname", serverlist[i].name))
			lvi.iImage = 5;
		else
			lvi.iImage = 2;	
		m_serverinfo.InsertItem(&lvi);

		m_serverinfo.SetItemText(i, 1, serverlist[i].ipstring);
		m_serverinfo.SetItemText(i, 2, serverlist[i].map);
		sprintf(txt, "%i", serverlist[i].players);
		m_serverinfo.SetItemText(i, 3, txt);
		sprintf(txt, "%i", serverlist[i].ping);
		m_serverinfo.SetItemText(i, 4, txt);
	}

	m_serverinfo.SetRedraw(TRUE);

	if(numServers > 0)
		sprintf(txt, "%i of %i servers responded", liveServers, numServers);
	else
		sprintf(txt, "Error retrieving server list...");
	m_status.SetWindowText(txt);
	sprintf(txt, "%i players", totalPlayers);
	m_playernum.SetWindowText(txt);

	refreshed = true;

}

void CGalaxyDlg::Check_Buddies()
{
	ifstream buddydb;
	char buddy[32];
	int i, j, x;
	bool online;
	LVITEM lvi;

	//destroy list
	m_buddylist.DeleteAllItems();

	m_buddylist.SetRedraw(FALSE);
	buddydb.open("buddylist.db");
	//we'll make max of 64 buddies to prevent congestion
	if(buddydb.good()) {
		x = 0;
		while(1) {
			buddydb.getline(buddy, 16);
			online = false;

			if(!buddy[0])
				break;

			//go through each server, check each player list
			for(i=0; i<numServers; i++){
				for(j=0; j<servers[i].curClients; j++) {
					if(!strcmp(buddy, servers[i].players[j].playername)) {
						//online
						online = true;
						break;
					}
				}
				if(online)
					break; //no need to continue
			}
			
			lvi.mask =  LVIF_IMAGE | LVIF_TEXT;
			lvi.iItem = x;
			lvi.iSubItem = 0;	
			lvi.pszText = (LPTSTR)(LPCTSTR)(buddy);
			
			if(online) 			
				lvi.iImage = 4;	
			else 
				lvi.iImage = 3;	
					
			m_buddylist.InsertItem(&lvi);
			x++;
			if(x>63)
				break;		
		}
	}
	buddydb.close();
	m_buddylist.SetRedraw(TRUE);
}	

void CGalaxyDlg::OnRefresh() 
{
	m_status.SetWindowText("Pinging master server...");
	Do_Refresh();	
	Check_Buddies();
}

void CGalaxyDlg::OnChangeEdit3() 
{
	m_chatstring.GetWindowText(m_sendstring);
}

void CGalaxyDlg::OnTimer(UINT nIDEvent) 
{
	int ichar =0;
	char line[3000];
	char prevLine[3000]; 

	if(nIDEvent==100)	
	{
		//send the buffer to the dialog
		if (sockete.len > 1) 
		{			
			memset(line,0,3000);
			for (int t=0;t<strlen(sockete.File_Buf);t++)
			{
				line[ichar]= sockete.File_Buf[t];
				ichar++;
				if(sockete.File_Buf[t]==13) {
					ichar=0;
					if(!strcmp(line, prevLine)) { //don't print duplicate messages
						memset(line,0,3000);
						return;
					}
					analizeLine(line);
					strcpy(prevLine, line);
					memset(line,0,3000);
				};
			}

		}
	}
/*	if(!refreshed) {
		refreshed = true;
		//Get server list, and show progress so people know what is going on
		//MessageBoxA("Obtaining Server List - May take a few moments...", "Galaxy Startup", NULL);
		//Do_Refresh();	
		//Check_Buddies();
		MessageBoxA("Click Refresh to see server list!");
	}*/
	CDialog::OnTimer(nIDEvent);
}

void CGalaxyDlg::analizeLine(char Line[1000])
{
	extern cUser user; 
	response Response;
    char cmpmsg[10][200]; 
    char outputmsg[1000];
	char msgLine[101];
	bool printed = false;

	outputmsg[0] = 0;
	msgLine[0] = 0;
	if(messagecount > 5000) { //clear everything 
		m_chatthread.DeleteAllItems();
		messagecount = 0;
	}
	m_chatthread.SetRedraw(FALSE);

	sprintf(cmpmsg[0],"353 %s", user.nick);
	if ((strstr(Line,cmpmsg[0]) != NULL))
    {
		memset((smensaje),'\0',1000);
		memset((Response.word[0]),'\0',1000);
		memset((outputmsg),'\0',1000);	
		for(int ti=0;ti<40;ti++)
		{
			smensaje[ti]=Line[ti];
			if (Line[ti]==':') 
			{
				Response = SkipWords(Line,6);
						
				//this is the string we need to check length of, and if it's over the size of 
				//the window width, we need to split it into multiple lines.  Should not be too
				//difficult to implement.
				sprintf(outputmsg,"<%s> %s ",smensaje,Response.word[0]);
				outputmsg[strlen(outputmsg)-2] = 0; //don't want that linefeed showing up
							
				int lines = 0;
				lines = strlen(outputmsg)/80 + 1; //how many lines do we have?
				for(int i=0; i<lines; i++) {
					//get a segment of the total message
					memset(msgLine,'\0', 81);
					for(int j=0; j<80; j++) 
						msgLine[j] = outputmsg[j+(80*i)];
					msgLine[80] = 0;

					m_chatthread.InsertItem(messagecount, msgLine);
					messagecount++;

					printed = true;
				}
			}
		}
		
	 }

     sprintf(cmpmsg[0],"372 %s", user.nick);
     sprintf(cmpmsg[1],"251 %s", user.nick);
     sprintf(cmpmsg[2],"252 %s", user.nick);
     sprintf(cmpmsg[3],"253 %s", user.nick);
     sprintf(cmpmsg[4],"254 %s", user.nick);
     sprintf(cmpmsg[5],"255 %s", user.nick);
     sprintf(cmpmsg[6],"322 %s", user.nick); // <-- LIST
     sprintf(cmpmsg[7],"421 %s", user.nick); // <-- unkwown command, etc.
     sprintf(cmpmsg[8],"461 %s", user.nick); // <-- no enougth parameters, etc.
				    
   if ((strstr(Line,cmpmsg[0]) != NULL)||(strstr(Line,cmpmsg[1]) != NULL)||(strstr(Line,cmpmsg[2]) != NULL)||(strstr(Line,cmpmsg[3]) != NULL)||(strstr(Line,cmpmsg[4]) != NULL)||(strstr(Line,cmpmsg[5]) != NULL)||(strstr(Line,cmpmsg[6]) != NULL)||(strstr(Line,cmpmsg[7]) != NULL)||(strstr(Line,cmpmsg[8]) != NULL))
   {
	   memset((Response.word[0]),'\0',256);
	   Response = SkipWords(Line,3);
	   Response.word[0][strlen(Response.word[0])-2]= 0;
		
	   m_chatthread.InsertItem(messagecount, Response.word[0]);
	   messagecount++;
	   m_chatthread.Scroll(CSize(0, messagecount)); //just just gets it to the bottom for sure
	   printed = true;
	}


	sprintf(cmpmsg[0],"PRIVMSG #"); // <-- LIST
	if ((strstr(Line,cmpmsg[0]) != NULL))
	{
		memset((smensaje),'\0',1000);
		memset((Response.word[0]),'\0',1000);
		memset((outputmsg),'\0',1000);
					
		for(int ti=0;ti<40;ti++)
		{
			smensaje[ti]=Line[ti];
			if (Line[ti]=='!') 
			{
				Response = SkipWords(Line,3);
						
				//this is the string we need to check length of, and if it's over the size of 
				//the window width, we need to split it into multiple lines.  Should not be too
				//difficult to implement.
				sprintf(outputmsg,"<%s> %s ",smensaje,Response.word[0]);
				outputmsg[strlen(outputmsg)-2] = 0; //don't want that linefeed showing up
							
				int lines = 0;
				lines = strlen(outputmsg)/100 + 1; //how many lines do we have?
				for(int i=0; i<lines; i++) {
					//get a segment of the total message
					memset(msgLine,'\0', 101);
					for(int j=0; j<100; j++) 
						msgLine[j] = outputmsg[j+(100*i)];
					msgLine[100] = 0;

					m_chatthread.InsertItem(messagecount, msgLine);
					messagecount++;
					printed = true;
				}
			}
		}
	}
	
	if(!printed) //users coming and going
	{
		memset((smensaje),'\0',1000);
		memset((Response.word[0]),'\0',1000);
		memset((outputmsg),'\0',1000);
			
		for(int ti=0;ti<40;ti++)
		{
			smensaje[ti]=Line[ti];
			if (Line[ti]=='!') 
			{
				Response = SkipWords(Line,1);
						
				//this is the string we need to check length of, and if it's over the size of 
				//the window width, we need to split it into multiple lines.  Should not be too
				//difficult to implement.
				sprintf(outputmsg,"<%s> %s ",smensaje,Response.word[0]);
				outputmsg[strlen(outputmsg)-2] = 0; //don't want that linefeed showing up
							
				int lines = 0;
				lines = strlen(outputmsg)/100 + 1; //how many lines do we have?
				for(int i=0; i<lines; i++) {
					//get a segment of the total message
					memset(msgLine,'\0', 101);
					for(int j=0; j<100; j++) 
						msgLine[j] = outputmsg[j+(100*i)];
					msgLine[100] = 0;

					m_chatthread.InsertItem(messagecount, msgLine);
					messagecount++;

				}
			}
		}
	}
	m_chatthread.SetRedraw(TRUE);
	int pos = m_chatthread.GetItemCount();
	m_chatthread.Scroll(CSize(0, pos));
}

void cSocket::init(int port,char* server)
{
	BUF_LEN=200; 
   
	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
		return;
	address.sin_family=AF_INET;       /* internet */
    address.sin_port = htons(port);    

    sprintf(HostName, server) ;

    if ( (host=gethostbyname(HostName)) == NULL )
 		return;
    address.sin_addr.s_addr=*((unsigned long *) host->h_addr);
  
}

void cSocket::sendData(char *msg)
{
	send(sock, msg,strlen(msg),0); 
};

void cSocket::connectf(void) 
{
	if ( (connect(sock,(struct sockaddr *) &address, sizeof(address))) != 0)
		return;
}

void cSocket::getData(void)
{

	len=0;	
	memset(File_Buf,0,3000);
	if((len=recv(sock,File_Buf,3000,0))>0) 
	{
	    // received a ping from server...
		if (!strnicmp(File_Buf,"PING",4)) 
		{
			lastPing = Sys_Milliseconds();
			canJoin = true;
			File_Buf[1]='O'; //cause of echo??
			sendData(File_Buf);
		};
	}
}

void cSocket::handle_error(void)
{
    switch ( WSAGetLastError() )
    {
      case WSANOTINITIALISED :
		AfxMessageBox("Unable to initialise socket.");
      break;
      case WSAEAFNOSUPPORT :
        AfxMessageBox("The specified address family is not supported.");
      break;
      case WSAEADDRNOTAVAIL :
        AfxMessageBox("Specified address is not available from the local machine.");
      break;
      case WSAECONNREFUSED :
        AfxMessageBox("The attempt to connect was forcefully rejected."); 
        break; 
      case WSAEDESTADDRREQ : 
        AfxMessageBox("address destination address is required.");
      break;
      case WSAEFAULT :
        AfxMessageBox("The namelen argument is incorrect.");
      break;
      case WSAEINVAL :
        AfxMessageBox("The socket is not already bound to an address.");
      break;
      case WSAEISCONN :
        AfxMessageBox("The socket is already connected.");
      break;
      case WSAEADDRINUSE :
        AfxMessageBox("The specified address is already in use.");
      break;
      case WSAEMFILE : 
        AfxMessageBox("No more file descriptors are available.");
      break;
      case WSAENOBUFS :
        AfxMessageBox("No buffer space available. The socket cannot be created.");
      break;
      case WSAEPROTONOSUPPORT :
        AfxMessageBox("The specified protocol is not supported.");
        break; 
      case WSAEPROTOTYPE :
        AfxMessageBox("The specified protocol is the wrong type for this socket.");
      break;
      case WSAENETUNREACH : 
        AfxMessageBox("The network can't be reached from this host at this time.");
      break; 
      case WSAENOTSOCK :
         AfxMessageBox("The descriptor is not a socket.");
      break;
      case WSAETIMEDOUT :
        AfxMessageBox("Attempt timed out without establishing a connection.");
      break;
      case WSAESOCKTNOSUPPORT :
         AfxMessageBox("Socket type is not supported in this address family.");
      break;
      case WSAENETDOWN :
        AfxMessageBox("Network subsystem failure.");
      break;
      case WSAHOST_NOT_FOUND :
        AfxMessageBox("Authoritative Answer Host not found.");
      break;
      case WSATRY_AGAIN :
        AfxMessageBox("Non-Authoritative Host not found or SERVERFAIL.");
       break;
      case WSANO_RECOVERY :
         AfxMessageBox("Non recoverable errors, FORMERR, REFUSED, NOTIMP.");
      break;
      case WSANO_DATA :
        AfxMessageBox("Valid name, no data record of requested type.");
      break;
        case WSAEINPROGRESS :
        AfxMessageBox("address blocking Windows Sockets operation is in progress.");
      break;
      default :
        AfxMessageBox("Unknown connection error.");
       break;
	}
}


void CGalaxyDlg::OnButton4() 
{

	response Response;

	WSASetLastError(0);
	sockete.init(6667,servidor);
	sockete.connectf();

	sprintf(mensaje,"USER %s %s: %s %s  \n\r", user.nick , user.email , user.ident , user.ident );
	sockete.sendData(mensaje);
	sprintf(mensaje,"NICK %s\n\r", user.nick);
	sockete.sendData(mensaje);

	connectedToChannel = false;
	canJoin = false;
	m_status2.SetWindowText("connected to server");
	connectedToServer = true;
	
	sockete.getData();
	//send the buffer to a messagebox
	if (sockete.len > 1) 
	{			
		Response = SkipWords(sockete.File_Buf,0);
		MessageBoxA(Response.word[0], "Welcome to #alienarena!", NULL);
	}
}

void CGalaxyDlg::OnShowusers() 
{ 
 
	if(connectedToServer)
		sockete.sendData("JOIN #alienarena\n\r");
	else
		AfxMessageBox("Must connect to chat server first!");

	return; //broken - fix later
	response Response;

	WSASetLastError(0);

	sprintf(mensaje, "NAMES %s\n\r", "#alienarena");
	sockete.sendData(mensaje);
	if(WSAGetLastError()) { //there was some error in connecting
			sockete.handle_error();
			m_status2.SetWindowText("not connected to #alienarena");
	}

	sockete.getData();
	//send the buffer to a messagebox
	if (sockete.len > 1) 
	{			
		Response = SkipWords(sockete.File_Buf,8);
		MessageBoxA(Response.word[0], "User List", NULL);
	}
}

void CGalaxyDlg::OnButton5() 
{
	char msgLine[111];
	char tempstring[1024]; 
	int i;

	//check if it's an "action"
	if(m_sendstring[0] == '/' && m_sendstring[1] == 'm' && m_sendstring[2] == 'e') {
		//trim out the command
		for(i = 4; i < 1024; i++)
			tempstring[i-4] = m_sendstring[i];
		sprintf(mensaje, "PRIVMSG #alienarena :\001ACTION %s\001\n\r", tempstring);
		WSASetLastError(0);
		sockete.sendData(mensaje);
		//send a junk string to clear the command
		sockete.sendData("PRIVMSG #alienarena :\n\r");
	}
	else {
		sprintf(mensaje, "PRIVMSG #alienarena :%s\n\r", m_sendstring);

		WSASetLastError(0);
		sockete.sendData(mensaje);
	}

	if(WSAGetLastError()) { //there was some error in connecting
			sockete.handle_error();
			m_status2.SetWindowText("not connected to #alienarena");
	}
	else {
		//update the list box
		sprintf(mensaje, "<%s> :%s", user.nick, m_sendstring);

		int lines = 0;
		lines = strlen(mensaje)/110 + 1; //how many lines do we have?
		for(int i=0; i<lines; i++) {
			//get a segment of the total message
			memset(msgLine,'\0', 111);
			for(int j=0; j<110; j++) 
				msgLine[j] = mensaje[j+(110*i)];
			msgLine[110] = 0;

			m_chatthread.InsertItem(messagecount, msgLine);
			messagecount++;
		}
		int pos = m_chatthread.GetItemCount();
		m_chatthread.Scroll(CSize(0, pos));
	}
	m_chatstring.SetWindowText(""); //ok we sent, or tried to send, clear the edit box
	m_sendstring = "";
	m_chatstring.UpdateData();
	
}

                                     /*--------------------------------*/
                                     /* BrowseProc                     */
                                     /*--------------------------------*/
int WINAPI BrowseProc( HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData )
{
  TCHAR szCur[301];
  int nIndx;

  switch( msg) 
  {
  case BFFM_INITIALIZED:
    nIndx = _tcslen( CRXPath ) - 1;
    if ( szStartFolder[nIndx] == _T('\\') ) 
    {
      nIndx--;
    }

    szStartFolder[nIndx+1] = 0;
    while( nIndx > 3 && _taccess( szStartFolder, 0 )) 
    {
      while( nIndx > 0 && szStartFolder[nIndx] != _T('\\') ) 
      {
        nIndx--;
      }

      if ( nIndx != 3 && szStartFolder[nIndx] == _T('\\') ) 
      {
        nIndx--;
      }

      szStartFolder[nIndx+1] = 0;
    }

    if ( nIndx > 0 ) 
    {
      SendMessage( hwnd, BFFM_SETSELECTION, 1, (LPARAM) szStartFolder );
    }
    else 
    {
      _tgetcwd( szCur, 301 );
      SendMessage( hwnd, BFFM_SETSELECTION, 1, (LPARAM) szCur );
    }
    break;
  }

  return( FALSE );
}

                                     /*--------------------------------*/
                                     /* BrowseForFolder                */
                                     /*--------------------------------*/
BOOL BrowseForFolder( TCHAR *szFolder, TCHAR *szTitle )
{
  BROWSEINFO pBrowseInfo;
  LPITEMIDLIST pDesktop;
  LPITEMIDLIST pBrowseList;

  _tcscpy( szStartFolder, szFolder );

      // Get the ITEMIDLIST for the desktop - this will be used to initialize the folder browser
  SHGetSpecialFolderLocation( NULL, CSIDL_DRIVES, /**CSIDL_DESKTOP,**/ &pDesktop );

      // Fill the BROWSEINFO data structure
  pBrowseInfo.hwndOwner = NULL;
  pBrowseInfo.pidlRoot = pDesktop;
  pBrowseInfo.pidlRoot = NULL;
  pBrowseInfo.pszDisplayName = szFolder;
  pBrowseInfo.lpszTitle = szTitle;
  pBrowseInfo.ulFlags = BIF_RETURNONLYFSDIRS; //BIF_BROWSEFORPRINTER | BIF_STATUSTEXT ;
  pBrowseInfo.lpfn = BrowseProc;
  pBrowseInfo.lParam = 0;
  pBrowseInfo.iImage = 0;

    // Start Browsing
  pBrowseList = SHBrowseForFolder( &pBrowseInfo );

    // if returning NULL we skip
  if ( !pBrowseList ) 
  {
    return( FALSE );
  }

  SHGetPathFromIDList( pBrowseList, szFolder );

  if ( szFolder[_tcslen(szFolder)-1] != _T('\\') ) 
  {
    _tcscat( szFolder, _T("\\") );
  }

  return( TRUE );
}

TCHAR GLOBAL_drive;

//#define NO_ERROR            0
#define READ_ERROR          1
#define WRITE_ERROR         2
#define FIND_ERROR_WRITE	3
#define FIND_ERROR_READ		4

void CGalaxyDlg::OnJoin(NMHDR* pNMHDR, LRESULT* pResult) 
{	
	STARTUPINFO s; 
	PROCESS_INFORMATION p;
	char cmdLine[512];
	char myCRXPath[MAX_PATH];
	FILE *file;

	strcpy (myCRXPath, CRXPath);
	memset (&s, 0, sizeof(s));
	s.cb = sizeof(s);
	myCRXPath[strlen(myCRXPath)-1] = '\0';

	sprintf (CRXbuff, "%s\\%s", myCRXPath, "alienarena.exe");

	//some checks in case their path got wonkered.
	file = NULL; //quiet compiler warning
	file = fopen (CRXbuff, "rb");
	if (!file) {

		if ( !BrowseForFolder( CRXPath, _T("Folder location of Alien Arena") )) 
		{
			AfxMessageBox("Unable to use this folder!");
		}
		else {
			strcpy (myCRXPath, CRXPath);
			memset (&s, 0, sizeof(s));
			s.cb = sizeof(s);
			myCRXPath[strlen(myCRXPath)-1] = '\0';
			sprintf (CRXbuff, "%s\\%s", myCRXPath, "alienarena.exe");
			file = fopen (CRXbuff, "rb");
			if (!file) {
				AfxMessageBox("Unable to locate alienarena.exe in this folder!");
			}
		}
	}

	sprintf (cmdLine, " +set game arena +set name %s +connect %s", user.nick, Server);
	CreateProcess (CRXbuff, cmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, myCRXPath, &s, &p);
	
	*pResult = 0;
}

void CGalaxyDlg::OnJoin2()  //used from the menu pulldown, doesn't return pointer val
{	
	STARTUPINFO s; 
	PROCESS_INFORMATION p;
	char cmdLine[512];
	char myCRXPath[MAX_PATH];
	FILE *file;

	strcpy (myCRXPath, CRXPath);
	memset (&s, 0, sizeof(s));
	s.cb = sizeof(s);
	myCRXPath[strlen(myCRXPath)-1] = '\0';

	sprintf (CRXbuff, "%s\\%s", myCRXPath, "alienarena.exe");

	//some checks in case their path got wonkered.
	file = NULL; //quiet compiler warning
	file = fopen (CRXbuff, "rb");
	if (!file) {

		if ( !BrowseForFolder( CRXPath, _T("Folder location of Alien Arena") )) 
		{
			AfxMessageBox("Unable to use this folder!");
		}
		else {
			strcpy (myCRXPath, CRXPath);
			memset (&s, 0, sizeof(s));
			s.cb = sizeof(s);
			myCRXPath[strlen(myCRXPath)-1] = '\0';
			sprintf (CRXbuff, "%s\\%s", myCRXPath, "alienarena.exe");
			file = fopen (CRXbuff, "rb");
			if (!file) {
				AfxMessageBox("Unable to locate crx.exe in this folder!");
			}
			else
				WritePrivateProfileString("Galaxy", "exe", myCRXPath, "galaxy.ini");
		}
	}

	
	sprintf (cmdLine, " +set game arena +set name %s +connect %s", user.nick, Server);
	CreateProcess (CRXbuff, cmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, myCRXPath, &s, &p);

}

void CGalaxyDlg::OnLaunch() 
{
	STARTUPINFO s; 
	PROCESS_INFORMATION p;
	char cmdLine[512];
	char myCRXPath[MAX_PATH];
	FILE *file;
	
	strcpy (myCRXPath, CRXPath);
	memset (&s, 0, sizeof(s));
	s.cb = sizeof(s);
	myCRXPath[strlen(myCRXPath)-1] = '\0';

	sprintf (CRXbuff, "%s\\%s", myCRXPath, "alienarena.exe");

	//some checks in case their path got wonkered.
	file = NULL; //quiet compiler warning
	file = fopen (CRXbuff, "rb");
	if (!file) {

		if ( !BrowseForFolder( CRXPath, _T("Folder location of Alien Arena") )) 
		{
			AfxMessageBox("Unable to use this folder!");
		}
		else {
			strcpy (myCRXPath, CRXPath);
			memset (&s, 0, sizeof(s));
			s.cb = sizeof(s);
			myCRXPath[strlen(myCRXPath)-1] = '\0';
			sprintf (CRXbuff, "%s\\%s", myCRXPath, "alienarena.exe");
			file = fopen (CRXbuff, "rb");
			if (!file) {
				AfxMessageBox("Unable to locate alienarena.exe in this folder!");
			}
			else
				WritePrivateProfileString("Galaxy", "exe", myCRXPath, "galaxy.ini");
		}
	}
	
	sprintf (cmdLine, " +set game arena +set name %s", user.nick);
	CreateProcess (CRXbuff, cmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, myCRXPath, &s, &p);

}

void CGalaxyDlg::OnButton6() 
{
	sprintf(mensaje,"QUIT\n\r");
	sockete.sendData(mensaje);
	m_status2.SetWindowText("Disconnected from chat channel...");
	canJoin = false;
	connectedToChannel = false;
	connectedToServer = false;
}

void CGalaxyDlg::Configure()
{
	PlayerProfile ppDlg;
	ppDlg.DoModal();
}

void CGalaxyDlg::HelpLinks()
{
	Help ppDlg;
	ppDlg.DoModal();
}

void CGalaxyDlg::About()
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();
}

void CGalaxyDlg::OnAddbuddy() 
{
	ofstream buddylist;
	int index;

	index = m_playerinfo.GetNextItem(-1,LVNI_SELECTED);
	
	strcpy(newBuddyName, players[index].orgname);

	BuddyName ppDlg;
		ppDlg.DoModal();
	
	buddylist.open("buddylist.db",  ios::app);
	if(newBuddyName[0])
		buddylist<<newBuddyName<<endl;
	buddylist.close();
	Check_Buddies();
}
void CGalaxyDlg::OnQuickAddbuddy() 
{
	int index;
	ofstream buddylist;

	index = m_playerinfo.GetNextItem(-1,LVNI_SELECTED);
	
	strcpy(newBuddyName, players[index].orgname);

	buddylist.open("buddylist.db",  ios::app);
	if(newBuddyName[0])
		buddylist<<newBuddyName<<endl;
	buddylist.close();
	Check_Buddies();
}
void CGalaxyDlg::OnDelbuddy() 
{
	ifstream buddylist;
	ofstream newlist;
	struct _buddies {
		char name[32];
		bool remove;
	} buddies[64];
	int i, j;

	//nuke array
	for(i=0; i<64; i++) {
		buddies[i].name[0] = 0;
		buddies[i].remove = false;
	}

	//read in list
	buddylist.open("buddylist.db");
	if(buddylist.good()) {
		i = 0;
		while(1) {
			buddylist.getline(buddies[i].name, 16);
			if(!buddies[i].name[0])
				break;
			if(!strcmp(buddies[i].name, currBuddyName))
				buddies[i].remove = true;
			i++;
			if(i>63)
				break;
		}
	}
	buddylist.close();
		
	//write out new file
	newlist.open("buddylist.db");
	for(j=0; j<i; j++) {
		if(!buddies[j].remove)
			newlist<<buddies[j].name<<endl;
	}
	newlist.close();
	Check_Buddies();
}

void CGalaxyDlg::OnSelchangeBuddylist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int index;

	index = m_buddylist.GetNextItem(-1,LVNI_SELECTED);
	strcpy(currBuddyName, m_buddylist.GetItemText(index, 0));

	*pResult = 0;
}

void CGalaxyDlg::OnPlayersort() 
{
	int i, k;
	int mostPlayers;
	LVITEM lvi;
	char txt[128];

	if(!refreshed)
		return;

	//destroy list
	m_serverinfo.DeleteAllItems();

	//nuke data array
	for(i = 0; i < 64; i++) {
		serverlist[i].name[0] = 0;
		serverlist[i].map[0] = 0;
		serverlist[i].ipstring[0] = 0;
		serverlist[i].real_pos = 0;
		serverlist[i].players = 0;
		serverlist[i].sorted = serverstring[i].sorted = false;
	}
	//sort the list by players
	int x = 0;
	while(1) {
		//find the next unsorted player value
		for(i=0; i<liveServers; i++) {
			if(!serverstring[i].sorted) {//got one
				mostPlayers = serverstring[i].players;
				break;
			}
		}
		for(i=0; i<liveServers; i++) { 
			
			if(!serverstring[i].sorted && serverstring[i].players >= mostPlayers) {
				mostPlayers = serverstring[i].players;
				k = i;
			}
		}
		//has the most players, put it in the array
		strcpy(serverlist[x].name, serverstring[k].name);
		strcpy(serverlist[x].map, serverstring[k].map);
		strcpy(serverlist[x].ipstring, serverstring[k].ipstring);
		serverlist[x].ping = serverstring[k].ping;
		serverlist[x].real_pos = serverstring[k].real_pos;
		serverlist[x].players = serverstring[k].players;
		serverstring[k].sorted = serverlist[x].sorted = true; //it was the lowest, it's inserted now
		x++;
		if(x>63 || x>=liveServers) //we have sorted all servers and don't overrun array size
			break;
	}

	//add the sorted list into the window 
	m_serverinfo.SetRedraw(FALSE);
	for(i=0; i<x; i++)
	{
			
		lvi.mask =  LVIF_IMAGE | LVIF_TEXT;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.pszText = (LPTSTR)(LPCTSTR)(serverlist[i].name);
		if(!strcmp("noname", serverlist[i].name))
			lvi.iImage = 5;
		else
			lvi.iImage = 2;	
		m_serverinfo.InsertItem(&lvi);

		m_serverinfo.SetItemText(i, 1, serverlist[i].ipstring);
		m_serverinfo.SetItemText(i, 2, serverlist[i].map);
		sprintf(txt, "%i", serverlist[i].players);
		m_serverinfo.SetItemText(i, 3, txt);
		sprintf(txt, "%i", serverlist[i].ping);
		m_serverinfo.SetItemText(i, 4, txt);
	}

	m_serverinfo.SetRedraw(TRUE);

}

void CGalaxyDlg::OnPingsort() 
{

	int i, k;
	int lowestPing;
	LVITEM lvi;
	char txt[128];

	if(!refreshed)
		return;

	//destroy list
	m_serverinfo.DeleteAllItems();

	//nuke data array
	for(i = 0; i < 64; i++) {
		serverlist[i].name[0] = 0;
		serverlist[i].map[0] = 0;
		serverlist[i].ipstring[0] = 0;
		serverlist[i].real_pos = 0;
		serverlist[i].players = 0;
		serverlist[i].sorted = serverstring[i].sorted = false;
	}
	//sort the list, by ping, or by players
	int x = 0;
	while(1) {
		//find the next unsorted ping value
		for(i=0; i<liveServers; i++) {
			if(!serverstring[i].sorted) {//got one
				lowestPing = serverstring[i].ping;
				break;
			}
		}
		for(i=0; i<liveServers; i++) { 
			
			if(!serverstring[i].sorted && serverstring[i].ping <= lowestPing) {
				lowestPing = serverstring[i].ping;
				k = i;
			}
		}
		//this is the lowest ping, put it in the array
		strcpy(serverlist[x].name, serverstring[k].name);
		strcpy(serverlist[x].map, serverstring[k].map);
		strcpy(serverlist[x].ipstring, serverstring[k].ipstring);
		serverlist[x].players = serverstring[k].players;
		serverlist[x].ping = serverstring[k].ping;
		serverlist[x].real_pos = serverstring[k].real_pos;
		serverstring[k].sorted = serverlist[x].sorted = true; //it was the lowest, it's inserted now
		x++;
		if(x>63 || x>=liveServers) //we have sorted all servers and don't overrun array size
			break;
	}

	//add the sorted list into the window 
	m_serverinfo.SetRedraw(FALSE);
	for(i=0; i<x; i++)
	{
			
		lvi.mask =  LVIF_IMAGE | LVIF_TEXT;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.pszText = (LPTSTR)(LPCTSTR)(serverlist[i].name);
		if(!strcmp("noname", serverlist[i].name))
			lvi.iImage = 5;
		else
			lvi.iImage = 2;	
		m_serverinfo.InsertItem(&lvi);

		m_serverinfo.SetItemText(i, 1, serverlist[i].ipstring);
		m_serverinfo.SetItemText(i, 2, serverlist[i].map);
		sprintf(txt, "%i", serverlist[i].players);
		m_serverinfo.SetItemText(i, 3, txt);
		sprintf(txt, "%i", serverlist[i].ping);
		m_serverinfo.SetItemText(i, 4, txt);
	}

	m_serverinfo.SetRedraw(TRUE);
}

void CGalaxyDlg::LookUpStats() 
{

	ifstream statslog;
	ofstream newlog;
	HINTERNET hINet, hFile;
	int foundplayer, rank;
	double real_points;
	double real_time;
	char name[32], remote_ip[32], points[32], frags[32], totalfrags[32], time[32], totaltime[32], ip[32], poll[32];
	CHAR buffer[1024];
	
	//get a copy of the latest database
	hINet = InternetOpen("InetURL/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( !hINet )
	{
		AfxMessageBox("Unable to connect to database at this time...");
		return;
	}
	hFile = InternetOpenUrl( hINet, "https://martianbackup.com/playerrank.db", NULL, 0, 0, 0 );
	if(hFile) {
		DWORD dwRead;
		newlog.open("stats.db");
		while ( InternetReadFile( hFile, buffer, 1023, &dwRead ) )
		{
			if ( dwRead == 0 )
				break;	
			buffer[dwRead] = 0;
			newlog << buffer;
		}
		newlog.close();
		InternetCloseHandle( hFile );
	}
	else {
		InternetCloseHandle(hINet);
		AfxMessageBox("Unable to read database...");
		return;
	}
	InternetCloseHandle( hINet );

	//we've got it, lets parse it and find the player's stats.
	statslog.open("stats.db");
	rank = 1;
	name[0] = 0;
	foundplayer = false;
	strcpy(name, "go");
	if(statslog.good()) {
		while(strlen(name)) {
			statslog.getline(name, 32); //name
			statslog.getline(remote_ip, 32); //remote ip
			statslog.getline(points, 32); //points
			statslog.getline(frags, 32); //frags
			statslog.getline(totalfrags, 32); //total frags
			statslog.getline(time, 32); //current time in poll
			statslog.getline(totaltime, 32);
			real_time = atof(totaltime);
			real_points = atof(points) * 100/real_time;
			statslog.getline(ip, 32); //last server.ip
			statslog.getline(poll, 32); //what poll was our last?
			if(!strcmp(name, user.nick)) {
				foundplayer = true;
				break; //get out we are done
			}
			rank++;
		}
	}
	else
		AfxMessageBox("Unable to parse database...");

	statslog.close();
	remove("stats.db");
	if(foundplayer) {
		//display stats in a messagebox
		sprintf(buffer, "%s Rank: %i Points: %4.2f Frags: %s Time: %4.2f hrs", name, rank, real_points, 
			totalfrags, real_time);
		AfxMessageBox(buffer);
	}
	else {
		AfxMessageBox("Player name not found in stats database!");
	}

}
void CGalaxyDlg::LookUpPlayerStats(NMHDR* pNMHDR, LRESULT* pResult) 
{

	ifstream statslog;
	ofstream newlog;
	HINTERNET hINet, hFile;
	int foundplayer, rank;
	double real_points;
	double real_time;
	char name[32], remote_ip[32], points[32], frags[32], totalfrags[32], time[32], totaltime[32], ip[32], poll[32];
	CHAR buffer[1024];
	int index;

	index = m_playerinfo.GetNextItem(-1,LVNI_SELECTED);
	
	//get a copy of the latest database
	hINet = InternetOpen("InetURL/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( !hINet )
	{
		AfxMessageBox("Unable to connect to database at this time...");
		return;
	}
	hFile = InternetOpenUrl( hINet, "https://martianbackup.com/playerrank.db", NULL, 0, 0, 0 );
	if(hFile) {
		DWORD dwRead;
		newlog.open("stats.db");
		while ( InternetReadFile( hFile, buffer, 1023, &dwRead ) )
		{
			if ( dwRead == 0 )
				break;	
			buffer[dwRead] = 0;
			newlog << buffer;
		}
		newlog.close();
		InternetCloseHandle( hFile );
	}
	else {
		InternetCloseHandle(hINet);
		AfxMessageBox("Unable to read database...");
		return;
	}
	InternetCloseHandle( hINet );

	//we've got it, lets parse it and find the player's stats.
	statslog.open("stats.db");
	rank = 1;
	name[0] = 0;
	foundplayer = false;
	strcpy(name, "go");
	if(statslog.good()) {
		while(strlen(name)) {
			statslog.getline(name, 32); //name
			statslog.getline(remote_ip, 32); //remote ip
			statslog.getline(points, 32); //points
			statslog.getline(frags, 32); //frags
			statslog.getline(totalfrags, 32); //total frags
			statslog.getline(time, 32); //current time in poll
			statslog.getline(totaltime, 32);
			real_time = atof(totaltime);
			real_points = atof(points) * 100/real_time;
			statslog.getline(ip, 32); //last server.ip
			statslog.getline(poll, 32); //what poll was our last?
			if(!strcmp(name, players[index].orgname)) {
					foundplayer = true;
					break; //done
			}
			rank++;
		}
	}
	else
		AfxMessageBox("Unable to parse database...");

	statslog.close();
	remove("stats.db");
	//change this to bring up a dialog that will allow the addition of this player to your buddy list

	if(foundplayer) {
		//display stats in a messagebox
		sprintf(buffer, "%s Rank: %i Points: %4.2f Frags: %s Time: %4.2f hrs", name, rank, real_points, 
			totalfrags, real_time);
		AfxMessageBox(buffer);
	}
	else {
		sprintf(buffer, "%s not found in stats database!", players[index].orgname);
		AfxMessageBox(buffer);
	}
	*pResult = 0;
}
void CGalaxyDlg::GetNews() 
{

	ifstream news;
	ofstream newslocal;
	HINTERNET hINet, hFile;
	char data[1024];
	CHAR buffer[1024];

	//get a copy of the latest news
	hINet = InternetOpen("InetURL/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( !hINet )
	{
		m_news.SetWindowText("Unable to connect to news feed at this time...");
		return;
	}
	hFile = InternetOpenUrl( hINet, "http://red.planetarena.org/news.db", NULL, 0, 0, 0 );
	if(hFile) {
		DWORD dwRead;
		newslocal.open("news.db");
		while ( InternetReadFile( hFile, buffer, 1023, &dwRead ) )
		{
			if ( dwRead == 0 )
				break;	
			buffer[dwRead] = 0;
			newslocal << buffer;
		}
		newslocal.close();
		InternetCloseHandle( hFile );
	}
	else {
		InternetCloseHandle(hINet);
		m_news.SetWindowText("Unable to connect to news feed at this time...");
		return;
	}
	InternetCloseHandle( hINet );

	news.open("news.db");
	if(news.good()) {
		news.getline(data, 1024);
		m_news.SetWindowText(data);
	}
	news.close();
	remove("news.db");
}

void IRCDisconnect() 
{
	sprintf(mensaje,"QUIT\n\r");
	sockete.sendData(mensaje);
}