/* *************************************************************** */
/* ********  Constants usefull for source - distribution  ******** */
/*        You are free to appropriate this file to your needs      */
/* *************************************************************** */

#if defined(__WIN32) && (! defined(__WIN32__))
#define __WIN32__ 1
#endif

/*	STANDARD_ALLIGNEMENT must be larger than void* !!!!!!!!!! */
#ifdef __i386__
#define STANDARD_ALLIGNEMENT sizeof(void*)
#else
#define STANDARD_ALLIGNEMENT sizeof(long int)
#endif

/* #define XREF_HUGE 1 */

/* ***********************   memory sizes ******************************  */

#define SIZE_TMP_MEM		       5000	/* temporary strings, not error messages */

#define SIZE_ppMemory 	  	      20000	/* macro args name in define */
#define SIZE_mbMemory 		     500000	/* pending macro expansions */
#define SIZE_opiMemory		     150000	/* memory used to store options strings */
#define SIZE_workMemory        10000000	/* parsing stack memory */
#define SIZE_tmpWorkMemory       200000	/* additional tmp parsing stack memory */
#define SIZE_ftMemory 		    4000000	/* memory for file (and class) table */

#define SIZE_ppmMemory	       15000000	/* macro definitions or java class files */
#ifdef SMALL_OLCX_MEMORY
#define SIZE_olcxMemory        15000000	/* memory for browsing symbol stack */
#else
#define SIZE_olcxMemory        15000000	/* memory for browsing symbol stack */
#endif
#ifdef LINEAR_ADD_REFERENCE
#define CX_MEMORY_CHUNK_SIZE   10000000	
#else
#define CX_MEMORY_CHUNK_SIZE	2000000	
#endif
                                        /* memory for cross references, can be
                                          increased also by -mf command line
                                          option.
										*/

#define CX_SPACE_RESERVE 		  50000	/* space reserve for refs from file */


/* ************************** symbol table sizes ************************ */

#define MAX_FILES			   50000	/* file and class table size, if you 
										   change this, RECREATE Tag file */
#define MAX_JAVA_ZIP_ARCHIVES	 200	/* max .jar archives in CLASSPATH */

#define MAX_CXREF_SYMBOLS	   65536	/* just hash table size, not limit */

#define FQT_CLASS_TAB_SIZE 	   10000	/* just hash list table size */
#define MAX_SYMBOLS 			5000	/* just hash list table size */
#define MAX_JSL_SYMBOLS			5000	/* just hash list table size 
										   size of types for java simple load
										 */
#define MAX_CL_SYMBOLS 			 50		/* hash list table size for class locals */
#define OLCX_TAB_SIZE			 1000	/* total num of frames */

#define MAX_CLASSES				 (MAX_FILES/2)

/* ***************** several (mainly string size) bornes *************** */

#define MAX_REF_LIST_LINE_LEN         1000    /* how long part of src is copied to list */
#define TMP_STRING_SIZE		           350
#define REFACTORING_TMP_STRING_SIZE  10000
#define MACRO_NAME_SIZE                500
#define COMPLETION_STRING_SIZE         500	/* size of line in completion list */
#define MAX_EXTRACT_FUN_HEAD_SIZE    10000

#define MAX_FILE_NAME_SIZE		1000
#define MAX_FUN_NAME_SIZE		1000
#define MAX_HTML_REF_LEN		3000
#define MAX_PROFILE_SIZE 		3000	/* max size of java profile string */
#define MAX_CX_SYMBOL_SIZE		3000
#define MAX_INHERITANCE_DEEP	 200
#define MAX_ANONYMOUS_FIELDS	 200
#define MAX_APPL_OVERLOAD_FUNS	1000	/* max applicable overloaded funs */
#define QNX_MSG_BUF_SIZE 		1000
#define MAX_OPTION_LEN		   10000
#define MAX_NESTED_ERR_ZONES 	 100
#define MAX_COMPLETIONS		   10002	/* max items in completion list */
#define MAX_NESTED_DEEP			 500	/* deep of nested structure defs. */
#define MAX_NESTED_CPP_IF		 100	/* deep of nested #if directives */
#define MAX_INNERS_CLASSES		 200	/* max number of inners classes */
#define MAX_HTML_CUT_PATHES		  50

#define MAX_OLCX_SUFF_SIZE		MAX_FUN_NAME_SIZE
#define MAX_SOURCE_PATH_SIZE	MAX_OPTION_LEN

/* if there is too much on-line references, they are not ordered
   alphabetically according file name. It takes too much time.
*/
#define MAX_OL_REFERENCES_TO_SORT	500	

#define MAX_SET_GET_OPTIONS 		50

/* ************************ preprocessor constants ********************** */

