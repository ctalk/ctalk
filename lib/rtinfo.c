/* $Id: rtinfo.c,v 1.3 2019/11/11 20:21:52 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2019  Robert Kiesling, rk3314042@gmail.com.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"
#include "parser.h"

#ifdef CLASSLIBDIR 
char *classlibdir = CLASSLIBDIR;
#else
char *classlibdir = NULL;
#endif
#ifdef PKGNAME
char *pkgname = PKGNAME;
#else
char *pkgname = NULL;
#endif
#ifdef INSTALLPREFIX
char *installprefix=INSTALLPREFIX;
#else
char *installprefix=NULL;
#endif

char *library_include_paths[MAXUSERDIRS] = { NULL, };

RT_INFO rtinfo;

RT_INFO *__call_stack[MAXARGS+1];
int __call_stack_ptr = MAXARGS;

I_PASS interpreter_pass;

RT_CLEANUP_MODE global_cleanup_mode = rt_cleanup_null;

static char argv_name[FILENAME_MAX];

static char classlib_name[FILENAME_MAX];

PARSER *parsers[MAXARGS+1];
int current_parser_ptr;

extern OBJECT *__ctalk_argstack[MAXARGS+1];
extern int __ctalk_arg_ptr;

void __argvName (char *s) {
  strcpy (argv_name, s);
}

char *__argvFileName (void) {
  return argv_name;
}

void __classLibName (char *s) {
  strcpy (classlib_name, s);
}

char *__classLibFileName (void) {
  return classlib_name;
}

void __setClassLibRead (bool b) {
  rtinfo.classlib_read = b;
}

bool __getClassLibRead (void) {
  return rtinfo.classlib_read;
}

int __ctalkRtSaveSourceFileName (char *s) {
  strcpy (rtinfo.source_file, s);
  return 0;
}

char *__ctalkClassSearchPath (void) {
  return __ctalkClassLibraryPath ();
}

char *__ctalkClassLibraryPath (void) {
  static char classlibpath[MAXMSG];
  char **dir, direntry[FILENAME_MAX];
  if (*library_include_paths == NULL) return NULL;
  dir = library_include_paths;
  strcpy (classlibpath, *dir);
  ++dir;
  while (*dir) {
    strcatx (direntry, ":", *dir, NULL);
    if ((strlen (classlibpath) + strlen (direntry)) > MAXMSG)
      break;
    strcatx2 (classlibpath, direntry, NULL);
    ++dir;
  }
  return classlibpath;
}

char *__source_filename (void) {
  return rtinfo.source_file;
}

void __ctalkRtReceiver (OBJECT *o) {
  rtinfo.rcvr_obj = o;
}

OBJECT *__ctalkRtReceiverObject (void) {
  return rtinfo.rcvr_obj;
}

void __ctalk_rtReceiverClass (OBJECT *o) {
  rtinfo.rcvr_class_obj = o;
}

OBJECT *__ctalk_rtReceiverClassObject (void) {
  return rtinfo.rcvr_class_obj;
}

void __ctalkRtMethodClass (OBJECT *o) {
  rtinfo.method_class_obj = o;
}

OBJECT *__ctalkRtMethodClassObject (void) {
  return rtinfo.method_class_obj;
}

void __ctalk_rtMethodFn (OBJECT *(*fn)()) {
  rtinfo.method_fn = fn;
}

void *__ctalkRtGetMethodFn (void) {
  return rtinfo.method_fn;
}

/*
 *  Only saved by __save_rt_info (). 
 */
METHOD *__ctalkRtGetMethod (void) {
  if (__call_stack_ptr < MAXARGS) {
    return (IS_METHOD(__call_stack[__call_stack_ptr+1]->method) ?
	    __call_stack[__call_stack_ptr+1] -> method : 
	    NULL);
  } else {
    return NULL;
  }
}

RT_FN *get_fn (void) { 
  if (__call_stack_ptr >= MAXARGS) return NULL;
  return (__call_stack[__call_stack_ptr+1]->_rt_fn ? 
	  __call_stack[__call_stack_ptr+1]->_rt_fn :
	  NULL); 
}

