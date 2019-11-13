/* $Id: optiondb.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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

#include <stdio.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"


OPTION_DESC option_desc[] = {
  {"-E",            "Preprocess the input and exit.", 
   opt_arg_none},
  {"-h",            "Print help message and exit.", 
   opt_arg_none},
  {"--help",        "", 
   opt_arg_none},
  {"-I",            "Add argument to include file search path.", 
   opt_arg_str},
  {"--keeppragmas", "Write pragmas untranslated to the output.", 
   opt_arg_none},
  {"--nostdinc",    "Do not include ctalk's system headers in the input",
   opt_arg_none},
  {"-o",            "Write the output to the file named by the argument.",
   opt_arg_str},
  {"-s",            "Add the argument to ctalk's system header search path.",
   opt_arg_str},
  {"-V",            "Print the ctalk version number and exit.", 
   opt_arg_none},
  {"-v",            "Print verbose warnings.", 
   opt_arg_none},
  {"\0", "\0", opt_arg_none}
};

#define N_OPTION_DESC (sizeof (option_desc) / sizeof (OPTION_DESC))

OPTION_VAL *option_vals[MAXARGS];
int n_option_vals;

void print_options (void) {

/*   int i; */
/*   char spacer[20], line_space[20]; */

/*   memset (spacer, (int)' ', 20); */

/*   for (i = 0; *option_desc[i].opt_name; i++) { */

/*     strncpy (line_space, spacer, strlen(spacer) -  */
/* 	    strlen (option_desc[i].opt_name) + */
/* 	    (((option_desc[i].opt_arg_type == opt_arg_int) || */
/* 	      (option_desc[i].opt_arg_type == opt_arg_str)) ? */
/* 	     6 : 0)); */
    
/*     printf ("%s %s%s%s\n", option_desc[i].opt_name, */
/* 	    ((option_desc[i].opt_arg_type == opt_arg_int) ? */
/* 	     "<int>" : ((option_desc[i].opt_arg_type == opt_arg_str) ? */
/* 			"<str>" : "")), */
/* 	    line_space, */
/* 	    option_desc[i].opt_desc); */
/*   } */
}

int getopts (char **a, int i) {
  
  print_options ();

  return SUCCESS;

}


