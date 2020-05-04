/* $Id: ftlib.c,v 1.25 2020/05/04 23:28:37 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2019 Robert Kiesling, rk3314042@gmail.com.
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
#include <object.h>
#include <message.h>
#include <stdlib.h>
#include <ctalk.h>

/*
 *  An adaptation the text example in the OpenGL Programming
 *  wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming,
 *  and another of NeHe's great OpenGL recipes.
 *  
 *  This library is written to work specifically with 
 *  GLXCanvasPane objects.
 *
 *  Tested with Mesa OpenGL and GLEW, which includes the
 *  GLEW_ARB_vertex_shader and GLEW_ARB_fragment_shader extensions
 *  on Linux, and with the ARB extensions on MacOS, after installing
 *  GLEW separately.
 */

#if defined (HAVE_FREETYPE_H) && defined (HAVE_GL_H) && defined (HAVE_GLEW_H)

#include <GL/glew.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftbitmap.h>

extern Display *display;  /* defined in x11lib.c */

extern bool has_GLEW20;
extern bool has_ARB;

static char *vertex_shader = {
  "attribute vec4 coord;\n"
  "varying vec2 texpos;\n"
  "void main(void) {\n"
  "gl_Position = vec4(coord.xy, 0, 1);\n"
  "texpos = coord.zw;\n"
  "}\n"
};

static char *fragment_shader = {
  "varying vec2 texpos;\n"
  "uniform sampler2D tex;\n"
  "uniform vec4 color;\n"
  "void main(void) {\n"
  "gl_FragColor = vec4(1, 1, 1, texture2D(tex, texpos).a) * color;\n"
  "}\n"
};

/* This is untested with ARB shaders!!! */
static void print_log (GLuint object) {
  GLint log_length = 0;
  if (has_ARB) {
    if (glIsShader (object))
      glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    else if (glIsProgramARB (object))
      glGetProgramivARB (object, GL_INFO_LOG_LENGTH, &log_length);
    else {
      fprintf(stderr, "printlog: Not a shader or a program\n");
      return;
    }
  } else {
    if (glIsShader(object))
      glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    else if (glIsProgram(object))
      glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    else {
      fprintf(stderr, "printlog: Not a shader or a program\n");
      return;
    }
  }

  char* log = (char*)malloc(log_length);

  if (has_ARB) {
    if (glIsShader(object))
      glGetShaderInfoLog(object, log_length, NULL, log);
    else if (glIsProgramARB (object))
      glGetProgramInfoLog(object, log_length, NULL, log);
  } else {
    if (glIsShader(object))
      glGetShaderInfoLog(object, log_length, NULL, log);
    else if (glIsProgram(object))
      glGetProgramInfoLog(object, log_length, NULL, log);
  }

  fprintf(stderr, "%s", log);
  free(log);
}

GLuint create_shader(const char* source, GLenum type)
{
  GLuint res;
  
  if (has_ARB) {
    res = glCreateShaderObjectARB (type);
  } else {
    res = glCreateShader(type);
  }

  if (has_ARB) {
    const char *vs = source;
    glShaderSourceARB (res, 1, &vs, NULL);
  } else {
    const GLchar* sources[] = {
      // Define GLSL version
#ifdef GL_ES_VERSION_2_0
    "#version 100\n"  // OpenGL ES 2.0
#else
    "#version 120\n"  // OpenGL 2.1
#endif
    ,
    // GLES2 precision specifiers
#ifdef GL_ES_VERSION_2_0
    // Define default float precision for fragment shaders:
    (type == GL_FRAGMENT_SHADER) ?
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
    "precision highp float;           \n"
    "#else                            \n"
    "precision mediump float;         \n"
    "#endif                           \n"
    : ""
    // Note: OpenGL ES automatically defines this:
    // #define GL_ES
#else
    // Ignore GLES 2 precision specifiers:
    "#define lowp   \n"
    "#define mediump\n"
    "#define highp  \n"
#endif
    ,
    source };
    glShaderSource(res, 3, sources, NULL);
  }

  if (has_ARB) {
    glCompileShaderARB (res);
  } else {
    glCompileShader(res);
  }

  GLint compile_ok = GL_FALSE;
  if (has_ARB) {
    glGetObjectParameterivARB (res, GL_COMPILE_STATUS, &compile_ok);
  } else {
    glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
  }
  if (compile_ok == GL_FALSE) {
    fprintf(stderr, "ctalk: ");
    print_log(res);
    glDeleteShader(res);
    return 0;
  }

  return res;
}

