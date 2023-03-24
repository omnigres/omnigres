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

Oid http_method_oid();
Oid http_response_oid();
Oid http_header_oid();

Oid http_header_array_oid();

int create_listening_socket(sa_family_t family, in_port_t port, char *address, in_port_t *out_port);

#define MAX_ADDRESS_SIZE sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255/128")

extern int num_http_workers;

static const char *OMNI_HTTPD_CONFIGURATION_NOTIFY_CHANNEL = "omni_httpd_configuration";

static const char *OMNI_HTTPD_CONFIGURATION_RELOAD_SEMAPHORE =
    "omni_httpd:" EXT_VERSION ":_configuration_reload_semaphore";

#define HTTP_RESPONSE_TUPLE_BODY 0
#define HTTP_RESPONSE_TUPLE_STATUS 1
#define HTTP_RESPONSE_TUPLE_HEADERS 2

#endif //  OMNI_HTTPD_H