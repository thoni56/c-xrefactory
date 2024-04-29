#include "fileio.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "log.h"
#include "constants.h"


/* Do any type of file entry exist at path? */
bool exists(char *path) {
    struct stat st;
    int rc = stat(path, &st);
    return rc == 0;
}

FILE *openFile(char *fileName, char *modes) {
    return fopen(fileName, modes);
}

int closeFile(FILE *file) {
    return fclose(file);
}

void createDirectory(char *dirname) {
    struct stat st;

    if (exists(dirname)) {
        stat(dirname, &st);
        if (S_ISDIR(st.st_mode))
            return;
        removeFile(dirname);
    }
#ifdef __WIN32__
    mkdir(dirname);
#else
    mkdir(dirname, 0777);
#endif
}


void recursivelyCreateFileDirIfNotExists(char *fpath) {
    char *p;
    int ch, len;
    bool loopFlag = true;

    /* Check each level from the deepest, stop when it exists */
    len = strlen(fpath);
    for (p=fpath+len; p>fpath && loopFlag; p--) {
        if (*p!=FILE_PATH_SEPARATOR)
            continue;
        ch = *p; *p = 0;        /* Truncate here, remember the char */
        if (directoryExists(fpath)) {
            loopFlag=false;
        }
        *p = ch;                /* Restore the char */
    }
    /* Create each of the remaining levels */
    for (p+=2; *p; p++) {
        if (*p!=FILE_PATH_SEPARATOR)
            continue;
        ch = *p; *p = 0;
        createDirectory(fpath);
        *p = ch;
    }
}


void removeFile(char *fileName) {
    log_trace("removing file '%s'", fileName);
    unlink(fileName);
}

int fileStatus(char *path, struct stat *statP) {
    struct stat st;
    int return_value;

    return_value = stat(path, &st); /* Returns 0 on success */
    if (statP != NULL)
        *statP = st;
    return return_value;
}

time_t fileModificationTime(char *path) {
    struct stat st;
    if (fileStatus(path, &st) !=0)
        return 0;               /* File not found? */
    return st.st_mtime;
}

size_t fileSize(char *path) {
    struct stat st;
    if (fileStatus(path, &st) !=0)
        return 0;               /* File not found? */
    return st.st_size;
}

bool isDirectory(char *path) {
    return directoryExists(path);
}

bool directoryExists(char *path) {
    struct stat st;
    int statResult;

    statResult = stat(path, &st);
    return statResult==0 && S_ISDIR(st.st_mode);
}

bool fileExists(char *fullPath) {
    struct stat st;
    int statResult;

    statResult = stat(fullPath, &st);
    return statResult==0 && S_ISREG(st.st_mode);
}

size_t readFile(FILE *file, void *buffer, size_t size, size_t count) {
    return fread(buffer, size, count, file);
}

size_t writeFile(FILE *file, void *buffer, size_t size, size_t count) {
    return fwrite(buffer, size, count, file);
}

int readChar(FILE *file) {
    return getc(file);
}

char *getEnv(const char *variable) {
    return getenv(variable);
}

char *getCwd(char *buffer, size_t size) {
    return getcwd(buffer, size);
}
