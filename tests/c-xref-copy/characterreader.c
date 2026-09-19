#include "characterreader.h"

#include <stdlib.h>
#include <string.h>

#include "head.h"
#include "fileio.h"

#include "log.h"


#define MAX_UNGET_CHARS 20


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
}

void initCharacterBufferFromFile(CharacterBuffer *characterbuffer, FILE *file) {
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
    if (buffer->file == NULL)
        return 0;
    else
        return readFile(buffer->file, outBuffer, 1, max_size);
}

void closeCharacterBuffer(CharacterBuffer *buffer) {
    ENTER();
    if (buffer->file != NULL)
        closeFile(buffer->file);
    LEAVE();
}

bool refillBuffer(CharacterBuffer *buffer) {
    char *next = buffer->nextUnread;
    char *end = buffer->end;

    char *cp;
    for (cp=buffer->chars+MAX_UNGET_CHARS; next<end; next++,cp++)
        *cp = *next;

    int max_size = CHARACTER_BUFFER_SIZE - (cp - buffer->chars);

    int charactersRead = readFromFileToBuffer(buffer, cp, max_size);

    if (charactersRead > 0) {
        buffer->filePos += charactersRead;
        buffer->end = cp+charactersRead;
        buffer->nextUnread = buffer->chars+MAX_UNGET_CHARS;
    }

    log_trace("refillBuffer: (%s) buffer->next=%p, buffer->end=%p", buffer->nextUnread == buffer->end?"equal":"not equal", buffer->nextUnread, buffer->end);
    return buffer->nextUnread != buffer->end;
}


void skipCharacters(CharacterBuffer *buffer, unsigned count) {

    if (buffer->nextUnread+count < buffer->end) {
        buffer->nextUnread += count;
        return;
    }

    count -= buffer->end - buffer->nextUnread;        /* How many to skip after refilling? */

    char *dd;
    int n;
    int max_size;

    log_trace("seeking over %d chars", count);
    fseek(buffer->file, count, SEEK_CUR);
    buffer->filePos += count;
    dd = buffer->chars + MAX_UNGET_CHARS;
    max_size = CHARACTER_BUFFER_SIZE - (dd - buffer->chars);
    if (buffer->file == NULL)
        n = 0;
    else
        n = readFile(buffer->file, dd, 1, max_size);
    buffer->filePos += n;
    buffer->end = dd + n;
    buffer->nextUnread = buffer->chars + MAX_UNGET_CHARS;
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
        log_trace("Ungetting ('%c'=%d)", ch, ch);
    *--(cb->nextUnread) = ch;
}
