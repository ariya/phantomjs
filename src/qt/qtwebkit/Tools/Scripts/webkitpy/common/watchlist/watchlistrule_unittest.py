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
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
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
from webkitpy.common.watchlist.watchlistrule import WatchListRule


class WatchListRuleTest(unittest.TestCase):
    def test_instruction_list(self):
        instructions = ['a', 'b']
        rule = WatchListRule('definition1', instructions[:])
        self.assertEqual(instructions, rule.instructions())

    def test_remove_instruction(self):
        instructions = ['a', 'b']
        rule = WatchListRule('definition1', instructions[:])
        rule.remove_instruction('b')
        self.assertEqual(['a'], rule.instructions())

    def test_simple_definition(self):
        definition_name = 'definition1'
        rule = WatchListRule(definition_name, [])
        self.assertTrue(rule.match([definition_name]))
        self.assertFalse(rule.match([definition_name + '1']))

    def test_complex_definition(self):
        definition_name1 = 'definition1'
        definition_name2 = 'definition2'
        definition_name3 = 'definition3'
        rule = WatchListRule(definition_name1 + '|' + definition_name2 + '|' + definition_name3, [])
        self.assertTrue(rule.match([definition_name1]))
        self.assertTrue(rule.match([definition_name2]))
        self.assertTrue(rule.match([definition_name3]))
        self.assertFalse(rule.match([definition_name1 + '1']))
        self.assertFalse(rule.match([definition_name2 + '1']))
        self.assertFalse(rule.match([definition_name3 + '1']))
