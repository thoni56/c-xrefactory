#include "lexembuffer.h"

void initLexemBuffer(LexemBuffer *buffer, FILE *file) {
    buffer->next = buffer->chars;
    buffer->end = buffer->chars;
    buffer->index = 0;
    initCharacterBuffer(&buffer->buffer, file);
}
