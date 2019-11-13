/* $Id: ccompat.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright � 2005-2011 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "ctpp.h"

extern int include_dir_opt;
extern int lang_cplusplus_opt;
extern char *user_include_dirs[MAXUSERDIRS];
extern int n_user_include_dirs;
extern char source_file[FILENAME_MAX];
extern char *pkgname;
extern char *classlibdir;

char cc_path[FILENAME_MAX];

char gcc_target[MAXLABEL];             /* GNU C target and version.         */
char gcc_version[MAXLABEL];

char cpp_subdir[FILENAME_MAX];

extern char *host_os;                 /* Defined in builtins.c.             */
extern char *host_cpu;

char gcc_prefix[FILENAME_MAX];

#if defined(__DJGPP__) || defined(__CYGWIN__)
#define GCC_BIN "gcc.exe"
#else
#define GCC_BIN "gcc"
#endif

/* The include paths first... */

/*
  supplied by ./configure --with-include-path
 */
#ifdef CONF_INCLUDE_PATH

/*static char *conf_include_path = CONF_INCLUDE_PATH;*//***/

char *searchdirs[MAXUSERDIRS * sizeof (char *)];

  int n_configure_include_dirs = 0;


# else

/* static char *conf_include_path = NULL; */

/* 
   Generated by ./configure from cpp or ./configure's options. 
*/
#include "searchdirs.h"

int n_configure_include_dirs = N_PATHS;

#endif /* CONF_INCLUDE_PATH */



#ifdef __GNUC__
# ifdef __DJGPP__
#  define GCC_LIBDIR "/djgpp/lib/gcc" /* GNU C library directory.          */
char *gcc_libdir = GCC_LIBDIR;
# else 
#  ifdef __APPLE__
#  define GCC_LIBDIR_PREFIX_DARWIN "/usr/lib/gcc/darwin"
#  define GCC_INCLUDE_PATH_DARWIN "/usr/include/gcc/darwin/default"
char *gcc_lib_path = "/usr/lib/libgcc.a";
char gcc_libdir[FILENAME_MAX];
#  else
char gcc_libdir[FILENAME_MAX];
#   define GCC_LIBSUBDIR1 "gcc-lib"
#   define GCC_LIBSUBDIR2 "gcc"
#  endif  /* __APPLE__ */
# endif /* __DJGPP__ */
# define GPP_DIR_PREFIX "g++"
#endif

#define GCC_LIBDIR_ENV "GCC_LIBDIR"


extern int gcc_macros_opt;    /* From rtinfo.c.  Enabled by default. */

int sol10_gcc_sfw_mod = FALSE;

void gcc_lib_subdir_warning (void) {
#ifdef __DJGPP__
  _warning ("Could not find the GCC library directories in\n");
  _warning ("%s\n", GCC_LIBDIR);
  _warning ("See the documentation for the -isystem command line option and the GCC_LIBDIR\n");
  _warning ("environment variable in ctpp(1).\n");
#else
# ifdef __APPLE__
  _warning ("Could not find the GCC library directories in\n");
  _warning ("%s or %s.\n", GCC_LIBDIR_PREFIX_DARWIN, GCC_INCLUDE_PATH_DARWIN);
  _warning ("See the documentation for the -isystem command line option and the GCC_LIBDIR\n");
  _warning ("environment variable in ctpp(1).\n");
# else 
  _warning ("Could not find the GCC library directories in\n");
  _warning ("%s/lib/%s\n", gcc_prefix, GCC_LIBSUBDIR1);
  _warning ("%s/lib/%s\n", gcc_prefix, GCC_LIBSUBDIR2);
  _warning ("See the documentation for the -isystem command line option and the GCC_LIBDIR\n");
  _warning ("environment variable in ctpp(1).\n");
# endif
#endif
  exit (ERROR);
}

/***/
#ifndef __have_preloaded_searchdirs
void include_dirs_from_conf_path ();
#endif

