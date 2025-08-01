#include <stdint.h>
#include <assert.h>
#include <iostream>

extern "C" uint64_t add(uint64_t a, uint64_t b);

int main(int argc, char **argv) {
    uint64_t sum = add(5, 6);
    assert(sum == 11);
    std::cout << "The sum is " << sum << std::endl;
}
