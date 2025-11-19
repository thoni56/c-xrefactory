#include "lexembuffer.h"

#include <string.h>
#include <ctype.h>

#include "characterreader.h"
#include "head.h"
#include "commons.h"
#include "globals.h"
#include "lexem.h"
#include "options.h"

//
// Basic manipulation, these should preferably be private in favour
// for the functions to put complete lexems below
//

/* Put elementary type values */

/* Char */
void putLexemChar(LexemBuffer *lb, char ch) {
    *(lb->write++) = ch;
}

/* Short */
protected void putLexShortAndAdvance(int shortValue, char **writePointerP) {
    assert(shortValue <= 65535);
    **writePointerP = ((unsigned)shortValue)%256;
    (*writePointerP)++;
    **writePointerP = ((unsigned)shortValue)/256;
    (*writePointerP)++;
}

protected int getLexShortAndAdvance(char **readPointerP) {
    int value = *(unsigned char*)(*readPointerP);
    (*readPointerP)++;
    value += 256 * *(unsigned char*)(*readPointerP);
    (*readPointerP)++;
    return value;
}

protected void putLexemInt(LexemBuffer *lb, int value) {
    unsigned tmp;
    tmp = value;
    *lb->write++ = tmp%256; tmp /= 256;
    *lb->write++ = tmp%256; tmp /= 256;
    *lb->write++ = tmp%256; tmp /= 256;
    *lb->write++ = tmp%256; tmp /= 256;
}

static int peekLexShort(char **readPointerP) {
    int first = *(unsigned char*)(*readPointerP);
    int second = *((unsigned char*)(*readPointerP)+1);

    return first + 256*second;
}

LexemCode getLexemCodeAndAdvance(char **readPointerP) {
    return (LexemCode)getLexShortAndAdvance(readPointerP);
}

LexemCode getLexemCode(LexemBuffer *lb) {
    return (LexemCode)getLexShortAndAdvance(&lb->read);
}

LexemCode peekLexemCodeAt(char *readPointer) {
    char *pointer = readPointer;
    return (LexemCode)peekLexShort(&pointer);
}

int getLexemIntAndAdvance(char **readPointerP) {
    unsigned int value;
    value = **(unsigned char**)readPointerP;
    (*readPointerP)++;
    value += 256 * **(unsigned char**)readPointerP;
    (*readPointerP)++;
    value += 256 * 256 * **(unsigned char**)readPointerP;
    (*readPointerP)++;
    value += 256 * 256 * 256 * **(unsigned char**)readPointerP;
    (*readPointerP)++;
    return value;
}

/* Compacted */
protected void putLexCompactedAndAdvance(int value, char **writePointerP) {
    assert(((unsigned) value)<4194304);
    if (((unsigned)value) < 128) {
        **writePointerP = ((unsigned char)value);
        (*writePointerP)++;
    } else if (((unsigned)value) < 16384) {
        **writePointerP = ((unsigned)value)%128+128;
        (*writePointerP)++;
        **writePointerP = ((unsigned)value)/128;
        (*writePointerP)++;
    } else {
        **writePointerP = ((unsigned)value)%128+128;
        (*writePointerP)++;
        **writePointerP = ((unsigned)value)/128%128+128;
        (*writePointerP)++;
        **writePointerP = ((unsigned)value)/16384;
        (*writePointerP)++;
    }
}

protected int getLexCompactedAndAdvance(char **readPointerP) {
    unsigned value;

    value = **(unsigned char**)readPointerP;
    (*readPointerP)++;
    if (value >= 128) {
        unsigned secondPart = **(unsigned char**)readPointerP;
        (*readPointerP)++;
        if (secondPart < 128) {
            value = ((unsigned)value)-128 + 128 * secondPart;
        } else {
            unsigned thirdPart = **(unsigned char**)readPointerP;
            (*readPointerP)++;
            value = ((unsigned)value)-128 + 128 * (secondPart-128) + 16384 * thirdPart;
        }
    }
    return value;
}


/* Buffer manipulation */
void initLexemBuffer(LexemBuffer *buffer) {
    buffer->read = buffer->lexemStream;
    buffer->begin = buffer->lexemStream;
    buffer->write = buffer->lexemStream;
}

void shiftAnyRemainingLexems(LexemBuffer *lb) {
    int remaining = lb->write - lb->read;
    char *src = lb->read;
    char *dest = lb->lexemStream;

    for (int i = 0; i < remaining; i++)
        *dest++ = *src++;

    lb->read = lb->lexemStream;
    lb->write = &lb->lexemStream[remaining];
}

