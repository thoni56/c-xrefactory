#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED


typedef enum inputType {
    INPUT_NORMAL,
    INPUT_MACRO,
    INPUT_MACRO_ARGUMENT,
    INPUT_CACHE,
} InputType;

typedef struct {
    char *begin;                /* where it begins */
    char *read;                 /* next to read */
    char *write;                /* where to write next */
    char *macroName;
    InputType inputType;
} LexInput;


extern LexInput currentInput;

extern LexInput makeLexInput(char *begin, char *read, char *write, char *macroName, InputType inputType);

#endif
