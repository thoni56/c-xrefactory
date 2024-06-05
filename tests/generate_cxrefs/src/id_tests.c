#include <cgreen/cgreen.h>

#include "id.h"

/* Dependencies: */
#include "cxref.mock"
#include "globals.mock"
#include "commons.mock"
#include "stackmemory.h"


Describe(Id);
BeforeEach(Id) {
    initOuterCodeBlock();
}
AfterEach(Id) {}

Ensure(Id, can_copy_id) {
    Id  id;
    Id *id1 = &id;
    Id *id2;

    id1->name          = "idName";
    id1->position.file = 1;
    id1->position.line = 2;
    id1->position.col  = 3;
    id1->symbol        = NULL;
    id1->next          = &id;

    id2 = newCopyOfId(id1);

    assert_that(memcmp(id1, id2, sizeof(Id)), is_equal_to(0));
}
