int compute(int n) {
    static int last = 0;
    int previous = last;
    last = n;
    return previous;
}
