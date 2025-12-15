#ifndef REFERENCE_H_INCLUDED
#define REFERENCE_H_INCLUDED

#include "position.h"
#include "usage.h"


// An occurence of a referenceableItem
typedef struct reference {
    struct position   position;
    Usage             usage;
    struct reference *next;
} Reference;


extern Reference *newReference(Position position, Usage usage, Reference *next);
extern Reference makeReference(Position position, Usage usage, Reference *next);
extern Reference *duplicateReferenceInCxMemory(Reference *r);
extern void freeReferences(Reference *references);
extern void setReferenceUsage(Reference *reference, Usage usage);
extern Reference **addToReferenceList(Reference **list, Position pos, Usage usage);
extern bool isReferenceInList(Reference *r, Reference *list);
extern Reference *addReferenceToList(Reference *ref, Reference **rlist);
extern int fileNumberOfReference(Reference reference);

#endif
