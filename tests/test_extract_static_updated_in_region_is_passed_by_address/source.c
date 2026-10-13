int compute(int n) {
    int total;
    static int counter = 0;
    counter = counter + n;
    total = counter;
    return total;
}
