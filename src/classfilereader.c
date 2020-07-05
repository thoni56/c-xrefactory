/* *********************** Read JAVA class file ************************* */

#include "classfilereader.h"

#include "commons.h"
#include "globals.h"
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

#include "log.h"
#include "utils.h"

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

ZipFileTableItem s_zipArchiveTable[MAX_JAVA_ZIP_ARCHIVES];

typedef enum exception {
    NO_EXCEPTION,
    END_OF_FILE_EXCEPTION
} Exception;

/* *********************************************************************** */

#define GetChar(ch, cb, exception) {                                    \
        if (cb->next >= cb->end) {                                      \
            (cb)->next = cb->next;                                      \
            if ((cb)->isAtEOF || refillBuffer(cb) == 0) {               \
                ch = -1;                                                \
                (cb)->isAtEOF = true;                                   \
                longjmp(exception, END_OF_FILE_EXCEPTION);              \
            } else {                                                    \
                cb->next = (cb)->next;                                  \
                cb->end = (cb)->end;                                    \
                ch = * ((unsigned char*)cb->next); cb->next ++;         \
            }                                                           \
        } else {                                                        \
            ch = * ((unsigned char*)cb->next); cb->next++;              \
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


static void compressionError(char *archivename, unsigned compressionMethod) {
    static bool compressionErrorWritten = false;
    char *separator_position, jar_filename[MAX_FILE_NAME_SIZE];

    if (!compressionErrorWritten) {
        char tmpBuff[TMP_BUFF_SIZE];
        assert(options.taskRegime);
        strcpy(jar_filename, archivename);
        separator_position = strchr(jar_filename, ZIP_SEPARATOR_CHAR);
        if (separator_position != NULL)
            *separator_position = 0;
        sprintf(tmpBuff,"\n\tfiles in %s are compressed by unknown method #%d\n", jar_filename, compressionMethod);
        sprintf(tmpBuff+strlen(tmpBuff),
                "\tYou can try running 'jar2jar0 %s' command to uncompress them.", jar_filename);
        assert(strlen(tmpBuff)+1 < TMP_BUFF_SIZE);
        errorMessage(ERR_ST,tmpBuff);
        compressionErrorWritten = true;
    }
}


static void corruptedError(char *archivename) {
    static bool messagePrinted = false;
    if (!messagePrinted) {
        char tmpBuff[TMP_BUFF_SIZE];
        messagePrinted = true;
        sprintf(tmpBuff,
                "archive %s is corrupted or modified while c-xref task running",
                archivename);
        errorMessage(ERR_ST, tmpBuff);
        if (options.taskRegime == RegimeEditServer) {
            fprintf(errOut,"\t\tplease, kill c-xref process and retry.\n");
        }
    }
}


static void readLocalFileHeader(CharacterBuffer *cb, ZipFileInfo *info, jmp_buf exception) {
    GetZU2(info->versionNeededToExtract, cb, exception);
    GetZU2(info->bitFlags, cb, exception);
    GetZU2(info->compressionMethod, cb, exception);
    GetZU2(info->lastModificationTime, cb, exception);
    GetZU2(info->lastModificationDate, cb, exception);
    GetZU4(info->crc32, cb, exception);
    GetZU4(info->compressedSize, cb, exception);
    GetZU4(info->unCompressedSize, cb, exception);
    GetZU2(info->fileNameLength, cb, exception);
    if (info->fileNameLength >= MAX_FILE_NAME_SIZE) {
        fatalError(ERR_INTERNAL, "file name too long", XREF_EXIT_ERR);
    }
    GetZU2(info->extraFieldLength, cb, exception);
    for(int i=0; i<info->fileNameLength; i++) {
        GetChar(info->filename[i], cb, exception);
    }
    info->filename[info->fileNameLength] = 0;
    skipCharacters(cb, info->extraFieldLength);
}


static bool zipReadLocalFileInfo(CharacterBuffer *cb,
                                 char *filename, unsigned *filesize,
                                 unsigned *lastSignature,
                                 char *archivename) {
    bool result = true;
    int signature;
    ZipFileInfo info;
    jmp_buf exception;

    switch (setjmp(exception)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    }

    GetZU4(signature, cb, exception);
    log_trace("zip file signature is %x", signature);
    *lastSignature = signature;
    if (signature != LOCAL_FILE_HEADER_SIGNATURE) {
        corruptedError(archivename);
        return false;
    }

    readLocalFileHeader(cb, &info, exception);

    *filesize = info.compressedSize;
    strcpy(filename, info.filename);

    if (info.compressionMethod == 0) {
        /* No compression */
    } else if (info.compressionMethod == Z_DEFLATED) {
        switchToZippedCharBuff(cb);
    } else {
        compressionError(archivename, info.compressionMethod);
        return false;
    }
    goto fin;
 endOfFile:
    errorMessage(ERR_ST,"unexpected end of file");
 fin:
    return result;
}


static CharacterBuffer s_zipTmpBuff;


static void initZipArchiveDir(ZipArchiveDir *dir) {
    dir->u.sub = NULL;
    dir->next = NULL;
    dir->name[0] = '\0';
}


static void fillZipFileTableItem(ZipFileTableItem *fileItem, struct stat st, ZipArchiveDir *dir) {
    fileItem->st = st;
    fileItem->dir = dir;
}


bool fsIsMember(ZipArchiveDir **dirPointer, char *filename, unsigned offset,
                AddYesNo addFlag, ZipArchiveDir **outDirPointer) {
    ZipArchiveDir *aa, **aaa, *p;
    int itemlen, res;
    char *slashPosition;

    if (dirPointer == NULL)
        return false;

    /* Allow NULL to indicate "not interested" */
    if (outDirPointer != NULL)
        *outDirPointer = *dirPointer;

    res = true;
    if (filename[0] == 0) {
        errorMessage(ERR_INTERNAL, "looking for empty file name in 'fsdir'");
        return false;
    }
    if (filename[0]=='/' && filename[1]==0) {
        errorMessage(ERR_INTERNAL, "looking for root in 'fsdir'");
        return false;  /* should not happen */
    }
 lastrecLabel:
    slashPosition = strchr(filename,'/');
    if (slashPosition == NULL) {
        itemlen = strlen(filename);
        if (itemlen == 0)
            return res;  /* directory */
    } else {
        itemlen = (slashPosition-filename) + 1;
    }
    for(aaa=dirPointer, aa= *aaa; aa!=NULL; aaa= &(aa->next), aa = *aaa) {
        if (strncmp(filename,aa->name,itemlen)==0 && aa->name[itemlen]==0)
            break;
    }
    assert(itemlen > 0);
    if (aa==NULL) {
        res = false;
        if (addFlag == ADD_YES) {
            p = StackMemoryAllocC(sizeof(ZipArchiveDir)+itemlen+1, ZipArchiveDir);
            initZipArchiveDir(p);
            strncpy(p->name, filename, itemlen);
            p->name[itemlen]=0;
            log_trace("adding new item '%s'", p->name);
            if (filename[itemlen-1] == '/') {         /* directory */
                p->u.sub = NULL;
            } else {
                p->u.offset = offset;
            }
            *aaa = aa = p;
        } else {
            return false;
        }
    }

    if (outDirPointer != NULL)
        *outDirPointer = aa;

    if (filename[itemlen-1] == '/') {
        dirPointer = &(aa->u.sub);
        filename = filename+itemlen;
        goto lastrecLabel;
    } else {
        return res;
    }
}


void fsRecMapOnFiles(ZipArchiveDir *dir, char *zip, char *path, void (*fun)(char *zip, char *file, void *arg), void *arg) {
    ZipArchiveDir     *aa;
    char                *fn;
    char                npath[MAX_FILE_NAME_SIZE];
    int                 len;
    if (dir == NULL) return;
    for(aa=dir; aa!=NULL; aa=aa->next) {
        fn = aa->name;
        len = strlen(fn);
        sprintf(npath, "%s%s", path, fn);
        if (len==0 || fn[len-1]=='/') {
            fsRecMapOnFiles(aa->u.sub, zip, npath, fun, arg);
        } else {
            (*fun)(zip, npath, arg);
        }
    }
}


static void seekToPosition(CharacterBuffer *cb, int offset) {
    fseek(cb->file, offset, SEEK_SET);
    cb->next = cb->end = cb->lineBegin = cb->chars;
}


static bool findEndOfCentralDirectory(CharacterBuffer *cb, int fileSize) {
    int offset;
    char *ccc, *ffin;
    bool found = true;

    if (fileSize < CHAR_BUFF_SIZE)
        offset = 0;
    else
        offset = fileSize-(CHAR_BUFF_SIZE-MAX_UNGET_CHARS);
    seekToPosition(cb, offset);
    refillBuffer(cb);
    ccc = ffin = cb->end;
    for (ccc-=4; ccc>cb->chars && strncmp(ccc,"\120\113\005\006",4)!=0; ccc--) {
    }
    if (ccc <= cb->chars) {
        found = false;
        assert(options.taskRegime);
        if (options.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_INTERNAL,"can't find end of central dir in archive");
        }
        goto fini;
    }
 fini:
    cb->next = ccc; cb->end = ffin;
    return found;
}


