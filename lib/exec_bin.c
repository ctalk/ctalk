/* $Id: exec_bin.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2016-2017 Robert Kiesling, rk3314042@gmail.com.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <glob.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#define SHELL_CHARS "|<>&"
#define SHELL "/bin/bash"


#if defined (__linux__) || defined (__MACH__) || defined (__unix__)

static char *ct_argv_1[MAXARGS] = {0, };
static char *ct_argv[MAXARGS] = {0, };
static bool stdout_redir = false;
static bool need_stdout_target = false;
static bool have_glob = false;
static char stdout_target[FILENAME_MAX];
static char stdout_mode[2];

glob_t glob_buf;

static bool has_glob_chars (char *s) {
  return strpbrk (s, "*?['\"");
}

static bool get_stdout_target (char *s) {
  if (strstr (s, ">>")) {
    stdout_redir = true;
    need_stdout_target = true;
    strcpy (stdout_mode, "a");
    return true;
  } else if (strchr (s, '>')) {
    stdout_redir = true;
    need_stdout_target = true;
    strcpy (stdout_mode, "w");
    return true;
  } else if (need_stdout_target) {
    expand_path (s, stdout_target);
    need_stdout_target = false;
    return true;
  }
  return false;
}

/* moves ct_argv_1 to ct_argv whether or not there are any strings.*/
static void collect_strings (void) {
  int i, j;
  bool in_string = false;
  char str_buf[MAXMSG];
  memset (str_buf, 0, MAXMSG);
  i = j = 0;
  while (ct_argv_1[i]) {
    if (strpbrk (ct_argv_1[i], "\"'")) {
      if (!in_string) {
	in_string = true;
	strcatx2 (str_buf, ct_argv_1[i++], " ",NULL);
      } else {
	in_string = false;
	strcatx2 (str_buf, ct_argv_1[i++], NULL);
	ct_argv[j++] = strdup (str_buf);
	memset (str_buf, 0, MAXMSG);
      }
    } else {
      if (in_string) {
	strcatx2 (str_buf, ct_argv_1[i++], " ", NULL);
      } else {
	ct_argv[j++] = strdup (ct_argv_1[i++]);
      }
    }
  }
  /* if we have a token with both quotes, it won't get un-diverted
     until here. */
  if (*str_buf != 0) {
    ct_argv[j++] = strdup (str_buf);
    memset (str_buf, 0, MAXMSG);
  }
}

static void _split_cmd_basic (char *cmdline, int start_idx) {
  int i;
  char *p, *q;
  char tmpbuf[MAXMSG];
  p = cmdline;
  i = start_idx;
  while (1) {
    if ((q = strchr (p, ' ')) != NULL) {
      memset (tmpbuf, 0, MAXMSG);
      strncpy (tmpbuf, p, q - p);
      if (!get_stdout_target (tmpbuf))
	ct_argv_1[i++] = strdup (tmpbuf);
      p = q + 1;
    } else {
      memset (tmpbuf, 0, MAXMSG);
      strcpy (tmpbuf, p);
      if (!get_stdout_target (tmpbuf))
	ct_argv_1[i++] = strdup (tmpbuf);
      break;
    }
  }
  collect_strings ();
}

static void _split_cmd_glob (char *cmdline) {
  int i, gl_offset, flags;
  char *p, *q;
  char tmpbuf[MAXMSG];
  p = cmdline;
  flags = i = 0;
  while (1) {
    if ((q = strchr (p, ' ')) != NULL) {
      memset (tmpbuf, 0, MAXMSG);
      strncpy (tmpbuf, p, q - p);
      ct_argv_1[i++] = strdup (tmpbuf);
      p = q + 1;
    } else {
      memset (tmpbuf, 0, MAXMSG);
      strcpy (tmpbuf, p);
      ct_argv_1[i++] = strdup (tmpbuf);
      break;
    }
  }
  collect_strings ();
  for (i = 0; ct_argv[i]; ++i)
    if (has_glob_chars (ct_argv_1[i]))
      break;
  glob_buf.gl_offs = i;
  flags = GLOB_DOOFFS|GLOB_NOSORT|GLOB_NOCHECK;
  for (i = glob_buf.gl_offs; ct_argv[i]; ++i) {
    if (has_glob_chars (ct_argv[i])) {
      /* now we just have to trim the quotes off to leave a
	 (possibly) multi-word pattern ... */
      if (*ct_argv[i] == '"') {
	strcpy (tmpbuf, ct_argv[i]);
	TRIM_LITERAL(tmpbuf);
	if ((p = strchr (tmpbuf, '"')) != NULL) {
	  /* if we left an extra space at the end of the
	     string, then TRIM_LITERAL won't remove it,
	     so do it ourselves. */
	  *p = 0;
	}
	glob (tmpbuf, flags, NULL, &glob_buf);
      } else if (*ct_argv[i] == '\'') {
	strcpy (tmpbuf, ct_argv[i]);
	TRIM_CHAR_BUF(tmpbuf);
	if ((p = strchr (tmpbuf, '\'')) != NULL) {
	  /* here, too. */
	  *p = 0;
	}
	glob (tmpbuf, flags, NULL, &glob_buf);
      } else {
	glob (ct_argv[i], flags, NULL, &glob_buf);
      }
      flags |= GLOB_APPEND;
    }
  }
  for (i = 0; i < glob_buf.gl_offs; ++i) {
    glob_buf.gl_pathv[i] = strdup (ct_argv[i]);
  }
}

