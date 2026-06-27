#include "options.h"
#include "optionsets.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "log.h"
#include "memory.h"

#include "commons.mock"
#include "cxref.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "ppc.mock"
#include "yylex.mock"


Describe(OptionSets);
BeforeEach(OptionSets) {
    log_set_level(LOG_ERROR);
    memoryInit(&options.memory, "", OptionsMemorySize);
}
AfterEach(OptionSets) {}


static void expectOptionRead(char *string) {
    int len = strlen(string);

    expect(getOptionFromFile,
           will_set_contents_of_output_parameter(foundText, string, len+1),
           will_return('A'));
}


Ensure(OptionSets, readPassOptions_returns_no_passes_for_empty_config) {
    OptionSets d = makeOptionSets();

    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);

    assert_that(d.set[0], is_null);
}

Ensure(OptionSets, readPassOptions_ignores_passN_marker) {
    OptionSets d = makeOptionSets();

    expectOptionRead("-pass1");
    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);

    assert_that(d.set[0], is_null);
    assert_that(d.set[1], is_null);
}

Ensure(OptionSets, readPassOptions_collects_base_option_in_delta_zero) {
    OptionSets d = makeOptionSets();

    expectOptionRead("-DBASE");
    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);   // memory param returns, mirroring readOptionsIntoArgs' (out, memory) order

    assert_that(d.set[0]->string, is_equal_to_string("-DBASE"));
}

Ensure(OptionSets, readPassOptions_routes_option_after_marker_into_that_pass) {
    OptionSets d = makeOptionSets();

    expectOptionRead("-pass1");
    expectOptionRead("-DPASS1");
    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);

    assert_that(d.set[1]->string, is_equal_to_string("-DPASS1"));
}

static bool stringListContains(StringList *d, char *wantedString) {
    for (StringList *s = d; s != NULL; s = s->next) {
        if (strcmp(s->string, wantedString) == 0)
            return true;
    }
    return false;
}

Ensure(OptionSets, readPassOptions_merges_sections_with_same_pass_number) {
    OptionSets d = makeOptionSets();

    expectOptionRead("-pass1");
    expectOptionRead("-DPASS1");
    expectOptionRead("-pass1");
    expectOptionRead("-DPASS2");
    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);

    assert_that(d.set[1], is_non_null);        // both defines live here — membership, not order
    /* assert delta[1] contains -DPASS1 AND -DPASS2 (walk the list, order-agnostic) */
    assert_that(stringListContains(d.set[1], "-DPASS1"));
    assert_that(stringListContains(d.set[1], "-DPASS2"));
    assert_that(d.set[2], is_null);            // THE point: same N merged, did not split
}

Ensure(OptionSets, readPassOptions_skips_section_markers) {
    OptionSets d = makeOptionSets();

    expectOptionRead("[CURDIR]");
    expectOptionRead("-DBASE");
    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);

    assert_that(d.set[0]->string, is_equal_to_string("-DBASE"));   // the marker is
    assert_that(d.set[0]->next, is_null);                          // gone, only the
                                                                     // real option
                                                                     // remains
}

Ensure(OptionSets, readPassOptions_can_handle_consequtive_options_after_one_pass) {
    OptionSets d = makeOptionSets();

    expectOptionRead("-pass1");
    expectOptionRead("-DDEFINE1");
    expectOptionRead("-DDEFINE2");
    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);

    assert_that(stringListContains(d.set[1], "-DDEFINE1"));
    assert_that(stringListContains(d.set[1], "-DDEFINE2"));
}

Ensure(OptionSets, readPassOptions_can_handle_consequtive_options_for_two_passes) {
    OptionSets d = makeOptionSets();

    expectOptionRead("-pass1");
    expectOptionRead("-DDEFINE1");
    expectOptionRead("-DDEFINE2");
    expectOptionRead("-pass2");
    expectOptionRead("-DDEFINE3");
    expectOptionRead("-DDEFINE4");
    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);

    assert_that(stringListContains(d.set[1], "-DDEFINE1"));
    assert_that(stringListContains(d.set[1], "-DDEFINE2"));
    assert_that(stringListContains(d.set[2], "-DDEFINE3"));
    assert_that(stringListContains(d.set[2], "-DDEFINE4"));
}

Ensure(OptionSets, readPassOptions_collects_options_in_source_order) {
    OptionSets d = makeOptionSets();

    expectOptionRead("-pass1");
    expectOptionRead("-DDEFINE1");
    expectOptionRead("-DDEFINE2");
    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);

    assert_that(d.set[1]->string, is_equal_to_string("-DDEFINE1"));
    assert_that(d.set[1]->next->string, is_equal_to_string("-DDEFINE2"));
}

Ensure(OptionSets, readPassOptions_collects_multitoken_options_as_multiple_strings) {
    OptionSets d = makeOptionSets();

    expectOptionRead("-o");
    expectOptionRead("output");
    expect(getOptionFromFile, will_return(EOF));

    readOptionSets(NULL, &d);

    assert_that(d.set[0]->string, is_equal_to_string("-o"));
    assert_that(d.set[0]->next->string, is_equal_to_string("output"));
}

Ensure(OptionSets, argsFromOptionList_empty_delta_yields_only_reserved_slot) {
    ArgumentsVector args;

    args = argsFromOptionList(NULL, &options.memory);

    assert_that(args.argc, is_equal_to(1));
}

Ensure(OptionSets, argsFromPassDelta_puts_single_option_after_reserved_slot) {
    ArgumentsVector args;
    StringList *delta = newStringList("-DPASS1", NULL);

    args = argsFromOptionList(delta, &options.memory);

    assert_that(args.argc, is_equal_to(2));
    assert_that(args.argv[1], is_equal_to_string("-DPASS1"));
}

Ensure(OptionSets, argsFromPassDelta_preserves_source_order) {
    ArgumentsVector args;
    StringList *delta = newStringList("-DPASS1", newStringList("-DPASS2", NULL));

    args = argsFromOptionList(delta, &options.memory);

    assert_that(args.argc, is_equal_to(3));
    assert_that(args.argv[1], is_equal_to_string("-DPASS1"));
    assert_that(args.argv[2], is_equal_to_string("-DPASS2"));
}
