#include "lexembuffer.h"

#include "commons.h"


void initLexemBuffer(LexemBuffer *buffer, FILE *file) {
    buffer->next = buffer->lexemStream;
    buffer->end = buffer->lexemStream;
    buffer->ringIndex = 0;
    initCharacterBuffer(&buffer->buffer, file);
}

/* New API with index: */
Lexem getLexemAt(LexemBuffer *lb, int index) {
    char *readPointer = &lb->lexemStream[index];
    return nextLexToken(&readPointer);
}

int getCurrentLexemIndexForBackpatching(LexemBuffer *lb) {
    return lb->next - lb->lexemStream;
}

void backpatchLexem(LexemBuffer *lb, int index, Lexem lexem) {
    char *writePointer = &lb->lexemStream[index];
    putLexToken(lexem, &writePointer);
}

void setLexemStreamEnd(LexemBuffer *lb, int index) {
    lb->end = &lb->lexemStream[index];
}


/* Char */
void putLexChar(LexemBuffer *lb, char ch) {
    *(lb->end++) = ch;
}

unsigned char getLexChar(char **readPointerP) {
    unsigned char ch = **readPointerP;
    (*readPointerP)++;
    return ch;
}

/* Short */
void putLexShort(int shortValue, char **writePointerP) {
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

static int nextLexShort(char **readPointerP) {
    int first = *(unsigned char*)(*readPointerP);
    int second = *((unsigned char*)(*readPointerP)+1);

    return first + 256*second;
}

/* Token */
void putLexToken(Lexem lexem, char **writePointerP) {
    putLexShort(lexem, writePointerP);
}

Lexem getLexToken(char **readPointerP) {
    return (Lexem)getLexShort(readPointerP);
}

Lexem nextLexToken(char **readPointerP) {
    return (Lexem)nextLexShort(readPointerP);
}

/* Int */
void putLexInt(int value, char **writePointerP) {
        unsigned tmp;
        tmp = value;
        *(*writePointerP)++ = tmp%256; tmp /= 256;
        *(*writePointerP)++ = tmp%256; tmp /= 256;
        *(*writePointerP)++ = tmp%256; tmp /= 256;
        *(*writePointerP)++ = tmp%256; tmp /= 256;
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
void putLexCompacted(int value, char **writePointerP) {
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

int getLexCompacted(char **readPointerP) {
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
    return lb->buffer.fileNumber;
}

int lineNumberFrom(LexemBuffer *lb) {
    return lb->buffer.lineNumber;
}

/* Lines */
void putLexLines(int lines, LexemBuffer *lb) {
    putLexToken(LINE_TOKEN, &(lb->end));
    putLexToken(lines, &(lb->end));
}

/* Position */
void putLexPositionFields(int file, int line, int column, char **writePointerP) {
    assert(file>=0 && file<MAX_FILES);
    putLexCompacted(file, writePointerP);
    putLexCompacted(line, writePointerP);
    putLexCompacted(column, writePointerP);
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

Position nextLexPosition(char **readPointerP) {
    char *tmptmpcc = *readPointerP;
    Position pos;
    pos = getLexPosition(&tmptmpcc);
    return pos;
}
