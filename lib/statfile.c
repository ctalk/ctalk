/* $Id: statfile.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2017  Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdbool.h>
#include <ctype.h>

/* From ctalk.h */
#define MAXMSG 8192

#ifdef __DJGPP__
#define PATHSEPCHAR ';'
#define DIRSEPCHAR '\\'
#define PATHSEPSTR ";"
#define DIRSEPSTR "\\"
#else 
#define PATHSEPCHAR ':'
#define DIRSEPCHAR '/'
#define PATHSEPSTR ":"
#define DIRSEPSTR "/"
#endif

/* Prototypes. */
int is_dir (char *);
char *substrcpy (char *, char *, int, int);
void __xfree(void **);
void *__xalloc (int);
int strcatx (char *, ...);
int strcatx2 (char *, ...);
int is_binary_file (char *);
/* Used by xfree<n> */
#ifndef MEMADDR
#define MEMADDR(x) ((void **)&(x))
#endif
/* from read.c */
#ifdef __DJGPP__
unsigned long int read_file (char *buf, char *path);
#else
size_t read_file (char *buf, char *path);
#endif


#ifdef __GNUC__
void _error (char *, ...) __attribute__ ((noreturn));
#else
void _error (char *, ...);
#endif

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
#define index strchr
#define rindex strrchr
#endif

int stat_err_s (char *path, struct stat *s_p) {
  int r;
  int errno_save = errno;
  r = stat (path, s_p);
  errno = errno_save;
  return r;
}

int file_exists (char *path) {

#if defined(__CYGWIN__) || defined (__DJGPP__) || defined(__APPLE__)
  DIR *d;
  struct dirent *d_ent;
  char dname[FILENAME_MAX], fname[FILENAME_MAX], *p;

  if (is_dir (path))
    return 0;

  if ((p = rindex (path, '/')) != NULL) {
    substrcpy (dname, path, 0, p - path);
    strcpy (fname, p + 1);
  } else {
    strcpy (fname, path);
    strcpy (dname, "./");
  }

  if (is_dir (dname)) {

    if ((d = opendir (dname)) == NULL)
      _error ("file_exists: %s.\n", strerror (errno));

    while ((d_ent = readdir (d)) != NULL)
      if (!strcmp (d_ent -> d_name, fname)) {
      	closedir (d);
      	return 1;
      }
    
    closedir (d);
  }
  return 0;
#else

  int r;
  struct stat statbuf;

  if (is_dir (path)) {
    return 0;
  }

  if ((r = stat_err_s (path, &statbuf)) == 0) {
    return 1;
  } else {
    return 0;
  }
#endif
}

int file_size (char *path) {
  struct stat statbuf;
  int r;

  if ((r = stat_err_s (path, &statbuf)) != 0)
    _error ("%s: %s.", path, strerror (errno));

  return statbuf.st_size;
}

int file_size_silent (char *path) {
  struct stat statbuf;
  int r;

  if ((r = stat_err_s (path, &statbuf)) != 0)
    return -1;

  return statbuf.st_size;
}

int __ctalkIsDir (char *path) { return is_dir (path); }

int is_dir (char *path) {
  struct stat statbuf;
  int r;
  
  if ((r = stat_err_s (path, &statbuf)) == 0) {
    if (S_ISDIR (statbuf.st_mode)) 
      return 1;
  }

  return 0;
}

char *which (char *prog) {
  static char pname[FILENAME_MAX];
  char path_var[MAXMSG],
    dname[FILENAME_MAX],
    *p, *q;
  DIR *d;
  struct dirent *d_ent;

  if (getenv ("PATH"))
    strcpy (path_var, getenv ("PATH"));
  else
    return NULL;

  p = path_var;
  while (p) {
    memset ((void *)dname, 0, FILENAME_MAX * sizeof (char));
    if ((q = index (p, PATHSEPCHAR)) != NULL)
      strncpy (dname, p, q - p);
    else
      strcpy (dname, p);

    if (is_dir (dname)) {

      if ((d = opendir (dname)) == NULL)
	_error ("which: %s.\n", strerror (errno));

      while ((d_ent = readdir (d)) != NULL)
	if (!strcmp (d_ent -> d_name, prog)) {
	  strcatx (pname, dname, DIRSEPSTR, d_ent -> d_name, NULL);
	  closedir (d);
	  return pname;
	}

      closedir (d);
    }

    p = (q) ? q + 1 : NULL;
  }

  return NULL;
}

