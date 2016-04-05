#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>

#include "libtsm.h"
#include "shl_pty.h"

#include "terminal.h"

extern char **environ;

void hup_handler(int s) {
  printf("Signal: %s\n", strsignal(s));
  exit(1);
}

void io_handler(int s) {
  printf("%s\n", strsignal(s));
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


void terminal_create(struct terminal **termp, int w, int h) {
  struct terminal *term = NULL;

  if (*termp == NULL) *termp = calloc(1, sizeof (struct terminal));

  term = *termp;

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
    perror("fork problem");
  } else if (term->pid != 0 ) {
    /* parent, pty master */
#if 0
    int fd = shl_pty_get_fd(term->pty);
    unsigned oflags = 0;
    /* enable SIGIO signal for this process when it has a ready file descriptor */
    signal(SIGIO, &io_handler);
    fcntl(fd, F_SETOWN, getpid(  ));
    oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, oflags | FASYNC);
#endif

    /* watch for SIGHUP */
    signal(SIGCHLD, hup_handler);
  } else {
    /* child, shell */
    char *shell = getenv("SHELL") ? : "/bin/bash";
    char **argv = (char*[]) {  shell, NULL };
    execve(argv[0], argv, environ);
    /* never reached except on execve error */
    perror("execve error");
    exit(-2);
  }
  tsm_screen_resize(term->screen, w, h);
  shl_pty_resize(term->pty, tsm_screen_get_width(term->screen), tsm_screen_get_height(term->screen));
}

