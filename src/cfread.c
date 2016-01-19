/*
	$Revision: 1.10 $
	$Date: 2002/09/14 17:27:02 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/

/* *********************** read JAVA class file ************************* */

struct javaCpClassInfo {
	short unsigned	nameIndex;
};
struct javaCpNameAndTypeInfo {
	short unsigned 	nameIndex;
	short unsigned 	signatureIndex;
};
struct javaCpRecordInfo {
	short unsigned	classIndex;
	short unsigned	nameAndTypeIndex;
};

union constantPoolUnion {
	char							*asciz;
	struct javaCpClassInfo 			clas;
	struct javaCpNameAndTypeInfo	nt;
	struct javaCpRecordInfo			rec;
};

/* *********************************************************************** */

#define GetChar(cch, ccc, ffin, bbb) {\
	if (ccc >= ffin) {\
		(bbb)->cc = ccc;\
		if ((bbb)->isAtEOF || getCharBuf(bbb) == 0) {\
			cch = -1;\
			(bbb)->isAtEOF = 1;\
            goto endOfFile;\
		} else {\
			ccc = (bbb)->cc; ffin = (bbb)->fin;\
			cch = * ((unsigned char*)ccc); ccc ++;\
		}\
	} else {\
		cch = * ((unsigned char*)ccc); ccc++;\
	}\
/*fprintf(dumpOut,"getting char *%x < %x == '0x%x'\n",ccc,ffin,cch);fflush(dumpOut);*/\
}

#define UngetChar(cch, ccc, ffin, bbb) {*--ccc = cch;}


#define GetU1(val, ccc, ffin, bbb) GetChar(val, ccc, ffin, bbb)

#define GetU2(val, ccc, ffin, bbb) {\
	register int chh;\
	GetChar(val, ccc, ffin, bbb);\
	GetChar(chh, ccc, ffin, bbb);\
	val = val*256+chh;\
}

#define GetU4(val, ccc, ffin, bbb) {\
	register int chh;\
	GetChar(val, ccc, ffin, bbb);\
	GetChar(chh, ccc, ffin, bbb);\
	val = val*256+chh;\
	GetChar(chh, ccc, ffin, bbb);\
	val = val*256+chh;\
	GetChar(chh, ccc, ffin, bbb);\
	val = val*256+chh;\
}

#define GetZU2(val, ccc, ffin, bbb) {\
	register unsigned chh;\
	GetChar(val, ccc, ffin, bbb);\
	GetChar(chh, ccc, ffin, bbb);\
	val = val+(chh<<8);\
}

#define GetZU4(val, ccc, ffin, bbb) {\
	register unsigned chh;\
	GetChar(val, ccc, ffin, bbb);\
	GetChar(chh, ccc, ffin, bbb);\
	val = val+(chh<<8);\
	GetChar(chh, ccc, ffin, bbb);\
	val = val+(chh<<16);\
	GetChar(chh, ccc, ffin, bbb);\
	val = val+(chh<<24);\
}

#define SkipNChars(count, ccc, ffin, bbb) {\
	register int ccount,i,ch;\
	ccount = (count);\
	if (ccc + ccount < ffin) ccc += ccount;\
	else {\
		(bbb)->cc = ccc;\
		skipNCharsInCharBuf(bbb,(count));\
		ccc = (bbb)->cc; ffin = (bbb)->fin;\
	}\
}

#define GetCurrentFileOffset(ccc,ffin,bbb,offset) {\
	offset = ftell((bbb)->ff) - (ffin-ccc);\
}

#define SeekToPosition(ccc,ffin,bbb,offset) {\
	fseek((bbb)->ff,offset,SEEK_SET);\
	ccc = ffin = (bbb)->cc = (bbb)->fin = (bbb)->lineBegin = (bbb)->a;\
}

#define SkipAttributes(ccc, ffin, iBuf) {\
	register int ind,count,aname,alen;\
	GetU2(count, ccc, ffin, iBuf);\
	for(ind=0; ind<count; ind++) {\
		GetU2(aname, ccc, ffin, iBuf);\
		GetU4(alen, ccc, ffin, iBuf);\
		SkipNChars(alen, ccc, ffin, iBuf);\
	}\
}

/* *************** first something to read zip-files ************** */

struct zipIndexItem {
	unsigned offset;
	char name;				/* array of char	*/
};

