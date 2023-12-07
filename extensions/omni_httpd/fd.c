/*! \file */

// from man 5 standards:
// SUS (XPG4v2)
// The application must define _XOPEN_SOURCE and set _XOPEN_SOURCE_EXTENDED=1. If _XOPEN_SOURCE is
// defined with a value, the value must be less than 500.
#ifdef __sun
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1
#endif

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#if defined(__FreeBSD__)
#include <sys/param.h>
#endif

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include "fd.h"

#define FD_BUFFER                                                                                  \
  union {                                                                                          \
    char buf[CMSG_SPACE(sizeof(int) * MAX_N_FDS)];                                                 \
    struct cmsghdr h;                                                                              \
  }

static int send_fds_with_buffer(int sock, cvec_fd *fds, void *buffer) {
  struct msghdr msghdr;
  struct iovec payload_vec[2];
  struct cmsghdr *cmsg;
  unsigned n_fds = cvec_fd_size(fds);
  bool has_more_fds = false;

  // Construct the payload vector - payload has a bool, to signal the
  // receiver if there are more fds, and all the original fds.
  payload_vec[0].iov_base = &has_more_fds;
  payload_vec[0].iov_len = sizeof(has_more_fds);

  msghdr.msg_name = NULL;
  msghdr.msg_namelen = 0;
  msghdr.msg_iov = payload_vec;
  msghdr.msg_iovlen = 2;
  msghdr.msg_flags = 0;
  msghdr.msg_control = buffer;
  // CMSG_FIRSTHDR depends on msg_controllen and that is not set until later in the loop.
  // So, directly access the msg_control buffer and initialise cmsg.
  cmsg = (struct cmsghdr *)buffer;
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;

  unsigned fds_it = 0, i = 0;
  int *cmsg_data = (int *)CMSG_DATA(cmsg);
  // If the n_fds are more than MAX_N_FDS, they will be sent over multiple messages
  while (fds_it < n_fds) {
    // Deduce number of fds to send this loop
    unsigned n_fds_to_send = n_fds - fds_it;
    if (n_fds_to_send > MAX_N_FDS) {
      // Can only send MAX_N_FDS per sendmsg
      n_fds_to_send = MAX_N_FDS;
      has_more_fds = true;
    } else {
      has_more_fds = false;
    }
    size_t fds_size = sizeof(int) * n_fds_to_send;
    msghdr.msg_controllen = CMSG_SPACE(fds_size);
    cmsg->cmsg_len = CMSG_LEN(fds_size);

    // Point the payload vector to the original fds
    int *fd_data = cvec_fd_front(fds);
    payload_vec[1].iov_base = fd_data + fds_it;
    payload_vec[1].iov_len = sizeof(cvec_fd_value) * n_fds_to_send;

    // Copy the fds into the ancillary message
    for (i = 0; i < n_fds_to_send; i++, fds_it++) {
      cmsg_data[i] = fd_data[fds_it];
    }

    if (sendmsg(sock, &msghdr, 0) < 0)
      return -1;
  }

  return 0;
}

int send_fds(int sock, cvec_fd *fds) {
  FD_BUFFER buffer;

  return send_fds_with_buffer(sock, fds, &buffer);
}

static void cvec_fd_fd_close_and_clear(cvec_fd_fd *vec) {
  c_FOREACH(it, cvec_fd_fd, *vec) { close(it.ref->fd); }
  cvec_fd_fd_clear(vec);
}

static cvec_fd_fd recv_fds_with_buffer(int sock, void *buffer) {
  struct msghdr msghdr;
  unsigned n_fds;
  struct iovec payload_vec[2];
  struct cmsghdr *cmsg;
  int i;
  int fds[sizeof(int) * MAX_N_FDS];
  bool has_more_fds;

  // Setup the payload vector - post read, it will have a bool, to signal
  // the receiver if there are more fds, and all the original fds.
  payload_vec[0].iov_base = &has_more_fds;
  payload_vec[0].iov_len = sizeof(has_more_fds);
  payload_vec[1].iov_base = fds;
  payload_vec[1].iov_len = sizeof(int) * MAX_N_FDS;

  msghdr.msg_name = NULL;
  msghdr.msg_namelen = 0;
  msghdr.msg_iov = payload_vec;
  msghdr.msg_iovlen = 2;
  msghdr.msg_flags = 0;
  msghdr.msg_control = buffer;
  msghdr.msg_controllen = CMSG_SPACE(sizeof(int) * MAX_N_FDS);

  cvec_fd_fd result = cvec_fd_fd_init();

  do {
    if (recvmsg(sock, &msghdr, 0) < 0) {
      // recv failed.
      // Close any fds received so far so as to not send back partial results.
      cvec_fd_fd_close_and_clear(&result);
      return result;
    }

    cmsg = CMSG_FIRSTHDR(&msghdr);
    if (cmsg == NULL) {
      // Close any fds received so far so as to not send back partial results.
      cvec_fd_fd_close_and_clear(&result);
      return result;
    }

    n_fds = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
    cvec_fd_fd_reserve(&result, cvec_fd_fd_size(&result) + n_fds);

    int *cmsg_data = (int *)CMSG_DATA(cmsg);
    for (i = 0; i < n_fds; i++) {
      cvec_fd_fd_push(&result, (fd_fd){.fd = cmsg_data[i], .master_fd = fds[i]});
    }

    // continue receiving if there are more FDS
  } while (has_more_fds);

  return result;
}

cvec_fd_fd recv_fds(int sock) {
  FD_BUFFER buffer;

  return recv_fds_with_buffer(sock, &buffer);
}
