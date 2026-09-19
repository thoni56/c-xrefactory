#ifndef REFERENCE_H_INCLUDED
#define REFERENCE_H_INCLUDED

/**
 * @file reference.h
 * @brief Reference List Management
 *
 * Category: Parsing Support Library - Data Structures
 *
 * Manages lists of symbol references (occurrences/usages) during parsing.
 * Not semantic actions itself, but provides data structures used by
 * symbol tracking during parse.
 *
 * Adapts behavior based on parsingConfig.operation:
 * - PARSER_OP_EXTRACT: Allows duplicate references at same position
 *   (needed for data flow analysis)
 * - Other operations: Deduplicates references at same position
 *   (updates usage type instead)
 *
 * Called from: cxref.c (handleFoundSymbolReference during parsing),
 *              cxfile.c (loading references from database)
 */

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
