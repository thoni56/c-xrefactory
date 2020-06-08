#include <cgreen/cgreen.h>

#include "../editor.h"

#include "yylex.mock"
#include "misc.mock"
#include "globals.mock"
#include "editorbuffertab.mock"
#include "filetab.mock"
#include "cxref.mock"
#include "commons.mock"
#include "fileio.mock"


Describe(Editor);
BeforeEach(Editor) {}
AfterEach(Editor) {}


Ensure(Editor, can_create_new_editor_region_list) {
    EditorBuffer buffer;
    EditorMarker *begin = newEditorMarker(&buffer, 1, NULL, NULL);
    EditorMarker *end = newEditorMarker(&buffer, 10, begin, NULL);
    S_editorRegionList *regionList = newEditorRegionList(begin, end, NULL);

    assert_that(regionList->r.b->buffer, is_equal_to(&buffer));
    assert_that(regionList->r.e->buffer, is_equal_to(&buffer));
}
