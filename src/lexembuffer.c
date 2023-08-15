#include "lexembuffer.h"

#include "characterreader.h"
#include "head.h"
#include "commons.h"
#include "globals.h"
#include "lexem.h"

//
// Basic manipulation, these should preferably be private in favour
// for the functions to put complete lexems below
//

void initLexemBuffer(LexemBuffer *buffer) {
    buffer->read = buffer->lexemStream;
    buffer->begin = buffer->lexemStream;
    buffer->write = buffer->lexemStream;
    buffer->ringIndex = 0;
}

void setLexemStreamWrite(LexemBuffer *lb, void *write) {
    lb->write = write;
}

void *getLexemStreamWrite(LexemBuffer *lb) {
    return (void *)(lb->write);
}

/* Char */
void putLexemChar(LexemBuffer *lb, char ch) {
    *(lb->write++) = ch;
}

/* Short */
protected void putLexShortAt(int shortValue, char **writePointerP) {
    assert(shortValue <= 65535);
    **writePointerP = ((unsigned)shortValue)%256;
    (*writePointerP)++;
    **writePointerP = ((unsigned)shortValue)/256;
    (*writePointerP)++;
}

int getLexShortAt(char **readPointerP) {
    int value = *(unsigned char*)(*readPointerP);
    (*readPointerP)++;
    value += 256 * *(unsigned char*)(*readPointerP);
    (*readPointerP)++;
    return value;
}

static int peekLexShort(char **readPointerP) {
    int first = *(unsigned char*)(*readPointerP);
    int second = *((unsigned char*)(*readPointerP)+1);

    return first + 256*second;
}

void putLexemCode(LexemBuffer *lb, LexemCode lexem) {
    putLexShortAt(lexem, &(lb->write));
}

/* For backpatching */
void backpatchLexemCodeAt(LexemCode lexem, void *writePointer) {
    char *pointer = (char *)writePointer;
    putLexShortAt(lexem, &pointer);
}

void saveBackpatchPosition(LexemBuffer *lb) {
    lb->backpatchPointer = lb->write;
}

void backpatchLexemCode(LexemBuffer *lb, LexemCode lexem) {
    /* Write, and advance, using backpatchPointer */
    putLexemCodeAt(lexem, &lb->backpatchPointer);
}

void moveLexemStreamWriteToBackpatchPositonWithOffset(LexemBuffer *lb, int offset) {
    lb->write = lb->backpatchPointer + offset;
}

int strlenOfBackpatchedIdentifier(LexemBuffer *lb) {
    assert(peekLexemCodeAt(lb->backpatchPointer) == IDENTIFIER);
    return strlen(lb->backpatchPointer + LEXEMCODE_SIZE);
}


LexemCode getLexemCodeAt(char **readPointerP) {
    return (LexemCode)getLexShortAt(readPointerP);
}

LexemCode getLexToken(LexemBuffer *lb) {
    return (LexemCode)getLexShortAt(&lb->read);
}

LexemCode peekLexemCodeAt(char *readPointer) {
    char *pointer = readPointer;
    return (LexemCode)peekLexShort(&pointer);
}

/* Int */
void putLexemInt(LexemBuffer *lb, int value) {
    unsigned tmp;
    tmp = value;
    *lb->write++ = tmp%256; tmp /= 256;
    *lb->write++ = tmp%256; tmp /= 256;
    *lb->write++ = tmp%256; tmp /= 256;
    *lb->write++ = tmp%256; tmp /= 256;
}

int getLexemIntAt(char **readPointerP) {
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
protected void putLexCompacted(int value, char **writePointerP) {
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

protected int getLexCompacted(char **readPointerP) {
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


//
// Put complete lexems (including position, string, ...)
//

/* Lines */
void putLexemLines(LexemBuffer *lb, int lines) {
    putLexemCode(lb, LINE_TOKEN);
    putLexemInt(lb, lines);
}

/* Position */
void putLexemPositionFields(LexemBuffer *lb, int file, int line, int column) {
    assert(file>=0 && file<MAX_FILES);
    putLexCompacted(file, &(lb->write));
    putLexCompacted(line, &(lb->write));
    putLexCompacted(column, &(lb->write));
}

void putLexemPosition(LexemBuffer *lb, Position position) {
    assert(position.file>=0 && position.file<MAX_FILES);
    putLexCompacted(position.file, &(lb->write));
    putLexCompacted(position.line, &(lb->write));
    putLexCompacted(position.col, &(lb->write));
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
             || (ch == '$' && (LANGUAGE(LANG_YACC) || LANGUAGE(LANG_JAVA))));
    putLexemChar(lexemBuffer, 0);
    putLexemPositionFields(lexemBuffer, characterBuffer->fileNumber, characterBuffer->lineNumber, column);

    return ch;
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
    putLexemInt(lb, absoluteFilePosition(cb) - lexStartFilePos);
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
    putLexemInt(lb, absoluteFilePosition(cb) - lexStartFilePos);
}



//
// Get functions
// TODO: Most of these are still low-level and don't even use the lexemBuffer...
//




Position getLexemPositionAt(char **readPointerP) {
    Position pos;
    pos.file = getLexCompacted(readPointerP);
    pos.line = getLexCompacted(readPointerP);
    pos.col = getLexCompacted(readPointerP);
    return pos;
}

Position getLexemPosition(LexemBuffer *lb) {
    Position pos;
    pos.file = getLexCompacted(&lb->read);
    pos.line = getLexCompacted(&lb->read);
    pos.col = getLexCompacted(&lb->read);
    return pos;
}

Position peekLexemPositionAt(char *readPointer) {
    char *pointer = readPointer;
    return getLexemPositionAt(&pointer);
}

/* DEPRECATED - writing with pointer to pointer that it advances, a
 * lot of code in yylex.c still uses this bad interface */
void putLexemCodeAt(LexemCode lexem, char **writePointerP) {
    putLexShortAt(lexem, writePointerP);
}

void putLexemPositionAt(Position position, char **writePointerP) {
    putLexCompacted(position.file, writePointerP);
    putLexCompacted(position.line, writePointerP);
    putLexCompacted(position.col, writePointerP);
}

void putLexemIntAt(int integer, char **writePointerP) {
    unsigned tmp;
    tmp = integer;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
    *(*writePointerP)++ = tmp%256; tmp /= 256;
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
