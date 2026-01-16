# Session State: Duplicate References Bug Fix
## Date: 2026-01-16

## Current Status: ALMOST COMPLETE ✓

All originally failing tests now pass! However, there's one new issue discovered:
- **Problem**: `free(): invalid pointer` error in `test_changed_source_updates_references`
- **Impact**: This test wasn't previously identified as failing, so it's a new regression

## What We've Accomplished

### 1. Root Cause Identified
The "duplicate references bug" where navigation lands at wrong line numbers after editor buffers are modified was caused by:

**Menu items contain STALE REFERENCE POINTERS after file reparse:**
- `BrowserMenu` contains a **copy** of `ReferenceableItem` (not a pointer)
- When a file is reparsed:
  1. `removeReferenceableItemsForFile()` modifies the database's reference lists
  2. `parseToCreateReferences()` creates new reference lists in the database
  3. BUT `menu->referenceable.references` still points to the OLD/FREED memory
  4. `recomputeSelectedReferenceable()` uses the stale pointers, getting wrong line numbers

### 2. Solution Implemented

**File: `/home/thoni/Utveckling/c-xrefactory/src/cxref.c`**

Added code in `gotoNextReference()` after the `parseToCreateReferences()` call to update all menu items with fresh reference pointers from the database:

```c
// Update menu items to have fresh reference pointers from database.
// After removeReferenceableItemsForFile() + parseToCreateReferences(),
// menu->referenceable.references points to freed/stale memory.
for (BrowserMenu *menu = sessionEntry->menu; menu != NULL; menu = menu->next) {
    // Look up fresh item from database (exact match including includeFileNumber)
    int index;
    ReferenceableItem *freshItem;
    if (isMemberInReferenceableItemTable(&menu->referenceable, &index, &freshItem)) {
        // Update menu's copy to point to fresh reference list
        menu->referenceable.references = freshItem->references;
    }
    // If not found, item might have been removed or menu is for a different file (leave as-is)
}
```

**Also added logging in `lexer.c`:**
- `handleNewline()` - traces line number increments
- `noteNewLexemPosition()` - traces position tracking

## Test Results

### ✅ Originally Failing Tests - ALL PASS:
- `test_modified_editor_buffer` - **FIXED** (was the original bug report)
- `test_multifile_navigation` - **FIXED**
- `test_rename_included_file` - **FIXED**
- `test_rename_with_update` - **FIXED**
- `test_cexercise_browsing` - **FIXED**
- `test_cexercise_rename_5_name_collision` - **FIXED**
- `test_push_by_name` - **FIXED**
- `test_rename_anglebracketed_include` - **FIXED**

### ❌ New Regression:
- `test_changed_source_updates_references` - **free(): invalid pointer**

## The Remaining Problem

**Error**: `free(): invalid pointer` in `test_changed_source_updates_references`

**Hypothesis**: The menu update code is creating a situation where:
1. Multiple menu items point to the same reference list
2. When menus are freed, the same list gets freed multiple times
3. OR: We're updating a menu item whose references shouldn't be touched

**What test_changed_source_updates_references does**: (Need to investigate)
- Likely similar to test_modified_editor_buffer but with different scenario
- May involve multiple files or multiple refreshes

## Next Steps

1. **Investigate the failing test**:
   ```bash
   cd /home/thoni/Utveckling/c-xrefactory/tests/test_changed_source_updates_references
   cat commands.input
   cat *.c *.h
   make 2>&1 | tail -50
   ```

2. **Possible fixes**:
   - Only update menu items for the specific file that was reparsed (check fileNum)
   - Check if menu->referenceable.references is already pointing to database memory before updating
   - Add NULL check or ownership tracking

3. **Debug approach**:
   - Run with valgrind to see exact memory issue
   - Add logging to see which menu items are being updated
   - Check if multiple menu items share the same reference list

## Code Changes Made

### `/home/thoni/Entwick/c-xrefactory/src/cxref.c`

**Function: `gotoNextReference()`** (around line 959-976)
- Added menu reference pointer update loop after `parseToCreateReferences()`

### `/home/thoni/Utveckling/c-xrefactory/src/lexer.c`

**Function: `handleNewline()`** (line 344)
- Added trace logging: `log_trace("handleNewline: line %d -> %d (file %d)", ...)`

**Function: `noteNewLexemPosition()`** (lines 90-91)
- Added trace logging: `log_trace("noteNewLexemPosition: file=%d line=%d col=%d offset=%d", ...)`

### Debug Logging Still Active
- Various DEBUG log statements in `cxref.c` around line 960-980
- Should be cleaned up once issue is fully resolved

## Important Context

### Memory Management Notes
- Reference lists are malloc'd in `addReferenceToListWithoutUsageCheck()` (reference.c:88)
- Menu items are freed in `freeBrowserMenuList()`
- Database references use `cxAlloc()` which is part of cx memory pool
- Session references use `malloc()` - these are copies for navigation

### Key Files
- `src/cxref.c` - Navigation logic, session management
- `src/lexer.c` - Position tracking during parsing
- `src/parsing.c` - Contains `parseToCreateReferences()`
- `src/referenceableitemtable.c` - Database operations
- `src/reference.c` - Reference list management
- `src/browsermenu.c` - Menu management

### Related Documentation
- `/home/thoni/Utveckling/c-xrefactory/doc/duplicate-references-investigation.md` - Full investigation notes

## Commands to Resume

```bash
# Check the new failing test
cd /home/thoni/Utveckling/c-xrefactory/tests/test_changed_source_updates_references
cat commands.input
make 2>&1 | tail -100

# Run with valgrind for memory debugging
cd /home/thoni/Utveckling/c-xrefactory/tests/test_changed_source_updates_references
valgrind --leak-check=full ../../c-xref < commands.input > /tmp/out 2>&1
tail -100 /tmp/out

# Test all previously failing tests
cd /home/thoni/Utveckling/c-xrefactory/tests
make test_modified_editor_buffer test_multifile_navigation test_rename_with_update

# Run full test suite
cd /home/thoni/Utveckling/c-xrefactory/tests
make quick
```

## Key Insight for Next Session

The fix works by updating menu items' reference pointers after reparse. The `free(): invalid pointer` suggests we're either:
1. Creating double-free situations (multiple menus pointing to same list)
2. Updating menus we shouldn't touch
3. Mixing malloc'd and cxAlloc'd memory incorrectly

Check if the reference lists in menus are supposed to be **independent copies** or **shared pointers** to database lists. This is likely the key to the solution.
