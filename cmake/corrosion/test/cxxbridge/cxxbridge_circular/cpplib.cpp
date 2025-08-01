#include "cpplib.h"
#include "cxxbridge/lib.h"
#include "rust/cxx.h"
#include <iostream>

RsImage read_image(rust::Str path) {
  std::cout << "read_image called" << std::endl;
  std::cout << path << std::endl;
  Rgba c = {1.0, 2.0, 3.0, 4.0};
  RsImage v = {1, 1, c};
  return v;
}

void assert_equality() {
  if (!read_image("dummy_path").equal_to("dummy path")) {
    throw std::runtime_error("equality_check failed");
  }
}
