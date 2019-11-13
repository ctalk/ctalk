/* $Id: hash.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011  Robert Kiesling, rk3314042@gmail.com.
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ctpp.h"
#include "phash.h"

HASHTAB macrodefs;
HASHTAB ansisymbols;

extern int warnunused_opt;     /* Defined in rtinfo.c.     */

static int symbol_line = 0;;
static int symbol_column = 0;
static char symbol_source_file[FILENAME_MAX];

void _hash_symbol_line (int l) { symbol_line = l; }
void _hash_symbol_column (int c) {symbol_column = c; }
void _hash_symbol_source_file (char *f) {
  strcpy (symbol_source_file, f);
}


void build_hashes (void) {
  int i;
  if ((macrodefs = (HASHTAB)__xalloc ((int)(N_HASH_BUCKETS + 1) * 
				   sizeof (struct _hashbucket *)))
      == NULL)
    _error ("_new_hash (): %s.\n", strerror (errno));

  for (i = 0; i < N_HASH_BUCKETS; i++) {
    (macrodefs)[i] = (HASHBUCKET *)__xalloc (sizeof (struct _hashbucket)); 
    macrodefs[i]->sig = HASH_SIG;
  }

  if ((ansisymbols = (HASHTAB)__xalloc ((int)(N_HASH_BUCKETS + 1) * 
				   sizeof (struct _hashbucket *)))
      == NULL)
    _error ("_new_hash (): %s.\n", strerror (errno));

  for (i = 0; i < N_HASH_BUCKETS; i++) {
    (ansisymbols)[i] = (HASHBUCKET *)__xalloc (sizeof (struct _hashbucket)); 
    ansisymbols[i] -> sig = HASH_SIG;
  }
}

HLIST *_new_hlist (void) {
  static HLIST *l;

  if ((l = (HLIST *)__xalloc (sizeof (struct _h_list))) == NULL)
    _error ("_new_hlist (): %s.\n", strerror (errno));

  l -> sig = HLIST_SIG;
 
  return l;
}

static int _hash_key (char *s) {

  char *q;
  long long int sum;

  for (q = s, sum = 0; *q; q++)
    sum += (long long int) *q;

  return (int)sum % N_HASH_BUCKETS;
}

void _hash_put (HASHTAB h, void *e, char *s_key) {

  HLIST *l;
  int key;

  key = _hash_key (s_key);
  l = _new_hlist ();
  strcpy (l -> s_key, s_key);
  l -> data = e;

  if (warnunused_opt) {
    l -> error_line = symbol_line;
    l -> error_column = symbol_column;
    strcpy (l -> source_file, symbol_source_file);
  }

  if (!h[key] -> mbrs_head) {
    h[key] -> mbrs_head = h[key] -> mbrs = l;
  } else {
    h[key] -> mbrs_head -> next = l;
    l -> prev = h[key] -> mbrs_head;
    h[key] -> mbrs_head = l;
  }

}

void *_hash_get (HASHTAB h, char *s_key) {

  int key;
  HLIST *l;

  key = _hash_key (s_key);

  for (l = h[key] -> mbrs; l; l = l -> next) {
    if (!basename_cmp (l -> s_key, s_key)) {
      ++(l -> hits);
      return l -> data;
    }
  }
  return NULL;
}

void *_hash_remove (HASHTAB h, char *s_key) {
  int key;
  HLIST *l, *n, *p, *tmp;
  DEFINITION *d, *d_warn;

  key = _hash_key (s_key);

  for (l = h[key] -> mbrs; l; l = l -> next) {
    if (!basename_cmp (l -> s_key, s_key)) {

      if (warnunused_opt && !l -> hits) {
	d_warn = (DEFINITION *) l -> data; 
	_warning ("%s:%d:warning: Unused macro definition %s.\n",
		  l -> source_file, l -> error_line, d_warn -> name);
      }

      tmp = l;

      if (tmp == h[key] -> mbrs) {  /* Top of list. */
	h[key] -> mbrs = 
	  (tmp && tmp -> next && IS_HLIST(tmp -> next)) ? tmp -> next : NULL;
	if (h[key] -> mbrs_head == tmp)
	  h[key] -> mbrs_head = NULL;
      } else {  /* End of list. */
	if (tmp == h[key] -> mbrs_head) { /* ...and not h[key] -> mbrs. */
	  h[key] -> mbrs_head = tmp -> prev;
	  h[key] -> mbrs_head -> next = NULL;
	} else { /* Middle of list. */
	  n = tmp -> next;
	  p = tmp -> prev;
	  n -> prev = p;
	  p -> next = n;
	} /* if (tmp == h[key] -> mbrs_head) */
      } /* if (tmp == h[key] -> mbrs) */
      d = tmp -> data;
      free (tmp);
      return d;
    } /* if (!basename_cmp (l -> s_key, s_key)) */
  } /* for (l = h[key] -> mbrs; l; l = l -> next) */
  return NULL;
}

static int l_key;
static HLIST *l_hlist;
static int last_key;

HLIST *_hash_first (HASHTAB *h) {

  l_key = last_key = 0;
  l_hlist = NULL;

  for (l_key = 0; l_key <N_HASH_BUCKETS; l_key++) {
    if ((l_hlist = (*h)[l_key] -> mbrs) != NULL) {
      last_key = l_key;
      return l_hlist;
    }
  }
  return NULL;
}

HLIST *_hash_next (HASHTAB *h) {

  HLIST *t;

  for (l_key = last_key; l_key < N_HASH_BUCKETS; l_key++) {

    if ((*h)[l_key] -> mbrs) {
      if (l_key == last_key) {
	for (t = l_hlist -> next; t; t = t -> next) {
	  l_hlist = t;
	  last_key = l_key;
	  return l_hlist;
	}
      } else {
	l_hlist = (*h)[l_key] -> mbrs;
	last_key = l_key;
	return l_hlist;
      }
    }
  }

  return NULL;
}
