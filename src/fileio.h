#ifndef FILEIO_H_INCLUDED
#define FILEIO_H_INCLUDED

#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>


extern FILE *openFile(char *fileName, char *modes);
extern int closeFile(FILE *file);
extern void createDir(char *dirname);
extern void removeFile(char *dirname);
extern int fileStatus(char *path, struct stat *statbuf);
extern bool dirExists(char *fullPath);
extern bool fileExists(char *fullPath);
extern size_t readFile(void *buffer, size_t size, size_t count, FILE *file);
extern size_t writeFile(void *buffer, size_t size, size_t count, FILE *file);
extern int readChar(FILE *file);

#endif
