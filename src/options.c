#include "options.h"

#include "commons.h"
#include "globals.h"
#include "misc.h"
#include "cxref.h"
#include "html.h"
#include "classfilereader.h"
#include "editor.h"

/* The following are currently needed from main:
   xrefSetenv -> common?
   mainHandleSetOption
   dirInputFile
   addHtmlCutPath
   createOptionsString -> here?

 */
#include "main.h"

#include "protocol.h"

#include "log.h"
#include "utils.h"


/* memory where on-line given options are stored */
#define SIZE_optMemory SIZE_opiMemory
static char optMemory[SIZE_optMemory];
static int optMemoryi;

static char s_javaSourcePathStatic[MAX_OPTION_LEN];
static char s_javaClassPathStatic[MAX_OPTION_LEN];

/*
  Here we define the things that are needed and which are defined by
  the compiler as builtins.  Examples are pre-defined include paths,
  various macros, some of which are kind of typedef's.

  NOTE1: that they need not translate to something real, just
  something that parses right.

  NOTE2: there is also an attempt at figuring out pre-defined include
  paths in main.c:getAndProcessGccOptions()

  NOTE3: perhaps it's possible to figure out what the compiler defines
  as builtin's automatically, perhaps using.

      echo | gcc -E -dM - > compiler_definitions.h

  NOTE4: note3 is implemented now, so do we need this now?

 */

/* All standard options have been removed... */

#define ENV_DEFAULT_VAR_FILE            "${__file}"
#define ENV_DEFAULT_VAR_PATH            "${__path}"
#define ENV_DEFAULT_VAR_NAME            "${__name}"
#define ENV_DEFAULT_VAR_SUFFIX          "${__suff}"
#define ENV_DEFAULT_VAR_THIS_CLASS      "${__this}"
#define ENV_DEFAULT_VAR_SUPER_CLASS     "${__super}"

char *expandSpecialFilePredefinedVariables_st(char *tt) {
    static char     res[MAX_OPTION_LEN];
    int             i,j,len,flen,plen,nlen,slen,tlen,suplen;
    char            *fvv, *suffix;
    char            file[MAX_FILE_NAME_SIZE];
    char            path[MAX_FILE_NAME_SIZE];
    char            name[MAX_FILE_NAME_SIZE];
    char            thisclass[MAX_FILE_NAME_SIZE];
    char            superclass[MAX_FILE_NAME_SIZE];
    fvv = getRealFileNameStatic(s_input_file_name);
    strcpy(file, fvv);
    assert(strlen(file) < MAX_FILE_NAME_SIZE-1);
    strcpy(path, directoryName_st(file));
    strcpy(name, simpleFileNameWithoutSuffix_st(file));
    suffix = lastOccurenceInString(file, '.');
    if (suffix==NULL) suffix="";
    else suffix++;
    strcpy(thisclass, s_cps.currentClassAnswer);
    strcpy(superclass, s_cps.currentSuperClassAnswer);
    javaDotifyFileName(thisclass);
    javaDotifyFileName(superclass);

    len = strlen(tt);
    flen = strlen(ENV_DEFAULT_VAR_FILE);
    plen = strlen(ENV_DEFAULT_VAR_PATH);
    nlen = strlen(ENV_DEFAULT_VAR_NAME);
    slen = strlen(ENV_DEFAULT_VAR_SUFFIX);
    tlen = strlen(ENV_DEFAULT_VAR_THIS_CLASS);
    suplen = strlen(ENV_DEFAULT_VAR_SUPER_CLASS);
    i = j = 0;
    //& fprintf(dumpOut,"path, name, suff == %s %s %s\n", path, name, suffix);
    while (i<len) {
        if (strncmp(&tt[i], ENV_DEFAULT_VAR_FILE, flen)==0) {
            sprintf(&res[j], "%s", file);
            i += flen;
            j += strlen(&res[j]);
        } else if (strncmp(&tt[i], ENV_DEFAULT_VAR_PATH, plen)==0) {
            sprintf(&res[j], "%s", path);
            i += plen;
            j += strlen(&res[j]);
        } else if (strncmp(&tt[i], ENV_DEFAULT_VAR_NAME, nlen)==0) {
            sprintf(&res[j], "%s", name);
            i += nlen;
            j += strlen(&res[j]);
        } else if (strncmp(&tt[i], ENV_DEFAULT_VAR_SUFFIX, slen)==0) {
            sprintf(&res[j], "%s", suffix);
            i += slen;
            j += strlen(&res[j]);
        } else if (strncmp(&tt[i], ENV_DEFAULT_VAR_THIS_CLASS, tlen)==0) {
            sprintf(&res[j], "%s", thisclass);
            i += tlen;
            j += strlen(&res[j]);
        } else if (strncmp(&tt[i], ENV_DEFAULT_VAR_SUPER_CLASS, suplen)==0) {
            sprintf(&res[j], "%s", superclass);
            i += suplen;
            j += strlen(&res[j]);
        } else {
            res[j++] = tt[i++];
        }
        assert(j<MAX_OPTION_LEN);
    }
    res[j]=0;
    return(res);
}

