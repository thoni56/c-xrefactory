#ifndef LSP_H_INCLUDED
#define LSP_H_INCLUDED

#include <stdio.h>
#include <stdbool.h>

#include "argumentsvector.h"


extern bool want_lsp_server(ArgumentsVector args);
extern int lsp_server(FILE *input);

#endif