static void readCentralDirectoryFileHeader(CharacterBuffer *cb, ZipFileInfo *info, jmp_buf exception) {
    GetZU2(info->versionMadeBy, cb, exception);
    GetZU2(info->versionNeededToExtract, cb, exception);
    GetZU2(info->bitFlags, cb, exception);
    GetZU2(info->compressionMethod, cb, exception);
    GetZU2(info->lastModificationTime, cb, exception);
    GetZU2(info->lastModificationDate, cb, exception);
    GetZU4(info->crc32, cb, exception);
    GetZU4(info->compressedSize, cb, exception);
    GetZU4(info->unCompressedSize, cb, exception);
    GetZU2(info->fileNameLength, cb, exception);
    if (info->fileNameLength >= MAX_FILE_NAME_SIZE) {
        fatalError(ERR_INTERNAL,"file name in .zip archive too long", XREF_EXIT_ERR);
    }
    GetZU2(info->extraFieldLength, cb, exception);
    GetZU2(info->fileCommentLength, cb, exception);
    GetZU2(info->diskNumber, cb, exception);
    GetZU2(info->internalFileAttributes, cb, exception);
    GetZU4(info->externalFileAttributes, cb, exception);
    GetZU4(info->localHeaderOffset, cb, exception);
    for(int i=0; i<info->fileNameLength; i++) {
        GetChar(info->filename[i], cb, exception);
    }
    info->filename[info->fileNameLength] = 0;
    log_trace("file '%s' in central dir", info->filename);
    skipCharacters(cb, info->extraFieldLength+info->fileCommentLength);
}


