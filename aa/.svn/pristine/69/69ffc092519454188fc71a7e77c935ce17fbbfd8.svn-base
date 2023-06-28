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
// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <float.h>

#include "client\client.h"
#include "winquake.h"
//#include "zmouse.h"

cvar_t *win_noalttab;

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  // message that will be supported by the OS
#endif

static UINT MSH_MOUSEWHEEL;

// Console variables that we need to access from this module
cvar_t		*vid_gamma;
cvar_t		*vid_ref;			// Name of Refresh DLL loaded
cvar_t		*vid_xpos;			// X coordinate of window position
cvar_t		*vid_ypos;			// Y coordinate of window position
cvar_t		*vid_fullscreen;
cvar_t		*vid_preferred_fullscreen;
cvar_t		*vid_width;
cvar_t		*vid_height;
cvar_t		*vid_displayfrequency;

// Global variables used internally by this module
viddef_t	viddef;				// global video state; used by other modules
qboolean	reflib_active = 0;

HWND        cl_hwnd;            // Main window handle for life of program

LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static qboolean s_alttab_disabled;

extern	unsigned	sys_msg_time;

/*
** WIN32 helper functions
*/
extern qboolean s_win95;

static void WIN_DisableAltTab( void )
{
	if ( s_alttab_disabled )
		return;

	if ( s_win95 )
	{
		BOOL old;
		SystemParametersInfo( SPI_SETSCREENSAVERRUNNING, 1, &old, 0 );
			// using MinGW compatible equivalent SPI constant
	}
	else
	{
		RegisterHotKey( 0, 0, MOD_ALT, VK_TAB );
		RegisterHotKey( 0, 1, MOD_ALT, VK_RETURN );
	}
	s_alttab_disabled = true;
}

static void WIN_EnableAltTab( void )
{
	if ( s_alttab_disabled )
	{
		if ( s_win95 )
		{
			BOOL old;
			SystemParametersInfo( SPI_SETSCREENSAVERRUNNING, 0, &old, 0 );
				// using MinGW compatible equivalent SPI constant
		}
		else
		{
			UnregisterHotKey( 0, 0 );
			UnregisterHotKey( 0, 1 );
		}

		s_alttab_disabled = false;
	}
}

//==========================================================================

byte        scantokey[128] =
					{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
	'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*',
	K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0  , K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11,
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7
};

/*
=======
MapKey

Map from windows to quake keynums
=======
*/
int MapKey (int key)
{
	int result;
	int modified = ( key >> 16 ) & 255;
	qboolean is_extended = false;

	if ( modified > 127)
		return 0;

	if ( key & ( 1 << 24 ) )
		is_extended = true;

	result = scantokey[modified];

	if ( !is_extended )
	{
		switch ( result )
		{
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		default:
			return result;
		}
	}
	else
	{
		switch ( result )
		{
		case 0x0D:
			return K_KP_ENTER;
		case 0x2F:
			return K_KP_SLASH;
		case 0xAF:
			return K_KP_PLUS;
		}
		return result;
	}
}

void AppActivate(BOOL fActive, BOOL minimize)
{
	Minimized = minimize;

	Key_ClearStates();

	// we don't want to act like we're active if we're minimized
	if (fActive && !Minimized)
		ActiveApp = true;
	else
		ActiveApp = false;

	// minimize/restore mouse-capture on demand
	if (!ActiveApp)
	{
		IN_Activate (false);
		S_Activate (false);

		if ( win_noalttab->value )
		{
			WIN_EnableAltTab();
		}
	}
	else
	{
		IN_Activate (true);
		S_Activate (true);
		if ( win_noalttab->value )
		{
			WIN_DisableAltTab();
		}
	}
}

