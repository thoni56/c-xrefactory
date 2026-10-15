#include "encoding.h"

static bool hasContinuationBytes(unsigned char *src, int count, unsigned char *max) {
    for (int i = 1; i <= count; i++)
        if (src+i >= max || (src[i] & 0xC0) != 0x80)
            return false;
    return true;
}

/* A multibyte character becomes one space, so that byte offsets equal the
 * character offsets the editor counts. A lead byte without its continuation
 * bytes is not UTF-8, e.g. Latin-1, which has one byte per character, so it
 * becomes one space and swallows nothing after it */
static bool convertUtf8(unsigned char **srcP, unsigned char **dstP, unsigned char *max) {
    if (**(srcP)&0x80) {
        unsigned z = **srcP;
        int continuationBytes = 0;
        if (z >= 0xC2 && z <= 0xDF) {
            continuationBytes = 1;  // 2-byte sequence (U+0080 - U+07FF)
        } else if (z >= 0xE0 && z <= 0xEF) {
            continuationBytes = 2;  // 3-byte sequence (U+0800 - U+FFFF)
        } else if (z >= 0xF0 && z <= 0xF4) {
            continuationBytes = 3;  // 4-byte sequence (U+10000 - U+10FFFF)
        }
        if (hasContinuationBytes(*srcP, continuationBytes, max))
            *srcP += continuationBytes;

        **dstP = ' '; // Replace the sequence with a single space
        return true;
    }
    return false;
}


/* CR LF and a lone CR become one newline, so a line break is one character
 * whatever the file had, as the editor counts it */
void performEncodingAdjustments(EditorBuffer *buffer) {
    unsigned char *text = (unsigned char *)getTextInEditorBuffer(buffer);
    unsigned char *max = text + getSizeOfEditorBuffer(buffer);
    unsigned char *s, *d;

    for (s=d=text; s<max; s++, d++) {
        if (convertUtf8(&s, &d, max))
            ;
        else if (*s == '\r') {
            if (s+1<max && *(s+1)=='\n')
                s++;
            *d = '\n';
        } else
            *d = *s;
    }
    setSizeOfEditorBuffer(buffer, d - text);
}
