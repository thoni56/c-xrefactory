#include "constants.h"
#include "head.h"
#ifdef __WIN32__
#include <direct.h>
#else
#include <dirent.h>
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "commons.h"
#include "editor.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "ppc.h"
#include "proto.h"
#include "protocol.h"
#include "server.h"


typedef struct integerList {
    int integer;
    struct integerList *next;
} IntegerList;


/* *********************************************************** */

void dumpArguments(int nargc, char **nargv) {
    char tmpBuff[TMP_BUFF_SIZE] = "";

    for (int i=0; i<nargc; i++) {
        sprintf(tmpBuff+strlen(tmpBuff), "%s", nargv[i]);
    }
    assert(strlen(tmpBuff)<TMP_BUFF_SIZE-1);
    ppcGenRecord(PPC_INFORMATION,tmpBuff);
}

/* *********************************************************** */

void typeDump(TypeModifier *t) {
    log_debug("dumpStart");
    for(; t!=NULL; t=t->next) {
        log_debug(" %x",t->type);
    }
    log_debug("dumpStop");
}

void dumpReferenceableItem(ReferenceableItem *r) {
    log_debug("%s\t%s %s %d %d %d %d",
              r->linkName,
              getFileItemWithFileNumber(r->includeFileNumber)->name,
              r->type, r->storage, r->scope,
              r->visibility);
}

/* *********************************************************************** */

void prettyPrintType(char *buffer, int *bufferSize, TypeModifier *typeModifier, char *name, int separator,
                bool typedefexp) {
    char    preString[COMPLETION_STRING_SIZE];
    char    postString[COMPLETION_STRING_SIZE];
    char    typeString[COMPLETION_STRING_SIZE];
    int     l;
    bool    par;

    typeString[0] = 0;

    int i = COMPLETION_STRING_SIZE - 1;
    preString[i] = 0;

    int j = 0;
    for (; typeModifier != NULL; typeModifier = typeModifier->next) {
        par = false;
        for (; typeModifier != NULL && typeModifier->type == TypePointer; typeModifier = typeModifier->next) {
            if (typeModifier->typedefSymbol != NULL && typedefexp) {
                assert(typeModifier->typedefSymbol->name);
                strcpy(typeString, typeModifier->typedefSymbol->name);
                goto typebreak;
            }
            typedefexp = true;

            preString[--i] = '*';
            par       = true;
        }
        assert(i > 2);
        if (typeModifier == NULL)
            goto typebreak;

        if (typeModifier->typedefSymbol != NULL && typedefexp) {
            assert(typeModifier->typedefSymbol->name);
            strcpy(typeString, typeModifier->typedefSymbol->name);
            goto typebreak;
        }
        typedefexp = true;

        switch (typeModifier->type) {
        case TypeArray:
            if (par) {
                preString[--i] = '(';
                postString[j++] = ')';
            }
            postString[j++] = '[';
            postString[j++] = ']';
            break;
        case TypeFunction:
            if (par) {
                preString[--i] = '(';
                postString[j++] = ')';
            }
            sprintf(postString + j, "(");
            j += strlen(postString + j);
            for (Symbol *symbol = typeModifier->args; symbol != NULL; symbol = symbol->next) {
                char *ttm;
                if (symbol->type == TypeElipsis)
                    ttm = "...";
                else if (symbol->name == NULL)
                    ttm = "";
                else
                    ttm = symbol->name;
                if (symbol->type == TypeDefault && symbol->typeModifier != NULL) {
                    /* TODO ALL, for string overflow */
                    int jj = COMPLETION_STRING_SIZE - j - TYPE_STR_RESERVE;
                    prettyPrintType(postString + j, &jj, symbol->typeModifier, ttm, ' ', true);
                    j += jj;
                } else {
                    sprintf(postString + j, "%s", ttm);
                    j += strlen(postString + j);
                }
                if (symbol->next != NULL && j < COMPLETION_STRING_SIZE)
                    sprintf(postString + j, ", ");
                j += strlen(postString + j);
            }
            postString[j++] = ')';
            break;
        case TypeStruct:
        case TypeUnion:
            if (typeModifier->type == TypeStruct)
                sprintf(typeString, "struct ");
            else
                sprintf(typeString, "union ");
            l = strlen(typeString);

            if (typeModifier->typeSymbol->name != NULL) {
                sprintf(typeString + l, "%s ", typeModifier->typeSymbol->name);
                l += strlen(typeString + l);
            }
            break;
        case TypeEnum:
            if (typeModifier->typeSymbol->name == NULL)
                sprintf(typeString, "enum ");
            else
                sprintf(typeString, "enum %s", typeModifier->typeSymbol->linkName);
            l = strlen(typeString);
            break;
        default:
            assert(typeModifier->type >= 0 && typeModifier->type < MAX_TYPE);
            assert(strlen(typeNamesTable[typeModifier->type]) < COMPLETION_STRING_SIZE);
            strcpy(typeString, typeNamesTable[typeModifier->type]);
            l = strlen(typeString);
            break;
        }
        assert(i > 2 && j < COMPLETION_STRING_SIZE - 3);
    }
typebreak:
    postString[j]  = 0;
    int realsize = strlen(typeString) + strlen(preString + i) + strlen(name) + strlen(postString) + 2;
    if (realsize < *bufferSize) {
        if (separator == ' ') {
            sprintf(buffer, "%s %s", typeString, preString + i);
        } else {
            sprintf(buffer, "%s%c %s", typeString, separator, preString + i);
        }
        *bufferSize = strlen(buffer);
        sprintf(buffer + *bufferSize, "%s", name);
        *bufferSize += strlen(buffer + *bufferSize);
        sprintf(buffer + *bufferSize, "%s", postString);
        *bufferSize += strlen(buffer + *bufferSize);
    } else {
        *bufferSize = 0;
        buffer[0] = 0;
    }
}

