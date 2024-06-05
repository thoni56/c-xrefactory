#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "proto.h"

typedef struct SessionData {
    OlcxReferencesStack	browserStack;
    OlcxReferencesStack	completionsStack;
    OlcxReferencesStack	retrieverStack;
    ClassTreeData		classTree;
} SessionData;


extern SessionData sessionData;

#endif
