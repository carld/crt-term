
struct display {
  struct bdf_font *font;

  GLuint font_width, font_height;
  GLubyte *font_pixels;
  int font_pixels_size;

  GLuint width, height;
  GLubyte *pixels;
  int pixels_size;

  int bpp;

  GLuint tex_id;

  int rows, cols;

  char * text_buffer;

  GLubyte * fg;
  GLubyte * bg;

  int age;
};


void display_put(struct display *disp, int ch, int x, int y, unsigned char fg[4], unsigned char bg[4]);
void display_create(struct display **disp, int w, int h, const char *font);
void display_update(struct display *disp);
void display_update_texture(struct display *disp);

