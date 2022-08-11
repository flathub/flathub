/* This file is part of GNU Paxutils.
   Copyright (C) 2009 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "system.h"
#include "system-ioctl.h"
#include <configmake.h>
#include <argp.h>
#include <argp-version-etc.h>
#include <getopt.h>
#include <full-write.h>
#include <configmake.h>
#include <inttostr.h>
#include <error.h>
#include <progname.h>
#include <c-ctype.h>
#include <safe-read.h>

#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif
#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif


int dbglev;
FILE *dbgout;

#define DEBUG(lev,msg)						\
  do { if (dbgout && (lev) <= dbglev) fprintf (dbgout, "%s", msg); } while (0)
#define DEBUG1(lev, fmt, x)						\
  do { if (dbgout && (lev) <= dbglev) fprintf (dbgout, fmt, x); } while (0)
#define DEBUG2(lev, fmt, x1, x2)					\
  do									\
    {									\
      if (dbgout && (lev) <= dbglev)					\
	fprintf (dbgout, fmt, x1, x2);					\
    }									\
  while (0)

#define VDEBUG(lev, pfx, fmt, ap)		\
  do						\
    {						\
      if (dbgout && (lev) <= dbglev)		\
	{					\
	  fprintf (dbgout, "%s", pfx);		\
	  vfprintf (dbgout, fmt, ap);		\
	}					\
    }						\
  while (0)



static void
trimnl (char *str)
{
  if (str)
    {
      size_t len = strlen (str);
      if (len > 1 && str[len-1] == '\n')
	str[len-1] = 0;
    }
}



char *input_buf_ptr = NULL;
size_t input_buf_size = 0;

static char *
rmt_read (void)
{
  ssize_t rc = getline (&input_buf_ptr, &input_buf_size, stdin);
  if (rc > 0)
    {
      DEBUG1 (10, "C: %s", input_buf_ptr);
      trimnl (input_buf_ptr);
      return input_buf_ptr;
    }
  DEBUG (10, "reached EOF");
  return NULL;
}

static void
rmt_write (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stdout, fmt, ap);
  fflush (stdout);
  VDEBUG (10, "S: ", fmt, ap);
}

static void
rmt_reply (uintmax_t code)
{
  char buf[UINTMAX_STRSIZE_BOUND];
  rmt_write ("A%s\n", umaxtostr (code, buf));
}

static void
rmt_error_message (int code, const char *msg)
{
  DEBUG1 (10, "S: E%d\n", code);
  DEBUG1 (10, "S: %s\n", msg);
  DEBUG1 (1, "error: %s\n", msg);
  fprintf (stdout, "E%d\n%s\n", code, msg);
  fflush (stdout);
}

static void
rmt_error (int code)
{
  rmt_error_message (code, strerror (code));
}


char *record_buffer_ptr;
size_t record_buffer_size;

static void
prepare_record_buffer (size_t size)
{
  if (size > record_buffer_size)
    {
      record_buffer_ptr = xrealloc (record_buffer_ptr, size);
      record_buffer_size = size;
    }
}



int device_fd = -1;

struct rmt_kw
{
  char const *name;
  size_t len;
  int value;
};

#define RMT_KW(s,v) { #s, sizeof (#s) - 1, v }

static int
xlat_kw (const char *s, const char *pfx,
	 struct rmt_kw const *kw, int *valp, const char **endp)
{
  size_t slen = strlen (s);

  if (pfx)
    {
      size_t pfxlen = strlen (pfx);
      if (slen > pfxlen && memcmp (s, pfx, pfxlen) == 0)
	{
	  s += pfxlen;
	  slen -= pfxlen;
	}
    }

  for (; kw->name; kw++)
    {
      if (slen >= kw->len
	  && memcmp (kw->name, s, kw->len) == 0
	  && !(s[kw->len] && c_isalnum (s[kw->len])))
	{
	  *valp = kw->value;
	  *endp = s + kw->len;
	  return 0;
	}
    }
  return 1;
}

static const char *
skip_ws (const char *s)
{
  while (*s && c_isblank (*s))
    s++;
  return s;
}

static struct rmt_kw const open_flag_kw[] =
  {
#ifdef O_APPEND
    RMT_KW(APPEND, O_APPEND),
#endif
    RMT_KW(CREAT, O_CREAT),
#ifdef O_DSYNC
    RMT_KW(DSYNC, O_DSYNC),
#endif
    RMT_KW(EXCL, O_EXCL),
#ifdef O_LARGEFILE
    RMT_KW(LARGEFILE, O_LARGEFILE),
#endif
#ifdef O_NOCTTY
    RMT_KW(NOCTTY, O_NOCTTY),
#endif
#if O_NONBLOCK
    RMT_KW(NONBLOCK, O_NONBLOCK),
#endif
    RMT_KW(RDONLY, O_RDONLY),
    RMT_KW(RDWR, O_RDWR),
#ifdef O_RSYNC
    RMT_KW(RSYNC, O_RSYNC),
#endif
#ifdef O_SYNC
    RMT_KW(SYNC, O_SYNC),
#endif
    RMT_KW(TRUNC, O_TRUNC),
    RMT_KW(WRONLY, O_WRONLY),
    { NULL }
  };

static int
decode_open_flag (const char *mstr, int *pmode)
{
  int numeric_mode = 0;
  int mode = 0;
  const char *p;

  mstr = skip_ws (mstr);
  if (c_isdigit (*mstr))
    {
      numeric_mode = strtol (mstr, (char**) &p, 10);
      mstr = skip_ws (p);
    }

  if (*mstr)
    {
      while (mstr)
	{
	  int v;
	  
	  mstr = skip_ws (mstr);
	  if (*mstr == 0)
	    break;
	  else if (c_isdigit (*mstr))
	    v = strtol (mstr, (char**) &p, 10);
	  else if (xlat_kw (mstr, "O_", open_flag_kw, &v, &p))
	    {
	      rmt_error_message (EINVAL, "invalid open mode");
	      return 1;
	    }

	  mode |= v;
	  
	  if (*p && c_isblank (*p))
	    p = skip_ws (p);
	  if (*p == 0)
	    break;
	  else if (*p == '|')
	    {
	      /* FIXMEL
		 if (p[1] == 0)
		 rmt_error_message (EINVAL, "invalid open mode");
	      */
	      mstr = p + 1;
	    }
	  else
	    {
	      rmt_error_message (EINVAL, "invalid open mode");
	      return 1;
	    }
	}
    }
  else
    mode = numeric_mode;
  *pmode = mode;
  return 0;
}


