#include <stdio.h>

void foo(char *text, int textSize) {
    // region begin
    int spaces = 0, tabs = 0;
    for (int pos = 0; pos < textSize; pos++) {
        if (text[pos] == ' ')
            spaces++;
        if (text[pos] == '\t')
            tabs++;
    }
    // region end
    printf("%d %d\n", spaces, tabs);
}
