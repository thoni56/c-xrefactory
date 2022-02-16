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
#include "filetable.h"

#include "log.h"

S_caching s_cache;


bool checkFileModifiedTime(int fileIndex) {
    time_t now;

    assert(fileTable.tab[fileIndex] != NULL);
    now = time(NULL);
    if (fileTable.tab[fileIndex]->lastInspected >= fileProcessingStartTime
        && fileTable.tab[fileIndex]->lastInspected <= now) {
        /* Assuming that files cannot change during one execution */
        return true;
    }
    if (!editorFileExists(fileTable.tab[fileIndex]->name)) {
        return true;
    }

    fileTable.tab[fileIndex]->lastInspected = now;
    time_t modificationTime = editorFileModificationTime(fileTable.tab[fileIndex]->name);
    if (modificationTime == fileTable.tab[fileIndex]->lastModified) {
        return true;
    } else {
        fileTable.tab[fileIndex]->lastModified = modificationTime;
        return false;
    }
}


static void deleteReferencesOutOfMemory(Reference **rr) {
    while (*rr!=NULL) {
        if (CX_FREED_POINTER(*rr)) {
            log_trace("deleting reference on %s:%d", fileTable.tab[(*rr)->position.file]->name, (*rr)->position.line);
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
        if (CX_FREED_POINTER(*pp)) {
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
        if (CX_FREED_POINTER(*hh)) {
            *hh = (*hh)->next;
            goto contlabel;     /* TODO: continue? */
        }
        hh= &(*hh)->next;       /* TODO: else? */
    contlabel:;
    }
    hh = &p->inferiorClasses;
    while (*hh!=NULL) {
        if (CX_FREED_POINTER(*hh)) {
            *hh = (*hh)->next;
            goto contlabel2;    /* TODO: continue? */
        }
        hh= &(*hh)->next;       /* TODO: else? */
    contlabel2:;
    }
}

static void structCachingFree(Symbol *symbol) {
    SymbolList **superList;
    assert(symbol->u.structSpec);
    if (freedPointer(symbol->u.structSpec->records) ||
        PPM_FREED_POINTER(symbol->u.structSpec->records)) {
        symbol->u.structSpec->records = NULL;
    }
    if (freedPointer(symbol->u.structSpec->casts.node) ||
        PPM_FREED_POINTER(symbol->u.structSpec->casts.node)) {
        symbol->u.structSpec->casts.node = NULL;
    }
    if (freedPointer(symbol->u.structSpec->casts.sub) ||
        PPM_FREED_POINTER(symbol->u.structSpec->casts.sub)) {
        symbol->u.structSpec->casts.sub = NULL;
    }

    superList = &symbol->u.structSpec->super;
    while (*superList!=NULL) {
        if (freedPointer(*superList) ||
            PPM_FREED_POINTER(*superList)) {
            *superList = (*superList)->next;
            goto contlabel;
        }
        superList = &(*superList)->next;
    contlabel:;
    }
}

static void symbolTableDeleteOutOfMemory(int i) {
    Symbol **pp;
    pp = &symbolTable->tab[i];
    while (*pp!=NULL) {
        switch ((*pp)->bits.symbolType) {
        case TypeMacro:
            if (PPM_FREED_POINTER(*pp)) {
                *pp = (*pp)->next;
                continue;
            }
            break;
        case TypeStruct:
        case TypeUnion:
            if (freedPointer(*pp) || PPM_FREED_POINTER(*pp)) {
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
        if (PPM_FREED_POINTER(*pp)) {
            *pp = (*pp)->next;
            continue;
        } else if (freedPointer((*pp)->d)
                   || PPM_FREED_POINTER((*pp)->d)) {
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
    pp = &currentBlock->trail;
    while (freedPointer(*pp)) {
        *pp = (*pp)->next;
    }
}

static void includeListDeleteOutOfMemory(void) {
    StringList **pp;
    pp = & options.includeDirs;
    while (*pp!=NULL) {
        if (PPM_FREED_POINTER(*pp)) {
            *pp = (*pp)->next;
            continue;
        }
        pp= &(*pp)->next;
    }
}

static int cachedIncludedFilePass(int cpi) {
    int mi,mt;
    assert (cpi > 0);
    mi = s_cache.cp[cpi].ibi;
    for (int i=s_cache.cp[cpi-1].ibi; i<mi; i++) {
        mt = checkFileModifiedTime(s_cache.ib[i]);
        log_debug("mtime of %s eval to %d", fileTable.tab[s_cache.ib[i]]->name, mt);
        if (mt == 0)
            return 0;
    }
    return 1;
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
                                          && options.taskRegime == RegimeXref \
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
    currentBlock = cp->topBlock;
    *currentBlock = cp->topBlockContent;
    s_javaStat = cp->javaCached;
    counters = cp->counters;
    trailDeleteOutOfMemory();
    assert(options.taskRegime);
    if (options.taskRegime==RegimeEditServer && currentPass==1) {
        /* remove old references, only on first pass of edit server */
        log_trace("removing references");
        cxMemory->index = cp->cxMemoryIndex;
        refTabMap3(&referenceTable, cxrefTabDeleteOutOfMemory);
        fileTableMapWithIndex(&fileTable, fileTabDeleteOutOfMemory);
    }
    log_trace("recovering 0");
    symbolTableMap3(symbolTable, symbolTableDeleteOutOfMemory);
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
    for (i=1; i<s_cache.cpi; i++) {
        log_trace("trying to recover cache point %d", i);
        if (cachedInputPass(i, &readUntil) == 0)
            break;
        if (cachedIncludedFilePass(i) == 0)
            break;
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
    if (!s_cache.activeCache) {
        log_trace("Caching is not active");
        LEAVE();
        return;
    }
    if (includeStackPointer != 0 || macroStackIndex != 0) {
        log_trace("In include or macro");
        LEAVE();
        return;
    }
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

static void fillCachePoint(CachePoint *cachePoint, CodeBlock *topBlock, int ppmMemoryIndex,
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
    pp = &s_cache.cp[s_cache.cpi];
    log_debug("placing cache point %d", s_cache.cpi);
    fillCachePoint(pp, currentBlock, ppmMemoryIndex, cxMemory->index, mbMemoryIndex, s_cache.lbcc,
                   s_cache.ibi, currentFile.lineNumber, currentFile.ifDepth,
                   currentFile.ifStack, s_javaStat, counters);
    s_cache.cpi ++;
}
