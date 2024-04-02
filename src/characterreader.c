#include "characterreader.h"

#include <stdlib.h>
#include <string.h>

#include "proto.h"              /* ERR_ST ...  */
#include "commons.h"            /* error() */
#include "fileio.h"

#include "log.h"


static z_stream emptyZStream = {NULL,};


void fillCharacterBuffer(CharacterBuffer *characterBuffer,
                         char *next,
                         char *end,
                         FILE *file,
                         unsigned filePos,
                         int fileNumber,
                         char *lineBegin
) {
    characterBuffer->nextUnread = next;
    characterBuffer->end = end;
    characterBuffer->file = file;
    characterBuffer->filePos = filePos;
    characterBuffer->fileNumber = fileNumber;
    characterBuffer->lineNumber = 0;
    characterBuffer->lineBegin = lineBegin;
    characterBuffer->columnOffset = 0;
    characterBuffer->isAtEOF = false;
    characterBuffer->inputMethod = INPUT_DIRECT;
    characterBuffer->zipStream = emptyZStream;
}

void initCharacterBuffer(CharacterBuffer *characterbuffer, FILE *file) {
    fillCharacterBuffer(characterbuffer, characterbuffer->chars, characterbuffer->chars,
                        file, 0, -1, characterbuffer->chars);
}

void initCharacterBufferFromString(CharacterBuffer *characterbuffer, char *string) {
    strcpy(characterbuffer->chars, string);
    fillCharacterBuffer(characterbuffer, characterbuffer->chars, characterbuffer->chars,
                        NULL, 0, 0, characterbuffer->chars);
    characterbuffer->end = &characterbuffer->chars[strlen(string)];
}


int fileNumberFrom(CharacterBuffer *cb) {
    return cb->fileNumber;
}

int lineNumberFrom(CharacterBuffer *cb) {
    return cb->lineNumber;
}

/* ***************************************************************** */
/*                        Character reading                          */
/* ***************************************************************** */


static int readFromFileToBuffer(CharacterBuffer  *buffer, char *outBuffer, int max_size) {
    int n;

    if (buffer->file == NULL) n = 0;
    else n = readFile(buffer->file, outBuffer, 1, max_size);
    return(n);
}

void closeCharacterBuffer(CharacterBuffer *buffer) {
    ENTER();
    if (buffer->file!=NULL)
        closeFile(buffer->file);
#ifdef HAVE_ZLIB
    if (buffer->inputMethod == INPUT_VIA_UNZIP) {
        inflateEnd(&buffer->zipStream);
    }
#endif
    LEAVE();
}

/* Need to match zlib'z alloc_func
   alloc_func zlibAlloc() ?
 */
static voidpf zlibAlloc(voidpf opaque, uInt items, uInt size) {
    return calloc(items, size);
}

static void zlibFree(voidpf opaque, voidpf address) {
    free(address);
}

static int readFromUnzipFilterToBuffer(CharacterBuffer *buffer, char *outBuffer, int max_size) {
    buffer->zipStream.next_out = (unsigned char *)outBuffer;
    buffer->zipStream.avail_out = max_size;
#ifdef HAVE_ZLIB
    int res = Z_DATA_ERROR;

    do {
        if (buffer->zipStream.avail_in == 0) {
            int fn = readFromFileToBuffer(buffer, buffer->z, CHARARACTER_BUFFER_SIZE);
            buffer->zipStream.next_in = (unsigned char *)buffer->z;
            buffer->zipStream.avail_in = fn;
        }
        //&fprintf(stderr,"sending to inflate :");
        //&for(iii=0;iii<100;iii++) fprintf(stderr, " %d", buffer->zipStream.next_in[iii]);
        //&fprintf(stderr,"\n\n");
        res = inflate(&buffer->zipStream, Z_SYNC_FLUSH);  // Z_NO_FLUSH
        //&fprintf(dumpOut,"res == %d\n", res);
        if (res==Z_OK) {
            //&fprintf(dumpOut,"successfull zip read\n");
        } else if (res==Z_STREAM_END) {
            //&fprintf(dumpOut,"end of zip read\n");
        } else {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "something is going wrong while reading zipped .jar archive, res == %d", res);
            errorMessage(ERR_ST, tmpBuff);
            buffer->zipStream.next_out = (unsigned char *)outBuffer;
        }
    } while (((char*)buffer->zipStream.next_out)==outBuffer && res==Z_OK);
#endif
    return ((char*)buffer->zipStream.next_out) - outBuffer;
}

