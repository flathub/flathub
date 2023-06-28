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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#if defined HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include <limits.h>

#include "qcommon/qcommon.h"
#include "ref_gl/r_local.h"
#include "client/keys.h"
#include "unix/glw_unix.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#if defined HAVE_XXF86VM
#include <X11/extensions/xf86vmode.h>
#endif // defined HAVE_XXF86VM
#if defined HAVE_XXF86DGA
# if defined HAVE_X11_EXTENSIONS_XXF86DGA_H
#include <X11/extensions/Xxf86dga.h>
# else // defined HAVE_X11_EXTENSIONS_XXF86DGA_H
#include <X11/extensions/xf86dga.h>
# endif // defined HAVE_X11_EXTENSIONS_XXF86DGA_H
#endif

#include <GL/glx.h>

/* extern globals */
// using include files for these has some problems
extern cursor_t cursor;
extern qboolean mouse_available;
extern qboolean mouse_is_position;
extern int mouse_diff_x;
extern int mouse_diff_y;
extern float rs_realtime;
extern viddef_t vid;

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
qboolean	GLimp_Init( void *hinstance, void *hWnd );
void		GLimp_Shutdown( void );
rserr_t    	GLimp_SetMode( unsigned *pwidth, unsigned *pheight, int mode, windowmode_t windowmode );
void		GLimp_AppActivate( qboolean active );


//---------------------------------------------------------------------------


glwstate_t glw_state;

static Display *dpy = NULL;
static int scrnum;
static Window win;
static GLXContext ctx = NULL;
static Atom wmDeleteWindow;
static Atom cor_clipboard;

qboolean have_stencil = false; // Stencil shadows - MrG

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask | ButtonMotionMask )

#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | \
	       	StructureNotifyMask | PropertyChangeMask )

extern cvar_t	*vid_xpos;
extern cvar_t	*vid_ypos;
extern cvar_t	*vid_width;
extern cvar_t	*vid_height;
static cvar_t	*r_fakeFullscreen;
extern cvar_t	*in_dgamouse;
extern cvar_t	*r_antialiasing;
static int win_x, win_y;

#ifdef HAVE_XXF86VM

static XF86VidModeModeInfo saved_modeline;
int (*old_handler)(Display*, XErrorEvent*);
static qboolean vidmode_error_occurred;
static XErrorEvent vidmode_XErrorEvent;

#endif

static qboolean vidmode_active = false;

qboolean mouse_active = false;
qboolean dgamouse = false;
qboolean vidmode_ext = false;

static Cursor CreateNullCursor(Display *display, Window root)
{
    Pixmap cursormask;
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
          &dummycolour,&dummycolour, 0,0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}

void install_grabs(void)
{

#if defined DEBUG_GDB_NOGRAB
// for running on gdb debug. prevents "lockup".
	Cvar_Set( "in_dgamouse", "0" );
	dgamouse = false;
	mouse_is_position = false;
#else // defined DEBUG_GDB_NOGRAB
# if !defined HAVE_XXF86DGA
	XGrabPointer(dpy, win, True, 0, GrabModeAsync, GrabModeAsync, win, None, CurrentTime);
	XWarpPointer(dpy, None, win, 0, 0, 0, 0, viddef.width / 2, viddef.height / 2);
	Cvar_Set( "in_dgamouse", "0" );
	dgamouse = false;
# else // !defined HAVE_XXF86DGA
	XGrabPointer(dpy, win, True, 0, GrabModeAsync, GrabModeAsync, win, None, CurrentTime);

	if (in_dgamouse->integer)
	{
		int MajorVersion, MinorVersion;

		if (XF86DGAQueryVersion(dpy, &MajorVersion, &MinorVersion)) {
			XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
			XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectMouse);
			dgamouse = true;
		} else {
			// unable to query, probably not supported
			Com_Printf ( "Failed to detect XF86DGA Mouse\n" );
			Cvar_Set( "in_dgamouse", "0" );
			dgamouse = false;
			XWarpPointer(dpy, None, win, 0, 0, 0, 0, viddef.width / 2, viddef.height / 2);
		}
	} else {
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, viddef.width / 2, viddef.height / 2);
	}
