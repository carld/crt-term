
#include <GL/glew.h>

#include <stdio.h>
#include <stdlib.h>

#include "shader.h"
#include "options.h"
#include "util_gl.h"

struct scene {
  GLuint program;

  GLint posAttrib;
  GLint colAttrib;
  GLint texAttrib;

  GLuint vertexBuffer;
  GLuint vao;
  GLuint tex;
  GLuint ebo;

  GLint sourceSize;
  GLint targetSize;
  GLint appTime;

  GLenum texture_filter;

  GLbyte * texture_pixels;

  struct shader shader[2];
};

static struct scene vtterm;

static float vertices[] = {
    //  Position  Color             Texcoords
    -1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Top-left
     1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // Top-right
     1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // Bottom-right
    -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left
};

static GLuint elements[] = {
    0, 1, 2,
    2, 3, 0
};

void scene_setup(struct options opts, struct scene **scn) {

  if (*scn == NULL)
    *scn = &vtterm;

  if (opts.linear_filter) 
    vtterm.texture_filter = GL_LINEAR;
  else
    vtterm.texture_filter = GL_NEAREST;

  glGenBuffers(1, &vtterm.vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vtterm.vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glGenVertexArrays(1, &vtterm.vao);
  glBindVertexArray(vtterm.vao);

  glGenTextures(1, &vtterm.tex);
  glBindTexture(GL_TEXTURE_2D, vtterm.tex);

  glGenBuffers(1, &vtterm.ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vtterm.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

  glClearColor(0.3,0.3,0.3,1.0);
  glColor4ub(255,255,255,255);

  vtterm.shader[0].filename = opts.fragment_shader_filename;
  vtterm.shader[0].type =  GL_FRAGMENT_SHADER;
  vtterm.shader[1].filename = opts.vertex_shader_filename;
  vtterm.shader[1].type =  GL_VERTEX_SHADER;
  
  load_shader(&vtterm.shader[0]);
  load_shader(&vtterm.shader[1]);

  vtterm.program = glCreateProgram();
  glAttachShader(vtterm.program, vtterm.shader[0].id);
  glAttachShader(vtterm.program, vtterm.shader[1].id);

  glBindFragDataLocation(vtterm.program, 0, "outColor");
  glLinkProgram(vtterm.program);
  glUseProgram(vtterm.program);

  gl_program_info_log(stdout, vtterm.program);

  vtterm.posAttrib = glGetAttribLocation(vtterm.program, "position");
  glEnableVertexAttribArray(vtterm.posAttrib);
  glVertexAttribPointer(vtterm.posAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), 0);

  vtterm.colAttrib = glGetAttribLocation(vtterm.program, "color");
  glEnableVertexAttribArray(vtterm.colAttrib);
  glVertexAttribPointer(vtterm.colAttrib, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(2*sizeof(float)));

  vtterm.texAttrib = glGetAttribLocation(vtterm.program, "texcoord");
  glEnableVertexAttribArray(vtterm.texAttrib);
  glVertexAttribPointer(vtterm.texAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(5*sizeof(float)));

  vtterm.sourceSize = glGetUniformLocation(vtterm.program, "sourceSize");
  vtterm.targetSize = glGetUniformLocation(vtterm.program, "targetSize");
  vtterm.appTime = glGetUniformLocation(vtterm.program, "appTime");

  glUniform2f(vtterm.sourceSize, opts.texture_wh[0], opts.texture_wh[1]);
  glUniform2f(vtterm.targetSize, opts.window_wh[0], opts.window_wh[1]);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, vtterm.texture_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, vtterm.texture_filter);

  vtterm.texture_pixels = calloc(opts.texture_wh[0]*opts.texture_wh[1], sizeof (GLbyte));

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, opts.texture_wh[0], opts.texture_wh[1], 0, 
    GL_RGB, GL_UNSIGNED_BYTE_3_3_2, vtterm.texture_pixels);
}

void render(float time, struct scene *scn) {
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(vtterm.program);
  glUniform1f(vtterm.appTime, time);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

#if 0
void blit_glyph(struct scene *scn, int encoding, int x, int y) {

    unsigned char *pixels;
    pixels = display_fetch_glyph(disp, ' ', fg, bg);
    assert(pixels);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
      posx * disp->glyph_width, posy * disp->glyph_height,
      disp->glyph_width, disp->glyph_height,
      GL_RGB, GL_UNSIGNED_BYTE_3_3_2,
      pixels);

}
#endif

