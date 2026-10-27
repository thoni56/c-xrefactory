static int other, count;

int moved(void) {
    return count;
}

int main(void) {
    return moved() + other;
}
