/* main.c - main program and argument processing for cpio.
   Copyright (C) 1990-1992, 2001, 2003-2007, 2009-2010, 2014-2015 Free
   Software Foundation, Inc.

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

/* Written by Phil Nelson <phil@cs.wwu.edu>,
   David MacKenzie <djm@gnu.ai.mit.edu>,
   John Oleynick <juo@klinzhai.rutgers.edu>,
   and Sergey Poznyakoff <gray@gnu.org> */

#include <system.h>
#include <paxlib.h>

#include <stdio.h>
#include <argp.h>
#include <argp-version-etc.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#include <progname.h>
#include <closeout.h>

#include "filetypes.h"
#include "cpiohdr.h"
#include "dstring.h"
#include "extern.h"
#include <rmt.h>
#include <rmt-command.h>
#include "configmake.h"

enum cpio_options {
  NO_ABSOLUTE_FILENAMES_OPTION=256,
  ABSOLUTE_FILENAMES_OPTION,
  NO_PRESERVE_OWNER_OPTION,      
  ONLY_VERIFY_CRC_OPTION,        
  RENAME_BATCH_FILE_OPTION,      
  RSH_COMMAND_OPTION,            
  QUIET_OPTION,                  
  SPARSE_OPTION,                 
  FORCE_LOCAL_OPTION,            
  DEBUG_OPTION,                  
  BLOCK_SIZE_OPTION,             
  TO_STDOUT_OPTION,
  RENUMBER_INODES_OPTION,
  IGNORE_DEVNO_OPTION,
  DEVICE_INDEPENDENT_OPTION
};

const char *program_authors[] =
  {
    "Phil Nelson",
    "David MacKenzie",
    "John Oleynick",
    "Sergey Poznyakoff",
    NULL
  };
  
const char *argp_program_bug_address = "<" PACKAGE_BUGREPORT ">";
static char doc[] = N_("GNU `cpio' copies files to and from archives\n\
\n\
Examples:\n\
  # Copy files named in name-list to the archive\n\
  cpio -o < name-list [> archive]\n\
  # Extract files from the archive\n\
  cpio -i [< archive]\n\
  # Copy files named in name-list to destination-directory\n\
  cpio -p destination-directory < name-list\n");

static void usage (int status);

/*  Print usage error message and exit with error.  */

#define CHECK_USAGE(cond, opt, mode_opt) \
 if (cond) \
   USAGE_ERROR ((0, 0, _("%s is meaningless with %s"), opt, mode_opt));

static struct argp_option options[] = {
  /* ********** */
#define GRID 10
  {NULL, 0, NULL, 0,
   N_("Main operation mode:"), GRID },
  {"create", 'o', 0, 0,
   N_("Create the archive (run in copy-out mode)"), GRID },
  {"extract", 'i', 0, 0,
   N_("Extract files from an archive (run in copy-in mode)"), GRID },
  {"pass-through", 'p', 0, 0,
   N_("Run in copy-pass mode"), GRID },
  {"list", 't', 0, 0,
   N_("Print a table of contents of the input"), GRID },
#undef GRID
  
  /* ********** */
#define GRID 100  
  {NULL, 0, NULL, 0,
   N_("Operation modifiers valid in any mode:"), GRID },

  {"directory", 'D', N_("DIR"), 0,
   N_("Change to directory DIR"), GRID+1 },
  