static int _shell_script (char *filepath) {
  char buf[MAXMSG];
  if (file_has_exec_permissions (filepath)) {
    return system (filepath);
  } else {
    sprintf (buf, "%s %s", SHELL, filepath);
    return system (buf);
  }
}

static int split_cmd (char *cmdline) {
  stdout_redir = need_stdout_target = have_glob = false;
  if (is_shell_script (cmdline)) {
    return _shell_script (cmdline);
  } else if (has_glob_chars (cmdline)) {
    have_glob = true;
    _split_cmd_glob (cmdline);
  } else {
    _split_cmd_basic (cmdline, 0);
  }
  return 0;
}

static void split_cmd_cleanup (void) {
  int i;
  for (i = 0; (ct_argv_1[i] || ct_argv[i]); i++) {
    if (ct_argv[i])
      __xfree (MEMADDR(ct_argv[i]));
    if (ct_argv_1[i])
      __xfree (MEMADDR(ct_argv[i]));
  }
  memset (ct_argv, 0, (MAXARGS * sizeof (char *)));
  memset (ct_argv_1, 0, (MAXARGS * sizeof (char *)));
  if (have_glob)
    globfree (&glob_buf);
}


/* the recipe for reading the subproccess's output from pipes
   is from www.microhowto.info and tfmps. */
int exec_bin (char *cmd) {

  int pipefds[2];
  pid_t child_pid;
  int retval;
  int count;
  int i;
  int r;
  int waitstatus;
  char readbuf[MAXMSG];
  FILE *f_out = NULL;

  if (pipe (pipefds) == -1) {
    printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
    exit (1);
  }

  for (i = 0; i < 2; ++i) {
    if (fcntl (pipefds[i], F_SETFD, O_NONBLOCK) == -1) {
      printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
      exit (1);
    }
  }

  if (split_cmd (cmd) != 0)
    return SUCCESS;  /* we've used system to run a shell script. */

  child_pid = fork ();

  switch (child_pid)
    {
    case -1:
      printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
      exit (1);
      break;
    case 0:
      /* dup2 closes the original pipefds[1] */
      while ((dup2 (pipefds[1], STDOUT_FILENO) == -1) &&
	 (errno == EINTR)) {}
      close (pipefds[0]);
      if (have_glob) {
	execv (glob_buf.gl_pathv[0], &glob_buf.gl_pathv[0]);
      } else {
	execv (ct_argv[0], ct_argv);
      }
      /* Only reached if execv fails. */
      printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
      break;
    default:
      if (stdout_redir) {
	if ((f_out = fopen (stdout_target, stdout_mode)) == NULL) {
	  fprintf (stderr, "exec_bin (fopen): error in subprocess: "
		   "cmd=%s, waitstatus = %d, ret = %d: %s.\n", 
		   cmd, waitstatus, r, strerror (errno));
	  kill (0, SIGKILL);
	  return -2;  /* Not necessarily reached. */
	}
      }
      close (pipefds[1]);
      while (1) {
	r = waitpid (-1, &waitstatus, WNOHANG);
	if (((r > 0) && !WIFEXITED(waitstatus)) || (r == -1)) {
	  fprintf (stderr, "exec_bin (waitpid): error in subprocess: "
		   "cmd=%s, waitstatus = %d, ret = %d.\n", 
		   cmd, waitstatus, r);
	  __warning_trace ();
	  kill (0, SIGKILL);
	  return -2;  /* Not necessarily reached. */
	}
	count = read (pipefds[0], readbuf, sizeof(readbuf));
	if (count == -1) {
	  switch (errno)
	    {
	    case EINTR:
	      continue;
	      break;
	    case EAGAIN:
	    default:
	      printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
	      exit (1);
	      break;
	    }
	} else if (count == 0) {
	  break;
	} else {
	  if (stdout_redir) {
	    fwrite (readbuf, sizeof (char), count, f_out);
	  } else {
	    printf ("%s", readbuf);
	  }
	}
      }
      break;
    }
  
  close (pipefds[0]);
  retval = (wait (0) == -1) ? -1 : 0;
  split_cmd_cleanup ();
  if (f_out != NULL) {
    *stdout_target = 0;
    stdout_redir = false;
    fclose (f_out);
  }
  
  return retval;
}

