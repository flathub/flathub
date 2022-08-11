/* makepath.c -- Ensure that a directory path exists.
   Copyright (C) 1990, 2006-2007, 2010, 2014-2015 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA.  */

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> and
   Jim Meyering <meyering@cs.utexas.edu>.  */

/* This copy of makepath is almost like the fileutils one, but has
   changes for HPUX CDF's.  Maybe the 2 versions of makepath can
   come together again in the future.  */

#include <system.h>
#include <paxlib.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cpiohdr.h"
#include "dstring.h"
#include "extern.h"

/* Ensure that the directory ARGPATH exists.
   Remove any trailing slashes from ARGPATH before calling this function.

   Make all directory components that don't already exist with
   permissions 700.
   If OWNER and GROUP are non-negative, make them the UID and GID of
   created directories.
   If VERBOSE_FMT_STRING is nonzero, use it as a printf format
   string for printing a message after successfully making a directory,
   with the name of the directory that was just made as an argument.

   Return 0 if ARGPATH exists as a directory with the proper
   ownership and permissions when done, otherwise 1.  */

int
make_path (char *argpath,
	   uid_t owner,
	   gid_t group,
	   const char *verbose_fmt_string)
{
  char *dirpath;		/* A copy we can scribble NULs on.  */
  struct stat stats;
  int retval = 0;
  mode_t tmpmode;
  mode_t invert_permissions;
  int we_are_root = getuid () == 0;
  dirpath = alloca (strlen (argpath) + 1);

  strcpy (dirpath, argpath);

  if (stat (dirpath, &stats))
    {
      tmpmode = MODE_RWX & ~ newdir_umask;
      invert_permissions = we_are_root ? 0 : MODE_WXUSR & ~ tmpmode;

      char *slash = dirpath;
      while (*slash == '/')
	slash++;
      while ((slash = strchr (slash, '/')))
	{
#ifdef HPUX_CDF
	  int	iscdf;
	  iscdf = 0;
#endif
	  *slash = '\0';
	  if (stat (dirpath, &stats))
	    {
#ifdef HPUX_CDF
	      /* If this component of the pathname ends in `+' and is
		 followed by 2 `/'s, then this is a CDF.  We remove the
		 `+' from the name and create the directory.  Later
		 we will "hide" the directory.  */
	      if ( (*(slash +1) == '/') && (*(slash -1) == '+') )
		{ 
		  iscdf = 1;
		  *(slash -1) = '\0';
		}
#endif
	      if (mkdir (dirpath, tmpmode ^ invert_permissions))
		{
		  error (0, errno, _("cannot make directory `%s'"), dirpath);
		  return 1;
		}
	      else
		{
		  if (verbose_fmt_string != NULL)
		    error (0, 0, verbose_fmt_string, dirpath);

		  if (stat (dirpath, &stats))
		    stat_error (dirpath);
		  else
		    {
		      if (owner != -1)
			stats.st_uid = owner;
		      if (group != -1)
			stats.st_gid = group;
		      
		      delay_set_stat (dirpath, &stats, invert_permissions);
		    }
		  
#ifdef HPUX_CDF
		  if (iscdf)
		    {
		      /*  If this is a CDF, "hide" the directory by setting
			  its hidden/setuid bit.  Also add the `+' back to
			  its name (since once it's "hidden" we must refer
			  to as `name+' instead of `name').  */
		      chmod (dirpath, 04700);
		      *(slash - 1) = '+';
		    }
#endif
		}
	    }
	  else if (!S_ISDIR (stats.st_mode))
	    {
	      error (0, 0, _("`%s' exists but is not a directory"), dirpath);
	      return 1;
	    }

	  *slash++ = '/';

	  /* Avoid unnecessary calls to `stat' when given
	     pathnames containing multiple adjacent slashes.  */
	  while (*slash == '/')
	    slash++;
	}

      /* We're done making leading directories.
	 Make the final component of the path. */

      if (mkdir (dirpath, tmpmode ^ invert_permissions))
	{
	  /* In some cases, if the final component in dirpath was `.' then we 
	     just got an EEXIST error from that last mkdir().  If that's
	     the case, ignore it.  */
	  if ( (errno != EEXIST) ||
	       (stat (dirpath, &stats) != 0) ||
	       (!S_ISDIR (stats.st_mode) ) )
	    {
	      error (0, errno, _("cannot make directory `%s'"), dirpath);
	      return 1;
	    }
	}
      else if (stat (dirpath, &stats))
	stat_error (dirpath);
      else
	{
	  if (owner != -1)
	    stats.st_uid = owner;
	  if (group != -1)
	    stats.st_gid = group;
	  
	  delay_set_stat (dirpath, &stats, invert_permissions);
	}
	
      if (verbose_fmt_string != NULL)
	error (0, 0, verbose_fmt_string, dirpath);

    }
  else
    {
      /* We get here if the entire path already exists.  */

      if (!S_ISDIR (stats.st_mode))
	{
	  error (0, 0, _("`%s' exists but is not a directory"), dirpath);
	  return 1;
	}

    }

  return retval;
}
