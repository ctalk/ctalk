/* $Id: spawn_bin.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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

/*
 *  This is a fairly generic daemon launcher - it launches the
 *  daemon process, and the session process - which detaches
 *  the daemon from the terminal I/O.  The process controlling
 *  the daemon session waits until the parent process exits before
 *  exiting also - when the parent exits, it gets reparented to
 *  init.  This lets us be more flexible with the parent process
 *  hanging around without its IO channels interfering with the
 *  daemon, and vice versa.
 *
 *  There is a also UNIX socket which the daemon uses to inform the
 *  parent process of its process id, which gets returned to the
 *  program.
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#define SOCKADDR ".ctexecbin"
int sock;
char sock_name[FILENAME_MAX];
struct sockaddr_un addr;

static char *ct_argv[MAXARGS] = {0, };
static char *ct_argv_1[MAXARGS] = {0, };

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
	/* strcat (str_buf, ct_argv_1[i++], NULL);
	   strcat (str_buf, " "); */
	strcatx2 (str_buf, ct_argv_1[i++], " ", NULL);
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
      /* if (!get_stdout_target (tmpbuf)) */
      ct_argv_1[i++] = strdup (tmpbuf);
      p = q + 1;
    } else {
      memset (tmpbuf, 0, MAXMSG);
      strcpy (tmpbuf, p);
      /* if (!get_stdout_target (tmpbuf)) */
	ct_argv_1[i++] = strdup (tmpbuf);
      break;
    }
  }
  collect_strings ();
}

int open_socket (void) {

  sprintf (sock_name, "%s/%s", P_tmpdir, SOCKADDR);
  if ((sock = socket (AF_UNIX, SOCK_DGRAM, 0)) == -1) {
    printf ("ctalk: %s\n", strerror (errno));
    return -1;
  }
  strncpy 
    (addr.sun_path, sock_name,
     sizeof(addr.sun_path) - 1);
  addr.sun_family = AF_UNIX;

  return 0;
}

int read_sock (void) {
  int pid_ret;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 20;
  fd_set read_set;
  int r;
  int read_sock;
  char buf[0xff];

  if (bind (sock, (struct sockaddr *)&addr, 
	    sizeof (struct sockaddr_un)) == -1) {
    if (errno) {
      printf ("ctalk (bind): %s\n", strerror (errno));
      return ERROR;
    }
  }
  listen  (sock, 5);

  FD_ZERO (&read_set);
  FD_SET (sock, &read_set);
  r = select (sock + 1, &read_set, 
	      (fd_set *)0, (fd_set *)0,
	      &timeout);
  if (r >= 0) {
    if ((r = read (sock, buf, 0xff)) > 0) {
      buf[r] = 0;
      pid_ret = atoi (buf);
    } else {
      printf ("ctalk (read): %s\n", strerror (errno));
    }
    close (read_sock);
  } else {
    printf ("ctalk (select): %s\n", strerror (errno));
  }
  return pid_ret;
}

void write_sock (int pid) {
  int r;
  char pid_buf[16];
  ctitoa (pid, pid_buf);

  if (connect (sock, 
	       (struct sockaddr *)&addr, 
	       strlen (addr.sun_path) + 
	       sizeof (addr.sun_family))
      == 1) {
    if (errno) {
      printf ("ctalk: %s\n", strerror (errno));
      return;
    }
  }

  sprintf (pid_buf, "%d", pid);
  if ((r = write (sock, 
		  (const void *)pid_buf, 
		  strlen (pid_buf)))
      < 0) {
  }
  return;
}

int spawn_bin (char *cmd, int restrict_io) {
  int session_pid, daemon_pid, child_pid;

  if (open_socket () < 0) {
    printf ("ctalk: %s\n", strerror (errno));
    return ERROR;
  }

  session_pid = fork ();

  switch (session_pid)
    {
    case -1:
      printf ("ctalk: %s.", strerror (errno));
      exit (-1);
      break;
    case 0:
      break;
    default:
      child_pid = read_sock ();
      close (sock);
      unlink (sock_name);
      return child_pid;
      break;
    }

  if (restrict_io) {
    if (chdir ("/")) {
      printf ("ctalk: (chdir): %s\n", strerror (errno));
      exit (-1);
    }
    umask (0);
  }
  setsid ();
  close (STDIN_FILENO); close (STDOUT_FILENO); close (STDERR_FILENO);

  daemon_pid = fork ();

  switch (daemon_pid)
    {
    case -1:
      exit (-1);
      break;
    case 0:
      break;
    default:
      while (1) {
	if (getppid () == 1)  /* when the parent exits, the orphaned */
	  break;              /* process gets reparented to init, so */
	                      /* we can exit and leave just the      */
	                      /* daemon process.                     */
	else
	  sleep (1);
      }
      _exit (0);
      break;
    }

  write_sock (getpid ());

  _split_cmd_basic (cmd, 0);
  execv (ct_argv[0], ct_argv);

  return SUCCESS;
}
