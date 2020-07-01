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


struct javaCpClassInfo {
    short unsigned  nameIndex;
};
struct javaCpNameAndTypeInfo {
    short unsigned  nameIndex;
    short unsigned  signatureIndex;
};
struct javaCpRecordInfo {
    short unsigned  classIndex;
    short unsigned  nameAndTypeIndex;
};

union constantPoolUnion {
    char                            *asciz;
    struct javaCpClassInfo          clas;
    struct javaCpNameAndTypeInfo    nt;
    struct javaCpRecordInfo         rec;
};

S_zipFileTableItem s_zipArchiveTable[MAX_JAVA_ZIP_ARCHIVES];

/* *********************************************************************** */

#define GetChar(ch, cb_next, cb_end, cb) {                              \
        if (cb_next >= cb_end) {                                        \
            (cb)->next = cb_next;                                       \
            if ((cb)->isAtEOF || refillBuffer(cb) == 0) {               \
                ch = -1;                                                \
                (cb)->isAtEOF = true;                                   \
                goto endOfFile;                                         \
            } else {                                                    \
                cb_next = (cb)->next;                                   \
                cb_end = (cb)->end;                                     \
                ch = * ((unsigned char*)cb_next); cb_next ++;           \
            }                                                           \
        } else {                                                        \
            ch = * ((unsigned char*)cb_next); cb_next++;                \
        }                                                               \
        (cb)->next = cb_next;                                           \
        (cb)->end = cb_end;                                             \
    }


#define GetU1(value, cb_next, cb_end, cb) {             \
        GetChar(value, cb_next, cb_end, cb);            \
        (cb)->next = cb_next;                           \
        (cb)->end = cb_end;                             \
    }


#define GetU2(value, cb_next, cb_end, cb) {             \
        int ch;                                         \
        GetChar(value, cb_next, cb_end, cb);            \
        GetChar(ch, cb_next, cb_end, cb);               \
        value = value*256+ch;                           \
        (cb)->next = cb_next;                           \
        (cb)->end = cb_end;                             \
    }

#define GetU4(value, cb_next, cb_end, cb) {             \
        int ch;                                         \
        GetChar(value, cb_next, cb_end, cb);            \
        GetChar(ch, cb_next, cb_end, cb);               \
        value = value*256+ch;                           \
        GetChar(ch, cb_next, cb_end, cb);               \
        value = value*256+ch;                           \
        GetChar(ch, cb_next, cb_end, cb);               \
        value = value*256+ch;                           \
        (cb)->next = cb_next;                           \
        (cb)->end = cb_end;                             \
    }

#define GetZU2(value, cb_next, cb_end, cb) {            \
        unsigned ch;                                    \
        GetChar(value, cb_next, cb_end, cb);            \
        GetChar(ch, cb_next, cb_end, cb);               \
        value = value+(ch<<8);                          \
        (cb)->next = cb_next;                           \
        (cb)->end = cb_end;                             \
    }

#define GetZU4(value, cb_next, cb_end, cb) {            \
        unsigned ch;                                    \
        GetChar(value, cb_next, cb_end, cb);            \
        GetChar(ch, cb_next, cb_end, cb);               \
        value = value+(ch<<8);                          \
        GetChar(ch, cb_next, cb_end, cb);               \
        value = value+(ch<<16);                         \
        GetChar(ch, cb_next, cb_end, cb);               \
        value = value+(ch<<24);                         \
        (cb)->next = cb_next;                           \
        (cb)->end = cb_end;                             \
    }

#define SkipNChars(count, cb_next, cb_end, cb) {           \
        if (cb_next + count < cb_end) {                    \
            cb_next += count;                              \
            (cb)->next = cb_next;                          \
        } else {                                           \
            (cb)->next = cb_next;                          \
            skipNCharsInCharacterBuffer(cb, (count));      \
            cb_next = (cb)->next;                          \
            cb_end = (cb)->end;                            \
        }                                                  \
        assert(cb_next == (cb)->next);                     \
        assert(cb_end == (cb)->end);                       \
    }

#define SeekToPosition(cb, offset) {                                    \
        fseek((cb)->file, offset, SEEK_SET);                            \
        (cb)->next = (cb)->end = (cb)->lineBegin = (cb)->chars;         \
    }