static GLuint create_program(void) {
  GLuint program;
  GLuint shader;

  if (has_ARB) {
    program = glCreateProgramObjectARB ();
  } else {
    program = glCreateProgram();
  }

  shader = create_shader(vertex_shader, GL_VERTEX_SHADER);
  if(!shader)
    return 0;
  if (has_ARB) {
    glAttachObjectARB (program, shader);
  } else {
    glAttachShader(program, shader);
  }

  shader = create_shader(fragment_shader, GL_FRAGMENT_SHADER);
  if(!shader)
    return 0;
  if (has_ARB) {
    glAttachObjectARB (program, shader);
  } else {
    glAttachShader(program, shader);
  }
  
  if (has_ARB) {
    glLinkProgramARB (program);
  } else {
    glLinkProgram(program);
  }
  GLint link_ok = GL_FALSE;
  if (has_ARB) {
    glGetObjectParameterivARB (program, GL_LINK_STATUS, &link_ok);
  } else {
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
  }
  if (!link_ok) {
    fprintf(stderr, "glLinkProgram:");
    print_log(program);
    glDeleteProgram(program);
    return 0;
  }

  return program;
}

static GLint get_attrib(GLuint program, const char *name) {
  GLint attribute;

  if (has_ARB) {
    attribute = glGetAttribLocationARB (program, name);
  } else {
    attribute = glGetAttribLocation(program, name);
  }
  if(attribute == -1)
    fprintf(stderr, "Could not bind attribute %s\n", name);
  return attribute;
}

static GLint get_uniform(GLuint program, const char *name) {
  GLint uniform;

  if (has_ARB) {
    uniform = glGetUniformLocationARB (program, name);
  } else {
    uniform = glGetUniformLocation(program, name);
  }
  if(uniform == -1)
    fprintf(stderr, "Could not bind uniform %s\n", name);
  return uniform;
}

FT_Library ft = NULL;
FT_Face face = NULL;
int font_height_px = 18;
GLuint program;
GLint attribute_coord;
GLint uniform_tex;
GLint uniform_color;

struct point {
  GLfloat x, y, s, t;
};

static GLuint vbo;
static bool vbo_init = false;
static GLuint width_tex;
static GLuint textures[256];
static bool texture_init = false;
static float g_alpha = 1.0f;

static struct point box[4] = {
  {0, 0, 0, 0},
  {0, 0, 1, 0},
  {0, -0, 0, 1},
  {0, 0, 1, 1},
};

typedef struct _gi {
  int bitmap_left,
    bitmap_top,
    advance_x,
    advance_y,
    bitmap_rows,
    bitmap_width;
} GLYPHINFO;

GLYPHINFO *glyphs[256] = {NULL, };

static bool glyphs_alloted (void) {
  int i;
  for (i = 0; i < 256; ++i)
    if (glyphs[i])
      return true;
  return false;
}

static void del_glyphs (void) {
  int i;
  for (i = 0; i < 256; ++i) {
    if (glyphs[i]) {
      free (glyphs[i]);
      glyphs[i] = NULL;
    }
  }
}

static void render_text (const char *text, float x, float y,
			 float sx, float sy) {
  const char *p;
  FT_GlyphSlot g = face->glyph;
  GLYPHINFO *gi;

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_BLEND);
  /* glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */ /***/
  glBlendFunc (GL_SRC_ALPHA, GL_DST_ALPHA);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (has_ARB) {
    glUniform1iARB (uniform_tex, 0);
  } else {
    glUniform1i(uniform_tex, 0);
  }

  if (has_ARB) {
    glEnableVertexAttribArrayARB (attribute_coord);
  } else {
    glEnableVertexAttribArray(attribute_coord);
  }
  if (has_ARB) {
    glGenBuffersARB (1, &vbo);
    glBindBufferARB (GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointerARB (attribute_coord, 4, GL_FLOAT, GL_FALSE,
			      0, 0);
  } else {
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);
  }

  for (p = text; *p; p++) {
    if (!glyphs[*p]) {
      if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
	continue;
      gi = malloc (sizeof (struct _gi));
      gi -> bitmap_left = g -> bitmap_left;
      gi -> bitmap_top = g -> bitmap_top;
      gi -> advance_x = g -> advance.x;
      gi -> advance_y = g -> advance.y;
      gi -> bitmap_rows = g -> bitmap.rows;
      gi -> bitmap_width = g -> bitmap.width;
      glyphs[*p] = gi;
      glBindTexture(GL_TEXTURE_2D, textures[*p]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
		   g -> bitmap.width, g -> bitmap.rows,
		   0, GL_ALPHA, GL_UNSIGNED_BYTE,
		   g -> bitmap.buffer);
    } else {
      gi = glyphs[*p];
      glBindTexture(GL_TEXTURE_2D, textures[*p]);
    }

    /* Calculate the vertex and texture coordinates */
    float x2 = x + gi -> bitmap_left * sx;
    float y2 = -y - gi -> bitmap_top * sy;
    float w = gi -> bitmap_width * sx;
    float h = gi -> bitmap_rows * sy;
    
    box[0].x = x2, box[0].y = -y2;
    box[1].x = x2 + w, box[1].y = -y2;
    box[2].x = x2, box[2].y = -y2 - h;
    box[3].x = x2 + w, box[3].y = -y2 - h;

    if (has_ARB) {
      glBufferDataARB (GL_ARRAY_BUFFER,
		       sizeof box, box,
		       GL_DYNAMIC_DRAW);
    } else {
      glBufferData(GL_ARRAY_BUFFER,
		   sizeof box, box,
		   GL_DYNAMIC_DRAW);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    x += (gi -> advance_x >> 6) * sx;
    y += (gi -> advance_y >> 6) * sy;
  }

  if (has_ARB) {
    glDisableVertexAttribArrayARB (attribute_coord);
  } else {
    glDisableVertexAttribArray(attribute_coord);
  }
  if (has_ARB) {
    glDeleteBuffersARB (1, &vbo);
  } else {
    glDeleteBuffers (1, &vbo);
  }

}

