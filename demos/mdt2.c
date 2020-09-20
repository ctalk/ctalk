/* $Id: mdt2.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
 *  mdt2.c - Like mdt.c, implemented with a loop instead of a map.
 *
 *  Usage: mdt2 <pathname>
 */

int main (int argc, char **argv) {

  DirectoryStream new mkdirStream;
  String new newDirName;
  Array new subDirNames;
  Integer new nSubDirs;
  Integer new i;
  
  WriteFileStream classInit;
  
  if (argc != 2) {
    stdoutStream writeStream "Usage: mdt2 <dirname>\n";
    exit (1);
  }

  newDirName = argv[1];

  nSubDirs = newDirName split '/', subDirNames;

  for (i = 0; i < nSubDirs; i = i + 1) {
    /*
     *  The, "at," method in the following statement's
     *  binding to, "subDirNames," is still somewhat
     *  experimental, because of the way the interpreter
     *  and run-tim expression evaluation match collection
     *  members.  It should be sufficient to determine
     *  that, "at's," receiver is, "subDirNames," and not,
     *  "mkDirStream."  
     *
     *  If there's any doubt about the class of an argument, 
     *  you can always parenthesize it, as in the following 
     *  statement, and let the primary receiver's method, 
     *  "chDir," in this case, handle different argument 
     *  classes.
     */
    mkdirStream mkDir subDirNames at i;
    mkdirStream chDir (subDirNames at i);
    stdoutStream writeStream mkdirStream getCwd + "\n";
  }
  exit (0);
}
