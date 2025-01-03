#ifndef FILEIO_H_INCLUDED
#define FILEIO_H_INCLUDED

#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>


extern bool exists(char *path);
extern time_t fileModificationTime(char *path);
extern size_t fileSize(char *path);
extern FILE *openFile(char *fileName, char *modes);
extern int closeFile(FILE *file);
extern void createDirectory(char *dirname);
extern void recursivelyCreateFileDirIfNotExists(char *fpath);
extern void removeFile(char *dirname);
extern bool isDirectory(char *fullPath);
extern bool directoryExists(char *fullPath);
extern bool fileExists(char *fullPath);
extern size_t readFile(FILE *file, void *buffer, size_t size, size_t count);
extern size_t writeFile(FILE *file, void *buffer, size_t size, size_t count);
extern int readChar(FILE *file);

extern char *getEnv(const char *variable);
extern char *getCwd(char *buf, size_t size);

#endif
