/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2010 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

A copy of the GNU General Public License is included;
see the file, GPLv2.

*/
/*
-------------------------------------------------------------------------------
Alien Arena File System  (February 2014)

------ Microsoft Windows ------

  c:\alienarena\
	data1\
	  --- game data from release package
	  --- including  botinfo\
	arena\ and tactical\
	  default.cfg
	  maps.lst
	  server.cfg
	  motd.txt
	  config.cfg
	  autoexec.cfg
	  qconsole.log
	  --- 3rd party, legacy, and downloaded game data
	  --- screenshots, demos
	Tools\doc\
	alienarena.exe
	aa-tactical-demo.exe

------ Linux, Unix, Darwin ------
  February 2014:

  $prefix/          /usr/local/
	$bindir/          /usr/local/bin/
	  alienarena
	  alienarena-ded
	  aa-tactical-demo
	$datadir/         /usr/local/share/
	  $pkgdatadir==$COR_DATADIR  /usr/local/share/alienarena/
		data1/
		  --- shared game data from release package
		  --- including botinfo/
		arena/ and tactical/
		  default.cfg
		  maps.lst
		  server.cfg
		  motd.txt

  $XDG_DATA_HOME/cor-games   $HOME/.local/share/cor-games
	arena/ and tactical/
	  --- downloaded 3rd party, legacy game data
	  --- screenshots, demos, botinfo
	  config.cfg
	  autoexec.cfg
	  qconsole.log

--- Alien Arena ACEBOT File System ---
 -- February 2014: release package botinfo moved to data1. Custom botinfo
	placed in arena or tactical directories.

 -- botinfo files and file types --
  - allbots.tmp   : data for bots included in the Add Bot menu.
  - team.tmp      : data for default set of bots spawned for a team game.
  - <botname>.cfg : config data for bots by name.
  - <mapname>.tmp : data for the set of bots spawned in a map.

--- Original Quake File System Comments ---
All of Quake's data access is through a hierchal file system, but the contents
of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all
game directories.  The sys_* files pass this to host_init in
quakeparms_t->basedir.  This can be overridden with the "-basedir" command line
parm to allow code debugging in a different directory. The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all
generated files (savegames, screenshots, demos, config files) will be saved to.
This can be overridden with the "-game" command line parameter. The game
directory can never be changed while quake is executing. This is a precacution
against having a malicious server instruct clients to write files over areas
they shouldn't.

-------------------------------------------------------------------------------
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>

#if defined HAVE_UNISTD_H
#include <unistd.h>
#elif defined HAVE_DIRECT_H
/* for _getcwd in Windows */
#include <direct.h>
#endif

#if defined HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if defined HAVE_ZLIB
# include <zlib.h>
#endif

#include "qcommon.h"
#include "game/q_shared.h"
#include "unix/glob.h"

#if !defined HAVE__GETCWD && defined HAVE_GETCWD
#define _getcwd getcwd
#endif

/* Unix/Linux: COR_DATADIR must be defined at compile-time
 *  normally same as $pkg_datadir
 */
#if defined UNIX_VARIANT && !defined COR_DATADIR
# define COR_DATADIR "*ERROR*"
#endif

/* fs_gamedir is the game name and game subdirectory name
 * fs_gamedirvar is the cvar holding the subdirectory name
 *   for the currently running game.
 */
char fs_gamedir[MAX_OSPATH - MAX_QPATH];
cvar_t *fs_gamedirvar;

/* This is the default for the user data subdirectory.
 * Prefix with $HOME to get the full absolute path.
 */
const char* xdg_data_home_default = ".local/share";

/* --- Search Paths ---
 * Pathname strings do not include a trailing '/'
 * The last slot is guard, containing an empty string
 */
#define GAME_SEARCH_SLOTS 6
char fs_gamesearch[GAME_SEARCH_SLOTS][MAX_OSPATH];


/**
 * @brief Initialize search paths.
 *
 */