  {"force-local", FORCE_LOCAL_OPTION, 0, 0,
   N_("Archive file is local, even if its name contains colons"), GRID+1 },
  {"format", 'H', N_("FORMAT"), 0,
   N_("Use given archive FORMAT"), GRID+1 },
  {NULL, 'B', NULL, 0,
   N_("Set the I/O block size to 5120 bytes"), GRID+1 },
  {"block-size", BLOCK_SIZE_OPTION, N_("BLOCK-SIZE"), 0,
   N_("Set the I/O block size to BLOCK-SIZE * 512 bytes"), GRID+1 },
  {NULL, 'c', NULL, 0,
   N_("Use the old portable (ASCII) archive format"), GRID+1 },
  {"dot", 'V', NULL, 0, 
   N_("Print a \".\" for each file processed"), GRID+1 },
  {"io-size", 'C', N_("NUMBER"), 0,
   N_("Set the I/O block size to the given NUMBER of bytes"), GRID+1 },
  {"quiet", QUIET_OPTION, NULL, 0,
   N_("Do not print the number of blocks copied"), GRID+1 },
  {"verbose", 'v', NULL, 0,
   N_("Verbosely list the files processed"), GRID+1 },
#ifdef DEBUG_CPIO
  {"debug", DEBUG_OPTION, NULL, 0,
   N_("Enable debugging info"), GRID+1 },
#endif
  {"warning", 'W', N_("FLAG"), 0,
   N_("Control warning display. Currently FLAG is one of 'none', 'truncate', 'all'. Multiple options accumulate."), GRID+1 },
  {"owner", 'R', N_("[USER][:.][GROUP]"), 0,
   N_("Set the ownership of all files created to the specified USER and/or GROUP"), GRID+1 },
#undef GRID

#define GRID 110
  {NULL, 0, NULL, 0,
   N_("Operation modifiers valid in copy-in and copy-out modes"), GRID },
  {"file", 'F', N_("[[USER@]HOST:]FILE-NAME"), 0,
   N_("Use this FILE-NAME instead of standard input or output. Optional USER and HOST specify the user and host names in case of a remote archive"), GRID+1 },
  {"message", 'M', N_("STRING"), 0,
   N_("Print STRING when the end of a volume of the backup media is reached"),
   GRID+1 },
  {"rsh-command", RSH_COMMAND_OPTION, N_("COMMAND"), 0,
   N_("Use COMMAND instead of rsh"), GRID+1 },
#undef GRID
  
  /* ********** */
#define GRID 200  
  {NULL, 0, NULL, 0,
   N_("Operation modifiers valid only in copy-in mode:"), GRID },
  {"nonmatching", 'f', 0, 0,
   N_("Only copy files that do not match any of the given patterns"), GRID+1 },
  {"numeric-uid-gid", 'n', 0, 0,
   N_("In the verbose table of contents listing, show numeric UID and GID"),
   GRID+1 },
  {"pattern-file", 'E', N_("FILE"), 0,
   N_("Read additional patterns specifying filenames to extract or list from FILE"), 210},
  {"only-verify-crc", ONLY_VERIFY_CRC_OPTION, 0, 0,
   N_("When reading a CRC format archive, only verify the CRC's of each file in the archive, don't actually extract the files"), 210},
  {"rename", 'r', 0, 0,
   N_("Interactively rename files"), GRID+1 },
  {"rename-batch-file", RENAME_BATCH_FILE_OPTION, N_("FILE"), OPTION_HIDDEN,
   "", GRID+1 },
  {"swap", 'b', NULL, 0,
   N_("Swap both halfwords of words and bytes of halfwords in the data. Equivalent to -sS"), GRID+1 },
  {"swap-bytes", 's', NULL, 0,
   N_("Swap the bytes of each halfword in the files"), GRID+1 },
  {"swap-halfwords", 'S', NULL, 0,
   N_("Swap the halfwords of each word (4 bytes) in the files"),
   GRID+1 },
  {"to-stdout", TO_STDOUT_OPTION, NULL, 0,
   N_("Extract files to standard output"), GRID+1 },
  {NULL, 'I', N_("[[USER@]HOST:]FILE-NAME"), 0,
   N_("Archive filename to use instead of standard input. Optional USER and HOST specify the user and host names in case of a remote archive"), GRID+1 },
#undef GRID   