static int zipReadLocalFileHeader(char **accc, char **affin, S_charBuf *iBuf,
								char *fn, unsigned *fsize, unsigned *lastSig,
								char *archivename, unsigned fileOffset) {
	int res;
	register char *ccc, *ffin;
	register int i,offset;
	int headSig,extractVersion,bitFlags,compressionMethod;
	int lastModTime,lastModDate,fnameLen,extraLen;
	unsigned crc32,compressedSize,unCompressedSize;
	static int compressionErrorWritten=0;
	char	*zzz, ttt[MAX_FILE_NAME_SIZE];
	res = 1;
	ccc = *accc; ffin = *affin;
	GetZU4(headSig,ccc,ffin,iBuf);
//&fprintf(dumpOut,"signature is %x\n",headSig); fflush(dumpOut);
	*lastSig = headSig;
	if (headSig != 0x04034b50) {
		static int messagePrinted = 0;
		if (messagePrinted==0) {
			messagePrinted = 1;
			sprintf(tmpBuff,
				"archiv %s is corrupted or modified while xref task running",
				archivename);
			error(ERR_ST, tmpBuff);
			if (s_opt.taskRegime == RegimeEditServer) {
				fprintf(errOut,"\t\tplease, kill xref process and retry.\n");
			}
		}
		res = 0;
		goto fin;
	}
	GetZU2(extractVersion,ccc,ffin,iBuf);
	GetZU2(bitFlags,ccc,ffin,iBuf);
	GetZU2(compressionMethod,ccc,ffin,iBuf);
	GetZU2(lastModTime,ccc,ffin,iBuf);
	GetZU2(lastModDate,ccc,ffin,iBuf);
	GetZU4(crc32,ccc,ffin,iBuf);
	GetZU4(compressedSize,ccc,ffin,iBuf);
	*fsize = compressedSize;
	GetZU4(unCompressedSize,ccc,ffin,iBuf);
	GetZU2(fnameLen,ccc,ffin,iBuf);
	if (fnameLen >= MAX_FILE_NAME_SIZE) {
		fatalError(ERR_INTERNAL,"file name too long", XREF_EXIT_ERR);
	}
	GetZU2(extraLen,ccc,ffin,iBuf);
	for(i=0; i<fnameLen; i++) {
		GetChar(fn[i],ccc,ffin,iBuf);
	}
	fn[i] = 0;
	SkipNChars(extraLen,ccc,ffin,iBuf);
	if (compressionMethod == 0) {
	}
#if defined(USE_LIBZ)		/*SBD*/
	else if (compressionMethod == Z_DEFLATED) {
		iBuf->cc = ccc;
		iBuf->fin = ffin;
		switchToZippedCharBuff(iBuf);
		ccc = iBuf->cc;
		ffin = iBuf->fin;
	} 
#endif						/*SBD*/
	else {
		res = 0;
		if (compressionErrorWritten==0) {
			assert(s_opt.taskRegime);
			// why the message was only for editserver?
			//&if (s_opt.taskRegime==RegimeEditServer) {
				strcpy(ttt, archivename);
				zzz = strchr(ttt, ZIP_SEPARATOR_CHAR);
				if (zzz!=NULL) *zzz = 0;
				sprintf(tmpBuff,"\n\tfiles in %s are compressed by unknown method #%d\n", ttt, compressionMethod);
				sprintf(tmpBuff+strlen(tmpBuff),
					"\tRun 'jar2jar0 %s' command to uncompress them.\n", ttt);
				assert(strlen(tmpBuff)+1 < TMP_BUFF_SIZE);
				error(ERR_ST,tmpBuff);
				//&}
			compressionErrorWritten = 1;
		}
	}
	goto fin;
endOfFile:
	error(ERR_ST,"unexpected end of file");
fin:
	*accc = ccc; *affin = ffin;
	return(res);
}

#define ReadZipCDRecord(ccc,ffin,iBuf) {\
	/* using many output values */ \
	GetZU4(headSig,ccc,ffin,iBuf);\
	if (headSig != 0x02014b50) goto endcd;\
	GetZU2(madeByVersion,ccc,ffin,iBuf);\
	GetZU2(extractVersion,ccc,ffin,iBuf);\
	GetZU2(bitFlags,ccc,ffin,iBuf);\
	GetZU2(compressionMethod,ccc,ffin,iBuf);\
	GetZU2(lastModTime,ccc,ffin,iBuf);\
	GetZU2(lastModDate,ccc,ffin,iBuf);\
	GetZU4(crc32,ccc,ffin,iBuf);\
	GetZU4(compressedSize,ccc,ffin,iBuf);\
	GetZU4(unCompressedSize,ccc,ffin,iBuf);\
	GetZU2(fnameLen,ccc,ffin,iBuf);\
	if (fnameLen >= MAX_FILE_NAME_SIZE) {\
		fatalError(ERR_INTERNAL,"file name in .zip archive too long", XREF_EXIT_ERR);\
	}\
	GetZU2(extraLen,ccc,ffin,iBuf);\
	GetZU2(fcommentLen,ccc,ffin,iBuf);\
	GetZU2(diskNumber,ccc,ffin,iBuf);\
	GetZU2(internFileAttribs,ccc,ffin,iBuf);\
	GetZU4(externFileAttribs,ccc,ffin,iBuf);\
	GetZU4(localHeaderOffset,ccc,ffin,iBuf);\
	for(i=0; i<fnameLen; i++) {\
		GetChar(fn[i],ccc,ffin,iBuf);\
	}\
	fn[i] = 0;\
/*&fprintf(dumpOut,"file %s in central dir\n",fn); fflush(dumpOut);&*/\
	SkipNChars(extraLen+fcommentLen,ccc,ffin,iBuf);\
}


static int zipFindFileInCd(char **accc, char **affin, S_charBuf *iBuf, 
							char *fname, unsigned *foffset
							) {
	char fn[MAX_FILE_NAME_SIZE];
	int headSig,madeByVersion,extractVersion,bitFlags,compressionMethod;
	int lastModTime,lastModDate,fnameLen,extraLen,fcommentLen,diskNumber;
	int i,internFileAttribs;
	unsigned externFileAttribs,localHeaderOffset;
	unsigned crc32,compressedSize,unCompressedSize;

	char *ccc, *ffin;
	unsigned fsize, lastSig;
	int res = 0;
	unsigned offset;
	ccc = *accc; ffin = *affin;
	for(;;) {
		ReadZipCDRecord(ccc,ffin,iBuf);
		if (strcmp(fn,fname)==0) {
			// file found
			*foffset = localHeaderOffset;
			res = 1;
			goto fin;
		}
	} endcd:
	assert(headSig == 0x06054b50 || headSig == 0x02014b50);
	goto fin;
endOfFile:
	error(ERR_ST,"unexpected end of file");
fin:
	*accc = ccc; *affin = ffin;	
	return(res);
}

static S_charBuf s_zipTmpBuff;

