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

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#if defined HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include "qcommon/qcommon.h"
#include "unix/rw_unix.h"

extern void HandleEvents(void);
extern void *GetGameAPI( void *import);

static qboolean stdin_enabled, stdout_enabled, stderr_enabled;
static void *game_library = NULL;

cvar_t	*nostdout;
cvar_t	*sys_ansicolor;
cvar_t	*con_ignorecolorcodes;
unsigned sys_frame_time;

/* forward reference */
void Sys_Warn( char *warning, ... );


/*----------------------------------------------------- Setuid Access ------*/

/* see 'libc' info doc,
 * section 29.6 "Setting the User ID"
 * section 29.9 "Setuid Program Example"
 *
 * 2013-12 Note: Not really needed at this time, because the game is
 * statically linked, not dll/so. Runtime dynamic linking was in the
 * code, but not tested. That could (and should) change.
 */
static uid_t ruid; /* saved 'real' user ID */
static uid_t euid; /* saved 'effective' user ID */

/**
 * Store initial state. If the program is setuid, then euid will be
 * the id with the access rights the program will use sometimes.
 * Initially, the effective id is set to the real id.
 */
static void init_setuid( void )
{
	int status;

	ruid = getuid();
	euid = geteuid();
#if defined _POSIX_SAVED_IDS
	status = seteuid( ruid );
	assert( status == 0 || errno != EINVAL );
#else
	status = setreuid( euid, ruid );
#endif
}

/*
 * switch the effective user id to the setuid user id
 */
static void do_setuid( void )
{
	int status;
#if defined _POSIX_SAVED_IDS
	status = seteuid( euid );
	assert( status == 0 || errno != EINVAL );
#else
	status = setreuid( euid, ruid );
#endif
	if ( status == -1 && errno == EPERM )
	{
		Sys_Warn("Not permitted to change user id (seteuid).");
	}
}

/*
 * switch the effective user id back to the real user id
 */
static void undo_setuid( void )
{
	int status;
#if defined _POSIX_SAVED_IDS
	status = seteuid( ruid );
	assert( status == 0 || errno != EINVAL );
#else
	status = setreuid( euid, ruid );
#endif
	assert( getuid() == geteuid() );
}

/*----------------------------------------------- Low-Level Console I/O ----*/

/**
 * if, for instance, the program is invoked through a Gnome Launcher
 * without the 'run in terminal' option, output can end up in some
 * operating system log.
 *
 * The dedicated server requires a terminal.
 * The client/listen server does not.
 *
 * Terminal output may  be disabled and re-enabled by toggling the
 * 'nostdout' cvar.
 */

/**
 * determine if there is a terminal for stdin, stdout, stderr
 */
static inline qboolean terminal_stdin_exists(void)
{
#if defined HAVE_UNISTD_H
	return isatty( fileno(stdin) );
#else
	return true;
#endif
}

static inline qboolean terminal_stdout_exists(void)
{
#if defined HAVE_UNISTD_H
	return isatty( fileno(stderr) );
#else
	return true;
#endif
}

static inline qboolean terminal_stderr_exists(void)
{
#if defined HAVE_UNISTD_H
	return isatty( fileno(stderr) );
#else
	return true;
#endif
}

/**
 * for preventing spurious output to system log when there is no terminal
 */
static inline qboolean stdout_disabled( void )
{
	return ( !stdout_enabled
			 || (nostdout && nostdout->integer)
			 || !terminal_stdout_exists() );
}

/**
 * if a console exists, non-blocking input is used to get
 * keyboard input. The 'select' function is used to determine
 * if there is keyboard input to be read.
 *
 * @return -1 on failure, non-negative flags on success
 */
static int set_nonblock_stdin( void )
{
	if ( !stdin_enabled || !terminal_stdin_exists() )
		return -1;

	int fcntl_flags = fcntl( fileno(stdin), F_GETFL, 0    );
	if ( fcntl_flags == -1 )
	{
		return -1; /* failure */
	}
	fcntl_flags |= O_NONBLOCK;

	return fcntl( fileno(stdin), F_SETFL, fcntl_flags );
}

/**
 * if a console exists, and stdin is nonblocking, this clears
 * the nonblocking state
 *
 * @return -1 on failure, non-negative flags on success
 */
static int clear_nonblock_stdin( void )
{
	/* maybe this should be done no matter what */
	/* if ( !stdin_active || !terminal_exists() ) */
	/* 	return -1; */

	int fcntl_flags = fcntl( fileno(stdin), F_GETFL, 0 );
	if ( fcntl_flags == -1 )
	{
		return -1; /* failure */
	}
	fcntl_flags &= O_NONBLOCK;

	fcntl_flags = fcntl( fileno(stdin), F_SETFL, fcntl_flags );

	return fcntl_flags;
}

/**
 * low-level, non-blocking console input.
 * called from qcommon/common.c
 *
 * @return - pointer to nul-terminated string in a static buffer
 */