static void FS_init_paths( void )
{
	int i;
	char fs_bindir[MAX_OSPATH];
	char fs_datadir[MAX_OSPATH];
	char fs_homedir[MAX_OSPATH];
	char base_gamedata[MAX_OSPATH];
	char game_gamedata[MAX_OSPATH];
#if defined(UNIX_VARIANT) && !defined(STEAM_VARIANT)
	char user_base_gamedata[MAX_OSPATH];
	char user_game_gamedata[MAX_OSPATH];
	char xdg_data_home[MAX_OSPATH];
	char *xdg_env;
	FILE *testfile;
	char testfile_path[MAX_OSPATH];
#endif
	char *cwdstr  = NULL;
	char *homestr = NULL;

	memset( fs_bindir, 0, sizeof(fs_bindir));
	memset( fs_datadir, 0, sizeof(fs_datadir));
	memset( fs_homedir, 0, sizeof(fs_homedir));
	memset( base_gamedata, 0, sizeof(base_gamedata));
	memset( game_gamedata, 0, sizeof(game_gamedata));
#if defined(UNIX_VARIANT) && !defined(STEAM_VARIANT)
	memset( user_base_gamedata, 0, sizeof(base_gamedata));
	memset( user_game_gamedata, 0, sizeof(game_gamedata));
	memset( xdg_data_home, 0, sizeof(xdg_data_home));
#endif

	/*
	 * For Windows, CWD is both the 'datadir' and the user data directory.
	 *
	 * For Linux running in Steam, if works the same as Windows.
	 *
	 * For Unix/Linux, CWD serves as an "in-place" 'datadir' overriding COR_DATADIR
	 *  (which is equivalent to what was the "alternate install"). To activate this
	 *  option, after 'make', the executables are copied from source/ to the
	 *  topdir manually, and run in the topdir (using ./alienarena, for example.)
	 *  This is compatible with the normal install, as long as the user remembers
	 *  that the normally installed executable is the PATH and the in-place
	 *  executable is probably not. There is still a separate user data
	 *  directory (unlike Windows).
	 * Note: in-place install is a tool for developers and testers. 
	 *  It is also needed for running the aaradiant map editor.
	 * Note: if you do a regular install (ie. program is in PATH,
	 *  eg. in /usr/local/bin). It is not necessary to copy the
	 *  executable, just cd to the topdir and run from there.
	 *
	 */
	cwdstr = _getcwd( fs_bindir, sizeof(fs_bindir) );
	if ( cwdstr == NULL )
		Sys_Error( "path initialization (getcwd error: %i)", strerror(errno) );
	Q_strncpyz2( fs_datadir, fs_bindir, sizeof(fs_datadir) );

#if defined(UNIX_VARIANT) && !defined(STEAM_VARIANT)
	/* use the traditional file to check existence of data1 in the cwd */
	Com_sprintf( testfile_path, sizeof( testfile_path ), "%s/%s",
				 fs_bindir, "data1/pics/colormap.pcx"  );
	testfile = fopen( testfile_path, "r" );
	if ( testfile == NULL )
	{
		/* Normal installation */
		/* absolute path where shared data files are installed */
		if ( strnlen( COR_DATADIR, sizeof(fs_datadir) ) >= sizeof( fs_datadir ) )
			{
				Com_Printf( "\nCOR_DATADIR is: %s\n", COR_DATADIR );
				Sys_Error( "\nCOR_DATADIR path name is too long\n" );
			}
		/* fs_datadir holds the absolute path for shared data
		 * normally this is $(pkgdatadir), set in source/Makefile.am
		 */
		Q_strncpyz2( fs_datadir, COR_DATADIR, sizeof( fs_datadir ));
	}
	else
	{
		/* we are running "in-place", fs_datadir is already the absolute path
		 * to the game data. The only option for Steam.
		 */
		fclose( testfile );
	}
#endif

	/* absolute path where official game resource files are installed
	 * normally, BASE_GAMEDATA is "data1"
	 */
	Com_sprintf( base_gamedata, sizeof( base_gamedata ), "%s/%s",
				 fs_datadir, BASE_GAMEDATA );

	/* Pending sorting out multiple game and plugin game module issues,
	 * game and gamedir cvars are fixed at startup.
	 *
	 * 'alienarena-ded' uses +set game arena|tactical" on command line.
	 *  defaults to arena if not set. (Is this true in Gen3?)
	 * 
	 */
#if defined DEDICATED_ONLY
	Com_sprintf( game_gamedata, sizeof(game_gamedata), "%s/%s",
				 fs_datadir, Cvar_VariableString( "gamedir" ));
#else
	Com_sprintf( game_gamedata, sizeof(game_gamedata), "%s/%s",
				 fs_datadir, GAME_GAMEDATA );
#endif

	Com_Printf("Game sub-directory: %s\n", game_gamedata );


#if defined(UNIX_VARIANT) && !defined(STEAM_VARIANT)
	/* absolute path to user-writeable data */
	/* per spec, $XDG_DATA_HOME must be an absolute path */
	/* To override, and put this some place else for testing
	 * do this, for instance:
	 * > cd
	 * > mkdir -p --mode=0700 devtest/cor-games/arena
	 * (and copy  your test configs there)
	 * > XDG_DATA_HOME=/home/user/devtest alienarena
	 */
	xdg_env = getenv( "XDG_DATA_HOME" );
	if ( xdg_env == NULL || xdg_env[0] != '/' || xdg_env[0] == '\0')
	{
		/* XDG_DATA_HOME not set, or violates spec, use default  */
		homestr = getenv( "HOME" );
		if ( homestr != NULL && homestr[0] != '\0' )
		{
			Com_sprintf( xdg_data_home, sizeof(xdg_data_home), "%s/%s",
						 homestr, xdg_data_home_default );
		}
	}
	else
	{
		Com_sprintf( xdg_data_home, sizeof(xdg_data_home), "%s", xdg_env );
	}
	if ( xdg_data_home[0] == '/' )
	{
		int result;

		Com_sprintf( fs_homedir, sizeof( fs_homedir ), "%s/%s",
					 xdg_data_home, USER_GAMEDATA );
		Com_sprintf( user_game_gamedata, sizeof(user_game_gamedata), "%s/%s",
					 fs_homedir, fs_gamedirvar->string  );
		Com_sprintf( user_base_gamedata, sizeof(user_base_gamedata), "%s/%s",
					 fs_homedir,  BASE_GAMEDATA );

		/* if does not exist, create the directory */
		Sys_Mkdir( fs_homedir );

		/* attempt write into the directory */
		Com_sprintf( testfile_path, sizeof(testfile_path), "%s/%s",
					 fs_homedir, ".version" );
		testfile = fopen( testfile_path, "w" );
		if ( testfile == NULL  )
			Sys_Error( "Home data directory error: %s", strerror( errno ));
		result = fprintf( testfile, "%s\n", VERSION );
		if ( result < 0 )
		{
			fclose( testfile );
			Sys_Error( "Home data directory error: %s", strerror( errno ));
		}
		fclose( testfile );
		Com_Printf("User data and configuration in: %s\n", fs_homedir ); 
	}
	else
	{
		Sys_Error( "Home data directory configuration error: %s",
				   xdg_data_home );
	}
#endif	

	/*
	 * Create absolute search paths
	 * As of 2014-02 version:
	 * In the user home directory there are 2: data1 and arena -xor- tactical
	 * In the shared directory there are 2: data1 and arena -xor- tactical
	 * Preference order:
	 *   home game, shared game, home data1, shared data1
	 */
	for ( i = 0 ; i < GAME_SEARCH_SLOTS ; i++ )
	{
		memset( fs_gamesearch[i], 0, MAX_OSPATH );
	}
	i = 0;
#if defined(UNIX_VARIANT) && !defined(STEAM_VARIANT)
	Q_strncpyz2(fs_gamesearch[i++],user_game_gamedata,sizeof(fs_gamesearch[0]));
#endif
	Q_strncpyz2( fs_gamesearch[i++], game_gamedata, sizeof(fs_gamesearch[0]) );
#if defined(UNIX_VARIANT) && !defined(STEAM_VARIANT)
	Q_strncpyz2(fs_gamesearch[i++],user_base_gamedata,sizeof(fs_gamesearch[0]));
#endif
	Q_strncpyz2( fs_gamesearch[i++], base_gamedata, sizeof(fs_gamesearch[0]) );

	// set up fs_gamedir, location for writing various files
	//  this is the first directory in the search path
	Q_strncpyz2( fs_gamedir, fs_gamesearch[0], sizeof(fs_gamedir) );

}

