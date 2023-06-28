/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys_win.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qcommon/qcommon.h"
#include "winquake.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>

#include "conproc.h"

#define MINIMUM_WIN_MEMORY	0x0a00000
#define MAXIMUM_WIN_MEMORY	0x1000000

//#define DEMO

qboolean s_win95;

int			ActiveApp;
qboolean	Minimized;

static HANDLE		hinput, houtput;

unsigned	sys_msg_time;
unsigned	sys_frame_time;

// attachment to statically linked game library
extern void *GetGameAPI ( void *import);

extern void Q_strncpyz( char *dest, const char *src, size_t size );

extern int window_center_x, window_center_y;
extern qboolean mouse_available;
extern int mouse_diff_x;
extern int mouse_diff_y;

static HANDLE		qwclsemaphore;

#define	MAX_NUM_ARGVS	128
int			argc;
char		*argv[MAX_NUM_ARGVS];

#define CONSOLE_WINDOW_CLASS_NAME	"CRX Console"
#define CONSOLE_WINDOW_NAME			"Alien Arena Console"
#define CONSOLE_WINDOW_STYLE		(WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN|WS_GROUP)

#define MAX_OUTPUT					32768
#define MAX_INPUT					256
#define	MAX_PRINTMSG				8192

typedef struct {
	int			outLen;					// To keep track of output buffer len
	char		cmdBuffer[MAX_INPUT];	// Buffered input from dedicated console
	qboolean	timerActive;			// Timer is active (for fatal errors)
	qboolean	flashColor;				// If true, flash error message to red

	// Window stuff
	HWND		hWnd;
	HWND		hWndCopy;
	HWND		hWndClear;
	HWND		hWndQuit;
	HWND		hWndOutput;
	HWND		hWndInput;
	HWND		hWndMsg;
	HFONT		hFont;
	HFONT		hFontBold;
	HBRUSH		hBrushMsg;
	HBRUSH		hBrushOutput;
	HBRUSH		hBrushInput;
	WNDPROC		defOutputProc;
	WNDPROC		defInputProc;
} sysConsole_t;

static sysConsole_t	sys_console;
void Sys_ShowConsole (qboolean show);
HINSTANCE			sys_hInstance;

unsigned			sys_msgTime;
unsigned			sys_frameTime;

/*
 =======================================================================

 DEDICATED CONSOLE

 =======================================================================
*/


/*
 =================
 Sys_GetCommand
 =================
*/
char *Sys_GetCommand (void){

	static char buffer[MAX_INPUT];

	if (!sys_console.cmdBuffer[0])
		return NULL;

	Q_strncpyz(buffer, sys_console.cmdBuffer, sizeof(buffer));
	sys_console.cmdBuffer[0] = 0;

	return buffer;
}

/*
 =================
 Sys_Print
 =================
*/
void Sys_Print (const char *text){

	char	buffer[MAX_PRINTMSG];
	int		len = 0;

	//FIXME: add a check for text == NULL and print an error message
	//without potentially creating an infinite recursion loop
	
	// Change \n to \r\n so it displays properly in the edit box and
	// remove color escapes
	while (*text){
		if (*text == '\n'){
			buffer[len++] = '\r';
			buffer[len++] = '\n';
		}
		else if (Q_IsColorString(text))
			text++;
		else
			buffer[len++] = *text;

		text++;
	}
	buffer[len] = 0;

	sys_console.outLen += len;
	if (sys_console.outLen >= MAX_OUTPUT){
		SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
		sys_console.outLen = len;
	}
	SendMessage(sys_console.hWndOutput, EM_REPLACESEL, FALSE, (LPARAM)buffer);

	// Scroll down
	SendMessage(sys_console.hWndOutput, EM_LINESCROLL, 0, 0xFFFF);
	SendMessage(sys_console.hWndOutput, EM_SCROLLCARET, 0, 0);
}

void MessageBoxForce (char *name, char *msg)
{
	MessageBox(NULL, name, name, 0 );
}

void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	CL_Shutdown ();
	Qcommon_Shutdown ();

	va_start (argptr, error);
	vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	MessageBox(NULL, text, "Error", 0 /* MB_OK */ );

	if (qwclsemaphore)
		CloseHandle (qwclsemaphore);

// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (1);
}

