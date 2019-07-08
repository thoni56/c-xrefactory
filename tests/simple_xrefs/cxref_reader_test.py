#! /usr/bin/env python3

import unittest

from cxref_reader import unpack_refs, Reference

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
        ref = unpack_refs("2l3cr", fileno=1)
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

if __name__ == '__main__':
    unittest.main()
