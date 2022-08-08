#include <stdio.h>

static void newFunction_(char str[]) {
    sprintf(str, "Hello");
    strcat(str, "World");
}

int main(int argc, char **argv) {
    char str[50];
    newFunction_(str);
}
