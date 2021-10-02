#include "fileio.h"

#include <unistd.h>
#include <sys/stat.h>

#include "log.h"
#include "head.h"


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

void createDir(char *dirname) {
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

void removeFile(char *fileName) {
    log_trace("removing file '%s'", fileName);
    unlink(fileName);
}

int fileStatus(char *path, struct stat *statP) {
    struct stat st;
    int return_value;

    return_value = stat(path, &st);
    if (statP != NULL)
        *statP = st;
    return return_value;
}

bool dirExists(char *path) {
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

size_t readFile(void *buffer, size_t size, size_t count, FILE *file) {
    return fread(buffer, size, count, file);
}

size_t writeFile(void *buffer, size_t size, size_t count, FILE *file) {
    return fwrite(buffer, size, count, file);
}

int readChar(FILE *file) {
    return getc(file);
}
