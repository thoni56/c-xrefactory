#include "utils.h"

#include "globals.h"


int creatingOlcxRefs(void) {
    return (
            s_opt.cxrefs==OLO_PUSH
            ||  s_opt.cxrefs==OLO_PUSH_ONLY
            ||  s_opt.cxrefs==OLO_PUSH_AND_CALL_MACRO
            ||  s_opt.cxrefs==OLO_GOTO_PARAM_NAME
            ||  s_opt.cxrefs==OLO_GET_PARAM_COORDINATES
            ||  s_opt.cxrefs==OLO_GET_AVAILABLE_REFACTORINGS
            ||  s_opt.cxrefs==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK
            ||  s_opt.cxrefs==OLO_PUSH_NAME
            ||  s_opt.cxrefs==OLO_PUSH_SPECIAL_NAME
            ||  s_opt.cxrefs==OLO_PUSH_ALL_IN_METHOD
            ||  s_opt.cxrefs==OLO_PUSH_FOR_LOCALM
            ||  s_opt.cxrefs==OLO_TRIVIAL_PRECHECK
            ||  s_opt.cxrefs==OLO_GET_SYMBOL_TYPE
            ||  s_opt.cxrefs==OLO_GLOBAL_UNUSED
            ||  s_opt.cxrefs==OLO_LOCAL_UNUSED
            ||  s_opt.cxrefs==OLO_LIST
            ||  s_opt.cxrefs==OLO_RENAME
            ||  s_opt.cxrefs==OLO_ENCAPSULATE
            ||  s_opt.cxrefs==OLO_ARG_MANIP
            ||  s_opt.cxrefs==OLO_VIRTUAL2STATIC_PUSH
            //&     ||  s_opt.cxrefs==OLO_SAFETY_CHECK1
            ||  s_opt.cxrefs==OLO_SAFETY_CHECK2
            ||  s_opt.cxrefs==OLO_CLASS_TREE
            ||  s_opt.cxrefs==OLO_SYNTAX_PASS_ONLY
            ||  s_opt.cxrefs==OLO_GET_PRIMARY_START
            ||  s_opt.cxrefs==OLO_USELESS_LONG_NAME
            ||  s_opt.cxrefs==OLO_USELESS_LONG_NAME_IN_CLASS
            ||  s_opt.cxrefs==OLO_MAYBE_THIS
            ||  s_opt.cxrefs==OLO_NOT_FQT_REFS
            ||  s_opt.cxrefs==OLO_NOT_FQT_REFS_IN_CLASS
            );
}
