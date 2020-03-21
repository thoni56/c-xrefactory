#ifndef YYLEX_H
#define YYLEX_H

#include "proto.h"
#include "editor.h"


extern char *yytext;


extern void ppMemInit(void);
extern void initAllInputs(void);
extern void initInput(FILE *ff, S_editorBuffer *buffer, char *prepend, char *name);
extern void addIncludeReference(int filenum, S_position *pos);
extern void addThisFileDefineIncludeReference(int filenum);
extern void pushNewInclude(FILE *f, S_editorBuffer *buff, char *name, char *prepend);
extern void popInclude(void);
extern int addFileTabItem(char *name);
extern void addMacroDefinedByOption(char *opt);
extern char *placeIdent(void);
extern int cachedInputPass(int cpoint, char **cfromto);
extern int cexpyylex(void);
extern int yylex(void);

#endif
