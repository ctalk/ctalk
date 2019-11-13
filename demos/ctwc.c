/* $Id: ctwc.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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
 *  Count the number of characters, words, and lines in the file given
 *  as the argument, or standard input.  Note that this program is very 
 *  slow (approximately 8.5 seconds to process ctwc.c on a 900Mhz Pentium
 *  system)
 */

int main (int argc, char **argv) {

  String new fileArg;
  String new appName;
  ReadFileStream new inputStream;
  String new inputString;
  Integer new inputLength;
  Integer new i;

  Character new input;
  Integer new nWords;
  Integer new nLines;
  Integer new inWord;
  Exception new e;

  e enableExceptionTrace;

  appName = argv[0];

  if (argc == 2) {
    fileArg = argv[1];
    if (fileArg == "-h") {
      printf ("Usage: %s [ -h | infile | - ]\n", appName);
      exit (0);
    } else {
      if (fileArg == "-") {
	inputStream = stdinStream;
      } else {
	inputStream openOn fileArg;
  	if (e pending) {
  	  e handle;
 	  exit (1);
  	}
	if (inputStream isDir) {
	  printf ("Input is a directory.\n");
	  exit (1);
	}
      }
    }
  } else {
    fileArg = "-";
    inputStream = stdinStream;
  }


  nWords = 0;
  nLines = 0;
  inWord = FALSE;

  inputString = inputStream readAll;
  inputLength = inputString length;

  for (i = 0; i < inputLength; i = i + 1) {
    input = inputString at i;
    if (input == '\n') 
      nLines = nLines + 1;
    if (inWord) {
      if (input isSpace) {
	nWords = nWords + 1;
	inWord = FALSE;
      }
    } else {
      if (!input isSpace) {
	inWord = TRUE;
      }
    }
  }

  printf ("\t%d\t%d\t%d\n", nLines, nWords, inputLength);

  if (fileArg != "-")
    inputStream closeStream;

  exit (0);
}
