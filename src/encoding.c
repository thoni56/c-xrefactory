#include "encoding.h"

#include "options.h"


#define EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, command) {          \
        unsigned char *s, *d, *max;                                     \
        unsigned char *text = (unsigned char *)getTextInEditorBuffer(buffer); \
        max = text + getSizeOfEditorBuffer(buffer);                     \
        for (s=d=text; s<max; s++) {                                    \
            command;                                                    \
            d++;                                                        \
        }                                                               \
        setSizeOfEditorBuffer(buffer, d - text);                        \
    }

#define EDITOR_ENCODING_CR_LF_OR_CR_TO_LF_CONVERSION(s,d)   \
    if (*s == '\r') {                                       \
        if (s+1<max && *(s+1)=='\n') {                      \
            s++;                                            \
            *d = *s;                                        \
        } else {                                            \
            *d = '\n';                                      \
        }                                                   \
    }
#define EDITOR_ENCODING_CR_LF_TO_LF_CONVERSION(s,d) \
    if (*s == '\r' && s+1<max && *(s+1)=='\n') {    \
        s++;                                        \
        *d = *s;                                    \
    }
#define EDITOR_ENCODING_CR_TO_LF_CONVERSION(s,d)      \
    if (*s == '\r') {                                 \
        *d = '\n';                                    \
    }

static bool convertUtf8(unsigned char **srcP, unsigned char **dstP) {
    if (**(srcP)&0x80) {
        unsigned z = **srcP;
        if (z >= 0xC2 && z <= 0xDF) {
            *srcP += 1;  // 2-byte sequence (U+0080 - U+07FF)
        } else if (z >= 0xE0 && z <= 0xEF) {
            *srcP += 2;  // 3-byte sequence (U+0800 - U+FFFF)
        } else if (z >= 0xF0 && z <= 0xF4) {
            *srcP += 3;  // 4-byte sequence (U+10000 - U+10FFFF)
        } else {
            // Illegal UTF-8, just skip one byte for minimal damage
            *srcP += 1;
        }

        **dstP = ' '; // Replace the sequence with a single space
        return true;
    }
    return false;
}


static void applyUtf8CrLfCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            if (convertUtf8(&s, &d)) ;
            else EDITOR_ENCODING_CR_LF_OR_CR_TO_LF_CONVERSION(s,d)
                else
                    *d = *s;
        });
}

static void applyUtf8CrLfConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            if (convertUtf8(&s, &d)) ;
            else EDITOR_ENCODING_CR_LF_TO_LF_CONVERSION(s,d)
                else
                    *d = *s;
        });
}

static void applyUtf8CrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            if (convertUtf8(&s, &d)) ;
            else EDITOR_ENCODING_CR_TO_LF_CONVERSION(s,d)
                else
                    *d = *s;
        });
}

static void applyUtf8Conversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            if (convertUtf8(&s, &d)) ;
            else
                *d = *s;
                });
}


void performEncodingAdjustments(EditorBuffer *buffer) {
    // utf-8
    if ((options.eolConversion&CR_LF_EOL_CONVERSION)
        && (options.eolConversion & CR_EOL_CONVERSION)) {
        applyUtf8CrLfCrConversion(buffer);
    } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
        applyUtf8CrLfConversion(buffer);
    } else if (options.eolConversion & CR_EOL_CONVERSION) {
        applyUtf8CrConversion(buffer);
    } else {
        applyUtf8Conversion(buffer);
    }
}