# endif // !defined HAVE_XXF86DGA

	XGrabKeyboard(dpy, win, False, GrabModeAsync, GrabModeAsync, CurrentTime);

	mouse_is_position = false;
	mouse_diff_x = mouse_diff_y = 0;
#endif // defined DEBUG_GDB_NOGRAB
}

void uninstall_grabs(void)
{
	if (!dpy || !win)
		return;

#if defined HAVE_XXF86DGA
	if (dgamouse) {
		dgamouse = false;
		XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
	}
#endif

	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);

	mouse_is_position = true;
}

static void IN_DeactivateMouse( void )
{
	if (!dpy || !win)
		return;

	if (!mouse_is_position) {
		uninstall_grabs();
	}
}

static void IN_ActivateMouse( void )
{
	if (!dpy || !win)
		return;

	if (mouse_is_position) {
		install_grabs();
	}
}

void IN_Activate(qboolean active)
{
	if (active || vidmode_active)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse ();
}

static int XLateKey( XKeyEvent *ev )
{

	int key;
	char buf[64];
	int buflen;
	KeySym keysym;

	key = 0;
	buflen = XLookupString( ev, buf, sizeof buf, &keysym, 0 );

	switch ( keysym )
	{

	case XK_KP_Page_Up:
		key = K_KP_PGUP;
		break;

	case XK_Page_Up:
		key = K_PGUP;
		break;

	case XK_KP_Page_Down:
		key = K_KP_PGDN;
		break;

	case XK_Page_Down:
		key = K_PGDN;
		break;

	case XK_KP_Home:
		key = K_KP_HOME;
		break;

	case XK_Home:
		key = K_HOME;
		break;

	case XK_KP_End:
		key = K_KP_END;
		break;

	case XK_End:
		key = K_END;
		break;

	case XK_KP_Left:
		key = K_KP_LEFTARROW;
		break;

	case XK_Left:
		key = K_LEFTARROW;
		break;

	case XK_KP_Right:
		key = K_KP_RIGHTARROW;
		break;

	case XK_Right:
		key = K_RIGHTARROW;
		break;

	case XK_KP_Down:
		key = K_KP_DOWNARROW;
		break;

	case XK_Down:
		key = K_DOWNARROW;
		break;

	case XK_KP_Up:
		key = K_KP_UPARROW;
		break;

	case XK_Up:
		key = K_UPARROW;
		break;

	case XK_Escape:
		key = K_ESCAPE;
		break;

	case XK_KP_Enter:
		key = K_KP_ENTER;
		break;

	case XK_Return:
		if ( ev->state & Mod1Mask ) 
		{ 	// ALT-ENTER			
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
		} else
		{
			key = K_ENTER;
		}
		break;

	case XK_Tab:
		key = K_TAB;
		break;

	case XK_F1:
		key = K_F1;
		break;

	case XK_F2:
		key = K_F2;
		break;

	case XK_F3:
		key = K_F3;
		break;

	case XK_F4:
		key = K_F4;
		break;

	case XK_F5:
		key = K_F5;
		break;

	case XK_F6:
		key = K_F6;
		break;

	case XK_F7:
		key = K_F7;
		break;

	case XK_F8:
		key = K_F8;
		break;

	case XK_F9:
		key = K_F9;
		break;

	case XK_F10:
		key = K_F10;
		break;

	case XK_F11:
		key = K_F11;
		break;

	case XK_F12:
		key = K_F12;
		break;

	case XK_BackSpace:
		key = K_BACKSPACE;
		break;

	case XK_KP_Delete:
		key = K_KP_DEL;
		break;

	case XK_Delete:
		key = K_DEL;
		break;

	case XK_Pause:
		key = K_PAUSE;
		break;

	case XK_Shift_L:
	case XK_Shift_R:
		key = K_SHIFT;
		break;

	case XK_Execute:
	case XK_Control_L:
	case XK_Control_R:
		key = K_CTRL;
		break;

	case XK_Alt_L:
	case XK_Meta_L:
	case XK_Alt_R:
	case XK_Meta_R:
		key = K_ALT;
		break;

	case XK_KP_Begin:
		key = K_KP_5;
		break;

	case XK_Insert:
		key = K_INS;
		break;

	case XK_KP_Insert:
		key = K_KP_INS;
		break;

	case XK_KP_Multiply:
		key = '*';
		break;

	case XK_KP_Add:
		key = K_KP_PLUS;
		break;

	case XK_KP_Subtract:
		key = K_KP_MINUS;
		break;

	case XK_KP_Divide:
		key = K_KP_SLASH;
		break;

	default:
		if ( buflen == 1 )
		{
			key = *(unsigned char*)buf;

			if ( key >= 'A' && key <= 'Z' )
				key = key - 'A' + 'a';
			else if ( key >= 1 && key <= 26 ) /* ctrl+alpha */
				key = key + 'a' - 1;
			else if ( key == 0 )
				key = K_SPACE;
		}
		break;
	}

	return key;
}

