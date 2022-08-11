/* util.c - Several utility routines for cpio.
   Copyright (C) 1990-1992, 2001, 2004, 2006-2007, 2010-2011, 2014-2015
   Free Software Foundation, Inc.

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

#include <system.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cpiohdr.h"
#include "dstring.h"
#include "extern.h"
#include <paxlib.h>
#include "filetypes.h"
#include <safe-read.h>
#include <full-write.h>
#include <rmt.h>
#include <hash.h>
#include <utimens.h>

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_MTIO_H
# ifdef HAVE_SYS_IO_TRIOCTL_H
#  include <sys/io/trioctl.h>
# endif
# include <sys/mtio.h>
#endif

#if !HAVE_DECL_ERRNO
extern int errno;
#endif

/* Write `output_size' bytes of `output_buffer' to file
   descriptor OUT_DES and reset `output_size' and `out_buff'.  */

void
tape_empty_output_buffer (int out_des)
{
  int bytes_written;

#ifdef BROKEN_LONG_TAPE_DRIVER
  static long output_bytes_before_lseek = 0;

  /* Some tape drivers seem to have a signed internal seek pointer and
     they lose if it overflows and becomes negative (e.g. when writing 
     tapes > 2Gb).  Doing an lseek (des, 0, SEEK_SET) seems to reset the 
     seek pointer and prevent it from overflowing.  */
  if (output_is_special
     && ( (output_bytes_before_lseek += output_size) >= 1073741824L) )
    {
      lseek(out_des, 0L, SEEK_SET);
      output_bytes_before_lseek = 0;
    }
#endif

  bytes_written = rmtwrite (out_des, output_buffer, output_size);
  if (bytes_written != output_size)
    {
      int rest_bytes_written;
      int rest_output_size;

      if (output_is_special
	  && (bytes_written >= 0
	      || (bytes_written < 0
		  && (errno == ENOSPC || errno == EIO || errno == ENXIO))))
	{
	  get_next_reel (out_des);
	  if (bytes_written > 0)
	    rest_output_size = output_size - bytes_written;
	  else
	    rest_output_size = output_size;
	  rest_bytes_written = rmtwrite (out_des, output_buffer,
					 rest_output_size);
	  if (rest_bytes_written != rest_output_size)
	    error (PAXEXIT_FAILURE, errno, _("write error"));
	}
      else
	error (PAXEXIT_FAILURE, errno, _("write error"));
    }
  output_bytes += output_size;
  out_buff = output_buffer;
  output_size = 0;
}

static ssize_t sparse_write (int fildes, char *buf, size_t nbyte, bool flush);

/* Write `output_size' bytes of `output_buffer' to file
   descriptor OUT_DES and reset `output_size' and `out_buff'.
   If `swapping_halfwords' or `swapping_bytes' is set,
   do the appropriate swapping first.  Our callers have
   to make sure to only set these flags if `output_size' 
   is appropriate (a multiple of 4 for `swapping_halfwords',
   2 for `swapping_bytes').  The fact that DISK_IO_BLOCK_SIZE
   must always be a multiple of 4 helps us (and our callers)
   insure this.  */

void
disk_empty_output_buffer (int out_des, bool flush)
{
  ssize_t bytes_written;

  if (swapping_halfwords || swapping_bytes)
    {
      if (swapping_halfwords)
	{
	  int complete_words;
	  complete_words = output_size / 4;
	  swahw_array (output_buffer, complete_words);
	  if (swapping_bytes)
	    swab_array (output_buffer, 2 * complete_words);
	}
      else
	{
	  int complete_halfwords;
	  complete_halfwords = output_size /2;
	  swab_array (output_buffer, complete_halfwords);
	}
    }

  if (sparse_flag)
    bytes_written = sparse_write (out_des, output_buffer, output_size, flush);
  else
    bytes_written = write (out_des, output_buffer, output_size);

  if (bytes_written != output_size)
    {
      if (bytes_written == -1)
	error (PAXEXIT_FAILURE, errno, _("write error"));
      else
	error (PAXEXIT_FAILURE, 0, _("write error: partial write"));
    }
  output_bytes += output_size;
  out_buff = output_buffer;
  output_size = 0;
}

/* Exchange the halfwords of each element of the array of COUNT longs
   starting at PTR.  PTR does not have to be aligned at a word
   boundary.  */

void
swahw_array (char *ptr, int count)
{
  char tmp;

  for (; count > 0; --count)
    {
      tmp = *ptr;
      *ptr = *(ptr + 2);
      *(ptr + 2) = tmp;
      ++ptr;
      tmp = *ptr;
      *ptr = *(ptr + 2);
      *(ptr + 2) = tmp;
      ptr += 3;
    }
}

/* Read at most NUM_BYTES or `io_block_size' bytes, whichever is smaller,
   into the start of `input_buffer' from file descriptor IN_DES.
   Set `input_size' to the number of bytes read and reset `in_buff'.
   Exit with an error if end of file is reached.  */

#ifdef BROKEN_LONG_TAPE_DRIVER
static long input_bytes_before_lseek = 0;
#endif

static void
tape_fill_input_buffer (int in_des, int num_bytes)
{
#ifdef BROKEN_LONG_TAPE_DRIVER
  /* Some tape drivers seem to have a signed internal seek pointer and
     they lose if it overflows and becomes negative (e.g. when writing 
     tapes > 4Gb).  Doing an lseek (des, 0, SEEK_SET) seems to reset the 
     seek pointer and prevent it from overflowing.  */
  if (input_is_special
      && ( (input_bytes_before_lseek += num_bytes) >= 1073741824L) )
    {
      lseek(in_des, 0L, SEEK_SET);
      input_bytes_before_lseek = 0;
    }
#endif
  in_buff = input_buffer;
  num_bytes = (num_bytes < io_block_size) ? num_bytes : io_block_size;
  input_size = rmtread (in_des, input_buffer, num_bytes);
  if (input_size == 0 && input_is_special)
    {
      get_next_reel (in_des);
      input_size = rmtread (in_des, input_buffer, num_bytes);
    }
  if (input_size == SAFE_READ_ERROR)
    error (PAXEXIT_FAILURE, errno, _("read error"));
  if (input_size == 0)
    error (PAXEXIT_FAILURE, 0, _("premature end of file"));
  input_bytes += input_size;
}

/* Read at most NUM_BYTES or `DISK_IO_BLOCK_SIZE' bytes, whichever is smaller,
   into the start of `input_buffer' from file descriptor IN_DES.
   Set `input_size' to the number of bytes read and reset `in_buff'.
   Exit with an error if end of file is reached.  */

