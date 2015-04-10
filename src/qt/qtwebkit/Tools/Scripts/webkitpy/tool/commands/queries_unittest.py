# Copyright (C) 2009 Google Inc. All rights reserved.
# Copyright (C) 2012 Intel Corporation. All rights reserved.
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

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.net.bugzilla import Bugzilla
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.thirdparty.mock import Mock
from webkitpy.port.test import TestPort
from webkitpy.tool.commands.commandtest import CommandsTest
from webkitpy.tool.commands.queries import *
from webkitpy.tool.mocktool import MockTool, MockOptions


class MockTestPort1(object):
    def skips_layout_test(self, test_name):
        return test_name in ["media/foo/bar.html", "foo"]


class MockTestPort2(object):
    def skips_layout_test(self, test_name):
        return test_name == "media/foo/bar.html"


class MockPortFactory(object):
    def __init__(self):
        self._all_ports = {
            "test_port1": MockTestPort1(),
            "test_port2": MockTestPort2(),
        }

    def all_port_names(self, options=None):
        return self._all_ports.keys()

    def get(self, port_name):
        return self._all_ports.get(port_name)


class QueryCommandsTest(CommandsTest):
    def test_bugs_to_commit(self):
        expected_logs = "Warning, attachment 10001 on bug 50000 has invalid committer (non-committer@example.com)\n"
        self.assert_execute_outputs(BugsToCommit(), None, "50000\n50003\n", expected_logs=expected_logs)

    def test_patches_in_commit_queue(self):
        expected_stdout = "http://example.com/10000\nhttp://example.com/10002\n"
        expected_logs = "Warning, attachment 10001 on bug 50000 has invalid committer (non-committer@example.com)\nPatches in commit queue:\n"
        self.assert_execute_outputs(PatchesInCommitQueue(), None, expected_stdout, expected_logs=expected_logs)

    def test_patches_to_commit_queue(self):
        expected_stdout = "http://example.com/10003&action=edit\n"
        expected_logs = "10000 already has cq=+\n10001 already has cq=+\n10004 committer = \"Eric Seidel\" <eric@webkit.org>\n"
        options = Mock()
        options.bugs = False
        self.assert_execute_outputs(PatchesToCommitQueue(), None, expected_stdout, expected_logs=expected_logs, options=options)

        expected_stdout = "http://example.com/50003\n"
        options.bugs = True
        self.assert_execute_outputs(PatchesToCommitQueue(), None, expected_stdout, expected_logs=expected_logs, options=options)

    def test_patches_to_review(self):
        options = Mock()

        # When no cc_email is provided, we use the Bugzilla username by default.
        # The MockBugzilla will fake the authentication using username@webkit.org
        # as login and it should match the username at the report header.
        options.cc_email = None
        options.include_cq_denied = False
        options.all = False
        expected_stdout = \
            "Bugs with attachments pending review that has username@webkit.org in the CC list:\n" \
            "http://webkit.org/b/bugid   Description (age in days)\n" \
            "Total: 0\n"
        expected_stderr = ""
        self.assert_execute_outputs(PatchesToReview(), None, expected_stdout, expected_stderr, options=options)

        options.cc_email = "abarth@webkit.org"
        options.include_cq_denied = True
        options.all = False
        expected_stdout = \
            "Bugs with attachments pending review that has abarth@webkit.org in the CC list:\n" \
            "http://webkit.org/b/bugid   Description (age in days)\n" \
            "http://webkit.org/b/50001   Bug with a patch needing review. (0)\n" \
            "Total: 1\n"
        expected_stderr = ""
        self.assert_execute_outputs(PatchesToReview(), None, expected_stdout, expected_stderr, options=options)

        options.cc_email = None
        options.include_cq_denied = True
        options.all = True
        expected_stdout = \
            "Bugs with attachments pending review:\n" \
            "http://webkit.org/b/bugid   Description (age in days)\n" \
            "http://webkit.org/b/50001   Bug with a patch needing review. (0)\n" \
            "Total: 1\n"
        self.assert_execute_outputs(PatchesToReview(), None, expected_stdout, expected_stderr, options=options)

        options.cc_email = None
        options.include_cq_denied = False
        options.all = True
        expected_stdout = \
            "Bugs with attachments pending review:\n" \
            "http://webkit.org/b/bugid   Description (age in days)\n" \
            "Total: 0\n"
        self.assert_execute_outputs(PatchesToReview(), None, expected_stdout, expected_stderr, options=options)

        options.cc_email = "invalid_email@example.com"
        options.all = False
        options.include_cq_denied = True
        expected_stdout = \
            "Bugs with attachments pending review that has invalid_email@example.com in the CC list:\n" \
            "http://webkit.org/b/bugid   Description (age in days)\n" \
            "Total: 0\n"
        self.assert_execute_outputs(PatchesToReview(), None, expected_stdout, expected_stderr, options=options)

    def test_tree_status(self):
        expected_stdout = "ok   : Builder1\nok   : Builder2\n"
        self.assert_execute_outputs(TreeStatus(), None, expected_stdout)


