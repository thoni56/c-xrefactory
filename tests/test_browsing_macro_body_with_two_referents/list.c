#include "node.h"

struct list_node {
    int value;
    struct list_node *next;
};

void walk_list(struct list_node *n) {
    n = NEXT(n);
}
