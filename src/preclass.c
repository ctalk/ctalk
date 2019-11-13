/* $Id: preclass.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014-2017 Robert Kiesling, rk3314042@gmail.com.
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
 *  Read and write cached classes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"


HASHTAB classes_output;

extern I_PASS interpreter_pass;

void init_pre_classes (void) {
  _new_hash (&classes_output);
}

int pre_class_is_output (char *classname) {
  if (_hash_get (classes_output, classname))
    return TRUE;
  else
    return FALSE;
}

char *class_cache_path_name (char *classname) {
  static char path[FILENAME_MAX];

  strcatx (path, home_libcache_path (), "/", classname, ".class", NULL);

  return path;
}

void save_class_init_info (char *classname, char *superclassname) {

  FILE *fp;
  int length, bytes_written, r;
  char buf[MAXMSG];
  char cache_path[FILENAME_MAX];

  if (!class_deps_updated (classname))
    return;

  strcpy (cache_path, class_cache_path_name (classname));

  if ((fp = fopen (cache_path, FILE_WRITE_MODE)) == NULL) 
    _error ("save_class_init_info (fopen): %s: %s.\n",
	    cache_path, strerror (errno));

  if (!superclassname)
    length = strcatx (buf, "c:", classname, ":NULLSTR\n", NULL);
  else
    length = strcatx (buf, "c:", classname, ":", superclassname, "\n", NULL);

  if ((bytes_written = fwrite ((void *)buf, sizeof (char), length, fp))
	!= length) 
      _error ("save_class_init_info (fwrite (c:)): %s: %s.\n",
	      cache_path, strerror (errno));

    if ((r = fclose (fp)) != 0)
      _error ("save_class_init_info (fclose): %s: %s.\n",
	      cache_path, strerror (errno));

}


void save_instance_var_init_info (char *classname, char *varname,
				  char *varclass, char *init) {
  FILE *fp;
  int r, length, bytes_written;
  char cache_path[FILENAME_MAX];
  char buf[MAXMSG];

  if (!class_deps_updated (classname))
    return;

  strcpy (cache_path, class_cache_path_name (classname));

  if ((fp = fopen (cache_path, FILE_APPEND_MODE)) == NULL) 
    _error ("save_instance_var_init_info (fopen): %s: %s.\n",
	    cache_path, strerror (errno));

  /* Instance and class variable declarations require only the member 
     class and name - the rest is optional.  Fill in the defaults anyway
     to make later operations easier. */
  length = strcatx (buf, "i:", classname, ":", varname, ":",
	   ((*varclass) ? varclass : classname), ":",
	   ((*init) ? init : NULLSTR), "\n", NULL);

  if ((bytes_written = fwrite ((void *)buf, sizeof (char), length, fp))
      != length) 
    _error ("save_instance_var_init_info (fwrite (i:)): %s: %s.\n",
	    cache_path, strerror (errno));

  if ((r = fclose (fp)) != 0)
    _error ("save_instance_var_init_info (fclose): %s: %s.\n",
	    cache_path, strerror (errno));

}

void save_class_var_init_info (char *classname, char *varname,
			       char *varclass, char *init) {

  FILE *fp;
  int r, length, bytes_written;
  char cache_path[FILENAME_MAX];
  char buf[MAXMSG];

  if (!class_deps_updated (classname))
    return;

  strcpy (cache_path, class_cache_path_name (classname));

  if ((fp = fopen (cache_path, FILE_APPEND_MODE)) == NULL) 
    _error ("save_class_var_init_info (fopen): %s: %s.\n",
	    cache_path, strerror (errno));

  /* Instance and class variable declarations require only the member 
     class and name - the rest is optional.  Fill in the defaults anyway
     to make later operations easier. */
  length = strcatx (buf, "l:", classname, ":", varname, ":",
	   ((*varclass) ? varclass : classname), ":",
	   ((*init) ? init : NULLSTR), "\n", NULL);

  if ((bytes_written = fwrite ((void *)buf, sizeof (char), length, fp))
      != length) 
    _error ("save_instance_var_init_info (fwrite (i:)): %s: %s.\n",
	    cache_path, strerror (errno));

  if ((r = fclose (fp)) != 0)
    _error ("save_class__var_init_info (fclose): %s: %s.\n",
	    cache_path, strerror (errno));

}

