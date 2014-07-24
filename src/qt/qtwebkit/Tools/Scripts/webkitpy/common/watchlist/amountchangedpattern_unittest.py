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


'''Unit tests for amountchangedpattern.py.'''


import re
import unittest2 as unittest


from webkitpy.common.watchlist.amountchangedpattern import AmountChangedPattern


class AmountChangedPatternTest(unittest.TestCase):

    # A quick note about the diff file structure.
    # The first column indicated the old line number.
    # The second column indicates the new line number.
    # 0 in either column indicates it had no old or new line number.
    _DIFF_FILE = ((0, 1, 'hi hi'),
                  (1, 0, 'bye hi'),
                  (2, 2, 'other hi'),
                  (3, 0, 'both'),
                  (0, 3, 'both'),
                  )

    def run_amount_changed_pattern_match(self, pattern, index_for_zero_value):
        return AmountChangedPattern(re.compile(pattern), index_for_zero_value).match(None, self._DIFF_FILE)

    def test_added_lines(self):
        self.assertTrue(self.run_amount_changed_pattern_match('hi', 0))
        self.assertTrue(self.run_amount_changed_pattern_match('hi hi', 0))
        self.assertFalse(self.run_amount_changed_pattern_match('other', 0))
        self.assertFalse(self.run_amount_changed_pattern_match('both', 0))
        self.assertFalse(self.run_amount_changed_pattern_match('bye', 0))
        self.assertFalse(self.run_amount_changed_pattern_match('MatchesNothing', 0))

    def test_removed_lines(self):
        self.assertFalse(self.run_amount_changed_pattern_match('hi', 1))
        self.assertFalse(self.run_amount_changed_pattern_match('hi hi', 1))
        self.assertFalse(self.run_amount_changed_pattern_match('other', 1))
        self.assertFalse(self.run_amount_changed_pattern_match('both', 1))
        self.assertTrue(self.run_amount_changed_pattern_match('bye', 1))
        self.assertFalse(self.run_amount_changed_pattern_match('MatchesNothing', 1))