static void expandEnvironmentVariables(char *tt, int ttsize, int *len,
                                       int envFlag) {
    char ttt[MAX_OPTION_LEN];
    char vname[MAX_OPTION_LEN];
    char *vval;
    int i, d, j, starti, termc, expanded, vlen, tilda;
    i = d = 0;
    //&fprintf(dumpOut, "expanding option '%s'\n", tt);
    while(tt[i] && i<ttsize-2) {
        starti = -1;
        expanded = 0;
        termc = 0;
        tilda = 0;
#if (!defined (__WIN32__))          /*SBD*/
        if (i==0 && tt[i]=='~' && tt[i+1]=='/') {starti = i; termc='~'; tilda=1;}
#endif                                                      /*SBD*/
        if (tt[i]=='$' && tt[i+1]=='{') {starti = i+2; termc='}';}
        if (tt[i]=='%') {starti = i+1; termc='%';}
        if (starti >= 0) {
            j = starti;
            while (isalpha(tt[j]) || isdigit(tt[j]) || tt[j]=='_') j++;
            if (j<ttsize-2 && tt[j]==termc) {
                vlen = j-starti;
                strncpy(vname, &tt[starti], vlen);
                vname[vlen]=0;
                vval = NULL;
                if (envFlag != GLOBAL_ENV_ONLY) {
                    vval = getXrefEnvironmentValue(vname);
                }
                if (vval==NULL) vval = getenv(vname);
#if (!defined (__WIN32__))          /*SBD*/
                if (tilda) vval = getenv("HOME");
#endif                                                      /*SBD*/
                if (vval != NULL) {
                    strcpy(&ttt[d], vval);
                    d += strlen(vval);
                    expanded = 1;
                }
                //& expanded = 1;
                if (expanded) i = j+1;
            }
        }
        if (expanded==0) {
            ttt[d++] = tt[i++];
        }
        assert(d<MAX_OPTION_LEN-2);
    }
    ttt[d] = 0;
    *len = d;
    strcpy(tt, ttt);
    //&fprintf(dumpOut, "result '%s'\n", tt);
}

