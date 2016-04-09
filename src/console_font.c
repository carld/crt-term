/* functions for blitting bitmap characters onto a surface 
 * Copyright (C) 2016 A. Carl Douglas
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bdf.h"

#include "console_font.h"

/* return index of top left pixel for given character in font pixels */
#define font_pixels_encoding_index(d, e) (d->glyph_width * d->glyph_height * e)

/* return pointer to pixels for given encoding aka glyph or character */
unsigned char * font_encoding_pixels(struct console_font *font, int encoding) 
{
  return & font->font_pixels[font_pixels_encoding_index(font, encoding)];
}

static const unsigned char white_pixel = 0xff;
static const unsigned char black_pixel = 0x00;

/* copy the given encoding into the temp glyph pixel space with given foreground and background colours */
unsigned char *font_fetch_glyph(struct console_font *font, int encoding, unsigned char fg, unsigned char bg)
{
  /* no need return colorised bitmap, can just return the glyph pointer */
  if (fg == white_pixel && bg == black_pixel) {
    font->hits++;
    return font_encoding_pixels(font, encoding);    
  }

  /* create bitmap for the glyph using the foreground and background colours */
  font->misses++;
  unsigned char *dst = font->temp_glyph_pixels;
  unsigned char *src = font->font_pixels;
  int x, y;
  int index_dst = 0;
  int index_src = font->glyph_width * font->glyph_height * encoding;
  for(y = 0; y < font->glyph_height; y++) {
    index_dst = y * font->glyph_width;
    for(x = 0; x < font->glyph_width; x++) {
      dst[index_dst + x] = 
        src[index_src + index_dst + x] == white_pixel ? fg : bg;
    }
  }
  return dst;
}

/* single pixels gain a neighbouring pixel on the right side:  010 -> 011 */
static void dot_stretch_row(unsigned char *row, unsigned int width) {
  int x = 0;
  for (x = width-1; x >= 0; x--) {
    if (row[x] == white_pixel && row[x+1] == black_pixel) {
      row[x+1] = white_pixel;
    }
  }
}

/* create bitmaps for each glyph from the BDF file */
static void font_load_bdf(struct console_font *font, const char *filename, int dot_stretching) {
  FILE *fp = NULL;
  int lines = 0;
  int ngly = 0;
  int total_chars = 255;

  fp = fopen(filename, "r");
  assert(fp != NULL);

  bdf_read(fp, &font->bdf, &lines);
  bdf_sort_glyphs(font->bdf);

  fclose(fp);

  font->font_width = font->bdf->bbox.width * total_chars;
  font->font_height = font->bdf->bbox.height;
  font->font_pixels_size = font->bdf->bbox.width * font->bdf->bbox.height * total_chars;

  font->font_pixels = calloc(font->font_pixels_size, sizeof (unsigned char));
  assert(font->font_pixels);

  font->temp_glyph_pixels = calloc(font->bdf->bbox.width * font->bdf->bbox.height, sizeof (unsigned char));
  assert(font->temp_glyph_pixels);

  for(ngly = 0; ngly < total_chars; ngly++) {
    struct bdf_glyph *glyph = bdf_find_glyph(font->bdf, ngly, 0);
    int row, col;

    if (glyph == NULL) 
      continue;

    int ncolbytes = (glyph->bbox.width + 7) / 8;
    for (row = 0; row < glyph->bbox.height; row++) {
      for (col = 0; col < glyph->bbox.width; col++) {

        /* in the BDF bitmap, each pixel is only one bit, so there's 8 pixels in a byte */

        int colbyte = col / 8;
        int colbits  = col % 8;

        unsigned char bmp = glyph->bitmap[row * ncolbytes + colbyte];
        unsigned char pixel = ((bmp>>(7-colbits)) & 1) == 1 ? white_pixel : black_pixel;

        int w = font->bdf->bbox.width;
        int h = font->bdf->bbox.height;
        int x = col;
        int y = row; /* + font->bdf->bbox.height - glyph->bbox.height - glyph->bbox.offy;*/
        int z = ngly;
        int index = x + w * y + w * h * z;
        font->font_pixels[index] = pixel;
      }

      /* pass back over the row and expand single pixels to double on the right side: 10 -> 011 */
      if (dot_stretching) {
        int w = font->bdf->bbox.width;
        int h = font->bdf->bbox.height;
        int x = 0;
        int y = row;
        int z = ngly;
        int index = x + w * y + w * h * z;
        dot_stretch_row(&font->font_pixels[index], w);
      }
    }
  }
}

/* create a new display */
void font_create(struct console_font ** fontpp, int x, int y, const char *filename, int dot_stretch) {
  struct console_font *font = NULL;
  if (*fontpp == NULL) {
    *fontpp = (struct console_font *) malloc(sizeof (struct console_font));
  }
  font = *fontpp;
  assert(font != NULL);

  font_load_bdf(font, filename, dot_stretch);

  font->glyph_width = font->bdf->bbox.width;
  font->glyph_height = font->bdf->bbox.height;
  font->glyph_pixels_size = font->glyph_width * font->glyph_height;

  font->hits = 0;
  font->misses = 0;
}