static int
disk_fill_input_buffer (int in_des, off_t num_bytes)
{
  in_buff = input_buffer;
  num_bytes = (num_bytes < DISK_IO_BLOCK_SIZE) ? num_bytes : DISK_IO_BLOCK_SIZE;
  input_size = read (in_des, input_buffer, num_bytes);
  if (input_size == SAFE_READ_ERROR)
    {
      input_size = 0;
      return (-1);
    }
  else if (input_size == 0)
    return (1);
  input_bytes += input_size;
  return (0);
}

/* Copy NUM_BYTES of buffer IN_BUF to `out_buff', which may be partly full.
   When `out_buff' fills up, flush it to file descriptor OUT_DES.  */

void
tape_buffered_write (char *in_buf, int out_des, off_t num_bytes)
{
  off_t bytes_left = num_bytes;	/* Bytes needing to be copied.  */
  off_t space_left;	/* Room left in output buffer.  */

  while (bytes_left > 0)
    {
      space_left = io_block_size - output_size;
      if (space_left == 0)
	tape_empty_output_buffer (out_des);
      else
	{
	  if (bytes_left < space_left)
	    space_left = bytes_left;
	  memcpy (out_buff, in_buf, (unsigned) space_left);
	  out_buff += space_left;
	  output_size += space_left;
	  in_buf += space_left;
	  bytes_left -= space_left;
	}
    }
}

/* Copy NUM_BYTES of buffer IN_BUF to `out_buff', which may be partly full.
   When `out_buff' fills up, flush it to file descriptor OUT_DES.  */

void
disk_buffered_write (char *in_buf, int out_des, off_t num_bytes)
{
  off_t bytes_left = num_bytes;	/* Bytes needing to be copied.  */
  off_t space_left;	/* Room left in output buffer.  */

  while (bytes_left > 0)
    {
      space_left = DISK_IO_BLOCK_SIZE - output_size;
      if (space_left == 0)
	disk_empty_output_buffer (out_des, false);
      else
	{
	  if (bytes_left < space_left)
	    space_left = bytes_left;
	  memcpy (out_buff, in_buf, (unsigned) space_left);
	  out_buff += space_left;
	  output_size += space_left;
	  in_buf += space_left;
	  bytes_left -= space_left;
	}
    }
}

/* Copy NUM_BYTES of buffer `in_buff' into IN_BUF.
   `in_buff' may be partly full.
   When `in_buff' is exhausted, refill it from file descriptor IN_DES.  */

void
tape_buffered_read (char *in_buf, int in_des, off_t num_bytes)
{
  off_t bytes_left = num_bytes;	/* Bytes needing to be copied.  */
  off_t space_left;	/* Bytes to copy from input buffer.  */

  while (bytes_left > 0)
    {
      if (input_size == 0)
	tape_fill_input_buffer (in_des, io_block_size);
      if (bytes_left < input_size)
	space_left = bytes_left;
      else
	space_left = input_size;
      memcpy (in_buf, in_buff, (unsigned) space_left);
      in_buff += space_left;
      in_buf += space_left;
      input_size -= space_left;
      bytes_left -= space_left;
    }
}

/* Copy the the next NUM_BYTES bytes of `input_buffer' into PEEK_BUF.
   If NUM_BYTES bytes are not available, read the next `io_block_size' bytes
   into the end of `input_buffer' and update `input_size'.

   Return the number of bytes copied into PEEK_BUF.
   If the number of bytes returned is less than NUM_BYTES,
   then EOF has been reached.  */

int
tape_buffered_peek (char *peek_buf, int in_des, int num_bytes)
{
  long tmp_input_size;
  long got_bytes;
  char *append_buf;

#ifdef BROKEN_LONG_TAPE_DRIVER
  /* Some tape drivers seem to have a signed internal seek pointer and
     they lose if it overflows and becomes negative (e.g. when writing 
     tapes > 4Gb).  Doing an lseek (des, 0, SEEK_SET) seems to reset the 
     seek pointer and prevent it from overflowing.  */
  if (input_is_special
      && ( (input_bytes_before_lseek += num_bytes) >= 1073741824L) )
    {
      lseek(in_des, 0L, SEEK_SET);
      input_bytes_before_lseek = 0;
    }
#endif

  while (input_size < num_bytes)
    {
      append_buf = in_buff + input_size;
      if ( (append_buf - input_buffer) >= input_buffer_size)
	{
	  /* We can keep up to 2 "blocks" (either the physical block size
	     or 512 bytes(the size of a tar record), which ever is
	     larger) in the input buffer when we are peeking.  We
	     assume that our caller will never be interested in peeking
	     ahead at more than 512 bytes, so we know that by the time
	     we need a 3rd "block" in the buffer we can throw away the
	     first block to make room.  */
	  int half;
	  half = input_buffer_size / 2;
	  memmove (input_buffer, input_buffer + half, half);
	  in_buff = in_buff - half;
	  append_buf = append_buf - half;
	}
      tmp_input_size = rmtread (in_des, append_buf, io_block_size);
      if (tmp_input_size == 0)
	{
	  if (input_is_special)
	    {
	      get_next_reel (in_des);
	      tmp_input_size = rmtread (in_des, append_buf, io_block_size);
	    }
	  else
	    break;
	}
      if (tmp_input_size < 0)
	error (PAXEXIT_FAILURE, errno, _("read error"));
      input_bytes += tmp_input_size;
      input_size += tmp_input_size;
    }
  if (num_bytes <= input_size)
    got_bytes = num_bytes;
  else
    got_bytes = input_size;
  memcpy (peek_buf, in_buff, (unsigned) got_bytes);
  return got_bytes;
}

/* Skip the next NUM_BYTES bytes of file descriptor IN_DES.  */

void
tape_toss_input (int in_des, off_t num_bytes)
{
  off_t bytes_left = num_bytes;	/* Bytes needing to be copied.  */
  off_t space_left;	/* Bytes to copy from input buffer.  */

  while (bytes_left > 0)
    {
      if (input_size == 0)
	tape_fill_input_buffer (in_des, io_block_size);
      if (bytes_left < input_size)
	space_left = bytes_left;
      else
	space_left = input_size;

      if (crc_i_flag && only_verify_crc_flag)
	{
 	  int k;
	  for (k = 0; k < space_left; ++k)
	    crc += in_buff[k] & 0xff;
	}

      in_buff += space_left;
      input_size -= space_left;
      bytes_left -= space_left;
    }
}

