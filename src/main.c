/* $Id: main.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2018 Robert Kiesling, rk3314042@gmail.com.
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
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>
#ifdef __DJGPP__
#include "djgpp.h"
#include <dpmi.h>
#include <crt0.h>
#endif
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "classlib.h"

extern char *classlibdir;                /* Declared in rtinfo.c.           */
extern char *pkgname;

char libcachedir[FILENAME_MAX];
char ctalkuserhomedir[FILENAME_MAX];
char usertemplatedir[FILENAME_MAX];

static int preprocess_only_opt = FALSE;  /* Preprocess only.                */
static int help_opt = FALSE;             /* Print help message and exit.    */
static int version_opt = FALSE;          /* Print version and exit.         */
static int keep_pragma_opt = FALSE;      /* Keep pragmas in output file.    */
int nolibinc_opt = FALSE;             /* Don't include ctalk's headers.     */
int nolinemarker_opt = FALSE;         /* Don't output line markers.         */
int verbose_opt = FALSE;              /* Verbose warnings.                  */
int warn_extension_opt = FALSE;       /* Print warning for some extensions. */
int warn_classlib_opt = FALSE;        /* Print class library loading.       */
int warn_duplicate_name_opt = FALSE;  /* Warn of C variable shadowing.      */
int warn_unresolved_self_opt = FALSE; /* Warn of "self" expressions that    */
                                      /* need to wait for compile time.     */
int print_libdirs_opt = FALSE;        /* Print library directories.         */
int show_progress_opt = FALSE;        /* Show compile progress with dots.   */
int nopreload_opt = FALSE;            /* Don't use preloaded methods.       */
int clearpreload_opt = FALSE;         /* Don't use preloaded methods.       */
int args_changed = FALSE;             /* User args changed.                 */
int print_templates_opt = FALSE;
char *user_include_dirs[MAXUSERDIRS]; /* User-defined include dirs.         */

/* 
 *   Option to specify the compiler's system include directory, if the 
 *   program can't determine it.  For GCC on UNIX, the directory should 
 *   be:
 *   /usr/lib/gcc-lib/<target>/<version>/include/
 *
 *   On older SunOS systems, GCC and its libraries might be under 
 *   /opt.  
 */
int systeminc_opt = FALSE;
char *systeminc_dirs[MAXUSERDIRS]; 
int n_system_inc_dirs = 0;
int n_user_include_dirs = 0;

static char apppath[FILENAME_MAX]; /* Path to the application, from argv[0].*/
extern char *source_files[MAXARGS]; /* Declared in infiles.c */
extern int n_input_files, input_idx;
char appname[FILENAME_MAX],        /* Ctalk executable name.                */
  input_source_file[FILENAME_MAX];/* Name of the current source file.       */

extern I_PASS interpreter_pass;

char *get_class_lib_dir (void) {
  return classlibdir;
}

char *get_package_name (void) {
  return pkgname;
}

                                    /* Output file options.                  */
extern int outputfile_opt;          /* Declared in tempio.c                  */
extern char 
output_file[FILENAME_MAX];

static char p_source_file[FILENAME_MAX]; /* Name of preprocessed input file. */
char ctpp_ofn[FILENAME_MAX];      /* File name of any preprocessor output.   */
static void clear_method_cache (void);
extern char *home_libcache_path (void);

void user_interrupt (int signo) {
  struct sigaction act, oldact;
  sigset_t mask;
  act.sa_handler = SIG_DFL;
  sigemptyset (&mask);
  act.sa_mask = mask;
  act.sa_flags = SA_RESETHAND;
  sigaction (SIGINT, &act, &oldact);
  cleanup_preprocessor_cache ();
  cleanup_temp_files (TRUE);
  killpg (getpgrp (), SIGINT);
}

static void show_progress (void) {
  if (show_progress_opt) {
    fprintf (stdout, ".");
    fflush (stdout);
  }
}

