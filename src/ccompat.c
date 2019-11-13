/* $Id: ccompat.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015 Robert Kiesling, rk3314042@gmail.com.
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

/*
 *   Compatibility functions for different C compilers.
 *   Revision history:
 *
 *   2006-02-15
 *     Compatibility with GCC on Linux and DJGPP on MS Windows.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

extern int verbose_opt;                /* Declared in main.c.               */
extern char *include_dirs[MAXUSERDIRS];
extern int n_include_dirs;
extern char input_source_file[FILENAME_MAX];
extern char *pkgname;
extern char *classlibdir;               /* Declared in rtinfo.c.            */

static char gcc_target[MAXLABEL];       /* GNU C target and version.        */

#ifdef __GNUC__
#ifdef __DJGPP__
#define GCC_LIBDIR "/djgpp/lib/gcc"    /* GNU C library directory.          */
#else
#define GCC_LIBDIR "/usr/lib/gcc-lib"
#endif
#endif

#ifdef __GNUC__                        /* GCC include directories.          */
char *cc_include_paths[] = {
  "/usr/local/include"                 /* Host-specific includes.           */
  "",                                  /* LIBDIR includes.                  */
  "",                                  /* /usr/TARGET/includes.             */
  "",                                  /* Standard includes.                */
  NULL
};
#define N_PATHS 4
#else                             /* Generic include directories.      */
char *cc_include_paths[] = {
  "/usr/local/include"       
  "/usr/include",
  NULL
};
#define N_PATHS 2
#endif

/* The compat include file's path must be relative to the include search
   directories. */
#ifdef __GNUC__
#define COMPAT_INCLUDE "gncompat.h"
#else
#define COMPAT_INCLUDE NULL       /* Only tested with GCC so far.     */
#endif

bool compat_include = False;   /* True if including compatibility
				     header files. */

#define _MPUTCHAR(__m,__c) (__m)->name[0] = (__c); (__m)->name[1] = '\0';

/*
 * Try to find the compiler target by looking for the subdirectory
 * under /usr/lib/gcc-lib.
 */

char *find_gcc_target_dir (void) {

#ifndef __DJGPP__
  struct dirent *d;
  DIR *dir;
#endif
  struct stat statbuf;
  char targetdir[FILENAME_MAX];

#ifdef __DJGPP__

  strcpy (gcc_target, "djgpp");

#else

  /* Try to find out what the compiler target is.  It may not 
     be the same as the autoconf $host.
  */
  if ((dir = opendir (GCC_LIBDIR)) != NULL) {
    *gcc_target = 0;
    while ((d = readdir (dir)) != NULL) {
      if (index (d -> d_name, '-'))
	strcpy (gcc_target, d -> d_name);
    }
    closedir (dir);
  }  else {
    _warning ("Could not find compiler target subdirectory in");
    _warning ("/usr/lib/gcc-lib. See the -s command line option.");
    exit (EXIT_FAILURE);
  }

#endif
  SNPRINTF (targetdir, (size_t)FILENAME_MAX, "%s/%s", GCC_LIBDIR, gcc_target);
  if (!stat (targetdir, &statbuf))
    if (S_ISDIR (statbuf.st_mode))
      return gcc_target;

  _warning ("Could not find compiler target subdirectory");
  _warning ("%s. See the -s command line option.", targetdir);
  exit (EXIT_FAILURE);

}

/* 
 *  For now, just elide the attributes.
 */

void gnu_attributes (MESSAGE_STACK messages, int msg_ptr) {

  int i,
    stack_end,
    attr_start,
    n_parens;

  stack_end = get_stack_top (messages);

  for (i = msg_ptr, n_parens = 0, attr_start = 0; i > stack_end; i--) {

    switch (M_TOK(messages[i]))
      {
      case LABEL:
	if (str_eq (M_NAME(messages[i]), "__attribute__"))
	  attr_start = i;
	break;
      case SEMICOLON:
	goto gnu_attribute_done;
	break;
      case OPENPAREN:
	if (attr_start)
	  ++n_parens;
	break;
      case CLOSEPAREN:
	if (--n_parens == 0) {
	  _MPUTCHAR(messages[i], ' ');
	  messages[i] -> tokentype = WHITESPACE;
	  goto gnu_attribute_done;
	}
      }
    _MPUTCHAR(messages[i], ' ');
    messages[i] -> tokentype = WHITESPACE;
    
  }

 gnu_attribute_done:
  return;
}

/*
 *  Later versions of the GNU C headers can cause the preprocessor
 *  to define out a tag, for example, 
 *  "extern void <tag-replaced-by-whitespace> __atribute ((NOTHROW));"
 *  This function elides such declarations so they don't cause compiler 
 *  warnings.
 */

#ifdef __GNUC__
void gnuc_fix_empty_extern (MESSAGE_STACK messages, int idx) {

  int i,
    stack_end, 
    decl_end_idx = -1; 
  bool have_tag = False,
    is_fn = False;
  MESSAGE *m;
  CFUNC *cfn;

  stack_end = get_stack_top (messages);

  for (i = idx; 
       (i > stack_end) && (decl_end_idx == -1);
       i--) {

    m = messages[i];

    switch (M_TOK(m))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case LABEL:
	if (!is_c_keyword (M_NAME (m))) {
	  if (global_var_is_declared (M_NAME(m)) ||
	      ((cfn = get_function (M_NAME(m))) != NULL)) {
	    have_tag = True;
	  }
	}
	break;
      case OPENBLOCK:
	is_fn = True;
	decl_end_idx = i;
	break;
      case SEMICOLON:
	decl_end_idx = i;
	break;
      }
  }

  if (!have_tag && !is_fn) {
    for (i = idx; i > decl_end_idx; i--) {
      m = messages[i];
      _MPUTCHAR(m, ' ');
      m ->  tokentype = WHITESPACE;
    }
  }

}
#endif

extern char *library_include_paths[MAXUSERDIRS];   /* Declared in rtinfo.c. */

void print_libdirs (void) {
  int i;
  for (i = 0; library_include_paths[i]; i++) 
    printf ("%s\n", library_include_paths[i]);
}