/*
================
FS_filelength
================
*/
int FS_filelength( FILE *f )
{
	int length = -1;

#if defined HAVE_FILELENGTH

	length = filelength( fileno( f ) );

#elif defined HAVE_FSTAT

	struct stat statbfr;
	int result;

	result = fstat( fileno(f), &statbfr );
	if ( result != -1 )
	{
		length = (int)statbfr.st_size;
	}

#else

	long int pos;

	pos = ftell( f );
	fseek( f, 0L, SEEK_END );
	length = ftell( f );
	fseek( f, pos, SEEK_SET );

#endif

	return length;
}


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void FS_CreatePath (char *path)
{
	char	*ofs;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_Mkdir (path);
			*ofs = '/';
		}
	}

}


/*
===
FS_CheckFile

Given the full path to a file, find out whether or not the file actually
exists.
===
*/
#if defined HAVE_STAT

qboolean FS_CheckFile( const char * search_path )
{
	struct stat statbfr;
	int result;
	result = stat( search_path, &statbfr );
	return ( result != -1 && S_ISREG(statbfr.st_mode) );
}

#else	// HAVE_STAT

qboolean FS_CheckFile( const char * search_path )
{
	FILE *pfile;

	pfile = fopen( search_path, "rb");
	if ( ! pfile )
		return false;
	fclose( pfile );
	return true;
}

