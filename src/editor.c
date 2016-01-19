/*
	$Revision: 1.35 $
	$Date: 2002/09/08 21:28:57 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "protocol.h"
//

#define EDITOR_BUFF_TAB_SIZE 100

#define MIN_EDITOR_MEMORY_BLOCK 11
#define MAX_EDITOR_MEMORY_BLOCK 32

// this has to cover at least allignement allocations
#define EDITOR_ALLOCATION_RESERVE 1024
#define EDITOR_FREE_PREFIX_SIZE 16

S_editorMemoryBlock *s_editorMemory[MAX_EDITOR_MEMORY_BLOCK];

S_editorBufferList *s_staticEditorBufferTabTab[EDITOR_BUFF_TAB_SIZE];
S_editorBufferTab s_editorBufferTab;

#define HASH_TAB_TYPE struct editorBufferTab
#define HASH_ELEM_TYPE S_editorBufferList
#define HASH_FUN_PREFIX editorBufferTab
#define HASH_FUN(elemp) hashFun(elemp->f->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->f->name,e2->f->name)==0)

#include "hashlist.tc"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#undef HASH_FUN
#undef HASH_ELEM_EQUAL

////////////////////////////////////////////////////////////////////
//                      encoding stuff

#define EDITOR_ENCODING_WALK_THROW_BUFFER(buff, command) {\
	register unsigned char *s, *d, *maxs;\
	unsigned char *space;\
	space = (unsigned char *)buff->a.text;\
	maxs = space + buff->a.bufferSize;\
	for(s=d=space; s<maxs; s++) {\
		command\
	}\
	buff->a.bufferSize = d - space;\
}
#define EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d) \
	if (*s == '\r') {\
		if (s+1<maxs && *(s+1)=='\n') {\
			s++;\
			*d++ = *s;\
		} else {\
			*d++ = '\n';\
		}\
	}
#define EDITOR_ENCODING_CR_LF_CONVERSION(s,d) \
	if (*s == '\r' && s+1<maxs && *(s+1)=='\n') {\
		s++;\
		*d++ = *s;\
	}
#define EDITOR_ENCODING_CR_CONVERSION(s,d) \
	if (*s == '\r') {\
		*d++ = '\n';\
	}
#define EDITOR_ENCODING_UTF8_CONVERSION(s,d) \
	if (*(s) & 0x80) {\
		unsigned z;\
		z = *s;\
		if (z <= 223) {s+=1; *d++ = ' ';}\
		else if (z <= 239) {s+=2; *d++ = ' ';}\
		else if (z <= 247) {s+=3; *d++ = ' ';}\
		else if (z <= 251) {s+=4; *d++ = ' ';}\
		else {s+=5; *d++ = ' ';}\
	}
#define EDITOR_ENCODING_EUC_CONVERSION(s,d) \
	if (*(s) & 0x80) {\
		unsigned z;\
		z = *s;\
		if (z == 0x8e) {s+=2; *d++ = ' ';}\
		else if (z == 0x8f) {s+=3; *d++ = ' ';}\
        else {s+=1; *d++ = ' ';}\
	}
#define EDITOR_ENCODING_SJIS_CONVERSION(s,d) \
	if (*(s) & 0x80) {\
		unsigned z;\
		z = *s;\
		if (z >= 0xa1 && z <= 0xdf) {*d++ = ' ';}\
        else {s+=1; *d++ = ' ';}\
	}
#define EDITOR_ENCODING_ELSE_BRANCH(s,d) \
	{\
		*d++ = *s;\
	}

static void editorApplyCrLfCrConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyCrLfConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyCrConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_CR_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyUtf8CrLfCrConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_UTF8_CONVERSION(s,d)
		else EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyUtf8CrLfConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_UTF8_CONVERSION(s,d)
		else EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyUtf8CrConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_UTF8_CONVERSION(s,d)
		else EDITOR_ENCODING_CR_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyUtf8Conversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_UTF8_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyEucCrLfCrConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_EUC_CONVERSION(s,d)
		else EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyEucCrLfConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_EUC_CONVERSION(s,d)
		else EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyEucCrConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_EUC_CONVERSION(s,d)
		else EDITOR_ENCODING_CR_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyEucConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_EUC_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplySjisCrLfCrConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_SJIS_CONVERSION(s,d)
		else EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplySjisCrLfConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_SJIS_CONVERSION(s,d)
		else EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplySjisCrConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_SJIS_CONVERSION(s,d)
		else EDITOR_ENCODING_CR_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplySjisConversion(S_editorBuffer *buff) {
	EDITOR_ENCODING_WALK_THROW_BUFFER(buff, {
		EDITOR_ENCODING_SJIS_CONVERSION(s,d)
		else EDITOR_ENCODING_ELSE_BRANCH(s,d)
	});
}
static void editorApplyUtf16Conversion(S_editorBuffer *buff) {
	register unsigned char 	*s, *d, *maxs;
	register unsigned		cb, cb2;
	int 					little_endian;
	unsigned char 			*space;
	space = (unsigned char *)buff->a.text;
	maxs = space + buff->a.bufferSize;
	s = space;
	// determine endian first
	cb = (*s << 8) + *(s+1);
	if (cb == 0xfeff) {
		little_endian = 0;
		s += 2;
	} else if (cb == 0xfffe) {
		little_endian = 1;
		s += 2;
	} else if (s_opt.fileEncoding == MULE_UTF_16LE) {
		little_endian = 1;
	} else if (s_opt.fileEncoding == MULE_UTF_16BE) {
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
				*d++ = (char)(cb & 0xff);
			} else {
				*d++ = ' ';
			}
		}
	}
	buff->a.bufferSize = d - space;
}
static int editorBufferStartsWithUtf16Bom(S_editorBuffer *buff) {
	register unsigned char 		*s;
	register unsigned			cb;
	s = buff->a.text;
	if (buff->a.bufferSize >= 2) {
		cb = (*s << 8) + *(s+1);
		if (cb == 0xfeff || cb == 0xfffe) return(1);
	}
	return(0);
}

static void editorPerformSimpleLineFeedConversion(S_editorBuffer *buff) {
	if ((s_opt.eolConversion&CR_LF_EOL_CONVERSION)
		&& (s_opt.eolConversion & CR_EOL_CONVERSION)) {
		editorApplyCrLfCrConversion(buff);
	} else if (s_opt.eolConversion & CR_LF_EOL_CONVERSION) {
		editorApplyCrLfConversion(buff);
	} else if (s_opt.eolConversion & CR_EOL_CONVERSION) {
		editorApplyCrConversion(buff);
	}
}

static void editorPerformEncodingAdjustemets(S_editorBuffer *buff) {
	// do different loops for efficiency reasons
	if (s_opt.fileEncoding == MULE_EUROPEAN) {
		editorPerformSimpleLineFeedConversion(buff);
	} else if (s_opt.fileEncoding == MULE_EUC) {
		if ((s_opt.eolConversion&CR_LF_EOL_CONVERSION)
			&& (s_opt.eolConversion & CR_EOL_CONVERSION)) {
			editorApplyEucCrLfCrConversion(buff);
		} else if (s_opt.eolConversion & CR_LF_EOL_CONVERSION) {
			editorApplyEucCrLfConversion(buff);
		} else if (s_opt.eolConversion & CR_EOL_CONVERSION) {
			editorApplyEucCrConversion(buff);
		} else {
			editorApplyEucConversion(buff);
		}
	} else if (s_opt.fileEncoding == MULE_SJIS) {
		if ((s_opt.eolConversion&CR_LF_EOL_CONVERSION)
			&& (s_opt.eolConversion & CR_EOL_CONVERSION)) {
			editorApplySjisCrLfCrConversion(buff);
		} else if (s_opt.eolConversion & CR_LF_EOL_CONVERSION) {
			editorApplySjisCrLfConversion(buff);
		} else if (s_opt.eolConversion & CR_EOL_CONVERSION) {
			editorApplySjisCrConversion(buff);
		} else {
			editorApplySjisConversion(buff);
		}
	} else {
		// default == utf
		if ((s_opt.fileEncoding != MULE_UTF_8 && editorBufferStartsWithUtf16Bom(buff))
			|| s_opt.fileEncoding == MULE_UTF_16 
			|| s_opt.fileEncoding == MULE_UTF_16LE
			|| s_opt.fileEncoding == MULE_UTF_16BE
			) {
			editorApplyUtf16Conversion(buff);
			editorPerformSimpleLineFeedConversion(buff);
		} else {
			// utf-8
			if ((s_opt.eolConversion&CR_LF_EOL_CONVERSION)
				&& (s_opt.eolConversion & CR_EOL_CONVERSION)) {
				editorApplyUtf8CrLfCrConversion(buff);
			} else if (s_opt.eolConversion & CR_LF_EOL_CONVERSION) {
				editorApplyUtf8CrLfConversion(buff);
			} else if (s_opt.eolConversion & CR_EOL_CONVERSION) {
				editorApplyUtf8CrConversion(buff);
			} else {
				editorApplyUtf8Conversion(buff);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

void editorInit() {
	s_editorBufferTab.tab = s_staticEditorBufferTabTab;
	editorBufferTabNAInit(&s_editorBufferTab, EDITOR_BUFF_TAB_SIZE);
}

int statb(char *path, struct stat *statbuf) {
	int 				res;
	S_editorBuffer 		*buff;
	buff = editorGetOpenedBuffer(path);
	if (buff!=NULL) {
		if (statbuf!=NULL) {
			*statbuf = buff->stat;
//&sprintf(tmpBuff,"returning stat of %s modified at %s", path, ctime(&buff->stat.st_mtime)); ppcGenRecord(PPC_IGNORE, tmpBuff, "\n");
		}
		return(0);
	}
	res = stat(path, statbuf);
	return(res);
}

static void editorError(int errCode, char *message) {
	error(errCode, message);
}

int editorMarkerLess(S_editorMarker *m1, S_editorMarker *m2) {
	int c;
	// m1->buffer->ftnum <> m2->buffer->ftnum;
	// following is tricky as it works also for renamed buffers
	if (m1->buffer < m2->buffer) return(1);
	if (m1->buffer > m2->buffer) return(0);
	if (m1->offset < m2->offset) return(1);
	if (m1->offset > m2->offset) return(0);
	return(0);
}

int editorMarkerLessOrEq(S_editorMarker *m1, S_editorMarker *m2) {
	return(! editorMarkerLess(m2, m1));
}

int editorMarkerGreater(S_editorMarker *m1, S_editorMarker *m2) {
	return(editorMarkerLess(m2, m1));
}

int editorMarkerGreaterOrEq(S_editorMarker *m1, S_editorMarker *m2) {
	return(editorMarkerLessOrEq(m2, m1));
}

int editorMarkerListLess(S_editorMarkerList *l1, S_editorMarkerList *l2) {
	return(editorMarkerLess(l1->d, l2->d));
}

int editorRegionListLess(S_editorRegionList *l1, S_editorRegionList *l2) {
	if (editorMarkerLess(l1->r.b, l2->r.b)) return(1);
	if (editorMarkerLess(l2->r.b, l1->r.b)) return(0);
	// region begins are equal, check end
	if (editorMarkerLess(l1->r.e, l2->r.e)) return(1);
	if (editorMarkerLess(l2->r.e, l1->r.e)) return(0);
	return(0);
}

static void editorAffectMarkerToBuffer(S_editorBuffer *buff, S_editorMarker *marker) {
	marker->buffer = buff;
	marker->next = buff->markers;
	buff->markers = marker;
	marker->previous = NULL;
	if (marker->next!=NULL) marker->next->previous = marker;
}

S_editorMarker *editorCrNewMarker(S_editorBuffer *buff, int offset) {
	S_editorMarker *m;
	ED_ALLOC(m, S_editorMarker);
	FILL_editorMarker(m, NULL, offset, NULL, NULL);
	editorAffectMarkerToBuffer(buff, m);
	return(m);
}

S_editorMarker *editorCrNewMarkerForPosition(S_position *pos) {
	S_editorBuffer 	*buf;
	S_editorMarker	*mm;
	if (pos->file==s_noneFileIndex || pos->file<0) {
		error(ERR_INTERNAL, "[editor] creating marker for nonexistant position");
	}
	buf = editorFindFile(s_fileTab.tab[pos->file]->name);
	mm = editorCrNewMarker(buf, 0);
	editorMoveMarkerToLineCol(mm, pos->line, pos->coll);
	return(mm);
}

S_editorMarker *editorDuplicateMarker(S_editorMarker *mm) {
	return(editorCrNewMarker(mm->buffer, mm->offset));
}

static void editorRemoveMarkerFromBufferNoFreeing(S_editorMarker *marker) {
	S_editorMarker 	**m, *next;
	S_editorBuffer	*buff;
	if (marker == NULL) return;
	if (marker->next!=NULL) marker->next->previous = marker->previous;
	if (marker->previous==NULL) {
		marker->buffer->markers = marker->next;
	} else {
		marker->previous->next = marker->next;
	}
}

void editorFreeMarker(S_editorMarker *marker) {
	if (marker == NULL) return;
	editorRemoveMarkerFromBufferNoFreeing(marker);
	ED_FREE(marker, sizeof(S_editorMarker));
}

static void editorFreeTextSpace(char *space, int index) {
	S_editorMemoryBlock *sp;
	sp = (S_editorMemoryBlock *) space;
	sp->next = s_editorMemory[index];
	s_editorMemory[index] = sp;
}

static void editorFreeBuffer(S_editorBufferList *ll) {
	S_editorMarker *m, *mm;
//&fprintf(stderr,"freeing buffer %s==%s\n", ll->f->name, ll->f->fileName); 
	if (ll->f->fileName != ll->f->name) {
		ED_FREE(ll->f->fileName, strlen(ll->f->fileName)+1);
	}
	ED_FREE(ll->f->name, strlen(ll->f->name)+1);
	for(m=ll->f->markers; m!=NULL;) {
		mm = m->next;
		ED_FREE(m, sizeof(S_editorMarker));
		m = mm;
	}
	if (ll->f->b.textLoaded) {
//&fprintf(dumpOut,"freeing %d of size %d\n",ll->f->a.allocatedBlock,ll->f->a.allocatedSize);
		// check for magic
		assert(ll->f->a.allocatedBlock[ll->f->a.allocatedSize] == 0x3b);
		editorFreeTextSpace(ll->f->a.allocatedBlock,ll->f->a.allocatedIndex);
	}
	ED_FREE(ll->f, sizeof(S_editorBuffer));
	ED_FREE(ll, sizeof(S_editorBufferList));
}

static void editorLoadFileIntoBufferText(S_editorBuffer *buff, struct stat *st) {
	register char *s, *d, *maxs;
	char 	*space, *fname;
	FILE 	*ff;
	char 	*bb;
	int 	n, ss, size;
	fname = buff->fileName;
	space = buff->a.text;
	size = buff->a.bufferSize;
#if defined (__WIN32__) || defined (__OS2__)    	/*SBD*/
	ff = fopen(fname, "r");			// was rb, but did not work
