/* *********************** Read JAVA class file ************************* */

#include "classfilereader.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "lexer.h"
#include "yylex.h"
#include "classcaster.h"
#include "misc.h"
#include "cxref.h"
#include "cxfile.h"
#include "jsemact.h"
#include "characterreader.h"
#include "symbol.h"
#include "list.h"
#include "filedescriptor.h"
#include "fileio.h"
#include "filetable.h"
#include "stackmemory.h"

#include "log.h"


/* This reads class files according to the Java Machine Specification,
 * https://docs.oracle.com/javase/specs/jvms/se8/jvms8.pdf */

/* *********************************************************************** */
/*                    JAVA Constant Pool Item Tags                         */

#define CONSTANT_Utf8               1
#define CONSTANT_Integer            3
#define CONSTANT_Float              4
#define CONSTANT_Long               5
#define CONSTANT_Double             6
#define CONSTANT_Class              7
#define CONSTANT_String             8
#define CONSTANT_Fieldref           9
#define CONSTANT_Methodref          10
#define CONSTANT_InterfaceMethodref	11
#define CONSTANT_NameandType        12
#define CONSTANT_MethodHandle       15
#define CONSTANT_MethodType         16
#define CONSTANT_Dynamic            17
#define CONSTANT_InvokeDynamic      18
#define CONSTANT_Module             19
#define CONSTANT_Package            20


/* NOTE the tag is read and discarded, not included in these structures */
struct CONSTANT_Class_info {
    /* U1 tag; */
    short unsigned name_index;
};
struct CONSTANT_NameAndType_info {
    /* U1 tag; */
    short unsigned name_index;
    short unsigned descriptor_index;
};
struct CONSTANT_Ref_info {
    /* U1 tag; */
    short unsigned class_index;
    short unsigned name_and_type_index;
};
struct CONSTANT_MethodHandle_info {
    /* U1 tag; */
    short unsigned reference_kind;
    short unsigned reference_index;
};
struct CONSTANT_MethodType_info {
    /* U1 tag; */
    short unsigned descriptor_index;
};
struct CONSTANT_Dynamic_info {  /* Also InvokeDynamic */
    /* U1 tag; */
    short unsigned bootstrap_method_attr_index;
    short unsigned name_and_type_index;
};

typedef union constantPoolUnion {
    char *utf8;
    struct CONSTANT_Class_info class;
    struct CONSTANT_NameAndType_info nameAndType;
    struct CONSTANT_Ref_info ref;
    struct CONSTANT_MethodHandle_info methodHandle;
    struct CONSTANT_MethodType_info methodType;
    struct CONSTANT_Dynamic_info dynamic;
} ConstantPoolUnion;

ZipFileTableItem zipArchiveTable[MAX_JAVA_ZIP_ARCHIVES];

typedef enum exception {
    NO_EXCEPTION,
    END_OF_FILE_EXCEPTION
} Exception;

/* *********************************************************************** */

#define GetChar(ch, cb, exception) {                                    \
        if (cb->nextUnread >= cb->end) {                                      \
            (cb)->nextUnread = cb->nextUnread;                                      \
            if ((cb)->isAtEOF || refillBuffer(cb) == 0) {               \
                ch = -1;                                                \
                (cb)->isAtEOF = true;                                   \
                longjmp(exception, END_OF_FILE_EXCEPTION);              \
            } else {                                                    \
                cb->nextUnread = (cb)->nextUnread;                                  \
                cb->end = (cb)->end;                                    \
                ch = * ((unsigned char*)cb->nextUnread); cb->nextUnread ++;         \
            }                                                           \
        } else {                                                        \
            ch = * ((unsigned char*)cb->nextUnread); cb->nextUnread++;              \
        }                                                               \
    }


#define GetU1(value, cb, exception) {           \
        GetChar(value, cb, exception);          \
    }

#define GetU2(value, cb, exception) {           \
        int ch;                                 \
        GetChar(value, cb, exception);          \
        GetChar(ch, cb, exception);             \
        value = value*256+ch;                   \
    }