static int getOptionFromFile(FILE *file, char *text, int text_size, int *chars_read) {
    int i, c;
    int comment, res, quotamess;
    int previous_char;
    UNUSED previous_char;

    c = getc(file);
    do {
        quotamess = 0;
        *chars_read = i = comment = 0;

        /* Skip all white space? */
        while ((c>=0 && c<=' ') || c=='\n' || c=='\t')
            c=getc(file);

        if (c==EOF) {
            res = EOF;
            goto fini;
        }

        if (c=='\"') {
            /* Read a double quoted string */
            previous_char=c; c=getc(file);
            // Escaping the double quote (\") is not allowed, it creates problems
            // when someone finished a section name by \ reverse slash
            while (c!=EOF && c!='\"') {
                if (i < text_size-1)
                    text[i++]=c;
                previous_char=c; c=getc(file);
            }
            if (c!='\"' && s_opt.taskRegime!=RegimeEditServer) {
                fatalError(ERR_ST, "option string through end of file", XREF_EXIT_ERR);
            }
        } else if (c=='`') {
            text[i++]=c;
            previous_char=c; c=getc(file);
            while (c!=EOF && c!='\n' && c!='`') {
                if (i < text_size-1)
                    text[i++]=c;
                previous_char=c; c=getc(file);
            }
            if (i < text_size-1)
                text[i++]=c;
            if (c!='`'  && s_opt.taskRegime!=RegimeEditServer) {
                error(ERR_ST, "option string through end of line");
            }
        } else if (c=='[') {
            text[i++] = c;
            previous_char=c; c=getc(file);
            while (c!=EOF && c!='\n' && (c!=']' /*|| lc=='\\'*/ )) {
                if (i < text_size-1)
                    text[i++]=c;
                previous_char=c; c=getc(file);
            }
            if (c==']')
                text[i++]=c;
        } else {
            while (c!=EOF && c>' ') {
                if (c=='\"')
                    quotamess = 1;
                if (i < text_size-1)
                    text[i++]=c;
                c=getc(file);
            }
        }
        text[i]=0;
        if (quotamess && s_opt.taskRegime!=RegimeEditServer) {
            static int messageWritten=0;
            if (! messageWritten) {
                messageWritten = 1;
                sprintf(tmpBuff,"option '%s' contains quotes.", text);
                warning(ERR_ST, tmpBuff);
            }
        }
        /* because QNX paths can start with // */
        if (i>=2 && text[0]=='/' && text[1]=='/') {
            while (c!=EOF && c!='\n')
                c=getc(file);
            comment = 1;
        }
    } while (comment);
    if (strcmp(text, END_OF_OPTIONS_STRING)==0) {
        i = 0; res = EOF;
    } else {
        res = 'A';
    }

 fini:
    text[i] = 0;
    *chars_read = i;

    return res;
}

static void processSingleSectionMarker(char *tt,char *section,
                                       int *writeFlag, char *resSection) {
    int sl,casesensitivity=1;
    sl = strlen(tt);
#if defined (__WIN32__)    /*SBD*/
    casesensitivity = 0;
#endif                                          /*SBD*/
    if (pathncmp(tt, section, sl, casesensitivity)==0
        && (section[sl]=='/' || section[sl]=='\\' || section[sl]==0)) {
        if (sl > strlen(resSection)) {
            strcpy(resSection,tt);
            assert(strlen(resSection)+1 < MAX_FILE_NAME_SIZE);
        }
        *writeFlag = 1;
    } else {
        *writeFlag = 0;
    }
}

static void processSectionMarker(char *ttt,int i,char *project,char *section,
                                 int *writeFlag, char *resSection) {
    char        *tt;
    char        firstPath[MAX_FILE_NAME_SIZE];

    ttt[i-1]=0;
    tt = ttt+1;
    firstPath[0]=0;
    log_debug("processing %s for file %s project==%s", tt, section, project);
#if 1
    *writeFlag = 0;
    JavaMapOnPaths(tt, {
            if (firstPath[0]==0) strcpy(firstPath, currentPath);
            if (project!=NULL) {
                if (strcmp(currentPath, project)==0) {
                    strcpy(resSection,currentPath);
                    assert(strlen(resSection)+1 < MAX_FILE_NAME_SIZE);
                    *writeFlag = 1;
                    goto fini;
                } else {
                    *writeFlag = 0;
                }
            } else {
                processSingleSectionMarker(currentPath, section, writeFlag, resSection);
                if (*writeFlag) goto fini;
            }
        });
#else
    strcpy(firstPath, tt);
    if (project!=NULL) {
        if (strcmp(tt, project)==0) {
            strcpy(resSection,tt);
            assert(strlen(resSection)+1 < MAX_FILE_NAME_SIZE);
            *writeFlag = 1;
        } else {
            *writeFlag = 0;
        }
    } else {
        processSingleSectionMarker(tt, section, writeFlag, resSection);
    }
#endif
 fini:;
    if (*writeFlag) {
        // TODO!!! YOU NEED TO ALLOCATE SPACE FOR THIS!!!
        strcpy(s_base, resSection);
        assert(strlen(resSection) < MAX_FILE_NAME_SIZE-1);
        xrefSetenv("__BASE", s_base);
        strcpy(resSection, firstPath);
        // completely wrong, what about file names from command line ?
        //&strncpy(s_cwd, resSection, MAX_FILE_NAME_SIZE-1);
        //&s_cwd[MAX_FILE_NAME_SIZE-1] = 0;
    }
}

