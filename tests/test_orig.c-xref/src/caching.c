/*
	$Revision: 1.5 $
	$Date: 2002/09/14 17:27:02 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "protocol.h"
//

int testFileModifTime(int ii) {
	struct stat fst;
	int res;
	time_t t0;
	assert(s_fileTab.tab[ii] != NULL);
	t0 = time(NULL);
	if (s_fileTab.tab[ii]->lastInspect >= s_fileProcessStartTime
		&& s_fileTab.tab[ii]->lastInspect <= t0) {
		/* not supposing files can change during one execution */
		res = 1;
		goto fini;
	}
	if (statb(s_fileTab.tab[ii]->name, &fst)) {
		res = 0;
		goto fini;
	}
	s_fileTab.tab[ii]->lastInspect = t0;
	if (fst.st_mtime == s_fileTab.tab[ii]->lastModif) {
		res = 1;
	} else {
		s_fileTab.tab[ii]->lastModif = fst.st_mtime;
		res = 0;
	}
fini:
	return(res);
}


static void deleteReferencesOutOfMemory(S_reference **rr) {
	while (*rr!=NULL) {
		if (DM_FREED_POINTER(cxMemory,*rr)) {
/*fprintf(dumpOut,"deleting reference on %s:%d\n",s_fileTab.tab[(*rr)->p.file]->name,(*rr)->p.line);*/
			*rr = (*rr)->next;
			continue;
		}
		rr= &(*rr)->next;
	}
}

static void cxrefTabDeleteOutOfMemory(int i) {
	S_symbolRefItem **pp;
	pp = &s_cxrefTab.tab[i]; 
	while (*pp!=NULL) {
		if (DM_FREED_POINTER(cxMemory,*pp)) {
			/* out of memory, delete it */
/*fprintf(dumpOut,"deleting all references on %s\n",(*pp)->name);*/
			*pp = (*pp)->next;
			continue;
		} else {
			/* in memory, examine all refs */
			deleteReferencesOutOfMemory(&(*pp)->refs);
		}
		pp= &(*pp)->next;
	}
}

static void fileTabDeleteOutOfMemory(S_fileItem *p, int i) {
	S_chReference	**hh;
	hh = &p->sups;
	while (*hh!=NULL) {
		if (DM_FREED_POINTER(cxMemory,*hh)) {
            *hh = (*hh)->next;
            goto contlabel;
#if ZERO
		} else {
			if (((char*)*hh)< ((char*)&cxMemory->b)||((char*)*hh)>= ((char*)&cxMemory->b)+cxMemory->i){
				fprintf(dumpOut,"!!!!!!keeping outsider %x: %s\n",*hh,p->name);
				fflush(dumpOut);
			}
#endif
		}
		hh= &(*hh)->next;
	contlabel:;
	}
	hh = &p->infs;
	while (*hh!=NULL) {
		if (DM_FREED_POINTER(cxMemory,*hh)) {
            *hh = (*hh)->next;
            goto contlabel2;
		}
		hh= &(*hh)->next;
	contlabel2:;
	}
}

#define MEM_FREED_POINTER(ppp) (\
	((char*)ppp) >= memory+s_topBlock->firstFreeIndex && \
	((char*)ppp) < memory+SIZE_workMemory\
)

static void structCachingFree(S_symbol *pp) {
	S_symbolList **tp;
	assert(pp->u.s);
	if (MEM_FREED_POINTER(pp->u.s->records) || 
		SM_FREED_POINTER(ppmMemory,pp->u.s->records)) {
		pp->u.s->records = NULL;
	}
	if (MEM_FREED_POINTER(pp->u.s->casts.node) || 
		SM_FREED_POINTER(ppmMemory,pp->u.s->casts.node)) {
		pp->u.s->casts.node = NULL;
	}
	if (MEM_FREED_POINTER(pp->u.s->casts.sub) || 
		SM_FREED_POINTER(ppmMemory,pp->u.s->casts.sub)) {
		pp->u.s->casts.sub = NULL;
	}

	tp = &pp->u.s->super;
	while (*tp!=NULL) {
		if (MEM_FREED_POINTER(*tp) || 
			SM_FREED_POINTER(ppmMemory,*tp)) {
			*tp = (*tp)->next;
            goto contlabel;
		}
		tp = &(*tp)->next;
	contlabel:;
	}
}