void ccompat_init (void) {

# ifdef __DJGPP__
  char incdir[FILENAME_MAX];

  strcpy (incdir, "/djgpp/include");
  searchdirs[1] = strdup (incdir);
  return;

# else

  char *gcc_path;

  if ((gcc_path = which (GCC_BIN)) != NULL) 
    strcpy (cc_path, gcc_path);
  substrcpy (gcc_prefix, cc_path, 0, strstr (cc_path, "/bin/gcc")-cc_path);

# endif /* ifdef __DJGPP__ */

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
  is_solaris10 ();   /* Set sunos_is_sol10 = TRUE, below. */
  sol10_compat_defines ();
#endif

#if defined(__GNUC__) && defined(USE_GCC_INCLUDES)

  strcpy (cc_path, which ("cc"));
  searchdirs[1] = strdup ("/usr/include");

#else

# if defined(CONF_INCLUDE_PATH)

#ifndef __have_preloaded_searchdirs

  include_dirs_from_conf_path ();

#endif  

# endif /* # if defined(CONF_INCLUDE_PATH) */


#endif  /* if defined(__GNUC__) && defined(USE_GCC_INCLUDES) */


  if (lang_cplusplus_opt) {
    find_gpp_subdir ();
  }
  gnu_builtins ();
  error_reset ();
}

extern MESSAGE *p_messages[P_MESSAGES+1];  /* Declared in preprocess.c. */

/*
 *    GCC Initialization.
 */

/*
 *  If there is a lang_c++ option, find the g++ subdir(s).
 *  Look for:
 *  1. <prefix>/include/<*++*>/
 *  2. <prefix>/include/<*++*>/<gcc-version>|<header-file>
 *  3. <prefix>/include/<*++*>/<gcc-version>/<header-file>
 *
 *  Note that gcc_version needs to be initialized before
 *  calling this function.
 */

void find_gpp_subdir (void) {

#ifdef __GNUC__
  char include_dir[FILENAME_MAX + 8], /* strlen ("include")  + 1 */
    gpp_subdir[FILENAME_MAX * 2],
    s[FILENAME_MAX * 3 + 10], /* strlen ("/iostream") + 1 */
    *subdir_sig_ptr;
  struct dirent *d;
  DIR *dir;

  sprintf (include_dir, "%s/%s", gcc_prefix, "include");

  if ((dir = opendir (include_dir)) != NULL) {
    while ((d = readdir (dir)) != NULL) {

      if ((subdir_sig_ptr = strstr (d -> d_name, "++")) != NULL) {

	sprintf (gpp_subdir, "%s/%s", include_dir, d -> d_name);

	if (gpp_version_subdir (gpp_subdir)) {
	  sprintf (s, "%s/%s/iostream", gpp_subdir, gcc_version);
	  if (file_exists (s)) {
	    sprintf (cpp_subdir, "%s/%s", d -> d_name, gcc_version);
	    return;
	  }
	} else {
	  sprintf (s, "%s/iostream", gpp_subdir);
	  if (file_exists (s)) {
	    strcpy (cpp_subdir, d -> d_name);
	    return;
	  }
	}
      }
    }
  }
#endif /* __GNUC__ */
}

int gpp_version_subdir (char *gpp_subdir) {

#ifdef __GNUC__
  char s[FILENAME_MAX];

  sprintf (s, "%s/%s", gpp_subdir, gcc_version);

  if (file_exists (s) && is_dir (s))
    return TRUE;
  else 
    return FALSE;
#else
  return FALSE;
#endif

}

/*
 *  I've made a few changes... 
 *  - Refactored into a new function.
 *  - Used tempnam () and a separate open () call - 
 *     mkstemp () as used in the patch is not compatible 
 *     with my system.  Tempnam generates a warning,
 *     though.
 *  - Used sprintf instead of strcpy in several places.
 *  - Note the presence of an include/ subdirectory in
 *    the same directory as libgcc.a.
 *  - Added the paths to searchdirs in this function.
 *    If possible, the routine to parse the path should
 *    be the same for both the compiler output and the
 *    autoconf or GCC_LIBDIR value. Then searchdirs 
 *    can be set from  the calling function, where it belongs.
 *
 *  The functions need testing with compiler installations other than 
 *  mine - rak.
 *
 *  Should be removed -- replaced by conf_lib_path ()
 */

#define N_PATH_ELEMENTS 3    /* target and version + include subdir */
#define INCLUDE_DIR     0
#define VERSION_DIR     1
#define TARGET_DIR      2

