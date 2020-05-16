/* $Id: mdt.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
 *  mdt.c - A program like mkdir -p, which creates all of the
 *  directories of a directory hierarchy.
 *
 *  Usage: mdt <pathname>
 */

Array instanceMethod mkSubDir (char *dirName) {
  DirectoryStream new mkdirStream;

  mkdirStream mkDir self;
  mkdirStream chDir self;
  printf ("%s\n", mkdirStream getCwd);
  return NULL;
}

int main (int argc, char **argv) {

  String new newDirName;
  Array new subDirNames;
  Integer new nSubDirs;
  
  if (argc != 2) {
    printf ("Usage: mdt <dirname>\n");
    exit (1);
  }

  newDirName = argv[1];

  nSubDirs = newDirName split '/', subDirNames;

  subDirNames map mkSubDir;

  exit (0);
}
