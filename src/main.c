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
#include "terminal.h"
#include "display.h"
#include "scene.h"
#include "util_gl.h" //make_pixel

#define SHADER_PATH_VAR  "SHADER_PATH"
#define FONT_PATH_VAR    "FONT_PATH"

struct vtconsole {
  struct scene *scene;
  struct display *display;
  struct terminal *terminal;
};

static Display *x_display = NULL;

static int draw_cb(struct tsm_screen *screen, uint32_t id,
                   const uint32_t *ch, size_t len,
                   unsigned int cwidth, unsigned int posx,
                   unsigned int posy,
                   const struct tsm_screen_attr *attr,
                   tsm_age_t age, void *data)
{
  unsigned short fg, bg;
  struct vtconsole *con = (struct vtconsole *)data;
  int skip;

  skip = age && con->display->age && age <= con->display->age;
  if (skip)
    return 0;

  if (attr->inverse) {
    fg = make_pixel_3_3_2(attr->br, attr->bg, attr->bb);
    bg = make_pixel_3_3_2(attr->fr, attr->fg, attr->fb);
  } else {
    fg = make_pixel_3_3_2(attr->fr, attr->fg, attr->fb);
    bg = make_pixel_3_3_2(attr->br, attr->bg, attr->bb);
  }

  if (!len) {
    unsigned char *pixels;
    pixels = display_fetch_glyph(con->display, ' ', fg, bg);
    assert(pixels);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 
      posx * con->display->glyph_width, posy * con->display->glyph_height,
      con->display->glyph_width, con->display->glyph_height,
      GL_RGB, GL_UNSIGNED_BYTE_3_3_2,
      pixels);
  } else {
    unsigned char *pixels;
    int i;
    for(i = 0; i < len; i++) {
      pixels = display_fetch_glyph(con->display, ch[i], fg, bg);
      assert(pixels);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 
        (posx+i) * con->display->glyph_width, posy * con->display->glyph_height,
        con->display->glyph_width, con->display->glyph_height,
        GL_RGB, GL_UNSIGNED_BYTE_3_3_2,
        pixels);
    }
  }
  return 0;
}

static void key_callback(GLFWwindow* window, int key /*glfw*/, int scancode, int action, int mods) {
  struct vtconsole *con = glfwGetWindowUserPointer(window);
  int m = 0;
  unsigned int ucs4 = 0;
  assert(con != NULL);

  if (action == GLFW_RELEASE) return;

  if (mods & GLFW_MOD_CONTROL)  m |= TSM_CONTROL_MASK;
  if (mods & GLFW_MOD_SHIFT) m |= TSM_SHIFT_MASK;
  if (mods & GLFW_MOD_ALT)   m |= TSM_ALT_MASK;
  if (mods & GLFW_MOD_SUPER)  m |= TSM_LOGO_MASK;

  key = XkbKeycodeToKeysym(x_display, scancode, 0, mods & GLFW_MOD_SHIFT ? 1 : 0);
  ucs4 = xkb_keysym_to_utf32(key);
  if (!ucs4) ucs4 = TSM_VTE_INVALID;
  tsm_vte_handle_keyboard(con->terminal->vte, key, XKB_KEY_NoSymbol, m, ucs4);
}

static void waitEvents(Display *xdisplay, struct terminal *term) {
  int fdlist[2];
  
  fdlist[0] = ConnectionNumber(xdisplay);
  fdlist[1] = shl_pty_get_fd(term->pty);

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
  int opt;
  struct options opts = options_defaults();
 
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

  /* displaySize is the size of the character terminal texture */
  display_create(&con.display, opts.texture_wh[0], opts.texture_wh[1], opts.font_filename, opts.dot_stretch);

  opts.screen_wh[0] = con.display->cols;
  opts.screen_wh[1] = con.display->rows;

  terminal_create(&con.terminal, opts.screen_wh[0], opts.screen_wh[1]);

  if (!glfwInit())
    return -1;

  monitor = glfwGetPrimaryMonitor();
  mode    = glfwGetVideoMode(monitor); 

  /* window_wh is the size of the window being rendered to */
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

  printf("Resolution      %d x %d, %d x %d, %d x %d\n",
    opts.window_wh[0], opts.window_wh[1],
    opts.texture_wh[0], opts.texture_wh[1],
    opts.screen_wh[0], opts.screen_wh[1]);

  glfwMakeContextCurrent(window);
  glfwSetWindowUserPointer(window, &con);
  glfwSwapInterval(1);
  glfwSetKeyCallback(window, key_callback);
  if (opts.show_pointer == 0)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  x_display = glfwGetX11Display();

  info();

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK) exit(1);
  if (!GLEW_VERSION_3_2) exit(1);

  scene_setup(opts, &con.scene);

  float lastTime = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {

    con.display->age = tsm_screen_draw(con.terminal->screen, draw_cb, &con);

    render( glfwGetTime(), con.scene );

    glfwSwapBuffers(window);

    if (opts.wait_events) {
      waitEvents(x_display, con.terminal);
    }

    glfwPollEvents();
    shl_pty_dispatch(con.terminal->pty);

    if (glfwGetTime() > lastTime + 1.0) {
      /* do something once per second */
      lastTime = glfwGetTime();
    }
  }

  glfwDestroyWindow(window);
  
  glfwTerminate();

  return 0;
}

