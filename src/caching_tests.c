#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "caching.h"
#include "stackmemory.h"
#include "log.h"

#include "commons.mock"
#include "counters.mock"
#include "cxref.mock" /* For freeOldestOlcx() */
#include "editor.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "input.mock"
#include "options.mock"
#include "reftab.mock"
#include "symboltable.mock"
#include "yylex.mock"

Describe(Caching);

BeforeEach(Caching) {
    log_set_level(LOG_ERROR);  // Reduce noise during testing
    // Initialize stack memory for cache point testing
    initOuterCodeBlock();
}

AfterEach(Caching) {
    // Clean up any caching state
    cache.active = false;
    cache.index = 0;
}

Ensure(Caching, can_manage_be_activated_and_deactivated) {
    // Caching should be inactive by default
    assert_that(cachingIsActive(), is_false);

    activateCaching();
    assert_that(cachingIsActive(), is_true);

    deactivateCaching();
    assert_that(cachingIsActive(), is_false);
}

Ensure(Caching, can_initialize_cache_structure) {
    // Mock both calls to getMacroBodyMemoryIndex in placeCachePoint
    expect(getMacroBodyMemoryIndex, will_return(0));
    expect(getMacroBodyMemoryIndex, will_return(0));

    // Start with clean state
    cache.index = 0;
    assert_that(cachingIsActive(), is_false);

    // Initialize caching system
    initCaching();

    // After init, cache should be inactive (implementation calls deactivateCaching)
    assert_that(cachingIsActive(), is_false);

    // Cache index should be 1 (initial cache point was placed)
    assert_that(cache.index, is_equal_to(1));

    // Include stack should start empty
    assert_that(cache.includeStackTop, is_equal_to(0));
}

/**
 * Test that placeCachePoint works when caching is active.
 * This validates the core cache point creation mechanism.
 */
Ensure(Caching, can_place_cache_point_when_active) {
    // Start with clean state
    cache.index = 0;
    int initialIndex = 0;

    // Activate caching first
    activateCaching();
    assert_that(cachingIsActive(), is_true);

    // Mock both calls to getMacroBodyMemoryIndex in placeCachePoint
    expect(getMacroBodyMemoryIndex, will_return(0));
    expect(getMacroBodyMemoryIndex, will_return(0));

    // Place a cache point
    placeCachePoint(false);

    // Cache index should have incremented
    assert_that(cache.index, is_equal_to(initialIndex + 1));
}

Ensure(Caching, ignores_cache_point_when_inactive) {
    cache.index = 0;
    int initialIndex = 0;

    deactivateCaching();

    placeCachePoint(false);

    assert_that(cache.index, is_equal_to(initialIndex));
}