static double render_text_width (const char *text, float sx, float sy) {
  const char *p;
  double x = 0.0, y = 0.0;
  FT_GlyphSlot g = face->glyph;


  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &width_tex);

  glBindTexture(GL_TEXTURE_2D, width_tex);
  if (has_ARB) {
    glUniform1iARB (uniform_tex, 0);
  } else {
    glUniform1i(uniform_tex, 0);
  }

  /* We require 1 byte alignment when uploading texture data */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  /* Clamping to edges is important to prevent artifacts when scaling */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  /* Linear filtering usually looks best for text */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  /* Set up the VBO for our vertex data */
  if (has_ARB) {
    glEnableVertexAttribArrayARB (attribute_coord);
  } else {
    glEnableVertexAttribArray(attribute_coord);
  }
  if (has_ARB) {
    glBindBufferARB (GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointerARB (attribute_coord, 4, GL_FLOAT, GL_FALSE,
			      0, 0);
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);
  }

  /* Loop through all characters */
  for (p = text; *p; p++) {
    /* Try to load and render the character */
    if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
      continue;

    /* 
       Upload the "bitmap", which contains an 8-bit grayscale 
       image, as an alpha texture 
    */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
		 g->bitmap.width, g->bitmap.rows,
		 0, GL_ALPHA, GL_UNSIGNED_BYTE,
		 g->bitmap.buffer);

    /* Calculate the vertex and texture coordinates */
    float x2 = x + g->bitmap_left * sx;
    float y2 = -y - g->bitmap_top * sy;
    float w = g->bitmap.width * sx;
    float h = g->bitmap.rows * sy;
    
    /* Advance the cursor to the start of the next character */
    x += (g->advance.x >> 6) * sx;
    y += (g->advance.y >> 6) * sy;
  }

  if (has_ARB) {
    glDisableVertexAttribArrayARB (attribute_coord);
  } else {
    glDisableVertexAttribArray(attribute_coord);
  }
  glDeleteTextures(1, &width_tex);
  return x;
}

void __ctalkGLXNamedColorFT (char *colorname, float *red_out,
			     float *green_out, float *blue_out) {
  XColor screen_color, exact_color;
  float factor = (float)10000 / (float)65535;
  if (XAllocNamedColor (display,
			DefaultColormap (display,
					 DefaultScreen (display)),
			colorname,
			&screen_color, &exact_color)) {
    *red_out = ((float)exact_color.red / 10000.0) * factor;
    *green_out = ((float)exact_color.green / 10000.0) * factor;
    *blue_out = ((float)exact_color.blue / 10000.0) * factor;
  }
}

void __ctalkGLXAlphaFT (float alpha) {
  g_alpha = alpha;
}

void __ctalkGLXPixelHeightFT (int px_height) {
  font_height_px = px_height;
  if (face != NULL) {
    /* change the face size immediately */
    FT_Set_Pixel_Sizes(face, 0, font_height_px);
    del_glyphs ();
  }
}