typedef enum {
  var_type_instance_var,
  var_type_class_var
} VARTYPE;

struct _var_init_info {
  char classname[MAXLABEL],
    varname[MAXLABEL],
    varclass[MAXLABEL],
    init[MAXMSG];
  VARTYPE vartype;
};

struct _class_init_info {
  char classname[MAXLABEL],
    superclassname[MAXLABEL];
  struct _var_init_info vars[MAXARGS];
} class_init_info;
int var_ptr = 0;


void fill_in_class_info (char *cache_fn) {

  struct stat statbuf;
  char *lptr, *rptr, *rptr2, *eptr, *buf;

  stat (cache_fn, &statbuf);

  if ((buf = (char *)malloc (statbuf.st_size + 1)) == NULL)
    _error ("fill_in_init_info (malloc): %s.\n", strerror (errno));

  memset (buf, 0, statbuf.st_size + 1);

  (void)read_file (buf, cache_fn);

  memset ((void *)&class_init_info, 0, sizeof (struct _class_init_info));
  
  var_ptr = 0;

  eptr = strchr (buf, '\0');
  lptr = buf;
  rptr2 = NULL;

  do {
    
    switch (*lptr)
      {
      case 'c':
	if ((rptr = strchr (lptr, ':')) != NULL) {
	  ++rptr;
	  if ((rptr2 = strchr (rptr, ':')) != NULL) {
	    strncpy (class_init_info.classname, 
		     rptr, rptr2 - rptr);
	    rptr = rptr2 + 1;
	  }
	}

	if ((rptr2 = strchr (rptr, '\n')) != NULL) {
	  strncpy (class_init_info.superclassname, 
		   rptr, rptr2 - rptr);
	  rptr = rptr2 + 1;
	}

	lptr = rptr2 + 1;

	break;
      case 'i':

	if ((rptr = strchr (lptr, ':')) != NULL) {
	  ++rptr;
	  if ((rptr2 = strchr (rptr, ':')) != NULL) {
	    strncpy (class_init_info.vars[var_ptr].classname, 
		     rptr, rptr2 - rptr);
	    rptr = rptr2 + 1;
	  }
	}

	if ((rptr2 = strchr (rptr, ':')) != NULL) {
	  strncpy (class_init_info.vars[var_ptr].varname, 
		   rptr, rptr2 - rptr);
	  rptr = rptr2 + 1;
	}

	
	if ((rptr2 = strchr (rptr, ':')) != NULL) {
	  strncpy (class_init_info.vars[var_ptr].varclass, 
		   rptr, rptr2 - rptr);
	  rptr = rptr2 + 1;
	}

	if ((rptr2 = strchr (rptr, '\n')) != NULL) {
	  strncpy (class_init_info.vars[var_ptr].init, 
		     rptr, rptr2 - rptr);
	  rptr = rptr2 + 1;
	}

	class_init_info.vars[var_ptr].vartype = var_type_instance_var;

	lptr = rptr2 + 1;

	++var_ptr;

	break;

      case 'l':

	if ((rptr = strchr (lptr, ':')) != NULL) {
	  ++rptr;
	  if ((rptr2 = strchr (rptr, ':')) != NULL) {
	    strncpy (class_init_info.vars[var_ptr].classname, 
		     rptr, rptr2 - rptr);
	    rptr = rptr2 + 1;
	  }
	}

	if ((rptr2 = strchr (rptr, ':')) != NULL) {
	  strncpy (class_init_info.vars[var_ptr].varname, 
		   rptr, rptr2 - rptr);
	  rptr = rptr2 + 1;
	}

	
	if ((rptr2 = strchr (rptr, ':')) != NULL) {
	  strncpy (class_init_info.vars[var_ptr].varclass, 
		   rptr, rptr2 - rptr);
	  rptr = rptr2 + 1;
	}

	if ((rptr2 = strchr (rptr, '\n')) != NULL) {
	  strncpy (class_init_info.vars[var_ptr].init, 
		     rptr, rptr2 - rptr);
	  rptr = rptr2 + 1;
	}

	class_init_info.vars[var_ptr].vartype = var_type_class_var;

	lptr = rptr2 + 1;

	++var_ptr;

	break;
      }

  } while (lptr < eptr);

  __xfree (MEMADDR(buf));
}

