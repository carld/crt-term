/* functions for blitting bitmap characters onto a surface 
 * Copyright (C) 2016 A. Carl Douglas
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "font/bdf.h"

#include "display.h"



/* return index of top left pixel for given character in font pixels */
#define font_pixels_encoding_index(d, e) (d->glyph_width * d->glyph_height * e)

/* return pointer to pixels for given encoding aka glyph or character */
unsigned short * display_encoding_pixels(struct display *disp, int encoding) 
{
  return & disp->font_pixels[font_pixels_encoding_index(disp, encoding)];
}

/* make an unsigned short rgba pixel with 4 bits per channel */
unsigned short make_pixel16(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  unsigned short p = 0;
  p |= (r >> 4) << 12;
  p |= (g >> 4) << 8; 
  p |= (b >> 4) << 4;
  p |= (a >> 4) << 0;
  return p;
}

static const unsigned short white_pixel = 0xffff;
static const unsigned short black_pixel = 0x000f;

/* copy the given encoding into the temp glyph pixel space with given foreground and background colours */
unsigned short *display_update_temp_glyph(struct display *disp, int encoding, unsigned short fg, unsigned short bg)
{
  unsigned short *dst = disp->temp_glyph_pixels;
  unsigned short *src = disp->font_pixels;
  int x, y;
  int index_dst = 0;
  int index_src = 0;
  for(y = 0; y < disp->glyph_height; y++) {
    for(x = 0; x < disp->glyph_width; x++) {
      index_dst = y * disp->glyph_width + x;
      index_src = font_pixels_encoding_index(disp, encoding) + index_dst;

      dst[index_dst] = src[index_src] == black_pixel ? bg : fg;
    }
  }
  return dst;
}

static void dot_stretch_row(unsigned short *row, unsigned int width) {
  int x = 0;
  for (x = width-1; x >= 0; x--) {
    if (row[x] == white_pixel && row[x+1] == black_pixel) {
      row[x+1] = white_pixel;
    }
  }
}

static void display_load_bdf(struct display *disp, const char *filename, int dot_stretching) {
  FILE *fp = NULL;
  int lines = 0;
  int ngly = 0;
  int total_chars = 255;

  fp = fopen(filename, "r");
  assert(fp != NULL);
  bdf_read(fp, &disp->font, &lines);
  bdf_sort_glyphs(disp->font);
  fclose(fp);

  disp->font_width = disp->font->bbox.width * total_chars;
  disp->font_height = disp->font->bbox.height;
  disp->font_pixels_size = disp->font->bbox.width * disp->font->bbox.height * total_chars;

  disp->font_pixels = calloc(disp->font_pixels_size, sizeof (unsigned short));
  assert(disp->font_pixels);
  disp->temp_glyph_pixels = calloc(disp->font->bbox.width * disp->font->bbox.height, sizeof (unsigned short));
  assert(disp->temp_glyph_pixels);

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
        unsigned short pixel = ((bmp>>(7-colbits)) & 1) == 1 ? white_pixel : black_pixel;
        int w = disp->font->bbox.width;
        int h = disp->font->bbox.height;
        int x = col;
        int y = row; /* + disp->font->bbox.height - glyph->bbox.height - glyph->bbox.offy;*/
        int z = ngly;
        int index = x + w * y + w * h * z;
        disp->font_pixels[index] = pixel;
      }
      if (dot_stretching) {
        int w = disp->font->bbox.width;
        int h = disp->font->bbox.height;
        int x = 0;
        int y = row;
        int z = ngly;
        int index = x + w * y + w * h * z;
        dot_stretch_row(&disp->font_pixels[index], w);
      }
    }
  }
}

void display_create(struct display ** dispp, int x, int y, const char *filename, int dot_stretch) {
  struct display *disp = NULL;
  if (*dispp==NULL) {
    *dispp = (struct display *) malloc(sizeof (struct display));
  }

  disp = *dispp;
  assert(disp != NULL);

  display_load_bdf(disp, filename, dot_stretch);

  disp->width = x;
  disp->height = y;
  disp->pixels_size = disp->width * disp->height;

  disp->pixels = calloc(disp->pixels_size, sizeof (unsigned short));
  assert(disp->pixels);

  disp->glyph_width = disp->font->bbox.width;
  disp->glyph_height = disp->font->bbox.height;
  disp->glyph_pixels_size = disp->glyph_width * disp->glyph_height;

  disp->rows = (disp->height / disp->font->bbox.height)-1;
  disp->cols = (disp->width / disp->font->bbox.width);

  disp->text_buffer = calloc(disp->rows * disp->cols, sizeof (unsigned char));
  assert(disp->text_buffer);
  disp->clean_buffer = calloc(disp->rows * disp->cols, sizeof (unsigned char));
  assert(disp->clean_buffer);

  disp->fg = calloc(disp->rows * disp->cols, sizeof (unsigned short));
  assert(disp->fg);
  disp->bg = calloc(disp->rows * disp->cols, sizeof (unsigned short));
  assert(disp->bg);

  disp->glyphs_rendered = 0;
  disp->glyphs_clean_skipped = 0;
}

