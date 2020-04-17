/* TODO: Make this independent on other modules */
#include "commons.h"

#include <errno.h>

#include "globals.h"
#include "misc.h"               /* ppcGenRecord() & ppcGenSynchroRecord() - extract ppc module? */
#include "yylex.h"              /* placeIdent() */
#include "main.h"               /* mainCloseOutputFile() - move here? */
#include "protocol.h"

#include "log.h"


void initCwd(void) {
    char *rr;

    rr = getcwd(s_cwd, MAX_FILE_NAME_SIZE);
    if (rr==NULL) {
        // try also with getenv, on some linuxes the statically linked
        // getcwd does not work.
        rr = getenv("PWD");
        if (rr==NULL) {
            error(ERR_ST, "can't get current working directory");
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
        && dffsect[1]==':' && dffsect[2]==SLASH
#else                   /*SBD*/
        && dffsect[0]==SLASH
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
#if defined (__WIN32__)    /*SBD*/
    } else if (name[0]=='\\' || name[0]=='/') {
        normalizedFileName[0] = relativeto[0]; normalizedFileName[1] = ':';
        l1 = 1;
    } else if (name[0]!=0 && name[1]==':') {
        normalizedFileName[0] = tolower(name[0]);      // normalize drive name
        normalizedFileName[1] = ':';
        l1 = 1;
        s1 = 2;
#else           /*SBD*/
    } else if (name[0] == SLASH) {
        l1 = -1;
#endif          /*SBD*/
    } else {
        if (l1+l2+2 >= MAX_FILE_NAME_SIZE) {
            l1 = -1;
        } else {
            strcpy(normalizedFileName,relativeto);
            if (! s_opt.fileNamesCaseSensitive) {
                for(ss=normalizedFileName; *ss; ss++) *ss = tolower(*ss);
            }
            if (l1>0 && normalizedFileName[l1-1] == SLASH) l1--;
            else normalizedFileName[l1]=SLASH;
        }
    }
    for(i=s1, j=l1+1; i<l2+1; ) {
        if (name[i]=='.' && (name[i+1]==SLASH||name[i+1]=='/')) {i+=2;
        } else if (name[i]=='.' && name[i+1]=='.' && (name[i+2]==SLASH||name[i+2]=='/')) {
            for(j-=2; j>=0 && normalizedFileName[j]!=SLASH && normalizedFileName[j]!='/'; j--) ;
            i+=3; j++;
            if (j==0) j++;
        } else {
            for(; name[i]!=0 && name[i]!=SLASH && name[i]!='/'; i++,j++) {
                normalizedFileName[j]=name[i];
                if (normalizedFileName[j]==ZIP_SEPARATOR_CHAR) inzip=1;
                if ((!inzip) && ! s_opt.fileNamesCaseSensitive) {
                    normalizedFileName[j]=tolower(normalizedFileName[j]);
                }
            }
            normalizedFileName[j]=name[i];
            if (normalizedFileName[j]=='/' && !inzip) normalizedFileName[j]=SLASH;
            i++; j++;
            if (i<l2+1 && (name[i]=='/' || name[i]==SLASH) && !inzip) i++;
        }
    }
    log_trace("returning %s",normalizedFileName);
    if (j>=2 && normalizedFileName[j-2]==SLASH && !inzip) normalizedFileName[j-2]=0;
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

void copyFile(char *source, char *destination) {
    FILE *sourceFile, *destinationFile;
    int readBytes, writtenBytes;

    ENTER();
    log_trace("attempting to copy '%s' to '%s'", source, destination);
    sourceFile = fopen(source,"r");
    if (sourceFile == NULL) {
        error(ERR_CANT_OPEN_FOR_READ, source);
        LEAVE();
        return;
    }
    destinationFile = fopen(destination,"w");
    if (destinationFile == NULL) {
        error(ERR_CANT_OPEN_FOR_WRITE, destination);
        fclose(sourceFile);
        LEAVE();
        return;
    }
    do {
        readBytes = fread(tmpBuff , 1, TMP_BUFF_SIZE, sourceFile);
        writtenBytes = fwrite(tmpBuff, 1, readBytes, destinationFile);
        if (readBytes != writtenBytes)
            error(ERR_ST,"problem with writing to a file.");
    } while (readBytes > 0);
    fclose(destinationFile);
    fclose(sourceFile);
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

static void errorMessage(char *out, int errCode, char *mess) {
    if (s_opt.taskRegime != RegimeEditServer) {
        sprintf(out,"%s ", placeIdent());
        out += strlen(out);
    }
    switch (errCode) {
    case ERR_CANT_OPEN:
        sprintf(out,"can't open file %s : %s\n", mess, strerror(errno));
        out += strlen(out);
        break;
    case ERR_CANT_OPEN_FOR_READ:
        sprintf(out,"can't open file %s for reading : %s\n", mess, strerror(errno));
        out += strlen(out);
        break;
    case ERR_CANT_OPEN_FOR_WRITE:
        sprintf(out,"can't open file %s for writing : %s\n", mess, strerror(errno));
        out += strlen(out);
        break;
    case ERR_NO_MEMORY:
        sprintf(out,"sorry, memory %s overflowed over borne\n",mess);
        out += strlen(out);
        sprintf(out,"\tread the TROUBLES section of the README file.\n");
        out += strlen(out);
        break;
    case ERR_INTERNAL:
        sprintf(out,"internal error, %s\n",mess);
        out += strlen(out);
        break;
    case ERR_INTERNAL_CHECK:
        sprintf(out,"internal check %s\n",mess);
        out += strlen(out);
        break;
    case ERR_CFG:
        sprintf(out,"a problem while reading config record %s\n",mess);
        out += strlen(out);
        break;
    default:
        sprintf(out,"%s\n",mess);
        out += strlen(out);
        break;
    }
    assert(strlen(ppcTmpBuff) < MAX_PPC_RECORD_SIZE-1);
}

void warning(int errCode, char *mess) {
    if ((! s_opt.noErrors) && (! s_javaPreScanOnly)) {
        if (! s_opt.xref2) fprintf(errOut,"![warning] ");
        errorMessage(ppcTmpBuff,errCode, mess);
        if (s_opt.xref2) {
            ppcGenRecord(PPC_WARNING, ppcTmpBuff,"\n");
        } else {
            log_warning("%s", ppcTmpBuff);
            fprintf(errOut, "%s", ppcTmpBuff);
            fflush(errOut);
        }
    }
}

static void writeErrorMessage(int errCode, char *mess) {
    if (! s_opt.xref2) fprintf(errOut,"![error] ");
    errorMessage(ppcTmpBuff,errCode, mess);
    if (s_opt.xref2) {
        ppcGenRecord(PPC_ERROR, ppcTmpBuff,"\n");
    } else {
        log_error("%s", ppcTmpBuff);
        fprintf(errOut, "%s", ppcTmpBuff);
        fflush(errOut);
    }
}

void error(int errCode, char *mess) {
    if ((! s_opt.noErrors) && (! s_javaPreScanOnly)) {
        writeErrorMessage(errCode, mess);
    }
}

void emergencyExit(int exitStatus) {
    mainCloseOutputFile();
    if (s_opt.xref2) {
        ppcGenSynchroRecord();
    }
    exit(exitStatus);
}


void fatalError(int errCode, char *mess, int exitStatus) {
    if (! s_opt.xref2) fprintf(errOut,"![error] ");
    errorMessage(ppcTmpBuff, errCode, mess);
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
            mainCloseOutputFile();
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