int preload_method_prototypes (char *p_lib_fn, CLASSLIB *c);

void generate_init_from_cache (void) {

  int i;
  struct stat statbuf;
  char cache_fn[FILENAME_MAX];
  char filebuf[MAXMSG];
  char *path;
  char *buf;
  CLASSLIB cl;
  METHOD_PROTO *mp, *mp2;

  generate_primitive_class_definition_call_from_init 
    (class_init_info.classname, class_init_info.superclassname);

  for (i = 0; i < var_ptr; i++) {

    switch (class_init_info.vars[i].vartype)
      {
      case var_type_instance_var:
	generate_define_instance_variable_call 
	  (class_init_info.vars[i].classname, 
	   class_init_info.vars[i].varname, 
	   class_init_info.vars[i].varclass, 
	   class_init_info.vars[i].init);
	break;
      case var_type_class_var:
	generate_define_class_variable_call 
	  (class_init_info.vars[i].classname, 
	   class_init_info.vars[i].varname, 
	   class_init_info.vars[i].varclass, 
	   class_init_info.vars[i].init);
	break;
      }

  }

  memset (&cl, 0, sizeof (CLASSLIB));

  if ((path = find_library_include (class_init_info.classname, FALSE))
      == NULL)
    return;

  preload_method_prototypes (path, &cl);

  for (mp = cl.proto; mp; mp = mp -> next) {
    strcatx (cache_fn, home_libcache_path (), "/", mp -> src, NULL);

    if (!stat (cache_fn, &statbuf)) {
      if (statbuf.st_size >= MAXLABEL) {
	if ((buf = (char *)__xalloc (statbuf.st_size + 1)) == NULL)
	  _error ("generate_init_from_cache (__xalloc): %s.\n", 
		  strerror (errno));
	(void)read_file (buf, cache_fn);
	output_pre_method (mp -> src, buf);
	  
	__xfree (MEMADDR(buf));
      } else {
	(void)read_file (filebuf, cache_fn);
	output_pre_method (mp -> src, filebuf);
      }
    }
  }

  if (cl.proto) {
    mp = cl.proto_head;
    while (mp != cl.proto) {
      mp2 = mp -> prev;
      __xfree (MEMADDR(mp -> src));
      __xfree (MEMADDR(mp));
      mp = mp2;
    }
    __xfree (MEMADDR(cl.proto -> src));
    __xfree (MEMADDR(cl.proto));
  }

}

void load_cached_class (char *classname) {
  
  I_PASS old_pass;

  char cache_fn[FILENAME_MAX];

  strcpy (cache_fn, class_cache_path_name (classname));

  if (!file_exists (cache_fn)) {

    old_pass = interpreter_pass;

    interpreter_pass = parsing_pass;

    library_search (classname, FALSE);

    interpreter_pass = old_pass;

  } else {

    if (!pre_class_is_output (classname)) {

      /* 
       * Put this first.  
       * It prevents this function from calling
       * generate_init_from_cache (), which calls  output_pre_method, 
       * which calls this function.... 
       */
      _hash_put (classes_output, classname, classname);

      if (!is_cached_class (classname) && 
	  !is_pending_class (classname)) {

	fill_in_class_info (cache_fn);

	generate_init_from_cache ();

      }

    }

  }

}
