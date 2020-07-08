/* $Id: tempio.c,v 1.3 2020/07/08 02:02:01 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2014, 2020  Robert Kiesling, rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include "pmessage.h"
#include "ctpp.h"

extern int file_size (char *);

char output_file[FILENAME_MAX];   /* Name of the output file.              */
char tmp_output[FILENAME_MAX];    /* Temporary name of old output file.    */

/* From rt_error.c. */
void _error (char *, ...);

/* From substrcpy.c */
char *substrcpy (char *, char *, int, int);

/* From statfile.c */
int file_exists (char *);

/* From tempname.c */
char *__tempname (char *d_path, char *pfx);

extern char output_file[FILENAME_MAX];

static char *tmp_path[MAXARGS];          /* Tmp paths stack.               */
static int tmp_fileno[MAXARGS];          /* Tmp file numbers.              */
static FILE *tmp_file[MAXARGS];          /* Tmp files descriptors.         */
static int tmp_path_ptr;                 /* Points to first empty entry    */
                                         /* in tmp_paths[].                */
static int max_tmp_paths;                /* The maximum number of temp     */
                                         /* files open in this session.    */
                                         /* Used by cleanup ().            */

#define TMPFILE_PFX     "c."             /* Generic tmp prefix from ctpp.h.*/

void init_tmp_files (void) { tmp_path_ptr = max_tmp_paths = 0; }

void create_tmp (void) {

  char *newpath;

  newpath = __tempname (P_tmpdir, TMPFILE_PFX);

  if ((tmp_file[tmp_path_ptr] = fopen (newpath, FILE_WRITE_MODE)) != NULL) {
    tmp_fileno[tmp_path_ptr] = fileno (tmp_file[tmp_path_ptr]);
    tmp_path[tmp_path_ptr] = strdup (newpath);
    tmp_path_ptr++;
    if (tmp_path_ptr > max_tmp_paths)
      max_tmp_paths = tmp_path_ptr;
  } else {
    _error ("create_tmp: %s: %s.\n", newpath, strerror (errno));
  }
}

#define LAST_TMP_IDX (tmp_path_ptr-1)

int unlink_tmp (void) {

  int r;
  
  /*
   *  fileno returns -1 if the file is already closed.
   */
  if (tmp_fileno[LAST_TMP_IDX] != -1) {
    if ((r = fileno (tmp_file[LAST_TMP_IDX])) != -1) {
      if (r == tmp_fileno[LAST_TMP_IDX])
	fclose (tmp_file[LAST_TMP_IDX]);
    }
  }

  if (unlink (tmp_path[LAST_TMP_IDX]) == 0) {
    free(tmp_path[LAST_TMP_IDX]);
    tmp_path_ptr --;
  } else {
    _error ("unlink_tmp: %s: %s.\n", tmp_path[LAST_TMP_IDX], strerror (errno));
  }
  return SUCCESS;
}

int close_tmp (void) {

  if (fileno (tmp_file[LAST_TMP_IDX]) != tmp_fileno[LAST_TMP_IDX])
    return ERROR;
  if (fclose (tmp_file[LAST_TMP_IDX]) == -1)
    _error ("close_tmp: %s, %s.\n", tmp_path[LAST_TMP_IDX], strerror (errno));
  tmp_fileno[LAST_TMP_IDX] = -1;
  return SUCCESS;
}

int write_tmp (char *s) {

  int r; 

  if (fileno (tmp_file[LAST_TMP_IDX]) == -1)
    _error ("write_tmp: %s: %s.\n", tmp_path[LAST_TMP_IDX], "Temp file closed.");
  if ((r = fputs (s, tmp_file[LAST_TMP_IDX])) == EOF)
    _error ("write_tmp: %s: %s.\n", tmp_path[LAST_TMP_IDX], strerror (errno));

  return 0;
}

char *get_tmpname (void) {
  return tmp_path[tmp_path_ptr - 1];
}

void tmp_to_output (void) {

  FILE *tmp_f, *output_f;
  int output_fileno;
  int tmp_size;
  char *buf;
  int r;

  if ((tmp_f = fopen (tmp_path[LAST_TMP_IDX], "r")) == NULL)
    _error ("tmp_to_output %s: %s.\n", tmp_path[LAST_TMP_IDX], strerror (errno));

  if (strcmp (output_file, "-")) {

    if ((output_f = fopen (output_file, FILE_WRITE_MODE)) == NULL)
      _error ("tmp_to_output %s: %s.\n", tmp_path[LAST_TMP_IDX], strerror (errno));
    output_fileno = fileno (output_f);
  } else {
     output_fileno = dup (1);   /* Standard output. */
  }

  tmp_size = file_size (tmp_path[LAST_TMP_IDX]);

  buf = calloc (tmp_size + 1, sizeof (char));

  if ((r = read (fileno (tmp_f), buf, tmp_size)) == 0) {
    if (!feof (tmp_f)) {
      printf ("tmp_to_output (read): %s.\n", strerror (errno));
      free (buf);
      fclose (tmp_f);
      close (output_fileno);
    }
  }

  if ((r = write (output_fileno, buf, tmp_size)) < 0) {
    printf ("tmp_to_output (write): %s.\n", strerror (errno));
  }

  free (buf);
  fclose (tmp_f);
  close (output_fileno);

}

