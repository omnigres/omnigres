/*! \file */
#ifndef OMNI_HTTPD_H
#define OMNI_HTTPD_H

#include <netinet/in.h>
#include <sys/socket.h>

#include <libgluepg_stc.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <omni/omni_v0.h>

Oid http_method_oid();
Oid http_request_oid();
Oid http_response_oid();

Oid http_outcome_oid();

Oid http_header_oid();

Oid http_header_array_oid();

Oid urlpattern_oid();
Oid route_priority_oid();

int create_listening_socket(sa_family_t family, in_port_t port, char *address, in_port_t *out_port);

#define MAX_ADDRESS_SIZE sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255/128")

extern int *num_http_workers;

extern char **temp_dir;

static const char *OMNI_HTTPD_CONFIGURATION_NOTIFY_CHANNEL = "omni_httpd_configuration";

static const char *OMNI_HTTPD_CONFIGURATION_RELOAD_SEMAPHORE =
    "omni_httpd(%d):" EXT_VERSION ":_configuration_reload_semaphore";

static const char *OMNI_HTTPD_CONFIGURATION_CONTROL = "omni_httpd(%d):" EXT_VERSION ":_control";

static const char *OMNI_HTTPD_MASTER_WORKER = "omni_httpd(%s):" EXT_VERSION ":_master_worker";

static const char *OMNI_HTTPD_GUC_NUM_HTTP_WORKERS =
    "omni_httpd:" EXT_VERSION ":_guc_num_http_workers";

#define HTTP_RESPONSE_TUPLE_BODY 0
#define HTTP_RESPONSE_TUPLE_STATUS 1
#define HTTP_RESPONSE_TUPLE_HEADERS 2

#define HTTP_PROXY_TUPLE_URL 0
#define HTTP_PROXY_TUPLE_PRESERVE_HOST 1

/**
 * Indicates whether this process is `master_worker`
 */
extern bool IsOmniHttpdWorker;

extern pg_atomic_uint32 *semaphore;
extern omni_bgworker_handle *master_worker_bgw;
extern pg_atomic_uint64 *master_worker_leader;

extern volatile bool BackendInitialized;

#endif //  OMNI_HTTPD_H