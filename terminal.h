
struct terminal {
  struct shl_pty *pty;
  struct tsm_vte *vte;
  struct tsm_screen *screen;
  struct tsm_screen_attr *attr;

  int pid;
};

void terminal_create(struct terminal **term, int w, int h);

