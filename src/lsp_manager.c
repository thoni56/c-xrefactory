#include "lsp_manager.h"

#include <string.h>


int lsp_handle_initialize(const char* input) {
    // For now, just check if it contains the initialize method
    if (strstr(input, "\"method\":\"initialize\"") == NULL) {
        return 0;
    }
    return 1;
}

int lsp_receive_message(char* buffer, size_t size) {
    // Most basic check - does it start with a {
    if (size < 1 || buffer[0] != '{') {
        return 0;
    }
    return 1;
}
