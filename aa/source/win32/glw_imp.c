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
/*
** GLW_IMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <windows.h>
#include <winuser.h>
#include "ref_gl/r_local.h"
#include "glw_win.h"
#include "winquake.h"
#include "resource.h"
#include "ref_gl/wglext.h"

static qboolean GLimp_SwitchFullscreen( int width, int height );
qboolean GLimp_InitGL ( qboolean );
void DestroyWindowGL ( glwstate_t* );
void GetARBPixelFormat( PIXELFORMATDESCRIPTOR );

char *	( WINAPI * wglGetExtensionsStringARB )( HDC );
int		( WINAPI * wglChoosePixelFormatARB )( HDC, CONST int *, CONST FLOAT *, UINT, int *, UINT * );

glwstate_t glw_state;

extern cvar_t *vid_xpos;
extern cvar_t *vid_ypos;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;
extern cvar_t *vid_displayfrequency;
extern cvar_t *r_antialiasing;
extern cvar_t *vid_width;
extern cvar_t *vid_height;
int pixelformatARB;

qboolean have_stencil = false; // Stencil shadows - MrG

static qboolean VerifyDriver( void )
{
	char buffer[1024];

	strcpy( buffer, qglGetString( GL_RENDERER ) );
	_strlwr( buffer );
	if ( strcmp( buffer, "gdi generic" ) == 0 )
		if ( !glw_state.mcd_accelerated )
			return false;
	return true;
}

/*
** VID_CreateWindow
*/
#define	WINDOW_CLASS_NAME	"Quake 2"


qboolean VID_CreateWindow(int x, int y, int width, int height, windowmode_t windowmode )
{
	WNDCLASS		wc;
	RECT			r;
	int				stylebits;
	int				exstyle;
	HMONITOR		monitor;
	MONITORINFO		info;
	int				workAreaWidth, workAreaHeight;
	int				showWindowMode = SW_SHOW;

	/* Register the frame class */
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)glw_state.wndproc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = glw_state.hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (void *)COLOR_GRAYTEXT;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClass (&wc) )
		Com_Error (ERR_FATAL, "Couldn't register window class");

	switch (windowmode)
	{
		case windowmode_exclusive_fullscreen:
			exstyle = WS_EX_TOPMOST;
			stylebits = FULLSCREEN_STYLE;
			break;
		case windowmode_windowed:
			exstyle = 0;
			stylebits = WINDOW_STYLE;
			break;
		case windowmode_borderless_windowed:
			exstyle = 0;
			stylebits = FULLSCREEN_STYLE;
			break;
	}

	r.left = 0;
	r.top = 0;
	r.right  = width;
	r.bottom = height;

	AdjustWindowRect (&r, stylebits, (windowmode == windowmode_windowed));

	if (windowmode == windowmode_exclusive_fullscreen || windowmode == windowmode_borderless_windowed)
		x = y = 0;
	
	pixelformatARB = 0;
	if ( r_antialiasing->integer ) {
		// We have to create a temporary window here, with a temporary context,
		// else the function pointer to wglChoosePixelFormatARB can't be found or it will not work.
		// And if we don't destroy it again afterwards it will crash.
		
		glw_state.hWnd = CreateWindowEx (
			 exstyle,
			 WINDOW_CLASS_NAME,
			 "CRX",
			 stylebits,
			 x, y, width, height,
			 NULL,
			 NULL,
			 glw_state.hInstance,
			 NULL );

		if (!glw_state.hWnd)
			Com_Error (ERR_FATAL, "Couldn't create window");

		// Only get the var pixelformatARB and return
		GLimp_InitGL( true ) ;

		DestroyWindowGL( &glw_state );
	}
	
	glw_state.hWnd = CreateWindowEx (
		 exstyle,
		 WINDOW_CLASS_NAME,
		 "CRX",
		 stylebits,
		 x, y, width, height,
		 NULL,
		 NULL,
		 glw_state.hInstance,
		 NULL );

	if (!glw_state.hWnd)
		Com_Error (ERR_FATAL, "Couldn't create window");

	// Check monitor work area width and height to see if we have to maximize the window or not. 
	// If the window width or height doesn't fit, it will be maximized.
	if (windowmode == windowmode_windowed)
	{
		info.cbSize = sizeof(MONITORINFO);
		monitor = MonitorFromWindow (glw_state.hWnd, MONITOR_DEFAULTTOPRIMARY);				

		GetMonitorInfo (monitor, &info);

		workAreaWidth = info.rcWork.right - info.rcWork.left;
		workAreaHeight = info.rcWork.bottom - info.rcWork.top;
			
		if (width >= workAreaWidth || height >= workAreaHeight)
			showWindowMode = SW_SHOWMAXIMIZED;
	}
	else
	{
		// Ignore scaling
		SetProcessDPIAware();
	}

	ShowWindow( glw_state.hWnd, showWindowMode );
	UpdateWindow( glw_state.hWnd );

	// init all the gl stuff for the window
	if (!GLimp_InitGL( false ))
	{
		Com_Printf ("VID_CreateWindow() - GLimp_InitGL failed\n");
		return false;
	}

	SetForegroundWindow( glw_state.hWnd );
	SetFocus( glw_state.hWnd );

	// let the sound and input subsystems know about the new window
	VID_NewWindow (width, height);
	VID_NewPosition (x, y);

	return true;
}

