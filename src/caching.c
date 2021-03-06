#include "caching.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
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


bool checkFileModifiedTime(int fileIndex) {
    struct stat fst;
    time_t t0;

    assert(fileTable.tab[fileIndex] != NULL);
    t0 = time(NULL);
    if (fileTable.tab[fileIndex]->lastInspected >= s_fileProcessStartTime
        && fileTable.tab[fileIndex]->lastInspected <= t0) {
        /* Assuming that files cannot change during one execution */
        return true;
    }
    if (editorFileStatus(fileTable.tab[fileIndex]->name, &fst)) {
        return true;
    }
    fileTable.tab[fileIndex]->lastInspected = t0;
    if (fst.st_mtime == fileTable.tab[fileIndex]->lastModified) {
        return true;
    } else {
        fileTable.tab[fileIndex]->lastModified = fst.st_mtime;
        return false;
    }
}


static void deleteReferencesOutOfMemory(Reference **rr) {
    while (*rr!=NULL) {
        if (DM_FREED_POINTER(cxMemory,*rr)) {
            log_trace("deleting reference on %s:%d", fileTable.tab[(*rr)->p.file]->name, (*rr)->p.line);
            *rr = (*rr)->next;
            continue;
        }
        rr= &(*rr)->next;
    }
}

static void cxrefTabDeleteOutOfMemory(int i) {
    SymbolReferenceItem **pp;

    pp = &referenceTable.tab[i];
    while (*pp!=NULL) {
        if (DM_FREED_POINTER(cxMemory, *pp)) {
            /* out of memory, delete it */
            log_trace("deleting all references on %s", (*pp)->name);
            *pp = (*pp)->next;  /* Unlink it and look at next */
            continue;
        } else {
            /* in memory, examine all refs */
            deleteReferencesOutOfMemory(&(*pp)->refs);
        }
        pp= &(*pp)->next;
    }
}

