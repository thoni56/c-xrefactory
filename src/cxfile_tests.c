#include <cgreen/assertions.h>
#include <cgreen/cgreen.h>
#include <cgreen/constraint.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include <ctype.h>

#include "cxfile.h"
#include "log.h"

#include "browsingmenu.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "completion.mock"
#include "cxref.mock"
#include "editor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "match.mock"
#include "misc.mock"
#include "options.mock"
#include "parsing.mock"
#include "reference.mock"
#include "referenceableitem.h"
#include "referenceableitemtable.mock"
#include "search.mock"
#include "session.h"
#include "session.mock"
#include "startup.mock"
#include "storage.h"
#include "visibility.h"


Describe(CxFile);
BeforeEach(CxFile) {
    log_set_level(LOG_ERROR); /* Set to LOG_DEBUG if needed */

    options.mode = ServerMode;
}
AfterEach(CxFile) {}


SessionStackEntry *newEmptySessionStackEntry(void) {
    SessionStackEntry *entry  = malloc(sizeof(SessionStackEntry));
    *entry = (SessionStackEntry){
        .references      = NULL,
        .current         = NULL,
        .operation       = options.serverOperation,
        .callerPosition  = NO_POSITION,
        .matches     = NULL,
        .hkSelectedSym   = NULL,
        .menuFilterLevel = 0,
        .refsFilterLevel = 0,
        .previous        = NULL};
    return entry;
}

#define will_set_contents_of_output_parameter_to_string(parameter_name, pointer_to_string)              \
    create_set_parameter_value_constraint(#parameter_name, (intptr_t)pointer_to_string,                 \
                                          strlen(pointer_to_string)+1)


Ensure(CxFile, can_check_references_for_referenceable_in_search) {
    ReferenceableItem item = makeReferenceableItem("item", TypeInt, StorageDefault,
                                                   VisibilityLocal, NO_FILE_NUMBER);
    Reference reference;
    SessionStackEntry *stackEntry = newEmptySessionStackEntry();

    options.searchString = "Sym";
    searchingStack.top = stackEntry;

    expect(prettyPrintLinkName, will_set_contents_of_output_parameter_to_string(buffer, "SymbolName"));
    expect(containsWildcard, will_return(false));

    expect(prependToMatches, when(name, is_equal_to_string("SymbolName")));

    searchSymbolCheckReference(&item, &reference);
}