#define ADD_OPTION_TO_ARGS(memFl,tt,len,argv,argc) {                    \
        char *cc;                                                       \
        cc=NULL;                                                        \
        if (memFl!=MEM_NO_ALLOC) {                                      \
            OPTION_SPACE_ALLOCC(memFl, cc, len+1, char);                \
            assert(cc);                                                 \
            strcpy(cc,tt);                                              \
            /*&fprintf(dumpOut,"option %s readed\n",cc); fflush(dumpOut);&*/ \
            argv[argc] = cc;                                            \
            if (argc < MAX_STD_ARGS-1) argc++;                          \
        }                                                               \
    }


#define OPTION_SPACE_ALLOCC(memFl,cc,num,type) {                        \
        if (memFl==MEM_ALLOC_ON_SM) {                                   \
            SM_ALLOCC(optMemory,cc,num,type);                           \
        } else if (memFl==MEM_ALLOC_ON_PP) {                            \
            PP_ALLOCC(cc,num,type);                                     \
        } else {                                                        \
            assert(0);                                                  \
        }                                                               \
    }

#define ACTIVE_OPTION() (isActiveSection && isActivePass)

bool readOptionFromFile(FILE *file, int *nargc, char ***nargv, int memFl,
                       char *sectionFile, char *project, char *resSection) {
    char text[MAX_OPTION_LEN];
    int len, argc, i, c, isActiveSection, isActivePass, passn=0;
    bool found = false;
    char **aargv,*argv[MAX_STD_ARGS];

    argc = 1; isActiveSection = isActivePass = 1; aargv=NULL;
    resSection[0]=0;
    if (memFl==MEM_ALLOC_ON_SM) SM_INIT(optMemory);
    c = 'a';
    while (c!=EOF) {
        c = getOptionFromFile(file, text, MAX_OPTION_LEN, &len);
        if (c==EOF) {
            log_trace("got option from file (@EOF): '%s'", text);
        } else {
            log_trace("got option from file: '%s'", text);
        }
        if (len>=2 && text[0]=='[' && text[len-1]==']') {
            log_trace("checking '%s'", text);
            expandEnvironmentVariables(text+1, MAX_OPTION_LEN, &len, GLOBAL_ENV_ONLY);
            log_trace("expanded '%s'", text);
            processSectionMarker(text,len+1,project,sectionFile,&isActiveSection,resSection);
        } else if (isActiveSection && strncmp(text,"-pass", 5) == 0) {
            sscanf(text+5, "%d", &passn);
            if (passn==s_currCppPass || s_currCppPass==ANY_CPP_PASS) {
                isActivePass = 1;
            } else {
                isActivePass = 0;
            }
            if (passn > s_maximalCppPass) s_maximalCppPass = passn;
        } else if (strcmp(text,"-set")==0 && ACTIVE_OPTION()
                   && memFl!=MEM_NO_ALLOC) {
            // pre-evaluation of -set
            found = true;
            ADD_OPTION_TO_ARGS(memFl,text,len,argv,argc);
            c = getOptionFromFile(file,text,MAX_OPTION_LEN,&len);
            expandEnvironmentVariables(text,MAX_OPTION_LEN,&len,DEFAULT_VALUE);
            ADD_OPTION_TO_ARGS(memFl,text,len,argv,argc);
            c = getOptionFromFile(file,text,MAX_OPTION_LEN,&len);
            expandEnvironmentVariables(text,MAX_OPTION_LEN,&len,DEFAULT_VALUE);
            ADD_OPTION_TO_ARGS(memFl,text,len,argv,argc);
            if (argc < MAX_STD_ARGS) {
                assert(argc>=3);
                mainHandleSetOption(argc, argv, argc-3);
            }
        } else if (c!=EOF && ACTIVE_OPTION()) {
            found = true;
            expandEnvironmentVariables(text,MAX_OPTION_LEN,&len,DEFAULT_VALUE);
            ADD_OPTION_TO_ARGS(memFl,text,len,argv,argc);
        }
    }
    if (argc >= MAX_STD_ARGS-1) error(ERR_ST,"too many options");
    if (found && memFl!=MEM_NO_ALLOC) {
        OPTION_SPACE_ALLOCC(memFl, aargv, argc, char*);
        for(i=1; i<argc; i++) aargv[i] = argv[i];
    }
    *nargc = argc;
    *nargv = aargv;

    return found;
}

