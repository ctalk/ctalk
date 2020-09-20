/* $Id: read.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011 Robert Kiesling, rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright 
  notice is included in all source code modules.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "ctpp.h"

#ifndef HAVE_OFF_T
int input_size;          /* Global size of the input - used in main.c. */
#else
off_t input_size;
#endif

/* 
 *  Return the number of bytes read, or -1.
 *
 *  If we can stat () an input file, then the function
 *  uses calloc () to size the buffer.  If the input is from stdin,
 *  then the function reads the stream to a temporary file, and 
 *  then allocates the input buffer from the temporary file.
 *
 *  Here also the program should free () the buffer after the input is 
 *  processed, although the program only processes one input at a time.
 */

#if  defined(__CYGWIN32__) || defined(__DJGPP__) || defined(__APPLE__)
int read_file (char **buf, char *path) {
#else
size_t read_file (char **buf, char *path) {
#endif

  struct stat statbuf;
  int r_stat = -1;
  int stdin_fileno;
#if defined(__CYGWIN32__) || defined(__DJGPP__) || defined(__APPLE__)
  int chars_read;
  int bytes_read;
#else
  size_t chars_read;
  ssize_t bytes_read;
#endif

#ifndef HAVE_OFF_T
  int stat_size;
#else
  off_t stat_size;
#endif

  char streambuf[IO_BLKSIZE+1];
  FILE *f;

  if (!strcmp (path, "-")) {
    if ((stdin_fileno = fileno (stdin)) == ERROR)
      _error ("stdin: %s.\n", strerror (errno));

    create_tmp ();

    memset ((void *)streambuf, 0, sizeof (char) * IO_BLKSIZE);

    while ((bytes_read = read(stdin_fileno, streambuf, IO_BLKSIZE)) > 0) {
      streambuf[bytes_read] = 0;
      write_tmp (streambuf);
      memset ((void *)streambuf, 0, sizeof (char) * IO_BLKSIZE);
    }

    close_tmp ();

    if ((r_stat = stat (get_tmpname (), &statbuf)) != 0)
      return r_stat;
    stat_size = input_size = statbuf.st_size;

    if (!stat_size) {
      unlink_tmp ();
      return ERROR;
    }

    if ((*buf = (char *)calloc ((size_t)(stat_size + 1), 
				sizeof(char))) == NULL)
      _error ("%s: %s.\n", path, strerror (errno));

    if ((f = fopen (get_tmpname (), FILE_READ_MODE)) == NULL) 
      return ERROR;

#ifdef __DJGPP__
    /*
     *  DJGPP uses a text-mode read, and it translates line
     *  endings, so the function needs to rely on errno to
     *  determine if there was an error.
     */
    errno = 0;
    fread ((void *)*buf, sizeof(char), (size_t) stat_size, f);
    if (errno)
      _warning ("read_file: %s: %s.\n", path, strerror (errno));
#else
    if ((chars_read = 
 	 fread ((void *)*buf, sizeof(char), (size_t) stat_size, f))
 	!= stat_size)
      _warning ("read_file: %s: %s.\n", path, strerror (errno));
#endif

    fclose (f);

    unlink_tmp ();

  } else {

    if ((r_stat = stat (path, &statbuf)) != 0)
      return r_stat;

    stat_size = statbuf.st_size;

    /* Check for directories in cases where '.' is an overloaded operator,
       in which case it gets statted as a file during library searches. */
    if (S_ISDIR (statbuf.st_mode)) {
      _warning ("read_file: %s is a directory.\n", path);
      exit (EXIT_FAILURE);
    }

    if ((*buf = (char *)calloc ((size_t)(stat_size + 1), sizeof(char))) == NULL)
      _error ("%s: %s.\n", path, strerror (errno));

    if ((f = fopen (path, FILE_READ_MODE)) == NULL) 
      return ERROR;

#ifdef __DJGPP__
    /*
     *  Here also, DJGPP uses a text-mode read, and it translates 
     *  line endings, so the function needs to rely on errno to
     *  determine if there was an error.
     */
    errno = 0;
    fread ((void *)*buf, sizeof(char), (size_t) stat_size, f);
    if (errno)
      _warning ("read_file: %s: %s.\n", path, strerror (errno));
#else
    if ((chars_read = 
	 fread ((void *)*buf, sizeof(char), (size_t) stat_size, f)) 
	!= stat_size)
      _warning ("read_file: %s: %s.\n", path, strerror (errno));
#endif

    fclose (f);
  }

  return chars_read;
}

