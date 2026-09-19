#include <cgreen/cgreen.h>
#include <cgreen/unit.h>

#include "editormarker.h"

#include "commons.mock"
#include "editorbuffer.mock"
#include "filetable.mock"
#include "misc.mock"
#include "ppc.mock"


Describe(EditorMarker);
BeforeEach(EditorMarker) {}
AfterEach(EditorMarker) {}

Ensure(EditorMarker, can_run_empty_test) {
}
