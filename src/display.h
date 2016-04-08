

struct display {

  struct bdf_font *font;

  /* the font bitmap */
  unsigned int font_width, font_height;
  unsigned char *font_pixels;
  int font_pixels_size;
  int glyph_width, glyph_height;
  int glyph_pixels_size;
  unsigned char *temp_glyph_pixels;

  /* keep track of how often a glyph bitmap has to been generated */
  unsigned int hits;
  unsigned int misses;

  /* the display bitmap */
  unsigned int width, height;
  unsigned char * pixels;
  int pixels_size;

  /* texture identifier for opengl */
  unsigned int tex_id;

  /* the dimension of the display in characters (glyphs) */
  int rows, cols;
  unsigned char * text_buffer;

  /* libtsm screen draw age */
  int age;
};

void display_create(struct display **disp, int w, int h, const char *font, int dot_stretch);
unsigned char * display_encoding_pixels(struct display *disp, int encoding);
unsigned char * display_fetch_glyph(struct display *disp, int encoding, unsigned char fg, unsigned char bg);

/* GL_RGA_3_3_2 format */
unsigned char make_pixel8(unsigned char r, unsigned char g, unsigned char b);

