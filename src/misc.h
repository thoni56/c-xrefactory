#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include "proto.h"
#include "symbol.h"
#include "editor.h"

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


extern void jarFileParse(char *file_name);
extern void scanJarFilesForTagSearch(void);
extern void classFileParse(void);
extern void fillTrivialSpecialRefItem( SymbolReferenceItem *ddd , char *name);

extern void dumpOptions(int nargc, char **nargv);

extern void symDump(Symbol *symbol);
extern void typeDump(TypeModifier *typeModifiers);
extern void symbolRefItemDump(SymbolReferenceItem *ss);
extern int javaTypeStringSPrint(char *buff, char *str, int nameStyle, int *oNamePos);
extern void typeSPrint(char *buff,int *size,TypeModifier *t,char*name,
                       int dclSepChar, int maxDeep, int typedefexp,
                       int longOrShortName, int *oNamePos);
extern void throwsSprintf(char *out, int outsize, SymbolList *exceptions);
extern void macDefSPrintf(char *buff, int *size, char *name1, char *name2, int argn, char **args, int *oNamePos);
extern char * string3ConcatInStackMem(char *str1, char *str2, char *str3);

extern void javaSignatureSPrint(char *buff, int *size, char *sig, int classstyle);
extern char *strmcpy(char *dest, char *src);
extern char *simpleFileName(char *fullFileName);
extern char *directoryName_st(char *fullFileName);
extern char *simpleFileNameWithoutSuffix_st(char *fullFileName);
extern bool containsWildcard(char *ss);
extern bool shellMatch(char *string, int stringLen, char *pattern, bool caseSensitive);
extern void expandWildcardsInOnePathRecursiveMaybe(char *fn, char **outpaths, int *freeolen);
extern void expandWildcardsInOnePath(char *fn, char *outpaths, int olen);
extern void expandWildcardsInPaths(char *paths, char *outpaths, int freeolen);
extern char * getRealFileName_static(char *fn);
extern int substringIndexWithLimit(char *s, int limit, char *subs);
extern bool stringContainsSubstring(char *s, char *subs);
extern void javaGetPackageNameFromSourceFileName(char *src, char *opack);
extern void javaGetClassNameFromFileNum(int nn, char *tmpOut, int dotify);
extern void javaDotifyFileName( char *ss);
extern char *javaGetNudePreTypeName_static( char *inn, CutOuters cutMode);
extern char *javaGetShortClassName(char *inn);
extern char *javaGetShortClassNameFromFileNum_st(int fnum);
extern int substringIndex(char *s, char *subs);
extern bool fileNameHasOneOfSuffixes(char *fname, char *suffs);
extern int mapDirectoryFiles(
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
extern int fnnCmp(char *ss1, char *ss2, int n);
extern void linkNamePrettyPrint(char *ff, char *javaLinkName, int maxlen,int argsStyle);
extern char *simpleFileNameFromFileNum(int fnum);
extern char *getShortClassNameFromClassNum_st(int fnum);
extern void printSymbolLinkNameString( FILE *ff, char *linkName);
extern void printClassFqtNameFromClassNum(FILE *ff, int fnum);
extern void sprintfSymbolLinkName(char *ttt, SymbolsMenu *ss);
extern void printSymbolLinkName( FILE *ff, SymbolsMenu *ss);

#endif