/*
 *  If there is a previous output file, save it under a temporary
 *  name.  
 */

int rename_file (char *oldname, char *newname) {

  FILE *old_f, *new_f;
  char c;
  int r;

  if ((old_f = fopen (oldname, FILE_READ_MODE)) == NULL)
    return ERROR;

  if ((new_f = fopen (newname, FILE_WRITE_MODE)) == NULL)
    return ERROR;

  while ((r = fread (&c, sizeof (char), 1, old_f)) != 0)
    fwrite (&c, sizeof (char), 1, new_f);

  fclose (old_f);
  fclose (new_f);

  if ((r = unlink (oldname)) == -1)
    fprintf (stderr, "%s: %s.\n", oldname, strerror (errno));

  return SUCCESS;
}

/*
 *  If there is a previous output file, save it under a temporary
 *  name.  
 */

void copy_file (char *oldname, char *newname) {

  FILE *old_f, *new_f;
  char c;
  int r;

  if ((old_f = fopen (oldname, FILE_READ_MODE)) == NULL) {
    fprintf (stderr, "Copy %s %s: %s\n", oldname, newname, strerror(errno));
    exit (EXIT_FAILURE);
  }

  if ((new_f = fopen (newname, FILE_WRITE_MODE)) == NULL) {
    fprintf (stderr, "Copy %s %s: %s\n", oldname, newname, strerror(errno));
    exit (EXIT_FAILURE);
  }

  while ((r = fread (&c, sizeof (char), 1, old_f)) != 0)
    fwrite (&c, sizeof (char), 1, new_f);

  fclose (old_f);
  fclose (new_f);

}

/* 
 *  Clean up any temporary files.  If there's an error, also 
 *  delete the output file if any.  For DJGPP, ensure that the 
 *  temporary file directory entry matches whatever tempnam () 
 *  mapped it to, by comparing it against the generated temporary 
 *  names.
 */

void cleanup (int unlink_output) {

  char tmpdir[FILENAME_MAX];
  char tmpname[FILENAME_MAX + 11]; /* FILENAME_MAX + P_tmpdir */
  char pidbuf[64];
  DIR *dir;
  struct dirent *d;
#ifdef __DJGPP__
  int i;
#endif

  strcpy (tmpdir, P_tmpdir);
  sprintf (pidbuf, "%d", getpid ());
  if ((dir = opendir (tmpdir)) != NULL) {

    while ((d = readdir (dir)) != NULL) {
#ifdef __DJGPP__
      for (i = 0; i < tmp_path_ptr; i++) {
	if (!strncmp (d -> d_name, basename (tmp_path[i]), 3)) {
	  sprintf (tmpname, "%s%s", P_tmpdir, d -> d_name);
	  unlink (tmpname);
	}
      }
#else
      /* if (!strncmp (d -> d_name, TMPFILE_PFX, strlen (TMPFILE_PFX))) { */
      if (!strncmp (d -> d_name, TMPFILE_PFX, strlen (TMPFILE_PFX)) &&
	  strstr (d -> d_name, pidbuf)) {
	sprintf (tmpname, "%s/%s", P_tmpdir, d -> d_name);
	unlink (tmpname);
      }
#endif
    }

    closedir (dir);

  } else {
    _error ("cleanup: %s: %s\n", P_tmpdir, strerror (errno));
  }
  
  if (unlink_output) {
    if (file_exists (output_file))
      unlink (output_file);
    rename_file (tmp_output, output_file);
  } else {
    if (file_exists (tmp_output))
      unlink (tmp_output);
  }

}

int output_djgpp (MESSAGE_STACK messages, int start, int end) {

  int i;

  if (*output_file == '-') {
    for (i = start; i > end; i--)
      fprintf (stdout, "%s", M_NAME(messages[i]));
  } else {
    FILE *f;
    int r;
    if ((f = fopen (output_file, FILE_WRITE_MODE)) == NULL)
      _error ("%s: %s.\n", output_file, strerror (errno));
    for (i = start; i > end; i--) {
      if ((r = fputs (M_NAME(messages[i]), f)) == EOF) {
	_error ("%s: %s.\n", output_file, strerror (errno));
      }
    }
    fclose (f);
  }
  return SUCCESS;
}
