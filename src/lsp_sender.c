#include "lsp_sender.h"

#include <stdio.h>
#include <string.h>

int send_response(const char *response) {
    printf("Content-Length: %zu\r\n\r\n%s\n", strlen(response), response);
    fflush(stdout);
    return 0;
}