static void symTabDeleteOutOfMemory(int i) {
	S_symbol **pp;
	pp = &s_symTab->tab[i];
	while (*pp!=NULL) {
/*
fprintf(dumpOut,"free *%x == %s %s, %x\n",*pp,typesName[(*pp)->symType],
(*pp)->linkName,(*pp)->next);
fflush(dumpOut);
*/
		switch ((*pp)->b.symType) {
		case TypeMacro:
			if (SM_FREED_POINTER(ppmMemory,*pp)) {
				*pp = (*pp)->next;  continue;
			}
			break;
		case TypeStruct: case TypeUnion:
			if (MEM_FREED_POINTER(*pp) || SM_FREED_POINTER(ppmMemory,*pp)) {
				*pp = (*pp)->next;  continue;
			} else {
				structCachingFree(*pp);
			}
			break;
		case TypeEnum:
			if (MEM_FREED_POINTER(*pp)) {
				*pp = (*pp)->next;  continue;
			} else if (MEM_FREED_POINTER((*pp)->u.enums)) {
				(*pp)->u.enums = NULL;
			}
			break;
		default:
			if (MEM_FREED_POINTER(*pp)) {
				*pp = (*pp)->next;  continue;
			}
			break;
		}
		pp= &(*pp)->next;
	}	
}

static void javaFqtTabDeleteOutOfMemory(int i) {
	S_symbolList **pp;
	pp = &s_javaFqtTab.tab[i];
	while (*pp!=NULL) {
		if (SM_FREED_POINTER(ppmMemory,*pp)) {
				*pp = (*pp)->next;  continue;
		} else if (MEM_FREED_POINTER((*pp)->d)
					|| SM_FREED_POINTER(ppmMemory,(*pp)->d)) {
				*pp = (*pp)->next;  continue;
		} else {
			structCachingFree((*pp)->d);
		}
		pp= &(*pp)->next;
	}	
}

static void trailDeleteOutOfMemory() {
	S_freeTrail **pp;
	pp = & s_topBlock->trail;
	while (MEM_FREED_POINTER(*pp)) {
		*pp = (*pp)->next;
	}
}

static void includeListDeleteOutOfMemory() {
	S_stringList **pp;
	pp = & s_opt.includeDirs;
	while (*pp!=NULL) {
		if (SM_FREED_POINTER(ppmMemory,*pp)) {
			*pp = (*pp)->next;  continue;
		} 
		pp= &(*pp)->next;
	}	
}

static int cachedIncludedFilePass(int cpi) {
	int i,mi,mt;
	assert (cpi > 0);
	mi = s_cache.cp[cpi].ibi;
	for(i=s_cache.cp[cpi-1].ibi; i<mi; i++) {
		mt = testFileModifTime(s_cache.ib[i]);
/*&
fprintf(dumpOut,"mtime of %s eval to %d\n",
s_fileTab.tab[s_cache.ib[i]]->name,mt);
&*/
		if (mt == 0) return(0);
	}
	return(1);
}

void recoverCxMemory(char *cxMemFreeBase) {
	CX_FREE_UNTIL(cxMemFreeBase);
	idTabMap3(&s_fileTab, fileTabDeleteOutOfMemory);
	refTabMap3(&s_cxrefTab, cxrefTabDeleteOutOfMemory);
}

// before allowing it, fix problem when modifying .xrefrc during run!!!!
#define CACHING_CLASSES 1
#define CAN_CONTINUE_CACHING_CLASSES(cp) (\
	CACHING_CLASSES\
	&& LANGUAGE(LAN_JAVA)\
	&& (s_opt.taskRegime == RegimeXref || s_opt.taskRegime == RegimeHtmlGenerate)\
	&& ppmMemoryi < (SIZE_ppmMemory/3)*2\
)


void recoverCachePointZero() {
	//&if (CACHING_CLASSES) {
		ppmMemoryi = s_cache.cp[0].ppmMemoryi;
	//&}
	recoverCachePoint(0,s_cache.cp[0].lbcc,0);
}

void recoverMemoriesAfterOverflow(char *cxMemFreeBase) {
	recoverCxMemory(cxMemFreeBase);
	initAllInputs();
	recoverCachePointZero();
}

