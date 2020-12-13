/* $Id: tempio.c,v 1.1.1.1 2020/12/13 14:51:03 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2016, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"

#define SOURCE_EXT     ".c"
#define PREPROCESS_EXT ".i"
#define CTPP_EXT       ".ctpp"

int outputfile_opt;               /* User defined output file.             */
char output_file[FILENAME_MAX];   /* Name of the output file.              */
char tmp_output[FILENAME_MAX];    /* Temporary name of old output file.    */

/* From rt_error.c. */
void _error (char *, ...);

/* From substrcpy.c */
char *substrcpy (char *, char *, int, int);

/* From statfile.c */
int file_exists (char *);

/* From tempname.c */
char *__tempname (char *, char *);
char *random_string (void);

extern int outputfile_opt;               /* Declared in main.c.            */
extern char output_file[FILENAME_MAX];

typedef struct _tempfile {
  char tmp_path[FILENAME_MAX];
  FILE *tmp_fp;
} TEMPFILE;

TEMPFILE *tmp_files;

static int tmp_path_ptr;                 /* Points to first empty entry    */
                                         /* in tmp_paths[].                */
static int max_tmp_paths;                /* The maximum number of temp     */
                                         /* files open in this session.    */
                                         /* Used by cleanup ().            */
static char 
tmp_macro_path[FILENAME_MAX];            /* Pathname of the current macro  */
                                         /* cache.                         */

#define TMPFILE_PFX     "c."             /* Generic tmp prefix.            */
#define TMPFILE_PFX_LENGTH 2
#define TMPDEFINES_PFX  "cm."
#define TMPDEFINES_PFX_LENGTH 3

#define INITIAL_BUFSIZ 0x8888

void init_tmp_files (void) { 
  tmp_path_ptr = max_tmp_paths = 0; 
  memset ((void *)tmp_macro_path, 0, FILENAME_MAX * sizeof (char));
  if ((tmp_files = (TEMPFILE *)__xalloc (MAXARGS * sizeof (struct _tempfile)))
      == NULL)
    _error ("init_tmp_files (__xalloc): %s.\n", strerror (errno));
}

extern char tmpname_pidbuf[64]; /* declared in tempname.c */

int create_tmp (void) {

  int r = SUCCESS;

  __tempname (TMPFILE_PFX, tmp_files[tmp_path_ptr].tmp_path);

  if ((tmp_files[tmp_path_ptr].tmp_fp = 
       fopen (tmp_files[tmp_path_ptr].tmp_path, FILE_WRITE_MODE))
      != NULL) {

    tmp_path_ptr++;
  } else {
    __ctalkWarning ("%s: %s.", tmp_files[tmp_path_ptr].tmp_path,
		    strerror (errno));
    r = ERROR;
  }
  return r;
}

#define LAST_TMP_IDX (tmp_path_ptr - 1)

int unlink_tmp (void) {

  if (tmp_path_ptr <= 0)
    return SUCCESS;

  if (tmp_files[LAST_TMP_IDX].tmp_fp != NULL) {
    fclose (tmp_files[LAST_TMP_IDX].tmp_fp);
    tmp_files[LAST_TMP_IDX].tmp_fp = NULL;
  }

  if (unlink (tmp_files[LAST_TMP_IDX].tmp_path) == 0) {
    tmp_files[LAST_TMP_IDX].tmp_fp = NULL;
    tmp_path_ptr--;
  } else {
    _error ("unlink_temp: %s: %s.\n", tmp_files[LAST_TMP_IDX].tmp_path, 
	    strerror (errno));
  }

  return SUCCESS;
}

void remove_tmpfile_entry (void) {
  tmp_files[LAST_TMP_IDX].tmp_fp = NULL;
  tmp_files[LAST_TMP_IDX].tmp_path[0] = '\0';
  tmp_path_ptr--;
}

