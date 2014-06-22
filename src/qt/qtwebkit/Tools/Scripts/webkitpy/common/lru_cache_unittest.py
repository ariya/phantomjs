# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import unittest2 as unittest

from webkitpy.common import lru_cache


class LRUCacheTest(unittest.TestCase):

    def setUp(self):
        self.lru = lru_cache.LRUCache(3)
        self.lru['key_1'] = 'item_1'
        self.lru['key_2'] = 'item_2'
        self.lru['key_3'] = 'item_3'

        self.lru2 = lru_cache.LRUCache(1)
        self.lru2['key_1'] = 'item_1'

    def test_items(self):
        self.assertEqual(set(self.lru.items()), set([('key_1', 'item_1'), ('key_3', 'item_3'), ('key_2', 'item_2')]))

    def test_put(self):
        self.lru['key_4'] = 'item_4'
        self.assertEqual(set(self.lru.items()), set([('key_4', 'item_4'), ('key_3', 'item_3'), ('key_2', 'item_2')]))

    def test_update(self):
        self.lru['key_1']
        self.lru['key_5'] = 'item_5'
        self.assertEqual(set(self.lru.items()), set([('key_1', 'item_1'), ('key_3', 'item_3'), ('key_5', 'item_5')]))

    def test_keys(self):
        self.assertEqual(set(self.lru.keys()), set(['key_1', 'key_2', 'key_3']))

    def test_delete(self):
        del self.lru['key_1']
        self.assertFalse('key_1' in self.lru)

    def test_contain(self):
        self.assertTrue('key_1' in self.lru)
        self.assertFalse('key_4' in self.lru)

    def test_values(self):
        self.assertEqual(set(self.lru.values()), set(['item_1', 'item_2', 'item_3']))

    def test_len(self):
        self.assertEqual(len(self.lru), 3)

    def test_size_one_pop(self):
        self.lru2['key_2'] = 'item_2'
        self.assertEqual(self.lru2.keys(), ['key_2'])

    def test_size_one_delete(self):
        del self.lru2['key_1']
        self.assertFalse('key_1' in self.lru2)

    def test_pop_error(self):
        self.assertRaises(KeyError, self.lru2.__getitem__, 'key_2')
        del self.lru2['key_1']
        self.assertRaises(KeyError, self.lru2.__getitem__, 'key_2')

    def test_get_middle_item(self):
        self.lru['key_2']
        self.lru['key_4'] = 'item_4'
        self.lru['key_5'] = 'item_5'
        self.assertEqual(set(self.lru.keys()), set(['key_2', 'key_4', 'key_5']))

    def test_set_again(self):
        self.lru['key_1'] = 'item_4'
        self.assertEqual(set(self.lru.items()), set([('key_1', 'item_4'), ('key_3', 'item_3'), ('key_2', 'item_2')]))