char *pwd (void) {
  static char buf[FILENAME_MAX+1];
  return getcwd (buf, FILENAME_MAX);
}

char *expand_path (char *path, char *expanded_path) {
  char basename[FILENAME_MAX],
    dirpath[FILENAME_MAX],
    parentpath[FILENAME_MAX],
    username[FILENAME_MAX],
    envvarvalue[FILENAME_MAX],
    *s_ptr, *q_ptr, *env_ptr;
  struct passwd *pwent;

  if (!strpbrk (path, ".~")) {
    strcpy (expanded_path, path);
    return expanded_path;
  }

  if (!strncmp (path, "..", 2)) {
    strcpy (dirpath, pwd ());
    s_ptr = rindex (dirpath, '/');
    if (s_ptr) {
      *s_ptr = 0;
      strcpy (parentpath, dirpath);
    } else {
      *parentpath = 0;
    }
    s_ptr = index (path, '/');
    if (s_ptr) {
      sprintf (expanded_path, parentpath, "DIRSEPSTR", s_ptr + 1, NULL);
    } else {
      strcpy (expanded_path, parentpath);
    }
    return expanded_path;
  }

  if (*path == '.') {
    s_ptr = index (path, '/');
    if (s_ptr) {
      strcatx (expanded_path, pwd (), DIRSEPSTR, s_ptr + 1, NULL);
      return expanded_path;
    } else {
      strcpy (expanded_path, pwd ());
      return expanded_path;
    }
  }

  /* TODO - Handle ~ in the middle of a path if needed. */
  if (path[0] == '~') {
    if (path[1] == '/') {
      strcatx (expanded_path, getenv ("HOME"), DIRSEPSTR, 
	       index (path, '/') + 1, NULL);
      return expanded_path;
    } else {
      if ((s_ptr = index (path, '/')) != NULL) {
	substrcpy (username, &path[1], 0, s_ptr - &path[1]);
      } else {
	strcpy (username,  &path[1]);
      }
      if ((pwent = getpwnam (username)) != NULL) {
	strcatx (expanded_path, pwent -> pw_dir, DIRSEPSTR,
		 index (path, '/') +  1, NULL);
	return expanded_path;
      }
    }
  }

  if ((s_ptr = strchr (path, '$')) != NULL) {
    strncpy (expanded_path, path, s_ptr - path);
    for (q_ptr = s_ptr + 1; isalnum((int)*q_ptr) || *q_ptr == '_'; ++q_ptr)
      ;
    strncpy (envvarvalue, s_ptr + 1, q_ptr - (s_ptr + 1));
    if ((env_ptr = getenv (envvarvalue)) != NULL) {
      strcatx2 (expanded_path, env_ptr, NULL);
    }
    strcatx2 (expanded_path, q_ptr, NULL);
    return expanded_path;
  }

  strcpy (expanded_path, path);
  return path;
}

int file_mtime (char *path) {
  struct stat statbuf;
  int r;
  
  if ((r = stat_err_s (path, &statbuf)) == 0) {
    return (int) statbuf.st_mtime;
  }
  return r;
}

char *basename_w_extent (char *s) {
  char *q;
  if ((q = rindex (s, DIRSEPCHAR)) != NULL) {
    return ++q;
  } else {
    return s;
  }
  return NULL;
}

bool is_shell_script (char *file_or_path) {
  char buf[512];
  if (!file_exists (file_or_path))
    return false;
  if (is_binary_file (file_or_path))
    return false;
  read_file (buf, file_or_path);
  if (strstr (buf, "#!")) {
    if (strstr (buf, "/bin/sh") || strstr (buf, "/bin/bash")) {
      return true;
    }
  }
  return false;
}

bool file_has_exec_permissions (char *file_or_path) {
  struct stat statbuf;
  if (!stat (file_or_path, &statbuf)) {
    if (S_IXUSR & statbuf.st_mode || S_IXGRP & statbuf.st_mode ||
	S_IXOTH & statbuf.st_mode) {
      return true;
    }
  }
  return false;
}