RT_FN *get_calling_fn (void) { 
  if ((__call_stack_ptr + 1) >= MAXARGS) return NULL;
  return (__call_stack[__call_stack_ptr+2]->_rt_fn ? 
	  __call_stack[__call_stack_ptr+2]->_rt_fn :
	  NULL); 
}

METHOD *get_calling_method (void) { 
  if ((__call_stack_ptr + 1) >= MAXARGS) return NULL;
  return (__call_stack[__call_stack_ptr+2]->method ? 
	  __call_stack[__call_stack_ptr+2]->method :
	  NULL); 
}

int __ctalk_initFn (char *name) {

  RT_FN *r;
  RT_INFO *r_info;

  r_info = _new_rtinfo ();

  r = new_rt_fn ();

  strcpy (r -> name, name);

  r_info -> _rt_fn = r;
  r_info -> _arg_frame_top = __ctalk_arg_ptr + 1;
  __call_stack[__call_stack_ptr--] = r_info;

  return 0;
}

extern int __cleanup_deletion;
int __app_exit = FALSE;

static void cleanup_non_local_object (OBJECT *o, VARENTRY *v) {
  if (o -> __o_vartags) {
    if (!IS_EMPTY_VARTAG (o -> __o_vartags)) {
      if (o -> __o_vartags -> tag != v) {
	unlink_varentry (v -> var_object);
      } else {
	remove_tag (o, v);
      }
    }
  }
  if (!(o -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION)) {
    if (object_is_deletable(o, NULL, NULL)) {
      /* __objRefCntZero (OBJREF(o)); */
      __ctalkDeleteObject (o);
    } else {
      if (__cleanup_deletion) {
	__ctalkDeleteObject (o);
      } else {
	__objRefCntDec (OBJREF(o));
      }
    }
  }
}

static void __delete_fn_local_objs (RT_FN *r) {
  VARENTRY *v_ptr, *v_ptr_prev;

  if (!__cleanup_deletion) {
    register_function_objects (r -> local_objects.vars);
    r -> local_objects.vars = NULL;
    return;
  }

  for (v_ptr = r -> local_objects.vars; v_ptr && v_ptr -> next;
       v_ptr = v_ptr -> next)
    ;
  if (v_ptr && (v_ptr == r -> local_objects.vars)) {
    if (IS_OBJECT(v_ptr -> var_object))
      cleanup_non_local_object (v_ptr -> var_object, v_ptr);

    if (IS_OBJECT(v_ptr -> orig_object_rec) &&
	(v_ptr -> orig_object_rec != v_ptr -> var_object)) {
      __ctalkDeleteObject (v_ptr -> orig_object_rec);
    }
    delete_varentry (v_ptr);
    r -> local_objects.vars = NULL;
  } else {
    while (v_ptr && (v_ptr != r -> local_objects.vars)) {
      v_ptr_prev = v_ptr -> prev;
      if (v_ptr && IS_OBJECT(v_ptr -> var_object)) {
	if (v_ptr -> var_object -> scope != GLOBAL_VAR) {
	  /* This should *always* be the case.  If it isn't,
	     then something else needs to be corrected. */
	  if (IS_OBJECT(v_ptr -> orig_object_rec)
	      && (v_ptr -> orig_object_rec != v_ptr -> var_object)) {
	    __ctalkDeleteObject (v_ptr -> orig_object_rec);
	  } 
	  cleanup_unlink_method_user_object (v_ptr);
	  cleanup_non_local_object (v_ptr -> var_object, v_ptr);
	} else {
	  v_ptr -> var_object = NULL;
	}
      }
      if (v_ptr && IS_OBJECT(v_ptr -> orig_object_rec) &&
	  (v_ptr -> orig_object_rec != v_ptr -> var_object)) {
	if (v_ptr -> orig_object_rec -> scope != GLOBAL_VAR) {
	  __ctalkDeleteObject (v_ptr -> orig_object_rec);
	} else {
	  v_ptr -> orig_object_rec = NULL;
	}
      }
      delete_varentry (v_ptr);
      v_ptr = v_ptr_prev;
    }
    if (v_ptr) {
      if (IS_OBJECT(v_ptr -> orig_object_rec)
	  && (v_ptr -> orig_object_rec != v_ptr -> var_object)) {
	if (v_ptr -> orig_object_rec -> scope != GLOBAL_VAR) {
	  __ctalkDeleteObject (v_ptr -> orig_object_rec);
	} else {
	  v_ptr -> orig_object_rec = NULL;
	}
      } 
      if (v_ptr && IS_OBJECT(v_ptr -> var_object)) {
	cleanup_unlink_method_user_object (v_ptr);
	cleanup_non_local_object (v_ptr -> var_object, v_ptr);
      }
      delete_varentry (v_ptr);
    }
    r -> local_objects.vars = NULL;
  }
  /* this is only reached when app_exit is true in __ctalk_exifFn* */
  cleanup_fn_objects ();
}

