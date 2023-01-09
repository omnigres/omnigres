// clang-format off
#include <postgres.h>
#include <utils/memutils.h>
#include <utils/guc.h>
// clang-format on

#include <libgluepg_curl.h>

#include "omni_containers.h"

DYNPGEXT_MAGIC;

void _Dynpgext_init(const dynpgext_handle *handle) {}