void HandleEvents( void )
{
	XEvent event;
	qboolean dowarp = false;
	int mwx = viddef.width / 2;
	int mwy = viddef.height / 2;
	int multiclicktime = 750;
	int mouse_button;
	static int last_mouse_x = 0, last_mouse_y = 0;

	float f_sys_msecs;
	unsigned u_sys_msecs;

	if ( !dpy )
		return;

	// do one read of time for consistency
	// theory is that all pending events occurring before this point in time
	// should be considered occurring at this single point in time.
	u_sys_msecs = (unsigned)Sys_Milliseconds();
	f_sys_msecs = (float)u_sys_msecs;

	while ( XPending( dpy ) )
	{
		XNextEvent( dpy, &event );

		switch ( event.type )
		{

		case KeyPress:
			Key_Event( XLateKey( &event.xkey ), true, u_sys_msecs );
			break;

		case KeyRelease:
			Key_Event( XLateKey( &event.xkey ), false, u_sys_msecs );
			break;

		case MotionNotify:
			last_mouse_x = event.xmotion.x;
			last_mouse_y = event.xmotion.y;
			break;

		case ButtonPress:
			if ( event.xbutton.button )
			{
				mouse_button = event.xbutton.button - 1;
				switch ( mouse_button )
				{ // index swap, buttons 2 & 3
				case 1:
					mouse_button = 2;
					break;
				case 2:
					mouse_button = 1;
					break;
				}
				
				if (mouse_button < MENU_CURSOR_BUTTON_MAX)
				{
					if ( (f_sys_msecs - cursor.buttontime[mouse_button])
							< multiclicktime )
						cursor.buttonclicks[mouse_button] += 1;
					else
						cursor.buttonclicks[mouse_button] = 1;

					if ( cursor.buttonclicks[mouse_button] > 3 )
						cursor.buttonclicks[mouse_button] = 3;

					cursor.buttontime[mouse_button] = f_sys_msecs;

					cursor.buttondown[mouse_button] = true;
					cursor.buttonused[mouse_button] = false;
					cursor.mouseaction = true;
				}

				switch ( event.xbutton.button )
				{
				case 1:
					Key_Event( K_MOUSE1, true, u_sys_msecs );
					break;
				case 2:
					Key_Event( K_MOUSE3, true, u_sys_msecs );
					break;
				case 3:
					Key_Event( K_MOUSE2, true, u_sys_msecs );
					break;
				case 4:
					Key_Event( K_MWHEELUP, true, u_sys_msecs );
					break;
				case 5:
					Key_Event( K_MWHEELDOWN, true, u_sys_msecs );
					break;
				case 6:
					Key_Event( K_MOUSE4, true, u_sys_msecs );
					break;
				case 7:
					Key_Event( K_MOUSE5, true, u_sys_msecs );
					break;
				case 8:
					Key_Event( K_MOUSE6, true, u_sys_msecs );
					break;
				case 9:
					Key_Event( K_MOUSE7, true, u_sys_msecs );
					break;
				case 10:
					Key_Event( K_MOUSE8, true, u_sys_msecs );
					break;
				case 11:
					Key_Event( K_MOUSE9, true, u_sys_msecs );
					break;
				}
			}
			break;

		case ButtonRelease:
			if ( event.xbutton.button )
			{
				mouse_button = event.xbutton.button - 1;
				switch ( mouse_button )
				{ // index swap, buttons 2 & 3
				case 1:
					mouse_button = 2;
					break;
				case 2:
					mouse_button = 1;
					break;
				}

				if (mouse_button < MENU_CURSOR_BUTTON_MAX)
				{
					cursor.buttondown[mouse_button] = false;
					cursor.buttonused[mouse_button] = false;
				}

				switch ( event.xbutton.button )
				{
				case 1:
					Key_Event( K_MOUSE1, false, u_sys_msecs );
					break;
				case 2:
					Key_Event( K_MOUSE3, false, u_sys_msecs );
					break;
				case 3:
					Key_Event( K_MOUSE2, false, u_sys_msecs );
					break;
				case 4:
					Key_Event( K_MWHEELUP, false, u_sys_msecs );
					break;
				case 5:
					Key_Event( K_MWHEELDOWN, false, u_sys_msecs );
					break;
				case 6:
					Key_Event( K_MOUSE4, false, u_sys_msecs );
					break;
				case 7:
					Key_Event( K_MOUSE5, false, u_sys_msecs );
					break;
				case 8:
					Key_Event( K_MOUSE6, false, u_sys_msecs );
					break;
				case 9:
					Key_Event( K_MOUSE7, false, u_sys_msecs );
					break;
				case 10:
					Key_Event( K_MOUSE8, false, u_sys_msecs );
					break;
				case 11:
					Key_Event( K_MOUSE9, false, u_sys_msecs );
					break;
				}
			}
			break;

		case CreateNotify:
			win_x = event.xcreatewindow.x;
			win_y = event.xcreatewindow.y;
			break;

		case ConfigureNotify:
			win_x = event.xconfigure.x;
			win_y = event.xconfigure.y;
			
			VID_NewWindow( event.xconfigure.width, event.xconfigure.height );
			VID_NewPosition( win_x, win_y );

			break;

		case ClientMessage:
			if ( event.xclient.data.l[0] == wmDeleteWindow )
				Cbuf_ExecuteText( EXEC_NOW, "quit" );
			break;
		}
	}

	if ( mouse_is_position )
	{ // allow mouse movement on menus in windowed mode
		mouse_diff_x = last_mouse_x;
		mouse_diff_y = last_mouse_y;
	}
	else
	{
		if ( dgamouse )
		{ // TODO: find documentation for DGA mouse, explain this
			mouse_diff_x += (last_mouse_x + (vidmode_active ? 0 : win_x)) * 2;
			mouse_diff_y += (last_mouse_y + (vidmode_active ? 0 : win_y)) * 2;
		}
		else
		{ // add the delta from the current position to the center
			//  to the pointer motion accumulator
			mouse_diff_x += ((int)last_mouse_x - mwx);
			mouse_diff_y += ((int)last_mouse_y - mwy);

			// flag to recenter pointer
			dowarp = (mouse_diff_x != 0 || mouse_diff_y != 0 );
		}
	}

	if ( dowarp )
	{ /* move the pointer back to the window center */
		XWarpPointer( dpy, None, win, 0, 0, 0, 0, mwx, mwy );
	}

}

