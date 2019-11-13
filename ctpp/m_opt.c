/* $Id: m_opt.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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
 *   -M options.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "ctpp.h"

extern char source_file[FILENAME_MAX];  /* Declared in rtinfo.c. */
extern char output_file[FILENAME_MAX];  /* Declared in main.c.   */

extern int makerule_opts;           /* Declared in lib/rtinfo.c. -M options */

static char make_target_file[FILENAME_MAX + MAKE_TARGET_EXT_LEN];
static char rules_filename[FILENAME_MAX];   /* File to output the rule.  */

static char make_target_headers[512][FILENAME_MAX];
static int target_headers_idx = -1;

static char user_targets[512][FILENAME_MAX];
static int user_target_pointer = 0;


int add_user_target (char *s) {
  strcpy (user_targets[user_target_pointer++], s);
  return user_target_pointer;
}

int make_target (void) {

  char *ext_ptr;

  if (!strcmp (source_file, "-")) {
    fprintf (stderr, "Error: The make rule requires a source file name.\n");
    makerule_opts = 0;
    return ERROR;
  }
    
  if (makerule_opts & MAKERULEUSERTARGET) {
    int i;
    *make_target_file = 0;
    for (i = 0; i < user_target_pointer; i++) {
      strcat (make_target_file, user_targets[i]);
      strcat (make_target_file, " ");
    }
  } else {
    if ((ext_ptr = rindex (source_file, '.')) == NULL) {
      sprintf (make_target_file, "%s%s", source_file, MAKE_TARGET_EXT);
    } else {
      substrcpy (make_target_file, source_file, 0, ext_ptr - source_file);
      strcat (make_target_file, MAKE_TARGET_EXT);
    }
  }

  return SUCCESS;
}

void include_dependency (char *path) {
  strcpy (make_target_headers[++target_headers_idx], path);
}

int makerule_filename (char **a, int idx, int cnt) {

  char *name_ptr;

  if (!strcmp (a[idx], "-MD") ||
      !strcmp (a[idx], "-MMD")) {
    /*
     *  The filename must be the next argument.
     */
    if (a[idx + 1][0] == '-') {
      printf ("ctpp: argument syntax error.\n");
      help ();
    }
    strcpy (rules_filename, a[idx + 1]);
    return 1;
  } else {
    /*
     *  The filename must be part of this argument.
     */
    if (!strncmp (a[idx], "-MMD", 4)) {
      name_ptr = &a[idx][4];
      strcpy (rules_filename, name_ptr);
    } else {
      if (!strncmp (a[idx], "-MD", 3)) {
	name_ptr = &a[idx][3];
	strcpy (rules_filename, name_ptr);
      }
    }
  }

  return 0;
}

void check_rules_file (void) {
  if (strcmp (source_file, "-")) {
    if (!strcmp (source_file, rules_filename)) {
      strcat (rules_filename, ".d");
      printf("ctpp: Warning: Make rule file will be renamed to %s.\n",
	     rules_filename);
    }
  }
  if (strcmp (output_file, "-")) {
    if (!strcmp (output_file, rules_filename)) {
      strcat (rules_filename, ".d");
      printf("ctpp: Warning: Make rule file will be renamed to %s.\n",
	     rules_filename);
    }
  }
}

int output_make_rule (void) {

  int i, r,
    handle_n;
  FILE *f = NULL;        /* Avoid a warning. */
  char s[FILENAME_MAX * 2 + 4];

  if (makerule_opts & MAKERULETOFILE) {
    if ((f = fopen (rules_filename, "w")) == NULL)
      fprintf (stdout, "%s: %s.\n", rules_filename, strerror (errno));
    handle_n = fileno (f);
  } else {
    handle_n = dup (fileno (stdout));
  }

  sprintf (s, "%s: %s ", make_target_file, source_file);
  if ((r = write (handle_n, (void *)s, sizeof (char) * strlen (s)))
      < 0) {
    printf ("output_make_rule: %s.\n", strerror (errno));
    fclose (f);
    return ERROR;
  }

  for (i = 0; i <= target_headers_idx; i++) {
    sprintf (s, "\\\n\t%s ", make_target_headers[i]);
    if ((r = write (handle_n, (void *)s, sizeof (char) * strlen (s)))
	< 0) {
      printf ("output_make_rule: %s.\n", strerror (errno));
      fclose (f);
      return ERROR;
    }
  }

  strcpy (s, "\n");
  if ((r = write (handle_n, (void *)s, sizeof (char) * strlen (s)))
      < 0) {
    printf ("output_make_rule: %s.\n", strerror (errno));
    fclose (f);
    return ERROR;
  }


  if (makerule_opts & MAKERULETOFILE) {
    fclose (f);
  }
  return SUCCESS;
}
