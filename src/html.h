#ifndef HTML_H
#define HTML_H

#include "proto.h"

extern void genClassHierarchyItemLinks(FILE *ff, S_olSymbolsMenu *itt,
                                int virtFlag);
extern void htmlGenNonVirtualGlobSymList(FILE *ff, char *fn, S_symbolRefItem *p );
extern void htmlGenGlobRefsForVirtMethod(FILE *ff, char *fn,
                                  S_olSymbolsMenu *rrr);
extern int htmlRefItemsOrderLess(S_olSymbolsMenu *ss1, S_olSymbolsMenu *ss2);
extern int isThereSomethingPrintable(S_olSymbolsMenu *itt);
extern void htmlGenEmptyRefsFile();
extern void javaGetClassNameFromFileNum(int nn, char *tmpOut, int dotify);
extern void javaSlashifyDotName(char *ss);
extern void javaDotifyClassName(char *ss);
extern void javaDotifyFileName( char *ss);
extern int isAbsolutePath(char *p);
extern char *htmlNormalizedPath(char *p);
extern char *htmlGetLinkFileNameStatic(char *link, char *file);
extern void recursivelyCreateFileDirIfNotExists(char *fpath);
extern void concatPathes(char *res, int rsize, char *p1, char *p2, char *p3);
extern void htmlPutChar(FILE *ff, int c);
extern void htmlPrint(FILE *ff, char *ss);
extern void htmlGenGlobalReferenceLists(char *cxMemFreeBase);
extern void htmlAddJavaDocReference(S_symbol  *p, S_position  *pos,
                             int  vFunClass, int  vApplClass);
extern void generateHtml();
extern int addHtmlCutPath(char *ss );
extern void htmlGetDefinitionReferences();
extern void htmlAddFunctionSeparatorReference();

#endif
