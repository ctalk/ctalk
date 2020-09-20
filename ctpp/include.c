/* $Id: include.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef __DJGPP__
#include <dirent.h>
#endif
#include "ctpp.h"

#define _error_out _warning
/*
 *  Note that ctpp does not use PKGNAME and CLASSLIBDIR by
 *  default.  When calling ctpp from ctalk, use the -I option 
 *  to specify CLASSLIBDIR.
 */

extern int include_dir_opt;       /* Declared in main.c.       */
extern int n_user_include_dirs;        /* Specified with -I option. */
extern char *user_include_dirs[MAXUSERDIRS];
extern int systeminc_opt;         /* Specified with -s option. */
extern int n_system_inc_dirs;
extern int lang_cplusplus_opt;    /* --lang-c++ option.        */
extern char *systeminc_dirs[MAXUSERDIRS]; 
extern int nostdinc_opt;

extern int include_inhibit_opt;   /* -I- option.                  */
extern int include_uninhibit_idx; /* First user include after -I- */

extern int n_configure_include_dirs; /* From ccompat.c.  Must be non-zero. */

extern char *pkgname;

extern char *searchdirs[];     /* Declared in ccompat.c     */
extern char cpp_subdir[FILENAME_MAX];

/*
 *  Directories in the secondary include path.
 */
extern char include_dirs_after[MAXARGS][FILENAME_MAX];
extern int include_dirs_after_idx;

typedef struct {
  char *path;
  bool inhibit;
} INCLUDE_PATH;

static INCLUDE_PATH include_paths[MAXUSERDIRS+1];

static char n_include_paths = 0;

static void add_path (char *);

/*
 *  1. User specified include directories.
 *     -Specified by the -I switch.
 *     -Search inhibited by a -I- switch if given on the command 
 *     line.
 *  2. User specified system include directories.
 *     -Specified by the -S switch if the program cannot locate
 *      them.
 *     -Disabled by the -nostdinc option.
 *  3. Compiler's standard include directories.
 *     -Disabled by the -nostdinc option.
 *  4. Include paths specified with -idirafter and
 *     -iwithprefix options.
 */

void init_include_paths (void) {

  int i, idx;

  memset (include_paths, 0, 
	  sizeof (INCLUDE_PATH) * (MAXUSERDIRS + 1));

  /* n_include_paths += n_user_include_dirs + n_system_inc_dirs +  */
  /*   ((nostdinc_opt) ? 0 : N_PATHS); */

  n_include_paths += n_user_include_dirs + n_system_inc_dirs + 
    ((nostdinc_opt) ? 0 : n_configure_include_dirs);

  for ( i = 0, idx = 0; idx < n_user_include_dirs; i++, idx++) {
    include_paths[i].path = strdup (user_include_dirs[idx]);
    include_paths[i].inhibit = 
      (((include_inhibit_opt) && 
	(idx < include_uninhibit_idx)) ? TRUE : FALSE);
  }

  for (idx = 0; idx < n_system_inc_dirs; i++, idx++)
    add_path (systeminc_dirs[idx]);

  if (!nostdinc_opt) {
    for ( idx = 0; idx < n_configure_include_dirs; i++, idx++) {
      if (searchdirs[idx])
	add_path (searchdirs[idx]);
    }
  }

  i += env_paths ();

  for (idx = 0; idx <= include_dirs_after_idx; i++, idx++) {
    include_paths[i].path = strdup (include_dirs_after[idx]);
    include_paths[i].inhibit = FALSE;
  }

}

/*
 *  Search for include files by name in:
 *   1.  Include directories given by -I command line option, and 
 *       also for the <pkgname> subdirectory of each.
 *   2.  CLASSLIBDIR, determined at compile time.
 *   3.  Compiler's include directories, and also for the <pkgname>
 *       subdirectory of each.
 *
 *   Note that DJGPP's stat () function matches directory entries case 
 *   insensitively.  That means we have to check each entry in the library 
 *   directory entry with strcmp ().
 *
 *   TO DO - It also means that ctalk won't work on MS-DOS
 *   or Windows machines that use the FAT file system.  See the
 *   DJGPP info page for _preserve_fncase ().  There might need to be
 *   an installation option that gives a special extension to class 
 *   libs on machines with FAT file systems.
 *
 *   Note also that this is different than find_include (), which
 *   does not need to worry about the case of file names, but
 *   does need to handle include file names with partial
 *   directory paths; e.g., #include <sys/stat.h>.  These two 
 *   functions should be folded together in a future release.
 */

