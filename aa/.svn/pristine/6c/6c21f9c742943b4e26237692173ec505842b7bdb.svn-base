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

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined HAVE_MREMAP
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>

#if defined HAVE_TIME_H
# include <time.h>
#elif defined HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include "unix/glob.h"
#include "qcommon/qcommon.h"


/*
 * State of currently open Hunk.
 * Valid between Hunk_Begin() and Hunk_End()
 */
byte *hunk_base = NULL;
size_t *phunk_size_store = NULL;
static const size_t hunk_header_size = 32;
byte *user_hunk_base = NULL ;
size_t rsvd_hunk_size;
size_t user_hunk_size;
size_t total_hunk_used_size;

void *Hunk_Begin (int maxsize)
{

	Com_DPrintf("Hunk_Begin:0x%X:\n", maxsize );
	if ( hunk_base != NULL )
		Com_DPrintf("Warning: Hunk_Begin: hunk_base != NULL\n");

	// reserve virtual memory space
	rsvd_hunk_size = (size_t)maxsize + hunk_header_size;
	hunk_base = mmap(0, rsvd_hunk_size, PROT_READ|PROT_WRITE,
		MAP_PRIVATE|MAP_ANON, -1, 0);
	if ( hunk_base == NULL || hunk_base == (byte *)-1)
		Sys_Error("unable to virtual allocate %d bytes", maxsize);

	total_hunk_used_size = hunk_header_size;
	user_hunk_size = 0;
	user_hunk_base = hunk_base + hunk_header_size;

	// store the size reserved for Hunk_Free()
	phunk_size_store = (size_t *)hunk_base;
	*phunk_size_store = rsvd_hunk_size; // the initial reserved size

	return user_hunk_base;
}

void *Hunk_Alloc (int size)
{
	byte *user_hunk_bfr;
	size_t user_hunk_block_size;
	size_t new_user_hunk_size;

	if ( hunk_base == NULL ) {
		Sys_Error("Program Error: Hunk_Alloc: hunk_base==NULL");
	}

	// size is sometimes odd, so increment size to even boundary to avoid
	//  odd-aligned accesses.
	user_hunk_block_size = ((size_t)size + 1) & ~0x01;
	new_user_hunk_size = user_hunk_size + user_hunk_block_size;
	total_hunk_used_size += user_hunk_block_size;

	if ( total_hunk_used_size > rsvd_hunk_size )
		Sys_Error("Program Error: Hunk_Alloc: overflow");

	user_hunk_bfr = user_hunk_base + user_hunk_size; // set base of new block
	user_hunk_size = new_user_hunk_size; // then update user hunk size

	// This will dump each allocate call. Too much info for regular use.
	//Com_DPrintf("Hunk_Alloc:%u @ %p:\n", user_hunk_block_size, user_hunk_bfr );

	return (void *)user_hunk_bfr;
}

#if defined HAVE_MREMAP
// easy version, mremap() should be available on all Linux
int Hunk_End (void)
{
	byte *remap_base;
	size_t new_rsvd_hunk_size;

	new_rsvd_hunk_size = total_hunk_used_size;
	remap_base = mremap( hunk_base, rsvd_hunk_size, new_rsvd_hunk_size, 0 );
	if ( remap_base != hunk_base ) {
		Sys_Error("Hunk_End:  Could not remap virtual block (%d)", errno);
	}
	// "close" this hunk, setting reserved size for Hunk_Free
	rsvd_hunk_size = new_rsvd_hunk_size;
	*phunk_size_store = rsvd_hunk_size;

	Com_DPrintf("Hunk_End.1:0x%X @ %p:\n", rsvd_hunk_size, remap_base );

	hunk_base = user_hunk_base = NULL;
	phunk_size_store = NULL;

	return user_hunk_size; // user buffer is user_hunk_size @ user_hunk_base
}

#else
// portable version (we hope)
int Hunk_End()
{
	size_t sys_pagesize;
	size_t rsvd_pages;
	size_t rsvd_size;
	size_t used_pages;
	size_t used_size;
	size_t unmap_size;
	void * unmap_base;

	// the portable way to get pagesize, according to documentation
	sys_pagesize = (size_t)sysconf( _SC_PAGESIZE );

	// calculate page-aligned size that was reserved
	rsvd_pages = (rsvd_hunk_size / sys_pagesize);
	if ( (rsvd_hunk_size % sys_pagesize) != 0 )
		rsvd_pages += 1;
	rsvd_size = rsvd_pages * sys_pagesize;

	// calculate page-aligned size that was used
	used_pages = total_hunk_used_size / sys_pagesize;
	if ( (total_hunk_used_size % sys_pagesize) != 0 )
		used_pages += 1;
	used_size = used_pages * sys_pagesize;

	// unmap the unused space
	if ( used_size < rsvd_size ) {
		unmap_size = rsvd_size - used_size;
		unmap_base = (void *)(hunk_base + used_size);
		if ( ( munmap( unmap_base, unmap_size )) != 0 ) {
			Com_DPrintf("Hunk_End: munmap failed [0x%X @ %p]\n",
					unmap_size, unmap_base );
			// throwing a Sys_Error is probably too drastic
			// Sys_Error("Program Error: Hunk_End: munmap failed");
		}
		else
		{
			// update size reserved for Hunk_Free
			rsvd_hunk_size = used_size;
			*phunk_size_store = rsvd_hunk_size;
		}

	}

	Com_DPrintf( "Hunk_End.2:0x%X @ %p:\n", rsvd_hunk_size, hunk_base );

	hunk_base = user_hunk_base = NULL;
	phunk_size_store = NULL;

	return user_hunk_size;
}
#endif