void readOptionFile(char *name, int *nargc, char ***nargv, char *sectionFile, char *project) {
    FILE *ff;
    char realSection[MAX_FILE_NAME_SIZE];
    ff = fopen(name,"r");
    if (ff==NULL) fatalError(ERR_CANT_OPEN,name, XREF_EXIT_ERR);
    readOptionFromFile(ff,nargc,nargv,MEM_ALLOC_ON_PP,sectionFile, project, realSection);
    fclose(ff);
}

void readOptionPipe(char *comm, int *nargc, char ***nargv, char *sectionFile) {
    FILE *ff;
    char realSection[MAX_FILE_NAME_SIZE];
    ff = popen(comm, "r");
    if (ff==NULL) fatalError(ERR_CANT_OPEN, comm, XREF_EXIT_ERR);
    readOptionFromFile(ff,nargc,nargv,MEM_ALLOC_ON_PP,sectionFile,NULL,realSection);
    fclose(ff);
}

static char *getClassPath(bool defaultCpAllowed) {
    char *cp;
    cp = s_opt.classpath;
    if (cp == NULL || *cp==0) cp = getenv("CLASSPATH");
    if (cp == NULL || *cp==0) {
        if (defaultCpAllowed) cp = s_defaultClassPath;
        else cp = NULL;
    }
    return(cp);
}

void javaSetSourcePath(int defaultCpAllowed) {
    char *cp;
    cp = s_opt.sourcePath;
    if (cp == NULL || *cp==0) cp = getenv("SOURCEPATH");
    if (cp == NULL || *cp==0) cp = getClassPath(defaultCpAllowed);
    if (cp == NULL) {
        s_javaSourcePaths = NULL;
    } else {
        expandWildcardsInPaths(cp, s_javaSourcePathStatic, MAX_OPTION_LEN);
        s_javaSourcePaths = s_javaSourcePathStatic;
    }
}


static void processClassPathString(char *cp) {
    char            *nn, *np, *sfp;
    char            ttt[MAX_FILE_NAME_SIZE];
    S_stringList    **ll;
    int             ind, nlen;

    for (ll = &s_javaClassPaths; *ll != NULL; ll = &(*ll)->next) ;

    while (*cp!=0) {
        for(ind=0; cp[ind]!=0 && cp[ind]!=CLASS_PATH_SEPARATOR; ind++) {
            ttt[ind]=cp[ind];
        }
        ttt[ind]=0;
        np = normalizeFileName(ttt,s_cwd);
        nlen = strlen(np); sfp = np+nlen-4;
        if (nlen>=4 && (strcmp(sfp,".zip")==0 || strcmp(sfp,".jar")==0)){
            // probably zip archive
            log_debug("Indexing '%s'", np);
            zipIndexArchive(np);
            log_debug("Done.");
        } else {
            // just path
            PP_ALLOCC(nn, strlen(np)+1, char);
            strcpy(nn,np);
            PP_ALLOC(*ll, S_stringList);
            **ll = (S_stringList){.d = nn, .next = NULL};
            ll = &(*ll)->next;
        }
        cp += ind;
        if (*cp == CLASS_PATH_SEPARATOR) cp++;
    }
}

