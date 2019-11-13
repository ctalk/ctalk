/* $Id: ctecho.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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
 *   ctecho.c - Echo the input to the output.
 *   Options
 *   -h    Print a help message and exit.
 *   -l    Translate the output to lower case.
 *   -n    Don't print a newline after the output.
 *   -u    Translate the output to upper case.
 */

/*
 * Prototypes
 */
void usage (char **);
int parse_args (int, char **);

/*
 *  Options
 */
int opt_l = 0;         /* Translate the output to lowercase.         */
int opt_n = 0;         /* Don't terminate the output with a newline. */
int opt_u = 0;         /* Translate the output to uppercase.         */

int main (int argc, char **argv) {

  String new inputString;
  Character new outputChar;

  Integer new stringIndex;
  Integer new stringLength;
  Integer new optionsEnd;
  Integer new i;

  if (argc <= 1)
    usage (argv);

  optionsEnd = parse_args (argc, argv);

  if (opt_l && opt_u)
    usage (argv);

   for (i = optionsEnd; i < argc; i = i + 1) {

     inputString = argv[i];

     stringLength = inputString length;

     for (stringIndex = 0; stringIndex < stringLength;
	  stringIndex = stringIndex + 1) {
       outputChar = inputString at stringIndex;
       if (opt_l)
	 printf ("%c", outputChar toLower);
       if (opt_u)
	 printf ("%c", outputChar toUpper);
       if (!opt_l && !opt_u)
	 printf ("%c", outputChar);
     }

     printf (" ");

   }

   if (!opt_n)
     printf ("\n");

  return 0;
}

/*
 *  Parse the command line options if any, and return the 
 *  index of the first non-option argument.
 */

int parse_args (int c, char**a) {

  int i;

  /*
   *  a[0] is the name of the executable.
   */
  for (i = 1; i < c; i++) {
    if (*a[i] == '-') {
      switch (a[i][1])
	{
	case 'l':
	  opt_l = 1;
	  break;
	case 'h':
	  usage (a);
	  break;
	case 'n':
	  opt_n = 1;
	  break;
	case 'u':
	  opt_u = 1;
	  break;
	default:
	  usage (a);
	  break;
	}
    } else {
      return i;
    }
  }

  if (*a[c - 1] == '-') exit (1);

  return 0;
}

void usage (char **argv) {
  printf ("Usage: %s [-l|-u] [-h] [-n]  <string>\n", argv[0]); 
  printf ("-h   Print this message and exit.\n");
  printf ("-l   Translate the output to lower case.\n");
  printf ("-n   Don't print a newline after the output.\n");
  printf ("-u   Translate the output to upper case.\n");
  exit (1);
}