#define SkipAttributes(cb_next, cb_end, cb) {               \
        int index, count, aname, alen;                      \
        GetU2(count, cb_next, cb_end, cb);                  \
        for(index=0; index<count; index++) {                \
            GetU2(aname, cb_next, cb_end, cb);              \
            GetU4(alen, cb_next, cb_end, cb);               \
            SkipNChars(alen, cb_next, cb_end, cb);          \
        }                                                   \
        (cb)->next = cb_next;                               \
        (cb)->end = cb_end;                                 \
    }

/* *************** first something to read zip-files ************** */

static int zipReadLocalFileHeader(char **accc, char **affin, CharacterBuffer *cb,
                                  char *fn, unsigned *fsize, unsigned *lastSig,
                                  char *archivename) {
    int res;
    char *ccc, *ffin;
    int i;
    int signature,extractVersion,bitFlags,compressionMethod;
    int lastModTime,lastModDate,fnameLen,extraLen;
    unsigned crc32,compressedSize,unCompressedSize;
    static int compressionErrorWritten=0;
    char *zzz, ttt[MAX_FILE_NAME_SIZE];

    assert(cb->next == *accc);
    assert(cb->end == *affin);
    res = 1;
    ccc = cb->next; ffin = cb->end;
    GetZU4(signature,ccc,ffin,cb);
    log_trace("zip file signature is %x", signature);
    *lastSig = signature;
    if (signature != 0x04034b50) {
        static int messagePrinted = 0;
        if (messagePrinted==0) {
            char tmpBuff[TMP_BUFF_SIZE];
            messagePrinted = 1;
            sprintf(tmpBuff,
                    "archive %s is corrupted or modified while xref task running",
                    archivename);
            errorMessage(ERR_ST, tmpBuff);
            if (options.taskRegime == RegimeEditServer) {
                fprintf(errOut,"\t\tplease, kill xref process and retry.\n");
            }
        }
        res = 0;
        goto fin;
    }
    GetZU2(extractVersion,ccc,ffin,cb);
    GetZU2(bitFlags,ccc,ffin,cb);
    GetZU2(compressionMethod,ccc,ffin,cb);
    GetZU2(lastModTime,ccc,ffin,cb);
    GetZU2(lastModDate,ccc,ffin,cb);
    GetZU4(crc32,ccc,ffin,cb);
    GetZU4(compressedSize,ccc,ffin,cb);
    *fsize = compressedSize;
    GetZU4(unCompressedSize,ccc,ffin,cb);
    GetZU2(fnameLen,ccc,ffin,cb);
    if (fnameLen >= MAX_FILE_NAME_SIZE) {
        fatalError(ERR_INTERNAL,"file name too long", XREF_EXIT_ERR);
    }
    GetZU2(extraLen,ccc,ffin,cb);
    for(i=0; i<fnameLen; i++) {
        GetChar(fn[i],ccc,ffin,cb);
    }
    fn[i] = 0;
    SkipNChars(extraLen,ccc,ffin,cb);
    if (compressionMethod == 0) {
    }
    else if (compressionMethod == Z_DEFLATED) {
        cb->next = ccc;
        cb->end = ffin;
        switchToZippedCharBuff(cb);
        ccc = cb->next;
        ffin = cb->end;
    }
    else {
        res = 0;
        if (compressionErrorWritten==0) {
            char tmpBuff[TMP_BUFF_SIZE];
            assert(options.taskRegime);
            // why the message was only for editserver?
            //&if (options.taskRegime==RegimeEditServer) {
            strcpy(ttt, archivename);
            zzz = strchr(ttt, ZIP_SEPARATOR_CHAR);
            if (zzz!=NULL) *zzz = 0;
            sprintf(tmpBuff,"\n\tfiles in %s are compressed by unknown method #%d\n", ttt, compressionMethod);
            sprintf(tmpBuff+strlen(tmpBuff),
                    "\tRun 'jar2jar0 %s' command to uncompress them.", ttt);
            assert(strlen(tmpBuff)+1 < TMP_BUFF_SIZE);
            errorMessage(ERR_ST,tmpBuff);
            //&}
            compressionErrorWritten = 1;
        }
    }
    goto fin;
 endOfFile:
    errorMessage(ERR_ST,"unexpected end of file");
 fin:
    *accc = ccc; *affin = ffin;
    cb->next = ccc; cb->end = ffin;
    return(res);
}


static CharacterBuffer s_zipTmpBuff;


static void initZipArchiveDir(S_zipArchiveDir *dir) {
    dir->u.sub = NULL;
    dir->next = NULL;
    dir->name[0] = '\0';
}


