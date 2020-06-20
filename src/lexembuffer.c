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
