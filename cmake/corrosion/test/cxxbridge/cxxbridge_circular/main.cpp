#include "cpplib.h"

#include <iostream>

int main(void) {
  std::cout << "Testing roundtrip..." << std::endl;

  try {
    assert_equality();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Roundtrip successful!";
  return 0;
}
