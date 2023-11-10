/*! \file */

// from man 5 standards:
// SUS (XPG4v2)
// The application must define _XOPEN_SOURCE and set _XOPEN_SOURCE_EXTENDED=1. If _XOPEN_SOURCE is
// defined with a value, the value must be less than 500.
#ifdef __sun
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1
#endif

#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#if defined(__FreeBSD__)
#include <sys/param.h>
#endif

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include "fd.h"

#define FD_BUFFER(n)                                                                               \
  union {                                                                                          \
    char buf[CMSG_SPACE(sizeof(int)) * n];                                                         \
    struct cmsghdr h;                                                                              \
  }

static int send_fds_with_buffer(int sock, cvec_fd *fds, void *buffer) {
  struct msghdr msghdr;
  struct iovec payload_ptr;
  struct cmsghdr *cmsg;
  unsigned n_fds = cvec_fd_size(fds);

  payload_ptr.iov_base = cvec_fd_front(fds);
  payload_ptr.iov_len = sizeof(cvec_fd_value) * cvec_fd_size(fds);

  msghdr.msg_name = NULL;
  msghdr.msg_namelen = 0;
  msghdr.msg_iov = &payload_ptr;
  msghdr.msg_iovlen = 1;
  msghdr.msg_flags = 0;
  msghdr.msg_control = buffer;
  msghdr.msg_controllen = CMSG_SPACE(sizeof(int) * n_fds);
  cmsg = CMSG_FIRSTHDR(&msghdr);
  cmsg->cmsg_len = CMSG_LEN(sizeof(int) * n_fds);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  int c = 0;
  c_FOREACH(i, cvec_fd, *fds) { ((int *)CMSG_DATA(cmsg))[c++] = *i.ref; }
  return (sendmsg(sock, &msghdr, 0) >= 0 ? 0 : -1);
}

int send_fds(int sock, cvec_fd *fds) {
  FD_BUFFER(MAX_N_FDS) buffer;

  Assert(cvec_fd_size(fds) <= MAX_N_FDS);
  return send_fds_with_buffer(sock, fds, &buffer);
}

static cvec_fd_fd recv_fds_with_buffer(int sock, void *buffer) {
  struct msghdr msghdr;
  unsigned n_fds;
  struct iovec payload_ptr;
  struct cmsghdr *cmsg;
  int i;
  int fds[sizeof(int) * MAX_N_FDS];

  payload_ptr.iov_base = fds;
  payload_ptr.iov_len = sizeof(int) * MAX_N_FDS;

  msghdr.msg_name = NULL;
  msghdr.msg_namelen = 0;
  msghdr.msg_iov = &payload_ptr;
  msghdr.msg_iovlen = 1;
  msghdr.msg_flags = 0;
  msghdr.msg_control = buffer;
  msghdr.msg_controllen = CMSG_SPACE(sizeof(int) * MAX_N_FDS);

  cvec_fd_fd result = cvec_fd_fd_with_capacity(MAX_N_FDS);
  if (recvmsg(sock, &msghdr, 0) < 0)
    return result;

  cmsg = CMSG_FIRSTHDR(&msghdr);
  if (cmsg == NULL)
    return result;
  n_fds = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);

  for (i = 0; i < n_fds; i++) {
    cvec_fd_fd_push(&result, (fd_fd){.fd = ((int *)CMSG_DATA(cmsg))[i], .master_fd = fds[i]});
  }

  return result;
}

cvec_fd_fd recv_fds(int sock) {
  FD_BUFFER(MAX_N_FDS) buffer;

  return recv_fds_with_buffer(sock, &buffer);
}
