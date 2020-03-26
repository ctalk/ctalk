/* $Id: x11defs.h,v 1.19 2020/03/26 02:58:39 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2014, 2016, 2019, 2020 
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

#ifndef _X11DEFS_H

#include <X11/Xlib.h>

#if defined(__sparc__) && defined(__svr4__)
#define GRAPHICS_WRITE_SEND_EVENT 1 
#endif

#define SHM_REQ 0x00
#define SHM_DISPLAY 0x01
#define SHM_DRAWABLE 0x20
#define SHM_GC 0x40
#define SHM_DATA 0x60
#define SHM_RETVAL 0xe0
#define SHM_SHUTDOWN 0xfe
/*
 *  Font data locations - will probably need more of them.
 */
#define SHM_FONT_BASE_ADDR  0x800
#define SHM_FONT_FID SHM_FONT_BASE_ADDR
#define SHM_FONT_XLFD SHM_FONT_BASE_ADDR + 0xff

#define SHM_FONT_FT_FAMILY 	SHM_FONT_BASE_ADDR + 0x100
#define SHM_FONT_FT_WEIGHT 	SHM_FONT_BASE_ADDR + 0x130
#define SHM_FONT_FT_SLANT 	SHM_FONT_BASE_ADDR + 0x140
#define SHM_FONT_FT_DPI 	SHM_FONT_BASE_ADDR + 0x150
#define SHM_FONT_FT_PT_SIZE 	SHM_FONT_BASE_ADDR + 0x160
#define SHM_FONT_FT_RED 	SHM_FONT_BASE_ADDR + 0x170
#define SHM_FONT_FT_GREEN 	SHM_FONT_BASE_ADDR + 0x180
#define SHM_FONT_FT_BLUE 	SHM_FONT_BASE_ADDR + 0x190
#define SHM_FONT_FT_ALPHA 	SHM_FONT_BASE_ADDR + 0x200

/* needed? */ /***/
/* #define SHM_TEXT_LINES          0x1100 */

#define SHM_EVENT_BASE      0x1100
#define SHM_EVENT_READY     SHM_EVENT_BASE
#define SHM_EVENT_TYPE      0x1110
#define SHM_EVENT_WIN       0x1120
#define SHM_EVENT_DATA1     0x1130
#define SHM_EVENT_DATA2     0x1140
#define SHM_EVENT_DATA3     0x1150
#define SHM_EVENT_DATA4     0x1160
#define SHM_EVENT_DATA5     0x1170
#define SHM_EVENT_DATA6     0x1180

#define SHM_EVENT_MASK      0x1190

/* This will make it easier to add/remove a conversion from a GC object
   to the C pointer in the future. */
#define make_req_gc_str(s,r,w,g,d)		\
  make_req(s,r,w,strtoul(g, NULL, 0),d)

#ifdef __x86_64
#define make_req(s,p,r,w,g,d) do {(s)[SHM_REQ] = (r);	\
    s[SHM_DISPLAY] =   ((uintptr_t)p & 0xf00000000000) >> 44;	\
    s[SHM_DISPLAY+1] = ((uintptr_t)p & 0xf0000000000) >> 40;	\
    s[SHM_DISPLAY+2] = ((uintptr_t)p & 0xf000000000) >> 36;	\
    s[SHM_DISPLAY+3] = ((uintptr_t)p & 0xf00000000) >> 32;	\
    s[SHM_DISPLAY+4] = ((uintptr_t)p & 0xf0000000) >> 28;	\
    s[SHM_DISPLAY+5] = ((uintptr_t)p & 0xf000000) >> 24;	\
    s[SHM_DISPLAY+6] = ((uintptr_t)p & 0xf00000) >> 20;		\
    s[SHM_DISPLAY+7] = ((uintptr_t)p & 0xf0000) >> 16;		\
    s[SHM_DISPLAY+8] = ((uintptr_t)p & 0xf000) >> 12;		\
    s[SHM_DISPLAY+9] = ((uintptr_t)p & 0xf00) >> 8;		\
    s[SHM_DISPLAY+10] = ((uintptr_t)p & 0xf0) >> 4;		\
    s[SHM_DISPLAY+11] = ((uintptr_t)p & 0xf);			\
    s[SHM_DRAWABLE] = (w & 0xf0000000) >> 28;		\
    s[SHM_DRAWABLE+1] = (w & 0xf000000) >> 24;		\
    s[SHM_DRAWABLE+2] = (w & 0xf00000) >> 20;		\
    s[SHM_DRAWABLE+3] = (w & 0xf0000) >> 16;		\
    s[SHM_DRAWABLE+4] = (w & 0xf000) >> 12;		\
    s[SHM_DRAWABLE+5] = (w & 0xf00) >> 8;		\
    s[SHM_DRAWABLE+6] = (w & 0xf0) >> 4;		\
    s[SHM_DRAWABLE+7] = (w & 0xf);			\
    s[SHM_GC] = ((uintptr_t)g & 0xf00000000000) >> 44;	\
    s[SHM_GC+1] = ((uintptr_t)g & 0xf0000000000) >> 40;	\
    s[SHM_GC+2] = ((uintptr_t)g & 0xf000000000) >> 36;	\
    s[SHM_GC+3] = ((uintptr_t)g & 0xf00000000) >> 32;	\
    s[SHM_GC+4] = ((uintptr_t)g & 0xf0000000) >> 28;	\
    s[SHM_GC+5] = ((uintptr_t)g & 0xf000000) >> 24;	\
    s[SHM_GC+6] = ((uintptr_t)g & 0xf00000) >> 20;	\
    s[SHM_GC+7] = ((uintptr_t)g & 0xf0000) >> 16;	\
    s[SHM_GC+8] = ((uintptr_t)g & 0xf000) >> 12;	\
    s[SHM_GC+9] = ((uintptr_t)g & 0xf00) >> 8;		\
    s[SHM_GC+10] = ((uintptr_t)g & 0xf0) >> 4;		\
    s[SHM_GC+11] = ((uintptr_t)g & 0xf);		\
    strcpy (&((s)[SHM_DATA]), d); } while (0);
