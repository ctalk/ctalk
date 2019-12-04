/* $Id: methods.c,v 1.1 2019/11/26 04:15:27 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014-2016 Robert Kiesling, rk3314042@gmail.com.
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
 *  methods.c - Print the method selectors defined in a class 
 *  library.
 *
 *  To build the program:
 *    $ ctcc methods.c -o methods
 *
 *  Typing "methods -h" displays a help message.
 */

Application class MethodBrowser;

/*
 *  TODO!!! - Make sure these work as instance variables.
 */
Array new classDirs;
String new classNameStr;
Boolean new prototypes;
Boolean new classMethods;
Boolean new docs;

MethodBrowser new methodsApp;

MethodBrowser instanceMethod classSearchPath (void) {

  String new s;

  s = self super classSearchPath;

  s split ':', classDirs;

  return NULL;
}

MethodBrowser instanceMethod methods (String classNameArg, 
				      Boolean findClassMethodsArg) {
  String new s;
  s = classNameArg methods findClassMethodsArg;
  return s;
}

MethodBrowser instanceMethod getPrototypes (String aClassName, 
					    String aMethodNamesList) {
  String new outputLine;
  String new source;
  String new thisMethodName;
  Array new names;

  outputLine = "";
  thisMethodName = "";
  aMethodNamesList split '\n', names;
  names map {
  /* methodSource retrieves all of the methods with a particular selector
     (i.e.) it doesn't distinguish methods by the number of arguments),
     so we'll already have gotten all of the prototypes the first
     time the loop encounters one of the method's names. */
    if (thisMethodName == self)
      continue;
    source = methodsApp methodSource aClassName, self;
    outputLine += methodsApp methodPrototypes source;
    thisMethodName = self;
  }

  return outputLine;
}

MethodBrowser instanceMethod getPrototypesAndDocs (String aClassName, 
						   String aMethodListStr) {
  String new outputLine;
  String new source;
  String new thisMethodName;
  Array new names;

  outputLine = "";
  thisMethodName = "";
  aMethodListStr split '\n', names;
  names map {
    /* See the note in getPrototypes, above. */
    if (thisMethodName == self)
      continue;
    source = methodsApp methodSource aClassName, self;
    outputLine += methodsApp methodPrototypes source;
    outputLine += methodsApp methodDocString source;
    outputLine += "\n\n";
    thisMethodName = self;
  }

  return outputLine;
}

MethodBrowser instanceMethod getDocs (String aClassName, 
				      String aMethodListStr) {
  String new outputLine;
  String new source;
  Array new names;

  outputLine = "";
  aMethodListStr split '\n', names;
  names map {
    outputLine += self + "\n";
    source = methodsApp methodSource aClassName, self;
    outputLine += methodsApp methodDocString source;
    outputLine += "\n\n";
  }

  return outputLine;
}

/* Function prototypes. */
void exit_help (void);
void parse_args (int, char **);

int main (int argc, char **argv) {
  String new methodList;
  String new output;
  Exception new ex;

  methodsApp classSearchPath;

  classNameStr = "";
  prototypes = False;
  docs = False;

  parse_args (argc, argv);

  if (classNameStr length == 0)
    exit_help ();

  methodList = methodsApp methods classNameStr, classMethods;

  if (ex pending) {
    printf ("%s\n", ex peek);
    ex handle;
    exit (1);
  }

  if (methodList length > 0) {
    if (prototypes && docs) {
      output = methodsApp getPrototypesAndDocs classNameStr, methodList;
      printf ("%s\n", output);
    } else if (prototypes) {
      output = methodsApp getPrototypes classNameStr, methodList;
      printf ("%s\n", output);
    } else if (docs) {
      output = methodsApp getDocs classNameStr, methodList;
      printf ("%s\n", output);
    } else {
      printf ("%s\n", methodList);
    }
  }

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
	case 'b':
	  /* This is the default. */
	  break;
	case 'c':
	  classMethods = True;
	  break;
	case 'd':
	  docs = True;
	  break;
	case 'i':
	  classMethods = False;
	  break;
	case 'p':
	  prototypes = True;
	  break;
	default:
	  exit_help ();
	  break;
	}
    } else {
      classNameStr = a[i];
    }
  }
}

void exit_help (void) {
  
  printf ("Usage: methods [-h] | [-b] [-c] [-d] [-i] [-p] <class>\n");
  printf ("List the methods in <class>.\n");
  printf ("-b       Print a brief listing (the default).\n");
  printf ("-c       List class methods (lists selectors only).\n");
  printf ("-d       Print the method's documentation.\n");
  printf ("-h       Display this message and exit.\n");
  printf ("-i       List instance methods (the default).\n");
  printf ("-p       Print the method's prototype.\n");
  printf ("Class search directories:\n");

  classDirs map {
    printf ("\t%s\n", self);
  }

  printf ("The CLASSLIBDIRS environment variable can define other\n"
	  "directories to search for classes.\n");
  printf ("Please report bugs to: rk3314042@gmail.com.\n");
  exit (1);
}
