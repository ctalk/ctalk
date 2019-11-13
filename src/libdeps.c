/* $Id: libdeps.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2014, 2016-2017 Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#include <unistd.h> 
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "classlib.h"

extern char *classlibdir;      /* Declared in main.c */
extern char libcachedir[FILENAME_MAX];
char ctalkuserhomedir[FILENAME_MAX];
extern int args_changed; 

/*
 *  The format of ctpp_ofn_path is generally something like
 *  /tmp/Object.ctpp.<pid>.<id>
 */
int cache_ctpp_output_file (char *ctpp_ofn_path) {
  char lib_cache_path[FILENAME_MAX];
  char buf[FILENAME_MAX];
  char *ext, *ctpp_ofn_lastslash;

  ext = strstr (ctpp_ofn_path, ".ctpp");
  ext += 5;  /* point to character after the extension */
  ctpp_ofn_lastslash = rindex (ctpp_ofn_path, '/');
  ++ctpp_ofn_lastslash;
  substrcpy (buf, ctpp_ofn_lastslash, 0, ext - ctpp_ofn_lastslash);
  
  strcatx (lib_cache_path, libcachedir, "/", buf, NULL);

  rename (ctpp_ofn_path, lib_cache_path);

  return SUCCESS;
}

int ctpp_deps_updated (char *class_path) {
  char fn[FILENAME_MAX];
  char lib_cache_path[FILENAME_MAX];
  char *lastslash;
  int lib_mtime, cache_mtime;

  if (args_changed)
    return TRUE;

  if ((lastslash = rindex (class_path, '/')) != NULL) {
    strcpy (fn, lastslash + 1);
  } else {
    strcpy (fn, class_path);
  }

  strcatx (lib_cache_path, libcachedir, "/", fn, ".ctpp", NULL);

  if (!file_exists (lib_cache_path))
    return TRUE;

  lib_mtime = file_mtime (class_path);
  cache_mtime = file_mtime (lib_cache_path);
  if ((lib_mtime != ERROR) && 
      (cache_mtime != ERROR) && 
      (lib_mtime > cache_mtime))
    return TRUE;
      
  return FALSE;
}

static char *cmd_opts[512];
static int n_saved_opts;

/*
 *  Record the ctalk executable and all opts that begin 
 *  with '-', and "-o" and its argument.
 *
 *  The only options that take an argument that 
 *  we need for dependency checking are "-I" and "-s".
 *
 *  The program exits before reaching here when
 *  the options are, "-V," "-h," and, "--printlibdirs."
 */

void save_opts (char **a, int n) {
  int i;
  n_saved_opts = 0;

  for (i = 1; i < n; i++) {

    if (a[i][0] == 0)
      continue;

    if (a[i][0] == '-') {

      switch (a[i][1])
	{
	case 'P':
	  cmd_opts[n_saved_opts++] = strdup (a[i]);
	  break;
	case 'I':
	  if (a[i][2]) { /* i.e., there's no space between the option
			    and its operand, so they're the same arg. */
	    cmd_opts[n_saved_opts++] = strdup (a[i]);
	  } else {
	    cmd_opts[n_saved_opts++] = strdup (a[i++]);
	    cmd_opts[n_saved_opts++] = strdup (a[i]);
	  }
	  break;
	case 's':
	  cmd_opts[n_saved_opts++] = strdup (a[i]);
	  break;
	case 'o':  /* We don't want to re-process the libs for a 
		      different output file. */
	  if (!a[i][2]) /* i.e., the option and operand are separate args. */
	    ++i;
	  break;
	case '-': /* Any extended arg. */
	  cmd_opts[n_saved_opts++] = strdup (a[i]);
	  break;
	  
	}

      /* if (a[i][1] == 'o') */
      /* 	continue; */

      /* if (strpbrk (a[i], opts_w_args)) { */

      /* 	cmd_opts[n_saved_opts++] = strdup (a[i++]); */
      /* 	cmd_opts[n_saved_opts++] = strdup (a[i++]); */

      /* } */


    }
  }

}

