#include "misc.h"
#include "proto.h"

#ifdef __WIN32__
#include <direct.h>
#else
#include <dirent.h>
#endif

#include "globals.h"
#include "options.h"
#include "jsemact.h"
#include "commons.h"
#include "caching.h"
#include "protocol.h"
#include "yylex.h"
#include "cxfile.h"
#include "cxref.h"
#include "classfilereader.h"
#include "editor.h"
#include "fileio.h"
#include "filetable.h"

#include "log.h"
#include "ppc.h"


typedef struct integerList {
    int integer;
    struct integerList *next;
} IntegerList;


/* ***********************************************************
 */

void dumpOptions(int nargc, char **nargv) {
    char tmpBuff[TMP_BUFF_SIZE] = "";

    for (int i=0; i<nargc; i++) {
        sprintf(tmpBuff+strlen(tmpBuff), "%s", nargv[i]);
    }
    assert(strlen(tmpBuff)<TMP_BUFF_SIZE-1);
    ppcGenRecord(PPC_INFORMATION,tmpBuff);
}

/* *************************************************************************
 */

void symDump(Symbol *s) {
    fprintf(dumpOut,"[symbol] %s\n",s->name);
}

void typeDump(TypeModifier *t) {
    fprintf(dumpOut,"dumpStart\n");
    for(; t!=NULL; t=t->next) {
        fprintf(dumpOut," %x\n",t->kind);
    }
    fprintf(dumpOut,"dumpStop\n");
}

void symbolRefItemDump(ReferencesItem *s) {
    fprintf(dumpOut,"%s\t%s %s %d %d %d %d %d\n",
            s->name,
            getFileItem(s->vApplClass)->name,
            getFileItem(s->vFunClass)->name,
            s->type, s->storage, s->scope,
            s->access, s->category);
}

/* *********************************************************************** */


int javaTypeStringSPrint(char *buff, char *str, int nameStyle, int *oNamePos) {
    int i;
    char *pp;
    i = 0;
    if (oNamePos!=NULL) *oNamePos = i;
    for(pp=str; *pp; pp++) {
        if (    currentLanguage == LANG_JAVA &&
                *pp == '.' &&
                *(pp+1) == '<') {
            /* java constructor */
            while (*pp && *pp!='>') pp++;
        } else if (*pp == '/' || *pp == '$') {
            if (nameStyle == SHORT_NAME) i=0;
            else buff[i++] = '.';
            if (oNamePos!=NULL) *oNamePos = i;
        } else {
            buff[i++] = *pp;
        }
    }
    buff[i] = 0;
    return(i);
}

#define CHECK_TYPEDEF(t,type,typedefexp,typebreak) {    \
        if (t->typedefSymbol != NULL && typedefexp) {       \
            assert(t->typedefSymbol->name);                 \
            strcpy(type, t->typedefSymbol->name);           \
            goto typebreak;                             \
        }                                               \
        typedefexp = 1;                                 \
    }


