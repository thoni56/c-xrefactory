#include <cgreen/cgreen.h>

#include "editor.h"

#include "memory.h"

#include "yylex.mock"
#include "options.mock"
#include "misc.mock"
#include "globals.mock"
#include "editorbuffertab.mock"
#include "filetable.mock"
#include "cxref.mock"
#include "commons.mock"
#include "fileio.mock"
#include "ppc.mock"



Describe(Editor);
BeforeEach(Editor) {}
AfterEach(Editor) {}


Ensure(Editor, can_create_new_editor_region_list) {
    EditorBuffer buffer;
    EditorMarker *begin = newEditorMarker(&buffer, 1, NULL, NULL);
    EditorMarker *end = newEditorMarker(&buffer, 10, begin, NULL);
    EditorRegionList *regionList = newEditorRegionList(begin, end, NULL);

    assert_that(regionList->region.begin->buffer, is_equal_to(&buffer));
    assert_that(regionList->region.end->buffer, is_equal_to(&buffer));
}