void
write_nuls_to_file (off_t num_bytes, int out_des, 
                    void (*writer) (char *in_buf, int out_des, off_t num_bytes))
{
  off_t	blocks;
  off_t	extra_bytes;
  off_t	i;
  static char zeros_512[512];
  
  blocks = num_bytes / sizeof zeros_512;
  extra_bytes = num_bytes % sizeof zeros_512;
  for (i = 0; i < blocks; ++i)
    writer (zeros_512, out_des, sizeof zeros_512);
  if (extra_bytes)
    writer (zeros_512, out_des, extra_bytes);
}

/* Copy a file using the input and output buffers, which may start out
   partly full.  After the copy, the files are not closed nor the last
   block flushed to output, and the input buffer may still be partly
   full.  If `crc_i_flag' is set, add each byte to `crc'.
   IN_DES is the file descriptor for input;
   OUT_DES is the file descriptor for output;
   NUM_BYTES is the number of bytes to copy.  */

void
copy_files_tape_to_disk (int in_des, int out_des, off_t num_bytes)
{
  off_t size;
  off_t k;

  while (num_bytes > 0)
    {
      if (input_size == 0)
	tape_fill_input_buffer (in_des, io_block_size);
      size = (input_size < num_bytes) ? input_size : num_bytes;
      if (crc_i_flag)
	{
	  for (k = 0; k < size; ++k)
	    crc += in_buff[k] & 0xff;
	}
      disk_buffered_write (in_buff, out_des, size);
      num_bytes -= size;
      input_size -= size;
      in_buff += size;
    }
}
/* Copy a file using the input and output buffers, which may start out
   partly full.  After the copy, the files are not closed nor the last
   block flushed to output, and the input buffer may still be partly
   full.  If `crc_i_flag' is set, add each byte to `crc'.
   IN_DES is the file descriptor for input;
   OUT_DES is the file descriptor for output;
   NUM_BYTES is the number of bytes to copy.  */

void
copy_files_disk_to_tape (int in_des, int out_des, off_t num_bytes,
			 char *filename)
{
  off_t size;
  off_t k;
  int rc;
  off_t original_num_bytes;

  original_num_bytes = num_bytes;

  while (num_bytes > 0)
    {
      if (input_size == 0)
	if ((rc = disk_fill_input_buffer (in_des,
					  num_bytes < DISK_IO_BLOCK_SIZE ?
					  num_bytes : DISK_IO_BLOCK_SIZE)))
	  {
	    if (rc > 0)
	      {
		  char buf[UINTMAX_STRSIZE_BOUND];
		  error (0, 0,
			 ngettext ("File %s shrunk by %s byte, padding with zeros",
				   "File %s shrunk by %s bytes, padding with zeros",
				   num_bytes),
			 filename,  STRINGIFY_BIGINT (num_bytes, buf));
	      }
	    else
	      error (0, 0, _("Read error at byte %lld in file %s, padding with zeros"),
			original_num_bytes - num_bytes, filename);
	    write_nuls_to_file (num_bytes, out_des, tape_buffered_write);
	    break;
	  }
      size = (input_size < num_bytes) ? input_size : num_bytes;
      if (crc_i_flag)
	{
	  for (k = 0; k < size; ++k)
	    crc += in_buff[k] & 0xff;
	}
      tape_buffered_write (in_buff, out_des, size);
      num_bytes -= size;
      input_size -= size;
      in_buff += size;
    }
}
/* Copy a file using the input and output buffers, which may start out
   partly full.  After the copy, the files are not closed nor the last
   block flushed to output, and the input buffer may still be partly
   full.  If `crc_i_flag' is set, add each byte to `crc'.
   IN_DES is the file descriptor for input;
   OUT_DES is the file descriptor for output;
   NUM_BYTES is the number of bytes to copy.  */

void
copy_files_disk_to_disk (int in_des, int out_des, off_t num_bytes,
			 char *filename)
{
  off_t size;
  off_t k;
  off_t original_num_bytes;
  int rc;

  original_num_bytes = num_bytes;
  while (num_bytes > 0)
    {
      if (input_size == 0)
	if ((rc = disk_fill_input_buffer (in_des, num_bytes)))
	  {
	    if (rc > 0)
	      {
		char buf[UINTMAX_STRSIZE_BOUND];
		error (0, 0,
		       ngettext ("File %s shrunk by %s byte, padding with zeros",
				 "File %s shrunk by %s bytes, padding with zeros",
				 num_bytes),
		       filename,  STRINGIFY_BIGINT (num_bytes, buf));
	      }
	    else
	      error (0, 0, _("Read error at byte %lld in file %s, padding with zeros"),
			original_num_bytes - num_bytes, filename);
	    write_nuls_to_file (num_bytes, out_des, disk_buffered_write);
	    break;
	  }
      size = (input_size < num_bytes) ? input_size : num_bytes;
      if (crc_i_flag)
	{
	  for (k = 0; k < size; ++k)
	    crc += in_buff[k] & 0xff;
	}
      disk_buffered_write (in_buff, out_des, size);
      num_bytes -= size;
      input_size -= size;
      in_buff += size;
    }
}

/* Warn if file changed while it was being copied.  */

void
warn_if_file_changed (char *file_name, off_t old_file_size,
		      time_t old_file_mtime)
{
  struct stat new_file_stat;
  if ((*xstat) (file_name, &new_file_stat) < 0)
    {
      stat_error (file_name);
      return;
    }

  /* Only check growth, shrinkage detected in copy_files_disk_to_{disk,tape}()
   */
  if (new_file_stat.st_size > old_file_size)
    error (0, 0,
	   ngettext ("File %s grew, %"PRIuMAX" new byte not copied",
		     "File %s grew, %"PRIuMAX" new bytes not copied",
		     (long)(new_file_stat.st_size - old_file_size)),
	   file_name, (uintmax_t) (new_file_stat.st_size - old_file_size));

  else if (new_file_stat.st_mtime != old_file_mtime)
    error (0, 0, _("File %s was modified while being copied"), file_name);
}

/* Create all directories up to but not including the last part of NAME.
   Do not destroy any nondirectories while creating directories.  */

void
create_all_directories (char *name)
{
  char *dir;
  int   mode;
#ifdef HPUX_CDF
  int   cdf;
#endif

  dir = dir_name (name);
  mode = 0700;
#ifdef HPUX_CDF
  cdf = islastparentcdf (name);
  if (cdf)
    {
      dir [strlen (dir) - 1] = '\0';	/* remove final + */
      mode = 04700;
    }
  
#endif
  
  if (dir == NULL)
    error (PAXEXIT_FAILURE, 0, _("virtual memory exhausted"));

  if (dir[0] != '.' || dir[1] != '\0')
    {
      const char *fmt;
      if (warn_option & CPIO_WARN_INTERDIR)
	fmt = _("Creating intermediate directory `%s'");
      else
	fmt = NULL;
      make_path (dir, -1, -1, fmt);
    }

  free (dir);
}

