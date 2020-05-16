/* $Id: unixsock.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2017, 2019  Robert Kiesling, 
    rk3314042@gmail.com.
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "typeof.h"
#include "defcls.h"


#if defined HAVE_SYS_SOCKET_H && defined HAVE_SYS_UN_H

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

int __ctalkUNIXSocketShutdown (int sock, int how) {
  return shutdown (sock, how);
}

int __ctalkUNIXSocketOpenReader (char *sockpath) {
  struct sockaddr_un addr;
  int s;
  s = socket (AF_UNIX, SOCK_DGRAM, 0);
  if (s < 0) {
    return -1;
  }
  strncpy (addr.sun_path, sockpath, sizeof(addr.sun_path) - 1);
  addr.sun_family = AF_UNIX;
  if (bind (s, (struct sockaddr *)&addr,
	    sizeof (struct sockaddr_un))  == -1) {
    return -1;
  }
  listen (s, SOMAXCONN);
  return s;
}

int __ctalkUNIXSocketOpenWriter (char *sockpath) {
  struct sockaddr_un addr;
  int s;
  s = socket (AF_UNIX, SOCK_DGRAM, 0);
  if (s < 0) {
    return ERROR;
  }

  strncpy (addr.sun_path, sockpath, sizeof(addr.sun_path) - 1);

#ifdef __APPLE__
  addr.sun_len = strlen (addr.sun_path) + 1;
  addr.sun_family = AF_UNIX;
  if (connect (s, (struct sockaddr *)&addr,
	       sizeof (struct sockaddr_un))
      == -1) {
    return ERROR;
  }
#else  /* #ifdef __APPLE__ */
  addr.sun_family = AF_UNIX;
  if (connect (s, (struct sockaddr *)&addr,
	       strlen (addr.sun_path) +
	       sizeof (addr.sun_family))
      == -1) {
    return ERROR;
  }
#endif  /* #ifdef __APPLE__ */
  return s;
}

int __ctalkUNIXSocketRead (int sockfd, void *buf) {
  struct timeval timeout;
  fd_set read_set;
  int r, chars_read = 0;

  timeout.tv_sec = 0;
  timeout.tv_usec = 500;
  FD_ZERO (&read_set);
  FD_SET (sockfd, &read_set);
  r = select (sockfd + 1, &read_set, (fd_set *)NULL, (fd_set *)NULL, &timeout);
  if (r > 0) {
    chars_read = read (sockfd, buf, MAXMSG);
    if (chars_read != ERROR) {
      ((char *)buf)[chars_read] = 0;  /* In case there is junk after the end. */
    }
  } else if (r == ERROR) {
    return ERROR;
  } 
  return chars_read;
}

int __ctalkUNIXSocketWrite (int sockfd, void *data, int length) {
  int r;
  r = write (sockfd, data, length);
  if (r != length) {
    return -1;
  } else {
    return r;
  }
 }

#else /* defined HAVE_SYS_SOCKET_H && defined HAVE_SYS_UN_H */

static void unix_socket_support (void) {
  printf ("UNIXNetworkStream classes require you to build Ctalk with "
	  "UNIX domain socket support.  Refer to the file README for "
	  "more information.\n");
}

int __ctalkUNIXSocketShutdown (int sock, int how) {
  unix_socket_support ();
  return -1;
}

int __ctalkUNIXSocketOpenReader (char *sockpath) {
  unix_socket_support ();
  return -1;
}

int __ctalkUNIXSocketOpenWriter (char *sockpath) {
  unix_socket_support ();
  return -1;
}

int __ctalkUNIXSocketRead (int sockfd, void *buf, int *len_out) {
  unix_socket_support ();
  return -1;
}

int __ctalkUNIXSocketWrite (int sockfd, void *data, int length) {
  unix_socket_support ();
  return -1;
}

#endif /* defined HAVE_SYS_SOCKET_H && defined HAVE_SYS_UN_H */