/*
====================
MainWndProc

main window procedure
====================
*/
LONG WINAPI MainWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
	LONG			lRet = 0;

	if ( uMsg == MSH_MOUSEWHEEL )
	{
		if ( ( ( int ) wParam ) > 0 )
		{
			Key_Event( K_MWHEELUP, true, sys_msg_time );
			Key_Event( K_MWHEELUP, false, sys_msg_time );
		}
		else
		{
			Key_Event( K_MWHEELDOWN, true, sys_msg_time );
			Key_Event( K_MWHEELDOWN, false, sys_msg_time );
		}
        return DefWindowProc (hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		/*
		** this chunk of code theoretically only works under NT4 and Win98
		** since this message doesn't exist under Win95
		*/
		if ( ( short ) HIWORD( wParam ) > 0 )
		{
			Key_Event( K_MWHEELUP, true, sys_msg_time );
			Key_Event( K_MWHEELUP, false, sys_msg_time );
		}
		else
		{
			Key_Event( K_MWHEELDOWN, true, sys_msg_time );
			Key_Event( K_MWHEELDOWN, false, sys_msg_time );
		}
		break;

	case WM_HOTKEY:
		return 0;

	case WM_CREATE:
		cl_hwnd = hWnd;

		MSH_MOUSEWHEEL = RegisterWindowMessage("MSWHEEL_ROLLMSG");
        return DefWindowProc (hWnd, uMsg, wParam, lParam);

	case WM_PAINT:
		return DefWindowProc (hWnd, uMsg, wParam, lParam);

	case WM_DESTROY:
		// let sound and input know about this?
		cl_hwnd = NULL;
        return DefWindowProc (hWnd, uMsg, wParam, lParam);

	case WM_ACTIVATE:
		{
			int	fActive, fMinimized;

			// KJB: Watch this for problems in fullscreen modes with Alt-tabbing.
			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);

			AppActivate( fActive != WA_INACTIVE, fMinimized);

			if ( reflib_active )
				R_AppActivate( !( fActive == WA_INACTIVE ) );
		}
        return DefWindowProc (hWnd, uMsg, wParam, lParam);

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO mmi = (LPMINMAXINFO) lParam;
			mmi->ptMinTrackSize.x = VID_MIN_WIDTH;
			mmi->ptMinTrackSize.y = VID_MIN_HEIGHT;
			mmi->ptMaxTrackSize.x = VID_MAX_WIDTH;
			mmi->ptMaxTrackSize.y = VID_MAX_HEIGHT;
		}
		return 0;

	case WM_SIZE:
		if (vid_fullscreen->integer == windowmode_windowed)
		{
			int width, height;
			width = (short) LOWORD(lParam);
			height = (short) HIWORD(lParam);

			VID_NewWindow (width, height);
		}
		return DefWindowProc (hWnd, uMsg, wParam, lParam);

	case WM_MOVE:
		{
			int		xPos, yPos;
			RECT r;
			int		style;

			if (vid_fullscreen->integer == windowmode_windowed)
			{
				xPos = (short) LOWORD(lParam);    // horizontal position
				yPos = (short) HIWORD(lParam);    // vertical position

				r.left   = 0;
				r.top    = 0;
				r.right  = 1;
				r.bottom = 1;

				style = GetWindowLong( hWnd, GWL_STYLE );
				AdjustWindowRect( &r, style, TRUE );

				VID_NewPosition (xPos, yPos);

				if (ActiveApp)
					IN_Activate (true);
			}
		}
        return DefWindowProc (hWnd, uMsg, wParam, lParam);

