/*! \file */
#ifndef OMNI_HTTPC_H
#define OMNI_HTTPC_H

// clang-format off
#include <postgres.h>
// clang-format on

void omni_httpc_h2o_fatal(const char *msg, ...) {
  size_t size = 1024;
  char *buf = (char *)palloc(size);
  va_list args;

  va_start(args, msg);
  vsnprintf(buf, size, msg, args);
  va_end(args);

  ereport(ERROR, errmsg("http failure: %s", buf));
}

#ifdef h2o_fatal
#error "h2o_fatal already defined"
#else
#define h2o_fatal(...) omni_httpc_h2o_fatal(__VA_ARGS__)
#endif
#endif //  OMNI_HTTPC_H