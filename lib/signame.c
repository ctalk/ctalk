/* $Id: signame.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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

/*
 *  Only includes the POSIX 1990 names.  Other signals return
 *  the string, "signal n."
 */
#include <stdio.h>
#include <signal.h>
#include <string.h>

#define MAXLABEL 0x100  /* From ctalk.h */

void _warning (char *, ...);

char *__ctalkSystemSignalName (int signo) {
    static char signame[MAXLABEL];

#if defined(__DJGPP__)
    switch (signo)
      {
      case SIGHUP:
	strcpy (signame, "SIGHUP");
	break;
      case SIGINT:
	strcpy (signame, "SIGINT");
	break;
      case SIGQUIT:
	strcpy (signame, "SIGQUIT");
	break;
      case SIGILL:
	strcpy (signame, "SIGILL");
	break;
      case SIGTRAP:
	strcpy (signame, "SIGTRAP");
	break;
      case SIGABRT:
	strcpy (signame, "SIGABRT");
	break;
      case SIGFPE:
	strcpy (signame, "SIGFPE");
	break;
      case SIGKILL:
	strcpy (signame, "SIGKILL");
	break;
      case SIGSEGV:
	strcpy (signame, "SIGSEGV");
	break;
      case SIGPIPE:
	strcpy (signame, "SIGPIPE");
	break;
      case SIGALRM:
	strcpy (signame, "SIGALRM");
	break;
      case SIGTERM:
	strcpy (signame, "SIGTERM");
	break;
      case SIGUSR1:
	strcpy (signame, "SIGUSR1");
	break;
      case SIGUSR2:
	strcpy (signame, "SIGUSR2");
	break;
      default:
	sprintf (signame, "signal %d", signo);
	break;
      }
#else
    switch (signo)
      {
      case SIGHUP:
	strcpy (signame, "SIGHUP");
	break;
      case SIGINT:
	strcpy (signame, "SIGINT");
	break;
      case SIGQUIT:
	strcpy (signame, "SIGQUIT");
	break;
      case SIGILL:
	strcpy (signame, "SIGILL");
	break;
      case SIGTRAP:
	strcpy (signame, "SIGTRAP");
	break;
      case SIGABRT:
	strcpy (signame, "SIGABRT");
	break;
      case SIGFPE:
	strcpy (signame, "SIGFPE");
	break;
      case SIGKILL:
	strcpy (signame, "SIGKILL");
	break;
      case SIGSEGV:
	strcpy (signame, "SIGSEGV");
	break;
      case SIGPIPE:
	strcpy (signame, "SIGPIPE");
	break;
      case SIGALRM:
	strcpy (signame, "SIGALRM");
	break;
      case SIGTERM:
	strcpy (signame, "SIGTERM");
	break;
      case SIGUSR1:
	strcpy (signame, "SIGUSR1");
	break;
      case SIGUSR2:
	strcpy (signame, "SIGUSR2");
	break;
      case SIGCHLD:
	strcpy (signame, "SIGCHLD");
	break;
      case SIGCONT:
	strcpy (signame, "SIGCONT");
	break;
      case SIGSTOP:
	strcpy (signame, "SIGSTOP");
	break;
      case SIGTSTP:
	strcpy (signame, "SIGTSTP");
	break;
      case SIGTTIN:
	strcpy (signame, "SIGTTIN");
	break;
      case SIGTTOU:
	strcpy (signame, "SIGTTOU");
	break;
      default:
	sprintf (signame, "signal %d", signo);
	break;
      }
#endif
    return signame;
}

int __ctalkSystemSignalNumber (char *signame) {
  if (!strcmp (signame, "SIGHUP")) return SIGHUP;
  if (!strcmp (signame, "SIGINT")) return SIGINT;
  if (!strcmp (signame, "SIGQUIT")) return SIGQUIT;
  if (!strcmp (signame, "SIGILL")) return SIGILL;
  if (!strcmp (signame, "SIGTRAP")) return SIGTRAP;
  if (!strcmp (signame, "SIGABRT")) return SIGABRT;
  if (!strcmp (signame, "SIGFPE")) return SIGFPE;
  if (!strcmp (signame, "SIGKILL")) return SIGKILL;
  if (!strcmp (signame, "SIGSEGV")) return SIGSEGV;
  if (!strcmp (signame, "SIGPIPE")) return SIGPIPE;
  if (!strcmp (signame, "SIGALRM")) return SIGALRM;
  if (!strcmp (signame, "SIGTERM")) return SIGTERM;
  if (!strcmp (signame, "SIGUSR1")) return SIGUSR1;
  if (!strcmp (signame, "SIGUSR2")) return SIGUSR2;
#ifndef DJGPP
  if (!strcmp (signame, "SIGCHLD")) return SIGCHLD;
  if (!strcmp (signame, "SIGCONT")) return SIGCONT;
  if (!strcmp (signame, "SIGSTOP")) return SIGSTOP;
  if (!strcmp (signame, "SIGTSTP")) return SIGTSTP;
  if (!strcmp (signame, "SIGTTIN")) return SIGTTIN;
  if (!strcmp (signame, "SIGTTOU")) return SIGTTOU;
#endif /* DJGPP */

  _warning ("__ctalkSystemSigNum: Unimplemented signal, \"%s.\"\n");
  return SIGKILL;
}
