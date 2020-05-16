/* $Id: premethod.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014-2018 Robert Kiesling, rk3314042@gmail.com.
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
 *  Read and write cached methods.
 *
 *  The fixups here work in synchronization, like the class library lookups,
 *  so there *could* yet be  _errors () if an individual cached method gets 
 *  deleted, if it was declared before the method being un-cached to the
 *  output.
 *
 *  If one cached method gets deleted, and it causes an error or exception,
 *  then the entire cache needs be cleared and rewritten.  Use the 
 *  --clearpreload command line option.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"


extern char output_file[];

HASHTAB selectors_output;
HASHTAB class_and_instance_vars_output;

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;

extern char *ascii[8193];             /* from intascii.h */

void init_pre_methods (void) {
  _new_hash (&selectors_output);
  _new_hash (&class_and_instance_vars_output);
}

int pre_method_is_output (char *selector) {
  if (_hash_get (selectors_output, selector))
    return TRUE;
  else
    return FALSE;
}

void pre_method_register_selector_as_output (char *selector) {
  _hash_put (selectors_output, selector, selector);
}

/*
 *  Save the information that generate_*_method_definition_call ()
 *  and generate_*_method_param_definition_call () need.
 *  We can queue this with the other global inits when
 *  reading the cached method.
 */
void save_pre_init_info (char *cache_fn, METHOD *n_method, 
			 OBJECT *class_object) {
  FILE *fp;
  int length, bytes_written, r, i;
  char path[FILENAME_MAX];
  char buf[MAXMSG], tbuf[MAXLABEL];
  OBJECT *var;
  NEWMETHOD *n;

  strcatx (path, cache_fn, ".init", NULL);
  

  if ((fp = fopen (path, FILE_WRITE_MODE)) == NULL) 
    _error ("save_pre_init_info (fopen): %s: %s.\n",
	    path, strerror (errno));

  n = new_methods[new_method_ptr+1];

  for (i = MAXARGS; i > n -> argblk_ptr; i--) {
    if (!(n -> argblks[i] -> attrs & METHOD_ARGBLK_ATTR)) {
      _error ("Badly formed argument block in method, \"%s.\"\n",
	      n -> method -> name);
    }
    length = strcatx (buf, "a:",
		      class_object -> __o_name, ":",
		      "Object:",  /* with the METHOD_ARGBLK_ATTR, the
				     class is always, "Object" */
		      n -> argblks[i] -> selector, ":",
		      n -> argblks[i] -> selector, "\n",
		      NULL);

    if ((bytes_written = fwrite ((void *)buf, sizeof (char), length, fp))
	!= length) 
      _error ("save_pre_init_info (fwrite (a:)): %s: %s.\n",
	      path, strerror (errno));

  }

  length = strcatx (buf, "m:",
		    class_object -> __o_name, ":",
		    n_method -> returnclass, ":",
		    n_method -> name, ":",
		    n_method -> selector, ":",
		    ascii[n_method -> n_params], ":",
		    ascii[method_definition_attributes (n_method)],
		    "\n", NULL);

  if ((bytes_written = fwrite ((void *)buf, sizeof (char), length, fp))
      != length) 
    _error ("save_pre_init_info (fwrite (m:)): %s: %s.\n",
	    path, strerror (errno));

  for (i = 0; i < n_method -> n_params; i++) {

    if (n_method -> params[i] -> attrs & PARAM_C_PARAM)
      continue;

    length = strcatx (buf, "p:",
		      class_object -> __o_name, ":",
		      n_method -> name, ":",
		      n_method -> params[i] -> name, ":",
		      n_method -> params[i] -> class, ":",
		      (n_method -> params[i] -> is_ptr ? "1" : "0"), ":",
		      (n_method -> params[i] -> is_ptrptr ? "1" : "0"), "\n",
		      NULL);

    if ((bytes_written = fwrite ((void *)buf, sizeof (char), length, fp))
	!= length) 
      _error ("save_pre_init_info (fwrite (p:)): %s: %s.\n",
	      path, strerror (errno));

  }

  if (class_object -> instancevars) {
    for (var = class_object -> instancevars; var; var = var -> next) {

      /* value gets created automatically at run time. */
      if (!strcmp (var -> __o_name, "value"))
	continue;

      length = strcatx (buf, "i:",
			class_object -> __o_name, ":",
			var -> __o_name, ":",
			((var -> instancevars) ? 
			 var -> instancevars -> __o_classname :
			 var -> __o_classname), ":",
			((var -> __o_value && *var -> __o_value) ? 
			 var -> __o_value : NULLSTR), "\n",
			NULL);

    if ((bytes_written = fwrite ((void *)buf, sizeof (char), length, fp))
	!= length) 
      _error ("save_pre_init_info (fwrite (i:)): %s: %s.\n",
	      path, strerror (errno));

    }

  }

  if (class_object -> classvars) {
    for (var = class_object -> classvars; var; var = var -> next) {

      /* again, value gets created automatically at run time. */
      if (!strcmp (var -> __o_name, "value"))
	continue;

      length = strcatx (buf, "c:",
			class_object -> __o_name, ":",
			var -> __o_name, ":",
			((var -> instancevars) ?
			 var -> instancevars -> CLASSNAME :
			 var -> CLASSNAME), ":",
			((var -> __o_value && *var -> __o_value) ? 
			 var -> __o_value : NULLSTR), "\n",
			NULL);

    if ((bytes_written = fwrite ((void *)buf, sizeof (char), length, fp))
	!= length) 
      _error ("save_pre_init_info (fwrite (c:)): %s: %s.\n",
	      path, strerror (errno));

    }

  }

  if (IS_LIST(n -> templates)) {
    LIST *t;
    *buf = 0;
    for (t = n -> templates; t; t = t -> next) {
      strcatx (tbuf, "t:", (char *)t -> data, "\n", NULL);
      strcatx2 (buf, tbuf, NULL);
    }

    length = strlen (buf);

    if ((bytes_written = fwrite ((void *)buf, sizeof (char), length, fp))
	!= length) 
      _error ("save_pre_init_info (fwrite (t:)): %s: %s.\n",
	      path, strerror (errno));
  }


  if ((r = fclose (fp)) != 0)
    _error ("save_pre_init_info (fclose): %s: %s.\n",
	    path, strerror (errno));

}

