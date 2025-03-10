#ifndef LSP_ERRORS_H_INCLUDED
#define LSP_ERRORS_H_INCLUDED


typedef enum {
    LSP_RETURN_OK,
    LSP_RETURN_EXIT,
    LSP_RETURN_ERROR_HEADER_INCOMPLETE,
    LSP_RETURN_ERROR_OUT_OF_MEMORY,
    LSP_RETURN_ERROR_IO_ERROR,
    LSP_RETURN_ERROR_JSON_PARSE_ERROR,
    LSP_RETURN_ERROR_METHOD_NOT_FOUND
} LspReturnCode;


#endif
