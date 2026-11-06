#include "includegraph.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filetable.h"
#include "referenceableitemtable.h"
#include "type.h"


typedef struct {
    int  includerFileNumber;
    int *fileNumbers;
    int  maxFileNumbers;
    int  count;
} IncludedFiles;


static int storedCount(IncludedFiles *included) {
    return included->count < included->maxFileNumbers ? included->count : included->maxFileNumbers;
}

static bool isCollected(IncludedFiles *included, int fileNumber) {
    for (int i = 0; i < storedCount(included); i++)
        if (included->fileNumbers[i] == fileNumber)
            return true;
    return false;
}

typedef struct {
    int includer;
    int included;
} IncludeEdge;

struct includeGraph {
    IncludeEdge *edges;             /* Sorted by includer */
    IncludeEdge *edgesByIncluded;   /* The same edges, sorted by included */
    int count;
    int allocated;
};

static void addEdge(IncludeGraph *graph, int includer, int included) {
    if (graph->count == graph->allocated) {
        graph->allocated = graph->allocated == 0 ? 64 : 2 * graph->allocated;
        graph->edges = realloc(graph->edges, graph->allocated * sizeof(IncludeEdge));
    }
    graph->edges[graph->count++] = (IncludeEdge){.includer = includer, .included = included};
}

static void addEdgesOf(ReferenceableItem *item, void *graphP) {
    if (item->type != TypeCppInclude)
        return;
    for (Reference *r = item->references; r != NULL; r = r->next)
        if (r->usage != UsageDefined)
            addEdge(graphP, r->position.file, item->includeFileNumber);
}

static void addScannedEdgesOf(FileItem *fileItem, int fileNumber, IncludeGraph *graph) {
    for (int i = 0; i < fileItem->scannedIncludeCount; i++)
        addEdge(graph, fileNumber, fileItem->scannedIncludes[i]);
}

static int compareByIncluder(const void *a, const void *b) {
    const IncludeEdge *edgeA = a, *edgeB = b;
    if (edgeA->includer != edgeB->includer)
        return edgeA->includer < edgeB->includer ? -1 : 1;
    return 0;
}

static int compareByIncluded(const void *a, const void *b) {
    const IncludeEdge *edgeA = a, *edgeB = b;
    if (edgeA->included != edgeB->included)
        return edgeA->included < edgeB->included ? -1 : 1;
    return 0;
}

/* One pass over the include references and one over the includes the scan
 * found, so a closure can be walked without either. Sorted both ways, so what
 * a file includes, and what includes it, are each one run of edges */
IncludeGraph *buildIncludeGraph(void) {
    IncludeGraph *graph = calloc(1, sizeof(IncludeGraph));
    mapOverReferenceableItemTableWithPointer(addEdgesOf, graph);
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1))
        addScannedEdgesOf(getFileItemWithFileNumber(i), i, graph);
    if (graph->count > 0) {
        qsort(graph->edges, graph->count, sizeof(IncludeEdge), compareByIncluder);
        graph->edgesByIncluded = malloc(graph->count * sizeof(IncludeEdge));
        memcpy(graph->edgesByIncluded, graph->edges, graph->count * sizeof(IncludeEdge));
        qsort(graph->edgesByIncluded, graph->count, sizeof(IncludeEdge), compareByIncluded);
    }
    return graph;
}

void freeIncludeGraph(IncludeGraph *graph) {
    free(graph->edges);
    free(graph->edgesByIncluded);
    free(graph);
}

static int firstEdgeOf(IncludeGraph *graph, int includer) {
    int low = 0, high = graph->count;
    while (low < high) {
        int middle = low + (high - low) / 2;
        if (graph->edges[middle].includer < includer)
            low = middle + 1;
        else
            high = middle;
    }
    return low;
}

static void collectIncludedBy(IncludeGraph *graph, IncludedFiles *included) {
    for (int i = firstEdgeOf(graph, included->includerFileNumber);
         i < graph->count && graph->edges[i].includer == included->includerFileNumber; i++) {
        IncludeEdge *edge = &graph->edges[i];
        if (!isCollected(included, edge->included)) {
            if (included->count < included->maxFileNumbers)
                included->fileNumbers[included->count] = edge->included;
            included->count++;
        }
    }
}

int collectIncludeClosure(IncludeGraph *graph, int fileNumber, int fileNumbers[],
                          int maxFileNumbers) {
    IncludedFiles included = {.includerFileNumber = fileNumber, .fileNumbers = fileNumbers,
                              .maxFileNumbers = maxFileNumbers};
    collectIncludedBy(graph, &included);
    for (int i = 0; i < storedCount(&included); i++) {
        included.includerFileNumber = fileNumbers[i];
        collectIncludedBy(graph, &included);
    }
    return included.count;
}

static int firstEdgeIncluding(IncludeGraph *graph, int included) {
    int low = 0, high = graph->count;
    while (low < high) {
        int middle = low + (high - low) / 2;
        if (graph->edgesByIncluded[middle].included < included)
            low = middle + 1;
        else
            high = middle;
    }
    return low;
}

static void collectIncludersOf(IncludeGraph *graph, int fileNumber, IncludedFiles *includers) {
    for (int i = firstEdgeIncluding(graph, fileNumber);
         i < graph->count && graph->edgesByIncluded[i].included == fileNumber; i++) {
        IncludeEdge *edge = &graph->edgesByIncluded[i];
        if (!isCollected(includers, edge->includer)) {
            if (includers->count < includers->maxFileNumbers)
                includers->fileNumbers[includers->count] = edge->includer;
            includers->count++;
        }
    }
}

int collectIncluderClosure(IncludeGraph *graph, int fileNumber, int fileNumbers[],
                           int maxFileNumbers) {
    IncludedFiles includers = {.fileNumbers = fileNumbers, .maxFileNumbers = maxFileNumbers};
    collectIncludersOf(graph, fileNumber, &includers);
    for (int i = 0; i < storedCount(&includers); i++)
        collectIncludersOf(graph, fileNumbers[i], &includers);
    return includers.count;
}