class FailureReasonTest(unittest.TestCase):
    def test_blame_line_for_revision(self):
        tool = MockTool()
        command = FailureReason()
        command.bind_to_tool(tool)
        # This is an artificial example, mostly to test the CommitInfo lookup failure case.
        self.assertEqual(command._blame_line_for_revision(0), "FAILED to fetch CommitInfo for r0, likely missing ChangeLog")

        def raising_mock(self):
            raise Exception("MESSAGE")
        tool.checkout().commit_info_for_revision = raising_mock
        self.assertEqual(command._blame_line_for_revision(0), "FAILED to fetch CommitInfo for r0, exception: MESSAGE")


class PrintExpectationsTest(unittest.TestCase):
    def run_test(self, tests, expected_stdout, platform='test-win-xp', **args):
        options = MockOptions(all=False, csv=False, full=False, platform=platform,
                              include_keyword=[], exclude_keyword=[], paths=False).update(**args)
        tool = MockTool()
        tool.port_factory.all_port_names = lambda: TestPort.ALL_BASELINE_VARIANTS
        command = PrintExpectations()
        command.bind_to_tool(tool)

        oc = OutputCapture()
        try:
            oc.capture_output()
            command.execute(options, tests, tool)
        finally:
            stdout, _, _ = oc.restore_output()
        self.assertMultiLineEqual(stdout, expected_stdout)

    def test_basic(self):
        self.run_test(['failures/expected/text.html', 'failures/expected/image.html'],
                      ('// For test-win-xp\n'
                       'failures/expected/image.html [ ImageOnlyFailure ]\n'
                       'failures/expected/text.html [ Failure ]\n'))

    def test_multiple(self):
        self.run_test(['failures/expected/text.html', 'failures/expected/image.html'],
                      ('// For test-win-vista\n'
                       'failures/expected/image.html [ ImageOnlyFailure ]\n'
                       'failures/expected/text.html [ Failure ]\n'
                       '\n'
                       '// For test-win-win7\n'
                       'failures/expected/image.html [ ImageOnlyFailure ]\n'
                       'failures/expected/text.html [ Failure ]\n'
                       '\n'
                       '// For test-win-xp\n'
                       'failures/expected/image.html [ ImageOnlyFailure ]\n'
                       'failures/expected/text.html [ Failure ]\n'),
                       platform='test-win-*')

    def test_full(self):
        self.run_test(['failures/expected/text.html', 'failures/expected/image.html'],
                      ('// For test-win-xp\n'
                       'Bug(test) failures/expected/image.html [ ImageOnlyFailure ]\n'
                       'Bug(test) failures/expected/text.html [ Failure ]\n'),
                      full=True)

    def test_exclude(self):
        self.run_test(['failures/expected/text.html', 'failures/expected/image.html'],
                      ('// For test-win-xp\n'
                       'failures/expected/text.html [ Failure ]\n'),
                      exclude_keyword=['image'])

    def test_include(self):
        self.run_test(['failures/expected/text.html', 'failures/expected/image.html'],
                      ('// For test-win-xp\n'
                       'failures/expected/image.html\n'),
                      include_keyword=['image'])

    def test_csv(self):
        self.run_test(['failures/expected/text.html', 'failures/expected/image.html'],
                      ('test-win-xp,failures/expected/image.html,BUGTEST,IMAGE\n'
                       'test-win-xp,failures/expected/text.html,BUGTEST,FAIL\n'),
                      csv=True)

    def test_paths(self):
        self.run_test([],
                      ('LayoutTests/TestExpectations\n'
                       'LayoutTests/platform/test/TestExpectations\n'
                       'LayoutTests/platform/test-win-xp/TestExpectations\n'),
                      paths=True)

    def test_platform(self):
        self.run_test(['platform/test-mac-leopard/http/test.html'],
                      ('// For test-mac-snowleopard\n'
                       'platform/test-mac-leopard [ Pass Skip WontFix ]\n'  # Note that this is the expectation (from being skipped internally), not the test name
                       '\n'
                       '// For test-mac-leopard\n'
                       'platform/test-mac-leopard/http/test.html [ Pass ]\n'),
                      platform='test-mac-*')

