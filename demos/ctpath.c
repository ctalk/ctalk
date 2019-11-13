/* $Id: ctpath.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012 Robert Kiesling, rk3314042@gmail.com.
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
 *   ctpath.c - Split a path into directories.  Does not handle shell
 *   expansions like ~/user or ~user, or . and .. directory links. 
 */

int main (int argc, char **argv) {

  String new path;
  Array new paths;
  Integer new nItems;
  Integer new i;
  Character new separator;

  if (argc != 2) {
    printf ("Usage: ctpath <path>\n");
    exit (1);
  }

  path = argv[1];

  separator = '/';

  nItems = path split separator, paths;

  for (i = 0; i < nItems; i = i + 1)
    printf ("%s\n", paths at i);

  exit (0);
}