/*****************************************************************************/

qboolean GLimp_InitGL (void);

// TODO: modernize signal handler

static void signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	GLimp_Shutdown();
	exit(0);
}

static void InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}


#ifdef HAVE_XXF86VM
/*
 * X Error handler for XF86 VidMode switching.
 */
int vidmode_handler( Display *dpy, XErrorEvent *error_event )
{
	// flag and copy the error information.
	vidmode_error_occurred = true;
	memcpy(&vidmode_XErrorEvent, error_event, sizeof(XErrorEvent));

	return 0; // per spec. this is ignored.
}

/*
 * If in xf86vm fullscreen, restore original mode
 */
static void xf86vm_shutdown(void)
{
	char msgbfr[256];
	
	// Trap mode switch error using special error handler.
	vidmode_error_occurred = false;
	XSync(dpy,False);
	old_handler = XSetErrorHandler(vidmode_handler);

	XF86VidModeSwitchToMode(dpy, scrnum, &saved_modeline);

	// Restore X error handler.
	XSync(dpy, False);
	XSetErrorHandler(old_handler);

	if ( vidmode_error_occurred )
	{
		/* This, almost certainly, will mess up video configuration.
		 * But, it means the saved_modeline got corrupted.
		 */
		Com_Printf("In video shutdown, restoring previous XF86vm mode failed:\n");
		XGetErrorText(dpy, vidmode_XErrorEvent.error_code, msgbfr, sizeof(msgbfr));
		Com_Printf("..%s\n..major opcode %u, minor opcode %u\n",
			msgbfr, vidmode_XErrorEvent.request_code, vidmode_XErrorEvent.minor_code );
	}
}