#else												/*SBD*/
	ff = fopen(fname, "r");
#endif												/*SBD*/
//&fprintf(dumpOut,":loading file %s==%s size %d\n", fname, buff->name, size);
	if (ff == NULL) {
		fatalError(ERR_CANT_OPEN, fname, XREF_EXIT_ERR);
	}
	bb = space; ss = size;
	assert(bb != NULL);
	do {
		n = fread(bb, 1, ss, ff);
		bb = bb + n;
		ss = ss - n;
	} while (n>0);
	fclose(ff);
	if (ss!=0) {
		// this is possible, due to <CR><LF> conversion under MS-DOS
		buff->a.bufferSize -= ss;
		if (ss < 0) {
			sprintf(tmpBuff,"File %s: readed %d chars of %d", fname, size-ss, size);
			editorError(ERR_INTERNAL, tmpBuff);
		}
	}
	editorPerformEncodingAdjustemets(buff);
	buff->stat = *st;
	buff->b.textLoaded = 1;
}

static void allocNewEditorBufferTextSpace(S_editorBuffer *ff, int size) {
	int minSize, allocIndex, allocSize;
	char *space;
	minSize = size + EDITOR_ALLOCATION_RESERVE + EDITOR_FREE_PREFIX_SIZE;
	allocIndex = 11; allocSize = 2048;
	for(; allocSize<minSize; ) {
		allocIndex ++;
		allocSize = allocSize << 1;
	}
	space = (char *)s_editorMemory[allocIndex];
	if (space == NULL) {
		space = malloc(allocSize+1);
		if (space == NULL) fatalError(ERR_NO_MEMORY, "global malloc", XREF_EXIT_ERR);
		// put magic
		space[allocSize] = 0x3b; 
	} else {
		s_editorMemory[allocIndex] = s_editorMemory[allocIndex]->next;
	}
//&memset(space, '@', allocSize); 
//&sprintf(tmpBuff,"allocating %d of size %d for %s\n", space, allocSize, ff->name);ppcGenTmpBuff();
	FILL_editorBufferAllocationData(&ff->a, 
		size, space+EDITOR_FREE_PREFIX_SIZE, 
		EDITOR_FREE_PREFIX_SIZE, space, allocIndex, allocSize
		);
}

