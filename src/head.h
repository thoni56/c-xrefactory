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

/* ****************** X-files hashing method****************************** */

#define XFILE_HASH_DEFAULT 0
#define XFILE_HASH_ALPHA1 1
#define XFILE_HASH_ALPHA2 2
#define XFILE_HASH_MAX 3

#define XFILE_HASH_ALPHA1_REFNUM ('z'-'a'+2)
#define XFILE_HASH_ALPHA2_REFNUM (XFILE_HASH_ALPHA1_REFNUM*XFILE_HASH_ALPHA1_REFNUM)

/* *********************************************************************** */


/* *********************************************************************** */

#define XREF_EXIT_BASE          64	// base for exit status
#define XREF_EXIT_ERR           65
#define XREF_EXIT_NO_PROJECT    66

/* *********************************************************************** */

#define ANY_FILE (-1)    // must be different from any file number
#define ANY_PASS (-1)    // must be different from any pass number which are 1 and up

/* *********************************************************************** */

#define CONTINUE_LABEL_NAME " %cntl%"
#define BREAK_LABEL_NAME " %brkl%"
#define SWITCH_LABEL_NAME " %swtl%"

// special link names starts by one space character
// special local link names starts by two spaces
#define LINK_NAME_UNIMPORTED_QUALIFIED_ITEM     "  unimported type"
#define LINK_NAME_FUNCTION_SEPARATOR            "  fun separator"
#define LINK_NAME_SAFETY_CHECK_MISSED           "  Conflicting References"
#define LINK_NAME_INDUCED_ERROR                 "  References Misinterpreted due to previous Errors"

#define LINK_NAME_SEPARATOR '!'
#define LINK_NAME_COLLATE_SYMBOL '#'
#define LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG '*'
#define LINK_NAME_EXTRACT_DEFAULT_FLAG ' '

#define LINK_NAME_INCLUDE_REFS "%%%i"		// no #, netscape does not like them

/* *********************************************************************** */

#define ABS(xxx) ((xxx>0)?(xxx):(-(xxx)))


/* *************************************************************** */

#define MAX_CHARS 127	/* maximal value storable in char encoded types     */
                        /* just for strings encoding fun-profiles for link  */

// !!!!!!! do not move the following into constants.h header, must be here
// because if not, YACC will put there defaults !!!!!!
#define YYSTACKSIZE 5000


/* ***************** OLCX COMMUNICATION CHARS ******************** */

#define COLCX_GOTO_REFERENCE        "#"
#define COLCX_GOTO_REFERENCE_LIST   "@"
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

#define OL_VIEWABLE_REFS(rrr) ((rrr)->usage.kind < UsageMaxOLUsages)

#define IS_BEST_FIT_MATCH(ss) (\
    (ss->ooBits&OOC_VIRTUAL_MASK)==OOC_VIRT_SAME_APPL_FUN_CLASS\
)

#define LINK_NAME_MAYBE_START(ch) (                     \
        ch=='.' || ch=='/' || ch==LINK_NAME_SEPARATOR   \
        || ch==LINK_NAME_COLLATE_SYMBOL                 \
        || ch=='$'                                      \
    )

#define SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs,pbflag) {\
    refs = sessionData.browserStack.top;\
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


/* *******************   Object-Oriented Resolutions   ******************* */

/* virtual resolution */

#define OOC_VIRTUAL_MASK				00700 /* mask for virtual resolution */
#define OOC_VIRT_ANY					00000 /* no matter the class */
#define OOC_VIRT_SUBCLASS_OF_RELATED	00100 /* sub. of a related class */
#define OOC_VIRT_RELATED				00200 /* + related to browsed applic.*/
#define OOC_VIRT_SAME_FUN_CLASS			00300 /* + having the same fun class */
#define OOC_VIRT_APPLICABLE				00400 /* + virtually applicable */
#define OOC_VIRT_SAME_APPL_FUN_CLASS	00500 /* + having the same class */

/* profile resolution */

#define OOC_PROFILE_MASK			07000 /* mask for profile resolution */
#define OOC_PROFILE_ANY				00000 /* no matter the profile */
#define OOC_PROFILE_ARITY			01000 /* same number of arguments */
#define OOC_PROFILE_APPLICABLE		02000 /* + applicable profile */
#define OOC_PROFILE_EQUAL			03000 /* + no conversion meeded */

/* when symbol is selected */

#define DEFAULT_SELECTION_OO_BITS (OOC_VIRT_SAME_FUN_CLASS | OOC_PROFILE_APPLICABLE)
#define RENAME_SELECTION_OO_BITS (OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE)

/* *********************************************************************** */

#define MAP_FUN_SIGNATURE char *file, char *a1, char *a2, Completions *a3, void *a4, int *a5

/* *********************************************************************** */

#define ENTER() {log_trace("Entering: %s", __func__); log_indent();}
#define LEAVE() {log_outdent(); log_trace("Leaving: %s", __func__);}

#endif
