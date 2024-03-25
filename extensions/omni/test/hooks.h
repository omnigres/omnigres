#ifndef TEST_HOOKS_H
#define TEST_HOOKS_H

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

void xact_callback(omni_hook_handle *handle, XactEvent event);
void planner(omni_hook_handle *handle, Query *parse, const char *query_string, int cursorOptions,
             ParamListInfo boundParams);
#endif // TEST_HOOKS_H
