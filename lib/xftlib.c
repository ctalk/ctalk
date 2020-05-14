/* $Id: xftlib.c,v 1.46 2020/05/14 15:25:29 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2017-2020 Robert Kiesling, rk3314042@gmail.com.
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

/* This is still experimental. */
/* #define FC_PATTERN_SET */

/* prototype from ctalk.h without the include dependencies */
#ifdef __GNUC__
void _error (char *, ...) __attribute__ ((noreturn));
#else
void _error (char *, ...);
#endif

static void xft_support_error (void) {
  _error ("The Xft font library is not supported on this system. "
	  "You must install the library\n"
	  "and rebuild Ctalk with the library support. "
	  "Type \"./configure --help\" in the top-level\n"
	  "source directory for a list of configuration options.\n");
}

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)

#ifdef HAVE_XFT_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fontconfig/fontconfig.h>
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>
#include FT_FREETYPE_H
#include "x11defs.h"
#include "xftfont.h"
#include <object.h>
#include <message.h>
#include <ctalk.h>

#define SUCCESS 0
#define ERROR -1
#define MAXMSG 8192

#define FT2_CT_ENCODING FT_ENCODING_ADOBE_LATIN_1

extern int is_dir (char *path);
char *__ctalkXftSelectedFontPath (void);

extern Display *display;  /* Defined in x11lib.c.  */

extern DIALOG_C *dpyrec;
#define DISPLAY (((dpyrec == NULL) || (dpyrec -> mapped == false)) ?	\
		 display : dpyrec -> d_p)

extern void sync_ft_font (bool);

FT_Library ft2_library = NULL;
FT_Face ft2_selectedface = NULL;

FcConfig *config = NULL;
XftFont *selected_font = NULL;
/* static char selected_font_path[FILENAME_MAX];*//***/

unsigned short fgred = 0, fggreen = 0, fgblue = 0, fgalpha = 0xffff;

static double rotation = 0.0f, scaleX = 0.0f, scaleY = 0.0f;

#define XFT_CONF_FILE "/usr/X11R6/lib/X11/XftConfig"

/* List of scalable families to look for when selecting a default font. */
#ifdef __APPLE__
static char *default_families[] = {
  "sans-serif",
  "Bitstream Vera Sans Mono",
  "Courier",
  "monospace",
  NULL
};
#else
static char *default_families[] = {
  "sans-serif",
  "DejaVu Sans Mono",
  "FreeMono",
  "Ubuntu Mono",
  "Liberation Mono",
  "monospace",
  NULL
};
#endif

static int xft_message_level = 0;

/* set some intelligent defaults. */
char selected_family[256] = "DejaVu Sans Mono";
int selected_slant = XFT_SLANT_ROMAN;
int selected_weight = XFT_WEIGHT_MEDIUM;
int selected_dpi = 72;
int selected_spacing = 90; /* dual */
int selected_width = 100;  /* normal */
double selected_pt_size = 12.0f;
bool monospace = true;

/* fill in when we load a font. */
static char selected_filename[FILENAME_MAX] = "";

static void xft_config_error (void) {
  printf ("%s: Could not find Xft outline font configuration, "
	  "exiting.\n", __argvFileName ());
  printf ("(Try using bitmap fonts if possible. Or, refer to the "
	  "xlsfonts(1) manual\n"
	  "page and the X11Font section of the "
	  "Ctalk Language Reference. Or, add\n"
	  "the Xft configuration to the machine.  Refer to the "
	  "Xft(3) manual page\n"
	  "and the X11FreeTypeFont class in the Ctalk Language "
	  "Reference.)\n");
  exit (EXIT_FAILURE);
}

static void xft_selectfont_error (void) {
  printf ("%s: Could not open a default Xft outline font, "
	  "exiting.\n", __argvFileName ());
  printf ("(Try using bitmap fonts if possible. Or, refer to the "
	  "xlsfonts(1) manual\n"
	  "page and the X11Font section of the "
	  "Ctalk Language Reference. Or, add\n"
	  "the Xft configuration to the machine.  Refer to the "
	  "Xft(3) manual page\n"
	  "and the X11FreeTypeFont class in the Ctalk Language "
	  "Reference.)\n");
  exit (EXIT_FAILURE);
}

static struct {
  char name[64];
  int value;
} fc_slant[4] = {{"italic", XFT_SLANT_ITALIC}, {"oblique", XFT_SLANT_OBLIQUE},
		 {"roman", XFT_SLANT_ROMAN}, {"", -1}};

static struct {
  char name[64];
  int value;
} fc_weight[17] = {{"thin", 0}, {"extralight", 40},
		   {"ultralight", 40}, {"light", XFT_WEIGHT_LIGHT},
		   {"demilight", 55}, {"semilight", 55},
		   {"book", 75}, {"regular", 75},
		   {"normal", 75}, {"regular", 80},
		   {"medium", XFT_WEIGHT_MEDIUM},
		   {"demibold", XFT_WEIGHT_DEMIBOLD},
		   {"semibold", XFT_WEIGHT_DEMIBOLD},
		   {"bold", XFT_WEIGHT_BOLD}, {"extrabold", 205},
		   {"black", XFT_WEIGHT_BLACK}, {"", -1}};
static struct {
  char name[64];
  int value;
} fc_spacing[5] = {{"proportional", XFT_PROPORTIONAL},
		   {"dual", 90},
		   {"mono", XFT_MONO},
		   {"charcell", XFT_CHARCELL},
		   {"", -1}};
static struct {
  char name[64];
  int value;
} fc_width[10] = {{"ultracondensed", 50},
		 {"extracondensed", 63},
		 {"condensed", 75},
		 {"semicondensed", 87},
		 {"normal", 100},
		 {"semiexpanded", 113},
		 {"expanded", 125},
		 {"extraexpanded", 150},
		 {"ultraexpanded", 200},
		 {"", -1}};

static char *fmt_fc_desc_str (FcPattern *pat) {
  static char s[MAXMSG];
  int i;
  FcChar8 *actual_family;
  double actual_pt_size;
  int actual_slant, actual_weight, actual_spacing, actual_width;

  XftPatternGetString (pat, FC_FAMILY,
		       0, (FcChar8 **)&actual_family);
  XftPatternGetDouble (pat, FC_SIZE,
		       0, &actual_pt_size);
  XftPatternGetInteger (pat, FC_SLANT, 0, &actual_slant);
  XftPatternGetInteger (pat, FC_WEIGHT, 0, &actual_weight);
  XftPatternGetInteger (pat, FC_WEIGHT, 0, &actual_weight);
  XftPatternGetInteger (pat, FC_SPACING, 0, &actual_spacing);

  sprintf (s, "%s-%d", actual_family, (int)actual_pt_size);
  for (i = 0; fc_slant[i].name[0]; ++i) {
    if (fc_slant[i].value == actual_slant) {
      strcatx2 (s, ":slant=", fc_slant[i].name, NULL);
      break;
    }
  }
  for (i = 0; fc_weight[i].name[0]; ++i) {
    if (fc_weight[i].value == actual_weight) {
      strcatx2 (s, ":weight=", fc_weight[i].name, NULL);
      break;
    }
  }
  for (i = 0; fc_spacing[i].name[0]; ++i) {
    if (fc_spacing[i].value == actual_spacing) {
      strcatx2 (s, ":spacing=", fc_spacing[i].name, NULL);
      break;
    }
  }
  for (i = 0; fc_width[i].name[0]; ++i) {
    if (fc_width[i].value == actual_width) {
      strcatx2 (s, ":width=", fc_width[i].name, NULL);
      break;
    }
  }
  return s;
}