  /* ********** */
#define GRID 300
  {NULL, 0, NULL, 0,
   N_("Operation modifiers valid only in copy-out mode:"), GRID },
  {"append", 'A', 0, 0,
   N_("Append to an existing archive."), GRID+1 },
  {NULL, 'O', N_("[[USER@]HOST:]FILE-NAME"), 0,
   N_("Archive filename to use instead of standard output. Optional USER and HOST specify the user and host names in case of a remote archive"), GRID+1 },
  {"renumber-inodes", RENUMBER_INODES_OPTION, NULL, 0,
   N_("Renumber inodes") },
  {"ignore-devno", IGNORE_DEVNO_OPTION, NULL, 0,
   N_("Don't store device numbers") },
  {"device-independent", DEVICE_INDEPENDENT_OPTION, NULL, 0,
   N_("Create device-independent (reproducible) archives") },
  {"reproducible", 0, NULL, OPTION_ALIAS },
#undef GRID
  
  /* ********** */
#define GRID 400
  {NULL, 0, NULL, 0,
   N_("Operation modifiers valid only in copy-pass mode:"), GRID},
  {"link", 'l', 0, 0,
   N_("Link files instead of copying them, when  possible"), GRID+1 },

#undef GRID
  
  /* ********** */
#define GRID 500
  {NULL, 0, NULL, 0,
   N_("Operation modifiers valid in copy-in and copy-out modes:"), GRID },
  {"absolute-filenames", ABSOLUTE_FILENAMES_OPTION, 0, 0,
   N_("Do not strip file system prefix components from the file names"),
   GRID+1 },
  {"no-absolute-filenames", NO_ABSOLUTE_FILENAMES_OPTION, 0, 0,
   N_("Create all files relative to the current directory"), GRID+1 },
#undef GRID  
  /* ********** */
#define GRID 600
  {NULL, 0, NULL, 0,
   N_("Operation modifiers valid in copy-out and copy-pass modes:"), GRID },
  {"null", '0', 0, 0,
   N_("Filenames in the list are delimited by null characters instead of newlines"), GRID+1 },
  {"dereference", 'L', 0, 0,
   N_("Dereference  symbolic  links  (copy  the files that they point to instead of copying the links)."), GRID+1 },
  {"reset-access-time", 'a', NULL, 0,
   N_("Reset the access times of files after reading them"), GRID+1 },

#undef GRID   
  /* ********** */
#define GRID 700
  {NULL, 0, NULL, 0,
   N_("Operation modifiers valid in copy-in and copy-pass modes:"), GRID },
  {"preserve-modification-time", 'm', 0, 0,
   N_("Retain previous file modification times when creating files"), GRID+1 },
  {"make-directories", 'd', 0, 0,
   N_("Create leading directories where needed"), GRID+1 },
  {"no-preserve-owner", NO_PRESERVE_OWNER_OPTION, 0, 0,
   N_("Do not change the ownership of the files"), GRID+1 },
  {"unconditional", 'u', NULL, 0,
   N_("Replace all files unconditionally"), GRID+1 },
  {"sparse", SPARSE_OPTION, NULL, 0,
   N_("Write files with large blocks of zeros as sparse files"), GRID+1 },
#undef GRID
  
  {0, 0, 0, 0}
};

static char *input_archive_name = 0;
static char *output_archive_name = 0;

