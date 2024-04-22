/* TODO: Make this independent on other modules */
#include "commons.h"

#include <errno.h>

#include "globals.h"
#include "options.h"
#include "yylex.h"              /* placeIdent() */
#include "protocol.h"
#include "semact.h"             /* displayingErrorMessages() */

/* These are ok to be dependent on */
#include "filedescriptor.h"
#include "fileio.h"
#include "log.h"
#include "stringlist.h"
#include "misc.h"
#include "ppc.h"


void openOutputFile(char *outfile) {
    closeMainOutputFile();
    if (outfile!=NULL) {
        log_trace("Opening output file '%s'", options.outputFileName);
#if defined (__WIN32__)
        // open it as binary file, so that record lengths will be correct
        communicationChannel = openFile(outfile, "wb");
#else
        communicationChannel = openFile(outfile, "w");
#endif
    } else {
        communicationChannel = stdout;
    }
    if (communicationChannel == NULL) {
        errorMessage(ERR_CANT_OPEN, outfile);
        communicationChannel = stdout;
    }
    errOut = communicationChannel;
}

void closeMainOutputFile(void) {
    if (communicationChannel!=stdout) {
        //&fprintf(dumpOut,"CLOSING OUTPUT FILE\n");
        closeFile(communicationChannel);
        communicationChannel = stdout;
    }
    errOut = communicationChannel;
}

void initCwd(void) {
    char *returnedCwd;

    returnedCwd = getCwd(cwd, MAX_FILE_NAME_SIZE);
    if (returnedCwd==NULL) {
        // Then try with getenv, on some linuxes the statically linked
        // getcwd does not work. TODO: Probably not nowadays
        returnedCwd = getEnv("PWD");
        if (returnedCwd==NULL) {
            errorMessage(ERR_ST, "can't get current working directory");
            sprintf(cwd, ".");
        } else {
            assert(strlen(returnedCwd) < MAX_FILE_NAME_SIZE);
            strcpy(cwd, returnedCwd);
        }
    }
#ifdef __WIN32__
    if (strlen(cwd)<=2 || cwd[1]!=':') {
        // starts with drive specification
        sprintf(nid,"%c:",tolower('c'));
        if (strlen(nid)+strlen(cwd) < MAX_FILE_NAME_SIZE-1) {
            strcpy(nid+strlen(nid),cwd);
            strcpy(cwd,nid);
        }
    }
    strcpy(cwd, normalizeFileName(cwd, "c:\\"));
#else
    strcpy(cwd, normalizeFileName(cwd, "/"));
#endif
}

void reInitCwd(char *dffname, char *dffsect) {
    if (dffname[0]!=0) {
        extractPathInto(dffname, cwd);
    }
    if (dffsect[0]!=0
#ifdef __WIN32__
        && dffsect[1]==':' && dffsect[2]==FILE_PATH_SEPARATOR
#else
        && dffsect[0]==FILE_PATH_SEPARATOR
#endif
        ) {
        strcpy(cwd, dffsect);
    }
}

/* this is the the number 1 of program hacking */
/* Returns: a pointer to a static area! */
char *normalizeFileName(char *name, char *relative_to) {
    static char normalizedFileName[MAX_FILE_NAME_SIZE];
    int l1,l2,i,j,s1;
    char *ss;

    log_trace("normalizing %s relative to %s (cwd=%s)", name, relative_to, cwd);
    l1 = strlen(relative_to);
    l2 = strlen(name);
    s1 = 0;
#ifdef __WIN32__
    if (name[0]=='\\' || name[0]=='/') {
        normalizedFileName[0] = relative_to[0];
        normalizedFileName[1] = ':';
        l1 = 1;
    } else if (name[0]!=0 && name[1]==':') {
        normalizedFileName[0] = tolower(name[0]);      // normalize drive name
        normalizedFileName[1] = ':';
        l1 = 1;
        s1 = 2;
#else
    if (name[0] == FILE_PATH_SEPARATOR) {
        l1 = -1;
#endif
    } else {
        if (l1+l2+2 >= MAX_FILE_NAME_SIZE) {
            l1 = -1;
        } else {
            strcpy(normalizedFileName,relative_to);
            if (!options.fileNamesCaseSensitive) {
                for(ss=normalizedFileName; *ss; ss++)
                    *ss = tolower(*ss);
            }
            if (l1>0 && normalizedFileName[l1-1] == FILE_PATH_SEPARATOR) l1--;
            else normalizedFileName[l1]=FILE_PATH_SEPARATOR;
        }
    }
    for(i=s1, j=l1+1; i<l2+1; ) {
        if (name[i]=='.' && (name[i+1]==FILE_PATH_SEPARATOR||name[i+1]=='/')) {
            i+=2;
        } else if (name[i]=='.' && name[i+1]=='.' && (name[i+2]==FILE_PATH_SEPARATOR||name[i+2]=='/')) {
            for(j-=2; j>=0 && normalizedFileName[j]!=FILE_PATH_SEPARATOR && normalizedFileName[j]!='/'; j--)
                ;
            i+=3;
            j++;
            if (j==0)
                j++;
        } else {
            for(; name[i]!=0 && name[i]!=FILE_PATH_SEPARATOR && name[i]!='/'; i++,j++) {
                normalizedFileName[j]=name[i];
                if (!options.fileNamesCaseSensitive) {
                    normalizedFileName[j]=tolower(normalizedFileName[j]);
                }
            }
            normalizedFileName[j]=name[i];
            if (normalizedFileName[j]=='/')
                normalizedFileName[j]=FILE_PATH_SEPARATOR;
            i++;
            j++;
            if (i<l2+1 && (name[i]=='/' || name[i]==FILE_PATH_SEPARATOR))
                i++;
        }
    }
    log_trace("returning %s", normalizedFileName);
    if (j>=2 && normalizedFileName[j-2]==FILE_PATH_SEPARATOR)
        normalizedFileName[j-2]=0;
    if (strlen(normalizedFileName) >= MAX_FILE_NAME_SIZE) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "file name %s is too long", normalizedFileName);
        FATAL_ERROR(ERR_ST, tmpBuff, XREF_EXIT_ERR);
    }
    return normalizedFileName;  /* A static variable! */
}


