#define GLEW_STATIC
#include <GL/glew.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "console_font.h"
#include "shader.h"
#include "options.h"
#include "util_gl.h"

struct renderer {
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

  struct console_font *font;
};

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

static void init_glew()
{
  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK)
    exit(1);
  if (!GLEW_VERSION_3_2)
    exit(1);
}

struct renderer * renderer_create(struct options opts) {
  struct renderer *renderer = calloc(1, sizeof(struct renderer));

  assert(renderer); 

  init_glew();

  if (opts.linear_filter) 
    renderer->texture_filter = GL_LINEAR;
  else
    renderer->texture_filter = GL_NEAREST;

  font_create(&renderer->font, opts.texture_wh[0], opts.texture_wh[1], opts.font_filename, opts.dot_stretch);

  glGenBuffers(1, &renderer->vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, renderer->vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glGenVertexArrays(1, &renderer->vao);
  glBindVertexArray(renderer->vao);

  glGenTextures(1, &renderer->tex);
  glBindTexture(GL_TEXTURE_2D, renderer->tex);

  glGenBuffers(1, &renderer->ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

  glClearColor(0.3,0.3,0.3,1.0);
  glColor4ub(255,255,255,255);

  renderer->shader[0].filename = opts.fragment_shader_filename;
  renderer->shader[0].type =  GL_FRAGMENT_SHADER;
  renderer->shader[1].filename = opts.vertex_shader_filename;
  renderer->shader[1].type =  GL_VERTEX_SHADER;
  
  load_shader(&renderer->shader[0]);
  load_shader(&renderer->shader[1]);

  renderer->program = glCreateProgram();
  glAttachShader(renderer->program, renderer->shader[0].id);
  glAttachShader(renderer->program, renderer->shader[1].id);

  glBindFragDataLocation(renderer->program, 0, "outColor");
  glLinkProgram(renderer->program);
  glUseProgram(renderer->program);

  gl_program_info_log(stdout, renderer->program);

  renderer->posAttrib = glGetAttribLocation(renderer->program, "position");
  glEnableVertexAttribArray(renderer->posAttrib);
  glVertexAttribPointer(renderer->posAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), 0);

  renderer->colAttrib = glGetAttribLocation(renderer->program, "color");
  glEnableVertexAttribArray(renderer->colAttrib);
  glVertexAttribPointer(renderer->colAttrib, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(2*sizeof(float)));

  renderer->texAttrib = glGetAttribLocation(renderer->program, "texcoord");
  glEnableVertexAttribArray(renderer->texAttrib);
  glVertexAttribPointer(renderer->texAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(5*sizeof(float)));

  renderer->sourceSize = glGetUniformLocation(renderer->program, "sourceSize");
  renderer->targetSize = glGetUniformLocation(renderer->program, "targetSize");
  renderer->appTime = glGetUniformLocation(renderer->program, "appTime");

  glUniform2f(renderer->sourceSize, opts.texture_wh[0], opts.texture_wh[1]);
  glUniform2f(renderer->targetSize, opts.window_wh[0], opts.window_wh[1]);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, renderer->texture_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, renderer->texture_filter);

  renderer->texture_pixels = calloc(opts.texture_wh[0]*opts.texture_wh[1], sizeof (GLbyte));

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, opts.texture_wh[0], opts.texture_wh[1], 0, 
    GL_RGB, GL_UNSIGNED_BYTE_3_3_2, renderer->texture_pixels);

  return renderer;
}

void render(float time, struct renderer *renderer) {
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(renderer->program);
  glUniform1f(renderer->appTime, time);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void renderer_putc(struct renderer *scn, unsigned ch, unsigned posx, unsigned posy, unsigned char fg, unsigned char bg)
{
  void *pixels = font_fetch_glyph(scn->font, ch, fg, bg);
  int x,y,w,h;

  x = posx * scn->font->glyph_width;
  y = posy * scn->font->glyph_height;
  w = scn->font->glyph_width;
  h = scn->font->glyph_height;

  assert(pixels);
  /* assuming opengl state is bound to the texture */
  glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE_3_3_2, pixels);
}

void renderer_get_glyph_wh(struct renderer *scn, int *wh) {
  wh[0] = scn->font->glyph_width;
  wh[1] = scn->font->glyph_height;
}