/*
** GLimp_SetMode
*/
rserr_t GLimp_SetMode (unsigned *pwidth, unsigned *pheight, int mode, windowmode_t windowmode)
{
	int x, y, width, height, max_width, max_height;
	const char *win_fs[] = {"Windowed", "Borderless Windowed", "Exclusive Fullscreen"};

	Com_Printf ("Initializing OpenGL display\n");

	Com_Printf ("...setting mode %d:", mode );

	if (!VID_GetModeInfo (&max_width, &max_height, &width, &height, mode, windowmode))
	{
		Com_Printf (" invalid mode\n");
		return rserr_invalid_mode;
	}
	
	x = vid_xpos->integer;
	y = vid_ypos->integer;

	Com_Printf ("Position %d %d, %d %d %s\n", x, y, width, height, win_fs[windowmode] );

	// destroy the existing window
	if (glw_state.hWnd)
	{
		GLimp_Shutdown ();
	}

	*pwidth = max_width;
	*pheight = max_height;

	// do a CDS if needed
	if (windowmode == windowmode_exclusive_fullscreen)
	{
		DEVMODE dm;

		x = y = 0;
		width = max_width;
		height = max_height;

		Com_Printf ("...attempting fullscreen\n" );

		memset( &dm, 0, sizeof( dm ) );

		dm.dmSize = sizeof( dm );

		dm.dmPelsWidth  = width;
		dm.dmPelsHeight = height;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		if ( gl_bitdepth->value != 0 )
		{
			dm.dmBitsPerPel = gl_bitdepth->value;
			dm.dmFields |= DM_BITSPERPEL;
			Com_Printf( "...using gl_bitdepth of %d\n", ( int ) gl_bitdepth->value );
		}
		else
		{
			HDC hdc = GetDC( NULL );
			int bitspixel = GetDeviceCaps( hdc, BITSPIXEL );

			Com_Printf ("...using desktop display depth of %d\n", bitspixel );

			ReleaseDC( 0, hdc );
		}

		if ( vid_displayfrequency->integer > 0 )
		{
			dm.dmFields |= DM_DISPLAYFREQUENCY;
			dm.dmDisplayFrequency = vid_displayfrequency->integer;
			Com_Printf ( "...using display frequency %i\n", dm.dmDisplayFrequency );
		}

		Com_Printf ("...calling CDS: " );
		if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL )
		{
			gl_state.fullscreen = true;

			Com_Printf ("ok\n" );

			if ( !VID_CreateWindow (x, y, width, height, windowmode_exclusive_fullscreen) )
				return rserr_invalid_mode;
			return rserr_ok;
		}
		else
		{
			Com_Printf ("failed\n" );

			Com_Printf ("...calling CDS assuming dual monitors:" );

			dm.dmPelsWidth = width * 2;
			dm.dmPelsHeight = height;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			if ( gl_bitdepth->value != 0 )
			{
				dm.dmBitsPerPel = gl_bitdepth->value;
				dm.dmFields |= DM_BITSPERPEL;
			}

			if ( vid_displayfrequency->integer > 0 )
			{
				dm.dmFields |= DM_DISPLAYFREQUENCY;
				dm.dmDisplayFrequency = vid_displayfrequency->integer;
				Com_Printf ( "...using display frequency %i\n", dm.dmDisplayFrequency );
			}

			/*
			** our first CDS failed, so maybe we're running on some weird dual monitor
			** system
			*/
			if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL )
			{
				Com_Printf (" failed\n" );

				Com_Printf ("...setting windowed mode\n" );

				ChangeDisplaySettings( 0, 0 );

				gl_state.fullscreen = false;
				if ( !VID_CreateWindow (x, y, width, height, windowmode_windowed) )
					return rserr_invalid_mode;
				return rserr_invalid_fullscreen;
			}
			else
			{
				Com_Printf (" ok\n" );
				if ( !VID_CreateWindow (x, y, width, height, windowmode_windowed) )
					return rserr_invalid_mode;

				gl_state.fullscreen = true;
				return rserr_ok;
			}
		}
	} else 
	{
		qboolean borderlessfullscreen = (windowmode == windowmode_borderless_windowed);
		if (borderlessfullscreen)
		{
			x = y = 0;
			width = max_width;
			height = max_height;
		}

		Com_Printf ("...setting %s mode\n", 
			borderlessfullscreen ? "borderless fullscreen" : "windowed" );

		ChangeDisplaySettings( 0, 0 );

		gl_state.fullscreen = false;
		if ( !VID_CreateWindow (x, y, width, height, windowmode) )
			return rserr_invalid_mode;
	}

	return rserr_ok;
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( void )
{

	if ( qwglMakeCurrent && !qwglMakeCurrent( NULL, NULL ) )
		Com_Printf ("ref_gl::R_Shutdown() - wglMakeCurrent failed\n");
	if ( glw_state.hGLRC )
	{
		if (  qwglDeleteContext && !qwglDeleteContext( glw_state.hGLRC ) )
			Com_Printf ("ref_gl::R_Shutdown() - wglDeleteContext failed\n");
		glw_state.hGLRC = NULL;
	}
	if (glw_state.hDC)
	{
		if ( !ReleaseDC( glw_state.hWnd, glw_state.hDC ) )
			Com_Printf ("ref_gl::R_Shutdown() - ReleaseDC failed\n" );
		glw_state.hDC   = NULL;
	}
	if (glw_state.hWnd)
	{
		ShowWindow (glw_state.hWnd, SW_HIDE);
		DestroyWindow (	glw_state.hWnd );
		glw_state.hWnd = NULL;
	}

	UnregisterClass (WINDOW_CLASS_NAME, glw_state.hInstance);

	if ( gl_state.fullscreen )
	{
		ChangeDisplaySettings( 0, 0 );
		gl_state.fullscreen = false;
	}
}

