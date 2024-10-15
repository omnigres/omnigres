/**
 * Duff's device [1] is a technique to implement loop unrolling through amalgamation of a switch
 * statement with a do-while loop.
 *
 * In this example, we are going to implement automatic generation of Duff's device.
 *
 * [1]: https://en.wikipedia.org/wiki/Duff's_device
 */

#include <metalang99.h>

#include <assert.h>

#define DUFFS_DEVICE(unrolling_factor, counter_ty, count, ...)                                     \
    do {                                                                                           \
        if ((count) > 0) {                                                                         \
            counter_ty DUFFS_DEVICE_n = ((count) + ML99_DEC(unrolling_factor)) / unrolling_factor; \
            switch ((count) % unrolling_factor) {                                                  \
            case 0:                                                                                \
                do {                                                                               \
                    __VA_ARGS__                                                                    \
                    ML99_EVAL(ML99_callUneval(genCases, ML99_DEC(unrolling_factor), __VA_ARGS__))  \
                } while (--DUFFS_DEVICE_n > 0);                                                    \
            }                                                                                      \
        }                                                                                          \
    } while (0)

#define genCases_IMPL(i, ...)                                                                      \
    ML99_IF(                                                                                       \
        ML99_NAT_EQ(i, 0),                                                                         \
        ML99_empty(),                                                                              \
        ML99_TERMS(                                                                                \
            v(/* FALLTHROUGH */ case i                                                             \
              : __VA_ARGS__),                                                                      \
            ML99_callUneval(genCases, ML99_DEC(i), __VA_ARGS__)))

int main(void) {
#define ARRAY_LEN        50
#define UNROLLING_FACTOR 3

    int array[ARRAY_LEN] = { ML99_EVAL(ML99_times(v(ARRAY_LEN), v(5, ))) };
    int *n_ptr = array;

    // Square all the elements in the array.
    DUFFS_DEVICE(UNROLLING_FACTOR, int, ARRAY_LEN, {
        *n_ptr *= *n_ptr;
        n_ptr++;
    });

    for (int i = 0; i < ARRAY_LEN; i++) {
        assert(25 == array[i]);
    }
}

/*
The generated Duff's device:

int DUFFS_DEVICE_n = ((50) + 2) / 3;
switch ((50) % 3) {
case 0:
    do {
        {
            *n_ptr *= *n_ptr;
            n_ptr++;
        }
    case 2: {
        *n_ptr *= *n_ptr;
        n_ptr++;
    }
    case 1: {
        *n_ptr *= *n_ptr;
        n_ptr++;
    }
    } while (--DUFFS_DEVICE_n > 0);
}
*/
