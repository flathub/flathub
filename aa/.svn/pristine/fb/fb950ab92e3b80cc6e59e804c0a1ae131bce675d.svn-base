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
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake2 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <float.h>
#include "ref_gl/r_local.h"
#include "glw_win.h"

int   ( WINAPI * qwglChoosePixelFormat )(HDC, CONST PIXELFORMATDESCRIPTOR *);
int   ( WINAPI * qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
int   ( WINAPI * qwglGetPixelFormat)(HDC);
BOOL  ( WINAPI * qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
BOOL  ( WINAPI * qwglSwapBuffers)(HDC);

BOOL  ( WINAPI * qwglCopyContext)(HGLRC, HGLRC, UINT);
HGLRC ( WINAPI * qwglCreateContext)(HDC);
HGLRC ( WINAPI * qwglCreateLayerContext)(HDC, int);
BOOL  ( WINAPI * qwglDeleteContext)(HGLRC);
HGLRC ( WINAPI * qwglGetCurrentContext)(VOID);
HDC   ( WINAPI * qwglGetCurrentDC)(VOID);
PROC  ( WINAPI * qwglGetProcAddress)(LPCSTR);
BOOL  ( WINAPI * qwglMakeCurrent)(HDC, HGLRC);
BOOL  ( WINAPI * qwglShareLists)(HGLRC, HGLRC);
BOOL  ( WINAPI * qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

BOOL  ( WINAPI * qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,
                                           FLOAT, int, LPGLYPHMETRICSFLOAT);

BOOL ( WINAPI * qwglDescribeLayerPlane)(HDC, int, int, UINT,
                                            LPLAYERPLANEDESCRIPTOR);
int  ( WINAPI * qwglSetLayerPaletteEntries)(HDC, int, int, int,
                                                CONST COLORREF *);
int  ( WINAPI * qwglGetLayerPaletteEntries)(HDC, int, int, int,
                                                COLORREF *);
BOOL ( WINAPI * qwglRealizeLayerPalette)(HDC, int, BOOL);
BOOL ( WINAPI * qwglSwapLayerBuffers)(HDC, UINT);

BOOL ( WINAPI * qwglSwapIntervalEXT)( int );

BOOL ( WINAPI * qwglGetDeviceGammaRampEXT ) ( unsigned char *, unsigned char *, unsigned char * );
BOOL ( WINAPI * qwglSetDeviceGammaRampEXT ) ( const unsigned char *, const unsigned char *, const unsigned char * );


#define T(x) x // type
#define N(x) ( APIENTRY * qgl ## x ) // name
#define A(x) x; // args

#include "../ref_gl/glfuncs.h"

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void )
{
	if ( glw_state.hinstOpenGL )
	{
		FreeLibrary( glw_state.hinstOpenGL );
		glw_state.hinstOpenGL = NULL;
	}

	glw_state.hinstOpenGL = NULL;

#undef T
#define T(x)
#undef N
#define N(x) qgl ## x = NULL;
#undef A
#define A(x)

#include "../ref_gl/glfuncs.h"


	qwglCopyContext              = NULL;
	qwglCreateContext            = NULL;
	qwglCreateLayerContext       = NULL;
	qwglDeleteContext            = NULL;
	qwglDescribeLayerPlane       = NULL;
	qwglGetCurrentContext        = NULL;
	qwglGetCurrentDC             = NULL;
	qwglGetLayerPaletteEntries   = NULL;
	qwglGetProcAddress           = NULL;
	qwglMakeCurrent              = NULL;
	qwglRealizeLayerPalette      = NULL;
	qwglSetLayerPaletteEntries   = NULL;
	qwglShareLists               = NULL;
	qwglSwapLayerBuffers         = NULL;
	qwglUseFontBitmaps           = NULL;
	qwglUseFontOutlines          = NULL;

	qwglChoosePixelFormat        = NULL;
	qwglDescribePixelFormat      = NULL;
	qwglGetPixelFormat           = NULL;
	qwglSetPixelFormat           = NULL;
	qwglSwapBuffers              = NULL;

	qwglSwapIntervalEXT	= NULL;

	qwglGetDeviceGammaRampEXT = NULL;
	qwglSetDeviceGammaRampEXT = NULL;
}

#	pragma warning (disable : 4113 4133 4047 )
#	define GPA( a ) GetProcAddress( glw_state.hinstOpenGL, a )

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to
** the appropriate GL stuff.  In Windows this means doing a
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
**
*/
qboolean QGL_Init( const char *dllname )
{
	static int firsttime = TRUE;

	// update 3Dfx gamma irrespective of underlying DLL
	if ( firsttime )
	{ // VC++ 2008 Express debugger memory checking failed on second entry.
		// redoing putenv()'s on video restart probably not a good idea.
		char envbuffer[1024];
		float g;

		g = 2.00 * ( 0.8 - ( vid_gamma->value - 0.5 ) ) + 1.0F;
		Com_sprintf( envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g );
		_putenv( envbuffer );
		Com_sprintf( envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g );
		_putenv( envbuffer );

		firsttime = FALSE;
	}

	if ( ( glw_state.hinstOpenGL = LoadLibrary( dllname ) ) == 0 )
	{
		char *buf = NULL;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &buf, 0, NULL);
		Com_Printf ("%s\n", buf );
		return false;
	}


#undef N
#define N(x) qgl ## x = GPA ( "gl"#x );
#include "../ref_gl/glfuncs.h"

	qwglCopyContext              = GPA( "wglCopyContext" );
	qwglCreateContext            = GPA( "wglCreateContext" );
	qwglCreateLayerContext       = GPA( "wglCreateLayerContext" );
	qwglDeleteContext            = GPA( "wglDeleteContext" );
	qwglDescribeLayerPlane       = GPA( "wglDescribeLayerPlane" );
	qwglGetCurrentContext        = GPA( "wglGetCurrentContext" );
	qwglGetCurrentDC             = GPA( "wglGetCurrentDC" );
	qwglGetLayerPaletteEntries   = GPA( "wglGetLayerPaletteEntries" );
	qwglGetProcAddress           = GPA( "wglGetProcAddress" );
	qwglMakeCurrent              = GPA( "wglMakeCurrent" );
	qwglRealizeLayerPalette      = GPA( "wglRealizeLayerPalette" );
	qwglSetLayerPaletteEntries   = GPA( "wglSetLayerPaletteEntries" );
	qwglShareLists               = GPA( "wglShareLists" );
	qwglSwapLayerBuffers         = GPA( "wglSwapLayerBuffers" );
	qwglUseFontBitmaps           = GPA( "wglUseFontBitmapsA" );
	qwglUseFontOutlines          = GPA( "wglUseFontOutlinesA" );

	qwglChoosePixelFormat        = GPA( "wglChoosePixelFormat" );
	qwglDescribePixelFormat      = GPA( "wglDescribePixelFormat" );
	qwglGetPixelFormat           = GPA( "wglGetPixelFormat" );
	qwglSetPixelFormat           = GPA( "wglSetPixelFormat" );
	qwglSwapBuffers              = GPA( "wglSwapBuffers" );

	qwglSwapIntervalEXT = 0;
	qglPointParameterfEXT = 0;
	qglPointParameterfvEXT = 0;
	qglColorTableEXT = 0;
	qglMTexCoord2fARB = 0;
	qglMTexCoord3fARB = 0;
	qglMultiTexCoord3fvARB = 0;

	return true;
}

#pragma warning (default : 4113 4133 4047 )