void typeSPrint(char *buff, int *size, TypeModifier *t,
                char *name, int dclSepChar, int maxDeep, int typedefexp,
                int longOrShortName, int *oNamePos) {
    Symbol *dd;   Symbol *ddd;
    char pref[COMPLETION_STRING_SIZE];
    char post[COMPLETION_STRING_SIZE];
    char type[COMPLETION_STRING_SIZE];
    char *ttm;
    int i,j,par,realsize,r,rr,jj,minInfi,typedefexpFlag;
    typedefexpFlag = typedefexp;
    type[0] = 0;
    i = COMPLETION_STRING_SIZE-1;
    pref[i]=0;
    j=0;
    for(;t!=NULL;t=t->next) {
        par = 0;
        for (;t!=NULL && t->kind==TypePointer;t=t->next) {
            CHECK_TYPEDEF(t,type,typedefexpFlag,typebreak);
            pref[--i]='*'; par=1;
        }
        assert(i>2);
        if (t==NULL) goto typebreak;
        CHECK_TYPEDEF(t,type,typedefexpFlag,typebreak);
        switch (t->kind) {
        case TypeArray:
            if (par) {pref[--i]='('; post[j++]=')'; }
            if (LANGUAGE(LANG_JAVA)) {
                pref[--i]=' '; pref[--i]=']'; pref[--i]='[';
            } else {
                post[j++]='['; post[j++]=']';
            }
            break;
        case TypeFunction:
            if (par) {pref[--i]='('; post[j++]=')'; }
            sprintf(post+j,"(");
            j += strlen(post+j);
            if (currentLanguage == LANG_JAVA) {
                jj = COMPLETION_STRING_SIZE - j - TYPE_STR_RESERVE;
                javaSignatureSPrint(post+j, &jj, t->u.m.signature,longOrShortName);
                j += jj;
            } else {
                for(dd=t->u.f.args; dd!=NULL; dd=dd->next) {
                    if (dd->type == TypeElipsis) ttm = "...";
                    else if (dd->name == NULL) ttm = "";
                    else ttm = dd->name;
                    if (dd->type == TypeDefault && dd->u.typeModifier!=NULL) {
                        /* TODO ALL, for string overflow */
                        jj = COMPLETION_STRING_SIZE - j - TYPE_STR_RESERVE;
                        typeSPrint(post+j,&jj,dd->u.typeModifier,ttm,' ',maxDeep-1,1,longOrShortName, NULL);
                        j += jj;
                    } else {
                        sprintf(post+j,"%s",ttm);
                        j += strlen(post+j);
                    }
                    if (dd->next!=NULL && j<COMPLETION_STRING_SIZE)
                        sprintf(post+j,", ");
                    j += strlen(post+j);
                }
            }
            post[j++]=')';
            break;
        case TypeStruct:
        case TypeUnion:
            if (currentLanguage != LANG_JAVA) {
                if (t->kind == TypeStruct) sprintf(type,"struct ");
                else sprintf(type,"union ");
                r = strlen(type);
            } else r=0;
            if (t->u.t->name!=NULL) {
                if (currentLanguage == LANG_JAVA) {
                    r += javaTypeStringSPrint(type+r, t->u.t->linkName,longOrShortName, NULL);
                } else {
                    sprintf(type+r,"%s ",t->u.t->name);
                    r += strlen(type+r);
                }
            }
            if (maxDeep>0) {
                minInfi = r;
                sprintf(type+r,"{ ");
                r += strlen(type+r);
                assert(t->u.t->u.structSpec);
                for(ddd=t->u.t->u.structSpec->records; ddd!=NULL; ddd=ddd->next) {
                    if (ddd->name == NULL) ttm = "";
                    else ttm = ddd->name;
                    rr = COMPLETION_STRING_SIZE - r - TYPE_STR_RESERVE;
                    assert(ddd->u.typeModifier);
                    typeSPrint(type+r, &rr, ddd->u.typeModifier, ttm,' ', maxDeep-1,1,longOrShortName, NULL);
                    r += rr;
                    if (ddd->next!=NULL && r<COMPLETION_STRING_SIZE) {
                        sprintf(type+r,"; ");
                        r += strlen(type+r);
                    }
                }
                sprintf(type+r,"}");
                r += strlen(type+r);
                if (r > *size - TYPE_STR_RESERVE) {
                    r = minInfi;
                    type[r] = 0;
                }
            }
            break;
        case TypeEnum:
            if (t->u.t->name==NULL) sprintf(type,"enum ");
            else sprintf(type,"enum %s",t->u.t->linkName);
            r = strlen(type);
            /*
              r += SLEN(sprintf(type+r," {"));
              for(dd=t->u.t->u.enums; dd!=NULL; dd=dd->next) {
              if (dd->d->name == NULL) ttm = "";
              else ttm = dd->d->name;
              if (r < COMPLETION_STRING_SIZE - TYPE_STR_RESERVE) {
              r += SLEN(sprintf(type+r,"%s",ttm));
              }
              if (dd->next!=NULL) r += SLEN(sprintf(type+r,", "));
              }
              r += SLEN(sprintf(type+r,"}"));
            */
            break;
        default:
            assert(t->kind >= 0 && t->kind < MAX_TYPE);
            assert(strlen(typeNamesTable[t->kind]) < COMPLETION_STRING_SIZE);
            strcpy(type, typeNamesTable[t->kind]);
            r = strlen(type);
            break;
        }
        assert(i>2 && j<COMPLETION_STRING_SIZE-3);
    }
 typebreak:
    post[j]=0;
    realsize = strlen(type) + strlen(pref+i) +
        strlen(name) + strlen(post) +2;
    if (realsize < *size) {
        if (dclSepChar==' ') {
            sprintf(buff,"%s %s", type, pref+i);
        } else {
            sprintf(buff,"%s%c %s", type, dclSepChar, pref+i);
        }
        *size = strlen(buff);
        if (oNamePos!=NULL) *oNamePos = *size;
            sprintf(buff+ *size,"%s", name);
            *size += strlen(buff+ *size);
        sprintf(buff+ *size,"%s", post);
        *size += strlen(buff+ *size);
    } else {
        *size = 0;
        if (oNamePos!=NULL) *oNamePos = *size;
        buff[0]=0;
    }
}