// this is complicated because Win32 seems to pack multiple mouse events into
// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
		{
			int	temp;

			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

			/*  TODO: Research Windows mouse button support.
			 *
			 * MK_XBUTTON1 and MK_XBUTTON2 are 2 additional. Meaning TBD
			 *
			 * (why was mousebuttons set to 7?, )
			 * (how many does win support. 5 for sure, more how?)
			 *
			 *  MK_LBUTTON 0x01
			 *  MK_RBUTTON 0x02
			 *  MK_MBUTTION 0x10
			 *  MK_XBUTTON1 0x20
			 *  MK_XBUTTON2 0x40
			 *
			 */

			IN_MouseEvent (temp);
		}
		break;

	case WM_SYSCOMMAND:
		if ( wParam == SC_SCREENSAVE )
			return 0;
        return DefWindowProc (hWnd, uMsg, wParam, lParam);
	case WM_SYSKEYDOWN:
		if ( wParam == 13 )
		{
			if ( vid_fullscreen )
			{
				switch ( vid_fullscreen->integer ) {
					case windowmode_windowed:
						Cvar_SetValue( "vid_fullscreen", vid_preferred_fullscreen->value );
						break;
					case windowmode_borderless_windowed:
					case windowmode_exclusive_fullscreen:
						Cvar_SetValue( "vid_preferred_fullscreen", vid_fullscreen->value );
						Cvar_SetValue( "vid_fullscreen", windowmode_windowed );
						break;
				}
			}
			return 0;
		}
		// fall through
	case WM_KEYDOWN:
		Key_Event( MapKey( lParam ), true, sys_msg_time);
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		Key_Event( MapKey( lParam ), false, sys_msg_time);
		break;

	default:	// pass all unhandled messages to DefWindowProc
        return DefWindowProc (hWnd, uMsg, wParam, lParam);
    }

    /* return 0 if handled message, 1 if not */
    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/

void VID_Restart_f (void)
{	
	vid_ref->modified = true;
}

void VID_Front_f( void )
{
	SetWindowLong( cl_hwnd, GWL_EXSTYLE, WS_EX_TOPMOST );
	SetForegroundWindow( cl_hwnd );
}

/*
** VID_GetModeInfo
*/

//Gonna get rid of the lower resolution - so that the HUD doesn't get knocked off the screen
//besides, who the hell wants to play at anything less than 640x480
vidmode_t vid_modes[] =
{
	{ "Mode 0: 640x480",   640, 480,   0 },
    { "Mode 1: 800x600",   800, 600,   1 },
    { "Mode 2: 960x720",   960, 720,   2 },
    { "Mode 3: 1024x768",  1024, 768,  3 },
    { "Mode 4: 1152x864",  1152, 864,  4 },
    { "Mode 5: 1280x960",  1280, 960, 5 },
	{ "Mode 6: 1280x1024", 1280, 1024, 6 },
	{ "Mode 7: 1360x768",  1360, 768, 7 },
	{ "Mode 8: 1366x768",  1366, 768, 8 },
    { "Mode 9: 1600x1200", 1600, 1200, 9 },
	{ "Mode 10: 1680x1050", 1680, 1050, 10 },
	{ "Mode 11: 1920x1080", 1920, 1080, 11 },
	{ "Mode 12: 2048x1536", 2048, 1536, 12 },
	{ "Mode 13: 2560x1080", 2560, 1080, 13 },
	{ "Mode 14: 2560x1440", 2560, 1440, 14 },
	{ "Mode 15: 3440x1440", 3440, 1440, 15 },
	{ "Mode 16: 3840x2160", 3840, 2160, 16 },
	{ "Mode 17: 7680x4320", 7680, 4320, 17 },
};

static int s_numVidModes = ( sizeof(vid_modes) / sizeof(vid_modes[0]) );
qboolean VID_GetModeInfo (int *max_width, int *max_height, int *current_width, int *current_height, int mode, windowmode_t windowmode)
{
	if (mode < -1 || mode >= s_numVidModes)
		return false;

	if (mode == -1)
	{
		*max_width = vid_width->integer;
		*max_height = vid_height->integer;
	}
	else
	{
		*max_width = vid_modes[mode].width;
		*max_height = vid_modes[mode].height;
	}

	if (windowmode == windowmode_windowed)
	{
		*current_width = (viddef.width && viddef.width < *max_width) ? viddef.width : *max_width;
		*current_height = (viddef.height && viddef.height < *max_height) ? viddef.height : *max_height;
	}
	else
	{
		*current_width = *max_width;
		*current_height = *max_height;
	}

	return true;
}

void VID_ModeList_f(void)
{
        int i;

        for ( i = 0; i < s_numVidModes; i++)
        {
                Com_Printf ( "%s\n", vid_modes[i].description );
        }
        Com_Printf("For custom resolutions, set 'gl_mode' to -1 and use 'vid_width' / 'vid_height'\n");
}

