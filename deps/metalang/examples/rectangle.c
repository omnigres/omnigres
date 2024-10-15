// Computes the area of a rectangle.

#include <metalang99.h>

#define rect(width, height) ML99_tuple(width, height)
#define rectWidth           ML99_tupleGet(0)
#define rectHeight          ML99_tupleGet(1)

#define rectArea(rect) ML99_mul(rectWidth(rect), rectHeight(rect))

/*
 *                15
 * +------------------------------+
 * |                              |
 * |                              |
 * |                              | 7
 * |                              |
 * |                              |
 * +------------------------------+
 */
#define RECTANGLE rect(v(15), v(7))

ML99_ASSERT_EQ(rectArea(RECTANGLE), v(15 * 7));

int main(void) {}
