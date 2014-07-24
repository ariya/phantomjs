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


class AmountChangedPattern:
    def __init__(self, compile_regex, index_for_zero_value):
        self._regex = compile_regex
        self._index_for_zero_value = index_for_zero_value

    def match(self, path, diff_file):
        examined_strings = set()
        for diff_line in diff_file:
            if diff_line[self._index_for_zero_value]:
                continue
            match = self._regex.search(diff_line[2])
            if not match:
                continue
            matching_string = match.group(0)
            if matching_string in examined_strings:
                continue
            if self._instance_difference(diff_file, matching_string) > 0:
                return True
            # Avoid reprocessing this same string.
            examined_strings.add(matching_string)
        return False

    def _instance_difference(self, diff_file, matching_string):
        '''Returns the difference between the number of string occurences in
        the added lines and deleted lines (which one is subtracted from the
        other depends on _index_for_zero_value).'''
        count = 0
        for diff_line in diff_file:
            # If the line is unchanged, then don't examine it.
            if diff_line[self._index_for_zero_value] and diff_line[1 - self._index_for_zero_value]:
                continue
            location_found = -len(matching_string)
            while True:
                location_found = diff_line[2].find(matching_string, location_found + len(matching_string))
                if location_found == -1:
                    break
                if not diff_line[self._index_for_zero_value]:
                    count += 1
                else:
                    count -= 1
        return count