int close_tmp (void) {

  int r;

  if (fileno (tmp_files[LAST_TMP_IDX].tmp_fp) < 0)
    return ERROR;

  if (fclose (tmp_files[LAST_TMP_IDX].tmp_fp) == -1)
    _error ("close_tmp(fclose): %s, %s.\n", 
	    tmp_files[LAST_TMP_IDX].tmp_path, strerror (errno));

  tmp_files[LAST_TMP_IDX].tmp_fp = NULL;

  return SUCCESS;
}

int write_tmp (char *s) {

  fputs (s, tmp_files[LAST_TMP_IDX].tmp_fp);

  return 0;

}

char *get_tmpname (void) {
  return tmp_files[tmp_path_ptr - 1].tmp_path;
}

int rename_file (char *oldname, char *newname) {

  FILE *old_f, *new_f;
  char c;
  char *s;
  int r, fsize;

  if ((fsize = file_size_silent (oldname)) == ERROR) 
    return ERROR;

  if ((old_f = fopen (oldname, FILE_READ_MODE)) == NULL)
    return ERROR;

  if ((new_f = fopen (newname, FILE_WRITE_MODE)) == NULL)
    return ERROR;

  if ((s = (char *)__xalloc (fsize)) == NULL)
    return ERROR;

  while ((r = fread (s, sizeof (char), fsize, old_f)) != 0)
    fwrite (s, sizeof (char), fsize, new_f);

  __xfree (MEMADDR(s));

  fclose (old_f);
  fclose (new_f);

  if ((r = unlink (oldname)) == -1)
    fprintf (stderr, "%s: %s.\n", oldname, strerror (errno));

  return SUCCESS;
}

int save_previous_output (char *filename) {

  char *ext_ptr;
  char basename[FILENAME_MAX];

  if (file_exists (filename)) {

    if ((ext_ptr = index (filename, '.')) != NULL) {
      substrcpy (basename, filename, 0, ext_ptr - filename);
      strcatx (tmp_output, basename, ".tmp", NULL); 
      rename_file (filename, tmp_output);
    }
  }
  return SUCCESS;
}

/* 
 *  Clean up any temporary files.  If there's an error, also 
 *  delete the output file if any.  For DJGPP, ensure that the 
 *  temporary file directory entry matches whatever tempnam () 
 *  mapped it to, by comparing it against the generated temporary 
 *  names.
 */

void cleanup_temp_files (int unlink_output) {

  char tmpdir[FILENAME_MAX];
  char tmpname[FILENAME_MAX];
  DIR *dir;
  struct dirent *d;
#ifdef __DJGPP__
  int i;
#endif

  if (*tmpname_pidbuf == 0)
    __ctalkDecimalIntegerToASCII (getpid (), tmpname_pidbuf);

  if ((dir = opendir (P_tmpdir)) != NULL) {

    while ((d = readdir (dir)) != NULL) {
#ifdef __DJGPP__
      for (i = 0; i < tmp_path_ptr; i++) {
	if (!strncmp (d -> d_name, basename (tmp_path[i]), 3)) {
	  strcatx (tmpname, P_tmpdir, d -> d_name, NULL);
	  unlink (tmpname);
	}
      }
#else
      if (!strncmp (d -> d_name, TMPFILE_PFX, TMPFILE_PFX_LENGTH) &&
	  strstr (d -> d_name, tmpname_pidbuf)) {
	strcatx (tmpname, P_tmpdir, "/", d -> d_name, NULL);
	unlink (tmpname);
      }
#endif
    }
    closedir (dir);
  } else {
    _error ("cleanup_temp_files: %s: %s\n", P_tmpdir, strerror (errno));
  }
  
  if (outputfile_opt && unlink_output) {
    if (file_exists (output_file))
      unlink (output_file);
    rename_file (tmp_output, output_file);
  } else {
    if (file_exists (tmp_output))
      unlink (tmp_output);
  }
  __xfree (MEMADDR(tmp_files));
}

