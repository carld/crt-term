#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

#include <xkbcommon/xkbcommon.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "libtsm.h"
#include "shl_pty.h"
#include "external/xkbcommon-keysyms.h"

#include "options.h"
#include "event.h"

#include "terminal.h"
#include "renderer.h"
#include "util_gl.h"
#include "select_array.h"

struct vtconsole {
  struct renderer *renderer;
  struct terminal *terminal;

  Display *x11_display;
};

/* received draw event from the terminal */
static void flush_callback(struct event *ev, void *user) {
  struct renderer *renderer = (struct renderer *) user;
  unsigned char fg = make_pixel_3_3_2(ev->t.fg[0], ev->t.fg[1], ev->t.fg[2]);
  unsigned char bg = make_pixel_3_3_2(ev->t.bg[0], ev->t.bg[1], ev->t.bg[2]);

  if (ev->t.len == 0) {
    renderer_putc(renderer, ' ', ev->t.x, ev->t.y, fg, bg);
  } else {
    int i;
    for(i = 0; i < ev->t.len; i++) {
      renderer_putc(renderer, ev->t.ch[i],ev->t.x+i, ev->t.y, fg, bg);
    }
  }
}

/* have a keyboard event for the terminal */
static void key_callback(GLFWwindow* window, int key /*glfw*/, int scancode, int action, int mods) {
  struct vtconsole *con = glfwGetWindowUserPointer(window);
  int m = 0;
  struct event event;

  assert(con != NULL);

  if (action == GLFW_RELEASE) return;

  event.k.scancode = scancode;

  if (mods & GLFW_MOD_CONTROL)  m |= TSM_CONTROL_MASK;
  if (mods & GLFW_MOD_SHIFT) m |= TSM_SHIFT_MASK;
  if (mods & GLFW_MOD_ALT)   m |= TSM_ALT_MASK;
  if (mods & GLFW_MOD_SUPER)  m |= TSM_LOGO_MASK;

  event.k.mods = m;
  event.k.key = key;
  event.k.ascii = XKB_KEY_NoSymbol;

  key = XkbKeycodeToKeysym(con->x11_display, scancode, 0, mods & GLFW_MOD_SHIFT ? 1 : 0);
  event.k.ucs4 = xkb_keysym_to_utf32(key);

  if (!event.k.ucs4) 
    event.k.ucs4 = TSM_VTE_INVALID;

  terminal_input(con->terminal, &event);
}

/*
  Calling glfwPostEmptyEvent calls XSendEvent,
  which done from another thread or signal handler,
  causes a race condition and eventual deadlock.

  glfwWaitEvent knows nothing about the pty file descriptor.

  To be able to redraw the screen on either a keyboard event 
  or receiving data from the slave end of the pty the shell in this case,
  select on both the X11 connection file descriptor and
  the pty file descriptor.
*/
static void waitEvents(Display *xdisplay, struct terminal *term) {
  int fdlist[2];
  
  fdlist[0] = ConnectionNumber(xdisplay);
  fdlist[1] = terminal_get_fd(term);

  assert(fdlist[0] > 0);
  assert(fdlist[1] > 0);

  while( select_fd_array(NULL, fdlist, 2) <= 0 )
    ;
}

static void window_hints(const GLFWvidmode *mode) {
  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  glfwWindowHint(GLFW_DECORATED, GL_FALSE);

}

int main(int argc, char *argv[], char *envp[])
{
  GLFWwindow *window;
  GLFWmonitor *monitor;
  const GLFWvidmode *mode;
  struct vtconsole con = {NULL,NULL,NULL};
  struct options opts = options_defaults();
  int opt;
 
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

  if (!glfwInit())
    return -1;

  terminal_create(&con.terminal);

  monitor = glfwGetPrimaryMonitor();
  mode    = glfwGetVideoMode(monitor); 

  if (opts.window_wh[0] == 0 || opts.window_wh[1] == 0) {
    opts.window_wh[0] = mode->width;
    opts.window_wh[1] = mode->height;
  }
  
  window_hints(mode);

  window = glfwCreateWindow(opts.window_wh[0], opts.window_wh[1], argv[0], opts.full_screen ? monitor : NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetWindowUserPointer(window, &con);
  glfwSwapInterval(1);
  glfwSetKeyCallback(window, key_callback);

  if (opts.show_pointer == 0)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  con.x11_display = glfwGetX11Display();

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK) exit(1);
  if (!GLEW_VERSION_3_2) exit(1);

  renderer_create(opts, &con.renderer);

  int glyph_wh[2];
  renderer_get_glyph_wh(con.renderer, glyph_wh) ;

  terminal_resize(con.terminal, opts.texture_wh[0] / glyph_wh[0], opts.texture_wh[1] / glyph_wh[1]);

  terminal_set_callback(con.terminal, flush_callback, con.renderer);

  print_options(opts);
  info();

  float lastTime = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {

    terminal_flush(con.terminal);

    render( glfwGetTime(), con.renderer );

    glfwSwapBuffers(window);

    if (opts.wait_events) {
      waitEvents(con.x11_display, con.terminal);
    }

    glfwPollEvents();

    if (glfwGetTime() > lastTime + 1.0) {
      /* do something once per second */
      lastTime = glfwGetTime();
    }
  }

  glfwDestroyWindow(window);
  
  glfwTerminate();

  return 0;
}

