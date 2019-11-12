/* $Id: make_builtins.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011 Robert Kiesling, rk3314042@gmail.com.
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

/* make_builtins.c -- 

   Takes the output of:
     echo ' ' | cpp -dM -

   and outputs the builtins[] struct.
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CPP_CMD "echo ' ' | cpp -dM -"
#define BUFSIZE 0xffff

char buf[BUFSIZE];
char def_buf[BUFSIZE];

enum state {
  state_keyword,
  state_macro,
  state_def
};
typedef enum state STATE;

char *escape_quotes (const char *def) {

  static char qbuf[BUFSIZE];
  int i, j;


  memset ((void *)qbuf, 0, BUFSIZE);
  for (i = 0, j =0; def[i];) {
    if (def[i] == '"') {
      qbuf[j++] = '\\';
      qbuf[j++] = '"';
      ++i;
    } else {
      qbuf[j++] = def[i++];
    }
  }
  return qbuf;
}

int main (int argc, char **argv) {

  FILE *p;
  char cbuf[2];
  int chars_read;
  int buf_ptr;
  int lookahead;
  int quoted_def;
  STATE state;

  if ((p = popen (CPP_CMD, "r")) == NULL) {
    printf ("make_builtins (popen): %s\n", strerror (errno));
    exit (-1);
  }
  memset ((void *)buf, 0, BUFSIZE);
  
  buf_ptr = 0;
  while ((chars_read = fread (cbuf, sizeof (char), 1, p)) != 0) 
    buf[buf_ptr++] = cbuf[0];

  if (ferror (p)) { /* Not end-of-file, but error. */
    printf ("make_builtins (fread): %s\n", strerror (errno));
    exit (-1);
  }

  pclose (p);

  printf ("%s\n", "/* This is a machine generated file. Do not edit! */");
  printf ("%s\n", "static char *builtins[] = {");
  
  state = state_keyword;
  for (buf_ptr = 0; buf[buf_ptr]; ++buf_ptr) {
    switch (buf[buf_ptr])
      {
      case '#':  /* skip, "#define," */
	++buf_ptr;
	while (isalpha ((int)buf[++buf_ptr])) {
	  lookahead = buf_ptr + 1;
	  if (isspace((int)buf[lookahead]))
	    break;
	}
	state = state_macro;
	break;
      case ' ':
      case '\t':
	
	continue;
	break;
      default:
	switch (state)
	  {
	  case state_macro:

	    --buf_ptr; /*** yic */

	    printf ("%c", '"');
	    while (!isspace ((int)buf[++buf_ptr])) {
	      lookahead = buf_ptr + 1;
	      printf ("%c", buf[buf_ptr]);
	      if (isspace((int)buf[lookahead])) {
		++buf_ptr;
		break;
	      }
	    }

	    printf ("%s", "\", \"");

	    state = state_def;

	    break;
	  case state_def:
	    memset ((void *)def_buf, 0, BUFSIZE);
	    for (lookahead = 0; buf[buf_ptr + lookahead]; ++lookahead) {
	      if (buf[buf_ptr + lookahead] == '\n')
		break;
	      def_buf[lookahead] = buf[buf_ptr + lookahead];
	    }

	    if (strstr (def_buf, "\""))
	      strcpy (def_buf, escape_quotes ((const char *)def_buf));

	    /*   printf ("%s,\"\\\"\n", def_buf); */
	    /* else */

	      printf ("%s\",\n", def_buf);

	    buf_ptr += lookahead;

	    state = state_keyword;
	    break;
	  case state_keyword:
	    break;
	  }
	break;
      }
  }

  printf ("%s\n", "(void *)0, (void *)0\n};");
}
