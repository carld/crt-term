#include <stdlib.h>
#include <stdio.h>
#include "options.h"
#include "files.h"

#define SHADER_PATH_VAR  "SHADER_PATH"
#define FONT_PATH_VAR    "FONT_PATH"

struct options options_defaults() {

  char * shader_path[] = { getenv(SHADER_PATH_VAR), ".", "./shaders" };
  char * font_path[] = { getenv(FONT_PATH_VAR), ".", "./fonts" };

  struct options opts;

  opts.full_screen = 0;
  opts.wait_events = 1;
  opts.show_pointer = 1;
  opts.dot_stretch = 1;
  opts.linear_filter = 0;

  opts.font_filename = find_file("9x15.bdf", font_path, 3);
  opts.vertex_shader_filename = find_file("vertex.glsl", shader_path, 3);
  opts.fragment_shader_filename = find_file("crt-lottes.glsl", shader_path, 3);

  opts.screen_wh[0] = 80;
  opts.screen_wh[1] = 25;

  opts.window_wh[0] = 80*9;
  opts.window_wh[1] = 25*15; 
  opts.texture_wh[0] = 80*9;
  opts.texture_wh[1] = 25*15;

  return opts;
}

static char * str_opt_bool(int i) {
  return i == 0 ? "off" : "on";
}

void print_options(struct options opts) {
  printf("Texture         %d %d\n", opts.texture_wh[0], opts.texture_wh[1]);
  printf("Window          %d %d\n", opts.window_wh[0], opts.window_wh[1]);
  printf("Characters      %d %d\n", opts.screen_wh[0], opts.screen_wh[1]);

  printf("Full screen     %s\n", str_opt_bool(opts.full_screen));
  printf("Animation       %s\n", str_opt_bool(!opts.wait_events));
  printf("Dot stretching  %s\n", str_opt_bool(opts.dot_stretch));
  printf("Linear filter   %s\n", str_opt_bool(opts.linear_filter));
  printf("Mouse cursor    %s\n", str_opt_bool(opts.show_pointer));

  printf("Fragment shader %s\n", opts.fragment_shader_filename);
  printf("Font            %s\n", opts.font_filename);
}


