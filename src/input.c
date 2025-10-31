#include "input.h"

LexemStream currentInput;

LexemStream makeLexemStream(char *begin, char *read, char *write, char *macroName, InputType streamType) {
    LexemStream stream;
    stream.begin      = begin;
    stream.read       = read;
    stream.write      = write;
    stream.macroName  = macroName;
    stream.streamType = streamType;

    return stream;
}

bool lexemStreamHasMore(LexemStream *stream) {
    return stream->read < stream->write;
}