#define MAX_MACRO_ARGS		500		/* max number of macro args */
#define INSTACK_SIZE		 80		/* max include deep */
#define MACSTACK_SIZE		500		/* max deep of macro bodies nesting */

#define MACRO_UNIT_SIZE		20000		/* allocation unit for macro body */
#define MACRO_ARG_UNIT_SIZE	20000		/* allocation unit for macro act arg */

/* **************  size of file reading buffers ***************** */

#define CHAR_BUFF_SIZE 1024
#define LEX_BUFF_SIZE (1024 + MAX_LEXEM_SIZE) /* must be bigger than MAX_LEXEM_SIZE */

#define MAX_UNGET_CHARS 20		/* reserve for ungetChar in char buffer */
#define MAX_LEXEM_SIZE MAX_FUN_NAME_SIZE /* max size of 1 lexem (string, ident)	 */
                                         /* should be bigger, than MAX_FUN_NAME_SIZE */

#define YYBUFFERED_ID_INDEX 128	/* number of buffered tokens       */
  /* for C++ has to be larger because of backtracking parser       */
  /* it should be around 2048, but then workmemory will overflow   */
  /* when loading lot of sourcepath jsl modules                    */
  /* NO, it will overflow due to the whole parsing stack which is  */
  /* stored there                                                  */

#define LEX_POSITIONS_RING_SIZE 16	/* for now, just for block markers */

/* ***************************** caching ******************************** */

#define LEX_BUF_CACHE_SIZE 		300000
#define MAX_CACHE_POINTS 		500
#define INCLUDE_CACHE_SIZE		1000


/* *************************************************************** */

#define MAX_PPC_RECORD_SIZE	   100000	/* max length of xref-2 communication record */

#define COMPACT_TAG_SEARCH_AFTER 10000	/* compact tag search results after n items*/

#define DEFAULT_MENU_FILTER_LEVEL FilterSameProfileRelatedClass
#define DEFAULT_REFS_FILTER_LEVEL RFilterAll

#define MAX_BUFFERED_SIZE_olcxMemory 	(MAX_OLCX_SUFF_SIZE+500)

#define TMP_BUFF_SIZE 	50000

#define QNX_COMPL_SRV_NAME "CCREF/COMPLSRV"

#define EXTRACT_REFERENCE_ARG_STRING "&"

#define MAX_STD_ARGS (MAX_FILES+20)
#define END_OF_OPTIONS_STRING "end-of-options"

#define DEFAULT_CXREF_FILE "Xrefs"

#define MAX_COMPLETIONS_HISTORY_DEEP 10	  /* maximal lenght of completion history */
#define MIN_COMPLETION_INDENT_REST 40     /* minimal columns for symbol informations */
#define MIN_COMPLETION_INDENT 20          /* minimal colums for symbol */
#define MAX_COMPLETION_INDENT 70          /* maximal completion indent with scroll bar */
#define MAX_TAG_SEARCH_INDENT 80          /* maximal tag search indentation with scroll */
#define MAX_TAG_SEARCH_INDENT_RATIO 66    /* maximal tag search indentation screen ratio in % */

#define HTML_SLASH '/'               /* directory separator for generated HTML */

/* just constants to be checked, that are data type limits              */
/* do not modify those constants, rather compile sources with XREF_HUGE */
#ifdef XREF_HUGE	
/* references are stored in 'int' */
#define MAX_REFERENCABLE_LINE 		2147483647
#define MAX_REFERENCABLE_COLUMN 	2147483647
#else
/*  references are compacted (up to 22 bits) */
#define MAX_REFERENCABLE_LINE 		4194304
#define MAX_REFERENCABLE_COLUMN 	4194304
#endif

/* ************************** PORT SPECIFICS ********************* */

#ifdef __WIN32__
#define SLASH '\\'
#define CLASS_PATH_SEPARATOR ';'
#define FILE_BEGIN_DOT '_'
/*typedef int pid_t;*/
#else
#ifdef __OS2__
#define SLASH '\\'
#define CLASS_PATH_SEPARATOR ';'
#define FILE_BEGIN_DOT '.'
#else
#define SLASH '/'
#define CLASS_PATH_SEPARATOR ':'
#define FILE_BEGIN_DOT '.'
#endif
#endif


/* ***************************  cxref files  ********************* */


#if defined(__WIN32__) || defined (__OS2__)		/*SBD*/

#define PRF_FILES 		"\\XFiles"		/*SBD*/
#define PRF_CLASS 		"\\XClasses"	/*SBD*/
#define PRF_REF_PREFIX 	"\\X"			/*SBD*/

#else									/*SBD*/

#define PRF_FILES 		"/XFiles"		/*SBD*/
#define PRF_CLASS 		"/XClasses"		/*SBD*/
#define PRF_REF_PREFIX 	"/X"			/*SBD*/

#endif									/*SBD*/