/*
 =================
 Sys_ShowConsole
 =================
*/
void Sys_ShowConsole (qboolean show){

	if (!show){
		ShowWindow(sys_console.hWnd, SW_HIDE);
		return;
	}

	ShowWindow(sys_console.hWnd, SW_SHOW);
	UpdateWindow(sys_console.hWnd);
	SetForegroundWindow(sys_console.hWnd);
	SetFocus(sys_console.hWnd);

	// Set the focus to the input edit box if possible
	SetFocus(sys_console.hWndInput);

	// Scroll down
	SendMessage(sys_console.hWndOutput, EM_LINESCROLL, 0, 0xFFFF);
	SendMessage(sys_console.hWndOutput, EM_SCROLLCARET, 0, 0);
}

/*
 =================
 Sys_ConsoleProc
 =================
*/
static LONG WINAPI Sys_ConsoleProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	switch (uMsg){
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE){
			SetFocus(sys_console.hWndInput);
			return 0;
		}

		break;
	case WM_CLOSE:
		Sys_Quit();

		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED){
			if ((HWND)lParam == sys_console.hWndCopy){
				SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
				SendMessage(sys_console.hWndOutput, WM_COPY, 0, 0);
			}
			else if ((HWND)lParam == sys_console.hWndClear){
				SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
				SendMessage(sys_console.hWndOutput, WM_CLEAR, 0, 0);
			}
			else if ((HWND)lParam == sys_console.hWndQuit)
				Sys_Quit();
		}
		else if (HIWORD(wParam) == EN_VSCROLL)
			InvalidateRect(sys_console.hWndOutput, NULL, TRUE);

		break;
	case WM_CTLCOLOREDIT:
		if ((HWND)lParam == sys_console.hWndOutput){
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetBkColor((HDC)wParam, RGB(54, 66, 83));
			SetTextColor((HDC)wParam, RGB(255, 255, 255));
			return (LONG)sys_console.hBrushOutput;
		}
		else if ((HWND)lParam == sys_console.hWndInput){
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetBkColor((HDC)wParam, RGB(255, 255, 255));
			SetTextColor((HDC)wParam, RGB(0, 0, 0));
			return (LONG)sys_console.hBrushInput;
		}

		break;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == sys_console.hWndMsg){
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetBkColor((HDC)wParam, RGB(127, 127, 127));

			if (sys_console.flashColor)
				SetTextColor((HDC)wParam, RGB(255, 0, 0));
			else
				SetTextColor((HDC)wParam, RGB(0, 0, 0));

			return (LONG)sys_console.hBrushMsg;
		}

		break;
	case WM_TIMER:
		sys_console.flashColor = !sys_console.flashColor;
		InvalidateRect(sys_console.hWndMsg, NULL, TRUE);

		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 =================
 Sys_ConsoleEditProc
 =================
*/
static LONG WINAPI Sys_ConsoleEditProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	switch (uMsg){
	case WM_CHAR:
		if (hWnd == sys_console.hWndInput){
			if (wParam == VK_RETURN){
				if (GetWindowText(sys_console.hWndInput, sys_console.cmdBuffer, sizeof(sys_console.cmdBuffer))){
					SetWindowText(sys_console.hWndInput, "");

					Com_Printf("]%s\n", sys_console.cmdBuffer);
				}

				return 0;	// Keep it from beeping
			}
		}
		else if (hWnd == sys_console.hWndOutput)
			return 0;	// Read only

		break;
	case WM_VSCROLL:
		if (LOWORD(wParam) == SB_THUMBTRACK)
			return 0;

		break;
	}

	if (hWnd == sys_console.hWndOutput)
		return CallWindowProc(sys_console.defOutputProc, hWnd, uMsg, wParam, lParam);
	else if (hWnd == sys_console.hWndInput)
		return CallWindowProc(sys_console.defInputProc, hWnd, uMsg, wParam, lParam);

	return 0;
}

/*
 =================
 Sys_ShutdownConsole
 =================
*/
static void Sys_ShutdownConsole (void){

	if (sys_console.timerActive)
		KillTimer(sys_console.hWnd, 1);

	if (sys_console.hBrushMsg)
		DeleteObject(sys_console.hBrushMsg);
	if (sys_console.hBrushOutput)
		DeleteObject(sys_console.hBrushOutput);
	if (sys_console.hBrushInput)
		DeleteObject(sys_console.hBrushInput);

	if (sys_console.hFont)
		DeleteObject(sys_console.hFont);
	if (sys_console.hFontBold)
		DeleteObject(sys_console.hFontBold);

	if (sys_console.defOutputProc)
		SetWindowLong(sys_console.hWndOutput, GWL_WNDPROC, (LONG)sys_console.defOutputProc);
	if (sys_console.defInputProc)
		SetWindowLong(sys_console.hWndInput, GWL_WNDPROC, (LONG)sys_console.defInputProc);

	ShowWindow(sys_console.hWnd, SW_HIDE);
	DestroyWindow(sys_console.hWnd);
	UnregisterClass(CONSOLE_WINDOW_CLASS_NAME, sys_hInstance);
}