void __delete_fn_user_objs (RT_FN *);

static int __ctalk_exitFn_internal (int p_app_exit,
				    bool handle_children) {

  RT_FN *r;
  RT_INFO *r_info;
  int cleanup_deletion_save = 0;

  __app_exit = p_app_exit;
  if (__call_stack_ptr < MAXARGS) {
    r_info = __call_stack[++__call_stack_ptr];
    __call_stack[__call_stack_ptr] = NULL;

    if (!r_info) return SUCCESS;
    r = r_info -> _rt_fn;
    __xfree (MEMADDR(r_info));

    if (r) {
      if (r -> local_objects.vars != NULL) {
	cleanup_deletion_save = __cleanup_deletion;
	__cleanup_deletion = p_app_exit;
	__delete_fn_local_objs (r);
      }
      if (r -> user_objects != NULL) {
	__delete_fn_user_objs (r);
	r -> user_objects = r -> user_object_ptr = NULL;
      }
      __cleanup_deletion = cleanup_deletion_save;
      if (IS_CVAR(r -> local_cvars)) {
	CVAR *__c, *__c_prev;
#ifdef WARN_EXTRA_CVARS
	_warning ("Warning: __ctalk_exitFn (): Function, \"%s,\" has extra CVARS.\n",
		  r -> name);
#endif	

	for (__c = r -> local_cvars; __c && __c -> next; __c = __c -> next)
	  ;
	while (__c != r ->  local_cvars) {
	  __c_prev = __c -> prev;
	  _delete_cvar (__c);
	  __c = __c_prev;
	}
	_delete_cvar (r -> local_cvars);
	r -> local_cvars = NULL;
      }
      __xfree (MEMADDR(r));
    }
  }

  if (p_app_exit) {
    if (handle_children) 
      delete_processes ();  /* this gets called on any app_exit. */
    __ctalkClassDictionaryCleanup ();
    delete_default_class_cache ();
    rt_delete_class_names ();
    cleanup_reused_messages ();
#ifdef RETURN_VALPTRS
    delete_return_valptrs ();
#endif
  }

  return 0;
}

int __ctalkErrorExit (void) {

  RT_FN *r;
  RT_INFO *r_info;

  for ( ++__call_stack_ptr; 
	__call_stack_ptr <= MAXARGS; ++__call_stack_ptr) {
    if ((r_info = __call_stack[++__call_stack_ptr]) == NULL)
      continue;
    __call_stack[__call_stack_ptr] = NULL;
    r = r_info -> _rt_fn;
    __xfree (MEMADDR(r_info));
    if (r) {
      if (r -> local_objects.vars != NULL) {
	__delete_fn_local_objs (r);
      }
      __xfree (MEMADDR(r));
    }
  }
  __ctalkClassDictionaryCleanup ();
#ifdef RETURN_VALPTRS
  delete_return_valptrs ();
#endif
  return 1;
}

RT_FN *new_rt_fn (void) {

  RT_FN *r;

  if ((r = (RT_FN *)__xalloc (sizeof (RT_FN))) == NULL)
    _error ("new_rt_fn: allocation error.\n");

  return r;
}

/*
 *  This doesn't seem to be very useful - delete? 
 */
void __init_time (void) {
  time_t t;
  struct tm *tm_ptr;
  t = time (NULL);
  tm_ptr = localtime (&t);
}

