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
unsigned char * display_encoding_pixels(struct display *disp, int encoding) 
{
  return & disp->font_pixels[font_pixels_encoding_index(disp, encoding)];
}

static const unsigned char white_pixel = 0xff;
static const unsigned char black_pixel = 0x00;

/* copy the given encoding into the temp glyph pixel space with given foreground and background colours */
unsigned char *display_fetch_glyph(struct display *disp, int encoding, unsigned char fg, unsigned char bg)
{
  /* no need return colorised bitmap, can just return the glyph pointer */
  if (fg == white_pixel && bg == black_pixel) {
    return display_encoding_pixels(disp, encoding);    
  }
  unsigned char *dst = disp->temp_glyph_pixels;
  unsigned char *src = disp->font_pixels;
  int x, y;
  int index_dst = 0;
  int index_src = disp->glyph_width * disp->glyph_height * encoding;
  for(y = 0; y < disp->glyph_height; y++) {
    index_dst = y * disp->glyph_width;
    for(x = 0; x < disp->glyph_width; x++) {
      dst[index_dst + x] = 
        src[index_src + index_dst + x] == white_pixel ? fg : bg;
    }
  }
  return dst;
}

static void dot_stretch_row(unsigned char *row, unsigned int width) {
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

  disp->font_pixels = calloc(disp->font_pixels_size, sizeof (unsigned char));
  assert(disp->font_pixels);
  disp->temp_glyph_pixels = calloc(disp->font->bbox.width * disp->font->bbox.height, sizeof (unsigned char));
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
        unsigned char pixel = ((bmp>>(7-colbits)) & 1) == 1 ? white_pixel : black_pixel;
        int w = disp->font->bbox.width;
        int h = disp->font->bbox.height;
        int x = col;
        int y = row; /* + disp->font->bbox.height - glyph->bbox.height - glyph->bbox.offy;*/
        int z = ngly;
        int index = x + w * y + w * h * z;
        disp->font_pixels[index] = pixel;
      }

      /* pass back over the row and expand single pixels to double on the right side: 10 -> 011 */
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

  disp->pixels = calloc(disp->pixels_size, sizeof (unsigned char));
  assert(disp->pixels);

  disp->glyph_width = disp->font->bbox.width;
  disp->glyph_height = disp->font->bbox.height;
  disp->glyph_pixels_size = disp->glyph_width * disp->glyph_height;

  disp->rows = (disp->height / disp->font->bbox.height)-1;
  disp->cols = (disp->width / disp->font->bbox.width);

  disp->text_buffer = calloc(disp->rows * disp->cols, sizeof (unsigned char));
  assert(disp->text_buffer);
}