static void fillZipFileTableItem(S_zipFileTableItem *fileItem, struct stat st, S_zipArchiveDir *dir) {
    fileItem->st = st;
    fileItem->dir = dir;
}

bool fsIsMember(S_zipArchiveDir **dirPointer, char *fn, unsigned offset,
                AddYesNo addFlag, S_zipArchiveDir **outDirPointer) {
    S_zipArchiveDir     *aa, **aaa, *p;
    int                 itemlen, res;
    char                *ss;

    if (dirPointer == NULL)
        return false;

    /* Allow NULL to indicate "not interested" */
    if (outDirPointer != NULL)
        *outDirPointer = *dirPointer;

    res = true;
    if (fn[0] == 0) {
        errorMessage(ERR_INTERNAL, "looking for empty file name in 'fsdir'");
        return false;
    }
    if (fn[0]=='/' && fn[1]==0) {
        errorMessage(ERR_INTERNAL, "looking for root in 'fsdir'");
        return false;  /* should not happen */
    }
 lastrecLabel:
    ss = strchr(fn,'/');
    if (ss == NULL) {
        itemlen = strlen(fn);
        if (itemlen == 0)
            return res;  /* directory */
    } else {
        itemlen = (ss-fn) + 1;
    }
    for(aaa=dirPointer, aa= *aaa; aa!=NULL; aaa= &(aa->next), aa = *aaa) {
        log_trace("comparing %s <-> %s of len %d", fn, aa->name, itemlen);
        if (strncmp(fn,aa->name,itemlen)==0 && aa->name[itemlen]==0)
            break;
    }
    assert(itemlen > 0);
    if (aa==NULL) {
        res = false;
        if (addFlag == ADD_YES) {
            p = StackMemoryAllocC(sizeof(S_zipArchiveDir)+itemlen+1, S_zipArchiveDir);
            initZipArchiveDir(p);
            strncpy(p->name, fn, itemlen);
            p->name[itemlen]=0;
            log_trace("adding new item '%s'", p->name);
            if (fn[itemlen-1] == '/') {         /* directory */
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

    if (fn[itemlen-1] == '/') {
        dirPointer = &(aa->u.sub);
        fn = fn+itemlen;
        goto lastrecLabel;
    } else {
        return res;
    }
}

void fsRecMapOnFiles(S_zipArchiveDir *dir, char *zip, char *path, void (*fun)(char *zip, char *file, void *arg), void *arg) {
    S_zipArchiveDir     *aa;
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

static int findEndOfCentralDirectory(char **accc, char **affin,
                                     CharacterBuffer *iBuf, int fsize) {
    int offset,res;
    char *ccc, *ffin;
    res = 1;
    if (fsize < CHAR_BUFF_SIZE) offset = 0;
    else offset = fsize-(CHAR_BUFF_SIZE-MAX_UNGET_CHARS);
    fseek(iBuf->file, offset, SEEK_SET);
    iBuf->next = iBuf->end;
    refillBuffer(iBuf);
    ccc = ffin = iBuf->end;
    for (ccc-=4; ccc>iBuf->chars && strncmp(ccc,"\120\113\005\006",4)!=0; ccc--) {
    }
    if (ccc <= iBuf->chars) {
        res = 0;
        assert(options.taskRegime);
        if (options.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_INTERNAL,"can't find end of central dir in archive");
        }
        goto fini;
    }
 fini:
    *accc = ccc; *affin = ffin;
    return(res);
}

static void zipArchiveScan(char **accc, char **affin, CharacterBuffer *cb,
                           S_zipFileTableItem *zip, int fsize) {
    char fn[MAX_FILE_NAME_SIZE];
    char *ccc, *ffin;
    S_zipArchiveDir *place;
    int signature,madeByVersion,extractVersion,bitFlags,compressionMethod;
    int lastModTime,lastModDate,fnameLen,extraLen,fcommentLen,diskNumber;
    int i,internFileAttribs,tmp;
    unsigned externFileAttribs,localHeaderOffset, cdOffset;
    unsigned crc32,compressedSize,unCompressedSize;
    unsigned foffset;
    UNUSED foffset;

    ccc = *accc; ffin = *affin;
    zip->dir = NULL;
    if (findEndOfCentralDirectory(&ccc, &ffin, cb, fsize)==0)
        goto fini;
    GetZU4(signature,ccc,ffin,cb);
    assert(signature == 0x06054b50);
    GetZU2(tmp,ccc,ffin,cb);
    GetZU2(tmp,ccc,ffin,cb);
    GetZU2(tmp,ccc,ffin,cb);
    GetZU2(tmp,ccc,ffin,cb);
    GetZU4(tmp,ccc,ffin,cb);
    GetZU4(cdOffset,ccc,ffin,cb);
    SeekToPosition(cb, cdOffset);
    ccc = cb->next;
    ffin = cb->end;

    /* Read signature */
    GetZU4(signature,ccc,ffin,cb);
    while (signature == 0x02014b50) {
        /* Read zip central directory using many output values */
        GetZU2(madeByVersion,ccc,ffin,cb);
        GetZU2(extractVersion,ccc,ffin,cb);
        GetZU2(bitFlags,ccc,ffin,cb);
        GetZU2(compressionMethod,ccc,ffin,cb);
        GetZU2(lastModTime,ccc,ffin,cb);
        GetZU2(lastModDate,ccc,ffin,cb);
        GetZU4(crc32,ccc,ffin,cb);
        GetZU4(compressedSize,ccc,ffin,cb);
        GetZU4(unCompressedSize,ccc,ffin,cb);
        GetZU2(fnameLen,ccc,ffin,cb);
        if (fnameLen >= MAX_FILE_NAME_SIZE) {
            fatalError(ERR_INTERNAL,"file name in .zip archive too long", XREF_EXIT_ERR);
        }
        GetZU2(extraLen,ccc,ffin,cb);
        GetZU2(fcommentLen,ccc,ffin,cb);
        GetZU2(diskNumber,ccc,ffin,cb);
        GetZU2(internFileAttribs,ccc,ffin,cb);
        GetZU4(externFileAttribs,ccc,ffin,cb);
        GetZU4(localHeaderOffset,ccc,ffin,cb);
        for(i=0; i<fnameLen; i++) {
            GetChar(fn[i],ccc,ffin,cb);
        }
        fn[i] = 0;
        log_trace("file '%s' in central dir", fn);
        SkipNChars(extraLen+fcommentLen,ccc,ffin,cb);

        if (strncmp(fn,"META-INF/",9)!=0) {
            log_trace("adding '%s'", fn);
            fsIsMember(&zip->dir, fn, localHeaderOffset, ADD_YES, &place);
        }
        /* Read next signature */
        GetZU4(signature,ccc,ffin,cb);
    }
 fini:
    *accc = ccc; *affin = ffin;
    return;

 endOfFile:
    errorMessage(ERR_ST, "unexpected end of file");
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
            goto forend;
        }
    }
 forend:;
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
        zipArchiveScan(&buffer->next,&buffer->end,buffer,&s_zipArchiveTable[archiveIndex], fst.st_size);
        closeFile(zipFile);
    }
    return(archiveIndex);
}

