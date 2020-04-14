#include <stdio.h>

int main(int argc, char **argv) {
    char str[50];
    struct s {int a; float f; char c; char *p;} s;
    s = (struct s){.a = 3, .f = 3.14, .c = 'c', .p = str};
    sprintf(str, "Hello");
    strcat(str, "World");
}
