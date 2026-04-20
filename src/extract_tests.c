#include <cgreen/cgreen.h>

#include "extract.h"
#include "extract_internal.h"

#include "log.h"
#include "memory.h"
#include "stackmemory.h"

char *extractNameFromLinkName(char *linkName);
bool isImplicitFunctionShadowLinkName(char *linkName);
void spliceShadowIfMatches(ProgramGraphNode **program, ReferenceableItem *shadowItem);


/* Dependencies: */
#include "commons.mock"
#include "cxref.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.mock"
#include "parsing.mock"
#include "ppc.mock"
#include "referenceableitemtable.mock"
#include "semact.mock"
#include "symbol.mock"
#include "symboltable.mock"


Describe(Extract);
BeforeEach(Extract) {
    log_set_level(LOG_ERROR);
    initOuterCodeBlock();
    initCxMemory(1 << 16);
}
AfterEach(Extract) {}


Ensure(Extract, extracts_variable_name_from_local_link_name) {
    assert_that(extractNameFromLinkName(" int! !pos!!27c-5-8-4"), is_equal_to_string("pos"));
    assert_that(extractNameFromLinkName(" int! !counter!!27c-7-12-5"), is_equal_to_string("counter"));
}

Ensure(Extract, extracts_variable_name_from_implicit_function_link_name) {
    assert_that(extractNameFromLinkName(" extern int! !pos!()!27c-9-13-7"), is_equal_to_string("pos"));
}

Ensure(Extract, splice_leaves_program_unchanged_for_non_matching_shadow) {
    ReferenceableItem localItem = {.linkName = " int! !pos!!27c-5-8-4"};
    Reference localDef = {.usage = UsageDefined};
    ProgramGraphNode localNode = {
        .reference = &localDef,
        .referenceableItem = &localItem,
        .regionSide = DATAFLOW_INSIDE_BLOCK,
    };
    ProgramGraphNode *program = &localNode;

    Reference shadowRef = {.usage = UsageUsed};
    ReferenceableItem shadowItem = {
        .linkName = " extern int! !other!()!27c-9-13-7",
        .references = &shadowRef,
    };

    spliceShadowIfMatches(&program, &shadowItem);

    assert_that(program, is_equal_to(&localNode));
    assert_that(program->next, is_null);
}

Ensure(Extract, splice_appends_outside_node_for_matching_shadow) {
    ReferenceableItem localItem = {.linkName = " int! !pos!!27c-5-8-4"};
    Reference localDef = {.usage = UsageDefined};
    ProgramGraphNode localNode = {
        .reference = &localDef,
        .referenceableItem = &localItem,
        .regionSide = DATAFLOW_INSIDE_BLOCK,
    };
    ProgramGraphNode *program = &localNode;

    Reference shadowRef = {.usage = UsageUsed};
    ReferenceableItem shadowItem = {
        .linkName = " extern int! !pos!()!27c-9-13-7",
        .references = &shadowRef,
    };

    spliceShadowIfMatches(&program, &shadowItem);

    assert_that(program->regionSide, is_equal_to(DATAFLOW_OUTSIDE_BLOCK));
    assert_that(program->referenceableItem, is_equal_to(&localItem));
    assert_that(program->reference, is_equal_to(&shadowRef));
    assert_that(program->next, is_equal_to(&localNode));
}

Ensure(Extract, recognizes_implicit_function_shadow_link_name) {
    /* Implicit-decl shadow: an undeclared identifier used as a value in
     * expression context, resolved by c_parser.y's K&R rule into an
     * `extern int pos()` symbol. This is what we want to detect. */
    assert_that(isImplicitFunctionShadowLinkName(" extern int! !pos!()!27c-9-13-7"), is_true);

    /* Local variable declaration (`int pos = 0;`) — same name, but storage
     * auto and no function-type marker. Must not be matched. */
    assert_that(isImplicitFunctionShadowLinkName(" int! !pos!!27c-5-8-4"), is_false);

    /* Local function-pointer (`int (*f)() = ...;`) — has `()` from function
     * type but storage auto; the prefix `" extern int! !"` rules it out. */
    assert_that(isImplicitFunctionShadowLinkName(" int! (* !f! )()!27c-5-8-4"), is_false);

    /* Explicit extern function-pointer declared inside the region
     * (`extern int (*f)();`) — storage extern and `()` present, but the
     * parenthesis disturbs the prefix (position 13 is `(`, not `!`). */
    assert_that(isImplicitFunctionShadowLinkName(" extern int! (* !f! )()!27c-5-8-4"), is_false);
}