#endif	// HAVE_STAT


/*
===
FS_FullPath

Given a relative path to a file
  search for the file in the installation-dependent filesystem hierarchy
  if found, return true, with the constructed full path
  otherwise return false, with full_path set to zero-length string

===
 */
qboolean FS_FullPath( char *full_path, size_t pathsize, const char *relative_path )
{
	char search_path[MAX_OSPATH];
	char * to_search;
	qboolean found = false;

	*full_path = 0;

	if ( strlen( relative_path ) >= MAX_QPATH )
	{
		Com_DPrintf("FS_FullPath: relative path size error: %s\n", relative_path );
		return false;
	}
	to_search = &fs_gamesearch[0][0];

	while ( to_search[0] && !found )
	{
		Com_sprintf( search_path, sizeof(search_path), "%s/%s",
					to_search, relative_path);
		found = FS_CheckFile( search_path );
		to_search += MAX_OSPATH;
	}

	if ( found )
	{
		if ( strlen( search_path ) < pathsize )
		{
			Q_strncpyz2( full_path, search_path, pathsize );
			if ( developer && developer->integer == 2 )
			{ // tracing for found files, not an error.
				Com_DPrintf("FS_FullPath: found : %s\n", full_path );
			}
		}
		else
		{
			Com_DPrintf("FS_FullPath: full path size error: %s\n", search_path );
			found = false;
		}
	}
	else if ( developer && developer->integer == 2 )
	{ // tracing for not found files, not necessarily an error.
		Com_DPrintf("FS_FullPath: not found : %s\n", relative_path );
	}

	return found;
}

qboolean FS_APPDATA( char *full_path, size_t pathsize, const char *relative_path )
{
	char search_path[MAX_OSPATH];
	qboolean found = false;
	char *appData = getenv("AppData");

	*full_path = 0;

	if ( strlen( relative_path ) >= MAX_QPATH )
	{
		Com_DPrintf("FS_APPDATA: relative path size error: %s\n", relative_path );
		return false;
	}
		
	Com_sprintf( search_path, sizeof(search_path), "%s/AAWoM/%s",
					appData, relative_path);
	found = FS_CheckFile( search_path );

	if ( found )
	{
		if ( strlen( search_path ) < pathsize )
		{
			Q_strncpyz2( full_path, search_path, pathsize );
			if ( developer && developer->integer == 2 )
			{ // tracing for found files, not an error.
				Com_DPrintf("FS_APPDATA: found : %s\n", full_path );
			}
		}
		else
		{
			Com_DPrintf("FS_APPDATA: full path size error: %s\n", search_path );
			found = false;
		}
	}
	else if ( developer && developer->integer == 2 )
	{ // tracing for not found files, not necessarily an error.
		Com_DPrintf("FS_APPDATA: not found : %s\n", relative_path );
	}

	return found;
}



/*
===
 FS_FullWritePath()

  Given a relative path for a file to be written
	Using the game writeable directory
	Create any sub-directories required
	Return the generated full path for file

===
*/
void FS_FullWritePath( char *full_path, size_t pathsize, const char* relative_path)
{
	if ( strlen( relative_path ) >= MAX_QPATH )
	{
		Com_DPrintf("FS_FullPath: relative path size error: %s\n", relative_path );
		*full_path = 0;
		return;
	}

	Com_sprintf( full_path, pathsize, "%s/%s", fs_gamedir, relative_path);
	FS_CreatePath( full_path );

}

