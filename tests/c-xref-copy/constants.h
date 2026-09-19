#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED


#if defined(__WIN32) && (! defined(__WIN32__))
#define __WIN32__ 1
#endif


/* ***********************   memory sizes ******************************  */

#define MacroArgumentsMemorySize  20000	/* macro args name in define */
#define MacroBodyMemorySize     2000000	/* macro expansions */
#define OptionsMemorySize        150000	/* memory used to store options strings */
#define StackMemorySize      100000000	/* parsing stack memory */
#define FileTableMemorySize     8000000	/* memory for file (and class) table */

#define PreprocessorMemorySize 30000000	/* macro definitions */
#define CX_MEMORY_INITIAL_SIZE 80000000
#define CX_MEMORY_CHUNK_SIZE	2000000
                                        /* memory for cross references, can be
                                          increased also by -mf command line
                                          option.
                                        */

#define CX_SPACE_RESERVE          50000	/* space reserve for refs from file */


/* ************************** table sizes ************************ */

#define MAX_FILES                       50000	/* file table size, if you
                                                   change this, RECREATE Tag file */
#define MAX_REFS_HASHTABLE_ENTRIES      65536	/* just hash table size, not limit */

#define MAX_SYMBOLS_HASHTABLE_ENTRIES    5000	/* just hash list table size */


/* ***************** several (mainly string size) bornes *************** */

#define MAX_REF_LIST_LINE_LEN         1000    /* how long part of src is copied to list */
#define TMP_STRING_SIZE               1000    /* Used by cxAddCollateReference for token pasting */
#define REFACTORING_TMP_STRING_SIZE  10000
#define COMPLETION_STRING_SIZE         500	/* size of line in completion list */
#define MAX_EXTRACT_FUN_HEAD_SIZE    10000

#define MAX_FILE_NAME_SIZE            1000
#define MAX_FUNCTION_NAME_LENGTH      1000
#define MAX_CX_SYMBOL_SIZE		3000
#define MAX_INHERITANCE_DEEP     200
#define MAX_ANONYMOUS_FIELDS     200
#define MAX_APPL_OVERLOAD_FUNS	1000	/* max applicable overloaded funs */
#define MAX_OPTION_LEN         10000
#define MAX_COMPLETIONS        10000	/* max items in completion list */

#define MAX_SOURCE_PATH_SIZE	MAX_OPTION_LEN

/* if there are too many on-line references, they are not ordered
   alphabetically according file name. It takes too much time.
*/
#define MAX_SET_GET_OPTIONS         50

/* ************************ preprocessor constants ********************** */

#define MAX_MACRO_ARGS		500		/* max number of macro args */
#define INCLUDE_STACK_SIZE   80		/* max include depth */

/* **************  size of file reading buffers ***************** */

#define CHARACTER_BUFFER_SIZE 1024
#define LEXEM_BUFFER_SIZE (1024 + MAX_LEXEM_SIZE) /* must be bigger than MAX_LEXEM_SIZE */

#define MAX_LEXEM_SIZE MAX_FUNCTION_NAME_LENGTH /* max size of 1 lexem (string, ident)
                                            should be bigger than MAX_FUNCTION_NAME_SIZE */

#define YYIDBUFFER_SIZE 128	/* number of buffered tokens       */


/* *************************************************************** */

#define MAX_PPC_RECORD_SIZE    100000	/* max length of xref-2 communication record */

#define TMP_BUFF_SIZE   50000


#define DEFAULT_CXREF_FILENAME "CXrefs"


/* just constants to be checked, that are data type limits              */
/*  references are compacted (up to 22 bits) */
#define MAX_REFERENCABLE_LINE       4194304
#define MAX_REFERENCABLE_COLUMN     4194304

/* ************************** PLATFORM SPECIFICS ********************* */

#ifdef __WIN32__
#define FILE_PATH_SEPARATOR '\\'
#define PATH_SEPARATOR ';'
#define FILE_BEGIN_DOT '_'
#else
#define FILE_PATH_SEPARATOR '/'
#define PATH_SEPARATOR ':'
#define FILE_BEGIN_DOT '.'
#endif

#endif