int packageOnCommandLine(char *fn) {
    char    *cp,*ss,*dd;
    char    ttt[MAX_FILE_NAME_SIZE];
    char    ppp[MAX_FILE_NAME_SIZE];
    struct stat     st;
    int             i, ind, stt, topCallFlag, res;
    void            *recursFlag;
    res = 0;
    for(dd=ppp,ss=fn; *ss; dd++,ss++) {
        if (*ss == '.') *dd = FILE_PATH_SEPARATOR;
        else *dd = *ss;
    }
    *dd = 0;
    assert(strlen(ppp)<MAX_FILE_NAME_SIZE-1);
    cp = s_javaSourcePaths;
    while (cp!=NULL && *cp!=0) {
        for(ind=0; cp[ind]!=0 && cp[ind]!=CLASS_PATH_SEPARATOR; ind++) {
            ttt[ind]=cp[ind];
        }
        ttt[ind] = 0;
        i = ind;
        if (i==0 || ttt[i-1]!=FILE_PATH_SEPARATOR) {
            ttt[i++] = FILE_PATH_SEPARATOR;
        }
        strcpy(ttt+i, ppp);
        assert(strlen(ttt)<MAX_FILE_NAME_SIZE-1);
        //&fprintf(dumpOut,"checking '%s'\n", ttt);
        stt = statb(ttt, &st);
        if (stt==0  && (st.st_mode & S_IFMT)==S_IFDIR) {
            // it is a package name, process all source files
            //&fprintf(dumpOut,"it is a package\n");
            res = 1;
            topCallFlag = 1;
            if (s_opt.recursivelyDirs) recursFlag = &topCallFlag;
            else recursFlag = NULL;
            dirInputFile(ttt,"",NULL,NULL,recursFlag,&topCallFlag);
        }
        cp += ind;
        if (*cp == CLASS_PATH_SEPARATOR) cp++;
    }
    return(res);
}

void addSourcePathsCut(void) {
    javaSetSourcePath(1);
    JavaMapOnPaths(s_javaSourcePaths,{
            addHtmlCutPath(currentPath);
        });
}


static char * canItBeJavaBinPath(char *ttt) {
    static char     res[MAX_FILE_NAME_SIZE];
    char            *np;
    struct stat     st;
    int             stt, len;
    np = normalizeFileName(ttt, s_cwd);
    len = strlen(np);
#if defined (__WIN32__)            /*SBD*/
    sprintf(res,"%s%cjava.exe", np, FILE_PATH_SEPARATOR);
#else                                                   /*SBD*/
    sprintf(res,"%s%cjava", np, FILE_PATH_SEPARATOR);
#endif                                                  /*SBD*/
    assert(len+6<MAX_FILE_NAME_SIZE);
    stt = stat(res, &st);
    if (stt==0  && (st.st_mode & S_IFMT)!=S_IFDIR) {
        res[len]=0;
        if (len>4 && fnCmp(res+len-3,"bin")==0 && res[len-4]==FILE_PATH_SEPARATOR) {
            sprintf(res+len-3,"jre%clib%crt.jar",FILE_PATH_SEPARATOR,FILE_PATH_SEPARATOR);
            assert(strlen(res)<MAX_FILE_NAME_SIZE-1);
            stt = stat(res,&st);
            if (stt==0) {
                return(res);
            }
        }
    }
    return(NULL);
}


