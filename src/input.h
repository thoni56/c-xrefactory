#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "proto.h" /* For InputType */

typedef struct {
    char *read;
    char *begin;
    char *write;
    char *macroName;
    InputType inputType;
} LexInput;


extern LexInput currentInput;

extern void fillLexInput(LexInput *input, char *read, char *begin, char *write, char *macroName,
                         InputType inputType);

#endif