void *getLexemStreamWrite(LexemBuffer *lb) {
    return (void *)(lb->write);
}

/* For backpatching */
void backpatchLexemCodeAndAdvance(LexemCode lexem, void *writePointer) {
    char *pointer = (char *)writePointer;
    putLexShortAndAdvance(lexem, &pointer);
}

void saveBackpatchPosition(LexemBuffer *lb) {
    lb->backpatchPointer = lb->write;
}

void backpatchLexemCode(LexemBuffer *lb, LexemCode lexem) {
    /* Write, and advance, using backpatchPointer */
    putLexemCodeAndAdvance(lexem, &lb->backpatchPointer);
}

void moveLexemStreamWriteToBackpatchPositonWithOffset(LexemBuffer *lb, int offset) {
    lb->write = lb->backpatchPointer + offset;
}

int strlenOfBackpatchedIdentifier(LexemBuffer *lb) {
    assert(peekLexemCodeAt(lb->backpatchPointer) == IDENTIFIER);
    return strlen(lb->backpatchPointer + LEXEMCODE_SIZE);
}


//
// Put complete lexems (including position, string, ...)
//

protected void putLexemCode(LexemBuffer *lb, LexemCode lexem) {
    putLexShortAndAdvance(lexem, &(lb->write));
}

void putLexemLines(LexemBuffer *lb, int lines) {
    putLexemCode(lb, LINE_TOKEN);
    putLexemInt(lb, lines);
}

void putLexemPositionFields(LexemBuffer *lb, int file, int line, int column) {
    assert(file>=0 && file<MAX_FILES);
    putLexCompactedAndAdvance(file, &(lb->write));
    putLexCompactedAndAdvance(line, &(lb->write));
    putLexCompactedAndAdvance(column, &(lb->write));
}

void putLexemPosition(LexemBuffer *lb, Position position) {
    assert(position.file>=0 && position.file<MAX_FILES);
    putLexCompactedAndAdvance(position.file, &(lb->write));
    putLexCompactedAndAdvance(position.line, &(lb->write));
    putLexCompactedAndAdvance(position.col, &(lb->write));
}

void putLexemCodeWithPosition(LexemBuffer *lb, LexemCode lexem, Position position) {
    putLexemCode(lb, lexem);
    putLexemPosition(lb, position);
}

/* Scans an identifier from CharacterBuffer and stores it in
 * LexemBuffer. 'ch' is the first character in the identifier. Returns
 * first character not part of the identifier */
int putIdentifierLexem(LexemBuffer *lexemBuffer, CharacterBuffer *characterBuffer, int ch) {
    int column;

    column = columnPosition(characterBuffer);
    putLexemCode(lexemBuffer, IDENTIFIER);
    do {
        putLexemChar(lexemBuffer, ch);
        ch = getChar(characterBuffer);
    } while (isalpha(ch) || isdigit(ch) || ch == '_'
             || (ch == '$' && LANGUAGE(LANG_YACC)));
    putLexemChar(lexemBuffer, 0);
    putLexemPositionFields(lexemBuffer, characterBuffer->fileNumber, characterBuffer->lineNumber, column);

    return ch;
}

void putStringConstantLexem(LexemBuffer *lb, CharacterBuffer *cb, int lexemStartingColumn) {
    int ch;
    int line = lineNumberFrom(cb);
    int size = 0;

    putLexemCode(lb, STRING_LITERAL);
    do {
        ch = getChar(cb);
        size++;
        if (ch != '\"'
            && size < MAX_LEXEM_SIZE - 10) /* WTF is 10? Perhaps required space for position info? */
            putLexemChar(lb, ch);
        if (ch == '\\') {
            ch = getChar(cb);
            size++;
            if (size < MAX_LEXEM_SIZE - 10)
                putLexemChar(lb, ch);
            if (ch == '\n') {
                /* Escaped newline inside string */
                cb->lineNumber++;
                cb->lineBegin    = cb->nextUnread;
                cb->columnOffset = 0;
            }
            /* TODO other escape sequences */
            ch = 0;             /* TODO Not sure why 0, but 'continue' does not work... */
        }
        if (ch == '\n') {
            /* Unescaped newline inside string */
            cb->lineNumber++;
            cb->lineBegin    = cb->nextUnread;
            cb->columnOffset = 0;
            if (options.strictAnsi && (options.debug || options.errors)) {
                warningMessage(ERR_ST, "string constant through end of line");
            }
        }
        // in Java CR LF can't be a part of string, even there
        // are benchmarks making Xrefactory coredump if CR or LF
        // is a part of strings
    } while (ch != '\"' && (ch != '\n' || !options.strictAnsi) && ch != -1);
    if (ch == -1 && options.mode != ServerMode) {
        warningMessage(ERR_ST, "string constant through EOF");
    }
    terminateLexemString(lb);
    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
    putLexemLines(lb, lineNumberFrom(cb) - line);
}

