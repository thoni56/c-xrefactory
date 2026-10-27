#include <stdio.h>

void foo(int n) {
    // region begin
    char message[100];
    sprintf(message, "%d", n);
    // region end
    puts(message);
}
