
struct shader {
  GLchar *filename;
  GLenum type;
  const GLchar *src;
  GLuint id;
};

GLuint shader_program(struct shader *shaders, const GLuint count);

void gl_info_log(GLuint prog);

