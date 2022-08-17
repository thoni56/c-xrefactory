#include "filedescriptor.h"

#include "filetable.h"


FileDescriptor currentFile = {0};

FileDescriptor includeStack[INCLUDE_STACK_SIZE];
int includeStackPointer = 0;


void fillFileDescriptor(FileDescriptor *fileDescriptor, char *fileName, char *bufferStart,
                        int bufferSize, FILE *file, unsigned offset) {
    fileDescriptor->fileName = fileName;
    fileDescriptor->lineNumber = 0;
    fileDescriptor->ifDepth = 0;
    fileDescriptor->ifStack = NULL;
    initLexemBuffer(&fileDescriptor->lexemBuffer, file);
    fillCharacterBuffer(&fileDescriptor->lexemBuffer.characterBuffer, bufferStart, bufferStart + bufferSize, file,
                        offset, noFileIndex, bufferStart);
}
