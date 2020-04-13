#include "caching.h"

#include "commons.h"
#include "globals.h"
#include "protocol.h"
#include "yylex.h"
#include "editor.h"
#include "reftab.h"
#include "javafqttab.h"
#include "classhierarchy.h"
#include "jsemact.h"
#include "filedescriptor.h"

#include "log.h"

S_caching s_cache;


int checkFileModifiedTime(int fileIndex) {
    struct stat fst;
    int res;
    time_t t0;

    assert(s_fileTab.tab[fileIndex] != NULL);
    t0 = time(NULL);
    if (s_fileTab.tab[fileIndex]->lastInspected >= s_fileProcessStartTime
        && s_fileTab.tab[fileIndex]->lastInspected <= t0) {
        /* not supposing files can change during one execution */
        res = 1;
        goto end;
    }
    if (statb(s_fileTab.tab[fileIndex]->name, &fst)) {
        res = 0;
        goto end;
    }
    s_fileTab.tab[fileIndex]->lastInspected = t0;
    if (fst.st_mtime == s_fileTab.tab[fileIndex]->lastModified) {
        res = 1;
    } else {
        s_fileTab.tab[fileIndex]->lastModified = fst.st_mtime;
        res = 0;
    }
 end:
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
    S_chReference   **hh;
    hh = &p->superClasses;
    while (*hh!=NULL) {
        if (DM_FREED_POINTER(cxMemory,*hh)) {
            *hh = (*hh)->next;
            goto contlabel;
        }
        hh= &(*hh)->next;
    contlabel:;
    }
    hh = &p->inferiorClasses;
    while (*hh!=NULL) {
        if (DM_FREED_POINTER(cxMemory,*hh)) {
            *hh = (*hh)->next;
            goto contlabel2;
        }
        hh= &(*hh)->next;
    contlabel2:;
    }
}

#define MEM_FREED_POINTER(ppp) (((char*)ppp) >= memory+s_topBlock->firstFreeIndex && \
                                ((char*)ppp) < memory+SIZE_workMemory)

