struct Point { int x, y; };

int process(void) {
    struct Point p = {10, 20};
    int sum;
    sum = p.x + p.y;
    return sum;
}