/* Syntax
   ------
   O<device>\n<flags>\n

   Function
   --------
   Opens the <device> with given <flags>. If a device had already been opened,
   it is closed before opening the new one.

   Arguments
   ---------
   <device> - name of the device to open.
   <flags>  - flags for open(2): a decimal number, or any valid O_* constant
   from fcntl.h (the initial O_ may be omitted), or a bitwise or (using '|')
   of any number of these, e.g.:

      576
      64|512
      CREAT|TRUNC

   In addition, a compined form is also allowed, i.e. a decimal mode followed
   by its symbolic representation.  In this case the symbolic representation
   is given preference.
      
   Reply
   -----
   A0\n on success, E0\n<msg>\n on error.

   Extensions
   ----------
   BSD version allows only decimal number as <flags>
*/

static void
open_device (char *str)
{
  char *device = xstrdup (str);
  char *flag_str;
  int flag;

  flag_str = rmt_read ();
  if (!flag_str)
    {
      DEBUG (1, "unexpected EOF");
      exit (EXIT_FAILURE);
    }
  if (decode_open_flag (flag_str, &flag) == 0)
    {
      if (device_fd >= 0)
	close (device_fd);

      device_fd = open (device, flag, MODE_RW);
      if (device_fd < 0)
	rmt_error (errno);
      else
	rmt_reply (0);
    }
  free (device);
}

/* Syntax
   ------
   C[<device>]\n

   Function
   --------
   Close the currently open device.

   Arguments
   ---------
   Any arguments are silently ignored.

   Reply
   -----
   A0\n on success, E0\n<msg>\n on error.
*/
static void
close_device (void)
{
  if (close (device_fd) < 0)
    rmt_error (errno);
  else
    {
      device_fd = -1;
      rmt_reply (0);
    }
}

/* Syntax
   ------
   L<whence>\n<offset>\n

   Function
   --------
   Perform an lseek(2) on the currently open device with the specified
   parameters.

   Arguments
   ---------
   <whence>  -  Where to measure offset from. Valid values are:
                0, SET, SEEK_SET to seek from the file beginning,
		1, CUR, SEEK_CUR to seek from the current location in file,
		2, END, SEEK_END to seek from the file end.
   Reply
   -----
   A<offset>\n on success. The <offset> is the new offset in file.
   E0\n<msg>\n on error.

   Extensions
   ----------
   BSD version allows only 0,1,2 as <whence>.
*/