char *Sys_ConsoleInput( void )
{
	static char text[256];

	int    len;
	fd_set fdset;
	struct timeval timeout;
	int    result;

	if ( !stdin_enabled )
		return NULL;

	FD_ZERO( &fdset );
	FD_SET( fileno( stdin ), &fdset);
	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;

	result = select( FD_SETSIZE, &fdset, NULL, NULL, &timeout );

	if ( result < 0 )
	{
		assert( errno == EINTR ); /* ok, signal interrupted */
		return NULL;
	}

	if ( result == 0 )
		return NULL; /* nothing to read */

	if ( !FD_ISSET( fileno(stdin), &fdset ) )
		return NULL;  

	memset( (void*)text, 0, sizeof(text) );
	len = read( fileno(stdin), text, sizeof(text) );
	if ( len < 1 )
	{
		if ( len == 0 ) /* EOF. not likely to happen. */
		{
			stdin_enabled = false;
		}
		else
		{ /* -1, mostly ok, but could be a program error */
			assert( len == EAGAIN || len == EINTR ); /* ok */
		}

		return NULL;
	}
	assert( len >= 1 );

	if ( text[len-1] == '\n' )
	{ /* nul terminate by replacing newline */
		 text[len-1] = 0;
		 /*TODO: check for problems with an empty string */
		 /* if ( text[0] == 0 )  */
		 /* { */
		 /* } */
	}
	else
	{
		if ( len >= (int)sizeof(text) )
		{ /* input is probably garbage */
			return NULL;
		}
		text[len] = 0; /* nul terminate, no newline */
	}

	return text;
}

/**
 * Output text to the console/terminal
 *
 * @parameter ostring - nul-terminated string to be output
 */
void Sys_ConsoleOutput( char *ostring )
{
	if ( stdout_disabled() )
		return;

	if ((sys_ansicolor != NULL && sys_ansicolor->integer) && 
		(con_ignorecolorcodes == NULL || !con_ignorecolorcodes->integer))
	{
		static int q3ToAnsi[ 8 ] =
		{
			30, // COLOR_BLACK
			31, // COLOR_RED
			32, // COLOR_GREEN
			33, // COLOR_YELLOW
			34, // COLOR_BLUE
			36, // COLOR_CYAN
			35, // COLOR_MAGENTA
			0   // COLOR_WHITE
		};

		while ( *ostring )
		{
			if ( *ostring == '^' && ostring[1] )
			{
				int colornum = ( ostring[1]-'0' ) & 7;
				printf( "\033[%dm", q3ToAnsi[colornum] );
				ostring += 2;
				continue;
			}
			if (*ostring == '\n')
				printf ("\033[0m\n");
			else if (*ostring == ' ')
				printf( "\033[0m " );
			else
				printf( "%c", *ostring );
			ostring++;
		}
	}
	else
	{
		fputs( ostring, stdout );
	}

}

/**
 * printf for console/terminal output
 */
void Sys_Printf( char *fmt, ... )
{
	if ( stdout_disabled() )
		return;

#ifndef NDEBUG
	/* paranoid and sanity checks */
	char *pc;
	assert( strlen(fmt) < 80 );
	for( pc=(char*)fmt ; (*pc) ; ++pc )
		assert( isprint( *pc ) );
#endif

	va_list argptr;
	char    text[256];
	int     textsize;

	va_start( argptr,fmt );
	textsize = vsnprintf( text, sizeof(text), fmt, argptr);
	va_end( argptr );

	if ( textsize >= (int)sizeof(text) )
	{
		return; /* too much */
	}

	fputs( text, stdout );
}

/*------------------------------------------------- Startup and Shutdown ----*/

/**
 * Normal program exit
 */
void Sys_Quit(void)
{
	
	clear_nonblock_stdin();

	CL_Shutdown();
	Qcommon_Shutdown();

	exit(0);
}

/**
 * System-dependent initialization.
 *
 * Called from Qcommon_Init().
 */
void Sys_Init(void)
{
}

/**
 * Somewhat controlled crash exit
 */
void Sys_Error( char *error, ... )
{
	clear_nonblock_stdin();

	CL_Shutdown();
	Qcommon_Shutdown();

	if ( stderr_enabled && terminal_stderr_exists() )
	{
		va_list     argptr;
		static char crash_string[1024];

		va_start( argptr, error );
		vsnprintf( crash_string, sizeof(crash_string), error, argptr );
		va_end( argptr );
		fprintf( stderr, "\nError: %s\n", crash_string );
	}
	/* TODO: maybe, post to syslog */

	exit (1);
}

/**
 * Non-crashing warning to stderr
 */
void Sys_Warn( char *warning, ... )
{
	if ( stderr_enabled && terminal_stderr_exists() )
	{
		va_list     argptr;
		static char warn_string[1024];

		va_start( argptr,warning );
		vsnprintf( warn_string, sizeof(warn_string), warning, argptr );
		va_end( argptr );
		fprintf( stderr, "Warning: %s", warn_string);
	}
	/* TODO: maybe, post to syslog */
}