static void fillEmptyEditorBuffer(S_editorBuffer *ff, char *aname, int ftnum, 
								  char*afname) {
	FILL_editorBufferBits(&ff->b, 0, 0, 0);
	FILL_editorBufferAllocationData(&ff->a, 0, NULL, 0, NULL, 0, 0);
	FILL_editorBuffer(ff, aname, ftnum, afname, s_noStat, NULL, ff->a, ff->b);
}

static S_editorBuffer *editorCrNewBuffer(char *name, char *fileName, struct stat *st) {
	int 							minSize, allocIndex, allocSize, ii;
	char 							*space;
	char 							*aname, *nname, *afname, *nfileName;
	S_editorBuffer					*ff;
	S_editorBufferList				*ffl;
	int								ftnum;
	nname = normalizeFileName(name, s_cwd);
	ED_ALLOCC(aname, strlen(nname)+1, char);
	strcpy(aname, nname);
	nfileName = normalizeFileName(fileName, s_cwd);
	if (strcmp(nfileName, aname)==0) {
		afname = aname;
	} else {
		ED_ALLOCC(afname, strlen(nfileName)+1, char);
		strcpy(afname, nfileName);
	}
	ED_ALLOC(ff, S_editorBuffer);
	fillEmptyEditorBuffer(ff, aname, 0, afname);
	ff->stat = *st;
	ED_ALLOC(ffl, S_editorBufferList);
	FILL_editorBufferList(ffl, ff, NULL);
//&sprintf(tmpBuff,"creating buffer %s %s\n", ff->name, ff->fileName);ppcGenRecord(PPC_IGNORE, tmpBuff, "\n");
	editorBufferTabAdd(&s_editorBufferTab, ffl, &ii);
	// set ftnum at the end, because, addfiletabitem calls back the statb
	// from editor, so be tip-top at this moment!
	addFileTabItem(aname, &ftnum);
	ff->ftnum = ftnum;
	return(ff);
}

static void editorSetBufferModifiedFlag(S_editorBuffer *buff) {
	buff->b.modified = 1;
	buff->b.modifiedSinceLastQuasySave = 1;
}

S_editorBuffer *editorGetOpenedBuffer(char *name) {
	S_editorBufferBits	ddb;
	S_editorBuffer 		dd;
	S_editorBufferList 	ddl, *memb;
	int					ii;
	fillEmptyEditorBuffer(&dd, name, 0, name);
	FILL_editorBufferList(&ddl, &dd, NULL);
	if (editorBufferTabIsMember(&s_editorBufferTab, &ddl, &ii, &memb)) {
		return(memb->f);
	}
	return(NULL);
}

S_editorBuffer *editorGetOpenedAndLoadedBuffer(char *name) {
	S_editorBuffer *res;
	res = editorGetOpenedBuffer(name);
	if (res!=NULL && res->b.textLoaded) return(res);
	return(NULL);
}

void editorRenameBuffer(S_editorBuffer *buff, char *nName, S_editorUndo **undo) {
	char				newName[MAX_FILE_NAME_SIZE];
	int					fti, ii, mem, deleted;
	S_editorBufferBits	ddb;
	S_editorBuffer 		dd, *removed;
	S_editorBufferList 	ddl, *memb, *memb2;
	S_editorUndo		*uu;
	char				*oldName;

	strcpy(newName, normalizeFileName(nName, s_cwd));
//&sprintf(tmpBuff, "Renaming %s (at %d) to %s (at %d)", buff->name, buff->name, newName, newName);warning(ERR_INTERNAL, tmpBuff);
	fillEmptyEditorBuffer(&dd, buff->name, 0, buff->name);
	FILL_editorBufferList(&ddl, &dd, NULL);
	mem = editorBufferTabIsMember(&s_editorBufferTab, &ddl, &ii, &memb);
	if (! mem) {
		sprintf(tmpBuff, "Trying to rename non existing buffer %s", buff->name);
		error(ERR_INTERNAL, tmpBuff);
		return;
	}
	assert(memb->f == buff);
	deleted = editorBufferTabDeleteExact(&s_editorBufferTab, memb);
	assert(deleted);
	oldName = buff->name;
	ED_ALLOCC(buff->name, strlen(newName)+1, char);
	strcpy(buff->name, newName);
	// update also ftnum
	addFileTabItem(newName, &fti);
	s_fileTab.tab[fti]->b.commandLineEntered = s_fileTab.tab[buff->ftnum]->b.commandLineEntered;
	buff->ftnum = fti;

	FILL_editorBufferList(memb, buff, NULL);
	if (editorBufferTabIsMember(&s_editorBufferTab, memb, &ii, &memb2)) {
		editorBufferTabDeleteExact(&s_editorBufferTab, memb2);
		editorFreeBuffer(memb2);
	}
	editorBufferTabAdd(&s_editorBufferTab, memb, &ii);

	// note undo operation
	if (undo!=NULL) {
		ED_ALLOC(uu, S_editorUndo);
		FILLF_editorUndo(uu, buff, UNDO_RENAME_BUFFER, 
						 rename, (oldName), 
						 *undo);
		*undo = uu;
	}
	editorSetBufferModifiedFlag(buff);

	// finally create a buffer with old name and empty text in order
	// to keep information that the file is no longer existing
	// so old references will be removed on update (fixing problem of
	// of moving a package into an existing package).
	removed = editorCrNewBuffer(oldName, oldName, &buff->stat);
	allocNewEditorBufferTextSpace(removed, 0);
	removed->b.textLoaded = 1;
	editorSetBufferModifiedFlag(removed);
}

