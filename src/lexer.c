#include "lexer.h"

#include "globals.h"
#include "lexem.h"
#include "lexembuffer.h"
#include "options.h"
#include "commons.h"
#include "yylex.h"
#include "filedescriptor.h"
#include "filetable.h"

#include "caching.h"            /* cache && cacheInput() */
#include "jslsemact.h"          /* s_jsl */

#include "log.h"
#include "misc.h"              /* requiresCreatingRefs() */


void gotOnLineCxRefs(Position *position) {
    if (requiresCreatingRefs(options.serverOperation)) {
        deactivateCaching();
        cxRefPosition = *position;
    }
}

/* ***************************************************************** */
/*                         Lexical Analysis                          */
/* ***************************************************************** */

static void passComment(CharacterBuffer *cb) {
    int ch;
    char oldCh;
    /*  ******* a block comment ******* */
    ch = getChar(cb);
    if (ch=='\n') {
        cb->lineNumber ++;
        cb->lineBegin = cb->nextUnread;
        cb->columnOffset = 0;
    }
    /* TODO test on cpp directive */
    do {
        oldCh = ch;
        ch = getChar(cb);
        if (ch=='\n') {
            cb->lineNumber ++;
            cb->lineBegin = cb->nextUnread;
            cb->columnOffset = 0;
        }
        /* TODO test on cpp directive */
    } while ((oldCh != '*' || ch != '/') && ch != -1);
    if (ch == -1)
        warningMessage(ERR_ST,"comment through eof");
}


static LexemCode scanConstantType(CharacterBuffer *cb, int *ch) {
    LexemCode rlex;

    rlex = CONSTANT;
    if (LANGUAGE(LANG_JAVA)) {
        if (*ch=='l' || *ch=='L') {
            rlex = LONG_CONSTANT;
            *ch = getChar(cb);
        }
    } else {
        for(; *ch=='l'||*ch=='L'||*ch=='u'||*ch=='U'; ){
            if (*ch=='l' || *ch=='L')
                rlex = LONG_CONSTANT;
            *ch = getChar(cb);
        }
    }

    return rlex;
}


static LexemCode scanFloatingPointConstant(CharacterBuffer *cb, int *chPointer) {
    int ch = *chPointer;
    LexemCode rlex;

    rlex = DOUBLE_CONSTANT;
    if (ch == '.') {
        do {
            ch = getChar(cb);
        } while (isdigit(ch));
    }
    if (ch == 'e' || ch == 'E') {
        ch = getChar(cb);
        if (ch == '+' || ch=='-')
            ch = getChar(cb);
        while (isdigit(ch))
            ch = getChar(cb);
    }
    if (LANGUAGE(LANG_JAVA)) {
        if (ch == 'f' || ch == 'F' || ch == 'd' || ch == 'D') {
            if (ch == 'f' || ch == 'F')
                rlex = FLOAT_CONSTANT;
            ch = getChar(cb);
        }
    } else {
        if (ch == 'f' || ch == 'F' || ch == 'l' || ch == 'L') {
            ch = getChar(cb);
        }
    }
    *chPointer = ch;

    return rlex;
}


static void noteNewLexemPosition(LexemBuffer *lb, CharacterBuffer *cb) {
    int index = lb->ringIndex % LEX_POSITIONS_RING_SIZE;
    lb->fileOffsetRing[index]    = absoluteFilePosition(cb);
    lb->positionRing[index].file = cb->fileNumber;
    lb->positionRing[index].line = cb->lineNumber;
    lb->positionRing[index].col  = columnPosition(cb);
    lb->ringIndex++;
}