char *__ctalkXftDescStr (void) {
  int i;
  static char s[MAXMSG];

  sprintf (s, "%s-%d", selected_family, (int)selected_pt_size);
  for (i = 0; fc_slant[i].name[0]; ++i) {
    if (fc_slant[i].value == selected_slant) {
      strcatx2 (s, ":slant=", fc_slant[i].name, NULL);
      break;
    }
  }
  for (i = 0; fc_weight[i].name[0]; ++i) {
    if (fc_weight[i].value == selected_weight) {
      strcatx2 (s, ":weight=", fc_weight[i].name, NULL);
      break;
    }
  }
  for (i = 0; fc_width[i].name[0]; ++i) {
    if (fc_width[i].value == selected_width) {
      strcatx2 (s, ":width=", fc_width[i].name, NULL);
      break;
    }
  }
  return s;
}

/* Some older include files don't define this. */
#ifndef FC_WIDTH
#define FC_WIDTH "width"
#endif

static FcPattern *construct_pattern (char *p_family,
				     double p_size,
				     int p_slant,
				     int p_weight, int p_dpi,
				     int p_spacing,
				     int p_width) {
  static FcPattern *pat;

  pat = FcPatternCreate ();

  if (*p_family)
    FcPatternAddString (pat, FC_FAMILY, p_family);
  if (p_slant >= 0)
    FcPatternAddInteger (pat, FC_SLANT,p_slant);
  if (p_size >= 0.0)
    FcPatternAddDouble (pat, FC_SIZE, p_size);
  if (p_weight >= 0)
    FcPatternAddInteger (pat, FC_WEIGHT, p_weight);
  if (p_dpi >= 0) 
    FcPatternAddDouble (pat, FC_DPI, (double)p_dpi);
  if (p_spacing >= 0)
    FcPatternAddInteger (pat, FC_SPACING, p_spacing);
  if (p_width >= 0)
    FcPatternAddInteger (pat, FC_WIDTH, p_width);

  return pat;
}
				     
static void selected_fn (char *family) { /***/
  FcPattern *pattern;
  FcObjectSet *os = NULL;
  FcFontSet *fs;
  FcChar8 *s, *q;

  pattern = FcNameParse ((FcChar8 *)family);
  os = FcObjectSetBuild (FC_FILE, NULL);
  fs = FcFontList (0, pattern, os);
  if (fs -> nfont > 0) {
    s = FcPatternFormat (fs -> fonts[0], "%{=fclist}");
    if (s) {
      strcpy (selected_filename, s);
      if ((q = strchr (selected_filename, ':')) != NULL) {
	*q = '\0';
      }
      FcStrFree (s);
    }
  }
  if (os)
    FcObjectSetDestroy (os);
  if (pattern)
    FcPatternDestroy (pattern);
  if (fs)
    FcFontSetDestroy (fs);
}

static XftFont *__select_font (char *p_family, int p_slant,
			       int p_weight, int p_dpi, double p_size) {
  int spacing;

#ifdef FC_PATTERN_SET
  FcPattern *pat = NULL, *font_pat_p;
  FcFontSet *font_set;
  FcObjectSet *properties;
  int i;
  pat = construct_pattern (p_family, p_size, p_slant, p_weight,
			   p_dpi, -1, -1);
  
  properties = FcObjectSetBuild (FC_FAMILY, NULL);

  font_set = FcFontList (config, pat, properties);
  if (font_set -> nfont > 0) {
#endif    /* FC_PATTERN_SET */

    if (*p_family)
      strcpy (selected_family, p_family);
    if (p_slant >= 0)
      selected_slant = p_slant;
    if (p_weight >= 0)
      selected_weight = p_weight;
    if (p_dpi > 0)
      selected_dpi = p_dpi;
    if (p_size > 0.0)
      selected_pt_size = p_size;
#ifdef FC_PATTERN_SET

    for (i = 0; i < font_set -> nfont; ++i) {
      if ((font_pat_p = font_set -> fonts[i]) != NULL)
	FcPatternDestroy (font_pat_p);
    }
  }
#endif /* FC_PATTERN_SET */
  
  if ((selected_font = XftFontOpen
    (display, DefaultScreen (display),
     XFT_FAMILY, XftTypeString, selected_family,
     XFT_SIZE, XftTypeDouble, selected_pt_size,
     XFT_SLANT, XftTypeInteger, selected_slant,
     XFT_WEIGHT, XftTypeInteger, selected_weight,
     XFT_DPI, XftTypeInteger, selected_dpi,
     NULL)) != NULL) {
    XftPatternGetInteger (selected_font -> pattern,
			  FC_SPACING, 0, &spacing);
    monospace = (spacing == XFT_MONO ? true : false);
    selected_fn (selected_family); /***/
    if (xft_message_level > XFT_NOTIFY_ERRORS) {
      _warning ("ctalk: Loaded font: %s\n", 
		fmt_fc_desc_str (selected_font -> pattern));
    }
  } else {
    if (xft_message_level >= XFT_NOTIFY_ERRORS) {
      _warning ("ctalk: Couldn't load font: %s\n",
		fmt_fc_desc_str (selected_font -> pattern));
    }
  }

#ifdef FC_PATTERN_SET
  if (pat)
    FcPatternDestroy (pat);
#endif /* FC_PATTERN_SET */

  return selected_font;
}

XftFont *__select_font_for_family (int p_slant,
					  int p_weight, int p_dpi,
					  double p_size) {
  int spacing;

#ifdef FC_PATTERN_SET
  FcPattern *pat = NULL, *font_pat_p;
  FcFontSet *font_set;
  FcObjectSet *properties;
  int i;
  pat = construct_pattern (p_family, p_size, p_slant, p_weight,
			   p_dpi, -1, -1);
  
  properties = FcObjectSetBuild (FC_FAMILY, NULL);

  font_set = FcFontList (config, pat, properties);
  if (font_set -> nfont > 0) {
#endif    /* FC_PATTERN_SET */

    return selected_font;
  
    if (p_slant >= 0)
      selected_slant = p_slant;
    if (p_weight >= 0)
      selected_weight = p_weight;
    if (p_dpi > 0)
      selected_dpi = p_dpi;
    if (p_size > 0.0)
      selected_pt_size = p_size;
#ifdef FC_PATTERN_SET

    for (i = 0; i < font_set -> nfont; ++i) {
      if ((font_pat_p = font_set -> fonts[i]) != NULL)
	FcPatternDestroy (font_pat_p);
    }
  }
#endif /* FC_PATTERN_SET */

  if ((selected_font = XftFontOpen
    (display, DefaultScreen (display),
     XFT_SIZE, XftTypeDouble, selected_pt_size,
     XFT_SLANT, XftTypeInteger, selected_slant,
     XFT_WEIGHT, XftTypeInteger, selected_weight,
     XFT_DPI, XftTypeInteger, selected_dpi,
     NULL)) != NULL) {
    XftPatternGetInteger (selected_font -> pattern,
			  FC_SPACING, 0, &spacing);
    monospace = (spacing == XFT_MONO ? true : false);
    selected_fn (selected_family);
    if (xft_message_level > XFT_NOTIFY_ERRORS) {
      _warning ("ctalk: Loaded font: %s\n", 
		fmt_fc_desc_str (selected_font -> pattern));
    }
  } else {
    if (xft_message_level >= XFT_NOTIFY_ERRORS) {
      _warning ("ctalk: Couldn't load font: %s\n",
		fmt_fc_desc_str (selected_font -> pattern));
    }
  }

#ifdef FC_PATTERN_SET
  if (pat)
    FcPatternDestroy (pat);
#endif /* FC_PATTERN_SET */

  return selected_font;
}

#ifndef FC_WIDTH  /* older header files don't have this. */
#define FC_WIDTH "width"
#endif

