#include "lexem.h"

char *lexemEnumNames[LAST_TOKEN];

void initLexemEnumNames(void) {
    lexemEnumNames['<'] = "LT";
    lexemEnumNames['>'] = "GT";
    lexemEnumNames['('] = "LPAR";
    lexemEnumNames[')'] = "RPAR";
    lexemEnumNames['{'] = "LBRACE";
    lexemEnumNames['}'] = "RBRACE";
    lexemEnumNames['['] = "LBRACKET";
    lexemEnumNames[']'] = "RBRACKET";
    lexemEnumNames['='] = "EQ";
    lexemEnumNames['\n'] = "NL";
    lexemEnumNames['"'] = "DQUOTE";
    lexemEnumNames['\''] = "SQUOTE";
    lexemEnumNames['-'] = "MINUS";
    lexemEnumNames['+'] = "PLUS";
    lexemEnumNames['*'] = "ASTERISK";
    lexemEnumNames['!'] = "EXLAMATION";
    lexemEnumNames['?'] = "QUESTION";
    lexemEnumNames[','] = "COMMA";
    lexemEnumNames['.'] = "PERIOD";
    lexemEnumNames[':'] = "COLON";
    lexemEnumNames[';'] = "SEMICOLON";
    lexemEnumNames['#'] = "HASH";
    lexemEnumNames['/'] = "SLASH";
    lexemEnumNames['&'] = "AMPERSAND";
    lexemEnumNames['|'] = "BAR";
    lexemEnumNames['~'] = "TILDE";
    lexemEnumNames['%'] = "PERCENT";
    lexemEnumNames[257] = "TYPE_NAME";
    lexemEnumNames[258] = "INC_OP";
    lexemEnumNames[259] = "DEC_OP";
    lexemEnumNames[260] = "LEFT_OP";
    lexemEnumNames[261] = "RIGHT_OP";
    lexemEnumNames[262] = "LE_OP";
    lexemEnumNames[263] = "GE_OP";
    lexemEnumNames[264] = "EQ_OP";
    lexemEnumNames[265] = "NE_OP";
    lexemEnumNames[266] = "AND_OP";
    lexemEnumNames[267] = "OR_OP";
    lexemEnumNames[268] = "MUL_ASSIGN";
    lexemEnumNames[269] = "DIV_ASSIGN";
    lexemEnumNames[270] = "MOD_ASSIGN";
    lexemEnumNames[271] = "ADD_ASSIGN";
    lexemEnumNames[272] = "SUB_ASSIGN";
    lexemEnumNames[273] = "LEFT_ASSIGN";
    lexemEnumNames[274] = "RIGHT_ASSIGN";
    lexemEnumNames[275] = "AND_ASSIGN";
    lexemEnumNames[276] = "XOR_ASSIGN";
    lexemEnumNames[277] = "OR_ASSIGN";
    lexemEnumNames[278] = "PTR_OP";
    lexemEnumNames[279] = "ELLIPSIS";
    lexemEnumNames[280] = "YACC_PERC";
    lexemEnumNames[281] = "YACC_DPERC";
    lexemEnumNames[282] = "STATIC";
    lexemEnumNames[283] = "BREAK";
    lexemEnumNames[284] = "CASE";
    lexemEnumNames[285] = "CHAR";
    lexemEnumNames[286] = "CONST";
    lexemEnumNames[287] = "CONTINUE";
    lexemEnumNames[288] = "DEFAULT";
    lexemEnumNames[289] = "DO";
    lexemEnumNames[290] = "DOUBLE";
    lexemEnumNames[291] = "ELSE";
    lexemEnumNames[292] = "FLOAT";
    lexemEnumNames[293] = "FOR";
    lexemEnumNames[294] = "GOTO";
    lexemEnumNames[295] = "IF";
    lexemEnumNames[296] = "INT";
    lexemEnumNames[297] = "LONG";
    lexemEnumNames[298] = "RETURN";
    lexemEnumNames[299] = "SHORT";
    lexemEnumNames[300] = "SWITCH";
    lexemEnumNames[301] = "VOID";
    lexemEnumNames[302] = "VOLATILE";
    lexemEnumNames[303] = "WHILE";
    lexemEnumNames[304] = "TYPEDEF";
    lexemEnumNames[305] = "EXTERN";
    lexemEnumNames[306] = "AUTO";
    lexemEnumNames[307] = "REGISTER";
    lexemEnumNames[308] = "SIGNED";
    lexemEnumNames[309] = "UNSIGNED";
    lexemEnumNames[310] = "STRUCT";
    lexemEnumNames[311] = "UNION";
    lexemEnumNames[312] = "ENUM";
    lexemEnumNames[313] = "SIZEOF";
    lexemEnumNames[314] = "RESTRICT";
    lexemEnumNames[315] = "_ATOMIC";
    lexemEnumNames[316] = "_BOOL";
    lexemEnumNames[317] = "_THREADLOCAL";
    lexemEnumNames[318] = "_NORETURN";
    lexemEnumNames[319] = "_STATIC_ASSERT";
    lexemEnumNames[320] = "INLINE";
    lexemEnumNames[321] = "ASM_KEYWORD";
    lexemEnumNames[322] = "ANONYMOUS_MODIFIER";
    lexemEnumNames[323] = "TRUE_LITERAL";
    lexemEnumNames[324] = "FALSE_LITERAL";
    lexemEnumNames[325] = "NULL_LITERAL";
    lexemEnumNames[326] = "TOKEN";
    lexemEnumNames[327] = "TYPE";
    lexemEnumNames[328] = "LABEL";
    lexemEnumNames[329] = "COMPLETE_FOR_STATEMENT1";
    lexemEnumNames[330] = "COMPLETE_FOR_STATEMENT2";
    lexemEnumNames[331] = "COMPLETE_TYPE_NAME";
    lexemEnumNames[332] = "COMPLETE_STRUCT_NAME";
    lexemEnumNames[333] = "COMPLETE_STRUCT_MEMBER_NAME";
    lexemEnumNames[334] = "COMPLETE_UP_FUN_PROFILE";
    lexemEnumNames[335] = "COMPLETE_ENUM_NAME";
    lexemEnumNames[336] = "COMPLETE_LABEL_NAME";
    lexemEnumNames[337] = "COMPLETE_OTHER_NAME";
    lexemEnumNames[338] = "COMPLETE_YACC_LEXEM_NAME";
    lexemEnumNames[339] = "CPP_TOKENS_START";
    lexemEnumNames[340] = "CPP_INCLUDE";
    lexemEnumNames[341] = "CPP_INCLUDE_NEXT";
    lexemEnumNames[342] = "CPP_DEFINE";
    lexemEnumNames[343] = "CPP_IFDEF";
    lexemEnumNames[344] = "CPP_IFNDEF";
    lexemEnumNames[345] = "CPP_IF";
    lexemEnumNames[346] = "CPP_ELSE";
    lexemEnumNames[347] = "CPP_ENDIF";
    lexemEnumNames[348] = "CPP_ELIF";
    lexemEnumNames[349] = "CPP_UNDEF";
    lexemEnumNames[350] = "CPP_PRAGMA";
    lexemEnumNames[351] = "CPP_LINE";
    lexemEnumNames[352] = "CPP_DEFINE0";
    lexemEnumNames[353] = "CPP_TOKENS_END";
    lexemEnumNames[354] = "CPP_COLLATION";
    lexemEnumNames[355] = "CPP_DEFINED_OP";
    lexemEnumNames[356] = "EOI_TOKEN";
    lexemEnumNames[357] = "OL_MARKER_TOKEN";
    lexemEnumNames[358] = "OL_MARKER_TOKEN1";
    lexemEnumNames[359] = "OL_MARKER_TOKEN2";
    lexemEnumNames[360] = "TMP_TOKEN1";
    lexemEnumNames[361] = "TMP_TOKEN2";
    lexemEnumNames[362] = "MULTI_TOKENS_START";
    lexemEnumNames[363] = "IDENTIFIER";
    lexemEnumNames[364] = "CONSTANT";
    lexemEnumNames[365] = "LONG_CONSTANT";
    lexemEnumNames[366] = "FLOAT_CONSTANT";
    lexemEnumNames[367] = "DOUBLE_CONSTANT";
    lexemEnumNames[368] = "STRING_LITERAL";
    lexemEnumNames[369] = "CHAR_LITERAL";
    lexemEnumNames[370] = "LINE_TOKEN";
    lexemEnumNames[371] = "IDENT_TO_COMPLETE";
    lexemEnumNames[372] = "CPP_MACRO_ARGUMENT";
    lexemEnumNames[373] = "IDENT_NO_CPP_EXPAND";
}
