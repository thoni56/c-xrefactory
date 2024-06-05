#ifndef AST_INCLUDED
#define AST_INCLUDED

#include "symbol.h"
#include "id.h"
#include "proto.h"              /* Because some AST nodes need types */


/* **************     parse tree with positions    *********** */

// The following structures are used in the parsers, they always
// contain fields for begin and end position of parse tree
// node and additional 'data' for semantic actions.

typedef struct {
    struct position begin, end;
    int             data;
} Ast_int;
typedef struct {
    struct position begin, end;
    unsigned        data;
} Ast_unsigned;
typedef struct {
    struct position begin, end;
    struct symbol  *data;
} Ast_symbol;
typedef struct {
    struct position begin, end;
    struct symbolList *data;
} Ast_symbolList;
typedef struct {
    struct position begin, end;
    struct typeModifier *data;
} Ast_typeModifiers;
typedef struct {
    struct position begin, end;
    struct typeModifierList *data;
} Ast_typeModifiersList;
typedef struct {
    struct position begin, end;
    struct stackFrame *data;
} Ast_frameAllocation;
typedef struct {
    struct position begin, end;
    Id             *data;
} Ast_id;
typedef struct {
    struct position begin, end;
    struct idList  *data;
} Ast_idList;
typedef struct {
    struct position begin, end;
    struct expressionTokenType data;
} Ast_expressionTokenType;
typedef struct {
    struct position begin, end;
    struct nestedConstrTokenType data;
} Ast_nestedConstrTokenType;
typedef struct {
    struct position begin, end;
    struct intPair  data;
} Ast_intPair;
typedef struct {
    struct position begin, end;
    struct whileExtractData *data;
} Ast_whileExtractData;
typedef struct {
    struct position begin, end;
    struct position data;
} Ast_position;
typedef struct {
    struct position begin, end;
    struct unsignedPositionPair data;
} Ast_unsignedPositionPair;
typedef struct {
    struct position begin, end;
    struct symbolPositionPair data;
} Ast_symbolPositionPair;
typedef struct {
    struct position begin, end;
    struct symbolPositionListPair data;
} Ast_symbolPositionListPair;
typedef struct {
    struct position begin, end;
    struct positionList *data;
} Ast_positionList;
typedef struct {
    struct position begin, end;
    struct typeModifiersListPositionListPair data;
} Ast_typeModifiersListPositionListPair;

#endif