void __ctalkGLXDrawTextFT (char *text, float x, float y) {

  float sx = 2.0 / __ctalkGLXWinXSize ();
  float sy = 2.0 / __ctalkGLXWinYSize ();
  float fgcolor[4];

  if (has_ARB) {
    glUseProgramObjectARB (program);
  } else {
    glUseProgram(program);
  }

  glDisable (GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glGetFloatv (GL_CURRENT_COLOR, fgcolor);
  fgcolor[3] = g_alpha;
  if (has_ARB) {
    glUniform4fvARB (uniform_color, 1, fgcolor);
  } else {
    glUniform4fv(uniform_color, 1, fgcolor);
  }

  render_text (text, x, y, sx, sy);

  if (has_ARB) {
    glUseProgramObjectARB (0);
  } else {
    glUseProgram(0);
  }
}

double __ctalkGLXTextWidthFT (char *text) {
  double width;
  float sx = 2.0 / __ctalkGLXWinXSize ();
  float sy = 2.0 / __ctalkGLXWinYSize ();
  float fgcolor[4];

  if (has_ARB) {
    glUseProgramObjectARB (program);
  } else {
    glUseProgram(program);
  }

  glDisable (GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glGetFloatv (GL_CURRENT_COLOR, fgcolor);
  fgcolor[3] = g_alpha;
  if (has_ARB) {
    glUniform4fvARB (uniform_color, 1, fgcolor);
  } else {
    glUniform4fv(uniform_color, 1, fgcolor);
  }

  width = render_text_width (text, sx, sy);

  if (has_ARB) {
    glUseProgramObjectARB (0);
  } else {
    glUseProgram(0);
  }

  return width;
}

int __ctalkGLXUseFTFont (char *fontfilepath) {
  
  if (__ctalkInitGLEW () < 0) {
    exit (1);
  }
  
  /* Initialize the FreeType2 library */
  if (FT_Init_FreeType(&ft)) {
    fprintf(stderr, "Could not init freetype library\n");
    return ERROR;
  }
  
  if (texture_init == false) {
    glGenTextures(256, textures);
    texture_init = true;
  }
  if (glyphs_alloted ()) {
    /* In case a program didn't release a previous font. */
    del_glyphs ();
  }
  
  /* Load a font */
  if (FT_New_Face(ft, fontfilepath, 0, &face)) {
    fprintf(stderr, "ctalk: Could not open font %s.\n", fontfilepath);
    return ERROR;
  }

  FT_Set_Pixel_Sizes(face, 0, font_height_px);

  program = create_program();
  if(program == 0) {
    fprintf (stderr, "ctalk: Error creating rendering program.\n");
    return ERROR;
  }

  attribute_coord = get_attrib(program, "coord");
  uniform_tex = get_uniform(program, "tex");
  uniform_color = get_uniform(program, "color");

  if(attribute_coord == -1 || uniform_tex == -1 || uniform_color == -1) {
    fprintf (stderr, "ctalk: Error creating rendering program.\n");
    return ERROR;
  }

  // Create the vertex buffer object
#if 0
  if (!vbo_init) {
    if (has_ARB) {
      glGenBuffersARB (1, &vbo);
    } else {
      glGenBuffers(1, &vbo);
    }
    vbo_init = true;
  }
#endif  

  return SUCCESS;
}

bool __ctalkGLXUsingFTFont (void) {
  return (bool) face && ft;
}

int __ctalkGLXFreeFTFont (void) {
  int ret = 0;

  if (texture_init) {
    glDeleteTextures (256, textures);
    texture_init = false;
  }

  if (face != NULL) {
    del_glyphs ();
    if ((ret = FT_Done_Face (face)) != FT_Err_Ok) {
      return ret;
    } else {
      face = NULL;
    }
  }
  if (ft != NULL) {
    if ((ret = FT_Done_FreeType (ft)) != FT_Err_Ok) {
      return ret;
    } else {
      ft = NULL;
    }
  }
  return SUCCESS;
}


#else /* #if defined (HAVE_FREETYPE_H) && defined (HAVE_GL_H) && defined (HAVE_GLEW_H) */

static void no_ft_support_msg (void) {
  printf ("Ctalk isn't configured with GLX, Freetype2, and GLEW "
	  "support. Refer to the file README in the Ctalk "
	  "source distribution.\n");
}

int __ctalkGLXUseFTFont (char *fontname) {
  no_ft_support_msg ();
  exit (1);
}
void __ctalkGLXDrawTextFT (char *text, float x, float y) {
  no_ft_support_msg ();
  exit (1);
}

int __ctalkGLXFreeFTFont (void) {
  no_ft_support_msg ();
  exit (1);
}

void __ctalkGLXPixelHeightFT (int px_height) {
  no_ft_support_msg ();
  exit (1);
}

void __ctalkGLXNamedColorFT (char *colorname, float *red_out,
			     float *green_out, float *blue_out) {
  no_ft_support_msg ();
  exit (1);
}

void __ctalkGLXalphaFT (float alpha) {
  no_ft_support_msg ();
  exit (1);
}

bool __ctalkGLXUsingFTFont (void) {
  no_ft_support_msg ();
  exit (1);
}
double __ctalkGLXTextWidthFT (char *text) {
  no_ft_support_msg ();
  exit (1);
}
#endif /* #if defined (HAVE_FREETYPE_H) && defined (HAVE_GL_H) */
