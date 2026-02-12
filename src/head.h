#ifndef HEAD_H_INCLUDED
#define HEAD_H_INCLUDED

/* An empty "keyword" for non-public functions that should be able to unittest */
#define protected


/* ******************** language identification ****************** */

/* bitwise different */
typedef enum language {
    LANG_NONE  = 0,
    LANG_C     = 1<<1,
    LANG_YACC  = 1<<2
} Language;

/* ****************** end of line conversions ***************************** */

#define NO_EOL_CONVERSION		0
#define CR_LF_EOL_CONVERSION	1
#define CR_EOL_CONVERSION		2

/* ****************** X-files hashing method ****************************** */

#define XFILE_HASH_MAX 3

/* *********************************************************************** */

#define ANY_FILE (-1)    // must be different from any file number
#define ANY_PASS (-1)    // must be different from any pass number which are 1 and up
#define NO_PASS  0       // sentinel to skip all pass-specific options

/* *********************************************************************** */

// special link names starts by one space character
// special local link names starts by two spaces
#define LINK_NAME_UNIMPORTED_QUALIFIED_ITEM     "  unimported type"
#define LINK_NAME_SAFETY_CHECK_MISSED           "  Conflicting References"

#define LINK_NAME_SEPARATOR '!'
#define LINK_NAME_COLLATE_SYMBOL '#'
#define LINK_NAME_EXTRACT_DEFAULT_FLAG ' '

#define LINK_NAME_INCLUDE_REFS "%%%i"		// no #, netscape does not like them


/* *************************************************************** */

#define MAX_CHARS 127	/* maximal value storable in char encoded types     */
                        /* just for strings encoding fun-profiles for link  */

// !!!!!!! do not move the following into constants.h header, must be here
// because if not, YACC will put there defaults !!!!!!
#define YYSTACKSIZE 5000


/* ***************** OLCX COMMUNICATION CHARS ******************** */

#define COLCX_LIST					";"

/* *************************************************************** */

#define ERROR_MESSAGE_STARTING_OFFSET 10

/* ********************************************************************** */

#define TYPE_STR_RESERVE   100  /* reserve when sprinting type name */

#define MAXIMAL_INT             ((int) (((unsigned) -2)>>1))

/* ******************************************************************** */

#define USAGE_TOP_LEVEL_USED UsageAddrUsed	/* type name at top-level */
#define USAGE_EXTEND_USAGE UsageLvalUsed	/* type name in extend context */

/* ******************************************************************** */

#define LINK_NAME_MAYBE_START(ch) (                     \
        ch=='.' || ch=='/' || ch==LINK_NAME_SEPARATOR   \
        || ch==LINK_NAME_COLLATE_SYMBOL                 \
        || ch=='$'                                      \
    )

#define SAFETY_CHECK_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs,pbflag) {\
    refs = sessionData.browsingStack.top;\
    if (refs==NULL || refs->previous==NULL || refs->previous->previous==NULL){\
        errorMessage(ERR_INTERNAL, "something goes wrong at safety check");\
        pbflag = 1;\
    } else {\
        newrefs = refs;\
        diffrefs = refs->previous;\
        origrefs = refs->previous->previous;\
    }\
}

#define SYMBOL_MENU_FIRST_LINE 0
#define MAX_ASCII_CHAR 256


#define MARKER_TO_POINTER(marker) (marker->buffer->allocation.text+marker->offset)
#define POINTER_AFTER_MARKER(marker) (marker->buffer->allocation.text+marker->offset+1)
#define POINTER_BEFORE_MARKER(marker) (marker->buffer->allocation.text+marker->offset-1)
#define CHAR_BEFORE_MARKER(marker) (*POINTER_BEFORE_MARKER(marker))
#define CHAR_ON_MARKER(marker) (*MARKER_TO_POINTER(marker))
#define CHAR_AFTER_MARKER(marker) (*POINTER_AFTER_MARKER(marker))


/* *******************   Symbol Match Quality Filters   ******************* */

/* File matching quality levels */

#define FILE_MATCH_MASK				00700 /* mask for file matching */
#define FILE_MATCH_ANY				00000 /* any file */
#define FILE_MATCH_RELATED			00100 /* related scope */
#define FILE_MATCH_CLOSE			00300 /* close match (intermediate) */
#define FILE_MATCH_SAME				00500 /* same file */

/* Name matching quality levels */

#define NAME_MATCH_MASK				07000 /* mask for name matching */
#define NAME_MATCH_ANY				00000 /* any name */
#define NAME_MATCH_APPLICABLE		02000 /* applicable signature */
#define NAME_MATCH_EXACT			03000 /* exact name match */

/* Filter levels for symbol selection */

#define DEFAULT_SELECTION_FILTER (FILE_MATCH_CLOSE | NAME_MATCH_APPLICABLE)
#define RENAME_SELECTION_FILTER (FILE_MATCH_RELATED | NAME_MATCH_APPLICABLE)

/* *********************************************************************** */

#define MAP_FUN_SIGNATURE char *file, char *a1, char *a2, Completions *a3, void *a4, int *a5

/* *********************************************************************** */

#define ENTER() {log_trace("Entering: %s", __func__); log_indent();}
#define LEAVE() {log_outdent(); log_trace("Leaving: %s", __func__);}

#endif