static void prettyPrintMacroArgument(char *buffer, char *arg, bool last) {
    if (arg != NULL) {
        if (strcmp(arg, cppVarArgsName) == 0)
            strcat(buffer, "...");
        else {
            char tmp[TMP_STRING_SIZE];
            sprintf(tmp, "%s", arg);
            strcat(buffer, tmp);
        }
    }
    if (!last) {
        strcat(buffer, ", ");
    }
}

void prettyPrintMacroDefinition(char *buffer, int *bufferSize, char *macroName, int argumentCount, char **arguments) {
    assert(*bufferSize > TYPE_STR_RESERVE);
    strcpy(buffer, "#define ");
    strcat(buffer, macroName);

    assert(strlen(buffer) < *bufferSize);
    if (argumentCount != -1) {
        strcat(buffer, "(");
        for (int i = 0; i < argumentCount; i++) {
            bool last = (i == argumentCount-1);
            prettyPrintMacroArgument(buffer, arguments[i], last);
            if (strlen(buffer) + TYPE_STR_RESERVE >= *bufferSize) {
                strcat(buffer, "...");
                break;
            }
        }
        strcat(buffer, ")");
    }
    *bufferSize = strlen(buffer);
}

/* ******************************************************************* */

void prettyPrintLinkName(char *buffer, char *linkName, int maxlen) {
    /* Skip any file reference in the linkName */
    char *chP = strchr(linkName, LINK_NAME_SEPARATOR);
    if (chP==NULL)
        chP = linkName;
    else
        chP++;

    /* Copy the name */
    char *targetP = buffer;
    for (; *chP && *chP!='('; chP++) {
        *targetP++ = *chP;
        maxlen--;
        if (maxlen <=0)
            break;
    }
    *targetP = 0;
}

char *simpleFileNameFromFileNum(int fnum) {
    return simpleFileName(getRealFileName_static(getFileItemWithFileNumber(fnum)->name));
}

void prettyPrintLinkNameForSymbolInMenu(char *buffer, BrowsingMenu *menu) {
    if (menu->referenceable.type == TypeCppInclude) {
        sprintf(buffer, "%s",
                simpleFileName(getRealFileName_static(getFileItemWithFileNumber(menu->referenceable.includeFileNumber)->name)));
    } else {
        prettyPrintLinkName(buffer, menu->referenceable.linkName, MAX_CX_SYMBOL_SIZE);
    }
}


/* ***************************************************************** */

/* Can handle overlapping strings and returns a pointer to position after the copy */
char *strmcpy(char *dest, char *src) {
    char *p1,*p2;
    for (p1=dest,p2=src; *p2; p1++, p2++)
        *p1 = *p2;
    *p1 = 0;
    return p1;
}

char *lastOccurenceInString(char *string, int ch) {
    return strrchr(string, ch);
}

char *getFileSuffix(char *fn) {
    char *cc;
    if (fn == NULL)
        return "";
    cc = fn + strlen(fn);
    while (*cc != '.' && *cc != FILE_PATH_SEPARATOR && cc > fn)
        cc--;
    return cc;
}