static LexemCode preprocessorLexemFromString(const char *preprocessorWord) {
    if (strcmp(preprocessorWord, "ifdef") == 0) {
        return CPP_IFDEF;
    } else if (strcmp(preprocessorWord, "ifndef") == 0) {
        return CPP_IFNDEF;
    } else if (strcmp(preprocessorWord, "if") == 0) {
        return CPP_IF;
    } else if (strcmp(preprocessorWord, "elif") == 0) {
        return CPP_ELIF;
    } else if (strcmp(preprocessorWord, "undef") == 0) {
        return CPP_UNDEF;
    } else if (strcmp(preprocessorWord, "else") == 0) {
        return CPP_ELSE;
    } else if (strcmp(preprocessorWord, "endif") == 0) {
        return CPP_ENDIF;
    } else if (strcmp(preprocessorWord, "pragma") == 0) {
        return CPP_PRAGMA;
    } else if (strcmp(preprocessorWord, "include") == 0) {
        return CPP_INCLUDE;
    } else if (strcmp(preprocessorWord, "include_next") == 0) {
        return CPP_INCLUDE_NEXT;
    } else if (strcmp(preprocessorWord, "define") == 0) {
        return CPP_DEFINE0;
    } else {
        // Some other un-interesting preprocessor line, like #error or #message...
        return CPP_LINE;
    }
}

static int extractPreprocessorWord(CharacterBuffer *cb, char preprocessorWord[]) {
    int ch;
    ch = getChar(cb);
    ch = skipBlanks(cb, ch);
    for (int i = 0; isalpha(ch) || isdigit(ch) || ch == '_'; i++) {
        preprocessorWord[i] = ch;
        preprocessorWord[i+1] = 0;
        ch = getChar(cb);
    }
    return ch;
}

static int processCppToken(CharacterBuffer *cb, LexemBuffer *lb) {
    int   ch;
    char  preprocessorWord[30];
    int   column;

    noteNewLexemPosition(lb, cb);

    column = columnPosition(cb);
    ch     = extractPreprocessorWord(cb, preprocessorWord);

    LexemCode lexem = preprocessorLexemFromString(preprocessorWord);
    switch (lexem) {
    case CPP_IFDEF:
    case CPP_IFNDEF:
    case CPP_IF:
    case CPP_ELIF:
    case CPP_UNDEF:
    case CPP_ELSE:
    case CPP_ENDIF:
    case CPP_PRAGMA:
    case CPP_LINE:
        putLexemWithPosition(lb, lexem, cb, column);
        break;
    case CPP_INCLUDE:
    case CPP_INCLUDE_NEXT:
        putLexemWithPosition(lb, lexem, cb, column);
        ch = putIncludeString(lb, cb, ch);
        break;
    case CPP_DEFINE0: {
        void *backpatchLexemP = getLexemStreamWrite(lb);
        putLexemWithPosition(lb, lexem, cb, column);
        ch = skipBlanks(cb, ch);
        noteNewLexemPosition(lb, cb);
        ch = putIdentifierLexem(lb, cb, ch);
        if (ch == '(') {
            /* Discovered parameters so backpatch the previous lexem
             * code (CPP_DEFINE0) to indicate that this is not a text
             * replacement but a macro "function" */
            backpatchLexemCodeAt(CPP_DEFINE, backpatchLexemP);
        }
        break;
    }
    default:
        errorMessage(ERR_ST, "Unexpected preprocessor line");
        break;
    }

    return ch;
}

