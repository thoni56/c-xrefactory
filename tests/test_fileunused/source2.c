#include <stdio.h>

static int unused_integer;

int main(int argc, char **argv) {
    char str[50];
    sprintf(str, "%s", argv[argc]);
}
