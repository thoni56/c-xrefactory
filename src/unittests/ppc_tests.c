#include <cgreen/cgreen.h>

#include "ppc.h"

#include "filetable.mock"


FILE *communicationChannel;

Describe(Ppc);
BeforeEach(Ppc) {
    communicationChannel = stdout;
}
AfterEach(Ppc) {}

char buf[1000];
void mocked_fprintf(FILE *stream, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);
}


Ensure(Ppc, can_beep_in_bottom_warnings) {
    ppcBottomWarning("some warning");
    assert_that(buf, contains_string("beep"));
}
