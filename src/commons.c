/* TODO: Make this independent on other modules */
#include "commons.h"

#include <errno.h>

#include "globals.h"
#include "misc.h"               /* ppcGenRecord() & ppcGenSynchroRecord() - extract ppc module? */
#include "yylex.h"              /* placeIdent() */
#include "protocol.h"
#include "semact.h"             /* displayingErrorMessages() */
#include "fileio.h"

#include "log.h"


void closeMainOutputFile(void) {
    if (ccOut!=stdout) {
        //&fprintf(dumpOut,"CLOSING OUTPUT FILE\n");
        closeFile(ccOut);
        ccOut = stdout;
    }
    errOut = ccOut;
    dumpOut = ccOut;
}

void initCwd(void) {
    char *rr;

    rr = getcwd(s_cwd, MAX_FILE_NAME_SIZE);
    if (rr==NULL) {
        // try also with getenv, on some linuxes the statically linked
        // getcwd does not work.
        rr = getenv("PWD");
        if (rr==NULL) {
            errorMessage(ERR_ST, "can't get current working directory");
            sprintf(s_cwd, ".");
        } else {
            assert(strlen(rr) < MAX_FILE_NAME_SIZE);
            strcpy(s_cwd, rr);
        }
    }
#if defined (__WIN32__)                         /*SBD*/
    if (strlen(s_cwd)<=2 || s_cwd[1]!=':') {
        // starting by drive specification
        sprintf(nid,"%c:",tolower('c'));
        if (strlen(nid)+strlen(s_cwd) < MAX_FILE_NAME_SIZE-1) {
            strcpy(nid+strlen(nid),s_cwd);
            strcpy(s_cwd,nid);
        }
    }
    strcpy(s_cwd, normalizeFileName(s_cwd, "c:\\"));
#else                   /*SBD*/
    strcpy(s_cwd, normalizeFileName(s_cwd, "/"));
#endif                  /*SBD*/
}

void reInitCwd(char *dffname, char *dffsect) {
    int ii;
    if (dffname[0]!=0) {
        copyDir(s_cwd, dffname, &ii);
    }
    if (dffsect[0]!=0
#if defined (__WIN32__)    /*SBD*/
        && dffsect[1]==':' && dffsect[2]==FILE_PATH_SEPARATOR
#else                   /*SBD*/
        && dffsect[0]==FILE_PATH_SEPARATOR
#endif                  /*SBD*/
        ) {
        strcpy(s_cwd, dffsect);
    }
}

/* this is the the number 1 of program hacking */
char *normalizeFileName(char *name, char *relativeto) {
    static char normalizedFileName[MAX_FILE_NAME_SIZE];
    int l1,l2,i,j,s1,inzip=0;
    char *ss;
    /*fprintf(dumpOut,"normalizing %s  (%s)\n",name,s_cwd); fflush(dumpOut);*/
    l1 = strlen(relativeto);
    l2 = strlen(name);
    s1 = 0;
    if (name[0] == ZIP_SEPARATOR_CHAR) {
        // special case a class name
        l1 = -1;
        inzip = 1;
#if defined (__WIN32__)
    } else if (name[0]=='\\' || name[0]=='/') {
        normalizedFileName[0] = relativeto[0]; normalizedFileName[1] = ':';
        l1 = 1;
    } else if (name[0]!=0 && name[1]==':') {
        normalizedFileName[0] = tolower(name[0]);      // normalize drive name
        normalizedFileName[1] = ':';
        l1 = 1;
        s1 = 2;
#else
    } else if (name[0] == FILE_PATH_SEPARATOR) {
        l1 = -1;
#endif
    } else {
        if (l1+l2+2 >= MAX_FILE_NAME_SIZE) {
            l1 = -1;
        } else {
            strcpy(normalizedFileName,relativeto);
            if (! s_opt.fileNamesCaseSensitive) {
                for(ss=normalizedFileName; *ss; ss++) *ss = tolower(*ss);
            }
            if (l1>0 && normalizedFileName[l1-1] == FILE_PATH_SEPARATOR) l1--;
            else normalizedFileName[l1]=FILE_PATH_SEPARATOR;
        }
    }
    for(i=s1, j=l1+1; i<l2+1; ) {
        if (name[i]=='.' && (name[i+1]==FILE_PATH_SEPARATOR||name[i+1]=='/')) {i+=2;
        } else if (name[i]=='.' && name[i+1]=='.' && (name[i+2]==FILE_PATH_SEPARATOR||name[i+2]=='/')) {
            for(j-=2; j>=0 && normalizedFileName[j]!=FILE_PATH_SEPARATOR && normalizedFileName[j]!='/'; j--) ;
            i+=3; j++;
            if (j==0) j++;
        } else {
            for(; name[i]!=0 && name[i]!=FILE_PATH_SEPARATOR && name[i]!='/'; i++,j++) {
                normalizedFileName[j]=name[i];
                if (normalizedFileName[j]==ZIP_SEPARATOR_CHAR) inzip=1;
                if ((!inzip) && ! s_opt.fileNamesCaseSensitive) {
                    normalizedFileName[j]=tolower(normalizedFileName[j]);
                }
            }
            normalizedFileName[j]=name[i];
            if (normalizedFileName[j]=='/' && !inzip) normalizedFileName[j]=FILE_PATH_SEPARATOR;
            i++; j++;
            if (i<l2+1 && (name[i]=='/' || name[i]==FILE_PATH_SEPARATOR) && !inzip) i++;
        }
    }
    log_trace("returning %s",normalizedFileName);
    if (j>=2 && normalizedFileName[j-2]==FILE_PATH_SEPARATOR && !inzip) normalizedFileName[j-2]=0;
    if (strlen(normalizedFileName) >= MAX_FILE_NAME_SIZE) {
        sprintf(tmpBuff, "file name %s is too long", normalizedFileName);
        fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
    }
    return normalizedFileName;  /* A static variable! */
}


