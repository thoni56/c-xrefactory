#include "characterbuffer.h"

#include "globals.h"            /* for s_defaultZStream */

#include "commons.h"            /* error() */
#include "fileio.h"


void fillCharacterBuffer(CharacterBuffer *characterBuffer,
                         char *next,
                         char *end,
                         FILE *file,
                         unsigned filePos,
                         int fileNumber,
                         char *lineBegin) {
    characterBuffer->next = next;
    characterBuffer->end = end;
    characterBuffer->file = file;
    characterBuffer->filePos = filePos;
    characterBuffer->fileNumber = fileNumber;
    characterBuffer->lineNumber = 0;
    characterBuffer->lineBegin = lineBegin;
    characterBuffer->columnOffset = 0;
    characterBuffer->isAtEOF = false;
    characterBuffer->inputMethod = INPUT_DIRECT;
    characterBuffer->zipStream = s_defaultZStream;
}

void initCharacterBuffer(CharacterBuffer *characterbuffer, FILE *file, int fileNumber) {
    fillCharacterBuffer(characterbuffer, characterbuffer->chars, characterbuffer->chars,
                        file, 0, fileNumber, characterbuffer->chars);
}


/* ***************************************************************** */
/*                        Character reading                          */
/* ***************************************************************** */


static int readFromFileToBuffer(struct characterBuffer  *buffer, char *outBuffer, int max_size) {
    int n;

    if (buffer->file == NULL) n = 0;
    else n = readFile(outBuffer, 1, max_size, buffer->file);
    return(n);
}

void closeCharacterBuffer(struct characterBuffer *buffer) {
    if (buffer->file!=NULL)
        closeFile(buffer->file);
    if (buffer->inputMethod == INPUT_VIA_UNZIP) {
        inflateEnd(&buffer->zipStream);
    }
}

/* Need to match zlib'z alloc_func
   alloc_func zlibAlloc() ?
 */
voidpf zlibAlloc(voidpf opaque, uInt items, uInt size) {
    return(calloc(items, size));
}

void zlibFree(voidpf opaque, voidpf address) {
    free(address);
}

static int readFromUnzipFilterToBuffer(struct characterBuffer *buffer, char *outBuffer, int max_size) {
    int n, fn, res;

    buffer->zipStream.next_out = (unsigned char *)outBuffer;
    buffer->zipStream.avail_out = max_size;
    do {
        if (buffer->zipStream.avail_in == 0) {
            fn = readFromFileToBuffer(buffer, buffer->z, CHAR_BUFF_SIZE);
            buffer->zipStream.next_in = (unsigned char *)buffer->z;
            buffer->zipStream.avail_in = fn;
        }
        //&fprintf(stderr,"sending to inflate :");
        //&for(iii=0;iii<100;iii++) fprintf(stderr, " %d", buffer->zipStream.next_in[iii]);
        //&fprintf(stderr,"\n\n");
        res = Z_DATA_ERROR;
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
    n = ((char*)buffer->zipStream.next_out) - outBuffer;
    return(n);
}

bool fillBuffer(struct characterBuffer *buffer) {
    char *cp;
    char *next;
    char *end;
    int charactersRead;
    int max_size;

    end = buffer->end;
    next = buffer->next;

    for(cp=buffer->chars+MAX_UNGET_CHARS; next<end; next++,cp++) *cp = *next;

    max_size = CHAR_BUFF_SIZE - (cp - buffer->chars);
    if (buffer->inputMethod == INPUT_DIRECT) {
        charactersRead = readFromFileToBuffer(buffer, cp, max_size);
    } else {
        charactersRead = readFromUnzipFilterToBuffer(buffer, cp, max_size);
    }

    buffer->filePos += charactersRead;
    buffer->end = cp+charactersRead;
    buffer->next = buffer->chars+MAX_UNGET_CHARS;

    return buffer->next != buffer->end;
}


static void fillZipStreamFromBuffer(struct characterBuffer  *buffer, char *dd) {
    memset(&buffer->zipStream, 0, sizeof(buffer->zipStream));
    buffer->zipStream.next_in = (Bytef*)buffer->z;
    buffer->zipStream.avail_in = dd-buffer->z;
    buffer->zipStream.next_out = (Bytef*)buffer->chars;
    buffer->zipStream.avail_out = CHAR_BUFF_SIZE;
    buffer->zipStream.zalloc = zlibAlloc;
    buffer->zipStream.zfree = zlibFree;
}


void switchToZippedCharBuff(struct characterBuffer *buffer) {
    char *dd;
    char *cc;
    char *fin;

    fillBuffer(buffer);     // just for now
    fin = buffer->end;
    cc = buffer->next;
    for(dd=buffer->z; cc<fin; cc++,dd++) *dd = *cc;

    fillZipStreamFromBuffer(buffer, dd);

    buffer->next = buffer->end = buffer->chars;
    buffer->inputMethod = INPUT_VIA_UNZIP;
    inflateInit2(&(buffer->zipStream), -MAX_WBITS);
    if (buffer->zipStream.msg != NULL) {
        fprintf(stderr, "initialization: %s\n", buffer->zipStream.msg);
        exit(1);
    }
}

int skipNCharsInCharBuf(struct characterBuffer *buffer, unsigned count) {
    char *dd;
    char *cc;
    char *fin;
    int n;
    int max_size;

    fin = buffer->end;
    cc = buffer->next;
    if (cc+count < fin) {
        buffer->next = cc+count;
        return(1);
    }
    if (buffer->inputMethod == INPUT_VIA_UNZIP) {
        // TODO FINISH THIS
        count -= fin-cc;
        buffer->next = buffer->end;
        fillBuffer(buffer);
        if (buffer->end != buffer->next) {
            // TODO remove last recursion
            skipNCharsInCharBuf(buffer, count);
        }
    } else {
        count -= fin-cc;
        /*&fprintf(dumpOut,"seeking %d chars\n",count); fflush(dumpOut);&*/
        fseek(buffer->file, count, SEEK_CUR);
        buffer->filePos += count;
        dd=buffer->chars+MAX_UNGET_CHARS;
        max_size = CHAR_BUFF_SIZE-(dd - buffer->chars);
        if (buffer->file == NULL) n = 0;
        else n = readFile(dd, 1, max_size, buffer->file);
        buffer->filePos += n;
        buffer->end = dd+n;
        buffer->next = buffer->chars+MAX_UNGET_CHARS;
    }
    return(buffer->next != buffer->end);
}