static bool isRealFile(ZipFileInfo info) {
    return strncmp(info.filename, "META-INF/", 9) != 0;
}


static void zipArchiveScan(CharacterBuffer *cb, ZipFileTableItem *zip, int fileSize) {
    ZipArchiveDir *place;
    unsigned signature;
    unsigned cdOffset;
    ZipFileInfo info;
    jmp_buf exception;

    ENTER();

    switch (setjmp(exception)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    }

    zip->dir = NULL;
    if (!findEndOfCentralDirectory(cb, fileSize)) {
        LEAVE();
        return;
    }

    GetZU4(signature, cb, exception);
    assert(signature == CENTRAL_DIRECTORY_SIGNATURE);
    skipCharacters(cb, 12);     /* Skip over 4 U2 and one U4 */
    GetZU4(cdOffset, cb, exception);
    seekToPosition(cb, cdOffset);

    /* Read signature, should be central directory file header */
    GetZU4(signature, cb, exception);
    while (signature == CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE) {
        readCentralDirectoryFileHeader(cb, &info, exception);

        if (isRealFile(info)) {
            log_trace("adding '%s'", info.filename);
            fsIsMember(&zip->dir, info.filename, info.localHeaderOffset, ADD_YES, &place);
        }
        /* Read next signature */
        GetZU4(signature, cb, exception);
    }
    LEAVE();
    return;

 endOfFile:
    errorMessage(ERR_ST, "unexpected end of file");
    LEAVE();
}


int zipIndexArchive(char *name) {
    int archiveIndex, namelen;
    FILE *zipFile;
    CharacterBuffer *buffer;
    struct stat fst;

    buffer = &s_zipTmpBuff;
    namelen = strlen(name);
    for(archiveIndex=0;
        archiveIndex<MAX_JAVA_ZIP_ARCHIVES && s_zipArchiveTable[archiveIndex].fn[0]!=0;
        archiveIndex++) {
        if (strncmp(s_zipArchiveTable[archiveIndex].fn,name,namelen)==0
            && s_zipArchiveTable[archiveIndex].fn[namelen]==ZIP_SEPARATOR_CHAR) {
            break;
        }
    }
    if (archiveIndex<MAX_JAVA_ZIP_ARCHIVES && s_zipArchiveTable[archiveIndex].fn[0] == 0) {
        // new file into the table
        log_debug("adding %s into index ",name);
        if (stat(name ,&fst)!=0) {
            assert(options.taskRegime);
            if (options.taskRegime!=RegimeEditServer) {
                static int singleOut=0;
                if (singleOut==0) warningMessage(ERR_CANT_OPEN, name);
                singleOut=1;
            }
            return(-1);
        }
#if defined (__WIN32__)
        zipFile = openFile(name,"rb");
#else
        zipFile = openFile(name,"r");
#endif
        if (zipFile == NULL) {
            assert(options.taskRegime);
            if (options.taskRegime!=RegimeEditServer) {
                warningMessage(ERR_CANT_OPEN, name);
            }
            return(-1);
        }
        initCharacterBuffer(buffer, zipFile);
        assert(namelen+2 < MAX_FILE_NAME_SIZE);
        strcpy(s_zipArchiveTable[archiveIndex].fn, name);
        s_zipArchiveTable[archiveIndex].fn[namelen] = ZIP_SEPARATOR_CHAR;
        s_zipArchiveTable[archiveIndex].fn[namelen+1] = 0;
        fillZipFileTableItem(&s_zipArchiveTable[archiveIndex], fst, NULL);
        zipArchiveScan(buffer,&s_zipArchiveTable[archiveIndex], fst.st_size);
        closeFile(zipFile);
    }
    return(archiveIndex);
}


static bool zipSeekToFile(CharacterBuffer *cb, char *name) {
    char *separatorPosition;
    int i, nameLength;
    unsigned lastSig, fsize;
    char fn[MAX_FILE_NAME_SIZE];
    ZipArchiveDir *place;

    separatorPosition = strchr(name, ZIP_SEPARATOR_CHAR);
    if (separatorPosition == NULL)
        return false;
    *separatorPosition = 0;
    nameLength = strlen(name);
    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && s_zipArchiveTable[i].fn[0]!=0; i++) {
        if (strncmp(s_zipArchiveTable[i].fn, name, nameLength) == 0
            && s_zipArchiveTable[i].fn[nameLength] == ZIP_SEPARATOR_CHAR) {
            break;
        }
    }
    *separatorPosition = ZIP_SEPARATOR_CHAR;
    if (i>=MAX_JAVA_ZIP_ARCHIVES || s_zipArchiveTable[i].fn[0]==0) {
        errorMessage(ERR_INTERNAL, "archive not indexed");
        return false;
    }
    if (!fsIsMember(&s_zipArchiveTable[i].dir, separatorPosition+1,0, ADD_NO, &place))
        return false;
    seekToPosition(cb, place->u.offset);

    if (!zipReadLocalFileInfo(cb, fn, &fsize,
                                &lastSig, s_zipArchiveTable[i].fn))
        return false;
    assert(lastSig == LOCAL_FILE_HEADER_SIGNATURE);
    assert(strcmp(fn,separatorPosition+1)==0);
    return true;
}

