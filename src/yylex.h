#ifndef YYLEX_H_INCLUDED
#define YYLEX_H_INCLUDED

#include "proto.h"
#include "editor.h"
#include "lexem.h"

/* ************************ PRE-PROCESSOR **************************** */

typedef struct cppIfStack {
    struct position    position;
    struct cppIfStack *next;
} CppIfStack;

typedef struct macroBody {
    short int argCount;
    int size;
    char *name;                 /* the name of the macro */
    char **argumentNames;       /* names of arguments */
    char *body;
} MacroBody;

typedef struct {
    char *name;
    char *linkName;
    int order;
} MacroArgumentTableElement;


extern char *yytext;
extern int macroStackIndex;

extern int mbMemoryIndex;       /* Needed by caching.c */

extern void initAllInputs(void);
extern void initInput(FILE *file, EditorBuffer *buffer, char *prepend, char *fileName);
extern void addIncludeReference(int filenum, Position *pos);
extern void addThisFileDefineIncludeReference(int filenum);
extern void pushInclude(FILE *file, EditorBuffer *buff, char *name, char *prepend);
extern void popInclude(void);
extern void addMacroDefinedByOption(char *opt);
extern char *placeIdent(void);
extern int cachedInputPass(int cpoint, char **cfromto);
extern Lexem cexp_yylex(void);
extern Lexem yylex(void);

#endif
