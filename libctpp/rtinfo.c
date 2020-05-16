/* $Id: rtinfo.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "ctpp.h"
#include "prtinfo.h"

RT_INFO rtinfo;

static char argv_name[FILENAME_MAX];

char source_file[FILENAME_MAX];
char appname[FILENAME_MAX];

INCLUDE *includes[MAXARGS + 1];
int include_ptr = MAXARGS + 1;

int nolinemarker_opt = FALSE;                /* Don't include line info in output. */
int nolinemarkerflags_opt = FALSE;          /* Don't include flags in the linemarkers. */
                                            /* The compiler sometimes gets confused if */
                                            /*  the class libraries line markers don't */
                                            /*  don't take into account ctalk's declarations. */
int nostdinc_opt = FALSE;                    /* Don't include standard libraries.  */
int keep_pragma_opt = FALSE;                 /* Keep pragmas in the output.        */
int warndollar_opt = FALSE;                  /* Warn if $ is an identifier char.   */
int warnundefsymbols_opt = FALSE;            /* Warn if a symbol is undefined.     */
int keepcomments_opt = FALSE;                /* Pass comments thru to the output.  */
int warnnestedcomments_opt = FALSE;          /* Generate nested comment warnings.  */
int unassert_opt = FALSE;                    /* -A- option.                        */
int definestooutput_opt = FALSE;             /* -dD option.                        */
int definesonly_opt = FALSE;                 /* -dM option.                        */
int definestofile_opt = FALSE;               /* -dF option.                        */
int makerule_opts = 0;                       /* -M options.                        */
int undef_builtin_macros_opt = FALSE;        /* -u | -undef option.         */
int use_trigraphs_opt = FALSE;               /* -trigraphs option.                 */
int warn_trigraphs_opt = FALSE;              /* -Wtrigraphs option.                */
int warn_all_opt = FALSE;                    /* -Wall option.                      */
int verbose_opt = FALSE;                     /* -v option.                         */
int no_warnings_opt = FALSE;                 /* -w option.                         */
int lang_cplusplus_opt = FALSE;              /* -lang-c++ option.                  */
int warnings_to_errors_opt = FALSE;          /* -Werror option.                    */
int print_headers_opt = FALSE;               /* -H option.                         */
int pre_preprocessed_opt = FALSE;            /* -fpreprocessed opt.                */
int gcc_macros_opt = TRUE;                   /* -gcc option - enabled by default.  */
int definenamesonly_opt = FALSE;             /* -dN option.                        */
int warnunused_opt = FALSE;                  /* -Wunused option.                   */
int no_include_opt = FALSE;                  /* --no-include option.               */
int no_simple_macros_opt = FALSE;            /* --no-simple-macros option.         */
int move_includes_opt = FALSE;               /* --move-includes                    */
int warn_dup_symbol_wo_args_opt = FALSE;     /* -Wmissing-args option.             */
int traditional_opt = FALSE;                 /* -traditional and 
                                                -traditional-cpp     */

struct _optdefs {
  BUILTIN_OPT opt;
  int val;
  char str[MAXLABEL];
} optdefs[] = {
  {nolinemarker_def,         0, "__CTPP_NOLINEMARKER_OPT__"},
  {nostdinc_def,             0, "__CTPP_NOSTDINC_OPT__"},
  {keep_pragma_def,          0, "__CTPP_KEEP_PRAGMA_OPT__"},
  {warndollar_def,           0, "__CTPP_WARNDOLLAR_OPT__"},
  {warnundefsymbols_def,     0, "__CTPP_WARNUNDEFSYMBOLS_OPT__"},
  {keepcomments_def,         0, "__CTPP_KEEPCOMMENTS_OPT__"},
  {warnnestedcomments_def,   0, "__CTPP_WARNNESTEDCOMMENTS_OPT__"},
  {unassert_def,             0, "__CTPP_UNASSERT_OPT__"},
  {definestooutput_def,      0, "__CTPP_DEFINESTOOUTPUT_OPT__"},
  {definestofile_def,        0, "__CTPP_DEFINESTOFILE_OPT__"},
  {definenamesonly_def,      0, "__CTPP_DEFINENAMESONLY_OPT__"},
  {definesonly_def,          0, "__CTPP_DEFINESONLY_OPT__"},
  {makerule_def,             0, "__CTPP_MAKERULE_OPT__"},
  {undef_builtin_def,        0, "__CTPP_UNDEF_BUILTIN_OPT__"},
  {use_trigraphs_def,        0, "__CTPP_USE_TRIGRAPHS_OPT__"},
  {warn_trigraphs_def,       0, "__CTPP_WARN_TRIGRAPHS_OPT__"},
  {warn_all_def,             0, "__CTPP_WARN_ALL_OPT__"},
  {lang_cplusplus_def,       0, "__CTPP_LANG_CPLUSPLUS_OPT__"},
  {no_warnings_def,          0, "__CTPP_NO_WARNINGS_OPT__"},
  {verbose_def,              0, "__CTPP_VERBOSE_OPT__"},
  {warnings_to_errors_def,   0, "__CTPP_WARNINGS_TO_ERRORS_OPT__"},
  {print_headers_def,        0, "__CTPP_PRINT_HEADERS_OPT__"},
  {pre_preprocessed_def,     0, "__CTPP_PRE_PREPROCESSED_OPT__"},
  {gcc_macros_def,           0, "__CTPP_GCC_MACROS_OPT__"},
  {warnunused_def,           0, "__CTPP_WARN_UNUSED_OPT__"},
  {no_include_def,           0, "__CTPP_NO_INCLUDE_OPT__"},
  {no_simple_macros_def,     0, "__CTPP_NO_SIMPLE_MACROS_OPT__"},
  {move_includes_def,        0, "__CTPP_MOVE_INCLUDES_OPT__"},
  {warn_missing_args_def,    0, "__CTPP_WARN_MISSING_ARGS_OPT__"},
  {(BUILTIN_OPT)0,           0, ""}
};

void __argvName (char *s) {
   strcpy (argv_name, s);
}

char *__argvFileName (void) {
  return argv_name;
}

void __source_file (char *s) {
  strcpy (rtinfo.source_file, s);
}

char *__source_filename (void) {
  return rtinfo.source_file;
}

void __init_time (void) {
   time_t t;
   t = time (NULL);
   (void)localtime (&t);
}

void set_traditional (void) {
  int i;
  for (i = 0; optdefs[i].opt; i++)
    optdefs[i].val = 0;
}

int symbol_is_command_line_builtin (char *sym) {
  int i;
  for (i = 0; optdefs[i].opt; i++) {
    if (!strcmp (sym, optdefs[i].str))
      return 1;
  }
  return 0;
}