static struct _var_init_info {
  char classname[MAXLABEL],
    varname[MAXLABEL],
    varclass[MAXLABEL],
    init[MAXMSG];
} var_info;

struct _param_init_info {
  char classname[MAXLABEL],
    methodname[MAXLABEL],
    paramname[MAXLABEL],
    paramclass[MAXLABEL];
  int is_ptr,
    is_ptrptr;
};

struct _argblk_init_info {
  char classname[MAXLABEL],
    returnclassname[MAXLABEL],
    methodname[MAXLABEL],
    selectorname[MAXLABEL];
};

static struct _init_info {
  char path[FILENAME_MAX],
    classname[MAXLABEL],
    returnclassname[MAXLABEL],
    methodname[MAXLABEL],
    selectorname[MAXLABEL];
  int required_args,
    method_attrs;
  struct _param_init_info params[MAXARGS];
  struct _argblk_init_info argblks[MAXARGS];
  char templates[MAXARGS][MAXLABEL];
} init_info;

static int init_param_ptr = 0;
static int init_argblk_ptr = 0;
static int init_template_ptr = 0;

#define I_OR_C_CLASS "class"
#define I_OR_C_INSTANCE "instance"

void set_class_and_instance_var_init_output (char *i_or_c) {
  char buf[MAXMSG];
  strcatx (buf, var_info.classname, "_",
	   i_or_c, "_",
	   var_info.varname, NULL);
  _hash_put (class_and_instance_vars_output, buf, buf);
}

void set_class_and_instance_var_init_output_2 (char *classname, char *i_or_c,
					       char *varname) {
  char buf[MAXMSG];
  strcatx (buf, classname, "_",
	   i_or_c, "_",
	   varname, NULL);
  _hash_put (class_and_instance_vars_output, buf, buf);
}

int class_or_instance_var_init_is_output (char *i_or_c) {
  char buf[MAXMSG];
  strcatx (buf, var_info.classname, "_",
	   i_or_c, "_",
	   var_info.varname, NULL);
  if (_hash_get (class_and_instance_vars_output, buf))
    return TRUE;
  else
    return FALSE;
}

int class_or_instance_var_init_is_output_2 (char *classname,
					    char *i_or_c, 
					    char *varname) {
  char buf[MAXMSG];
  strcatx (buf, classname, "_",
	   i_or_c, "_",
	   varname, NULL);
  if (_hash_get (class_and_instance_vars_output, buf))
    return TRUE;
  else
    return FALSE;
}

