#include "lex.h"

#include "globals.h"
#include "commons.h"
#include "parsers.h"
#include "cxref.h"
#include "characterbuffer.h"
#include "yylex.h"
#include "filedescriptor.h"

#include "caching.h"            /* cacheInput() */
#include "jslsemact.h"          /* s_jsl */

#include "log.h"
#include "utils.h"              /* creatingOlcxRefs() */


void gotOnLineCxRefs( Position *ps ) {
    if (creatingOlcxRefs()) {
        s_cache.activeCache = 0;
        s_cxRefPos = *ps;
    }
}

/* ***************************************************************** */
/*                         Lexical Analysis                          */
/* ***************************************************************** */

#define PutLexLine(line,dd) {                               \
        if (line!=0) {                                      \
            PutLexToken(LINE_TOK,dd); PutLexToken(line,dd); \
        }                                                   \
    }

#define PutCurrentLexPosition(ccc,dd,cline,clo,clb,cfile) { \
        int ccol;                                           \
        ccol = COLUMN_POS(ccc,clb,clo);                     \
        PutLexPosition(cfile,cline,ccol,dd);                \
    }                                                       \

#define GetChar(cch, ccc, ffin, bbb, clb, clo) {        \
        if (ccc >= ffin) {                              \
            bbb->next = ccc;                            \
            clo = ccc-clb;                              \
            if (bbb->isAtEOF || fillBuffer(bbb) == 0) { \
                cch = -1;                               \
                bbb->isAtEOF = true;                    \
            } else {                                    \
                ccc = bbb->next; ffin = bbb->end;       \
                clb = ccc;                              \
                cch = * ((unsigned char *)ccc);         \
                ccc = ((char *)ccc) + 1;                \
            }                                           \
        } else {                                        \
            cch = * ((unsigned char *)ccc);             \
            ccc = ((char *)ccc) + 1;                    \
        }                                               \
   }

#define UngetChar(cch, ccc) {                        \
    log_trace("Ungetting %s(%c) at %s", #cch, cch, #ccc); \
    *--ccc = cch;                                         \
}

#define DeleteBlank(cch,ccc,ffin,bbb,clb,clo) {                         \
        while (cch==' '|| cch=='\t' || cch=='\004') {                   \
            GetChar(cch,ccc,ffin,bbb,clb,clo);                          \
        }                                                               \
    }

#define PassComment(ch,oldCh,ccc,cfin,cb,dd,cline,clb,clo) {            \
        /*  ******* a block comment ******* */                          \
        line = cline;                                                   \
        GetChar(ch,ccc,cfin,cb,clb,clo);                                \
        if (ch=='\n') {cline ++; clb = ccc; clo = 0;}                   \
        /* TODO test on cpp directive */                                \
        do {                                                            \
            oldCh = ch;                                                 \
            GetChar(ch,ccc,cfin,cb,clb,clo);                            \
            if (ch=='\n') {cline ++; clb = ccc; clo = 0;}               \
            /* TODO test on cpp directive */                            \
        } while ((oldCh != '*' || ch != '/') && ch != -1);              \
        if (ch == -1) warningMessage(ERR_ST,"comment through eof");            \
        PutLexLine(cline-line,dd);                                      \
        GetChar(ch,ccc,cfin,cb,clb,clo);                                \
    }

#define conType(val, cch, ccc, ffin, bbb, clb, clo, lex) {      \
        lex = CONSTANT;                                         \
        if (LANGUAGE(LANG_JAVA)) {                               \
            if (cch=='l' || cch=='L') {                         \
                lex = LONG_CONSTANT;                            \
                GetChar(cch, ccc, ffin, bbb, clb, clo);         \
            }                                                   \
        } else {                                                \
            for(; cch=='l'||cch=='L'||cch=='u'||cch=='U'; ){    \
                if (cch=='l' || cch=='L') lex = LONG_CONSTANT;  \
                GetChar(cch, ccc, ffin, bbb, clb, clo);         \
            }                                                   \
        }                                                       \
    }