int main (int argc, char **argv) {

  char *input;
  struct stat statbuf;
  int r_stat,                       /* Results from stat ().                 */
    r;                              /* Results from other lib calls.         */
  long long source_stat_size;       /* Size of the source file, from stat(). */
  int exit_code = EXIT_SUCCESS;
  CLASSLIB *declarations;
  struct rlimit rlim;
  struct sigaction act, oldact;
  sigset_t mask;

  rlim.rlim_cur = 10000000;
  rlim.rlim_max = 10000000;
  setrlimit (RLIMIT_CORE, &rlim);

  act.sa_handler = user_interrupt;
  sigemptyset (&mask);
  act.sa_mask = mask;
  act.sa_flags = SA_RESETHAND;
  sigaction (SIGINT, &act, &oldact);

  outputfile_opt = FALSE;

#ifdef MALLOC_TRACE
  mtrace ();
#endif

  CTFLAGS_args();
  args (argv, argc, FALSE);

  if (version_opt) {
    printf ("%s\n", VERSION);
    exit (EXIT_SUCCESS);
  }

  init_library_paths ();

  if (print_libdirs_opt) {
    print_libdirs ();
    exit (EXIT_SUCCESS);
  }

  if (clearpreload_opt) {
    clear_method_cache ();
    if (!n_input_files)
      exit (EXIT_SUCCESS);
  }

  if (help_opt || ! n_input_files)
    exit_help ();

  save_opts (argv, argc);
  init_error_location ();
  /*
   *  Make sure we have the name of a source file for error
   *  checking during the initialization.
   */
  __ctalkRtSaveSourceFileName (source_files[0]);

  init_library_include_stack ();
  init_user_home_path ();
  init_lib_cache_path ();
  init_user_template_path ();
  rt_get_class_names ();
  check_user_args ();
  init_tmp_files ();
  init_object_size ();
  get_user_template_names ();
  restore_decl_list ();

  init_cvars ();
  init_frames ();
  init_message_stack ();
  init_method_stack ();
  init_new_method_stack ();
  init_pre_methods ();
  init_pre_classes ();
  init_control_blocks ();
  init_arg_blocks ();
  init_r_messages ();
  init_loops ();
  init_parser_frame_ptr ();
  init_parsers ();
  init_method_proto ();
  init_imported_method_queue ();
  init_fn_templates ();
  init_fn_template_cache ();
  init_lib_cache_path ();
  init_ct_default_class_cache ();

  if (outputfile_opt == TRUE) {
    if (is_input_file (output_file)) {
      _warning ("%s: Output file %s would overwrite input file of the same name.\n", appname, output_file);
      exit (EXIT_FAILURE);
    }
    save_previous_output (output_file);
    if ((r_stat = stat (output_file, &statbuf)) != -1) {
      if ((r = unlink (output_file)) == -1)
 	_error ("%s: %s: %s\n", appname, output_file, strerror (errno));
    }
  }

  init_output_vars ();
  if (nolinemarker_opt)
    get_ctalkdefs_h_lines ();

#ifdef __DJGPP__
/*   sbrk (DJGPP_HEAP_SIZE); */
  _crt0_startup_flags |= _CRT0_FLAG_PRESERVE_FILENAME_CASE;
#endif

  /* Do not generate any code before this. */
  if (!preprocess_only_opt)
    init_default_classes ();

  for (input_idx = 0; input_idx < n_input_files; input_idx++) {

    if (is_binary_file (source_files[input_idx])) {
      printf ("ctalk: %s: is a binary file.\n", source_files[input_idx]);
      exit_code = EXIT_FAILURE;
      break;
    }

    strcpy (input_source_file, source_files[input_idx]);

    /* Pass 1 - Preprocessing. */

    if (stat (input_source_file, &statbuf) != 0) {
      exit_code = EXIT_FAILURE;
      printf ("%s: %s.\n", input_source_file, strerror (errno));
      break;
    }

    /*
     *  Fills in ctpp_ofn file name.
     */
    show_progress ();
    if (preprocess (input_source_file, (input_idx == 0), false) == ERROR) {
      exit_code = EXIT_FAILURE;
      break;
    }

    if (preprocess_only_opt) {
      preprocess_to_output (input_source_file);
      cleanup_temp_files (FALSE);
      continue;
    }

    strcpy (p_source_file, ctpp_ofn);

    /* Pass 2 - Register variables. */

    interpreter_pass = var_pass;

    parse_vars (p_source_file);
    resolve_incomplete_types ();

    /* Pass 3 - Parsing. */

    interpreter_pass = parsing_pass;

    if (stat (p_source_file, &statbuf) != 0)
      _error ("%s source file %s: %s.", appname, input_source_file,
	      strerror (errno));

    source_stat_size = statbuf.st_size;

    if ((input = (char *)__xalloc (source_stat_size + 1)) == NULL)
      _error ("%s: %s.", p_source_file, strerror (errno));

    push_input_declaration (p_source_file, p_source_file, 0);
    input_prototypes (p_source_file);

    r = read_file (input, p_source_file);

    input = parse (input, source_stat_size);

    __xfree (MEMADDR(input));
    
    if (file_exists (p_source_file))
      if (unlink (p_source_file)) {
	printf ("ctalk: unlink: %s: %s.\n", p_source_file, strerror (errno));
	exit_code = EXIT_FAILURE;
	break;
      }

    declarations = pop_input_declaration ();
    save_infile_declarations (declarations);

  }
  
  cleanup_template_cache (); 
  record_opts ();
  save_decl_list ();
  delete_class_library ();
  cleanup_dictionary ();
  cleanup_preprocessor_cache ();
  cleanup_output_vars ();
  cleanup_temp_files (0);
  cleanup_CTFLAGS_args ();
  cleanup_user_prototypes ();
  delete_ct_default_class_cache ();
  rt_delete_class_names ();

#ifdef MALLOC_TRACE
  muntrace ();
#endif

  exit (exit_code);
}