/* Turn an identifier into a COMPLETE-lexem, return next character to process */
static int processCompletionOrSearch(CharacterBuffer *characterBuffer, LexemBuffer *lb, char *startOfCurrentLexem, Position position,
                                     int fileOffsetForCurrentLexem, int startingCh) {
    int ch   = skipBlanks(characterBuffer, startingCh);
    int apos = absoluteFilePosition(characterBuffer);

    if (fileOffsetForCurrentLexem < options.olCursorPosition
        && (apos >= options.olCursorPosition || (ch == -1 && apos + 1 == options.olCursorPosition))) {
        log_trace("offset for current lexem, options.olCursorPos, ABS_FILE_POS, ch == %d, %d, %d, %d",
                  fileOffsetForCurrentLexem, options.olCursorPosition, apos, ch);
        LexemCode thisLexToken = peekLexTokenAt(startOfCurrentLexem);
        if (thisLexToken == IDENTIFIER) {
            int len = options.olCursorPosition - fileOffsetForCurrentLexem;
            log_trace(":check %s[%d] <-> %d", startOfCurrentLexem + TOKEN_SIZE, len,
                      strlen(startOfCurrentLexem + TOKEN_SIZE));
            if (len <= strlen(startOfCurrentLexem + TOKEN_SIZE)) {
                /* Need to backpatch the current lexem to a COMPLETE lexem */
                char *backpatchP = startOfCurrentLexem;
                /* We want to overwrite with an IDENT_TO_COMPLETE,
                 * which we don't want to know how many bytes it
                 * takes, so we use ...WithPointer() which advances... */
                putLexTokenAt(IDENT_TO_COMPLETE, &backpatchP);
                if (options.serverOperation == OLO_COMPLETION) {
                    /* And for completion we need to terminate the identifier where the cursor is */
                    /* Move to position cursor is on in already written identifier */
                    /* We can use the backpatchP since it has moved to begining of string */
                    setLexemStreamWrite(lb, backpatchP + len);
                    /* Terminate identifier here */
                    putLexemChar(lb, 0);
                    /* And write the position */
                    putLexemPosition(lb, position);
                }
                log_trace(":ress %s", startOfCurrentLexem + TOKEN_SIZE);
            } else {
                // completion after an identifier
                putCompletionLexem(lb, characterBuffer, apos - options.olCursorPosition);
            }
        } else if ((thisLexToken == LINE_TOKEN || thisLexToken == STRING_LITERAL)
                   && (apos != options.olCursorPosition)) {
            // completion inside special lexems, do
            // NO COMPLETION
        } else {
            // completion after another lexem
            putCompletionLexem(lb, characterBuffer, apos - options.olCursorPosition);
        }
    }

    return ch;
}

