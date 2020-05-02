#include "fileio.h"

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
