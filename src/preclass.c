/* $Id: preclass.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

typedef struct _method_line_info {
  char selector[MAXLABEL];
  int line;
  bool prefix, varargs, class_method;
  int n_params;
  struct _method_line_info *next;
} MLI;

typedef struct _class_line_info {
  char class[MAXLABEL];
  MLI *methods, *methods_head;
  struct _class_line_info *next;
} CLSLI;

CLSLI *method_line_info = NULL, *method_line_info_head = NULL;


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

#define INST_KEYWORD_LEN  14
#define CLASS_KEYWORD_LEN 11

/***/
static void pre_class_ln_parse (FILE *fw, char *classname, char *classbuf,
				int *lineno) {
  int i, i_2, i_3, n_commas, classnamelength, decl_start_line;
  int selector_start, selector_end, paramlist_start, paramlist_end,
    n_parens, n_params = 0;
  char c, lastprint = '\0',
    entrybuf[MAXLABEL], selectorbuf[MAXLABEL], paramlist_buf[MAXMSG],
    ln_buf[64], n_param_buf[64];
  bool class_method = false;
  bool varargs = false;
  bool prefix = false;

  if (method_line_info == NULL) {
    method_line_info = method_line_info_head =
      __xalloc (sizeof (struct _class_line_info));
  } else {
    method_line_info_head -> next =
      __xalloc (sizeof (struct _class_line_info));
    method_line_info_head = method_line_info_head -> next;
  }
  strcpy (method_line_info_head -> class, classname);
  
  *lineno = 1;
  classnamelength = strlen (classname);
  for (i = 0; classbuf[i]; i++) {
    c = classbuf[i];
    if (c == '\n') {
      ++(*lineno);
    } else if (c == ' ' || c == '\t') {
      continue;
    } else if (c == *classname) {
      if (!strncmp (&classbuf[i], classname, classnamelength)) {
	if (lastprint == ';' || lastprint == '}' || lastprint == '/') {
	  /* ie, preceded by an expression terminator, or method body end, 
	     or comment end */
	  i_2 = i + classnamelength + 1;
	  while (isspace ((int)classbuf[i_2]))
	    ++i_2;
	  
	  if (!strncmp (&classbuf[i_2], "instanceMethod", INST_KEYWORD_LEN)) {
	    i_2 += INST_KEYWORD_LEN;
	    class_method = false;
	  } else if (!strncmp (&classbuf[i_2], "instanceMethod",
			       CLASS_KEYWORD_LEN)) {
	    i_2 += CLASS_KEYWORD_LEN;
	    class_method = true;
	  } else {
	    i += classnamelength;
	    continue;
	  }

	  decl_start_line = *lineno;
	  while (isspace ((int)classbuf[i_2]))
	    ++i_2;

	  selector_start = i_2;

	  while (!isspace ((int)classbuf[i_2]))
	    ++i_2;

	  selector_end = i_2;
	  
	  memset (selectorbuf, 0, MAXLABEL);
	  strncpy (selectorbuf, &classbuf[selector_start],
		   selector_end - selector_start);

	  /* Find limits of parameter list */
	  while (classbuf[i_2] != '(')
	    ++i_2;

	  paramlist_start = i_2++;
	  n_parens = 1;

	  while (classbuf[i_2]) {
	    if (classbuf[i_2] == '(')
	      ++n_parens;
	    if (classbuf[i_2] == ')')
	      --n_parens;
	    ++i_2;
	    if (n_parens == 0)
	      break;
	  }
	  paramlist_end = i_2;

	  n_commas = 0;
	  for (i_3 = paramlist_start; i_3 > paramlist_end; i_3++) {
	    if (classbuf[i_3] == ',')
	      ++n_commas;
	  }

	  memset (paramlist_buf, 0, MAXMSG);
	  strncpy (paramlist_buf, &classbuf[paramlist_start],
		   paramlist_end - paramlist_start);
	  if (n_commas == 0) {
	    /* if the paramlist is "void" then n_params == 0,
	       if the paramlist is "__prefix__" tnen prefix = true
	       otherwise n_params == 1 */
	    if (strstr (paramlist_buf, "void")) {
	      n_params = 0;
	    } else if (strstr (paramlist_buf, "__prefix__")) {
	      prefix = true;
	    } else {
	      n_params = 1;
	    }
	  } else {
	    if (strstr (paramlist_buf, "...")) {
	      varargs = true;
	    } else {
	      n_params = n_commas - 1;
	    }
	  }

	  ctitoa (decl_start_line, ln_buf);
	  if (varargs) {
	    n_param_buf[0] = 'v', n_param_buf[1] = '\0';
	  } else if (prefix) {
	    n_param_buf[0] = 'p', n_param_buf[1] = '\0';
	  } else {
	    ctitoa (n_params, n_param_buf);
	  }
	  
	  strcatx (entrybuf, "n:", ln_buf, ":",
		   (class_method ? "c" : "i"), ":",
		   selectorbuf, ":", n_param_buf, "\n", NULL);
	  fwrite ((void *)entrybuf, sizeof (char), strlen (entrybuf), fw);

	  if (method_line_info_head -> methods == NULL) {
	    method_line_info_head -> methods =
	      method_line_info_head -> methods_head =
	      __xalloc (sizeof (struct _method_line_info));
	  } else {
	    method_line_info_head -> methods_head -> next =
	      __xalloc (sizeof (struct _method_line_info));
	    method_line_info_head -> methods_head =
	      method_line_info_head -> methods_head -> next;
	  }
	  strcpy (method_line_info_head -> methods_head -> selector,
		  selectorbuf);
	  method_line_info_head -> methods_head -> line =
	    decl_start_line;
	  if (class_method)
	    method_line_info_head -> methods_head -> class_method =
	      true;
	  if (prefix) {
	    method_line_info_head -> methods_head -> prefix =
	      true;
	  } else if (varargs) {
	    method_line_info_head -> methods_head -> varargs =
	      true;
	  } else {
	    method_line_info_head -> methods_head -> n_params = 
	      n_params;
	  }

	  varargs = prefix = false;
	  i += classnamelength;
	}
      }
    } else {
      lastprint = c;
    }
  }

}

