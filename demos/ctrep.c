/* $Id: ctrep.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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
 *  ctrep [-v] [-h] <pattern> <replacement> <infile >outfile
 *
 *  A simple filter that replaces every occurence of <word>
 *  in infile with <replacement>.  The program handles exact 
 *  matches only.
 */

#include <stdio.h>

/*
 *  Function prototypes.
 */
void exit_help (void);
int check_args (int, char **);

/*
 *   Global Objects.
 */
Integer new verboseOpt;    /* Print what we're replacing before processing. */

String new pattern;        /* The text pattern to replace.                  */
String new replacement;    /* The replacement text for <pattern>.           */

int main (int argc, char **argv) {

  String new inputLine;
  String new word;
  Integer new i;
  Character new inputChar;

  verboseOpt = FALSE;

  /*
   *  Strings must still be explicitly initialized.
   */
  pattern = "";
  replacement = "";

  check_args (argc, argv);

  /*
   *  Initialize stdinStream and stdoutStream.
   */
  ReadFileStream classInit;
  WriteFileStream classInit;

  if (verboseOpt) {
    printf ("Replacing %s with %s.\n", pattern, replacement);
  }

  /*
   *  Loop until the end of input.
   */
  while (TRUE) {
    inputLine = stdinStream readLine;
    if (stdinStream streamEof)
      break;

    word = "";

    inputLine map {
      if (self isSpace) {
	if (word == pattern) {
	  stdoutStream writeStream replacement;
	} else {
	  stdoutStream writeStream word;
	}
	stdoutStream writeChar self;
	word = "";
      } else {
	word += self asString;
      }
    }
  }

  exit (0);
}


int check_args (int c, char **a) {

  int i, have_pattern, have_replacement;

  have_pattern = have_replacement = FALSE;

  for (i = 1; i < c; i++) {

    if (a[i][0] == '-') {
	
      if (a[i][1] == 'h') {
	exit_help ();
      } else {
	if (a[i][1] == 'v') {
	  verboseOpt = TRUE;
	} else {
	  exit_help ();
	}
      }

    } else {

      if (!have_pattern) {
	have_pattern = TRUE;
	pattern = a[i];
      } else {
	if (!have_replacement) {
	  have_replacement = TRUE;
	  replacement = a[i];
	} else {
	  exit_help ();
	}
      }

    }
  }

  return 0;
}

void exit_help (void) {
  printf ("Usage: ctrep [-v] [-h] <pattern> <replacement>\n");
  exit (1);
}