/* Easier than strtok (), (probably) safer than strsep () */
#ifndef __have_preloaded_searchdirs
void include_dirs_from_conf_path (void) {

  char *p, *q;
  char buf[FILENAME_MAX];
  int i = 0;
#ifdef CONF_INCLUDE_PATH
  static char *conf_include_path = CONF_INCLUDE_PATH;
#else
  static char *conf_include_path = NULL;
#endif  

  memset ((void *)searchdirs, 0, MAXUSERDIRS * sizeof (char *));

  p = q = conf_include_path;

  if ((q = strchr (p, ':')) == NULL) {
    searchdirs[0] = strdup (conf_include_path);
    n_configure_include_dirs = 1;
    return;
  } else {

    do {
      strncpy (buf, p, q - p);
      searchdirs[i++] = strdup (buf);
      n_configure_include_dirs = i;
      p = q + 1;
    } while ((q = strchr (p, ':')) != NULL); 

    strcpy (buf, p);
    searchdirs[i++] = strdup (buf);
    n_configure_include_dirs = i;
  }
}
#endif  


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

    if (!strcmp (messages[i] -> name, "__attribute__"))
      attr_start = i;

    if (attr_start) {

      if (messages[i] -> tokentype == OPENPAREN)
	++n_parens;

      if (messages[i] -> tokentype == CLOSEPAREN) {
	if (--n_parens == 0) {
	  strcpy (messages[i] -> name, " ");
	  messages[i] -> tokentype = WHITESPACE;
	  goto done;
	}
      }

      strcpy (messages[i] -> name, " ");
      messages[i] -> tokentype = WHITESPACE;
    }

  }

 done:
  return;
}

/* From builtins.c.  In case this version defines them earlier. */
extern int have__GNUC__, have__GNUC_MINOR__, have__GNUC_PATCHLEVEL__;

void gcc_builtins (void) {

/* #ifndef NO_BUILTINS */
#if !defined(NO_BUILTINS) && defined(CTPP_GNUC_VERSION)
#ifdef __GNUC__

  char s[MAXLABEL];

  if (!have__GNUC__) {
    sprintf (s, "#define __GNUC__ %d\n", __GNUC__);
    tokenize_define (s);
  }

#ifdef __GNUC_MINOR__
  if (!have__GNUC_MINOR__) {
    sprintf (s, "#define __GNUC_MINOR__ %d\n", __GNUC_MINOR__);
    tokenize_define (s);
  }
#endif

#ifdef __GNUC_PATCHLEVEL__
  if (!have__GNUC_PATCHLEVEL__) {
    sprintf (s, "#define __GNUC_PATCHLEVEL__ %d\n", __GNUC_PATCHLEVEL__);
    tokenize_define (s);
  }
#endif

#endif /* __GNUC__ */
#endif /* NO_BUILTINS */

}

int is_gnuc_symbol (char *s) {
#ifdef __GNUC__
  if (!strcmp (s, "__GNUC__") ||
      !strcmp (s, "__GNUC_MINOR__") ||
      !strcmp (s, "__GNUC_PATCHLEVEL__"))
    return TRUE;
#endif
  return FALSE;
}

#if defined(__sparc__) && defined(__svr4__)
static int sunos_is_sol10 = FALSE;
#endif

/*
 *  Also sets the variable above.
 */
int is_solaris10 (void) {
#if defined(__sparc__) && defined(__svr4__)
  char cmd[MAXMSG], cmdoutput[MAXMSG+1];
  FILE *P;

  if (!which ("uname"))
    return FALSE;
  sprintf (cmd, "%s -r", which ("uname"));
  
  if ((P = popen (cmd, "r")) != NULL) {
    while (fread (cmdoutput, sizeof (char), MAXMSG, P))
      ;
  } else {
    printf ("is_solaris10: %s.\n", strerror (errno));
    return ERROR;
  }
  pclose (P);

  if (strstr (cmdoutput, "5.10")) {
    sunos_is_sol10 = TRUE;
    return TRUE;
  } else {
    return FALSE;
  }
#else
  return FALSE;
#endif
}

/*
 *  This is the route used by GCC's fixincludes to
 *  fix the redefinition of gnuc_va_list and va_list -
 *  instead of including Solaris' sys/va_list.h,
 *  include stdarg.h from GCC's library includes.
 *  We also need to re-#define __va_list as va_list.
 */
int sol10_use_gnuc_stdarg_h (INCLUDE *inc) {
#if defined (__GNUC__) && defined(__sparc__) && defined(__svr4__)
  /*
   *  GCC on Solaris 10 with GNU headers.
   */
  if (sunos_is_sol10 && !sol10_gcc_sfw_mod) {
    if (!strcmp (inc -> name, "sys/va_list.h")) {
	strcpy (inc -> name, "stdarg.h");
    }
  }
#endif
  return SUCCESS;
}