int fsIsMember(S_zipArchiveDir **dir, char *fn, unsigned offset, 
						int addFlag, S_zipArchiveDir **place) {
	S_zipArchiveDir 	*aa, **aaa, *p;
	int					itemlen, res;
	char				*ss;
	if (dir == NULL) return(0);
	*place = *dir;
	res = 1;
	if (fn[0] == 0) {
		error(ERR_INTERNAL,"looking for empty file name in 'fsdir'");
		return(0);
	}
	if (fn[0]=='/' && fn[1]==0) {
		error(ERR_INTERNAL,"looking for root in 'fsdir'");
		return(0);	/* should not arrive */
	}
lastrecLabel:
	ss = strchr(fn,'/');
	if (ss == NULL) {
		itemlen = strlen(fn);
		if (itemlen == 0) return(res);	/* directory */
	} else {
		itemlen = (ss-fn) + 1;
	}
	for(aaa=dir, aa= *aaa; aa!=NULL; aaa= &(aa->next), aa = *aaa) {
/*&fprintf(dumpOut,"comparing %s <-> %s of len %d\n", fn, aa->name, itemlen);fflush(dumpOut);&*/
		if (strncmp(fn,aa->name,itemlen)==0 && aa->name[itemlen]==0) break;
	}
	assert(itemlen > 0);
	if (aa==NULL) {
		res = 0;
		if (addFlag == ADD_YES) {
			XX_ALLOCC(ss, sizeof(S_zipArchiveDir)+itemlen+1, char);
			p = (S_zipArchiveDir*)ss;
			FILL_zipArchiveDir(p,sub,NULL,NULL);
			strncpy(p->name, fn, itemlen);
			p->name[itemlen]=0;
/*&fprintf(dumpOut,"adding new item\n", p->name);fflush(dumpOut);&*/
			if (fn[itemlen-1] == '/') {			/* directory */
				p->u.sub = NULL;
			} else {
				p->u.offset = offset;
			}
			*aaa = aa = p;
		} else {
			return(0);
		}
	}
	*place = aa;
	if (fn[itemlen-1] == '/') {
		dir = &(aa->u.sub);
		fn = fn+itemlen;
		goto lastrecLabel;
	} else {
		return(res);					/* yet in the table */
	}
}

void fsRecMapOnFiles(S_zipArchiveDir *dir, char *zip, char *path, void (*fun)(char *zip, char *file, void *arg), void *arg) {
	S_zipArchiveDir 	*aa;
	char 				*fn;
	char				npath[MAX_FILE_NAME_SIZE];
	int					len;
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
								S_charBuf *iBuf, int fsize) {
	int offset,res;
	char *ccc, *ffin;
	res = 1;
	if (fsize < CHAR_BUFF_SIZE) offset = 0;
	else offset = fsize-(CHAR_BUFF_SIZE-MAX_UNGET_CHARS);
	fseek(iBuf->ff, offset, SEEK_SET);
	iBuf->cc = iBuf->fin;
	getCharBuf(iBuf);
	ccc = ffin = iBuf->fin;
	for (ccc-=4; ccc>iBuf->a && strncmp(ccc,"\120\113\005\006",4)!=0; ccc--) {
	}
	if (ccc <= iBuf->a) {
		res = 0;
		assert(s_opt.taskRegime);
		if (s_opt.taskRegime!=RegimeEditServer) {
			warning(ERR_INTERNAL,"can't find end of central dir in archive");
		}
		goto fini;
	}
fini:
	*accc = ccc; *affin = ffin;	
	return(res);
}

static void zipArchiveScan(char **accc, char **affin, S_charBuf *iBuf,
								S_zipFileTabItem *zip, int fsize) {
	char fn[MAX_FILE_NAME_SIZE];
	char *ccc, *ffin;
	S_zipArchiveDir *place;
	int headSig,madeByVersion,extractVersion,bitFlags,compressionMethod;
	int lastModTime,lastModDate,fnameLen,extraLen,fcommentLen,diskNumber;
	int i,internFileAttribs,tmp;
	unsigned externFileAttribs,localHeaderOffset, cdOffset;
	unsigned crc32,compressedSize,unCompressedSize;
	unsigned foffset;
	ccc = *accc; ffin = *affin;
	zip->dir = NULL;
	if (findEndOfCentralDirectory(&ccc, &ffin, iBuf, fsize)==0) goto fini;
	GetZU4(headSig,ccc,ffin,iBuf);
	assert(headSig == 0x06054b50);
	GetZU2(tmp,ccc,ffin,iBuf);
	GetZU2(tmp,ccc,ffin,iBuf);
	GetZU2(tmp,ccc,ffin,iBuf);
	GetZU2(tmp,ccc,ffin,iBuf);
	GetZU4(tmp,ccc,ffin,iBuf);
	GetZU4(cdOffset,ccc,ffin,iBuf);
	SeekToPosition(ccc,ffin,iBuf,cdOffset);
	for(;;) {
		GetCurrentFileOffset(ccc,ffin,iBuf, foffset);
		ReadZipCDRecord(ccc,ffin,iBuf);
		if (strncmp(fn,"META-INF/",9)!=0) {
//&fprintf(dumpOut,"adding %s\n",fn);fflush(dumpOut);
			fsIsMember(&zip->dir, fn, localHeaderOffset, ADD_YES, &place);
		}
	} endcd:;
	goto fini;
endOfFile:
	error(ERR_ST,"unexpected end of file");
fini:
	*accc = ccc; *affin = ffin;	
}

#if 0
static void zipArchiveCdOffset(char **accc, char **affin, S_charBuf *iBuf,
								unsigned *offset, char *archivename) {
	char fn[MAX_FILE_NAME_SIZE];
	char *ccc, *ffin;
	int headSig,madeByVersion,extractVersion,bitFlags,compressionMethod;
	int lastModTime,lastModDate,fnameLen,extraLen,fcommentLen,diskNumber;
	int i,internFileAttribs;
	unsigned externFileAttribs,localHeaderOffset;
	unsigned crc32,compressedSize,unCompressedSize;
	unsigned fsize, lastSig;
	ccc = *accc; ffin = *affin;
	for(;;) {
		GetCurrentFileOffset(ccc,ffin,iBuf, (*offset));
		zipReadLocalFileHeader(&ccc, &ffin, iBuf, fn, &fsize, &lastSig, 
								archivename);
		if (lastSig != 0x04034b50) break;
//&fprintf(dumpOut,"skipping %d\n",compressedSize);fflush(dumpOut);
		SkipNChars(fsize,ccc,ffin,iBuf);
	}
	*accc = ccc; *affin = ffin;	
}
#endif

