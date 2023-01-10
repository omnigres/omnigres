#ifndef OMNI_EXT_STRVERSCP_H
#define OMNI_EXT_STRVERSCP_H

#include <ctype.h>
#include <string.h>

#ifndef HAVE_STRVERSCMP
int strverscmp(const char *l0, const char *r0);
#endif

#endif // OMNI_EXT_STRVERSCP_H