#ifdef __DJGPP__
extern int __opendir_flags;
#endif

/*
 *  find_include () finds files case insensitively, but they
 *  can have a partial path name; e.g., #include <sys/stat.h>.
 */

static int last_include_path_idx;

char *find_include (char *fname, int find_next) {

  static char path[FILENAME_MAX * 3];
  int i;
  struct stat statbuf;
  int r_stat = -1;

  if (!fname || !strlen (fname))
    return NULL;

  if (find_next) {
    i = last_include_path_idx + 1;
    if (i > n_include_paths)
      return NULL;
  } else {
    i = 0;
  }
    
  for (; include_paths[i].path; i++) {
    last_include_path_idx = i;
    sprintf (path, "%s/%s", include_paths[i].path, fname);
    if ((r_stat = stat (path, &statbuf)) == 0) 
      break;

    sprintf (path, "%s/%s/%s", include_paths[i].path, pkgname, fname);
    if ((r_stat = stat (path, &statbuf)) == 0) 
      break;

    if (lang_cplusplus_opt) {
      sprintf (path, "%s/%s/%s", include_paths[i].path, cpp_subdir, fname);
      if ((r_stat = stat (path, &statbuf)) == 0) 
	break;
    }

  }

  if (!r_stat)
    return path;
  else
    return NULL;
}

static int last_has_include_path_idx;

bool has_include (char *fname, bool find_next) {

  static char path[FILENAME_MAX * 3];
  int i;
  struct stat statbuf;
  int r_stat = -1;

  if (!fname || !strlen (fname))
    return NULL;

  if (find_next) {
    i = last_has_include_path_idx + 1;
    if (i > n_include_paths)
      return NULL;
  } else {
    i = 0;
  }
    
  for (; include_paths[i].path; i++) {
    last_has_include_path_idx = i;
    sprintf (path, "%s/%s", include_paths[i].path, fname);
    if ((r_stat = stat (path, &statbuf)) == 0) 
      break;

    sprintf (path, "%s/%s/%s", include_paths[i].path, pkgname, fname);
    if ((r_stat = stat (path, &statbuf)) == 0) 
      break;

    if (lang_cplusplus_opt) {
      sprintf (path, "%s/%s/%s", include_paths[i].path, cpp_subdir, fname);
      if ((r_stat = stat (path, &statbuf)) == 0) 
	break;
    }

  }

  if (!r_stat)
    return true;
    /* return path; *//***/
  else
    return false;
    /* return NULL; *//***/
}

void dump_include_paths (void) {

  int i;
  char buf[MAXMSG];

  for (i = 0; i < n_include_paths; i++) {
    sprintf (buf, "%s\n", include_paths[i].path);
    _error_out (buf);
  }

}

/*
 *  The CPATH environment variable should be used for all language
 *  options.  The C_INCLUE_PATH variable should only be used for
 *  standard C, but in ctpp it is on by default.  The 
 *  CPLUS_INCLUDE_PATH variable is only checked if lang_cplusplus_opt
 *  is True.
 */

int env_paths (void) {

  char *v;
  int n_paths = 0;

  if ((v = getenv ("CPATH")) != NULL)
    n_paths += split_path_var (v);
  
  if (!lang_cplusplus_opt &&((v = getenv ("C_INCLUDE_PATH")) != NULL))
    n_paths += split_path_var (v);
  
  if (lang_cplusplus_opt && ((v = getenv ("CPLUS_INCLUDE_PATH")) != NULL))
    n_paths += split_path_var (v);

  return n_paths;
}

static void add_path (char *s) {
  int i;

  for (i = 0; ; i++){
    if (!include_paths[i].path) {
      include_paths[i].path = strdup (s);
      include_paths[i].inhibit = FALSE;
      return;
    }
  }
}

int split_path_var (char *pathvar) {
  char dname[FILENAME_MAX],
    *p, *q;
  int n_paths = 0;

  p = pathvar;
  while (p) {
    memset ((void *)dname, 0, FILENAME_MAX * sizeof (char));
    if ((q = index (p, ':')) != NULL)
      strncpy (dname, p, q - p);
    else
      strcpy (dname, p);

    add_path (dname);
    ++n_paths;

    p = (q) ? q + 1 : NULL;
  }

  return n_paths;
}














