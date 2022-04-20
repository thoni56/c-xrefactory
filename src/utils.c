#include "utils.h"

#include "globals.h"
#include "options.h"
#include "fileio.h"
#include "server.h"


bool creatingOlcxRefs(void) {
    /* TODO: what does this actually test? that we need to create refs?  */
    return (
            options.serverOperation==OLO_PUSH
            ||  options.serverOperation==OLO_PUSH_ONLY
            ||  options.serverOperation==OLO_PUSH_AND_CALL_MACRO
            ||  options.serverOperation==OLO_GOTO_PARAM_NAME
            ||  options.serverOperation==OLO_GET_PARAM_COORDINATES
            ||  options.serverOperation==OLO_GET_AVAILABLE_REFACTORINGS
            ||  options.serverOperation==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK
            ||  options.serverOperation==OLO_PUSH_NAME
            ||  options.serverOperation==OLO_PUSH_SPECIAL_NAME
            ||  options.serverOperation==OLO_PUSH_ALL_IN_METHOD
            ||  options.serverOperation==OLO_PUSH_FOR_LOCALM
            ||  options.serverOperation==OLO_TRIVIAL_PRECHECK
            ||  options.serverOperation==OLO_GET_SYMBOL_TYPE
            ||  options.serverOperation==OLO_GET_LAST_IMPORT_LINE
            ||  options.serverOperation==OLO_GLOBAL_UNUSED
            ||  options.serverOperation==OLO_LOCAL_UNUSED
            ||  options.serverOperation==OLO_LIST
            ||  options.serverOperation==OLO_RENAME
            ||  options.serverOperation==OLO_ENCAPSULATE
            ||  options.serverOperation==OLO_ARG_MANIP
            ||  options.serverOperation==OLO_VIRTUAL2STATIC_PUSH
            //&     ||  options.serverOperation==OLO_SAFETY_CHECK1
            ||  options.serverOperation==OLO_SAFETY_CHECK2
            ||  options.serverOperation==OLO_CLASS_TREE
            ||  options.serverOperation==OLO_SYNTAX_PASS_ONLY
            ||  options.serverOperation==OLO_GET_PRIMARY_START
            ||  options.serverOperation==OLO_USELESS_LONG_NAME
            ||  options.serverOperation==OLO_USELESS_LONG_NAME_IN_CLASS
            ||  options.serverOperation==OLO_MAYBE_THIS
            ||  options.serverOperation==OLO_NOT_FQT_REFS
            ||  options.serverOperation==OLO_NOT_FQT_REFS_IN_CLASS
            );
}