S_editorBuffer *editorOpenBufferNoFileLoad(char *name, char *fileName) {
	S_editorBuffer 	*res;
	struct stat		st;
	res = editorGetOpenedBuffer(name);
	if (res != NULL) {
		return(res);
	}
	stat(fileName, &st);
	res = editorCrNewBuffer(name, fileName, &st);
	return(res);
}

S_editorBuffer *editorFindFile(char *name) {
	int					size;
	struct stat 		st;
	S_editorBuffer		*res;
	res = editorGetOpenedAndLoadedBuffer(name);
	if (res==NULL) {
		res = editorGetOpenedBuffer(name);
		if (res == NULL) {
			if (stat(name, &st)==0 && (st.st_mode & S_IFMT)!=S_IFDIR) {
				res = editorCrNewBuffer(name, name, &st);
			}
		}
		if (res != NULL && stat(res->fileName, &st)==0 && (st.st_mode & S_IFMT)!=S_IFDIR) {
			// O.K. supposing that I have a regular file
			size = st.st_size;
			allocNewEditorBufferTextSpace(res, size);
			editorLoadFileIntoBufferText(res, &st);
		} else {
			return(NULL);
		}
	}
	return(res);
}

S_editorBuffer *editorFindFileCreate(char *name) {
	S_editorBuffer	*res;
	struct stat		st;
	res = editorFindFile(name);
	if (res == NULL) {
		// create new buffer
		st.st_size = 0;
		st.st_mtime = st.st_atime = st.st_ctime = time(NULL);
		st.st_mode = S_IFCHR;
		res = editorCrNewBuffer(name, name, &st);
		assert(res!=NULL);
		allocNewEditorBufferTextSpace(res, 0);
		res->b.textLoaded = 1;
	}
	return(res);
}

void editorReplaceString(S_editorBuffer *buff, int position, int delsize, 
						 char *str, int strlength, S_editorUndo **undo) {
	int 			nsize, oldsize, index, undosize, pattractor;
	char			*text, *space, *undotext;
	S_editorBuffer 	*nb;
	S_editorMarker 	*m;
	S_editorUndo	*uu;
	assert(position >=0 && position <= buff->a.bufferSize);
	assert(delsize >= 0);
	assert(strlength >= 0);
	oldsize = buff->a.bufferSize;
	if (delsize+position > oldsize) {
		// deleting over end of buffer, 
		// delete only until end of buffer
		delsize = oldsize - position;
		if (s_opt.debug) assert(0);
	}
//&fprintf(dumpOut,"replacing string in buffer %d (%s)\n", buff, buff->name);
	nsize = oldsize + strlength - delsize;
	// prepare operation
	if (nsize >= buff->a.allocatedSize - buff->a.allocatedFreePrefixSize) {
		// resize buffer
//&sprintf(tmpBuff,"resizing %s from %d(%d) to %d\n", buff->name, buff->a.bufferSize, buff->a.allocatedSize, nsize);ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");fflush(ccOut);
		text = buff->a.text; 
		space = buff->a.allocatedBlock; index = buff->a.allocatedIndex;
		allocNewEditorBufferTextSpace(buff, nsize);
		memcpy(buff->a.text, text, oldsize);
		buff->a.bufferSize = oldsize;
		editorFreeTextSpace(space, index);
	}
	assert(nsize < buff->a.allocatedSize - buff->a.allocatedFreePrefixSize);
	if (undo!=NULL) {
		// note undo information
		undosize = strlength;
		assert(delsize >= 0);
		// O.K. allocate also 0 at the end
		ED_ALLOCC(undotext, delsize+1, char);
		memcpy(undotext, buff->a.text+position, delsize);
		undotext[delsize]=0;
		ED_ALLOC(uu, S_editorUndo);
		FILLF_editorUndo(uu, buff, UNDO_REPLACE_STRING, 
						 replace, (position, undosize, delsize, undotext), 
						 *undo);
		*undo = uu;
	}
	// edit text
	memmove(buff->a.text+position+strlength, buff->a.text+position+delsize,
			buff->a.bufferSize - position - delsize);
	memcpy(buff->a.text+position, str, strlength);
	buff->a.bufferSize = buff->a.bufferSize - delsize + strlength;
//&sprintf(tmpBuff,"setting buffersize of  %s to %d\n", buff->name, buff->a.bufferSize);ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");fflush(ccOut);
	// update markers
	if (delsize > strlength) {
		if (strlength > 0) pattractor = position + strlength - 1;
		else pattractor = position + strlength;
		for(m=buff->markers; m!=NULL; m=m->next) {
			if (m->offset >= position + strlength) {
				if (m->offset < position+delsize) {
					m->offset = pattractor;
				} else {
					m->offset = m->offset - delsize + strlength;
				}
			}
		}
	} else {
		for(m=buff->markers; m!=NULL; m=m->next) {
			if (m->offset >= position + delsize) {
				m->offset = m->offset - delsize + strlength;
			}
		}
	}
	editorSetBufferModifiedFlag(buff);
}

void editorMoveBlock(S_editorMarker *dest, S_editorMarker *src, int size,
					 S_editorUndo **undo) {
	S_editorMarker 	*mmoved, *tmp, **mmm, *mm;
	S_editorBuffer	*sb, *db;
	S_editorUndo	*uu;
	int				off1, off2, offd, undodoffset;
	mmoved = NULL;
	assert(size>=0);
	if (dest->buffer == src->buffer 
		&& dest->offset > src->offset
		&& dest->offset < src->offset+size) {
		error(ERR_INTERNAL, "[editor] moving block to its original place");
		return;
	}	
	sb = src->buffer;
	db = dest->buffer;
	// insert the block to target position
	offd = dest->offset;
	off1 = src->offset;
	off2 = off1+size;
	assert(off1 <= off2);
	// do it at two steps for the case if source buffer equals target buffer
	// first just allocate space
	editorReplaceString(db, offd, 0, sb->a.text+off1, off2-off1, NULL);
	// now copy text
	off1 = src->offset;
	off2 = off1+size;
	editorReplaceString(db, offd, off2-off1, sb->a.text+off1, off2-off1, NULL);
	// save target for undo;
	undodoffset = src->offset;
	// move all markers from moved block
	assert(off1 == src->offset);
	assert(off2 == off1+size);
	mm = sb->markers; 
	while (mm!=NULL) {
		tmp = mm->next; 
		if (mm->offset>=off1 && mm->offset<off2) {
			editorRemoveMarkerFromBufferNoFreeing(mm);
			mm->offset = offd + (mm->offset-off1);
			editorAffectMarkerToBuffer(db, mm);
		}
		mm = tmp;
	}
	// remove the source block
	editorReplaceString(sb, off1, off2-off1, sb->a.text+off1, 0, NULL);
	//
	editorSetBufferModifiedFlag(sb);
	editorSetBufferModifiedFlag(db);
	// add the whole operation into undo
	if (undo!=NULL) {
		ED_ALLOC(uu, S_editorUndo);
		FILLF_editorUndo(uu, db, UNDO_MOVE_BLOCK, 
						 moveBlock, (src->offset, off2-off1, sb, undodoffset), 
						 *undo);
		*undo = uu;		
	}
}

