/* $Id: main.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2018 Robert Kiesling, rk3314042@gmail.com.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "ctpp.h"

/* 
 *    CLASSLIBDIR and PKGNAME should be defined in CFLAGs.
 *    If not, use the include directories with the PACKAGE name to
 *    search for class library headers, and use appname[] for the 
 *    package name.
 */
#ifdef CLASSLIBDIR 
char *classlibdir = CLASSLIBDIR;
#else
/*@null@*/
char *classlibdir = NULL;
#endif

#ifdef PKGNAME
char *pkgname = PKGNAME;
#else
/*@null@*/
char *pkgname = NULL;
#endif

#define _error_out _warning

int n_user_include_dirs = 0;

/* Declared in lib/rtinfo.c. */
extern int nolinemarker_opt;             /* Don't include line info in output. */
extern int nolinemarkerflags_opt;        /* Don't add enter/exit flags to the linemarkers. */
extern int nostdinc_opt;                 /* Don't include standard libraries.  */
extern int keep_pragma_opt;              /* Keep pragmas in the output.        */
extern int warndollar_opt;               /* Warn if $ is an identifier char.   */
extern int warnundefsymbols_opt;         /* Warn if a symbol is undefined.     */
extern int keepcomments_opt;             /* Pass comments to the output.       */
extern int warnnestedcomments_opt;       /* Generate nested comment warnings.  */
extern int unassert_opt;                 /* -A- option.                        */
extern int definestooutput_opt;          /* -dD option.                        */
extern int definestofile_opt;            /* -dF option.                        */
extern int definenamesonly_opt;          /* -dN option.                        */
extern int definesonly_opt;              /* -dM option.                        */
extern int makerule_opts;                /* -M... options.                     */
extern int undef_builtin_macros_opt;     /* -u | -undef.                       */
extern int use_trigraphs_opt;            /* -trigraphs option.                 */
extern int warn_trigraphs_opt;           /* -Wtrigraphs option.                */
extern int warn_all_opt;                 /* -Wall option.                      */
extern int lang_cplusplus_opt;           /* --lang-c++ option.                 */
extern int no_warnings_opt;              /* -w option.                         */
extern int verbose_opt;                  /* -v option.                         */
extern int warnings_to_errors_opt;       /* -Werror option.                    */
extern int print_headers_opt;            /* -H option.                         */
extern int pre_preprocessed_opt;         /* -fpreprocessed option.             */
extern int gcc_macros_opt;               /* -gcc, -no_gcc - True by default.   */
extern int warnunused_opt;               /* -Wunused-macros                    */
extern int no_include_opt;               /* --no-include option - implies -P.  */
extern int no_simple_macros_opt;         /* --no-simple-macros option.         */
extern int move_includes_opt;            /* --move-includes option.            */
extern int traditional_opt;              /* --traditional and --traditional-cpp*/
extern int warn_dup_symbol_wo_args_opt;  /* -Wmissing-args option.   */
extern char source_file[FILENAME_MAX];
extern char appname[FILENAME_MAX];

extern struct _optdefs {
  BUILTIN_OPT opt;
  int val;
  char str[MAXLABEL];
} optdefs[];

#define OPT_BUILTIN_SET(__opt) {if (!traditional_opt)optdefs[__opt].val = TRUE;}

extern char 
output_file[FILENAME_MAX];           /* Name of the output file. Used in   */
                                     /* lib/tempio.c.                      */

#ifndef HAVE_OFF_T
extern int input_size;               /* Declared in lib/read.c.            */
#else
extern off_t input_size;
#endif

int include_dir_opt = FALSE;         /* Look for ctalk headers in <dir>.  */
char *user_include_dirs[MAXUSERDIRS];/* User-defined include dirs.        */

int include_inhibit_opt = FALSE;     /* -I- option.                       */
int include_uninhibit_idx = -1;      /* First include after -I-           */

extern HASHTAB macrodefs;            /* Declared in hash.c.               */
extern HASHTAB ansisymbols;
extern void build_hashes (void);


