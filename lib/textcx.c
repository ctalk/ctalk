/* $Id: textcx.c,v 1.2 2020/02/29 01:11:22 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2016-2019  Robert Kiesling, rk3314042@gmail.com.
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include <X11/Xlib.h>
#include "xlibfont.h"

#ifdef HAVE_XFT_H

#include <fontconfig/fontconfig.h>
#include <X11/Xft/Xft.h>
#include FT_FREETYPE_H
#include "xftfont.h"
#include "x11defs.h"

extern XFTFONT ft_font;

#else /* HAVE_XFT_H */

#include "x11defs.h"

#endif /* HAVE_XFT_H */

extern char *shm_mem;
extern int mem_id;

extern char *ascii[8193];             /* from intascii.h */

extern DEFAULTCLASSCACHE *rt_defclasses;

static bool shm_mem_attached  = false;

bool textcx_word_wrap = true;
int textcx_wrap_margin = 300;

extern bool monospace;

extern Display *display;
extern XLIBFONT xlibfont;

#define TCXOBJVAL(x) (*(OBJECT **)(x))

#define TAB_WIDTH 8

/* these are valid if the textobx object is created in
   push_textlist_word, below. */
#define TB_X_VAR(obj) ((obj)->instancevars->next)
#define TB_Y_VAR(obj) ((obj)->instancevars->next->next)
#define TB_WIDTH_VAR(obj) ((obj)->instancevars->next->next->next)
#define TB_HEIGHT_VAR(obj) ((obj)->instancevars->next->next->next->next)
#define TB_WRAPAFTER_VAR(obj) ((obj)->instancevars->next->next->next->next->next)
#define TB_ESCTAG_VAR(obj) ((obj)->instancevars->next->next->next->next->next->next)

MESSAGE *tx_messages[N_MESSAGES + 1];  /* C language message stack and      */
int tx_message_ptr = N_MESSAGES;       /* stack pointer.  Also used by      */

int tx_message_push (MESSAGE *m) {
  if (tx_message_ptr == 0) {
    _warning ("tx_message_push: stack overflow.");
    return ERROR;
  }
  tx_messages[tx_message_ptr--] = m;
  return tx_message_ptr;
}

bool natural_text = false;

OBJECT *list_head;

static void push_textlist_word (OBJECT *list_out, char *text,
				bool escaped_tag) {
  char intbuf[2], buf[0xff];
  OBJECT *textbox_object, *var, *key_object, *var_ptr;
  int scope;

  scope = list_out -> scope | VAR_REF_OBJECT;

  textbox_object = __ctalkCreateObjectInit 
    ("textbox_object", "TextBox", "String", scope, text);

  var_ptr = textbox_object -> instancevars;
  var_ptr -> next =
    create_object_init_internal
    ("x", rt_defclasses -> p_integer_class, scope, "0");

  var_ptr = var_ptr -> next;

  var_ptr -> next =
    create_object_init_internal
    ("y", rt_defclasses -> p_integer_class, scope, "0");


  var_ptr = var_ptr -> next;

  var_ptr -> next =
    create_object_init_internal
    ("width", rt_defclasses -> p_integer_class, scope, "0");

  var_ptr = var_ptr -> next;

  var_ptr -> next =
    create_object_init_internal
    ("height", rt_defclasses -> p_integer_class, scope, "0");

  var_ptr = var_ptr -> next;

  var_ptr -> next =
    create_object_init_internal
    ("wrapAfter", rt_defclasses -> p_boolean_class, scope, "0");

  var_ptr = var_ptr -> next;

  var_ptr -> next =
    create_object_init_internal
    ("escapedTag", rt_defclasses -> p_boolean_class, scope,
     (escaped_tag ? "1" : "0"));

  __objRefCntSet (OBJREF(textbox_object), 1);

  key_object =
    create_object_init_internal
    ("textList_key", rt_defclasses -> p_key_class,
     list_out -> scope, NULLSTR);

  __objRefCntSet (OBJREF(key_object), list_out -> nrefs);
  key_object -> attrs |= OBJECT_IS_MEMBER_OF_PARENT_COLLECTION;
  *(OBJECT **)key_object -> __o_value =
    *(OBJECT **)key_object -> instancevars -> __o_value = textbox_object;

  list_head -> next = key_object;
  key_object -> prev = list_head;
  list_head = key_object;
}