int n_args = 0;
char *CTFLAGS_strings[MAXARGS] = {NULL,};

static char *next_whitespace (char *s) {
  int i;
  
  for (i = 0; s[i]; i++)
    if (isspace ((int)s[i]))
      return &s[i];

  return NULL;
}

static char *next_nonwhitespace (char *s) {
  int i;
  
  for (i = 0; s[i]; i++)
    if (!isspace ((int)s[i]))
      return &s[i];

  return NULL;
}

void cleanup_CTFLAGS_args (void) {
  int i;
  for (i = 0; i < n_args; i++)
    if (CTFLAGS_strings[i])
      __xfree (MEMADDR(CTFLAGS_strings[i]));
}

void CTFLAGS_args (void) {

  char *env, *s, *t;
  char buf[MAXMSG];

  if ((env = getenv ("CTFLAGS")) == NULL) 
    return;

  if (*env == 0)
    return;

  s = env;

  while (TRUE) {

    if (((t = strchr (s, ' ')) == NULL) &&
	((t = strchr (s, '\t')) == NULL) &&
	((t = strchr (s, '\n')) == NULL)) {

      memset (buf, 0, MAXMSG);
      strcpy (buf, s);
      
      CTFLAGS_strings[n_args++] = strdup (buf);

      args (CTFLAGS_strings, n_args, TRUE);

      return;

    }

    if ((t = next_whitespace (s)) != NULL) {

      memset (buf, 0, MAXMSG);
      strncpy (buf, s, t - s);
      
      CTFLAGS_strings[n_args++] = strdup (buf);
    }

    if ((s = next_nonwhitespace (t)) == NULL) 
      return;

  }

}

