#ifndef COMPLETION_H_INCLUDED
#define COMPLETION_H_INCLUDED

#include "reference.h"
#include "type.h"

typedef struct completion {
    char                 *name;
    char                 *fullName;
    char                 *vclass;
    short int             jindent;
    short int             lineCount;
    char                  category; /* Global/Local TODO: enum!*/
    Type                  csymType; /* symtype of completion */
    struct reference      ref;
    struct referencesItem sym;
    struct completion  *next;
} Completion;


#endif