int main (int argc, char **argv) {

  char *input;
  struct rlimit rlim;

  rlim.rlim_cur = 10000000;
  rlim.rlim_max = 10000000;
  setrlimit (RLIMIT_CORE, &rlim);

  build_hashes ();
  ansi_symbol_init ();

  parse_args (argc, argv);
  opt_builtins ();

  ccompat_init ();
  init_include_paths ();
  init_tmp_files ();
  init_poison_identifiers ();
  init_preprocess_if_vals ();

  if (verbose_opt) verbose_info ();

  if (unassert_opt) handle_unassert ();

  if (makerule_opts & MAKERULE) {
    if (make_target () == ERROR)
      exit (EXIT_FAILURE);
  }

  if (undef_builtin_macros_opt) undefine_builtin_macros ();

  if (makerule_opts & MAKERULETOFILE) check_rules_file ();

  /*
   *  Process macros files specified with -imacros <file>.
   */
  define_imacros ();

  perform_undef_macro_opt ();

  /*
   *  Process files named with -include <file>.
   */
  process_include_opt_files ();

  if (pre_preprocessed_opt) {
    copy_file (source_file, output_file);
    exit (0);
  }

  /*
   *  If there's a read error on the input, print
   *  an appropriate error message depending on whether
   *  the input is a file or stdin, and exit.
   */
  if ((input_size = read_file (&input, source_file)) < 0) {
    if (strcmp (source_file, "-") && errno) {
      fprintf (stderr, "%s: %s.\n", source_file, strerror (errno));
    } else {
      if (errno)
	fprintf (stderr, "stdin: %s.\n", strerror (errno));
    }
    exit (EXIT_FAILURE);
  }

  ansi__FILE__ (source_file);
  
  create_tmp ();

  preprocess (input);

  free (input);

#if defined CONF_INCLUDE_PATH
  cleanup_conf_include_searchdirs ();
#endif

  exit (0);

}

