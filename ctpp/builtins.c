/* $Id: builtins.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

/*
 *  Host-specific builtin macros.  When implementing builtins for 
 *  a new system, you at least need to create a file with the 
 *  "builtins" array in the config/ subdirectory.  See 
 *  config/linux-gnu-x86.h for an example.
 *
 *  You will also need to write a function to add the builtins
 *  to the preprocessor's name space, and you also need to call
 *  the function from ccompat_init () in ccompat.c.  See 
 *  gnu_builtins (), below, and ccompat_init () for examples.
 *
 *  When adding #defines, format the #define as an input line, and
 *  use tokenize_define () to process the input line and add the
 *  definition to the preprocessor's name space.
 */

#ifndef NO_BUILTINS
#ifdef USE_OLD_BUILTINS
/*
 *  Linux x86, Cygwin, and DJGPP. 
 */
#ifdef __CYGWIN32__
#include "config/cygwin32-pc-x86.h"
char *host_os="cygwin";
char *host_cpu="x86";
#else
#if defined(__linux__) && defined(__i386__)
# include "config/linux-gnu-x86.h"
char *host_os="linux";
char *host_cpu="x86";
# else
#  ifdef __DJGPP__
#  include "config/djgpp-pc-x86.h"
char *host_os = "djgpp";
char *host_cpu = "x86";
#  endif /* __DJGPP__ */
# endif  /* (HOST_OS == linux-gnu) */
#endif

/*
 *  Sun Sparc with Solaris 2.8-2.10 (32-bit) and GCC.
 */
#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
#include "config/sparc-sun-solaris2.8.h"
#else
# if defined(__GNUC__) && defined(__sparc__) && defined(__linux__)
# include "config/sparc-linux-gnu.h"
# endif /* defined(__GNUC__) && defined(__sparc__) && defined(__linux__) */
#endif /* defined(__GNUC__) && defined(__sparc__) && defined(__svr4__) */

#if defined (__APPLE__) && defined (__POWERPC__)
# include "config/powerpc-apple-darwin.h"
#endif

#ifdef GCC_STDINT_WRAPPER
# include "config/gcc45_types.h"
#endif


/*
 *  See the comments in the include file(s) for systems 64-bit support
 *  has been tested on.
 */
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
#include "config/x86_64-amd64-linux-gnu.h"
#else
# if defined (__GNUC__) && defined (__ia64__) && defined (__itanium__)
# include "config/ia64-unknown-linux-gnu.h"
# endif /* defined (__GNUC__) && defined (__ia64__) && defined (__itanium__) */
#endif /* defined (x86_64) && defined (__amd64__) */

/* OpenSuse on IBM s390. */
#if defined (__GNUC__) && defined (__s390__)
# include "config/s390x-suse-linux.h"
#endif

#else /* USE_OLD_BUILTINS */
/* Using the new system is much easier. */
#include <builtins.h>
#endif /* USE_OLD_BUILTINS */
#else /* NO_BUILTINS */
static char *builtins[] = { (void *)0, (void *)0 };
#endif /* NO_BUILTINS */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include "ctpp.h"

extern int traditional_opt;

extern struct _optdefs {
  BUILTIN_OPT opt;
  int val;
  char str[MAXLABEL];
} optdefs[];

/*
 *  Built-in #defines from GNU configure, plus #defines specified
 *  by C99.
 */

extern int gcc_macros_opt;     /* Defined in rtinfo.c, enabled by default. */

/* In case our version of GCC defines them earlier. */
int have__GNUC__ = 0, 
  have__GNUC_MINOR__ = 0, 
  have__GNUC_PATCHLEVEL__ = 0;

int gnu_builtins (void) {

  int i;
  char s[MAXLABEL];

  /*
   *  Host-dependent symbol/definition pairs from the 
   *  config file included above.  See the comments in 
   *  config/linux-gnu-x86.c
   */
  for (i = 0; builtins[i]; i+=2) {
    if (!strcmp (builtins[i], "__GNUC__"))
      have__GNUC__ = 1;
    if (!strcmp (builtins[i], "__GNUC_MINOR__"))
      have__GNUC_MINOR__ = 1;
    if (!strcmp (builtins[i], "__GNUC_PATCHLEVEL__"))
      have__GNUC_PATCHLEVEL__ = 1;
    sprintf (s, "#define %s %s\n", builtins[i], builtins[i+1]);
    tokenize_define (s);
  }

#ifdef GCC_STDINT_WRAPPER
  for (i = 0; gcc45_types[i]; i+=2) {
    sprintf (s, "#define %s %s\n", gcc45_types[i], gcc45_types[i+1]);
    tokenize_define (s);
  }
#endif

  if (gcc_macros_opt) gcc_builtins ();

  return 0;
}

int is_builtin_symbol (char *name) {
  int i;
  for (i = 0; builtins[i]; i+=2) {
    if (!strcmp (name, builtins[i]))
      return TRUE;
  }
#ifdef GCC_STDINT_WRAPPER
  for (i = 0; gcc45_types[i]; i+=2) {
    if (!strcmp (name, gcc45_types[i]))
      return TRUE;
  }
#endif
  return FALSE;
}

void opt_builtins (void) {
  int i;
  char s[MAXLABEL + 11];  /* strlen ("#define ")  + strlen (" 1\n")*/

  if (traditional_opt)
    return;

  for (i = 0; *optdefs[i].str; i++) {
    if (optdefs[i].val) {
      sprintf (s, "#define %s 1\n", optdefs[i].str);
      tokenize_define (s);
    }
  }

}