static struct rmt_kw const seek_whence_kw[] =
  {
    RMT_KW(SET, SEEK_SET),
    RMT_KW(CUR, SEEK_CUR),
    RMT_KW(END, SEEK_END),
    { NULL }
  };

static void
lseek_device (const char *str)
{
  char *p;
  int whence;
  off_t off;
  uintmax_t n;

  if (str[0] && str[1] == 0)
    {
      switch (str[0])
	{
	case '0':
	  whence = SEEK_SET;
	  break;

	case '1':
	  whence = SEEK_CUR;
	  break;

	case '2':
	  whence = SEEK_END;
	  break;

	default:
	  rmt_error_message (EINVAL, N_("Seek direction out of range"));
	  return;
	}
    }
  else if (xlat_kw (str, "SEEK_", seek_whence_kw, &whence, (const char **) &p))
    {
      rmt_error_message (EINVAL, N_("Invalid seek direction"));
      return;
    }

  str = rmt_read ();
  n = off = strtoumax (str, &p, 10);
  if (*p)
    {
      rmt_error_message (EINVAL, N_("Invalid seek offset"));
      return;
    }

  if (n != off || errno == ERANGE)
    {
      rmt_error_message (EINVAL, N_("Seek offset out of range"));
      return;
    }

  off = lseek (device_fd, off, whence);
  if (off < 0)
    rmt_error (errno);
  else
    rmt_reply (off);
}

/* Syntax
   ------
   R<count>\n

   Function
   --------
   Read <count> bytes of data from the current device.

   Arguments
   ---------
   <count>  -  number of bytes to read.

   Reply
   -----
   On success: A<rdcount>\n, followed by <rdcount> bytes of data read from
   the device.
   On error: E0\n<msg>\n
*/

static void
read_device (const char *str)
{
  char *p;
  size_t size;
  uintmax_t n;
  size_t status;

  n = size = strtoumax (str, &p, 10);
  if (*p)
    {
      rmt_error_message (EINVAL, N_("Invalid byte count"));
      return;
    }

  if (n != size || errno == ERANGE)
    {
      rmt_error_message (EINVAL, N_("Byte count out of range"));
      return;
    }

  prepare_record_buffer (size);
  status = safe_read (device_fd, record_buffer_ptr, size);
  if (status == SAFE_READ_ERROR)
    rmt_error (errno);
  else
    {
      rmt_reply (status);
      full_write (STDOUT_FILENO, record_buffer_ptr, status);
    }
}

/* Syntax
   ------
   W<count>\n followed by <count> bytes of input data.

   Function
   --------
   Write data onto the current device.

   Arguments
   ---------
   <count>  - number of bytes.

   Reply
   -----
   On success: A<wrcount>\n, where <wrcount> is number of bytes actually
   written.
   On error: E0\n<msg>\n
*/

static void
write_device (const char *str)
{
  char *p;
  size_t size;
  uintmax_t n;
  size_t status;

  n = size = strtoumax (str, &p, 10);
  if (*p)
    {
      rmt_error_message (EINVAL, N_("Invalid byte count"));
      return;
    }

  if (n != size || errno == ERANGE)
    {
      rmt_error_message (EINVAL, N_("Byte count out of range"));
      return;
    }

  prepare_record_buffer (size);
  if (fread (record_buffer_ptr, size, 1, stdin) != 1)
    {
      if (feof (stdin))
	rmt_error_message (EIO, N_("Premature eof"));
      else
	rmt_error (errno);
      return;
    }

  status = full_write (device_fd, record_buffer_ptr, size);
  if (status != size)
    rmt_error (errno);
  else
    rmt_reply (status);
}

/* Syntax
   ------
   I<opcode>\n<count>\n

   Function
   --------
   Perform a MTIOCOP ioctl(2) command using the specified paramedters.

   Arguments
   ---------
   <opcode>   -  MTIOCOP operation code.
   <count>    -  mt_count.

   Reply
   -----
   On success: A0\n
   On error: E0\n<msg>\n
*/

static void
iocop_device (const char *str)
{
  char *p;
  long opcode;
  off_t count;
  uintmax_t n;

  opcode = strtol (str, &p, 10);
  if (*p)
    {
      rmt_error_message (EINVAL, N_("Invalid operation code"));
      return;
    }
  str = rmt_read ();
  n = count = strtoumax (str, &p, 10);
  if (*p)
    {
      rmt_error_message (EINVAL, N_("Invalid byte count"));
      return;
    }

  if (n != count || errno == ERANGE)
    {
      rmt_error_message (EINVAL, N_("Byte count out of range"));
      return;
    }

#ifdef MTIOCTOP
  {
    struct mtop mtop;

    mtop.mt_count = count;
    if (mtop.mt_count != count)
      {
	rmt_error_message (EINVAL, N_("Byte count out of range"));
	return;
      }

    mtop.mt_op = opcode;
    if (ioctl (device_fd, MTIOCTOP, (char *) &mtop) < 0)
      rmt_error (errno);
    else
      rmt_reply (0);
  }
#else
  rmt_error_message (ENOSYS, N_("Operation not supported"));
#endif
}