bool buildLexemFromCharacters(CharacterBuffer *cb, LexemBuffer *lb) {
    int ch;
    char *lexemLimit;
    char *startOfCurrentLexem;
    LexemCode lexem;
    int line, column, size;
    int lexemStartingColumn, lexStartFilePos;

    /* first test whether the input is cached */
    /* TODO: why do we need to know this? */
    if (cachingIsActive() && includeStack.pointer == 0 && macroStackIndex == 0) {
        cacheInput(&currentInput);
        cache.nextToCache = lb->lexemStream;
    }

    shiftAnyRemainingLexems(lb);

    lexemLimit = getLexemStreamWrite(lb) + LEXEM_BUFFER_SIZE - MAX_LEXEM_SIZE;

    ch = getChar(cb);
    do {
        ch = skipBlanks(cb, ch);
        /* Space for more lexems? */
        if ((char *)getLexemStreamWrite(lb) >= lexemLimit) {
            ungetChar(cb, ch);
            break;
        }
        noteNewLexemPosition(lb, cb);
        startOfCurrentLexem = getLexemStreamWrite(lb);
        lexemStartingColumn = columnPosition(cb);
        log_trace("lexStartCol = %d", lexemStartingColumn);
        if (ch == '_' || isalpha(ch) || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))) {
            ch = putIdentifierLexem(lb, cb, ch);
            goto nextLexem;
        } else if (isdigit(ch)) {
            /* ***************   number *******************************  */
            long unsigned val=0;
            lexStartFilePos = absoluteFilePosition(cb);
            if (ch=='0') {
                ch = getChar(cb);
                if (ch=='x' || ch=='X') {
                    /* hexadecimal */
                    ch = getChar(cb);
                    while (isdigit(ch)||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F')) {
                        if (ch>='a') val = val*16+ch-'a'+10;
                        else if (ch>='A') val = val*16+ch-'A'+10;
                        else val = val*16+ch-'0';
                        ch = getChar(cb);
                    }
                } else {
                    /* octal */
                    while (isdigit(ch) && ch<='8') {
                        val = val*8+ch-'0';
                        ch = getChar(cb);
                    }
                }
            } else {
                /* decimal */
                while (isdigit(ch)) {
                    val = val*10+ch-'0';
                    ch = getChar(cb);
                }
            }
            if (ch == '.' || ch=='e' || ch=='E'
                || ((ch=='d' || ch=='D'|| ch=='f' || ch=='F') && LANGUAGE(LANG_JAVA))) {
                /* floating point */
                lexem = scanFloatingPointConstant(cb, &ch);
                putFloatingPointLexem(lb, lexem, cb, lexemStartingColumn, lexStartFilePos);
                goto nextLexem;
            }
            /* integer */
            lexem = scanConstantType(cb, &ch);
            putLexemCode(lb, lexem);
            putLexemInt(lb, val);
            putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
            putLexemInt(lb, absoluteFilePosition(cb)-lexStartFilePos);
            goto nextLexem;
        } else switch (ch) {
                /* ************   special character *********************  */
            case '.':
                lexStartFilePos = absoluteFilePosition(cb);
                ch = getChar(cb);
                if (ch == '.' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    if (ch == '.') {
                        ch = getChar(cb);
                        putLexemCode(lb, ELLIPSIS);
                        putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '.';
                    }
                    putLexemCode(lb, '.');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                } else if (isdigit(ch)) {
                    /* floating point constant */
                    ungetChar(cb, ch);
                    ch = '.';
                    lexem = scanFloatingPointConstant(cb, &ch);
                    putFloatingPointLexem(lb, lexem, cb, lexemStartingColumn, lexStartFilePos);
                    goto nextLexem;
                } else {
                    putLexemCode(lb, '.');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                }

            case '-':
                ch = getChar(cb);
                if (ch=='=') {
                    putLexemCode(lb, SUB_ASSIGN);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='-') {
                    putLexemCode(lb, DEC_OP);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='>' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    putLexemCode(lb, PTR_OP);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                } else {
                    putLexemCode(lb, '-');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                }

            case '+':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemCode(lb, ADD_ASSIGN);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '+') {
                    putLexemCode(lb, INC_OP);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemCode(lb, '+');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                }

            case '>':
                ch = getChar(cb);
                if (ch == '>') {
                    ch = getChar(cb);
                    if(ch=='>' && LANGUAGE(LANG_JAVA)) {
                        ch = getChar(cb);
                        if(ch=='=') {
                            putLexemCode(lb, URIGHT_ASSIGN);
                            putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                            ch = getChar(cb);
                            goto nextLexem;
                        } else {
                            putLexemCode(lb, URIGHT_OP);
                            putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                            goto nextLexem;
                        }
                    } else if(ch=='=') {
                        putLexemCode(lb, RIGHT_ASSIGN);
                        putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexemCode(lb, RIGHT_OP);
                        putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexemCode(lb, GE_OP);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemCode(lb, '>');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                }

            case '<':
                ch = getChar(cb);
                if (ch == '<') {
                    ch = getChar(cb);
                    if(ch=='=') {
                        putLexemCode(lb, LEFT_ASSIGN);
                        putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexemCode(lb, LEFT_OP);
                        putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexemCode(lb, LE_OP);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemCode(lb, '<');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                }

            case '*':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemCode(lb, MUL_ASSIGN);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemCode(lb, '*');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                }

            case '%':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemCode(lb, MOD_ASSIGN);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                /*&
                  else if (LANGUAGE(LANG_YACC) && ch == '{') {
                      putLexemCode(lb, YACC_PERC_LPAR);
                      putLexemPositionFields(lb, fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  else if (LANGUAGE(LANG_YACC) && ch == '}') {
                      putLexemCode(lb, YACC_PERC_RPAR);
                      putLexemPositionFields(lb, fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  &*/
                else {
                    putLexemCode(lb, '%');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                }

            case '&':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemCode(lb, AND_ASSIGN);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '&') {
                    putLexemCode(lb, AND_OP);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                else if (ch == '*') {
                    ch = getChar(cb);
                    if (ch == '/') {
                        /* a code comment, ignore */
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '*';
                        putLexemCode(lb, '&');
                        putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                        goto nextLexem;
                    }
                } else {
                    putLexemCode(lb, '&');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                }

            case '^':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemCode(lb, XOR_ASSIGN);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemCode(lb, '^');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    goto nextLexem;
                }

            case '|':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemCode(lb, OR_ASSIGN);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                } else if (ch == '|') {
                    putLexemCode(lb, OR_OP);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                } else {
                    putLexemCode(lb, '|');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                }
                goto nextLexem;

            case '=':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemCode(lb, EQ_OP);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                } else {
                    putLexemCode(lb, '=');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                }
                goto nextLexem;

            case '!':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemCode(lb, NE_OP);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                } else {putLexemCode(lb, '!');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                }
                goto nextLexem;

            case ':':
                ch = getChar(cb);
                putLexemCode(lb, ':');
                putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                goto nextLexem;

            case '\'': {
                unsigned chval = 0;

                lexStartFilePos = absoluteFilePosition(cb);
                do {
                    ch = getChar(cb);
                    while (ch == '\\') {
                        ch = getChar(cb);
                        /* TODO escape sequences */
                        ch = getChar(cb);
                    }
                    if (ch != '\'')
                        chval = chval * 256 + ch;
                } while (ch != '\'' && ch != '\n');
                if (ch == '\'') {
                    putLexemCode(lb, CHAR_LITERAL);
                    putLexemInt(lb, chval);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    putLexemInt(lb, absoluteFilePosition(cb) - lexStartFilePos);
                    ch = getChar(cb);
                }
                goto nextLexem;
            }

            case '\"':
                line = lineNumberFrom(cb);
                size = 0;
                putLexemCode(lb, STRING_LITERAL);
                do {
                    ch = getChar(cb);
                    size ++;
                    if (ch!='\"' && size<MAX_LEXEM_SIZE-10)
                        putLexemChar(lb, ch);
                    if (ch=='\\') {
                        ch = getChar(cb);
                        size ++;
                        if (size < MAX_LEXEM_SIZE-10)
                            putLexemChar(lb, ch);
                        /* TODO escape sequences */
                        if (ch == '\n') {
                            cb->lineNumber++;
                            cb->lineBegin = cb->nextUnread;
                            cb->columnOffset = 0;
                        }
                        ch = 0;
                    }
                    if (ch == '\n') {
                        cb->lineNumber++;
                        cb->lineBegin = cb->nextUnread;
                        cb->columnOffset = 0;
                        if (options.strictAnsi && (options.debug || options.errors)) {
                            warningMessage(ERR_ST,"string constant through end of line");
                        }
                    }
                    // in Java CR LF can't be a part of string, even there
                    // are benchmarks making Xrefactory coredump if CR or LF
                    // is a part of strings
                } while (ch != '\"' && (ch != '\n' || !options.strictAnsi) && ch != -1);
                if (ch == -1 && options.mode!=ServerMode) {
                    warningMessage(ERR_ST,"string constant through EOF");
                }
                putLexemChar(lb, 0);
                putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                putLexemLines(lb, lineNumberFrom(cb)-line);
                ch = getChar(cb);
                goto nextLexem;

            case '/':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemCode(lb, DIV_ASSIGN);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='*') {
                    ch = getChar(cb);
                    if (ch == '&') {
                        /* a program comment, ignore and continue with next lexem */
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '*';
                    }   /* !!! COPY BLOCK TO '/n' */

                    int line = lineNumberFrom(cb);
                    passComment(cb);
                    putLexemLines(lb, lineNumberFrom(cb)-line);
                    ch = getChar(cb);
                    goto nextLexem;

                } else if (ch=='/') {
                    /*  ******* a // comment ******* */
                    ch = getChar(cb);
                    if (ch == '&') {
                        /* ****** a program comment, ignore */
                        ch = getChar(cb);
                        goto nextLexem;
                    }
                    line = lineNumberFrom(cb);
                    while (ch!='\n' && ch != -1) {
                        ch = getChar(cb);
                        if (ch == '\\') {
                            ch = getChar(cb);
                            if (ch=='\n') {
                                cb->lineNumber++;
                                cb->lineBegin = cb->nextUnread;
                                cb->columnOffset = 0;
                            }
                            ch = getChar(cb);
                        }
                    }
                    putLexemLines(lb, lineNumberFrom(cb)-line);
                } else {
                    putLexemCode(lb, '/');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                }
                goto nextLexem;

            case '\\':
                ch = getChar(cb);
                if (ch == '\n') {
                    cb->lineNumber++;
                    cb->lineBegin = cb->nextUnread;
                    cb->columnOffset = 0;
                    putLexemLines(lb, 1);
                    ch = getChar(cb);
                } else {
                    putLexemCode(lb, '\\');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                }
                goto nextLexem;

            case '\n':
                column = columnPosition(cb);
                if (column >= MAX_REFERENCABLE_COLUMN) {
                    FATAL_ERROR(ERR_ST, "position over MAX_REFERENCABLE_COLUMN, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                if (lineNumberFrom(cb) >= MAX_REFERENCABLE_LINE) {
                    FATAL_ERROR(ERR_ST, "position over MAX_REFERENCABLE_LINE, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                cb->lineNumber++;
                cb->lineBegin = cb->nextUnread;
                cb->columnOffset = 0;
                ch = getChar(cb);
                ch = skipBlanks(cb, ch);
                if (ch == '/') {
                    ch = getChar(cb);
                    if (ch == '*') {
                        ch = getChar(cb);
                        if (ch == '&') {
                            /* ****** a code comment, ignore */
                            ch = getChar(cb);
                        } else {
                            ungetChar(cb, ch);
                            ch = '*';
                            int line = lineNumberFrom(cb);
                            passComment(cb);
                            putLexemLines(lb, lineNumberFrom(cb)-line);
                            ch = getChar(cb);
                            ch = skipBlanks(cb, ch);
                        }
                    } else {
                        ungetChar(cb, ch);
                        ch = '/';
                    }
                }
                putLexemCode(lb, '\n');
                putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                if (ch == '#' && LANGUAGE(LANG_C | LANG_YACC)) {
                    ch = processCppToken(cb, lb);
                }
                goto nextLexem;

            case '#':
                ch = getChar(cb);
                if (ch == '#') {
                    ch = getChar(cb);
                    putLexemCode(lb, CPP_COLLATION);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                } else {
                    putLexemCode(lb, '#');
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                }
                goto nextLexem;

            case -1:
                /* ** probably end of file ** */
                goto nextLexem;

            default:
                if (ch >= 32) {         /* small chars ignored */
                    putLexemCode(lb, ch);
                    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
                }
                ch = getChar(cb);
                goto nextLexem;
            }
        assert(0);
    nextLexem:
        if (options.mode == ServerMode) {
            int pi, parChar;
            Position position;
            int currentLexemOffset;

            /* Since lb->ringIndex is incremented *after* adding, we need to subtract 1 to get current */
            pi = (lb->ringIndex-1) % LEX_POSITIONS_RING_SIZE;
            currentLexemOffset = lb->fileOffsetRing[pi];
            position = lb->positionRing[pi];

            if (fileNumberFrom(cb) == olOriginalFileNumber && fileNumberFrom(cb) != NO_FILE_NUMBER
                && fileNumberFrom(cb) != -1 && s_jsl == NULL) {
                if (options.serverOperation == OLO_EXTRACT && lb->ringIndex>=2) { /* TODO: WTF does "lb->index >= 2" mean? */
                    ch = skipBlanks(cb, ch);
                    int apos = absoluteFilePosition(cb);
                    log_trace(":pos1==%d, olCursorPos==%d, olMarkPos==%d",apos,options.olCursorPosition,options.olMarkPos);
                    // all this is very, very HACK!!!
                    if (apos >= options.olCursorPosition && !parsedInfo.marker1Flag) {
                        if (LANGUAGE(LANG_JAVA))
                            parChar = ';';
                        else {
                            if (parsedInfo.marker2Flag)
                                parChar='}';
                            else
                                parChar = '{';
                        }
                        putLexemCode(lb, parChar);
                        putLexemPosition(lb, position);
                        putLexemCode(lb, parChar);
                        putLexemPosition(lb, position);
                        putLexemCode(lb, ';');
                        putLexemPosition(lb, position);
                        putLexemCode(lb, ';');
                        putLexemPosition(lb, position);
                        putLexemCode(lb, OL_MARKER_TOKEN);
                        putLexemPosition(lb, position);
                        putLexemCode(lb, parChar);
                        putLexemPosition(lb, position);
                        putLexemCode(lb, parChar);
                        putLexemPosition(lb, position);
                        parsedInfo.marker1Flag = true;
                    } else if (apos >= options.olMarkPos && !parsedInfo.marker2Flag){
                        if (LANGUAGE(LANG_JAVA))
                            parChar = ';';
                        else {
                            if (parsedInfo.marker1Flag)
                                parChar='}';
                            else
                                parChar = '{';
                        }
                        putLexemCode(lb, parChar);
                        putLexemPosition(lb, position);
                        putLexemCode(lb, parChar);
                        putLexemPosition(lb, position);
                        putLexemCode(lb, ';');
                        putLexemPosition(lb, position);
                        putLexemCode(lb, ';');
                        putLexemPosition(lb, position);
                        putLexemCode(lb, OL_MARKER_TOKEN);
                        putLexemPosition(lb, position);
                        putLexemCode(lb, parChar);
                        putLexemPosition(lb, position);
                        putLexemCode(lb, parChar);
                        putLexemPosition(lb, position);
                        parsedInfo.marker2Flag = true;
                    }
                } else if (options.serverOperation == OLO_COMPLETION
                           ||  options.serverOperation == OLO_SEARCH) {
                    ch = processCompletionOrSearch(cb, lb, startOfCurrentLexem, position, currentLexemOffset, ch);
                } else {
                    if (currentLexemOffset <= options.olCursorPosition
                        && absoluteFilePosition(cb) >= options.olCursorPosition) {
                        gotOnLineCxRefs(&position);
                        LexemCode lastlex = peekLexTokenAt(startOfCurrentLexem);
                        if (lastlex == IDENTIFIER) {
                            strcpy(s_olstring, startOfCurrentLexem+TOKEN_SIZE);
                        }
                    }
                    // TODO: Figure out what the problem was with this for C
                    if (LANGUAGE(LANG_JAVA)) {
                        // there is a problem with this, when browsing at CPP construction
                        // that is why I restrict it to Java language! It is usefull
                        // only for Java refactorings
                        ch = skipBlanks(cb, ch);
                        int apos = absoluteFilePosition(cb);
                        if (apos >= options.olCursorPosition && !parsedInfo.marker1Flag) {
                            putLexemCode(lb, OL_MARKER_TOKEN);
                            putLexemPosition(lb, position);
                            parsedInfo.marker1Flag = true;
                        }
                    }
                }
            }
        }
    } while (ch != -1);

    if ((char *)getLexemStreamWrite(lb) == lb->lexemStream)
        return false;
    else
        return true;
}