void parse_args (int c, char **a) {

  int i, l;

  *source_file = *output_file = '\0';

  strcpy (appname, a[0]);

  for (i = 1; i < c; i++) {

    switch (a[i][0])
      {
      case '-':
	if ((l = strlen (a[i])) == 1) {

	  /* stdin or stdout */
	  strcpy (((!*source_file) ? source_file : output_file), a[i]);

	} else {
	  switch (a[i][1])
	    {
	    case '-':
	      if (a[i][2] == 'i') {
		/*
		 *  Args like --imacro, --include, etc.
		 */
		i += include_opt (a, i, c);
	      } else {
		i += extended_arg (a[i]);
	      }
	      break;
	    case '$':
	      warndollar_opt = TRUE;
	      OPT_BUILTIN_SET (warndollar_def);
	      break;
	    case 'A':
	      i += assert_opt (a, i, c);
	      break;
	    case 'C':
	      keepcomments_opt = TRUE;
	      OPT_BUILTIN_SET (keepcomments_def);
	      break;
	    case 'D':
	      i += define_opt (a, i, c);
	      break;
	    case 'H':
	      print_headers_opt = TRUE;
	      OPT_BUILTIN_SET (print_headers_def);
	      break;
	    case 'I':
	      if (a[i][2] == '-') {
		include_inhibit_opt = TRUE;
		include_uninhibit_idx = n_user_include_dirs;
	      } else {
		include_dir_opt = TRUE;
		if (a[i][2]) {
		  user_include_dirs[n_user_include_dirs++] = strdup  (&a[i][2]);
		} else {
		  user_include_dirs[n_user_include_dirs++] = strdup  (a[++i]);
		}
	      }
	      break;
	    case 'M':
	      if (a[i][2]) {
		switch (a[i][2])
		  {
		  case 'D':
		    makerule_opts |= (MAKERULE | MAKERULETOFILE);
		    i += makerule_filename (a, i, c);
		    break;
		  case 'G':
		    /*
		     *  -MG must be specified in addition to 
		     *  -M options.  Don't set MAKERULE bit 
		     *  here.
		     */
		    makerule_opts |= MAKERULEGENHEADER;
		    break;
		  case 'M':
		    makerule_opts |= (MAKERULE | MAKERULELOCALHEADER);
		    if (a[i][3]) {
		      switch (a[i][3])
			{
			case 'D':
			  makerule_opts |= (MAKERULE | MAKERULETOFILE);
			  i += makerule_filename (a, i, c);
			  break;
			default:
			  help ();
			  break;
			}
		    }
		    break;
		  case 'T':
		  case 'Q':
		    makerule_opts |= (MAKERULE | MAKERULEUSERTARGET);
		    if (a[i + 1][0] == '-')
		      help ();
		    add_user_target (a[++i]);
		    break;
		  default:
		    help ();
		    break;
		  }
	      } else {
		makerule_opts |= MAKERULE;
	      }
	      OPT_BUILTIN_SET (makerule_def);
	      break;
	    case 'o':
	      if (strlen (a[i]) == 2) {
		if (((i + 1) == c) || (((i + 1) < c) && (a[i+1][0] == '-')))
		  help ();
		strcpy (output_file, a[++i]);
	      } else {
		strcpy (output_file, &a[i][2]);
	      }
	      break;
	    case 'P':
	      if (a[i][2] == 'f') {
		nolinemarkerflags_opt = TRUE;
	      } else {
		nolinemarker_opt = TRUE;
		OPT_BUILTIN_SET (nolinemarker_def);
	      }
	      break;
	    case 'U':
	      i += undefine_macro_opt (a, i, c);
	      break;
	    case 'd':
	      switch (a[i][2])
		{
		case 'D':
		  definestooutput_opt = TRUE;
		  OPT_BUILTIN_SET (definestooutput_def);
		  break;
		case 'F':
		  i += set_defines_file_name (a, i, c);
		  definestofile_opt = TRUE;
		  OPT_BUILTIN_SET (definestofile_def);
		  break;
		case 'M':
		  definesonly_opt = TRUE;
		  OPT_BUILTIN_SET (definesonly_def);
		  break;
		case 'N':
		  definenamesonly_opt = TRUE;
		  OPT_BUILTIN_SET (definenamesonly_def);
		  break;
		default:
		  help ();
		  break;
		}
	      break;
	    case 'f':
	      i += extended_arg (a[i]);
	      break;
	    case 'i':
	      i += include_opt (a, i, c);
	      break;
	    case 'u':
	      undef_builtin_macros_opt = TRUE;
	      break;
	    case 'v':
	      /*
	       *  Decide whether the arg is -v or -version.
	       */
	      if (strlen (a[i]) == 2) {
		verbose_opt = TRUE;
	      } else {
		i += extended_arg (a[i]);
	      }
	      break;
	    case 'w':
	      no_warnings_opt = TRUE;
	      OPT_BUILTIN_SET (no_warnings_def);
	      break;
	    case 'x':
	      /*
	       *  The same as -lang-c++, the only language
	       *  switch that has any effect.
	       */
	      if (!strcmp (a[++i], "c++")) {
		lang_cplusplus_opt = TRUE;
		OPT_BUILTIN_SET (lang_cplusplus_def);
	      }
	      if (!strcmp (a[++i], "c"))
		lang_cplusplus_opt = FALSE;
	      /*
	       *  The other language options are no-ops.
	       */
	      if (strcmp (a[i], "objective-c") &&
		  strcmp (a[i], "assembler-with-cpp"))
		help ();
	      break;
	    default:
	      if (!strncmp (a[i], "-std", strlen ("-std")) ||
		  !strncmp (a[i], "--std", strlen ("--std"))) {
		i += std_opt (a, i, c); 
	      } else {
		i += extended_arg (a[i]);
	      }
	      break;
	    }
	}
	break;
      default:
	strcpy (((!*source_file) ? source_file : output_file), a[i]);
	break;
      }
  }

  /*
   *  If the user doesn't specify an input or output file,
   *  use stdin or stdout.
   */
  if (!*source_file) strcpy (source_file, "-");
  if (!*output_file) strcpy (output_file, "-");

  input_lang_from_file (source_file);

}

