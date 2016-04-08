

struct display {

  struct bdf_font *font;

  /* the font bitmap */
  unsigned int font_width, font_height;
  unsigned short *font_pixels;
  int font_pixels_size;
  int glyph_width, glyph_height;
  int glyph_pixels_size;
  unsigned short *temp_glyph_pixels;

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
  unsigned char * clean_buffer;

  /* libtsm screen draw age */
  int age;

  /* statistics */
  unsigned glyphs_rendered;
  unsigned glyphs_clean_skipped;
};

void display_create(struct display **disp, int w, int h, const char *font, int dot_stretch);
unsigned short * display_encoding_pixels(struct display *disp, int encoding);
unsigned short *display_update_temp_glyph(struct display *disp, int encoding, unsigned short fg, unsigned short bg);
unsigned short make_pixel16(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