int zipIndexArchive(char *name) {
	int 		archi,namelen,fsize;
	FILE 		*ff;
	S_charBuf 	*bbb;
	struct stat	fst;
	bbb = &s_zipTmpBuff;
	namelen = strlen(name);
	for(archi=0; 
		archi<MAX_JAVA_ZIP_ARCHIVES && s_zipArchivTab[archi].fn[0]!=0; 
		archi++) {
		if (strncmp(s_zipArchivTab[archi].fn,name,namelen)==0
				&& s_zipArchivTab[archi].fn[namelen]==ZIP_SEPARATOR_CHAR) {
			goto forend;
		}
	} 
 forend:;
	if (archi<MAX_JAVA_ZIP_ARCHIVES && s_zipArchivTab[archi].fn[0] == 0) {
		// new file into the table 
		DPRINTF2("adding %s into index \n",name);
		if (stat(name ,&fst)!=0) {
			assert(s_opt.taskRegime);
			if (s_opt.taskRegime!=RegimeEditServer) {
				static int singleOut=0;
				if (singleOut==0) warning(ERR_CANT_OPEN, name);
				singleOut=1;
			}
			return(-1);
		}
#if defined (__WIN32__) || defined (__OS2__)				/*SBD*/
		ff = fopen(name,"rb");
#else														/*SBD*/
		ff = fopen(name,"r");
#endif														/*SBD*/
		if (ff == NULL) {
			assert(s_opt.taskRegime);
			if (s_opt.taskRegime!=RegimeEditServer) {
				warning(ERR_CANT_OPEN, name);
			}
			return(-1);
		}
		FILL_charBuf(bbb, bbb->a, bbb->a, ff, 0, 0, 0, bbb->a, 0, 0,INPUT_DIRECT,s_defaultZStream);
		assert(namelen+2 < MAX_FILE_NAME_SIZE);
		strcpy(s_zipArchivTab[archi].fn, name);
		s_zipArchivTab[archi].fn[namelen] = ZIP_SEPARATOR_CHAR;
		s_zipArchivTab[archi].fn[namelen+1] = 0;
		FILL_zipFileTabItem(&s_zipArchivTab[archi], fst, NULL);
		zipArchiveScan(&bbb->cc,&bbb->fin,bbb,&s_zipArchivTab[archi], fst.st_size);
		fclose(ff);
	}
	return(archi);
}

static int zipSeekToFile(char **accc, char **affin, S_charBuf *iBuf,
						char *name
						) {
	char *sep;
	char *ccc, *ffin;
	int i,namelen,res = 0;
	unsigned lastSig,fsize,lhoffset;
	char fn[MAX_FILE_NAME_SIZE];
	S_zipArchiveDir *place;
	ccc = *accc; ffin = *affin;
	sep = strchr(name,ZIP_SEPARATOR_CHAR);
	if (sep == NULL) {return(0);}
	*sep = 0;
	namelen = strlen(name);
	for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && s_zipArchivTab[i].fn[0]!=0; i++) {
		if (strncmp(s_zipArchivTab[i].fn, name,namelen)==0
				&& s_zipArchivTab[i].fn[namelen] == ZIP_SEPARATOR_CHAR) {
			break;
		}
	}
	*sep = ZIP_SEPARATOR_CHAR;
	if (i>=MAX_JAVA_ZIP_ARCHIVES || s_zipArchivTab[i].fn[0]==0) {
		error(ERR_INTERNAL, "archive not indexed\n");
		goto fini;
	}
	if (fsIsMember(&s_zipArchivTab[i].dir,sep+1,0,ADD_NO,&place)==0) goto fini;
	SeekToPosition(ccc,ffin,iBuf,place->u.offset);
	if (zipReadLocalFileHeader(&ccc, &ffin, iBuf, fn, &fsize, 
					&lastSig, s_zipArchivTab[i].fn, place->u.offset) == 0) goto fini;
	assert(lastSig == 0x04034b50);
	assert(strcmp(fn,sep+1)==0);
	res = 1;
fini:
	*accc = ccc; *affin = ffin;	
	return(res);
}

int zipFindFile(char *name,
				char **resName,				/* can be NULL !!! */
				S_zipFileTabItem *zipfile
				) {
	char			*pp,*ccc,*ffin;
	char 			fname[MAX_FILE_NAME_SIZE];
	S_charBuf 		*bbb;
	S_zipArchiveDir	*place;
	bbb = &s_zipTmpBuff;
	strcpy(fname,name);
	strcat(fname,".class");
	InternalCheck(strlen(fname)+1 < MAX_FILE_NAME_SIZE);
/*&fprintf(dumpOut,"looking for file %s in %s\n",fname,zipfile->fn);fflush(dumpOut);&*/
	if (fsIsMember(&zipfile->dir,fname,0,ADD_NO,&place)==0) return(0);
	if (resName != NULL) {
		XX_ALLOCC(*resName, strlen(zipfile->fn)+strlen(fname)+2, char);
		pp = strmcpy(*resName, zipfile->fn);
		strcpy(pp, fname);
	}
	return(1);
}

void javaMapZipDirFile(
		S_zipFileTabItem *zipfile,
		char *packfile,
		S_completions *a1,
		void *a2,
		int *a3,
		void (*fun)(MAP_FUN_PROFILE),
		char *classPath,
		char *dirname
		) {
	char			fn[MAX_FILE_NAME_SIZE];
	char			dirn[MAX_FILE_NAME_SIZE];
	S_zipArchiveDir	*place,*aa;
	char			*pp,*ccc,*ffin,*tmpp,*ss;
	int				dirlen;

	dirlen = strlen(dirname);
	strcpy(dirn,dirname);
	dirn[dirlen]='/';	dirn[dirlen+1]=0;
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
			error(ERR_INTERNAL,"empty name in .zip directory structure");
			continue;
		}
		if (*(ss-1) == '/') *(ss-1) = 0;
/*&fprintf(dumpOut,":mapping %s of %s pack %s\n",fn,dirname,packfile);fflush(dumpOut);&*/
		(*fun)(fn, classPath, packfile, a1, a2, a3);
	}
}


/* **************************************************************** */

