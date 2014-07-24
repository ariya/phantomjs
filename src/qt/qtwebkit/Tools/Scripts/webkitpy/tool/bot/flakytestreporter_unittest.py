# Copyright (c) 2010 Google Inc. All rights reserved.
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

from webkitpy.common.config.committers import Committer
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.layout_tests.models import test_results
from webkitpy.layout_tests.models import test_failures
from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.bot.flakytestreporter import FlakyTestReporter
from webkitpy.tool.mocktool import MockTool
from webkitpy.common.net.statusserver_mock import MockStatusServer


# Creating fake CommitInfos is a pain, so we use a mock one here.
class MockCommitInfo(object):
    def __init__(self, author_email):
        self._author_email = author_email

    def author(self):
        # It's definitely possible to have commits with authors who
        # are not in our contributors.json list.
        if not self._author_email:
            return None
        return Committer("Mock Committer", self._author_email)


class FlakyTestReporterTest(unittest.TestCase):
    def _mock_test_result(self, testname):
        return test_results.TestResult(testname, [test_failures.FailureTextMismatch()])

    def _assert_emails_for_test(self, emails):
        tool = MockTool()
        reporter = FlakyTestReporter(tool, 'dummy-queue')
        commit_infos = [MockCommitInfo(email) for email in emails]
        tool.checkout().recent_commit_infos_for_files = lambda paths: set(commit_infos)
        self.assertEqual(reporter._author_emails_for_test([]), set(emails))

    def test_author_emails_for_test(self):
        self._assert_emails_for_test([])
        self._assert_emails_for_test(["test1@test.com", "test1@test.com"])
        self._assert_emails_for_test(["test1@test.com", "test2@test.com"])

    def test_create_bug_for_flaky_test(self):
        reporter = FlakyTestReporter(MockTool(), 'dummy-queue')
        expected_logs = """MOCK create_bug
bug_title: Flaky Test: foo/bar.html
bug_description: This is an automatically generated bug from the dummy-queue.
foo/bar.html has been flaky on the dummy-queue.

foo/bar.html was authored by test@test.com.
http://trac.webkit.org/browser/trunk/LayoutTests/foo/bar.html

FLAKE_MESSAGE

The bots will update this with information from each new failure.

If you believe this bug to be fixed or invalid, feel free to close.  The bots will re-open if the flake re-occurs.

If you would like to track this test fix with another bug, please close this bug as a duplicate.  The bots will follow the duplicate chain when making future comments.

component: Tools / Tests
cc: test@test.com
blocked: 50856
"""
        OutputCapture().assert_outputs(self, reporter._create_bug_for_flaky_test, ['foo/bar.html', ['test@test.com'], 'FLAKE_MESSAGE'], expected_logs=expected_logs)

    def test_follow_duplicate_chain(self):
        tool = MockTool()
        reporter = FlakyTestReporter(tool, 'dummy-queue')
        bug = tool.bugs.fetch_bug(50004)
        self.assertEqual(reporter._follow_duplicate_chain(bug).id(), 50002)

    def test_report_flaky_tests_creating_bug(self):
        tool = MockTool()
        tool.filesystem = MockFileSystem({"/mock-results/foo/bar-diffs.txt": "mock"})
        tool.status_server = MockStatusServer(bot_id="mock-bot-id")
        reporter = FlakyTestReporter(tool, 'dummy-queue')
        reporter._lookup_bug_for_flaky_test = lambda bug_id: None
        patch = tool.bugs.fetch_attachment(10000)
        expected_logs = """Bug does not already exist for foo/bar.html, creating.
MOCK create_bug
bug_title: Flaky Test: foo/bar.html
bug_description: This is an automatically generated bug from the dummy-queue.
foo/bar.html has been flaky on the dummy-queue.

foo/bar.html was authored by abarth@webkit.org.
http://trac.webkit.org/browser/trunk/LayoutTests/foo/bar.html

The dummy-queue just saw foo/bar.html flake (text diff) while processing attachment 10000 on bug 50000.
Bot: mock-bot-id  Port: MockPort  Platform: MockPlatform 1.0

The bots will update this with information from each new failure.

If you believe this bug to be fixed or invalid, feel free to close.  The bots will re-open if the flake re-occurs.

If you would like to track this test fix with another bug, please close this bug as a duplicate.  The bots will follow the duplicate chain when making future comments.

component: Tools / Tests
cc: abarth@webkit.org
blocked: 50856
MOCK add_attachment_to_bug: bug_id=60001, description=Failure diff from mock-bot-id filename=failure.diff mimetype=None
MOCK bug comment: bug_id=50000, cc=None
--- Begin comment ---
The dummy-queue encountered the following flaky tests while processing attachment 10000:

foo/bar.html bug 60001 (author: abarth@webkit.org)
The dummy-queue is continuing to process your patch.
--- End comment ---

"""
        test_results = [self._mock_test_result('foo/bar.html')]

        class MockZipFile(object):
            def read(self, path):
                return ""

            def namelist(self):
                return ['foo/bar-diffs.txt']

        OutputCapture().assert_outputs(self, reporter.report_flaky_tests, [patch, test_results, MockZipFile()], expected_logs=expected_logs)

    def test_optional_author_string(self):
        reporter = FlakyTestReporter(MockTool(), 'dummy-queue')
        self.assertEqual(reporter._optional_author_string([]), "")
        self.assertEqual(reporter._optional_author_string(["foo@bar.com"]), " (author: foo@bar.com)")
        self.assertEqual(reporter._optional_author_string(["a@b.com", "b@b.com"]), " (authors: a@b.com and b@b.com)")

    def test_results_diff_path_for_test(self):
        reporter = FlakyTestReporter(MockTool(), 'dummy-queue')
        self.assertEqual(reporter._results_diff_path_for_test("test.html"), "test-diffs.txt")

    def test_find_in_archive(self):
        reporter = FlakyTestReporter(MockTool(), 'dummy-queue')

        class MockZipFile(object):
            def namelist(self):
                return ["tmp/layout-test-results/foo/bar-diffs.txt"]

        reporter._find_in_archive("foo/bar-diffs.txt", MockZipFile())
        # This is not ideal, but its
        reporter._find_in_archive("txt", MockZipFile())
