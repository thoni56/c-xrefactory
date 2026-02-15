#ifndef YYLEX_H_INCLUDED
#define YYLEX_H_INCLUDED

#include <stdio.h>
#include "editorbuffer.h"
#include "lexem.h"
#include "position.h"


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

extern int getMacroBodyMemoryIndex(void);
extern void setMacroBodyMemoryIndex(int index);

extern void initAllInputs(void);

/* Expose helper to mask a macro as undefined (mbody == NULL) */
void undefineMacroByName(const char *name);
extern void initInput(FILE *file, EditorBuffer *buffer, char *prepend, char *fileName);
extern void addFileAsIncludeReference(int filenum);
extern void pushInclude(FILE *file, EditorBuffer *buff, char *name, char *prepend);
extern void popInclude(void);
extern void addMacroDefinedByOption(char *option);
extern LexemCode cppexp_yylex(void);
extern LexemCode yylex(void);

extern void yylexMemoryStatistics(void);

#endif
