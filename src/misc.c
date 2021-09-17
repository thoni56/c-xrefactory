#include "misc.h"

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

#include "log.h"
#include "ppc.h"


typedef struct integerList {
    int i;
    struct integerList *next;
} IntegerList;


/* ***********************************************************
 */

void dumpOptions(int nargc, char **nargv) {
    int i;
    char tmpBuff[TMP_BUFF_SIZE] = "";

    for(i=0; i<nargc; i++) {
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

void symbolRefItemDump(SymbolReferenceItem *ss) {
    fprintf(dumpOut,"%s\t%s %s %d %d %d %d %d\n",
            ss->name,
            fileTable.tab[ss->vApplClass]->name,
            fileTable.tab[ss->vFunClass]->name,
            ss->b.symType, ss->b.storage, ss->b.scope,
            ss->b.accessFlags, ss->b.category);
}

/* *********************************************************************** */


int javaTypeStringSPrint(char *buff, char *str, int nameStyle, int *oNamePos) {
    int i;
    char *pp;
    i = 0;
    if (oNamePos!=NULL) *oNamePos = i;
    for(pp=str; *pp; pp++) {
        if (    s_language == LANG_JAVA &&
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
            if (s_language == LANG_JAVA) {
                jj = COMPLETION_STRING_SIZE - j - TYPE_STR_RESERVE;
                javaSignatureSPrint(post+j, &jj, t->u.m.signature,longOrShortName);
                j += jj;
            } else {
                for(dd=t->u.f.args; dd!=NULL; dd=dd->next) {
                    if (dd->bits.symbolType == TypeElipsis) ttm = "...";
                    else if (dd->name == NULL) ttm = "";
                    else ttm = dd->name;
                    if (dd->bits.symbolType == TypeDefault && dd->u.type!=NULL) {
                        /* TODO ALL, for string overflow */
                        jj = COMPLETION_STRING_SIZE - j - TYPE_STR_RESERVE;
                        typeSPrint(post+j,&jj,dd->u.type,ttm,' ',maxDeep-1,1,longOrShortName, NULL);
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
        case TypeStruct: case TypeUnion:
            if (s_language != LANG_JAVA) {
                if (t->kind == TypeStruct) sprintf(type,"struct ");
                else sprintf(type,"union ");
                r = strlen(type);
            } else r=0;
            if (t->u.t->name!=NULL) {
                if (s_language == LANG_JAVA) {
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
                assert(t->u.t->u.s);
                for(ddd=t->u.t->u.s->records; ddd!=NULL; ddd=ddd->next) {
                    if (ddd->name == NULL) ttm = "";
                    else ttm = ddd->name;
                    rr = COMPLETION_STRING_SIZE - r - TYPE_STR_RESERVE;
                    assert(ddd->u.type);
                    typeSPrint(type+r, &rr, ddd->u.type, ttm,' ', maxDeep-1,1,longOrShortName, NULL);
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
            sprintf(out+outi, "%c%s", firstflag?' ':',', ee->d->name);
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
        if (fnnCmp(cp->string, fname, len) == 0) {
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
            if (fnnCmp(currentPath, fname, len) == 0) {
                res = fname+len;
                goto fini;
            }
        });
    // cut auto-detected source-path
    if (s_javaStat!=NULL && s_javaStat->namedPackagePath != NULL) {
        len = strlen(s_javaStat->namedPackagePath);
        if (fnnCmp(s_javaStat->namedPackagePath, fname, len) == 0) {
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
    ss = javaCutClassPathFromFileName(getRealFileNameStatic(fileTable.tab[fnum]->name));
    strcpy(ttt, ss);
    dd = lastOccurenceInString(ttt, '.');
    if (dd!=NULL) *dd=0;
}

// file num is not neccessary a class item !
void javaGetClassNameFromFileNum(int nn, char *tmpOut, int dotify) {
    getClassFqtNameFromFileNum(nn, tmpOut);
    if (dotify==DOTIFY_NAME) javaDotifyFileName(tmpOut);
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
    javaGetClassNameFromFileNum(fnum, res, DOTIFY_NAME);
    return(javaGetShortClassName(res));
}

char *javaGetNudePreTypeName_st( char *inn, int cutMode) {
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
    if (sig == NULL) return;
    j = 0;
    /* fprintf(dumpOut,":processing '%s'\n",sig); fflush(dumpOut); */
    assert(*sig == '(');
    ssig = sig; posti=0; post[0]=0;
    for(ssig++; *ssig && *ssig!=')'; ssig++) {
        assert(j+1 < *size);
        if (j+TYPE_STR_RESERVE > *size) goto fini;
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
            typ = s_javaCharCodeBaseTypes[*ssig];
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
    return(simpleFileName(getRealFileNameStatic(fileTable.tab[fnum]->name)));
}

char *getShortClassNameFromClassNum_st(int fnum) {
    return(javaGetNudePreTypeName_st(getRealFileNameStatic(fileTable.tab[fnum]->name),options.nestedClassDisplaying));
}

void printSymbolLinkNameString( FILE *ff, char *linkName) {
    char ttt[MAX_CX_SYMBOL_SIZE];
    linkNamePrettyPrint(ttt, linkName, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
    fprintf(ff,"%s", ttt);
}

void printClassFqtNameFromClassNum(FILE *ff, int fnum) {
    char ttt[MAX_CX_SYMBOL_SIZE];
    getClassFqtNameFromFileNum(fnum, ttt);
    printSymbolLinkNameString(ff, ttt);
}

void sprintfSymbolLinkName(char *ttt, SymbolsMenu *ss) {
    if (ss->s.b.symType == TypeCppInclude) {
        sprintf(ttt, "%s", simpleFileName(getRealFileNameStatic(
                                                                fileTable.tab[ss->s.vApplClass]->name)));
    } else {
        linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
    }
}

// this is just to print to file, make any change into sprint...
void printSymbolLinkName(FILE *ff, SymbolsMenu *ss) {
    char ttt[MAX_CX_SYMBOL_SIZE];
    sprintfSymbolLinkName(ttt, ss);
    fprintf(ff, "%s", ttt);
}

void fillTrivialSpecialRefItem( SymbolReferenceItem *ddd , char *name) {
    fillSymbolRefItem(ddd, name, cxFileHashNumber(name),
                                noFileIndex, noFileIndex);
    fillSymbolRefItemBits(&ddd->b, TypeUnknown, StorageAuto,
                           ScopeAuto, AccessDefault, CategoryLocal);
}

/* ***************************************************************** */

char *strmcpy(char *dest, char *src) {
    char *p1,*p2;
    for(p1=dest,p2=src; *p2; p1++, p2++) *p1 = *p2;
    *p1 = 0;
    return(p1);
}

char *lastOccurenceInString(char *string, int ch) {
    char *s,*res;
    res = NULL;
    for(s=string; *s; s++) {
        if (*s == ch) res=s;
    }
    return res;
}

char *lastOccurenceOfSlashOrBackslash(char *string) {
    char *s,*res;
    res = NULL;
    for(s=string; *s; s++) {
        if (*s == '/' || *s == '\\') res=s;
    }
    return(res);
}

char * getFileSuffix(char *fn) {
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

int pathncmp(char *ss1, char *ss2, int n, bool caseSensitive) {
    char *s1,*s2;
    int i;
    int res;

    res = 0;
#if (!defined (__WIN32__))
    if (caseSensitive) return(strncmp(ss1,ss2,n));
#endif
    if (n<=0) return(0);
#if defined (__WIN32__)
    // there is also problem of drive name on windows
    if (ss1[0]!=0 && tolower(ss1[0])==tolower(ss2[0]) && ss1[1]==':' && ss2[1]==':') {
        ss1+=2;
        ss2+=2;
        n -= 2;
    }
#endif
    if (n<=0) return(0);
    for(s1=ss1,s2=ss2,i=1; *s1 && *s2 && i<n; s1++,s2++,i++) {
#if defined (__WIN32__)
        if (    (*s1 == '/' || *s1 == '\\')
                &&  (*s2 == '/' || *s2 == '\\')) continue;
#endif
        if (caseSensitive) {
            if (*s1 != *s2) break;
        } else {
            if (tolower(*s1) != tolower(*s2)) break;
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

int fnnCmp(char *ss1, char *ss2, int n) {
    return pathncmp(ss1, ss2, n, options.fileNamesCaseSensitive);
}

/* Handle mixed case file names */
int compareFileNames(char *ss1, char *ss2) {
    int n;
#if (!defined (__WIN32__))
    if (options.fileNamesCaseSensitive) return(strcmp(ss1,ss2));
#endif
    n = strlen(ss1);
    return fnnCmp(ss1,ss2,n+1);
}

// ------------------------------------------- SHELL (SUB)EXPRESSIONS ---

static IntegerList *shellMatchNewState(int s, IntegerList *next) {
    IntegerList     *res;
    OLCX_ALLOC(res, IntegerList);
    res->i = s;
    res->next = next;
    return(res);
}

static void shellMatchDeleteState(IntegerList **s) {
    IntegerList *p;
    p = *s;
    *s = (*s)->next;
    OLCX_FREE(p, sizeof(IntegerList));
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
        errorMessage(ERR_ST,"wrong [] pattern in regexp");
    }
    return(i);
}

int shellMatch(char *string, int stringLen, char *pattern, bool caseSensitive) {
    int             si, pi, slen, plen, res;
    IntegerList       *states, **p, *f;
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
            pi = (*p)->i;
            //&fprintf(dumpOut,"checking char %d(%c) and state %d(%c)\n", si, string[si], pi, pattern[pi]);
            if (pattern[pi] == 0) {shellMatchDeleteState(p); continue;}
            if (pattern[pi] == '*') {
                (*p)->next = shellMatchNewState(pi+1, (*p)->next);
            } else if (pattern[pi] == '?') {
                (*p)->i = pi + 1;
            } else if (pattern[pi] == '[') {
                pi = shellMatchParseBracketPattern(pattern, pi, 1, asciiMap);
                if (! asciiMap[string[si]]) {shellMatchDeleteState(p); continue;}
                if (pattern[pi]==']') (*p)->i = pi+1;
            } else if (isalpha(pattern[pi]) && ! caseSensitive) {
                // simple case unsensitive letter
                if (tolower(pattern[pi]) != tolower(string[si])) {
                    shellMatchDeleteState(p); continue;
                }
                (*p)->i = pi + 1;
            } else {
                // simple character
                if (pattern[pi] != string[si]) {
                    shellMatchDeleteState(p); continue;
                }
                (*p)->i = pi + 1;
            }
            p= &(*p)->next;
        }
        si ++;
    }
 fini:
    res = 0;
    for(f=states; f!=NULL; f=f->next) {
        if (f->i == plen || (f->i < plen
                             && strncmp(pattern+f->i,"**************",plen-f->i)==0)
            ) {
            res = 1;
            break;
        }
    }
    while (states!=NULL) shellMatchDeleteState(&states);

    return(res);
}

int containsWildcard(char *ss) {
    int c;
    for(; *ss; ss++) {
        c = *ss;
        if (c=='*' || c=='?' || c=='[') return(1);
    }
    return(0);
}


static void expandWildcardsMapFun(MAP_FUN_SIGNATURE) {
    char            ttt[MAX_FILE_NAME_SIZE];
    char            *dir1, *pattern, *dir2, **outpath;
    int             *freeolen;
    struct stat     st;
    dir1 = (char*) a1;
    pattern = (char*) a2;
    dir2 = (char*) a3;
    outpath = (char **) a4;
    freeolen = a5;
    //&fprintf(dumpOut,"checking match %s <-> %s   %s\n", file, pattern, dir2);fflush(dumpOut);
    if (dir2[0] == FILE_PATH_SEPARATOR) {
        // small optimisation, restrict search to directories
        sprintf(ttt, "%s%s", dir1, file);
        if (editorFileStatus(ttt, &st)!=0 || (st.st_mode & S_IFMT) != S_IFDIR) return;
    }
    if (shellMatch(file, strlen(file), pattern, options.fileNamesCaseSensitive)) {
        sprintf(ttt, "%s%s%s", dir1, file, dir2);
        expandWildcardsInOnePathRecursiveMaybe(ttt, outpath, freeolen);
    }
}

// Dont use this function!!!! what you need is: expandWildcardsInOnePath
/* TODO: WTF, why? we *are* using it... */
void expandWildcardsInOnePathRecursiveMaybe(char *fn, char **outpaths, int *availableSpace) {
    char                ttt[MAX_FILE_NAME_SIZE];
    int                 i, si,di, ldi, len;
    struct stat         st;

    //&fprintf(dumpOut,"expandwc(%s)\n", fn);fflush(dumpOut);
    if (containsWildcard(fn)) {
        si = 0; di = 0;
        while (fn[si]) {
            ldi = di;
            while (fn[si] && fn[si]!=FILE_PATH_SEPARATOR)  {
                ttt[di] = fn[si];
                si++; di++;
            }
            ttt[di] = 0;
            if (containsWildcard(ttt+ldi)) {
                for(i=di; i>=ldi; i--) ttt[i+1] = ttt[i];
                ttt[ldi]=0;
                //&fprintf(dumpOut,"mapdirectoryfiles(%s, %s, %s)\n", ttt, ttt+ldi+1, fn+si);fflush(dumpOut);
                mapDirectoryFiles(ttt, expandWildcardsMapFun, 0, ttt, ttt+ldi+1,
                                  (Completions*)(fn+si), outpaths, availableSpace);
            } else {
                ttt[di] = fn[si];
                if (fn[si]) { di++; si++; }
            }
        }
    } else if (editorFileStatus(fn, &st) == 0) {
        len = strlen(fn);
        strcpy(*outpaths, fn);
        //&fprintf(dumpOut,"adding expandedpath==%s\n", fn);fflush(dumpOut);
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

char *getRealFileNameStatic(char *fn) {
    static char         realFilename[MAX_FILE_NAME_SIZE];
#if defined (__WIN32__)
    WIN32_FIND_DATA     fdata;
    HANDLE              han;
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
        han = FindFirstFile(realFilename, &fdata);
        if (han == INVALID_HANDLE_VALUE) goto bbreak;
        strcpy(realFilename+bdi, fdata.cFileName);
        di = bdi + strlen(realFilename+bdi);
        FindClose(han);
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
                           S_completions *a3,
                           void *a4, int *a5) {
#if defined (__WIN32__)
    WIN32_FIND_DATA     fdata;
    HANDLE              han;
    int res;

    res = 0;
    han = FindFirstFile(pattern, &fdata);
    if (han != INVALID_HANDLE_VALUE) {
        do {
            if (    strcmp(fdata.cFileName,".")!=0
                    &&  strcmp(fdata.cFileName,"..")!=0) {
                (*fun)(fdata.cFileName, a1, a2, a3, a4, a5);
                res = 1;
            }
        } while (FindNextFile(han,&fdata));
        FindClose(han);
    }
    return(res);
#else
    FILEFINDBUF3 fdata = {0};
    HDIR han = HDIR_CREATE;
    ULONG nEntries = 1;
    int res = 0;

    if (!DosFindFirst (pattern, &han, FILE_NORMAL | FILE_DIRECTORY, &fdata, sizeof (FILEFINDBUF3), &nEntries, FIL_STANDARD)) {
        do {
            if ( strcmp(fdata.achName,".")!=0
                 && strcmp(fdata.achName,"..")!=0) {
                (*fun)(fdata.achName, a1, a2, a3, a4, a5);
                res = 1;
            }
        } while (!DosFindNext (han, &fdata, sizeof (FILEFINDBUF3), &nEntries));
        DosFindClose(han);
    }
    return(res);
#endif
}
#endif


int mapDirectoryFiles(
                      char *dirname,
                      void (*fun)(MAP_FUN_SIGNATURE),
                      int allowEditorFilesFlag,
                      char *a1,
                      char *a2,
                      Completions *a3,
                      void *a4,
                      int *a5
                      ){
    int res=0;
#ifdef __WIN32__
    WIN32_FIND_DATA     fdata;
    HANDLE              han;
    char                *s,*d;
    char                ttt[MAX_FILE_NAME_SIZE];
    for (s=dirname,d=ttt; *s; s++,d++) {
        if (*s=='/') *d=FILE_PATH_SEPARATOR;
        else *d = *s;
    }
    assert(d-ttt < MAX_FILE_NAME_SIZE-3);
    sprintf(d,"%c*",FILE_PATH_SEPARATOR);
    res = mapPatternFiles( ttt, fun, a1, a2, a3, a4, a5);
#else
    struct stat     stt;
    DIR             *fd;
    struct dirent   *dirbuf;

    if (  editorFileStatus(dirname,&stt) == 0
          && (stt.st_mode & S_IFMT) == S_IFDIR
          && (fd = opendir(dirname)) != NULL) {
        while ((dirbuf=readdir(fd)) != NULL) {
            if (  dirbuf->d_ino != 0 &&
                  strcmp(dirbuf->d_name, ".") != 0
                  && strcmp(dirbuf->d_name, "..") != 0) {
                /*fprintf(dumpOut,"mapping file %s\n",dirbuf->d_name);fflush(dumpOut);*/
                (*fun)(dirbuf->d_name, a1, a2, a3, a4, a5);
                res = 1;
            }
        }
        closedir(fd);
    }
#endif
    // as special case, during refactorings you have to examine
    // also files stored in renamed buffers
    if (refactoringOptions.refactoringRegime == RegimeRefactory
        && allowEditorFilesFlag==ALLOW_EDITOR_FILES) {
        res |= editorMapOnNonexistantFiles(dirname, fun, DEPTH_ONE, a1, a2, a3, a4, a5);
    }
    return(res);
}

static char *concatFNameInTmpMemory(char *directoryName, char *packageFilename) {
    char *tt, *fname;
    fname = tmpMemory;
    tt = strmcpy(fname, directoryName);
    if (*packageFilename) {
        *tt = FILE_PATH_SEPARATOR;
        strcpy(tt+1, packageFilename);
#if defined (__WIN32__)
        for(s=tt+1; *s; s++)
            if (*s=='/')
                *s = FILE_PATH_SEPARATOR;
#endif
    }
    return(fname);
}

static int pathsStringContainsPath(char *paths, char *path) {
    MapOnPaths(paths, {
            //&fprintf(dumpOut,"[sp]checking %s<->%s\n", currentPath, path);
            if (compareFileNames(currentPath, path)==0) {
                //&fprintf(dumpOut,"[sp] saving of mapping %s\n", path);
                return(1);
            }
        });
    return(0);
}

static int classPathContainsPath(char *path) {
    StringList    *cp;
    for (cp=javaClassPaths; cp!=NULL; cp=cp->next) {
        //&fprintf(dumpOut,"[cp]checking %s<->%s\n", cp->string, path);
        if (compareFileNames(cp->string, path)==0) {
            //&fprintf(dumpOut,"[cp] saving of mapping %s\n", path);
            return(1);
        }
    }
    return(0);
}

int fileNameHasOneOfSuffixes(char *fname, char *suffs) {
    char *suff;
    suff = getFileSuffix(fname);
    if (suff==NULL) return(0);
    if (*suff == '.') suff++;
    return(pathsStringContainsPath(suffs, suff));
}

int stringEndsBySuffix(char *s, char *suffix) {
    int sl, sfl;
    sl = strlen(s);
    sfl = strlen(suffix);
    if (sl >= sfl && strcmp(s+sl-sfl, suffix)==0) return(1);
    return(0);
}

int stringContainsSubstring(char *s, char *subs) {
    int i, im;
    int sl, sbl;
    sl = strlen(s);
    sbl = strlen(subs);
    im = sl-sbl;
    for(i=0; i<=im; i++) {
        if (strncmp(s+i, subs, sbl)==0) return(1);
    }
    return(0);
}

int substringIndexWithLimit(char *s, int limit, char *subs) {
    int i, im;
    int sl, sbl;
    sl = limit;
    sbl = strlen(subs);
    im = sl-sbl;
    for(i=0; i<=im; i++) {
        if (strncmp(s+i, subs, sbl)==0) return(i);
    }
    return(-1);
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

void javaMapDirectoryFiles1(
                            char *packageFilename,
                            void (*fun)(MAP_FUN_SIGNATURE),
                            Completions *a1,
                            void *a2,
                            int *a3
                            ){
    StringList    *cp;
    char            *fname;
    int             i;
    // avoiding recursivity?
    //&static bitArray fileMapped[BIT_ARR_DIM(MAX_FILES)];

    // Following can make that classes are added several times
    // makes things slow and memory consuming
    // TODO! optimize this - do we know that it matters?
    if (packageFilename == NULL)
        packageFilename = "";

    // source paths
    MapOnPaths(javaSourcePaths, {
            fname = concatFNameInTmpMemory(currentPath, packageFilename);
            mapDirectoryFiles(fname,fun,ALLOW_EDITOR_FILES,currentPath,packageFilename,a1,a2,a3);
        });
    // class paths
    for (cp=javaClassPaths; cp!=NULL; cp=cp->next) {
        // avoid double mappings
        if ((! pathsStringContainsPath(javaSourcePaths, cp->string))) {
            assert(strlen(cp->string)+strlen(packageFilename)+2 < SIZE_TMP_MEM);
            fname = concatFNameInTmpMemory(cp->string, packageFilename);
            mapDirectoryFiles(fname,fun,ALLOW_EDITOR_FILES,cp->string,packageFilename,a1,a2,a3);
        }
    }
    // databazes
    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && zipArchiveTable[i].fn[0]!=0; i++) {
        javaMapZipDirFile(&zipArchiveTable[i],packageFilename,a1,a2,a3,fun,
                          zipArchiveTable[i].fn,packageFilename);
    }
    // auto-inferred source path
    if (s_javaStat->namedPackagePath != NULL) {
        if ((! pathsStringContainsPath(javaSourcePaths, s_javaStat->namedPackagePath))
            && (! classPathContainsPath(s_javaStat->namedPackagePath))) {
            fname = concatFNameInTmpMemory(s_javaStat->namedPackagePath, packageFilename);
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

static void scanClassFile(char *zip, char *file, void *arg) {
    char        ttt[MAX_FILE_NAME_SIZE];
    char        *tt, *suff;
    Symbol    *memb;
    int         cpi, fileInd;

    log_trace("scanning %s ; %s", zip, file);
    suff = getFileSuffix(file);
    if (compareFileNames(suff, ".class")==0) {
        cpi = s_cache.cpi;
        s_cache.activeCache = true;
        log_trace("firstFreeIndex = %d", s_topBlock->firstFreeIndex);
        placeCachePoint(false);
        s_cache.activeCache = false;
        memb = javaGetFieldClass(file, &tt);
        fileInd = javaCreateClassFileItem(memb);
        if (! fileTable.tab[fileInd]->b.cxSaved) {
            // read only if not saved (and returned through overflow)
            sprintf(ttt, "%s%s", zip, file);
            assert(strlen(ttt) < MAX_FILE_NAME_SIZE-1);
            // recover memories, only cxrefs are interesting
            assert(memb->u.s);
            log_trace("adding %s %s", memb->name, fileTable.tab[fileInd]->name);
            javaReadClassFile(ttt, memb, DO_NOT_LOAD_SUPER);
        }
        // following is to free CF_MEMORY taken by scan, only
        // cross references in CX_MEMORY are interesting in this case.
        recoverCachePoint(cpi-1, s_cache.cp[cpi-1].lbcc, 0);
        log_trace("firstFreeIndex = %d", s_topBlock->firstFreeIndex);
        //&fprintf(dumpOut,": ppmmem == %d/%d %x-%x\n",ppmMemoryIndex,SIZE_ppmMemory,ppmMemory,ppmMemory+SIZE_ppmMemory);
    }
}

void jarFileParse(char *file_name) {
    int archive, fileIndex;

    archive = zipIndexArchive(file_name);
    assert(fileTableExists(&fileTable, file_name)); /* Filename has to exist in the table */
    fileIndex = addFileTabItem(inputFilename);
    checkFileModifiedTime(fileIndex);
    // set loading to 1, no matter whether saved (by overflow) or not
    // following make create a loop, but it is very unprobable
    fileTable.tab[fileIndex]->b.cxLoading = true;
    if (archive>=0 && archive<MAX_JAVA_ZIP_ARCHIVES) {
        fsRecMapOnFiles(zipArchiveTable[archive].dir, zipArchiveTable[archive].fn,
                        "", scanClassFile, NULL);
    }
    fileTable.tab[fileIndex]->b.cxLoaded = true;
}

void scanJarFilesForTagSearch(void) {
    int i;
    for (i=0; i<MAX_JAVA_ZIP_ARCHIVES; i++) {
        fsRecMapOnFiles(zipArchiveTable[i].dir, zipArchiveTable[i].fn,
                        "", scanClassFile, NULL);
    }
}

void classFileParse(void) {
    char    ttt[MAX_FILE_NAME_SIZE];
    char    *t,*tt;
    assert(strlen(inputFilename) < MAX_FILE_NAME_SIZE-1);
    strcpy(ttt, inputFilename);
    tt = strchr(ttt, ';');
    if (tt==NULL) {
        ttt[0]=0;
        t = inputFilename;
    } else {
        *(tt+1) = 0;
        t = strchr(inputFilename, ';');
        assert(t!=NULL);
        t ++;
    }
    scanClassFile(ttt, t, NULL);
}