void sol10_compat_defines (void) {
#if defined (__GNUC__) && defined(__sparc__) && defined(__svr4__)
  char s[MAXMSG];
  if (sunos_is_sol10) {
  /*
   *  Solaris 10 packaged GCC in /usr/sfw/bin, Sun headers.
   */
    if (!strcmp (cc_path, "/usr/sfw/bin/gcc") &&
	!strcmp (gcc_version, "3.4.3")) {
      sol10_gcc_sfw_mod = TRUE;
    } else {
      /*
       *  Solaris 10 GCC aftermarket /usr/local/bin, GNU headers.
       */
      strcpy (s, "#define __va_list va_list\n");
      tokenize_define (s);
    }
  }
#endif
}

int sol10_attribute_macro_sub (DEFINITION *d) {
    /*
     *  I'm not sure if this is a typo or a misfeature: 
     *  Solaris 10 /usr/include/sys/ccompile.h contains the line:
     *  #define	__sun_attr__(__a)	___sun_attr_inner __a
     *
     *  Replace_macro, below should deal with the issue of 
     *  replacing a macro with a parameterized macro, because
     *  there's actually nothing to substitute.  Once Solaris
     *  et al, figure that out, then ctpp will make ad-hoc 
     *  substitutions. 
     *
     *  Ccompile.h ony defines the attributes if __STDC__
     *  is defined.
     *
     *  Here some of the more common GNU C equivalents.
     *  Note that, "packed," doesn't have a shorthand form.
     */
#if defined (__GNUC__) && defined(__sparc__) && defined(__svr4__)

  if (!sunos_is_sol10)
    return TRUE;

  if (!strcmp (d -> name, "__NORETURN")) {
    strcpy (d -> value, "__attribute__((__noreturn__))");
    return FALSE;
  }
  if (!strcmp (d -> name, "__CONST")) {
    strcpy (d -> value, "__attribute__((__const__))");
    return FALSE;
  }
  if (!strcmp (d -> name, "__PURE")) {
    strcpy (d -> value, "__attribute__((__pure__))");
    return FALSE;
  }
  if (!strcmp (d -> name, "__sun_attr___packed__")) {
    strcpy (d -> value, "__attribute__((__packed__))");
    return FALSE;
  }
#endif
    return TRUE;
}

extern HASHTAB macrodefs;             /* Declared in hash.c.           */

/*
 *  These checks could be tighter, but we want to correctly catch as
 *  many possible redefinitions between the wrapped stdint
 *  definitions and the legacy header definitions as possible.
 *
 *  Linux - Called from preprocess.c if we run into a definiton of
 *  __intN_t or __u_intN_t from the Linux headers, which the
 *  preprocessor should encounter first..  So we use the Linux
 *  definitions and undefine the conflicting wrapped stdint
 *  definitions.
 */

void fixup_wrapped_gcc_stdints_linux (void) {
#if defined (GCC_STDINT_WRAPPER)
#ifdef __linux__
  DEFINITION *t;
  if ((t = 
       (DEFINITION *)_hash_remove (macrodefs, 
				   "__INT8_TYPE__")) != NULL)  {
    free (t);
  }
  if ((t = 
       (DEFINITION *)_hash_remove (macrodefs, 
				   "__INT16_TYPE__")) != NULL)  {
    free (t);
  }
  if ((t = 
       (DEFINITION *)_hash_remove (macrodefs, 
				   "__INT32_TYPE__")) != NULL)  {
    free (t);
  }
  if ((t = 
       (DEFINITION *)_hash_remove (macrodefs, 
				   "__INT64_TYPE__")) != NULL)  {
    free (t);
  }
  if ((t = 
       (DEFINITION *)_hash_remove (macrodefs, 
				   "__INTPTR_TYPE__")) != NULL)  {
    free (t);
  }
#endif /* __linux__ */
#endif /* GCC_STDINT_WRAPPER */
}

/*
 *  For OS X wrapped stdint defs, check if the symbol is already 
 *  defined, and don't define a duplicate in define_symbol () if it
 *  is.
 */
int wrapped_gcc_stdints_dup_osx (const char *name) {
#ifdef GCC_STDINT_WRAPPER
# ifdef __APPLE__
  DEFINITION *d;
  char *c;
  if ((c = strstr (name, "_TYPE__")) != NULL)
    if ((d = get_symbol (name, TRUE)) != NULL)
      return TRUE;
# endif /* __APPLE__ */
#endif /* GCC_STDINT_WRAPPER */
  return FALSE;
}

#if defined CONF_INCLUDE_PATH
void cleanup_conf_include_searchdirs (void) {
  int i;

  if (!conf_include_path)
    return;

  for (i = 0; searchdirs[i]; ++i) 
    free (searchdirs[i]);
}
#endif