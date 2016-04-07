/* functions for blitting bitmap characters onto a surface 
 * Copyright (C) 2016 A. Carl Douglas
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "font/bdf.h"

#include "display.h"

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

static unsigned short make_pixel16(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  unsigned short p = 0;
  p |= (r >> 4) << 12;
  p |= (g >> 4) << 8; 
  p |= (b >> 4) << 4;
  p |= (a >> 4) << 0;
  return p;
}

static const unsigned short white_pixel = 0xffff;
static const unsigned short black_pixel = 0x000f;

/* turn encoded text buffer into rendered glyph pixels */
void display_update(struct display *disp) {
  int glinew = disp->font->bbox.width;
  int dlinew = disp->width;
  int x, y, r, c;
  for(y = 0; y < disp->rows; y++) {
    for(x = 0; x < disp->cols; x++) {
      int text_ix = y * disp->cols + x;
      char ch = disp->text_buffer[text_ix];
      int src_index = font_pixels_encoding_index(disp, ch);
      int dst_index = display_pixels_index(disp, x * glinew, y * disp->font->bbox.height);

      if (disp->clean_buffer[text_ix] > 0) {
        disp->glyphs_clean_skipped++;
        continue;
      }
      disp->glyphs_rendered++;

      for (r = 0; r < disp->font->bbox.height; r++) {
        int src_ch_ix = src_index + r * glinew;
        int dst_ch_ix = dst_index + r * dlinew;
        for (c = 0; c < disp->font->bbox.width; c++) {
          disp->pixels[dst_ch_ix + c] = 
            disp->font_pixels[src_ch_ix + c] == black_pixel ? 
              disp->bg[text_ix] : disp->fg[text_ix];
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
  unsigned short fgpx = make_pixel16(fg[0],fg[1],fg[2],fg[3]);
  unsigned short bgpx = make_pixel16(bg[0],bg[1],bg[2],bg[3]);

  /* the position is clean if the character or attributes haven't changed */
  int clean = 
    disp->text_buffer[index] == ch 
    && disp->fg[index] == fgpx && disp->bg[index] == bgpx;

  if (clean) {
    disp->clean_buffer[index]++; /* age the cleanliness */
  } else {
    disp->clean_buffer[index] = 0;
  }

  disp->text_buffer[index] = ch;
  disp->fg[index] = fgpx;
  disp->bg[index] = bgpx;
}

static void dot_stretch_row(unsigned short *row, unsigned int width) {
  int x = 0;
  for (x = width-1; x >= 0; x--) {
    if (row[x]==white_pixel && row[x+1] == black_pixel) {
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


