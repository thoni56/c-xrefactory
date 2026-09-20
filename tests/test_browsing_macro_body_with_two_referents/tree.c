#include "node.h"

struct tree_node {
    struct tree_node *child;
    struct tree_node *next;
};

void walk_tree(struct tree_node *n) {
    n = NEXT(n);
}
