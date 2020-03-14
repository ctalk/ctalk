/* $Id: panelib.c,v 1.2 2020/03/08 22:33:14 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2017, 2019  
    Robert Kiesling, rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"

extern RT_INFO *__call_stack[MAXARGS+1];  /* Declared in rtinfo.c. */
extern int __call_stack_ptr;

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

#define PANE_RCVR_OBJ (__ctalk_receivers[__ctalk_receiver_ptr + 1])

/* cell_size should only need to be 1 * sizeof (int) for most 
   visual classes. 
   Note that we buffer an extra row and column in case the pane requires
   a shadow.  And also there is an extra row (that should stay NULL) 
   in case of overrruns or to help delete the buffer.
*/
int **__ctalkCreateWinBuffer (int x_size, int y_size, int cell_size) {
  static int **p, i, is_text_pane = FALSE;
  OBJECT *rcvr_object;

  if ((rcvr_object = PANE_RCVR_OBJ) == NULL)
    return (int **)NULL;
  if  (is_class_or_subclass 
       (rcvr_object, __ctalkGetClass ("ANSITerminalPane"))) 
    is_text_pane = TRUE;

  p = (int **)__xalloc ((y_size + 3) * sizeof (int *));
  for (i = 0; i <= y_size + 1; i++) {
    p[i] = (int *)__xalloc((x_size + 3) * sizeof (int));
    if (is_text_pane) {
      int j, space_int = (int)' ';
      for (j = 0; j <= x_size + 1; j++) {
	p[i][j] = space_int;
      }
    }
  }
  return p;
}

int __ctalkDeleteWinBuffer (OBJECT *symbol_instancevar_object) {
  int **p, **q, r;
  OBJECT *xid_instvar;
  int id;

  if (str_eq (symbol_instancevar_object -> __o_class -> __o_name,
	      "X11Bitmap")) {
    errno = 0;
    if ((xid_instvar = __ctalkGetInstanceVariable
	 (symbol_instancevar_object, "xID", FALSE)) != NULL) {

      id = SYMVAL(xid_instvar -> instancevars ?
		  xid_instvar -> instancevars -> __o_value :
		  xid_instvar -> __o_value);
      __ctalkX11DeletePixmap (id);
    }
  } else if (!str_eq (symbol_instancevar_object -> instancevars ->
		      __o_value, NULLSTR)) {
    /* the casts just work and produce no warnings */
    p = (int **)*(int **)(((symbol_instancevar_object -> instancevars) ?
		   symbol_instancevar_object -> instancevars -> __o_value :
		   symbol_instancevar_object -> __o_value));
    if (p == NULL)
      return ERROR;

    /* TODO This is about the only place we don't use __xfree (). */
    for (q = p; *q; q++)
      free (*q);
    free (p);
  } else if ((xid_instvar = __ctalkGetInstanceVariable
	      (symbol_instancevar_object , "xID", FALSE)) != NULL) {
    id = INTVAL(xid_instvar -> instancevars ?
		xid_instvar -> instancevars -> __o_value :
		xid_instvar -> __o_value);
    INTVAL(xid_instvar -> __o_value) = 0;
    if (IS_OBJECT(xid_instvar -> instancevars)) {
      INTVAL(xid_instvar -> instancevars -> __o_value) = 0;
    }
    __ctalkX11DeletePixmap (id);
    
  }
  return SUCCESS;
}

static FILE *__pane_output_stream (OBJECT *pane_object) {
  OBJECT *stream_instance_var,
    *stream_outputstream_instance_var,
    *stream_outputstream_instance_var_value;
  static FILE *f;
  if ((stream_instance_var = 
       __ctalkGetInstanceVariable (pane_object, "paneStream", TRUE)) 
      == NULL) {
    return NULL;
  }
  if ((stream_outputstream_instance_var = 
       __ctalkGetInstanceVariable (stream_instance_var, 
				   "outputHandle", TRUE)) == NULL) {
    return NULL;
  }
  if ((stream_outputstream_instance_var_value =
       stream_outputstream_instance_var -> instancevars) == NULL)
    return NULL;
  errno = 0;
  f = (FILE *)strtoul 
    (stream_outputstream_instance_var_value -> __o_value, NULL, 16);
  if (errno != 0) {
    strtol_error (errno, "__pane_output_stream ()", 
		  stream_outputstream_instance_var_value -> __o_value);
  }
  return f;
}

