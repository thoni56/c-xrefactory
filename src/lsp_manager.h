#ifndef LSP_H_INCLUDED
#define LSP_H_INCLUDED

#include <stdio.h>

// Return 1 if successfully initialized, 0 otherwise
int lsp_handle_initialize(const char* input);
// Return 1 if message was valid JSON, 0 otherwise
int lsp_receive_message(char *buffer, size_t size);

#endif
