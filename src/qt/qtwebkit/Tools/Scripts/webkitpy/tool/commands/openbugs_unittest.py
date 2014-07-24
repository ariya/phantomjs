# Copyright (C) 2009 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

from webkitpy.tool.commands.commandtest import CommandsTest
from webkitpy.tool.commands.openbugs import OpenBugs

class OpenBugsTest(CommandsTest):

    find_bugs_in_string_expectations = [
        ["123", []],
        ["1234", ["1234"]],
        ["12345", ["12345"]],
        ["123456", ["123456"]],
        ["1234567", []],
        [" 123456 234567", ["123456", "234567"]],
    ]

    def test_find_bugs_in_string(self):
        openbugs = OpenBugs()
        for expectation in self.find_bugs_in_string_expectations:
            self.assertEqual(openbugs._find_bugs_in_string(expectation[0]), expectation[1])

    def test_args_parsing(self):
        expected_logs = "2 bugs found in input.\nMOCK: user.open_url: http://example.com/12345\nMOCK: user.open_url: http://example.com/23456\n"
        self.assert_execute_outputs(OpenBugs(), ["12345\n23456"], expected_logs=expected_logs)
