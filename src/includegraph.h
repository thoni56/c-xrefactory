#ifndef INCLUDEGRAPH_H_INCLUDED
#define INCLUDEGRAPH_H_INCLUDED

typedef struct includeGraph IncludeGraph;

extern IncludeGraph *buildIncludeGraph(void);
extern void freeIncludeGraph(IncludeGraph *graph);
extern int collectIncludeClosure(IncludeGraph *graph, int fileNumber, int fileNumbers[],
                                 int maxFileNumbers);

#endif
