


#define make_pixel_3_3_2(r,g,b) ((r & 0xe0) | (g & 0xe0) >> 3 | (b & 0xc0) >> 6)


void info();

void gl_program_info_log(FILE *fp, GLuint prog);
void gl_shader_info_log(FILE *fp, GLuint shader);

