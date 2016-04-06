
struct shader {
  GLchar *filename;
  GLenum type;
  const GLchar *src;
  GLuint id;
};

GLuint shader_program(struct shader *shaders, const GLuint count);

void gl_program_info_log(FILE *fp, GLuint prog);
void gl_shader_info_log(FILE *fp, GLuint shader);

