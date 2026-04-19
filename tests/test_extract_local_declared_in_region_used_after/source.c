#include <stdio.h>

void foo(char *text, int textSize) {
    // region begin
    int pos = 0;
    while (pos < textSize && (text[pos] == ' ' || text[pos] == '\t'))
        pos++;
    // region end
    printf("%d\n", pos);
}