static StringList *temporary_filenames = NULL;
static bool registered = false;

static void delete_all_temporary_files(void) {
    /* This is only done when exiting so we don't care about freeing the list nodes */
    for (StringList *list = temporary_filenames; list != NULL; list = list->next)
        unlink(list->string);
}

char *create_temporary_filename(void) {
    static char temporary_name[MAX_FILE_NAME_SIZE] = "";
#ifdef __WIN32__
    char *temp_dir;

    // under Windows tmpnam returns file names in \ root.
    static int count = 0;
    temp_dir = getEnv("TEMP");
    if (temp_dir == NULL) {
        temp_dir = tmpnam(NULL);
        strcpy(temporary_name, temp_dir);
    } else {
        sprintf(temporary_name,"%s\\xrefu%d.tmp", temp_dir, count++);
        strcpy(temporary_name, normalizeFileName(temporary_name, cwd));
    }
    assert(strlen(temporary_name)+1 < MAX_FILE_NAME_SIZE);
#else
    if (getEnv("TMPDIR") == NULL)
        strcpy(temporary_name, "/tmp/c-xref-temp-XXXXXX");
    else
        sprintf(temporary_name, "%s/c-xref-temp-XXXXXX", getEnv("TMPDIR"));

    /* Create and open a temporary file with safe mkstemp(), then
       close it in order to stay with the semantics of this
       function */
    int fd = mkstemp(temporary_name);
    if (fd == -1)
        /* Error? Use only the base, this will not work well... */
        strcpy(temporary_name, "/tmp/c-xref-temp");
    else
        close(fd);
#endif
    log_trace("Created temporary filename: %s", temporary_name);
    if (strlen(temporary_name) == 0)
        FATAL_ERROR(ERR_ST, "couldn't create temporary file name", XREF_EXIT_ERR);

    // Remember to remove this file at exit
    temporary_filenames = newStringList(temporary_name, temporary_filenames);
    if (!registered) {
      atexit(delete_all_temporary_files);
      registered = true;
    }

    return temporary_name;
}

void copyFileFromTo(char *source, char *destination) {
    FILE *sourceFile, *destinationFile;
    int readBytes, writtenBytes;
    char tmpBuff[TMP_BUFF_SIZE];

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
        readBytes = readFile(sourceFile, tmpBuff, 1, TMP_BUFF_SIZE);
        writtenBytes = writeFile(destinationFile, tmpBuff, 1, readBytes);
        if (readBytes != writtenBytes)
            errorMessage(ERR_ST, "problem copying file.");
    } while (readBytes > 0);
    closeFile(destinationFile);
    closeFile(sourceFile);
    LEAVE();
}

/*  'dest' and 'source' might be the same pointer !!!!!!!!!!!!!!! */
int extractPathInto(char *source, char *dest) {
    int l = 0;

    for (int i=0; source[i]!=0; i++) {
        dest[i] = source[i];
        if (source[i]=='/' || source[i]=='\\')
            l = i+1;
    }
    dest[l] = 0;

    return l;
}

/*************************************************************************/

static char *identifierReference_static(void) {
    static char tempString[2 * MAX_REF_LEN];
    char        fileName[MAX_FILE_NAME_SIZE];
    char        fileNameAndLineNumber[MAX_REF_LEN];
    if (currentFile.fileName != NULL) {
        if (options.xref2 && options.mode != ServerMode) {
            strcpy(fileName, getRealFileName_static(normalizeFileName(currentFile.fileName, cwd)));
            assert(strlen(fileName) < MAX_FILE_NAME_SIZE);
            sprintf(fileNameAndLineNumber, "%s:%d", simpleFileName(fileName), currentFile.lineNumber);
            assert(strlen(fileNameAndLineNumber) < MAX_REF_LEN);
            sprintf(tempString, "<A HREF=\"file://%s#%d\" %s=%ld>%s</A>", fileName, currentFile.lineNumber, PPCA_LEN,
                    (unsigned long)strlen(fileNameAndLineNumber), fileNameAndLineNumber);
        } else {
            sprintf(tempString, "%s:%d ", simpleFileName(getRealFileName_static(currentFile.fileName)),
                    currentFile.lineNumber);
        }
        int length = strlen(tempString);
        assert(length < MAX_REF_LEN);
        return tempString;
    }
    return "";
}

