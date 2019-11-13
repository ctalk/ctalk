/* $Id: sls.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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
 *  sls - List the current directory, one file per line.
 */

List instanceMethod printEntries (void) {
  /*
   *  Unlike other collection types, Lists are only 
   *  concerned with the actual objects stored within 
   *  them, in the order that they were added, so an 
   *  in-line method like this one does not need to worry 
   *  about the value of a key or index, only the
   *  item stored there.  
   *
   *  For examples of in-line methods that use collection
   *  keys and value objects, look at ctenv.c and 
   *  ctcheckquery.c in the programs/cgi directory.
   */
  printf ("%s\n", self value);
}

int main (int argc, char **argv) {
  DirectoryStream new dir;
  List new directoryFiles;

  dir directoryList ".", directoryFiles;

  directoryFiles map printEntries;
}