char *simpleFileName(char *fullFileName) {
    char *pp,*fn;
    for(fn=pp=fullFileName; *pp!=0; pp++) {
        if (*pp == '/' || *pp == FILE_PATH_SEPARATOR) fn = pp+1;
    }
    return fn;
}

char *simpleFileNameWithoutSuffix_static(char *fullFileName) {
    static char res[MAX_FILE_NAME_SIZE];
    char *pp,*fn;
    int i;

    for(fn=pp=fullFileName; *pp!=0; pp++) {
        if (*pp == '/' || *pp == FILE_PATH_SEPARATOR) fn = pp+1;
    }
    for (i=0; *fn!='.' && *fn; i++,fn++) {
        res[i] = *fn;
    }
    res[i] = 0;
    return res;
}

char *directoryName_static(char *fullFileName) {
    static char path[MAX_FILE_NAME_SIZE];
    int pathLength;

    pathLength = extractPathInto(fullFileName, path);
    assert(pathLength < MAX_FILE_NAME_SIZE-1);
    if (pathLength>2 && path[pathLength-1]==FILE_PATH_SEPARATOR)
        path[pathLength-1] = '\0';

    return path;
}

int pathncmp(char *path1, char *path2, int length, bool caseSensitive) {
    char *s1, *s2;
    int   i;
    int   cmp;

    cmp = 0;
#if (!defined(__WIN32__))
    if (caseSensitive)
        return strncmp(path1, path2, length);
#endif

    if (length <= 0)
        return 0;

#if defined(__WIN32__)
    // there is also problem with drive name on windows
    // TODO: And why don't we include the drive letter in the comparison?
    // Because they are always case-insensitive?
    if (path1[0] != 0 && tolower(path1[0]) == tolower(path2[0]) && path1[1] == ':' && path2[1] == ':') {
        path1 += 2;
        path2 += 2;
        length -= 2;
    }
    if (length <= 0)
        return 0;
#endif

    for (s1 = path1, s2 = path2, i = 1; *s1 && *s2 && i < length; s1++, s2++, i++) {
#if defined(__WIN32__)
        /* For Win32 we also consider path separators equal */
        if ((*s1 == '/' || *s1 == '\\') && (*s2 == '/' || *s2 == '\\'))
            continue;
#endif
        if (caseSensitive) {
            if (*s1 != *s2)
                break;
        } else {
            if (tolower(*s1) != tolower(*s2))
                break;
        }
    }
#if defined(__WIN32__)
    /* For Win32 we also consider path separators equal */
    if ((*s1 == '/' || *s1 == '\\') && (*s2 == '/' || *s2 == '\\')) {
        cmp = 0;
    } else
#endif
    if (caseSensitive) {
        cmp = *s1 - *s2;
    } else {
        cmp = tolower(*s1) - tolower(*s2);
    }
    return cmp;
}

/* Compare the strings allowing '/' or '\' as path separator */
int filenameCompare(char *fileName1, char *fileName2, int length) {
    return pathncmp(fileName1, fileName2, length, options.fileNamesCaseSensitive);
}

/* Compare mixed case file names -  */
int compareFileNames(char *name1, char *name2) {
    int n;
#if (!defined (__WIN32__))
    if (options.fileNamesCaseSensitive)
        return strcmp(name1, name2);
#endif
    n = strlen(name1);
    return filenameCompare(name1, name2, n+1);
}

// ------------------------------------------- SHELL (SUB)EXPRESSIONS ---

static IntegerList *shellMatchNewState(int s, IntegerList *next) {
    IntegerList *res = malloc(sizeof(IntegerList));
    res->integer = s;
    res->next = next;
    return res;
}

static void shellMatchDeleteState(IntegerList **s) {
    IntegerList *p;
    p = *s;
    *s = (*s)->next;
    free(p);
}

