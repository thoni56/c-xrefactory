#ifndef _RECYACC__H

struct yyGlobalState {
	int gyydebug;
	int gyynerrs;
	int gyyerrflag;
	int gyychar;
	int lastgyystate;
	short *gyyssp;
	YYSTYPE *gyyvsp;
	YYSTYPE gyyval;
	YYSTYPE gyylval;
	short gyyss[YYSTACKSIZE];
	YYSTYPE gyyvs[YYSTACKSIZE];
};

extern struct yyGlobalState *s_yygstate;
extern struct yyGlobalState *s_initYygstate;

#define yylval (s_yygstate->gyylval)
#define yydebug (s_yygstate->gyydebug)
#define yynerrs (s_yygstate->gyynerrs)
#define yyerrflag (s_yygstate->gyyerrflag)
#define yychar (s_yygstate->gyychar)
#define lastyystate (s_yygstate->lastgyystate)
#define yyssp (s_yygstate->gyyssp)
#define yyval (s_yygstate->gyyval)
#define yyss (s_yygstate->gyyss)
#define yyvs (s_yygstate->gyyvs)
#define yyvsp (s_yygstate->gyyvsp)

#endif