#define fpConstFin(cch, ccc, ffin, bbb, clb, clo, lex) {                \
        lex = DOUBLE_CONSTANT;                                          \
        if (cch == '.') {                                               \
            do { GetChar(cch, ccc, ffin, bbb, clb, clo);                \
            } while (isdigit(cch));                                     \
        }                                                               \
        if (cch == 'e' || cch == 'E') {                                 \
            GetChar(cch, ccc, ffin, bbb, clb, clo);                     \
            if (cch == '+' || cch=='-') GetChar(cch, ccc, ffin, bbb, clb, clo); \
            while (isdigit(cch)) GetChar(cch, ccc, ffin, bbb, clb, clo); \
        }                                                               \
        if (LANGUAGE(LANG_JAVA)) {                                      \
            if (cch == 'f' || cch == 'F' || cch == 'd' || cch == 'D') { \
                if (cch == 'f' || cch == 'F') lex = FLOAT_CONSTANT;     \
                GetChar(cch, ccc, ffin, bbb, clb, clo);                 \
            }                                                           \
        } else {                                                        \
            if (cch == 'f' || cch == 'F' || cch == 'l' || cch == 'L') { \
                GetChar(cch, ccc, ffin, bbb, clb, clo);                 \
            }                                                           \
        }                                                               \
   }

#define ProcessIdentifier(ch, ccc, cfin, cb, dd, cfile, cline, clb, clo, lab){ \
        int idcoll;                                                     \
        char *ddd;                                                      \
        /* ***************  identifier ****************************  */ \
        ddd = dd;                                                       \
        idcoll = COLUMN_POS(ccc,clb,clo);                               \
        PutLexToken(IDENTIFIER,dd);                                     \
        do {                                                            \
            PutLexChar(ch,dd);                                          \
        identCont##lab:                                                 \
            GetChar(ch,ccc,cfin,cb, clb, clo);                          \
        } while (isalpha(ch) || isdigit(ch) || ch=='_' || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))); \
        if (ch == '@' && *(dd-1)=='C') {                                \
            int i,len;                                                  \
            len = strlen(s_editCommunicationString);                    \
            for (i=2;i<len;i++) {                                       \
                GetChar(ch,ccc,cfin,cb, clb, clo);                      \
                if (ch != s_editCommunicationString[i]) break;          \
            }                                                           \
            if (i>=len) {                                               \
                /* it is the place marker */                            \
                dd--; /* delete the C */                                \
                GetChar(ch,ccc,cfin,cb, clb, clo);                      \
                if (ch == CC_COMPLETION) {                              \
                    PutLexToken(IDENT_TO_COMPLETE,ddd);                 \
                    GetChar(ch,ccc,cfin,cb, clb, clo);                  \
                } else if (ch == CC_CXREF) {                            \
                    s_cache.activeCache = 0;                            \
                    fillPosition(&s_cxRefPos,cfile,cline,idcoll);      \
                    goto identCont##lab;                                \
                } else errorMessage(ERR_INTERNAL,"unknown communication char"); \
            } else {                                                    \
                /* not a place marker, undo reading */                  \
                for(i--;i>=1;i--) {                                     \
                    UngetChar(ch, ccc);                                 \
                    ch = s_editCommunicationString[i];                  \
                }                                                       \
            }                                                           \
        }                                                               \
        PutLexChar(0,dd);                                               \
        PutLexPosition(cfile,cline,idcoll,dd);                          \
    }