void editorDumpBuffer(S_editorBuffer *buff) {
	int i;
	for(i=0; i<buff->a.bufferSize; i++) {
		putc(buff->a.text[i], dumpOut);
	}
}

void editorDumpBuffers() {
	int 					i;
	S_editorBufferList 		*ll;
	fprintf(dumpOut,"[editorDumpBuffers] start\n");
	for(i=0; i<s_editorBufferTab.size; i++) {
		for(ll=s_editorBufferTab.tab[i]; ll!=NULL; ll=ll->next) {
			fprintf(dumpOut,"%d : %s==%s, %d\n", i, ll->f->name, ll->f->fileName,
					ll->f->b.textLoaded);
		}
	}
	fprintf(dumpOut,"[editorDumpBuffers] end\n");
}

static void editorQuasySaveBuffer(S_editorBuffer *buff) {
	buff->b.modifiedSinceLastQuasySave = 0;
	buff->stat.st_mtime = time(NULL);  //? why it does not work with 1;
	assert(s_fileTab.tab[buff->ftnum]);
	s_fileTab.tab[buff->ftnum]->lastModif = buff->stat.st_mtime;
}

void editorQuasySaveModifiedBuffers() {
	int 					i, saving;
	static time_t			lastQuazySaveTime = 0;
	time_t					timeNull;
	S_editorBufferList 		*ll;
	saving = 0;
	for(i=0; i<s_editorBufferTab.size; i++) {
		for(ll=s_editorBufferTab.tab[i]; ll!=NULL; ll=ll->next) {
			if (ll->f->b.modifiedSinceLastQuasySave) {
				saving = 1;
				goto cont;
			}
		}
	}
 cont:
	if (saving) {
		// sychronization, since last quazy save, there must
		// be at least one second, otherwise times will be wrong
		timeNull = time(NULL);
		if (lastQuazySaveTime > timeNull+5) {
			fatalError(ERR_INTERNAL, "last save in the future, travelling in time?", XREF_EXIT_ERR);
		} else if (lastQuazySaveTime >= timeNull) {
			sleep(1+lastQuazySaveTime-timeNull);
//&			ppcGenRecord(PPC_INFORMATION,"slept","\n");
		}
	}
	for(i=0; i<s_editorBufferTab.size; i++) {
		for(ll=s_editorBufferTab.tab[i]; ll!=NULL; ll=ll->next) {
			if (ll->f->b.modifiedSinceLastQuasySave) {
				editorQuasySaveBuffer(ll->f);
			}
		}
	}
	lastQuazySaveTime = time(NULL);
}

void editorLoadAllOpenedBufferFiles() {
	int 					i, size;
	S_editorBufferList 		*ll,*nn;
	struct stat 			st;
	for(i=0; i<s_editorBufferTab.size; i++) {
		for(ll=s_editorBufferTab.tab[i]; ll!=NULL; ll=ll->next) {
			if (!ll->f->b.textLoaded) {
				if (stat(ll->f->fileName, &st)==0) {
					size = st.st_size;
					allocNewEditorBufferTextSpace(ll->f, size);
					editorLoadFileIntoBufferText(ll->f, &st);
//&sprintf(tmpBuff,"preloading %s into %s\n", ll->f->fileName, ll->f->name); ppcGenRecord(PPC_IGNORE, tmpBuff,"\n");
				}
			}
		}
	}
}

int editorRunWithMarkerUntil(S_editorMarker *m, int (*until)(int), int step) {
	int 	offset, max;
	char	*text;
	assert(step==-1 || step==1);
	offset = m->offset;
	max = m->buffer->a.bufferSize;
	text = m->buffer->a.text;
	while (offset>=0 && offset<max && (*until)(text[offset])==0) offset += step;
	m->offset = offset;
	if (offset<0) {
		m->offset = 0;
		return(0);
	}
	if (offset>=max) {
		m->offset = max-1;
		return(0);
	}
	return(1);
}

int editorCountLinesBetweenMarkers(S_editorMarker *m1, S_editorMarker *m2) {
	int 	i, max, count;
	char	*text;
	// this can happen after an error in moving, just pass in this case
	if (m1 == NULL || m2==NULL) return(0);
	assert(m1->buffer == m2->buffer);
	assert(m1->offset <= m2->offset);
	text = m1->buffer->a.text;
	max = m2->offset;
	count = 0;
	for(i=m1->offset; i<max; i++) {
		if (text[i]=='\n') count ++;
	}
	return(count);
}

static int isNewLine(int c) {return(c=='\n');}
int editorMoveMarkerToNewline(S_editorMarker *m, int direction) {
	return(editorRunWithMarkerUntil(m, isNewLine, direction));
}

static int isNonBlank(int c) {return(! isspace(c));}
int editorMoveMarkerToNonBlank(S_editorMarker *m, int direction) {
	return(editorRunWithMarkerUntil(m, isNonBlank, direction));
}

static int isNonBlankOrNewline(int c) {return(c=='\n' || ! isspace(c));}
int editorMoveMarkerToNonBlankOrNewline(S_editorMarker *m, int direction) {
	return(editorRunWithMarkerUntil(m, isNonBlankOrNewline, direction));
}

static int isNotIdentPart(int c) {return((! isalnum(c)) && c!='_' && c!='$');}
int editorMoveMarkerBeyondIdentifier(S_editorMarker *m, int direction) {
	return(editorRunWithMarkerUntil(m, isNotIdentPart, direction));
}

void editorRemoveBlanks(S_editorMarker *mm, int direction, S_editorUndo **undo) {
	int moffset, len;
	moffset = mm->offset;
	if (direction < 0) {
		mm->offset --;
		editorMoveMarkerToNonBlank(mm, -1);
		mm->offset ++;
		editorReplaceString(mm->buffer, mm->offset, moffset - mm->offset, "", 0, undo);
	} else if (direction > 0) {
		editorMoveMarkerToNonBlank(mm, 1);
		editorReplaceString(mm->buffer, moffset, mm->offset - moffset, "", 0, undo);		
	} else {
		// both directions
		mm->offset --;
		editorMoveMarkerToNonBlank(mm, -1);
		mm->offset ++;
		moffset = mm->offset;
		editorMoveMarkerToNonBlank(mm, 1);
		editorReplaceString(mm->buffer, moffset, mm->offset - moffset, "", 0, undo);		
	}
}

void editorMoveMarkerToLineCol(S_editorMarker *m, int line, int col) {
	register char 	*s, *smax;
	register int 	ln;
	S_editorBuffer 	*buff;
	int				c;
	assert(m);
	buff = m->buffer;
	s = buff->a.text;
	smax = s + buff->a.bufferSize;
	ln = 1;
	if (line > 1) {
		for(; s<smax; s++) {
			if (*s == '\n') {
				ln ++;
				if (ln == line) break;
			}
		}
		if (s < smax) s++;
	}
	c = 0;
	for(; s<smax && c<col; s++,c++) {
		if (*s == '\n') break;
	}
	m->offset = s - buff->a.text;
	assert(m->offset>=0 && m->offset<=buff->a.bufferSize);
}

