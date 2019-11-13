/* $Id: chash.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2015, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "chash.h"

void _new_hash (HASHTAB *h) {
  int i;

  if ((*h = (HASHBUCKET **)calloc (N_HASH_BUCKETS + 1, sizeof (struct _hashbucket *)))
      == NULL)
    _error ("_new_hash (): %s.\n", strerror (errno));

  for (i = 0; i <= N_HASH_BUCKETS; i++) {
    (*h)[i] = calloc (1, sizeof (struct _hashbucket));
  }
}

HLIST *_new_hlist (void) {
  HLIST *l;

  if ((l = (HLIST *)__xalloc (sizeof (struct _h_list))) == NULL)
    _error ("_new_hlist (): %s.\n", strerror (errno));

  return l;
}

static int _hash_key (const char *s) {

  char *q;
  long long int sum;

  for (q = (char *)s, sum = 0; *q; q++)
    sum += (long long int) *q;

  return (int)(sum & N_HASH_BUCKETS);
}

void _hash_put (HASHTAB h, void *e, char *s_key) {

  HLIST *l;
  int key;

  key = _hash_key (s_key);
  l = _new_hlist ();
  strcpy (l -> s_key, s_key);
  l -> data = e;

  if (!h[key] -> mbrs_head) {
    h[key] -> mbrs_head = h[key] -> mbrs = l;
  } else {
    h[key] -> mbrs_head -> next = l;
    l -> prev = h[key] -> mbrs_head;
    h[key] -> mbrs_head = l;
  }

}

void *_hash_get (HASHTAB h, const char *s_key) {

  int key;
  HLIST *l;

  if (!h)
    return NULL;

  if ((key = _hash_key (s_key)) < 0) /* Maybe we have international chars
					in s_key. */
    return NULL;

  if (h[key] -> mbrs == NULL) {
    return NULL;
  }

  for (l = h[key] -> mbrs; l; l = l -> next) {
    if (str_eq (l -> s_key, (char *)s_key)) {
      return l -> data;
    }
  }
  return NULL;
}

void *_hash_remove (HASHTAB h, const char *s_key) {
  int key;
  HLIST *l, *n, *p, *tmp;
  DEFINITION *d;

  key = _hash_key (s_key);

  for (l = h[key] -> mbrs; l; l = l -> next) {
    if (str_eq (l -> s_key, (char *)s_key)) {

      tmp = l;

      if (tmp == h[key] -> mbrs) {  /* Top of list. */
	h[key] -> mbrs = (tmp && tmp -> next) ? tmp -> next : NULL;
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
      __xfree (MEMADDR(tmp));
      return d;
    } /* if (!basename_cmp (l -> s_key, s_key)) */
  } /* for (l = h[key] -> mbrs; l; l = l -> next) */
  return NULL;
}

static int l_key;
static HLIST *l_hlist;
static int last_key;

void *_hash_first (HASHTAB h) {

  l_key = last_key = 0;
  l_hlist = NULL;

  for (l_key = 0; l_key <N_HASH_BUCKETS; l_key++) {
    if ((l_hlist = (h)[l_key] -> mbrs) != NULL) {
      last_key = l_key;
      return l_hlist -> data;
    }
  }
  return NULL;
}

void *_hash_next (HASHTAB h) {

  HLIST *t;

  for (l_key = last_key; l_key < N_HASH_BUCKETS; l_key++) {

    if ((h)[l_key] -> mbrs) {
      if (l_key == last_key) {
	l_hlist = l_hlist -> next;
	if (l_hlist)
	  return l_hlist -> data;
      } else {
	l_hlist = (h)[l_key] -> mbrs;
	last_key = l_key;
	return l_hlist -> data;
      }
    }
  }

  return NULL;
}

static int _hash_first_lookup = FALSE;

void _hash_all_initialize (void) {
  _hash_first_lookup = TRUE;
}

void *_hash_all (HASHTAB h) {

  if (_hash_first_lookup) {
    _hash_first_lookup = FALSE;
    return _hash_first (h);
  } else {
    return _hash_next (h);
  }
  return NULL;
}

/* 
 *  Anything that calls this function should have placed a unique
 *  copy of the data in the hash, because this function deletes
 *  everything.
 */
void _delete_hash (HASHTAB ht) {
  int i;
  HLIST *h, *h_prev;
  for (i = 0; i < N_HASH_BUCKETS; i++) {
    if (ht[i] -> mbrs != NULL) {
      h = ht[i] -> mbrs_head;
      while (h != ht[i] -> mbrs) {
	h_prev = h -> prev;
	__xfree (MEMADDR(h -> data));
	h = h_prev;
      }
      __xfree (MEMADDR(h -> data));
      __xfree (MEMADDR(h));
      __xfree (MEMADDR(ht[i]));
    }
  }
}
