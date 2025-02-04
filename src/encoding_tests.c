#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/unit.h>

#include "encoding.h"

#include "options.mock"
#include "editorbuffer.mock"


Describe(Encoding);
BeforeEach(Encoding) {}
AfterEach(Encoding) {}


Ensure(Encoding, will_not_convert_ascii) {
    const char *all_ascii_characters =
        " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    EditorBuffer *buffer = (EditorBuffer*)0x1316447a4;

    char text[strlen(all_ascii_characters)+1];
    strcpy(text, all_ascii_characters);

    always_expect(getTextInEditorBuffer, will_return(text));
    always_expect(getSizeOfEditorBuffer, will_return(strlen(all_ascii_characters)));
    always_expect(setSizeOfEditorBuffer);

    performEncodingAdjustments(buffer);

    assert_that(text, is_equal_to_string(all_ascii_characters));
}