void __ctalkSplitText (char *text, OBJECT *list_out) {

  char s[2], buf[MAXLABEL], *tab, tabbuf[MAXLABEL];
  int start_idx, end_idx;
  int i, j, x, y, width, height;
  bool inword;
  OBJECT *key_object, *textbox_object, *var;
  char collectbuf[MAXLABEL * 2];
  
  if (str_eq (text, NULLSTR) || *text == '\0')
    return;

  *tabbuf = 0;
  for (i = 0; i < TAB_WIDTH; ++i)
    strcatx2 (tabbuf, " ", NULL);

  start_idx = tx_message_ptr;
  natural_text = true;
  end_idx = tokenize_no_error (tx_message_push, text);
  natural_text = false;
  
  for (list_head = list_out -> instancevars; list_head && list_head -> next;
       list_head = list_head -> next)
    ;

  for (i = start_idx; i > end_idx; --i) {
    switch (M_TOK(tx_messages[i]))
      {
      case LABEL:
	switch (M_TOK(tx_messages[i-1]))
	  {
	  case ARGSEPARATOR:
	  case PERIOD:
	  case COLON:
	  case SEMICOLON:
	    strcatx (collectbuf, M_NAME(tx_messages[i]), M_NAME(tx_messages[i-1]),
		     NULL);
	    push_textlist_word (list_out, collectbuf, false);
	    --i;
	    break;
	  default:
	    push_textlist_word (list_out, M_NAME(tx_messages[i]), false);
	    break;
	  }
	break;
      case LT:
	collectbuf[0] = '<'; collectbuf[1] = 0;
	for (j = i - 1; ; j--) {
	  strcatx2 (collectbuf, M_NAME(tx_messages[j]), NULL);
	  if (M_TOK(tx_messages[j]) == GT)
	    break;
	}
	if (i < start_idx && (tx_messages[i+1] -> name[0] != '\\')) {
	  push_textlist_word (list_out, collectbuf, false);
	} else {
	  push_textlist_word (list_out, collectbuf, true);
	}
	i = j;
	break;
      case WHITESPACE:
	/* TO DO - handle a real set of tab stops. */
	if ((tab = strchr (tx_messages[i] -> name, '\t')) != NULL) {
	  if (tab - tx_messages[i] -> name < (MAXLABEL - TAB_WIDTH)) {
	    *tab = 0;
	    strcatx2 (tx_messages[i] -> name, tabbuf, NULL);
	  } else {
	    /* else leave for when we have a real example */
	  }
	} /* fall through */
      case NEWLINE:
      default:
	push_textlist_word (list_out, M_NAME(tx_messages[i]), false);
	break;
      }
  }
  
  DELETE_MESSAGES(tx_messages,tx_message_ptr,N_MESSAGES);
}

#ifdef HAVE_XFT_H
extern XftFont *selected_font;  /* THE selected outline font ptr. */
#endif

/* this takes the key object, because we often need to set
   the attribute on the previous token. set_object_wrapafter_2
   uses the textbox directly. */
void set_prev_tb_wrapafter (OBJECT *l, int val) {
  OBJECT *textbox;
  if (!IS_OBJECT(l))
    return;
  if ((textbox = (OBJECT *)TCXOBJVAL(l -> __o_value)) != NULL)
    SETINTVARS(TB_WRAPAFTER_VAR(textbox),val);

}

static int xlib_text_width (Display *d_l, char *text, XFontStruct **xfs) {
  int width;
  if (*xfs == NULL) {
    if ((*xfs = XLoadQueryFont (d_l,
			       &shm_mem[SHM_FONT_XLFD])) == NULL) {
      if ((*xfs = XLoadQueryFont (d_l, FIXED_FONT_XLFD)) == NULL) {
	printf ("ctalk: Couldn't load font, \"%s.\"\n",
		&shm_mem[SHM_FONT_XLFD]);
	/* the class lib's default width. */
	return strlen (text) * 12;
      }
    }
  }
  width = XTextWidth (*xfs, text, strlen (text));
  return width;
}

static void select_new_face (Display *d_l, Drawable d, unsigned long int gc_ptr,
			     int typeface, XFontStruct **xfs) {
  __ctalkSelectXFontFace (d_l, d, gc_ptr, typeface);
  if (!__ctalkXftInitialized ()) {
    if (*xfs) XFreeFont (d_l, *xfs);
    *xfs = NULL;
  }
}

