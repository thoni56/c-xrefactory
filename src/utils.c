#include "utils.h"

#include "globals.h"


int creatingOlcxRefs(void) {
    /* TODO: what does this actually test? that we need to create
       refs?  And why does this not work when we introduce a OLO_NONE
       before OLO_COMPLETE? */
    return (
            s_opt.server_operation==OLO_PUSH
            ||  s_opt.server_operation==OLO_PUSH_ONLY
            ||  s_opt.server_operation==OLO_PUSH_AND_CALL_MACRO
            ||  s_opt.server_operation==OLO_GOTO_PARAM_NAME
            ||  s_opt.server_operation==OLO_GET_PARAM_COORDINATES
            ||  s_opt.server_operation==OLO_GET_AVAILABLE_REFACTORINGS
            ||  s_opt.server_operation==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK
            ||  s_opt.server_operation==OLO_PUSH_NAME
            ||  s_opt.server_operation==OLO_PUSH_SPECIAL_NAME
            ||  s_opt.server_operation==OLO_PUSH_ALL_IN_METHOD
            ||  s_opt.server_operation==OLO_PUSH_FOR_LOCALM
            ||  s_opt.server_operation==OLO_TRIVIAL_PRECHECK
            ||  s_opt.server_operation==OLO_GET_SYMBOL_TYPE
            ||  s_opt.server_operation==OLO_GLOBAL_UNUSED
            ||  s_opt.server_operation==OLO_LOCAL_UNUSED
            ||  s_opt.server_operation==OLO_LIST
            ||  s_opt.server_operation==OLO_RENAME
            ||  s_opt.server_operation==OLO_ENCAPSULATE
            ||  s_opt.server_operation==OLO_ARG_MANIP
            ||  s_opt.server_operation==OLO_VIRTUAL2STATIC_PUSH
            //&     ||  s_opt.server_operation==OLO_SAFETY_CHECK1
            ||  s_opt.server_operation==OLO_SAFETY_CHECK2
            ||  s_opt.server_operation==OLO_CLASS_TREE
            ||  s_opt.server_operation==OLO_SYNTAX_PASS_ONLY
            ||  s_opt.server_operation==OLO_GET_PRIMARY_START
            ||  s_opt.server_operation==OLO_USELESS_LONG_NAME
            ||  s_opt.server_operation==OLO_USELESS_LONG_NAME_IN_CLASS
            ||  s_opt.server_operation==OLO_MAYBE_THIS
            ||  s_opt.server_operation==OLO_NOT_FQT_REFS
            ||  s_opt.server_operation==OLO_NOT_FQT_REFS_IN_CLASS
            );
}