bool zipFindFile(char *name,
                 char **resName,             /* can be NULL !!! */
                 ZipFileTableItem *zipfile
) {
    char *pp;
    char fname[MAX_FILE_NAME_SIZE];
    ZipArchiveDir *place;

    strcpy(fname,name);
    strcat(fname,".class");
    assert(strlen(fname)+1 < MAX_FILE_NAME_SIZE);
    /*&fprintf(dumpOut,"looking for file %s in %s\n",fname,zipfile->fn);fflush(dumpOut);&*/
    if (fsIsMember(&zipfile->dir,fname,0,ADD_NO,&place)==0) return false;
    if (resName != NULL) {
        *resName = StackMemoryAllocC(strlen(zipfile->fn)+strlen(fname)+2, char);
        pp = strmcpy(*resName, zipfile->fn);
        strcpy(pp, fname);
    }
    return true;
}


void javaMapZipDirFile(
    ZipFileTableItem *zipfile,
    char *packfile,
    Completions *a1,
    void *a2,
    int *a3,
    void (*fun)(MAP_FUN_SIGNATURE),
    char *classPath,
    char *dirname
) {
    char fn[MAX_FILE_NAME_SIZE];
    char dirn[MAX_FILE_NAME_SIZE];
    ZipArchiveDir *place, *aa;
    char *ss;
    int dirlen;

    dirlen = strlen(dirname);
    strcpy(dirn,dirname);
    dirn[dirlen]='/';   dirn[dirlen+1]=0;
    /*&fprintf(dumpOut,":mapping dir, class, pack == %s,%s,%s\n",dirname,classPath,packfile);fflush(dumpOut);&*/
    if (dirlen == 0) {
        aa = zipfile->dir;
    } else if (fsIsMember(&zipfile->dir, dirn, 0, ADD_NO, &place)==1) {
        aa=place->u.sub;
    } else {
        return;
    }
    for(; aa!=NULL; aa=aa->next) {
        ss = strmcpy(fn, aa->name);
        if (fn[0]==0) {
            errorMessage(ERR_INTERNAL,"empty name in .zip directory structure");
            continue;
        }
        if (*(ss-1) == '/') *(ss-1) = 0;
        /*&fprintf(dumpOut,":mapping %s of %s pack %s\n",fn,dirname,packfile);fflush(dumpOut);&*/
        (*fun)(fn, classPath, packfile, a1, a2, a3);
    }
}


/* **************************************************************** */

static ConstantPoolUnion *cfReadConstantPool(CharacterBuffer *cb,
                                             int *cpSize) {
    ConstantPoolUnion *cp=NULL;
    int count, tag, index;
    int value, stringIndex;
    int length;
    char *string;
    jmp_buf exception;

    switch (setjmp(exception)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    }

    GetU2(count, cb, exception);
    CF_ALLOCC(cp, count, ConstantPoolUnion);
    //& memset(cp,0, count*sizeof(ConstantPoolUnion));    // if broken file
    for(index=1; index<count; index++) {
        GetU1(tag, cb, exception);
        switch (tag) {
        case CONSTANT_Utf8:
            GetU2(length, cb, exception);
            CF_ALLOCC(string, length+1, char);
            for(int i=0; i<length; i++)
                GetChar(string[i], cb, exception);
            string[length]=0;
            cp[index].utf8 = string;
            break;
        case CONSTANT_Class:
            GetU2(cp[index].class.name_index, cb, exception);
            break;
        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
            GetU2(cp[index].ref.class_index, cb, exception);
            GetU2(cp[index].ref.name_and_type_index, cb, exception);
            break;
        case CONSTANT_NameandType:
            GetU2(cp[index].nameAndType.name_index, cb, exception);
            GetU2(cp[index].nameAndType.descriptor_index, cb, exception);
            break;
        case CONSTANT_String:
            GetU2(stringIndex, cb, exception);
            break;
        case CONSTANT_Integer:
        case CONSTANT_Float:
            GetU4(value, cb, exception);
            break;
        case CONSTANT_Long:
        case CONSTANT_Double:
            GetU4(value, cb, exception);
            index ++;
            GetU4(value, cb, exception);
            break;
        case CONSTANT_MethodHandle:
            GetU1(cp[index].methodHandle.reference_kind, cb, exception);
            GetU2(cp[index].methodHandle.reference_index, cb, exception);
            break;
        case CONSTANT_MethodType:
            GetU2(cp[index].methodType.descriptor_index, cb, exception);
            break;
        case CONSTANT_Dynamic:
        case CONSTANT_InvokeDynamic:
            GetU2(cp[index].dynamic.bootstrap_method_attr_index, cb, exception);
            GetU2(cp[index].dynamic.name_and_type_index, cb, exception);
            break;
        default: {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,"unexpected tag %d in constant pool of %s", tag, currentFile.fileName);
            fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
        }
        }
    }
    goto fin;
 endOfFile:
    errorMessage(ERR_ST, "unexpected end of file");
 fin:
    *cpSize = count;
    return(cp);
}