static void premethod_scan_line (char *text, char pattern, int *offsets) {
  int i, n_th;

  for (i = 0, n_th = 0; text[i]; ++i) {
    if (text[i] == pattern) {
      offsets[n_th++] = i;
    }
    if (text[i] == '\n') {
      offsets[n_th++] = i;
      break;
    }
  }
  offsets[n_th] = -1;
}

static void fill_in_init_info (char *selector, int gen_var_init) {
  char path[FILENAME_MAX], *buf;
  char ibuf[0xff];
  struct stat statbuf;
  char *lptr, *eptr;
  int pos_array[64];
  int read_fd, bytes_read;

  strcatx (path, home_libcache_path (), "/", selector, ".init", NULL);

  stat (path, &statbuf);

  if ((buf = (char *)__xalloc (statbuf.st_size + 1)) == NULL)
    _error ("fill_in_init_info (malloc): %s.\n", strerror (errno));

  if ((read_fd = open (path, O_RDONLY)) == ERROR)
    _error ("ctalk: file %s: %s.\n", path, strerror (errno));
  if ((bytes_read = read (read_fd, buf, statbuf.st_size)) !=
      statbuf.st_size)
    _error ("ctalk: file %s: %s.\n", path, strerror (errno));
  if (close (read_fd))
    _error ("ctalk: file %s: %s.\n", path, strerror (errno));

  memset ((void *)&init_info, 0, sizeof (struct _init_info));
  
  init_param_ptr = 0;
  init_argblk_ptr = 0;
  init_template_ptr = 0;

  lptr = buf;
  eptr = strchr (buf, '\0');

  do {

    switch (*lptr)
      {
      case 'm':
	premethod_scan_line (lptr, ':', pos_array);
	substrcpy (init_info.classname, lptr, pos_array[0] + 1,
		   pos_array[1] - (pos_array[0] + 1));
	substrcpy (init_info.returnclassname, lptr, pos_array[1] + 1,
		   pos_array[2] - (pos_array[1] + 1));
	substrcpy (init_info.methodname, lptr, pos_array[2] + 1,
		   pos_array[3] - (pos_array[2] + 1));
	substrcpy (init_info.selectorname, lptr, pos_array[3] + 1,
		   pos_array[4] - (pos_array[3] + 1));
	substrcpy (ibuf, lptr, pos_array[4] + 1,
		   pos_array[5] - (pos_array[4] + 1));
	init_info.required_args = atoi (ibuf);
	substrcpy (ibuf, lptr, pos_array[5] + 1,
		   pos_array[6] - (pos_array[5] + 1));
	init_info.method_attrs = atoi (ibuf);
	lptr += pos_array[6] + 1;
	break;

      case 'a':
	premethod_scan_line (lptr, ':', pos_array);
	substrcpy (init_info.argblks[init_argblk_ptr].classname,
		   lptr, pos_array[0]+ 1, pos_array[1] - (pos_array[0] + 1));
	substrcpy (init_info.argblks[init_argblk_ptr].returnclassname,
		   lptr, pos_array[1]+ 1, pos_array[2] - (pos_array[1] + 1));
	substrcpy (init_info.argblks[init_argblk_ptr].methodname,
		   lptr, pos_array[2]+ 1, pos_array[3] - (pos_array[2] + 1));
	substrcpy (init_info.argblks[init_argblk_ptr].selectorname,
		   lptr, pos_array[3]+ 1, pos_array[4] - (pos_array[3] + 1));
	lptr += pos_array[4] + 1;
	++init_argblk_ptr;
	break;

      case 'p':
	premethod_scan_line (lptr, ':', pos_array);
	substrcpy (init_info.params[init_param_ptr].classname,
		   lptr, pos_array[0] + 1, pos_array[1] - (pos_array[0] + 1)); 
	substrcpy (init_info.params[init_param_ptr].methodname,
		   lptr, pos_array[1] + 1, pos_array[2] - (pos_array[1] + 1)); 
	substrcpy (init_info.params[init_param_ptr].paramname,
		   lptr, pos_array[2] + 1, pos_array[3] - (pos_array[2] + 1)); 
	substrcpy (init_info.params[init_param_ptr].paramclass,
		   lptr, pos_array[3] + 1, pos_array[4] - (pos_array[3] + 1)); 
	substrcpy (ibuf,
		   lptr, pos_array[4] + 1, pos_array[5] - (pos_array[4] + 1)); 
	init_info.params[init_param_ptr].is_ptr = atoi (ibuf);
	substrcpy (ibuf,
		   lptr, pos_array[5] + 1, pos_array[6] - (pos_array[5] + 1)); 
	init_info.params[init_param_ptr].is_ptrptr = atoi (ibuf);
	lptr += pos_array[6] + 1;
	++init_param_ptr;
	break;

      case 'i':
	premethod_scan_line (lptr, ':', pos_array);
	if (gen_var_init) {
	  substrcpy (var_info.classname, lptr,
		     pos_array[0] + 1, pos_array[1] - (pos_array[0] + 1));
	  substrcpy (var_info.varname, lptr,
		     pos_array[1] + 1, pos_array[2] - (pos_array[1] + 1));
	  substrcpy (var_info.varclass, lptr,
		     pos_array[2] + 1, pos_array[3] - (pos_array[2] + 1));
	  substrcpy (var_info.init, lptr,
		     pos_array[3] + 1, pos_array[4] - (pos_array[3] + 1));
	  generate_define_instance_variable_call 
	    (var_info.classname, var_info.varname, 
	     var_info.varclass, var_info.init);
	}
	lptr += pos_array[4] + 1;
	break;

      case 'c':
	premethod_scan_line (lptr, ':', pos_array);
	if (gen_var_init) {
	  substrcpy (var_info.classname, lptr,
		     pos_array[0] + 1, pos_array[1] - (pos_array[0] + 1));
	  substrcpy (var_info.varname, lptr,
		     pos_array[1] + 1, pos_array[2] - (pos_array[1] + 1));
	  substrcpy (var_info.varclass, lptr,
		     pos_array[2] + 1, pos_array[3] - (pos_array[2] + 1));
	  substrcpy (var_info.init, lptr,
		     pos_array[3] + 1, pos_array[4] - (pos_array[3] + 1));
	  generate_define_class_variable_call 
	    (var_info.classname, var_info.varname, 
	     var_info.varclass, var_info.init);
	}
	lptr += pos_array[4] + 1;
	break;

      case 't':
	premethod_scan_line (lptr, ':', pos_array);
	substrcpy (init_info.templates[init_template_ptr], lptr,
		   pos_array[0] + 1, pos_array[1] - (pos_array[0] + 1));
	lptr += pos_array[1] + 1;
	++init_template_ptr;
	break;
      }

    if (*lptr == 0)
      break;

  }  while (lptr < eptr);

  __xfree (MEMADDR(buf));

}

