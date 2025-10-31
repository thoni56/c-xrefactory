#include "input.h"

LexInput currentInput;

LexInput makeLexInput(char *begin, char *read, char *write, char *macroName, InputType inputType) {
    LexInput input;
    input.begin     = begin;
    input.read      = read;
    input.write     = write;
    input.macroName = macroName;
    input.inputType = inputType;

    return input;
}

bool lexInputHasMore(LexInput *input) {
    return input->read < input->write;
}