#ifdef HAVE_XFT_H
static int __textcx_get_xft_width (Display *d_l, XftFont *selected_font,
				   char *str) {
  XGlyphInfo extents = {0,};
  XftTextExtents8 (d_l, selected_font,
		   (XftChar8 *)str, strlen (str), &extents);
  /* If a font has zero-width spaces, just use 
     the selected_font's width of an 'e' for monospaced fonts
     or "ii" for proportional fonts, which seems to look
     okay for most proportional fonts.  (we don't need
     to justify the line endings (yet)). */
  if (isspace (str[0]) && extents.width == 0) {
    if (monospace) {
      XftTextExtents8 (d_l, selected_font,
		       (XftChar8 *)"e", 1, &extents);
    } else {
      XftTextExtents8 (d_l, selected_font,
		       (XftChar8 *)"ii", 2, &extents);
    }
    extents.width *= strlen (str);
  }

  return extents.width;
}
#endif /* HAVE_XFT_H */

static void center_text (Display *d_l, unsigned long int gc_ptr,
			 XFontStruct **xfs,
			 OBJECT *center_start_key, OBJECT *center_end_key,
			 int pane_width, int lmargin, int line_y) {
  OBJECT *l, *t;
  char buf[MAXMSG];
  int line_width, word_width, line_lmargin;
  int x_dim, y_dim, height_dim;

  *buf = 0;
  for (l = center_start_key; ; l = l -> next) {
    if ((t = (OBJECT *)TCXOBJVAL(l -> __o_value)) == NULL) {
      fprintf (stderr, "ctalk: NULL textbox in center_text.\n");
      continue;
    }
    strcatx2 (buf, t -> instancevars -> __o_value, NULL);
    if (l == center_end_key)
      break;
  }

#ifdef HAVE_XFT_H
  if (__ctalkXftInitialized ()) {
    line_width = __textcx_get_xft_width (d_l, selected_font, buf);
  } else {
    line_width = xlib_text_width (d_l, buf, xfs);
  }
#else
    line_width = xlib_text_width (d_l, buf, xfs);
#endif  

  line_lmargin = (pane_width / 2) - (line_width / 2);
  for (l = center_start_key; ; l = l -> next) {
    if ((t = TCXOBJVAL(l -> __o_value)) == NULL) {
      fprintf (stderr, "ctalk: NULL textbox in center_text.\n");
      continue;
    }
#ifdef HAVE_XFT_H
    if (__ctalkXftInitialized ()) {
      word_width = __textcx_get_xft_width (d_l, selected_font, 
					   t -> instancevars -> __o_value);
    } else {
      word_width = xlib_text_width (d_l, 
				    t -> instancevars -> __o_value, xfs);
    }
#else
      word_width = xlib_text_width (d_l, 
				    t -> instancevars -> __o_value, xfs);
#endif    
      SETINTVARS(TB_X_VAR(t),line_lmargin);
      /* the Y position of the box is always relative to the vertical
       scrolling. */
    line_lmargin += word_width;
    if (l == center_end_key)
      break;
  }
  
}

/* The definition from List class. */
#define __LIST_HEAD(__o) ((__o)->instancevars->next)

static void combine_tboxes (OBJECT *t1, OBJECT *t2) {
  int t1_width, t2_width;
  char intbuf[0xff], textbuf[MAXMSG];
  t1_width = INTVAL(TB_WIDTH_VAR(t1)->__o_value);
  t2_width = INTVAL(TB_WIDTH_VAR(t2)->__o_value);
  strcatx (textbuf, t1 -> __o_value, t2 -> __o_value, NULL);
  __xfree (MEMADDR(t1 -> __o_value));
  __xfree (MEMADDR(t1 -> instancevars -> __o_value));
  t1 -> __o_value = strdup (textbuf);
  t1 -> instancevars -> __o_value = strdup (textbuf);
  SETINTVARS(TB_WIDTH_VAR(t1),(t1_width + t2_width));
  if (INTVAL(TB_WRAPAFTER_VAR(t2) -> __o_value) == 1)
    SETINTVARS(TB_WRAPAFTER_VAR(t1),1);
}


/* #define PRINT_TEXT_LIST_SIZE */

