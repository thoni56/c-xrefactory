#ifndef MISC_H
#define MISC_H

#include "proto.h"

/* stack memory synchronized with program block structure */
#define XX_ALLOC(p,t)           {p = (t*) stackMemoryAlloc(sizeof(t)); }
#define XX_ALLOCC(p,n,t)        {p = (t*) stackMemoryAlloc((n)*sizeof(t)); }
#define XX_FREE(p)              { }

#define StackMemPush(x,t) ((t*) stackMemoryPush(x,sizeof(t)))
#define StackMemAlloc(t) ((t*) stackMemoryAlloc(sizeof(t)))

extern void ppcGenSynchroRecord();
extern void ppcIndentOffset();
extern void ppcGenGotoOffsetPosition(char *fname, int offset);
extern void ppcGenRecordBegin(char *kind);
extern void ppcGenRecordWithAttributeBegin(char *kind, char *attr, char *val);
extern void ppcGenRecordWithNumAttributeBegin(char *kind, char *attr, int val);
extern void ppcGenRecordEnd(char *kind);
extern void ppcGenNumericRecordBegin(char *kind, int val);
extern void ppcGenTwoNumericAndRecordBegin(char *kind, char *attr1, int val1, char *attr2, int val2);
extern void ppcGenWithNumericAndRecordBegin(char *kind, int val, char *attr, char *attrVal);
extern void ppcGenAllCompletionsRecordBegin(int nofocus, int len);
extern void ppcGenTwoNumericsAndrecord(char *kind, char *attr1, int val1, char *attr2, int val2, char *message,char *suff);
extern void ppcGenRecordWithNumeric(char *kind, char *attr, int val, char *message,char *suff);
extern void ppcGenNumericRecord(char *kind, int val, char *message, char *suff);
extern void ppcGenRecord(char *kind, char *message, char *suffix);
extern void ppcGenTmpBuff();
extern void ppcGenDisplaySelectionRecord(char *message, int messageType, int continuation);
extern void ppcGenGotoMarkerRecord(S_editorMarker *pos);
extern void ppcGenPosition(S_position *p);
extern void ppcGenDefinitionNotFoundWarning();
extern void ppcGenDefinitionNotFoundWarningAtBottom();
extern void ppcGenReplaceRecord(char *file, int offset, char *oldName, int oldNameLen, char *newName);
extern void ppcGenPreCheckRecord(S_editorMarker *pos, int oldLen);
extern void ppcGenReferencePreCheckRecord(S_reference *r, char *text);
extern void ppcGenGotoPositionRecord(S_position *p);
extern void ppcGenOffsetPosition(char *fn, int offset);
extern void ppcGenMarker(S_editorMarker *m);
extern char * stringNumStr( int rr, int ed, int em, int ey, char *own );
extern void jarFileParse();
extern void scanJarFilesForTagSearch();
extern void classFileParse();
extern void fillTrivialSpecialRefItem( S_symbolRefItem *ddd , char *name);
extern void setExpirationFromLicenseString();
extern int optionsOverflowHandler(int n);
extern int cxMemoryOverflowHandler(int n);

extern void noSuchRecordError(char *rec);
extern void methodAppliedOnNonClass(char *rec);
extern void methodNameNotRecognized(char *rec);
extern void dumpOptions(int nargc, char **nargv);
extern void stackMemoryInit();
extern void *stackMemoryAlloc(int size);
extern void *stackMemoryPush(void *p, int size);
extern int  *stackMemoryPushInt(int x);
extern char *stackMemoryPushString(char *s);
extern void stackMemoryPop(void *, int size);
extern void stackMemoryBlockStart();
extern void stackMemoryBlockFree();
extern void stackMemoryStartErrZone();
extern void stackMemoryStopErrZone();
extern void stackMemoryErrorInZone();
extern void stackMemoryDump();

extern void addToTrail (void (*action)(void*),  void *p);
extern void removeFromTrailUntil(S_freeTrail *untilP);