bool refillBuffer(CharacterBuffer *buffer) {
    char *next = buffer->nextUnread;
    char *end = buffer->end;

    char *cp;
    for (cp=buffer->chars+MAX_UNGET_CHARS; next<end; next++,cp++)
        *cp = *next;

    int max_size = CHARARACTER_BUFFER_SIZE - (cp - buffer->chars);

    int charactersRead;
    if (buffer->inputMethod == INPUT_DIRECT) {
        charactersRead = readFromFileToBuffer(buffer, cp, max_size);
    } else {
        charactersRead = readFromUnzipFilterToBuffer(buffer, cp, max_size);
    }

    if (charactersRead > 0) {
        buffer->filePos += charactersRead;
        buffer->end = cp+charactersRead;
        buffer->nextUnread = buffer->chars+MAX_UNGET_CHARS;
    }

    log_trace("refillBuffer: (%s) buffer->next=%p, buffer->end=%p", buffer->nextUnread == buffer->end?"equal":"not equal", buffer->nextUnread, buffer->end);
    return buffer->nextUnread != buffer->end;
}


static void fillZipStreamFromBuffer(CharacterBuffer  *buffer, char *dd) {
    memset(&buffer->zipStream, 0, sizeof(buffer->zipStream));
    buffer->zipStream.next_in = (Bytef*)buffer->z;
    buffer->zipStream.avail_in = dd-buffer->z;
    buffer->zipStream.next_out = (Bytef*)buffer->chars;
    buffer->zipStream.avail_out = CHARARACTER_BUFFER_SIZE;
    buffer->zipStream.zalloc = zlibAlloc;
    buffer->zipStream.zfree = zlibFree;
}


void switchToZippedCharBuff(CharacterBuffer *buffer) {
    char *dd;
    char *next;
    char *end;

    refillBuffer(buffer);     // just for now
#ifdef HAVE_ZLIB
    end = buffer->end;
    next = buffer->nextUnread;
    for(dd=buffer->z; next<end; next++,dd++)
        *dd = *next;

    fillZipStreamFromBuffer(buffer, dd);

    buffer->nextUnread = buffer->end = buffer->chars;
    buffer->inputMethod = INPUT_VIA_UNZIP;
    inflateInit2(&(buffer->zipStream), -MAX_WBITS);
    if (buffer->zipStream.msg != NULL) {
        fprintf(stderr, "initialization: %s\n", buffer->zipStream.msg);
        exit(1);
    }
#endif
}

void skipCharacters(CharacterBuffer *buffer, unsigned count) {

    if (buffer->nextUnread+count < buffer->end) {
        buffer->nextUnread += count;
        return;
    }

    count -= buffer->end - buffer->nextUnread;        /* How many to skip after refilling? */
    if (buffer->inputMethod == INPUT_VIA_UNZIP) {
        buffer->nextUnread = buffer->end;
        refillBuffer(buffer);
        if (buffer->end != buffer->nextUnread) {
            // TODO remove last recursion
            skipCharacters(buffer, count);
        }
    } else {
        char *dd;
        int n;
        int max_size;

        log_trace("seeking over %d chars", count);
        fseek(buffer->file, count, SEEK_CUR);
        buffer->filePos += count;
        dd = buffer->chars + MAX_UNGET_CHARS;
        max_size = CHARARACTER_BUFFER_SIZE - (dd - buffer->chars);
        if (buffer->file == NULL)
            n = 0;
        else
            n = readFile(buffer->file, dd, 1, max_size);
        buffer->filePos += n;
        buffer->end = dd + n;
        buffer->nextUnread = buffer->chars + MAX_UNGET_CHARS;
    }
}


int columnPosition(CharacterBuffer *cb) {
    return cb->nextUnread - cb->lineBegin + cb->columnOffset - 1;
}


int fileOffsetFor(CharacterBuffer *cb) {
    return cb->filePos - (cb->end - cb->nextUnread) - 1;
}


int skipBlanks(CharacterBuffer *cb, int ch) {
    while (ch==' '|| ch=='\t' || ch=='\004') { /* EOT? */
        ch = getChar(cb);
    }
    return ch;
}


int skipWhiteSpace(CharacterBuffer *cb, int ch) {
    while (ch==' ' || ch=='\n' || ch=='\t') {
        ch = getChar(cb);
    }

    return ch;
}

/* Return next unread character from CharacterBuffer and advance */
int getChar(CharacterBuffer *cb) {
        if (cb->nextUnread >= cb->end &&
            (cb->isAtEOF || refillBuffer(cb) == 0)) {
                cb->isAtEOF = true;
                return EOF;
        } else {
            return *cb->nextUnread++;
        }
}


void getString(CharacterBuffer *cb, char *string, int length) {
    char ch;

    for (int i=0; i<length; i++) {
        ch = getChar(cb);
        string[i] = ch;
    }
    string[length] = 0;
}

void ungetChar(CharacterBuffer *cb, int ch) {
    if (ch == '\n')
        log_trace("Ungetting ('\\n')");
    else
        log_trace("Ungetting ('%c')", ch);
    *--(cb->nextUnread) = ch;
}
