#include "lsp.h"

#include <string.h>
#include <stdio.h>


bool want_lsp_server(int argc, char **argv) {
    for (int i=0; i<argc; i++)
        if (strcmp(argv[i], "-lsp") == 0)
            return true;
    return false;
}

int lsp_server() {
    char buffer[8192];  // Adjust buffer size as necessary
    if (fgets(buffer, sizeof(buffer), stdin)) {
        return 0;  // Success
    }
    return -1;  // Fail if no input is read
}