char *skipFirstArgumentInDescriptorString(char *descriptor) {
    char *d = descriptor;

    assert(descriptor);
    log_trace("skipping first argument of %s", descriptor);
    if (*d != '(')
        return(d);
    d++;
    assert(*d);
    switch (*d) {
    case ')':
        break;
    case '[':
        for(d++; *d && isdigit(*d); d++)
            ;
        break;
    case 'L':
        for(; *d && *d!=';'; d++)
            ;
        d++;
        break;
    default:
        d++;
    }
    return(d);
}


TypeModifier *cfUnPackResultType(char *sig, char **restype) {
    TypeModifier *res, **ares, *tt;
    int typ;
    char *fqname;
    char *ssig, *ccname;

    res = NULL;
    ares = &res;
    ssig = sig;
    assert(ssig);
    /*fprintf(dumpOut, ": decoding %s\n",sig);*/
    if (*ssig == '(') {
        while (*ssig && *ssig!=')') ssig++;
        *restype = ssig+1;
    } else {
        *restype = ssig;
    }
    assert(*ssig);
    for(; *ssig; ssig++) {
        CF_ALLOC(tt, TypeModifier);
        *ares = tt;
        ares = &(tt->next);
        switch (*ssig) {
        case ')':
            initTypeModifierAsMethod(tt, NULL, NULL, NULL, NULL);
            assert(*sig == '(');
            /* signature must be set later !!!!!!!!!! */
            break;
        case '[':
            initTypeModifierAsArray(tt, NULL, NULL);
            for(ssig++; *ssig && isdigit(*ssig); ssig++) ;
            ssig--;
            break;
        case 'L':
            initTypeModifierAsStructUnionOrEnum(tt, TypeStruct, NULL, NULL, NULL);
            fqname = ++ssig;
            ccname = fqname;
            for(; *ssig && *ssig!=';'; ssig++) {
                if (*ssig == '/' || *ssig == '$') ccname = ssig+1;
            }
            assert(*ssig == ';');
            *ssig = 0;
            tt->u.t = javaFQTypeSymbolDefinition(ccname, fqname);
            *ssig = ';';
            break;
        default:
            typ = s_javaCharCodeBaseTypes[*ssig];
            assert(typ != 0);
            initTypeModifier(tt, typ);
        }
    }
    return(res);
}


static void cfAddRecordToClass(char *name,
                               char *sig,
                               Symbol *clas,
                               int accessFlags,
                               Storage storage,
                               SymbolList *exceptions
                               ) {
    static char pp[MAX_PROFILE_SIZE];
    Symbol *symbol, *memb;
    char *linkName,*prof;
    TypeModifier *tt;
    char *restype;
    int len, vFunCl;
    int rr,bc;
    Position dpos;

    tt = cfUnPackResultType(sig, &restype);
    if (tt->kind==TypeFunction) {
        // hack, this will temporary cut the result,
        // however if this is the shared string with name ....? B of B for ex.
        bc = *restype; *restype = 0;
        if ((accessFlags & AccessStatic)) {
            /*&
              sprintf(pp,"%s.%s%s", clas->linkName, name, sig);
              vFunCl = noFileIndex;
              &*/
        }
        sprintf(pp,"%s%s", name, sig);
        vFunCl = clas->u.s->classFile;
        *restype = bc;
    } else {
        sprintf(pp,"%s", name);
        vFunCl = clas->u.s->classFile;
        /*&
          sprintf(pp,"%s.%s", clas->linkName, name);
          vFunCl = noFileIndex;
          &*/
    }
    //& rr = javaExistEquallyProfiledFun(clas, name, sig, &memb);
    rr = javaIsYetInTheClass(clas, pp, &memb);
    //& rr = 0;
    if (rr) {
        linkName = memb->linkName;
    } else {
        len = strlen(pp);
        CF_ALLOCC(linkName, len+1, char);
        strcpy(linkName, pp);
    }
    prof = strchr(linkName,'(');
    if (tt->kind == TypeFunction) {
        assert(*sig == '(');
        assert(*prof == '(');
        tt->u.m.signature = prof;
        tt->u.m.exceptions = exceptions;
    }

    log_trace("adding definition of %s == %s", name, linkName);
    /* TODO If this was allocated in "normal" memory we could use newSymbol() */
    CF_ALLOC(symbol, Symbol);
    fillSymbolWithType(symbol, name, linkName, s_noPos, tt);
    fillSymbolBits(&symbol->bits, accessFlags, TypeDefault, storage);

    assert(clas->u.s);
    LIST_APPEND(Symbol, clas->u.s->records, symbol);
    if (options.allowClassFileRefs) {
        fillPosition(&dpos, clas->u.s->classFile, 1, 0);
        addCxReference(symbol, &dpos, UsageClassFileDefinition, vFunCl, vFunCl);
    }
}


static void cfReadFieldInfos(CharacterBuffer *cb,
                             Symbol *memb,
                             ConstantPoolUnion *cp
) {
    int count, ind;
    int access_flags, nameind, sigind;
    jmp_buf exception;

    switch (setjmp(exception)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    }

    GetU2(count, cb, exception);
    for(ind=0; ind<count; ind++) {
        GetU2(access_flags, cb, exception);
        GetU2(nameind, cb, exception);
        GetU2(sigind, cb, exception);
        log_trace("field '%s' of type '%s'", cp[nameind].utf8, cp[sigind].utf8);
        cfAddRecordToClass(cp[nameind].utf8, cp[sigind].utf8, memb, access_flags,
                           StorageField, NULL);
        SkipAttributes(cb);
    }
    return;
 endOfFile:
    errorMessage(ERR_ST, "unexpected end of file");
}

