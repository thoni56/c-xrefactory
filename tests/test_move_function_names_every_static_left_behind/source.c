static int count;

static int helper(void) {
    return 1;
}

int moved(void) {
    return helper() + count;
}

int main(void) {
    return moved();
}