void putCharLiteralLexem(LexemBuffer *lb, CharacterBuffer *cb, int lexemStartingColumn,
                         int length, unsigned chval) {
    putLexemCode(lb, CHAR_LITERAL);
    putLexemInt(lb, chval);
    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
    putLexemInt(lb, length);
}

void terminateLexemString(LexemBuffer *lb) {
    putLexemChar(lb, 0);
}


void putLexemWithColumn(LexemBuffer *lb, LexemCode lexem, CharacterBuffer *cb, int column) {
    putLexemCode(lb, lexem);
    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), column);
}

int putIncludeString(LexemBuffer *lb, CharacterBuffer *cb, int ch) {
    ch = skipBlanks(cb, ch);
    if (ch == '\"' || ch == '<') {
        char terminator = ch == '\"' ? '\"' : '>';
        int  col        = columnPosition(cb);
        putLexemCode(lb, STRING_LITERAL);
        do {
            putLexemChar(lb, ch);
            ch = getChar(cb);
        } while (ch != terminator && ch != '\n');
        putLexemChar(lb, 0);
        putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), col);
        if (ch == terminator)
            ch = getChar(cb);
    }

    return ch;
}

void putIntegerLexem(LexemBuffer *lb, LexemCode lexem, long unsigned value, CharacterBuffer *cb,
                     int lexemStartingColumn, int lexStartFilePos) {
    putLexemCode(lb, lexem);
    putLexemInt(lb, value);
    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
    putLexemInt(lb, fileOffsetFor(cb) - lexStartFilePos);
}

void putCompletionLexem(LexemBuffer *lb, CharacterBuffer *cb, int len) {
    putLexemCode(lb, IDENT_TO_COMPLETE);
    putLexemChar(lb, '\0');
    putLexemPositionFields(lb, cb->fileNumber, cb->lineNumber,
                           columnPosition(cb) - len);
}

void putFloatingPointLexem(LexemBuffer *lb, LexemCode lexem, CharacterBuffer *cb, int lexemStartingColumn,
                         int lexStartFilePos) {
    putLexemCode(lb, lexem);
    putLexemPositionFields(lb, fileNumberFrom(cb), lineNumberFrom(cb), lexemStartingColumn);
    putLexemInt(lb, fileOffsetFor(cb) - lexStartFilePos);
}



//
// Get functions
//

Position getLexemPosition(LexemBuffer *lb) {
    Position pos;
    pos.file = getLexCompactedAndAdvance(&lb->read);
    pos.line = getLexCompactedAndAdvance(&lb->read);
    pos.col = getLexCompactedAndAdvance(&lb->read);
    return pos;
}

// TODO: Most of these are still low-level and don't even use the lexemBuffer...

Position getLexemPositionAndAdvance(char **readPointerP) {
    Position pos;
    pos.file = getLexCompactedAndAdvance(readPointerP);
    pos.line = getLexCompactedAndAdvance(readPointerP);
    pos.col = getLexCompactedAndAdvance(readPointerP);
    return pos;
}

Position peekLexemPositionAt(char *readPointer) {
    char *pointer = readPointer;
    return getLexemPositionAndAdvance(&pointer);
}

/* DEPRECATED - writing with pointer to pointer that it advances, a
 * lot of code in yylex.c still uses this bad interface */
void putLexemCodeAndAdvance(LexemCode lexem, char **writePointerP) {
    putLexShortAndAdvance(lexem, writePointerP);
}

void putLexemPositionAndAdvance(Position position, char **writePointerP) {
    putLexCompactedAndAdvance(position.file, writePointerP);
    putLexCompactedAndAdvance(position.line, writePointerP);
    putLexCompactedAndAdvance(position.col, writePointerP);
}

void putLexemIntAndAdvance(int integer, char **writePointerP) {
    unsigned tmp;
    tmp = integer;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
}