void FS_WriteAPPDATA( char *full_path, size_t pathsize, const char* relative_path)
{
	char *appData = getenv("AppData");

	if ( strlen( relative_path ) >= MAX_QPATH )
	{
		Com_DPrintf("FS_FullPath: relative path size error: %s\n", relative_path );
		*full_path = 0;
		return;
	}

	Com_sprintf( full_path, pathsize, "%s/AAWoM/%s", appData, relative_path);
	FS_CreatePath( full_path );	
}

/*
==============
FS_FCloseFile
==============
*/
void FS_FCloseFile (FILE *f)
{
	fclose (f);
}

static qboolean is_cfg(const char *filename)
{
	size_t exti = strlen(filename) - 4;
	return  !Q_strncasecmp( ".cfg", &filename[exti], 4 );
}

/*
===========
FS_FOpenFile

 Given relative path, search for a file
  if found, open it and return length
  if not found, return length -1 and NULL file

===========
*/
int FS_FOpenFile (const char *filename, FILE **file)
{
	char netpath[MAX_OSPATH];
	qboolean found = false;
	int length = -1;

	//For windows, check appData first for config files
	if(is_cfg(filename))
		found = FS_APPDATA( netpath, sizeof(netpath), filename );

	if(!found)
		found = FS_FullPath( netpath, sizeof(netpath), filename );
	if ( found )
	{
		*file = fopen( netpath, "rb" );
		if ( !(*file) )
		{
			Com_DPrintf("FS_FOpenFile: failed file open: %s:\n", netpath);
		}
		else
		{
			length = FS_filelength( *file );

			// On error, the file's length will be negative, and we probably
			// can't read from that.
			if ( length < 0 )
			{
				FS_FCloseFile( *file );
				*file = NULL;
			}
		}
	}
	else
	{
		*file = NULL;
	}

	return length;
}

/*
=================
FS_ReadFile

Properly handles partial reads
=================
*/
#define	MAX_READ	0x10000		// read in blocks of 64k
void FS_Read (void *buffer, int len, FILE *f)
{
	int		block, remaining;
	int		read;
	byte	*buf;
	int		tries;

	buf = (byte *)buffer;

	// read in chunks for progress bar
	remaining = len;
	tries = 0;
	while (remaining)
	{
		block = remaining;
		if (block > MAX_READ)
			block = MAX_READ;
		read = fread (buf, 1, block, f);
		if (read == 0)
		{
			if ( feof( f ) )
				Com_Error( ERR_FATAL, "FS_Read: premature end-of-file");
			if ( ferror( f ) )
				Com_Error( ERR_FATAL, "FS_Read: file read error");
			Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
		}
		if (read == -1)
			Com_Error (ERR_FATAL, "FS_Read: -1 bytes read");

		// do some progress bar thing here...

		remaining -= read;
		buf += read;
	}
}

/*
==================
FS_TolowerPath

Makes the path to the given file lowercase on case-sensitive systems.
Kludge for case-sensitive and case-insensitive systems inteoperability.
Background: Some people may extract game paths/files as uppercase onto their
			HDD (while their system is case-insensitive, so game will work, but
			will case trouble for case-sensitive systems if they are acting
			as servers [propagating their maps etc. in uppercase]). Indeed the
			best approach here would be to make fopen()ing always case-
			insensitive, but due to resulting complexity and fact that
			Linux people will always install the game files with correct
			name casing, this is just fine.
-JR / 20050802 / 1
==================
*/
static const char *FS_TolowerPath (const char *path)
{
	int	i = 0;
	static char buf[MAX_OSPATH]; // path can be const char *, so thats why

	do
		buf[i] = tolower(path[i]);
	while (path[i++]);

	return buf;
}

/*
============
FS_LoadFile

Given relative path
 if buffer arg is NULL, just return the file length
 otherwise,
   Z_malloc a buffer, read the file into it, return pointer to the new buffer
   The new buffer is nul-terminated; its size == returned length + 1.
   For text files, this means the buffer is nul-terminated c-string.
============
*/
int FS_LoadFile (const char *path, void **buffer)
{
	FILE	*h;
	byte	*buf = NULL;
	int		len;
	char lc_path[MAX_OSPATH];

	len = FS_FOpenFile (path, &h);

	//-JR
	if (!h)
	{
		Q_strncpyz2( lc_path, FS_TolowerPath( path ), sizeof(lc_path) );
		if ( strcmp( path, lc_path ) )
		{ // lowercase conversion changed something
			len = FS_FOpenFile( lc_path, &h);
		}
	}

	if (!h)
	{
		if (buffer)
			*buffer = NULL;
		return -1;
	}

	if (!buffer)
	{
		FS_FCloseFile (h);
		return len;
	}

	buf = Z_Malloc(len+1);
	buf[len] = 0;
	*buffer = buf;

	FS_Read (buf, len, h);

	FS_FCloseFile (h);

	return len;
}

