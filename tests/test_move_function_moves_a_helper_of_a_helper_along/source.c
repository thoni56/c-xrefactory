static int inner(void) {
    return 1;
}

static int helper(void) {
    return inner();
}

int moved(void) {
    return helper();
}

int main(void) {
    return moved();
}