static int
warn_control (char *arg)
{
  static struct warn_tab {
    char *name;
    int flag;
  } warn_tab[] = {
    { "none",       CPIO_WARN_ALL       },
    { "truncate",   CPIO_WARN_TRUNCATE  },
    { "all",        CPIO_WARN_ALL       },
    { "interdir",   CPIO_WARN_INTERDIR  },
    { NULL }
  };
  struct warn_tab *wt;
  int offset = 0;

  if (strcmp (arg, "none") == 0)
    {
      warn_option = 0;
      return 0;
    }
  
  if (strlen (arg) > 2 && memcmp (arg, "no-", 3) == 0)
    offset = 3;
      
  for (wt = warn_tab; wt->name; wt++)
    if (strcmp (arg + offset, wt->name) == 0)
      {
	if (offset)
	  warn_option &= ~wt->flag;
	else
	  warn_option |= wt->flag;
	return 0;
      }

  return 1;
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case '0':		/* Read null-terminated filenames.  */
      name_end = '\0';
      break;

    case 'a':		/* Reset access times.  */
      reset_time_flag = true;
      break;

    case 'A':		/* Append to the archive.  */
      append_flag = true;
      break;

    case 'b':		/* Swap bytes and halfwords.  */
      swap_bytes_flag = true;
      swap_halfwords_flag = true;
      break;

    case 'B':		/* Set block size to 5120.  */
      io_block_size = 5120;
      break;

    case BLOCK_SIZE_OPTION:		/* --block-size */
      io_block_size = atoi (arg);
      if (io_block_size < 1)
	USAGE_ERROR ((0, 0, _("invalid block size")));
      io_block_size *= 512;
      break;

    case 'c':		/* Use the old portable ASCII format.  */
      if (archive_format != arf_unknown)
	USAGE_ERROR ((0, 0, _("Archive format multiply defined")));
#ifdef SVR4_COMPAT
      archive_format = arf_newascii; /* -H newc.  */
#else
      archive_format = arf_oldascii; /* -H odc.  */
#endif
      break;

    case 'C':		/* Block size.  */
      io_block_size = atoi (arg);
      if (io_block_size < 1)
	USAGE_ERROR ((0, 0, _("invalid block size")));
      break;

    case 'd':		/* Create directories where needed.  */
      create_dir_flag = true;
      break;

    case 'D':
      change_directory_option = arg;
      break;
      
    case 'f':		/* Only copy files not matching patterns.  */
      copy_matching_files = false;
      break;

    case 'E':		/* Pattern file name.  */
      pattern_file_name = arg;
      break;

    case 'F':		/* Archive file name.  */
      archive_name = arg;
      break;

    case 'H':		/* Header format name.  */
      if (archive_format != arf_unknown)
	USAGE_ERROR ((0, 0, _("Archive format multiply defined")));
      if (!strcasecmp (arg, "crc"))
	archive_format = arf_crcascii;
      else if (!strcasecmp (arg, "newc"))
	archive_format = arf_newascii;
      else if (!strcasecmp (arg, "odc"))
	archive_format = arf_oldascii;
      else if (!strcasecmp (arg, "bin"))
	archive_format = arf_binary;
      else if (!strcasecmp (arg, "ustar"))
	archive_format = arf_ustar;
      else if (!strcasecmp (arg, "tar"))
	archive_format = arf_tar;
      else if (!strcasecmp (arg, "hpodc"))
	archive_format = arf_hpoldascii;
      else if (!strcasecmp (arg, "hpbin"))
	archive_format = arf_hpbinary;
      else
	USAGE_ERROR ((0, 0, _("\
invalid archive format `%s'; valid formats are:\n\
crc newc odc bin ustar tar (all-caps also recognized)"), arg));
      break;
	  
    case 'i':		/* Copy-in mode.  */
      if (copy_function != 0)
	USAGE_ERROR ((0, 0, _("Mode already defined")));
      copy_function = process_copy_in;
      break;

    case 'I':		/* Input archive file name.  */
      input_archive_name = arg;
      break;

    case 'l':		/* Link files when possible.  */
      link_flag = true;
      break;

    case 'L':		/* Dereference symbolic links.  */
      xstat = stat;
      break;

    case 'm':		/* Retain previous file modify times.  */
      retain_time_flag = true;
      break;

    case 'M':		/* New media message.  */
      set_new_media_message (arg);
      break;

    case 'n':		/* Long list owner and group as numbers.  */
      numeric_uid = true;
      break;

    case NO_ABSOLUTE_FILENAMES_OPTION:		/* --no-absolute-filenames */
      no_abs_paths_flag = true;
      break;

    case ABSOLUTE_FILENAMES_OPTION:		/* --absolute-filenames */
      no_abs_paths_flag = false;
      break;
      
    case NO_PRESERVE_OWNER_OPTION:		/* --no-preserve-owner */
      if (set_owner_flag || set_group_flag)
	USAGE_ERROR ((0, 0,  
		      _("--no-preserve-owner cannot be used with --owner")));
      no_chown_flag = true;
      break;

    case 'o':		/* Copy-out mode.  */
      if (copy_function != 0)
	USAGE_ERROR ((0, 0, _("Mode already defined")));
      copy_function = process_copy_out;
      break;

    case 'O':		/* Output archive file name.  */
      output_archive_name = arg;
      break;

    case ONLY_VERIFY_CRC_OPTION:
      only_verify_crc_flag = true;
      break;

    case 'p':		/* Copy-pass mode.  */
      if (copy_function != 0)
	USAGE_ERROR ((0, 0, _("Mode already defined")));
      copy_function = process_copy_pass;
      break;

    case IGNORE_DEVNO_OPTION:
      ignore_devno_option = 1;
      break;
      
    case RENUMBER_INODES_OPTION:
      renumber_inodes_option = 1;
      break;

    case DEVICE_INDEPENDENT_OPTION:
      ignore_devno_option = renumber_inodes_option = 1;
      break;
      
    case RSH_COMMAND_OPTION:
      rsh_command_option = arg;
      break;

    case 'r':		/* Interactively rename.  */
      rename_flag = true;
      break;

    case RENAME_BATCH_FILE_OPTION:
      rename_batch_file = arg;
      break;

    case QUIET_OPTION:
      quiet_flag = true;
      break;

    case 'R':		/* Set the owner.  */
      if (no_chown_flag)
	USAGE_ERROR ((0, 0,  
		      _("--owner cannot be used with --no-preserve-owner")));
      else
	{
	  char *e, *u, *g;
	  
	  e = parse_user_spec (arg, &set_owner, &set_group, &u, &g);
	  if (e)
	    USAGE_ERROR ((0, 0, "%s: %s", arg, e));
	  if (u)
	    {
	      free (u);
	      set_owner_flag = true;
	    }
	  if (g)
	    {
	      free (g);
	      set_group_flag = true;
	    }
	}
      break;

    case 's':		/* Swap bytes.  */
      swap_bytes_flag = true;
      break;

    case 'S':		/* Swap halfwords.  */
      swap_halfwords_flag = true;
      break;

    case 't':		/* Only print a list.  */
      table_flag = true;
      break;

    case 'u':		/* Replace all!  Unconditionally!  */
      unconditional_flag = true;
      break;

    case 'v':		/* Verbose!  */
      verbose_flag = true;
      break;

    case 'V':		/* Print `.' for each file.  */
      dot_flag = true;
      break;

    case 'W':
      if (warn_control (arg))
	USAGE_ERROR ((0, 0, _("Invalid value for --warning option: %s"), arg));
      break;
      
    case SPARSE_OPTION:
      sparse_flag = true;
      break;

    case FORCE_LOCAL_OPTION:
      force_local_option = 1;
      break;

#ifdef DEBUG_CPIO
    case DEBUG_OPTION:
      debug_flag = true;
      break;
#endif

    case TO_STDOUT_OPTION:
      to_stdout_option = true;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  N_("[destination-directory]"),
  doc,
  NULL,
  NULL,
  NULL
};