int args (char **a, int c, int cflags_args) {

  char *p;
  int i;
  int start_arg;

  *input_source_file = *output_file = 0;

  if (!cflags_args) {
    strcpy (apppath, a[0]);

    if (((p = rindex (a[0], '/')) != NULL) ||
	((p = rindex (a[0], '\\')) != NULL)) {
      strcpy (appname, ++p);
    } else {
      strcpy (appname, a[0]);
    }

    if (!pkgname)
      pkgname = appname;

    start_arg = 1;

  } else {
    start_arg = 0;
  }

  for (i = start_arg, n_input_files = 0; i < c; i++) {
    if (a[i][0] == '-') {
      switch (a[i][1])
	{
	case '-':
	  i += extended_arg (a[i]);
	  break;
	case 'v':
	  verbose_opt = TRUE;
	  warn_extension_opt = TRUE;
	  warn_duplicate_name_opt = TRUE;
	  warn_unresolved_self_opt = TRUE;
	  break;
	case 'I':
	  if (a[i][2]) {
	    user_include_dirs[n_user_include_dirs++] = strdup  (&a[i][2]);
	  } else {
	    user_include_dirs[n_user_include_dirs++] = strdup  (a[++i]);
	  }
	  if (!is_dir (user_include_dirs[n_user_include_dirs - 1])) {
	    printf ("Include directory %s not found.\n",
		    user_include_dirs[n_user_include_dirs - 1]);
	    exit_help ();
	  }
	  break;
	case 'E':
	  preprocess_only_opt = TRUE;
	  break;
	case 'h':
	  help_opt = TRUE;
	  break;
	case 's':
	  systeminc_opt = TRUE;
	  if (a[i][2]) {
	    systeminc_dirs[n_system_inc_dirs++] = strdup  (&a[i][2]);
	  } else {
	    if (a[i+1]) {
	      systeminc_dirs[n_system_inc_dirs++] = strdup  (a[++i]);
	    } else {
	      exit_help ();
	    }
	  }
	  if ((!file_exists (systeminc_dirs[n_system_inc_dirs - 1])) ||
	      !is_dir (systeminc_dirs[n_system_inc_dirs - 1]))
	    exit_help ();
	  break;
	case 'o':
	  outputfile_opt = TRUE;
	  if (a[i][2]) {
	    strcpy (output_file, &a[i][2]);
	  } else {
	    strcpy (output_file, a[++i]);
	  }
	  break;
	case 'V':
	  version_opt = TRUE;
	  break;
	case 'P':
	  nolinemarker_opt = TRUE;
	  break;
	default:
	  printf ("Warning: Unrecognized option %s.\n", a[i]);
	  break;
	}
    } else {
      source_files[n_input_files++] = strdup (a[i]);
    }
  }

  return 0;
}

int extended_arg (char *s) {

  if (!strcmp (s, "--verbose")) {
    verbose_opt = TRUE;
    warn_extension_opt = TRUE;
    warn_duplicate_name_opt = TRUE;
    warn_unresolved_self_opt = TRUE;
  } else if (!strcmp (s, "--nolibinc")) {
    nolibinc_opt = TRUE;
  } else if (!strcmp (s, "--keeppragmas")) {
    keep_pragma_opt = TRUE;
  } else if (!strcmp (s, "--printlibdirs")) {
    print_libdirs_opt = TRUE;
  } else if (!strcmp (s, "--printtemplates")) {
    print_templates_opt = TRUE;
  } else if (!strcmp (s, "--warnextension")) {
    warn_extension_opt = TRUE;
  } else if (!strcmp (s, "--warnclasslibs")) {
    warn_classlib_opt = TRUE;
  } else if (!strcmp (s, "--warnduplicatenames")) {
    warn_duplicate_name_opt = TRUE;
  } else if (!strcmp (s, "--progress")) {
    show_progress_opt = TRUE;
  } else if (!strcmp (s, "--nopreload")) {
    nopreload_opt = TRUE;
  } else if (!strcmp (s, "--clearpreload")) {
    clearpreload_opt = TRUE;
  } else if (!strcmp (s, "--warnunresolvedselfexpr")) {
    warn_unresolved_self_opt = TRUE;
  } else if (!strcmp (s, "--help")) {
    exit_help ();
  } else {  /* Unknown option. */
    exit_help ();
  }

  return SUCCESS;
}

void exit_help (void) {

  printf (_("Usage: %s [-E] | [-h | --help] | [<option>] input_file(s) [-o output_file]\n"),
	  appname);
  printf (_("OPTIONS:\n"));
  printf (_("--clearpreload   Clear preloaded methods so they can be rewritten.\n"));
  printf (_("-E               Preprocess the input and exit.\n"));
  printf (_("-h               Print help a message and exit.\n"));
  printf (_("--help\n"));
  printf (_("-I <dir>         Add <dir> to ctalk include search path.\n"));
  printf (_("--keeppragmas    Write pragmas untranslated to the output.\n"));
  printf (_("--printlibdirs   Print library directories and exit.\n"));
  printf (_("--printtemplates Print function templates as they are cached.\n"));
  printf (_("--nolibinc       Do not include ctalk's system headers in the input.\n"));
  printf (_("--nopreload      Do not use preloaded methods.\n"));
  printf (_("-o <file>        Write output to <file> instead of standard output.\n"));
  printf (_("-P               Do not output line number information.\n"));
  printf (_("--progress       Print dots to indicate Ctalk's progress.\n"));
  printf (_("-s <dir>         Add <dir> to compiler system include search path.\n"));
  printf (_("-V               Print the ctalk version number and exit.\n"));
  printf (_("-v               Print verbose warnings.\n"));
  printf (_("--verbose        Same as -v.\n"));
  printf (_("--warnclasslibs  Print class library names as they are being loaded.\n"));
  printf (_("--warnduplicatenames\n"));
  printf (_("                 Print warnings when object names duplicate C variable names.\n"));
  printf (_("--warnextension  Print warnings for some compiler extensions.\n"));
  printf (_("--warnunresolvedselfexpr\n"));
  printf (_("                 Print warnings when expressions that contain, \"self,\" mainly\n"));
  printf (_("                 in argument blocks, can't be resolved until run time.\n"));
  printf (_("Report bugs to rk3314042@gmail.com.\n"));
  exit ((help_opt == TRUE) ? EXIT_SUCCESS : EXIT_FAILURE);
}