extern void symDump(S_symbol *);
extern void symbolRefItemDump(S_symbolRefItem *ss);
extern int javaTypeStringSPrint(char *buff, char *str, int nameStyle, int *oNamePos);
extern void typeSPrint(char *buff,int *size,S_typeModifiers *t,char*name,
                       int dclSepChar, int maxDeep, int typedefexp,
                       int longOrShortName, int *oNamePos);
extern void throwsSprintf(char *out, int outsize, S_symbolList *exceptions);
extern void macDefSPrintf(char *buff, int *size, char *name1, char *name2, int argn, char **args, int *oNamePos);
extern char * string3ConcatInStackMem(char *str1, char *str2, char *str3);

extern unsigned hashFun(char *s);
extern void javaSignatureSPrint(char *buff, int *size, char *sig, int classstyle);
extern void fPutDecimal(FILE *f, int n);
extern char *strmcpy(char *dest, char *src);
extern char *simpleFileName(char *fullFileName);
extern char *directoryName_st(char *fullFileName);
extern char *simpleFileNameWithoutSuffix_st(char *fullFileName);
extern int containsWildCharacter(char *ss);
extern int shellMatch(char *string, int stringLen, char *pattern, int caseSensitive);
extern void expandWildCharactersInOnePathRec(char *fn, char **outpaths, int *freeolen);
extern void expandWildCharactersInOnePath(char *fn, char *outpaths, int olen);
extern void expandWildCharactersInPaths(char *paths, char *outpaths, int freeolen);
extern char * getRealFileNameStatic(char *fn);
extern char *create_temporary_filename();
extern void copyFile(char *src, char *dest);
extern void createDir(char *dirname);
extern void removeFile(char *dirname);
extern int substringIndexWithLimit(char *s, int limit, char *subs);
extern int stringContainsSubstring(char *s, char *subs);
extern void javaGetPackageNameFromSourceFileName(char *src, char *opack);
extern char *javaGetNudePreTypeName_st( char *inn, int cutMode);
extern char *javaGetShortClassNameFromFileNum_st(int fnum);
extern int substringIndex(char *s, char *subs);
extern int stringEndsBySuffix(char *s, char *suffix);
extern int fileNameHasOneOfSuffixes(char *fname, char *suffs);
extern int mapPatternFiles(
        char *pattern,
        void (*fun) (MAP_FUN_PROFILE),
        char *a1,
        char *a2,
        S_completions *a3,
        void *a4,
        int *a5
    );
extern int mapDirectoryFiles(
        char *dirname,
        void (*fun) (MAP_FUN_PROFILE),
        int allowEditorFilesFlag,
        char *a1,
        char *a2,
        S_completions *a3,
        void *a4,
        int *a5
    );
extern void javaMapDirectoryFiles1(
        char *packfile,
        void (*fun)(MAP_FUN_PROFILE),
        S_completions *a1,
        void *a2,
        int *a3
    );
extern void javaMapDirectoryFiles2(
        S_idIdentList *packid,
        void (*fun)(MAP_FUN_PROFILE),
        S_completions *a1,
        void *a2,
        int *a3
    );
extern char *lastOccurenceInString(char *ss, int ch);
extern char *lastOccurenceOfSlashOrAntiSlash(char *ss);
extern char * getFileSuffix(char *fn);
extern char *javaCutClassPathFromFileName(char *fname);
extern char *javaCutSourcePathFromFileName(char *fname);
extern int pathncmp(char *ss1, char *ss2, int n, int caseSensitive);
extern int fnCmp(char *ss1, char *ss2);
extern int fnnCmp(char *ss1, char *ss2, int n);
extern void linkNamePrettyPrint(char *ff, char *javaLinkName, int maxlen,int argsStyle);
extern char *simpleFileNameFromFileNum(int fnum);
extern char *getShortClassNameFromClassNum_st(int fnum);
extern void printSymbolLinkNameString( FILE *ff, char *linkName);
extern void printClassFqtNameFromClassNum(FILE *ff, int fnum);
extern void sprintfSymbolLinkName(char *ttt, S_olSymbolsMenu *ss);
extern void printSymbolLinkName( FILE *ff, S_olSymbolsMenu *ss);

#endif
