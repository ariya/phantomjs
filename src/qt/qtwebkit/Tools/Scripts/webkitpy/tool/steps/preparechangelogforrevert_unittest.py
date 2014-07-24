# Copyright (C) 2011 Google Inc. All rights reserved.
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

from webkitpy.common.checkout.changelog import ChangeLog
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.tool.steps.preparechangelogforrevert import *


class UpdateChangeLogsForRevertTest(unittest.TestCase):
    _revert_entry_with_bug_url = '''2009-08-19  Eric Seidel  <eric@webkit.org>

        Unreviewed, rolling out r12345.
        http://trac.webkit.org/changeset/12345
        http://example.com/123

        Reason

        * Scripts/bugzilla-tool:
'''

    _revert_entry_without_bug_url = '''2009-08-19  Eric Seidel  <eric@webkit.org>

        Unreviewed, rolling out r12345.
        http://trac.webkit.org/changeset/12345

        Reason

        * Scripts/bugzilla-tool:
'''

    _multiple_revert_entry_with_bug_url = '''2009-08-19  Eric Seidel  <eric@webkit.org>

        Unreviewed, rolling out r12345, r12346, and r12347.
        http://trac.webkit.org/changeset/12345
        http://trac.webkit.org/changeset/12346
        http://trac.webkit.org/changeset/12347
        http://example.com/123

        Reason

        * Scripts/bugzilla-tool:
'''

    _multiple_revert_entry_without_bug_url = '''2009-08-19  Eric Seidel  <eric@webkit.org>

        Unreviewed, rolling out r12345, r12346, and r12347.
        http://trac.webkit.org/changeset/12345
        http://trac.webkit.org/changeset/12346
        http://trac.webkit.org/changeset/12347

        Reason

        * Scripts/bugzilla-tool:
'''

    _revert_with_log_reason = """2009-08-19  Eric Seidel  <eric@webkit.org>

        Unreviewed, rolling out r12345.
        http://trac.webkit.org/changeset/12345
        http://example.com/123

        This is a very long reason which should be long enough so that
        _message_for_revert will need to wrap it.  We'll also include
        a
        https://veryveryveryveryverylongbugurl.com/reallylongbugthingy.cgi?bug_id=12354
        link so that we can make sure we wrap that right too.

        * Scripts/bugzilla-tool:
"""

    def _assert_message_for_revert_output(self, args, expected_entry):
        changelog_contents = u"%s\n%s" % (changelog_unittest.ChangeLogTest._new_entry_boilerplate, changelog_unittest.ChangeLogTest._example_changelog)
        changelog_path = "ChangeLog"
        fs = MockFileSystem({changelog_path: changelog_contents.encode("utf-8")})
        changelog = ChangeLog(changelog_path, fs)
        changelog.update_with_unreviewed_message(PrepareChangeLogForRevert._message_for_revert(*args))
        actual_entry = changelog.latest_entry()
        self.assertMultiLineEqual(actual_entry.contents(), expected_entry)
        self.assertIsNone(actual_entry.reviewer_text())
        # These checks could be removed to allow this to work on other entries:
        self.assertEqual(actual_entry.author_name(), "Eric Seidel")
        self.assertEqual(actual_entry.author_email(), "eric@webkit.org")

    def test_message_for_revert(self):
        self._assert_message_for_revert_output([[12345], "Reason"], self._revert_entry_without_bug_url)
        self._assert_message_for_revert_output([[12345], "Reason", "http://example.com/123"], self._revert_entry_with_bug_url)
        self._assert_message_for_revert_output([[12345, 12346, 12347], "Reason"], self._multiple_revert_entry_without_bug_url)
        self._assert_message_for_revert_output([[12345, 12346, 12347], "Reason", "http://example.com/123"], self._multiple_revert_entry_with_bug_url)
        long_reason = "This is a very long reason which should be long enough so that _message_for_revert will need to wrap it.  We'll also include a https://veryveryveryveryverylongbugurl.com/reallylongbugthingy.cgi?bug_id=12354 link so that we can make sure we wrap that right too."
        self._assert_message_for_revert_output([[12345], long_reason, "http://example.com/123"], self._revert_with_log_reason)