/*
** VID_NewWindow
*/
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;

	cl.force_refdef = true;		// can't use a paused refdef
}

void VID_NewPosition( int x, int y )
{
	Cvar_SetValue( "vid_xpos", x);
	Cvar_SetValue( "vid_ypos", y);	
}

void VID_FreeReflib (void)
{
	reflib_active  = false;
}

/*
==============
VID_LoadRefresh
==============
*/
qboolean VID_LoadRefresh(void)
{
	if ( reflib_active )
	{
		R_Shutdown();
		VID_FreeReflib ();
	}

	if ( R_Init( global_hInstance, MainWndProc ) == -1 )
	{
		R_Shutdown();
		VID_FreeReflib ();
		Com_Error (ERR_FATAL, "Couldn't initialize OpenGL renderer!\nUpdate your video drivers and be sure opengl is enabled!\n");

		return false;
	}

	Com_Printf( "------------------------------------\n");
	reflib_active = true;

	return true;
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (void)
{
	if ( win_noalttab->modified )
	{
		if ( win_noalttab->value )
		{
			WIN_DisableAltTab();
		}
		else
		{
			WIN_EnableAltTab();
		}
		win_noalttab->modified = false;
	}

	if ( vid_ref->modified )
	{
		cl.force_refdef = true;		// can't use a paused refdef
		S_StopAllSounds();
	}
	while (vid_ref->modified)
	{
		/*
		** refresh has changed
		*/
		vid_ref->modified = false;
		vid_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		if ( !VID_LoadRefresh() )
		{
			/*
			** drop the console if we fail to load a refresh
			*/
			if ( cls.key_dest != key_console )
			{
				CON_ToggleConsole();
			}
		}
		cls.disable_screen = false;
	}
}

/*
============
VID_Init
============
*/
void VID_Init (void)
{
	/* Create the video variables so we know how to start the graphics drivers */
	vid_ref = Cvar_Get ("vid_ref", "soft", CVAR_ARCHIVE);
	vid_xpos = Cvar_Get ("vid_xpos", "0", CVAR_ARCHIVE);
	vid_ypos = Cvar_Get ("vid_ypos", "0", CVAR_ARCHIVE);
	vid_fullscreen = Cvar_Get ("vid_fullscreen", "1", CVAR_ARCHIVE);
	vid_preferred_fullscreen = Cvar_Get ("vid_preferred_fullscreen", "1", CVAR_ARCHIVE);
	vid_gamma = Cvar_Get ( "vid_gamma", "1", CVAR_ARCHIVE );
	vid_width = Cvar_Get ( "vid_width", "1024", CVAR_ARCHIVE );
	vid_height = Cvar_Get ( "vid_height", "768", CVAR_ARCHIVE );
	win_noalttab = Cvar_Get( "win_noalttab", "0", CVAR_ARCHIVE );
	vid_displayfrequency = Cvar_Get( "vid_displayfrequency", "0", CVAR_ARCHIVE );

	/* Add some console commands that we want to handle */
	Cmd_AddCommand ("vid_restart", VID_Restart_f);
	Cmd_AddCommand ("vid_front", VID_Front_f);

	/*
	** this is a gross hack but necessary to clamp the mode for 3Dfx
	*/
#if 0
	{
		cvar_t *gl_driver = Cvar_Get( "gl_driver", "opengl32", 0 );
		cvar_t *gl_mode = Cvar_Get( "gl_mode", "3", 0 );

		if ( Q_strcasecmp( gl_driver->string, "3dfxgl" ) == 0 )
		{
			Cvar_SetValue( "gl_mode", 3 );
			viddef.width  = 640;
			viddef.height = 480;
		}
	}
#endif

	/* Disable the 3Dfx splash screen */
	_putenv("FX_GLIDE_NO_SPLASH=0");

	/* Start the graphics mode and load refresh DLL */
	VID_CheckChanges();
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if ( reflib_active )
	{
		R_Shutdown ();
		VID_FreeReflib ();
	}
}


