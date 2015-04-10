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

from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.mocktool import MockOptions, MockTool
from webkitpy.tool.steps.discardlocalchanges import DiscardLocalChanges
from webkitpy.common.system.executive import ScriptError


class DiscardLocalChangesTest(unittest.TestCase):
    def test_skip_on_clean(self):
        tool = MockTool()
        tool._scm = Mock()
        step = DiscardLocalChanges(tool, MockOptions(clean=False))
        step.run({})
        self.assertEqual(tool._scm.discard_local_changes.call_count, 0)

    def test_working_changes_exist_with_force(self):
        tool = MockTool()
        tool._scm = Mock()
        tool._scm.has_working_directory_changes = lambda: True
        tool._scm.has_local_commits = lambda: False
        step = DiscardLocalChanges(tool, MockOptions(clean=True, force_clean=True))
        step.run({})
        self.assertEqual(tool._scm.discard_local_changes.call_count, 1)

    def test_local_commits_exist_with_force(self):
        tool = MockTool()
        tool._scm = Mock()
        tool._scm.has_working_directory_changes = lambda: False
        tool._scm.has_local_commits = lambda: True
        step = DiscardLocalChanges(tool, MockOptions(clean=True, force_clean=True))
        step.run({})
        self.assertEqual(tool._scm.discard_local_changes.call_count, 1)

    def test_local_commits_and_working_changes_exist_with_force(self):
        tool = MockTool()
        tool._scm = Mock()
        tool._scm.has_working_directory_changes = lambda: True
        tool._scm.has_local_commits = lambda: True
        step = DiscardLocalChanges(tool, MockOptions(clean=True, force_clean=True))
        step.run({})
        self.assertEqual(tool._scm.discard_local_changes.call_count, 1)

    def test_no_changes_exist_with_force(self):
        tool = MockTool()
        tool._scm = Mock()
        tool._scm.has_working_directory_changes = lambda: False
        tool._scm.has_local_commits = lambda: False
        step = DiscardLocalChanges(tool, MockOptions(clean=True, force_clean=True))
        step.run({})
        self.assertEqual(tool._scm.discard_local_changes.call_count, 1)

    def test_error_working_changes_exist_without_force(self):
        tool = MockTool()
        tool._scm = Mock()
        tool._scm.has_working_directory_changes = lambda: True
        tool._scm.has_local_commits = lambda: False
        step = DiscardLocalChanges(tool, MockOptions(clean=True, force_clean=False))
        self.assertRaises(ScriptError, step.run, {})
        self.assertEqual(tool._scm.discard_local_changes.call_count, 0)

    def test_error_local_commits_exist_without_force(self):
        tool = MockTool()
        tool._scm = Mock()
        tool._scm.has_working_directory_changes = lambda: False
        tool._scm.has_local_commits = lambda: True
        step = DiscardLocalChanges(tool, MockOptions(clean=True, force_clean=False))
        self.assertRaises(ScriptError, step.run, {})
        self.assertEqual(tool._scm.discard_local_changes.call_count, 0)