S_editorMarkerList *editorReferencesToMarkers(S_reference *refs, 
											  int (*filter)(S_reference *, void *), 
											  void *filterParam) {
	S_reference			*r;
	S_editorMarker 		*m;
	S_editorMarkerList 	*res, *rrr;
	int 				line, col, file, maxoffset;
	register char 		*s, *smax;
	register int 		ln, c;
	S_editorBuffer 		*buff;
	res = NULL;
	r = refs; 
	while (r!=NULL) {
		while (r!=NULL && ! filter(r,filterParam)) r = r->next;
		if (r != NULL) {
			file = r->p.file;
			line = r->p.line;
			col = r->p.coll;
			buff = editorFindFile(s_fileTab.tab[file]->name);
			if (buff==NULL) {
				error(ERR_CANT_OPEN, s_fileTab.tab[file]->name);
				while (r!=NULL && file == r->p.file) r = r->next;
			} else {
				s = buff->a.text;
				smax = s + buff->a.bufferSize;
				maxoffset = buff->a.bufferSize - 1;
				if (maxoffset < 0) maxoffset = 0;
				ln = 1; c = 0;
				for(; s<smax; s++, c++) {
					if (ln==line && c==col) {
						m = editorCrNewMarker(buff, s - buff->a.text);
						ED_ALLOC(rrr, S_editorMarkerList);
						FILL_editorMarkerList(rrr, m, r->usg, res);
						res = rrr;
						r = r->next;
						while (r!=NULL && ! filter(r,filterParam)) r = r->next;
						if (r==NULL || file != r->p.file) break;
						line = r->p.line;
						col = r->p.coll;
					}
					if (*s=='\n') {ln++; c = -1;}
				}
				// references beyond end of buffer
				while (r!=NULL && file == r->p.file) {
					m = editorCrNewMarker(buff, maxoffset);
					ED_ALLOC(rrr, S_editorMarkerList);
					FILL_editorMarkerList(rrr, m, r->usg, res);
					res = rrr;
					r = r->next; 
					while (r!=NULL && ! filter(r,filterParam)) r = r->next;
				}
			}
		}
	}
	// get markers in the same order as were references
	// ?? is this still needed?
	LIST_REVERSE(S_editorMarkerList, res);
	return(res);
}

S_reference *editorMarkersToReferences(S_editorMarkerList **mms) {
	S_editorMarkerList 	*mm;
	S_editorBuffer		*buf;
	char				*s, *smax, *off;
	int					ln, c;
	S_reference			*res, *rr;
	LIST_MERGE_SORT(S_editorMarkerList, *mms, editorMarkerListLess);
	res = NULL;
	mm = *mms;
	while (mm!=NULL) {
		buf = mm->d->buffer;
		s = buf->a.text;
		smax = s + buf->a.bufferSize;
		off = buf->a.text + mm->d->offset;
		ln = 1; c = 0;
		for( ; s<smax; s++, c++) {
			if (s == off) {
				OLCX_ALLOC(rr, S_reference);
				FILL_position(&rr->p, buf->ftnum, ln, c);
				FILL_reference(rr, mm->usg, rr->p, res);
				res = rr;
				mm = mm->next;
				if (mm==NULL || mm->d->buffer != buf) break;
				off = buf->a.text + mm->d->offset;
			}
			if (*s=='\n') {ln++; c = -1;}
		}
		while (mm!=NULL && mm->d->buffer==buf) {
			OLCX_ALLOC(rr, S_reference);
			FILL_position(&rr->p, buf->ftnum, ln, 0);
			FILL_reference(rr, mm->usg, rr->p, res);
			res = rr;
			mm = mm->next;
		}
	}
	LIST_MERGE_SORT(S_reference, res, olcxReferenceInternalLessFunction);
	return(res);
}

void editorFreeRegionListNotMarkers(S_editorRegionList *occs) {
	S_editorRegionList 	*o, *next;
	for(o=occs; o!=NULL; ) {
		next = o->next;
		ED_FREE(o, sizeof(S_editorRegionList));
		o = next;
	}
}

void editorFreeMarkersAndRegionList(S_editorRegionList *occs) {
	S_editorRegionList 	*o, *next;
	for(o=occs; o!=NULL; ) {
		next = o->next;
		editorFreeMarker(o->r.b);
		editorFreeMarker(o->r.e);
		ED_FREE(o, sizeof(S_editorRegionList));
		o = next;
	}
}

void editorFreeMarkerListNotMarkers(S_editorMarkerList *occs) {
	S_editorMarkerList 	*o, *next;
	for(o=occs; o!=NULL; ) {
		next = o->next;
		ED_FREE(o, sizeof(S_editorMarkerList));
		o = next;
	}
}

void editorFreeMarkersAndMarkerList(S_editorMarkerList *occs) {
	S_editorMarkerList 	*o, *next;
	for(o=occs; o!=NULL; ) {
		next = o->next;
		editorFreeMarker(o->d);
		ED_FREE(o, sizeof(S_editorMarkerList));
		o = next;
	}
}

void editorDumpMarker(S_editorMarker *mm) {
	sprintf(tmpBuff, "[%s:%d] --> %c", simpleFileName(mm->buffer->name), mm->offset, CHAR_ON_MARKER(mm)); ppcGenTmpBuff();
}

void editorDumpMarkerList(S_editorMarkerList *mml) {
	S_editorMarkerList *mm;
	sprintf(tmpBuff, "------------------[[dumping editor markers]]");ppcGenTmpBuff();
	for(mm=mml; mm!=NULL; mm=mm->next) {
		if (mm->d == NULL) {
			sprintf(tmpBuff, "[null]");ppcGenTmpBuff();
		} else {
			sprintf(tmpBuff, "[%s:%d] --> %c", simpleFileName(mm->d->buffer->name), mm->d->offset, CHAR_ON_MARKER(mm->d)); ppcGenTmpBuff();
		}
	}
	sprintf(tmpBuff, "------------------[[dumpend]]\n");ppcGenTmpBuff();
}

void editorDumpRegionList(S_editorRegionList *mml) {
	S_editorRegionList *mm;
	sprintf(tmpBuff,"-------------------[[dumping editor regions]]\n");
	fprintf(dumpOut,"%s\n",tmpBuff);
	//ppcGenTmpBuff();
	for(mm=mml; mm!=NULL; mm=mm->next) {
		if (mm->r.b == NULL || mm->r.e == NULL) {
			sprintf(tmpBuff,"%d: [null]", mm);
			fprintf(dumpOut,"%s\n",tmpBuff);
			//ppcGenTmpBuff();
		} else {
			sprintf(tmpBuff,"%d: [%s: %d - %d] --> %c - %c", mm, simpleFileName(mm->r.b->buffer->name), mm->r.b->offset, mm->r.e->offset, CHAR_ON_MARKER(mm->r.b), CHAR_ON_MARKER(mm->r.e));
			fprintf(dumpOut,"%s\n",tmpBuff);
			//ppcGenTmpBuff();
		}
	}
	sprintf(tmpBuff, "------------------[[dumpend]]\n");	
	fprintf(dumpOut,"%s\n",tmpBuff);
	//ppcGenTmpBuff();
}

