#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if LINUX
#include <GL/glew.h>
#endif

#if DARWIN
#include <OpenGL/gl.h>
#endif

void info() {
  printf("OpenGL          %s\n", glGetString(GL_VERSION));
  printf("GLSL            %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  printf("Vendor          %s\n", glGetString(GL_VENDOR));
  printf("Renderer        %s\n", glGetString(GL_RENDERER));
  //printf("Extensions\n%s\n", glGetString(GL_EXTENSIONS));
}


#if 0
static void gl_info_log(FILE *fp, void(*giv)(GLuint,GLint,GLint *), void(*getlog)(GLuint,GLuint,GLuint*,GLchar*), GLuint id, GLint iv)
{
  GLchar *info = NULL;
  GLint len = 0;
  giv(id, iv, &len);
  if (len > 0) {
    info = calloc(len, sizeof(GLubyte));
    assert(info);
    getlog(id, len, NULL, info);
    if (len > 0)
      fprintf(fp, "%s\n", info);

    if (info)
      free(info);
  }
}
#endif

void gl_shader_info_log(FILE *fp, GLuint shader) {
  GLchar *info = NULL;
  GLint len = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
  if (len > 0) {
    info = calloc(len, sizeof(GLubyte));
    assert(info);
    glGetShaderInfoLog(shader, len, NULL, info);
    if (len > 0)
      fprintf(fp, "%s\n", info);

    if (info)
      free(info);
  }
}

void gl_program_info_log(FILE *fp, GLuint prog) {
  GLchar *info = NULL;
  GLint len = 0;
  glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
  if (len > 0) {
    info = calloc(len, sizeof(GLchar));
    assert(info);

    glGetProgramInfoLog(prog, len, NULL, info);

    if (len > 0)
      fprintf(fp, "%s\n", info);

    if (info)
      free(info);
  }
}