/* Prepare to append to an archive.  We have been in
   process_copy_in, keeping track of the position where
   the last header started in `last_header_start'.  Now we
   have the starting position of the last header (the TRAILER!!!
   header, or blank record for tar archives) and we want to start
   writing (appending) over the last header.  The last header may
   be in the middle of a block, so to keep the buffering in sync
   we lseek back to the start of the block, read everything up
   to but not including the last header, lseek back to the start
   of the block, and then do a copy_buf_out of what we read.
   Actually, we probably don't have to worry so much about keeping the
   buffering perfect since you can only append to archives that
   are disk files.  */

void
prepare_append (int out_file_des)
{
  int start_of_header;
  int start_of_block;
  int useful_bytes_in_block;
  char *tmp_buf;

  start_of_header = last_header_start;
  /* Figure out how many bytes we will rewrite, and where they start.  */
  useful_bytes_in_block = start_of_header % io_block_size;
  start_of_block = start_of_header - useful_bytes_in_block;

  if (lseek (out_file_des, start_of_block, SEEK_SET) < 0)
    error (PAXEXIT_FAILURE, errno, _("cannot seek on output"));
  if (useful_bytes_in_block > 0)
    {
      tmp_buf = (char *) xmalloc (useful_bytes_in_block);
      read (out_file_des, tmp_buf, useful_bytes_in_block);
      if (lseek (out_file_des, start_of_block, SEEK_SET) < 0)
	error (PAXEXIT_FAILURE, errno, _("cannot seek on output"));
      /* fix juo -- is this copy_tape_buf_out?  or copy_disk? */
      tape_buffered_write (tmp_buf, out_file_des, useful_bytes_in_block);
      free (tmp_buf);
    }

  /* We are done reading the archive, so clear these since they
     will now be used for reading in files that we are appending
     to the archive.  */
  input_size = 0;
  input_bytes = 0;
  in_buff = input_buffer;
}

/* Support for remembering inodes with multiple links.  Used in the
   "copy in" and "copy pass" modes for making links instead of copying
   the file.  */

struct inode_val
{
  ino_t inode;
  unsigned long major_num;
  unsigned long minor_num;
  ino_t trans_inode;
  char *file_name;
};

/* Inode hash table.  Allocated by first call to add_inode.  */
static Hash_table *hash_table = NULL;

static size_t
inode_val_hasher (const void *val, size_t n_buckets)
{
  const struct inode_val *ival = val;
  return ival->inode % n_buckets;
}

static bool
inode_val_compare (const void *val1, const void *val2)
{
  const struct inode_val *ival1 = val1;
  const struct inode_val *ival2 = val2;
  return ival1->inode == ival2->inode
         && ival1->major_num == ival2->major_num
         && ival1->minor_num == ival2->minor_num;
}

static struct inode_val *
find_inode_val (ino_t node_num, unsigned long major_num,
		 unsigned long minor_num)
{
  struct inode_val sample;
  struct inode_val *ival;
  
  if (!hash_table)
    return NULL;
  
  sample.inode = node_num;
  sample.major_num = major_num;
  sample.minor_num = minor_num;
  return hash_lookup (hash_table, &sample);
}

char *
find_inode_file (ino_t node_num, unsigned long major_num,
		 unsigned long minor_num)
{
  struct inode_val *ival = find_inode_val (node_num, major_num, minor_num);
  return ival ? ival->file_name : NULL;
}

/* Associate FILE_NAME with the inode NODE_NUM.  (Insert into hash table.)  */

static ino_t next_inode;

struct inode_val *
add_inode (ino_t node_num, char *file_name, unsigned long major_num,
	   unsigned long minor_num)
{
  struct inode_val *temp;
  struct inode_val *e = NULL;
  
  /* Create new inode record.  */
  temp = (struct inode_val *) xmalloc (sizeof (struct inode_val));
  temp->inode = node_num;
  temp->major_num = major_num;
  temp->minor_num = minor_num;
  temp->file_name = file_name ? xstrdup (file_name) : NULL;

  if (renumber_inodes_option)
    temp->trans_inode = next_inode++;
  else
    temp->trans_inode = temp->inode;

  if (!((hash_table
	 || (hash_table = hash_initialize (0, 0, inode_val_hasher,
					   inode_val_compare, 0)))
	&& (e = hash_insert (hash_table, temp))))
    xalloc_die ();
  return e;
}

static ino_t
get_inode_and_dev (struct cpio_file_stat *hdr, struct stat *st)
{
  if (renumber_inodes_option)
    {
      if (st->st_nlink > 1)
	{
	  struct inode_val *ival = find_inode_val (st->st_ino,
						   major (st->st_dev),
						   minor (st->st_dev));
	  if (!ival)
	    ival = add_inode (st->st_ino, NULL,
			      major (st->st_dev), minor (st->st_dev));
	  hdr->c_ino = ival->trans_inode;
	}
      else
	hdr->c_ino = next_inode++;
    }
  else
    hdr->c_ino = st->st_ino;
  if (ignore_devno_option)
    {
      hdr->c_dev_maj = 0;
      hdr->c_dev_min = 0;
    }
  else
    {
      hdr->c_dev_maj = major (st->st_dev);
      hdr->c_dev_min = minor (st->st_dev);
    }
}


/* Open FILE in the mode specified by the command line options
   and return an open file descriptor for it,
   or -1 if it can't be opened.  */

int
open_archive (char *file)
{
  int fd;
  void (*copy_in) ();		/* Workaround for pcc bug.  */

  copy_in = process_copy_in;

  if (copy_function == copy_in)
    fd = rmtopen (file, O_RDONLY | O_BINARY, MODE_RW, rsh_command_option);
  else
    {
      if (!append_flag)
	fd = rmtopen (file, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, MODE_RW,
			rsh_command_option);
      else
	fd = rmtopen (file, O_RDWR | O_BINARY, MODE_RW, rsh_command_option);
    }

  return fd;
}

/* Attempt to rewind the tape drive on file descriptor TAPE_DES
   and take it offline.  */

void
tape_offline (int tape_des)
{
#if defined(MTIOCTOP) && defined(MTOFFL)
  struct mtop control;

  control.mt_op = MTOFFL;
  control.mt_count = 1;
  rmtioctl (tape_des, MTIOCTOP, (char*) &control);	/* Don't care if it fails.  */
#endif
}

