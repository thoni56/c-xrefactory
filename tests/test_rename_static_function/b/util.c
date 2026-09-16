static int helper(void) {
    return 1;
}

int use_in_b(void) {
    return helper();
}
