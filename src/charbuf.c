#include "charbuf.h"

#include "globals.h"

#include "commons.h"            /* error() */


void fill_CharacterBuffer(CharacterBuffer *characterBuffer,
                          char *next,
                          char *end,
                          FILE *file,
                          unsigned filePos,
                          int fileNumber,
                          int lineNum,
                          char *lineBegin,
                          int columnOffset,
                          bool isAtEOF,
                          InputMethod inputMethod,
                          z_stream zipStream) {
    characterBuffer->next = next;
    characterBuffer->end = end;
    characterBuffer->file = file;
    characterBuffer->filePos = filePos;
    characterBuffer->fileNumber = fileNumber;
    characterBuffer->lineNum = 0;
    characterBuffer->lineBegin = lineBegin;
    characterBuffer->columnOffset = 0;
    characterBuffer->isAtEOF = false;
    characterBuffer->inputMethod = INPUT_DIRECT;
    characterBuffer->zipStream = s_defaultZStream;
}


/* ***************************************************************** */
/*                        Character reading                          */
/* ***************************************************************** */


static int charBuffReadFromFile(struct CharacterBuffer  *buffer, char *outBuffer, int max_size) {
    int n;

    if (buffer->file == NULL) n = 0;
    else n = fread(outBuffer, 1, max_size, buffer->file);
    return(n);
}

void charBuffClose(struct CharacterBuffer *buffer) {
    if (buffer->file!=NULL) fclose(buffer->file);
#if defined(USE_LIBZ)       /*SBD*/
    if (buffer->inputMethod == INPUT_VIA_UNZIP) {
        inflateEnd(&buffer->zipStream);
    }
#endif                      /*SBD*/
}

voidpf zlibAlloc(voidpf opaque, uInt items, uInt size) {
    return(calloc(items, size));
}

void zlibFree(voidpf opaque, voidpf address) {
    free(address);
}

static int charBuffReadFromUnzipFilter(struct CharacterBuffer *buffer, char *outBuffer, int max_size) {
    int n, fn, res;
    buffer->zipStream.next_out = (unsigned char *)outBuffer;
    buffer->zipStream.avail_out = max_size;
#if defined(USE_LIBZ)       /*SBD*/
    do {
        if (buffer->zipStream.avail_in == 0) {
            fn = charBuffReadFromFile(buffer, buffer->z, CHAR_BUFF_SIZE);
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
            sprintf(tmpBuff, "something is going wrong while reading zipped .jar archive, res == %d", res);
            error(ERR_ST, tmpBuff);
            buffer->zipStream.next_out = (unsigned char *)outBuffer;
        }
    } while (((char*)buffer->zipStream.next_out)==outBuffer && res==Z_OK);
#endif                      /*SBD*/
    n = ((char*)buffer->zipStream.next_out) - outBuffer;
    return(n);
}

bool getCharBuf(struct CharacterBuffer *buffer) {
    char *dd;
    char *cc;
    char *fin;
    int n;
    int max_size;
    fin = buffer->end;
    cc = buffer->next;
    for(dd=buffer->chars+MAX_UNGET_CHARS; cc<fin; cc++,dd++) *dd = *cc;
    max_size = CHAR_BUFF_SIZE - (dd - buffer->chars);
    if (buffer->inputMethod == INPUT_DIRECT) {
        n = charBuffReadFromFile(buffer, dd, max_size);
    } else {
        n = charBuffReadFromUnzipFilter(buffer, dd, max_size);
    }
    buffer->filePos += n;
    buffer->end = dd+n;
    buffer->next = buffer->chars+MAX_UNGET_CHARS;
    return(buffer->next != buffer->end);
}


static void fillZipStreamFromBuffer(struct CharacterBuffer  *buffer, char *dd) {
    memset(&buffer->zipStream, 0, sizeof(buffer->zipStream));
    buffer->zipStream.next_in = (Bytef*)buffer->z;
    buffer->zipStream.avail_in = dd-buffer->z;
    buffer->zipStream.next_out = (Bytef*)buffer->chars;
    buffer->zipStream.avail_out = CHAR_BUFF_SIZE;
    buffer->zipStream.zalloc = zlibAlloc;
    buffer->zipStream.zfree = zlibFree;
}


void switchToZippedCharBuff(struct CharacterBuffer *buffer) {
    char *dd;
    char *cc;
    char *fin;

    getCharBuf(buffer);     // just for now
#if defined(USE_LIBZ)
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
#endif
}

int skipNCharsInCharBuf(struct CharacterBuffer *buffer, unsigned count) {
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
        getCharBuf(buffer);
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
        else n = fread(dd, 1, max_size, buffer->file);
        buffer->filePos += n;
        buffer->end = dd+n;
        buffer->next = buffer->chars+MAX_UNGET_CHARS;
    }
    return(buffer->next != buffer->end);
}
