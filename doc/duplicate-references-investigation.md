# Duplicate References Bug Investigation

## LSP Context and Filtering Observations

### LSP Protocol Limitations
Research into LSP clients reveals:

1. **LSP protocol is minimal** - only provides reference locations, no semantic filtering
2. **JetBrains IDEs** - have some filtering but implemented locally on client side (not in protocol)
3. **Most LSP clients** - present simple flat list where "next" is sequential like search results
4. **c-xrefactory's filtering** is advanced but opaque:
   - Uses magic numbers for filter levels (`refsFilterLevel`, `menuFilterLevel`)
   - Filters by usage type (definition, declaration, usage, etc.)
   - UI doesn't make filtering obvious - hard to discover/use
   - Functionality exists but value is unclear without better UX

### Implications for Fix
- **Filtering complicates staleness handling** - session references are filtered subset
- **Index-based approach harder** - filtering changes index mapping between database and session
- **Could simplify** - if filtering is rarely used, might simplify fix to ignore it initially
- **LSP mode doesn't need it** - basic LSP just needs all references in order
- **Custom LSP extensions** - could add filtering but requires custom client/server (defeats LSP purpose)

### Question for Later
Is the filtering system worth preserving in its current complexity? Or simplify for LSP compatibility and better UX?

## Deep Dive: How Filtering Actually Works

### Two Independent Filter Systems

**1. Menu Filtering** (which symbols to show in browser menu)
- Controls which symbols appear in the selection menu
- 3 levels:
  ```c
  MenuFilterAllWithName        = 0  // All symbols with same name
  MenuFilterExactMatch         = 1  // Exact type/storage match
  MenuFilterExactMatchSameFile = 2  // Exact match + same file
  ```
- Uses bit flags: `FILE_MATCH_ANY`, `NAME_MATCH_APPLICABLE`, `FILE_MATCH_RELATED`

**2. Reference Filtering** (which references to navigate through)
- Controls which references you see when navigating with NEXT/PREV
- 4 levels (`cxref.h:19-25`):
  ```c
  ReferenceFilterAll                    = 0  // Show all references
  ReferenceFilterExcludeReads           = 1  // Hide simple reads
  ReferenceFilterExcludeReadsAndAddress = 2  // Also hide address-taken
  ReferenceFilterDefinitionsOnly        = 3  // Only show definitions
  ```

### The Clever Usage Ordering System

From `usage.h:12-30`, usages are **ordered by importance**:
```c
/* Most important */
UsageOLBestFitDefined    // Special "best fit" marker
UsageDefined             // int foo = 5;      (definition)
UsageDeclared            // extern int foo;   (declaration)
/* filter level 2 excludes below here */
UsageLvalUsed            // foo = 10;         (write/lvalue)
/* filter level 1 excludes below here */
UsageAddrUsed            // &foo              (address taken)
/* filter level 0 excludes below here */
UsageUsed                // x = foo;          (simple read)
UsageUndefinedMacro
UsageFork
/* INVISIBLE USAGES - never shown in navigation */
UsageMaxOnLineVisibleUsages
UsageNone
UsageMacroBaseFileUsage
```

**The key insight**: Usages are ordered so filtering can be done by **comparison**: `usage > filterLevel`!

### How Navigation Filtering Works

From `cxref.c:895-911`:
```c
static void setCurrentReferenceToFirstVisible(SessionStackEntry *refs, Reference *r) {
    int rlevel = usageFilterLevels[refs->refsFilterLevel];

    // Skip references that are "at most as important as" the filter level
    while (r!=NULL && isAtMostAsImportantAs(r->usage, rlevel))
        r = r->next;

    if (r != NULL) {
        refs->current = r;
    } else {
        // Wrap around to first visible reference
        ppcBottomInformation("Moving to the first reference");
        r = refs->references;
        while (r!=NULL && isAtMostAsImportantAs(r->usage, rlevel))
            r = r->next;
        refs->current = r;
    }
}
```