#define GetU4(value, cb, exception) {           \
        int ch;                                 \
        GetChar(value, cb, exception);          \
        GetChar(ch, cb, exception);             \
        value = value*256+ch;                   \
        GetChar(ch, cb, exception);             \
        value = value*256+ch;                   \
        GetChar(ch, cb, exception);             \
        value = value*256+ch;                   \
    }

#define GetZU2(value, cb, exception) {          \
        unsigned ch;                            \
        GetChar(value, cb, exception);          \
        GetChar(ch, cb, exception);             \
        value = value+(ch<<8);                  \
    }

#define GetZU4(value, cb, exception) {          \
        unsigned ch;                            \
        GetChar(value, cb, exception);          \
        GetChar(ch, cb, exception);             \
        value = value+(ch<<8);                  \
        GetChar(ch, cb, exception);             \
        value = value+(ch<<16);                 \
        GetChar(ch, cb, exception);             \
        value = value+(ch<<24);                 \
    }

#define SkipAttributes(cb) {                    \
        int index, count, aname, alen;          \
        GetU2(count, cb, exception);            \
        for(index=0; index<count; index++) {    \
            GetU2(aname, cb, exception);        \
            GetU4(alen, cb, exception);         \
            skipCharacters(cb, alen);           \
        }                                       \
    }

/* *************** first something to read zip-files ************** */

#define CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE 0x02014b50
#define CENTRAL_DIRECTORY_SIGNATURE 0x06054b50
#define LOCAL_FILE_HEADER_SIGNATURE 0x04034b50

typedef struct zipFileInfo {
    unsigned versionMadeBy,versionNeededToExtract,bitFlags,compressionMethod;
    unsigned lastModificationTime,lastModificationDate;
    unsigned crc32,compressedSize,unCompressedSize;
    unsigned fileNameLength,extraFieldLength,fileCommentLength,diskNumber;
    char filename[MAX_FILE_NAME_SIZE];
    unsigned internalFileAttributes, externalFileAttributes, localHeaderOffset;
} ZipFileInfo;


/* **************************************************************** */

Symbol *cfAddCastsToModule(Symbol *memb, Symbol *sup) {
    assert(memb->u.structSpec);
    cctAddSimpleValue(&memb->u.structSpec->casts, sup, 1);
    assert(sup->u.structSpec);
    cctAddCctTree(&memb->u.structSpec->casts, &sup->u.structSpec->casts, 1);
    return(sup);
}


void addSuperClassOrInterface(Symbol *member, Symbol *super, int originFileNumber) {
    SymbolList *symbolList, *s;
    char tmpBuff[TMP_BUFF_SIZE];

    super = javaFQTypeSymbolDefinition(super->name, super->linkName);
    for(s = member->u.structSpec->super; s != NULL && s->element != super; s = s->next)
        ;
    if (s != NULL && s->element == super)
        return; // avoid multiple occurrences
    log_debug("adding superclass %s to %s", super->linkName, member->linkName);
    if (cctIsMember(&super->u.structSpec->casts, member, 1) || member == super) {
        sprintf(tmpBuff, "detected cycle in super classes of %s",
                member->linkName);
        errorMessage(ERR_ST, tmpBuff);
        return;
    }
    cfAddCastsToModule(member, super);

    symbolList = cfAlloc(SymbolList);
    /* REPLACED: FILL_symbolList(ssl, supp, NULL); with compound literal */
    *symbolList = (SymbolList){.element = super, .next = NULL};
    LIST_APPEND(SymbolList, member->u.structSpec->super, symbolList);
    addSubClassItemToFileTab(super->u.structSpec->classFileNumber,
                             member->u.structSpec->classFileNumber,
                             originFileNumber);
}


void convertLinkNameToClassFileName(char classFileName[], char *linkName) {
    sprintf(classFileName, "%c%s.class", ZIP_SEPARATOR_CHAR, linkName);
    assert(strlen(classFileName)+1 < MAX_FILE_NAME_SIZE);
}

int javaCreateClassFileItem( Symbol *memb) {
    char ftname[MAX_FILE_NAME_SIZE];
    int fileNumber;

    convertLinkNameToClassFileName(ftname, memb->linkName);
    fileNumber = addFileNameToFileTable(ftname);
    memb->u.structSpec->classFileNumber = fileNumber;

    return fileNumber;
}