static union constantPoolUnion * cfReadConstantPool(
						char **accc, char **affin, S_charBuf *iBuf,
						int *cpSize
						) {
	register char *ccc, *ffin;
	register int cval;
	int count,tag,ind,classind,nameind,typeind,strind;
	int size,i;
	union constantPoolUnion *cp=NULL;
	char *str;
	ccc = *accc; ffin = *affin;
	GetU2(count, ccc, ffin, iBuf);
	CF_ALLOCC(cp, count, union constantPoolUnion);
//&	memset(cp,0, count*sizeof(union constantPoolUnion));	// if broken file
	for(ind=1; ind<count; ind++) {
		GetU1(tag, ccc, ffin, iBuf);
		switch (tag) {
		case 0:
			GetChar(cval, ccc, ffin, iBuf);
			break;
		case CONSTANT_Asciz:
			GetU2(size, ccc, ffin, iBuf);
			CF_ALLOCC(str, size+1, char);
			for(i=0; i<size; i++) GetChar(str[i], ccc, ffin, iBuf);
			str[i]=0;
			cp[ind].asciz = str;
			break;
		case CONSTANT_Unicode:
			error(ERR_ST,"[cfReadConstantPool] Unicode not yet implemented");
			GetU2(size, ccc, ffin, iBuf);
			SkipNChars(size*2, ccc, ffin, iBuf);
			cp[ind].asciz = "Unicode ??";
			break;
		case CONSTANT_Class:
			GetU2(classind, ccc, ffin, iBuf);
			cp[ind].clas.nameIndex = classind;
			break;
		case CONSTANT_Fieldref:
		case CONSTANT_Methodref:
		case CONSTANT_InterfaceMethodref:
			GetU2(classind, ccc, ffin, iBuf);
			GetU2(nameind, ccc, ffin, iBuf);
			cp[ind].rec.classIndex = classind;
			cp[ind].rec.nameAndTypeIndex = nameind;
			break;
		case CONSTANT_NameandType:
			GetU2(nameind, ccc, ffin, iBuf);
			GetU2(typeind, ccc, ffin, iBuf);
			cp[ind].nt.nameIndex = nameind;
			cp[ind].nt.signatureIndex = typeind;
			break;
		case CONSTANT_String:
			GetU2(strind, ccc, ffin, iBuf);
			break;
		case CONSTANT_Integer:
		case CONSTANT_Float:
			GetU4(cval, ccc, ffin, iBuf);
			break;
		case CONSTANT_Long:
		case CONSTANT_Double:
			GetU4(cval, ccc, ffin, iBuf);
			ind ++;
			GetU4(cval, ccc, ffin, iBuf);
			break;
		default:
			sprintf(tmpBuff,"unknown tag %d in constant pool of %s\n",tag,cFile.fileName);
			error(ERR_ST,tmpBuff);
		}
	}
	goto fin;
endOfFile:
	error(ERR_ST,"unexpected end of file");
fin:
	*accc = ccc; *affin = ffin; *cpSize = count;
	return(cp);
}