int preprocess (char *fn, bool include_lib, bool classlib_read) {
 
  char cmd[MAXMSG],
    argbuf[MAXMSG],
    tmpbuf[MAXMSG],
    cachebuf[MAXMSG],
    oldcache[FILENAME_MAX];
  char cmdbuf[2];
  char *lib_tmp_fn = NULL;     /* Avoid a warning. */
  int inc_idx;
  int r;
  int retval;
  FILE *P;
  char *path;

  *argbuf = 0;

  /*
   *  If there are cached macros, include them.
   */

  if (macro_cache_path ()) {
    strcpy (oldcache, macro_cache_path ());
    if (file_exists (oldcache)) {
      strcatx (cachebuf, "-imacros ", macro_cache_path (), " ", NULL);
      strcatx2 (argbuf, cachebuf, NULL);
    }
  } else {
    *oldcache = 0;
  }

  /*
   *  Includes with -I, then includes with -s.
   *  Use args from the command line if necessary.
   */
  for (inc_idx = 0; inc_idx < n_user_include_dirs; inc_idx++) {
    strcatx (tmpbuf, "-I", user_include_dirs[inc_idx], " ", NULL);
    strcatx2 (argbuf, tmpbuf, NULL);
  }
  for (inc_idx = 0; inc_idx < n_system_inc_dirs; inc_idx++) {
    strcatx (tmpbuf, "-I", systeminc_dirs[inc_idx], " ", NULL);
    strcatx2 (argbuf, tmpbuf, NULL);
  }
  if (nolinemarker_opt) strcatx2 (argbuf, "-P ", NULL);
  if (classlib_read) strcatx2 (argbuf, " -Pf ", NULL);
  
  show_progress ();

  if (!preprocess_only_opt && !nolibinc_opt && include_lib) {

    if ((path = ctalklib_path ()) != NULL) {
      strcatx (tmpbuf, "-include ", path, " ", NULL);
      strcatx2 (argbuf, tmpbuf, NULL);

    }
  }

  strcatx (cachebuf, "-dF ", new_macro_cache_path (), " ", NULL);
  strcatx2 (argbuf, cachebuf, NULL);

#ifndef __CYGWIN__
  if (include_lib)
    strcatx2 (argbuf, "-move-includes ", NULL);
#endif
  strcpy (ctpp_ofn, ctpp_tmp_name (fn, TRUE));

#ifdef __DJGPP__
  sprintf (cmd, "%s %s \"%s\" %s",
 	   which (CTPP_BIN), argbuf, fn, ctpp_ofn);
#else
  strcatx (cmd, which (CTPP_BIN), " ", argbuf, " ", fn, " ", ctpp_ofn, NULL);
#endif

  if ((P = popen (cmd, "r")) != NULL) {
    /*
     *  Needed for Solaris fprintf output.
     */
    cmdbuf[1] = '\0';
    while ((r = fread (cmdbuf, sizeof (char), 1, P)) > 0)
      printf ("%c", cmdbuf[0]);
  } else {
    printf ("%s: %s.\n", fn, strerror (errno));
    return ERROR;
  }

  show_progress ();

  retval = pclose (P);

  if (retval == -1) {
    printf ("%s: Error: %s\n", fn, strerror (errno));
    cleanup_temp_files (TRUE);
    exit (EXIT_FAILURE);
  }

  if (!preprocess_only_opt && 
      !nolibinc_opt && 
      include_lib && 
      lib_tmp_fn) {
    unlink (lib_tmp_fn);
  }

  if (*oldcache && file_exists (oldcache)) unlink (oldcache);

  if (!file_exists (ctpp_ofn)) {
    cleanup_temp_files (TRUE);
    exit (EXIT_FAILURE);
  }

  /*
   *  If the preprocessor encounters a fatal error, it
   *  will not produce an output file.
   */
  show_progress ();
  if (file_exists (ctpp_ofn)) {
    return SUCCESS;
  } else {
    return ERROR;
  }
}

