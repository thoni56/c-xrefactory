#ifndef LSP_H_INCLUDED
#define LSP_H_INCLUDED

#include <stdbool.h>


extern bool want_lsp_server(int argc, char *argv[]);
extern int lsp_server(void);

#endif
