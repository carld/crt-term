#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "font/bdf.h"

#include "libtsm.h"
#include "terminal.h"

#include "display.h"

// -misc-fixed-medium-r-normal-*-15-*-*-*-*-*


/* return index of top left pixel for given character in font pixels */
static int font_pixels_encoding_index(struct display *disp, int encoding) {
  int w = disp->font->bbox.width;
  int h = disp->font->bbox.height;
  int x = 0;
  int y = 0;
  int z = encoding;
  return x + w * y + w * h * z;
}

/* return index of pixel at location x y */
static int display_pixels_index(struct display *disp, int x, int y) {
  int w = disp->width;
  return x + w * y;
}

static GLushort make_pixel16(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
  GLushort p = 0;
  p |= (r >> 4) << 12;
  p |= (g >> 4) << 8; 
  p |= (b >> 4) << 4;
  p |= (a >> 4) << 0;
  return p;
}

#define white_pixel make_pixel16(0xff,0xff,0xff,0xff)
#define black_pixel make_pixel16(0x00,0x00,0x00,0xff)

#if 1
/* for debugging */
static void display_dump_glyph(struct display *disp, int encoding) {
  int x, y;
  int index = font_pixels_encoding_index(disp, encoding);
  int w = disp->font->bbox.width;
  printf("dumping glyph %d at %d\n", encoding, index);
  for(y = 0; y < disp->font->bbox.height; y++ ) {
    for(x = 0; x < disp->font->bbox.width; x++ ) {
      int offset = x + y * w;
      int pixel = disp->font_pixels[index + offset];
      printf("%02X", pixel);
    }
    printf("\n");
  }
}
#endif

/* turn encoded text buffer into rendered glyph pixels */
void display_update(struct display *disp) {
  int glinew = disp->font->bbox.width;
  int dlinew = disp->width;
  int x, y, r, c;
  for(y = 0; y < disp->rows; y++) {
    for(x = 0; x < disp->cols; x++) {
      int text_index = y * disp->cols + x;
      char ch = disp->text_buffer[text_index];
      int src_index = font_pixels_encoding_index(disp, ch);
      int dst_index = display_pixels_index(disp, x * disp->font->bbox.width, y * disp->font->bbox.height);
      int attr_index = y * disp->cols + x;
      for (r = 0; r < disp->font->bbox.height; r++) {
        for (c = 0; c < disp->font->bbox.width; c++) {
          int src_channel_index = src_index + r * glinew + c;
          int dst_channel_index = dst_index + r * dlinew + c;

          disp->pixels[dst_channel_index] = 
            disp->font_pixels[src_channel_index] != black_pixel ? 
              disp->fg[attr_index] : disp->bg[attr_index];
        }
      }
    }
  }
}

void display_print(struct display *disp, const char *text, int x, int y) {
  int index = y * disp->cols + x;
  memcpy(&disp->text_buffer[index], text, strlen(text));
}

void display_put(struct display *disp, int ch, int x, int y, unsigned char fg[4], unsigned char bg[4]) {
  int index = y * disp->cols + x;
  disp->text_buffer[index] = ch;
  disp->fg[index] = make_pixel16(fg[0],fg[1],fg[2],fg[3]);
  disp->bg[index] = make_pixel16(bg[0],bg[1],bg[2],bg[3]);
  //display_dump_glyph(disp,ch);
}

void display_update_texture(struct display *disp) {
  glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
  glBindTexture(GL_TEXTURE_2D, disp->tex_id);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
    disp->width, disp->height, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, disp->pixels);
  //glGenerateMipmap(GL_TEXTURE_2D);
}

static void display_load_bdf(struct display *disp, const char *filename) {
  FILE *fp = NULL;
  int lines = 0;
  int ngly = 0;
  int total_chars = 255;

  printf("Opening '%s'\n", filename);
  fp = fopen(filename, "r");
  assert(fp != NULL);
  bdf_read(fp, &disp->font, &lines);
  bdf_sort_glyphs(disp->font);
  fclose(fp);

  printf("white: %04X  black: %04X\n", white_pixel, black_pixel);

  disp->font_width = disp->font->bbox.width * total_chars;
  disp->font_height = disp->font->bbox.height;
  disp->font_pixels_size = disp->font->bbox.width * disp->font->bbox.height * total_chars;
  disp->font_pixels = calloc(disp->font_pixels_size, sizeof (GLushort));

  for(ngly = 0; ngly < total_chars; ngly++) {
    struct bdf_glyph *glyph = bdf_find_glyph(disp->font, ngly, 0);
    int row, col;
    if (glyph == NULL) continue;
    int ncolbytes = (glyph->bbox.width + 7) / 8;
    for (row = 0; row < glyph->bbox.height; row++) {
      for (col = 0; col < glyph->bbox.width; col++) {
        int colbyte = col / 8;
        int colbits  = col % 8;
        unsigned char bmp = glyph->bitmap[row * ncolbytes + colbyte];
        GLushort pixel = ((bmp>>(7-colbits)) & 1) == 1 ? white_pixel : black_pixel;
        int w = disp->font->bbox.width;
        int h = disp->font->bbox.height;
        int x = col;
        int y = row; /* + disp->font->bbox.height - glyph->bbox.height - glyph->bbox.offy;*/
        int z = ngly;
        int index = x + w * y + w * h * z;
        disp->font_pixels[index] = pixel;
      }
    }
  }
}

static unsigned int nearest_power_of_two(unsigned int v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return v++;
}

void display_create(struct display ** dispp, int x, int y, const char *filename) {
  struct display *disp = NULL;
  if (*dispp==NULL) *dispp = (struct display *) malloc(sizeof (struct display));

  disp = *dispp;
  assert(disp != NULL);

  display_load_bdf(disp, filename);

  disp->width = nearest_power_of_two(x); // needs to be a power of two to be a texture?
  disp->height = nearest_power_of_two(y);
  disp->pixels_size = disp->width * disp->height;
  printf("texture width %d height %d\n", disp->width, disp->height);

  disp->pixels = calloc(disp->pixels_size, sizeof (GLushort));
  assert(disp->pixels);

  disp->rows = disp->height / disp->font->bbox.height;
  disp->cols = disp->width / disp->font->bbox.width;
  disp->text_buffer = calloc(disp->rows * disp->cols, sizeof (GLubyte));
  assert(disp->text_buffer);
  disp->fg = calloc(disp->rows * disp->cols, sizeof (GLushort));
  assert(disp->fg);
  disp->bg = calloc(disp->rows * disp->cols, sizeof (GLushort));
  assert(disp->bg);

  glGenTextures(1, & disp->tex_id);

  printf("Loaded font\n");
}


