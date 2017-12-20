#ifndef MISC_H
#define MISC_H

#include "proto.h"

void ppcGenSynchroRecord();
void ppcIndentOffset();
void ppcGenGotoOffsetPosition(char *fname, int offset);
void ppcGenRecordBegin(char *kind);
void ppcGenRecordWithAttributeBegin(char *kind, char *attr, char *val);
void ppcGenRecordWithNumAttributeBegin(char *kind, char *attr, int val);
void ppcGenRecordEnd(char *kind);
void ppcGenNumericRecordBegin(char *kind, int val);
void ppcGenTwoNumericAndRecordBegin(char *kind, char *attr1, int val1, char *attr2, int val2);
void ppcGenWithNumericAndRecordBegin(char *kind, int val, char *attr, char *attrVal);
void ppcGenAllCompletionsRecordBegin(int nofocus, int len);
void ppcGenTwoNumericsAndrecord(char *kind, char *attr1, int val1, char *attr2, int val2, char *message,char *suff);
void ppcGenRecordWithNumeric(char *kind, char *attr, int val, char *message,char *suff);
void ppcGenNumericRecord(char *kind, int val, char *message, char *suff);
void ppcGenRecord(char *kind, char *message, char *suffix);
void ppcGenTmpBuff();
void ppcGenDisplaySelectionRecord(char *message, int messageType, int continuation);
void ppcGenGotoMarkerRecord(S_editorMarker *pos);
void ppcGenPosition(S_position *p);
void ppcGenDefinitionNotFoundWarning();
void ppcGenDefinitionNotFoundWarningAtBottom();
void ppcGenReplaceRecord(char *file, int offset, char *oldName, int oldNameLen, char *newName);
void ppcGenPreCheckRecord(S_editorMarker *pos, int oldLen);
void ppcGenReferencePreCheckRecord(S_reference *r, char *text);
void ppcGenGotoPositionRecord(S_position *p);
void ppcGenOffsetPosition(char *fn, int offset);
void ppcGenMarker(S_editorMarker *m);
char * stringNumStr( int rr, int ed, int em, int ey, char *own );
void jarFileParse();
void scanJarFilesForTagSearch();
void classFileParse();
void fillTrivialSpecialRefItem( S_symbolRefItem *ddd , char *name);
void setExpirationFromLicenseString();
int optionsOverflowHandler(int n);
int cxMemoryOverflowHandler(int n);

void noSuchRecordError(char *rec);
void methodAppliedOnNonClass(char *rec);
void methodNameNotRecognized(char *rec);
void dumpOptions(int nargc, char **nargv);
void stackMemoryInit();
void *stackMemoryAlloc(int size);
void *stackMemoryPush(void *p, int size);
int  *stackMemoryPushInt(int x);
char *stackMemoryPushString(char *s);
void stackMemoryPop(void *, int size);
void stackMemoryBlockStart();
void stackMemoryBlockFree();
void stackMemoryStartErrZone();
void stackMemoryStopErrZone();
void stackMemoryErrorInZone();
void stackMemoryDump();

void addToTrail (void (*action)(void*),  void *p);
void removeFromTrailUntil(S_freeTrail *untilP);

void symDump(S_symbol *);
void symbolRefItemDump(S_symbolRefItem *ss);
int javaTypeStringSPrint(char *buff, char *str, int nameStyle, int *oNamePos);
void typeSPrint(char *buff,int *size,S_typeModifiers *t,char*name,
                       int dclSepChar, int maxDeep, int typedefexp,
                       int longOrShortName, int *oNamePos);
void throwsSprintf(char *out, int outsize, S_symbolList *exceptions);
void macDefSPrintf(char *buff, int *size, char *name1, char *name2, int argn, char **args, int *oNamePos);
char * string3ConcatInStackMem(char *str1, char *str2, char *str3);

unsigned hashFun(char *s);
void javaSignatureSPrint(char *buff, int *size, char *sig, int classstyle);
void fPutDecimal(FILE *f, int n);
char *strmcpy(char *dest, char *src);
char *simpleFileName(char *fullFileName);
char *directoryName_st(char *fullFileName);
char *simpleFileNameWithoutSuffix_st(char *fullFileName);
int containsWildCharacter(char *ss);
int shellMatch(char *string, int stringLen, char *pattern, int caseSensitive);
void expandWildCharactersInOnePathRec(char *fn, char **outpaths, int *freeolen);
void expandWildCharactersInOnePath(char *fn, char *outpaths, int olen);
void expandWildCharactersInPaths(char *paths, char *outpaths, int freeolen);
char * getRealFileNameStatic(char *fn);
char *create_temporary_filename();
void copyFile(char *src, char *dest);
void createDir(char *dirname);
void removeFile(char *dirname);
int substringIndexWithLimit(char *s, int limit, char *subs);
int stringContainsSubstring(char *s, char *subs);
void javaGetPackageNameFromSourceFileName(char *src, char *opack);
int substringIndex(char *s, char *subs);
int stringEndsBySuffix(char *s, char *suffix);
int fileNameHasOneOfSuffixes(char *fname, char *suffs);
int mapPatternFiles(
        char *pattern,
        void (*fun) (MAP_FUN_PROFILE),
        char *a1,
        char *a2,
        S_completions *a3,
        void *a4,
        int *a5
    );
int mapDirectoryFiles(
        char *dirname,
        void (*fun) (MAP_FUN_PROFILE),
        int allowEditorFilesFlag,
        char *a1,
        char *a2,
        S_completions *a3,
        void *a4,
        int *a5
    );
void javaMapDirectoryFiles1(
        char *packfile,
        void (*fun)(MAP_FUN_PROFILE),
        S_completions *a1,
        void *a2,
        int *a3
    );
void javaMapDirectoryFiles2(
        S_idIdentList *packid,
        void (*fun)(MAP_FUN_PROFILE),
        S_completions *a1,
        void *a2,
        int *a3
    );
char *lastOccurenceInString(char *ss, int ch);
char *lastOccurenceOfSlashOrAntiSlash(char *ss);
char * getFileSuffix(char *fn);
char *javaCutClassPathFromFileName(char *fname);
char *javaCutSourcePathFromFileName(char *fname);
int pathncmp(char *ss1, char *ss2, int n, int caseSensitive);
int fnCmp(char *ss1, char *ss2);
int fnnCmp(char *ss1, char *ss2, int n);

#endif
