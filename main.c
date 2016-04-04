#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <GL/glu.h>

#include <xkbcommon/xkbcommon.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "font/bdf.h"

#include "libtsm.h"
#include "shl_pty.h"
#include "external/xkbcommon-keysyms.h"

#include "terminal.h"
#include "display.h"

Display *x_display;

static int draw_cb(struct tsm_screen *screen, uint32_t id,
                   const uint32_t *ch, size_t len,
                   unsigned int cwidth, unsigned int posx,
                   unsigned int posy,
                   const struct tsm_screen_attr *attr,
                   tsm_age_t age, void *data)
{
  uint8_t fg[4], bg[4];
  struct display *disp = (struct display *)data;

  if (attr->inverse) {
    fg[0] = attr->br; fg[1] = attr->bg; fg[2] = attr->bb; fg[3] = 255;
    bg[0] = attr->fr; bg[1] = attr->fg; bg[2] = attr->fb; bg[3] = 255;
  } else {
    fg[0] = attr->fr; fg[1] = attr->fg; fg[2] = attr->fb; fg[3] = 255;
    bg[0] = attr->br; bg[1] = attr->bg; bg[2] = attr->bb; bg[3] = 255;
  }

  if (!len) {
    display_put(disp, ' ', posx, posy, fg, bg);
  } else {
    int i;
    for(i = 0; i < len; i++)
      display_put(disp, ch[i], posx, posy, fg, bg);
  }
  return 0;
}

static void key_callback(GLFWwindow* window, int key /*glfw*/, int scancode, int action, int mods) {
  struct terminal *term = glfwGetWindowUserPointer(window);
  int m = 0;
  unsigned int ucs4 = 0;
  assert(term != NULL);

  if (action == GLFW_RELEASE) return;

  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
  if (mods & GLFW_MOD_CONTROL)  m |= TSM_CONTROL_MASK;
  if (mods & GLFW_MOD_SHIFT) m |= TSM_SHIFT_MASK;
  if (mods & GLFW_MOD_ALT)   m |= TSM_ALT_MASK;
  if (mods & GLFW_MOD_SUPER)  m |= TSM_LOGO_MASK;

  key = XkbKeycodeToKeysym(x_display, scancode, 0, mods & GLFW_MOD_SHIFT ? 1 : 0);
  ucs4 = xkb_keysym_to_utf32(key);
  if (!ucs4) ucs4 = TSM_VTE_INVALID;
  tsm_vte_handle_keyboard(term->vte, key, XKB_KEY_NoSymbol, m, ucs4);
}

int main(int argc, char *argv[], char *envp[])
{
  GLFWwindow* window;
  GLFWmonitor* monitor;
  const GLFWvidmode* mode;
  struct terminal *terminal = NULL;
  struct display *display = NULL;
  GLfloat width = 1280, height = 800;
  GLfloat rot = 5, rotd = 0.1;

  if (!glfwInit())
    return -1;
 
  monitor = glfwGetPrimaryMonitor();
  mode = glfwGetVideoMode(monitor); 
  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  window = glfwCreateWindow(mode->width, mode->height, "Hello World", monitor, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  display_create(&display, width * 0.5, height * 1.0, "9x15.bdf");
  terminal_create(&terminal, display->width / display->font->bbox.width, display->height / display->font->bbox.height);  // 9x15.bdf

  glfwSetWindowUserPointer(window, terminal);
  glfwSwapInterval(1);
  glfwSetKeyCallback(window, key_callback);
  x_display = glfwGetX11Display();
  while (!glfwWindowShouldClose(window)) {
   
    shl_pty_dispatch(terminal->pty); 

    tsm_screen_draw(terminal->screen, draw_cb, display);

    display_update(display);
    display_update_texture(display);

    glViewport(0, 0, (GLsizei) width, (GLsizei) height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#if ORTHO
    glOrtho(0, width, 0, height, 0, 100);
#else
    gluPerspective(90, 1.0 /*width / height*/ , 0.0, 1024.0);
#endif
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.1,0.1,0.1,1.0);
    glColor4ub(255,255,255,255);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    //glEnable(GL_DEPTH_TEST);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBindTexture(GL_TEXTURE_2D, display->tex_id);
#if ORTHO
#else
    glRotatef(rot, 1.0,0.0,0.0);
    glTranslatef(0,0,-(display->width * 0.75)); 
    glTranslatef(-(display->width * 0.5),-(display->height * 0.5),0); 
#endif
    glPolygonMode(GL_FRONT,GL_LINE);
    glBegin(GL_QUADS);
      glTexCoord2f(0.0, 1.0); glVertex3f(0, 0, 0.0);
      glTexCoord2f(0.0, 0.0); glVertex3f(0, display->height, 0.0);
      glTexCoord2f(1.0, 0.0); glVertex3f(display->width, display->height, 0.0);
      glTexCoord2f(1.0, 1.0); glVertex3f(display->width, 0.0, 0.0);
    glEnd();
//    glRecti(-100,-100,100,100);
    glFlush();
    glDisable(GL_TEXTURE_2D);

    glfwSwapBuffers(window);
    glfwPollEvents();
    usleep(50000);
    if (rot >= 3) rotd = -0.1;
    if (rot <= -3) rotd = +0.1;
    rot += rotd;
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