void cleanup_preprocessor_cache (void) {
  char tmpdir[FILENAME_MAX];
  char tmpname[FILENAME_MAX];
  DIR *dir;
  struct dirent *d;
  if ((dir = opendir (P_tmpdir)) != NULL) {
    while ((d = readdir (dir)) != NULL) {
      if (!strncmp (d -> d_name, TMPDEFINES_PFX, TMPDEFINES_PFX_LENGTH)) {
	strcatx (tmpname, P_tmpdir, "/", d -> d_name, NULL);
  	unlink (tmpname);
      }
    }
    closedir (dir);
  } else {
    _error ("cleanup_preprocessor_cache: %s: %s\n", P_tmpdir, strerror (errno));
  }
}

char *preprocess_name (char *fn, int path) {

  static char pname[FILENAME_MAX],
    ppath[FILENAME_MAX],
    *dot,
    *slash;

  if ((slash = rindex (fn, '/')) != NULL) {
    strcpy (pname, ++slash);
  } else {
    strcpy (pname, fn);
  }

  if (((dot = rindex (pname, '.')) != NULL) &&
      (!strncmp (dot, SOURCE_EXT, 2)))
    *dot = 0;

  if (path) {
    strcatx (ppath, P_tmpdir, "/", pname, PREPROCESS_EXT, NULL);
    return ppath;
  } else {
    strcatx2 (pname, PREPROCESS_EXT, NULL);
    return pname;
  }

  return pname;
}

char *ctpp_name (char *fn, int path) {

  static char pname[FILENAME_MAX],
    ppath[FILENAME_MAX], hbuf[64],
    *dot,
    *slash;
  struct stat statbuf;
  int r;

  if ((slash = rindex (fn, '/')) != NULL) {
    strcpy (pname, ++slash);
  } else {
    strcpy (pname, fn);
  }

  if (((dot = rindex (pname, '.')) != NULL) &&
      (!strncmp (dot, SOURCE_EXT, 2)))
    *dot = 0;

  while (1) {
    if (path) {
      strcatx (ppath, P_tmpdir, "/", pname, CTPP_EXT, ".",
	       random_string (), NULL);
    } else {
      strcatx (ppath, pname, CTPP_EXT, ".", random_string (),
	       NULL);
    }
    if ((r = stat (ppath, &statbuf)) != 0)
      return ppath;
  }

  return NULL;
}

char *ctpp_tmp_name (char *fn, int path) {

  static char pname[FILENAME_MAX],
    ppath[FILENAME_MAX],
    hbuf[64],*dot, *slash;
  struct stat statbuf;
  int r;

  if (*tmpname_pidbuf == 0) {
    ctitoa (getpid (), tmpname_pidbuf);
  }

  if ((slash = rindex (fn, '/')) != NULL) {
    strcpy (pname, ++slash);
  } else {
    strcpy (pname, fn);
  }

  if (((dot = rindex (pname, '.')) != NULL) &&
      (!strncmp (dot, SOURCE_EXT, 2)))
    *dot = 0;

  while (1) {
    if (path) {
      strcatx (ppath, P_tmpdir, "/", pname, CTPP_EXT, ".",
	       tmpname_pidbuf, ".", 
	       random_string (), NULL);
    } else {
      strcatx (ppath, pname, CTPP_EXT, ".", tmpname_pidbuf,
	       ".", random_string (), NULL);
    }
    if ((r = stat (ppath, &statbuf)) != 0)
      return ppath;
  }

  return NULL;
}

char *new_macro_cache_path (void) {
  __tempname (TMPDEFINES_PFX, tmp_macro_path);
  return tmp_macro_path;
}

char *macro_cache_path (void) {
  if (*tmp_macro_path == '\0') return NULL;
  return tmp_macro_path;
}

FILE *fopen_tmp (char *path, char *mode) {
  static FILE *f;  

  if ((f = fopen (path, mode)) == NULL)
    _error ("open_tmp: %s.\n", strerror (errno));
  return f;
}

int fclose_tmp (FILE *f) {
  return fclose (f);
}