static int __pane_shadow (OBJECT *pane_object) {
  OBJECT *shadow_instance_var,
    *shadow_instance_var_value;
  if ((shadow_instance_var = 
       __ctalkGetInstanceVariable (pane_object, "shadow", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((shadow_instance_var_value =
       shadow_instance_var -> instancevars) == NULL)
    return ERROR;
  return *(int *)shadow_instance_var_value -> __o_value;
}

static int __pane_border (OBJECT *pane_object) {
  OBJECT *border_instance_var,
    *border_instance_var_value;

  if ((border_instance_var = 
       __ctalkGetInstanceVariable (pane_object, "border", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((border_instance_var_value =
       border_instance_var -> instancevars) == NULL)
    return ERROR;
  return *(int *)border_instance_var_value -> __o_value;
}

static char *__pane_title (OBJECT *pane_object) {
  OBJECT *title_instance_var,
    *title_instance_var_value;

  if ((title_instance_var = 
       __ctalkGetInstanceVariable (pane_object, "titleString", FALSE)) 
      == NULL) {
    return NULL;
  }
  if ((title_instance_var_value =
       title_instance_var -> instancevars) == NULL)
    return NULL;
  return title_instance_var_value -> __o_value;
}

static int __pane_mapped (OBJECT *pane_object) {
  OBJECT *mapped_instance_var,
    *mapped_instance_var_value;
  static int mapped;

  if ((mapped_instance_var = 
       __ctalkGetInstanceVariable (pane_object, "mapped", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((mapped_instance_var_value =
       mapped_instance_var -> instancevars) == NULL)
    return ERROR;
  return *(int *)mapped_instance_var_value -> __o_value;
}

int __pane_x_org (OBJECT *paneobject) {
  OBJECT *pane_self_object, *origin_instance_var,
    *origin_x_instance_var, *origin_x_instance_var_value;

  if (IS_VALUE_INSTANCE_VAR(paneobject))
    pane_self_object = paneobject -> __o_p_obj;
  else
    pane_self_object = paneobject;

  if ((origin_instance_var = 
       __ctalkGetInstanceVariable (pane_self_object, "origin", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((origin_x_instance_var = 
       __ctalkGetInstanceVariable (origin_instance_var, "x", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((origin_x_instance_var_value =
       origin_x_instance_var -> instancevars) == NULL)
    return ERROR;
  return *(int *)origin_x_instance_var_value -> __o_value;
}

int __pane_y_org (OBJECT *paneobject) {
  OBJECT *pane_self_object, *origin_instance_var,
    *origin_y_instance_var, *origin_y_instance_var_value;
  int y_org;

  if (IS_VALUE_INSTANCE_VAR(paneobject))
    pane_self_object = paneobject -> __o_p_obj;
  else
    pane_self_object = paneobject;

  if ((origin_instance_var = 
       __ctalkGetInstanceVariable (pane_self_object, "origin", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((origin_y_instance_var = 
       __ctalkGetInstanceVariable (origin_instance_var, "y", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((origin_y_instance_var_value =
       origin_y_instance_var -> instancevars) == NULL)
    return ERROR;
  return *(int *)origin_y_instance_var_value -> __o_value;
}

int __pane_x_size (OBJECT *paneobject) {
  OBJECT *pane_self_object, *size_instance_var,
    *size_x_instance_var, *size_x_instance_var_value;
  int x_size;

  if (IS_VALUE_INSTANCE_VAR(paneobject))
    pane_self_object = paneobject -> __o_p_obj;
  else
    pane_self_object = paneobject;

  if ((size_instance_var = 
       __ctalkGetInstanceVariable (pane_self_object, "size", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((size_x_instance_var = 
       __ctalkGetInstanceVariable (size_instance_var, "x", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((size_x_instance_var_value =
       size_x_instance_var -> instancevars) == NULL)
    return ERROR;
  return *(int *)size_x_instance_var_value -> __o_value;
}

int __pane_y_size (OBJECT *paneobject) {
  OBJECT *pane_self_object, *size_instance_var,
    *size_y_instance_var, *size_y_instance_var_value;
  int y_size;

  if (IS_VALUE_INSTANCE_VAR(paneobject))
    pane_self_object = paneobject -> __o_p_obj;
  else
    pane_self_object = paneobject;

  if ((size_instance_var = 
       __ctalkGetInstanceVariable (pane_self_object, "size", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((size_y_instance_var = 
       __ctalkGetInstanceVariable (size_instance_var, "y", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((size_y_instance_var_value =
       size_y_instance_var -> instancevars) == NULL)
    return ERROR;
  return *(int *)size_y_instance_var_value -> __o_value;
}

static int __pane_cursor_x (OBJECT *pane_object) {
  OBJECT *cursor_instance_var,
    *cursor_x_instance_var, *cursor_x_instance_var_value;
  static int cursor_x;

  if ((cursor_instance_var = 
       __ctalkGetInstanceVariable (pane_object, "cursor", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((cursor_x_instance_var = 
       __ctalkGetInstanceVariable (cursor_instance_var, "x", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((cursor_x_instance_var_value =
       cursor_x_instance_var -> instancevars) == NULL)
    return ERROR;
  return *(int *)cursor_x_instance_var_value -> __o_value;
}

static int __pane_cursor_y (OBJECT *pane_object) {
  OBJECT *cursor_instance_var,
    *cursor_y_instance_var, *cursor_y_instance_var_value;
  static int cursor_y;

  if ((cursor_instance_var = 
       __ctalkGetInstanceVariable (pane_object, "cursor", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((cursor_y_instance_var = 
       __ctalkGetInstanceVariable (cursor_instance_var, "y", TRUE)) 
      == NULL) {
    return ERROR;
  }
  if ((cursor_y_instance_var_value =
       cursor_y_instance_var -> instancevars) == NULL)
    return ERROR;
  return *(int *)cursor_y_instance_var_value -> __o_value;
}

static int **__pane_buf (OBJECT *pane_object) {
  OBJECT *pane_instance_var,
    *pane_instance_var_value;
  void *buf;

  if ((pane_instance_var = 
       __ctalkGetInstanceVariable (pane_object, "paneBuffer", TRUE)) 
      == NULL) {
    return NULL;
  }
  if ((pane_instance_var_value =
       pane_instance_var -> instancevars) == NULL)
    return NULL;
  buf = *(void **)(pane_instance_var_value -> __o_value);
  return (int **)buf;
}

static int **__pane_backing_store (OBJECT *pane_object) {
  OBJECT *pane_instance_var,
    *pane_instance_var_value;
  void *buf;

  if ((pane_instance_var = 
       __ctalkGetInstanceVariable (pane_object, "paneBackingStore", TRUE)) 
      == NULL) {
    return NULL;
  }
  if ((pane_instance_var_value =
       pane_instance_var -> instancevars) == NULL)
    return NULL;
  buf = *(void **)(pane_instance_var_value -> __o_value);
	  
  return (int **)buf;
}

static int graphics_bits;

static int __pane_graphics_bits (OBJECT *pane_object) {
  OBJECT *pane_attrs_var, *pane_attrs_value_var;
  if ((pane_attrs_var = 
       __ctalkGetInstanceVariable (pane_object, "graphicsBits", TRUE)) 
      == NULL) {
    return 0;
  }
  if ((pane_attrs_value_var =
       pane_attrs_var -> instancevars) == NULL)
    return 0;
  graphics_bits = *(int *)pane_attrs_value_var -> __o_value;
  return graphics_bits;
}

#define BOLD      (1 << 0)
#define REVERSE   (1 << 1)
#define UNDERLINE (1 << 2)
#define BLINK     (1 << 3)

static char *__ansi_graphics_attr_str (int bits) {

  static char attr_str[MAXMSG];
 
  *attr_str = '\0';

  if (bits & BOLD)
    strcatx (attr_str, ((EMPTY_STR(attr_str)) ? "1" : ";1"), NULL);
  if (bits & REVERSE)
    strcatx (attr_str, ((EMPTY_STR(attr_str)) ? "7" : ";7"), NULL);
  if (bits & UNDERLINE)
    strcatx (attr_str, ((EMPTY_STR(attr_str)) ? "4" : ";4"), NULL);
  if (bits & BLINK)
    strcatx (attr_str, ((EMPTY_STR(attr_str)) ? "5" : ";5"), NULL);

  if (*attr_str == '\0')
    strcatx2 (attr_str, "0", NULL);
  strcatx2 (attr_str, "m", NULL);
  return attr_str;
}

/* ANSI/VT100/Xterm escape sequences. */
#define VT_REV    "\033[7m"     /* Reverse video.                    */
#define VT_NORM   "\033[0m"     /* Normal video.                     */
#define VT_BOLD   "\033[1m"     /* Bold video.                     */
#define VT_CPOS   "\033[%d;%df" /* Set cursor position: row, column. */
#define VT_CLS    "\033[2J"     /* Clear the screen.                 */
#define VT_RESET  "\033[!p"     /* Soft reset.                       */
#define VT_SGR    "\033[%cm"    /* Set graphics.                     */
#define VT_SCS_GRAPHICS    "\033(0"    /* Set graphics characters.   */
#define VT_SCS_ASCII       "\033(B"    /* Set ASCII characters.      */

static void __ansi_graphics_charset (FILE *stream) {
  fputs (VT_SCS_GRAPHICS, stream);
  fprintf (stream, "\033[%s", __ansi_graphics_attr_str (graphics_bits));
}

static void __ansi_ascii_charset (FILE *stream) {
  fputs (VT_SCS_ASCII, stream);
  fprintf (stream, "\033[%s", __ansi_graphics_attr_str (graphics_bits));
}

int __ctalkANSITerminalPaneRefresh (void) {
  int **buf, x_org, y_org, x_size, y_size, buf_x, buf_y,
    shadow, border, cursor_x, cursor_y;
  struct termios old_termios_s, new_termios_s;
  char rowbuf[MAXMSG], *title;
  FILE *stream;

  buf = __pane_buf (PANE_RCVR_OBJ);
  x_org = __pane_x_org (PANE_RCVR_OBJ);
  y_org = __pane_y_org (PANE_RCVR_OBJ);
  x_size = __pane_x_size (PANE_RCVR_OBJ);
  y_size = __pane_y_size (PANE_RCVR_OBJ);
  cursor_x = __pane_cursor_x (PANE_RCVR_OBJ);
  cursor_y = __pane_cursor_y (PANE_RCVR_OBJ);
  stream = __pane_output_stream (PANE_RCVR_OBJ);
  shadow = __pane_shadow (PANE_RCVR_OBJ);
  border = __pane_border (PANE_RCVR_OBJ);
  title = __pane_title (PANE_RCVR_OBJ);

  tcgetattr (fileno(stream), &old_termios_s);
  memcpy ((void *)&new_termios_s, (void *)&old_termios_s, 
	  sizeof (struct termios));
#if defined(__sparc__) && defined(__svr4__)
  new_termios_s.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
			     | INLCR | IGNCR | ICRNL | IXON);
  new_termios_s.c_oflag &= ~OPOST;
  new_termios_s.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  new_termios_s.c_cflag &= ~(CSIZE | PARENB);
  new_termios_s.c_cflag |= CS8;
  new_termios_s.c_cc[VMIN] = 0;
  new_termios_s.c_cc[VTIME] = 0;
#else /* Linux */
  cfmakeraw (&new_termios_s);
#endif
  tcsetattr (fileno(stream), TCSADRAIN, &new_termios_s);

  __pane_graphics_bits (PANE_RCVR_OBJ);

  fprintf (stream, VT_CPOS, x_org, y_org);
  fprintf (stream, "\033[%s",  __ansi_graphics_attr_str (graphics_bits));
  fflush (stream);
  for (buf_y = 0; buf_y < y_size; buf_y++) {
    if (border) {
      if (buf_y == 0) {
	fprintf (stream, VT_CPOS, y_org, x_org);
	__ansi_graphics_charset (stream);
	fputc ('l', stream);
	for (buf_x = 0; buf_x < (x_size - 2); buf_x++)
	  fputc ('q', stream);
	fputc ('k', stream);
	__ansi_ascii_charset (stream);
	continue;
      }
      if (buf_y == (y_size - 1)) {
	fprintf (stream, VT_CPOS, y_org + buf_y, x_org);
	__ansi_graphics_charset (stream);
	fputc ('m', stream);
	for (buf_x = 0; buf_x < (x_size - 2); buf_x++)
	  fputc ('q', stream);
	fputc ('j', stream);
	__ansi_ascii_charset (stream);
	if (shadow) {
	  fprintf (stream, "\033[%s", 
		   __ansi_graphics_attr_str (graphics_bits));
	  fputs (VT_REV, stream);
	  fputs (" ", stream);
	  fprintf (stream, "\033[%s", 
		   __ansi_graphics_attr_str (graphics_bits));
	  fputs (VT_NORM, stream);
	}
	continue;
      }

      fprintf (stream, VT_CPOS, y_org + buf_y, x_org);
      __ansi_graphics_charset (stream);
      fputc ('x', stream);
      __ansi_ascii_charset (stream);
      for (buf_x = 0; buf_x < (x_size - 2); buf_x++) {
	if ((char)buf[buf_y][buf_x+1] != 0)
	  rowbuf[buf_x] = (char)buf[buf_y][buf_x+1];
	else
	  rowbuf[buf_x] = ' ';
      }
      rowbuf[x_size-2] = '\0';
      fprintf (stream, VT_CPOS, y_org + buf_y, x_org + 1);
      fprintf (stream, "\033[%s", __ansi_graphics_attr_str (graphics_bits));
      fputs (rowbuf, stream);
      __ansi_graphics_charset (stream);
      fputc ('x', stream);
      __ansi_ascii_charset (stream);
    } else {
      for (buf_x = 0; buf_x < x_size; buf_x++) {
	if ((char)buf[buf_y][buf_x+1] != 0)
	  rowbuf[buf_x] = (char)buf[buf_y][buf_x+1];
	else
	  rowbuf[buf_x] = ' ';
      }
      rowbuf[x_size] = '\0';
      fprintf (stream, VT_CPOS, y_org + buf_y, x_org);
      fprintf (stream, "\033[%s", __ansi_graphics_attr_str (graphics_bits));
      fputs (rowbuf, stream);
    }
    if (shadow) {
      if (buf_y > 0) {
	fprintf (stream, "\033[%s", __ansi_graphics_attr_str (graphics_bits));
	fputs (VT_REV, stream);
	fputs (" ", stream);
	fprintf (stream, "\033[%s", __ansi_graphics_attr_str (graphics_bits));
	fputs (VT_NORM, stream);
      }
    }
  }
  if (title && strcmp (title, NULLSTR)) {
    int __title_start;
    __title_start = (x_size / 2) - (strlen (title) / 2);
    fprintf (stream, VT_CPOS, y_org, __title_start + x_org);
    fputs (VT_BOLD, stream);
    fputs (title, stream);
    fputs (VT_NORM, stream);
  }
  if (shadow) {
    int i;
    fprintf (stream, "\033[%s",  __ansi_graphics_attr_str (graphics_bits));
    fputs (VT_REV, stream);
    fprintf (stream, VT_CPOS, y_org + y_size, x_org + 1);
    for (i = 0; i < x_size; i++)
      fputs (" ", stream);
    fprintf (stream, "\033[%s", __ansi_graphics_attr_str (graphics_bits));
    fputs (VT_NORM, stream);
  }
  fprintf (stream, VT_CPOS, (y_org  - 1) + cursor_y, 
	   (x_org  - 1) + cursor_x);
  fflush (stream);
  tcsetattr (fileno(stream), TCSADRAIN, &old_termios_s);
  return SUCCESS;
}

int __ctalkANSITerminalPanePutChar (int x, int y, char c) {
  int **buf, x_org, y_org;
  FILE *stream;

  stream = __pane_output_stream (PANE_RCVR_OBJ);
  buf = __pane_buf (PANE_RCVR_OBJ);
  x_org = __pane_x_org (PANE_RCVR_OBJ);
  y_org = __pane_y_org (PANE_RCVR_OBJ);

  __pane_graphics_bits (PANE_RCVR_OBJ);

  fprintf (stream, VT_CPOS, y_org + y - 1, x_org + x - 1);

  if (graphics_bits)
    fprintf (stream, "\033[%s",  __ansi_graphics_attr_str (graphics_bits));

  fprintf (stream, "%c",  c);
  fflush (stream);

  /* Translate from pane coords to buffer coords. */
  buf[y-1][x-1] = (int)c;
  return SUCCESS;
}

int __ctalkANSITerminalPaneMapWindow (OBJECT *__child) {
  int x_org, y_org, x_size, y_size, p_x_org, p_y_org,
    p_x_clip, p_y_clip, x_limit, y_limit, shadow, 
    **childbuf, **parentbuf, i, j;
  OBJECT *__pane_object;
  __pane_object = (IS_VALUE_INSTANCE_VAR(__child)) ? __child -> __o_p_obj :
    __child;
  if (!__pane_mapped (__pane_object))
    return SUCCESS;
  parentbuf = __pane_buf(PANE_RCVR_OBJ);
  childbuf = __pane_backing_store(__pane_object);
  x_org = __pane_x_org (__pane_object);
  y_org = __pane_y_org (__pane_object);
  x_size = __pane_x_size (__pane_object);
  y_size = __pane_y_size (__pane_object);
  shadow = __pane_shadow (__pane_object);
  p_x_org = __pane_x_org(PANE_RCVR_OBJ);
  p_y_org = __pane_y_org(PANE_RCVR_OBJ);
  p_x_clip = __pane_x_size(PANE_RCVR_OBJ);
  p_y_clip = __pane_y_size(PANE_RCVR_OBJ);

  x_limit = x_org + x_size + (shadow ? 1 : 0);
  x_limit = ((x_limit > p_x_clip) ? p_x_clip : x_limit);
  x_limit -= x_org;
  y_limit = y_org + y_size + (shadow ? 1 : 0);
  y_limit = ((y_limit > p_y_clip) ? p_y_clip : y_limit);
  y_limit -= y_org ;

  for (i = 0; i < y_limit; i++) {
    for (j = 0; j < x_limit; j++) {
      childbuf[i][j] = parentbuf[i+y_org][j+x_org];
    }
  }
  return SUCCESS;
}

int __ctalkANSITerminalPaneUnMapWindow (OBJECT *__child) {
  int x_org, y_org, x_size, y_size, p_x_org, p_y_org,
    p_x_clip, p_y_clip, x_limit, y_limit, shadow, 
    **childbuf, **parentbuf, i, j;
  OBJECT *__child_pane_object;
  __child_pane_object = (IS_VALUE_INSTANCE_VAR(__child)) ? __child -> __o_p_obj :
    __child;
  if (!__pane_mapped (__child_pane_object))
    return SUCCESS;
  parentbuf = __pane_buf(PANE_RCVR_OBJ);
  childbuf = __pane_backing_store(__child_pane_object);
  x_org = __pane_x_org (__child_pane_object);
  y_org = __pane_y_org (__child_pane_object);
  x_size = __pane_x_size (__child_pane_object);
  y_size = __pane_y_size (__child_pane_object);
  shadow = __pane_shadow (__child_pane_object);
  p_x_org = __pane_x_org(PANE_RCVR_OBJ);
  p_y_org = __pane_y_org(PANE_RCVR_OBJ);
  p_x_clip = __pane_x_size(PANE_RCVR_OBJ);
  p_y_clip = __pane_y_size(PANE_RCVR_OBJ);

  x_limit = x_org + x_size + (shadow ? 1 : 0);
  x_limit = ((x_limit > p_x_clip) ? p_x_clip : x_limit);
  x_limit -= x_org;
  y_limit = y_org + y_size + (shadow ? 1 : 0);
  y_limit = ((y_limit > p_y_clip) ? p_y_clip : y_limit);
  y_limit -= y_org ;

  for (i = 0; i < y_limit; i++) {
    for (j = 0; j < x_limit; j++) {
      parentbuf[j+x_org][i+y_org] = childbuf[j][i];
    }
  }
  return SUCCESS;
}

int __ctalkANSIClearPaneLine (OBJECT *self, int line) {
  OBJECT *self_obj;
  int x_org, y_org, x_size, y_size, border, shadow,
    **pane_buf, i;
  self_obj = (IS_VALUE_INSTANCE_VAR(self) ? self -> __o_p_obj : self);
  pane_buf = __pane_buf (self_obj);
  x_org = __pane_x_org (self_obj);
  y_org = __pane_y_org (self_obj);
  x_size = __pane_x_size (self_obj);
  y_size = __pane_y_size (self_obj);
  shadow = __pane_shadow (self_obj);
  border = __pane_border (self_obj);
  for (i = border; i <= x_size - border; i++)
    pane_buf[line-1][i] = (int)' ';
  return SUCCESS;
}

int __ctalkCopyPaneStreams (OBJECT *dest, OBJECT *src) {
  OBJECT *dest_obj, *src_obj;
  OBJECT *dest_panestream, *src_panestream;
  OBJECT *dest_inputhandle, *src_inputhandle;
  OBJECT *dest_outputhandle, *src_outputhandle;
  dest_obj = (IS_VALUE_INSTANCE_VAR(dest) ? dest -> __o_p_obj : dest);
  src_obj = (IS_VALUE_INSTANCE_VAR(src) ? src -> __o_p_obj : src);
  dest_panestream = 
    __ctalkGetInstanceVariable (dest_obj, "paneStream", TRUE);
  src_panestream = 
    __ctalkGetInstanceVariable (src_obj, "paneStream", TRUE);
  dest_inputhandle = 
    __ctalkGetInstanceVariable (dest_panestream, "inputHandle", TRUE);
  src_inputhandle = 
    __ctalkGetInstanceVariable (src_panestream, "inputHandle", TRUE);
  dest_outputhandle = 
    __ctalkGetInstanceVariable (dest_panestream, "outputHandle", TRUE);
  src_outputhandle = 
    __ctalkGetInstanceVariable (src_panestream, "outputHandle", TRUE);
  __ctalkSetObjectValueVar (dest_inputhandle,
			    src_inputhandle -> instancevars -> __o_value);
  __ctalkSetObjectValueVar (dest_outputhandle,
			    src_outputhandle -> instancevars -> __o_value);
  return SUCCESS;
}
