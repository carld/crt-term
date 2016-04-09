#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>

#include "libtsm.h"
#include "shl_pty.h"

#include "event.h"
#include "terminal.h"

extern char **environ;


struct terminal {
  struct shl_pty *pty;
  struct tsm_vte *vte;
  struct tsm_screen *screen;
  struct tsm_screen_attr *attr;

  /* process id of the child process (slave side of psuedo terminal) */
  int pid;

  int age;

  void (*flush_callback) (struct event *, void *);
  void *user;
};

void hup_handler(int sig) {
  printf("%s\n", strsignal(sig));
  exit(1);
}

/* called when data has been read from the fd slave -> master  (vte input )*/
static void term_read_cb(struct shl_pty *pty, char *u8, size_t len, void *data) {
  struct terminal *term = (struct terminal *)data;
  assert(term != NULL);

  tsm_vte_input(term->vte, u8, len);
}

/* called when there is data to be written to the fd master -> slave  (vte output) */
static void term_write_cb(struct tsm_vte *vtelocal, const char *u8, size_t len, void *data) {
  int r;
  struct terminal *term = (struct terminal *)data;
  assert(term != NULL);
  r = shl_pty_write(term->pty, u8, len);
  if (r < 0) {
    printf ("could not write to pty, %d\n", r);
  }
}

static void log_tsm(void *data, const char *file, int line, const char *fn,
                    const char *subs, unsigned int sev, const char *format,
                    va_list args)
{
  fprintf(stderr, "%d: %s: ", sev, subs);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
}


struct terminal * terminal_create() {
  struct terminal *term = NULL;

  term = calloc(1, sizeof (struct terminal));

  assert(term != NULL);

  tsm_screen_new(&term->screen, log_tsm, 0);
  tsm_vte_new(&term->vte, term->screen, term_write_cb, term, log_tsm, 0);

  assert(term->screen);
  assert(term->vte);

  /* this call will fork */
  term->pid = shl_pty_open(&term->pty,
                   term_read_cb,
                   term,
                   tsm_screen_get_width(term->screen),
                   tsm_screen_get_height(term->screen));

  if (term->pid < 0) {
    perror("shl_pty_open");
  } else if (term->pid != 0 ) {
    /* parent, pty master */

    /* enable SIGCHD signal when it's child process exits */
    signal(SIGCHLD, hup_handler);

  } else {
    /* child, pty slave, shell */
    char *shell = getenv("SHELL") ? : "/bin/bash";
    char **argv = (char*[]) {  shell, NULL };
    execve(argv[0], argv, environ);
    /* never reached except on execve error */
    perror("execve error");
    exit(-2);
  }
  return term;
}

void terminal_resize(struct terminal *term, int w, int h) {
  tsm_screen_resize(term->screen, w, h);
  shl_pty_resize(term->pty, w, h);
}

void terminal_input(struct terminal *term, struct event *ev) {
  tsm_vte_handle_keyboard(term->vte, ev->k.key, ev->k.ascii, ev->k.mods, ev->k.ucs4);
}

int terminal_get_fd(struct terminal *term){
  return shl_pty_get_fd(term->pty);
}

static int draw_cb(struct tsm_screen *screen, uint32_t id,
                   const uint32_t *ch, size_t len,
                   unsigned int cwidth, unsigned int posx,
                   unsigned int posy,
                   const struct tsm_screen_attr *attr,
                   tsm_age_t age, void *data)
{
  struct terminal *term = (struct terminal *)data;
  struct event ev;
  int skip;

  skip = age && term->age && age <= term->age;

  if (skip)
    return 0;

  if (attr->inverse) {
    ev.t.fg[0] = attr->br; ev.t.fg[1] = attr->bg; ev.t.fg[2] = attr->bb;
    ev.t.bg[0] = attr->fr; ev.t.bg[1] = attr->fg; ev.t.bg[2] = attr->fb;
  } else {
    ev.t.fg[0] = attr->fr; ev.t.fg[1] = attr->fg; ev.t.fg[2] = attr->fb;
    ev.t.bg[0] = attr->br; ev.t.bg[1] = attr->bg; ev.t.bg[2] = attr->bb;
  }
  ev.t.ch = ch;
  ev.t.len = len;
  ev.t.x = posx;
  ev.t.y = posy;

  term->flush_callback(&ev, term->user);

  return 0;
}

void terminal_flush(struct terminal *term) {
  shl_pty_dispatch(term->pty);
  term->age = tsm_screen_draw(term->screen, draw_cb, term);
}

void terminal_set_callback(struct terminal *term, void (*fn) (struct event *, void *), void *user) 
{
  term->user = user;
  term->flush_callback = fn;
}