/*
 =================
 Sys_InitConsole
 =================
*/
static void Sys_InitConsole (void){

	WNDCLASSEX	wc;
	HDC			hDC;
	RECT		r;
	int			x, y, w, h;

	// Center the window in the desktop
	hDC = GetDC(0);
	w = GetDeviceCaps(hDC, HORZRES);
	h = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(0, hDC);

	r.left = (w - 540) / 2;
	r.top = (h - 455) / 2;
	r.right = r.left + 540;
	r.bottom = r.top + 455;

	AdjustWindowRect(&r, CONSOLE_WINDOW_STYLE, FALSE);

	x = r.left;
	y = r.top;
	w = r.right - r.left;
	h = r.bottom - r.top;

	wc.style			= 0;
	wc.lpfnWndProc		= (WNDPROC)Sys_ConsoleProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= sys_hInstance;
	wc.hIcon			= 0;//LoadIcon(sys_hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hIconSm			= 0;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)COLOR_WINDOW;
	wc.lpszMenuName		= 0;
	wc.lpszClassName	= CONSOLE_WINDOW_CLASS_NAME;
	wc.cbSize			= sizeof(WNDCLASSEX);

	if (!RegisterClassEx(&wc)){
		MessageBox(NULL, "Could not register console window class", "ERROR", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		exit(0);
	}

	sys_console.hWnd = CreateWindowEx(0, CONSOLE_WINDOW_CLASS_NAME, CONSOLE_WINDOW_NAME, CONSOLE_WINDOW_STYLE, x, y, w, h, NULL, NULL, sys_hInstance, NULL);
	if (!sys_console.hWnd){
		UnregisterClass(CONSOLE_WINDOW_CLASS_NAME, sys_hInstance);

		MessageBox(NULL, "Could not create console window", "ERROR", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		exit(0);
	}

	sys_console.hWndMsg = CreateWindowEx(0, "STATIC", "", WS_CHILD | SS_SUNKEN, 5, 5, 530, 30, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndOutput = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE, 5, 40, 530, 350, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndInput = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 5, 395, 530, 20, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndCopy = CreateWindowEx(0, "BUTTON", "copy", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 5, 425, 70, 25, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndClear = CreateWindowEx(0, "BUTTON", "clear", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 80, 425, 70, 25, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndQuit = CreateWindowEx(0, "BUTTON", "quit", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 465, 425, 70, 25, sys_console.hWnd, NULL, sys_hInstance, NULL);

	// Create and set fonts
	sys_console.hFont = CreateFont(14, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
	sys_console.hFontBold = CreateFont(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "System");

	SendMessage(sys_console.hWndMsg, WM_SETFONT, (WPARAM)sys_console.hFont, FALSE);
	SendMessage(sys_console.hWndOutput, WM_SETFONT, (WPARAM)sys_console.hFont, FALSE);
	SendMessage(sys_console.hWndInput, WM_SETFONT, (WPARAM)sys_console.hFont, FALSE);
	SendMessage(sys_console.hWndCopy, WM_SETFONT, (WPARAM)sys_console.hFontBold, FALSE);
	SendMessage(sys_console.hWndClear, WM_SETFONT, (WPARAM)sys_console.hFontBold, FALSE);
	SendMessage(sys_console.hWndQuit, WM_SETFONT, (WPARAM)sys_console.hFontBold, FALSE);

	// Create brushes
	sys_console.hBrushMsg = CreateSolidBrush(RGB(127, 127, 127));
	sys_console.hBrushOutput = CreateSolidBrush(RGB(54, 66, 83));
	sys_console.hBrushInput = CreateSolidBrush(RGB(255, 255, 255));

	// Subclass edit boxes
	sys_console.defOutputProc = (WNDPROC)SetWindowLong(sys_console.hWndOutput, GWL_WNDPROC, (LONG)Sys_ConsoleEditProc);
	sys_console.defInputProc = (WNDPROC)SetWindowLong(sys_console.hWndInput, GWL_WNDPROC, (LONG)Sys_ConsoleEditProc);

	// Set text limit for input edit box
	SendMessage(sys_console.hWndInput, EM_SETLIMITTEXT, (WPARAM)(MAX_INPUT-1), 0);

	// Show it
	Sys_ShowConsole(true);
}


// =====================================================================


/*
===============================================================================

SYSTEM IO

===============================================================================
*/
/*
 =================
 Sys_GetClipboardText
 =================
*/
char *Sys_GetClipboardText (void){

	HANDLE	hClipboardData;
	LPVOID	lpData;
	DWORD	dwSize;
	char	*text;

	if (!OpenClipboard(NULL))
		return NULL;

	hClipboardData = GetClipboardData(CF_TEXT);
	if (!hClipboardData){
		CloseClipboard();
		return NULL;
	}

	lpData = GlobalLock(hClipboardData);
	if (!lpData){
		CloseClipboard();
		return NULL;
	}

	dwSize = GlobalSize(hClipboardData);

	text = Z_Malloc(dwSize+1);
	memcpy(text, lpData, dwSize);

	GlobalUnlock(hClipboardData);
	CloseClipboard();

	return text;
}

/*
 =================
 Sys_PumpMessages

 Pump window messages
 =================
*/
void Sys_PumpMessages (void){

    MSG		msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
		if (!GetMessage(&msg, NULL, 0, 0))
			Sys_Quit();

		sys_msgTime = msg.time;

      	TranslateMessage(&msg);
      	DispatchMessage(&msg);
	}

	// Grab frame time
	sys_frameTime = timeGetTime();	// FIXME: should this be at start?
}

void Sys_Quit (void)
{
	timeEndPeriod( 1 );

	CL_Shutdown();
	Qcommon_Shutdown ();
	CloseHandle (qwclsemaphore);
	if (dedicated && dedicated->value)
		FreeConsole ();

// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (0);
}


void WinError (void)
{
	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);

	// Display the string.
	MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
}

/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	OSVERSIONINFO	vinfo;

#if 0
	// allocate a named semaphore on the client so the
	// front end can tell if it is alive

	// mutex will fail if semephore already exists
    qwclsemaphore = CreateMutex(
        NULL,         /* Security attributes */
        0,            /* owner       */
        "qwcl"); /* Semaphore name      */
	if (!qwclsemaphore)
		Sys_Error ("QWCL is already running on this system");
	CloseHandle (qwclsemaphore);

    qwclsemaphore = CreateSemaphore(
        NULL,         /* Security attributes */
        0,            /* Initial count       */
        1,            /* Maximum count       */
        "qwcl"); /* Semaphore name      */
#endif

	timeBeginPeriod( 1 );
	Cvar_Get("sys_hInstance", va("%i", sys_hInstance), CVAR_ROM);
	Cvar_Get("sys_wndProc", va("%i", MainWndProc), CVAR_ROM);

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if (vinfo.dwMajorVersion < 4)
		Sys_Error ("Alien Arena requires windows version 4 or greater");
	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("Alien Arena doesn't run on Win32s");
	else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
		s_win95 = true;

	}


static char	console_text[256];
static int	console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	INPUT_RECORD	recs[1024];
	int		dummy;
	int		ch, numread, numevents;

	if (!dedicated || !dedicated->value)
		return NULL;


	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);

						if (console_textlen)
						{
							console_text[console_textlen] = 0;
							console_textlen = 0;
							return console_text;
						}
						break;

					case '\b':
						if (console_textlen)
						{
							console_textlen--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);
						}
						break;

					default:
						if (ch >= ' ')
						{
							if (console_textlen < sizeof(console_text)-2)
							{
								WriteFile(houtput, &ch, 1, &dummy, NULL);
								console_text[console_textlen] = ch;
								console_textlen++;
							}
						}

						break;

				}
			}
		}
	}

	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (char *string)
{
	int		dummy;
	char	text[256];

	if (!dedicated || !dedicated->value)
		return;

	if (console_textlen)
	{
		text[0] = '\r';
		memset(&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile(houtput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile(houtput, string, strlen(string), &dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
	MSG        msg;
	POINT      current_pos;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();
		sys_msg_time = msg.time;
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	if ( mouse_available && GetCursorPos( &current_pos) ) {
		mouse_diff_x += current_pos.x - window_center_x;
		mouse_diff_y += current_pos.y - window_center_y;
		if ( mouse_diff_x || mouse_diff_y ) {
			SetCursorPos( window_center_x, window_center_y );
		}
	}

	// grab frame time
	sys_frame_time = timeGetTime();	// FIXME: should this be at start?
}



/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 )
	{
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 )
		{
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 )
			{
				data = malloc( GlobalSize( hClipboardData ) + 1 );
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{
	ShowWindow ( cl_hwnd, SW_RESTORE);
	SetForegroundWindow ( cl_hwnd );
}

/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE	game_library = NULL;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if ( game_library != NULL )
	{
	if (!FreeLibrary (game_library))
		Com_Error (ERR_FATAL, "FreeLibrary failed for game library");
	}
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game module

2010-08 : Implements a statically linked game module. Loading a game module DLL
  is supported if it exists.
  To prevent problems with attempting to load an older, incompatible version,
    a DLL will not be loaded from arena/ nor data1/

=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*ptrGetGameAPI) (void *) = NULL;
	char	name[MAX_OSPATH];
	char	*path;
	size_t pathlen;
//	char	cwd[MAX_OSPATH];

	const char *gamename = "gamex86.dll";

/*
#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

	if (game_library != NULL)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	// check the current debug directory first for development purposes
	_getcwd (cwd, sizeof(cwd));
	Com_sprintf (name, sizeof(name), "%s/%s/%s", cwd, debugdir, gamename);
	game_library = LoadLibrary ( name );
	if (game_library != NULL )
	{
		Com_DPrintf ("LoadLibrary (%s)\n", name);
	}
	else
	{
#ifdef DEBUG
		// check the current directory for other development purposes
		Com_sprintf (name, sizeof(name), "%s/%s", cwd, gamename);
		game_library = LoadLibrary ( name );
		if (game_library)
		{
			Com_DPrintf ("LoadLibrary (%s)\n", name);
		}
		else
#endif
		{
			// now run through the search paths

			path = NULL;
			while (1)
			{
				path = FS_NextPath (path);
				if (!path)
					break; // Search did not turn up a game DLL

				Com_sprintf (name, sizeof(name), "%s/%s", path, gamename);
				game_library = LoadLibrary (name);
				if (game_library)
				{
					Com_DPrintf ("LoadLibrary (%s)\n",name);
					break;
				}
			}
		}
	}
*/

// 2010-08

	if (game_library != NULL)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	path = NULL;
	for (;;)
	{
		path = FS_NextPath( path );
		if ( !path )
			break;

		pathlen = strlen( path );
		// old game DLL in data1 is a problem
		if ( !Q_strncasecmp( "data1", &path[ pathlen-5 ], 5 ) )
			continue;
		// may want to have a game DLL in arena, but disable for now
		if ( !Q_strncasecmp( "arena", &path[ pathlen-5 ], 5 ) )
			continue;

		Com_sprintf (name, sizeof(name), "%s/%s", path, gamename);
		game_library = LoadLibrary (name);
		if (game_library)
		{ // found a game module DLL
			break;
		}
	}

	if ( game_library != NULL )
	{ // game module from DLL
		Com_Printf ("LoadLibrary (%s)\n",name);
		ptrGetGameAPI = (void *)GetProcAddress (game_library, "GetGameAPI");
	}

	/*
	 * No game DLL found, use statically linked game
	 */
	if ( ptrGetGameAPI == NULL )
	{
		ptrGetGameAPI = &GetGameAPI;
	}

	if ( ptrGetGameAPI == NULL )
	{ // program error
		Sys_UnloadGame ();
		return NULL;
	}

	return ptrGetGameAPI (parms);
}

//=======================================================================


/*
==================
ParseCommandLine

==================
*/
void ParseCommandLine (LPSTR lpCmdLine)
{
	argc = 1;
	argv[0] = "exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}

}

/*
==================
WinMain

==================
*/
HINSTANCE	global_hInstance;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	int				time, oldtime, newtime;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

	global_hInstance = hInstance;

	ParseCommandLine (lpCmdLine);

	// Initialize the dedicated console
	Sys_InitConsole();

	Qcommon_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

    /* main window message loop */
	while (1)
	{
		// if at a full screen console, don't update unless needed
		if (Minimized || (dedicated && dedicated->value) )
		{
			Sleep (5);
		}

		Sys_PumpMessages();

		do {
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} while (time < 1);
		// curtime setting moved from Sys_Milliseconds()
		//   so it consistent for entire frame
		curtime = newtime;

		_controlfp(_PC_24, _MCW_PC);

		Qcommon_Frame(time);

		oldtime = newtime;
	}

	// never gets here
    return TRUE;
}
