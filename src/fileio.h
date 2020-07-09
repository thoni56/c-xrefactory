#ifndef _FILEIO_H_
#define _FILEIO_H_

#include <stdio.h>
#include <stdbool.h>


extern FILE *openFile(char *fileName, char *modes);
extern int closeFile(FILE *file);
extern size_t readFile(void *buffer, size_t size, size_t count, FILE *file);
extern size_t writeFile(void *buffer, size_t size, size_t count, FILE *file);
extern void createDir(char *dirname);
extern void removeFile(char *dirname);
extern bool dirExists(char *fullPath);
extern bool fileExists(char *fullPath);

#endif
