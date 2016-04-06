
struct terminal {
  struct shl_pty *pty;
  struct tsm_vte *vte;
  struct tsm_screen *screen;
  struct tsm_screen_attr *attr;

  /* process id of the child process (slave side of psuedo terminal) */
  int pid;
};

void terminal_create(struct terminal **term, int w, int h);

void terminal_set_callback(void (*callback) (void));

int select_fd_array(struct timeval *timeout, int *fd_list, int len);


