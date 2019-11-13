/* $Id: rt_signal.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012  Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#include <unistd.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include <signal.h>

extern char *__ctalkSystemSignalName (int);
extern int __ctalkSystemSignalNumber (char *);

int __ctalkIgnoreSignal (int signo) {
#if defined(__sparc__) && defined(__svr4__)
  sigignore (signo);
#else
  signal (signo, SIG_IGN);
#endif
  return SUCCESS;
}

int __ctalkDefaultSignalHandler (int signo) {
#if defined(__sparc__) && defined(__svr4__)
  sigignore (signo);
#else
  signal (signo, SIG_DFL);
#endif
  return SUCCESS;
}

int __ctalkInstallHandler (int signo, OBJECT *(*method_cfn)()) {
  struct sigaction s, old_s;
#if defined(__DJGPP__)
  s.sa_flags = 0;
#else
  s.sa_flags = SA_RESETHAND;
#endif
  sigemptyset (&(s.sa_mask));
  s.sa_handler = (void (*)())method_cfn;
  sigaction (signo, &s, &old_s);
  return SUCCESS;
}

void __ctalkSignalHandlerBasic (int signo) {
  printf ("\n%s: Caught %s.\n", __argvFileName (), 
	  __ctalkSystemSignalName (signo));
  if (__ctalkGetExceptionTrace()) 
    __warning_trace ();
  __ctalk_exitFn(1);
  __ctalkDefaultSignalHandler (signo);
  kill (0, signo);
  /*
   *  Make sure the program finishes terminating.
   */
  usleep (1000);
  kill (0, __ctalkSystemSignalNumber ("SIGKILL"));
}

#include <sys/wait.h>