/*---------------------------------------------------- Game Load & Unload ---*/

/* 2014-02 
 * Note: To be updated for game module plugins.
 * Removed old, untested, pretend code.
 */

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI
=================
*/
void *Sys_GetGameAPI( void *parms )
{
	void *(*ptrGetGameAPI)(void*) = NULL;

	ptrGetGameAPI = &GetGameAPI;

	return ptrGetGameAPI( parms );
}

/*--------------------------------------------------------- Other stuff ----*/

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int	Sys_FileTime (char *path)
{
	struct stat buf;

	if (stat (path,&buf) == -1)
		return -1;

	return buf.st_mtime;
}

/**
 * Not sure why this is here.
 */
void floating_point_exception_handler( int unused  )
{
//	Sys_Warn("floating point exception\n");
	signal( SIGFPE, floating_point_exception_handler );
}

/**
 *
 * Called from CL_ConnectionlessPacket()
 *
 * MS Windows does:
 * 	ShowWindow ( cl_hwnd, SW_RESTORE);
 *	SetForegroundWindow ( cl_hwnd );
 */
void Sys_AppActivate (void)
{
}

/**
 * The system-dependent keyboard/mouse input for X11
 *
 */
void Sys_SendKeyEvents( void )
{

#if !defined DEDICATED_ONLY
	HandleEvents();
#endif

	// grab frame time
	sys_frame_time = Sys_Milliseconds();
}

/*----------------------------------------------------------------- main()---*/

#if defined DEDICATED_ONLY

/**
 * main() for terminal-based dedicated servers
 */
int main( int argc, char** argv )
{
	int dtime, oldtime, newtime;

	/* init setuid access, remember real and effective user IDs */
	init_setuid();

	/* a terminal with non-blocking input is required */
	stdin_enabled = true;
	if ( set_nonblock_stdin() == -1 )
	{
		exit(1);
	}
	stdout_enabled = terminal_stdout_exists();
	stderr_enabled = terminal_stderr_exists();
	if ( !stdout_enabled || !stderr_enabled )
	{
		exit(1);
	}

	Qcommon_Init( argc, argv );

	/* TODO: add help string */
	nostdout = Cvar_Get( "nostdout", "0", CVARDOC_BOOL );
	sys_ansicolor = Cvar_Get( "sys_ansicolor", "1", CVARDOC_BOOL|CVAR_ARCHIVE );
	con_ignorecolorcodes = Cvar_Get( "con_ignorecolorcodes", "0", CVARDOC_BOOL|CVAR_ARCHIVE );

    oldtime = Sys_Milliseconds();
	for (;;)
	{
		do
		{
			newtime = Sys_Milliseconds();
			dtime = newtime - oldtime;
		} while ( dtime < 1 );
		oldtime = newtime;
		curtime = newtime; /* consistent curtime for the frame */
		
		Qcommon_Frame( dtime );

	}

	return 0; /* unreachable */
}

#else

/**
 * main() for X11-based client/listen-server. Commonly, will be run
 * using a window manager's launcher, and may not be run using 
 * a terminal.
 */
int
main( int argc, char** argv )
{
	int dtime, oldtime, newtime;

	/* init setuid access, remember real and effective user IDs */
	// init_setuid(); TBD.

	stdin_enabled  = terminal_stdin_exists();
	stdout_enabled = terminal_stdout_exists();
	stderr_enabled = terminal_stderr_exists();

	/* one strategy for handling no console terminal: redirection  */
	if ( !stdout_enabled )
	{
		if ( NULL == freopen( "/dev/null", "w", stdout ) )
			Com_DPrintf("main: stdout redirect to /dev/null failed\n");
	}
	if ( !stderr_enabled )
	{
		if ( NULL == freopen( "/dev/null", "w", stderr ) )
			Com_DPrintf("main: stderr redirect to /dev/null failed\n");
	}

	if ( stdin_enabled )
		set_nonblock_stdin();

	Qcommon_Init( argc, argv );

 	nostdout = Cvar_Get( "nostdout", "0", CVARDOC_BOOL );
 	sys_ansicolor = Cvar_Get( "sys_ansicolor", "1", CVARDOC_BOOL|CVAR_ARCHIVE );
	con_ignorecolorcodes = Cvar_Get( "con_ignorecolorcodes", "0", CVARDOC_BOOL|CVAR_ARCHIVE );

	oldtime = Sys_Milliseconds();
	for (;;)
	{
		do {
			newtime = Sys_Milliseconds();
			dtime   = newtime - oldtime;
		} while ( dtime < 1 );
		oldtime = newtime;
		curtime = newtime; /* consistent curtime for the frame */

		Qcommon_Frame( dtime );
	}

	return 0; /* unreachable */
}
#endif