static char *getJdk12AutoClassPathFastly(void) {
    char            ttt[MAX_FILE_NAME_SIZE];
    char            *res;
    char            *cp;
    int             len;
    cp = getenv("JAVA_HOME");
    if (cp!=NULL) {
        strcpy(ttt,cp);
        len = strlen(ttt);
        if (len>0 && (ttt[len-1]=='/' || ttt[len-1]=='\\')) len--;
        sprintf(ttt+len, "%cbin", FILE_PATH_SEPARATOR);
        res = canItBeJavaBinPath(ttt);
        if (res!=NULL) return(res);
    }
    cp = getenv("PATH");
    if (cp!=NULL) {
        JavaMapOnPaths(cp, {
                res = canItBeJavaBinPath(currentPath);
                if (res != NULL) return(res);
            });
    }
    return(NULL);
}

static char *getJdkClassPathFastly(void) {
    char *jdkcp;
    jdkcp = s_opt.jdkClassPath;
    if (jdkcp == NULL || *jdkcp==0) jdkcp = getenv("JDKCLASSPATH");
    if (jdkcp == NULL || *jdkcp==0) jdkcp = getJdk12AutoClassPathFastly();
    return(jdkcp);
}

static char *s_defaultPossibleJavaBinPaths[] = {
    "/usr/java/bin",
    "/usr/java/j2sdk1.4.3/bin",
    "/usr/java/j2sdk1.4.2_03/bin",
    "/usr/java/j2sdk1.4.2_02/bin",
    "/usr/java/j2sdk1.4.2_01/bin",
    "/usr/java/j2sdk1.4.2/bin",
    "/usr/java/j2sdk1.4.1_03/bin",
    "/usr/java/j2sdk1.4.1_02/bin",
    "/usr/java/j2sdk1.4.1_01/bin",
    "/usr/java/j2sdk1.4.1/bin",
    "/usr/java/j2sdk1.4.0_03/bin",
    "/usr/java/j2sdk1.4.0_02/bin",
    "/usr/java/j2sdk1.4.0_01/bin",
    "/usr/java/j2sdk1.4.0/bin",
    "/usr/java/j2sdk1.3.1_03/bin",
    "/usr/java/j2sdk1.3.1_02/bin",
    "/usr/java/j2sdk1.3.1_01/bin",
    "/usr/java/j2sdk1.3.1/bin",
    "/usr/java/j2sdk1.3.0/bin",
    "/usr/java1.2/bin",

    "C:\\j2sdk1.4.3\\bin",
    "C:\\j2sdk1.4.2_03\\bin",
    "C:\\j2sdk1.4.2_02\\bin",
    "C:\\j2sdk1.4.2_01\\bin",
    "C:\\j2sdk1.4.2\\bin",
    "C:\\j2sdk1.4.1_03\\bin",
    "C:\\j2sdk1.4.1_02\\bin",
    "C:\\j2sdk1.4.1_01\\bin",
    "C:\\j2sdk1.4.1\\bin",
    "C:\\j2sdk1.4.0_03\\bin",
    "C:\\j2sdk1.4.0_02\\bin",
    "C:\\j2sdk1.4.0_01\\bin",
    "C:\\j2sdk1.4.0\\bin",
    "C:\\j2sdk1.3.1_03\\bin",
    "C:\\j2sdk1.3.1_02\\bin",
    "C:\\j2sdk1.3.1_01\\bin",
    "C:\\j2sdk1.3.1\\bin",
    "C:\\j2sdk1.3.0\\bin",
    NULL
};

static char *getJdkClassPath(void) {
    char *res;
    int i;

    res = getJdkClassPathFastly();
    if (res!=NULL && *res!=0) return(res);
    // Can't determine it fastly, try other methods
    for(i=0; s_defaultPossibleJavaBinPaths[i]!=NULL; i++) {
        res = canItBeJavaBinPath(s_defaultPossibleJavaBinPaths[i]);
        if (res!=NULL && *res!=0) return(res);
    }
    return(NULL);
}

