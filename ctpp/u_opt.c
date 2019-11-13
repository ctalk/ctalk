/* $Id: u_opt.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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
 *  Handle -U and -undef options.
 */

#include <string.h>
#include <stdlib.h>
#include "ctpp.h"

extern DEFINITION *macro_symbols;  /* Defined macro list.              */
extern DEFINITION *last_symbol;    /* List pointer.                    */

static char undef_symbols[MAXARGS][MAXLABEL];
static int undef_symbols_idx = -1;

static void free_macro_args (MACRO_ARG **args) {
  int i;
  for (i = 0; args[i]; i++)
    free (args[i]);
}

extern HASHTAB macrodefs;             /* Declared in hash.c.           */

static void delete_builtin_symbol (char *name) {
  DEFINITION *t;

  if ((t = (DEFINITION *)_hash_remove (macrodefs, mbasename(name))) != NULL) {
    if (t -> m_args[0]) free_macro_args (t -> m_args);
    free (t);
  }
}

/*
 *  Undefine builtin macros.
 */
void undefine_builtin_macros (void) {

  HLIST *l;
  DEFINITION *d;

  if ((l = _hash_first (&macrodefs)) != NULL) {
    d = (DEFINITION *) l -> data;
#ifdef __GNUC__
    if (is_builtin_symbol (d -> name) ||
	is_gnuc_symbol (d -> name))
#else
    if (is_builtin_symbol (d -> name))
#endif
	delete_builtin_symbol (d -> name);
  }

  while ((l = _hash_next (&macrodefs)) != NULL) {
    d = (DEFINITION *) l -> data;
#ifdef __GNUC__
    if (is_builtin_symbol (d -> name) ||
	is_gnuc_symbol (d -> name)) {
#else
    if (is_builtin_symbol (d -> name)) {
#endif
      delete_builtin_symbol (d -> name);
    } else {
      if (!strcmp (d -> name, "__CTPP_DEFINESONLY_OPT__"))
	delete_builtin_symbol (d -> name);
    }

#if defined (__GNUC__) && defined(__sparc__) && defined(__svr4__)
    if (!strcmp (d -> name, "__va_list"))
      delete_builtin_symbol (d -> name);
#endif
  }

}

int undefine_macro_opt (char **args, int idx, int cnt) {

  int i = 0;

  if (strlen (args[idx]) == 2) {
    /*
     * Form -U <name>
     */
    if (args[idx + 1][0] == '-') {
      help ();
    } else {
      strcpy (undef_symbols[++undef_symbols_idx], args[idx + 1]);
      i = 1;
    }
  } else {
    strcpy (undef_symbols[++undef_symbols_idx], &args[idx][2]);
    i = 0;
  }

  return i;
}

/* Declared in preprocess.c. */
extern MESSAGE *t_messages[P_MESSAGES +1];/* Temporary stack for include and */
extern int t_message_ptr;             /* macro tokenization.                 */

/*
 *  Undefine a symbol defined with a -U option before processing
 *  the main input file.
 */

void perform_undef_macro_opt (void) {

  int i, j, start, end;
  char s[MAXMSG];

  for (i = 0; i <= undef_symbols_idx; i++) {
    sprintf (s, "#undef %s\n", undef_symbols[i]);
    start = stack_start (t_messages);
    end = tokenize_reuse (t_message_push, s);
    undefine_symbol (t_messages, start - 1);
    for (j = end; j <= start; j++)
      reuse_message (t_message_pop ());
  }
}