void recoverCachePoint(int i, char *readedUntil, int activeCaching) {
	S_cachePoint *cp;
//&fprintf(dumpOut,"recovering cache point %d\n",i); fflush(dumpOut);
	cp = &s_cache.cp[i];
	if (! CAN_CONTINUE_CACHING_CLASSES(cp)) {
		ppmMemoryi = cp->ppmMemoryi;
		//& if (CACHING_CLASSES) fprintf(dumpOut, "\nflushing classes\n\n");
	}
	mbMemoryi = cp->mbMemoryi;
	s_topBlock = cp->topBlock;
	tmpWorkMemoryi = 0;
	*s_topBlock = cp->starTopBlock;
	s_javaStat = cp->javaCached;
	s_count = cp->counts;
	trailDeleteOutOfMemory();
	assert(s_opt.taskRegime);
	if (s_opt.taskRegime==RegimeEditServer && s_currCppPass==1) {
		/* remove old references, only on first pass of edit server */
//&fprintf(dumpOut,":removing references !!!!!!!!\n",i); fflush(dumpOut);
		cxMemory->i = cp->cxMemoryi;
		refTabMap3(&s_cxrefTab, cxrefTabDeleteOutOfMemory);
		idTabMap3(&s_fileTab, fileTabDeleteOutOfMemory);
	}
/*fprintf(dumpOut,"recovering 0\n"); fflush(dumpOut);*/
	symTabMap3(s_symTab, symTabDeleteOutOfMemory);
/*fprintf(dumpOut,"recovering 1\n"); fflush(dumpOut);*/
	javaFqtTabMap3(&s_javaFqtTab, javaFqtTabDeleteOutOfMemory);
/*fprintf(dumpOut,"recovering 2\n"); fflush(dumpOut);*/
	/*& idTabMap3(&s_fileTab, fileTabDeleteOutOfMemory); &*/
	// do not forget that includes are listed in PP_MEMEORY too.
	includeListDeleteOutOfMemory();

	cFile.lineNumber = cp->lineNumber;
	cFile.ifDeep = cp->ifDeep;
	cFile.ifstack = cp->ifstack;
	FILL_lexInput(&cInput, cp->lbcc, readedUntil, s_cache.lb, NULL, II_CACHE);
	FILL_caching(&s_cache, 
				activeCaching,
				i+1, 
				cp->ibi,
				cp->lbcc,
				cInput.cc, 
				cInput.cc, 
				cInput.fin
			);
/*fprintf(dumpOut,"recovering 3\n"); fflush(dumpOut);*/
}

/* ******************************************************************* */
/*                         recover from cache                          */
/* ******************************************************************* */

void recoverFromCache() {
	int i;
	char *readedUntil;
	assert(s_cache.cpi >= 1);
	s_cache.activeCache = 0;
/*	s_cache.recoveringFromCache = 1;*/
	DPRINTF1(": reading from cache\n");
	readedUntil = s_cache.cp[0].lbcc;
	for(i=1; i<s_cache.cpi; i++) {
/*fprintf(dumpOut,"try to recover cache point %d\n",i);fflush(dumpOut);*/
		if (cachedInputPass(i,&readedUntil) == 0) break;
		if (cachedIncludedFilePass(i) == 0) break;
	}
	assert(i > 1);
	/* now, recover state from the cache point 'i-1' */
	DPRINTF2("recovering cache point %d\n",i-1);
	recoverCachePoint(i-1, readedUntil, 1);
}

void initCaching() {
	FILL_caching(&s_cache, 1, 0, 0, s_cache.lb, cFile.lb.cc, NULL,NULL);
	poseCachePoint(0);
	s_cache.activeCache = 0;
}

/* ****************************************************************** */
/*        caching of input from 's_cache.lexcc' to 'cInput.cc'       */
/* ****************************************************************** */

void cacheInput() {
	int size;
/*fprintf(dumpOut,"enter cacheInput\n");*/
	if (s_cache.activeCache == 0) return;
	if (inStacki != 0 || macStacki != 0) return;
	size = cInput.cc - s_cache.lexcc;
	if ( s_cache.lbcc - s_cache.lb + size >= LEX_BUF_CACHE_SIZE) {
		s_cache.activeCache = 0;
		return;
	}
	/* if from cache, don't copy on the same place */
	if (cInput.margExpFlag != II_CACHE) memcpy(s_cache.lbcc, s_cache.lexcc, size);
	s_cache.lbcc += size;	
	s_cache.lexcc = cInput.cc;
}

void cacheInclude(int fileNum) {
	if (s_cache.activeCache == 0) return;
/*
fprintf(dumpOut,"caching include of file %d: %s\n",
s_cache.ibi, s_fileTab.tab[fileNum]->name);
*/
	testFileModifTime(fileNum);
	assert(s_cache.ibi < INCLUDE_CACHE_SIZE);
	s_cache.ib[s_cache.ibi] = fileNum;
	s_cache.ibi ++;
	if (s_cache.ibi >= INCLUDE_CACHE_SIZE) s_cache.activeCache = 0;
}

void poseCachePoint(int inputCaching) {
	struct cachePoint *pp;
	if (s_cache.activeCache == 0) return;
	if (inStacki != 0 || macStacki != 0) return;
	if (s_cache.cpi >= MAX_CACHE_POINTS) {
		s_cache.activeCache = 0;
		return;
	}
	if (inputCaching) cacheInput();
	if (s_cache.activeCache == 0) return;
	if (tmpWorkMemoryi != 0) return; /* something in non cached tmp memory */
	pp = &s_cache.cp[s_cache.cpi];
	DPRINTF2("posing cache point %d \n",s_cache.cpi);
//&fprintf(dumpOut,"posing cache point %d\n",s_cache.cpi);
	FILL_cachePoint(pp, s_topBlock, *s_topBlock,
						ppmMemoryi, cxMemory->i, mbMemoryi, 
						s_cache.lbcc, s_cache.ibi, 
					cFile.lineNumber, cFile.ifDeep, cFile.ifstack,
						s_javaStat, s_count
					);
	s_cache.cpi ++;
}



