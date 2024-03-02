#ifndef TEST_HOOKS_H
#define TEST_HOOKS_H

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

void xact_callback(omni_hook_handle *handle, XactEvent event);

#endif // TEST_HOOKS_H