static void
usage (int status)
{
  argp_help (&argp, stderr, ARGP_HELP_SEE, (char*) program_name);
  close_stdout ();
  exit (status);
}

/* Process the arguments.  Set all options and set up the copy pass
   directory or the copy in patterns.  */

void
process_args (int argc, char *argv[])
{
  void (*copy_in) ();		/* Work around for pcc bug.  */
  void (*copy_out) ();
  int index;

  xstat = lstat;

  if (argp_parse (&argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    exit (PAXEXIT_FAILURE); 

  /* Do error checking and look at other args.  */

  if (copy_function == 0)
    {
      if (table_flag)
	copy_function = process_copy_in;
      else
	USAGE_ERROR ((0, 0,
	       _("You must specify one of -oipt options.\nTry `%s --help' or `%s --usage' for more information.\n"),
		      program_name, program_name));
    }

  /* Work around for pcc bug.  */
  copy_in = process_copy_in;
  copy_out = process_copy_out;

  if (copy_function == copy_in)
    {
      archive_des = 0;
      CHECK_USAGE (link_flag, "--link", "--extract");
      CHECK_USAGE (reset_time_flag, "--reset", "--extract");
      CHECK_USAGE (xstat != lstat, "--dereference", "--extract");
      CHECK_USAGE (append_flag, "--append", "--extract");
      CHECK_USAGE (output_archive_name, "-O", "--extract");
      CHECK_USAGE (renumber_inodes_option, "--renumber-inodes", "--extract");
      CHECK_USAGE (ignore_devno_option, "--ignore-devno", "--extract");
      if (to_stdout_option)
	{
	  CHECK_USAGE (create_dir_flag, "--make-directories", "--to-stdout");
	  CHECK_USAGE (rename_flag, "--rename", "--to-stdout");
	  CHECK_USAGE (no_chown_flag, "--no-preserve-owner", "--to-stdout");
	  CHECK_USAGE (set_owner_flag||set_group_flag,
		       "--owner", "--to-stdout");
	  CHECK_USAGE (retain_time_flag, "--preserve-modification-time",
		       "--to-stdout");
	}
      
      if (archive_name && input_archive_name)
	USAGE_ERROR ((0, 0,  
		      _("Both -I and -F are used in copy-in mode")));

      if (archive_format == arf_crcascii)
	crc_i_flag = true;
      num_patterns = argc - index;
      save_patterns = &argv[index];
      if (input_archive_name)
	archive_name = input_archive_name;
    }
  else if (copy_function == copy_out)
    {
      if (index != argc)
	USAGE_ERROR ((0, 0, _("Too many arguments")));

      archive_des = 1;
      CHECK_USAGE (create_dir_flag, "--make-directories", "--create");
      CHECK_USAGE (rename_flag, "--rename", "--create");
      CHECK_USAGE (table_flag, "--list", "--create");
      CHECK_USAGE (unconditional_flag, "--unconditional", "--create");
      CHECK_USAGE (link_flag, "--link", "--create");
      CHECK_USAGE (sparse_flag, "--sparse", "--create");
      CHECK_USAGE (retain_time_flag, "--preserve-modification-time",
		   "--create");
      CHECK_USAGE (no_chown_flag, "--no-preserve-owner", "--create");
      CHECK_USAGE (swap_bytes_flag, "--swap-bytes (--swap)", "--create");
      CHECK_USAGE (swap_halfwords_flag, "--swap-halfwords (--swap)",
		   "--create");
      CHECK_USAGE (to_stdout_option, "--to-stdout", "--create");
      
      if (append_flag && !(archive_name || output_archive_name))
	USAGE_ERROR ((0, 0,  
		      _("--append is used but no archive file name "
			"is given (use -F or -O options)")));

      CHECK_USAGE (rename_batch_file, "--rename-batch-file", "--create");
      CHECK_USAGE (input_archive_name, "-I", "--create");
      if (archive_name && output_archive_name)
	USAGE_ERROR ((0, 0,  
		      _("Both -O and -F are used in copy-out mode")));

      if (archive_format == arf_unknown)
	archive_format = arf_binary;
      if (output_archive_name)
	archive_name = output_archive_name;

      if (!arf_stores_inode_p (archive_format))
	renumber_inodes_option = ignore_devno_option = 0;
    }
  else
    {
      /* Copy pass.  */
      if (index < argc - 1)
	USAGE_ERROR ((0, 0, _("Too many arguments")));
      else if (index > argc - 1)
	USAGE_ERROR ((0, 0, _("Not enough arguments")));

      if (archive_format != arf_unknown)
	USAGE_ERROR ((0, 0,  
		      _("Archive format is not specified in copy-pass mode "
			"(use --format option)")));

      CHECK_USAGE (swap_bytes_flag, "--swap-bytes (--swap)", "--pass-through");
      CHECK_USAGE (swap_halfwords_flag, "--swap-halfwords (--swap)",
		   "--pass-through");
      CHECK_USAGE (table_flag, "--list", "--pass-through");
      CHECK_USAGE (rename_flag, "--rename", "--pass-through");
      CHECK_USAGE (append_flag, "--append", "--pass-through");
      CHECK_USAGE (rename_batch_file, "--rename-batch-file", "--pass-through");
      CHECK_USAGE (no_abs_paths_flag, "--no-absolute-pathnames",
		   "--pass-through");
      CHECK_USAGE (no_abs_paths_flag, "--absolute-pathnames",
		   "--pass-through");
      CHECK_USAGE (to_stdout_option, "--to-stdout", "--pass-through");
      CHECK_USAGE (renumber_inodes_option, "--renumber-inodes",
		   "--pass-through");
      CHECK_USAGE (ignore_devno_option, "--ignore-devno", "--pass-through");
      
      directory_name = argv[index];
    }

  if (archive_name)
    {
      if (copy_function != copy_in && copy_function != copy_out)
	error (PAXEXIT_FAILURE, 0, 
	       _("-F can be used only with --create or --extract"));
      archive_des = open_archive (archive_name);
      if (archive_des < 0)
	error (PAXEXIT_FAILURE, errno, _("Cannot open %s"), 
               quotearg_colon (archive_name));
    }
		     
  /* Prevent SysV non-root users from giving away files inadvertantly.
     This happens automatically on BSD, where only root can give
     away files.  */
  if (set_owner_flag == false && set_group_flag == false && geteuid ())
    no_chown_flag = true;
}

/* Initialize the input and output buffers to their proper size and
   initialize all variables associated with the input and output
   buffers.  */

void
initialize_buffers ()
{
  int in_buf_size, out_buf_size;

  if (copy_function == process_copy_in)
    {
      /* Make sure the input buffer can always hold 2 blocks and that it
	 is big enough to hold 1 tar record (512 bytes) even if it
	 is not aligned on a block boundary.  The extra buffer space
	 is needed by process_copyin and peek_in_buf to automatically
	 figure out what kind of archive it is reading.  */
      if (io_block_size >= 512)
	in_buf_size = 2 * io_block_size;
      else
	in_buf_size = 1024;
      out_buf_size = DISK_IO_BLOCK_SIZE;
    }
  else if (copy_function == process_copy_out)
    {
      in_buf_size = DISK_IO_BLOCK_SIZE;
      out_buf_size = io_block_size;
    }
  else
    {
      in_buf_size = DISK_IO_BLOCK_SIZE;
      out_buf_size = DISK_IO_BLOCK_SIZE;
    }

  input_buffer = (char *) xmalloc (in_buf_size);
  in_buff = input_buffer;
  input_buffer_size = in_buf_size;
  input_size = 0;
  input_bytes = 0;

  output_buffer = (char *) xmalloc (out_buf_size);
  out_buff = output_buffer;
  output_size = 0;
  output_bytes = 0;
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  set_program_name (argv[0]);
  argp_version_setup ("cpio", program_authors);
  process_args (argc, argv);
  
  initialize_buffers ();

  (*copy_function) ();

  if (archive_des >= 0 && rmtclose (archive_des) == -1)
    error (PAXEXIT_FAILURE, errno, _("error closing archive"));

  pax_exit ();
}