/* Syntax
   ------
   S\n

   Function
   --------
   Return the status of the open device, as obtained with a MTIOCGET
   ioctl call.

   Arguments
   ---------
   None

   Reply
   -----
   On success: A<count>\n followed by <count> bytes of data.
   On error: E0\n<msg>\n
*/

static void
status_device (const char *str)
{
  if (*str)
    {
      rmt_error_message (EINVAL, N_("Unexpected arguments"));
      return;
    }
#ifdef MTIOCGET
  {
    struct mtget mtget;

    if (ioctl (device_fd, MTIOCGET, (char *) &mtget) < 0)
      rmt_error (errno);
    else
      {
	rmt_reply (sizeof (mtget));
	full_write (STDOUT_FILENO, (char *) &mtget, sizeof (mtget));
      }
  }
#else
  rmt_error_message (ENOSYS, N_("Operation not supported"));
#endif
}



const char *argp_program_version = "rmt (" PACKAGE_NAME ") " VERSION;
const char *argp_program_bug_address = "<" PACKAGE_BUGREPORT ">";

static char const doc[] = N_("Manipulate a tape drive, accepting commands from a remote process");

enum {
  DEBUG_FILE_OPTION = 256
};

static struct argp_option options[] = {
  { "debug", 'd', N_("NUMBER"), 0,
    N_("set debug level"), 0 },
  { "debug-file", DEBUG_FILE_OPTION, N_("FILE"), 0,
    N_("set debug output file name"), 0 },
  { NULL }
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'd':
      dbglev = strtol (arg, NULL, 0);
      break;

    case DEBUG_FILE_OPTION:
      dbgout = fopen (arg, "w");
      if (!dbgout)
	error (EXIT_FAILURE, errno, _("cannot open %s"), arg);
      break;

    case ARGP_KEY_FINI:
      if (dbglev)
	{
	  if (!dbgout)
	    dbgout = stderr;
	}
      else if (dbgout)
	dbglev = 1;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  NULL,
  doc,
  NULL,
  NULL,
  NULL
};

static const char *rmt_authors[] = {
  "Sergey Poznyakoff",
  NULL
};


void
xalloc_die (void)
{
  rmt_error (ENOMEM);
  exit (EXIT_FAILURE);
}


int
main (int argc, char **argv)
{
  char *buf;
  int idx;
  int stop = 0;

  set_program_name (argv[0]);
  argp_version_setup ("rmt", rmt_authors);

  if (isatty (STDOUT_FILENO))
    {
      setlocale (LC_ALL, "");
      bindtextdomain (PACKAGE, LOCALEDIR);
      textdomain (PACKAGE);
    }

  if (argp_parse (&argp, argc, argv, ARGP_IN_ORDER, &idx, NULL))
    exit (EXIT_FAILURE);
  if (idx != argc)
    {
      if (idx != argc - 1)
	error (EXIT_FAILURE, 0, _("too many arguments"));
      dbgout = fopen (argv[idx], "w");
      if (!dbgout)
	error (EXIT_FAILURE, errno, _("cannot open %s"), argv[idx]);
      dbglev = 1;
    }

  while (!stop && (buf = rmt_read ()) != NULL)
    {
      switch (buf[0])
	{
	case 'C':
	  close_device ();
	  stop = 1;
	  break;

	case 'I':
	  iocop_device (buf + 1);
	  break;

	case 'L':
	  lseek_device (buf + 1);
	  break;

	case 'O':
	  open_device (buf + 1);
	  break;

	case 'R':
	  read_device (buf + 1);
	  break;

	case 'S':
	  status_device (buf + 1);
	  break;

	case 'W':
	  write_device (buf + 1);
	  break;

	default:
	  DEBUG1 (1, "garbage input %s\n", buf);
	  rmt_error_message (EINVAL, N_("Garbage command"));
	  return EXIT_FAILURE;	/* exit status used to be 3 */
	}
    }
  if (device_fd >= 0)
    close_device ();
  free (input_buf_ptr);
  free (record_buffer_ptr);
  return EXIT_SUCCESS;
}
