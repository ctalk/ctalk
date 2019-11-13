/* $Id: read.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2015, 2017 Robert Kiesling, rk3314042@gmail.com.
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
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#if HAVE_GNU_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

/*
 *  DJGPP can perform line-ending translation, so the functions
 *  below need to rely on errno, not the number of bytes read,
 *  to determine if an error occurred.
 */
#ifdef __DJGPP__

unsigned long int __perform_read (FILE *f, char buf[], 
				  long long int _stat_size) {
  unsigned long int __chars_read;
  errno = 0;
  __chars_read = fread ((void *)buf, sizeof(char), (size_t) _stat_size, f);
  if (errno)
    return -1;
  return __chars_read;
}

#else

size_t __perform_read (FILE *f, char buf[], long long int _stat_size) {
  size_t __chars_read;
  if ((__chars_read = fread ((void *)buf, sizeof(char), (size_t) 
			     _stat_size, f)) 
      != _stat_size)
    return -1;
  /* We sometimes get crud at the end of a read.... */
  buf[__chars_read] = 0;
  return __chars_read;
}

#endif  /* #defined __DJGPP__ */

/* 
 *  Return the number of bytes read, or -1.
 */
#ifdef __DJGPP__
unsigned long int read_file (char *buf, char *path) {
#else
size_t read_file (char *buf, char *path) {
#endif

  struct stat statbuf;
  int r_stat = -1;

  size_t chars_read;

#ifndef HAVE_OFF_T
  int stat_size;
#else
  off_t stat_size;
#endif
  FILE *f;

  if ((r_stat = stat (path, &statbuf)) != 0) 
    return r_stat;
  stat_size = statbuf.st_size;

  /* Check for directories in cases where '.' is an overloaded operator,
     in which case it gets statted as a file during library searches. */
  if (S_ISDIR (statbuf.st_mode)) {
    _warning ("read_file: %s is a directory.\n", path);
    return ERROR;
  }
  if ((f = fopen (path, FILE_READ_MODE)) == NULL) 
    return ERROR;
  chars_read = __perform_read (f, buf, (long long int)stat_size);
  if (chars_read == -1)
    _warning ("read_file: %s: %s.", path, strerror (errno));
    
  fclose (f);

  return chars_read;
}

/*
 *  Ctpp version of read_file (), which allows for input from stdin.
 *  If the input is from stdin, then the function reads the stream to
 *  a temporary file, and then allocates the input buffer from the
 *  temporary file.
 *
 *  Here also the program should free () the buffer after the input is 
 *  processed, although the program only processes one input at a time.
 */

#ifdef __DJGPP__
unsigned long int p_read_file (char **buf, char *path) {
#else
size_t p_read_file (char **buf, char *path) {
#endif

  struct stat statbuf;
  int r_stat = -1;
  int stdin_fileno;
#ifdef __DJGPP__
  unsigned long int chars_read;
#else
  size_t chars_read;
#endif
  ssize_t bytes_read;
#ifndef HAVE_OFF_T
  int stat_size;
#else
  off_t stat_size;
#endif
  char streambuf[IO_BLKSIZE+1];
  FILE *f;

  if (!strcmp (path, "-")) {
    if ((stdin_fileno = fileno (stdin)) == ERROR)
      _error ("stdin: %s.", strerror (errno));
    create_tmp ();
    while ((bytes_read = read(stdin_fileno, streambuf, IO_BLKSIZE)) > 0) {
      streambuf[bytes_read] = 0;
      write_tmp (streambuf);
    }

    close_tmp ();

    if ((r_stat = stat (get_tmpname (), &statbuf)) != 0)
      return r_stat;
    stat_size = statbuf.st_size;

    if ((*buf = (char *)__xalloc (stat_size + 1 * sizeof(char))) == NULL)
      _error ("%s: %s.", path, strerror (errno));

    if ((f = fopen (get_tmpname (), FILE_READ_MODE)) == NULL) 
      return ERROR;
    chars_read = __perform_read (f, *buf, (long long int)stat_size);
    if (chars_read == -1)
      _warning ("p_read_file: %s: %s.", path, strerror (errno));
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
      return ERROR;
    }

    if ((*buf = (char *)__xalloc (stat_size + 1 * sizeof(char))) == NULL)
      _error ("%s: %s.", path, strerror (errno));

    if ((f = fopen (path, FILE_READ_MODE)) == NULL) 
      return ERROR;
    chars_read = __perform_read (f, *buf, (long long int) stat_size);
    if (chars_read == -1)
      _warning ("p_read_file: %s: %s.", path, strerror (errno));
    fclose (f);
  }

  return chars_read;
}

int is_binary_file (char *path) {
  FILE *f;
  char buf[64+1];
  unsigned long int __chars_read;
  int r;
  errno = 0;
  if ((f = fopen (path, "r")) == NULL) {
    if (errno != ENOENT)
      _warning ("is_binary_file: %s: %s.\n", path, strerror (errno));
    return FALSE;
  }
  __chars_read = fread ((void *)buf, sizeof(char), (size_t) 64, f);
#ifdef __DJGPP__
  if (!memcmp ((const void *)&buf[0], (const void *)"M", 
	       (unsigned long int)1) &&
      !memcmp ((const void *)&buf[1], (const void *)"Z", 
	       (unsigned long int)1))
    r = TRUE;
  else
    r = FALSE;
#else
# ifdef __APPLE__
  if ((((unsigned int)buf[0] & 0xff) == 0xfe) && 
      (((unsigned int)buf[1] & 0xff) == 0xed) && 
      (((unsigned int)buf[2] & 0xff) == 0xfa) && 
      (((unsigned int)buf[3] & 0xff) == 0xce)) {
    r = TRUE;
  } else {
    r = FALSE;
  }
# else
  if (!memcmp ((const void *)&buf[1], (const void *)"ELF", 
	       (unsigned long int)3))
    r = TRUE;
  else
    r = FALSE;
# endif  /* # ifdef __APPLE__ */
#endif /* #ifdef __DJGPP */
  fclose (f);
  return r;
}

/* Providing a wrapper function here avoids the complexity
   of arranging the Readline API in the class libraries. */
void __ctalkConsoleReadLine (OBJECT *self, char *promptString) {
#if HAVE_GNU_READLINE
  char *buf;
  buf = readline (promptString);
  __ctalkSetObjectValueVar (self, buf);
  add_history (buf);
  __xfree (MEMADDR(buf));
#else  
  char buf[MAXMSG], *r, *s;
  printf (promptString);
  fflush (stdout);
  s = fgets (buf, MAXMSG, stdin);
  if ((r = strchr (buf, '\n')) != NULL)
    *r = '\0';
  __ctalkSetObjectValueVar (self, buf);
#endif
}