int is_recursive_method_call (METHOD *);

int __save_rt_info (OBJECT *__rcvr_obj, OBJECT *__rcvr_class_obj, 
		    OBJECT *__method_class_obj, METHOD *method,
		    OBJECT *(*fn) (), bool inline_call) {
  RT_INFO *r;
  r = _new_rtinfo ();

  r -> inline_call = inline_call;
  r -> rcvr_obj = rtinfo.rcvr_obj = __rcvr_obj;
  r -> rcvr_class_obj = rtinfo.rcvr_class_obj = __rcvr_class_obj;
  r -> method_class_obj = rtinfo.method_class_obj = __method_class_obj;
  r -> method = method;
  r -> method_fn = rtinfo.method_fn = fn;
  r -> _arg_frame_top = __ctalk_arg_ptr + 1;
  strcpy (r -> arg_text, ctalk_arg_text ());
  if (is_recursive_method_call (r -> method)) {
    r -> local_object_cache[r -> local_obj_cache_ptr] = 
      M_LOCAL_VAR_LIST(r -> method);
    M_LOCAL_VAR_LIST(r -> method) = NULL;
    r ->  local_obj_cache_ptr++;
  }
  __call_stack[__call_stack_ptr--] = r;

  return SUCCESS;
}

static void __rrti_delete_orig_object (VARENTRY *v) {
  if (IS_OBJECT(v -> orig_object_rec)) {
    if (v -> orig_object_rec != v -> var_object) {
      if (!is_receiver (v -> orig_object_rec) &&
	  !is_arg (v -> orig_object_rec)) {
	__ctalkDeleteObject (v -> orig_object_rec);
	v -> orig_object_rec = NULL;
      }
    }
  }
}

extern void cleanup_reffed_object (OBJECT *__o, OBJECT *__r);