void save_class_init_info (char *classname, char *superclassname,
			   char *library_pathname) {

  FILE *fp;
  int length, bytes_written, r, class_length, lineno;
  char buf[MAXMSG], *classbuf;
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

  /* file_size handles any errors itself. */
  /***/
  class_length = file_size (library_pathname);
  classbuf = __xalloc (class_length + 1);
  if (read_file (classbuf, library_pathname) < 0) {
    _error ("ctalk: %s: %s.\n", library_pathname, strerror (errno));
  }

  pre_class_ln_parse (fp, classname, classbuf, &lineno);
  
  __xfree (MEMADDR(classbuf));


  if ((r = fclose (fp)) != 0)
    _error ("save_class_init_info (fclose (fp)): %s: %s.\n",
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
  MLI *mli;
  CLSLI *cinfo;
  
  stat (cache_fn, &statbuf);

  if ((buf = (char *)malloc (statbuf.st_size + 1)) == NULL)
    _error ("fill_in_init_info (malloc): %s.\n", strerror (errno));

  memset (buf, 0, statbuf.st_size + 1);

  (void)read_file (buf, cache_fn);

  memset ((void *)&class_init_info, 0, sizeof (struct _class_init_info));
  
  var_ptr = 0;

  cinfo = __xalloc (sizeof (struct _class_line_info));
  
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
	    /***/
	    strncpy (cinfo -> class, rptr, rptr2 - rptr);
	    if (method_line_info == NULL) {
	      method_line_info = method_line_info_head = cinfo;
	    } else {
	      method_line_info_head -> next = cinfo;
	      method_line_info_head = cinfo;
	    }
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

	/* line number info lines */
	/***/
      case 'n':

	mli = __xalloc (sizeof (struct _method_line_info));

	if ((rptr = strchr (lptr, ':')) != NULL) {
	  ++rptr;
	  if ((rptr2 = strchr (rptr, ':')) != NULL) {
	    mli -> line = strtol (rptr, &rptr2, 10);
	    rptr = rptr2 + 1;
	  }
	}
	if ((rptr2 = strchr (rptr, ':')) != NULL) {
	  if (*rptr == 'i') {
	    mli -> class_method = false;
	  } else if (*rptr == 'c') {
	    mli -> class_method = true;
	  }
	  rptr = rptr2 + 1;
	}
	if ((rptr2 = strchr (rptr, ':')) != NULL) {
	  strncpy (mli -> selector, rptr, rptr2 - rptr);
	  rptr = rptr2 + 1;
	}
	if ((rptr2 = strchr (rptr, '\n')) != NULL) {
	  if (*rptr == 'p') {
	    mli -> prefix = true;
	  } else if (*rptr == 'v') {
	    mli -> varargs = true;
	  } else {
	    mli -> n_params = strtol (rptr, &rptr2, 10);
	  }
	}
	lptr = rptr2 + 1;

	if (method_line_info_head -> methods == NULL) {
	  method_line_info_head -> methods =
	    method_line_info_head -> methods_head = mli;
	} else {
	  method_line_info_head -> methods_head -> next = mli;
	  method_line_info_head -> methods_head = mli;
	}

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