/* str_object should be a String object, but the function
   itself works with any class.  str_object should already
   be the value instance variable, but we'll check anyway. */
int exec_bin_to_buf (char *cmd, OBJECT *str_object) {

  int pipefds[2];
  pid_t child_pid;
  int retval;
  int count;
  int total_count;
  int i;
  int r;
  int waitstatus;
  char readbuf[MAXMSG];
  int x_max_size = MAXMSG;
  FILE *f_out = NULL;
  OBJECT *str_object_value;

  if (IS_VALUE_INSTANCE_VAR(str_object))
    str_object_value = str_object;
  else
    str_object_value = str_object -> instancevars;

  __xfree (MEMADDR(str_object_value -> __o_value));
  str_object -> __o_value = __xalloc (MAXMSG);

  if (pipe (pipefds) == -1) {
    printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
    exit (1);
  }

  for (i = 0; i < 2; ++i) {
    if (fcntl (pipefds[i], F_SETFD, O_NONBLOCK) == -1) {
      printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
      exit (1);
    }
  }

  if (split_cmd (cmd) != 0)
    return SUCCESS;
  
  child_pid = fork ();

  switch (child_pid)
    {
    case -1:
      printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
      exit (1);
      break;
    case 0:
      /* here, too, dup2 closes the original pipefds[1] */
      while ((dup2 (pipefds[1], STDOUT_FILENO) == -1) &&
	 (errno == EINTR)) {}
      close (pipefds[0]);
      if (have_glob) {
	execv (glob_buf.gl_pathv[0], &glob_buf.gl_pathv[0]);
      } else {
	execv (ct_argv[0], ct_argv);
      }
      printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
      break;
    default:
      if (stdout_redir) {
	if ((f_out = fopen (stdout_target, stdout_mode)) == NULL) {
	  fprintf (stderr, "exec_bin (fopen): error in subprocess: "
		   "cmd=%s, waitstatus = %d, ret = %d: %s.\n", 
		   cmd, waitstatus, r, strerror (errno));
	  kill (0, SIGKILL);
	  return -2;  /* Not necessarily reached. */
	}
      }
      close (pipefds[1]);
      total_count = 0;
      while (1) {
	r = waitpid (-1, &waitstatus, WNOHANG);
	if (((r > 0) && !WIFEXITED(waitstatus)) || (r == -1)) {
	  fprintf (stderr, "exec_bin (waitpid): error in subprocess: "
		   "cmd=%s, waitstatus = %d, ret = %d.\n", 
		   cmd, waitstatus, r);
	  __warning_trace ();
	  kill (0, SIGKILL);
	  return -2;  /* Not necessarily reached. */
	}
	count = read (pipefds[0], readbuf, sizeof(readbuf));
	if (count == -1) {
	  switch (errno)
	    {
	    case EINTR:
	      continue;
	      break;
	    case EAGAIN:
	    default:
	      printf ("ctalk: %s: %s.\n", cmd, strerror (errno));
	      exit (1);
	      break;
	    }
	} else if (count == 0) {
	  break;
	} else {
	  if (stdout_redir) {
	    fwrite (readbuf, sizeof (char), count, f_out);
	  } else {
	    strncat (str_object_value -> __o_value, readbuf, count);
	    total_count += count;
	    if (total_count >= x_max_size) {
	      x_max_size *= 2;
	      __xrealloc ((void **)&(str_object_value->__o_value),
			  x_max_size);
	    }
	  }
	}
      }
      break;
    }
  
  close (pipefds[0]);
  retval = (wait (0) == -1) ? -1 : 0;
  split_cmd_cleanup ();
  if (f_out != NULL) {
    *stdout_target = 0;
    stdout_redir = false;
    fclose (f_out);
  }
  
  return retval;
}

#endif /*#if defined(__linux__) || defined(__MACH__) || defined(__unix__)*/


