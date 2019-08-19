#include "charbuf.h"

#include "globals.h"
#include "commons.h"
#include "unigram.h"
#include "caching.h"
#include "protocol.h"
#include "cxref.h"
#include "jslsemact.h"
#include "utils.h"



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
            sprintf(tmpBuff, "something is going wrong while reading zipped .jar archiv, res == %d", res);
            error(ERR_ST, tmpBuff);
            buffer->zipStream.next_out = (unsigned char *)outBuffer;
        }
    } while (((char*)buffer->zipStream.next_out)==outBuffer && res==Z_OK);
#endif                      /*SBD*/
    n = ((char*)buffer->zipStream.next_out) - outBuffer;
    return(n);
}

int getCharBuf(struct CharacterBuffer *buffer) {
    char *dd;
    char *cc;
    char *fin;
    int n;
    int max_size;
    fin = buffer->end;
    cc = buffer->next;
    for(dd=buffer->buffer+MAX_UNGET_CHARS; cc<fin; cc++,dd++) *dd = *cc;
    max_size = CHAR_BUFF_SIZE - (dd - buffer->buffer);
    if (buffer->inputMethod == INPUT_DIRECT) {
        n = charBuffReadFromFile(buffer, dd, max_size);
    } else {
        n = charBuffReadFromUnzipFilter(buffer, dd, max_size);
    }
    buffer->filePos += n;
    buffer->end = dd+n;
    buffer->next = buffer->buffer+MAX_UNGET_CHARS;
    return(buffer->next != buffer->end);
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
    FILL_z_stream_s(&buffer->zipStream,
                    (Bytef*)buffer->z, dd-buffer->z, 0,
                    (Bytef*)buffer->buffer, CHAR_BUFF_SIZE, 0,
                    NULL, NULL,
                    zlibAlloc, zlibFree,
                    NULL, 0, 0, 0
                    );
    buffer->next = buffer->end = buffer->buffer;
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
        dd=buffer->buffer+MAX_UNGET_CHARS;
        max_size = CHAR_BUFF_SIZE-(dd - buffer->buffer);
        if (buffer->file == NULL) n = 0;
        else n = fread(dd, 1, max_size, buffer->file);
        buffer->filePos += n;
        buffer->end = dd+n;
        buffer->next = buffer->buffer+MAX_UNGET_CHARS;
    }
    return(buffer->next != buffer->end);
}

void gotOnLineCxRefs( S_position *ps ) {
    if (creatingOlcxRefs()) {
        s_cache.activeCache = 0;
        s_cxRefPos = *ps;
    }
}
