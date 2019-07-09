#! /usr/bin/env python3

import unittest

from cxref_reader import unpack_refs, Reference, unpack_files, FileReference


class TestReferenceUnpacker(unittest.TestCase):

    def test_empty_line_gives_empty_references(self):
        self.assertEqual(unpack_refs(""), [])

    def test_explicit_file_line_col_returns_them(self):
        ref = unpack_refs("1f2l3cr")
        self.assertEqual([Reference(1, 2, 3)], ref)

    def test_another_explicit_file_returns_reference(self):
        ref = unpack_refs("2f3l4cr")
        self.assertEqual([Reference(2, 3, 4)], ref)

    def test_no_file_uses_argument(self):
        ref = unpack_refs("2l3cr", fileid=1)
        self.assertEqual([Reference(1, 2, 3)], ref)

    def test_no_line_uses_argument(self):
        ref = unpack_refs("1f3cr", lineno=2)
        self.assertEqual([Reference(1, 2, 3)], ref)

    def test_no_column_uses_argument(self):
        ref = unpack_refs("1f2lr", colno=3)
        self.assertEqual([Reference(1, 2, 3)], ref)

    def test_two_references_are_unpacked_to_a_list_of_two(self):
        ref = unpack_refs("1f2l3cr4f5l6cr")
        self.assertEqual([Reference(1, 2, 3),
                          Reference(4, 5, 6)], ref)

    def test_two_references_are_unpacked_to_a_list_of_two_even_if_second_is_incomplete(self):
        ref = unpack_refs("1f2l3cr5l6cr")
        self.assertEqual([Reference(1, 2, 3),
                          Reference(1, 5, 6)], ref)


class TestFileReferenceUnpacker(unittest.TestCase):

    def test_unpack_files_returns_empty_list_for_empty_list(self):
        files = unpack_files([])
        self.assertEqual(files, [])

    def test_unpack_files_returns_empty_list_for_only_empty_lines(self):
        files = unpack_files(["", "", ""])
        self.assertEqual(len(files), 0)

    def test_unpack_files_ignores_non_file_lines(self):
        files = unpack_files(["34v file format: C-xrefactory 1.6.0 "])
        self.assertEqual(len(files), 0)

    def test_unpack_files_returns_the_fileid_for_one_line(self):
        files = unpack_files(["3191f 1562271264p m1ia 71:/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int2.c"])
        self.assertEqual(len(files), 1)
        self.assertEqual(files[0].fileid, "3191")
        self.assertEqual(files[0].filename, "/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int2.c")

    def test_unpack_files_returns_two_filereferences_for_two_valid_lines(self):
        files = unpack_files([
            "3191f 1562271264p m1ia 71:/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int2.c",
            "9708f 1562271254p  71:/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int1.c"])
        self.assertEqual(len(files), 2)
        self.assertEqual(files[0].fileid, "3191")
        self.assertEqual(files[0].filename, "/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int2.c")
        self.assertEqual(files[1].fileid, "9708")
        self.assertEqual(files[1].filename, "/Users/thomas/Utveckling/c-xrefactory/tests/simple_xrefs/single_int1.c")


if __name__ == '__main__':
    unittest.main()