#define HandleCppToken(ch,ccc,cfin,cb,dd, cfile, cline, clb, clo) {     \
        char *ddd,tt[10];                                               \
        int i,lcoll,scol;                                               \
        lcoll = COLUMN_POS(ccc,clb,clo);                                \
        GetChar(ch,ccc,cfin,cb, clb, clo);                              \
        DeleteBlank(ch,ccc,cfin,cb, clb, clo);                          \
        for(i=0; i<9 && (isalpha(ch) || isdigit(ch) || ch=='_') ; i++) { \
            tt[i] = ch;                                                 \
            GetChar(ch,ccc,cfin,cb, clb, clo);                          \
        }                                                               \
        tt[i]=0;                                                        \
        if (! strcmp(tt,"ifdef")) {                                     \
            PutLexToken(CPP_IFDEF,dd); PutLexPosition(cfile,cline,lcoll,dd); \
        } else if (! strcmp(tt,"ifndef")) {                             \
            PutLexToken(CPP_IFNDEF,dd); PutLexPosition(cfile,cline,lcoll,dd); \
        } else if (! strcmp(tt,"if")) {                                 \
            PutLexToken(CPP_IF,dd); PutLexPosition(cfile,cline,lcoll,dd); \
        } else if (! strcmp(tt,"elif")) {                               \
            PutLexToken(CPP_ELIF,dd); PutLexPosition(cfile,cline,lcoll,dd); \
        } else if (! strcmp(tt,"undef")) {                              \
            PutLexToken(CPP_UNDEF,dd); PutLexPosition(cfile,cline,lcoll,dd); \
        } else if (! strcmp(tt,"else")) {                               \
            PutLexToken(CPP_ELSE,dd); PutLexPosition(cfile,cline,lcoll,dd); \
        } else if (! strcmp(tt,"endif")) {                              \
            PutLexToken(CPP_ENDIF,dd); PutLexPosition(cfile,cline,lcoll,dd); \
        } else if (! strcmp(tt,"include")) {                            \
            char endCh;                                                 \
            PutLexToken(CPP_INCLUDE,dd);                                \
            PutLexPosition(cfile,cline,lcoll,dd);                       \
            DeleteBlank(ch,ccc,cfin,cb, clb, clo);                      \
            if (ch == '\"' || ch == '<') {                              \
                if (ch == '\"') endCh = '\"';                           \
                else endCh = '>';                                       \
                scol = COLUMN_POS(ccc,clb,clo);                         \
                PutLexToken(STRING_LITERAL,dd);                         \
                do { PutLexChar(ch,dd); GetChar(ch,ccc,cfin,cb, clb, clo); \
                } while (ch!=endCh && ch!='\n');                        \
                PutLexChar(0,dd);                                       \
                PutLexPosition(cfile,cline,scol,dd);                    \
                if (ch == endCh) GetChar(ch,ccc,cfin,cb, clb, clo);     \
            }                                                           \
        } else if (! strcmp(tt,"define")) {                             \
            ddd = dd;                                                   \
            PutLexToken(CPP_DEFINE0,dd);                                \
            PutLexPosition(cfile,cline,lcoll,dd);                       \
            DeleteBlank(ch,ccc,cfin,cb, clb, clo);                      \
            NOTE_NEW_LEXEM_POSITION(ccc,cfin,cb,lb,cfile,cline,clb,clo); \
            ProcessIdentifier(ch, ccc, cfin, cb, dd, cfile, cline, clb, clo,lab1); \
            if (ch == '(') {                                            \
                PutLexToken(CPP_DEFINE,ddd);                            \
            }                                                           \
        } else if (! strcmp(tt,"pragma")) {                             \
            PutLexToken(CPP_PRAGMA,dd); PutLexPosition(cfile,cline,lcoll,dd); \
        } else {                                                        \
            PutLexToken(CPP_LINE,dd); PutLexPosition(cfile,cline,lcoll,dd); \
        }                                                               \
   }

#define CommentaryBegRef(ccc, cfile, cline, clb, clo) {   \
        if (s_opt.taskRegime==RegimeHtmlGenerate && s_opt.htmlNoColors==0) { \
            int         lcoll;                                          \
            Position  pos;                                            \
            char        ttt[TMP_STRING_SIZE];                           \
            lcoll = COLUMN_POS(ccc,clb,clo) -1;                         \
            fillPosition(&pos, cfile, cline, lcoll);                   \
            sprintf(ttt,"%x/*",cfile);                                  \
            addTrivialCxReference(ttt,TypeComment,StorageDefault, &pos, UsageDefined); \
        }                                                               \
    }

#define CommentaryEndRef(ccc, cfile, cline, clb, clo, jdoc) { \
        if (s_opt.taskRegime==RegimeHtmlGenerate) {                     \
            int         lcoll;                                          \
            Position  pos;                                            \
            char        ttt[TMP_STRING_SIZE];                           \
            lcoll = COLUMN_POS(ccc,clb,clo);                            \
            fillPosition(&pos, cfile, cline, lcoll);                   \
            sprintf(ttt,"%x/*",cfile);                                  \
            if (s_opt.htmlNoColors==0) {                                \
                addTrivialCxReference(ttt,TypeComment,StorageDefault, &pos, UsageUsed); \
            }                                                           \
            if (jdoc) {                                                 \
                pos.col -= 2;                                          \
                addTrivialCxReference(ttt,TypeComment,StorageDefault, &pos, UsageJavaDoc); \
            }                                                           \
        }                                                               \
    }

