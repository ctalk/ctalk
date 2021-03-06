/* $Id: membervars.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright � 2014-2015 Robert Kiesling, rk3314042@gmail.com.
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

Application new varsApp;

String new className;
Array new classDirs;

Application instanceMethod classSearchPathAsString (void) {

  String new s;

  s = self classSearchPath;

  s split ':', classDirs;

  return NULL;
}


/* Function prototypes. */
void exit_help (void);
void parse_args (int, char **);

int main (int argc, char **argv) {
  String new varList;
  Exception new ex;

  parse_args (argc, argv);

  if (className length == 0) {
    exit_help ();
  }

  varList = varsApp membervars className;

  if (ex pending) {
    printf ("%s\n", ex peek);
    ex handle;
    exit (1);
  }

  if (varList length > 0)
    printf ("%s\n", varList);

  exit (0);
}

void parse_args (int c, char **a) {
  int i;

  for (i = 1; i < c; i++) {

    if (a[i][0] == '-') {
      switch (a[i][1])
	{
	case 'h':
	  exit_help ();
	  break;
	default:
	  exit_help ();
	  break;
	}
    } else {
      className = a[i];
    }
  }
}

void exit_help (void) {
  
  printf ("Usage: membervars [-h] | <class>\n");
  printf ("List the member variables in <class>.\n");
  printf ("-h       Display this message and exit.\n");
  printf ("Class search directories:\n");

  varsApp classSearchPathAsString;
  classDirs map {
    printf ("\t%s\n", self);
  }

  printf ("The CLASSLIBDIRS environment variable can define other\n"
	  "directories to search for classes.\n");
  printf ("Please report bugs to: rk3314042@gmail.com.\n");
  exit (1);
}
