static char *greeting = "a;b";

char *moved(void) {
    return greeting;
}

int main(void) {
    return moved() != 0;
}
