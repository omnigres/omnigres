// Sums all nodes of a binary tree, recursively.

#include <metalang99.h>

#define leaf(x)              ML99_choice(v(leaf), x)
#define node(lhs, data, rhs) ML99_choice(v(node), lhs, data, rhs)

#define sumTree(tree)                     ML99_match(tree, v(sumTree_))
#define sumTree_leaf_IMPL(x)              v(x)
#define sumTree_node_IMPL(lhs, data, rhs) ML99_add3(sumTree(v(lhs)), v(data), sumTree(v(rhs)))

/*
 *         4
 *        / \
 *       /   \
 *      /     \
 *     2       6
 *    / \     / \
 *   1   3   5   7
 */
#define TREE node(node(leaf(v(1)), v(2), leaf(v(3))), v(4), node(leaf(v(5)), v(6), leaf(v(7))))

ML99_ASSERT_EQ(sumTree(TREE), v(28));

int main(void) {}
