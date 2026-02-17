#include <cppgres.hpp>

extern "C" {
PG_MODULE_MAGIC;

#include <omni/omni_v0.h>
OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni_signal", .version = EXT_VERSION,
                 .identity = "7521046f-35ee-4f96-81f5-38785a37aba8");
}
