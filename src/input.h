#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include <stdbool.h>

typedef enum inputType {
    NORMAL_STREAM,
    MACRO_STREAM,
    MACRO_ARGUMENT_STREAM,
    INPUT_CACHE,                /* TODO: Remove this value, not used */
} InputType;

typedef struct {
    char *begin;                /* where it begins */
    char *read;                 /* next to read */
    char *write;                /* where to write next */
    char *macroName;
    InputType streamType;
} LexemStream;


extern LexemStream currentInput;

extern LexemStream makeLexemStream(char *begin, char *read, char *write, char *macroName, InputType inputType);

/* LexInput API functions */
bool lexemStreamHasMore(LexemStream *input);

#endif