static void fileTabDeleteOutOfMemory(FileItem *p, int i) {
    ClassHierarchyReference **hh;
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

static void structCachingFree(Symbol *symbol) {
    SymbolList **superList;
    assert(symbol->u.s);
    if (freedPointer(symbol->u.s->records) ||
        SM_FREED_POINTER(ppmMemory,symbol->u.s->records)) {
        symbol->u.s->records = NULL;
    }
    if (freedPointer(symbol->u.s->casts.node) ||
        SM_FREED_POINTER(ppmMemory,symbol->u.s->casts.node)) {
        symbol->u.s->casts.node = NULL;
    }
    if (freedPointer(symbol->u.s->casts.sub) ||
        SM_FREED_POINTER(ppmMemory,symbol->u.s->casts.sub)) {
        symbol->u.s->casts.sub = NULL;
    }

    superList = &symbol->u.s->super;
    while (*superList!=NULL) {
        if (freedPointer(*superList) ||
            SM_FREED_POINTER(ppmMemory,*superList)) {
            *superList = (*superList)->next;
            goto contlabel;
        }
        superList = &(*superList)->next;
    contlabel:;
    }
}

static void symbolTableDeleteOutOfMemory(int i) {
    Symbol **pp;
    pp = &s_symbolTable->tab[i];
    while (*pp!=NULL) {
        switch ((*pp)->bits.symbolType) {
        case TypeMacro:
            if (SM_FREED_POINTER(ppmMemory,*pp)) {
                *pp = (*pp)->next;
                continue;
            }
            break;
        case TypeStruct:
        case TypeUnion:
            if (freedPointer(*pp) || SM_FREED_POINTER(ppmMemory,*pp)) {
                *pp = (*pp)->next;
                continue;
            } else {
                structCachingFree(*pp);
            }
            break;
        case TypeEnum:
            if (freedPointer(*pp)) {
                *pp = (*pp)->next;
                continue;
            } else if (freedPointer((*pp)->u.enums)) {
                (*pp)->u.enums = NULL;
            }
            break;
        default:
            if (freedPointer(*pp)) {
                *pp = (*pp)->next;
                continue;
            }
            break;
        }
        pp= &(*pp)->next;
    }
}

static void javaFqtTabDeleteOutOfMemory(int i) {
    SymbolList **pp;
    pp = &javaFqtTable.tab[i];
    while (*pp!=NULL) {
        if (SM_FREED_POINTER(ppmMemory,*pp)) {
            *pp = (*pp)->next;
            continue;
        } else if (freedPointer((*pp)->d)
                   || SM_FREED_POINTER(ppmMemory,(*pp)->d)) {
            *pp = (*pp)->next;
            continue;
        } else {
            structCachingFree((*pp)->d);
        }
        pp= &(*pp)->next;
    }
}

static void trailDeleteOutOfMemory(void) {
    FreeTrail **pp;
    pp = &s_topBlock->trail;
    while (freedPointer(*pp)) {
        *pp = (*pp)->next;
    }
}

static void includeListDeleteOutOfMemory(void) {
    StringList **pp;
    pp = & options.includeDirs;
    while (*pp!=NULL) {
        if (SM_FREED_POINTER(ppmMemory,*pp)) {
            *pp = (*pp)->next;
            continue;
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
        log_debug("mtime of %s eval to %d", fileTable.tab[s_cache.ib[i]]->name,mt);
        if (mt == 0) return(0);
    }
    return(1);
}

static void recoverCxMemory(char *cxMemFreeBase) {
    CX_FREE_UNTIL(cxMemFreeBase);
    fileTableMapWithIndex(&fileTable, fileTabDeleteOutOfMemory);
    refTabMap3(&referenceTable, cxrefTabDeleteOutOfMemory);
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
                                          && (options.taskRegime == RegimeXref || options.taskRegime == RegimeHtmlGenerate) \
                                          && ppmMemoryIndex < (SIZE_ppmMemory/3)*2 \
                                                                        )


void recoverCachePointZero(void) {
    //&if (CACHING_CLASSES) {
    ppmMemoryIndex = s_cache.cp[0].ppmMemoryIndex;
    //&}
    recoverCachePoint(0,s_cache.cp[0].lbcc,0);
}

void recoverMemoriesAfterOverflow(char *cxMemFreeBase) {
    recoverCxMemory(cxMemFreeBase);
    initAllInputs();
    recoverCachePointZero();
}

void recoverCachePoint(int i, char *readUntil, int activeCaching) {
    CachePoint *cp;

    log_trace("recovering cache point %d", i);
    cp = &s_cache.cp[i];
    if (! CAN_CONTINUE_CACHING_CLASSES(cp)) {
        ppmMemoryIndex = cp->ppmMemoryIndex;
        //& if (CACHING_CLASSES) fprintf(dumpOut, "\nflushing classes\n\n");
    }
    mbMemoryIndex = cp->mbMemoryIndex;
    s_topBlock = cp->topBlock;
    tmpWorkMemoryIndex = 0;
    *s_topBlock = cp->topBlockContent;
    s_javaStat = cp->javaCached;
    counters = cp->counters;
    trailDeleteOutOfMemory();
    assert(options.taskRegime);
    if (options.taskRegime==RegimeEditServer && currentCppPass==1) {
        /* remove old references, only on first pass of edit server */
        log_trace("removing references");
        cxMemory->index = cp->cxMemoryIndex;
        refTabMap3(&referenceTable, cxrefTabDeleteOutOfMemory);
        fileTableMapWithIndex(&fileTable, fileTabDeleteOutOfMemory);
    }
    log_trace("recovering 0");
    symbolTableMap3(s_symbolTable, symbolTableDeleteOutOfMemory);
    log_trace("recovering 1");
    javaFqtTableMap3(&javaFqtTable, javaFqtTabDeleteOutOfMemory);
    log_trace("recovering 2");

    /*& fileTabMapWithIndex(&fileTable, fileTabDeleteOutOfMemory); &*/

    // do not forget that includes are listed in PP_MEMORY too.
    includeListDeleteOutOfMemory();

    currentFile.lineNumber = cp->lineNumber;
    currentFile.ifDepth = cp->ifDepth;
    currentFile.ifStack = cp->ifStack;
    fillLexInput(&currentInput, cp->lbcc, readUntil, s_cache.lb, NULL, INPUT_CACHE);
    fillCaching(&s_cache,
                activeCaching, i+1, cp->ibi, cp->lbcc,
                currentInput.currentLexemP, currentInput.currentLexemP, currentInput.endOfBuffer);
    log_trace("finished recovering");
}

/* ******************************************************************* */
/*                         recover from cache                          */
/* ******************************************************************* */

void recoverFromCache(void) {
    int i;
    char *readUntil;

    assert(s_cache.cpi >= 1);
    s_cache.activeCache = false;
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
    fillCaching(&s_cache, 1, 0, 0, s_cache.lb, currentFile.lexBuffer.next, NULL,NULL);
    placeCachePoint(false);
    s_cache.activeCache = false;
}

/* ****************************************************************** */
/*        caching of input from 's_cache.lexcc' to 'cInput.cc'       */
/* ****************************************************************** */

void cacheInput(void) {
    int size;

    ENTER();
    if (!s_cache.activeCache) return;
    if (includeStackPointer != 0 || macroStackIndex != 0) return;
    size = currentInput.currentLexemP - s_cache.lexcc;
    if (s_cache.lbcc - s_cache.lb + size >= LEX_BUF_CACHE_SIZE) {
        s_cache.activeCache = false;
        LEAVE();
        return;
    }
    /* if from cache, don't copy on the same place */
    if (currentInput.inputType != INPUT_CACHE)
        memcpy(s_cache.lbcc, s_cache.lexcc, size);
    s_cache.lbcc += size;
    s_cache.lexcc = currentInput.currentLexemP;
    LEAVE();
}

void cacheInclude(int fileNum) {
    if (!s_cache.activeCache)
        return;
    log_debug("caching include of file %d: %s",
              s_cache.ibi, fileTable.tab[fileNum]->name);
    checkFileModifiedTime(fileNum);
    assert(s_cache.ibi < INCLUDE_CACHE_SIZE);
    s_cache.ib[s_cache.ibi] = fileNum;
    s_cache.ibi ++;
    if (s_cache.ibi >= INCLUDE_CACHE_SIZE)
        s_cache.activeCache = false;
}

static void fillCachePoint(CachePoint *cachePoint, TopBlock *topBlock, int ppmMemoryIndex,
                           int cxMemoryIndex, int mbMemoryIndex, char *lbcc, short int ibi,
                           short int lineNumber, short int ifDepth, S_cppIfStack *ifStack,
                           S_javaStat *javaCached, Counters counters) {
    cachePoint->topBlock = topBlock;
    cachePoint->topBlockContent = *topBlock;
    cachePoint->ppmMemoryIndex = ppmMemoryIndex;
    cachePoint->cxMemoryIndex = cxMemoryIndex;
    cachePoint->mbMemoryIndex = mbMemoryIndex;
    cachePoint->lbcc = lbcc;
    cachePoint->ibi = ibi;
    cachePoint->lineNumber = lineNumber;
    cachePoint->ifDepth = ifDepth;
    cachePoint->ifStack = ifStack;
    cachePoint->javaCached = javaCached;
    cachePoint->counters = counters;
}

void placeCachePoint(bool inputCaching) {
    CachePoint *pp;
    if (!s_cache.activeCache)
        return;
    if (includeStackPointer != 0 || macroStackIndex != 0)
        return;
    if (s_cache.cpi >= MAX_CACHE_POINTS) {
        s_cache.activeCache = false;
        return;
    }
    if (inputCaching)
        cacheInput();
    if (!s_cache.activeCache)
        return;
    if (tmpWorkMemoryIndex != 0)
        return; /* something in non-cached tmp memory */
    pp = &s_cache.cp[s_cache.cpi];
    log_debug("placing cache point %d", s_cache.cpi);
    fillCachePoint(pp, s_topBlock, ppmMemoryIndex, cxMemory->index, mbMemoryIndex, s_cache.lbcc,
                   s_cache.ibi, currentFile.lineNumber, currentFile.ifDepth,
                   currentFile.ifStack, s_javaStat, counters);
    s_cache.cpi ++;
}