char *getJavaHome(void) {
    static char     res[MAX_FILE_NAME_SIZE];
    char            *tt;
    int             ii;

    tt = getJdkClassPath();
    if (tt!=NULL && *tt!=0) {
        copyDir(res, tt, &ii);
        if (ii>0) res[ii-1] = 0;
        copyDir(res, res, &ii);
        if (ii>0) res[ii-1] = 0;
        copyDir(res, res, &ii);
        if (ii>0) res[ii-1] = 0;
        return(res);
    }
    return(NULL);
}

void getJavaClassAndSourcePath(void) {
    char *cp, *jdkcp;
    int i;

    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES; i++) {
        if (s_zipArchiveTable[i].fn[0] == 0) break;
        s_zipArchiveTable[i].fn[0]=0;
    }

    // Keeping this comment as a historical artefact:
    // optimize wild char expand and getenv [5.2.2003]
    s_javaClassPaths = NULL;

    if (LANGUAGE(LANG_JAVA)) {
        javaSetSourcePath(0);

        if (s_javaSourcePaths==NULL) {
            if (LANGUAGE(LANG_JAVA)) {
                error(ERR_ST,"no classpath or sourcepath specified");
            }
            s_javaSourcePaths = s_defaultClassPath;
        }

        cp = getClassPath(true);
        expandWildcardsInPaths(cp, s_javaClassPathStatic, MAX_OPTION_LEN);
        cp = s_javaClassPathStatic;

        createOptionString(&s_opt.classpath, cp);  //??? why is this, only optimisation of getenv?
        processClassPathString(cp);
        jdkcp = getJdkClassPathFastly();
        if (jdkcp != NULL && *jdkcp!=0) {
            createOptionString(&s_opt.jdkClassPath, jdkcp);  //only optimisation of getenv?
            processClassPathString( jdkcp);
        }

        if (LANGUAGE(LANG_JAVA)
            && s_opt.taskRegime != RegimeEditServer
            && s_opt.taskRegime != RegimeGenerate
            ) {
            static int messageFlag=0;
            if (messageFlag==0 && ! s_opt.briefoutput) {
                if (s_opt.xref2) {
                    if (jdkcp!=NULL && *jdkcp!=0) {
                        sprintf(tmpBuff,"java runtime == %s", jdkcp);
                        ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
                    }
                    sprintf(tmpBuff,"classpath == %s", cp);
                    ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
                    sprintf(tmpBuff,"sourcepath == %s", s_javaSourcePaths);
                    ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
                } else {
                    if (jdkcp!=NULL && *jdkcp!=0) {
                        fprintf(dumpOut,"java runtime == %s\n", jdkcp);
                    }
                    fprintf(dumpOut,"classpath == %s\n", cp);
                    fprintf(dumpOut,"sourcepath == %s\n", s_javaSourcePaths);
                    fflush(dumpOut);
                }
                messageFlag = 1;
            }
        }
    }
}

int changeRefNumOption(int newRefNum) {
    int check;
    if (s_opt.refnum == 0) check=1;
    else if (s_opt.refnum == 1) check = (newRefNum <= 1);
    else check = (newRefNum == s_opt.refnum);
    s_opt.refnum = newRefNum;
    return(check);
}

void getXrefrcFileName(char *filename) {
    int hlen;
    char *hh;

    if (s_opt.xrefrc!=NULL) {
        sprintf(filename, "%s", normalizeFileName(s_opt.xrefrc, s_cwd));
        return;
    }
    hh = getenv("HOME");
#ifdef __WIN32__
    if (hh == NULL) hh = "c:\\";
#else
    if (hh == NULL) hh = "";
#endif
    hlen = strlen(hh);
    if (hlen>0 && (hh[hlen-1]=='/' || hh[hlen-1]=='\\')) {
        sprintf(filename, "%s%cc-xrefrc", hh, FILE_BEGIN_DOT);
    } else {
        sprintf(filename, "%s%c%cc-xrefrc", hh, FILE_PATH_SEPARATOR, FILE_BEGIN_DOT);
    }
    assert(strlen(filename) < MAX_FILE_NAME_SIZE-1);
}