static char *simpleClassNameFromFQTName(char *fqtName) {
    char *res, *ss;
    res = fqtName;
    for(ss=fqtName; *ss; ss++) {
        if (*ss=='/' || *ss=='$') res = ss+1;
    }
    return(res);
}


static void cfReadMethodInfos(CharacterBuffer *cb,
                              Symbol *memb,
                              ConstantPoolUnion *cp
) {
    char *name, *descriptor, *sign2;
    unsigned count, ind;
    unsigned aind, acount, aname, alen, excount;
    int i, access_flags, name_index, descriptor_index, exclass;
    Storage storage;
    Symbol *exc;
    SymbolList *exclist, *ee;
    jmp_buf exception;

    switch (setjmp(exception)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    }

    GetU2(count, cb, exception);
    for(ind=0; ind<count; ind++) {
        GetU2(access_flags, cb, exception);
        GetU2(name_index, cb, exception);
        GetU2(descriptor_index, cb, exception);
        log_trace("method '%s' of type '%s'", cp[name_index].utf8,cp[descriptor_index].utf8);
        // TODO more efficiently, just index checking
        name = cp[name_index].utf8;
        descriptor = cp[descriptor_index].utf8;
        storage = StorageMethod;
        exclist = NULL;
        if (strcmp(name, JAVA_CONSTRUCTOR_NAME1)==0
            || strcmp(name, JAVA_CONSTRUCTOR_NAME2)==0) {
            // isn't it already done by javac?
            //&if (strcmp(name, JAVA_CONSTRUCTOR_NAME2)==0) {
            //& access_flags |= AccessStatic;
            //&}
            // if constructor, put there type name as constructor name
            // instead of <init>
            log_trace("constructor '%s' of '%s'", name, memb->name);
            assert(memb && memb->bits.symType==TypeStruct && memb->u.s);
            name = memb->name;
            storage = StorageConstructor;
            if (fileTable.tab[memb->u.s->classFile]->directEnclosingInstance != noFileIndex) {
                // the first argument is direct enclosing instance, remove it
                sign2 = skipFirstArgumentInDescriptorString(descriptor);
                CF_ALLOCC(descriptor, strlen(sign2)+2, char);
                descriptor[0] = '(';  strcpy(descriptor+1, sign2);
                log_trace("nested constructor '%s' '%s'", name, descriptor);
            }
        } else if (name[0] == '<') {
            // still some strange constructor name?
            log_trace("strange constructor '%s' '%s'", name, descriptor);
            storage = StorageConstructor;
        }
        GetU2(acount, cb, exception);
        for(aind=0; aind<acount; aind++) {
            GetU2(aname, cb, exception);
            GetU4(alen, cb, exception);
            // berk, really I need to compare strings?
            if (strcmp(cp[aname].utf8, "Exceptions")==0) {
                GetU2(excount, cb, exception);
                for(i=0; i<excount; i++) {
                    char *exname, *exsname;
                    GetU2(exclass, cb, exception);
                    exname = cp[cp[exclass].class.name_index].utf8;
                    log_trace("throws '%s'", exname);
                    exsname = simpleClassNameFromFQTName(exname);
                    exc = javaFQTypeSymbolDefinition(exsname, exname);
                    CF_ALLOC(ee, SymbolList);
                    /* REPLACED: FILL_symbolList(ee, exc, exclist); with compound literal */
                    *ee = (SymbolList){.d = exc, .next = exclist};
                    exclist = ee;
                }
            } else if (strcmp(cp[aname].utf8, "Code")==0) {
                // forget this, it is useless as .jar usually do not cantain
                // informations about variable names.
                unsigned max_stack, max_locals, code_length;
                unsigned exception_table_length,attributes_count;
                unsigned caname, calen;
                int ii;
                // look here for local variable names
                GetU2(max_stack, cb, exception);
                GetU2(max_locals, cb, exception);
                GetU4(code_length, cb, exception);
                skipCharacters(cb, code_length);
                GetU2(exception_table_length, cb, exception);
                skipCharacters(cb, (exception_table_length*8));
                GetU2(attributes_count, cb, exception);
                for(ii=0; ii<attributes_count; ii++) {
                    int iii;
                    GetU2(caname, cb, exception);
                    GetU4(calen, cb, exception);
                    if (strcmp(cp[caname].utf8, "LocalVariableTable")==0) {
                        unsigned local_variable_table_length;
                        unsigned start_pc,length,name_index,descriptor_index;
                        unsigned index;
                        GetU2(local_variable_table_length, cb, exception);
                        for(iii=0; iii<local_variable_table_length; iii++) {
                            GetU2(start_pc, cb, exception);
                            GetU2(length, cb, exception);
                            GetU2(name_index, cb, exception);
                            GetU2(descriptor_index, cb, exception);
                            GetU2(index, cb, exception);
                            log_trace("local variable %s, index == %d", cp[name_index].utf8, index);
                        }
                    } else {
                        skipCharacters(cb, calen);
                    }
                }
            } else {
                log_trace("skipping %s", cp[aname].utf8);
                skipCharacters(cb, alen);
            }
        }
        cfAddRecordToClass(name, descriptor, memb, access_flags, storage, exclist);
    }
    return;
 endOfFile:
    errorMessage(ERR_ST,"unexpected end of file");
}