static XftFont *__select_font_fc (char *p_family, int p_slant,
				  int p_weight, int p_dpi, double p_size,
				  int p_spacing, int p_width) {
  int i, spacing;
  XftFont *font;
#ifdef FC_PATTERN_SET
  XftPattern *pat, *font_pat_p;
  FcFontSet *font_set;
  FcObjectSet *properties;

  pat = construct_pattern (p_family, /*p_size*/-1.0, /*p_slant*/-1, p_weight, p_dpi,
			   p_spacing, p_width);

  properties = FcObjectSetBuild (FC_FAMILY, NULL);

  font_set = FcFontList (config, pat, properties);

  if (font_set -> nfont > 0) {
#endif    
    if (*p_family)
      strcpy (selected_family, p_family);
    if (p_slant >= 0)
      selected_slant = p_slant;
    if (p_weight >= 0)
      selected_weight = p_weight;
    if (p_dpi > 0)
      selected_dpi = p_dpi;
    if (p_spacing > 0)
      selected_spacing = p_spacing;
    if (p_size > 0.0)
      selected_pt_size = p_size;
    if (p_width > 0)
      selected_width = p_width;
#ifdef FC_PATTERN_SET
    for (i = 0; i < font_set -> nfont; ++i) {
      if ((font_pat_p = font_set -> fonts[i]) != NULL)
	FcPatternDestroy (font_pat_p);
    }
  } else {
    if (xft_message_level >= XFT_NOTIFY_ERRORS) 
      _warning ("ctalk: Could not find font %s.\n",
		fmt_fc_desc_str (pat));
    if (pat)
      FcPatternDestroy (pat);
    return selected_font;
  }
#endif    

  if ((selected_font = XftFontOpen
       (display, DefaultScreen (display),
	XFT_FAMILY, XftTypeString, selected_family,
	XFT_SIZE, XftTypeDouble, selected_pt_size,
	XFT_SLANT, XftTypeInteger, selected_slant,
	XFT_WEIGHT, XftTypeInteger, selected_weight,
	XFT_DPI, XftTypeInteger, selected_dpi,
	XFT_SPACING, XftTypeInteger, selected_spacing,
	FC_WIDTH, XftTypeInteger, selected_width,
	NULL)) != NULL) {
    XftPatternGetInteger (selected_font -> pattern,
			  FC_SPACING, 0, &spacing);
    monospace = (spacing == XFT_MONO ? true : false);
    selected_fn (selected_family);
    if (xft_message_level > XFT_NOTIFY_ERRORS) {
      _warning ("ctalk: Loaded font: %s\n",
		fmt_fc_desc_str (selected_font -> pattern));
    }
  }

  sync_ft_font (false);
#ifdef FC_PATTERN_SET
  if (pat)
    FcPatternDestroy (pat);
#endif  
  return selected_font;
}

static int default_screen_dpi (Display *d) {
  double width, width_mm, height, height_mm;
  double dpi_x, dpi_y;
  width = (double)DisplayWidth (d, DefaultScreen (d));
  width_mm = (double)DisplayWidthMM (d, DefaultScreen (d));
  height = (double)DisplayHeight (d, DefaultScreen (d));
  height_mm = (double)DisplayHeightMM (d, DefaultScreen (d));
  dpi_x = width/(width_mm / 25.4);
  dpi_y = height/(height_mm / 25.4);
   if ((int)dpi_x != (int)dpi_y) {
     /* approximate */
     selected_dpi = (dpi_x + dpi_y) / 2;
  } else {
     selected_dpi = dpi_x;
  }
   return selected_dpi;
}

extern char *__argvFileName (void);

void __ctalkXftShowFontLoad (int lvl) {
  xft_message_level = lvl;
}

int __ctalkXftVerbosity (void) {
  return xft_message_level;
}

void XFT_CONFIG_init (void) {
  char *xftenv = NULL;
  char *homedir = NULL;
  char *xdg_dir = NULL;
  char *p, *q;
  char config_path[FILENAME_MAX];
  char font_dirs[512][FILENAME_MAX];
  char font_path[FILENAME_MAX];
  char linebuf[512];
  struct stat s_buf;
  FILE *f_config;
  int n_dirs, i;
  DIR *d;
  struct dirent *d_ent;

  if ((xftenv = getenv ("XFT_CONFIG")) == NULL) {
    if ((homedir = getenv ("HOME")) != NULL) {
      sprintf (config_path, "%s/.fonts.conf", homedir);
      if (!stat (config_path, &s_buf)) {
	goto found_font_config;
      }
    }
    if ((xdg_dir = getenv ("XDG_CONFIG_HOME")) != NULL) {
      sprintf (config_path, "%s/fontconfig/fonts.conf", xdg_dir);
      if (!stat (config_path, &s_buf)) {
	goto found_font_config;
      }
    }
    if (!stat (config_path, &s_buf)) {
      goto found_font_config;
    }
    if (!stat (config_path, &s_buf)) {
      return;
    }
  } else {
    strcpy (config_path, xftenv);
  }
 found_font_config:

  if (config == NULL)
    return;

  if ((f_config = fopen (config_path, "r")) == NULL) {
    return;
  }

  n_dirs = 0;

  while (fgets (linebuf, sizeof(linebuf), f_config) != NULL) {
    /* A line like <dir>...</dir> won't cause the xft internals
       to complain - this all we need right now, instead of a big
       config parser */
    if (strstr (linebuf, "<dir>")) {
      p = strchr (linebuf, '>');
      ++p;
      q = strchr (p, '<');
      strncpy (font_dirs[n_dirs], p, q - p);
      ++n_dirs;
    }
  }

  fclose (f_config);

  if (n_dirs == 0)
    return;

  for (i = 0; i < n_dirs; i++) {
    if ((d = opendir (font_dirs[i])) != NULL) {
      while ((d_ent = readdir (d)) != NULL) {
	if (!strcmp (d_ent -> d_name, ".") || 
	    !strcmp (d_ent -> d_name, ".."))
	  continue;
	if (strstr (d_ent -> d_name, ".ttf") ||
	    strstr (d_ent -> d_name, ".pfb") ||
	    strstr (d_ent -> d_name, ".pfa")) {
	  sprintf (font_path, "%s/%s", font_dirs[i], d_ent -> d_name);
	  FcConfigAppFontAddFile (config, (const FcChar8 *)font_path);
	}
      }
      closedir (d);
    }
  }

}

int __ctalkXftInitLib (void) {
  struct stat statbuf;
  char *xftenv = NULL;
  char *homedir = NULL;
  char *xdg_dir = NULL;
  char font_dirs[512][FILENAME_MAX];
  char font_path[FILENAME_MAX];
  char linebuf[512];
  char *p, *q;
  int n_dirs, i, j;
  FILE *f_config;
  DIR *d;
  struct dirent *d_ent;
  struct stat s_buf;
  char *v;
  FcPattern *pat;
  FcObjectSet* properties;
  FcPattern* font;
  FcFontSet* fonts;
  FcChar8  *name, *file, *config_path;

  default_screen_dpi (display);

  XftInit ("");

#if  __APPLE__
  config = FcConfigCreate ();
  config_path = FcConfigFilename (NULL);
  FcConfigParseAndLoad (config, config_path, true);
  FcConfigSetCurrent (config);
#else
  config = FcInitLoadConfigAndFonts ();
#endif    


  for (i = 0; default_families[i]; i++) {
    pat = FcPatternCreate ();
    FcPatternAddString (pat, FC_FAMILY, default_families[i]);

    properties = FcObjectSetBuild (FC_FILE, NULL);
    fonts = FcFontList (config, pat, properties);

    for (j = 0; j < fonts -> nfont; ++j) {
      font = fonts -> fonts[j];

      if (FcPatternGetString(font, 
			     FC_FILE, 0, &file) == FcResultMatch)
	FcConfigAppFontAddFile (config, (const FcChar8 *)file);

      FcPatternDestroy (font);

    }

    FcPatternDestroy (pat);

  }

  /* Add in our old-style .fonts.conf fonts. */
  XFT_CONFIG_init ();

#if 0
  if (!FcConfigSetCurrent(config)) {
    _error ("FcConfigSetCurrent failed.\n");
  }
  XftInit ("");
#endif  


  for (i = 0; default_families[i]; ++i)
    if ((selected_font = __select_font (default_families[i], -1, -1, -1, -1.0))
	!= NULL)
      break;

  if (selected_font == NULL) {
    xft_selectfont_error (); /* doesn't return */
  }

  return SUCCESS;
}