/*
===================
CPU Detect from Q2E
===================
*/
// does not appear to be used
/*
extern void	Q_strncpyz( char *dest, const char *src, size_t size );
extern void Q_strncatz( char *dest, const char *src, size_t size );
qboolean Sys_DetectCPU (char *cpuString, int maxSize)
	{
	char				vendor[16];
	int					stdBits, features, extFeatures;
	int					family, model;
	unsigned __int64	start, end, counter, stop, frequency;
	unsigned			speed;
	qboolean			hasMMX, hasMMXExt, has3DNow, has3DNowExt, hasSSE, hasSSE2;

	// Check if CPUID instruction is supported
	__try {
		__asm {
			mov eax, 0
			cpuid
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER){
		return false;
	}

	// Get CPU info
	__asm {
		; // Get vendor identifier
		mov eax, 0
		cpuid
		mov dword ptr[vendor+0], ebx
		mov dword ptr[vendor+4], edx
		mov dword ptr[vendor+8], ecx
		mov dword ptr[vendor+12], 0

		; // Get standard bits and features
		mov eax, 1
		cpuid
		mov stdBits, eax
		mov features, edx

		; // Check if extended functions are present
		mov extFeatures, 0
		mov eax, 80000000h
		cpuid
		cmp eax, 80000000h
		jbe NoExtFunction

		; // Get extended features
		mov eax, 80000001h
		cpuid
		mov extFeatures, edx

NoExtFunction:
	}

	// Get CPU name
	family = (stdBits >> 8) & 15;
	model = (stdBits >> 4) & 15;

	if (!Q_strcasecmp(vendor, "AuthenticAMD")){
		Q_strncpyz(cpuString, "AMD", maxSize);

		switch (family){
		case 5:
			switch (model){
			case 0:
			case 1:
			case 2:
			case 3:
				Q_strncatz(cpuString, " K5", maxSize);
				break;
			case 6:
			case 7:
				Q_strncatz(cpuString, " K6", maxSize);
				break;
			case 8:
				Q_strncatz(cpuString, " K6-2", maxSize);
				break;
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				Q_strncatz(cpuString, " K6-III", maxSize);
				break;
			}
			break;
		case 6:
			switch (model){
			case 1:		// 0.25 core
			case 2:		// 0.18 core
				Q_strncatz(cpuString, " Athlon", maxSize);
				break;
			case 3:		// Spitfire core
				Q_strncatz(cpuString, " Duron", maxSize);
				break;
			case 4:		// Thunderbird core
			case 6:		// Palomino core
				Q_strncatz(cpuString, " Athlon", maxSize);
				break;
			case 7:		// Morgan core
				Q_strncatz(cpuString, " Duron", maxSize);
				break;
			case 8:		// Thoroughbred core
			case 10:	// Barton core
				Q_strncatz(cpuString, " Athlon", maxSize);
				break;
			}
			break;
		}
	}
	else if (!Q_strcasecmp(vendor, "GenuineIntel")){
		Q_strncpyz(cpuString, "Intel", maxSize);

		switch (family){
		case 5:
			switch (model){
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 7:
			case 8:
				Q_strncatz(cpuString, " Pentium", maxSize);
				break;
			}
			break;
		case 6:
			switch (model){
			case 0:
			case 1:
				Q_strncatz(cpuString, " Pentium Pro", maxSize);
				break;
			case 3:
			case 5:		// Actual differentiation depends on cache settings
				Q_strncatz(cpuString, " Pentium II", maxSize);
				break;
			case 6:
				Q_strncatz(cpuString, " Celeron", maxSize);
				break;
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:	// Actual differentiation depends on cache settings
				Q_strncatz(cpuString, " Pentium III", maxSize);
				break;
			}
			break;
		case 15:
			Q_strncatz(cpuString, " Pentium 4", maxSize);
			break;
		}
	}
	else
		return false;

	// Check if RDTSC instruction is supported
	if ((features >> 4) & 1){
		// Measure CPU speed
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);

		__asm {
			rdtsc
			mov dword ptr[start+0], eax
			mov dword ptr[start+4], edx
		}

		QueryPerformanceCounter((LARGE_INTEGER *)&stop);
		stop += frequency;

		do {
			QueryPerformanceCounter((LARGE_INTEGER *)&counter);
		} while (counter < stop);

		__asm {
			rdtsc
			mov dword ptr[end+0], eax
			mov dword ptr[end+4], edx
		}

		speed = (unsigned)((end - start) / 1000000);

		Q_strncatz(cpuString, va(" %u MHz", speed), maxSize);
	}

	// Get extended instruction sets supported
	hasMMX = (features >> 23) & 1;
	hasMMXExt = (extFeatures >> 22) & 1;
	has3DNow = (extFeatures >> 31) & 1;
	has3DNowExt = (extFeatures >> 30) & 1;
	hasSSE = (features >> 25) & 1;
	hasSSE2 = (features >> 26) & 1;

	if (hasMMX || has3DNow || hasSSE){
		Q_strncatz(cpuString, " w/", maxSize);

		if (hasMMX){
			Q_strncatz(cpuString, " MMX", maxSize);
			if (hasMMXExt)
				Q_strncatz(cpuString, "+", maxSize);
		}
		if (has3DNow){
			Q_strncatz(cpuString, " 3DNow!", maxSize);
			if (has3DNowExt)
				Q_strncatz(cpuString, "+", maxSize);
		}
		if (hasSSE){
			Q_strncatz(cpuString, " SSE", maxSize);
			if (hasSSE2)
				Q_strncatz(cpuString, "2", maxSize);
		}
	}

	return true;


}
*/