void editorDumpUndoList(S_editorUndo *uu) {
	fprintf(dumpOut,"\n\n[undodump] begin\n");
	while (uu!=NULL) {
		switch (uu->operation) {
		case UNDO_REPLACE_STRING:
			sprintf(tmpBuff,"replace string [%s:%d] %d (%d)%s %d", uu->buffer->name, uu->u.replace.offset, uu->u.replace.size, uu->u.replace.str, uu->u.replace.str, uu->u.replace.strlen);
			if (strlen(uu->u.replace.str)!=uu->u.replace.strlen) fprintf(dumpOut,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			fprintf(dumpOut,"%s\n",tmpBuff);
			break;
		case UNDO_RENAME_BUFFER:
			sprintf(tmpBuff,"rename buffer %s %s", uu->buffer->name, uu->u.rename.name);
			fprintf(dumpOut,"%s\n",tmpBuff);
			break;
		case UNDO_MOVE_BLOCK:
			sprintf(tmpBuff,"move block [%s:%d] [%s:%d] size==%d", uu->buffer->name, uu->u.moveBlock.offset, uu->u.moveBlock.dbuffer->name, uu->u.moveBlock.doffset, uu->u.moveBlock.size);
			fprintf(dumpOut,"%s\n",tmpBuff);
			break;
		default:
			error(ERR_INTERNAL,"Unknown operation to undo");
		}
		uu = uu->next;
	}
	fprintf(dumpOut,"[undodump] end\n");
	fflush(dumpOut);
}


void editorMarkersDifferences(S_editorMarkerList **list1, S_editorMarkerList **list2,
							  S_editorMarkerList **diff1, S_editorMarkerList **diff2) {
	S_editorMarkerList *l1, *l2, *ll;
	S_editorMarker *m;
	LIST_MERGE_SORT(S_editorMarkerList, *list1, editorMarkerListLess);
	LIST_MERGE_SORT(S_editorMarkerList, *list2, editorMarkerListLess);
	*diff1 = *diff2 = NULL;
	for(l1 = *list1, l2 = *list2; l1!=NULL && l2!=NULL; ) {
		if (editorMarkerListLess(l1, l2)) {
			m = editorCrNewMarker(l1->d->buffer, l1->d->offset);
			ED_ALLOC(ll, S_editorMarkerList);
			FILL_editorMarkerList(ll, m, l1->usg, *diff1);
			*diff1 = ll;
			l1 = l1->next;
		} else if (editorMarkerListLess(l2, l1)) {
			m = editorCrNewMarker(l2->d->buffer, l2->d->offset);
			ED_ALLOC(ll, S_editorMarkerList);
			FILL_editorMarkerList(ll, m, l2->usg, *diff2);
			*diff2 = ll;
			l2 = l2->next;
		} else {
			l1 = l1->next; l2 = l2->next;
		}
	}
	while (l1 != NULL) {
		m = editorCrNewMarker(l1->d->buffer, l1->d->offset);
		ED_ALLOC(ll, S_editorMarkerList);
		FILL_editorMarkerList(ll, m, l1->usg, *diff1);
		*diff1 = ll;
		l1 = l1->next;		
	}
	while (l2 != NULL) {
		m = editorCrNewMarker(l2->d->buffer, l2->d->offset);
		ED_ALLOC(ll, S_editorMarkerList);
		FILL_editorMarkerList(ll, m, l2->usg, *diff2);
		*diff2 = ll;
		l2 = l2->next;
	}
}

void editorSortRegionsAndRemoveOverlaps(S_editorRegionList **regions) {
	S_editorRegionList 	*rr, *rrr;
	S_editorMarker		*newend;
	LIST_MERGE_SORT(S_editorRegionList, *regions, editorRegionListLess);
	for(rr= *regions; rr!=NULL; rr=rr->next) {
	contin:
		rrr = rr->next;
		if (rrr!=NULL && rr->r.b->buffer==rrr->r.b->buffer) {
			assert(rr->r.b->buffer == rr->r.e->buffer);  // region consistency check
			assert(rrr->r.b->buffer == rrr->r.e->buffer);  // region consistency check
			assert(rr->r.b->offset <= rrr->r.b->offset);
			newend = NULL;
			if (rrr->r.e->offset <= rr->r.e->offset) {
				// second inside first
				newend = rr->r.e;
				editorFreeMarker(rrr->r.b);
				editorFreeMarker(rrr->r.e);
			} else if (rrr->r.b->offset <= rr->r.e->offset) {
				// they have common part
				newend = rrr->r.e;
				editorFreeMarker(rrr->r.b);
				editorFreeMarker(rr->r.e);
			}
			if (newend!=NULL) {
				rr->r.e = newend;
				rr->next = rrr->next;
				ED_FREE(rrr, sizeof(S_editorRegionList));
				rrr = NULL;
				goto contin;
			}
		}
	}
}

void editorSplitMarkersWithRespectToRegions(
	S_editorMarkerList 	**inMarkers, 
	S_editorRegionList 	**inRegions,
	S_editorMarkerList 	**outInsiders, 
	S_editorMarkerList 	**outOutsiders
	) {
	S_editorMarkerList 	*mm, *nn, *nextmm;
	S_editorRegionList 	*rr;

	*outInsiders = NULL;
	*outOutsiders = NULL;
 
	LIST_MERGE_SORT(S_editorMarkerList, *inMarkers, editorMarkerListLess);
	editorSortRegionsAndRemoveOverlaps(inRegions);

	LIST_REVERSE(S_editorRegionList, *inRegions);
	LIST_REVERSE(S_editorMarkerList, *inMarkers);

//&editorDumpRegionList(*inRegions);
//&editorDumpMarkerList(*inMarkers);

	rr = *inRegions;
	mm= *inMarkers;
	while (mm!=NULL) {
		nn = mm->next;
		while (rr!=NULL && editorMarkerGreater(rr->r.b, mm->d)) rr = rr->next;
		if (rr!=NULL && editorMarkerGreater(rr->r.e, mm->d)) {
			// is inside
			mm->next = *outInsiders;
			*outInsiders = mm;
		} else {
			// is outside
			mm->next = *outOutsiders;
			*outOutsiders = mm;
		}
		mm = nn;
	}

	*inMarkers = NULL;
	LIST_REVERSE(S_editorRegionList, *inRegions);
	LIST_REVERSE(S_editorMarkerList, *outInsiders);
	LIST_REVERSE(S_editorMarkerList, *outOutsiders);
//&editorDumpMarkerList(*outInsiders);
//&editorDumpMarkerList(*outOutsiders);
}

void editorRestrictMarkersToRegions(S_editorMarkerList **mm, S_editorRegionList **regions) {
	S_editorMarkerList *ins, *outs;
	editorSplitMarkersWithRespectToRegions(mm, regions, &ins, &outs);
	*mm = ins;
	editorFreeMarkersAndMarkerList(outs);
}

S_editorMarker *editorCrMarkerForBufferBegin(S_editorBuffer *buffer) {
	return(editorCrNewMarker(buffer,0));
}

S_editorMarker *editorCrMarkerForBufferEnd(S_editorBuffer *buffer) {
	return(editorCrNewMarker(buffer,buffer->a.bufferSize));
}

S_editorRegionList *editorWholeBufferRegion(S_editorBuffer *buffer) {
	S_editorMarker		*bufferBegin, *bufferEnd;
	S_editorRegionList	*theBufferRegion;

	bufferBegin = editorCrMarkerForBufferBegin(buffer);
	bufferEnd = editorCrMarkerForBufferEnd(buffer);
	ED_ALLOC(theBufferRegion, S_editorRegionList);
	FILLF_editorRegionList(theBufferRegion, bufferBegin, bufferEnd, NULL);
	return(theBufferRegion);
}

void editorScheduleModifiedBuffersToUpdate() {
	int 					i;
	S_editorBufferList 		*ll;
	for(i=0; i<s_editorBufferTab.size; i++) {
		for(ll=s_editorBufferTab.tab[i]; ll!=NULL; ll=ll->next) {
			if (ll->f->b.modified) {
//&sprintf(tmpBuff,"scheduling %s == %s to update\n", ll->f->name, s_fileTab.tab[ll->f->ftnum]->name);ppcGenRecord(PPC_IGNORE, tmpBuff,"\n");
				s_fileTab.tab[ll->f->ftnum]->b.scheduledToUpdate = 1;
			}
		}
	}
}

static S_editorBufferList *editorComputeAllBuffersList() {
	int 				i;
	S_editorBufferList	*ll, *rr, *res;
	res = NULL;
	for(i=0; i<s_editorBufferTab.size; i++) {
		for(ll=s_editorBufferTab.tab[i]; ll!=NULL; ll=ll->next) {
			ED_ALLOC(rr, S_editorBufferList);
			FILL_editorBufferList(rr, ll->f, res);
			res = rr;
		}
	}
	return(res);
}

static void editorFreeBufferListButNotBuffers(S_editorBufferList *list) {
	S_editorBufferList *ll, *nn;
	ll=list; 
	while (ll!=NULL) {
		nn = ll->next;
		ED_FREE(ll, sizeof(S_editorBufferList));
		ll = nn;
	}
}

static int editorBufferNameLess(S_editorBufferList*l1,S_editorBufferList*l2) {
	return(strcmp(l1->f->name, l2->f->name));
}

// TODO, do all this stuff better!
// This is still quadratic on number of opened buffers 
// for recursive search
int editorMapOnNonexistantFiles(
		char *dirname,
		void (*fun)(MAP_FUN_PROFILE),
		int deep,
		char *a1,
		char *a2,
		S_completions *a3,
		void *a4,
		int *a5
	) {
	int 					i, dlen, fnlen, res, lastMappedLen;
	S_editorBufferList 		*ll, *bl;
	char					*ss, *lastMapped;
	char					fname[MAX_FILE_NAME_SIZE];
	struct stat				st;
	// In order to avoid mapping of the same directory several 
	// times, first just create list of all files, sort it, and then
	// map them
	res = 0;
	dlen = strlen(dirname);
	bl = editorComputeAllBuffersList();
	LIST_MERGE_SORT(S_editorBufferList, bl, editorBufferNameLess);
	ll = bl;
//&sprintf(tmpBuff, "ENTER!!!"); ppcGenRecord(PPC_IGNORE,tmpBuff,"\n");
	while(ll!=NULL) {
		if (fnnCmp(ll->f->name, dirname, dlen)==0 
			&& (ll->f->name[dlen]=='/' || ll->f->name[dlen]=='\\')) {
			if (deep == DEEP_ONE) {
				ss = strchr(ll->f->name+dlen+1, '/');
				if (ss==NULL) ss = strchr(ll->f->name+dlen+1, '\\');
				if (ss==NULL) {
					strcpy(fname, ll->f->name+dlen+1);
					fnlen = strlen(fname);
				} else {
					fnlen = ss-(ll->f->name+dlen+1);
					strncpy(fname, ll->f->name+dlen+1, fnlen);
					fname[fnlen]=0;
				}
			} else {
				strcpy(fname, ll->f->name+dlen+1);
				fnlen = strlen(fname);
			}
			// check if file exists, map only nonexistant
			if (stat(ll->f->name, &st)!=0) {
				// get file name
//&sprintf(tmpBuff, "MAPPING %s as %s in %s", ll->f->name, fname, dirname); ppcGenRecord(PPC_IGNORE,tmpBuff,"\n");
				(*fun)(fname, a1, a2, a3, a4, a5);
				res = 1;
				// skip all files in the same directory
				lastMapped = ll->f->name;
				lastMappedLen = dlen+1+fnlen;
				ll = ll->next;
				while (ll!=NULL 
					   && fnnCmp(ll->f->name, lastMapped, lastMappedLen)==0 
					   && (ll->f->name[lastMappedLen]=='/' || ll->f->name[lastMappedLen]=='\\')) {
//&sprintf(tmpBuff, "SKIPPING %s", ll->f->name); ppcGenRecord(PPC_IGNORE,tmpBuff,"\n");
					ll = ll->next;
				}
			} else {
				ll = ll->next;
			}
		} else {
			ll = ll->next;
		}
	}
	editorFreeBufferListButNotBuffers(bl);
//&sprintf(tmpBuff, "QUIT!!!"); ppcGenRecord(PPC_IGNORE,tmpBuff,"\n");
	return(res);
}

static void editorCloseBuffer(S_editorBufferList *memb, int ii) {
	S_editorBufferList **ll;
//&sprintf(tmpBuff,"closing buffer %s %s\n", memb->f->name, memb->f->fileName);ppcGenRecord(PPC_IGNORE, tmpBuff, "\n");
	for(ll= &s_editorBufferTab.tab[ii]; (*ll)!=NULL; ll = &(*ll)->next) {
		if (*ll == memb) break;
	}
	if (*ll == memb) {
		// O.K. now, free the buffer
		*ll = (*ll)->next;
		editorFreeBuffer(memb);
	}
}

#define BUFFER_IS_CLOSABLE(buffer) (\
	buffer->b.textLoaded\
	&& buffer->markers==NULL\
	&& buffer->name==buffer->fileName  /* not -preloaded */\
	&& ! buffer->b.modified\
)