#else

#define make_req(s,p,r,w,g,d) do {(s)[SHM_REQ] = (r);	\
    s[SHM_DISPLAY] = ((uintptr_t)p & 0xf0000000) >> 28;		\
    s[SHM_DISPLAY+1] = ((uintptr_t)p & 0xf000000) >> 24;	\
    s[SHM_DISPLAY+2] = ((uintptr_t)p & 0xf00000) >> 20;		\
    s[SHM_DISPLAY+3] = ((uintptr_t)p & 0xf0000) >> 16;		\
    s[SHM_DISPLAY+4] = ((uintptr_t)p & 0xf000) >> 12;		\
    s[SHM_DISPLAY+5] = ((uintptr_t)p & 0xf00) >> 8;		\
    s[SHM_DISPLAY+6] = ((uintptr_t)p & 0xf0) >> 4;		\
    s[SHM_DISPLAY+7] = ((uintptr_t)p & 0xf);			\
    s[SHM_DRAWABLE] = (w & 0xf0000000) >> 28;		\
    s[SHM_DRAWABLE+1] = (w & 0xf000000) >> 24;		\
    s[SHM_DRAWABLE+2] = (w & 0xf00000) >> 20;		\
    s[SHM_DRAWABLE+3] = (w & 0xf0000) >> 16;		\
    s[SHM_DRAWABLE+4] = (w & 0xf000) >> 12;		\
    s[SHM_DRAWABLE+5] = (w & 0xf00) >> 8;		\
    s[SHM_DRAWABLE+6] = (w & 0xf0) >> 4;		\
    s[SHM_DRAWABLE+7] = (w & 0xf);			\
    s[SHM_GC] = ((uintptr_t)g & 0xf0000000) >> 28;		\
    s[SHM_GC+1] = ((uintptr_t)g & 0xf000000) >> 24;		\
    s[SHM_GC+2] = ((uintptr_t)g & 0xf00000) >> 20;		\
    s[SHM_GC+3] = ((uintptr_t)g & 0xf0000) >> 16;		\
    s[SHM_GC+4] = ((uintptr_t)g & 0xf000) >> 12;		\
    s[SHM_GC+5] = ((uintptr_t)g & 0xf00) >> 8;		\
    s[SHM_GC+6] = ((uintptr_t)g & 0xf0) >> 4;		\
    s[SHM_GC+7] = ((uintptr_t)g & 0xf);			\
    strcpy (&((s)[SHM_DATA]), d); } while (0);

#endif

#define wait_req(s) do { usleep (1000); } while ((s)[SHM_REQ]) 

#define DEFAULT_GCV_MASK (GCFunction | GCFillStyle | GCForeground | \
			  GCBackground | GCFont)
#define PANE_PUT_STR_REQUEST   1
#define PANE_WM_TITLE_REQUEST  2
#define PANE_CLEAR_WINDOW_REQUEST 3
#define PANE_CLEAR_RECTANGLE_REQUEST 4
#define PANE_DRAW_POINT_REQUEST 5
#define PANE_DRAW_LINE_REQUEST 6
#define PANE_DRAW_RECTANGLE_REQUEST 7
#define PANE_RESIZE_REQUEST 8
#define PANE_MOVE_REQUEST 9
#define PANE_REFRESH_REQUEST 10
#define PANE_CURSOR_REQUEST 11
#define PANE_CHANGE_GC_REQUEST 12
#define PANE_SET_WINDOW_BACKGROUND_REQUEST 13
#define PANE_RESIZE_PIXMAP_REQUEST 14
#define PANE_XPM_FROM_DATA_REQUEST 15
#define PANE_SET_RESOURCE_REQUEST 16
#define PANE_COPY_PIXMAP_REQUEST 17
#define PANE_DRAW_CIRCLE_REQUEST 18
#ifdef HAVE_XFT_H
#define PANE_PUT_STR_REQUEST_FT   19
#endif

