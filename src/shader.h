
struct shader {
  GLchar *filename;
  GLenum type;
  const GLchar *src;
  GLuint id;
  GLint status;
};

GLuint shader_program(struct shader *shaders, const GLuint count);
GLuint load_shader(struct shader *shader);