void help (void) {

  printf ("Usage: ctpp [-$] [-A predicate [(value)]] [-ansi] [-C] [-CC] [-Dname[=definition] [-dD] [-dF file] [-dM] [-dN] [-fno-show-column] [-fpreprocessed] [-gcc] [-H] [-h |-help|--help] [-I <dir>] [-I-] [-idirafter <dir>] [-imacros <file>] [-include <file>] [-iprefix <prefix>] [-isystem <dir>] [-iwithprefix <dir>] [-lang-c] [-lang-c++] [-lang-objc] [-lang-objc++] [-lint] [-M [-MG]] [-M ] [-MD file] [-MM] [-MMD file] [-MQ target] [-MT target] [-lang-c] [-lang-c++] [-lang-objc] [-lang-objc++] [-lint] [-move-includes] [-no-gcc] [-nostdinc] [-nostdinc++] [-P] [-pedantic] [-pedantic-errors] [-remap] [-std=gnu99...[-target-help] [-traditional] [-traditional-cpp] [-trigraphs] [-U <symbol>] [-u|-undef] [-v] [-version] [--version] [-w] [-Wall] [-Wcomment] [-Wendif-labels] [-Werror] [-Wimport] [-Wmising-args] [-Wsystem_headers] [-Wtraditional] [-Wtrigraphs] [-Wundef] [-Wunused-macros] [-x <lang>] [-no-include] [-no-simple-macros] infile|- [-o] outfile|-\n");
  printf ("Report bugs to rkies@cpan.org.\n");
  exit (EXIT_FAILURE);
}

int include_opt (char **a, int i, int c) {

  if (!strcmp (a[i], "-imacros") ||
      !strcmp (a[i], "--imacros"))
    return imacro_filename (a, i, c);

  if (!strcmp (a[i], "-include") ||
      !strcmp (a[i], "--include"))
    return include_filename (a, i, c);

  if (!strcmp (a[i], "-idirafter") ||
      !strcmp (a[i], "--idirafter"))
    return include_dirafter_name (a, i, c);

  if (!strcmp (a[i], "-iprefix") ||
      !strcmp (a[i], "--iprefix"))
    return include_prefixafter_name (a, i, c);

  if (!strcmp (a[i], "-iwithprefix") ||
      !strcmp (a[i], "--iwithprefix"))
    return include_dirafter_name_prefix (a, i, c);

  if (!strcmp (a[i], "-isystem") ||
      !strcmp (a[i], "--isystem"))
    return include_systemdir_name (a, i, c);

  return 0;
}