**Example**: If user sets `refsFilterLevel = 1` (ExcludeReads):
- Maps to `UsageUsed` in the `usageFilterLevels[]` array
- Navigation skips all references with `usage <= UsageUsed`
- Only navigates to: UsageAddrUsed, UsageLvalUsed, UsageDeclared, UsageDefined

### User Control

Users control filtering via operations:
- `OLO_MENU_FILTER_SET` - set filter to specific level (0-3)
- `OLO_MENU_FILTER_PLUS` - stronger filtering (increase level)
- `OLO_MENU_FILTER_MINUS` - weaker filtering (decrease level)

Implementation just changes the integer:
```c
refs->refsFilterLevel = filterLevel;  // Set to 0, 1, 2, or 3
setCurrentReferenceToFirstVisible(refs, refs->current);  // Jump to next visible
```

### Practical Value

**Use Case 1**: "Show me only where this variable is modified"
- Variable has 100 read usages, 5 write usages
- Set filter to `ReferenceFilterExcludeReads` (level 1)
- Navigate only through the 5 writes, skipping 100 reads

**Use Case 2**: "Where is this function defined?"
- Function has 50 call sites, 1 definition
- Set filter to `ReferenceFilterDefinitionsOnly` (level 3)
- Jump straight to definition

**Use Case 3**: Debug who takes address of variable
- Set filter to `ReferenceFilterExcludeReadsAndAddress` (level 2)
- See only: definitions and lvalue usages (writes)
- Useful for finding who passes pointer to function

### Why It's Not Obvious (UX Problems)

**Problem 1: Magic Numbers**
- User sees "filter level 0, 1, 2, 3"
- No explanation of what each level means
- No visual feedback about what's being filtered

**Problem 2: No Discoverability**
- Feature exists but hidden
- No UI showing "you are viewing 5 of 87 references"
- No indication that references are being filtered

**Problem 3: Unclear Emacs Integration**
- How does user actually change filter level?
- Keybindings? Menu? Command?
- Documentation doesn't explain

### Potential UX Improvements

**Better Labels**:
```
[ ] All references (87)
[ ] Exclude reads (12)
[x] Exclude reads & address (5)  ← you are here
[ ] Definitions only (1)
```

**Status Line Feedback**:
```
References: 5 of 87 visible (filter: writes only) | n/p: next/prev  f: change filter
```

**Quick Toggle Key**:
- `f` - cycle through filter levels
- `a` - show all
- `d` - definitions only

### Assessment for Staleness Fix

**Verdict**: Keep the filtering system
- **Core concept is valuable** - semantic filtering is powerful for large codebases
- **Implementation is sound** - usage ordering is clever and efficient
- **Not a complexity bottleneck** - filtering is simple comparison
- **`recomputeSelectedReferenceable()` already exists** - handles rebuilding filtered lists
- **UX can be improved later** - separate concern from staleness fix

**For LSP mode**: Provide unfiltered references (standard LSP behavior), let client filter if desired

## Problem Summary

**Bug**: When navigating references (`OLO_NEXT`/`OLO_PREVIOUS`) after editor buffers have been modified, navigation lands at wrong line numbers because references are stale.

**Test Case**: `tests/test_modified_editor_buffer` (currently suspended)

**Scenario**:
```
1. PUSH at source.c (no preload) → parses disk file → creates references
2. User edits file in editor (adds blank line, shifting positions)
3. NEXT with -preload source.c source.preload → loads modified buffer
4. Navigation uses OLD positions from step 1 instead of NEW positions from modified buffer
```

**Expected**: Navigate to line 7 (where reference is in modified buffer)
**Actual**: Navigate to line 6 (where reference was in original disk file)

## Root Cause