#define NOTE_NEW_LEXEM_POSITION(ccc, cfin, cb, lb, cfile, cline, clb, clo){ \
        int pi = lb->posi % LEX_POSITIONS_RING_SIZE;                    \
        lb->fpRing[pi] = ABS_FILE_POS(cb,cfin,ccc);                     \
        lb->pRing[pi].file = cfile;                                     \
        lb->pRing[pi].line = cline;                                     \
        lb->pRing[pi].col = COLUMN_POS(ccc,clb,clo);                   \
        lb->posi ++;                                                    \
    }

#define PUT_EMPTY_COMPLETION_ID(ccc, dd, cline, clo, clb, cfile, llen) {  \
        PutLexToken(IDENT_TO_COMPLETE,dd);                          \
        PutLexChar(0,dd);                                           \
        PutLexPosition(cfile,cline,                                 \
                       COLUMN_POS(ccc,clb,clo) - (llen), dd);       \
    }

int getLexBuf(struct lexBuf *lb) {
    int ch;
    CharacterBuffer *cb;
    char *ccc, *cfin;
    char *cc, *dd, *lmax, *lexStartDd;
    unsigned chval=0;
    int rlex;
    int currentLineNumber;
    int currentLineOffset;
    char *currentLineBeginning;
    char oldCh;
    int line,size,cfile,lexStartCol, lexStartFilePos, column;

    /* first test whether the input is cached */
    if (s_cache.activeCache && inStacki==0 && macroStackIndex==0) {
        cacheInput();
        s_cache.lexcc = lb->chars;
    }
    lmax = lb->chars + LEX_BUFF_SIZE - MAX_LEXEM_SIZE;
    for(dd=lb->chars,cc=lb->next; cc<lb->end; cc++,dd++) *dd = *cc;
    lb->next = lb->chars;
    cb = &lb->buffer;
    currentLineNumber = cb->lineNum; currentLineBeginning = cb->lineBegin; currentLineOffset = cb->columnOffset;
    ccc = cb->next; cfin = cb->end; cfile = cb->fileNumber;
    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);

    do {
        DeleteBlank(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
        if (dd >= lmax) {
            UngetChar(ch,ccc);
            break;
        }
        NOTE_NEW_LEXEM_POSITION(ccc,cfin,cb,lb,cfile,currentLineNumber,currentLineBeginning,currentLineOffset);
        /*  yytext = ccc; */
        lexStartDd = dd;
        lexStartCol = COLUMN_POS(ccc,currentLineBeginning,currentLineOffset);
        if (ch == '_' || isalpha(ch) || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))) {
            ProcessIdentifier(ch, ccc, cfin, cb, dd, cfile, currentLineNumber, currentLineBeginning, currentLineOffset, lab2);
            goto nextLexem;
        } else if (isdigit(ch)) {
            /* ***************   number *******************************  */
            register long unsigned val=0;
            lexStartFilePos = ABS_FILE_POS(cb,cfin,ccc);
            if (ch=='0') {
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch=='x' || ch=='X') {
                    /* hexa */
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    while (isdigit(ch)||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F')) {
                        if (ch>='a') val = val*16+ch-'a'+10;
                        else if (ch>='A') val = val*16+ch-'A'+10;
                        else val = val*16+ch-'0';
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    }
                } else {
                    /* octal */
                    while (isdigit(ch) && ch<='8') {
                        val = val*8+ch-'0';
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    }
                }
            } else {
                /* decimal */
                while (isdigit(ch)) {
                    val = val*10+ch-'0';
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                }
            }
            if (ch == '.' || ch=='e' || ch=='E'
                || ((ch=='d' || ch=='D'|| ch=='f' || ch=='F') && LANGUAGE(LANG_JAVA))) {
                /* floating point */
                fpConstFin(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset,rlex);
                PutLexToken(rlex,dd);
                PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                PutLexInt(ABS_FILE_POS(cb,cfin,ccc)-lexStartFilePos, dd);
                goto nextLexem;
            }
            /* integer */
            conType(val,ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset, rlex);
            PutLexToken(rlex,dd);
            PutLexInt(val,dd);
            PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
            PutLexInt(ABS_FILE_POS(cb,cfin,ccc)-lexStartFilePos, dd);
            goto nextLexem;
        } else switch (ch) {
                /* ************   special character *********************  */
            case '.':
                lexStartFilePos = ABS_FILE_POS(cb,cfin,ccc);
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '.' && LANGUAGE(LANG_C|LANG_YACC)) {
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    if (ch == '.') {
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        PutLexToken(ELIPSIS,dd);
                        PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                        goto nextLexem;
                    } else {
                        UngetChar(ch,ccc);
                        ch = '.';
                    }
                    PutLexToken('.',dd);
                    PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                    goto nextLexem;
                } else if (isdigit(ch)) {
                    /* floating point constant */
                    UngetChar(ch,ccc);
                    ch = '.';
                    fpConstFin(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset,rlex);
                    PutLexToken(rlex,dd);
                    PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                    PutLexInt(ABS_FILE_POS(cb,cfin,ccc)-lexStartFilePos, dd);
                    goto nextLexem;
                } else {
                    PutLexToken('.',dd);
                    PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '-':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch=='=') {
                    PutLexToken(SUB_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd); GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    goto nextLexem;
                } else if (ch=='-') {PutLexToken(DEC_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem;}
                else if (ch=='>' && LANGUAGE(LANG_C|LANG_YACC)) {
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    PutLexToken(PTR_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;
                } else {PutLexToken('-',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case '+':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '=') { PutLexToken(ADD_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem;}
                else if (ch == '+') { PutLexToken(INC_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else {PutLexToken('+',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case '>':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '>') {
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    if(ch=='>' && LANGUAGE(LANG_JAVA)){
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        if(ch=='='){PutLexToken(URIGHT_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem;}
                        else {PutLexToken(URIGHT_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}
                    } else if(ch=='='){PutLexToken(RIGHT_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem;}
                    else {PutLexToken(RIGHT_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}
                } else if (ch == '='){PutLexToken(GE_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem;}
                else {PutLexToken('>',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case '<':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '<') {
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    if(ch=='='){PutLexToken(LEFT_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem;}
                    else {PutLexToken(LEFT_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}
                } else if (ch == '='){PutLexToken(LE_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem;}
                else {PutLexToken('<',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case '*':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '='){ PutLexToken(MUL_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else {PutLexToken('*',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case '%':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '='){ PutLexToken(MOD_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                /*&
                  else if (LANGUAGE(LANG_YACC) && ch == '{'){ PutLexToken(YACC_PERC_LPAR,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                  else if (LANGUAGE(LANG_YACC) && ch == '}'){ PutLexToken(YACC_PERC_RPAR,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                  &*/
                else {PutLexToken('%',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case '&':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '='){ PutLexToken(AND_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else if (ch == '&'){ PutLexToken(AND_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else if (ch == '*') {
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    if (ch == '/') {
                        /* a program commentary, ignore */
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        CommentaryEndRef(ccc,cfile,currentLineNumber,currentLineBeginning,currentLineOffset,0);
                        goto nextLexem;
                    } else {
                        UngetChar(ch,ccc); ch = '*';
                        PutLexToken('&',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;
                    }}
                else {PutLexToken('&',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case '^':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '='){ PutLexToken(XOR_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else {PutLexToken('^',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case '|':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '='){ PutLexToken(OR_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else if (ch == '|'){ PutLexToken(OR_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else {PutLexToken('|',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case '=':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '='){ PutLexToken(EQ_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else {PutLexToken('=',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd); goto nextLexem;}

            case '!':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '='){ PutLexToken(NE_OP,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else {PutLexToken('!',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);goto nextLexem;}

            case ':':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                PutLexToken(':',dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                goto nextLexem;

            case '\'':
                chval = 0;
                lexStartFilePos = ABS_FILE_POS(cb,cfin,ccc);
                do {
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    while (ch=='\\') {
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        /* TODO escape sequences */
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    }
                    if (ch != '\'') chval = chval * 256 + ch;
                } while (ch != '\'' && ch != '\n');
                if (ch=='\'') {
                    PutLexToken(CHAR_LITERAL,dd);
                    PutLexInt(chval,dd);
                    PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                    PutLexInt(ABS_FILE_POS(cb,cfin,ccc)-lexStartFilePos, dd);
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                }
                goto nextLexem;

            case '\"':
                line = currentLineNumber; size = 0;
                PutLexToken(STRING_LITERAL,dd);
                do {
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);                size ++;
                    if (ch!='\"' && size<MAX_LEXEM_SIZE-10) PutLexChar(ch,dd);
                    if (ch=='\\') {
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);            size ++;
                        if (size < MAX_LEXEM_SIZE-10) PutLexChar(ch,dd);
                        /* TODO escape sequences */
                        if (ch == '\n') {currentLineNumber ++; currentLineBeginning = ccc; currentLineOffset = 0;}
                        ch = 0;
                    }
                    if (ch == '\n') {
                        currentLineNumber ++; currentLineBeginning = ccc; currentLineOffset = 0;
                        if (s_opt.strictAnsi && (s_opt.debug || s_opt.err)) {
                            warningMessage(ERR_ST,"string constant through end of line");
                        }
                    }
                    // in Java CR LF can't be a part of string, even there
                    // are benchmarks making Xrefactory coredump if CR or LF
                    // is a part of strings
                } while (ch != '\"' && (ch != '\n' || !s_opt.strictAnsi) && ch != -1);
                if (ch == -1 && s_opt.taskRegime!=RegimeEditServer) {
                    warningMessage(ERR_ST,"string constant through EOF");
                }
                PutLexChar(0,dd);
                PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                PutLexLine(currentLineNumber-line,dd);
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                goto nextLexem;

            case '/':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '='){ PutLexToken(DIV_ASSIGN,dd); PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset); goto nextLexem; }
                else if (ch=='*') {
                    int javadoc=0;
                    CommentaryBegRef(ccc,cfile,currentLineNumber,currentLineBeginning,currentLineOffset);
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    if (ch == '&') {
                        /* ****** a program comment, ignore */
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        goto nextLexem;
                    } else {
                        if (ch=='*' && LANGUAGE(LANG_JAVA)) javadoc = 1;
                        UngetChar(ch,ccc); ch = '*';
                    }   /* !!! COPY BLOCK TO '/n' */
                    PassComment(ch,oldCh,ccc,cfin,cb,dd,currentLineNumber,currentLineBeginning,currentLineOffset);
                    CommentaryEndRef(ccc,cfile,currentLineNumber,currentLineBeginning,currentLineOffset,javadoc);
                    goto nextLexem;
                } else if (ch=='/' && s_opt.cpp_comment) {
                    /*  ******* a // comment ******* */
                    CommentaryBegRef(ccc,cfile,currentLineNumber,currentLineBeginning,currentLineOffset);
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    if (ch == '&') {
                        /* ****** a program comment, ignore */
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        CommentaryEndRef(ccc,cfile,currentLineNumber,currentLineBeginning,currentLineOffset,0);
                        goto nextLexem;
                    }
                    line = currentLineNumber;
                    while (ch!='\n' && ch != -1) {
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        if (ch == '\\') {
                            GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                            if (ch=='\n') {currentLineNumber ++; currentLineBeginning = ccc; currentLineOffset = 0;}
                            GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        }
                    }
                    CommentaryEndRef(ccc,cfile,currentLineNumber,currentLineBeginning,currentLineOffset,0);
                    PutLexLine(currentLineNumber-line,dd);
                    goto nextLexem;
                } else {
                    PutLexToken('/',dd);
                    PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '\\':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '\n') {
                    currentLineNumber ++; currentLineBeginning = ccc;  currentLineOffset = 0;
                    PutLexLine(1, dd);
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                } else {
                    PutLexToken('\\',dd);
                    PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case '\n':
                column = COLUMN_POS(ccc,currentLineBeginning,currentLineOffset);
                if (column >= MAX_REFERENCABLE_COLUMN) {
                    fatalError(ERR_ST, "position over MAX_REFERENCABLE_COLUMN, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                if (currentLineNumber >= MAX_REFERENCABLE_LINE) {
                    fatalError(ERR_ST, "position over MAX_REFERENCABLE_LINE, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                currentLineNumber ++; currentLineBeginning = ccc; currentLineOffset = 0;
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                DeleteBlank(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '/') {
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    if (ch == '*') {
                        CommentaryBegRef(ccc,cfile,currentLineNumber,currentLineBeginning,currentLineOffset);
                        GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        if (ch == '&') {
                            /* ****** a program comment, ignore */
                            GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        } else {
                            int javadoc=0;
                            if (ch == '*' && LANGUAGE(LANG_JAVA)) javadoc = 1;
                            UngetChar(ch,ccc); ch = '*';
                            PassComment(ch,oldCh,ccc,cfin,cb,dd,currentLineNumber,currentLineBeginning,currentLineOffset);
                            CommentaryEndRef(ccc,cfile,currentLineNumber,currentLineBeginning,currentLineOffset,javadoc);
                            DeleteBlank(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        }
                    } else {
                        UngetChar(ch,ccc);
                        ch = '/';
                    }
                }
                PutLexToken('\n',dd);
                PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                if (ch == '#' && LANGUAGE(LANG_C|LANG_YACC)) {
                    NOTE_NEW_LEXEM_POSITION(ccc,cfin,cb,lb,cfile,currentLineNumber,currentLineBeginning,currentLineOffset);
                    HandleCppToken(ch,ccc,cfin,cb,dd,cfile,currentLineNumber,currentLineBeginning,currentLineOffset);
                }
                goto nextLexem;

            case '#':
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                if (ch == '#') {
                    GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    PutLexToken(CPP_COLLATION,dd);
                    PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                } else {
                    PutLexToken('#',dd);
                    PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case -1:
                /* ** probably end of file ** */
                goto nextLexem;
            default:
                if (ch >= 32) {         /* small chars ignored */
                    PutLexToken(ch,dd);
                    PutLexPosition(cfile, currentLineNumber, lexStartCol, dd);
                }
                GetChar(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                goto nextLexem;
            }
        assert(0);
    nextLexem:
        if (s_opt.taskRegime == RegimeEditServer) {
            int pi,len,lastlex,parChar,apos;
            Position *ps;
            int pos1,currentLexemPosition;
            pi = (lb->posi-1) % LEX_POSITIONS_RING_SIZE;
            ps = & lb->pRing[pi];
            currentLexemPosition = lb->fpRing[pi];
            if (    cfile == s_olOriginalFileNumber
                    && cfile != s_noneFileIndex
                    && cfile != -1
                    && s_jsl==NULL
                    ) {
                if (s_opt.server_operation == OLO_EXTRACT && lb->posi>=2) {
                    DeleteBlank(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    pos1 = ABS_FILE_POS(cb, cfin, ccc);
                    //&idcoll = COLUMN_POS(ccc,currentLineBeginning,currentLineOffset);
                    //&fprintf(dumpOut,":pos1==%d, olCursorPos==%d, olMarkPos==%d\n",pos1,s_opt.olCursorPos,s_opt.olMarkPos);
                    // all this is very, very HACK!!!
                    if (pos1 >= s_opt.olCursorPos && ! s_cps.marker1Flag) {
                        if (LANGUAGE(LANG_JAVA)) parChar = ';';
                        else {
                            if (s_cps.marker2Flag) parChar='}';
                            else parChar = '{';
                        }
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(';',dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(';',dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(OL_MARKER_TOKEN,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        s_cps.marker1Flag=1;
                    } else if (pos1 >= s_opt.olMarkPos && ! s_cps.marker2Flag){
                        if (LANGUAGE(LANG_JAVA)) parChar = ';';
                        else {
                            if (s_cps.marker1Flag) parChar='}';
                            else parChar = '{';
                        }
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(';',dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(';',dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(OL_MARKER_TOKEN,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        s_cps.marker2Flag=1;
                    }
                } else if (     s_opt.server_operation == OLO_COMPLETION
                                ||  s_opt.server_operation == OLO_SEARCH) {
                    DeleteBlank(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                    apos = ABS_FILE_POS(cb, cfin, ccc);
                    if (currentLexemPosition < s_opt.olCursorPos
                        && (apos >= s_opt.olCursorPos
                            || (ch == -1 && apos+1 == s_opt.olCursorPos))) {
                        //&sprintf(tmpBuff,"currentLexemPosition, s_opt.olCursorPos, ABS_FILE_POS, ch == %d, %d, %d, %d\n",currentLexemPosition, s_opt.olCursorPos, apos, ch);ppcGenTmpBuff();
                        //&fprintf(dumpOut,":check\n");fflush(dumpOut);
                        lastlex = NextLexToken(lexStartDd);
                        if (lastlex == IDENTIFIER) {
                            len = s_opt.olCursorPos-currentLexemPosition;
                            //&fprintf(dumpOut,":check %s[%d] <-> %d\n", lexStartDd+TOKEN_SIZE, len,strlen(lexStartDd+TOKEN_SIZE));fflush(dumpOut);
                            if (len <= strlen(lexStartDd+TOKEN_SIZE)) {
                                if (s_opt.server_operation == OLO_SEARCH) {
                                    char *ddd;
                                    ddd = lexStartDd;
                                    PutLexToken(IDENT_TO_COMPLETE, ddd);
                                } else {
                                    dd = lexStartDd;
                                    PutLexToken(IDENT_TO_COMPLETE, dd);
                                    dd += len;
                                    PutLexChar(0,dd);
                                    PutLexPosition(ps->file,ps->line,ps->col,dd);
                                }
                                //&fprintf(dumpOut,":ress %s\n", lexStartDd+TOKEN_SIZE);fflush(dumpOut);
                            } else {
                                // completion after an identifier
                                PUT_EMPTY_COMPLETION_ID(ccc,dd,currentLineNumber,currentLineOffset,currentLineBeginning,
                                                        cfile,
                                                        apos-s_opt.olCursorPos);
                            }
                        } else if ((lastlex == LINE_TOK || lastlex == STRING_LITERAL)
                                   && (apos-s_opt.olCursorPos != 0)) {
                            // completion inside special lexems, do
                            // NO COMPLETION
                        } else {
                            // completion after another lexem
                            PUT_EMPTY_COMPLETION_ID(ccc,dd,currentLineNumber,currentLineOffset,currentLineBeginning,
                                                    cfile,
                                                    apos-s_opt.olCursorPos);
                        }
                    }
                    // TODO, make this in a more standard way, !!!
                } else {
                    //&fprintf(dumpOut,":testing %d <= %d <= %d\n", currentLexemPosition, s_opt.olCursorPos, ABS_FILE_POS(cb, cfin, ccc));
                    if (currentLexemPosition <= s_opt.olCursorPos
                        && ABS_FILE_POS(cb, cfin, ccc) >= s_opt.olCursorPos) {
                        gotOnLineCxRefs( ps);
                        lastlex = NextLexToken(lexStartDd);
                        if (lastlex == IDENTIFIER) {
                            strcpy(s_olstring, lexStartDd+TOKEN_SIZE);
                        }
                    }
                    if (LANGUAGE(LANG_JAVA)) {
                        // there is a problem with this, when browsing at CPP construction
                        // that is why I restrict it to Java language! It is usefull
                        // only for Java refactorings
                        DeleteBlank(ch,ccc,cfin,cb,currentLineBeginning,currentLineOffset);
                        apos = ABS_FILE_POS(cb, cfin, ccc);
                        if (apos >= s_opt.olCursorPos && ! s_cps.marker1Flag) {
                            PutLexToken(OL_MARKER_TOKEN,dd);
                            PutLexPosition(ps->file,ps->line,ps->col,dd);
                            s_cps.marker1Flag=1;
                        }
                    }
                }
            }
        }
    } while (ch != -1);

    cb->next = ccc; cb->end = cfin;
    cb->lineNum = currentLineNumber; cb->lineBegin = currentLineBeginning; cb->columnOffset = currentLineOffset;
    lb->end = dd;

    if (lb->end == lb->chars)
        return 0;
    else
        return 1;
}