static int shellMatchParseBracketPattern(char *pattern, int pi, bool caseSensitive, char *asciiMap) {
    int        i,j,m;
    int     setval = 1;
    i = pi;
    assert(pattern[i] == '[');
    i++;
    if (pattern[i] == '^') {
        setval = ! setval;
        i++;
    }
    memset(asciiMap, ! setval, MAX_ASCII_CHAR);
    // handle the first ] as special case
    if (pattern[i]==']') {
        asciiMap[pattern[i]] = setval;
        i++;
    }
    while (pattern[i] && pattern[i]!=']') {
        if (pattern[i+1]=='-') {
            // interval
            m = pattern[i+2];
            for(j=pattern[i]; j<=m; j++) asciiMap[j] = setval;
            if ((! caseSensitive) && isalpha(pattern[i]) && isalpha(pattern[i+2])) {
                m = tolower(pattern[i+2]);
                for(j=tolower(pattern[i]); j<=m; j++) asciiMap[j] = setval;
                m = toupper(pattern[i+2]);
                for(j=toupper(pattern[i]); j<=m; j++) asciiMap[j] = setval;
            }
            i += 3;
        } else {
            // single char
            asciiMap[pattern[i]] = setval;
            if ((! caseSensitive) && isalpha(pattern[i])) {
                asciiMap[tolower(pattern[i])] = setval;
                asciiMap[toupper(pattern[i])] = setval;
            }
            i ++;
        }
    }
    if (pattern[i] != ']') {
        errorMessage(ERR_ST, "wrong [] pattern in regexp");
    }
    return i;
}

bool shellMatch(char *string, int stringLength, char *pattern, bool caseSensitive) {
    int             pi;
    IntegerList    *states, **p;
    char            asciiMap[MAX_ASCII_CHAR];

    int i = 0;
    int patternLength = strlen(pattern);

    states = shellMatchNewState(0, NULL);
    while (i<stringLength) {
        if (states == NULL) goto fini;
        p= &states;
        while(*p!=NULL) {
            pi = (*p)->integer;
            //&fprintf(dumpOut,"checking char %d(%c) and state %d(%c)\n", i, string[i], pi, pattern[pi]);
            if (pattern[pi] == 0) {shellMatchDeleteState(p); continue;}
            if (pattern[pi] == '*') {
                (*p)->next = shellMatchNewState(pi+1, (*p)->next);
            } else if (pattern[pi] == '?') {
                (*p)->integer = pi + 1;
            } else if (pattern[pi] == '[') {
                pi = shellMatchParseBracketPattern(pattern, pi, 1, asciiMap);
                if (! asciiMap[string[i]]) {shellMatchDeleteState(p); continue;}
                if (pattern[pi]==']') (*p)->integer = pi+1;
            } else if (isalpha(pattern[pi]) && ! caseSensitive) {
                // simple case unsensitive letter
                if (tolower(pattern[pi]) != tolower(string[i])) {
                    shellMatchDeleteState(p); continue;
                }
                (*p)->integer = pi + 1;
            } else {
                // simple character
                if (pattern[pi] != string[i]) {
                    shellMatchDeleteState(p); continue;
                }
                (*p)->integer = pi + 1;
            }
            p= &(*p)->next;
        }
        i ++;
    }
 fini: ;
    bool res = false;
    for (IntegerList *f=states; f!=NULL; f=f->next) {
        if (f->integer == patternLength || (f->integer < patternLength
                                   && strncmp(pattern+f->integer,"**************",patternLength-f->integer)==0)
        ) {
            res = true;
            break;
        }
    }
    while (states!=NULL)
        shellMatchDeleteState(&states);

    return res;
}

bool containsWildcard(char *string) {
    for (; *string; string++) {
        int c = *string;
        if (c=='*' || c=='?' || c=='[')
            return true;
    }
    return false;
}

/* Forward */
static void expandWildcardsInOnePathRecursiveMaybe(char *fn, char **outpaths, int *freeolen);

static void expandWildcardsMapFun(MAP_FUN_SIGNATURE) {
    char path[MAX_FILE_NAME_SIZE];
    char *dir1, *pattern, *dir2, **outpath;
    int *freeolen;

    dir1 = (char*) a1;
    pattern = (char*) a2;
    dir2 = (char*) a3;
    outpath = (char **) a4;
    freeolen = a5;

    log_debug("checking match '%s' <-> '%s %s'", file, pattern, dir2);
    if (dir2[0] == FILE_PATH_SEPARATOR) {
        // small optimisation, restrict search to directories
        sprintf(path, "%s%s", dir1, file);
        if (!directoryExists(path))
            return;
    }
    if (shellMatch(file, strlen(file), pattern, options.fileNamesCaseSensitive)) {
        sprintf(path, "%s%s%s", dir1, file, dir2);
        expandWildcardsInOnePathRecursiveMaybe(path, outpath, freeolen);
    }
}

