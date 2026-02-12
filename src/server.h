#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <stdbool.h>

#include "enums.h"
#include "argumentsvector.h"


/* ************** on-line (browsing) operations for c-xref server  ********** */
#define ALL_OPERATION_ENUMS(ENUM)               \
    ENUM(OP_NONE)                              \
    ENUM(OP_ABOUT)                                      \
    ENUM(OP_ACTIVE_PROJECT)                             \
    ENUM(OP_BROWSE_GOTO_N)                              \
    ENUM(OP_BROWSE_NEXT)                                \
    ENUM(OP_BROWSE_POP)                                 \
    ENUM(OP_BROWSE_POP_ONLY)                            \
    ENUM(OP_BROWSE_PREVIOUS)                            \
    ENUM(OP_BROWSE_PUSH)                                \
    ENUM(OP_BROWSE_PUSH_AND_CALL_MACRO)                 \
    ENUM(OP_BROWSE_PUSH_NAME)                           \
    ENUM(OP_BROWSE_PUSH_ONLY)                           \
    ENUM(OP_BROWSE_REPUSH)                              \
    ENUM(OP_COMPLETION)                                 \
    ENUM(OP_COMPLETION_GOTO_N)                          \
    ENUM(OP_COMPLETION_NEXT)                            \
    ENUM(OP_COMPLETION_PREVIOUS)                        \
    ENUM(OP_COMPLETION_SELECT)                          \
    ENUM(OP_FILTER_MINUS)                               \
    ENUM(OP_FILTER_PLUS)                                \
    ENUM(OP_FILTER_SET)                                 \
    ENUM(OP_GET_AVAILABLE_REFACTORINGS)                 \
    ENUM(OP_GET_ENV_VALUE)                              \
    ENUM(OP_INTERNAL_GET_FUNCTION_BOUNDS)               \
    ENUM(OP_INTERNAL_LIST)                              \
    ENUM(OP_INTERNAL_PARSE_TO_EXTRACT)                  \
    ENUM(OP_INTERNAL_PARSE_TO_GET_PARAM_COORDINATES)    \
    ENUM(OP_INTERNAL_PARSE_TO_GOTO_PARAM_NAME)          \
    ENUM(OP_INTERNAL_PARSE_TO_SET_MOVE_TARGET)          \
    ENUM(OP_INTERNAL_PUSH_FOR_ARGUMENT_MANIPULATION)    \
    ENUM(OP_INTERNAL_PUSH_FOR_USAGE_CHECK)             \
    ENUM(OP_INTERNAL_PUSH_FOR_RENAME)                   \
    ENUM(OP_INTERNAL_SAFETY_CHECK)                      \
    ENUM(OP_MENU_FILTER_MINUS)                          \
    ENUM(OP_MENU_FILTER_PLUS)                           \
    ENUM(OP_MENU_FILTER_SET)                            \
    ENUM(OP_MENU_SELECT_ALL)                            \
    ENUM(OP_MENU_SELECT_NONE)                           \
    ENUM(OP_MENU_SELECT_ONLY)                           \
    ENUM(OP_MENU_TOGGLE_SELECT)                         \
    ENUM(OP_ORGANIZE_INCLUDES)                          \
    ENUM(OP_SEARCH)                                     \
    ENUM(OP_SEARCH_GOTO_N)                              \
    ENUM(OP_SEARCH_NEXT)                                \
    ENUM(OP_SEARCH_PREVIOUS)                            \
    ENUM(OP_SEARCH_SELECT)                              \
    ENUM(OP_UNUSED_GLOBAL)                              \
    ENUM(OP_UNUSED_LOCAL)                               \


#if OP_NONE != 0
#error OP_NONE != 0
#endif

typedef enum {
       ALL_OPERATION_ENUMS(GENERATE_ENUM_VALUE)
} ServerOperation;

extern const char* operationNamesTable[];

extern void initServer(ArgumentsVector args);
extern void callServer(ArgumentsVector args, ArgumentsVector nargs);
extern void server(ArgumentsVector args);

#endif
