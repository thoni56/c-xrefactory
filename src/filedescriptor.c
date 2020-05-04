#include "filedescriptor.h"

#include "globals.h"            /*& s_noneFileIndex */

S_fileDesc cFile = {0};

S_fileDesc inStack[INCLUDE_STACK_SIZE];
int inStacki=0;


void fillFileDescriptor(S_fileDesc *fileDescriptor, char *fileName, char *bufferStart, int bufferSize, FILE *file, unsigned offset) {
    fileDescriptor->fileName = fileName;
    fileDescriptor->lineNumber = 0;
    fileDescriptor->ifDepth = 0;
    fileDescriptor->ifStack = NULL; /* lex buf */
    fileDescriptor->lexBuffer.next = NULL;
    fileDescriptor->lexBuffer.end = NULL;
    fileDescriptor->lexBuffer.index = 0; /* char buf */
    fileDescriptor->lexBuffer.buffer.next = bufferStart;
    fileDescriptor->lexBuffer.buffer.end = bufferStart+bufferSize;
    fileDescriptor->lexBuffer.buffer.file = file;
    fileDescriptor->lexBuffer.buffer.filePos = offset;
    fileDescriptor->lexBuffer.buffer.fileNumber = s_noneFileIndex;
    fileDescriptor->lexBuffer.buffer.lineNumber = 0;
    fileDescriptor->lexBuffer.buffer.lineBegin = bufferStart;
    fileDescriptor->lexBuffer.buffer.columnOffset = 0;
    fileDescriptor->lexBuffer.buffer.isAtEOF = 0;
    fileDescriptor->lexBuffer.buffer.inputMethod = INPUT_DIRECT; /* z_stream */
    fileDescriptor->lexBuffer.buffer.zipStream.next_in = NULL;
    fileDescriptor->lexBuffer.buffer.zipStream.avail_in = 0;
    fileDescriptor->lexBuffer.buffer.zipStream.total_in = 0,		// i;
    fileDescriptor->lexBuffer.buffer.zipStream.next_out = NULL;
    fileDescriptor->lexBuffer.buffer.zipStream.avail_out = 0;
    fileDescriptor->lexBuffer.buffer.zipStream.total_out = 0,		// ou;
    fileDescriptor->lexBuffer.buffer.zipStream.msg = NULL;
    fileDescriptor->lexBuffer.buffer.zipStream.state = NULL;
    fileDescriptor->lexBuffer.buffer.zipStream.zalloc = NULL;
    fileDescriptor->lexBuffer.buffer.zipStream.zfree = NULL;		// zlibAlloc, zlibFree
    fileDescriptor->lexBuffer.buffer.zipStream.opaque = NULL;
    fileDescriptor->lexBuffer.buffer.zipStream.data_type = 0;
    fileDescriptor->lexBuffer.buffer.zipStream.adler = 0;
    fileDescriptor->lexBuffer.buffer.zipStream.reserved = 0;
}
