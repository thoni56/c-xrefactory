#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/unit.h>

#include "encoding.h"

#include "commons.mock"
#include "editorbuffer.mock"
#include "options.mock"


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

Ensure(Encoding, will_convert_utf8_character) {
    const char *some_utf8_characters =
        "àö";
    EditorBuffer *buffer = (EditorBuffer*)0x13131313;

    char text[strlen(some_utf8_characters)+1];
    strcpy(text, some_utf8_characters);

    always_expect(getTextInEditorBuffer, will_return(text));
    always_expect(getSizeOfEditorBuffer, will_return(strlen(some_utf8_characters)));

    expect(setSizeOfEditorBuffer, when(size, is_equal_to(2)));

    performEncodingAdjustments(buffer);
    text[2] = '\0'; /* Any shortening of the text will not necessarily null-terminate it */

    assert_that(text, is_equal_to_string("  "));
}
