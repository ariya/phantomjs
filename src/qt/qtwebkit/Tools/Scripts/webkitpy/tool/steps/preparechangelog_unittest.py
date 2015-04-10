# Copyright (C) 2010 Google Inc. All rights reserved.
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

import unittest2 as unittest

# Do not import changelog_unittest.ChangeLogTest directly as that will cause it to be run again.
from webkitpy.common.checkout import changelog_unittest

from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.mocktool import MockOptions, MockTool
from webkitpy.tool.steps.preparechangelog import PrepareChangeLog

class PrepareChangeLogTest(changelog_unittest.ChangeLogTest):
    def test_resolve_existing_entry(self):
        step = PrepareChangeLog(MockTool(), MockOptions())

        headers = ["2013-01-18  Timothy Loh  <timloh@chromium.com>\n\n",
                   "2013-01-20  Timothy Loh  <timloh@chromium.com>\n\n",
                  u"2009-08-17  Tor Arne Vestb\xf8  <vestbo@webkit.org>\n\n",
                  u"2009-08-18  Tor Arne Vestb\xf8  <vestbo@webkit.org>\n\n",
                   "2013-01-18  Eric Seidel  <eric@webkit.org>\n\n",
                   "2013-01-20  Eric Seidel  <eric@webkit.org>\n\n",
                  ]

        bug_descs = ["        prepare-Changelog should support updating the list of changed files\n",
                     "        webkit-patch upload should support updating the list of changed files\n"]

        bug_url = "        https://bugs.webkit.org/show_bug.cgi?id=74358\n\n"

        descriptions = ["", "        A description of the changes.\n\n",
                "        A description.\n\n        With some\n        line breaks\n\n"]

        changes = [
"""        * Scripts/webkitpy/tool/steps/preparechangelog.py:
        (PrepareChangeLog):
        (PrepareChangeLog.run):\n\n""",
"""        * Scripts/webkitpy/tool/steps/preparechangelog.py:
        (PrepareChangeLog._resolve_existing_entry):
        (PrepareChangeLog):
        (PrepareChangeLog.run):\n\n""",
"""        * Scripts/webkitpy/tool/steps/preparechangelog.py:
        (PrepareChangeLog): Some annotations
        (PrepareChangeLog.run):
            More annotations\n\n""",
"""        * Scripts/webkitpy/tool/steps/preparechangelog.py:
        (PrepareChangeLog): Some annotations
        (PrepareChangeLog.run):
            More annotations

        * Scripts/webkitpy/tool/steps/preparechangelog.py:
        (PrepareChangeLog._resolve_existing_entry):
        (PrepareChangeLog):
        (PrepareChangeLog.run):\n\n""",
            ]

        def make_entry(indices):
            a, b, c, d = indices
            return headers[a] + bug_descs[b] + bug_url + descriptions[c] + changes[d]

        test_cases = [((0, 0, 0, 0), (0, 0, 0, 0), (0, 0, 0, 0)),
                      ((0, 0, 0, 0), (0, 0, 1, 0), (0, 0, 1, 0)),
                      ((1, 0, 0, 0), (0, 0, 2, 0), (1, 0, 2, 0)),
                      ((0, 1, 0, 0), (0, 0, 1, 0), (0, 1, 1, 0)),
                      ((0, 0, 0, 1), (0, 0, 0, 0), (0, 0, 0, 1)),
                      ((0, 0, 0, 0), (0, 0, 1, 1), (0, 0, 1, 0)),
                      ((0, 0, 0, 0), (0, 0, 2, 2), (0, 0, 2, 2)),
                      ((0, 0, 0, 1), (0, 0, 1, 2), (0, 0, 1, 3)),
                      ((1, 1, 0, 1), (0, 0, 0, 2), (1, 1, 0, 3)),
                      ((3, 0, 0, 0), (2, 0, 1, 0), (3, 0, 1, 0)),
                      ((4, 0, 0, 0), (0, 0, 0, 0), (0, 0, 0, 0)),
                      ((5, 0, 0, 0), (0, 0, 0, 0), (1, 0, 0, 0)),
                      ((0, 0, 0, 0), (4, 0, 0, 0), (4, 0, 0, 0)),
                      ((1, 0, 0, 0), (4, 0, 0, 0), (5, 0, 0, 0)),
        ]

        for new, old, final in test_cases:
            new_entry = make_entry(new)
            old_entry = make_entry(old)
            start_file = new_entry + old_entry + self._rolled_over_footer

            final_entry = make_entry(final)
            end_file = final_entry + self._rolled_over_footer

            path = "ChangeLog"
            step._tool.filesystem = MockFileSystem()
            step._tool.filesystem.write_text_file(path, start_file)
            step._resolve_existing_entry(path)
            actual_output = step._tool.filesystem.read_text_file(path)
            self.assertEquals(actual_output, end_file)

    def test_ensure_bug_url(self):
        capture = OutputCapture()
        step = PrepareChangeLog(MockTool(), MockOptions())
        changelog_contents = u"%s\n%s" % (self._new_entry_boilerplate, self._example_changelog)
        changelog_path = "ChangeLog"
        state = {
            "bug_title": "Example title",
            "bug_id": 1234,
            "changelogs": [changelog_path],
        }
        step._tool.filesystem = MockFileSystem()
        step._tool.filesystem.write_text_file(changelog_path, changelog_contents)
        capture.assert_outputs(self, step._ensure_bug_url, [state])
        actual_contents = step._tool.filesystem.read_text_file(changelog_path)
        expected_message = "Example title\n        http://example.com/1234"
        expected_contents = changelog_contents.replace("Need a short description (OOPS!).\n        Need the bug URL (OOPS!).", expected_message)
        self.assertEqual(actual_contents, expected_contents)