#define PANE_XLIB_FACE_REQUEST     20
#define PANE_XLIB_FACE_REQUEST_FT  21
#define PANE_TEXT_FROM_DATA_REQUEST 22
#define PANE_GET_PRIMARY_SELECTION_REQUEST 23


#define WM_CONFIGURE_EVENTS (ExposureMask|StructureNotifyMask|PropertyChangeMask|SubstructureNotifyMask|FocusChangeMask|EnterWindowMask|LeaveWindowMask)
#define WM_INPUT_EVENTS (KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask)
#define KBDCHAR             (1 << 0)
#define KBDCUR              (1 << 1)
#define WINDELETE           (1 << 2)
#define BUTTONPRESS         (1 << 3)
#define BUTTONRELEASE       (1 << 4)
#define KEYPRESS            (1 << 5)
#define KEYRELEASE          (1 << 6)
#define MOTIONNOTIFY        (1 << 7)
#define EXPOSE              (1 << 8)
#define MAPNOTIFY           (1 << 12)
#define CONFIGURENOTIFY     (1 << 15)
#define MOVENOTIFY          (1 << 16)
#define RESIZENOTIFY        (1 << 17)
#define SELECTIONREQUEST    (1 << 18)
#define SELECTIONCLEAR      (1 << 19)
#define WMFOCUSCHANGENOTIFY (1 << 20)
#define ENTERWINDOWNOTIFY   (1 << 21)
#define LEAVEWINDOWNOTIFY   (1 << 22)
#define FOCUSIN             (1 << 23)
#define FOCUSOUT            (1 << 24)
/***/
/* To be added. */
/*  #define DESTROYNOTIFY    (1 << 9) */
/*  #define VISIBILITYCHANGE (1 << 10) */
/*  #define STRUCTURENOTIFY  (1 << 11) */
/*  #define MAPREQUEST       (1 << 13) */
/*  #define CONFIGUREREQUEST (1 << 14) */

/* If changing these, also change in X11TextEditorPane class. */
#define BUTTON1MASK  (1 << 0)
#define BUTTON2MASK  (1 << 1)
#define BUTTON3MASK  (1 << 2)  

#define IS_XK_MODIFIER(k) (((k) == XK_Shift_L) ||	\
 ((k) == XK_Shift_R) || ((k) == XK_Control_L) || \
   ((k) == XK_Control_R) || ((k) == XK_Caps_Lock) || \
    ((k) == XK_Shift_Lock) || ((k) == XK_Meta_L) || \
    ((k) == XK_Meta_R) || ((k) == XK_Alt_L) || \
    ((k) == XK_Alt_R) || ((k) == XK_Super_L) || \
    ((k) == XK_Super_R) || ((k) == XK_Hyper_L) || \
    ((k) == XK_Hyper_R))

#define CURSOR_GRAB_MOVE 1
#define CURSOR_SCROLL_ARROW 2
#define CURSOR_WATCH 3
#define CURSOR_ARROW 4

#define PANEBUFFER_INSTANCE_VAR "paneBuffer"
#define PANEBACKINGSTORE_INSTANCE_VAR "paneBackingStore"
#define X11BITMAP_ID_INSTANCE_VAR "xID"
#define X11BITMAP_GC_INSTANCE_VAR "xGC"

#define DEFAULT_WIDTH 250
#define DEFAULT_HEIGHT 250
#define DEFAULT_X 100
#define DEFAULT_Y 100

#define FIXED_FONT_XLFD "*-fixed-*-*-*-*-*-120-*"

#define X_FACE_REGULAR     (1 << 0)
#define X_FACE_BOLD        (1 << 1)
#define X_FACE_ITALIC      (1 << 2)
#define X_FACE_BOLD_ITALIC (1 << 3)

/* Contents of the server-to-client event buffer when 
   sending over the socket. */
#define _SCLASS 0
#define _SWIN   1
#define _SDT1   2
#define _SDT2   3
#define _SDT3   4
#define _SDT4   5
#define _SDT5   6
#define _SDT6   7

/* Comment out if you don't want to use the GC range check. (The
   X11 server glue still uses the SIGSEGV handler.) */
#define GC_RANGE_CHECK

typedef struct _dc {
  Display *d_p;
  int d_p_screen, d_p_screen_depth;
  Window d_p_root;
  bool mapped;
} DIALOG_C;

#define DIALOG(d) (dpyrec && dpyrec -> mapped ? true : false)

#endif /* _X11DEFS_H */
