#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <stdbool.h>

#include "enums.h"


/* ************** on-line (browsing) operations for c-xref server  ********** */
#define ALL_OPERATION_ENUMS(ENUM)                                       \
    ENUM(OLO_NOOP)                                                      \
        ENUM(OLO_COMPLETION)                                            \
        ENUM(OLO_SEARCH)                                                \
        ENUM(OLO_TAG_SEARCH)                                            \
        ENUM(OLO_RENAME)                                                \
        ENUM(OLO_ENCAPSULATE)                                           \
        ENUM(OLO_ARG_MANIP)                                             \
        ENUM(OLO_VIRTUAL2STATIC_PUSH)                                   \
        ENUM(OLO_GET_AVAILABLE_REFACTORINGS)                            \
        ENUM(OLO_PUSH)                                                  \
        ENUM(OLO_PUSH_NAME)                                             \
        ENUM(OLO_PUSH_SPECIAL_NAME)                                     \
        ENUM(OLO_POP)                                                   \
        ENUM(OLO_POP_ONLY)                                              \
        ENUM(OLO_NEXT)                                                  \
        ENUM(OLO_PREVIOUS)                                              \
        ENUM(OLO_GOTO_CURRENT)                                          \
        ENUM(OLO_GET_CURRENT_REFNUM)                                    \
        ENUM(OLO_GOTO_PARAM_NAME)                                       \
        ENUM(OLO_GLOBAL_UNUSED)                                         \
        ENUM(OLO_LOCAL_UNUSED)                                          \
        ENUM(OLO_LIST)                                                  \
        ENUM(OLO_LIST_TOP)                                              \
        ENUM(OLO_PUSH_ONLY)                                             \
        ENUM(OLO_PUSH_AND_CALL_MACRO)                                   \
        ENUM(OLO_PUSH_ALL_IN_METHOD)                                    \
        ENUM(OLO_PUSH_FOR_LOCALM)                                       \
        ENUM(OLO_GOTO)                                                  \
        ENUM(OLO_CGOTO)                                                 \
        ENUM(OLO_TAGGOTO)                                               \
        ENUM(OLO_TAGSELECT)                                             \
        ENUM(OLO_CBROWSE)                                               \
        ENUM(OLO_REF_FILTER_SET)                                        \
        ENUM(OLO_REF_FILTER_PLUS)                                       \
        ENUM(OLO_REF_FILTER_MINUS)                                      \
        ENUM(OLO_CSELECT)                                               \
        ENUM(OLO_COMPLETION_BACK)                                       \
        ENUM(OLO_COMPLETION_FORWARD)                                    \
        ENUM(OLO_EXTRACT)                                               \
        ENUM(OLO_CT_INSPECT_DEF)                                        \
        ENUM(OLO_MENU_INSPECT_DEF)                                      \
        ENUM(OLO_MENU_INSPECT_CLASS)                                    \
        ENUM(OLO_MENU_SELECT)                                           \
        ENUM(OLO_MENU_SELECT_ONLY)                                      \
        ENUM(OLO_MENU_SELECT_ALL)                                       \
        ENUM(OLO_MENU_SELECT_NONE)                                      \
        ENUM(OLO_MENU_FILTER_SET)                                       \
        ENUM(OLO_MENU_FILTER_PLUS)                                      \
        ENUM(OLO_MENU_FILTER_MINUS)                                     \
        ENUM(OLO_MENU_GO)                                               \
        ENUM(OLO_CHECK_VERSION)                                         \
        ENUM(OLO_TRIVIAL_PRECHECK)                                      \
        ENUM(OLO_MM_PRE_CHECK)                                          \
        ENUM(OLO_PP_PRE_CHECK)                                          \
        ENUM(OLO_SAFETY_CHECK_INIT)                                     \
        ENUM(OLO_SAFETY_CHECK1)                                         \
        ENUM(OLO_SAFETY_CHECK2)                                         \
        ENUM(OLO_INTERSECTION)                                          \
        ENUM(OLO_REMOVE_WIN)                                            \
        ENUM(OLO_GOTO_DEF)                                              \
        ENUM(OLO_GOTO_CALLER)                                           \
        ENUM(OLO_SHOW_TOP)                                              \
        ENUM(OLO_SHOW_TOP_APPL_CLASS)                                   \
        ENUM(OLO_SHOW_TOP_TYPE)                                         \
        ENUM(OLO_SHOW_CLASS_TREE)                                       \
        ENUM(OLO_ACTIVE_PROJECT)                                        \
        ENUM(OLO_JAVA_HOME)                                             \
        ENUM(OLO_REPUSH)                                                \
        ENUM(OLO_CLASS_TREE)                                            \
        ENUM(OLO_USELESS_LONG_NAME)                                     \
        ENUM(OLO_USELESS_LONG_NAME_IN_CLASS)                            \
        ENUM(OLO_MAYBE_THIS)                                            \
        ENUM(OLO_NOT_FQT_REFS)                                          \
        ENUM(OLO_NOT_FQT_REFS_IN_CLASS)                                 \
        ENUM(OLO_GET_ENV_VALUE)                                         \
        ENUM(OLO_SET_MOVE_TARGET)                                       \
        ENUM(OLO_SET_MOVE_CLASS_TARGET)                                 \
        ENUM(OLO_SET_MOVE_METHOD_TARGET)                                \
        ENUM(OLO_GET_CURRENT_CLASS)                                     \
        ENUM(OLO_GET_CURRENT_SUPER_CLASS)                               \
        ENUM(OLO_GET_METHOD_COORD)                                      \
        ENUM(OLO_GET_CLASS_COORD)                                       \
        ENUM(OLO_GET_SYMBOL_TYPE)                                       \
        ENUM(OLO_GET_LAST_IMPORT_LINE)                                  \
        ENUM(OLO_TAG_SEARCH_FORWARD)                                    \
        ENUM(OLO_TAG_SEARCH_BACK)                                       \
        ENUM(OLO_PUSH_ENCAPSULATE_SAFETY_CHECK)                         \
        ENUM(OLO_ENCAPSULATE_SAFETY_CHECK)                              \
        ENUM(OLO_SYNTAX_PASS_ONLY)                                      \
        ENUM(OLO_GET_PRIMARY_START)                                     \
        ENUM(OLO_GET_PARAM_COORDINATES)                                 \
        ENUM(OLO_ABOUT)                                                 \