class PrintBaselinesTest(unittest.TestCase):
    def setUp(self):
        self.oc = None
        self.tool = MockTool()
        self.test_port = self.tool.port_factory.get('test-win-xp')
        self.tool.port_factory.get = lambda port_name=None: self.test_port
        self.tool.port_factory.all_port_names = lambda: TestPort.ALL_BASELINE_VARIANTS

    def tearDown(self):
        if self.oc:
            self.restore_output()

    def capture_output(self):
        self.oc = OutputCapture()
        self.oc.capture_output()

    def restore_output(self):
        stdout, stderr, logs = self.oc.restore_output()
        self.oc = None
        return (stdout, stderr, logs)

    def test_basic(self):
        command = PrintBaselines()
        command.bind_to_tool(self.tool)
        self.capture_output()
        command.execute(MockOptions(all=False, include_virtual_tests=False, csv=False, platform=None), ['passes/text.html'], self.tool)
        stdout, _, _ = self.restore_output()
        self.assertMultiLineEqual(stdout,
                          ('// For test-win-xp\n'
                           'passes/text-expected.png\n'
                           'passes/text-expected.txt\n'))

    def test_multiple(self):
        command = PrintBaselines()
        command.bind_to_tool(self.tool)
        self.capture_output()
        command.execute(MockOptions(all=False, include_virtual_tests=False, csv=False, platform='test-win-*'), ['passes/text.html'], self.tool)
        stdout, _, _ = self.restore_output()
        self.assertMultiLineEqual(stdout,
                          ('// For test-win-vista\n'
                           'passes/text-expected.png\n'
                           'passes/text-expected.txt\n'
                           '\n'
                           '// For test-win-win7\n'
                           'passes/text-expected.png\n'
                           'passes/text-expected.txt\n'
                           '\n'
                           '// For test-win-xp\n'
                           'passes/text-expected.png\n'
                           'passes/text-expected.txt\n'))

    def test_csv(self):
        command = PrintBaselines()
        command.bind_to_tool(self.tool)
        self.capture_output()
        command.execute(MockOptions(all=False, platform='*xp', csv=True, include_virtual_tests=False), ['passes/text.html'], self.tool)
        stdout, _, _ = self.restore_output()
        self.assertMultiLineEqual(stdout,
                          ('test-win-xp,passes/text.html,None,png,passes/text-expected.png,None\n'
                           'test-win-xp,passes/text.html,None,txt,passes/text-expected.txt,None\n'))
