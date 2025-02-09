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

#define EDITOR_ENCODING_EUC_CONVERSION(s,d)     \
    if (*(s) & 0x80) {                          \
        unsigned z;                             \
        z = *s;                                 \
        if (z == 0x8e) {s+=2; *d = ' ';}      \
        else if (z == 0x8f) {s+=3; *d = ' ';} \
        else {s+=1; *d = ' ';}                \
    }
#define EDITOR_ENCODING_SJIS_CONVERSION(s,d)        \
    if (*(s) & 0x80) {                              \
        unsigned z;                                 \
        z = *s;                                     \
        if (z >= 0xa1 && z <= 0xdf) {*d = ' ';}   \
        else {s+=1; *d = ' ';}                    \
    }

static void applyCrLfCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_CR_LF_OR_CR_TO_LF_CONVERSION(s,d)
            else
                *d = *s;
        });
}

static void applyCrLfConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_CR_LF_TO_LF_CONVERSION(s,d)
            else
                *d = *s;
        });
}

static void applyCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_CR_TO_LF_CONVERSION(s,d)
            else
                *d = *s;
        });
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

static void applyEucCrLfCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_OR_CR_TO_LF_CONVERSION(s,d)
                else
                    *d = *s;
        });
}

static void applyEucCrLfConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_TO_LF_CONVERSION(s,d)
                else
                    *d = *s;
        });
}

static void applyEucCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_TO_LF_CONVERSION(s,d)
                else
                    *d = *s;
        });
}

static void applyEucConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else
                *d = *s;
        });
}

static void applySjisCrLfCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_OR_CR_TO_LF_CONVERSION(s,d)
                else
                    *d = *s;
        });
}

static void applySjisCrLfConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_TO_LF_CONVERSION(s,d)
                else
                    *d = *s;
        });
}

static void applySjisCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_TO_LF_CONVERSION(s,d)
                else
                    *d = *s;
        });
}

static void applySjisConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else
                *d = *s;
        });
}

static void applyUtf16Conversion(EditorBuffer *buffer) {
    unsigned char  *s, *d, *maxs;
    unsigned int cb, cb2;
    int little_endian;
    unsigned char *space;

    space = (unsigned char *)getTextInEditorBuffer(buffer);
    maxs = space + getSizeOfEditorBuffer(buffer);
    s = space;
    // determine endian first
    cb = (*s << 8) + *(s+1);
    if (cb == 0xfeff) {
        little_endian = 0;
        s += 2;
    } else if (cb == 0xfffe) {
        little_endian = 1;
        s += 2;
    } else if (options.fileEncoding == MULE_UTF_16LE) {
        little_endian = 1;
    } else if (options.fileEncoding == MULE_UTF_16BE) {
        little_endian = 0;
    } else {
        little_endian = 1;
    }
    for(s++,d=space; s<maxs; s+=2) {
        if (little_endian) cb = (*(s) << 8) + *(s-1);
        else cb = (*(s-1) << 8) + *(s);
        if (cb != 0xfeff) {
            if (cb >= 0xd800 && cb <= 0xdfff) {
                // 32 bit character
                s += 2;
                if (little_endian) cb2 = (*(s) << 8) + *(s-1);
                else cb2 = (*(s-1) << 8) + *(s);
                cb = 0x10000 + ((cb & 0x3ff) << 10) + (cb2 & 0x3ff);
            }
            if (cb < 0x80) {
                *d = (char)(cb & 0xff);
            } else {
                *d = ' ';
            }
        }
    }
    setSizeOfEditorBuffer(buffer, d - space);
}

static bool bufferStartsWithUtf16Bom(EditorBuffer *buffer) {
    unsigned char *s;

    s = (unsigned char *)getTextInEditorBuffer(buffer);
    if (getSizeOfEditorBuffer(buffer) >= 2) {
        unsigned cb = (*s << 8) + *(s+1);
        if (cb == 0xfeff || cb == 0xfffe)
            return true;
    }
    return false;
}

static void performSimpleLineFeedConversion(EditorBuffer *buffer) {
    if ((options.eolConversion&CR_LF_EOL_CONVERSION)
        && (options.eolConversion & CR_EOL_CONVERSION)) {
        applyCrLfCrConversion(buffer);
    } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
        applyCrLfConversion(buffer);
    } else if (options.eolConversion & CR_EOL_CONVERSION) {
        applyCrConversion(buffer);
    }
}

void performEncodingAdjustments(EditorBuffer *buffer) {
    // do different loops for efficiency reasons
    if (options.fileEncoding == MULE_EUROPEAN) {
        performSimpleLineFeedConversion(buffer);
    } else if (options.fileEncoding == MULE_EUC) {
        if ((options.eolConversion&CR_LF_EOL_CONVERSION)
            && (options.eolConversion & CR_EOL_CONVERSION)) {
            applyEucCrLfCrConversion(buffer);
        } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
            applyEucCrLfConversion(buffer);
        } else if (options.eolConversion & CR_EOL_CONVERSION) {
            applyEucCrConversion(buffer);
        } else {
            applyEucConversion(buffer);
        }
    } else if (options.fileEncoding == MULE_SJIS) {
        if ((options.eolConversion&CR_LF_EOL_CONVERSION)
            && (options.eolConversion & CR_EOL_CONVERSION)) {
            applySjisCrLfCrConversion(buffer);
        } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
            applySjisCrLfConversion(buffer);
        } else if (options.eolConversion & CR_EOL_CONVERSION) {
            applySjisCrConversion(buffer);
        } else {
            applySjisConversion(buffer);
        }
    } else {
        // default == utf
        if ((options.fileEncoding != MULE_UTF_8 && bufferStartsWithUtf16Bom(buffer))
            || options.fileEncoding == MULE_UTF_16 || options.fileEncoding == MULE_UTF_16LE
            || options.fileEncoding == MULE_UTF_16BE) {
            applyUtf16Conversion(buffer);
            performSimpleLineFeedConversion(buffer);
        } else {
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
    }
}
