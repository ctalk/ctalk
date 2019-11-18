/* $Id: clibtmpl.c,v 1.2 2019/11/18 21:26:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2016, 2019 Robert Kiesling, rk3314042@gmail.com.
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
 *  Translate a macro name to a C99 name.  First check the
 *  name as it is translated by the library, then the C99
 *  name.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

struct user_template_name {
  char *actual_name, *api_name;
};

static struct _c99_names {
  char *lib_name,
    *c99_name;
} c99_names[] = {
#ifdef __GNUC__
  {"_IO_getc", "getc"},
  {"abs",      "abs"},
  {"acos",     "acos"},
  {"acosh",    "acosh"},
  {"asctime",  "asctime"},
  {"asin",     "asin"},
  {"asinh",    "asinh"},
  {"atanh",    "atanh"},
  {"atof",     "atof"},
  {"atoi",     "atoi"},
  {"atol",     "atol"},
#ifndef __APPLE__
  {"atoll",    "atoll"},
#endif
  {"cbrt",     "cbrt"},
  {"ceil",     "ceil"},
  {"chdir",    "chdir"},
  {"clearerr", "clearerr"},
  {"clock",    "clock"},
  {"copysign", "copysign"},
  {"cos",      "cos"},
  {"cosh",     "cosh"},
  {"ctime",    "ctime"},
  {"difftime", "difftime"},
  {"erf",      "erf"},
  {"erfc",     "erfc"},
  {"exp",      "exp"},
  {"expm1",    "expm1"},
  {"fabs",     "fabs"},
  {"fabsf",    "fabsf"},
  {"fflush",   "fflush"},
  {"floor",    "floor"},
  {"floorf",    "floorf"},
  {"floorl",    "floorl"},
  {"fopen",    "fopen"},
  {"fprintf",  "fprintf"},
  {"fscanf",   "fscanf"},
  {"getchar",  "getchar"},
  {"getcwd",   "getcwd"},
  {"getenv",   "getenv"},
  {"getpid",   "getpid"},
  {"lrint",    "lrint"},
  {"lrintf",    "lrintf"},
  {"lrintl",    "lrintl"},
  {"llrint",    "llrint"},
  {"llrintf",    "llrintf"},
  {"llrintl",    "llrintl"},
  {"mkdir",    "mkdir"},
  {"printf",   "printf"},
  {"rand",     "rand"},
  {"rint",     "rint"},
  {"rmdir",    "rmdir"},
  {"round",    "round"},
  {"scanf",    "scanf"},
  {"sin",      "sin"},
  {"sprintf",  "sprintf"},
  {"sqrt",     "sqrt"},
  {"sscanf",   "sscanf"},
  {"strcat",   "strcat"},
  {"strchr",   "strchr"},
  {"strcasecmp",   "strcasecmp"},
  {"strcmp",   "strcmp"},
  {"strcpy",   "strcpy"},
  {"strerror", "strerror"},
  {"strlen",   "strlen"},
  {"strncat",  "strncat"},
  {"strncasecmp",  "strncasecmp"},
  {"strncmp",  "strncmp"},
  {"strncpy",  "strncpy"},
  {"time",     "time"},
  {"xsprintf",     "xsprintf"},
  {"xstrcat",  "xstrcat"},
  {"xstrcpy",  "xstrcpy"},
  {"xstrncat", "xstrncat"},
  {"xstrncpy", "xstrncpy"},
  {NULL, NULL}
#endif  /* #ifdef __GNUC__ */
};

char *c99_name (char *name, int warn) {
  int i;

  for (i = 0; c99_names[i].lib_name; i++) {
    if (!strcmp (c99_names[i].lib_name, name))
      return c99_names[i].c99_name;
  }

  if (warn)
    _warning ("c99_name: unknown function %s.\n", name);

  return NULL;
}

/* 
 * See if we can remove one of these.
 */
char *get_clib_template_file(char *fn_name) {

  char templatedir[FILENAME_MAX],
    templatefile[FILENAME_MAX],
    *filebuf;
  DIR *dir;
  struct dirent *d;
  struct stat statbuf;
  
  if (!fn_name) return NULL;

  if (user_template_name (fn_name)) {
    strcatx (templatedir, getenv("HOME"), "/", USERDIR, 
	     "/", USERTEMPLATEDIR, NULL);
  } else if (c99_name (fn_name,  FALSE)) {
    strcatx (templatedir, CLASSLIBDIR,"/", CLIBDIR, NULL);
  } else {
    _error 
      ("get_clib_template_file (): Undefined template for function, \"%s\".\n",
       fn_name);
  }

  if ((dir = opendir (templatedir)) == NULL)
    _error ("get_clib_template_file: %s.\n", strerror (errno));

  while ((d = readdir (dir)) != NULL) 
    if (*d -> d_name == *fn_name) break;
  closedir (dir);
    
  if (!d) {
    _warning ("get_clib_template_file: file for %s not found.\n", fn_name);
    return NULL;
  }

  strcatx (templatefile, templatedir, "/", d -> d_name, NULL);

  stat (templatefile, &statbuf);

  if ((filebuf = (char *)__xalloc (statbuf.st_size * sizeof (char))) == NULL)
    _error ("get_clib_template: %s.\n", strerror (errno));

  read_file (filebuf, templatefile);

  return filebuf;
}