// Dont use this function!!!! what you need is: expandWildcardsInOnePath
/* TODO: WTF, why? we *are* using it... */
static void expandWildcardsInOnePathRecursiveMaybe(char *fileName, char **outpaths, int *availableSpace) {
    log_debug("expand wildcards('%s')", fileName);
    if (containsWildcard(fileName)) {
        int si = 0;
        int di = 0;
        while (fileName[si]) {
            char tempString[MAX_FILE_NAME_SIZE];
            int ldi = di;
            while (fileName[si] && fileName[si]!=FILE_PATH_SEPARATOR)  {
                tempString[di] = fileName[si];
                si++; di++;
            }
            tempString[di] = 0;
            if (containsWildcard(tempString+ldi)) {
                for (int i=di; i>=ldi; i--)
                    tempString[i+1] = tempString[i];
                tempString[ldi]=0;
                log_debug("mapOverDirectoryFiles(%s, %s, %s)", tempString, tempString+ldi+1, fileName+si);
                mapOverDirectoryFiles(tempString, expandWildcardsMapFun, 0, tempString, tempString+ldi+1,
                                  (Completions*)(fileName+si), outpaths, availableSpace);
            } else {
                tempString[di] = fileName[si];
                if (fileName[si]) { di++; si++; }
            }
        }
    } else if (directoryExists(fileName)) {
        int len = strlen(fileName);
        strcpy(*outpaths, fileName);
        log_debug("adding expanded path=='%s'", fileName);
        *outpaths += len;
        *availableSpace -= len;
        *((*outpaths)++) = PATH_SEPARATOR;
        *(*outpaths) = 0;
        *availableSpace -= 1;
        if (*availableSpace <= 0) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "expanded option %s overflows over MAX_OPTION_LEN",
                    *outpaths-(MAX_OPTION_LEN-*availableSpace));
            FATAL_ERROR(ERR_ST, tmpBuff, EXIT_FAILURE);
        }
    }
}

void expandWildcardsInOnePath(char *filename, char *outpaths, int availableSpace) {
    char    *oop, *opaths;
    int     olen;
    assert(availableSpace == MAX_OPTION_LEN);
    oop = opaths = outpaths; olen = availableSpace;
    expandWildcardsInOnePathRecursiveMaybe(filename, &opaths, &olen);
    *opaths = 0;
    if (opaths != oop) *(opaths-1) = 0;
}

/* *****************************************************************
   TODO: Is this really necessary? For non-win32 it returns the input
   string, and that seemed to work perfectly. Why does win32 require
   this complicated code?
*/

char *getRealFileName_static(char *fn) {
    static char         realFilename[MAX_FILE_NAME_SIZE];
#if defined (__WIN32__)
    WIN32_FIND_DATA     fdata;
    HANDLE              handle;
    int                 si,di,bdi;
    // there is only drive name before the first slash, copy it.
    for(si=0,di=0; fn[si]&&fn[si]!=FILE_PATH_SEPARATOR; si++,di++) realFilename[di]=fn[si];
    if (fn[si]) realFilename[di++]=fn[si++];
    while (fn[si] && fn[si]!=ZIP_SEPARATOR_CHAR) {
        bdi = di;
        while (fn[si] && fn[si]!=FILE_PATH_SEPARATOR && fn[si]!=ZIP_SEPARATOR_CHAR)  {
            realFilename[di] = fn[si];
            si++; di++;
        }
        realFilename[di] = 0;
        //fprintf(ccOut,"translating %s\n",ttt);
        handle = FindFirstFile(realFilename, &fdata);
        if (handle == INVALID_HANDLE_VALUE) goto bbreak;
        strcpy(realFilename+bdi, fdata.cFileName);
        di = bdi + strlen(realFilename+bdi);
        FindClose(handle);
        assert(di < MAX_FILE_NAME_SIZE-1);
        realFilename[di] = fn[si];
        //fprintf(ccOut,"res %s\n",ttt);
        if (fn[si]) { di++; si++; }
    }
 bbreak:
    strcpy(realFilename+di, fn+si);
    return realFilename;
#else
    assert(strlen(fn) < MAX_FILE_NAME_SIZE-1);
    strcpy(realFilename, fn);
    return realFilename;
#endif
}

/* ***************************************************************** */

#if defined (__WIN32__)