#ifdef HAVE_XRENDER_H

#include <X11/extensions/Xrender.h>

/* The point size we calibrated last at. */
static float m_pt_size = 0.0f;
static int units_per_point = 0;
static char m_filename[FILENAME_MAX] = "";
static FT_Library glyph_ft = NULL;
static FT_Face glyph_face = NULL;

int __ctalkXftGetStringDimensions (char *str, int *x, int *y,
				   int *width, int *height,
				   int *rbearing) {
  XGlyphInfo extents;
  Display *d_l;
  char *d_env = NULL;
  char *p;
  FcPattern *pattern;
  FcObjectSet *os = NULL;
  FcFontSet *fs;
  FcChar8 *s;
  FT_GlyphSlot g;
  int pxSize;
  int strPxSize;

  if (selected_font == NULL) {
    *x = *y = *width = *height = 0;
    return SUCCESS;
  }
  if (*selected_filename == '\0') {
    /* We haven't actually selected a font yet - use Xft's 
       algorightm. */
    if ((d_env = getenv ("DISPLAY")) == NULL) {
      printf ("This program requires the X Window System. Exiting.\n");
      exit (1);
    }

    d_l = XOpenDisplay (d_env);

    XftTextExtents8 (d_l, selected_font,
		     (XftChar8 *)str, strlen (str),
		     &extents);
    XCloseDisplay (d_l);
    *x = extents.x;
    *y = extents.y;
    *width = extents.width;
    *height = extents.height;
    *rbearing = 0;
    return SUCCESS;
    
  } else {

    if (*m_filename == '\0' || !str_eq (m_filename, selected_filename)) {
      strcpy (m_filename, selected_filename);
      if (glyph_face) {
	FT_Done_Face (glyph_face);
	glyph_face = NULL;
      }
      if (glyph_ft) {
	FT_Done_FreeType (glyph_ft);
	glyph_ft = NULL;
      }
    }
    
    if (!glyph_ft)
      if (FT_Init_FreeType (&glyph_ft)) {
	printf ("ctalk: Could not init freetype library.\n");
	return ERROR;
      }

    if (!glyph_face)
      if (FT_New_Face (glyph_ft, selected_filename, 0, &glyph_face)) {
	printf ("ctalk: Could not open font %s.\n", selected_filename);
	return ERROR;
      }

    pxSize = selected_pt_size * (selected_dpi / 72);
    FT_Set_Pixel_Sizes (glyph_face, 0, pxSize);

    g = glyph_face -> glyph;

    if (m_pt_size == 0.0 || m_pt_size != selected_pt_size) {
      m_pt_size = selected_pt_size;
      units_per_point = glyph_face -> ascender / m_pt_size;
    }

    for (strPxSize = 0, p = str; *p; ++p) {
      FT_Load_Char (glyph_face, *p, FT_LOAD_NO_SCALE);
      strPxSize += g -> advance.x / units_per_point;
    }

    /* Provide a rightward space "hint" past the final character that
       is the distance between the horizontal advance and the glyph
       width (this is the X font definition of right bearing, but not
       Freetype's definition).  For the final character only. */
    strPxSize += (g -> advance.x - g -> metrics.width) / units_per_point;

    *x = *y = 0;
    *height = pxSize;
    *width = strPxSize;
    *rbearing = (g -> advance.x - g -> metrics.width) / units_per_point;

    return SUCCESS;
  }

}

#else /* #ifdef HAVE_XRENDER_H */
int __ctalkXftGetStringDimensions (char *str, int *x, int *y,
				   int *width, int *height) {
  *x = *y = *width = *height = 0;
  return SUCCESS;
}

#endif /* #ifdef HAVE_XRENDER_H */

char *__ctalkXftQualifyFontName (char *xftpattern) {
  XftPattern *pat;
  static char name[MAXMSG];

  pat = XftNameParse (xftpattern);
  if (pat) {
    XftNameUnparse (pat, name, MAXMSG);
    XftPatternDestroy (pat);
    return name;
  }
  return NULL;
}

void __ctalkXftSelectFont (char *family, int slant, int weight, int dpi,
			   double pt_size) {
  XftFont *f;
  if ((f = __select_font (family, slant, weight, dpi, pt_size))
      != NULL) {
    selected_font = f;
    sync_ft_font (false);
  }
}

int __ctalkMatchText (char *, char *, long long int*);
char *__ctalkMatchAt (int);

static char req_family[MAXLABEL] = "";
static int req_pt_size = 0;
static int req_slant = 0;
static int req_weight = 0;
static int req_dpi = 0;
static int req_width = 0;