Symbol *cfAddCastsToModule(Symbol *memb, Symbol *sup) {
    assert(memb->u.s);
    cctAddSimpleValue(&memb->u.s->casts, sup, 1);
    assert(sup->u.s);
    cctAddCctTree(&memb->u.s->casts, &sup->u.s->casts, 1);
    return(sup);
}


void addSuperClassOrInterface(Symbol *member, Symbol *super, int origin) {
    SymbolList *symbolList, *s;
    char tmpBuff[TMP_BUFF_SIZE];

    super = javaFQTypeSymbolDefinition(super->name, super->linkName);
    for(s = member->u.s->super; s != NULL && s->d != super; s = s->next)
        ;
    if (s != NULL && s->d == super)
        return; // avoid multiple occurrences
    log_debug("adding superclass %s to %s", super->linkName, member->linkName);
    if (cctIsMember(&super->u.s->casts, member, 1) || member == super) {
        sprintf(tmpBuff, "detected cycle in super classes of %s",
                member->linkName);
        errorMessage(ERR_ST, tmpBuff);
        return;
    }
    cfAddCastsToModule(member, super);
    CF_ALLOC(symbolList, SymbolList);
    /* REPLACED: FILL_symbolList(ssl, supp, NULL); with compound literal */
    *symbolList = (SymbolList){.d = super, .next = NULL};
    LIST_APPEND(SymbolList, member->u.s->super, symbolList);
    addSubClassItemToFileTab(super->u.s->classFile,
                             member->u.s->classFile,
                             origin);
}


void addSuperClassOrInterfaceByName(Symbol *member, char *super, int origin,
                                    LoadSuperOrNot loadSuper) {
    Symbol *s;

    s = javaGetFieldClass(super, NULL);
    if (loadSuper == LOAD_SUPER)
        javaLoadClassSymbolsFromFile(s);
    addSuperClassOrInterface(member, s, origin);
}


void convertLinkNameToClassFileName(char classFileName[], char *linkName) {
    sprintf(classFileName, "%c%s.class", ZIP_SEPARATOR_CHAR, linkName);
    assert(strlen(classFileName)+1 < MAX_FILE_NAME_SIZE);
}


int javaCreateClassFileItem( Symbol *memb) {
    char ftname[MAX_FILE_NAME_SIZE];
    int fileIndex;

    convertLinkNameToClassFileName(ftname, memb->linkName);
    fileIndex = addFileTabItem(ftname);
    memb->u.s->classFile = fileIndex;

    return fileIndex;
}


/* ********************************************************************* */