static void formatMessage(char *out, int errCode, char *text) {
    if (options.mode != ServerMode) {
        sprintf(out, "%s ", identifierReference_static());
        out += strlen(out);
    }
    switch (errCode) {
    case ERR_CANT_OPEN:
        sprintf(out, "Can't open file %s : %s", text, strerror(errno));
        break;
    case ERR_CANT_OPEN_FOR_READ:
        sprintf(out, "Can't open file %s for reading : %s", text, strerror(errno));
        break;
    case ERR_CANT_OPEN_FOR_WRITE:
        sprintf(out, "Can't open file %s for writing : %s", text, strerror(errno));
        break;
    case ERR_NO_MEMORY:
        sprintf(out, "Memory %s overflowed, read TROUBLES section of README file.", text);
        break;
    case ERR_INTERNAL:
        sprintf(out, "Internal error, %s", text);
        break;
    case ERR_INTERNAL_CHECK:
        sprintf(out, "Internal check %s failed", text);
        break;
    case ERR_CFG:
        sprintf(out, "Problem while reading config record %s", text);
        break;
    default:
        sprintf(out, "%s", text);
        break;
    }
    out += strlen(out);
}

void warningMessage(int errCode, char *message) {
    char buffer[MAX_PPC_RECORD_SIZE];

    if (!options.noErrors && !javaPreScanOnly) {
        formatMessage(buffer, errCode, message);
        if (options.xref2) {
            strcat(buffer, "\n");
            ppcGenRecord(PPC_WARNING, buffer);
        } else {
            if (displayingErrorMessages())
                log_warn("%s", buffer);
        }
    }
}

void infoMessage(char message[]) {
    if (options.xref2) {
        ppcGenRecord(PPC_INFORMATION, message);
    } else {
        log_info(message);
    }
}


static void writeErrorMessage(int errorCode, char *message) {
    char buffer[MAX_PPC_RECORD_SIZE];

    formatMessage(buffer, errorCode, message);
    if (options.xref2) {
        strcat(buffer, "\n");
        ppcGenRecord(PPC_ERROR, buffer);
    } else {
        if (displayingErrorMessages())
            log_error("%s", buffer);
    }
}

void errorMessage(int errCode, char *mess) {
    if (!options.noErrors && !javaPreScanOnly) {
        writeErrorMessage(errCode, mess);
    }
}

static void emergencyExit(int exitStatus) {
    closeMainOutputFile();
    if (options.xref2) {
        ppcSynchronize();
    }
    exit(exitStatus);
}


void fatalError(int errorCode, char *message, int exitStatus, char *file, int line) {
    char buffer[MAX_PPC_RECORD_SIZE];

    formatMessage(buffer, errorCode, message);
    if (options.xref2) {
        ppcGenRecord(PPC_FATAL_ERROR, buffer);
    } else {
        log_with_explicit_file_and_line(LOG_ERROR, file, line, buffer);
    }
    emergencyExit(exitStatus);
}

void internalCheckFail(char *expr, char *file, int line) {
    char msg[TMP_BUFF_SIZE];

    if (errOut == NULL)
        errOut = stderr;
    sprintf(msg,"'%s' is not true in '%s:%d'", expr, file, line);
    log_with_explicit_file_and_line(LOG_FATAL, file, line, "'%s' is not true",  expr);
    writeErrorMessage(ERR_INTERNAL_CHECK,msg);

    // Asserts to explore if options.refactoringMode is actually needed...
    if (options.refactoringMode == RefactoryMode)
        assert(options.mode == RefactoryMode);

    if (options.mode == ServerMode || options.refactoringMode == RefactoryMode) {
        if (options.xref2) {
            ppcGenRecord(PPC_INFORMATION,"Exiting");
            closeMainOutputFile();
            ppcSynchronize();
        } else {
            fprintf(errOut, "\t exiting!\n");
            fflush(errOut);
        }
    }
    if (options.mode == ServerMode
        || options.refactoringMode == RefactoryMode
        || !fileAbortEnabled
    ) {
        emergencyExit(XREF_EXIT_ERR);
    }

    fprintf(errOut, "\t file aborted!\n");
    fflush(errOut);
    // TODO: WAS: longjump is causing problems with refactory, the longjmp
    // is missplaced. Is it? Test for this case?
    longjmp(cxmemOverflow, LONGJMP_REASON_FILE_ABORT);
}
