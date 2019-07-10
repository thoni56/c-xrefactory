#! /usr/bin/env python3

import unittest

from cxref_reader import unpack_positions, SymbolPosition, unpack_files, FileReference, unpack_symbols, Symbol


class TestPositionUnpacker(unittest.TestCase):

    def test_empty_line_gives_empty_positions(self):
        self.assertEqual(unpack_positions(""), [])

    def test_explicit_file_line_col_returns_them(self):
        ref = unpack_positions("4uA1f2l3cr")
        self.assertEqual([SymbolPosition(1, 2, 3)], ref)

    def test_another_explicit_file_returns_position(self):
        ref = unpack_positions("4uA2f3l4cr")
        self.assertEqual([SymbolPosition(2, 3, 4)], ref)

    def test_no_file_uses_argument(self):
        ref = unpack_positions("4uA2l3cr", fileid=1)
        self.assertEqual([SymbolPosition(1, 2, 3)], ref)

    def test_no_line_uses_argument(self):
        ref = unpack_positions("4uA1f3cr", lineno=2)
        self.assertEqual([SymbolPosition(1, 2, 3)], ref)

    def test_no_column_uses_argument(self):
        ref = unpack_positions("4uA1f2lr", colno=3)
        self.assertEqual([SymbolPosition(1, 2, 3)], ref)

    def test_two_positions_are_unpacked_to_a_list_of_two(self):
        ref = unpack_positions("4uA1f2l3cr4f5l6cr")
        self.assertEqual([SymbolPosition(1, 2, 3),
                          SymbolPosition(4, 5, 6)], ref)

    def test_two_positions_are_unpacked_to_a_list_of_two_even_if_second_is_incomplete(self):
        ref = unpack_positions("4uA1f2l3cr5l6cr")
        self.assertEqual([SymbolPosition(1, 2, 3),
                          SymbolPosition(1, 5, 6)], ref)


class TestFileReferenceUnpacker(unittest.TestCase):

    def test_returns_empty_list_for_empty_list(self):
        files = unpack_files([])
        self.assertEqual(files, [])

    def test_returns_empty_list_for_only_empty_lines(self):
        files = unpack_files(["", "", ""])
        self.assertEqual(len(files), 0)

    def test_ignores_non_file_lines(self):
        files = unpack_files(["34v file format: C-xrefactory 1.6.0 "])
        self.assertEqual(len(files), 0)

    def test_returns_the_fileid_for_one_line(self):
        files = unpack_files(
            ["3191f 1562271264p m1ia 71:/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int2.c"])
        self.assertEqual(len(files), 1)
        self.assertEqual(files[0].fileid, 3191)
        self.assertEqual(
            files[0].filename, "/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int2.c")

    def test_returns_two_filepositions_for_two_valid_lines(self):
        files = unpack_files([
            "3191f 1562271264p m1ia 71:/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int2.c",
            "9708f 1562271254p  71:/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int1.c"])
        self.assertEqual(len(files), 2)
        self.assertEqual(files[0].fileid, 3191)
        self.assertEqual(
            files[0].filename, "/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int2.c")
        self.assertEqual(files[1].fileid, 9708)
        self.assertEqual(
            files[1].filename, "/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int1.c")


class TestSymbolUnpacker(unittest.TestCase):

    def test_returns_empty_list_for_no_lines(self):
        symbols = unpack_symbols([])
        self.assertEqual(symbols, [])

    def test_returns_one_symbol_for_one_line(self):
        symbols = unpack_symbols(
            ["t24597d24597ha4g\t27/single_int_on_line_1_col_4\t4uA20900f1l4cr4lcr48151f1l4cr4lcr"])
        self.assertEqual(len(symbols), 1)
        self.assertEqual(symbols[0].symbolname, "single_int_on_line_1_col_4")

    def test_ignores_lines_not_starting_with_t(self):
        symbols = unpack_symbols(
            ["", "111", "t24597d24597ha4g\t27/single_int_on_line_1_col_4\t4uA20900f1l4cr4lcr48151f1l4cr4lcr"])
        self.assertEqual(len(symbols), 1)
        self.assertEqual(symbols[0].symbolname, "single_int_on_line_1_col_4")

    def test_returns_two_symbols_for_two_valid_lines(self):
        symbols = unpack_symbols(
            ["t24597d24597ha4g\t27/single_int_on_line_1_col_4\t4uA20900f1l4cr4lcr48151f1l4cr4lcr",
             "t24597d24597ha4g\t27/single_int_on_line_2_col_5\t4uA20900f2l5cr5l1cr48151f2l5cr5l1cr"])
        self.assertEqual(len(symbols), 2)
        self.assertEqual(symbols[0].symbolname, "single_int_on_line_1_col_4")
        self.assertEqual(symbols[1].symbolname, "single_int_on_line_2_col_5")

    def test_stores_position_reference(self):
        symbols = unpack_symbols(
            ["t24597d24597ha4g\t27/single_int_on_line_1_col_4\t4uA20900f1l4cr4lcr48151f1l4cr4lcr",
             "t24597d24597ha4g\t27/single_int_on_line_2_col_5\t4uA20900f2l5cr5l1cr48151f2l5cr5l1cr"])
        self.assertEqual(len(symbols), 2)
        self.assertEqual(symbols[0].positions,
                         "4uA20900f1l4cr4lcr48151f1l4cr4lcr")
        self.assertEqual(symbols[1].positions,
                         "4uA20900f2l5cr5l1cr48151f2l5cr5l1cr")


if __name__ == '__main__':
    unittest.main()