/* The file on file descriptor TAPE_DES is assumed to be magnetic tape
   (or floppy disk or other device) and the end of the medium
   has been reached.  Ask the user for to mount a new "tape" to continue
   the processing.  If the user specified the device name on the
   command line (with the -I, -O, -F or --file options), then we can
   automatically re-open the same device to use the next medium.  If the
   user did not specify the device name, then we have to ask them which
   device to use.  */

void
get_next_reel (int tape_des)
{
  static int reel_number = 1;
  FILE *tty_in;			/* File for interacting with user.  */
  FILE *tty_out;		/* File for interacting with user.  */
  int old_tape_des;
  char *next_archive_name;
  dynamic_string new_name;
  char *str_res;

  ds_init (&new_name, 128);

  /* Open files for interactive communication.  */
  tty_in = fopen (TTY_NAME, "r");
  if (tty_in == NULL)
    error (PAXEXIT_FAILURE, errno, TTY_NAME);
  tty_out = fopen (TTY_NAME, "w");
  if (tty_out == NULL)
    error (PAXEXIT_FAILURE, errno, TTY_NAME);

  old_tape_des = tape_des;
  tape_offline (tape_des);
  rmtclose (tape_des);

  /* Give message and wait for carrage return.  User should hit carrage return
     only after loading the next tape.  */
  ++reel_number;
  if (new_media_message)
    fprintf (tty_out, "%s", new_media_message);
  else if (new_media_message_with_number)
    fprintf (tty_out, "%s%d%s", new_media_message_with_number, reel_number,
	     new_media_message_after_number);
  else if (archive_name)
    fprintf (tty_out, _("Found end of tape.  Load next tape and press RETURN. "));
  else
    fprintf (tty_out, _("Found end of tape.  To continue, type device/file name when ready.\n"));

  fflush (tty_out);

  if (archive_name)
    {
      int c;

      do
	c = getc (tty_in);
      while (c != EOF && c != '\n');

      tape_des = open_archive (archive_name);
      if (tape_des == -1)
	open_error (archive_name);
    }
  else
    {
      do
	{
	  if (tape_des < 0)
	    {
	      fprintf (tty_out,
		       _("To continue, type device/file name when ready.\n"));
	      fflush (tty_out);
	    }

	  str_res = ds_fgets (tty_in, &new_name);
	  if (str_res == NULL || str_res[0] == '\0')
	    exit (PAXEXIT_FAILURE);
	  next_archive_name = str_res;

	  tape_des = open_archive (next_archive_name);
	  if (tape_des == -1)
	    open_error (next_archive_name);
	}
      while (tape_des < 0);
    }

  /* We have to make sure that `tape_des' has not changed its value even
     though we closed it and reopened it, since there are local
     copies of it in other routines.  This works fine on Unix (even with
     rmtread and rmtwrite) since open will always return the lowest
     available file descriptor and we haven't closed any files (e.g.,
     stdin, stdout or stderr) that were opened before we originally opened
     the archive.  */

  if (tape_des != old_tape_des)
    error (PAXEXIT_FAILURE, 0, _("internal error: tape descriptor changed from %d to %d"),
	   old_tape_des, tape_des);

  free (new_name.ds_string);
  fclose (tty_in);
  fclose (tty_out);
}

/* If MESSAGE does not contain the string "%d", make `new_media_message'
   a copy of MESSAGE.  If MESSAGES does contain the string "%d", make
   `new_media_message_with_number' a copy of MESSAGE up to, but
   not including, the string "%d", and make `new_media_message_after_number'
   a copy of MESSAGE after the string "%d".  */

void
set_new_media_message (char *message)
{
  char *p;
  int prev_was_percent;

  p = message;
  prev_was_percent = 0;
  while (*p != '\0')
    {
      if (*p == 'd' && prev_was_percent)
	break;
      prev_was_percent = (*p == '%');
      ++p;
    }
  if (*p == '\0')
    {
      new_media_message = xstrdup (message);
    }
  else
    {
      int length = p - message - 1;

      new_media_message_with_number = xmalloc (length + 1);
      strncpy (new_media_message_with_number, message, length);
      new_media_message_with_number[length] = '\0';
      length = strlen (p + 1);
      new_media_message_after_number = xmalloc (length + 1);
      strcpy (new_media_message_after_number, p + 1);
    }
}

#ifdef SYMLINK_USES_UMASK
/* Most machines always create symlinks with rwxrwxrwx protection,
   but some (HP/UX 8.07; maybe DEC's OSF on MIPS, too?) use the
   umask when creating symlinks, so if your umask is 022 you end
   up with rwxr-xr-x symlinks (although HP/UX seems to completely
   ignore the protection).  There doesn't seem to be any way to
   manipulate the modes once the symlinks are created (e.g.
   a hypothetical "lchmod"), so to create them with the right
   modes we have to set the umask first.  */

int
umasked_symlink (char *name1, char *name2, int mode)
{
  int	old_umask;
  int	rc;
  mode = ~(mode & 0777) & 0777;
  old_umask = umask (mode);
  rc = symlink (name1, name2);
  umask (old_umask);
  return rc;
}
#endif /* SYMLINK_USES_UMASK */

#ifdef HPUX_CDF
/* When we create a cpio archive we mark CDF's by putting an extra `/'
   after their component name so we can distinguish the CDF's when we
   extract the archive (in case the "hidden" directory's files appear
   in the archive before the directory itself).  E.g., in the path
   "a/b+/c", if b+ is a CDF, we will write this path as "a/b+//c" in
   the archive so when we extract the archive we will know that b+
   is actually a CDF, and not an ordinary directory whose name happens
   to end in `+'.  We also do the same thing internally in copypass.c.  */


/* Take an input pathname and check it for CDF's.  Insert an extra
   `/' in the pathname after each "hidden" directory.  If we add
   any `/'s, return a malloced string instead of the original input
   string.
   FIXME: This creates a memory leak.
*/