char *create_temporary_filename(void) {
    static char temporary_name[MAX_FILE_NAME_SIZE] = "";
#ifdef __WIN32__
    char *temp_dir;

    // under Windows tmpnam returns file names in \ root.
    static int count = 0;
    temp_dir = getenv("TEMP");
    if (temp_dir == NULL) {
        temp_dir = tmpnam(NULL);
        strcpy(temporary_name, temp_dir);
    } else {
        sprintf(temporary_name,"%s\\xrefu%d.tmp", temp_dir, count++);
        strcpy(temporary_name, normalizeFileName(temporary_name, s_cwd));
    }
    assert(strlen(temporary_name)+1 < MAX_FILE_NAME_SIZE);
#else
    if (getenv("TMPDIR") == NULL)
        strcpy(temporary_name, "/tmp/c-xref-temp-XXXXXX");
    else
        sprintf(temporary_name, "%s/c-xref-temp-XXXXXX", getenv("TMPDIR"));

    /* Create and open a temporary file with safe mkstemp(), then
       close it in order to stay with the semantics of this
       function */
    int fd = mkstemp(temporary_name);
    if (fd == -1)
        /* Error? Use only the base, this will not work well... */
        strcpy(temporary_name, "/tmp/c-xref-temp");
    else
        close(fd);
#endif                  /*SBD*/
    //&fprintf(dumpOut,"temp file: %s\n", temporary_name);
    if (strlen(temporary_name) == 0)
        fatalError(ERR_ST, "couldn't create temporary file name", XREF_EXIT_ERR);

    return temporary_name;
}

void copyFileFromTo(char *source, char *destination) {
    FILE *sourceFile, *destinationFile;
    int readBytes, writtenBytes;

    ENTER();
    log_trace("attempting to copy '%s' to '%s'", source, destination);
    sourceFile = openFile(source,"r");
    if (sourceFile == NULL) {
        errorMessage(ERR_CANT_OPEN_FOR_READ, source);
        LEAVE();
        return;
    }
    destinationFile = openFile(destination,"w");
    if (destinationFile == NULL) {
        errorMessage(ERR_CANT_OPEN_FOR_WRITE, destination);
        closeFile(sourceFile);
        LEAVE();
        return;
    }
    do {
        readBytes = readFile(tmpBuff , 1, TMP_BUFF_SIZE, sourceFile);
        writtenBytes = writeFile(tmpBuff, 1, readBytes, destinationFile);
        if (readBytes != writtenBytes)
            errorMessage(ERR_ST,"problem with writing to a file.");
    } while (readBytes > 0);
    closeFile(destinationFile);
    closeFile(sourceFile);
    LEAVE();
}

void createDir(char *dirname) {
#ifdef __WIN32__                        /*SBD*/
    mkdir(dirname);
#else                                   /*SBD*/
    mkdir(dirname,0777);
#endif                                  /*SBD*/
}

void removeFile(char *dirname) {
    log_trace("removing file '%s'", dirname);
    unlink(dirname);
}

/*  'dest' and 's' can be the same pointer !!!!!!!!!!!!!!! */
void copyDir(char *dest, char *s, int *i) {
    int ii;
    *i = 0;
    for(ii=0; s[ii]!=0; ii++) {
        dest[ii] = s[ii];
        if (s[ii]=='/' || s[ii]=='\\') *i = ii+1;
    }
    dest[*i] = 0;
}