void throwsSprintf(char *out, int outsize, SymbolList *exceptions) {
    int outi,firstflag;
    SymbolList *ee;
    outi = 0;
    //& sprintf(out+outi, " !!! ");
    //& outi += strlen(out+outi);
    if (exceptions != NULL ) {
        sprintf(out+outi, " throws");
        outi += strlen(out+outi);
        firstflag = 1;
        for(ee=exceptions; ee!=NULL; ee=ee->next) {
            if (outi-10 > outsize) break;
            sprintf(out+outi, "%c%s", firstflag?' ':',', ee->element->name);
            outi += strlen(out+outi);
        }
        if (ee!=NULL) sprintf(out+outi, "...");
    }
}

void macDefSPrintf(char *tt, int *size, char *name1, char *name2,
                   int argn, char **args, int *oNamePos) {
    int ii,ll,i,brief=0;
    ii = 0;
    sprintf(tt,"#define ");
    ll = strlen(tt);
    if (oNamePos!=NULL) *oNamePos = ll;
    sprintf(tt+ll,"%s%s",name1,name2);
    ii = strlen(tt);
    assert(ii< *size);
    if (argn != -1) {
        sprintf(tt+ii,"(");
        ii += strlen(tt+ii);
        for(i=0;i<argn;i++) {
            if (args[i]!=NULL && !brief) {
                if (strcmp(args[i], s_cppVarArgsName)==0) sprintf(tt+ii,"...");
                else sprintf(tt+ii,"%s",args[i]);
                ii += strlen(tt+ii);
            }
            if (i+1<argn) {
                sprintf(tt+ii,", ");
                ii += strlen(tt+ii);
            }
            if (ii+TYPE_STR_RESERVE>= *size) {
                sprintf(tt+ii,"...");
                ii += strlen(tt+ii);
                goto pbreak;
            }
        }
    pbreak:
        sprintf(tt+ii,")");
        ii += strlen(tt+ii);
    }
    *size = ii;
}

char * string3ConcatInStackMem(char *str1, char *str2, char *str3) {
    int l1,l2,l3;
    char *s;
    l1 = strlen(str1);
    l2 = strlen(str2);
    l3 = strlen(str3);
    s = StackMemoryAllocC(l1+l2+l3+1, char);
    strcpy(s,str1);
    strcpy(s+l1,str2);
    strcpy(s+l1+l2,str3);
    return(s);
}

/* ******************************************************************* */

char *javaCutClassPathFromFileName(char *fname) {
    StringList    *cp;
    int             len;
    char            *res,*ss;
    res = fname;
    ss = strchr(fname, ZIP_SEPARATOR_CHAR);
    if (ss!=NULL) {         // .zip archive symbol
        res = ss+1;
        goto fini;
    }
    for(cp=javaClassPaths; cp!=NULL; cp=cp->next) {
        len = strlen(cp->string);
        if (filenameCompare(cp->string, fname, len) == 0) {
            res = fname+len;
            goto fini;
        }
    }
 fini:
    if (*res=='/' || *res=='\\') res++;
    return(res);
}

