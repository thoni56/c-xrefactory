/* -*- c -*-*/

/* These are the parser/yacc fields required. They should be the same
   for all parsers since the YYSTYPE need to be the same. Include this
   file in the %union section instead of local field declarations and
   make additions here.
*/

Symbol                                 *symbol;
TypeModifier                           *typeModifier;
FrameAllocation                        *frameAllocation;

Ast_int                                ast_integer;
Ast_unsigned                           ast_unsigned;
Ast_symbol                             ast_symbol;
Ast_symbolList                         ast_symbolList;
Ast_typeModifiers                      ast_typeModifiers;
Ast_id                                 ast_id;
Ast_idList                             ast_idList;
Ast_expressionTokenType                ast_expressionType;
Ast_intPair                            ast_intPair;
Ast_whileExtractData                   ast_whileData;
Ast_position                           ast_position;
Ast_unsignedPositionPair               ast_unsignedPositionPair;
Ast_symbolPositionPair                 ast_symbolPositionPair;
Ast_symbolPositionListPair             ast_symbolPositionListPair;
Ast_positionList                       ast_positionList;
Ast_typeModifiersListPositionListPair  ast_typeModifiersListPositionListPair;
Ast_nestedConstrTokenType              ast_nestedConstrTokenType;