/*
============
FS_LoadFile_TryStatic

Given relative path, and assuming statbuffer is already allocated with at
least statbuffer_len bytes,
 if buffer or statbuffer arg is NULL, just return the file length. Otherwise,
   If the file length is less than statbuffer_len, load file contents into it
   and return it in the buffer output pointer. Otherwise,
	 Z_malloc a buffer, read the file into it, return pointer to the new
	 buffer.

This is a variation on FS_LoadFile which attempts to reduce needless dynamic
memory allocations, by using a static buffer when possible.

The calling function is responsible for comparing the value of *buffer with
statbuffer, and determining whether or not to use FS_FreeFile on it.
============
*/
int FS_LoadFile_TryStatic (const char *path, void **buffer, void *statbuffer, size_t statbuffer_len)
{
	FILE	*h;
	byte	*buf = NULL;
	int		len;
	char lc_path[MAX_OSPATH];

	len = FS_FOpenFile (path, &h);

	//-JR
	if (!h)
	{
		Q_strncpyz2( lc_path, FS_TolowerPath( path ), sizeof(lc_path) );
		if ( strcmp( path, lc_path ) )
		{ // lowercase conversion changed something
			len = FS_FOpenFile( lc_path, &h);
		}
	}

	if (!h)
	{
		if (buffer)
			*buffer = NULL;
		return -1;
	}

	if (buffer == NULL || statbuffer == NULL || statbuffer_len == 0)
	{
		FS_FCloseFile (h);
		return len;
	}

	if (statbuffer_len > len)
	{
		memset (statbuffer, 0, statbuffer_len);
		buf = statbuffer;
	}
	else
	{
		buf = Z_Malloc(len+1);
	}
	buf[len] = 0;
	*buffer = buf;

	FS_Read (buf, len, h);

	FS_FCloseFile (h);

	return len;
}

/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile (void *buffer)
{
	Z_Free (buffer);
}

/*
=========
FS_FileExists

 Given relative path, search for a file
  if found, return true
  if not found, return false

========
 */
qboolean FS_FileExists(char *path)
{
	char fullpath[MAX_OSPATH];

	return ( FS_FullPath( fullpath, sizeof(fullpath), path ) );
}

/*
============
FS_Gamedir

Called to find where to write a file (demos, savegames, etc)
============
*/
const char *FS_Gamedir (void)
{
	return fs_gamedir;
}

/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec (void)
{
	char name [MAX_OSPATH];
	int i;

	// search through all the paths for an autoexec.cfg file
	for ( i = 0; fs_gamesearch[i][0] ; i++ )
	{
		Com_sprintf(name, sizeof(name), "%s/autoexec.cfg", fs_gamesearch[i] );

		if (Sys_FindFirst(name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM))
		{
			Cbuf_AddText ("exec autoexec.cfg\n");
			Sys_FindClose();
			break;
		}
		Sys_FindClose();
	}
}

/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.

================
*/
void FS_SetGamedir (char *dir)
{

	Com_DPrintf("FS_SetGamedir stub (%s).\n", dir );

#if 0
/* 2014-02
 * Stubbed out pending sorting out multiple game and game plugin
 * module issues.
 */
	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Com_Printf ("Gamedir should be a single filename, not a path\n");
		return;
	}

	if ( dedicated==NULL || !dedicated->integer )
	{
		/* for client, re-initialize */
		Cbuf_AddText ("vid_restart\nsnd_restart\n");
	}

	/* Cvar_FullSet triggers sending new game to client */
	if ( *dir = 0 || Q_strncasecmp( "data1", dir, 5 ) )
		/* "" or "data1", set to our default, which is  */
		Cvar_FullSet( "game", GAME_GAMEDATA, 
					  CVAR_LATCH|CVAR_SERVERINFO|CVAR_USERINFO );
	else
		/* otherwise, set to requested game sub-directory */
		Cvar_FullSet( "game", dir, 
					  CVAR_LATCH|CVAR_SERVERINFO|CVAR_USERINFO );

	// re-initialize search paths
	FS_init_paths();
