#ifndef ML99_H
#define ML99_H

#if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
#error Please, specify /Zc:preprocessor to enable a standard-compliant C99/C++11 preprocessor.
#endif

#include <metalang99/assert.h>
#include <metalang99/bool.h>
#include <metalang99/choice.h>
#include <metalang99/either.h>
#include <metalang99/gen.h>
#include <metalang99/ident.h>
#include <metalang99/lang.h>
#include <metalang99/list.h>
#include <metalang99/maybe.h>
#include <metalang99/nat.h>
#include <metalang99/seq.h>
#include <metalang99/stmt.h>
#include <metalang99/tuple.h>
#include <metalang99/util.h>
#include <metalang99/variadics.h>

#define ML99_MAJOR 1
#define ML99_MINOR 13
#define ML99_PATCH 2

#define ML99_VERSION_COMPATIBLE(x, y, z)                                                           \
    (ML99_MAJOR == (x) && ((ML99_MINOR == (y) && ML99_PATCH >= (z)) || (ML99_MINOR > (y))))

#define ML99_VERSION_EQ(x, y, z) (ML99_MAJOR == (x) && ML99_MINOR == (y) && ML99_PATCH == (z))

#endif // ML99_H