void __ctalkXftSelectFontFromXLFD (char *xlfd) {
  /***/
  int n, /* ptsize, dpi_x,*/ dpi_y; /* weight_def, slant_def; */
  long long int offsets[0xff];
  /***/
  char /* family[MAXLABEL],*/ weight[MAXLABEL], slant,
    ptsizestr[64], dpi_x_str[64], dpi_y_str[64],
    *r, *p, *q;
  /* we can't create a regex to match a XFLD (with optional *'s for
     the blank fields), so just parse the xlfd for the fields we
     need. */
  if ((r = strchr (xlfd, '-')) != NULL) {
    ++r;
    if ((r = strchr (r, '-')) != NULL) {
      ++r;
      if ((q = strchr (r, '-')) != NULL) {
	/* memset (family, 0, MAXLABEL);
	   strncpy (family, r, q - r); *//***/
	memset (req_family, 0, MAXLABEL);
	strncpy (req_family, r, q - r);
	r = q + 1;
	if ((q = strchr (r, '-')) != NULL) {
	  memset (weight, 0, MAXLABEL);
	  strncpy (weight, r, q - r);
	  r = q + 1;
	  slant = *r;
	  r += 2;
	  if ((q = strchr (r, '-')) != NULL) {
	    r = q + 1;
	    if ((q = strchr (r, '-')) != NULL) {
	      r = q + 1;
	      if ((q = strchr (r, '-')) != NULL) {
		memset (ptsizestr, 0, 64);
		strncpy (ptsizestr, r, q - r);
		/* ptsize = atoi (ptsizestr); *//***/
		req_pt_size = atoi (ptsizestr);
		r = q + 1;
		if ((q = strchr (r, '-')) != NULL) {
		  memset (dpi_x_str, 0, 64);
		  strncpy (dpi_x_str, r, q - r);
		  /* dpi_x = atoi (dpi_x_str); *//***/
		  req_dpi = atoi (dpi_x_str);
		  r = q + 1;
		  if ((q = strchr (r, '-')) != NULL) {
		    memset (dpi_y_str, 0, 64);
		    strncpy (dpi_y_str, r, q - r);
		    dpi_y = atoi (dpi_y_str);
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }

  if (!strcmp (weight, "bold")) {
    req_weight = XFT_WEIGHT_BOLD;
  } else if (!strcmp (weight, "demibold")) {
    req_weight = XFT_WEIGHT_DEMIBOLD;
  } else if (!strcmp (weight, "medium")) {
    req_weight = XFT_WEIGHT_MEDIUM;
  }
  switch (slant)
    {
    case 'r': req_slant = XFT_SLANT_ROMAN; break;
    case 'i': req_slant = XFT_SLANT_ITALIC; break;
    case 'o': req_slant = XFT_SLANT_OBLIQUE; break;
    }
  /***/
  /* __select_font (family, slant_def, weight_def, dpi_x, (double)ptsize); */
  __select_font (req_family, req_slant, req_weight, req_dpi, (double)req_pt_size);
}

#include <object.h>
#include <message.h>
#include <lex.h>

/* these defs are from ctalk.h */
#ifndef __need_MESSAGE_STACK
#define __need_MESSAGE_STACK
typedef MESSAGE ** MESSAGE_STACK;
#endif

#ifndef MAXARGS
#define MAXARGS 512
#endif

#ifndef N_MESSAGES 
#define N_MESSAGES (MAXARGS * 120)
#endif

#ifndef P_MESSAGES
#define P_MESSAGES (N_MESSAGES * 30)  /* Enough to include all ISO headers. */
#endif

#define M_NAME(m) (m -> name)
#define M_TOK(m) (m -> tokentype)

#define M_ISSPACE(m) (((m) -> tokentype == WHITESPACE) || \
                      ((m) -> tokentype == NEWLINE))


MESSAGE *fc_messages[P_MESSAGES+1]; /* fontconfig messages */
int fc_message_ptr = P_MESSAGES;

int fc_message_push (MESSAGE *m) {
#ifdef MINIMUM_MESSAGE_HANDLING
  fc_messages[fc_message_ptr--] = m;
  return fc_message_ptr + 1;
#else  
  if (fc_message_ptr == 0) {
    _warning (_("fc_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("fc_message_push %d. %s."), fc_message_ptr, m -> name);
#endif
  fc_messages[fc_message_ptr--] = m;
  return fc_message_ptr + 1;
#endif /* #if MINIMUM_MESSAGE_HANDLING */
}

MESSAGE *fc_message_pop (void) {
#ifdef MINIMUM_MESSAGE_HANDLING
  MESSAGE *m;
    m = fc_messages[fc_message_ptr + 1];
    fc_messages[++fc_message_ptr] = NULL;
    return m;
#else
  MESSAGE *m;
  if (fc_message_ptr == P_MESSAGES) {
    _warning (_("fc_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("fc_message_pop %d. %s."), fc_message_ptr, 
	 fc_messages[fc_message_ptr+1] -> name);
#endif
  if (fc_messages[fc_message_ptr + 1] && 
      IS_MESSAGE(fc_messages[fc_message_ptr + 1])) {
    m = fc_messages[fc_message_ptr + 1];
    fc_messages[++fc_message_ptr] = NULL;
    return m;
  } else {
    fc_messages[++fc_message_ptr] = NULL;
    return NULL;
  }
#endif /* #if MINIMUM_MESSAGE_HANDLING */
}

int next_fc_message (MESSAGE_STACK messages, int i, int stack_end) {
  --i;
  while (i >= stack_end) {
    if (M_ISSPACE(messages[i]))
      --i;
    else
      return i;
  }
  return -1;
}
int next_fc_colon (MESSAGE_STACK messages, int i, int stack_end) {
  while (i >= stack_end) {
    if (M_TOK(messages[i]) == COLON)
      return i;
    else
      --i;
  }
  return -1;
}

static int single_fc_value (MESSAGE_STACK messages, int tok_idx,
			    int *p_slant, int *p_weight, int *p_width,
			    int *p_spacing) {
  MESSAGE *m_prop = messages[tok_idx];
  int j;

  /* check for a value without the property; especially as the
     final token of the specification, e.g.,
     DejaVu Sans-12:bold
  */
    for (j = 0; *fc_slant[j].name; j++) {
      if (str_eq (M_NAME(m_prop), fc_slant[j].name)) {
	*p_slant = fc_slant[j].value;
	return 0;
      }
    }
    for (j = 0; *fc_weight[j].name; j++) {
      if (str_eq (M_NAME(m_prop), fc_weight[j].name)) {
	*p_weight = fc_weight[j].value;
	return 0;
      }
    }
    for (j = 0; *fc_width[j].name; j++) {
      if (str_eq (M_NAME(m_prop), fc_width[j].name)) {
	*p_width = fc_width[j].value;
	return 0;
      }
    }
    for (j = 0; *fc_spacing[j].name; j++) {
      if (str_eq (M_NAME(m_prop), fc_slant[j].name)) {
	*p_weight = fc_spacing[j].value;
	return 0;
      }
    }
    return -1;
}

/* When parsing the font family, do a check for further hyphens before
   the end of the stack or the colon that starts the property tokens, so
   we can distinguish between i.e., "sans serif-12" and
   "sans-serif-12"

   idx points to a MINUS token.
*/
static bool last_family_hyphen (MESSAGE_STACK messages, int idx,
				int stack_end) {
  int i;
  for (i = idx - 1; i > stack_end; --i) {
    if (M_TOK(messages[i]) == MINUS) {
      return false;
    } else if (M_TOK(messages[i]) == COLON) {
      return true;
    }
  }
  return true;
}

char *__ctalkXftRequestedFamily (void) {
  return req_family;
}

int __ctalkXftRequestedPointSize (void) {
  return req_pt_size;
}

int __ctalkXftRequestedSlant (void) {
  return req_slant;
}

int __ctalkXftRequestedWeight (void) {
  return req_weight;
}

int __ctalkXftRequestedDPI (void) {
  return req_dpi;
}


void __ctalkXftSelectFontFromFontConfig (char *font_config_str) {
#if 0 /***/
  int n, ptsize, dpi_x, dpi_y, weight_def, slant_def,
    stack_end, i, lookahead, lookahead2, lookahead3,
    spacing_def, j, width_def;
#endif
  int i, n, stack_end_i, lookahead, lookahead2, lookahead3,
    spacing_def, j, stack_end;
  long long int offsets[0xff];
  char family[MAXLABEL], weight[MAXLABEL], slant,
    ptsizestr[64], dpi_x_str[64], dpi_y_str[64],
    *r, *p, *q;
  MESSAGE *m_prop;

  *req_family = '\0';
  req_pt_size = 0.0f;
  req_slant = 0;
  req_weight = 0;
  req_dpi = 0;

#if 0 /***/
  ptsize = -1.0;
  dpi_x = dpi_y = -1;
  weight_def = -1;
  slant_def = -1;
  spacing_def = -1;
  width_def = -1;
#endif  

  if (!font_config_str || *font_config_str == 0)
    return;

  stack_end = tokenize (fc_message_push, font_config_str);

  memset (family, 0, MAXLABEL);
  for (i = P_MESSAGES; i >= stack_end; i--) {
    if ((M_TOK(fc_messages[i]) == MINUS) &&
	last_family_hyphen (fc_messages, i, stack_end)) {
      break;
    } else if (M_TOK(fc_messages[i]) == COLON) {
      break;
    }
    strcatx2 (family, M_NAME(fc_messages[i]), NULL);
  }

  if (i > stack_end) {
    if (M_TOK(fc_messages[i]) == MINUS) {
      i = next_fc_message (fc_messages, i, stack_end);
      /* ptsize = atoi (M_NAME(fc_messages[i])); *//***/
      req_pt_size = atoi (M_NAME(fc_messages[i]));
      i = next_fc_message (fc_messages, i, stack_end);
    }
  } else {
    /*  ptsize = 12; *//***/
    req_pt_size = 12;
    goto fontconfig_parse_done;
  }

  if ((i = next_fc_colon (fc_messages, i, stack_end)) < 0)
    goto fontconfig_parse_done;
  if ((i = next_fc_message (fc_messages, i, stack_end)) < 0)
    goto fontconfig_parse_done;

  if (i == stack_end) {
    if (single_fc_value (fc_messages, i, &req_slant, &req_weight,
			 &req_width, &spacing_def) < 0)
      printf ("ctalk: Badly formed fontconfig string: %s.\n",
	      font_config_str);
    goto fontconfig_parse_done;
  }

  while (i > stack_end) {
    while ((lookahead = next_fc_message (fc_messages, i, stack_end)) > 0) {
      m_prop = fc_messages[i];
      switch (M_TOK(m_prop))
	{
	case LABEL:
	  if (M_TOK(fc_messages[lookahead]) == EQ) {
	    if ((lookahead2 = next_fc_message (fc_messages, lookahead,
					       stack_end)) < 0) {
	      printf ("ctalk: Badly formed fontconfig string: %s.\n",
		      font_config_str);
	      goto fontconfig_parse_done;
	    }
	  }
	  if (str_eq (M_NAME(m_prop), "slant")) {
	    for (j = 0; *fc_slant[j].name; j++) {
	      if (str_eq (M_NAME(fc_messages[lookahead2]),
			  fc_slant[j].name)) {
		/* slant_def = fc_slant[j].value; *//***/
		req_slant = fc_slant[j].value;
		goto fc_param_done;
	      }
	    }
	    printf ("ctalk: Badly formed fontconfig string: %s.\n",
		    font_config_str);
	    goto fontconfig_parse_done;
	  } else if (str_eq (M_NAME(m_prop), "weight")) {
	    for (j = 0; *fc_weight[j].name; j++) {
	      if (str_eq (M_NAME(fc_messages[lookahead2]),
			  fc_weight[j].name)) {
		/* weight_def = fc_weight[j].value; *//***/
		req_weight = fc_weight[j].value;
		goto fc_param_done;
	      }
	    }
	    printf ("ctalk: Badly formed fontconfig string: %s.\n",
		    font_config_str);
	    goto fontconfig_parse_done;
	  } else if (str_eq (M_NAME(m_prop), "width")) {
	    for (j = 0; *fc_width[j].name; j++) {
	      if (str_eq (M_NAME(fc_messages[lookahead2]),
			  fc_width[j].name)) {
		/* width_def = fc_width[j].value; *//***/
		req_width = fc_width[j].value;
		goto fc_param_done;
	      }
	    }
	    printf ("ctalk: Badly formed fontconfig string: %s.\n",
		    font_config_str);
	    goto fontconfig_parse_done;
	  } else if (str_eq (M_NAME(m_prop), "style")) {
	    printf ("ctalk: the fontconfig \"style\" keyword is not (yet) "
		    "supported.\n");
	  } else if (str_eq (M_NAME(m_prop), "spacing")) {
	    for (j = 0; *fc_spacing[j].name; j++) {
	      if (str_eq (M_NAME(fc_messages[lookahead2]), fc_slant[j].name)) {
		/* weight_def = fc_spacing[j].value; *//***/
		req_weight = fc_spacing[j].value;
		goto fc_param_done;
	      }
	    }
	    printf ("ctalk: Badly formed fontconfig string: %s.\n",
		    font_config_str);
	    goto fontconfig_parse_done;
	  } else {
	    /* check individual names without the property name; e.g., 
	       DejaVu Sans-12:bold
	    */
	    for (j = 0; *fc_slant[j].name; j++) {
	      if (str_eq (M_NAME(m_prop), fc_slant[j].name)) {
		/* slant_def = fc_slant[j].value; *//***/
		req_slant = fc_slant[j].value;
		goto fc_param_done;
	      }
	    }
	    for (j = 0; *fc_weight[j].name; j++) {
	      if (str_eq (M_NAME(m_prop), fc_weight[j].name)) {
		/* weight_def = fc_weight[j].value; *//***/
		req_weight = fc_weight[j].value;
		goto fc_param_done;
	      }
	    }
	    for (j = 0; *fc_width[j].name; j++) {
	      if (str_eq (M_NAME(m_prop), fc_width[j].name)) {
		/* width_def = fc_width[j].value; *//***/
		req_width = fc_width[j].value;
		goto fc_param_done;
	      }
	    }
	    for (j = 0; *fc_spacing[j].name; j++) {
	      if (str_eq (M_NAME(m_prop), fc_slant[j].name)) {
		/* weight_def = fc_spacing[j].value; *//***/
		req_weight = fc_spacing[j].value;
		goto fc_param_done;
	      }
	    }
	    printf ("ctalk: Badly formed fontconfig string: %s.\n",
		    font_config_str);
	    goto fontconfig_parse_done;
	  }
	fc_param_done:
	  if ((i = next_fc_colon (fc_messages, i, stack_end)) < 0)
	    goto fontconfig_parse_done;
	  if ((i = next_fc_message (fc_messages, i, stack_end)) < 0)
	    goto fontconfig_parse_done;
	  continue;
	  break;
	default:
	  break;
	}
    }

    if (i == stack_end) {
      if (single_fc_value (fc_messages, i, &req_slant, &req_weight,
			   &req_width, &spacing_def) < 0)
	printf ("ctalk: Badly formed fontconfig string: %s.\n",
		font_config_str);
      goto fontconfig_parse_done;
    }
    i = lookahead;
  }

 fontconfig_parse_done:
  for (i = stack_end; i <= P_MESSAGES; i++)
    delete_message (fc_message_pop ());

  __select_font_fc (family, req_slant, req_weight, req_dpi, (double)req_pt_size,
		    spacing_def, req_width);
}

char *__ctalkXftSelectedFontDescriptor (void) {
  static char buf[MAXMSG];
  memset (buf, 0, MAXMSG);
  if (selected_font) {
    XftNameUnparse (selected_font -> pattern, buf, MAXMSG);
    return buf;
  }
  return NULL;
}

char *__ctalkXftSelectedFontPath (void) {
  char pattern_buf[MAXMSG], *p, *q;
  static char path_buf[FILENAME_MAX];
  memset (pattern_buf, 0, MAXMSG);
  if (selected_font) {
    XftNameUnparse (selected_font -> pattern, pattern_buf, MAXMSG);
    if ((p = strstr (pattern_buf, "file=")) != NULL) {
      p += 5;
      if ((q = index (p, ':')) != NULL) {
	memset (path_buf, 0, FILENAME_MAX);
	strncpy (path_buf, p, q - p);
	return path_buf;
      }
    }
  }
  return NULL;
}

static XftFontSet *font_set = NULL;
static int nth_font;
static char __xftpat[MAXMSG];

char *__ctalkXftListFontsFirst (char *xftpattern) {
  static char buf[512];
  static XftPattern *pattern = NULL;
  
  if (display == NULL)
    return NULL;

  if (config == NULL)
    return NULL;

  font_set = NULL;

  strcpy (__xftpat, xftpattern);

  font_set = XftListFonts (display, DefaultScreen (display),
			   NULL,
			   XFT_FAMILY, XFT_STYLE, XFT_SLANT, XFT_WEIGHT,
			   XFT_SIZE, XFT_PIXEL_SIZE, XFT_ENCODING,
			   XFT_SPACING, XFT_FOUNDRY, XFT_CORE, XFT_ANTIALIAS,
			   XFT_XLFD, XFT_FILE, XFT_INDEX, XFT_RASTERIZER,
			   XFT_OUTLINE, XFT_SCALABLE, XFT_RGBA,
			   XFT_SCALE, XFT_RENDER, XFT_MINSPACE,
			   NULL);
  for (nth_font = 0; nth_font < font_set -> nfont; nth_font++) {
    pattern = font_set -> fonts[nth_font];
    XftNameUnparse(pattern, buf, sizeof (buf));
    if (*xftpattern || strcmp (xftpattern, "*")) {
      if (strstr (buf, xftpattern)) {
	++nth_font;
	return buf;
      }
    } else {
      ++nth_font;
      return buf;
    }
  }

  return NULL;
}

char *__ctalkXftListFontsNext (void) {
  static XftPattern *pattern = NULL;
  static char buf[512];

  while (nth_font < font_set -> nfont) {
    pattern = font_set -> fonts[nth_font];
    XftNameUnparse(pattern, buf, sizeof (buf));
    if (*__xftpat || strcmp (__xftpat, "*")) {
      if (strstr (buf, __xftpat)) {
	++nth_font;
	return buf;
      }
    } else {
      ++nth_font;
      return buf;
    }
    ++nth_font;
  }
  return NULL;
}

char  *__ctalkXftSelectedFamily (void) {
  char *desc, *p;
  static char buf[MAXLABEL];

  if ((desc = __ctalkXftSelectedFontDescriptor ()) == NULL)
    return NULL;

  if ((p = strchr (desc, ':')) != NULL) {
    memset (buf, 0, MAXLABEL);
    strncpy (buf, desc, p - desc);
    /* Trim the pointsize if it exists. */
    if ((p = strrchr (buf, '-')) != NULL)
      *p = 0;
    return buf;
  }
  return NULL;
}

int __ctalkXftSelectedSlant (void) {
  char *desc, *p;
  static char buf[MAXLABEL];
  int n;
  int slant = -1;

  if ((desc = __ctalkXftSelectedFontDescriptor ()) == NULL)
    return slant;

  if ((p = strstr (desc, ":slant=")) != NULL)
    sscanf (p, ":slant=%d", &slant);

  return slant;
}

int __ctalkXftSelectedWeight (void) {
  char *desc, *p;
  static char buf[MAXLABEL];
  int n;
  int weight = -1;

  if ((desc = __ctalkXftSelectedFontDescriptor ()) == NULL)
    return weight;

  if ((p = strstr (desc, ":weight=")) != NULL)
    sscanf (p, ":weight=%d", &weight);

  return weight;
}

int __ctalkXftSelectedDPI (void) {
  char *desc, *p;
  static char buf[MAXLABEL];
  int n;
  int dpi = -1;

  if ((desc = __ctalkXftSelectedFontDescriptor ()) == NULL)
    return dpi;

  if ((p = strstr (desc, ":dpi=")) != NULL)
    sscanf (p, ":dpi=%d", &dpi);

  return dpi;
}

double __ctalkXftSelectedPointSize (void) {
  return selected_pt_size;
}

int __ctalkXftFgRed (void) {return (int)fgred;}
int __ctalkXftFgGreen (void) {return (int)fggreen;}
int __ctalkXftFgBlue (void) {return (int)fgblue;}
int __ctalkXftFgAlpha (void) {return (int)fgalpha;}

int __ctalkXftAscent (void) {return selected_font -> ascent;}
int __ctalkXftDescent (void) {return selected_font -> descent;}
int __ctalkXftHeight (void) {return selected_font -> height;}
int __ctalkXftMaxAdvance (void) {return selected_font -> max_advance_width;}


void __ctalkXftSetForeground (int r, int g, int b, int alpha) {
  fgred = (unsigned short) r;
  fggreen = (unsigned short)g;
  fgblue = (unsigned short)b;
  fgalpha = (unsigned short)alpha;
}

/* 
   the prototype is here so we don't have to include Xlib.h,
   etc. in ctalk.h.  Defined in x11lib.c.
*/
extern int lookup_color (XColor *, char *name);

/* Opens a connection to the display independently if the
   program hasn't yet connected to the X server. */
void __ctalkXftSetForegroundFromNamedColor (char *color_name) {
  Display *l_display;
  XColor screen_color;
  char *d_env;
  XColor exact_color;
  Colormap default_cmap;


  d_env = getenv ("DISPLAY");
  l_display = XOpenDisplay (d_env);

  default_cmap = DefaultColormap (l_display, DefaultScreen (l_display));
  
  if (!XAllocNamedColor 
      (l_display, default_cmap, color_name, &screen_color, &exact_color)) {
    return;
  }
  fgred = screen_color.red; fggreen = screen_color.green;
  fgblue = screen_color.blue;
  sync_ft_font (true);
  XCloseDisplay (l_display);
}


void __ctalkXftListFontsEnd (void) {
  if (font_set)
    XftFontSetDestroy(font_set);
}

int __ctalkXftMajorVersion (void) {
  return XFT_MAJOR;
}

int __ctalkXftMinorVersion (void) {
  return XFT_MINOR;
}

int __ctalkXftRevision (void) {
  return XFT_REVISION;
}

int __ctalkXftVersion (void) {
  return XFT_VERSION;
}

int __ctalkXftInitialized (void) {
  return (int)(config && selected_font);
}

char *__xft_selected_pattern_internal (void) {
  char family[256];
  char full_desc[MAXMSG], *p, *q;
  static char select_desc[MAXMSG];
  int slant;
  int weight;
  int width;
  int dpi;

  if (selected_font == NULL)
    return NULL;

  memset (full_desc, 0, MAXMSG);
  strncpy (full_desc, __ctalkXftSelectedFontDescriptor (), MAXMSG);
  p = strchr (full_desc, ':');
  memset (family, 0, 256);
  strncpy (family, full_desc, p - full_desc);
  /* If we have a size in the family name, trim it. */
  if ((q = strchr (family, '-')) != NULL)
    *q = 0;

  XftPatternGetInteger (selected_font -> pattern,
			"weight", 0, &weight);
  XftPatternGetInteger (selected_font -> pattern,
			"slant", 0, &slant);
  XftPatternGetInteger (selected_font -> pattern,
			"dpi", 0, &dpi);

  memset (select_desc, 0, MAXMSG);
  sprintf (select_desc, "%s|%d|%d|%d|%d|%d|%d|%d|%.1f",
   	   family, weight, slant, dpi, (int)fgred, (int)fggreen, (int)fgblue,
	   (int)fgalpha,
	   selected_pt_size);

  return select_desc;
}

void __ctalkXftRed (int r) { fgred = (unsigned short)r; }
void __ctalkXftGreen (int r) { fggreen = (unsigned short)r; }
void __ctalkXftBlue (int r) { fgblue = (unsigned short)r; }
void __ctalkXftAlpha (int r) { fgalpha = (unsigned short)r; }

/* specs of the current font, so we know if the font changes. */
static char g_family[MAXLABEL];
static int g_weight = -1;
static int g_slant = -1;
static int g_dpi = -1;
static double g_pt_size = 0.0;
Display *g_dpy = NULL;

/*  The color allocation is done in x11lib.c, because it
    needs the drawable and colormap information and we
    don't need to handle it here. */
bool new_color_spec (XRenderColor *c) {
  if (c -> red != fgred ||
      c -> green != fggreen ||
      c -> blue != fgblue ||
      c -> alpha != fgalpha)
    return true;
  else
    return false;
}

void save_new_color_spec (XRenderColor *c) {
  fgred = c -> red;
  fggreen = c -> green;
  fgblue = c -> blue;
  fgalpha = c -> alpha;
}

static bool new_font_spec (char *p_family, int p_weight, int p_slant,
			   int p_dpi, double p_pt_size) {
  if (strcmp (p_family, g_family))
    return true;
  if ((p_weight != g_weight) ||
      (p_slant != g_slant) ||
      (p_dpi != g_dpi) ||
      (p_pt_size != g_pt_size) ||
      (DISPLAY != g_dpy))
    return true;
  else
    return false;
}

static void save_new_font_spec (char *p_family, int p_weight, int p_slant,
				int p_dpi, double p_pt_size) {
  strcpy (g_family, p_family);
  g_weight = p_weight;
  g_slant = p_slant;
  g_dpi = p_dpi;
  g_pt_size = p_pt_size;
  g_dpy = DISPLAY;
}

XFTFONT ft_font;

int load_ft_font_faces_internal (char *family, double pt_size,
				 unsigned short int slant,
				 unsigned short int weight,
				 unsigned short int dpi) {
  if (new_font_spec (family, weight, slant, dpi, pt_size))  {
    if ((ft_font.normal =
	 XftFontOpen (DISPLAY, DefaultScreen (display),
		      XFT_FAMILY, XftTypeString, family,
		      XFT_SIZE, XftTypeDouble, pt_size,
		      XFT_SLANT, XftTypeInteger, slant,
		      XFT_WEIGHT, XftTypeInteger, (int)weight,
		      XFT_DPI, XftTypeInteger, (int)dpi,
		      NULL)) == NULL)
      return ERROR;
    save_new_font_spec (family, weight, slant, dpi, pt_size);
  }
  return SUCCESS;
}

bool __ctalkXftIsMonospace (void) {
  return monospace;
}

#else /* HAVE_XFT_H */

int __ctalkInitFTLib (void) {
  xft_support_error ();
}

int __ctalkXftInitLib (void) {
  xft_support_error ();
}

int __ctalkXftInitialized (void) {
  xft_support_error ();
}

char *__xft_selected_pattern_internal (void) {
  xft_support_error ();
}

int __ctalkXftGetStringDimensions (char *str, int *x, int *y,
				   int *width, int *height) {
  xft_support_error ();
}

void __ctalkXftSelectFont (char *family, int slant, int weight, int dpi,
			   double pt_size) {
  xft_support_error ();
}

void __ctalkXftSelectFontFromXLFD (char *xlfd) {
  xft_support_error ();
}

char  *__ctalkXftSelectedFamily (void) { xft_support_error (); }

int __ctalkXftSelectedSlant (void) { xft_support_error (); }
int __ctalkXftSelectedWeight (void) { xft_support_error (); }
int __ctalkXftSelectedDPI (void) { xft_support_error (); }
char *__ctalkXftFontPathFirst (char *s) {xft_support_error ();};
char *__ctalkXftFontPathNext (void) {xft_support_error ();};
double __ctalkXftSelectedPointSize (void) { xft_support_error (); }
void __ctalkXftRed (int r) { xft_support_error ();}
void __ctalkXftGreen (int r) { xft_support_error (); }
void __ctalkXftBlue (int r) { xft_support_error (); }
void __ctalkXftAlpha (int r) { xft_support_error (); }

int __ctalkXftFgRed (void) {xft_support_error ();}
int __ctalkXftFgGreen (void) {xft_support_error ();}
int __ctalkXftFgBlue (void) {xft_support_error ();}
int __ctalkXftFgAlpha (void) {xft_support_error ();}

void __ctalkXftSetForegroundFromNamedColor (char *color_name) {
  xft_support_error ();
}
char *__ctalkXftQualifyFontName (char *xftpattern) {
  xft_support_error ();
}

int __ctalkXftMajorVersion (void) { xft_support_error ();}
int __ctalkXftMinorVersion (void) { xft_support_error ();}
int __ctalkXftRevision (void) { xft_support_error ();}
int __ctalkXftVersion (void) { xft_support_error (); }

void __ctalkXftSetForeground (int r, int g, int b, int alpha) {
  xft_support_error ();
}
char *__ctalkXftSelectedFontDescriptor (void) {
  xft_support_error ();
}
void __ctalkXftListFontsEnd (void) {
  xft_support_error ();
}
char *__ctalkXftListFontsNext (void) {
  xft_support_error ();
}
char *__ctalkXftListFontsFirst (char *xftpattern) {
  xft_support_error ();
}
int __ctalkXftMaxAdvance (void) {
  xft_support_error ();
  return 0; /* notreached */
}
int __ctalkXftAscent (void) {
  xft_support_error ();
  return 0; /* notreached */
}
int __ctalkXftDescent (void) {
  xft_support_error ();
  return 0; /* notreached */
}
int __ctalkXftHeight (void) {
  xft_support_error ();
  return 0; /* notreached */
}
void __ctalkXftSelectFontFromFontConfig (char *font_config_str) {
  xft_support_error ();
}

#endif /* HAVE_XFT_H */

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

void xft_support_error (void);

int __ctalkInitFTLib (void) {
  xft_support_error ();
  return 0; /* notreached */
}

int __ctalkXftInitLib (void) {
  xft_support_error ();
  return 0; /* notreached */
}

int __ctalkXftInitialized (void) {
  xft_support_error ();
  return 0; /* notreached */
}

char *__xft_selected_pattern_internal (void) {
  xft_support_error ();
  return 0; /* notreached */
}

int __ctalkXftGetStringDimensions (char *str, int *x, int *y,
				   int *width, int *height) {
  xft_support_error ();
  return 0; /* notreached */
}

void __ctalkXftSelectFont (char *family, int slant, int weight, int dpi,
			   int pt_size) {
  xft_support_error ();
}

char  *__ctalkXftSelectedFamily (void) { xft_support_error ();  return NULL; }

int __ctalkXftSelectedSlant (void) { 
	xft_support_error (); 
 	return 0;  /* notreached */
}
int __ctalkXftSelectedWeight (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}
int __ctalkXftSelectedDPI (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}
char *__ctalkXftFontPathFirst (char *s) { 
  xft_support_error ();
  return NULL; /* notreached */
}
char *__ctalkXftFontPathNext (void) { 
  xft_support_error (); 
  return NULL; /* notreached */
};
double __ctalkXftSelectedPointSize (void) { 
  xft_support_error (); 
  return 0.0; /* notreached */
}

void __ctalkXftRed (int r) { xft_support_error (); }
void __ctalkXftGreen (int r) { xft_support_error (); }
void __ctalkXftBlue (int r) { xft_support_error (); }
void __ctalkXftAlpha (int r) { xft_support_error (); }

int __ctalkXftFgRed (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}
int __ctalkXftFgGreen (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}
int __ctalkXftFgBlue (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}
int __ctalkXftFgAlpha (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}

void __ctalkXftSetForegroundFromNamedColor (char *color_name) {
  xft_support_error ();
}
char *__ctalkXftQualifyFontName (char *xftpattern) {
  xft_support_error ();
  return NULL; /* notreached */
}
int __ctalkXftMajorVersion (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}
int __ctalkXftMinorVersion (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}
int __ctalkXftRevision (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}
int __ctalkXftVersion (void) { 
  xft_support_error (); 
  return 0; /* notreached */
}

void __ctalkXftSetForeground (int r, int g, int b, int alpha) {
  xft_support_error ();
}
char *__ctalkXftSelectedFontDescriptor (void) {
  xft_support_error ();
  return NULL; /* notreached */
}
void __ctalkXftListFontsEnd (void) {
  xft_support_error ();
}
char *__ctalkXftListFontsNext (void) {
  xft_support_error ();
  return NULL; /* notreached */
}
char *__ctalkXftListFontsFirst (char *xftpattern) {
  xft_support_error ();
  return NULL; /* notreached */
}

int load_ft_font_faces_internal (char *family, double pt_size,
				 int slant,
				 unsigned short int weight,
				 unsigned short int dpi) {
  xft_support_error ();
  return 0; /* notreached */
}
void __ctalkXftSelectFontFromFontConfig (char *font_config_str) {
  xft_support_error ();
}
void __ctalkXftSelectFontFromXLFD (char *xlfd) {
  xft_support_error ();
}
int __ctalkXftMaxAdvance (void) {
  xft_support_error ();
  return 0; /* notreached */
}
int __ctalkXftAscent (void) {
  xft_support_error ();
  return 0; /* notreached */
}
int __ctalkXftDescent (void) {
  xft_support_error ();
  return 0; /* notreached */
}
int __ctalkXftHeight (void) {
  xft_support_error ();
  return 0; /* notreached */
}

void __ctalkXftShowFontLoad (int lvl) {
  xft_support_error ();
}
int __ctalkXftVerbosity (void) {
  xft_support_error ();
  return 0;
}
char *__ctalkXftDescStr (void) {
  xft_support_error ();
  return NULL;
}
char *__ctalkXftRequestedFamily (void) {
  xft_support_error ();
  return NULL;
}

int __ctalkXftRequestedPointSize (void) {
  xft_support_error ();
  return 0;
}

int __ctalkXftRequestedSlant (void) {
  xft_support_error ();
  return 0;
}
int __ctalkXftRequestedWeight (void) {
  xft_support_error ();
  return 0;
}
int __ctalkXftRequestedDPI (void) {
  xft_support_error ();
  return 0;
}
bool __ctalkXftIsMonospace (void) {
  xft_support_error ();
  return true;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */ 
