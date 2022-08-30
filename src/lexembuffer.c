#include "lexembuffer.h"

#include "characterreader.h"
#include "head.h"
#include "commons.h"


void initLexemBuffer(LexemBuffer *buffer, CharacterBuffer *characterBuffer) {
    buffer->next = buffer->lexemStream;
    buffer->end = buffer->lexemStream;
    buffer->ringIndex = 0;
    buffer->characterBuffer = characterBuffer;
}

/* Should use LexemBuffer, but doesn't yet... */
Lexem getLexemAt(LexemBuffer *lb, void *readPointer) {
    return peekLexTokenAt(readPointer);
}

void setLexemStreamEnd(LexemBuffer *lb, void *end) {
    lb->end = end;
}

void *getLexemStreamEnd(LexemBuffer *lb) {
    return (void *)(lb->end);
}

/* Char */
void putLexChar(LexemBuffer *lb, char ch) {
    *(lb->end++) = ch;
}

/* Short */
protected void putLexShort(int shortValue, char **writePointerP) {
    assert(shortValue <= 65535);
    **writePointerP = ((unsigned)shortValue)%256;
    (*writePointerP)++;
    **writePointerP = ((unsigned)shortValue)/256;
    (*writePointerP)++;
}

int getLexShort(char **readPointerP) {
    int value = *(unsigned char*)(*readPointerP);
    (*readPointerP)++;
    value += 256 * *(unsigned char*)(*readPointerP);
    (*readPointerP)++;
    return value;
}

static int peekLexShort(char **readPointerP) {
    int first = *(unsigned char*)(*readPointerP);
    int second = *((unsigned char*)(*readPointerP)+1);

    return first + 256*second;
}

/* Token */
void putLexToken(LexemBuffer *lb, Lexem lexem) {
    putLexShort(lexem, &(lb->end));
}

/* For backpatching */
void putLexTokenAtPointer(Lexem lexem, void *writePointer) {
    char *pointer = (char *)writePointer;
    putLexShort(lexem, &pointer);
}

Lexem getLexTokenAtPointer(char **readPointerP) {
    return (Lexem)getLexShort(readPointerP);
}

Lexem getLexToken(LexemBuffer *lb) {
    return (Lexem)getLexShort(&lb->next);
}

Lexem peekLexTokenAt(char *readPointer) {
    char *pointer = readPointer;
    return (Lexem)peekLexShort(&pointer);
}

/* Int */
void putLexInt(LexemBuffer *lb, int value) {
    unsigned tmp;
    tmp = value;
    *lb->end++ = tmp%256; tmp /= 256;
    *lb->end++ = tmp%256; tmp /= 256;
    *lb->end++ = tmp%256; tmp /= 256;
    *lb->end++ = tmp%256; tmp /= 256;
}

int getLexInt(char **readPointerP) {
    unsigned int value;
    value = **(unsigned char**)readPointerP;
    (*readPointerP)++;
    value += 256 * **(unsigned char**)readPointerP;
    (*readPointerP)++;
    value += 256 * 256 * **(unsigned char**)readPointerP;
    (*readPointerP)++;
    value += 256 * 256 * 256 * **(unsigned char**)readPointerP;
    (*readPointerP)++;
    return value;
}

/* Compacted */
protected void putLexCompacted(int value, char **writePointerP) {
    assert(((unsigned) value)<4194304);
    if (((unsigned)value) < 128) {
        **writePointerP = ((unsigned char)value);
        (*writePointerP)++;
    } else if (((unsigned)value) < 16384) {
        **writePointerP = ((unsigned)value)%128+128;
        (*writePointerP)++;
        **writePointerP = ((unsigned)value)/128;
        (*writePointerP)++;
    } else {
        **writePointerP = ((unsigned)value)%128+128;
        (*writePointerP)++;
        **writePointerP = ((unsigned)value)/128%128+128;
        (*writePointerP)++;
        **writePointerP = ((unsigned)value)/16384;
        (*writePointerP)++;
    }
}

protected int getLexCompacted(char **readPointerP) {
    unsigned value;

    value = **(unsigned char**)readPointerP;
    (*readPointerP)++;
    if (value >= 128) {
        unsigned secondPart = **(unsigned char**)readPointerP;
        (*readPointerP)++;
        if (secondPart < 128) {
            value = ((unsigned)value)-128 + 128 * secondPart;
        } else {
            unsigned thirdPart = **(unsigned char**)readPointerP;
            (*readPointerP)++;
            value = ((unsigned)value)-128 + 128 * (secondPart-128) + 16384 * thirdPart;
        }
    }
    return value;
}

int fileNumberFrom(LexemBuffer *lb) {
    return lb->characterBuffer->fileNumber;
}

int lineNumberFrom(LexemBuffer *lb) {
    return lb->characterBuffer->lineNumber;
}

/* Lines */
void putLexLines(LexemBuffer *lb, int lines) {
    putLexToken(lb, LINE_TOKEN);
    putLexToken(lb, lines);
}

/* Position */
void putLexPositionFields(LexemBuffer *lb, int file, int line, int column) {
    assert(file>=0 && file<MAX_FILES);
    putLexCompacted(file, &(lb->end));
    putLexCompacted(line, &(lb->end));
    putLexCompacted(column, &(lb->end));
}

void putLexPosition(LexemBuffer *lb, Position position) {
    assert(position.file>=0 && position.file<MAX_FILES);
    putLexCompacted(position.file, &(lb->end));
    putLexCompacted(position.line, &(lb->end));
    putLexCompacted(position.col, &(lb->end));
}

Position getLexPosition(char **readPointerP) {
    Position pos;
    pos.file = getLexCompacted(readPointerP);
    pos.line = getLexCompacted(readPointerP);
    pos.col = getLexCompacted(readPointerP);
    return pos;
}

Position peekLexPosition(char **readPointerP) {
    char *pointer = *readPointerP;
    return getLexPosition(&pointer);
}

/* DEPRECATED - writing with pointer to pointer that it advances, a
 * lot of code in yylex.c still uses this bad interface */
void putLexTokenWithPointer(Lexem lexem, char **writePointerP) {
    putLexShort(lexem, writePointerP);
}

void putLexPositionWithPointer(Position position, char **writePointerP) {
    putLexCompacted(position.file, writePointerP);
    putLexCompacted(position.line, writePointerP);
    putLexCompacted(position.col, writePointerP);
}

void putLexIntWithPointer(int integer, char **writePointerP) {
    unsigned tmp;
    tmp = integer;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
}

void shiftAnyRemainingLexems(LexemBuffer *lb) {
    int remaining = lb->end - lb->next;
    char *src = lb->next;
    char *dest = lb->lexemStream;

    for (int i = 0; i < remaining; i++)
        *dest++ = *src++;

    lb->next = lb->lexemStream;
    lb->end = &lb->lexemStream[remaining];
}
