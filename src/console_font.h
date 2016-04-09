
struct console_font {

  struct bdf_font *bdf;

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

  /* the dimension of the display in characters */
  int rows, cols;

};

/* create a new display in the given pixel dimensions, and the provided bdf font file */
void font_create(struct console_font **disp, int w, int h, const char *font, int dot_stretch);

/* retrieve the bitmap for a given character glyph encoding, coloring with foreground and background */
unsigned char * font_fetch_glyph(struct console_font *disp, int encoding, unsigned char fg, unsigned char bg);