int __restore_rt_info (void) {

  RT_INFO *r;
  VARENTRY *__v, *__v_prev;
  METHOD *__m;

  r = __call_stack[++__call_stack_ptr];
  __call_stack[__call_stack_ptr] = NULL;

  if (__call_stack_ptr >= MAXARGS) {
    rtinfo.rcvr_obj = rtinfo.rcvr_class_obj = rtinfo.method_class_obj = NULL;
    rtinfo.method_fn = NULL;
  } else {
    rtinfo.rcvr_obj = __call_stack[__call_stack_ptr+1] -> rcvr_obj;
    rtinfo.rcvr_class_obj = __call_stack[__call_stack_ptr+1] -> rcvr_class_obj;
    rtinfo.method_class_obj = 
      __call_stack[__call_stack_ptr+1] -> method_class_obj;
    rtinfo.method_fn = __call_stack[__call_stack_ptr+1] -> method_fn;
    rtinfo.method = __call_stack[__call_stack_ptr+1] -> method;
  }
  if (is_recursive_method_call (r -> method)) {
    if (r -> method && M_LOCAL_VAR_LIST(r -> method)) {
      __m = r -> method;
      for (__v = M_LOCAL_VAR_LIST(__m);
	   __v && IS_OBJECT(__v -> var_object) && 
	     __v -> next; 
	   __v = __v -> next)
	; 
      if (__v == M_LOCAL_VAR_LIST(__m)) {
	if (M_LOCAL_VAR_LIST(__m) && M_LOCAL_VAR_LIST(__m) -> var_object) {
	  __ctalkDeleteObject (M_LOCAL_VAR_LIST(__m) -> var_object);
	}
	delete_varentry (M_LOCAL_VAR_LIST(__m));
	M_LOCAL_VAR_LIST(__m) = NULL;
      } else {
	while (__v != M_LOCAL_VAR_LIST(__m)) {
	  __v_prev = __v -> prev;
	  if (__v && __v -> var_object) {
	    if (is_receiver (__v -> var_object) ||
		is_arg (__v -> var_object)) {
	      __objRefCntDec (OBJREF(__v -> var_object));
	      __rrti_delete_orig_object (__v);
	    } else {
	      if (!(__v -> var_object -> scope & METHOD_USER_OBJECT)) {
		OBJECT *__r;
		if ((__r = obj_ref_str (__v->var_object -> __o_value)) != NULL) {
		  __v -> var_object -> __o_value[0] = '\0';
		}
		__ctalkDeleteObject (__v -> var_object);
	      } else {
		if (__v -> var_object -> __o_vartags -> tag == __v) {
		  __v -> var_object -> __o_vartags -> tag = NULL;
		}
		__rrti_delete_orig_object (__v);
	      }
	    }
	  }
	  delete_varentry (__v);
	  __v = __v_prev;
	  if (!__v) break;
	  __v -> next = NULL;
	}
	if (M_LOCAL_VAR_LIST(__m) && M_LOCAL_VAR_LIST(__m) -> var_object) { 
	  if (is_receiver (M_LOCAL_VAR_LIST(__m) -> var_object) ||
	      is_arg (M_LOCAL_VAR_LIST(__m) -> var_object)) {
	    __objRefCntDec (OBJREF(M_LOCAL_VAR_LIST(__m) -> var_object));
	  } else {
	    if (!(M_LOCAL_VAR_LIST(__m) -> var_object -> scope &
		  METHOD_USER_OBJECT)) {
	      OBJECT *__r;
	      if ((__r = 
		   obj_ref_str (M_LOCAL_VAR_LIST(__m)->var_object -> __o_value))
		  != NULL) {
		M_LOCAL_VAR_LIST(__m) -> var_object -> __o_value[0] = '\0';
	      }
	      __ctalkDeleteObject (M_LOCAL_VAR_LIST(__m) -> var_object); 
	    } else {
	      if (M_LOCAL_VAR_LIST(__m) -> var_object -> __o_vartags -> tag 
		  == __v)
		M_LOCAL_VAR_LIST(__m) -> var_object -> __o_vartags -> tag 
		  = NULL;
	    }
	  }
	  delete_varentry (M_LOCAL_VAR_LIST(__m));
	  M_LOCAL_VAR_LIST(__m) = NULL;
	}
      }
    }
    --r -> local_obj_cache_ptr;
    M_LOCAL_VAR_LIST(r -> method) =
      r -> local_object_cache[r -> local_obj_cache_ptr];
    set_ctalk_arg_text (r -> arg_text);
  }
  __xfree (MEMADDR(r));

  return SUCCESS;
}

int __get_call_stack_ptr (void) {
  return __call_stack_ptr;
}

int __current_call_stack_idx (void) {
  return __call_stack_ptr + 1;
}

bool __ctalkIsInlineCall (void) {
  /* TODO - replace rtinfo with the current call stack entry 
     everywhere else in the library. */
  return __call_stack[__call_stack_ptr+1] -> inline_call;
}

bool __ctalkCallerIsInlineCall (void) {
  return __call_stack[__call_stack_ptr + 1]->inline_call;
}

bool __ctalkIsArgBlkScope (void) {
  return __call_stack[__call_stack_ptr + 1]->block_scope;
}

RT_INFO *_new_rtinfo (void) {
  static RT_INFO *r;
  r = (RT_INFO *)__xalloc (sizeof (struct _rtinfo));
  return r;
}

void _delete_rtinfo (RT_INFO *r) {
  __xfree (MEMADDR(r));
}

/*
 *  Returns the call stack index of the previous method
 *  call, or zero.
 */
int is_recursive_method_call (METHOD *m) {
  int i;
  for (i = __call_stack_ptr + 1; i <= MAXARGS; i++) {
    if (__call_stack[i] && (m == __call_stack[i]->method))
      return i;
  }
  return FALSE;
}

/*
 *  The *successive_call* functions are called only from
 *  __ctalkInitLocalObjects at the moment.  Local objects are restored
 *  in __restore_rt_info (), above.
 *
 *  See the notes for last_eval_result () in rt_expr.c. 
 */
int is_successive_method_call (METHOD *m) {
  VARENTRY *v;
  int i;

  for (v = M_LOCAL_VAR_LIST(m); v; v = v -> next) {
    if (IS_OBJECT(v -> var_object)) {
      if (v -> var_object && 
	  (is_receiver (v -> var_object) || is_arg (v -> var_object)))
	return TRUE;

      if (v -> var_object == last_eval_result ())
	return TRUE;

      if (expr_n_occurrences (m) > 1)
	return TRUE;
    }
  }
  return FALSE;
}