char *
add_cdf_double_slashes (char *input_name)
{
  static char *ret_name = NULL;	/* re-usuable return buffer (malloc'ed)  */
  static int ret_size = -1;	/* size of return buffer.  */
  char *p;
  char *q;
  int n;
  struct stat dir_stat;

  /*  Search for a `/' preceeded by a `+'.  */

  for (p = input_name; *p != '\0'; ++p)
    {
      if ( (*p == '+') && (*(p + 1) == '/') )
	break;
    }

  /* If we didn't find a `/' preceeded by a `+' then there are
     no CDF's in this pathname.  Return the original pathname.  */

  if (*p == '\0')
    return input_name;

  /* There was a `/' preceeded by a `+' in the pathname.  If it is a CDF 
     then we will need to copy the input pathname to our return
     buffer so we can insert the extra `/'s.  Since we can't tell
     yet whether or not it is a CDF we will just always copy the
     string to the return buffer.  First we have to make sure the
     buffer is large enough to hold the string and any number of
     extra `/'s we might add.  */

  n = 2 * (strlen (input_name) + 1);
  if (n >= ret_size)
    {
      if (ret_size < 0)
	ret_name = (char *) malloc (n);
      else
	ret_name = (char *)realloc (ret_name, n);
      ret_size = n;
    }

  /* Clear the `/' after this component, so we can stat the pathname 
     up to and including this component.  */
  ++p;
  *p = '\0';
  if ((*xstat) (input_name, &dir_stat) < 0)
    {
      stat_error (input_name);
      return input_name;
    }

  /* Now put back the `/' after this component and copy the pathname up to
     and including this component and its trailing `/' to the return
     buffer.  */
  *p++ = '/';
  strncpy (ret_name, input_name, p - input_name);
  q = ret_name + (p - input_name);

  /* If it was a CDF, add another `/'.  */
  if (S_ISDIR (dir_stat.st_mode) && (dir_stat.st_mode & 04000) )
    *q++ = '/';

  /* Go through the rest of the input pathname, copying it to the
     return buffer, and adding an extra `/' after each CDF.  */
  while (*p != '\0')
    {
      if ( (*p == '+') && (*(p + 1) == '/') )
	{
	  *q++ = *p++;

	  *p = '\0';
	  if ((*xstat) (input_name, &dir_stat) < 0)
	    {
	      stat_error (input_name);
	      return input_name;
	    }
	  *p = '/';

	  if (S_ISDIR (dir_stat.st_mode) && (dir_stat.st_mode & 04000) )
	    *q++ = '/';
	}
      *q++ = *p++;
    }
  *q = '\0';

  return ret_name;
}

/* Is the last parent directory (e.g., c in a/b/c/d) a CDF?  If the
   directory name ends in `+' and is followed by 2 `/'s instead of 1
   then it is.  This is only the case for cpio archives, but we don't
   have to worry about tar because tar always has the directory before
   its files (or else we lose).  */
int
islastparentcdf (char *path)
{
  char *newpath;
  char *slash;
  int slash_count;
  int length;			/* Length of result, not including NUL.  */

  slash = strrchr (path, '/');
  if (slash == 0)
    return 0;
  else
    {
      slash_count = 0;
      while (slash > path && *slash == '/')
	{
	  ++slash_count;
	  --slash;
	}


      if ( (*slash == '+') && (slash_count >= 2) )
	return 1;
    }
  return 0;
}
#endif

#define DISKBLOCKSIZE	(512)

static int
buf_all_zeros (char *buf, int bufsize)
{
  int	i;
  for (i = 0; i < bufsize; ++i)
    {
      if (*buf++ != '\0')
	return 0;
    }
  return 1;
}

/* Write NBYTE bytes from BUF to file descriptor FILDES, trying to
   create holes instead of writing blockfuls of zeros.
   
   Return the number of bytes written (including bytes in zero
   regions) on success, -1 on error.

   If FLUSH is set, make sure the trailing zero region is flushed
   on disk.
*/

static ssize_t
sparse_write (int fildes, char *buf, size_t nbytes, bool flush)
{
  size_t nwritten = 0;
  ssize_t n;
  char *start_ptr = buf;

  static off_t delayed_seek_count = 0;
  off_t seek_count = 0;

  enum { begin, in_zeros, not_in_zeros } state =
			   delayed_seek_count ? in_zeros : begin;
  
  while (nbytes)
    {
      size_t rest = nbytes;

      if (rest < DISKBLOCKSIZE)
	/* Force write */
	state = not_in_zeros;
      else
	{
	  if (buf_all_zeros (buf, rest))
	    {
	      if (state == not_in_zeros)
		{
		  ssize_t bytes = buf - start_ptr + rest;
		  
		  n = write (fildes, start_ptr, bytes);
		  if (n == -1)
		    return -1;
		  nwritten += n;
		  if (n < bytes)
		    return nwritten + seek_count;
		  start_ptr = NULL;
		}
	      else
		seek_count += rest;
	      state = in_zeros;
	    }
	  else
	    {
	      seek_count += delayed_seek_count;
	      if (lseek (fildes, seek_count, SEEK_CUR) == -1)
		return -1;
	      delayed_seek_count = seek_count = 0;
	      state = not_in_zeros;
	      start_ptr = buf;
	    }
	}
      buf += rest;
      nbytes -= rest;
    }

  if (state != in_zeros)
    {
      seek_count += delayed_seek_count;
      if (seek_count && lseek (fildes, seek_count, SEEK_CUR) == -1)
	return -1;
      delayed_seek_count = seek_count = 0;

      n = write (fildes, start_ptr, buf - start_ptr);
      if (n == -1)
	return n;
      nwritten += n;
    }
  delayed_seek_count += seek_count;

  if (flush && delayed_seek_count)
    {
      if (lseek (fildes, delayed_seek_count - 1, SEEK_CUR) == -1)
	return -1;
      n = write (fildes, "", 1);
      if (n != 1)
	return n;
      delayed_seek_count = 0;
    }      
  
  return nwritten + seek_count;
}

#define CPIO_UID(uid) (set_owner_flag ? set_owner : (uid))
#define CPIO_GID(gid) (set_group_flag ? set_group : (gid))