/*
 * XF86 VidMode setting for full screen.
 *
 * Reference:
 *  man 3 XF86VidModeQueryVersion
 *  man 3 XF86VidModeSwitchToMode
 *  man 3 XSetErrorHandler
 */
static void xf86vm_fullscreen( int req_width, int req_height )
{
	int major, minor;
	int num_vidmodes;
	XF86VidModeModeInfo **vidmodes;
	int best_fit, best_dist, dist, x, y;
	int actualWidth, actualHeight;
	int i;
	int xpos, ypos;
	char msgbfr[256];

	vidmode_ext = XF86VidModeQueryVersion(dpy, &major, &minor);
	if ( !vidmode_ext )
	{
		Com_Printf("XF86VidModeQueryVersion failed. XF86VidMode Extension not active.\n");
		return;
	}
	Com_Printf("...Using XF86VidMode Extension Version %d.%d\n", major, minor);

	if ( !XF86VidModeGetAllModeLines(dpy, scrnum, &num_vidmodes, &vidmodes) )
	{
		vidmode_ext = false;
		Com_Printf("XF86VidModeGetAllModeLines failed. XF86VidMode Extension not active.\n");
		return;
	}

	// Current ModeLine is the first in list. Save it for later restoration.
	memcpy( &saved_modeline, vidmodes[0], sizeof(XF86VidModeModeInfo) );

	// Find best fit modeline for requested resolution.
	best_dist = INT_MAX;
	best_fit  = -1;
	for (i = 0; i < num_vidmodes; i++)
	{
		Com_Printf("....XF86vm %i: %i x %i\n", i,
				   vidmodes[i]->hdisplay, vidmodes[i]->vdisplay);
		
		if ( req_width > vidmodes[i]->hdisplay
			 || req_height > vidmodes[i]->vdisplay)
			continue; // vidmode has smaller width or height than requested resolution

		x = req_width - vidmodes[i]->hdisplay;
		y = req_height - vidmodes[i]->vdisplay;
		dist = (x * x) + (y * y);
		if (dist < best_dist)
		{
			// closest to requested resolution, so far
			best_dist = dist;
			best_fit = i;
		}
	}

	if ( best_fit != -1 )
	{
		// the resolution to be used for fullscreen.
		actualWidth  = vidmodes[best_fit]->hdisplay;
		actualHeight = vidmodes[best_fit]->vdisplay;
		Com_Printf("....XF86vm: requested %i X %i  actual: %i X %i\n",
				   req_width, req_height, actualWidth, actualHeight);

		// Trap mode switch error using special error handler.
		vidmode_error_occurred = false;
		XSync(dpy,False);
		old_handler = XSetErrorHandler(vidmode_handler);

		XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[best_fit]);

		// Restore error handler.
		XSync(dpy,False);
		XSetErrorHandler(old_handler); 

		if ( vidmode_error_occurred )
		{
			Com_Printf("....XF86vm mode switch failed:\n");
			XGetErrorText(dpy, vidmode_XErrorEvent.error_code, msgbfr, sizeof(msgbfr));
			Com_Printf(".....%s\n......major opcode %u, minor opcode %u\n",
					msgbfr, vidmode_XErrorEvent.request_code, vidmode_XErrorEvent.minor_code );
			
			// cleanup and return.
			vidmode_active = false; // just to be sure.
			XFree(vidmodes);
			return;
		}
	
		vidmode_active = true; // XF86vm used for fullscreen.

		// Move the viewport to top left
		XF86VidModeSetViewPort(dpy, scrnum, 0, 0);

		if (req_width != actualWidth || req_height != actualHeight)
		{
			xpos = vid_xpos->integer;
			ypos = vid_ypos->integer;
			Com_Printf("Resolution %dx%d is not supported natively by the display!\n", req_width, req_height);
			Com_Printf("Closest screen resolution is %dx%d. ", actualWidth, actualHeight);
			Com_Printf("Use vid_xpos and vid_ypos to adjust the position of the game window (current offset is %d, %d)\n", xpos, ypos);

		}
	}
	else
	{
		Com_Printf("....XF86vm mode match for %u X %u failed.\n", req_width, req_height);
	}

	XFree(vidmodes); // per spec
}
#endif // HAVE_XXF86VM

