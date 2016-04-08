#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <GL/glew.h>

#include "shader.h"
#include "util_gl.h"

static GLchar * read_file(const GLchar *fname, GLint *len) {
  struct stat buf;
  int fd = -1;
  GLchar *src = NULL;
  size_t bytes = 0;

  if (stat(fname, &buf) != 0) 
    goto error;

  fd = open(fname, O_RDWR);
  if (fd < 0) 
    goto error;

  src = calloc(buf.st_size + 1, sizeof (GLchar));
  assert(src);

  bytes = read(fd, src, buf.st_size);
  if (bytes < 0) 
    goto error;

  if (len) 
    *len = buf.st_size;

  close(fd);
  return src;

error:
  perror(fname);
  exit(-2);
}

GLuint load_shader(struct shader *shader) {
  GLint len = 0;

  shader->id = glCreateShader(shader->type);

  assert(shader->id != 0);

  shader->src = read_file(shader->filename, &len);
  len = -1;
  glShaderSource(shader->id, 1, &shader->src, &len);
  glCompileShader(shader->id);
  // can free shader->src now
  printf("Compiled        %s\n",shader->filename);

  glGetShaderiv(shader->id, GL_COMPILE_STATUS, &shader->status);
  if (shader->status == 0) {
    gl_shader_info_log(stdout, shader->id);
  }
  return shader->id;
}

GLuint shader_program(struct shader *shaders, GLuint len) {
  GLuint prog;
  int i;

  prog = glCreateProgram();

  for (i = 0; i < len; i++) {
    load_shader(&shaders[i]);
    glAttachShader(prog, shaders[i].id);
  }
  return prog;
}

