/* $Id: include.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2016 Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#ifdef __DJGPP__
#include <dirent.h>
#endif
#include "object.h"
#include "message.h"
#include "ctalk.h"

extern int include_dir_opt;       /* Declared in main.c.       */
extern int n_user_include_dirs;        /* Specified with -I option. */
extern char *user_include_dirs[MAXUSERDIRS];
extern int systeminc_opt;         /* Specified with -s option. */
extern int n_system_inc_dirs;
extern char *systeminc_dirs[MAXUSERDIRS]; 

extern char *classlibdir;         /* Declared in rtinfo.c, read-only.      */
extern char *pkgname;
extern char libcachedir[FILENAME_MAX];
extern char ctalkuserhomedir[FILENAME_MAX];
extern char usertemplatedir[FILENAME_MAX];

extern char *library_include_paths[MAXUSERDIRS];   /* Declared in rtinfo.c. */

int ctalkdefs_h_lines = 0;

#ifdef __DJGPP__
extern int __opendir_flags;
#endif

static char **last_classlib_path_ptr;

char *find_library_include (char *fname, int find_next) {
  static char pathbuf[FILENAME_MAX];
  if (find_next) {
    ++last_classlib_path_ptr;
    while (*last_classlib_path_ptr) {
      strcatx (pathbuf, *last_classlib_path_ptr, "/", fname, NULL);
      if (file_exists(pathbuf)) {
	return pathbuf;
      }
      ++last_classlib_path_ptr;
    }
    /* In case we get another find_next after an unsuccessful find_next. */
    last_classlib_path_ptr = library_include_paths;
   } else {
     last_classlib_path_ptr = library_include_paths;
     while (*last_classlib_path_ptr) {
       strcatx (pathbuf, *last_classlib_path_ptr, "/", fname, NULL);
       if (file_exists(pathbuf)) {
	 return pathbuf;
       }
       ++last_classlib_path_ptr;
     }
   }
  return NULL;
}

void init_library_paths (void) {
  int i;
  char libdir[FILENAME_MAX],
    libenvdir[FILENAME_MAX],
    libenvdir2[FILENAME_MAX],
    libenvdir3[FILENAME_MAX], *p, *q;
  char path_out[FILENAME_MAX];
  int n_paths;

  memset (library_include_paths, 0, MAXUSERDIRS * sizeof (char *));

  for (i = 0, n_paths = 0; i < n_user_include_dirs; i++) {
    expand_path (user_include_dirs[i], libdir);
    if (is_dir (libdir))
	library_include_paths[n_paths++] = strdup (libdir);
    strcatx (libdir, expand_path(user_include_dirs[i], path_out), "/",
	     pkgname, NULL);
    if (is_dir (libdir))
      library_include_paths[n_paths++] = strdup (libdir);
  }

  if ((p = getenv ("CLASSLIBDIRS")) != NULL) {
    do {
      q = strchr (p, ':');
      if (q) {
	substrcpy (libenvdir, p, 0, q - p);
	p = q + 1;
      } else {
	strcpy (libenvdir, p);
      }
      expand_path (libenvdir, libenvdir2);
      library_include_paths[n_paths++] = strdup (libenvdir2);
      strcatx (libenvdir3, libenvdir2, "/", pkgname, NULL);
      if (is_dir (libenvdir3)) {
	library_include_paths[n_paths++] = strdup (libenvdir3);
      }
    } while (q != NULL);
  }

  library_include_paths[n_paths++] = strdup (classlibdir);

  strcatx (libdir, classlibdir, "/", pkgname, NULL);
  if (is_dir (libdir)) {
    library_include_paths[n_paths++] = strdup (libdir);
  }

  get_class_names (library_include_paths, n_paths);
}

/*
 *  init_user_home_path (), below must be called first.
 */