void GoBorderlessFullscreen() 
{
	Atom state = XInternAtom(dpy, "_NET_WM_STATE", False);
	Atom stateFullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	Atom atoms[2] = { stateFullscreen, None };	

	XChangeProperty(
		dpy, 
		win, 
		state,
		XA_ATOM, 32, PropModeReplace, (unsigned char *) atoms, 1
	);
}

/*
** GLimp_SetMode
*/
rserr_t GLimp_SetMode (unsigned *pwidth, unsigned *pheight, int mode, windowmode_t windowmode)
{
	int xpos, ypos;

	int width, height, max_width, max_height;
	const char *win_fs[] = {"Windowed", "Borderless Windowed", "Exclusive Fullscreen"};

	int attribMSAA[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 4,
		GLX_GREEN_SIZE, 4,
		GLX_BLUE_SIZE, 4,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		GLX_SAMPLE_BUFFERS  , 1,
		GLX_SAMPLES         , r_antialiasing->integer,
		None
	};
	
	int attrib[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 4,
		GLX_GREEN_SIZE, 4,
		GLX_BLUE_SIZE, 4,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		None
	};
	int attrib2[] = { //no stencil buffer, original settings
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		None
	};

	Window root;
	XVisualInfo *visinfo;
	XSetWindowAttributes attr;
	XSizeHints *sizehints;
	XClassHint *class_hint;
	unsigned long mask;

	/* release keyboard and mouse until after initialization */
	uninstall_grabs();

	r_fakeFullscreen = Cvar_Get ("r_fakeFullscreen", "0", CVAR_ARCHIVE);

	Com_Printf ( "Initializing OpenGL display\n");

	switch (windowmode) {
		case windowmode_exclusive_fullscreen:
			Com_Printf ("...setting fullscreen mode %d:", mode);
			break;
		case windowmode_borderless_windowed:
			Com_Printf ("...setting borderless windowed mode %d:", mode);
			break;
		default:
			Com_Printf ("...setting mode %d:", mode);
	}

	if (!VID_GetModeInfo (&max_width, &max_height, &width, &height, mode, windowmode))
	{
		Com_Printf (" invalid mode\n");
		return rserr_invalid_mode;
	}

	xpos = vid_xpos->integer;
	ypos = vid_ypos->integer;

	Com_Printf ("Position %d %d, %d %d %s\n", xpos, ypos, width, height, win_fs[windowmode]);

	// destroy the existing window
	GLimp_Shutdown ();

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Error couldn't open the X display\n");
		return rserr_invalid_mode;
	}

	scrnum = DefaultScreen(dpy);
	root = RootWindow(dpy, scrnum);

	if ( r_antialiasing->integer )
	{
		visinfo = qglXChooseVisual(dpy, scrnum, attribMSAA);
	}
	
	if ( !r_antialiasing->integer || !visinfo )
	{
		visinfo = qglXChooseVisual(dpy, scrnum, attrib);
	}
	
	if (!visinfo) {
		fprintf(stderr, "Error couldn't get an RGB, Double-buffered, Depth visual, Stencil Buffered\n");
		visinfo = qglXChooseVisual(dpy, scrnum, attrib2);
		if(!visinfo){
			fprintf(stderr, "Error couldn't get an RGB, Double-buffered, Depth visual\n");
			return rserr_invalid_mode;
		}
	}
	else
		have_stencil = true;

	vidmode_ext = false; // XF86vm not used, until otherwise determined.
	vidmode_active = false; // not XF86vm fullscreen, until otherwise determined.