char *get_init_return_class (char *selector) {

  static char *returnclass;

  fill_in_init_info (selector, FALSE);

  returnclass = init_info.returnclassname;
  return returnclass;

}

void register_init (char *selector) {
  int param_idx, argblk_idx;

  fill_in_init_info (selector, TRUE);

  for (argblk_idx = 0; argblk_idx < init_argblk_ptr; argblk_idx++) {
    generate_instance_method_definition_call
      (init_info.argblks[argblk_idx].classname,
       init_info.argblks[argblk_idx].selectorname,
       init_info.argblks[argblk_idx].selectorname,
       0, 0);
  }

  if (strstr (init_info.selectorname, "_instance_")) {
    generate_instance_method_definition_call
      (init_info.classname,
       init_info.methodname,
       init_info.selectorname,
       init_info.required_args,
       init_info.method_attrs);
    generate_instance_method_return_class_call
      (init_info.classname,
       init_info.methodname,
       init_info.selectorname,
       init_info.returnclassname,
       init_info.required_args);
  } else if (strstr (init_info.selectorname, "_class_")) {
    generate_class_method_definition_call
      (init_info.classname,
       init_info.methodname,
       init_info.selectorname,
       init_info.required_args);
    generate_class_method_return_class_call
      (init_info.classname,
       init_info.methodname,
       init_info.returnclassname,
       init_info.required_args);
  }

  for (param_idx = 0; param_idx < init_param_ptr; param_idx++) {

    if (strstr (init_info.selectorname, "instance")) {
      generate_instance_method_param_definition_call
	(init_info.params[param_idx].classname,
	 init_info.params[param_idx].methodname,
	 init_info.selectorname,
	 init_info.params[param_idx].paramname,
	 init_info.params[param_idx].paramclass,
	 init_info.params[param_idx].is_ptr,
	 init_info.params[param_idx].is_ptrptr);
    } else if (strstr (init_info.selectorname, "class")) {
      generate_class_method_param_definition_call
	(init_info.params[param_idx].classname,
	 init_info.params[param_idx].methodname,
	 init_info.selectorname,
	 init_info.params[param_idx].paramname,
	 init_info.params[param_idx].paramclass,
	 init_info.params[param_idx].is_ptr,
	 init_info.params[param_idx].is_ptrptr);
    }

  }

}

