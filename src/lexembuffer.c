#include "lexembuffer.h"

#include "commons.h"


void initLexemBuffer(LexemBuffer *buffer, FILE *file) {
    buffer->next = buffer->lexemStream;
    buffer->end = buffer->lexemStream;
    buffer->index = 0;
    initCharacterBuffer(&buffer->buffer, file);
}

/* Char */
void putLexChar(char ch, char **writePointer) {
    **writePointer = ch;
    (*writePointer)++;
}

unsigned char getLexChar(char **readPointer) {
    unsigned char ch = **readPointer;
    (*readPointer)++;
    return ch;
}

/* Short */
void putLexShort(int shortValue, char **writePointer) {
    assert(shortValue <= 65535);
    **writePointer = ((unsigned)shortValue)%256;
    (*writePointer)++;
    **writePointer = ((unsigned)shortValue)/256;
    (*writePointer)++;
}

int getLexShort(char **readPointer) {
    int value = *(unsigned char*)(*readPointer);
    (*readPointer)++;
    value += 256 * *(unsigned char*)(*readPointer);
    (*readPointer)++;
    return value;
}

static int nextLexShort(char **readPointer) {
    int first = *(unsigned char*)(*readPointer);
    int second = *((unsigned char*)(*readPointer)+1);

    return first + 256*second;
}

/* Token */
void putLexToken(Lexem lexem, char **writePointer) {
    putLexShort(lexem, writePointer);
}

Lexem getLexToken(char **readPointer) {
    return (Lexem)getLexShort(readPointer);
}

Lexem nextLexToken(char **readPointer) {
    return (Lexem)nextLexShort(readPointer);
}

/* Int */
void putLexInt(int value, char **writePointer) {
        unsigned tmp;
        tmp = value;
        *(*writePointer)++ = tmp%256; tmp /= 256;
        *(*writePointer)++ = tmp%256; tmp /= 256;
        *(*writePointer)++ = tmp%256; tmp /= 256;
        *(*writePointer)++ = tmp%256; tmp /= 256;
    }

int getLexInt(char **readPointer) {
    unsigned int value;
    value = **(unsigned char**)readPointer;
    (*readPointer)++;
    value += 256 * **(unsigned char**)readPointer;
    (*readPointer)++;
    value += 256 * 256 * **(unsigned char**)readPointer;
    (*readPointer)++;
    value += 256 * 256 * 256 * **(unsigned char**)readPointer;
    (*readPointer)++;
    return value;
}

/* Compacted */
void putLexCompacted(int value, char **writePointer) {
    assert(((unsigned) value)<4194304);
    if (((unsigned)value) < 128) {
        **writePointer = ((unsigned char)value);
        (*writePointer)++;
    } else if (((unsigned)value) < 16384) {
        **writePointer = ((unsigned)value)%128+128;
        (*writePointer)++;
        **writePointer = ((unsigned)value)/128;
        (*writePointer)++;
    } else {
        **writePointer = ((unsigned)value)%128+128;
        (*writePointer)++;
        **writePointer = ((unsigned)value)/128%128+128;
        (*writePointer)++;
        **writePointer = ((unsigned)value)/16384;
        (*writePointer)++;
    }
}

int getLexCompacted(char **readPointer) {
    unsigned value;

    value = **(unsigned char**)readPointer;
    (*readPointer)++;
    if (value >= 128) {
        unsigned secondPart = **(unsigned char**)readPointer;
        (*readPointer)++;
        if (secondPart < 128) {
            value = ((unsigned)value)-128 + 128 * secondPart;
        } else {
            unsigned thirdPart = **(unsigned char**)readPointer;
            (*readPointer)++;
            value = ((unsigned)value)-128 + 128 * (secondPart-128) + 16384 * thirdPart;
        }
    }
    return value;
}

/* Lines */
void putLexLines(int lines, char **writePointer) {
    putLexToken(LINE_TOKEN, writePointer);
    putLexToken(lines, writePointer);
}

/* Position */
void putLexPosition(int file, int line, int column, char **writePointer) {
    assert(file>=0 && file<MAX_FILES);
    putLexCompacted(file, writePointer);
    putLexCompacted(line, writePointer);
    putLexCompacted(column, writePointer);
}

Position getLexPosition(char **readPointer) {
    Position pos;
    pos.file = getLexCompacted(readPointer);
    pos.line = getLexCompacted(readPointer);
    pos.col = getLexCompacted(readPointer);
    return pos;
}

Position nextLexPosition(char **readPointer) {
    char *tmptmpcc = *readPointer;
    Position pos;
    pos = getLexPosition(&tmptmpcc);
    return pos;
}