void javaHumanizeLinkName( char *inn, char *outn, int size) {
	int 	i;
	char 	*cut;
	cut = strchr(inn, LINK_NAME_CUT_SYMBOL);
	if (cut==NULL) cut = inn;
	else cut ++;
	for(i=0; cut[i]; i++) {
		outn[i] = cut[i];
//&		if (LANGUAGE(LAN_JAVA)) {
			if (outn[i]=='/') outn[i]='.';
//&		}
		InternalCheck(i<size-1);
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

S_typeModifiers * cfUnPackResultType(char *sig, char **restype) {
	S_typeModifiers *res,**ares,*tt;
	S_symbol dd,*pdd;
	S_symbolList ddl,*memb;
	int typ,ii,nmlen;
	char *fqname,*nnn;
	char *ssig,*ccname;
	int ccnameOffset;
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
		CF_ALLOC(tt, S_typeModifiers);
		*ares = tt;
		ares = &(tt->next);
		switch (*ssig) {
		case ')': 
			FILLF_typeModifiers(tt, TypeFunction,f,( NULL,NULL) ,NULL, NULL);
			assert(*sig == '(');
			tt->u.m.sig = NULL;	/* must be set later !!!!!!!!!! */
			break;
		case '[': 
			FILLF_typeModifiers(tt, TypeArray,f,( NULL,NULL) ,NULL, NULL);
			for(ssig++; *ssig && isdigit(*ssig); ssig++) ;
			ssig --;
			break;
		case 'L':
			FILLF_typeModifiers(tt, TypeStruct,t, NULL,NULL, NULL);
			fqname = ++ssig; 
			ccname = fqname;
			for(; *ssig && *ssig!=';'; ssig++) {
				if (*ssig == '/' || *ssig == '$') ccname = ssig+1;
			}
			assert(*ssig == ';');
			nmlen = ssig-fqname;
			ccnameOffset = ccname - fqname;
			*ssig = 0;

#if ! ZERO  // [13.1.2003]

			tt->u.t = javaFQTypeSymbolDefinition(ccname, fqname);

#else

			FILL_symbolBits(&dd.b,0,0, 0,0, 0, TypeStruct, StorageNone,0);
			FILL_symbol(&dd, ccname, fqname,s_noPos,dd.b,s,NULL,NULL);
			FILL_symbolList(&ddl, &dd, NULL);
			if (! javaFqtTabIsMember(&s_javaFqtTab,&ddl,&ii,&memb)) {
				// TODO rather call 'javaFQTypeSymbolDefinitionCreate' !!!!
				CF_ALLOCC(nnn, nmlen+1, char);
				strcpy(nnn,fqname);
				CF_ALLOC(pdd, S_symbol);			
				FILL_symbolBits(&pdd->b,0,0,0,0,0,TypeStruct,StorageNone,0);
				FILL_symbol(pdd,nnn+ccnameOffset,nnn,s_noPos,pdd->b,s,NULL,NULL);
				CF_ALLOC(pdd->u.s, S_symStructSpecific);
				FILLF_symStructSpecific(pdd->u.s, NULL,
								NULL, NULL, NULL, 0, NULL,
								TypeStruct,t,NULL,NULL,NULL,
								TypePointer,f,(NULL,NULL),NULL,&pdd->u.s->stype,
								javaFqtNamesAreFromTheSamePackage(nnn, s_javaThisPackageName),0, -1,0);
				pdd->u.s->stype.u.t = pdd;
				CF_ALLOC(memb, S_symbolList);
				FILL_symbolList(memb, pdd, NULL);
				javaFqtTabSet(&s_javaFqtTab,memb,ii);
				// I think this can be there, as it is very used
				javaCreateClassFileItem(memb->d);
			}
			tt->u.t = memb->d;
#endif
			*ssig = ';';
			break;
		default:
			typ = s_javaCharCodeBaseTypes[*ssig];
			assert(typ != 0);
			FILLF_typeModifiers(tt, typ,f,( NULL,NULL) ,NULL, NULL);
		}
	}
	return(res);
}

static void cfAddRecordToClass(	char *name,
								char *sig,
								S_symbol *clas,
								int accessFlags,
								int storage,
								S_symbolList *exceptions
	) {
	static char pp[MAX_PROFILE_SIZE];
	S_symbol *d,*memb;
	S_symbolList *dl;
	char *ln,*prof;
	S_typeModifiers *tt;
	char *restype;
	int len, vFunCl;
	int rr,bc;
	S_position dpos;
	tt = cfUnPackResultType(sig, &restype);
	if (tt->m==TypeFunction) {
		// hack, this will temporary cut the result,
		// however if this is the shared string with name ....? B of B for ex.
		bc = *restype; *restype = 0;
		if ((accessFlags & ACC_STATIC)) {
/*&
			sprintf(pp,"%s.%s%s", clas->linkName, name, sig);
			vFunCl = s_noneFileIndex;
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
		vFunCl = s_noneFileIndex;
&*/
	}
//&	rr = javaExistEquallyProfiledFun(clas, name, sig, &memb);
	rr = javaIsYetInTheClass(clas, pp, &memb);
//&	rr = 0;
	if (rr) {
		ln = memb->linkName;
	} else {
		len = strlen(pp);
		CF_ALLOCC(ln, len+1, char);
		strcpy(ln, pp);
	}
	prof = strchr(ln,'(');
	if (tt->m == TypeFunction) {
		assert(*sig == '(');
		assert(*prof == '(');
		tt->u.m.sig = prof;
		tt->u.m.exceptions = exceptions;
	}
/*fprintf(dumpOut,"add definition of %s == %s\n",name, ln);fflush(dumpOut);*/
	CF_ALLOC(d, S_symbol);
	FILL_symbolBits(&d->b,0,0,accessFlags,0, 0,TypeDefault, storage, 0);
	FILL_symbol(d, name, ln, s_noPos,d->b,type,tt,NULL);
	d->u.type = tt;
	assert(clas->u.s);
	LIST_APPEND(S_symbol, clas->u.s->records, d);
	if (s_opt.allowClassFileRefs) {
		FILL_position(&dpos, clas->u.s->classFile, 1, 0);
		addCxReference(d, &dpos, UsageClassFileDefinition, vFunCl, vFunCl);
	}
}

static void cfReadFieldInfos(	char **accc, 
								char **affin, 
								S_charBuf *iBuf, 
								S_symbol *memb,
								union constantPoolUnion *cp
							) {
	register char *ccc, *ffin;
	register int cval,count,ind;
	int size,i,accesFlags,nameind,sigind;
	ccc = *accc; ffin = *affin;
	GetU2(count, ccc, ffin, iBuf);
	for(ind=0; ind<count; ind++) {
		GetU2(accesFlags, ccc, ffin, iBuf);
		GetU2(nameind, ccc, ffin, iBuf);
		GetU2(sigind, ccc, ffin, iBuf);
/*
fprintf(dumpOut, "field '%s' of type '%s'\n",
cp[nameind].asciz,cp[sigind].asciz); fflush(dumpOut);
*/
		cfAddRecordToClass(cp[nameind].asciz,cp[sigind].asciz,memb,accesFlags,
						   StorageField,NULL);
		SkipAttributes(ccc, ffin, iBuf);
	}
	goto fin;
endOfFile:
	error(ERR_ST,"unexpected end of file");
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

static void cfReadMethodInfos(	char **accc, 
								char **affin, 
								S_charBuf *iBuf, 
								S_symbol *memb,
								union constantPoolUnion *cp
	) {
	register char *ccc, *ffin, *name, *sign, *sign2;
	register unsigned cval,count,ind;
	register unsigned aind,acount,aname,alen,excount;
	int size,i,accesFlags,nameind,sigind,storage,exclass;
	char *exname, *exsname;
	S_symbol *exc;
	S_symbolList *exclist, *ee;
	ccc = *accc; ffin = *affin;
	GetU2(count, ccc, ffin, iBuf);
	for(ind=0; ind<count; ind++) {
		GetU2(accesFlags, ccc, ffin, iBuf);
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
			char ttt[TMP_STRING_SIZE];
			// isn't it yet done by javac?
			//&if (strcmp(name, JAVA_CONSTRUCTOR_NAME2)==0) {
			//&	accesFlags |= ACC_STATIC;
			//&}
			// if constructor, put there type name as constructor name
			// instead of <init>
//&fprintf(dumpOut,"constructor %s of %s\n", name, memb->name);
			assert(memb && memb->b.symType==TypeStruct && memb->u.s);
			name = memb->name;
#if ZERO
			sprintf(ttt,"%s%c%s",memb->linkName,LINK_NAME_CUT_SYMBOL,memb->name);
			
			CF_ALLOCC(name, strlen(ttt)+1, char);
			strcpy(name, ttt);
#endif
			storage = StorageConstructor;
			if (s_fileTab.tab[memb->u.s->classFile]->directEnclosingInstance != s_noneFileIndex) {
				//&	assert(memb->u.s->existsDEIarg);
				// the first argument is direct enclosing instance, remove it
				sign2 = cfSkipFirstArgumentInSigString(sign);
				CF_ALLOCC(sign, strlen(sign2)+2, char);
				sign[0] = '(';  strcpy(sign+1, sign2);
//&fprintf(dumpOut,"it is nested constructor %s %s %d\n", name, sign, memb->u.s->existsDEIarg);
			} else {
				//&	assert(memb->u.s->existsDEIarg == 0);
			}
		} else if (name[0] == '<') {
			// still some strange constructor name?
//&fprintf(dumpOut,"strange constructor %s %s\n", name, sign);
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
					CF_ALLOC(ee, S_symbolList);
					FILL_symbolList(ee, exc, exclist);
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
		cfAddRecordToClass(name, sign, memb, accesFlags, storage, exclist);
	}
	goto fin;
 endOfFile:
	error(ERR_ST,"unexpected end of file");
 fin:
	*accc = ccc; *affin = ffin;
}

S_symbol *cfAddCastsToModule(S_symbol *memb, S_symbol *sup) {
	assert(memb->u.s);
	cctAddSimpleValue(&memb->u.s->casts, sup, 1);
	assert(sup->u.s);
	cctAddCctTree(&memb->u.s->casts, &sup->u.s->casts, 1);
	return(sup);
}

void addSuperClassOrInterface( S_symbol *memb, S_symbol *supp, int origin ) {
	S_symbolList *ssl, *ss;
	// [14.9]
	supp = javaFQTypeSymbolDefinition(supp->name, supp->linkName);
	//
	for(ss=memb->u.s->super; ss!=NULL && ss->d!=supp; ss=ss->next) ;
	if (ss!=NULL && ss->d==supp) return; // avoid multiple occurences
	DPRINTF3(" adding supperclass %s to %s\n", supp->linkName,memb->linkName);
	if (cctIsMember(&supp->u.s->casts, memb, 1) || memb==supp) {
		sprintf(tmpBuff,"a cycle in super classes of %s detected",
				memb->linkName);
		error(ERR_ST, tmpBuff);
		return;
	}
	cfAddCastsToModule(memb, supp);
	CF_ALLOC(ssl, S_symbolList);
	FILL_symbolList(ssl, supp, NULL);
	LIST_APPEND(S_symbolList, memb->u.s->super, ssl);
	addSubClassItemToFileTab(supp->u.s->classFile,
							 memb->u.s->classFile,
							 origin);
}

void addSuperClassOrInterfaceByName(S_symbol *memb, char *super, int origin,
									int loadSuper) {
	S_symbol 		*supp;
	supp = javaGetFieldClass(super,NULL);
	if (loadSuper==LOAD_SUPER) javaLoadClassSymbolsFromFile(supp);
	addSuperClassOrInterface( memb, supp, origin);
}

int javaCreateClassFileItem( S_symbol *memb) {
	char ftname[MAX_FILE_NAME_SIZE];
	char *rftname;
	int ii, newItem;
	SPRINT_FILE_TAB_CLASS_NAME(ftname, memb->linkName);
	newItem = addFileTabItem(ftname, &ii);
	memb->u.s->classFile = ii;
	// this is a hack, as this file is never on command line
	// but why this was set to 1? This makes forgotting all class hierarchy
	// from cxfile on update !!!!
	// I am removing it, but dont know what will happen
	//& s_fileTab.tab[ii]->b.cxLoading = 1;
	//& s_fileTab.tab[ii]->b.cxLoaded = 1;
	return(ii);
}

/* ********************************************************************* */

void javaReadClassFile(char *name, S_symbol *memb, int loadSuper) {
	register int cval,tmp,i,inum,innval,rinners,snum,modifs;
	FILE *ff;
	int fnlen,upp,innNameInd,membFlag;
	char *ccc, *ffin;
	char *super,*interf,*innerCName, *zipsep;
	S_symbol *interfs, *inners, *supp;
	S_symbolList *ssl;
	int thisClass, superClass, accesFlags, cpSize;
	int fileInd,mm,ind,count,aname,alen,cn;
	char *inner,*upper,*thisClassName;
	S_charBuf *inBuf;
	union constantPoolUnion *constantPool;
	S_fileItem ffi;
	S_position pos;

	memb->b.javaFileLoaded = 1;
/*&
fprintf(dumpOut,": ppmmem == %d/%d\n",ppmMemoryi,SIZE_ppmMemory);
fprintf(dumpOut,":reading file %s arg class == %s == %s\n", 
name, memb->name, memb->linkName); fflush(dumpOut);
&*/
	zipsep = strchr(name, ZIP_SEPARATOR_CHAR);
	if (zipsep != NULL) *zipsep = 0;
	ff = fopen(name, "rb");
	if (zipsep != NULL) *zipsep = ZIP_SEPARATOR_CHAR;

	if (ff == NULL) {
		error(ERR_CANT_OPEN, name);
		return;
	}

	fileInd = javaCreateClassFileItem( memb);
	s_fileTab.tab[fileInd]->b.cxLoading = 1;

	FILL_position(&pos, fileInd,1,0);
	addCxReference(memb, &pos, UsageClassFileDefinition, 
				   s_noneFileIndex, s_noneFileIndex);
	addCfClassTreeHierarchyRef(fileInd, UsageClassFileDefinition);
//&fprintf(dumpOut,"ftitem==%s\n", s_fileTab.tab[fileInd]->name);
	pushNewInclude( ff, NULL, s_fileTab.tab[fileInd]->name, "");

	DPRINTF2("\nreading file %s\n",name); fflush(dumpOut);


	inBuf = &cFile.lb.cb;
	ccc = cFile.lb.cb.cc; ffin = cFile.lb.cb.fin;
	if (zipsep != NULL) {
		if (zipSeekToFile(&ccc,&ffin,&cFile.lb.cb,name) == 0) goto fini;
	}
	GetU4(cval, ccc, ffin, &cFile.lb.cb);
/*&fprintf(dumpOut, "magic is %x\n", cval); fflush(dumpOut);&*/
	if (cval != 0xcafebabe) {
		sprintf(tmpBuff,"%s is not a valid class file\n",name);
		error(ERR_ST,tmpBuff);
		goto fini;
	}
	InternalCheck(cval == 0xcafebabe);
	GetU4(cval, ccc, ffin, &cFile.lb.cb);
/*&fprintf(dumpOut, "version is %d\n", cval);&*/
	constantPool = cfReadConstantPool(&ccc, &ffin, &cFile.lb.cb, &cpSize);
	GetU2(accesFlags, ccc, ffin, &cFile.lb.cb);
	memb->b.accessFlags = accesFlags;
//&fprintf(dumpOut,"reading accessFlags %s == %x\n", name, accesFlags);
	if (accesFlags & ACC_INTERFACE) s_fileTab.tab[fileInd]->b.isInterface=1;
	GetU2(thisClass, ccc, ffin, &cFile.lb.cb);
	if (thisClass<0 || thisClass>=cpSize) goto corrupted;
	thisClassName = constantPool[constantPool[thisClass].clas.nameIndex].asciz;
	// TODO!!!, it may happen that name of class differ in cases from name of file,
	// what to do in such case? abandon with an error?
//&fprintf(dumpOut,"this class == %s\n", thisClassName);fflush(dumpOut);
	GetU2(superClass, ccc, ffin, &cFile.lb.cb);
	if (superClass != 0) {
		if (superClass<0 || superClass>=cpSize) goto corrupted;
		super = constantPool[constantPool[superClass].clas.nameIndex].asciz;
		addSuperClassOrInterfaceByName(memb, super, memb->u.s->classFile,
									   loadSuper);
	}	

	GetU2(inum, ccc, ffin, &cFile.lb.cb);
	/* implemented interfaces */

	for(i=0; i<inum; i++) {
		GetU2(cval, ccc, ffin, &cFile.lb.cb);
		if (cval != 0) {
			if (cval<0 || cval>=cpSize) goto corrupted;
			interf = constantPool[constantPool[cval].clas.nameIndex].asciz;
			addSuperClassOrInterfaceByName(memb, interf, memb->u.s->classFile,
										   loadSuper);
		}
	}
//& addSubClassesItemsToFileTab(memb, memb->u.s->classFile);

	cfReadFieldInfos(&ccc, &ffin, &cFile.lb.cb, memb, constantPool);
	if (cFile.lb.cb.isAtEOF) goto endOfFile;
	cfReadMethodInfos(&ccc, &ffin, &cFile.lb.cb, memb, constantPool);
	if (cFile.lb.cb.isAtEOF) goto endOfFile;

	GetU2(count, ccc, ffin, &cFile.lb.cb);
	for(ind=0; ind<count; ind++) {
		GetU2(aname, ccc, ffin, &cFile.lb.cb);
		GetU4(alen, ccc, ffin, &cFile.lb.cb);
		if (strcmp(constantPool[aname].asciz,"InnerClasses")==0) {
			GetU2(inum, ccc, ffin, &cFile.lb.cb);
			memb->u.s->nnested = inum;
			// TODO: replace the inner tab by inner list
			if (inum >= MAX_INNERS_CLASSES) {
				fatalError(ERR_ST,"number of nested classes overflowed over MAX_INNERS_CLASSES", XREF_EXIT_ERR);
			}
			memb->u.s->nest = NULL;
			if (inum > 0) {
				// I think this should be optimized, not all mentioned here
				// are my inners classes
//&				CF_ALLOCC(memb->u.s->nest, MAX_INNERS_CLASSES, S_nestedSpec);
				CF_ALLOCC(memb->u.s->nest, inum, S_nestedSpec);
			}
			for(rinners=0; rinners<inum; rinners++) {
				GetU2(innval, ccc, ffin, &cFile.lb.cb);
				inner = constantPool[constantPool[innval].clas.nameIndex].asciz;
//&fprintf(dumpOut,"inner %s \n",inner);fflush(dumpOut);
				GetU2(upp, ccc, ffin, &cFile.lb.cb);
				GetU2(innNameInd, ccc, ffin, &cFile.lb.cb);
				if (innNameInd==0) innerCName = "";			// !!!!!!!! hack
				else innerCName = constantPool[innNameInd].asciz;
//&fprintf(dumpOut,"class name %x='%s'\n",innerCName,innerCName);fflush(dumpOut);
				inners = javaFQTypeSymbolDefinition(innerCName, inner);
				membFlag = 0;
				if (upp != 0) {
					upper = constantPool[constantPool[upp].clas.nameIndex].asciz;
//&{fprintf(dumpOut,"upper %s encloses %s\n",upper, inner);fflush(dumpOut);}
					if (strcmp(upper,thisClassName)==0) {
						membFlag = 1;
/*&fprintf(dumpOut,"set as member class \n"); fflush(dumpOut);&*/
					}
				}
				GetU2(modifs, ccc, ffin, &cFile.lb.cb);
				//& inners->b.accessFlags |= modifs;
//&fprintf(dumpOut,"modif? %x\n",modifs);fflush(dumpOut);
 
				FILL_nestedSpec(& memb->u.s->nest[rinners], inners, membFlag, modifs);
				assert(inners && inners->b.symType==TypeStruct && inners->u.s);
				cn = inners->u.s->classFile;
				if (membFlag && ! (modifs & ACC_STATIC)) {
					// note that non-static direct enclosing class exists
					//&inners->u.s->existsDEIarg = 1;
					assert(s_fileTab.tab[cn]);
					s_fileTab.tab[cn]->directEnclosingInstance = memb->u.s->classFile;
				} else {
					//&inners->u.s->existsDEIarg = 0;
					s_fileTab.tab[cn]->directEnclosingInstance = s_noneFileIndex;
				}
			}
#if ZERO
			// no, do not load them, it seems that important access flags were readed
			// in its enclosing class
			for(rinners=0; rinners<inum; rinners++) {
				// now load inner classes, I will need their access flags for visibilities
				javaLoadClassSymbolsFromFile(memb->u.s->nest[rinners].cl);
			}
#endif
		} else {
			SkipNChars(alen, ccc, ffin, &cFile.lb.cb);
		}
	}
	s_fileTab.tab[fileInd]->b.cxLoaded = 1;
	goto fini;

endOfFile:
	sprintf(tmpBuff,"unexpected end of file '%s'", name);
	error(ERR_ST, tmpBuff);
	goto emergency;
corrupted:
	sprintf(tmpBuff,"corrupted file '%s'", name);
	error(ERR_ST, tmpBuff);
	goto emergency;
emergency:
	memb->u.s->nnested = 0;
fini:
	DPRINTF2("closing file %s\n",name);
//&{fprintf(dumpOut,": closing file %s\n",name);fflush(dumpOut);fprintf(dumpOut,": ppmmem == %d/%d\n",ppmMemoryi,SIZE_ppmMemory);fflush(dumpOut);}
	popInclude();
}




