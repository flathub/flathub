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

#define INCLUDE_GLX
#include <GL/glx.h>

#include <float.h>
#include "ref_gl/r_local.h"
#include "glw_unix.h"


#if defined HAVE_DLFCN_H
#include <dlfcn.h>
#endif

// FIXME HACK
void * (*qwglGetProcAddress) (const char*);


/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void )
{
	if ( glw_state.OpenGLLib )
	{
		dlclose ( glw_state.OpenGLLib );
		glw_state.OpenGLLib = NULL;
	}

	glw_state.OpenGLLib = NULL;

#undef T
#define T(x)
#undef N
#define N(x) qgl ## x = NULL;
#undef A
#define A(x)

#include "../ref_gl/glfuncs.h"

}

#define GPA( a ) dlsym( glw_state.OpenGLLib, a )

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
	// update 3Dfx gamma irrespective of underlying DLL
	{
		char envbuffer[1024];
		float g;

		g = 2.00 * ( 0.8 - ( vid_gamma->value - 0.5 ) ) + 1.0F;
		Com_sprintf( envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g );
		putenv( envbuffer );
		Com_sprintf( envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g );
		putenv( envbuffer );
	}

	if ( ( glw_state.OpenGLLib = dlopen( dllname, RTLD_LAZY | RTLD_GLOBAL ) ) == 0 )
	{
#if 1
		// basedir deprecated.
		return false;

#else
		char	fn[MAX_OSPATH];
		char	*path;
		// FILE *fp; // unused

//		Com_Printf(PRINT_ALL, "QGL_Init: Can't load %s from /etc/ld.so.conf: %s\n",
//				dllname, dlerror());

		// probably not useful in unix/linux
		// try basedir next
		path = Cvar_Get ("basedir", ".", CVAR_NOSET)->string;
		snprintf (fn, MAX_OSPATH, "%s/%s", path, dllname );
		if ( ( glw_state.OpenGLLib = dlopen( fn, RTLD_LAZY ) ) == 0 ) {
			Com_Printf( PRINT_ALL, "%s\n", dlerror() );
			return false;
		}
#endif
	}

#undef N
#define N(x) \
	qgl ## x = GPA ( "gl"#x ); \
	if (qgl ## x == NULL) \
		Com_Printf ("Could not load gl"#x"!\n");
#include "../ref_gl/glfuncs.h"

    qwglGetProcAddress = qglXGetProcAddress;

	qglLockArraysEXT = 0;
	qglUnlockArraysEXT = 0;
	qglPointParameterfEXT = 0;
	qglPointParameterfvEXT = 0;
	qglColorTableEXT = 0;
	qglMTexCoord2fARB = 0;
	qglActiveTextureARB = 0;
	qglClientActiveTextureARB = 0;
	qglMultiTexCoord3fvARB = 0;

	return true;
}
