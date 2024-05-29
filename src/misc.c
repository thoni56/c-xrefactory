#ifdef __WIN32__
#include <direct.h>
#else
#include <dirent.h>
#endif

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


/* ***********************************************************
 */

void dumpArguments(int nargc, char **nargv) {
    char tmpBuff[TMP_BUFF_SIZE] = "";

    for (int i=0; i<nargc; i++) {
        sprintf(tmpBuff+strlen(tmpBuff), "%s", nargv[i]);
    }
    assert(strlen(tmpBuff)<TMP_BUFF_SIZE-1);
    ppcGenRecord(PPC_INFORMATION,tmpBuff);
}

/* *************************************************************************
 */

void typeDump(TypeModifier *t) {
    log_debug("dumpStart");
    for(; t!=NULL; t=t->next) {
        log_debug(" %x",t->type);
    }
    log_debug("dumpStop");
}

void symbolRefItemDump(ReferenceItem *s) {
    log_debug("%s\t%s %s %d %d %d %d %d",
              s->linkName,
              getFileItem(s->vApplClass)->name,
              getFileItem(s->vFunClass)->name,
              s->type, s->storage, s->scope,
              s->category);
}

/* *********************************************************************** */

void typeSPrint(char *buffer, int *size, TypeModifier *t, char *name, int dclSepChar, bool typedefexp,
                int longOrShortName, int *oNamePos) {
    char    preString[COMPLETION_STRING_SIZE];
    char    postString[COMPLETION_STRING_SIZE];
    char    typeString[COMPLETION_STRING_SIZE];
    int     r;
    bool    par;

    typeString[0] = 0;

    int i = COMPLETION_STRING_SIZE - 1;
    preString[i] = 0;

    int j = 0;
    for (; t != NULL; t = t->next) {
        par = false;
        for (; t != NULL && t->type == TypePointer; t = t->next) {
            if (t->typedefSymbol != NULL && typedefexp) {
                assert(t->typedefSymbol->name);
                strcpy(typeString, t->typedefSymbol->name);
                goto typebreak;
            }
            typedefexp = true;

            preString[--i] = '*';
            par       = true;
        }
        assert(i > 2);
        if (t == NULL)
            goto typebreak;

        if (t->typedefSymbol != NULL && typedefexp) {
            assert(t->typedefSymbol->name);
            strcpy(typeString, t->typedefSymbol->name);
            goto typebreak;
        }
        typedefexp = true;

        switch (t->type) {
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
            for (Symbol *symbol = t->u.f.args; symbol != NULL; symbol = symbol->next) {
                char *ttm;
                if (symbol->type == TypeElipsis)
                    ttm = "...";
                else if (symbol->name == NULL)
                    ttm = "";
                else
                    ttm = symbol->name;
                if (symbol->type == TypeDefault && symbol->u.typeModifier != NULL) {
                    /* TODO ALL, for string overflow */
                    int jj = COMPLETION_STRING_SIZE - j - TYPE_STR_RESERVE;
                    typeSPrint(postString + j, &jj, symbol->u.typeModifier, ttm, ' ', true,
                               longOrShortName, NULL);
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
            if (t->type == TypeStruct)
                sprintf(typeString, "struct ");
            else
                sprintf(typeString, "union ");
            r = strlen(typeString);

            if (t->u.t->name != NULL) {
                sprintf(typeString + r, "%s ", t->u.t->name);
                r += strlen(typeString + r);
            }
            break;
        case TypeEnum:
            if (t->u.t->name == NULL)
                sprintf(typeString, "enum ");
            else
                sprintf(typeString, "enum %s", t->u.t->linkName);
            r = strlen(typeString);
            break;
        default:
            assert(t->type >= 0 && t->type < MAX_TYPE);
            assert(strlen(typeNamesTable[t->type]) < COMPLETION_STRING_SIZE);
            strcpy(typeString, typeNamesTable[t->type]);
            r = strlen(typeString);
            break;
        }
        assert(i > 2 && j < COMPLETION_STRING_SIZE - 3);
    }
typebreak:
    postString[j]  = 0;
    int realsize = strlen(typeString) + strlen(preString + i) + strlen(name) + strlen(postString) + 2;
    if (realsize < *size) {
        if (dclSepChar == ' ') {
            sprintf(buffer, "%s %s", typeString, preString + i);
        } else {
            sprintf(buffer, "%s%c %s", typeString, dclSepChar, preString + i);
        }
        *size = strlen(buffer);
        if (oNamePos != NULL)
            *oNamePos = *size;
        sprintf(buffer + *size, "%s", name);
        *size += strlen(buffer + *size);
        sprintf(buffer + *size, "%s", postString);
        *size += strlen(buffer + *size);
    } else {
        *size = 0;
        if (oNamePos != NULL)
            *oNamePos = *size;
        buffer[0] = 0;
    }
}

void macroDefinitionSPrintf(char *buffer, int *bufferSize, char *name1, char *name2, int argc,
                            char **argv, int *oNamePos) {
    int ll, i, brief = 0;

    int ii = 0;
    sprintf(buffer, "#define ");
    ll = strlen(buffer);
    if (oNamePos != NULL)
        *oNamePos = ll;
    sprintf(buffer + ll, "%s%s", name1, name2);
    ii = strlen(buffer);
    assert(ii < *bufferSize);
    if (argc != -1) {
        sprintf(buffer + ii, "(");
        ii += strlen(buffer + ii);
        for (i = 0; i < argc; i++) {
            if (argv[i] != NULL && !brief) {
                if (strcmp(argv[i], cppVarArgsName) == 0)
                    sprintf(buffer + ii, "...");
                else
                    sprintf(buffer + ii, "%s", argv[i]);
                ii += strlen(buffer + ii);
            }
            if (i + 1 < argc) {
                sprintf(buffer + ii, ", ");
                ii += strlen(buffer + ii);
            }
            if (ii + TYPE_STR_RESERVE >= *bufferSize) {
                sprintf(buffer + ii, "...");
                ii += strlen(buffer + ii);
                goto pbreak;
            }
        }
    pbreak:
        sprintf(buffer + ii, ")");
        ii += strlen(buffer + ii);
    }
    *bufferSize = ii;
}

/* ******************************************************************* */

static void javaSignatureSPrint(char *buff, int *size, char *sig, int longOrShortName) {
    char post[COMPLETION_STRING_SIZE];
    int posti;
    char *ssig;
    int j,bj,typ;

    if (sig == NULL)
        return;
    j = 0;
    /* fprintf(dumpOut,":processing '%s'\n",sig); fflush(dumpOut); */
    assert(*sig == '(');
    ssig = sig; posti=0; post[0]=0;
    for(ssig++; *ssig && *ssig!=')'; ssig++) {
        assert(j+1 < *size);
        if (j+TYPE_STR_RESERVE > *size)
            goto fini;
    switchLabel:
        switch (*ssig) {
        case '[':
            sprintf(post+posti,"[]");
            posti += strlen(post+posti);
            for(ssig++; *ssig && isdigit(*ssig); ssig++) ;
            goto switchLabel;
        case 'L':
            bj = j;
            for(ssig++; *ssig && *ssig!=';'; ssig++) {
                if (*ssig != '/' && *ssig != '$')
                    buff[j++] = *ssig;
                else if (longOrShortName==LONG_NAME)
                    buff[j++] = '.';
                else
                    j=bj;
            }
            break;
        default:
            typ = javaCharCodeBaseTypes[*ssig];
            assert(typ > 0 && typ < MAX_TYPE);
            sprintf(buff+j, "%s", typeNamesTable[typ]);
            j += strlen(buff+j);
        }
        sprintf(buff+j, "%s",post);
        j += strlen(buff+j);
        posti = 0; post[0] = 0;
        if (*(ssig+1)!=')') {
            sprintf(buff+j, ", ");
            j += strlen(buff+j);
        }
    }
 fini:
    assert(j+1 < *size);
    if (j+TYPE_STR_RESERVE > *size) {
        j = *size - TYPE_STR_RESERVE - 3;
        sprintf(buff+j, "...");
        j += strlen(buff+j);
    }
    buff[j] = 0;
    *size = j;
}

void linkNamePrettyPrint(char *ff, char *javaLinkName, int maxlen,
                         int argsStyle) {
    int tlen;
    char *l;

    l = strchr(javaLinkName, LINK_NAME_SEPARATOR);
    if (l==NULL)
        l = javaLinkName;
    else
        l ++;

    for(; *l && *l!='('; l++) {
        if (*l == '/' || *l=='\\' || *l=='$')
            *ff++ = '.';
        else
            *ff++ = *l;
        maxlen--;
        if (maxlen <=0)
            goto fini;
    }

    if (*l == '(') {
        /* TODO: This looks like it is related to Java, why? */
        tlen = maxlen + TYPE_STR_RESERVE;
        if (tlen <= TYPE_STR_RESERVE)
            goto fini;
        *ff++ = '('; tlen--;
        javaSignatureSPrint(ff, &tlen, l, argsStyle);
        ff += tlen;
        *ff++ = ')';
    }
 fini:
    *ff = 0;
}

char *simpleFileNameFromFileNum(int fnum) {
    return(simpleFileName(getRealFileName_static(getFileItem(fnum)->name)));
}

void sprintfSymbolLinkName(SymbolsMenu *menu, char *name) {
    if (menu->references.type == TypeCppInclude) {
        sprintf(name, "%s",
                simpleFileName(getRealFileName_static(getFileItem(menu->references.vApplClass)->name)));
    } else {
        linkNamePrettyPrint(name, menu->references.linkName, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
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
    if (fn == NULL) return("");
    cc = fn + strlen(fn);
    while (*cc != '.' && *cc != FILE_PATH_SEPARATOR && cc > fn) cc--;
    return(cc);
}

char *simpleFileName(char *fullFileName) {
    char *pp,*fn;
    for(fn=pp=fullFileName; *pp!=0; pp++) {
        if (*pp == '/' || *pp == FILE_PATH_SEPARATOR) fn = pp+1;
    }
    return(fn);
}

char *simpleFileNameWithoutSuffix_st(char *fullFileName) {
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

int filenameCompare(char *ss1, char *ss2, int n) {
    return pathncmp(ss1, ss2, n, options.fileNamesCaseSensitive);
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
    IntegerList *res = olcxAlloc(sizeof(IntegerList));
    res->integer = s;
    res->next = next;
    return res;
}

static void shellMatchDeleteState(IntegerList **s) {
    IntegerList *p;
    p = *s;
    *s = (*s)->next;
    olcxFree(p, sizeof(IntegerList));
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
    return(i);
}

bool shellMatch(char *string, int stringLen, char *pattern, bool caseSensitive) {
    int             si, pi, slen, plen;
    IntegerList    *states, **p;
    char            asciiMap[MAX_ASCII_CHAR];

    si = 0;
    //&slen = strlen(string);
    slen = stringLen;
    plen = strlen(pattern);
    states = shellMatchNewState(0, NULL);
    while (si<slen) {
        if (states == NULL) goto fini;
        p= &states;
        while(*p!=NULL) {
            pi = (*p)->integer;
            //&fprintf(dumpOut,"checking char %d(%c) and state %d(%c)\n", si, string[si], pi, pattern[pi]);
            if (pattern[pi] == 0) {shellMatchDeleteState(p); continue;}
            if (pattern[pi] == '*') {
                (*p)->next = shellMatchNewState(pi+1, (*p)->next);
            } else if (pattern[pi] == '?') {
                (*p)->integer = pi + 1;
            } else if (pattern[pi] == '[') {
                pi = shellMatchParseBracketPattern(pattern, pi, 1, asciiMap);
                if (! asciiMap[string[si]]) {shellMatchDeleteState(p); continue;}
                if (pattern[pi]==']') (*p)->integer = pi+1;
            } else if (isalpha(pattern[pi]) && ! caseSensitive) {
                // simple case unsensitive letter
                if (tolower(pattern[pi]) != tolower(string[si])) {
                    shellMatchDeleteState(p); continue;
                }
                (*p)->integer = pi + 1;
            } else {
                // simple character
                if (pattern[pi] != string[si]) {
                    shellMatchDeleteState(p); continue;
                }
                (*p)->integer = pi + 1;
            }
            p= &(*p)->next;
        }
        si ++;
    }
 fini: ;
    bool res = false;
    for (IntegerList *f=states; f!=NULL; f=f->next) {
        if (f->integer == plen || (f->integer < plen
                                   && strncmp(pattern+f->integer,"**************",plen-f->integer)==0)
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

//#define MAP_FUN_SIGNATURE char *file, char *a1, char *a2, Completions *a3, void *a4, int *a5
static void expandWildcardsMapFun(MAP_FUN_SIGNATURE) {
    char path[MAX_FILE_NAME_SIZE];
    char *dir1, *pattern, *dir2, **outpath;
    int *freeolen;

    dir1 = (char*) a1;
    pattern = (char*) a2;
    dir2 = (char*) a3;
    outpath = (char **) a4;
    freeolen = a5;

    log_trace("checking match %s <-> %s %s", file, pattern, dir2);
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
    struct stat stat;

    log_trace("expand wildcards(%s)", fileName);
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
                log_trace("mapOverDirectoryFiles(%s, %s, %s)", tempString, tempString+ldi+1, fileName+si);
                mapOverDirectoryFiles(tempString, expandWildcardsMapFun, 0, tempString, tempString+ldi+1,
                                  (Completions*)(fileName+si), outpaths, availableSpace);
            } else {
                tempString[di] = fileName[si];
                if (fileName[si]) { di++; si++; }
            }
        }
    } else if (editorFileStatus(fileName, &stat) == 0) {
        int len = strlen(fileName);
        strcpy(*outpaths, fileName);
        log_trace("adding expanded path==%s", fileName);
        *outpaths += len;
        *availableSpace -= len;
        *((*outpaths)++) = CLASS_PATH_SEPARATOR;
        *(*outpaths) = 0;
        *availableSpace -= 1;
        if (*availableSpace <= 0) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "expanded option %s overflows over MAX_OPTION_LEN",
                    *outpaths-(MAX_OPTION_LEN-*availableSpace));
            FATAL_ERROR(ERR_ST, tmpBuff, XREF_EXIT_ERR);
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

void expandWildcardsInPaths(char *paths, char *outpaths, int availableSpace) {
    char    *oop, *opaths;
    int     olen;
    assert(availableSpace == MAX_OPTION_LEN);
    oop = opaths = outpaths; olen = availableSpace;
    MapOverPaths(paths, { expandWildcardsInOnePathRecursiveMaybe(currentPath, &opaths, &olen); });
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
    return(res);
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
                log_trace("mapping file %s", dirbuf->d_name);
                (*fun)(dirbuf->d_name, a1, a2, a3, a4, a5);
            }
        }
        closedir(fd);
    }
#endif
    // as special case, during refactorings you have to examine
    // also files stored in renamed buffers
    if (allowEditorFilesFlag == ALLOW_EDITOR_FILES) {
        editorMapOnNonexistantFiles(dirname, fun, DEPTH_ONE, a1, a2, a3, a4, a5);
    }
}

/* Non-static just for unittesting */
char *concatDirectoryWithFileName(char *output, char *directoryName, char *fileName) {
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
    MapOverPaths(paths, {
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

bool stringContainsSubstring(char *s, char *subs) {
    int im;
    int sl, sbl;
    sl = strlen(s);
    sbl = strlen(subs);
    im = sl-sbl;
    for (int i=0; i<=im; i++) {
        if (strncmp(s+i, subs, sbl)==0)
            return true;
    }
    return false;
}

int substringIndex(char *s, char *subs) {
    int i, im;
    int sl, sbl;
    sl = strlen(s);
    sbl = strlen(subs);
    im = sl-sbl;
    for(i=0; i<=im; i++) {
        if (strncmp(s+i, subs, sbl)==0) return(i);
    }
    return(-1);
}

/* ************************************************************* */

bool requiresCreatingRefs(ServerOperation operation) {
    return operation==OLO_PUSH
        ||  operation==OLO_PUSH_ONLY
        ||  operation==OLO_PUSH_AND_CALL_MACRO
        ||  operation==OLO_GOTO_PARAM_NAME
        ||  operation==OLO_GET_PARAM_COORDINATES
        ||  operation==OLO_GET_AVAILABLE_REFACTORINGS
        ||  operation==OLO_PUSH_NAME
        ||  operation==OLO_PUSH_FOR_LOCALM
        ||  operation==OLO_GET_LAST_IMPORT_LINE
        ||  operation==OLO_GLOBAL_UNUSED
        ||  operation==OLO_LOCAL_UNUSED
        ||  operation==OLO_LIST
        ||  operation==OLO_RENAME
        ||  operation==OLO_ENCAPSULATE
        ||  operation==OLO_ARG_MANIP
        ||  operation==OLO_SAFETY_CHECK2
        ||  operation==OLO_GET_PRIMARY_START
        ;
}

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

void getBareName(char *name, char **start, int *len) {
    int   _c_;
    char *_ss_;
    _ss_ = *start = name;
    while ((_c_ = *_ss_)) {
        if (_c_ == '(')
            break;
        if (LINK_NAME_MAYBE_START(_c_))
            *start = _ss_ + 1;
        _ss_++;
    }
    *len = _ss_ - *start;
}

Language getLanguageFor(char *fileName) {
    char *suffix;
    Language language = LANG_C;

    if (fileNameHasOneOfSuffixes(fileName, options.javaFilesSuffixes)) {
        FATAL_ERROR(ERR_ST, "Java is no longer supported", -1);
    } else {
        suffix = getFileSuffix(fileName);
        if (compareFileNames(suffix, ".y")==0) {
            language = LANG_YACC;
        } else {
            language = LANG_C;
        }
    }
    return language;
}