char *get_clib_template_fn (char *func, char *filename_out) {

  char templatedir[FILENAME_MAX];
  DIR *dir;
  struct dirent *d;
  
  if (!func) return NULL;

  if (user_template_name (func)) {
    strcatx (templatedir, getenv("HOME"), "/", USERDIR, "/",
	     "/", USERTEMPLATEDIR, NULL);
  } else if (c99_name (func,  FALSE)) {
    strcatx (templatedir, CLASSLIBDIR, "/", CLIBDIR, NULL);
  } else {
    _error 
      ("get_clib_template_fn (): Undefined template for function, \"%s\".\n",
       func);
  }

  if ((dir = opendir (templatedir)) == NULL)
    _error ("get_clib_template_fn: %s.\n", strerror (errno));

  *filename_out = 0;
  while ((d = readdir (dir)) != NULL)  {
    if (*d -> d_name == *func) {
      strcatx (filename_out, templatedir, "/", d -> d_name, NULL);
      break;
    }
  }
  closedir (dir);
    
  if (!*filename_out) {
    _warning ("get_clib_template_fn: file for %s not found.\n", func);
    return NULL;
  }

  return filename_out;
}


static struct _c99_writable_args {
  char *lib_name,
    *c99_name;
} c99_writable_args[] = {
#ifdef __GNUC__
  {"fscanf",   "fscanf"},
  {"scanf",    "scanf"},
  {"sscanf",   "sscanf"},
  {"sprintf",  "sprintf"},
  {"strcpy",   "strcpy"},
  {"strcat",   "strcat"},
  {"strncpy",  "strncpy"},
  {"strncat",  "strncat"},
  {"xstrcpy",  "xstrcpy"},
  {"xstrcat",  "xstrcat"},
  {"xstrncpy", "xstrncpy"},
  {"xstrncat", "xstrncat"},
  {NULL, NULL}
#endif  /* #ifdef __GNUC__ */
};

int libc_fn_needs_writable_args (char *fn_name) {
  int i;
  for (i = 0; c99_writable_args[i].lib_name; i++) {
    if (!strcmp (c99_writable_args[i].lib_name, fn_name))
      return TRUE;
  }
  return FALSE;
}

char *template_name (char *fn_name) {
  char *c;
  if ((c = user_template_name (fn_name)) != NULL) {
    return c;
  } else if ((c = c99_name (fn_name, FALSE)) != NULL) {
    return c;
  } else {
    return NULL;
  }
}

static LIST *user_templates = NULL;

int get_user_template_names (void) {
  char path[FILENAME_MAX];
  char rbuf[MAXMSG];
  char pbuf[MAXMSG];
  char cbuf[MAXMSG];
  char actual_name[MAXLABEL];
  FILE *f;
  int f_size;
  char *c_ptr;
  int i, j;
  LIST *l;
  struct user_template_name *utn;

  strcatx (path, getenv("HOME"), "/.ctalk/", USERTEMPLATEDIR,
	   "/", USERTEMPLATEFILE, NULL);

  if (!file_exists (path))
    return SUCCESS;

  f_size = file_size (path);

  f = fopen (path, "r");
  if (!f) {
    printf ("get_user_template_names: %s: %s\n", path, strerror (errno));
    exit (EXIT_FAILURE);
  }

  while (fgets (rbuf, MAXMSG, f)) {
    trim_leading_whitespace (rbuf, pbuf);
    if (*pbuf == '#' || *pbuf == 0 || *pbuf == '\n')
      continue;
    if ((c_ptr = strchr (pbuf, ',')) == NULL) {
      printf ("get_user_template_names: %s: badly formed line \"%s\".\n",
		path, rbuf);
      continue;
    }

    /* Squeeze the whitespace out of the line. */
    for (i = 0, j = 0; pbuf[i]; ++i) {
      if (!isspace ((int)pbuf[i])) {
	cbuf[j++] = pbuf[i]; cbuf[j] = 0;
      }
    }
    if ((c_ptr = strchr (cbuf, ',')) == NULL) {
      printf ("get_user_template_names: %s: bad entry: %s\n",
	      USERTEMPLATEFILE, pbuf);
      continue;
    }
    utn = __xalloc (sizeof (struct user_template_name));
    memset (actual_name, 0, MAXLABEL);
    strncpy (actual_name, cbuf, c_ptr - cbuf);
    utn -> actual_name = strdup (actual_name);
    ++c_ptr;
    utn -> api_name = strdup (c_ptr);

    if (user_templates == NULL) {
      user_templates = new_list ();
      user_templates -> data = (void *)utn;
    } else {
      l = new_list ();
      l -> data = (void *)utn;
      list_push (&user_templates, &l);
    }

  }

  fclose (f);
  return SUCCESS;
}

static void delete_utn (struct user_template_name *u) {
  __xfree (MEMADDR (u -> actual_name));
  __xfree (MEMADDR (u -> api_name));
  __xfree (MEMADDR (u));
}

void cleanup_user_templates (void) {
  LIST *l, *l_prev;
  struct user_template_name *utn;
  if (user_templates != NULL) {

    for (l = user_templates; l && l -> next; l = l -> next)
      ;
    
    if (l == user_templates) {

      utn = (struct user_template_name *)l -> data;
      delete_utn (utn);
      l -> data = NULL;
      delete_list_element (l);

    } else {

      while (l != user_templates) {

	l_prev = l -> prev;

	utn = (struct user_template_name *)l -> data;
	delete_utn (utn);
	l -> data = NULL;
	delete_list_element (l);
	l = l_prev;

      }

      utn = (struct user_template_name *)user_templates -> data;
      delete_utn (utn);
      user_templates -> data = NULL;
      delete_list_element (user_templates);

    }
  }
}

/* For a (if macroized to another function name and then preprocessed)
   function name (the "actual_name"), return the "api_name". */
char *user_template_name (char *fn_name) {
  LIST *l;
  struct user_template_name *utn;
  if (user_templates != NULL) {
    for (l = user_templates; l; l = l -> next) {
      utn = (struct user_template_name *)l -> data;
      if (str_eq (utn -> actual_name, fn_name))
	return utn -> api_name;
    }
  }
  return NULL;
}
