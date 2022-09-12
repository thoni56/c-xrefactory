#include "input.h"

LexInput currentInput;

void fillLexInput(LexInput *input, char *read, char *begin, char *write, char *macroName,
                  InputType inputType) {
    input->read      = read;
    input->begin     = begin;
    input->write     = write;
    input->macroName = macroName;
    input->inputType = inputType;
}