#ifdef HAVE_XXF86VM	
	if (windowmode == windowmode_exclusive_fullscreen && !r_fakeFullscreen->integer)
	{
		// Use XF86VidMode to request fullscreen of specified resolution.
		xf86vm_fullscreen(width, height);
	}
#endif	

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = X_MASK;
	if (vidmode_active) {
		mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore |
			CWEventMask | CWOverrideRedirect;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	} else
		mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow(dpy, root, xpos, ypos, width, height,
	 					0, visinfo->depth, InputOutput,
	 					visinfo->visual, mask, &attr);
	
	if (windowmode == windowmode_borderless_windowed) 
	{		
		GoBorderlessFullscreen();
	} else if (windowmode == windowmode_windowed) 
	{	
		sizehints = XAllocSizeHints();
		if ( sizehints ) {
			sizehints->base_width = width;
			sizehints->base_height = height;
			sizehints->max_width = max_width;
			sizehints->max_height = max_height;
			sizehints->min_width = VID_MIN_WIDTH;
			sizehints->min_height = VID_MIN_HEIGHT;
			sizehints->x = xpos;
			sizehints->y = ypos;
			
			sizehints->flags = PMinSize | PMaxSize | PBaseSize | PPosition;
		}

		XSetWMProperties(dpy, win, NULL, NULL, NULL, 0,
				sizehints, None, None);

		if (sizehints)
			XFree(sizehints);
	}
	/* For Gnome 3
	 * Ref: .desktop files, and Gnome 3 doc
	 */
#ifdef TACTICAL

	XStoreName(dpy, win, "Alien Arena Tactical Demo");

	class_hint = XAllocClassHint();
	if ( class_hint != NULL )
	{
		class_hint->res_name = "Alien Arena Tactical Demo";
		class_hint->res_class = "aa-tactical-demo"; /* .desktop */
		XSetClassHint( dpy, win, class_hint );

		XFree( class_hint );
	}	
#else

	XStoreName(dpy, win, "Alien Arena");

	class_hint = XAllocClassHint();
	if ( class_hint != NULL )
	{
		class_hint->res_name = "Alien Arena";
		class_hint->res_class = "alienarena"; /* .desktop */
		XSetClassHint( dpy, win, class_hint );

		XFree( class_hint );
	}