// be very carefull when using this function, because of interpretation 
// of 'Closable', this should be additional field: 'closable' or what
void editorCloseBufferIfClosable(char *name) {
	S_editorBuffer 		dd;
	S_editorBufferList 	ddl, *memb, **ll;
	int					ii;
	fillEmptyEditorBuffer(&dd, name, 0, name);
	FILL_editorBufferList(&ddl, &dd, NULL);
	if (editorBufferTabIsMember(&s_editorBufferTab, &ddl, &ii, &memb)) {
		if (BUFFER_IS_CLOSABLE(memb->f)) {
			editorCloseBuffer(memb, ii);
		}
	}
}

void editorCloseAllBuffersIfClosable() {
	int 					i;
	S_editorBufferList 		*ll,*nn;
	S_editorMarker 			*m, *mm;
	for(i=0; i<s_editorBufferTab.size; i++) {
		for(ll=s_editorBufferTab.tab[i]; ll!=NULL;) {
			nn = ll->next;
//& fprintf(dumpOut, "closable %d for %s(%d) %s(%d)\n", BUFFER_IS_CLOSABLE(ll->f), ll->f->name, ll->f->name, ll->f->fileName, ll->f->fileName);fflush(dumpOut);
			if (BUFFER_IS_CLOSABLE(ll->f)) editorCloseBuffer(ll, i);
			ll = nn;
		}
	}
}

void editorCloseAllBuffers() {
	int 					i;
	S_editorBufferList 		*ll,*nn;
	S_editorMarker 			*m, *mm;
	for(i=0; i<s_editorBufferTab.size; i++) {
		for(ll=s_editorBufferTab.tab[i]; ll!=NULL;) {
			nn = ll->next;
			editorFreeBuffer(ll);
			ll = nn;
		}
		s_editorBufferTab.tab[i] = NULL;
	}
}

void editorTest() {
	S_editorBuffer *buff;
}