static void structCachingFree(Symbol *pp) {
    SymbolList **tp;
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

static void symbolTableDeleteOutOfMemory(int i) {
    Symbol **pp;
    pp = &s_symbolTable->tab[i];
    while (*pp!=NULL) {
        /*
          fprintf(dumpOut,"free *%x == %s %s, %x\n",*pp,typeName[(*pp)->symType],
          (*pp)->linkName,(*pp)->next);
          fflush(dumpOut);
        */
        switch ((*pp)->bits.symType) {
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
    SymbolList **pp;
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

static void trailDeleteOutOfMemory(void) {
    S_freeTrail **pp;
    pp = & s_topBlock->trail;
    while (MEM_FREED_POINTER(*pp)) {
        *pp = (*pp)->next;
    }
}

static void includeListDeleteOutOfMemory(void) {
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
        mt = checkFileModifiedTime(s_cache.ib[i]);
        /*&
          fprintf(dumpOut,"mtime of %s eval to %d\n",
          s_fileTab.tab[s_cache.ib[i]]->name,mt);
          &*/
        if (mt == 0) return(0);
    }
    return(1);
}

static void recoverCxMemory(char *cxMemFreeBase) {
    CX_FREE_UNTIL(cxMemFreeBase);
    fileTabMapWithIndex(&s_fileTab, fileTabDeleteOutOfMemory);
    refTabMap3(&s_cxrefTab, cxrefTabDeleteOutOfMemory);
}

static void fillCaching(S_caching *caching,
                         char activeCache,
                         int cpi,
                         int ibi,
                         char *lbcc,
                         char *lexcc,
                         char *cc,
                         char *cfin
                         ) {
    caching->activeCache = activeCache;
    caching->cpi = cpi;
    caching->ibi = ibi;
    caching->lbcc = lbcc;
    caching->lexcc = lexcc;
    caching->cc = cc;
    caching->cfin = cfin;
}


// before allowing it, fix problem when modifying .xrefrc during run!!!!
#define CACHING_CLASSES 1
#define CAN_CONTINUE_CACHING_CLASSES(cp) (                              \
                                          CACHING_CLASSES               \
                                          && LANGUAGE(LANG_JAVA)         \
                                          && (s_opt.taskRegime == RegimeXref || s_opt.taskRegime == RegimeHtmlGenerate) \
                                          && ppmMemoryi < (SIZE_ppmMemory/3)*2 \
                                                                        )


void recoverCachePointZero(void) {
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

void recoverCachePoint(int i, char *readUntil, int activeCaching) {
    S_cachePoint *cp;

    log_trace("recovering cache point %d", i);
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
        log_trace("removing references");
        cxMemory->i = cp->cxMemoryi;
        refTabMap3(&s_cxrefTab, cxrefTabDeleteOutOfMemory);
        fileTabMapWithIndex(&s_fileTab, fileTabDeleteOutOfMemory);
    }
    log_trace("recovering 0");
    symbolTableMap3(s_symbolTable, symbolTableDeleteOutOfMemory);
    log_trace("recovering 1");
    javaFqtTabMap3(&s_javaFqtTab, javaFqtTabDeleteOutOfMemory);
    log_trace("recovering 2");

    /*& fileTabMapWithIndex(&s_fileTab, fileTabDeleteOutOfMemory); &*/

    // do not forget that includes are listed in PP_MEMORY too.
    includeListDeleteOutOfMemory();

    cFile.lineNumber = cp->lineNumber;
    cFile.ifDeep = cp->ifDeep;
    cFile.ifStack = cp->ifstack;
    fillLexInput(&cInput, cp->lbcc, readUntil, s_cache.lb, NULL, II_CACHE);
    fillCaching(&s_cache,
                 activeCaching,
                 i+1,
                 cp->ibi,
                 cp->lbcc,
                 cInput.currentLexem,
                 cInput.currentLexem,
                 cInput.endOfBuffer
                 );
    log_trace("finished recovering");
}

/* ******************************************************************* */
/*                         recover from cache                          */
/* ******************************************************************* */

void recoverFromCache(void) {
    int i;
    char *readUntil;

    assert(s_cache.cpi >= 1);
    s_cache.activeCache = 0;
    /*  s_cache.recoveringFromCache = 1;*/
    log_debug("reading from cache");
    readUntil = s_cache.cp[0].lbcc;
    for(i=1; i<s_cache.cpi; i++) {
        log_trace("trying to recover cache point %d", i);
        if (cachedInputPass(i,&readUntil) == 0) break;
        if (cachedIncludedFilePass(i) == 0) break;
    }
    assert(i > 1);
    /* now, recover state from the cache point 'i-1' */
    log_debug("recovering cache point %d", i-1);
    recoverCachePoint(i-1, readUntil, 1);
}

void setupCaching(void) {
    fillCaching(&s_cache,0,0,0,NULL,NULL,NULL,NULL);
}

void initCaching(void) {
    fillCaching(&s_cache, 1, 0, 0, s_cache.lb, cFile.lexBuffer.next, NULL,NULL);
    placeCachePoint(0);
    s_cache.activeCache = 0;
}

/* ****************************************************************** */
/*        caching of input from 's_cache.lexcc' to 'cInput.cc'       */
/* ****************************************************************** */

void cacheInput(void) {
    int size;
    /*fprintf(dumpOut,"enter cacheInput\n");*/
    if (s_cache.activeCache == 0) return;
    if (inStacki != 0 || macroStackIndex != 0) return;
    size = cInput.currentLexem - s_cache.lexcc;
    if ( s_cache.lbcc - s_cache.lb + size >= LEX_BUF_CACHE_SIZE) {
        s_cache.activeCache = 0;
        return;
    }
    /* if from cache, don't copy on the same place */
    if (cInput.margExpFlag != II_CACHE) memcpy(s_cache.lbcc, s_cache.lexcc, size);
    s_cache.lbcc += size;
    s_cache.lexcc = cInput.currentLexem;
}

void cacheInclude(int fileNum) {
    if (s_cache.activeCache == 0) return;
    log_debug("caching include of file %d: %s\n",
              s_cache.ibi, s_fileTab.tab[fileNum]->name);
    checkFileModifiedTime(fileNum);
    assert(s_cache.ibi < INCLUDE_CACHE_SIZE);
    s_cache.ib[s_cache.ibi] = fileNum;
    s_cache.ibi ++;
    if (s_cache.ibi >= INCLUDE_CACHE_SIZE) s_cache.activeCache = 0;
}

static void fillCachePoint(S_cachePoint *cachePoint, S_topBlock *topBlock,
                           S_topBlock starTopBlock, int ppmMemoryi,
                           int cxMemoryi, int mbMemoryi, char *lbcc, short int ibi,
                           short int lineNumber, short int ifDeep, S_cppIfStack *ifstack,
                           S_javaStat *javaCached, S_counters counts) {
    cachePoint->topBlock = topBlock;
    cachePoint->starTopBlock = starTopBlock;
    cachePoint->ppmMemoryi = ppmMemoryi;
    cachePoint->cxMemoryi = cxMemoryi;
    cachePoint->mbMemoryi = mbMemoryi;
    cachePoint->lbcc = lbcc;
    cachePoint->ibi = ibi;
    cachePoint->lineNumber = lineNumber;
    cachePoint->ifDeep = ifDeep;
    cachePoint->ifstack = ifstack;
    cachePoint->javaCached = javaCached;
    cachePoint->counts = counts;
}

void placeCachePoint(int inputCaching) {
    S_cachePoint *pp;
    if (s_cache.activeCache == 0) return;
    if (inStacki != 0 || macroStackIndex != 0) return;
    if (s_cache.cpi >= MAX_CACHE_POINTS) {
        s_cache.activeCache = 0;
        return;
    }
    if (inputCaching) cacheInput();
    if (s_cache.activeCache == 0) return;
    if (tmpWorkMemoryi != 0) return; /* something in non-cached tmp memory */
    pp = &s_cache.cp[s_cache.cpi];
    log_debug("placing cache point %d", s_cache.cpi);
    fillCachePoint(pp, s_topBlock, *s_topBlock,
                    ppmMemoryi, cxMemory->i, mbMemoryi,
                    s_cache.lbcc, s_cache.ibi,
                    cFile.lineNumber, cFile.ifDeep, cFile.ifStack,
                    s_javaStat, s_count
                    );
    s_cache.cpi ++;
}