static int mapPatternFiles(char *pattern ,
                           void (*fun)(MAP_FUN_SIGNATURE),
                           char *a1, char *a2,
                           Completions *a3,
                           void *a4, int *a5) {
    WIN32_FIND_DATA     fdata;
    HANDLE              handle;
    int res;

    res = 0;
    handle = FindFirstFile(pattern, &fdata);
    if (handle != INVALID_HANDLE_VALUE) {
        do {
            if (    strcmp(fdata.cFileName,".")!=0
                    &&  strcmp(fdata.cFileName,"..")!=0) {
                (*fun)(fdata.cFileName, a1, a2, a3, a4, a5);
                res = 1;
            }
        } while (FindNextFile(handle,&fdata));
        FindClose(handle);
    }
    return res;
}
#endif

void mapOverDirectoryFiles(char *dirname, void (*fun)(MAP_FUN_SIGNATURE), int allowEditorFilesFlag, char *a1,
                       char *a2, Completions *a3, void *a4, int *a5) {
#ifdef __WIN32__
    WIN32_FIND_DATA fdata;
    HANDLE          handle;
    char           *d;
    char            tempBuffer[MAX_FILE_NAME_SIZE];

    for (char *s = dirname, d = tempBuffer; *s; s++, d++) {
        if (*s == '/')
            *d = FILE_PATH_SEPARATOR;
        else
            *d = *s;
    }
    assert(d - tempBuffer < MAX_FILE_NAME_SIZE - 3);
    sprintf(d, "%c*", FILE_PATH_SEPARATOR);
    mapPatternFiles(tempBuffer, fun, a1, a2, a3, a4, a5);
#else
    DIR           *fd;
    struct dirent *dirbuf;

    if (isDirectory(dirname) && (fd = opendir(dirname)) != NULL) {
        while ((dirbuf = readdir(fd)) != NULL) {
            if (dirbuf->d_ino != 0 && strcmp(dirbuf->d_name, ".") != 0
                && strcmp(dirbuf->d_name, "..") != 0) {
                log_debug("mapping file '%s'", dirbuf->d_name);
                (*fun)(dirbuf->d_name, a1, a2, a3, a4, a5);
            }
        }
        closedir(fd);
    }
#endif
    // as special case, during refactorings you have to examine
    // also files stored in renamed buffers
    if (allowEditorFilesFlag == ALLOW_EDITOR_FILES) {
        editorMapOnNonExistantFiles(dirname, fun, DEPTH_ONE, a1, a2, a3, a4, a5);
    }
}

/* Non-static just for unittesting */
protected char *concatDirectoryWithFileName(char *output, char *directoryName, char *fileName) {
    char *charP;

    charP = strmcpy(output, directoryName);
    if (*fileName) {
        *charP = FILE_PATH_SEPARATOR;
        strcpy(charP+1, fileName);
#if defined (__WIN32__)
        for (char *s=charP+1; *s; s++)
            if (*s=='/')
                *s = FILE_PATH_SEPARATOR;
#endif
    }
    return output;
}

static bool pathsStringContainsPath(char *paths, char *path) {
    MAP_OVER_PATHS(paths, {
        //&fprintf(dumpOut,"[sp]checking %s<->%s\n", currentPath, path);
        if (compareFileNames(currentPath, path) == 0) {
            //&fprintf(dumpOut,"[sp] saving of mapping %s\n", path);
            return true;
        }
    });
    return false;
}

bool fileNameHasOneOfSuffixes(char *fname, char *suffs) {
    char *suff;
    suff = getFileSuffix(fname);
    if (suff==NULL)
        return false;
    if (*suff == '.')
        suff++;
    return pathsStringContainsPath(suffs, suff);
}

/* ************************************************************* */

void formatOutputLine(char *line, int startingColumn) {
    int pos, n;
    char *nlp, *p;

    pos = startingColumn; nlp=NULL;
    assert(options.tabulator>1);
    p = line;
    for (;;) {
        while (pos<options.olineLen || nlp==NULL) {
            if (*p == 0)
                return;
            if (*p == ' ')
                nlp = p;
            if (*p == '\t') {
                nlp = p;
                n = options.tabulator-(pos-1)%options.tabulator-1;
                pos += n;
            } else {
                pos++;
            }
            p++;
        }
        *nlp = '\n';
        p = nlp+1;
        nlp=NULL;
        pos = 0;
    }
}

bool isCompilationUnit(char *fileName) {
    return fileNameHasOneOfSuffixes(fileName, options.cFilesSuffixes)
        || compareFileNames(getFileSuffix(fileName), ".y") == 0;
}

Language getLanguageFor(char *fileName) {
    char *suffix;
    Language language = LANG_C;

    suffix = getFileSuffix(fileName);
    if (compareFileNames(suffix, ".y")==0) {
        language = LANG_YACC;
    }
    return language;
}