void
stat_to_cpio (struct cpio_file_stat *hdr, struct stat *st)
{
  get_inode_and_dev (hdr, st);

  /* For POSIX systems that don't define the S_IF macros,
     we can't assume that S_ISfoo means the standard Unix
     S_IFfoo bit(s) are set.  So do it manually, with a
     different name.  Bleah.  */
  hdr->c_mode = (st->st_mode & 07777);
  if (S_ISREG (st->st_mode))
    hdr->c_mode |= CP_IFREG;
  else if (S_ISDIR (st->st_mode))
    hdr->c_mode |= CP_IFDIR;
#ifdef S_ISBLK
  else if (S_ISBLK (st->st_mode))
    hdr->c_mode |= CP_IFBLK;
#endif
#ifdef S_ISCHR
  else if (S_ISCHR (st->st_mode))
    hdr->c_mode |= CP_IFCHR;
#endif
#ifdef S_ISFIFO
  else if (S_ISFIFO (st->st_mode))
    hdr->c_mode |= CP_IFIFO;
#endif
#ifdef S_ISLNK
  else if (S_ISLNK (st->st_mode))
    hdr->c_mode |= CP_IFLNK;
#endif
#ifdef S_ISSOCK
  else if (S_ISSOCK (st->st_mode))
    hdr->c_mode |= CP_IFSOCK;
#endif
#ifdef S_ISNWK
  else if (S_ISNWK (st->st_mode))
    hdr->c_mode |= CP_IFNWK;
#endif
  hdr->c_nlink = st->st_nlink;
  hdr->c_uid = CPIO_UID (st->st_uid);
  hdr->c_gid = CPIO_GID (st->st_gid);
  hdr->c_rdev_maj = major (st->st_rdev);
  hdr->c_rdev_min = minor (st->st_rdev);
  hdr->c_mtime = st->st_mtime;
  hdr->c_filesize = st->st_size;
  hdr->c_chksum = 0;
  hdr->c_tar_linkname = NULL;
}

void
cpio_to_stat (struct stat *st, struct cpio_file_stat *hdr)
{
  memset (st, 0, sizeof (*st));
  st->st_dev = makedev (hdr->c_dev_maj, hdr->c_dev_min);
  st->st_ino = hdr->c_ino;
  st->st_mode = hdr->c_mode & 0777;
  if (hdr->c_mode & CP_IFREG)
    st->st_mode |= S_IFREG;
  else if (hdr->c_mode & CP_IFDIR)
    st->st_mode |= S_IFDIR;
#ifdef S_IFBLK
  else if (hdr->c_mode & CP_IFBLK)
    st->st_mode |= S_IFBLK;
#endif
#ifdef S_IFCHR
  else if (hdr->c_mode & CP_IFCHR)
    st->st_mode |= S_IFCHR;
#endif
#ifdef S_IFFIFO
  else if (hdr->c_mode & CP_IFIFO)
    st->st_mode |= S_IFIFO;
#endif
#ifdef S_IFLNK
  else if (hdr->c_mode & CP_IFLNK)
    st->st_mode |= S_IFLNK;
#endif
#ifdef S_IFSOCK
  else if (hdr->c_mode & CP_IFSOCK)
    st->st_mode |= S_IFSOCK;
#endif
#ifdef S_IFNWK
  else if (hdr->c_mode & CP_IFNWK)
    st->st_mode |= S_IFNWK;
#endif
  st->st_nlink = hdr->c_nlink;
  st->st_uid = CPIO_UID (hdr->c_uid);
  st->st_gid = CPIO_GID (hdr->c_gid);
  st->st_rdev = makedev (hdr->c_rdev_maj, hdr->c_rdev_min);
  st->st_mtime = hdr->c_mtime;
  st->st_size = hdr->c_filesize;
}

#ifndef HAVE_FCHOWN
# define HAVE_FCHOWN 0
#endif
#ifndef HAVE_FCHMOD
# define HAVE_FCHMOD 0
#endif

int
fchown_or_chown (int fd, const char *name, uid_t uid, uid_t gid)
{
  if (HAVE_FCHOWN && fd != -1)
    return fchown (fd, uid, gid);
  else
    return chown (name, uid, gid);
}

int
fchmod_or_chmod (int fd, const char *name, mode_t mode)
{
  if (HAVE_FCHMOD && fd != -1)
    return fchmod (fd, mode);
  else
    return chmod (name, mode);
}

void
set_perms (int fd, struct cpio_file_stat *header)
{
  if (!no_chown_flag)
    {
      uid_t uid = CPIO_UID (header->c_uid);
      gid_t gid = CPIO_GID (header->c_gid); 
      if ((fchown_or_chown (fd, header->c_name, uid, gid) < 0)
	  && errno != EPERM)
	chown_error_details (header->c_name, uid, gid);
    }
  /* chown may have turned off some permissions we wanted. */
  if (fchmod_or_chmod (fd, header->c_name, header->c_mode) < 0)
    chmod_error_details (header->c_name, header->c_mode);
#ifdef HPUX_CDF
  if ((header->c_mode & CP_IFMT) && cdf_flag)
    /* Once we "hide" the directory with the chmod(),
       we have to refer to it using name+ instead of name.  */
    file_hdr->c_name [cdf_char] = '+';
#endif
  if (retain_time_flag)
    set_file_times (fd, header->c_name, header->c_mtime, header->c_mtime);
}

void
set_file_times (int fd,
		const char *name, unsigned long atime, unsigned long mtime)
{
  struct timespec ts[2];
  
  memset (&ts, 0, sizeof ts);

  ts[0].tv_sec = atime;
  ts[1].tv_sec = mtime;

  /* Silently ignore EROFS because reading the file won't have upset its 
     timestamp if it's on a read-only filesystem. */
  if (fdutimens (fd, name, ts) < 0 && errno != EROFS)
    utime_error (name);
}

/* Do we have to ignore absolute paths, and if so, does the filename
   have an absolute path?  */
void
cpio_safer_name_suffix (char *name, bool link_target, bool absolute_names,
			bool strip_leading_dots)
{
  char *p = safer_name_suffix (name, link_target, absolute_names);
  if (strip_leading_dots && strcmp (p, "./"))
    /* strip leading `./' from the filename.  */
    while (*p == '.' && *(p + 1) == '/')
      {
	++p;
	while (*p == '/')
	  ++p;
      }
  if (p != name)
    memmove (name, p, (size_t)(strlen (p) + 1));
}


/* This is a simplified form of delayed set_stat used by GNU tar.
   With the time, both forms will merge and pass to paxutils
   
   List of directories whose statuses we need to extract after we've
   finished extracting their subsidiary files.  If you consider each
   contiguous subsequence of elements of the form [D]?[^D]*, where [D]
   represents an element where AFTER_LINKS is nonzero and [^D]
   represents an element where AFTER_LINKS is zero, then the head
   of the subsequence has the longest name, and each non-head element
   in the prefix is an ancestor (in the directory hierarchy) of the
   preceding element.  */

struct delayed_set_stat
  {
    struct delayed_set_stat *next;
    struct cpio_file_stat stat;
    mode_t invert_permissions;
  };

static struct delayed_set_stat *delayed_set_stat_head;

void
delay_cpio_set_stat (struct cpio_file_stat *file_stat,
		     mode_t invert_permissions)
{
  size_t file_name_len = strlen (file_stat->c_name);
  struct delayed_set_stat *data =
    xmalloc (sizeof (struct delayed_set_stat) + file_name_len + 1);
  data->next = delayed_set_stat_head;
  memcpy (&data->stat, file_stat, sizeof data->stat);
  data->stat.c_name = (char*) (data + 1);
  strcpy (data->stat.c_name, file_stat->c_name);
  data->invert_permissions = invert_permissions;
  delayed_set_stat_head = data;
}

