#include "lexembuffer.h"

void initLexemBuffer(LexemBuffer *buffer, FILE *file) {
    buffer->next = buffer->chars;
    buffer->end = buffer->chars;
    buffer->index = 0;
    initCharacterBuffer(&buffer->buffer, file);
}

void putLexChar(char ch, char **destinationPointer) {
    **destinationPointer = ch;
    (*destinationPointer)++;
}

void putLexShort(int shortValue, char **destinationPointer) {
    **destinationPointer = ((unsigned)shortValue)%256;
    (*destinationPointer)++;
    **destinationPointer = ((unsigned)shortValue)/256;
    (*destinationPointer)++;
}

unsigned char getLexChar(char **nextPointer) {
    unsigned char ch = **nextPointer;
    (*nextPointer)++;
    return ch;
}
