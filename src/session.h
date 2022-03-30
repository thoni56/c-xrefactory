#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "proto.h"

typedef struct SessionData {
    struct OlcxReferencesStack	browserStack;
    struct OlcxReferencesStack	completionsStack;
    struct OlcxReferencesStack	retrieverStack;
    struct classTreeData		classTree;
} SessionData;


extern SessionData sessionData;

#endif
