/* $Id: i_opt.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

/*
 *  Handle the include options: -imacros, -include, -idirafter, 
 *  -iprefix, and -iwithprefix.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ctpp.h"

extern char source_file[FILENAME_MAX];  /* Declared in rtinfo.c. */
extern int move_includes_opt;

/* 
 *   Option to specify the compiler's system include directory, if the 
 *   program can't determine it.  For GCC on UNIX, the directory should 
 *   be:
 *   <prefix>/lib/gcc-lib/<target>/<version>/include/
 *
 *   On older SunOS systems, GCC and its libraries might be under 
 *   /opt.  
 */
int systeminc_opt = FALSE;
char *systeminc_dirs[MAXUSERDIRS]; 
int n_system_inc_dirs = 0;

/*
 *  Files included with -imacros <file> option.  There is
 *  no option switch - the files are preprocessed before
 *  the input file.
 */
static char imacros_files[MAXARGS][FILENAME_MAX];
static int imacros_files_idx = -1;

/*
 *  Files included with -include <file> option.  Processed
 *  after imacros_files and before input file.
 */
static char include_files[MAXARGS][FILENAME_MAX];
static int include_files_idx = -1;

/*
 *  Directories in the secondary include path.
 */
char include_dirs_after[MAXARGS][FILENAME_MAX];
int include_dirs_after_idx = -1;

/*
 *  Directory prefix with -iprefix option, used with
 *  subsequent -iwithprefix options.  "./" is the
 *  default prefix.
 */
static char include_dir_prefix[FILENAME_MAX] = ".";

extern MESSAGE *p_messages[P_MESSAGES+1]; /* Preprocess message stack.   */
extern int p_message_ptr;                 /* Preprocess stack pointer.   */

int preprocess_stack_start = -1;          /* p_message_ptr when starting */
                                          /* to -include files.          */

void define_imacros (void) {

  int i, j, start, end;
#ifndef HAVE_OFF_T
  int file_size;
#else
  off_t file_size;
#endif
  char *input;
  MESSAGE *m;
  VAL result;

  for (i = 0; i <= imacros_files_idx; i++) {

    if ((file_size = read_file (&input, imacros_files[i])) < 0) {
      fprintf (stderr, "%s: %s.\n", imacros_files[i], strerror (errno));
      exit (1);
    }

    start = stack_start (p_messages);
    end = tokenize_reuse (p_message_push, input);

    memset ((void *)&result, 0, sizeof (VAL));
    result.__type = 1;
    (void)macro_parse (p_messages, start, &end, &result);
    handle_preprocess_exception (p_messages, end, start, end);

    for (j = start; j >= end; j--) {
      m = p_message_pop ();
      if (m && IS_MESSAGE (m))
	reuse_message (m);
    }

    free (input);
  }

}

void process_include_opt_files (void) {

  int i, start, end;
  int line_marker_length;
#ifndef HAVE_OFF_T
  int file_size;
#else
  off_t file_size;
#endif
  VAL result;
  char *input;
  char save_fn[FILENAME_MAX];

  preprocess_stack_start = get_stack_top (p_messages);

  for (i = 0; i <= include_files_idx; i++) {
    
    if ((file_size = read_file (&input, include_files[i])) < 0) {
      fprintf (stderr, "%s: %s.\n", include_files[i], strerror (errno));
      exit (EXIT_FAILURE);
    }

    strcpy (save_fn, source_file);
    strcpy (source_file, include_files[i]);

    start = get_stack_top (p_messages);
    end = tokenize_reuse (p_message_push, input);

    if (move_includes_opt)
      start = move_includes (p_messages, start, &end);

    line_marker_length = line_info (start, include_files[i], 1, 1);
    adj_include_stack (-line_marker_length);
    end -= line_marker_length;

    memset ((void *)&result, 0, sizeof (VAL));
    result.__type = 1;

    (void)macro_parse (p_messages, start, &end, &result);
    handle_preprocess_exception (p_messages, end, start, end);

    free (input);
    
    strcpy (source_file, save_fn);

  }

}

int imacro_filename (char **args, int idx, int cnt) {

  if (args[idx + 1][0] == '-')
    help ();

  if ((idx + 1) == cnt)
    help ();

  strcpy (imacros_files[++imacros_files_idx], args[idx + 1]);

  return 1;
}

int include_filename (char **args, int idx, int cnt) {

  if (args[idx + 1][0] == '-')
    help ();

  if ((idx + 1) == cnt)
    help ();

  strcpy (include_files[++include_files_idx], args[idx + 1]);

  return 1;
}

int include_dirafter_name (char **args, int idx, int cnt) {

  if (args[idx + 1][0] == '-')
    help ();

  if ((idx + 1) == cnt)
    help ();

  strcpy (include_dirs_after[++include_dirs_after_idx], args[idx + 1]);

  return 1;
}

int include_dirafter_name_prefix (char **args, int idx, int cnt) {

  if (args[idx + 1][0] == '-')
    help ();

  if ((idx + 1) == cnt)
    help ();

  sprintf (include_dirs_after[++include_dirs_after_idx], "%s/%s",
	   include_dir_prefix, args[idx + 1]);

  return 1;
}

int include_prefixafter_name (char **args, int idx, int cnt) {

  if (args[idx + 1][0] == '-')
    help ();

  if ((idx + 1) == cnt)
    help ();

  strcpy (include_dir_prefix, args[idx + 1]);

  /*
   *  Remove a trailing slash if present.
   *  This makes later operations much simpler.
   */
  if (include_dir_prefix[strlen (include_dir_prefix) - 1] == '/')
    include_dir_prefix[strlen (include_dir_prefix) - 1] = 0;

  return 1;
}

int include_systemdir_name (char **args, int idx, int cnt) {

  if ((args[idx + 1][0] == '-') || (idx + 1) == cnt)
    help ();

  systeminc_dirs[n_system_inc_dirs++] = strdup  (args[idx + 1]);

  if ((!file_exists (systeminc_dirs[n_system_inc_dirs - 1]))||
      !is_dir (systeminc_dirs[n_system_inc_dirs - 1])) {
    printf ("%s: %s.", systeminc_dirs[n_system_inc_dirs-1],
	    strerror (errno));
    exit (1);
  }

  return 1;
}
