#ifndef _ATR_X_H_
#define _ATR_X_H_

typedef struct list List;
typedef struct symbol Symbol;
typedef struct context Context;

extern void analyzeAttributes(List *attributeList, Symbol *symbol, Context *context);

#endif
