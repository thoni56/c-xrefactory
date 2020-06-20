#include "lexembuffer.h"


void initLexemBuffer(LexemBuffer *buffer, FILE *file) {
    buffer->next = buffer->chars;
    buffer->end = buffer->chars;
    buffer->index = 0;
    initCharacterBuffer(&buffer->buffer, file);
}

void putLexChar(char ch, char **writePointer) {
    **writePointer = ch;
    (*writePointer)++;
}

void putLexShort(int shortValue, char **writePointer) {
    **writePointer = ((unsigned)shortValue)%256;
    (*writePointer)++;
    **writePointer = ((unsigned)shortValue)/256;
    (*writePointer)++;
}

void putLexToken(Lexem lexem, char **writePointer) {
    putLexShort(lexem, writePointer);
}

void putLexInt(int value, char **writePointer) {
        unsigned tmp;
        tmp = value;
        *(*writePointer)++ = tmp%256; tmp /= 256;
        *(*writePointer)++ = tmp%256; tmp /= 256;
        *(*writePointer)++ = tmp%256; tmp /= 256;
        *(*writePointer)++ = tmp%256; tmp /= 256;
    }

unsigned char getLexChar(char **readPointer) {
    unsigned char ch = **readPointer;
    (*readPointer)++;
    return ch;
}

int getLexShort(char **readPointer) {
    int value = *(unsigned char*)(*readPointer);
    (*readPointer)++;
    value += 256 * *(unsigned char*)(*readPointer);
    (*readPointer)++;
    return value;
}

Lexem getLexToken(char **readPointer) {
    return (Lexem)getLexShort(readPointer);
}
