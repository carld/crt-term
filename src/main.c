#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "options.h"
#include "interface.h"

int main(int argc, char *argv[], char *envp[])
{
  struct options opts = options_defaults();
  int opt;

  void * interface;

  opts.name = argv[0];
 
  while ((opt = getopt(argc, argv, "f:s:g:w:ldphm")) != -1) {
    switch (opt) {
    case 'f':
      opts.font_filename = optarg;
      break;
    case 's':
      opts.fragment_shader_filename = optarg;
      break;    
    case 'd':
      opts.dot_stretch ^= 1;
      break;      
    case 'g':
      opts.texture_wh[0] = atoi(strtok(optarg,"x"));
      opts.texture_wh[1] = atoi(strtok(NULL,"x"));
      break;
    case 'w':
      opts.window_wh[0] = atoi(strtok(optarg,"x"));
      opts.window_wh[1] = atoi(strtok(NULL,"x"));
      break;
    case 'l':
      opts.linear_filter ^= 1;
      break;
    case 'p':
      opts.wait_events ^= 1; 
      break;
    case 'm':
      opts.full_screen ^= 1;
      opts.show_pointer ^= 1;
      opts.window_wh[0] = 0; /* 0 means auto detect */
      opts.window_wh[1] = 0;
      break;
    case 'h':
    default: /* '?' */
       printf("Usage: %s [-f bdf file] [-s glsl shader] [-g w x h] [-w w x h] [-d] [-l] [-p] [-m]\n", argv[0]);
       exit(EXIT_SUCCESS);
    }
  }

  interface = setup(opts);

  run(interface);

  release(interface);

  return 0;
}