ServerMode's `OLO_NEXT`/`OLO_PREVIOUS` operations don't process files (don't reparse), they only navigate using cached session references.

From `server.c:51-60`:
```c
static bool requiresProcessingInputFile(ServerOperation operation) {
    return operation==OLO_COMPLETION
           || operation==OLO_EXTRACT
           || operation==OLO_TAG_SEARCH
           || operation==OLO_SET_MOVE_TARGET
           || operation==OLO_GET_FUNCTION_BOUNDS
           || operation==OLO_GET_ENV_VALUE
           || needsReferenceDatabase(operation)  // OLO_NEXT/PREV NOT included
        ;
}
```

## Architectural Understanding

### Source of Truth Design Decision

**Agreed principle**: In-memory reference database is the source of truth
- Disk `.cx` files are just cache/startup optimization
- Session menus are *views* into the database, not independent snapshots
- Goal: Eventually operate completely without disk database

### Two Sub-Problems

#### 1. Server Knowledge Gap
**Problem**: Server doesn't know about modified buffers in editor
- Client only sends current buffer with `-preload`
- Other modified files in editor are invisible to server

**Current protocol** (already supports multiple preloads):
```
-preload file1 content1 -preload file2 content2
```
But Emacs client only sends current buffer.

**Potential solution**: Modify Emacs client to send all modified buffers on PUSH operations

#### 2. Stale Session Menus
**Problem**: References in session menu are from old parse state
- Session holds filtered copies of references
- When files are reparsed, session references become stale

**Potential solution**: Re-parse modified files and intelligently rebuild menu

## Four Staleness Scenarios

### Scenario 1: Both files unmodified in buffers
- ✅ Works correctly
- No preload, or preload matches disk
- References are fresh

### Scenario 2: Modified source buffer (unsaved), target untouched
- ❌ PUSH parses disk version if no preload on PUSH command
- If preload sent with NEXT, PUSH already created stale references

### Scenario 3: Source untouched, target modified (unsaved)
- ❌ **This is the test scenario**
- PUSH creates references from disk file positions
- Target file has preloaded content at different positions
- Navigation lands at wrong line

### Scenario 4: File modified and saved to disk
- ✅ Should work - mtime checking triggers reparse
- `editorFileModificationTime()` checks buffer mtime vs `lastParsedMtime`
- But only in XrefMode's update scheduling, not in ServerMode navigation

## How References Work in Sessions

### SessionStackEntry Structure
```c
typedef struct SessionStackEntry {
    Reference *references;      // Filtered list for navigation (COPIES)
    Reference *current;         // Current position in navigation
    BrowserMenu *hkSelectedSym; // The symbol(s) being navigated
    int refsFilterLevel;        // User-adjustable filter
    // ... other fields
}
```

### PUSH Creates Filtered Reference Copies

**Flow**: `mainAnswerReferencePushingAction()` → `createSelectionMenuForOperation()` → `processSelectedReferences()` → `genOnLineReferences()`

**Key function** (`cxref.c:1079`):
```c
static void genOnLineReferences(SessionStackEntry *sessionStackEntry, BrowserMenu *menu) {
    if (menu->selected) {
        addReferencesFromFileToList(menu->referenceable.references,
                                     ANY_FILE,
                                     &sessionStackEntry->references);
    }
}
```

**Important**: `addReferencesFromFileToList()` calls `addReferenceToList()` which **malloc's new Reference structs** (`reference.c:88-90`):
```c
r = malloc(sizeof(Reference));
*r = *ref;  // Copy data
LIST_CONS(r, *placeInList);
```

### Filtering Happens at Two Levels

1. **Build-time filtering** (when creating session):
   - Usage visibility (`isVisibleUsage()` filters out implementation details)
   - File number filter (can be ANY_FILE or specific file)

2. **Navigation-time filtering** (user-adjustable):
   - `sessionStackEntry->refsFilterLevel` - controlled by user
   - `OLO_MENU_FILTER_SET/PLUS/MINUS` operations change this
   - Navigation skips references that don't pass current filter

### Existing Rebuild Mechanism

**Important discovery**: `recomputeSelectedReferenceable()` already exists!

From `cxref.c:1083-1087`:
```c
void recomputeSelectedReferenceable(SessionStackEntry *entry) {
    freeReferences(entry->references);
    entry->references = NULL;
    processSelectedReferences(entry, genOnLineReferences);
}
```

Called when user changes menu selection or filter level - it **rebuilds the filtered reference list from the in-memory database**.

This is the mechanism we could leverage for staleness handling!

## How `recomputeSelectedReferenceable()` Solves the Problem

### What It Does

From `cxref.c:1083-1087`:
```c
void recomputeSelectedReferenceable(SessionStackEntry *entry) {
    freeReferences(entry->references);           // Free old reference list
    entry->references = NULL;
    processSelectedReferences(entry, genOnLineReferences);  // Rebuild from database
}
```

**Key operations**:
1. **Frees** the session's filtered reference list (`entry->references`)
2. **Rebuilds** it from the in-memory database
3. **Applies** current filter level during rebuild
4. **Preserves** the menu structure (which symbols are selected)

### Why It's Perfect for Staleness

Currently used when user changes menu selection or filter level - it **already knows how to rebuild the reference list from the current database state**.

**The insight**: If we update the database (reparse), then call `recomputeSelectedReferenceable()`, the session automatically gets fresh references!

### Algorithm Using `recomputeSelectedReferenceable()`

```c
// In gotoNextReference() or a helper function called before navigation:

1. Detect stale file:
   Reference *nextRef = refs->current->next;  // or current, depending on semantics
   int fileNum = nextRef->position.file;
   EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(
       getFileItemWithFileNumber(fileNum)->name);

   if (buffer != NULL && buffer->preLoadedFromFile != NULL) {
       // File is modified in editor - references are stale!

2. Save navigation context:
       Position savedPosition = refs->current->position;
       int savedFilterLevel = refs->refsFilterLevel;

3. Update database:
       removeReferenceableItemsForFile(fileNum);  // Remove stale references
       // TODO: Reparse the file to add fresh references

4. Rebuild session:
       recomputeSelectedReferenceable(refs);  // Magic! Rebuilds from fresh database

5. Restore navigation position:
       // Find reference after savedPosition
       refs->current = refs->references;
       while (refs->current != NULL) {
           if (positionIsLessThan(savedPosition, refs->current->position)) {
               break;  // Found first reference after where we were
           }
           refs->current = refs->current->next;
       }

       // Apply filter to find next visible reference
       setCurrentReferenceToFirstVisible(refs, refs->current);
   }

6. Continue with normal navigation
```

### Why This Works

**Memory safety**:
- `recomputeSelectedReferenceable()` frees the old malloc'd copies
- Creates fresh copies from database
- No dangling pointers!

**Filter preservation**:
- `refs->refsFilterLevel` is preserved
- `processSelectedReferences()` applies current filter when rebuilding
- Navigation continues with same filter settings

**Menu structure preserved**:
- `refs->hkSelectedSym` and `refs->menu` unchanged
- Still navigating the same symbol
- Just with updated positions

**Simple integration**:
- One function call does all the work
- Already tested and working (used for filter changes)
- No need to manually rebuild complex structures

### What We Still Need to Figure Out

**1. How to trigger lightweight reparse** ✅ SOLVED!

**Discovery**: `parseToCreateReferences()` already exists and is perfect!

From `parsing.c:165-193`:
```c
void parseToCreateReferences(const char *fileName) {
    EditorBuffer *buffer;

    /* Try to get editor buffer first (preferred for LSP, refactoring, xref with open editors) */
    buffer = getOpenedAndLoadedEditorBuffer((char *)fileName);

    /* Fall back to disk file if editor buffer not found (common in xref batch mode) */
    if (buffer == NULL) {
        buffer = findOrCreateAndLoadEditorBufferForFile((char *)fileName);
        if (buffer == NULL) {
            log_error("parseToCreateReferences: Could not open '%s'", fileName);
            return;
        }
    }

    /* Setup input for parsing */
    initInput(NULL, buffer, "\n", (char *)fileName);
    setupParsingConfig(currentFile.characterBuffer.fileNumber);
    parsingConfig.operation = PARSE_TO_CREATE_REFERENCES;

    /* Parse the file - populating the ReferenceableItemTable */
    callParser(parsingConfig.fileNumber, parsingConfig.language);
}
```

**Why it's perfect**:
- ✅ **Lightweight** - no heavy initialization, just parses
- ✅ **Gets preloaded buffers** - calls `getOpenedAndLoadedEditorBuffer()` first!
- ✅ **Creates references** - populates ReferenceableItemTable directly
- ✅ **Already tested** - used in LSP mode for `textDocument/didOpen`
- ✅ **Simple to call** - just needs filename, no state setup

**How LSP uses it** (`lsp_handler.c:131`):
```c
// When file is opened in editor:
EditorBuffer *buffer = createNewEditorBuffer(fileName, NULL, time(NULL), strlen(text));
loadTextIntoEditorBuffer(buffer, time(NULL), text);
parseToCreateReferences(buffer->fileName);  // Parse buffer content!
```

**This means our algorithm simplifies to**:
```c
1. removeReferenceableItemsForFile(fileNum)
2. parseToCreateReferences(fileName)  // Automatically uses preloaded buffer!
3. recomputeSelectedReferenceable(refs)
```

No need for complex parsing setup - it's all self-contained!

**2. Position restoration strategy**

After rebuild, references might be:
- At different positions (lines shifted)
- Deleted entirely (reference removed during edit)
- Added (new references appeared)

Current algorithm finds "next reference after saved position" which seems reasonable, but consider:

```c
Original: refs at lines 5, 10, 15, 20  (current = 10)
User edits: adds blank line at top
New refs: at lines 6, 11, 16, 21
Saved position: line 10
After rebuild: finds line 11 ✓ (correct!)

But what if:
Original: refs at lines 5, 10, 15, 20  (current = 10)
User deletes line 10 reference
New refs: at lines 5, 15, 20
Saved position: line 10
After rebuild: finds line 15 ✓ (next available, reasonable!)
```

Seems robust!

**3. Multi-file staleness**

What if user has multiple files modified?

```
Session has references to:
- file A: lines 10, 20, 30
- file B: lines 5, 15, 25
- file A: lines 40, 50

Current at: file A line 20
Both A and B are modified!
```

Should we:
- Only refresh the next file we're navigating to? (lazy)
- Refresh all files with references in the session? (eager)
- Track which files are "dirty" and refresh on demand?

**4. When to trigger the refresh**

**Option A**: On every navigation (NEXT/PREV)
- Simple: just detect and refresh before each navigation
- Expensive: could reparse multiple times if navigating through same file
- But: only reparses when actually stale

**Option B**: Detect and warn, require explicit refresh
- Add `OLO_REFRESH_REFERENCES` operation
- User manually triggers when they know files changed
- Less automatic, more user control

**Option C**: Smart caching
- Track last parse time per file
- Only reparse if buffer mtime > parse time
- Avoid redundant reparses

**Option D**: Batch refresh
- Detect stale on first NEXT
- Refresh all stale files at once
- Then navigate normally through fresh references

### Recommended Approach

**Phase 1 - Simple but functional** (Now very straightforward!):
```c
In gotoNextReference():
1. Check if NEXT reference's file is stale (has preloaded buffer)
2. If stale:
   a. Save position: Position savedPos = refs->current->position;
   b. Remove old references: removeReferenceableItemsForFile(fileNum)
   c. Reparse file: parseToCreateReferences(fileName)  // ✅ Uses preloaded buffer!
   d. Rebuild session: recomputeSelectedReferenceable(refs)
   e. Restore position: find next ref after savedPos
   f. Apply filter: setCurrentReferenceToFirstVisible(refs, refs->current)
3. Continue with normal navigation
```

**Complete implementation sketch**:
```c
static void gotoNextReference(void) {
    SessionStackEntry *refs;
    Reference *r;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL_YES))
        return;

    // Determine next reference
    r = (refs->current == NULL) ? refs->references : refs->current->next;

    // Check if next reference's file is stale
    if (r != NULL) {
        int fileNum = r->position.file;
        char *fileName = getFileItemWithFileNumber(fileNum)->name;
        EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(fileName);

        if (buffer != NULL && buffer->preLoadedFromFile != NULL) {
            // File is stale - refresh it!
            log_debug("Refreshing stale references for file %d: %s", fileNum, fileName);

            Position savedPos = refs->current ? refs->current->position : NO_POSITION;

            removeReferenceableItemsForFile(fileNum);
            parseToCreateReferences(fileName);  // Magic happens here!
            recomputeSelectedReferenceable(refs);

            // Find next reference after saved position
            refs->current = refs->references;
            while (refs->current != NULL) {
                if (positionIsLessThan(savedPos, refs->current->position)) {
                    break;
                }
                refs->current = refs->current->next;
            }
        }
    }

    // Normal navigation continues...
    if (refs->current == NULL)
        refs->current = refs->references;
    else {
        r = refs->current->next;
        setCurrentReferenceToFirstVisible(refs, r);
    }
    olcxGenGotoActReference(refs);
}
```

**Advantages**:
- ✅ Leverages existing `recomputeSelectedReferenceable()`
- ✅ Leverages existing `parseToCreateReferences()` from LSP work
- ✅ Filter preservation automatic
- ✅ Memory management automatic
- ✅ Preloaded buffer handling automatic
- ✅ Single file refresh (simpler to start)
- ✅ No complex parsing setup needed
- ✅ Already tested infrastructure (LSP uses parseToCreateReferences)

**Phase 2 - Optimizations**:
- Cache: Don't reparse same file multiple times in one navigation session
- Batch: Detect all stale files, reparse all, then navigate
- Multi-buffer preload: Have Emacs send all modified buffers on PUSH

**Phase 3 - Polish**:
- Better UX: Show "refreshing references..." message
- Statistics: "Refreshed 3 files, found 45 updated references"
- Error handling: What if reparse fails?

## Proposed Algorithm (Draft)

For `OLO_NEXT` when target file has been modified in editor:

1. **Detect file has changed**
   ```c
   Reference *targetRef = /* next reference we want to navigate to */;
   int targetFileNum = targetRef->position.file;
   EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(
       getFileItemWithFileNumber(targetFileNum)->name);

   if (buffer != NULL && buffer->preLoadedFromFile != NULL) {
       // File is modified, need to refresh
   }
   ```

2. **Delete references in that file**
   - `removeReferenceableItemsForFile(targetFileNum)` - already exists!
   - Removes all references for that file from in-memory database

3. **Re-parse the file**
   - Challenge: Parsing needs full setup (file opening, buffers, etc.)
   - Current `processFile()` → `singlePass()` → `parseInputFile()` is complex
   - Need lightweight reparse mechanism, or higher-level orchestration

4. **Rebuild session's reference list**
   - Save position: `Position savedPos = refs->current->position;`
   - Call `recomputeSelectedReferenceable(refs)` to rebuild from database
   - Restore position: find next reference after savedPos
   - If exact reference disappeared, navigate to "next after where it was"

## Challenges Identified

### 1. Where to implement this?
- `gotoNextReference()` is presentation layer, too low-level for parsing
- Need higher-level operation or helper function
- Called from where? Before each navigation? On demand?

### 2. Memory lifetime concerns
- Session references are malloc'd copies
- After `removeReferenceableItemsForFile()`, original references are freed
- Session copies become dangling if we don't rebuild

### 3. Index vs Pointer question
- Session references are filtered subset of ReferenceableItem references
- Can't use simple index because filtering changes index mapping
- Example:
  ```
  ReferenceableItem.references: [Ref0, Ref1, Ref2, Ref3]
  Session.references (filtered): [Ref0, Ref1, Ref2]  // Ref3 filtered out
  ```
  Index 2 refers to different references!

### 4. Complex corner cases
- **Reference deleted**: Symbol removed during edit - navigate to next available
- **Reference added**: New reference appears - should be included in navigation
- **Symbol renamed**: Symbol no longer exists - invalidate session?
- **Multiple stale files**: References span multiple modified files
- **Cross-file implications**: Header change affects references in other files
- **Session stack depth**: Multiple symbols pushed, some before/after edits

## Key Functions Reference

### Detection
- `getOpenedAndLoadedEditorBuffer(char *name)` - returns EditorBuffer if exists
- `buffer->preLoadedFromFile != NULL` - indicates modified buffer

### Database Manipulation
- `removeReferenceableItemsForFile(int fileNumber)` - removes all refs for file
- `callParser(int fileNumber, Language language)` - parses a file

### Session Rebuilding
- `recomputeSelectedReferenceable(SessionStackEntry *entry)` - rebuilds reference list
- `processSelectedReferences()` - applies filtering and rebuilding
- `genOnLineReferences()` - copies filtered references to session

### Navigation
- `gotoNextReference()` - `cxref.c:913`
- `gotoPreviousReference()` - `cxref.c:927`
- `setCurrentReferenceToFirstVisible()` - applies filter during navigation

## Open Questions

1. **Should we use indices instead of pointers?**
   - Pointers break when references are freed/recreated
   - Indices break when filtering changes the subset
   - Stable identifier that survives reparsing?

2. **When to trigger refresh?**
   - On every NEXT/PREV? (Expensive)
   - Only when detecting stale file? (Need staleness check)
   - New operation `OLO_REFRESH`? (User explicitly requests)

3. **What about #included files?**
   - Should we reparse include closure?
   - Start with just the direct file, add includes later?

4. **How to handle reparsing from navigation code?**
   - Too complex to call full `processFile()` machinery
   - Need lightweight reparse helper?
   - Or raise to higher level that can orchestrate?

## Next Steps

1. **Understand if index-based approach is viable**
   - Investigate stable reference identification
   - How does filtering affect index mapping?

2. **Explore lightweight reparse mechanism**
   - Can we trigger parse without full `initializeFileProcessing()`?
   - What's minimum setup needed?

3. **Prototype detection and rebuild**
   - Start with just detection and error message
   - Add rebuild using `recomputeSelectedReferenceable()`
   - Test with `test_modified_editor_buffer`

4. **Consider multi-buffer preload approach**
   - Modify Emacs client to send all modified buffers on PUSH
   - Would fix staleness at source
   - Requires client changes

## File Locations

- Test: `tests/test_modified_editor_buffer/`
- Session structure: `src/session.h`
- Navigation: `src/cxref.c:913` (gotoNextReference)
- PUSH handling: `src/cxref.c:1736` (mainAnswerReferencePushingAction)
- Reference database: `src/referenceableitemtable.c`
- Server mode check: `src/server.c:51` (requiresProcessingInputFile)
- Reference copying: `src/reference.c:82` (addReferenceToListWithoutUsageCheck)
- Rebuild mechanism: `src/cxref.c:1083` (recomputeSelectedReferenceable)

## Investigation Notes: REPUSH

Investigated whether `OLO_REPUSH` could help - **it cannot**.

`OLO_REPUSH` = "Undo Pop" - restores a previously popped session stack entry, doesn't refresh stale references.

From `cxref.c:1274`:
```c
static void olcxReferenceRePush(void) {
    nextrr = getNextTopStackItem(&sessionData.browsingStack);  // Get previously popped
    if (nextrr != NULL) {
        sessionData.browsingStack.top = nextrr;  // Restore it
        olcxGenGotoActReference(sessionData.browsingStack.top);
    }
}
```

No existing mechanism for refreshing stale references - we need to design one.