#define ARGSFILE "args"

void check_user_args (void) {
  char path[FILENAME_MAX];
  char new_args[MAXMSG];
  char old_args[MAXMSG];
  char *p;
  int i;
  FILE *f;

  memset (new_args, 0, MAXMSG);

  for (i = 0; i < n_saved_opts; i++) {

    if (!cmd_opts[i] || *cmd_opts[i] == '\0') continue;

    strcatx2 (new_args, cmd_opts[i], " ", NULL);

  }
  strcatx (path, ctalkuserhomedir, "/", ARGSFILE, NULL);
  /*
   *  Don't issue error if args file does not exist.
   */
  if ((f = fopen (path, "r")) != NULL) {
    if (!fgets (old_args, MAXMSG, f)) {
      if (errno)
	_error ("%s: %s.\n", ctalkuserhomedir, strerror(errno));
    }
    if (old_args[0]) {
      /*
       *  When fgets grabs the newline (and there is a trailing
       *  space in the new args).
       */
      for (p = &old_args[strlen(old_args) - 1]; 
	   (*p == ' ') || (*p == '\n'); 
	   p--)
	*p = '\0';
      for (p = &new_args[strlen(new_args) - 1]; 
	   (*p == ' ') || (*p == '\n'); 
	   p--)
	*p = '\0';
      if (strcmp (new_args, old_args))
	args_changed = TRUE;
      else
	args_changed = FALSE;
    } else {
      args_changed = TRUE;
    }
    fclose (f);
  } else {
    args_changed = TRUE;
  }
}

void record_opts (void) {
  char path[FILENAME_MAX];
  int i;
  FILE *f;
  if (args_changed) {
    strcatx (path, ctalkuserhomedir, "/", ARGSFILE, NULL);
    if ((f = fopen (path, "w")) == NULL)
      _error ("%s: %s.\n", path, strerror(errno));
    for (i = 0; i < n_saved_opts; i++) {
      if (cmd_opts[i] && *cmd_opts[i])
	fprintf (f, "%s ", cmd_opts[i]);
    }
    fprintf (f, "\n");
    fclose (f);
  }
}

int class_deps_updated (char *classname) {

  char *lib_path;
  char lib_cache_path[FILENAME_MAX];
  int lib_mtime, cache_mtime;

  if ((lib_path = find_library_include (classname, FALSE)) == NULL)
    return FALSE;

  if (args_changed)
    return TRUE;

  strcpy (lib_cache_path, class_cache_path_name (classname)); 

  if (!file_exists (lib_cache_path))
    return TRUE;

  lib_mtime = file_mtime (lib_path);
  cache_mtime = file_mtime (lib_cache_path);

  if ((lib_mtime != ERROR) && 
      (cache_mtime != ERROR) && 
      (lib_mtime > cache_mtime))
    return TRUE;
      
  return FALSE;
}

int method_deps_updated (char *selector) {

  char *lib_path;
  char lib_cache_path[FILENAME_MAX];
  char classname[MAXLABEL], *s;
  int lib_mtime, cache_mtime;

  if (args_changed)
    return TRUE;

  if ((s = strchr (selector, '_')) == NULL) 
    return FALSE;

  memset (classname, 0, MAXLABEL);
  strncpy (classname, selector, s - selector);

  if ((lib_path = find_library_include (classname, FALSE)) == NULL)
    return FALSE;

  strcatx (lib_cache_path, home_libcache_path (), "/", selector, NULL);

  if (!file_exists (lib_cache_path))
    return TRUE;

  lib_mtime = file_mtime (lib_path);
  cache_mtime = file_mtime (lib_cache_path);

  if ((lib_mtime != ERROR) && 
      (cache_mtime != ERROR) && 
      (lib_mtime > cache_mtime))
    return TRUE;
      
  return FALSE;
}