static void compress_tokens (OBJECT *text_list) {
  OBJECT *l, *l_del, *tbox, *tbox_next;
#ifdef PRINT_TEXT_LIST_SIZE
  int lsize1, lsize2;
#endif  

#ifdef PRINT_TEXT_LIST_SIZE
  l = __LIST_HEAD(text_list);
  lsize1 = 0;
  for (l = __LIST_HEAD(text_list); l; l = l -> next) {
    ++lsize1;
  }
  printf ("list size 1: %d\n", lsize1);
#endif  

    /* 
       On the first pass, combine alphanumeric word boxes with
       the spaces of the following token, if any. 
    */
  l = __LIST_HEAD(text_list);
  while (l -> next) {
    tbox = TCXOBJVAL(l -> __o_value);
    tbox_next = TCXOBJVAL(l -> next -> __o_value);

    if (isalnum (tbox -> __o_value[0]) &&
	isblank (tbox_next -> __o_value[0])) {
      combine_tboxes (tbox, tbox_next);
      l_del = l -> next;
      l -> next = l_del -> next;
      if (l_del -> next)
	l_del -> next -> prev = l;
      l_del -> next = l_del -> prev = NULL;
      if (!(tbox_next -> scope & METHOD_USER_OBJECT)) {
	__objRefCntZero (OBJREF(tbox_next));
	__ctalkDeleteObject (tbox_next);
      }
      if (!(l_del -> scope & METHOD_USER_OBJECT)) {
	__objRefCntZero (OBJREF(l_del));
	__ctalkDeleteObject (l_del);
      }
    }
    
    if ((l = l -> next) == NULL)
      break;
  }

#ifdef PRINT_TEXT_LIST_SIZE
  l = __LIST_HEAD(text_list);
  lsize2 = 0;
  for (l = __LIST_HEAD(text_list); l; l = l -> next) {
    ++lsize2;
  }
  printf ("list size 2: %d\n", lsize2);
#endif

  /*
    On the second and any following passes, combine sequential
    textboxes that start with an alphanumeric character and
    end with a space.  In practice, any following passes don't
    combine many more tokens this way.
   */
  l = __LIST_HEAD(text_list);
  while (l -> next) {
    tbox = TCXOBJVAL(l -> __o_value);
    tbox_next = TCXOBJVAL(l -> next -> __o_value);
    if (isalnum (tbox -> __o_value[0]) &&
	isblank (tbox -> __o_value[strlen (tbox -> __o_value)-1]) &&
	isalnum (tbox_next -> __o_value[0]) &&
	isblank (tbox_next -> __o_value[strlen (tbox -> __o_value)-1])) {
      combine_tboxes (tbox, tbox_next);
      l_del = l -> next;
      l -> next = l -> next -> next;
      if (l_del -> next)
	l_del -> next -> prev = l;
      if (!(tbox_next -> scope & METHOD_USER_OBJECT)) {
	__objRefCntZero (OBJREF(tbox_next));
	__ctalkDeleteObject (tbox_next);
      }
      if (!(l_del -> scope & METHOD_USER_OBJECT)) {
	__objRefCntZero (OBJREF(l_del));
	__ctalkDeleteObject (l_del);
      }
    }
	
    if ((l = l -> next) == NULL)
      break;
  }

#ifdef PRINT_TEXT_LIST_SIZE
  l = __LIST_HEAD(text_list);
  lsize2 = 0;
  for (l = __LIST_HEAD(text_list); l; l = l -> next) {
    ++lsize2;
  }
  printf ("list size 3: %d\n", lsize2);
#endif

}

static char starting_xlfd[MAXLABEL] = "";