static bool zipSeekToFile(CharacterBuffer *cb, char *name) {
    char *sep;
    char *ccc, *ffin;
    int i, namelen;
    bool result = false;
    unsigned lastSig, fsize;
    char fn[MAX_FILE_NAME_SIZE];
    S_zipArchiveDir *place;

    sep = strchr(name,ZIP_SEPARATOR_CHAR);
    if (sep == NULL)
        return false;
    *sep = 0;
    namelen = strlen(name);
    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && s_zipArchiveTable[i].fn[0]!=0; i++) {
        if (strncmp(s_zipArchiveTable[i].fn, name,namelen)==0
            && s_zipArchiveTable[i].fn[namelen] == ZIP_SEPARATOR_CHAR) {
            break;
        }
    }
    *sep = ZIP_SEPARATOR_CHAR;
    if (i>=MAX_JAVA_ZIP_ARCHIVES || s_zipArchiveTable[i].fn[0]==0) {
        errorMessage(ERR_INTERNAL, "archive not indexed");
        goto fini;
    }
    if (fsIsMember(&s_zipArchiveTable[i].dir, sep+1,0, ADD_NO, &place)==0)
        goto fini;
    SeekToPosition(cb, place->u.offset);
    if (zipReadLocalFileHeader(&cb->next, &cb->end, cb, fn, &fsize,
                               &lastSig, s_zipArchiveTable[i].fn) == 0)
        goto fini;
    assert(lastSig == 0x04034b50);
    assert(strcmp(fn,sep+1)==0);
    result = true;
 fini:
    return result;
}

