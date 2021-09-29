#include "utils.h"

#include "globals.h"
#include "options.h"
#include "fileio.h"


bool creatingOlcxRefs(void) {
    /* TODO: what does this actually test? that we need to create refs?  */
    return (
            options.server_operation==OLO_PUSH
            ||  options.server_operation==OLO_PUSH_ONLY
            ||  options.server_operation==OLO_PUSH_AND_CALL_MACRO
            ||  options.server_operation==OLO_GOTO_PARAM_NAME
            ||  options.server_operation==OLO_GET_PARAM_COORDINATES
            ||  options.server_operation==OLO_GET_AVAILABLE_REFACTORINGS
            ||  options.server_operation==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK
            ||  options.server_operation==OLO_PUSH_NAME
            ||  options.server_operation==OLO_PUSH_SPECIAL_NAME
            ||  options.server_operation==OLO_PUSH_ALL_IN_METHOD
            ||  options.server_operation==OLO_PUSH_FOR_LOCALM
            ||  options.server_operation==OLO_TRIVIAL_PRECHECK
            ||  options.server_operation==OLO_GET_SYMBOL_TYPE
            ||  options.server_operation==OLO_GLOBAL_UNUSED
            ||  options.server_operation==OLO_LOCAL_UNUSED
            ||  options.server_operation==OLO_LIST
            ||  options.server_operation==OLO_RENAME
            ||  options.server_operation==OLO_ENCAPSULATE
            ||  options.server_operation==OLO_ARG_MANIP
            ||  options.server_operation==OLO_VIRTUAL2STATIC_PUSH
            //&     ||  options.server_operation==OLO_SAFETY_CHECK1
            ||  options.server_operation==OLO_SAFETY_CHECK2
            ||  options.server_operation==OLO_CLASS_TREE
            ||  options.server_operation==OLO_SYNTAX_PASS_ONLY
            ||  options.server_operation==OLO_GET_PRIMARY_START
            ||  options.server_operation==OLO_USELESS_LONG_NAME
            ||  options.server_operation==OLO_USELESS_LONG_NAME_IN_CLASS
            ||  options.server_operation==OLO_MAYBE_THIS
            ||  options.server_operation==OLO_NOT_FQT_REFS
            ||  options.server_operation==OLO_NOT_FQT_REFS_IN_CLASS
            );
}

void recursivelyCreateFileDirIfNotExists(char *fpath) {
    char    *p;
    int     ch,len;
    struct stat  st;
    bool loopFlag = true;

    /* Check each level from the deepest, stop when it exists */
    len = strlen(fpath);
    for (p=fpath+len; p>fpath && loopFlag; p--) {
        if (*p!=FILE_PATH_SEPARATOR)
            continue;
        ch = *p; *p = 0;        /* Truncate here, remember the char */
        if (dirExists(fpath)) {
            loopFlag=false;
        }
        *p = ch;                /* Restore the char */
    }
    /* Create each of the remaining levels */
    for(p+=2; *p; p++) {
        if (*p!=FILE_PATH_SEPARATOR)
            continue;
        ch = *p; *p = 0;
        createDir(fpath);
        *p = ch;
    }
}
