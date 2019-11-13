/* $Id: termsize.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014  Robert Kiesling, rk3314042@gmail.com.
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
#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

/*
 * This has only been tested on Linux.
 */

static int __get_window_size (int fd, struct winsize *w) {
  
  return ioctl (fd, TIOCGWINSZ, (char *)w);

}

int __ctalkTerminalWidth (void) {
#if defined TIOCGWINSZ
  struct winsize win;

  if (!__get_window_size (fileno (stdin), &win))
    return win.ws_col;
  else
    return ERROR;
#else
  return 0;
#endif
}

int __ctalkTerminalHeight (void) {
#if defined TIOCGWINSZ
  struct winsize win;

  if (!__get_window_size (fileno (stdin), &win))
    return win.ws_row;
  else
    return ERROR;
#else
  return 0;
#endif
}