char *javaCutSourcePathFromFileName(char *fname) {
    int             len;
    char            *res,*ss;
    res = fname;
    ss = strchr(fname, ZIP_SEPARATOR_CHAR);
    if (ss!=NULL) return(ss+1);         // .zip archive symbol
    MapOnPaths(javaSourcePaths, {
            len = strlen(currentPath);
            if (filenameCompare(currentPath, fname, len) == 0) {
                res = fname+len;
                goto fini;
            }
        });
    // cut auto-detected source-path
    if (s_javaStat!=NULL && s_javaStat->namedPackagePath != NULL) {
        len = strlen(s_javaStat->namedPackagePath);
        if (filenameCompare(s_javaStat->namedPackagePath, fname, len) == 0) {
            res = fname+len;
            goto fini;
        }
    }
 fini:
    if (*res=='/' || *res=='\\') res++;
    return(res);
}

void javaDotifyFileName(char *ss) {
    char *s;
    for (s=ss; *s; s++) {
        if (*s == '/' || *s == '\\') *s = '.';
    }
}

// file num is not neccessary a class item !
static void getClassFqtNameFromFileNum(int fnum, char *ttt) {
    char *dd, *ss;
    ss = javaCutClassPathFromFileName(getRealFileName_static(getFileItem(fnum)->name));
    strcpy(ttt, ss);
    dd = lastOccurenceInString(ttt, '.');
    if (dd!=NULL) *dd=0;
}

// file num is not neccessarily a class item !
/* TODO: dotify has one of the values DOTIFY_NAME, KEEP_SLASHES or 0
   Convert to enums or keep bool?
*/
void javaGetClassNameFromFileIndex(int nn, char *tmpOut, DotifyMode dotifyMode) {
    getClassFqtNameFromFileNum(nn, tmpOut);
    if (dotifyMode == DOTIFY_NAME)
        javaDotifyFileName(tmpOut);
}

char *javaGetShortClassName(char *inn) {
    int     i;
    char    *cut,*res;
    cut = strchr(inn, LINK_NAME_SEPARATOR);
    if (cut==NULL) cut = inn;
    else cut ++;
    res = cut;
    for(i=0; cut[i]; i++) {
        if (cut[i]=='.' || cut[i]=='/' || cut[i]=='\\' || cut[i]=='$') {
            res = cut+i+1;
        }
    }
    return(res);
}

char *javaGetShortClassNameFromFileNum_st(int fnum) {
    static char res[TMP_STRING_SIZE];
    javaGetClassNameFromFileIndex(fnum, res, DOTIFY_NAME);
    return(javaGetShortClassName(res));
}

char *javaGetNudePreTypeName_static(char *inn, CutOuters cutMode) {
    int             i,len;
    char            *cut,*res,*res2;
    static char     ttt[TMP_STRING_SIZE];
    //&fprintf(dumpOut,"getting type name from %s\n", inn);
    cut = strchr(inn, LINK_NAME_SEPARATOR);
    if (cut==NULL) cut = inn;
    else cut ++;
    res = res2 = cut;
    for(i=0; cut[i]; i++) {
        if (cut[i]=='.' || cut[i]=='/' || cut[i]=='\\'
            || cut[i] == ZIP_SEPARATOR_CHAR
            || (cut[i]=='$' && cutMode==CUT_OUTERS)) {
            res = res2;
            res2 = cut+i+1;
        }
    }
    len = res2-res-1;
    if (len<0) len=0;
    strncpy(ttt, res, len);
    ttt[len]=0;
    //&fprintf(dumpOut,"result is %s\n", ttt);
    return(ttt);
}

void javaSignatureSPrint(char *buff, int *size, char *sig, int classstyle) {
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
                if (*ssig != '/' && *ssig != '$') buff[j++] = *ssig;
                else if (classstyle==LONG_NAME) buff[j++] = '.';
                else j=bj;
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
    char *tt;

    tt = strchr(javaLinkName, LINK_NAME_SEPARATOR);
    if (tt==NULL) tt = javaLinkName;
    else tt ++;
    for(; *tt && *tt!='('; tt++) {
        if (*tt == '/' || *tt=='\\' || *tt=='$') *ff++ = '.';
        else *ff++ = *tt;
        maxlen--;
        if (maxlen <=0) goto fini;
    }
    if (*tt == '(') {
        tlen = maxlen + TYPE_STR_RESERVE;
        if (tlen <= TYPE_STR_RESERVE) goto fini;
        *ff ++ = '('; tlen--;
        javaSignatureSPrint(ff, &tlen, tt, argsStyle);
        ff += tlen;
        *ff ++ = ')';
    }
 fini:
    *ff = 0;
}