void init_lib_cache_path (void) {
  char buf[FILENAME_MAX];
#ifdef DJGPP
  strcpy (libcache_path, "/djgpp/include/ctalk/libcache");
  if (file_exists (libcachedir) &&
      !is_dir (libcachedir))
    unlink (libcachedir);
  strcpy (libcachedir, buf);
  if (!file_exists (libcachedir)) {
    mkdir (buf, 0700);
  }
#else
  strcatx (buf, getenv ("HOME"), "/.ctalk/libcache", NULL);
  strcpy (libcachedir, buf);
  if (file_exists (libcachedir) &&
      !is_dir (libcachedir))
    unlink (libcachedir);
  if (!is_dir (libcachedir)) {
    if (mkdir (buf, 0700))
      _error ("%s: %s.\n", libcachedir, strerror (errno));
  }
#endif
}

/*
 *  init_user_home_path (), below, must be called first.
 *  Not tested (yet) with djgpp.
 */
void init_user_template_path (void) {
  FILE *f;
  char template_fname[FILENAME_MAX];
  strcatx (usertemplatedir, getenv ("HOME"), "/.ctalk/templates", NULL);
  if (file_exists (usertemplatedir) &&
      !is_dir (usertemplatedir))
    unlink (usertemplatedir);
  if (!is_dir (usertemplatedir)) {
    if (mkdir (usertemplatedir, 0700))
      _error ("%s: %s.\n", usertemplatedir, strerror (errno));
  }

  strcatx (template_fname, usertemplatedir, "/fnnames", NULL);

  if (!file_exists (template_fname)) {
    if ((f = fopen (template_fname, "w")) == NULL) {
      _error ("init_user_template_path (fopen): %s: %s.\n",
	      template_fname, strerror (errno));
    }
    fprintf (f, "# This is a machine generated file.\n");
    fprintf (f, "# The format of each line is:\n");
    fprintf (f, "# <source_function_name>,<api_function_name>\n");
    fprintf (f, "# See the fnnames(5ctalk) and template(1) man pages for more\n");
    fprintf (f, "# information.\n");
    fclose (f);
  }
}

void init_user_home_path (void) {
  char buf[FILENAME_MAX];
#ifdef DJGPP
  strcpy (buf, "/djgpp/include/ctalk/libcache");
  strcpy (ctalkuserhomedir, buf);
  if (file_exists (ctalkuserhomedir) &&
      !is_dir (ctalkuserhomedir))
    unlink (ctalkuserhomedir);
  if (!file_exists (ctalkuserhomedir)) {
    mkdir (buf, 0700);
  }
#else
  strcatx (buf, getenv ("HOME"), "/.ctalk", NULL);
  strcpy (ctalkuserhomedir, buf);
  if (file_exists (ctalkuserhomedir) &&
      !is_dir (ctalkuserhomedir))
    unlink (ctalkuserhomedir);
  if (!is_dir (ctalkuserhomedir)) {
    if (mkdir (buf, 0700))
      _error ("%s: %s.\n", buf, strerror (errno));
  }
#endif
}

char *home_libcache_path (void) {
  return libcachedir;
}

char *ctalklib_path (void) {
  static char path[FILENAME_MAX];
  int i;

  strcatx (path, classlibdir, "/ctalklib", NULL);
  if (file_exists (path))
    return path;

  for (i = 0; library_include_paths[i]; i++) {
    strcatx (path, library_include_paths[i], "/ctalklib", NULL);
    if (file_exists (path))
      return path;
  }

  return NULL;
}

/* This is not the greatest, but it keeps the line numbers in
   the Object class correct when there are no line markers. */
void get_ctalkdefs_h_lines (void) {
  char path[FILENAME_MAX];
  char *inbuf, *n;
  FILE *f;
  int r;
  struct stat statbuf;
  strcatx (path, classlibdir, "/ctalkdefs.h", NULL);
  if ((f = fopen (path, "r")) == NULL) {
    return;
  }

  stat (path, &statbuf);

  if ((inbuf = (char *)__xalloc (statbuf.st_size + 1)) == NULL) {
    fclose (f);
    return;
  }

  if ((r = fread (inbuf, sizeof(char), statbuf.st_size, f)) !=
      statbuf.st_size) {
    if (errno != SUCCESS)
      printf ("get_ctalkdefs_h_lines: %s: %s\n", path, strerror (errno));
  }

  for (n = inbuf; *n; ++n) {
    if (*n == '\n') {
      ++ctalkdefs_h_lines;
    }
  }
}
