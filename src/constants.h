#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED

/* ************************************************************** */
/* ********  Constants useful for source - distribution  ******** */
/*        You are free to adapt this file to your needs           */
/* ************************************************************** */

#if defined(__WIN32) && (! defined(__WIN32__))
#define __WIN32__ 1
#endif


/* ***********************   memory sizes ******************************  */

#define MacroArgumentsMemorySize  20000	/* macro args name in define */
#define MacroBodyMemorySize     2000000	/* macro expansions */
#define SIZE_optMemory           150000	/* memory used to store options strings */
#define SIZE_stackMemory       30000000	/* parsing stack memory */
#define SIZE_tmpWorkMemory       400000	/* additional tmp parsing stack memory */
#define FileTableMemorySize     8000000	/* memory for file (and class) table */

#define SIZE_ppmMemory         15000000	/* macro definitions */
#define CX_MEMORY_CHUNK_SIZE	2000000
                                        /* memory for cross references, can be
                                          increased also by -mf command line
                                          option.
                                        */

#define CX_SPACE_RESERVE          50000	/* space reserve for refs from file */


/* ************************** symbol table sizes ************************ */

#define MAX_FILES              50000	/* file table size, if you
                                           change this, RECREATE Tag file */
#define MAX_CXREF_ENTRIES      65536	/* just hash table size, not limit */

#define MAX_SYMBOLS             5000	/* just hash list table size */


/* ***************** several (mainly string size) bornes *************** */

#define MAX_REF_LIST_LINE_LEN         1000    /* how long part of src is copied to list */
#define TMP_STRING_SIZE                350
#define REFACTORING_TMP_STRING_SIZE  10000
#define MACRO_NAME_SIZE                500
#define COMPLETION_STRING_SIZE         500	/* size of line in completion list */
#define MAX_EXTRACT_FUN_HEAD_SIZE    10000

#define MAX_FILE_NAME_SIZE		1000
#define MAX_FUNCTION_NAME_LENGTH		1000
#define MAX_REF_LEN             3000
#define MAX_CX_SYMBOL_SIZE		3000
#define MAX_INHERITANCE_DEEP     200
#define MAX_ANONYMOUS_FIELDS     200
#define MAX_APPL_OVERLOAD_FUNS	1000	/* max applicable overloaded funs */
#define MAX_OPTION_LEN         10000
#define MAX_COMPLETIONS        10002	/* max items in completion list */

#define MAX_SOURCE_PATH_SIZE	MAX_OPTION_LEN

/* if there are too many on-line references, they are not ordered
   alphabetically according file name. It takes too much time.
*/
#define MAX_SET_GET_OPTIONS         50

/* ************************ preprocessor constants ********************** */

#define MAX_MACRO_ARGS		500		/* max number of macro args */
#define INCLUDE_STACK_SIZE   80		/* max include depth */
#define MACRO_INPUT_STACK_SIZE	500		/* max depth of macro bodies nesting */

#define MACRO_BODY_BUFFER_SIZE		 2000		/* allocation unit for macro body */
#define MACRO_ARGUMENTS_BUFFER_SIZE	 2000		/* allocation unit for macro act arg */

/* **************  size of file reading buffers ***************** */

#define CHARACTER_BUFFER_SIZE 1024
#define LEXEM_BUFFER_SIZE (1024 + MAX_LEXEM_SIZE) /* must be bigger than MAX_LEXEM_SIZE */

#define MAX_UNGET_CHARS 20		/* reserve for ungetChar in char buffer */
#define MAX_LEXEM_SIZE MAX_FUNCTION_NAME_LENGTH /* max size of 1 lexem
                                            (string, ident) should be
                                            bigger than
                                            MAX_FUN_NAME_SIZE */

#define YYIDBUFFER_SIZE 128	/* number of buffered tokens       */


/* ***************************** caching ******************************** */

#define LEXEM_STREAM_CACHE_SIZE      300000
#define MAX_CACHE_POINTS        500
#define INCLUDE_STACK_CACHE_SIZE		1000


/* *************************************************************** */

#define MAX_PPC_RECORD_SIZE    100000	/* max length of xref-2 communication record */

#define COMPACT_TAGS_AFTER_SEARCH_COUNT 10000	/* compact tag search results after n items*/

#define DEFAULT_MENU_FILTER_LEVEL FilterSameProfileRelatedClass
#define DEFAULT_REFS_FILTER_LEVEL RFilterAll

#define TMP_BUFF_SIZE   50000

#define EXTRACT_REFERENCE_ARG_STRING "&"

#define MAX_STD_ARGS (MAX_FILES+20)
#define END_OF_OPTIONS_STRING "end-of-options"

#define DEFAULT_CXREF_FILENAME "CXrefs"

#define MAX_COMPLETIONS_HISTORY_DEEP 10   /* maximal length of completion history */
#define MIN_COMPLETION_INDENT_REST 40     /* minimal columns for symbol informations */
#define MIN_COMPLETION_INDENT 20          /* minimal colums for symbol */
#define MAX_COMPLETION_INDENT 70          /* maximal completion indent with scroll bar */
#define MAX_TAG_SEARCH_INDENT 80          /* maximal tag search indentation with scroll */
#define MAX_TAG_SEARCH_INDENT_RATIO 66    /* maximal tag search indentation screen ratio in % */


/* just constants to be checked, that are data type limits              */
/*  references are compacted (up to 22 bits) */
#define MAX_REFERENCABLE_LINE       4194304
#define MAX_REFERENCABLE_COLUMN     4194304

/* ************************** PLATFORM SPECIFICS ********************* */

#ifdef __WIN32__
#define FILE_PATH_SEPARATOR '\\'
#define PATH_SEPARATOR ';'
#define FILE_BEGIN_DOT '_'
/*typedef int pid_t;*/
#else
#define FILE_PATH_SEPARATOR '/'
#define PATH_SEPARATOR ':'
#define FILE_BEGIN_DOT '.'
#endif


/* ***************************  cxref filenames ********************* */


#if defined(__WIN32__)

#define REFERENCE_FILENAME_FILES "\\XFiles"
#define REFERENCE_FILENAME_PREFIX "\\X"

#else

#define REFERENCE_FILENAME_FILES "/XFiles"
#define REFERENCE_FILENAME_PREFIX "/X"

#endif

#endif
