#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

int select_fd_array(struct timeval *timeout, int *fd_list, int len) {
  fd_set fds;
  int count = 0, i = 0;
  FD_ZERO(&fds);

  for (i = 0; i < len; i++) {
    int fd = fd_list[i];

    FD_SET(fd, &fds);

    if (fd > count)
      count = fd;
  }

  count++;

  return select(count, &fds, NULL, NULL, timeout);
}