#endif

	wmDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &wmDeleteWindow, 1);

	cor_clipboard = XInternAtom(dpy, "COR_CLIPBOARD", False);

	XMapWindow(dpy, win);

#if defined HAVE_XXF86VM
	if (vidmode_active) {
		xpos = ypos = 0;

		XMoveWindow(dpy, win, xpos, ypos);
		XRaiseWindow(dpy, win);
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		XFlush(dpy);
		// Move the viewport to top left
		XF86VidModeSetViewPort(dpy, scrnum, xpos, ypos);
	}
#endif // defined HAVE_XXF86VM

	XFlush(dpy);

	ctx = qglXCreateContext(dpy, visinfo, NULL, True);

	qglXMakeCurrent(dpy, win, ctx);

	*pwidth = max_width;
	*pheight = max_height;

	// let the sound and input subsystems know about the new window
	VID_NewWindow (width, height);
	VID_NewPosition (xpos, ypos);

	XDefineCursor(dpy, win, CreateNullCursor(dpy, win));

	qglXMakeCurrent(dpy, win, ctx);

	RS_ScanPathForScripts();		// load all found scripts

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
	uninstall_grabs();
	mouse_is_position = true;
	dgamouse = false;

	if (dpy) {
		if (ctx)
			qglXDestroyContext(dpy, ctx);
		if (win)
			XDestroyWindow(dpy, win);
		
#ifdef HAVE_XXF86VM
		 // XF86vm fullscreen back to original resolution.
		if (vidmode_active)
			xf86vm_shutdown();
#endif		

		XUngrabKeyboard(dpy, CurrentTime);
		XCloseDisplay(dpy);
	}
	ctx = NULL;
	dpy = NULL;
	win = 0;
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.
*/
qboolean GLimp_Init( void *hinstance, void *wndproc )
{


	InitSig();

	return true;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_seperation )
{
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
	qglXSwapBuffers(dpy, win); // does glFlush()

	// rscript - MrG
	rs_realtime = (float)Sys_Milliseconds() * 0.0005f;
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{
}

/*------------------------------------------------*/
/* X11 Input Stuff				  */
/*------------------------------------------------*/

/*
** Sys_GetClipboardData()
**
** Returns the contents of the X clipboard; needs to
** be here instead of inside sys_unix.c because it
** needs to access the X display.
*/
char *Sys_GetClipboardData(void)
{
	XEvent evt;
	int sent_at;
	Bool received;
	Atom prop_type;
	int prop_fmt;
	unsigned long prop_items, prop_size;
	unsigned char *buffer, *output = NULL;

	// request the contents of the clipboard
	XConvertSelection( dpy, XA_PRIMARY, XA_STRING,
		cor_clipboard, win, CurrentTime );
	sent_at = Sys_Milliseconds();

	// now we need to wait until either:
	//  1) we get a response
	//  2) 250ms have gone by and we got nothing
	do
       	{
		received = XCheckTypedEvent( dpy, SelectionNotify, &evt );
	} while ( !( received || Sys_Milliseconds() - sent_at > 250 ) );

	// no reply received, return NULL
	if ( !received )
		return NULL;

	// get information about property
	XGetWindowProperty( dpy , win, cor_clipboard, 0, 0, False,
			AnyPropertyType, &prop_type, &prop_fmt,
			&prop_items, &prop_size, &buffer );
	XFree(buffer);

	// only get the text if it's actually ASCII
	if ( prop_fmt == 8 )
	{
		XGetWindowProperty( dpy , win, cor_clipboard, 0,
				prop_size, False, AnyPropertyType,
				&prop_type, &prop_fmt, &prop_items,
				&prop_size, &buffer );

		output = malloc( prop_items + 1 );
		memcpy( output, buffer, prop_items );
		output[prop_items] = 0;
	}

	XDeleteProperty(dpy, win, cor_clipboard);

	return (char*)output;
}