bool zipFindFile(char *name,
                 char **resName,             /* can be NULL !!! */
                 S_zipFileTableItem *zipfile
) {
    char *pp;
    char fname[MAX_FILE_NAME_SIZE];
    S_zipArchiveDir *place;

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
    S_zipFileTableItem *zipfile,
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
    S_zipArchiveDir *place, *aa;
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

static union constantPoolUnion *cfReadConstantPool(char **accc, char **affin,
                                                   CharacterBuffer *cb,
                                                   int *cpSize) {
    char *ccc, *ffin;
    int cval;
    int count,tag,ind,classind,nameind,typeind,strind;
    int size,i;
    union constantPoolUnion *cp=NULL;
    char *str;
    char tmpBuff[TMP_BUFF_SIZE];

    ccc = *accc; ffin = *affin;
    GetU2(count, ccc, ffin, cb);
    CF_ALLOCC(cp, count, union constantPoolUnion);
    //& memset(cp,0, count*sizeof(union constantPoolUnion));    // if broken file
    for(ind=1; ind<count; ind++) {
        GetU1(tag, ccc, ffin, cb);
        switch (tag) {
        case 0:
            GetChar(cval, ccc, ffin, cb);
            break;
        case CONSTANT_Asciz:
            GetU2(size, ccc, ffin, cb);
            CF_ALLOCC(str, size+1, char);
            for(i=0; i<size; i++) GetChar(str[i], ccc, ffin, cb);
            str[i]=0;
            cp[ind].asciz = str;
            break;
        case CONSTANT_Unicode:
            errorMessage(ERR_ST,"[cfReadConstantPool] Unicode not yet implemented");
            GetU2(size, ccc, ffin, cb);
            SkipNChars(size*2, ccc, ffin, cb);
            cp[ind].asciz = "Unicode ??";
            break;
        case CONSTANT_Class:
            GetU2(classind, ccc, ffin, cb);
            cp[ind].clas.nameIndex = classind;
            break;
        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
            GetU2(classind, ccc, ffin, cb);
            GetU2(nameind, ccc, ffin, cb);
            cp[ind].rec.classIndex = classind;
            cp[ind].rec.nameAndTypeIndex = nameind;
            break;
        case CONSTANT_NameandType:
            GetU2(nameind, ccc, ffin, cb);
            GetU2(typeind, ccc, ffin, cb);
            cp[ind].nt.nameIndex = nameind;
            cp[ind].nt.signatureIndex = typeind;
            break;
        case CONSTANT_String:
            GetU2(strind, ccc, ffin, cb);
            break;
        case CONSTANT_Integer:
        case CONSTANT_Float:
            GetU4(cval, ccc, ffin, cb);
            break;
        case CONSTANT_Long:
        case CONSTANT_Double:
            GetU4(cval, ccc, ffin, cb);
            ind ++;
            GetU4(cval, ccc, ffin, cb);
            break;
        default:
            sprintf(tmpBuff,"unknown tag %d in constant pool of %s",tag,currentFile.fileName);
            errorMessage(ERR_ST,tmpBuff);
        }
    }
    goto fin;
 endOfFile:
    errorMessage(ERR_ST,"unexpected end of file");
 fin:
    *accc = ccc; *affin = ffin; *cpSize = count;
    return(cp);
}

void javaHumanizeLinkName( char *inn, char *outn, int size) {
    int     i;
    char    *cut;
    cut = strchr(inn, LINK_NAME_SEPARATOR);
    if (cut==NULL) cut = inn;
    else cut ++;
    for(i=0; cut[i]; i++) {
        outn[i] = cut[i];
        //&     if (LANGUAGE(LANG_JAVA)) {
        if (outn[i]=='/') outn[i]='.';
        //&     }
        assert(i<size-1);
    }
    outn[i] = 0;
}

char * cfSkipFirstArgumentInSigString(char *sig) {
    char *ssig;
    ssig = sig;
    assert(ssig);
    /*fprintf(dumpOut, ": skipping first argument of %s\n",sig);*/
    if (*ssig != '(') return(ssig);
    ssig ++;
    assert(*ssig);
    switch (*ssig) {
    case ')':
        break;
    case '[':
        for(ssig++; *ssig && isdigit(*ssig); ssig++) ;
        break;
    case 'L':
        for(; *ssig && *ssig!=';'; ssig++) ;
        ssig++;
        break;
    default:
        ssig++;
    }
    return(ssig);
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

static void cfReadFieldInfos(   char **accc,
                                char **affin,
                                CharacterBuffer *iBuf,
                                Symbol *memb,
                                union constantPoolUnion *cp
                                ) {
    char *ccc, *ffin;
    int count, ind;
    int access_flags, nameind, sigind;
    ccc = *accc; ffin = *affin;
    GetU2(count, ccc, ffin, iBuf);
    for(ind=0; ind<count; ind++) {
        GetU2(access_flags, ccc, ffin, iBuf);
        GetU2(nameind, ccc, ffin, iBuf);
        GetU2(sigind, ccc, ffin, iBuf);
        /*
          fprintf(dumpOut, "field '%s' of type '%s'\n",
          cp[nameind].asciz,cp[sigind].asciz); fflush(dumpOut);
        */
        cfAddRecordToClass(cp[nameind].asciz,cp[sigind].asciz,memb,access_flags,
                           StorageField,NULL);
        SkipAttributes(ccc, ffin, iBuf);
    }
    goto fin;
 endOfFile:
    errorMessage(ERR_ST,"unexpected end of file");
 fin:
    *accc = ccc; *affin = ffin;
}

static char *simpleClassNameFromFQTName(char *fqtName) {
    char *res, *ss;
    res = fqtName;
    for(ss=fqtName; *ss; ss++) {
        if (*ss=='/' || *ss=='$') res = ss+1;
    }
    return(res);
}

static void cfReadMethodInfos(  char **accc,
                                char **affin,
                                CharacterBuffer *iBuf,
                                Symbol *memb,
                                union constantPoolUnion *cp
                                ) {
    char *ccc, *ffin, *name, *sign, *sign2;
    unsigned count, ind;
    unsigned aind, acount, aname, alen, excount;
    int i, access_flags, nameind, sigind, exclass;
    Storage storage;
    char *exname, *exsname;
    Symbol *exc;
    SymbolList *exclist, *ee;

    ccc = *accc; ffin = *affin;
    GetU2(count, ccc, ffin, iBuf);
    for(ind=0; ind<count; ind++) {
        GetU2(access_flags, ccc, ffin, iBuf);
        GetU2(nameind, ccc, ffin, iBuf);
        GetU2(sigind, ccc, ffin, iBuf);
        /*
          fprintf(dumpOut, "method '%s' of type '%s'\n",
          cp[nameind].asciz,cp[sigind].asciz); fflush(dumpOut);
        */
        // TODO more efficiently , just index checking
        name = cp[nameind].asciz;
        sign = cp[sigind].asciz;
        storage = StorageMethod;
        exclist = NULL;
        if (strcmp(name, JAVA_CONSTRUCTOR_NAME1)==0
            || strcmp(name, JAVA_CONSTRUCTOR_NAME2)==0) {
            // isn't it yet done by javac?
            //&if (strcmp(name, JAVA_CONSTRUCTOR_NAME2)==0) {
            //& access_flags |= AccessStatic;
            //&}
            // if constructor, put there type name as constructor name
            // instead of <init>
            log_trace("constructor %s of %s", name, memb->name);
            assert(memb && memb->bits.symType==TypeStruct && memb->u.s);
            name = memb->name;
            storage = StorageConstructor;
            if (fileTable.tab[memb->u.s->classFile]->directEnclosingInstance != noFileIndex) {
                // the first argument is direct enclosing instance, remove it
                sign2 = cfSkipFirstArgumentInSigString(sign);
                CF_ALLOCC(sign, strlen(sign2)+2, char);
                sign[0] = '(';  strcpy(sign+1, sign2);
                log_trace("it is nested constructor %s %s", name, sign);
            }
        } else if (name[0] == '<') {
            // still some strange constructor name?
            log_trace("strange constructor %s %s", name, sign);
            storage = StorageConstructor;
        }
        GetU2(acount, ccc, ffin, iBuf);
        for(aind=0; aind<acount; aind++) {
            GetU2(aname, ccc, ffin, iBuf);
            GetU4(alen, ccc, ffin, iBuf);
            // berk, really I need to compare strings?
            if (strcmp(cp[aname].asciz, "Exceptions")==0) {
                GetU2(excount, ccc, ffin, iBuf);
                for(i=0; i<excount; i++) {
                    GetU2(exclass, ccc, ffin, iBuf);
                    exname = cp[cp[exclass].clas.nameIndex].asciz;
                    //&fprintf(dumpOut,"throws %s\n", exname);fflush(dumpOut);
                    exsname = simpleClassNameFromFQTName(exname);
                    exc = javaFQTypeSymbolDefinition(exsname, exname);
                    CF_ALLOC(ee, SymbolList);
                    /* REPLACED: FILL_symbolList(ee, exc, exclist); with compound literal */
                    *ee = (SymbolList){.d = exc, .next = exclist};
                    exclist = ee;
                }
            } else if (1 && strcmp(cp[aname].asciz, "Code")==0) {
                // forget this, it is useless as .jar usually do not cantain
                // informations about variable names.
                unsigned max_stack, max_locals, code_length;
                unsigned exception_table_length,attributes_count;
                unsigned caname, calen;
                int ii;
                // look here for local variable names
                GetU2(max_stack, ccc, ffin, iBuf);
                GetU2(max_locals, ccc, ffin, iBuf);
                GetU4(code_length, ccc, ffin, iBuf);
                SkipNChars(code_length, ccc, ffin, iBuf);
                GetU2(exception_table_length, ccc, ffin, iBuf);
                SkipNChars((exception_table_length*8), ccc, ffin, iBuf);
                GetU2(attributes_count, ccc, ffin, iBuf);
                for(ii=0; ii<attributes_count; ii++) {
                    int iii;
                    GetU2(caname, ccc, ffin, iBuf);
                    GetU4(calen, ccc, ffin, iBuf);
                    if (strcmp(cp[caname].asciz, "LocalVariableTable")==0) {
                        unsigned local_variable_table_length;
                        unsigned start_pc,length,name_index,descriptor_index;
                        unsigned index;
                        GetU2(local_variable_table_length, ccc, ffin, iBuf);
                        for(iii=0; iii<local_variable_table_length; iii++) {
                            GetU2(start_pc, ccc, ffin, iBuf);
                            GetU2(length, ccc, ffin, iBuf);
                            GetU2(name_index, ccc, ffin, iBuf);
                            GetU2(descriptor_index, ccc, ffin, iBuf);
                            GetU2(index, ccc, ffin, iBuf);
                            //&fprintf(dumpOut,"local variable %s, index == %d\n", cp[name_index].asciz, index);fflush(dumpOut);
                        }
                    } else {
                        SkipNChars(calen, ccc, ffin, iBuf);
                    }
                }
            } else {
                //&fprintf(dumpOut, "skipping %s\n", cp[aname].asciz);
                SkipNChars(alen, ccc, ffin, iBuf);
            }
        }
        cfAddRecordToClass(name, sign, memb, access_flags, storage, exclist);
    }
    goto fin;
 endOfFile:
    errorMessage(ERR_ST,"unexpected end of file");
 fin:
    *accc = ccc; *affin = ffin;
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
                                    int loadSuper) {
    Symbol *s;

    s = javaGetFieldClass(super, NULL);
    if (loadSuper == LOAD_SUPER)
        javaLoadClassSymbolsFromFile(s);
    addSuperClassOrInterface(member, s, origin);
}

int javaCreateClassFileItem( Symbol *memb) {
    char ftname[MAX_FILE_NAME_SIZE];
    int fileIndex;

    SPRINT_FILE_TAB_CLASS_NAME(ftname, memb->linkName);
    fileIndex = addFileTabItem(ftname);
    memb->u.s->classFile = fileIndex;

    return fileIndex;
}

/* ********************************************************************* */

void javaReadClassFile(char *className, Symbol *symbol, int loadSuper) {
    int readValue, i, inum, innval, rinners, modifs;
    FILE *zipFile;
    int upp, innNameInd;
    bool membFlag;
    char *cb_next, *cb_end;
    char *super, *interf, *innerCName, *zipSeparatorIndex;
    Symbol *inners;
    int thisClass, superClass, access, cpSize;
    int fileIndex, ind, count, aname, alen, cn;
    char *inner, *upper, *thisClassName;
    union constantPoolUnion *constantPool;
    Position pos;
    char tmpBuff[TMP_BUFF_SIZE];
    int major, minor;           /* Version numbers of class file format */
    CharacterBuffer *cb;

    ENTER();
    symbol->bits.javaFileIsLoaded = 1;
    /*&
      fprintf(dumpOut,": ppmmem == %d/%d\n",ppmMemoryi,SIZE_ppmMemory);
      fprintf(dumpOut,":reading file %s arg class == %s == %s\n",
      className, symbol->name, symbol->linkName); fflush(dumpOut);
      &*/

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
    cb_next = cb->next;
    cb_end = cb->end;
    GetU4(readValue, cb_next, cb_end, cb);
    log_trace("magic is %x", readValue);
    if (readValue != 0xcafebabe) {
        sprintf(tmpBuff,"%s is not a valid class file", className);
        errorMessage(ERR_ST, tmpBuff);
        goto finish;
    }
    GetU2(minor, cb_next, cb_end, cb);
    GetU2(major, cb_next, cb_end, cb);
    log_trace("version of '%s' is %d.%d", className, major, minor);
    constantPool = cfReadConstantPool(&cb_next, &cb_end, cb, &cpSize);
    GetU2(access, cb_next, cb_end, cb);
    symbol->bits.access = access;
    log_trace("reading accessFlags %s == %x", className, access);
    if (access & AccessInterface) fileTable.tab[fileIndex]->b.isInterface = true;
    GetU2(thisClass, cb_next, cb_end, cb);
    if (thisClass<0 || thisClass>=cpSize) goto corrupted;
    thisClassName = constantPool[constantPool[thisClass].clas.nameIndex].asciz;
    // TODO!!!, it may happen that name of class differ in cases from name of file,
    // what to do in such case? abandon with an error?
    log_trace("this class == %s", thisClassName);
    GetU2(superClass, cb_next, cb_end, cb);
    if (superClass != 0) {
        if (superClass<0 || superClass>=cpSize) goto corrupted;
        super = constantPool[constantPool[superClass].clas.nameIndex].asciz;
        addSuperClassOrInterfaceByName(symbol, super, symbol->u.s->classFile,
                                       loadSuper);
    }

    GetU2(inum, cb_next, cb_end, cb);
    /* implemented interfaces */

    for(i=0; i<inum; i++) {
        GetU2(readValue, cb_next, cb_end, cb);
        if (readValue != 0) {
            if (readValue<0 || readValue>=cpSize) goto corrupted;
            interf = constantPool[constantPool[readValue].clas.nameIndex].asciz;
            addSuperClassOrInterfaceByName(symbol, interf, symbol->u.s->classFile,
                                           loadSuper);
        }
    }
    //& addSubClassesItemsToFileTab(symbol, symbol->u.s->classFile);

    cfReadFieldInfos(&cb_next, &cb_end, cb, symbol, constantPool);
    if (currentFile.lexBuffer.buffer.isAtEOF) goto endOfFile;
    cfReadMethodInfos(&cb_next, &cb_end, cb, symbol, constantPool);
    if (currentFile.lexBuffer.buffer.isAtEOF) goto endOfFile;

    GetU2(count, cb_next, cb_end, cb);
    for(ind=0; ind<count; ind++) {
        GetU2(aname, cb_next, cb_end, cb);
        GetU4(alen, cb_next, cb_end, cb);
        if (strcmp(constantPool[aname].asciz,"InnerClasses")==0) {
            GetU2(inum, cb_next, cb_end, cb);
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
                GetU2(innval, cb_next, cb_end, cb);
                inner = constantPool[constantPool[innval].clas.nameIndex].asciz;
                //&fprintf(dumpOut,"inner %s \n",inner);fflush(dumpOut);
                GetU2(upp, cb_next, cb_end, cb);
                GetU2(innNameInd, cb_next, cb_end, cb);
                if (innNameInd==0) innerCName = "";         // !!!!!!!! hack
                else innerCName = constantPool[innNameInd].asciz;
                //&fprintf(dumpOut,"class name %x='%s'\n",innerCName,innerCName);fflush(dumpOut);
                inners = javaFQTypeSymbolDefinition(innerCName, inner);
                membFlag = false;
                if (upp != 0) {
                    upper = constantPool[constantPool[upp].clas.nameIndex].asciz;
                    //&{fprintf(dumpOut,"upper %s encloses %s\n",upper, inner);fflush(dumpOut);}
                    if (strcmp(upper,thisClassName)==0) {
                        membFlag = true;
                        /*&fprintf(dumpOut,"set as member class \n"); fflush(dumpOut);&*/
                    }
                }
                GetU2(modifs, cb_next, cb_end, cb);
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
            SkipNChars(alen, cb_next, cb_end, cb);
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
    //&{fprintf(dumpOut,": closing file %s\n",className);fflush(dumpOut);fprintf(dumpOut,": ppmmem == %d/%d\n",ppmMemoryi,SIZE_ppmMemory);fflush(dumpOut);}
    popInclude();

    LEAVE();
}