void __ctalkWrapText (int dr, unsigned long int gc_ptr,
		      OBJECT *text_list, int pane_width, int lmargin) {

  Display *l_display;
  int line_height, x, y, width, char_height, line_length,
    line_max_length;
  int x_dim, y_dim, height_dim;
  OBJECT *l, *textbox, *var, *center_start, *prev_textbox;
  Font fid;
  XFontStruct *xfs = NULL;
  XGCValues v;
  Drawable d = (Drawable)dr;
  Font newfont;
  int r;
  int tag_length;
  long long int offsets[MAXARGS];
  char tag_text[MAXLABEL];
  char xlfd[0xff];
  bool escaped_tag;
  bool centering_text = false;
  bool xft;

  l_display = XOpenDisplay (getenv ("DISPLAY"));
  xft = __ctalkXftInitialized ();

#ifdef HAVE_XFT_H
  if (xft) {
    if (selected_font) {
      char_height = selected_font -> height;
      line_height = char_height + 2;
    } else {
      char_height = 18;
    }
  } else  {
    if (!shm_mem_attached) {
      if ((shm_mem = (char *)get_shmem (mem_id)) == NULL) {
	_exit (0);
      }
      shm_mem_attached = true;
    }
    if (*starting_xlfd == 0) {
      strcpy (starting_xlfd, &shm_mem[SHM_FONT_XLFD]);
    }
    if ((xfs = XLoadQueryFont (l_display, starting_xlfd)) == NULL) {
      if ((xfs = XLoadQueryFont (l_display, FIXED_FONT_XLFD)) == NULL) {
	printf ("__ctalkWrapText: Couldn't load font.\n");
	return;
      }
    }
    char_height = xfs -> max_bounds.ascent + xfs -> max_bounds.descent;
    line_height = char_height;
  }
#else /* HAVE_XFT_H */
  if (!shm_mem_attached) {
    if ((shm_mem = (char *)get_shmem (mem_id)) == NULL) {
      _exit (0);
    }
    shm_mem_attached = true;
  }
  if (*starting_xlfd == 0) {
    strcpy (starting_xlfd, &shm_mem[SHM_FONT_XLFD]);
  }
  if ((xfs = XLoadQueryFont (l_display, starting_xlfd)) == NULL) {
    if ((xfs = XLoadQueryFont (l_display, FIXED_FONT_XLFD)) == NULL) {
      printf ("__ctalkWrapText: Couldn't load font.\n");
      return;
    }
  }
  char_height = xfs -> max_bounds.ascent + xfs -> max_bounds.descent;
  line_height = char_height;
#endif /* HAVE_XFT_H */  
  
  x = lmargin; y = line_height;
  line_length = 0;
  line_max_length = pane_width - lmargin;

  for (l = __LIST_HEAD(text_list); l; l = l -> next) {

    if ((textbox = TCXOBJVAL(l -> __o_value)) == NULL) {
      fprintf (stderr, "WARNING! NULL textbox in __ctalkWrapText.\n");
      continue;
    }
  
    if (textbox -> instancevars -> __o_value[0] == '<') {
      if (INTVAL(TB_ESCTAG_VAR(textbox) -> __o_value) == 1) {
	escaped_tag = true;
      } else {
	escaped_tag = false;
      }
      tag_length = strlen (textbox -> instancevars -> __o_value) - 1;
    }

    if (textbox -> instancevars -> __o_value[0] == '<' &&
	textbox -> instancevars -> __o_value[tag_length] == '>') {
      if (escaped_tag) {
	SETINTVARS(TB_WIDTH_VAR(prev_textbox), 0);
	SETINTVARS(TB_HEIGHT_VAR(prev_textbox), char_height);
#ifdef HAVE_XFT_H
	if (xft) {
	  /* We have to use l_display here, or we run into
	     concurrency problems with the main display connection
	     (which is what __ctalkXftGetStringDimensions uses). */
	  width = __textcx_get_xft_width 
	    (l_display, selected_font,
	     textbox -> instancevars -> __o_value);
	} else  {
	  width = xlib_text_width (l_display,
				   textbox -> instancevars -> __o_value,
				   &xfs); 
	}
#else
	width = xlib_text_width (l_display,
				 textbox -> instancevars -> __o_value,
				 &xfs); 
#endif	
	SETINTVARS(TB_WIDTH_VAR(textbox), width);
	SETINTVARS(TB_HEIGHT_VAR(textbox), char_height);
      } else { /* if (escaped_tag)*/
	width = 0;
	SETINTVARS(TB_WIDTH_VAR(textbox), width);
	SETINTVARS(TB_HEIGHT_VAR(textbox), char_height);
	if (__ctalkMatchText ("<(/*\\l*)>", textbox -> instancevars -> __o_value,
			      offsets)) {
	  strcpy (tag_text, __ctalkMatchAt (0));
	  if (str_eq (tag_text, "/b") ||
	      str_eq (tag_text, "/i")) {
	    /*select_new_face (l_display, gc_ptr, X_FACE_REGULAR, &xfs);*/
	    select_new_face (l_display, d, gc_ptr, X_FACE_REGULAR, &xfs);
	  } else if (str_eq (tag_text, "b")) {
	    select_new_face (l_display, d, gc_ptr, X_FACE_BOLD, &xfs);
	    /*select_new_face (l_display, gc_ptr, X_FACE_BOLD, &xfs);*/
	  } else if (str_eq (tag_text, "i")) {
	    /*select_new_face (l_display, gc_ptr, X_FACE_ITALIC, &xfs);*/
	    select_new_face (l_display, d, gc_ptr, X_FACE_ITALIC, &xfs);
	  } else if (str_eq (tag_text, "center")) {
	    if (l -> next) {
	      center_start = l -> next;
	      centering_text = true;
	    }
	  } else if (str_eq (tag_text, "/center")) {
	    center_text (l_display, gc_ptr, &xfs, center_start, l -> prev,
			 pane_width, lmargin, y);
	    centering_text = false;
	    continue;
	  }
	} else {
	  printf ("ctalk: unknown tag: %s\n",
		  textbox -> instancevars -> __o_value);
	}
      } /* if (escaped_tag) */
    } else if (textbox -> instancevars -> __o_value[0] == '\n' ||
	       textbox -> instancevars -> __o_value[0] == '\r') {
      width = 0;
      SETINTVARS(TB_WIDTH_VAR(textbox), width);
      SETINTVARS(TB_HEIGHT_VAR(textbox), char_height);
    } else {
#ifdef HAVE_XFT_H
      if (xft) {
	/* We have to use l_display here, or we run into
	   concurrency problems with the main display connection
	   (which is what __ctalkXftGetStringDimensions uses). */
	width = __textcx_get_xft_width 
	  (l_display, selected_font,
	   textbox -> instancevars -> __o_value);
      } else  {
	width = xlib_text_width (l_display,
				 textbox -> instancevars -> __o_value,
				 &xfs); 
      }
#else
      width = xlib_text_width (l_display,
			       textbox -> instancevars -> __o_value,
			       &xfs); 
#endif      

      SETINTVARS(TB_WIDTH_VAR(textbox), width);
      SETINTVARS(TB_HEIGHT_VAR(textbox),char_height);
    }
    if (centering_text)
      continue;
    
    if (textbox -> instancevars -> __o_value[0] == '\n' ||
	textbox -> instancevars -> __o_value[0] == '\r') {
      SETINTVARS(TB_X_VAR(textbox),x);
      x = lmargin;
      y += line_height;
      line_length = 0;
      SETINTVARS(TB_WRAPAFTER_VAR(textbox),TRUE);
    } else if ((line_length + width) > line_max_length) {
      x = lmargin; y += line_height;
      line_length = 0;
      SETINTVARS(TB_X_VAR(textbox),x);
      x += width;
      line_length += width;
      set_prev_tb_wrapafter (l -> prev, TRUE);
      SETINTVARS(TB_WRAPAFTER_VAR(textbox),FALSE);
      if (isblank (textbox -> instancevars -> __o_value[0])
	  && (textbox -> instancevars -> __o_value[1] == '\0')) {
	x = lmargin;
	SETINTVARS(TB_WIDTH_VAR(textbox), 0);
	SETINTVARS(TB_HEIGHT_VAR(textbox), line_height);
      }
    } else {
      SETINTVARS(TB_X_VAR(textbox),x);
      x += width;
      line_length += width;
      SETINTVARS(TB_WRAPAFTER_VAR(textbox),FALSE);
    }
    prev_textbox = textbox;
  }

  if (!xft) {
    if (xfs) XFreeFont (l_display, xfs);
  }
  XCloseDisplay (l_display);

  compress_tokens (text_list);
}

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

bool natural_text = false;

void __ctalkSetWordWrap (bool b) {
  printf ("__ctalkSetWordWrap: This function requires the X Window System.\n");
}
bool __ctalkGetWordWrap (void) {
  printf ("__ctalkGetWordWrap: This function requires the X Window System.\n");
  return false;
}
void __ctalkSplitText (char *text, OBJECT *list_out) {
  printf ("__ctalkSplitText: This function requires the X Window System.\n");
}
void __ctalkWrapText (int dr, unsigned long int gc_ptr,
		      OBJECT *text_list, int pane_width, int lmargin) {
    printf ("__ctalkWrapText: This function requires the X Window System.\n");
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
