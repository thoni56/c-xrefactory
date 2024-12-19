#include "filedescriptor.h"

#include "characterreader.h"
#include "filetable.h"


FileDescriptor currentFile = {0};

IncludeStack includeStack;


void fillFileDescriptor(FileDescriptor *fileDescriptor, char *fileName, char *bufferStart,
                        int bufferSize, FILE *file, unsigned offset) {
    fileDescriptor->fileName = fileName;
    fileDescriptor->lineNumber = 0;
    fileDescriptor->ifDepth = 0;
    fileDescriptor->ifStack = NULL;
    initLexemBuffer(&fileDescriptor->lexemBuffer);
    initCharacterBufferFromFile(&fileDescriptor->characterBuffer, file);
    fillCharacterBuffer(&fileDescriptor->characterBuffer, bufferStart, bufferStart + bufferSize, file,
                        offset, NO_FILE_NUMBER, bufferStart);
}