void javaReadClassFile(char *className, Symbol *symbol, LoadSuperOrNot loadSuper) {
    int readValue, i, inum, innval, rinners, modifs;
    FILE *zipFile;
    int upp, innNameInd;
    bool membFlag;
    char *super, *interf, *innerCName, *zipSeparatorIndex;
    Symbol *inners;
    int thisClass, superClass, access, cpSize;
    int fileIndex, ind, count, aname, alen, cn;
    char *inner, *upper, *thisClassName;
    ConstantPoolUnion *constantPool;
    Position pos;
    char tmpBuff[TMP_BUFF_SIZE];
    int major, minor;           /* Version numbers of class file format */
    CharacterBuffer *cb;
    jmp_buf exception;

    ENTER();

    switch (setjmp(exception)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    }

    symbol->bits.javaFileIsLoaded = 1;

    /* Open the file, the name is prefixing the actual class name separated by separator, if any */
    zipSeparatorIndex = strchr(className, ZIP_SEPARATOR_CHAR);
    if (zipSeparatorIndex != NULL)
        *zipSeparatorIndex = 0;
    zipFile = openFile(className, "rb");
    if (zipSeparatorIndex != NULL)
        *zipSeparatorIndex = ZIP_SEPARATOR_CHAR;

    if (zipFile == NULL) {
        errorMessage(ERR_CANT_OPEN, className);
        LEAVE();
        return;
    }

    fileIndex = javaCreateClassFileItem(symbol);
    fileTable.tab[fileIndex]->b.cxLoading = true;

    fillPosition(&pos, fileIndex, 1, 0);
    addCxReference(symbol, &pos, UsageClassFileDefinition,
                   noFileIndex, noFileIndex);
    addCfClassTreeHierarchyRef(fileIndex, UsageClassFileDefinition);
    log_trace("fileitem=='%s'", fileTable.tab[fileIndex]->name);
    pushNewInclude(zipFile, NULL, fileTable.tab[fileIndex]->name, "");

    log_debug("reading file '%s'", className);

    cb = &currentFile.lexBuffer.buffer;
    if (zipSeparatorIndex != NULL) {
        if (!zipSeekToFile(cb, className))
            goto finish;
    }
    GetU4(readValue, cb, exception);
    log_trace("magic is 0x%x", readValue);
    if (readValue != 0xcafebabe) {
        sprintf(tmpBuff,"%s is not a valid class file", className);
        errorMessage(ERR_ST, tmpBuff);
        goto finish;
    }
    GetU2(minor, cb, exception);
    GetU2(major, cb, exception);
    log_trace("version of '%s' is %d.%d", className, major, minor);
    constantPool = cfReadConstantPool(cb, &cpSize);
    GetU2(access, cb, exception);
    symbol->bits.access = access;
    log_trace("reading accessFlags %s == %x", className, access);
    if (access & AccessInterface)
        fileTable.tab[fileIndex]->b.isInterface = true;
    GetU2(thisClass, cb, exception);
    if (thisClass<0 || thisClass>=cpSize)
        goto corrupted;
    thisClassName = constantPool[constantPool[thisClass].class.name_index].utf8;
    // TODO!!!, it may happen that name of class differ in cases from name of file,
    // what to do in such case? abandon with an error?
    log_trace("this class == %s", thisClassName);
    GetU2(superClass, cb, exception);
    if (superClass != 0) {
        if (superClass<0 || superClass>=cpSize)
            goto corrupted;
        super = constantPool[constantPool[superClass].class.name_index].utf8;
        addSuperClassOrInterfaceByName(symbol, super, symbol->u.s->classFile,
                                       loadSuper);
    }

    GetU2(inum, cb, exception);
    /* implemented interfaces */

    for(i=0; i<inum; i++) {
        GetU2(readValue, cb, exception);
        if (readValue != 0) {
            if (readValue<0 || readValue>=cpSize) goto corrupted;
            interf = constantPool[constantPool[readValue].class.name_index].utf8;
            addSuperClassOrInterfaceByName(symbol, interf, symbol->u.s->classFile,
                                           loadSuper);
        }
    }
    //& addSubClassesItemsToFileTab(symbol, symbol->u.s->classFile);

    cfReadFieldInfos(cb, symbol, constantPool);

    if (currentFile.lexBuffer.buffer.isAtEOF)
        goto endOfFile;
    cfReadMethodInfos(cb, symbol, constantPool);
    if (currentFile.lexBuffer.buffer.isAtEOF)
        goto endOfFile;

    GetU2(count, cb, exception);
    for(ind=0; ind<count; ind++) {
        GetU2(aname, cb, exception);
        GetU4(alen, cb, exception);
        if (strcmp(constantPool[aname].utf8,"InnerClasses")==0) {
            GetU2(inum, cb, exception);
            symbol->u.s->nestedCount = inum;
            // TODO: replace the inner tab by inner list
            if (inum >= MAX_INNERS_CLASSES) {
                fatalError(ERR_ST,"number of nested classes overflowed over MAX_INNERS_CLASSES", XREF_EXIT_ERR);
            }
            symbol->u.s->nest = NULL;
            if (inum > 0) {
                // I think this should be optimized, not all mentioned here
                // are my inners classes
                //&             CF_ALLOCC(symbol->u.s->nest, MAX_INNERS_CLASSES, S_nestedSpec);
                CF_ALLOCC(symbol->u.s->nest, inum, S_nestedSpec);
            }
            for(rinners=0; rinners<inum; rinners++) {
                GetU2(innval, cb, exception);
                inner = constantPool[constantPool[innval].class.name_index].utf8;
                //&fprintf(dumpOut,"inner %s \n",inner);fflush(dumpOut);
                GetU2(upp, cb, exception);
                GetU2(innNameInd, cb, exception);
                if (innNameInd==0) innerCName = "";         // !!!!!!!! hack
                else innerCName = constantPool[innNameInd].utf8;
                //&fprintf(dumpOut,"class name %x='%s'\n",innerCName,innerCName);fflush(dumpOut);
                inners = javaFQTypeSymbolDefinition(innerCName, inner);
                membFlag = false;
                if (upp != 0) {
                    upper = constantPool[constantPool[upp].class.name_index].utf8;
                    //&{fprintf(dumpOut,"upper %s encloses %s\n",upper, inner);fflush(dumpOut);}
                    if (strcmp(upper,thisClassName)==0) {
                        membFlag = true;
                        /*&fprintf(dumpOut,"set as member class \n"); fflush(dumpOut);&*/
                    }
                }
                GetU2(modifs, cb, exception);
                //& inners->bits.access |= modifs;
                //&fprintf(dumpOut,"modif? %x\n",modifs);fflush(dumpOut);

                fill_nestedSpec(& symbol->u.s->nest[rinners], inners, membFlag, modifs);
                assert(inners && inners->bits.symType==TypeStruct && inners->u.s);
                cn = inners->u.s->classFile;
                if (membFlag && ! (modifs & AccessStatic)) {
                    // note that non-static direct enclosing class exists
                    assert(fileTable.tab[cn]);
                    fileTable.tab[cn]->directEnclosingInstance = symbol->u.s->classFile;
                } else {
                    fileTable.tab[cn]->directEnclosingInstance = noFileIndex;
                }
            }
        } else {
            skipCharacters(cb, alen);
        }
    }
    fileTable.tab[fileIndex]->b.cxLoaded = true;
    goto finish;

 endOfFile:
    sprintf(tmpBuff,"unexpected end of file '%s'", className);
    errorMessage(ERR_ST, tmpBuff);
    goto emergency;
 corrupted:
    sprintf(tmpBuff,"corrupted file '%s'", className);
    errorMessage(ERR_ST, tmpBuff);
    goto emergency;
 emergency:
    symbol->u.s->nestedCount = 0;
 finish:
    log_debug("closing file %s", className);
    popInclude();

    LEAVE();
}
