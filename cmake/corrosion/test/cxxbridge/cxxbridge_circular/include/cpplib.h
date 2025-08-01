#pragma once
#include "cxxbridge/lib.h"

::RsImage read_image(::rust::Str path);

void assert_equality();
