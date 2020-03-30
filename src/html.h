#ifndef HTML_H
#define HTML_H

#include "proto.h"
#include "symbol.h"

typedef struct intlist {
    int         i;
    struct intlist   *next;
} S_intlist;

typedef struct htmlData {
    struct position          *cp;
    struct reference         *np;
    struct symbolRefItem     *nri;
} S_htmlData;

typedef struct htmlRefList {
    struct symbolRefItem	*s;
    struct reference		*r;
    struct symbolRefItem	*slist;		/* the hash list containing s, for virtuals */
    struct htmlRefList		*next;
} S_htmlRefList;

extern void genClassHierarchyItemLinks(FILE *ff, S_olSymbolsMenu *itt,
                                       int virtFlag);
extern void htmlGenNonVirtualGlobSymList(FILE *ff, char *fn, S_symbolRefItem *p );
extern void htmlGenGlobRefsForVirtMethod(FILE *ff, char *fn,
                                         S_olSymbolsMenu *rrr);
extern int htmlRefItemsOrderLess(S_olSymbolsMenu *ss1, S_olSymbolsMenu *ss2);
extern int isAbsolutePath(char *p);
extern char *htmlNormalizedPath(char *p);
extern void recursivelyCreateFileDirIfNotExists(char *fpath);
extern void concatPaths(char *res, int rsize, char *p1, char *p2, char *p3);
extern void htmlPutChar(FILE *ff, int c);
extern void htmlGenGlobalReferenceLists(char *cxMemFreeBase);
extern void htmlAddJavaDocReference(Symbol  *p, S_position  *pos,
                                    int  vFunClass, int  vApplClass);
extern void htmlGetDefinitionReferences(void);
extern void htmlAddFunctionSeparatorReference(void);
extern void generateHtml(void);

#endif
