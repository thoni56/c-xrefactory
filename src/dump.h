#ifndef DUMP_H_INCLUDED
#define DUMP_H_INCLUDED

/* Answers with facts from the in-memory table, one record per fact. The selection
 * names which facts, so a new kind of fact never changes the answer for an old one. */
extern void dumpTable(char *selection);

#endif