#if 0
   Here are comments about some of the operation enums:
       OLO_RENAME,              - same as push, just another ordering
       OLO_ENCAPSULATE,         - same as rename, remove private references
       OLO_ARG_MANIP,           - as rename, constructors resolved as functions
       OLO_VIRTUAL2STATIC_PUSH, - same as rename, another message on virtuals
       OLO_PUSH_SPECIAL_NAME,	- also reparsing current file
       OLO_CGOTO,               - goto completion item definition
       OLO_TAGGOTO,             - goto tag search result
       OLO_TAGSELECT,           - select tag search result
       OLO_CBROWSE,             - browse javadoc of completion item
       OLO_CSELECT,             - select completion
       OLO_EXTRACT,             - extract block into separate function
       OLO_CT_INSPECT_DEF,		- inspect definition from class tree
       OLO_MENU_INSPECT_DEF,	- inspect definition from symbol menu
       OLO_MENU_INSPECT_CLASS,	- inspect class from symbol menu
       OLO_MENU_SELECT,         - select the line from symbol menu
       OLO_MENU_SELECT_ONLY,	- select only the line from symbol menu
       OLO_MENU_SELECT_ALL,     - select all from symbol menu
       OLO_MENU_SELECT_NONE,	- select none from symbol menu
       OLO_MENU_FILTER_SET,     - more strong filtering
       OLO_MENU_FILTER_PLUS,	- more strong filtering
       OLO_MENU_FILTER_MINUS,	- smaller filtering
       OLO_MENU_GO,             - push references from selected menu items
       OLO_CHECK_VERSION,       - check version correspondance
       OLO_TRIVIAL_PRECHECK,	- trivial pre-refactoring checks
       OLO_MM_PRE_CHECK,		- move method pre check
       OLO_PP_PRE_CHECK,		- push-down/pull-up method pre check
       OLO_INTERSECTION,        - just provide intersection of top references
       OLO_REMOVE_WIN,          - just remove window of top references
       OLO_GOTO_DEF,            - goto definition reference
       OLO_GOTO_CALLER,         - goto caller reference
       OLO_SHOW_TOP,            - show top symbol
       OLO_SHOW_TOP_APPL_CLASS, - show current reference appl class
       OLO_SHOW_TOP_TYPE,       - show current symbol type
       OLO_SHOW_CLASS_TREE,     - show current class tree
       OLO_TOP_SYMBOL_RES,      - show top symbols resolution
       OLO_ACTIVE_PROJECT,      - show active project name
       OLO_JAVA_HOME,           - show inferred jdkclasspath
       OLO_REPUSH,              - re-push pop-ed top
       OLO_CLASS_TREE,          - display class tree
       OLO_USELESS_LONG_NAME,   - display useless long class names
       OLO_USELESS_LONG_NAME_IN_CLASS, - display useless long class names
       OLO_MAYBE_THIS,          - display 'this' class dependencies
       OLO_NOT_FQT_REFS,        - display not fully qualified names in method
       OLO_NOT_FQT_REFS_IN_CLASS, - display not fully qualified names of class
       OLO_GET_ENV_VALUE,       - get a value set by -set
       OLO_SET_MOVE_TARGET,     - set target place for moving action
       OLO_SET_MOVE_CLASS_TARGET,	- set target place for xref2 move class
       OLO_SET_MOVE_METHOD_TARGET,	- set target place for xref2 move method
       OLO_GET_METHOD_COORD,	- get method beginning and end lines
       OLO_GET_CLASS_COORD,     - get class beginning and end lines
       OLO_GET_SYMBOL_TYPE,     - get type of a symbol
       OLO_SYNTAX_PASS_ONLY,    - should replace OLO_GET_PRIMARY_START && OLO_GET_PARAM_COORDINATES
       OLO_GET_PRIMARY_START,    - get start position of primary expression
#endif

#if OLO_NOP != 0
#error OLO_NOP != 0
#endif

typedef enum {
    ALL_OPERATION_ENUMS(GENERATE_ENUM_VALUE)
} ServerOperation;

extern const char* operationNamesTable[];

extern void initServer(int nargc, char **nargv);
extern void callServer(int argc, char **argv, int nargc, char **nargv, bool *firstPass);
extern void server(int argc, char **argv);

#endif
