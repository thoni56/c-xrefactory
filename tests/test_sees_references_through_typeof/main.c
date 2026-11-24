struct point {
    int x;
    int y;
};

int yOf(struct point *p) {
    typeof(*p) *q = p;
    return q->y;
}

static struct point origin;

int xOfACopy(void) {
    __typeof__(origin) copy = {0};
    return copy.x;
}
