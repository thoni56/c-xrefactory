#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <stdbool.h>

#include "enums.h"
#include "argumentsvector.h"


/* ************** on-line (browsing) operations for c-xref server  ********** */
#define ALL_OPERATION_ENUMS(ENUM)               \
    ENUM(OLO_NONE)                              \
    ENUM(OLO_ABOUT)                             \
    ENUM(OLO_ACTIVE_PROJECT)                    \
    ENUM(OLO_ARGUMENT_MANIPULATION)             \
    ENUM(OLO_COMPLETION)                        \
    ENUM(OLO_COMPLETION_BACK)                   \
    ENUM(OLO_COMPLETION_FORWARD)                \
    ENUM(OLO_COMPLETION_GOTO)                   \
    ENUM(OLO_COMPLETION_SELECT)                 \
    ENUM(OLO_EXTRACT)                           \
    ENUM(OLO_GET_AVAILABLE_REFACTORINGS)        \
    ENUM(OLO_GET_ENV_VALUE)                     \
    ENUM(OLO_GET_METHOD_COORD)                  \
    ENUM(OLO_GET_PARAM_COORDINATES)             \
    ENUM(OLO_GLOBAL_UNUSED)                     \
    ENUM(OLO_GOTO)                              \
    ENUM(OLO_GOTO_PARAM_NAME)                   \
    ENUM(OLO_LIST)                              \
    ENUM(OLO_LOCAL_UNUSED)                      \
    ENUM(OLO_MENU_FILTER_MINUS)                 \
    ENUM(OLO_MENU_FILTER_PLUS)                  \
    ENUM(OLO_MENU_FILTER_SET)                   \
    ENUM(OLO_MENU_SELECT_THIS_AND_GOTO_DEFINITION)  \
    ENUM(OLO_MENU_SELECT_ALL)                   \
    ENUM(OLO_MENU_SELECT_NONE)                  \
    ENUM(OLO_MENU_SELECT_ONLY)                  \
    ENUM(OLO_NEXT)                              \
    ENUM(OLO_ORGANIZE_INCLUDES)                 \
    ENUM(OLO_POP)                               \
    ENUM(OLO_POP_ONLY)                          \
    ENUM(OLO_PREVIOUS)                          \
    ENUM(OLO_PUSH)                              \
    ENUM(OLO_PUSH_AND_CALL_MACRO)               \
    ENUM(OLO_PUSH_FOR_LOCAL_MOTION)             \
    ENUM(OLO_PUSH_NAME)                         \
    ENUM(OLO_PUSH_ONLY)                         \
    ENUM(OLO_REF_FILTER_MINUS)                  \
    ENUM(OLO_REF_FILTER_PLUS)                   \
    ENUM(OLO_REF_FILTER_SET)                    \
    ENUM(OLO_RENAME)                            \
    ENUM(OLO_REPUSH)                            \
    ENUM(OLO_SAFETY_CHECK)                      \
    ENUM(OLO_SET_MOVE_TARGET)                   \
    ENUM(OLO_GET_FUNCTION_BOUNDS)               \
    ENUM(OLO_TAGGOTO)                           \
    ENUM(OLO_TAGSELECT)                         \
    ENUM(OLO_TAG_SEARCH)                        \
    ENUM(OLO_TAG_SEARCH_BACK)                   \
    ENUM(OLO_TAG_SEARCH_FORWARD)                \


#if OLO_NONE != 0
#error OLO_NONE != 0
#endif

typedef enum {
       ALL_OPERATION_ENUMS(GENERATE_ENUM_VALUE)
} ServerOperation;

extern const char* operationNamesTable[];

extern void initServer(ArgumentsVector args);
extern void callServer(ArgumentsVector args, ArgumentsVector nargs);
extern void server(ArgumentsVector args);

#endif
