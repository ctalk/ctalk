/* $Id: lsort.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2018, 2019 Robert Kiesling, rk3314042@gmail.com.
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

#include <stdbool.h>
#include <string.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

/* we really, really want strcmp here */
#undef strcmp

/* from classes/ctalkdefs.h */
#define __LIST_HEAD(__o) ((__o)->instancevars->next)

static void swapvals (OBJECT *key_1, OBJECT *key_2) {
  char tmpbuf[MAXLABEL];
  memcpy (tmpbuf, key_1 -> instancevars -> __o_value, sizeof (uintptr_t));
  memcpy (key_1 -> instancevars -> __o_value,
	  key_2 -> instancevars -> __o_value, sizeof (uintptr_t));
  memcpy (key_2 -> instancevars -> __o_value, tmpbuf, sizeof (uintptr_t));
}

/* the mechanics of determining earlier/later members of lists make
   this as fast as any other type of sort for small or medium
   collections... */
static void sort (OBJECT *list_head, bool sortdescending) {
  OBJECT *key_j, *key_j_next, *mbr_j, *mbr_j_next;
  bool swaps;
  while (1) {
    swaps = false;
    for (key_j = list_head; key_j && key_j -> next; key_j = key_j -> next) {
      mbr_j = *(OBJECT **) (key_j -> instancevars -> __o_value);
      mbr_j_next = *(OBJECT **)(key_j -> next -> instancevars -> __o_value);
      if (sortdescending) {
	if (strcmp (mbr_j -> instancevars -> __o_value,
		    mbr_j_next -> instancevars -> __o_value) < 0) {
	  swapvals (key_j, key_j -> next);
	  swaps = true;
	}
      } else {
	if (strcmp (mbr_j -> instancevars -> __o_value,
		    mbr_j_next -> instancevars -> __o_value) > 0) {
	  swapvals (key_j, key_j -> next);
	  swaps = true;
	}
      }
    }
    if (swaps == false)
      return;
  }
}

static void sort_by_name (OBJECT *list_head, bool sortdescending) {
  OBJECT *key_j, *key_j_next, *mbr_j, *mbr_j_next;
  bool swaps;
  while (1) {
    swaps = false;
    for (key_j = list_head; key_j && key_j -> next; key_j = key_j -> next) {
      mbr_j = *(OBJECT **)(key_j -> instancevars -> __o_value);
      mbr_j_next = *(OBJECT **)(key_j -> next -> instancevars -> __o_value);
      if (sortdescending) {
	if (strcmp (mbr_j -> __o_name, mbr_j_next -> __o_name) < 0) {
	  swapvals (key_j, key_j -> next);
	  swaps = true;
	}
      } else {
	if (strcmp (mbr_j -> __o_name, mbr_j_next -> __o_name) > 0) {
	  swapvals (key_j, key_j -> next);
	  swaps = true;
	}
      }
    }
    if (swaps == false)
      return;
  }
}

int __ctalkSort (OBJECT *collection, bool sortdescending) {
  OBJECT *list_head;
  if (collection -> attrs & OBJECT_IS_VALUE_VAR)
    list_head = collection -> next;
  else
    list_head = __LIST_HEAD(collection);
  sort (list_head, sortdescending);
  return SUCCESS;
}

int __ctalkSortByName (OBJECT *collection, bool sortdescending) {
  OBJECT *list_head;
  if (collection -> attrs & OBJECT_IS_VALUE_VAR)
    list_head = collection -> next;
  else
    list_head = __LIST_HEAD(collection);
  sort_by_name (list_head, sortdescending);
  return SUCCESS;
}