static int cache_method_src_to_output (char *selector) {

  char cache_fn[FILENAME_MAX], *srcbuf;
  int chars_read, read_fd;
  struct stat statbuf;

  strcatx (cache_fn, home_libcache_path (), "/", selector, NULL);

  /* 
     TO DO! If a cached method for a fixup doesn't exist, 
     generate an exception and reparse the entire class, including 
     that method. This could happen if an individual cached method 
     gets deleted.

     Also, if a method's dependencies get updated, the entire
     class should need to be re-parsed.  The functions in 
     libdeps.c and used in primitives.c don't handle methods
     included in fixups, so this should also generate an 
     exception and cause a re-parse.

  */

  stat (cache_fn, &statbuf);

  if ((srcbuf = (char *)__xalloc (statbuf.st_size + 1)) == NULL)
    _error ("cache_method_src_to_output: %s.\n", strerror (errno));
  if ((read_fd = open (cache_fn, O_RDONLY)) == ERROR)
    _error ("ctalk: file %s: %s.\n", cache_fn, strerror (errno));
  if ((chars_read = read (read_fd, srcbuf, statbuf.st_size)) !=
      statbuf.st_size)
    _error ("ctalk: file %s: %s.\n", cache_fn, strerror (errno));
  if (close (read_fd))
    _error ("ctalk: file %s: %s.\n", cache_fn, strerror (errno));


  output_pre_method (selector, srcbuf);

  __xfree (MEMADDR(srcbuf));
  
  return 0;
}

static inline void elide_fixup_marker (char *marker_start) {
  char *s;

  s = marker_start;
  do {
    *s = ' ';
    ++s;
  } while (*s != '\n');

}

void output_pre_method_templates (char *selector) {
  int i;
  /* We shouldn't need to call this yet again. */
  /*  fill_in_init_info (selector, FALSE); */
  if (init_template_ptr > 0) {
    for (i = 0; i < init_template_ptr; ++i) {
      premethod_output_template (init_info.templates[i]);
    }
  }
}

void output_pre_method (char *selector, char *src) {

  char *o, *p, *q, *r, buf[MAXLABEL + 1];

  if (!selector || !src)
    return;

  if (_hash_get (selectors_output, selector))
    return;

  if (!strstr (src, "__ctalk_method")) {

    _hash_put (selectors_output, selector, selector);

    __fileout_cache (src);
    register_init (selector);

  } else {

    o = src;

    /* Fixup the classes first. */
    while ((p = strstr (o, CONSTRUCTOR_ARG_FN)) != NULL) {

      q = strchr (p, '"'); ++q;
      p = strchr (q, '"');

      memset (buf, 0, MAXLABEL);
      strncpy (buf, q, p - q);

      if (is_pending_class (buf) ||
	  is_cached_class (buf)) {
	o = q;
	continue;
      }

      load_cached_class (buf);
      o = q;

    }

    o = src;

    /* Then any method calls. */
    while ((p = strstr (o, "__ctalk_method")) != NULL) {

      q = strchr (p, ',');

      ++q;
      while (isspace ((int)*q))
       	++q;

      p = strchr (q, ',');

      memset (buf, 0, MAXLABEL);

      strncpy (buf, q, p - q);

      if (_hash_get (selectors_output, buf)) {
	o = q;
	continue;
      }

      cache_method_src_to_output (buf);

      o = q;

    }

    o = src;

    /* 
       Also take care of fixup markers. They're only written
       in eval_arg () right now for method messages that occur in 
       argument expressions.
    */
    while ((p = strstr (o, "/@ctalkfixup/")) != NULL) {

      r = p;     /* Find start of fixup marker. */
      while (*(--r) != '#')
	;

      ++p; q = strchr (p, '/'); /* Find end of selector. */
      ++q; p = q;
      q = strchr (p, '"');

      memset ((void *)buf, 0, MAXLABEL+1);
      strncpy (buf, p, q - p);

      if (_hash_get (selectors_output, buf)) {
	elide_fixup_marker (r);
	o = q;
	continue;
      }

      cache_method_src_to_output (buf);

      elide_fixup_marker (r);

      o = q;

    }

    if (!_hash_get (selectors_output, selector)) {

      _hash_put (selectors_output, selector, selector);
      __fileout_cache (src);
      register_init (selector);

    }

  }
  output_pre_method_templates (selector);
}
