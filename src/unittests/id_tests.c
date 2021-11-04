#include <cgreen/cgreen.h>

#include "id.h"

/* Dependencies: */
#include "globals.mock"
#include "cxref.mock"


Describe(Id);
BeforeEach(Id) {
    stackMemoryInit();
}
AfterEach(Id) {}


Ensure(Id, can_copy_id) {
    Id id;
    Id *id1 = &id;
    Id *id2;

    id1->name = "idName";
    id1->p.file = 1;
    id1->p.line = 2;
    id1->p.col = 3;
    id1->symbol = NULL;
    id1->next = &id;

    id2 = newCopyOfId(id1);

    assert_that(memcmp(id1, id2, sizeof(Id)), is_equal_to(0));
}