void register_successive_method_call (void) {
  __call_stack[__call_stack_ptr + 1] -> _successive_call = TRUE;
  __call_stack[__call_stack_ptr + 1] -> method -> nth_local_ptr += 1;
}

void register_arg_active_varentry (VARENTRY *v) {
  __call_stack[__call_stack_ptr + 1] -> arg_active_tag = v;
}

void clear_arg_active_varentry (void) {
  if (__call_stack_ptr < MAXARGS)
    __call_stack[__call_stack_ptr + 1] -> arg_active_tag = NULL;
}

/*
 *  When check from within a __ctalk_method () call, __ctalk_method ()
 *  has added another RTINFO entry, so the current rtinfo entry when
 *  the arg is pushed is the method call's RTINFO entry + 1.
 */
VARENTRY *arg_active_varentry (void) {
  if (__call_stack_ptr < (MAXARGS - 1))
    return __call_stack[__call_stack_ptr + 2] -> arg_active_tag;
  else
    return NULL;
}

static void init_session (char *);

extern bool char_size_1; /* declared in rtxalloc.c. */

extern void init_object_size (void);

void __rt_init_library_paths (void) {
  char libdir[FILENAME_MAX],
    libenvdir[FILENAME_MAX],
    libenvdir2[FILENAME_MAX],
    libenvdir3[FILENAME_MAX],
    *libenv, *p, *q;
  int n_paths = 0;

  if ((libenv = getenv ("CLASSLIBDIRS")) != NULL) {
    p = libenv;
    do {
      q = strchr (p, ':');
      if (q) {
	substrcpy (libenvdir, p, 0, q - p);
	p = q + 1;
      } else {
	strcpy (libenvdir, p);
      }
      expand_path (libenvdir, libenvdir2);
      library_include_paths[n_paths++] = strdup (libenvdir2);
      strcatx (libenvdir3, libenvdir2, "/", pkgname, NULL);
      if (is_dir (libenvdir3)) {
	library_include_paths[n_paths++] = strdup (libenvdir3);
      }
    } while (q != NULL);
  }

  library_include_paths[n_paths++] = strdup (classlibdir);

  strcatx (libdir, classlibdir, "/", pkgname, NULL);
  if (is_dir (libdir)) {
    library_include_paths[n_paths++] = strdup (libdir);
  }

  get_stdin_stdout_termios ();
  init_session (argv_name);

  if (sizeof (char) != 1) {
    char_size_1 = false;
  }

  init_object_size ();
}

/*
 *  Should only be called by error ().  
 */
void cleanup_rt_fns (void) {
  int i;
  RT_FN *r;
  for (i = __call_stack_ptr + 1; i <= MAXARGS; i++) {
    if ((r = __call_stack[i] -> _rt_fn) != NULL) {
      if (r -> local_objects.vars) {
	DELETE_VAR_LIST (r -> local_objects.vars);
      }
    }
  }
}

int __ctalkNArgs(void) {
  int arg_frame_top_1, arg_frame_top_2;
  if (__call_stack_ptr <= 510) {
    return __call_stack[__call_stack_ptr+2] -> _arg_frame_top - 
      __call_stack[__call_stack_ptr+1] -> _arg_frame_top;
  } else {
    return ERROR;
  }
}

struct termios stdout_term;
struct termios stdin_term;
static bool have_termios = false;

void get_stdin_stdout_termios (void) {
  if (!have_termios) {
    tcgetattr (fileno (stdout), &stdout_term);
    tcgetattr (fileno (stdin), &stdin_term);
    have_termios = true;
  }
}

void restore_stdin_stdout_termios (void) {
  if (have_termios) {
    tcsetattr (fileno (stdout), TCSADRAIN, &stdout_term);
    tcsetattr (fileno (stdin), TCSADRAIN, &stdin_term);
  }
}

static bool str_rcvr_is_modified = false;
static bool __need_rcvr_mod_catch = false;

