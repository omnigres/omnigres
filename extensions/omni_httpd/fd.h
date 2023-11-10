/*! \file */

#ifndef OMNI_HTTPD_FD_H
#define OMNI_HTTPD_FD_H

#include <libgluepg_stc.h>

#define i_key int
#define i_tag fd
#include <stc/cvec.h>

/**
 * This structure keeps a file descriptor and its master descriptor id for
 * uniqueness comparison.
 */
typedef struct {
  int fd;
  int master_fd;
} fd_fd;

inline int fd_fd_cmp(const fd_fd *fd1, const fd_fd *fd2) {
  int fd = fd1->fd - fd2->fd;
  if (fd == 0) {
    return fd1->master_fd - fd2->master_fd;
  } else {
    return fd;
  }
}

#define i_key fd_fd
#define i_tag fd_fd
#define i_cmp fd_fd_cmp
#include <stc/cvec.h>

#ifndef SCM_MAX_FD
#define SCM_MAX_FD 253
#endif

/**
 * Maximum number of fds that can be sent or received
 */
#define MAX_N_FDS SCM_MAX_FD

int send_fds(int sock, cvec_fd *fds);
cvec_fd_fd recv_fds(int sock);

#endif // OMNI_HTTPD_FD_H