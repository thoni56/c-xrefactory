#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include "proto.h"
#include "symbol.h"
#include "editor.h"
#include "server.h"

/*  some constants depending on architercture	*/

#ifndef USE_LONG_BITARRAYS
typedef unsigned char bitArray;
#define BIT_ARR_NBITS 8			/* number of bits */
#define BIT_ARR_NBITSLOG 3		/* log_2(BIT_ARR_NBITS) */
#else
typedef unsigned bitArray;
#define BIT_ARR_NBITS 32        /* number of bits */
#define BIT_ARR_NBITSLOG 5		/* log_2(BIT_ARR_NBITS) */
#endif



/* auxiliary macros */
#define BIT_ARR_NBITS1 (BIT_ARR_NBITS-1)        /* BIT_ARR_NBITS-1 */
#define MODMSK (BIT_ARR_NBITS1)
#define DIVMSK ~(MODMSK)

#define BIT_ARR_DIVNBITS(n) (((n) & DIVMSK)>> BIT_ARR_NBITSLOG)
#define BIT_ARR_N_TH_BIT(n) (1<<((n) & MODMSK))


/* main macros    */

#define BIT_ARR_DIM(nn) (((nn)+BIT_ARR_NBITS1)/BIT_ARR_NBITS)
#define THEBIT(bitarr,s) ((bitarr[BIT_ARR_DIVNBITS(s)] & BIT_ARR_N_TH_BIT(s))!=0)
#define SETBIT(bitarr,s) {bitarr[BIT_ARR_DIVNBITS(s)] |= BIT_ARR_N_TH_BIT(s);}
#define NULLBIT(bitarr,s) {bitarr[BIT_ARR_DIVNBITS(s)] &= ~(BIT_ARR_N_TH_BIT(s));}


#define MapOverPaths(/* const char * */ thePathsToMapOver, /* block of code */ COMMAND) \
    {                                                                   \
        char *currentPath; /* "public" to COMMAND */                    \
        char *mop_pathPointer, *mop_endOfPaths;                         \
        int mop_index;                                                  \
        char mop_paths[MAX_SOURCE_PATH_SIZE]; /* Must be writable which parameter might not be */    \
        assert(thePathsToMapOver!=NULL);                                \
        mop_pathPointer = thePathsToMapOver;                            \
        strcpy(mop_paths, mop_pathPointer);                             \
        currentPath = mop_paths;                                        \
        mop_endOfPaths = currentPath+strlen(currentPath);               \
        while (currentPath<mop_endOfPaths) {                            \
            for (mop_index=0;                                           \
                currentPath[mop_index]!=0 && currentPath[mop_index]!=CLASS_PATH_SEPARATOR; \
                 mop_index++) ;                                         \
            currentPath[mop_index] = 0;                                 \
            if (mop_index>0 && currentPath[mop_index-1]==FILE_PATH_SEPARATOR) \
                currentPath[mop_index-1] = 0;                           \
            COMMAND;                                                    \
            currentPath += mop_index;                                   \
            currentPath++;                                              \
        }                                                               \
    }


extern void jarFileParse(char *file_name);
extern void scanJarFilesForTagSearch(void);
extern void classFileParse(void);

extern void dumpOptions(int nargc, char **nargv);

extern void symDump(Symbol *symbol);
extern void typeDump(TypeModifier *typeModifiers);
extern void symbolRefItemDump(ReferencesItem *ss);
extern int javaTypeStringSPrint(char *buff, char *str, int nameStyle, int *oNamePos);
extern void typeSPrint(char *buff,int *size,TypeModifier *t,char*name,
                       int dclSepChar, int maxDeep, int typedefexp,
                       int longOrShortName, int *oNamePos);
extern void throwsSprintf(char *out, int outsize, SymbolList *exceptions);
extern void macDefSPrintf(char *buff, int *size, char *name1, char *name2, int argn, char **args, int *oNamePos);
extern char *string3ConcatInStackMem(char *str1, char *str2, char *str3);

extern void javaSignatureSPrint(char *buff, int *size, char *sig, int classstyle);
extern char *strmcpy(char *dest, char *src);
extern char *simpleFileName(char *fullFileName);
extern char *directoryName_st(char *fullFileName);
extern char *simpleFileNameWithoutSuffix_st(char *fullFileName);
extern bool containsWildcard(char *ss);
extern bool shellMatch(char *string, int stringLen, char *pattern, bool caseSensitive);
extern void expandWildcardsInOnePath(char *fn, char *outpaths, int olen);
extern void expandWildcardsInPaths(char *paths, char *outpaths, int freeolen);
extern char *getRealFileName_static(char *fn);
extern bool stringContainsSubstring(char *s, char *subs);
extern void javaGetPackageNameFromSourceFileName(char *src, char *opack);
extern void javaGetClassNameFromFileIndex(int nn, char *tmpOut, DotifyMode dotifyMode);
extern void javaDotifyFileName(char *ss);
extern char *javaGetNudePreTypeName_static(char *name, NestedClassesDisplay displayMode);
extern char *javaGetShortClassName(char *name);
extern char *javaGetShortClassNameFromFileNum_static(int fnum);
extern int substringIndex(char *string, char *substring);
extern bool fileNameHasOneOfSuffixes(char *fname, char *suffs);
extern void mapDirectoryFiles(
        char *dirname,
        void (*fun) (MAP_FUN_SIGNATURE),
        int allowEditorFilesFlag,
        char *a1,
        char *a2,
        Completions *a3,
        void *a4,
        int *a5
    );
extern void javaMapDirectoryFiles1(
        char *packfile,
        void (*fun)(MAP_FUN_SIGNATURE),
        Completions *a1,
        void *a2,
        int *a3
    );
extern void javaMapDirectoryFiles2(
        IdList *packid,
        void (*fun)(MAP_FUN_SIGNATURE),
        Completions *a1,
        void *a2,
        int *a3
    );
extern char *lastOccurenceInString(char *ss, int ch);
extern char *lastOccurenceOfSlashOrBackslash(char *ss);
extern char *getFileSuffix(char *fn);
extern char *javaCutClassPathFromFileName(char *fname);
extern char *javaCutSourcePathFromFileName(char *fname);
extern int pathncmp(char *ss1, char *ss2, int n, bool caseSensitive);
extern int compareFileNames(char *ss1, char *ss2);
extern int filenameCompare(char *ss1, char *ss2, int n);
extern void linkNamePrettyPrint(char *ff, char *javaLinkName, int maxlen,int argsStyle);
extern char *simpleFileNameFromFileNum(int fnum);
extern char *getShortClassNameFromClassNum_st(int fnum);
extern void printSymbolLinkNameString( FILE *ff, char *linkName);
extern void printClassFqtNameFromClassNum(FILE *ff, int fnum);
extern void sprintfSymbolLinkName(SymbolsMenu *ss, char *ttt);
extern void printSymbolLinkName(SymbolsMenu *ss, FILE *ff);

#define UNUSED (void)

/* Complement log.c with function to explicitly give file & line */
#define log_with_explicit_file_and_line(level, file, line, ...) log_log(level, file, line, __VA_ARGS__)

extern bool requiresCreatingRefs(ServerOperation operation);
extern void formatOutputLine(char *tt, int startingColumn);
extern void get_bare_name(char *name, char **start, int *len);
extern Language getLanguageFor(char *inFileName);

#endif