/* ***********************************************************************
 */

static void formatMessage(char *out, int errCode, char *mess) {
    if (s_opt.taskRegime != RegimeEditServer) {
        sprintf(out, "%s ", placeIdent());
        out += strlen(out);
    }
    switch (errCode) {
    case ERR_CANT_OPEN:
        sprintf(out, "can't open file %s : %s", mess, strerror(errno));
        break;
    case ERR_CANT_OPEN_FOR_READ:
        sprintf(out, "can't open file %s for reading : %s", mess, strerror(errno));
        break;
    case ERR_CANT_OPEN_FOR_WRITE:
        sprintf(out, "can't open file %s for writing : %s", mess, strerror(errno));
        break;
    case ERR_NO_MEMORY:
        sprintf(out, "memory %s overflowed, read TROUBLES section of README file.", mess);
        break;
    case ERR_INTERNAL:
        sprintf(out, "internal error, %s", mess);
        break;
    case ERR_INTERNAL_CHECK:
        sprintf(out, "internal check %s", mess);
        break;
    case ERR_CFG:
        sprintf(out, "a problem while reading config record %s", mess);
        break;
    default:
        sprintf(out, "%s", mess);
        break;
    }
    out += strlen(out);
    assert(strlen(ppcTmpBuff) < MAX_PPC_RECORD_SIZE-1);
}

void warningMessage(int errCode, char *message) {
    if ((! s_opt.noErrors) && (! s_javaPreScanOnly)) {
        formatMessage(ppcTmpBuff, errCode, message);
        if (s_opt.xref2) {
            strcat(ppcTmpBuff, "\n");
            ppcGenRecord(PPC_WARNING, ppcTmpBuff,"\n");
        } else {
            if (displayingErrorMessages())
                log_warning("%s", ppcTmpBuff);
            else {
                fprintf(errOut,"![warning] %s\n", ppcTmpBuff);
                fflush(errOut);
            }
        }
    }
}

static void writeErrorMessage(int errCode, char *mess) {
    formatMessage(ppcTmpBuff, errCode, mess);
    if (s_opt.xref2) {
        strcat(ppcTmpBuff, "\n");
        ppcGenRecord(PPC_ERROR, ppcTmpBuff, "\n");
    } else {
        if (displayingErrorMessages())
            log_error("%s", ppcTmpBuff);
        else {
            fprintf(errOut,"![error] %s\n", ppcTmpBuff);
            fflush(errOut);
        }
    }
}

void errorMessage(int errCode, char *mess) {
    if ((! s_opt.noErrors) && (! s_javaPreScanOnly)) {
        writeErrorMessage(errCode, mess);
    }
}

void emergencyExit(int exitStatus) {
    closeMainOutputFile();
    if (s_opt.xref2) {
        ppcGenSynchroRecord();
    }
    exit(exitStatus);
}


void fatalError(int errCode, char *mess, int exitStatus) {
    if (! s_opt.xref2) fprintf(errOut,"![error] ");
    formatMessage(ppcTmpBuff, errCode, mess);
    log_error(ppcTmpBuff);
    if (s_opt.xref2) {
        ppcGenRecord(PPC_FATAL_ERROR, ppcTmpBuff,"\n");
    } else {
        fprintf(errOut, "%s", ppcTmpBuff);
        fprintf(errOut,"\t exiting\n");
        fflush(errOut);
    }
    emergencyExit(exitStatus);
}

void internalCheckFail(char *expr, char *file, int line) {
    char msg[TMP_BUFF_SIZE];

    if (errOut == NULL) errOut = stderr;
    sprintf(msg,"'%s' is not true in '%s:%d'",expr,file,line);
    log_fatal(msg);
    writeErrorMessage(ERR_INTERNAL_CHECK,msg);
    if (s_opt.taskRegime == RegimeEditServer || s_opt.refactoringRegime == RegimeRefactory) {
        if (s_opt.xref2) {
            ppcGenRecord(PPC_INFORMATION,"Exiting","\n");
            closeMainOutputFile();
            ppcGenSynchroRecord();
        } else {
            fprintf(errOut, "\t exiting!\n"); fflush(stderr);
        }
    }
    if (s_opt.taskRegime == RegimeEditServer
        || s_opt.refactoringRegime == RegimeRefactory
        || s_fileAbortionEnabled == 0
        ) {
        emergencyExit(XREF_EXIT_ERR);
    }
    fprintf(errOut, "\t file aborted!\n"); fflush(errOut);
    // longjump is causing problems with refactory, the longjmp
    // is missplaced
    longjmp(cxmemOverflow,MESS_FILE_ABORT);
}