void
delay_set_stat (char const *file_name, struct stat *st,
		mode_t invert_permissions)
{
  struct cpio_file_stat fs;

  stat_to_cpio (&fs, st);
  fs.c_name = (char*) file_name;
  delay_cpio_set_stat (&fs, invert_permissions);
}

/* Update the delayed_set_stat info for an intermediate directory
   created within the file name of DIR.  The intermediate directory turned
   out to be the same as this directory, e.g. due to ".." or symbolic
   links.  *DIR_STAT_INFO is the status of the directory.  */
int
repair_inter_delayed_set_stat (struct stat *dir_stat_info)
{
  struct delayed_set_stat *data;
  for (data = delayed_set_stat_head; data; data = data->next)
    {
      struct stat st;
      if (stat (data->stat.c_name, &st) != 0)
	{
	  stat_error (data->stat.c_name);
	  return -1;
	}

      if (st.st_dev == dir_stat_info->st_dev
	  && st.st_ino == dir_stat_info->st_ino)
	{
	  stat_to_cpio (&data->stat, dir_stat_info);
	  data->invert_permissions =
	    ((dir_stat_info->st_mode ^ st.st_mode)
	     & MODE_RWX & ~ newdir_umask);
	  return 0;
	}
    }
  return 1;
}

/* Update the delayed_set_stat info for a directory matching
   FILE_HDR.

   Return 0 if such info was found, 1 otherwise. */
int
repair_delayed_set_stat (struct cpio_file_stat *file_hdr)
{
  struct delayed_set_stat *data;
  for (data = delayed_set_stat_head; data; data = data->next)
    {
      if (strcmp (file_hdr->c_name, data->stat.c_name) == 0)
	{
	  data->invert_permissions = 0;
	  memcpy (&data->stat, file_hdr,
		  offsetof (struct cpio_file_stat, c_name));
	  return 0;
	}
    }
  return 1;
}

void
apply_delayed_set_stat ()
{
  while (delayed_set_stat_head)
    {
      struct delayed_set_stat *data = delayed_set_stat_head;
      if (data->invert_permissions)
	{
	  data->stat.c_mode ^= data->invert_permissions;
	}
      set_perms (-1, &data->stat);
      delayed_set_stat_head = data->next;
      free (data);
    }
}


static int
cpio_mkdir (struct cpio_file_stat *file_hdr, int *setstat_delayed)
{
  int rc;
  mode_t mode = file_hdr->c_mode;
  
  if (!(file_hdr->c_mode & S_IWUSR))
    {
      rc = mkdir (file_hdr->c_name, mode | S_IWUSR);
      if (rc == 0)
	{
	  delay_cpio_set_stat (file_hdr, 0);
	  *setstat_delayed = 1;
	}
    }
  else
    {
      rc = mkdir (file_hdr->c_name, mode);
      *setstat_delayed = 0;
    }
  return rc;
}

int
cpio_create_dir (struct cpio_file_stat *file_hdr, int existing_dir)
{
  int res;			/* Result of various function calls.  */
#ifdef HPUX_CDF
  int cdf_flag;                 /* True if file is a CDF.  */
  int cdf_char;                 /* Index of `+' char indicating a CDF.  */
#endif
  int setstat_delayed = 0;
  
  if (to_stdout_option)
    return 0;
  
  /* Strip any trailing `/'s off the filename; tar puts
     them on.  We might as well do it here in case anybody
     else does too, since they cause strange things to happen.  */
  strip_trailing_slashes (file_hdr->c_name);

  /* Ignore the current directory.  It must already exist,
     and we don't want to change its permission, ownership
     or time.  */
  if (file_hdr->c_name[0] == '.' && file_hdr->c_name[1] == '\0')
    {
      return 0;
    }

#ifdef HPUX_CDF
  cdf_flag = 0;
#endif
  if (!existing_dir)
    {
#ifdef HPUX_CDF
      /* If the directory name ends in a + and is SUID,
	 then it is a CDF.  Strip the trailing + from
	 the name before creating it.  */
      cdf_char = strlen (file_hdr->c_name) - 1;
      if ( (cdf_char > 0) &&
	   (file_hdr->c_mode & 04000) && 
	   (file_hdr->c_name [cdf_char] == '+') )
	{
	  file_hdr->c_name [cdf_char] = '\0';
	  cdf_flag = 1;
	}
#endif
      res = cpio_mkdir (file_hdr, &setstat_delayed);
    }
  else
    res = 0;
  if (res < 0 && create_dir_flag)
    {
      create_all_directories (file_hdr->c_name);
      res = cpio_mkdir (file_hdr, &setstat_delayed);
    }
  if (res < 0)
    {
      /* In some odd cases where the file_hdr->c_name includes `.',
	 the directory may have actually been created by
	 create_all_directories(), so the mkdir will fail
	 because the directory exists.  If that's the case,
	 don't complain about it.  */
      struct stat file_stat;
      if (errno != EEXIST)
	{
	  mkdir_error (file_hdr->c_name);
	  return -1;
	}
      if (lstat (file_hdr->c_name, &file_stat))
	{
	  stat_error (file_hdr->c_name);
	  return -1;
	}
      if (!(S_ISDIR (file_stat.st_mode)))
	{
	  error (0, 0, _("%s is not a directory"),
		 quotearg_colon (file_hdr->c_name));
	  return -1;
	}
    }

  if (!setstat_delayed && repair_delayed_set_stat (file_hdr))
    set_perms (-1, file_hdr);
  return 0;
}

void
change_dir ()
{
  if (change_directory_option && chdir (change_directory_option))
    {
      if (errno == ENOENT && create_dir_flag)
	{
	  if (make_path (change_directory_option, -1, -1,
			 (warn_option & CPIO_WARN_INTERDIR) ?
			 _("Creating directory `%s'") : NULL))
	    exit (PAXEXIT_FAILURE);

	  if (chdir (change_directory_option) == 0)
	    return;
	}
      error (PAXEXIT_FAILURE, errno,
	     _("cannot change to directory `%s'"), change_directory_option);
    }
}

/* Return true if the archive format ARF stores inode numbers */
int
arf_stores_inode_p (enum archive_format arf)
{
  switch (arf)
    {
    case arf_tar:
    case arf_ustar:
      return 0;

    default:
      break;
    }
  return 1;
}
  
