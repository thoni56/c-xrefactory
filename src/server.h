#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <stdbool.h>

#include "enums.h"
#include "argumentsvector.h"


/* ************** on-line (browsing) operations for c-xref server  ********** */
// NOOP *must* be 0!
#define ALL_OPERATION_ENUMS(ENUM)               \
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
    ENUM(OLO_GET_CURRENT_REFNUM)                \
    ENUM(OLO_GET_ENV_VALUE)                     \
    ENUM(OLO_GET_LAST_IMPORT_LINE)              \
    ENUM(OLO_GET_METHOD_COORD)                  \
    ENUM(OLO_GET_PARAM_COORDINATES)             \
    ENUM(OLO_GLOBAL_UNUSED)                     \
    ENUM(OLO_GOTO)                              \
    ENUM(OLO_GOTO_CALLER)                       \
    ENUM(OLO_GOTO_CURRENT)                      \
    ENUM(OLO_GOTO_DEF)                          \
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
    ENUM(OLO_NOOP)                              \
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


#if 0
Here are some original comments about some of the operation enums (not all of them makes any sense...)
       OLO_RENAME,              - same as push, just another ordering
       OLO_ARG_MANIP,           - as rename, constructors resolved as functions
       OLO_PUSH_SPECIAL_NAME,	- also reparsing current file
       OLO_CGOTO,               - goto completion item definition
       OLO_TAGGOTO,             - goto tag search result
       OLO_TAGSELECT,           - select tag search result
       OLO_EXTRACT,             - extract block into separate function
       OLO_MENU_SELECT,         - select/toggle the line from symbol menu
       OLO_MENU_SELECT_THIS_AND_GOTO_DEFINITION,	- select only the line from symbol menu
       OLO_MENU_SELECT_ALL,     - select all from symbol menu
       OLO_MENU_SELECT_NONE,	- select none from symbol menu
       OLO_MENU_FILTER_SET,     - set filter level
       OLO_MENU_FILTER_PLUS,	- stronger filtering
       OLO_MENU_FILTER_MINUS,	- weaker filtering
       OLO_MENU_GO,             - push references from selected menu items
       OLO_CHECK_VERSION,       - check version correspondance
       OLO_REMOVE_WIN,          - just remove window of top references
       OLO_GOTO_DEF,            - goto definition reference
       OLO_GOTO_CALLER,         - goto caller reference
       OLO_SHOW_TOP,            - show top symbol
       OLO_SHOW_TOP_TYPE,       - show current symbol type
       OLO_TOP_SYMBOL_RES,      - show top symbols resolution
       OLO_ACTIVE_PROJECT,      - show active project name
       OLO_REPUSH,              - re-push pop-ed top
       OLO_GET_ENV_VALUE,       - get a value set by -set
       OLO_GET_METHOD_COORD,	- get method beginning and end lines
       OLO_GET_SYMBOL_TYPE,     - get type of a symbol
       OLO_SYNTAX_PASS_ONLY,    - should replace OLO_GET_PRIMARY_START && OLO_GET_PARAM_COORDINATES
       OLO_GET_PRIMARY_START,    - get start position of primary expression
#endif

#if OLO_NOP != 0
#error OLO_NOP != 0
#endif

typedef enum {
       OLO_NONE = 0,
       ALL_OPERATION_ENUMS(GENERATE_ENUM_VALUE)
} ServerOperation;

extern const char* operationNamesTable[];

extern void initServer(ArgumentsVector args);
extern void callServer(ArgumentsVector args, ArgumentsVector nargs, bool *firstPass);
extern void server(ArgumentsVector args);

#endif