void Hunk_Free (void *base)
{
	byte *hunk_base;
	size_t hunk_rsvd_size;

	if ( base != NULL )
	{
		// find hunk base and retreive the hunk reserved size
		hunk_base = base - hunk_header_size;
		hunk_rsvd_size = *((size_t *)hunk_base);

		Com_DPrintf("Hunk_Free:0x%X @ %p:\n", hunk_rsvd_size, hunk_base );

		if ( munmap( hunk_base, hunk_rsvd_size ) )
			Sys_Error("Hunk_Free: munmap failed (%d)", errno);
	}
}

//===============================================================================


/*
================
Sys_Milliseconds
================
*/

int curtime; // curtime set at beginning of the main loop

#if defined HAVE_CLOCK_GETTIME
// version with more modern system time function
int Sys_Milliseconds( void )
{
	static qboolean first_time = true;
	static time_t start_secs = 0;
	int errorflag;
	struct timespec tp;
	long timeofday;

	errorflag = clock_gettime( CLOCK_REALTIME, &tp );
	if ( errorflag )
	{
		Com_Printf("Sys_Milliseconds: clock_gettime() error\n");
		timeofday = 0L;
		// fail
	}
	else if ( first_time )
	{
		start_secs = tp.tv_sec;
		timeofday = tp.tv_nsec / 1000000L;
		first_time = false;
	}
	else
	{
		timeofday = ( tp.tv_sec - start_secs ) * 1000L;
		timeofday += tp.tv_nsec / 1000000L;
	}

	return (int)timeofday;
}

#else
// version with old time function
int Sys_Milliseconds (void)
{
	struct timeval tp;
	static long secbase;
	long timeofday;

	// per documentation TZ arg is obsolete, never used, should be NULL.
	// POSIX.1-2008 recommends clock_gettime()
	gettimeofday( &tp, NULL );

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	timeofday = (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;

	return (int)timeofday;
}
#endif

void Sys_Mkdir (char *path)
{
	int result;

	result = mkdir( path, 0700 );
	// drwx------ appears to be preferred unix/linux practice for home dirs
	if ( !result )
	{ // success
		Com_Printf("Created directory %s\n", path );
	}
	else
	{
		if ( errno != EEXIST )
		{
			Com_Printf("Creating directory %s failed\n", path );
		}
	}
}

//============================================

static	char	findbase[MAX_OSPATH];
static	char	findpath[MAX_OSPATH];
static	char	findpattern[MAX_OSPATH];
static	DIR		*fdir;

static qboolean CompareAttributes(char *path, char *name,
	unsigned musthave, unsigned canthave )
{
	struct stat st;
	char fn[MAX_OSPATH];

// . and .. never match
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return false;

	return true;

	if (stat(fn, &st) == -1)
		return false; // shouldn't happen

	if ( ( st.st_mode & S_IFDIR ) && ( canthave & SFF_SUBDIR ) )
		return false;

	if ( ( musthave & SFF_SUBDIR ) && !( st.st_mode & S_IFDIR ) )
		return false;

	return true;
}

char *Sys_FindFirst (char *path, unsigned musthave, unsigned canhave)
{
	struct dirent *d;
	char *p;

	if (fdir)
		Sys_Error ("Sys_BeginFind without close");

//	COM_FilePath (path, findbase);
	strcpy(findbase, path);

	if ((p = strrchr(findbase, '/')) != NULL) {
		*p = 0;
		strcpy(findpattern, p + 1);
	} else
		strcpy(findpattern, "*");

	if (strcmp(findpattern, "*.*") == 0)
		strcpy(findpattern, "*");

	if ((fdir = opendir(findbase)) == NULL)
		return NULL;
	while ((d = readdir(fdir)) != NULL) {
		if (!*findpattern || glob_match(findpattern, d->d_name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d->d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

char *Sys_FindNext (unsigned musthave, unsigned canhave)
{
	struct dirent *d;

	if (fdir == NULL)
		return NULL;
	while ((d = readdir(fdir)) != NULL) {
		if (!*findpattern || glob_match(findpattern, d->d_name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d->d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

void Sys_FindClose (void)
{
	if (fdir != NULL)
		closedir(fdir);
	fdir = NULL;
}


//============================================