int extended_arg (char *s) {

  char buf[MAXMSG];

  if (!strcmp (s, "--nostdinc") ||
      !strcmp (s, "-nostdinc")) {
    OPT_BUILTIN_SET (nostdinc_def);
    nostdinc_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "--keeppragmas")) {
    OPT_BUILTIN_SET (keep_pragma_def);
    keep_pragma_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "-lang-c++") ||
      !strcmp (s, "--lang-c++")) {
    OPT_BUILTIN_SET (lang_cplusplus_def);
    lang_cplusplus_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "-version")) {
    SNPRINTF (buf, MAXMSG, "%s\n", VERSION);
    _error_out (buf);
    return 0;
  }

  if (!strcmp (s, "--version")) {
    SNPRINTF (buf, MAXMSG, "%s\n", VERSION);
    _error_out (buf);
    exit (0);
  }

  if (!strcmp (s, "-Werror") ||
      !strcmp (s, "--Werror")) {
    OPT_BUILTIN_SET (warnings_to_errors_def);
    warnings_to_errors_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "-Wunused-macros") ||
      !strcmp (s, "--Wunused-macros")) {
    OPT_BUILTIN_SET (warnunused_def);
    warnunused_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "-no-include") ||
      !strcmp (s, "--no-include")) {
    OPT_BUILTIN_SET (no_include_def);
    OPT_BUILTIN_SET (nolinemarker_def);
    no_include_opt = TRUE;
    nolinemarker_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "-no-simple-macros") ||
      !strcmp (s, "--no-simple-macros")) {
    no_simple_macros_opt = TRUE;
    OPT_BUILTIN_SET (no_simple_macros_def);
    return 0;
  }

  if (!strcmp (s, "-fpreprocessed") ||
      !strcmp (s, "--fpreprocessed")) {
    pre_preprocessed_opt = TRUE;
    OPT_BUILTIN_SET (pre_preprocessed_def);
    return 0;
  }

  if (!strcmp (s, "-gcc") ||
      !strcmp (s, "--gcc")) {
    gcc_macros_opt = TRUE;
    OPT_BUILTIN_SET (gcc_macros_def);
    return 0;
  }

  if (!strcmp (s, "-no-gcc") ||
      !strcmp (s, "--no-gcc")) {
    gcc_macros_opt = FALSE;
    return 0;
  }

  if (!strcmp (s, "-ansi") ||
      !strcmp (s, "--ansi")) {
    lang_cplusplus_opt = FALSE;
    return 0;
  }
 
  if (!strcmp (s, "-lang-c") ||
      !strcmp (s, "--lang-c")) {
    lang_cplusplus_opt = FALSE;
    return 0;
  }
    
  if (!strcmp (s, "-move-includes") ||
      !strcmp (s, "--move-includes")) {
    move_includes_opt = TRUE;
    OPT_BUILTIN_SET (move_includes_def);
    return 0;
  }

  if (!strcmp (s, "-traditional") ||
      !strcmp (s, "--traditional") ||
      !strcmp (s, "-traditional-cpp") ||
      !strcmp (s, "--traditional-cpp")) {
    /* Suppresses builtin command line macros. 
       This option isn't in the builtin set. */
    set_traditional ();
    traditional_opt =  TRUE;
  }

 /*
   *  Options for compatibility with GNU cpp(1).  They are no-ops
   *  at the present.
   *
   *  cpp 2.95.
   */
  if (!strcmp (s, "-lang-objc") ||
      !strcmp (s, "--lang-objc") ||
       !strcmp (s, "-lang-objc++") ||
       !strcmp (s, "--lang-objc++") ||
       !strcmp (s, "-lint") ||
       !strcmp (s, "--lint") ||
       !strcmp (s, "-pedantic") ||
       !strcmp (s, "--pedantic") ||
       !strcmp (s, "-pedantic-errors") ||
       !strcmp (s, "--pedantic-errors") ||
       !strcmp (s, "-traditional") ||
       !strcmp (s, "--traditional") ||
       !strcmp (s, "-Wtraditional") ||
       !strcmp (s, "--Wtraditional") ||
      /*
       *  cpp 3.3.
       */
      !strcmp (s, "-Wimport") ||
      !strcmp (s, "--Wimport") ||
      !strcmp (s, "-Wendif-labels") ||
      !strcmp (s, "--Wendif-labels") ||
      !strcmp (s, "-Wsystem-headers") ||
      !strcmp (s, "--Wsystem-headers") ||
      !strcmp (s, "-nostdinc++") ||
      !strcmp (s, "--nostdinc++") ||
      !strcmp (s, "-fpreprocessed") ||
      !strcmp (s, "--fpreprocessed") ||
      !strcmp (s, "-fno-show-column") ||
      !strcmp (s, "--fno-show-column") ||
      !strcmp (s, "-remap") ||
      !strcmp (s, "--remap"))
    return 0;

  if (!strcmp (s, "-trigraphs") ||
      !strcmp (s, "--trigraphs")) {
    OPT_BUILTIN_SET (use_trigraphs_def);
    use_trigraphs_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "-Wtrigraphs") ||
      !strcmp (s, "--Wtrigraphs")) {
    warn_trigraphs_opt = TRUE;
    OPT_BUILTIN_SET (warn_trigraphs_def);
    return 0;
  }

  if (!strcmp (s, "-undef") ||
      !strcmp (s, "--undef")) {
    undef_builtin_macros_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "-Wundef") ||
      !strcmp (s, "--Wundef")) {
    warnundefsymbols_opt = TRUE;
    OPT_BUILTIN_SET (warnunused_def);
    return 0;
  }

  if (!strcmp (s, "-Wcomment") ||
      !strcmp (s, "-Wcomments") ||
      !strcmp (s, "--Wcomment") ||
      !strcmp (s, "--Wcomments")) {
    OPT_BUILTIN_SET (warnnestedcomments_def);
    warnnestedcomments_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "-Wmissing-args") ||
      !strcmp (s, "--Wmissing-args")) {
    OPT_BUILTIN_SET (warn_missing_args_def);
    warn_dup_symbol_wo_args_opt = TRUE;
    return 0;
  }

  if (!strcmp (s, "-Wall") ||
      !strcmp (s, "--Wall")) {
    warn_all_opt = TRUE;
    warnnestedcomments_opt = TRUE;
    warnundefsymbols_opt = TRUE;
    warn_trigraphs_opt = TRUE;
    warndollar_opt = TRUE;
    OPT_BUILTIN_SET (warnnestedcomments_def);
    OPT_BUILTIN_SET (warnundefsymbols_def);
    OPT_BUILTIN_SET (warn_trigraphs_def);
    return 0;
  }
    
      /*
	FIX!  Implement these.
       */
  if (!strcmp (s, "-iwithprefixbefore") ||
      !strcmp (s, "--iwithprefixbefore") ||
      !strcmp (s, "-ftabstop") ||
      !strcmp (s, "--ftabstop")) {
    printf ("Option %s is not implemented in ctpp %s.\n",
	    s, VERSION);
    exit (1);
  }

  if (!strcmp (s, "-help") ||
      !strcmp (s, "--help") ||
      !strcmp (s, "-target_help") ||
      !strcmp (s, "--target_help"))
    help ();

  /* Unknown option. */
  help ();

  return SUCCESS;
}

