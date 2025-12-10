#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "referencesstack.h"

typedef struct SessionData {
    BrowserStack	browserStack;
    CompletionStack	completionsStack;
    RetrieverStack	retrieverStack;
} SessionData;


extern SessionData sessionData;

#endif
