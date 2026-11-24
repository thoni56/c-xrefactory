struct point {
    int x;
    int y;
};

unsigned long offsetOfY(void) {
    return offsetof(struct point, y);
}

unsigned long offsetOfX(void) {
    return __builtin_offsetof(struct point, x);
}