/*
 *  If neither of the language options -lang-c++ or 
 *  -fpreprocessed options is set, determine the
 *  language option from the file extension.
 */
#define EXT_CHAR '.'
#define STD_C_EXT ".c"
#define PREPROCESSED_C_EXT ".i"
#define STD_CPLUSPLUS_EXT1 ".cc"
#define STD_CPLUSPLUS_EXT2 ".cpp"
#define PREPROCESSED_CPLUSPLUS_EXT ".ii"

void input_lang_from_file (char *fn) {

  char *ext_ptr;

  if (pre_preprocessed_opt || lang_cplusplus_opt)
    return;

  if ((ext_ptr = rindex (fn, EXT_CHAR)) != NULL) {
    if (!strcmp (ext_ptr, STD_C_EXT))
      lang_cplusplus_opt = FALSE;
    if (!strcmp (ext_ptr, STD_CPLUSPLUS_EXT1) ||
	!strcmp (ext_ptr, STD_CPLUSPLUS_EXT2))
      lang_cplusplus_opt = TRUE;
    if (!strcmp (ext_ptr, PREPROCESSED_C_EXT) ||
	!strcmp (ext_ptr, PREPROCESSED_CPLUSPLUS_EXT))
      pre_preprocessed_opt = TRUE;
    
  }

}

void verbose_info (void) {
  char buf[MAXMSG];
  sprintf (buf, "%s\n", VERSION);
  _error_out (buf);
  strcpy (buf, "Include paths:\n");
  _error_out (buf);
  dump_include_paths ();
}

static char *c_lang_opt_vals[] = {
  "iso9899:1990",
  "c89",
  "iso9899:199409",
  "iso9899:1999",
  "c99",
  "iso9899:199x",
  "c9x",
  "gnu89",
  "gnu99",
  "gnu9x"
};

#define N_C_LANG_OPTS 10

static char *cplus_lang_opt_vals[] = {
  "c++98",
  "gnu++98"
};

#define N_CPLUS_LANG_OPTS 2

int std_opt (char **a, int idx, int cnt) { 

  char *p, langval[MAXLABEL];
  int arg_idx, i;
  bool match;

  arg_idx = idx;

  if ((p = index (a[idx], '=')) != NULL) {
    /*
     *  Form -std=<arg>
     */
    strcpy (langval, p + 1);
  } else {
    if (strcmp (a[++arg_idx], "=")) {
      /*
       *  Form -std =<arg>
       */
      if (a[arg_idx][1])
	strcpy (langval, &a[arg_idx][1]);
    } else {
      if (a[arg_idx][0] != '=') {
	_warning ("Argument syntax error: %s.\n", a[arg_idx]);
	return 0;
      }
      /*
       *  Form -std = <arg>
       */
      strcpy (langval, a[++arg_idx]);
    }
  }

  for (i = 0, match = False; (i < N_C_LANG_OPTS) && !match; i++) {
    if (!strcmp (langval, c_lang_opt_vals[i])) {
      match = True;
      lang_cplusplus_opt = FALSE;
    }
  }

  if (!match) {
    for (i = 0; (i < N_CPLUS_LANG_OPTS) && !match; i++) {
      if (!strcmp (langval, cplus_lang_opt_vals[i])) {
	match = True;
	lang_cplusplus_opt = TRUE;
      }
    }
  }

  if (!match)
    _warning ("ctpp: Bad language option %s.\n", langval);

  return arg_idx - idx;

}