/*
 *  Used in functions like __ctalkStringifyName (), which
 *  are called by methods like String : setEqual, etc.
 */
bool str_rcvr_mod (void) {
  return str_rcvr_is_modified;
}

void register_str_rcvr_mod (bool b) {
  if (__need_rcvr_mod_catch)
    str_rcvr_is_modified = b;
}

void set_rcvr_mod_catch (void) {
  __need_rcvr_mod_catch = true;
}

void clear_rcvr_mod_catch (void) {
  __need_rcvr_mod_catch = false;
}

bool need_rcvr_mod_catch (void) {
  return __need_rcvr_mod_catch;
}

static bool __need_postfix_fetch_update = false;

void register_postfix_fetch_update (void) {
  __need_postfix_fetch_update = true;
}

/* note that i_post gets cleared to I_UNDEF in active_i, so if
   active_i is called before need_postfix_fetch_update, then 
   we also need to call clear_postfix_fetch_update ourselves. */
bool need_postfix_fetch_update (void) {
  return __need_postfix_fetch_update;
}

void clear_postfix_fetch_update (void) {
  __need_postfix_fetch_update = false;
}

static bool __string_needs_assignment_by_value = false;

void register_string_assign_by_value (void) {
  __string_needs_assignment_by_value = true;
}

bool assign_string_by_value (void) {
  return __string_needs_assignment_by_value;
}

void clear_string_assign_by_value (void) {
  __string_needs_assignment_by_value = false;
}

char *__ctalkInstallPrefix (void) {
  return installprefix;
}

int __ctalkSystemSignalNumber (char *);

/* All this does right now is kill any process with the same name as
   ours if it's a zombie process - usually this happens when we exit
   a desktop session with a window manager that simply reparents
   the X clients to the root window - Ctalk's X apps don't handle
   a second reparenting well (not yet at least), and so they end
   up as zombies by the next session. 

   Linux with /proc filesystems only - this is not very applicable to
   OSX, which kills everything anyway when you exit the X app.
*/
static void init_session (char *argvname) {
#ifdef linux
  char *cmdname;
  char buf[MAXMSG], statpath[FILENAME_MAX];
  struct stat statbuf;
  DIR *procdir;
  struct dirent *dirent_p;
  FILE *statf;
  int target_pid, last_pid, our_pid;
  
  if ((cmdname = strrchr (argvname, '/')) == NULL) {
    cmdname = argvname;
  } else {
    ++cmdname;
  }

  if (stat ("/proc", &statbuf)) {
    return;
  }

  if ((procdir = opendir ("/proc")) == NULL) {
    return;
  }

  our_pid = getpid ();

  while ((dirent_p = readdir (procdir)) != NULL) {
    if (!isdigit ((int)dirent_p -> d_name[0])) {
      /* only digits in the /proc/[pid] subdir names */
      continue;
    }
    strcatx (statpath, "/proc/", dirent_p -> d_name, "/stat", NULL); 
    if ((statf = fopen (statpath, "r")) != NULL) {
      if (fgets (buf, MAXMSG, statf)) {
	/* If we find our commandname on a zombie process, kill it
	   and the previous process entry with our command name. */
	if (strstr (buf, cmdname)) {
	  target_pid = atoi (dirent_p -> d_name);
	  if (target_pid == our_pid) {
	    /* don't do anything to ourselves, we're just starting. */
	    continue;
	  }
	  if (strstr (buf, " Z ")) {
	    kill (last_pid, __ctalkSystemSignalNumber ("SIGTERM"));
	    kill (target_pid, __ctalkSystemSignalNumber ("SIGTERM"));
	  } else {
	    last_pid = target_pid;
	  }
	}
      }
      fclose (statf);
    }
  }

  closedir (procdir);

#endif /* linux */
}

int __ctalk_exitFn (int app_exit) {
  return __ctalk_exitFn_internal (app_exit, true);
}

int __ctalk_process_exitFn (int app_exit) {
  return __ctalk_exitFn_internal (app_exit, false);
}

char *__ctalkDocDir (void) {
  return DOCDIR;
}
