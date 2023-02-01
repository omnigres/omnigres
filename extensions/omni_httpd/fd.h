/*! \file */

#ifndef OMNI_HTTPD_FD_H
#define OMNI_HTTPD_FD_H

#include <libgluepg_stc.h>

#define i_key int
#define i_tag fd
#include <stc/cvec.h>

#ifndef SCM_MAX_FD
#define SCM_MAX_FD 253
#endif

/**
 * Maximum number of fds that can be sent or received
 */
#define MAX_N_FDS SCM_MAX_FD

int send_fds(int sock, cvec_fd *fds);
cvec_fd recv_fds(int sock);

#endif // OMNI_HTTPD_FD_H