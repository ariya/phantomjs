# Copyright (C) 2010 Google Inc. All rights reserved.
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

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.mocktool import MockOptions, MockTool
from webkitpy.tool.steps.validatechangelogs import ValidateChangeLogs


class ValidateChangeLogsTest(unittest.TestCase):

    def _assert_start_line_produces_output(self, start_line, should_fail=False, non_interactive=False):
        tool = MockTool()
        step = ValidateChangeLogs(tool, MockOptions(git_commit=None, non_interactive=non_interactive))
        diff_file = Mock()
        diff_file.filename = "mock/ChangeLog"
        diff_file.lines = [(start_line, start_line, "foo")]
        expected_stdout = expected_stderr = expected_logs = ""
        if should_fail and not non_interactive:
            expected_logs = "The diff to mock/ChangeLog looks wrong. Are you sure your ChangeLog entry is at the top of the file?\nOK to continue?\n"
        result = OutputCapture().assert_outputs(self, step._check_changelog_diff, [diff_file], expected_logs=expected_logs)
        self.assertEqual(not result, should_fail)

    def test_check_changelog_diff(self):
        self._assert_start_line_produces_output(1)
        self._assert_start_line_produces_output(7)
        self._assert_start_line_produces_output(8, should_fail=True)

        self._assert_start_line_produces_output(1, non_interactive=False)
        self._assert_start_line_produces_output(8, non_interactive=True, should_fail=True)

    def test_changelog_contains_oops(self):
        tool = MockTool()
        tool._checkout.is_path_to_changelog = lambda path: True
        step = ValidateChangeLogs(tool, MockOptions(git_commit=None, non_interactive=True, check_oops=True))
        diff_file = Mock()
        diff_file.filename = "mock/ChangeLog"
        diff_file.lines = [(1, 1, "foo"), (2, 2, "bar OOPS! bar"), (3, 3, "foo")]
        self.assertTrue(OutputCapture().assert_outputs(self, step._changelog_contains_oops, [diff_file], expected_logs=''))

        diff_file.lines = [(1, 1, "foo"), (2, 2, "bar OOPS bar"), (3, 3, "foo")]
        self.assertFalse(OutputCapture().assert_outputs(self, step._changelog_contains_oops, [diff_file], expected_logs=''))
