
struct options 
{
  /* window size in pixels,e.g. 640x480 */
  int window_wh[2];

  /* texture size in pixels, e.g. 512x512 */
  int texture_wh[2];

  /* screen size in characters, e.g. 80x25 */
  int screen_wh[2];

  char *fragment_shader_filename;
  char *vertex_shader_filename;

  char *font_filename;

  unsigned int full_screen : 1;
  unsigned int wait_events : 1;
  unsigned int show_pointer : 1;
  unsigned int dot_stretch : 1;

  unsigned int linear_filter : 1; /* otherwise nearest */
};

struct options options_defaults();