#endif

}

/*
** FS_ListFiles
**
** IMPORTANT: does not count the guard in returned "numfiles" anymore, to
** avoid adding/subtracting 1 all the time.
*/
char **FS_ListFiles( char *findname, int *numfiles, unsigned musthave, unsigned canthave )
{
	char *namestr;
	int nfiles = 0;
	char **list = NULL; // pointer to array of pointers

	// --- count the matching files ---
	namestr = Sys_FindFirst( findname, musthave, canthave );
	while ( namestr  )
	{
		if ( namestr[ strlen( namestr )-1 ] != '.' ) // not '..' nor '.'
			nfiles++;
		namestr = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose ();

	if ( !nfiles )
		return NULL;

	*numfiles = nfiles;
	nfiles++; // add space for a guard

	// --- create the file name string array ---
	list = malloc( sizeof( char * ) * nfiles ); // array of pointers
	memset( list, 0, sizeof( char * ) * nfiles );

	namestr = Sys_FindFirst( findname, musthave, canthave );
	nfiles = 0;
	while ( namestr )
	{
		if ( namestr[ strlen(namestr) - 1] != '.' )  // not ".." nor "."
		{
			list[nfiles] = strdup( namestr );  // list[n] <- malloc'd pointer
#if defined WIN32_VARIANT
			_strlwr( list[nfiles] );
#endif
			nfiles++;
		}
		namestr = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose ();

	return list;
}

void FS_FreeFileList (char **list, int n) // jit
{
	int i;

	for (i = 0; i < n; i++)
	{
		free(list[i]);
		list[i] = 0;
	}

	free(list);
}


/*
 * FS_ListFilesInFS
 *
 * Create a list of files that match a criteria.
 *
 * Searchs are relative to the game directory and use all the search paths
 */
char **
FS_ListFilesInFS(char *findname, int *numfiles, unsigned musthave, unsigned canthave)
{
//	searchpath_t	*search;		/* Search path. */
	int		i, j;			/* Loop counters. */
	int		nfiles;			/* Number of files found. */
	int		tmpnfiles;		/* Temp number of files. */
	char		**tmplist;		/* Temporary list of files. */
	char		**list;			/* List of files found. */
	char		**new_list;
	char		path[MAX_OSPATH];	/* Temporary path. */
	int s;

	nfiles = 0;
	list = malloc(sizeof(char *));

	for ( s = 0 ; fs_gamesearch[s][0]; s++ )
	{ // for non-empty search slots
		Com_sprintf(path, sizeof(path), "%s/%s",fs_gamesearch[s], findname);
		tmplist = FS_ListFiles(path, &tmpnfiles, musthave, canthave);
		if (tmplist != NULL)
		{
			nfiles += tmpnfiles;
			new_list = realloc(list, nfiles * sizeof(char *));
			if (new_list == NULL) {
				FS_FreeFileList (tmplist, tmpnfiles);
				Com_Printf ("WARN: SYSTEM MEMORY EXHAUSTION!\n");
				break;
			}
			list = new_list;
			for (i = 0, j = nfiles - tmpnfiles; i < tmpnfiles; i++, j++)
			{ // copy from full path to relative path
				list[j] = strdup( tmplist[i] + strlen( fs_gamesearch[s] ) + 1  );
			}
			FS_FreeFileList(tmplist, tmpnfiles);
		}
	}

	/* Delete duplicates. */
	tmpnfiles = 0;
	for (i = 0; i < nfiles; i++)
	{
		if (list[i] == NULL)
			continue;
		for (j = i + 1; j < nfiles; j++)
			if (list[j] != NULL &&
				strcmp(list[i], list[j]) == 0) {
				free(list[j]);
				list[j] = NULL;
				tmpnfiles++;
			}
	}

	if (tmpnfiles > 0) {
		nfiles -= tmpnfiles;
		tmplist = malloc(nfiles * sizeof(char *));
		for (i = 0, j = 0; i < nfiles + tmpnfiles; i++)
			if (list[i] != NULL)
				tmplist[j++] = list[i];
		free(list);
		list = tmplist;
	}

	/* Add a guard. */
	if (nfiles > 0)
	{
		nfiles++;
		new_list = realloc(list, nfiles * sizeof(char *));
		if (new_list == NULL) {
			Com_Printf ("WARN: SYSTEM MEMORY EXHAUSTION!\n");
			nfiles--; // convert previous entry into a guard
		} else {
			list = new_list;
		}
		list[nfiles - 1] = NULL;
	} else
	{
		free(list);
		list = NULL;
	}

	/* IMPORTANT: Don't count the guard when returning nfiles. */
	nfiles--;

	*numfiles = nfiles;

	return (list);
}

/*
** FS_Dir_f
**
** target of "dir" command
*/
static void FS_Dir_f (void)
{
	char	*path = NULL;
	char	findname[1024];
	char	wildcard[1024] = "*.*";
	char	**dirnames;
	int		ndirs;

	if ( Cmd_Argc() != 1 )
	{
		strcpy( wildcard, Cmd_Argv( 1 ) );
	}

	while ( ( path = FS_NextPath( path ) ) != NULL )
	{
		char *tmp = findname;

		Com_sprintf( findname, sizeof(findname), "%s/%s", path, wildcard );

		// limit to game directories.
		// FIXME: figure out a way to look in botinfo directories
		if ( strstr( findname, ".." ) )
		{
			continue;
		}

		while ( *tmp != 0 )
		{
			if ( *tmp == '\\' )
				*tmp = '/';
			tmp++;
		}
		Com_Printf( "Directory of %s\n", findname );
		Com_Printf( "----\n" );

		if ( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != 0 )
		{
			int i;

			for ( i = 0; i < ndirs; i++ )
			{
				if ( strrchr( dirnames[i], '/' ) )
					Com_Printf( "%s\n", strrchr( dirnames[i], '/' ) + 1 );
				else
					Com_Printf( "%s\n", dirnames[i] );

				free( dirnames[i] );
			}
			free( dirnames );
		}
		Com_Printf( "\n" );
	};
}

/*
============
FS_Path_f

 target of "path" command

============
*/
static void FS_Path_f (void)
{
	int i;

	Com_Printf ("Game data search path:\n");
	for ( i = 0; fs_gamesearch[i][0] ; i++ )
	{
		Com_Printf( "%s/\n", fs_gamesearch[i] );
	}

}

/*
================
FS_NextPath

Allows enumerating all of the directories in the search path

 prevpath to NULL on first call, previously returned char* on subsequent
 returns NULL when done

================
*/
char *FS_NextPath (char *prevpath)
{
	char *nextpath;
	int i = 0;

	if ( prevpath == NULL )
	{ // return the first
		nextpath = fs_gamesearch[0];
	}
	else
	{ // scan the fs_gamesearch elements for an address match with prevpath
		nextpath = NULL;
		while ( prevpath != fs_gamesearch[i++] )
		{
			if ( i >= GAME_SEARCH_SLOTS )
			{
				Sys_Error("Program Error in FS_NextPath()");
			}
		}
		if ( fs_gamesearch[i][0] )
		{ // non-empty slot
			nextpath = fs_gamesearch[i];
		}
	}

	return nextpath;
}

/*
================
FS_InitFilesystem
================
*/
void FS_InitFilesystem (void)
{
	cvar_t* var;

	Cmd_AddCommand ("path", FS_Path_f);
	Cmd_AddCommand ("dir", FS_Dir_f );

#if defined DEDICATED_ONLY
	/* game from command line */
	var = Cvar_Get( "game","", CVAR_NOSET|CVAR_SERVERINFO );
	if ( var == NULL || var->string[0] == '\0' )
	{
		Cvar_FullSet( "game", "arena", CVAR_ROM|CVAR_SERVERINFO );
		var = Cvar_Get( "game", "", 0 );
	}
	Cvar_FullSet( "gamedir", var->string, CVAR_ROM|CVAR_SERVERINFO );
#else
	/* game from client/listen server preprocessor def
	 */
	Cvar_FullSet( "game", GAME_GAMEDATA, CVAR_ROM|CVAR_SERVERINFO );
	var = Cvar_Get( "game", "", 0 );
	Cvar_FullSet( "gamedir", var->string, CVAR_ROM|CVAR_SERVERINFO );
#endif

	fs_gamedirvar = Cvar_Get( "gamedir", "", 0 );
	FS_init_paths();

#if defined HAVE_ZLIB && !defined DEDICATED_ONLY
	Com_Printf("using zlib version %s\n", zlibVersion() );
#endif

}
