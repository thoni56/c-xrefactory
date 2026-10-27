static int helper(void) {
    return 1;
}

int moved(void) {
    return helper();
}

int main(void) {
    return moved() + helper();
}