static void Sys_SetCpuCore (void)
{
	SYSTEM_INFO cpuInfo;
/*
	cpumask=1 - use core #0
	cpumask=2 - use core #1
	cpumask=3 - use cores #0 & #1
*/
	if(!sys_affinity->value)
		return;

	if(sys_affinity->value >3)
		Cvar_SetValue("sys_affinity", 3);

	GetSystemInfo(&cpuInfo);

	/* if number of cpu core > 1
	we can run run game on second core or use both cores*/
	if (cpuInfo.dwNumberOfProcessors > 1)
			SetProcessAffinityMask(GetCurrentProcess(), (DWORD32)sys_affinity->value);

	CloseHandle(GetCurrentProcess());
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  Under Win32 this means dealing with the pixelformats and
** doing the wgl interface stuff.
*/
qboolean GLimp_Init( void *hinstance, void *wndproc )
{
#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	glw_state.allowdisplaydepthchange = false;

	//set high process priority for fullscreen mode
	if(vid_fullscreen->integer == windowmode_exclusive_fullscreen && sys_priority->value )
		SetPriorityClass (GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else
		SetPriorityClass (GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	Sys_SetCpuCore();

	if ( GetVersionEx( &vinfo) )
	{
		if ( vinfo.dwMajorVersion > 4 )
		{
			glw_state.allowdisplaydepthchange = true;
		}
		else if ( vinfo.dwMajorVersion == 4 )
		{
			if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
			{
				glw_state.allowdisplaydepthchange = true;
			}
			else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
			{
				if ( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
				{
					glw_state.allowdisplaydepthchange = true;
				}
			}
		}
	}
	else
	{
		Com_Printf ("GLimp_Init() - GetVersionEx failed\n" );
		return false;
	}

	glw_state.hInstance = ( HINSTANCE ) hinstance;
	glw_state.wndproc = wndproc;

	return true;
}

qboolean GLimp_InitGL ( qboolean getPixelFormatARB )
{
    PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		32,								// 32-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		24,								// 24-bit z-buffer
		8,								// 8 bit stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
    };
    int pixelformat = 0;
	cvar_t *stereo;
	stereo = Cvar_Get( "cl_stereo", "0", 0 );

	/*
	** set PFD_STEREO if necessary
	*/
	if ( stereo->value != 0 )
	{
		Com_Printf ("...attempting to use stereo\n" );
		pfd.dwFlags |= PFD_STEREO;
		gl_state.stereo_enabled = true;
	}
	else
	{
		gl_state.stereo_enabled = false;
	}

	/*
	** figure out if we're running on a minidriver or not
	*/
	if ( strstr( gl_driver->string, "opengl32" ) != 0 )
		glw_state.minidriver = false;
	else
		glw_state.minidriver = true;

	/*
	** Get a DC for the specified window
	*/
	if ( glw_state.hDC != NULL )
		Com_Printf ("GLimp_Init() - non-NULL DC exists\n" );

    if ( ( glw_state.hDC = GetDC( glw_state.hWnd ) ) == NULL )
	{
		Com_Printf ("GLimp_Init() - GetDC failed\n" );
		return false;
	}

	if ( getPixelFormatARB ) {
		// Needed for MSAA.
		// Just get the ARB pixel format and return, the window will be destroyed.
		// The next time it will use the ARB pixel format and skips qwglChoosePixelFormat or ChoosePixelFormat.
		GetARBPixelFormat( pfd );
		return true;
	} else if ( pixelformatARB )
	{
		pixelformat = pixelformatARB;
	}

	if ( glw_state.minidriver )
	{
		if ( !pixelformat ) 
		{
			if ( (pixelformat = qwglChoosePixelFormat( glw_state.hDC, &pfd)) == 0 )
			{
				Com_Printf ("GLimp_Init() - qwglChoosePixelFormat failed\n");
				return false;
			}
		}
		if ( qwglSetPixelFormat( glw_state.hDC, pixelformat, &pfd) == FALSE )
		{
			Com_Printf ("GLimp_Init() - qwglSetPixelFormat failed\n");
			return false;
		}
		qwglDescribePixelFormat( glw_state.hDC, pixelformat, sizeof( pfd ), &pfd );
	}
	else
	{
		if ( !pixelformat ) 
		{
			if ( ( pixelformat = ChoosePixelFormat( glw_state.hDC, &pfd)) == 0 )
			{
				Com_Printf ("GLimp_Init() - ChoosePixelFormat failed\n");
				return false;
			}
		}
		if ( SetPixelFormat( glw_state.hDC, pixelformat, &pfd) == FALSE )
		{
			Com_Printf ("GLimp_Init() - SetPixelFormat failed\n");
			return false;
		}
		DescribePixelFormat( glw_state.hDC, pixelformat, sizeof( pfd ), &pfd );

		if ( !( pfd.dwFlags & PFD_GENERIC_ACCELERATED ) )
		{
			glw_state.mcd_accelerated = false;
		}
		else
		{
			glw_state.mcd_accelerated = true;
		}
	}

	/*
	** report if stereo is desired but unavailable
	*/
	if ( !( pfd.dwFlags & PFD_STEREO ) && ( stereo->value != 0 ) )
	{
		Com_Printf ("...failed to select stereo pixel format\n" );
		Cvar_SetValue( "cl_stereo", 0 );
		gl_state.stereo_enabled = false;
	}

	/*
	** startup the OpenGL subsystem by creating a context and making
	** it current
	*/
	if ( ( glw_state.hGLRC = qwglCreateContext( glw_state.hDC ) ) == 0 )
	{
		Com_Printf ( "GLimp_Init() - qwglCreateContext failed\n");
		goto fail;
	}

    if ( !qwglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) )
	{
		Com_Printf ("GLimp_Init() - qwglMakeCurrent failed\n");
		goto fail;
	}

	if ( !VerifyDriver() )
	{
		Com_Printf ("GLimp_Init() - no hardware acceleration detected\n" );
		goto fail;
	}

	RS_ScanPathForScripts();		// load all found scripts

	/*
	** print out PFD specifics
	*/
	Com_Printf ("GL PFD: color(%d-bits) Z(%d-bit)\n", ( int ) pfd.cColorBits, ( int ) pfd.cDepthBits );

	if (pfd.cStencilBits) have_stencil = true; // Stencil shadows - MrG

	return true;

fail:
	if ( glw_state.hGLRC )
	{
		qwglDeleteContext( glw_state.hGLRC );
		glw_state.hGLRC = NULL;
	}

	if ( glw_state.hDC )
	{
		ReleaseDC( glw_state.hWnd, glw_state.hDC );
		glw_state.hDC = NULL;
	}
	return false;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_separation )
{
	if ( gl_bitdepth->modified )
	{
		if ( gl_bitdepth->value != 0 && !glw_state.allowdisplaydepthchange )
		{
			Cvar_SetValue( "gl_bitdepth", 0 );
			Com_Printf ("gl_bitdepth requires Win95 OSR2.x or WinNT 4.x\n" );
		}
		gl_bitdepth->modified = false;
	}

	if ( camera_separation < 0 && gl_state.stereo_enabled )
	{
		qglDrawBuffer( GL_BACK_LEFT );
	}
	else if ( camera_separation > 0 && gl_state.stereo_enabled )
	{
		qglDrawBuffer( GL_BACK_RIGHT );
	}
	else
	{
		qglDrawBuffer( GL_BACK );
	}
}

/*
** GLimp_EndFrame
**
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame (void)
{
	int		err;

	err = qglGetError();
	assert( err == GL_NO_ERROR ); // 2010-08 This assert is happening

	if ( Q_strcasecmp( gl_drawbuffer->string, "GL_BACK" ) == 0 )
	{
		if ( !qwglSwapBuffers( glw_state.hDC ) )
			Com_Error( ERR_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n" );
	}

	// rscript - MrG
	rs_realtime=Sys_Milliseconds() * 0.0005f;
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{
	if ( active )
	{
		SetForegroundWindow( glw_state.hWnd );
		ShowWindow( glw_state.hWnd, vid_fullscreen->integer == windowmode_exclusive_fullscreen ? SW_RESTORE : SW_SHOW );
	}
	else
	{
		if ( vid_fullscreen->integer == windowmode_exclusive_fullscreen )
		{
			ShowWindow( glw_state.hWnd, SW_MINIMIZE );
		}
	}
}

/*
** DestroyWindowGL
**
** Destroy the OpenGL window and release resources
**/
void DestroyWindowGL ( glwstate_t* window )			
{
	if ( window->hWnd != 0 )
	{	
		if ( window->hDC != 0 )
		{
			qwglMakeCurrent( window->hDC, 0 );

			if ( window->hGLRC != 0 )
			{
				qwglDeleteContext ( window->hGLRC );
				window->hGLRC = 0;
			}

			ReleaseDC( window->hWnd, window->hDC );
			window->hDC = 0;
		}
		DestroyWindow( window->hWnd );
		window->hWnd = 0;
	}
}

/*
** GetARBPixelFormat
**
** Get the ARB pixel format for MSAA. 
** If it can be retrieved successfully, it will be used in the second call to GLimp_InitGL 
** and it skips ChoosePixelFormat or qwglChoosePixelFormat.
**/
void GetARBPixelFormat( PIXELFORMATDESCRIPTOR pfd )
{
	int pixelformat;
	int iAttributes[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
		WGL_RED_BITS_ARB, 8,
		WGL_GREEN_BITS_ARB, 8, 
		WGL_BLUE_BITS_ARB, 8,
		WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
        WGL_SAMPLES_ARB, r_antialiasing->integer,
        0
	};

    UINT numFormats;
	HGLRC dummyContext;
	const char *ARBExtensions;		

	// ChoosePixelFormat, SetPixelFormat, and setting a dummy context, 
	// is all needed to be able to get the function pointer to wglChoosePixelFormatARB.
	if ( glw_state.minidriver )
	{
		pixelformat = qwglChoosePixelFormat( glw_state.hDC, &pfd );
		qwglSetPixelFormat( glw_state.hDC, pixelformat, &pfd );
	} else
	{
		pixelformat = ChoosePixelFormat( glw_state.hDC, &pfd );
		SetPixelFormat( glw_state.hDC, pixelformat, &pfd );
	}
	dummyContext = qwglCreateContext( glw_state.hDC );
	qwglMakeCurrent ( glw_state.hDC, dummyContext );
			
	if ( !wglGetExtensionsStringARB )
	{
		wglGetExtensionsStringARB = ( char* (WINAPI *)(HDC) ) qwglGetProcAddress( "wglGetExtensionsStringARB" );
	}

	if ( wglGetExtensionsStringARB )
	{
		ARBExtensions = wglGetExtensionsStringARB( qwglGetCurrentDC() );
	}

	if ( ARBExtensions ) 
	{
		if ( strstr( ARBExtensions, "WGL_ARB_multisample" ) && strstr( ARBExtensions, "WGL_ARB_pixel_format" ) )
		{
			Com_Printf( "WGL_ARB_multisample and WGL_ARB_pixel_format supported\n" );

			if ( !wglChoosePixelFormatARB )
			{
				wglChoosePixelFormatARB = ( int (WINAPI *)(HDC, CONST int *, CONST FLOAT *, UINT, int *, UINT *) ) 
					qwglGetProcAddress( "wglChoosePixelFormatARB" );
			}
			
			if ( wglChoosePixelFormatARB && wglChoosePixelFormatARB( glw_state.hDC, iAttributes, NULL, 1, &pixelformatARB, &numFormats ) )
			{
				if ( pixelformatARB && ( numFormats >= 1 ) )
				{
					qglEnable( GL_MULTISAMPLE_ARB );
				} else 
				{
					pixelformatARB = 0;
				}
			}
		}
	}

	// Clean up the dummy context
	qwglMakeCurrent( 0, 0 );
	qwglDeleteContext( dummyContext );
}