char *simpleFileNameFromFileNum(int fnum) {
    return(simpleFileName(getRealFileName_static(getFileItem(fnum)->name)));
}

char *getShortClassNameFromClassNum_st(int fnum) {
    return(javaGetNudePreTypeName_static(getRealFileName_static(getFileItem(fnum)->name),
                                         options.displayNestedClasses));
}

void printSymbolLinkNameString(FILE *file, char *linkName) {
    char temp[MAX_CX_SYMBOL_SIZE];
    linkNamePrettyPrint(temp, linkName, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
    fprintf(file, "%s", temp);
}

void printClassFqtNameFromClassNum(FILE *file, int fnum) {
    char temp[MAX_CX_SYMBOL_SIZE];
    getClassFqtNameFromFileNum(fnum, temp);
    printSymbolLinkNameString(file, temp);
}

void sprintfSymbolLinkName(char *outString, SymbolsMenu *menu) {
    if (menu->references.type == TypeCppInclude) {
        sprintf(outString, "%s",
                simpleFileName(getRealFileName_static(getFileItem(menu->references.vApplClass)->name)));
    } else {
        linkNamePrettyPrint(outString, menu->references.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
    }
}

// this is just to print to file, make any change into sprint...
void printSymbolLinkName(FILE *ff, SymbolsMenu *ss) {
    char ttt[MAX_CX_SYMBOL_SIZE];
    sprintfSymbolLinkName(ttt, ss);
    fprintf(ff, "%s", ttt);
}

void fillTrivialSpecialRefItem(ReferencesItem *ddd , char *name) {
    fillReferencesItem(ddd, name, cxFileHashNumber(name),
                       noFileIndex, noFileIndex, TypeUnknown, StorageAuto,
                       ScopeAuto, AccessDefault, CategoryLocal);
}

/* ***************************************************************** */

/* Can handle overlapping strings and returns a pointer to after the copy */
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

char *lastOccurenceOfSlashOrBackslash(char *string) {
    char *result;
    result = NULL;
    for (char *s=string; *s; s++) {
        if (*s == '/' || *s == '\\')
            result=s;
    }
    return result;
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

char *directoryName_st(char *fullFileName) {
    static char res[MAX_FILE_NAME_SIZE];
    int pathLength;

    pathLength = extractPathInto(fullFileName, res);
    assert(pathLength < MAX_FILE_NAME_SIZE-1);
    if (pathLength>2 && res[pathLength-1]==FILE_PATH_SEPARATOR) res[pathLength-1] = 0;

    return res;
}

int pathncmp(char *path1, char *path2, int n, bool caseSensitive) {
    char *s1,*s2;
    int i;
    int res;

    res = 0;
#if (!defined (__WIN32__))
    if (caseSensitive)
        return strncmp(path1,path2,n);
#endif
    if (n<=0)
        return 0;
#if defined (__WIN32__)
    // there is also problem of drive name on windows
    if (path1[0]!=0 && tolower(path1[0])==tolower(path2[0]) && path1[1]==':' && path2[1]==':') {
        path1+=2;
        path2+=2;
        n -= 2;
    }
#endif
    if (n<=0)
        return 0;

    for (s1=path1,s2=path2,i=1; *s1 && *s2 && i<n; s1++,s2++,i++) {
#if defined (__WIN32__)
        if ((*s1 == '/' || *s1 == '\\')
            &&  (*s2 == '/' || *s2 == '\\'))
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
#if defined (__WIN32__)
    if (    (*s1 == '/' || *s1 == '\\')
            &&  (*s2 == '/' || *s2 == '\\')) {
        res = 0;
    } else
#endif
        if (caseSensitive) {
            res = *s1 - *s2;
        } else {
            res = tolower(*s1) - tolower(*s2);
        }
    return res;
}

int filenameCompare(char *ss1, char *ss2, int n) {
    return pathncmp(ss1, ss2, n, options.fileNamesCaseSensitive);
}

/* Handle mixed case file names */
int compareFileNames(char *ss1, char *ss2) {
    int n;
#if (!defined (__WIN32__))
    if (options.fileNamesCaseSensitive)
        return strcmp(ss1, ss2);
#endif
    n = strlen(ss1);
    return filenameCompare(ss1, ss2, n+1);
}

// ------------------------------------------- SHELL (SUB)EXPRESSIONS ---

static IntegerList *shellMatchNewState(int s, IntegerList *next) {
    IntegerList *res = olcx_alloc(sizeof(IntegerList));
    res->integer = s;
    res->next = next;
    return res;
}

static void shellMatchDeleteState(IntegerList **s) {
    IntegerList *p;
    p = *s;
    *s = (*s)->next;
    olcx_memory_free(p, sizeof(IntegerList));
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
                log_trace("mapdirectoryfiles(%s, %s, %s)", tempString, tempString+ldi+1, fileName+si);
                mapDirectoryFiles(tempString, expandWildcardsMapFun, 0, tempString, tempString+ldi+1,
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
            fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
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
    MapOnPaths(paths, {
            expandWildcardsInOnePathRecursiveMaybe(currentPath, &opaths, &olen);
        });
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


void mapDirectoryFiles(char *dirname,
                      void (*fun)(MAP_FUN_SIGNATURE),
                      int allowEditorFilesFlag,
                      char *a1,
                      char *a2,
                      Completions *a3,
                      void *a4,
                      int *a5
){
#ifdef __WIN32__
    WIN32_FIND_DATA     fdata;
    HANDLE              handle;
    char                *s,*d;
    char                ttt[MAX_FILE_NAME_SIZE];
    for (s=dirname,d=ttt; *s; s++,d++) {
        if (*s=='/') *d=FILE_PATH_SEPARATOR;
        else *d = *s;
    }
    assert(d-ttt < MAX_FILE_NAME_SIZE-3);
    sprintf(d,"%c*",FILE_PATH_SEPARATOR);
    mapPatternFiles(ttt, fun, a1, a2, a3, a4, a5);
#else
    DIR             *fd;
    struct dirent   *dirbuf;

    if (isDirectory(dirname) && (fd = opendir(dirname)) != NULL) {
        while ((dirbuf=readdir(fd)) != NULL) {
            if (dirbuf->d_ino != 0 && strcmp(dirbuf->d_name, ".") != 0 && strcmp(dirbuf->d_name, "..") != 0) {
                log_trace("mapping file %s", dirbuf->d_name);
                (*fun)(dirbuf->d_name, a1, a2, a3, a4, a5);
            }
        }
        closedir(fd);
    }
#endif
    // as special case, during refactorings you have to examine
    // also files stored in renamed buffers
    if (refactoringOptions.refactoringRegime == RegimeRefactory
        && allowEditorFilesFlag==ALLOW_EDITOR_FILES) {
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
    MapOnPaths(paths, {
            //&fprintf(dumpOut,"[sp]checking %s<->%s\n", currentPath, path);
            if (compareFileNames(currentPath, path)==0) {
                //&fprintf(dumpOut,"[sp] saving of mapping %s\n", path);
                return true;
            }
        });
    return false;
}

static bool classPathContainsPath(char *path) {
    for (StringList *cp=javaClassPaths; cp!=NULL; cp=cp->next) {
        //&fprintf(dumpOut,"[cp]checking %s<->%s\n", cp->string, path);
        if (compareFileNames(cp->string, path)==0) {
            //&fprintf(dumpOut,"[cp] saving of mapping %s\n", path);
            return true;
        }
    }
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

void javaGetPackageNameFromSourceFileName(char *src, char *opack) {
    char *sss, *dd;
    sss = javaCutSourcePathFromFileName(src);
    strcpy(opack, sss);
    assert(strlen(opack)+1 < MAX_FILE_NAME_SIZE);
    dd = lastOccurenceInString(opack, '.');
    if (dd!=NULL) *dd=0;
    javaDotifyFileName(opack);
    dd = lastOccurenceInString(opack, '.');
    if (dd!=NULL) *dd=0;
}

void javaMapDirectoryFiles1(char *packageFilename,
                            void (*fun)(MAP_FUN_SIGNATURE),
                            Completions *a1,
                            void *a2,
                            int *a3
){
    char            *fname;

    // Following can make that classes are added several times
    // makes things slow and memory consuming
    // TODO! optimize this - do we know that it matters?
    if (packageFilename == NULL)
        packageFilename = "";

    // source paths
    MapOnPaths(javaSourcePaths, {
            char tmpString[MAX_SOURCE_PATH_SIZE];
            fname = concatDirectoryWithFileName(tmpString, currentPath, packageFilename);
            mapDirectoryFiles(fname,fun,ALLOW_EDITOR_FILES,currentPath,packageFilename,a1,a2,a3);
        });
    // class paths
    for (StringList *cp=javaClassPaths; cp!=NULL; cp=cp->next) {
        char tmpString[MAX_SOURCE_PATH_SIZE];
        // avoid double mappings
        if (!pathsStringContainsPath(javaSourcePaths, cp->string)) {
            assert(strlen(cp->string)+strlen(packageFilename)+2 < SIZE_TMP_MEM);
            fname = concatDirectoryWithFileName(tmpString, cp->string, packageFilename);
            mapDirectoryFiles(fname,fun,ALLOW_EDITOR_FILES,cp->string,packageFilename,a1,a2,a3);
        }
    }
    // databazes
    for (int i=0; i<MAX_JAVA_ZIP_ARCHIVES && zipArchiveTable[i].fn[0]!=0; i++) {
        javaMapZipDirFile(&zipArchiveTable[i],packageFilename,a1,a2,a3,fun,
                          zipArchiveTable[i].fn,packageFilename);
    }
    // auto-inferred source path
    if (s_javaStat->namedPackagePath != NULL) {
        if (!pathsStringContainsPath(javaSourcePaths, s_javaStat->namedPackagePath)
            && !classPathContainsPath(s_javaStat->namedPackagePath)) {
            char tmpString[MAX_SOURCE_PATH_SIZE];
            fname = concatDirectoryWithFileName(tmpString, s_javaStat->namedPackagePath, packageFilename);
            mapDirectoryFiles(fname,fun,ALLOW_EDITOR_FILES,s_javaStat->namedPackagePath,packageFilename,a1,a2,a3);
        }
    }
}

void javaMapDirectoryFiles2(
                            IdList *packid,
                            void (*fun)(MAP_FUN_SIGNATURE),
                            Completions *a1,
                            void *a2,
                            int *a3
                            ){
    char            *packfile;
    char            dname[MAX_FILE_NAME_SIZE];
    packfile=javaCreateComposedName(NULL,packid,'/',NULL,dname,MAX_FILE_NAME_SIZE);
    javaMapDirectoryFiles1(packfile,fun,a1,a2,a3);
}


/* ************************************************************* */

static void scanClassFile(char *zip, char *file, void *dummy) {
    char name[MAX_FILE_NAME_SIZE];
    char *tt, *suff;
    Symbol *memb;
    int cpi;

    log_trace("scanning %s ; %s", zip, file);
    suff = getFileSuffix(file);
    if (compareFileNames(suff, ".class")==0) {
        cpi = cache.cpIndex;
        cache.active = true;
        log_trace("firstFreeIndex = %d", currentBlock->firstFreeIndex);
        placeCachePoint(false);
        cache.active = false;
        memb = javaGetFieldClass(file, &tt);
        FileItem *fileItem = getFileItem(javaCreateClassFileItem(memb));
        if (!fileItem->cxSaved) {
            // read only if not saved (and returned through overflow)
            sprintf(name, "%s%s", zip, file);
            assert(strlen(name) < MAX_FILE_NAME_SIZE-1);
            // recover memories, only cxrefs are interesting
            assert(memb->u.structSpec);
            log_trace("adding %s %s", memb->name, fileItem->name);
            javaReadClassFile(name, memb, DO_NOT_LOAD_SUPER);
        }
        // following is to free CF_MEMORY taken by scan, only
        // cross references in CX_MEMORY are interesting in this case.
        recoverCachePoint(cpi-1, cache.cp[cpi-1].lbcc, 0);
        log_trace("firstFreeIndex = %d", currentBlock->firstFreeIndex);
        log_trace(":ppmmem == %d/%d %x-%x", ppmMemoryIndex, SIZE_ppmMemory, ppmMemory, ppmMemory+SIZE_ppmMemory);
    }
}

void jarFileParse(char *file_name) {
    int archive, fileIndex;

    archive = zipIndexArchive(file_name);
    assert(existsInFileTable(file_name)); /* Filename has to exist in the table */
    fileIndex = addFileNameToFileTable(inputFilename);
    checkFileModifiedTime(fileIndex);
    // set loading to true, no matter whether saved (by overflow) or not
    // the following may create a loop, but it is very unprobable
    FileItem *fileItem = getFileItem(fileIndex);
    fileItem->cxLoading = true;
    if (archive>=0 && archive<MAX_JAVA_ZIP_ARCHIVES) {
        fsRecMapOnFiles(zipArchiveTable[archive].dir, zipArchiveTable[archive].fn,
                        "", scanClassFile, NULL);
    }
    fileItem->cxLoaded = true;
}

void scanJarFilesForTagSearch(void) {
    for (int i=0; i<MAX_JAVA_ZIP_ARCHIVES; i++) {
        fsRecMapOnFiles(zipArchiveTable[i].dir, zipArchiveTable[i].fn,
                        "", scanClassFile, NULL);
    }
}

void classFileParse(void) {
    char    temp[MAX_FILE_NAME_SIZE];
    char    *t,*tt;
    assert(strlen(inputFilename) < MAX_FILE_NAME_SIZE-1);
    strcpy(temp, inputFilename);
    tt = strchr(temp, ';');
    if (tt==NULL) {
        temp[0]=0;
        t = inputFilename;
    } else {
        *(tt+1) = 0;
        t = strchr(inputFilename, ';');
        assert(t!=NULL);
        t ++;
    }
    scanClassFile(temp, t, NULL);
}

bool creatingOlcxRefs(void) {
    /* TODO: what does this actually test? that we need to create refs?  */
    return (
            options.serverOperation==OLO_PUSH
            ||  options.serverOperation==OLO_PUSH_ONLY
            ||  options.serverOperation==OLO_PUSH_AND_CALL_MACRO
            ||  options.serverOperation==OLO_GOTO_PARAM_NAME
            ||  options.serverOperation==OLO_GET_PARAM_COORDINATES
            ||  options.serverOperation==OLO_GET_AVAILABLE_REFACTORINGS
            ||  options.serverOperation==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK
            ||  options.serverOperation==OLO_PUSH_NAME
            ||  options.serverOperation==OLO_PUSH_SPECIAL_NAME
            ||  options.serverOperation==OLO_PUSH_ALL_IN_METHOD
            ||  options.serverOperation==OLO_PUSH_FOR_LOCALM
            ||  options.serverOperation==OLO_TRIVIAL_PRECHECK
            ||  options.serverOperation==OLO_GET_SYMBOL_TYPE
            ||  options.serverOperation==OLO_GET_LAST_IMPORT_LINE
            ||  options.serverOperation==OLO_GLOBAL_UNUSED
            ||  options.serverOperation==OLO_LOCAL_UNUSED
            ||  options.serverOperation==OLO_LIST
            ||  options.serverOperation==OLO_RENAME
            ||  options.serverOperation==OLO_ENCAPSULATE
            ||  options.serverOperation==OLO_ARG_MANIP
            ||  options.serverOperation==OLO_VIRTUAL2STATIC_PUSH
            //&     ||  options.serverOperation==OLO_SAFETY_CHECK1
            ||  options.serverOperation==OLO_SAFETY_CHECK2
            ||  options.serverOperation==OLO_CLASS_TREE
            ||  options.serverOperation==OLO_SYNTAX_PASS_ONLY
            ||  options.serverOperation==OLO_GET_PRIMARY_START
            ||  options.serverOperation==OLO_USELESS_LONG_NAME
            ||  options.serverOperation==OLO_USELESS_LONG_NAME_IN_CLASS
            ||  options.serverOperation==OLO_MAYBE_THIS
            ||  options.serverOperation==OLO_NOT_FQT_REFS
            ||  options.serverOperation==OLO_NOT_FQT_REFS_IN_CLASS
            );
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
