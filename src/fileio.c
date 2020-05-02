#include "fileio.h"
#include <unistd.h>

#include "log.h"
#include "head.h"


FILE *openFile(char *fileName, char *modes) {
    return fopen(fileName, modes);
}

int closeFile(FILE *file) {
    return fclose(file);
}

size_t readFile(void *buffer, size_t size, size_t count, FILE *file) {
    return fread(buffer, size, count, file);
}

size_t writeFile(void *buffer, size_t size, size_t count, FILE *file) {
    return fwrite(buffer, size, count, file);
}

void createDir(char *dirname) {
#ifdef __WIN32__
    mkdir(dirname);
#else
    mkdir(dirname,0777);
#endif
}

void removeFile(char *fileName) {
    log_trace("removing file '%s'", fileName);
    unlink(fileName);
}
