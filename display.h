
struct display {

  struct bdf_font *font;

  /* the font bitmap */
  unsigned int font_width, font_height;
  unsigned short *font_pixels;
  int font_pixels_size;

  /* the display bitmap */
  unsigned int width, height;
  unsigned short * pixels;
  unsigned short * fg;
  unsigned short * bg;
  int pixels_size;

  /* texture identifier for opengl */
  unsigned int tex_id;

  /* the dimension of the display in characters (glyphs) */
  int rows, cols;
  unsigned char * text_buffer;

  /* libtsm screen draw age */
  int age;
};

void display_put(struct display *disp, int ch, int x, int y, unsigned char fg[4], unsigned char bg[4]);
void display_create(struct display **disp, int w, int h, const char *font);
void display_update(struct display *disp);