int preprocess_to_output (char *fn) {

  if (outputfile_opt) {
    rename_file (ctpp_ofn, output_file);
  } else {
    rename_file (ctpp_ofn, preprocess_name (fn, FALSE));
  }

  return 0;
}

#define TMPFILE_PFX     "c."         /* From tempio.c. */

extern int error_line;               /* From lex.c. */

/*
 *  Add an ending line marker to ctalklib.  Returns the name
 *  of the temporary file.
 */

char *ctalk_lib_include (void) {

  FILE *outfile;
  char ctalklibpath[FILENAME_MAX];
  char lineinfo_buf[MAXMSG];
  char *libinput;
  static char fn[FILENAME_MAX];
  int r, r_w;
  struct stat statbuf;
#ifdef DJGPP
  int tmp_ext;
#endif

#ifdef DJGPP

  for (tmp_ext = 1; ; tmp_ext++) {
    sprintf (fn, "%s/c.%d.%#x", P_tmpdir, getpid (),  (void *)tmp_ext); 
    if (!file_exists (fn))
       break;
  }

#else

  sprintf (fn, "%s/c.XXXXXX", P_tmpdir);
  if ((r = mkstemp (fn)) == -1) {
    _error ("ctalk_lib_include (mkstemp): %s: %s.\n",
	    fn, strerror (errno));
  }

#endif

  sprintf (fn, "%s/c.XXXXXX", P_tmpdir);
  if ((r = mkstemp (fn)) == -1) {
    _error ("ctalk_lib_include (mkstemp): %s: %s.\n",
 	    fn, strerror (errno));
  }

  strcatx (ctalklibpath, classlibdir, "/ctalklib", NULL);

  stat (ctalklibpath, &statbuf);

  if ((libinput = (char *)__xalloc (statbuf.st_size + 1)) == NULL)
    _error ("%s: %s.", ctalklibpath, strerror (errno));

  r = read_file (libinput, ctalklibpath);

  if ((outfile = fopen (fn, FILE_WRITE_MODE)) == NULL)
    _error ("%s: %s.\n", fn, strerror (errno));

  if ((r_w = fwrite (libinput, sizeof (char), r, outfile)) != r)
    _error ("%s: %s.\n", fn, strerror (errno));

  if (!nolinemarker_opt)
    sprintf (lineinfo_buf, "# %d \"%s\" %d\n", 0,
	     __source_filename (), 1);

  r = strlen (lineinfo_buf);

  if ((r_w = fwrite (lineinfo_buf, sizeof (char), r, outfile)) != r)
    _error ("%s: %s.\n", fn, strerror (errno));

  fclose (outfile);
  __xfree (MEMADDR(libinput));
  return fn;
}

static void clear_method_cache (void) {
  DIR *cachedir;
  struct dirent *dentry;
  char dirname[FILENAME_MAX];
  char entname[FILENAME_MAX];

  init_user_home_path ();
  init_lib_cache_path ();
  strcpy (dirname, home_libcache_path ());

  if ((cachedir = opendir (dirname)) == NULL) {
    _warning ("%s: %s.\n", dirname, strerror (errno));
    return;
  }

  while ((dentry = readdir (cachedir)) != NULL) {

    if (!strcmp (dentry -> d_name, ".") ||
	!strcmp (dentry -> d_name, ".."))
      continue;

    strcatx (entname, dirname, "/", dentry -> d_name, NULL);
    (void)unlink (entname);

  }

  closedir (cachedir);

}
