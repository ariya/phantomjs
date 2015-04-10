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

import os
import StringIO

from webkitpy.common.checkout.scm import CheckoutNeedsUpdate
from webkitpy.common.checkout.scm.scm_mock import MockSCM
from webkitpy.common.net.bugzilla import Attachment
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.layout_tests.models import test_results
from webkitpy.layout_tests.models import test_failures
from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.commands.commandtest import CommandsTest
from webkitpy.tool.commands.queues import *
from webkitpy.tool.commands.queuestest import QueuesTest
from webkitpy.tool.commands.stepsequence import StepSequence
from webkitpy.common.net.statusserver_mock import MockStatusServer
from webkitpy.tool.mocktool import MockTool, MockOptions


class TestCommitQueue(CommitQueue):
    def __init__(self, tool=None):
        CommitQueue.__init__(self)
        if tool:
            self.bind_to_tool(tool)
        self._options = MockOptions(confirm=False, parent_command="commit-queue", port=None)

    def begin_work_queue(self):
        output_capture = OutputCapture()
        output_capture.capture_output()
        CommitQueue.begin_work_queue(self)
        output_capture.restore_output()


class TestQueue(AbstractPatchQueue):
    name = "test-queue"


class TestReviewQueue(AbstractReviewQueue):
    name = "test-review-queue"


class TestFeederQueue(FeederQueue):
    _sleep_duration = 0


class AbstractQueueTest(CommandsTest):
    def test_log_directory(self):
        self.assertEqual(TestQueue()._log_directory(), os.path.join("..", "test-queue-logs"))

    def _assert_run_webkit_patch(self, run_args, port=None):
        queue = TestQueue()
        tool = MockTool()
        tool.status_server.bot_id = "gort"
        tool.executive = Mock()
        queue.bind_to_tool(tool)
        queue._options = Mock()
        queue._options.port = port

        queue.run_webkit_patch(run_args)
        expected_run_args = ["echo", "--status-host=example.com", "--bot-id=gort"]
        if port:
            expected_run_args.append("--port=%s" % port)
        expected_run_args.extend(run_args)
        tool.executive.run_command.assert_called_with(expected_run_args, cwd='/mock-checkout')

    def test_run_webkit_patch(self):
        self._assert_run_webkit_patch([1])
        self._assert_run_webkit_patch(["one", 2])
        self._assert_run_webkit_patch([1], port="mockport")

    def test_iteration_count(self):
        queue = TestQueue()
        queue._options = Mock()
        queue._options.iterations = 3
        self.assertTrue(queue.should_continue_work_queue())
        self.assertTrue(queue.should_continue_work_queue())
        self.assertTrue(queue.should_continue_work_queue())
        self.assertFalse(queue.should_continue_work_queue())

    def test_no_iteration_count(self):
        queue = TestQueue()
        queue._options = Mock()
        self.assertTrue(queue.should_continue_work_queue())
        self.assertTrue(queue.should_continue_work_queue())
        self.assertTrue(queue.should_continue_work_queue())
        self.assertTrue(queue.should_continue_work_queue())

    def _assert_log_message(self, script_error, log_message):
        failure_log = AbstractQueue._log_from_script_error_for_upload(script_error, output_limit=10)
        self.assertTrue(failure_log.read(), log_message)

    def test_log_from_script_error_for_upload(self):
        self._assert_log_message(ScriptError("test"), "test")
        unicode_tor = u"WebKit \u2661 Tor Arne Vestb\u00F8!"
        utf8_tor = unicode_tor.encode("utf-8")
        self._assert_log_message(ScriptError(unicode_tor), utf8_tor)
        script_error = ScriptError(unicode_tor, output=unicode_tor)
        expected_output = "%s\nLast %s characters of output:\n%s" % (utf8_tor, 10, utf8_tor[-10:])
        self._assert_log_message(script_error, expected_output)


class FeederQueueTest(QueuesTest):
    def test_feeder_queue(self):
        queue = TestFeederQueue()
        tool = MockTool(log_executive=True)
        expected_logs = {
            "begin_work_queue": self._default_begin_work_queue_logs("feeder-queue"),
            "process_work_item": """Warning, attachment 10001 on bug 50000 has invalid committer (non-committer@example.com)
Warning, attachment 10001 on bug 50000 has invalid committer (non-committer@example.com)
MOCK setting flag 'commit-queue' to '-' on attachment '10001' with comment 'Rejecting attachment 10001 from commit-queue.\n\nnon-committer@example.com does not have committer permissions according to http://trac.webkit.org/browser/trunk/Tools/Scripts/webkitpy/common/config/committers.py.

- If you do not have committer rights please read http://webkit.org/coding/contributing.html for instructions on how to use bugzilla flags.

- If you have committer rights please correct the error in Tools/Scripts/webkitpy/common/config/committers.py by adding yourself to the file (no review needed).  The commit-queue restarts itself every 2 hours.  After restart the commit-queue will correctly respect your committer rights.'
MOCK: update_work_items: commit-queue [10005, 10000]
Feeding commit-queue items [10005, 10000]
Feeding EWS (1 r? patch, 1 new)
MOCK: submit_to_ews: 10002
""",
            "handle_unexpected_error": "Mock error message\n",
        }
        self.assert_queue_outputs(queue, tool=tool, expected_logs=expected_logs)


class AbstractPatchQueueTest(CommandsTest):
    def test_next_patch(self):
        queue = AbstractPatchQueue()
        tool = MockTool()
        queue.bind_to_tool(tool)
        queue._options = Mock()
        queue._options.port = None
        self.assertIsNone(queue._next_patch())
        tool.status_server = MockStatusServer(work_items=[2, 10000, 10001])
        expected_stdout = "MOCK: fetch_attachment: 2 is not a known attachment id\n"  # A mock-only message to prevent us from making mistakes.
        expected_logs = "MOCK: release_work_item: None 2\n"
        patch = OutputCapture().assert_outputs(self, queue._next_patch, expected_stdout=expected_stdout, expected_logs=expected_logs)
        # The patch.id() == 2 is ignored because it doesn't exist.
        self.assertEqual(patch.id(), 10000)
        self.assertEqual(queue._next_patch().id(), 10001)
        self.assertEqual(queue._next_patch(), None)    # When the queue is empty


class PatchProcessingQueueTest(CommandsTest):
    def test_upload_results_archive_for_patch(self):
        queue = PatchProcessingQueue()
        queue.name = "mock-queue"
        tool = MockTool()
        queue.bind_to_tool(tool)
        queue._options = Mock()
        queue._options.port = None
        patch = queue._tool.bugs.fetch_attachment(10001)
        expected_logs = """MOCK add_attachment_to_bug: bug_id=50000, description=Archive of layout-test-results from bot for mac-snowleopard filename=layout-test-results.zip mimetype=None
-- Begin comment --
The attached test failures were seen while running run-webkit-tests on the mock-queue.
Port: mac-snowleopard  Platform: MockPlatform 1.0
-- End comment --
"""
        OutputCapture().assert_outputs(self, queue._upload_results_archive_for_patch, [patch, Mock()], expected_logs=expected_logs)


class NeedsUpdateSequence(StepSequence):
    def _run(self, tool, options, state):
        raise CheckoutNeedsUpdate([], 1, "", None)


class AlwaysCommitQueueTool(object):
    def __init__(self):
        self.status_server = MockStatusServer()

    def command_by_name(self, name):
        return CommitQueue


class SecondThoughtsCommitQueue(TestCommitQueue):
    def __init__(self, tool=None):
        self._reject_patch = False
        TestCommitQueue.__init__(self, tool)

    def run_command(self, command):
        # We want to reject the patch after the first validation,
        # so wait to reject it until after some other command has run.
        self._reject_patch = True
        return CommitQueue.run_command(self, command)

    def refetch_patch(self, patch):
        if not self._reject_patch:
            return self._tool.bugs.fetch_attachment(patch.id())

        attachment_dictionary = {
            "id": patch.id(),
            "bug_id": patch.bug_id(),
            "name": "Rejected",
            "is_obsolete": True,
            "is_patch": False,
            "review": "-",
            "reviewer_email": "foo@bar.com",
            "commit-queue": "-",
            "committer_email": "foo@bar.com",
            "attacher_email": "Contributer1",
        }
        return Attachment(attachment_dictionary, None)


class CommitQueueTest(QueuesTest):
    def _mock_test_result(self, testname):
        return test_results.TestResult(testname, [test_failures.FailureTextMismatch()])

    def test_commit_queue(self):
        tool = MockTool()
        tool.filesystem.write_text_file('/tmp/layout-test-results/full_results.json', '')  # Otherwise the commit-queue will hit a KeyError trying to read the results from the MockFileSystem.
        tool.filesystem.write_text_file('/tmp/layout-test-results/webkit_unit_tests_output.xml', '')
        expected_logs = {
            "begin_work_queue": self._default_begin_work_queue_logs("commit-queue"),
            "process_work_item": """Running: webkit-patch --status-host=example.com clean --port=mac
MOCK: update_status: commit-queue Cleaned working directory
Running: webkit-patch --status-host=example.com update --port=mac
MOCK: update_status: commit-queue Updated working directory
Running: webkit-patch --status-host=example.com apply-attachment --no-update --non-interactive 10000 --port=mac
MOCK: update_status: commit-queue Applied patch
Running: webkit-patch --status-host=example.com validate-changelog --check-oops --non-interactive 10000 --port=mac
MOCK: update_status: commit-queue ChangeLog validated
Running: webkit-patch --status-host=example.com build --no-clean --no-update --build-style=release --port=mac
MOCK: update_status: commit-queue Built patch
Running: webkit-patch --status-host=example.com build-and-test --no-clean --no-update --test --non-interactive --port=mac
MOCK: update_status: commit-queue Passed tests
Running: webkit-patch --status-host=example.com land-attachment --force-clean --non-interactive --parent-command=commit-queue 10000 --port=mac
MOCK: update_status: commit-queue Landed patch
MOCK: update_status: commit-queue Pass
MOCK: release_work_item: commit-queue 10000
""",
            "handle_script_error": "ScriptError error message\n\nMOCK output\n",
            "handle_unexpected_error": "MOCK setting flag 'commit-queue' to '-' on attachment '10000' with comment 'Rejecting attachment 10000 from commit-queue.\n\nMock error message'\n",
        }
        self.assert_queue_outputs(CommitQueue(), tool=tool, expected_logs=expected_logs)

    def test_commit_queue_failure(self):
        expected_logs = {
            "begin_work_queue": self._default_begin_work_queue_logs("commit-queue"),
            "process_work_item": """MOCK: update_status: commit-queue Cleaned working directory
MOCK: update_status: commit-queue Updated working directory
MOCK: update_status: commit-queue Patch does not apply
MOCK setting flag 'commit-queue' to '-' on attachment '10000' with comment 'Rejecting attachment 10000 from commit-queue.\n\nMOCK script error
Full output: http://dummy_url'
MOCK: update_status: commit-queue Fail
MOCK: release_work_item: commit-queue 10000
""",
            "handle_script_error": "ScriptError error message\n\nMOCK output\n",
            "handle_unexpected_error": "MOCK setting flag 'commit-queue' to '-' on attachment '10000' with comment 'Rejecting attachment 10000 from commit-queue.\n\nMock error message'\n",
        }
        queue = CommitQueue()

        def mock_run_webkit_patch(command):
            if command[0] == 'clean' or command[0] == 'update':
                # We want cleaning to succeed so we can error out on a step
                # that causes the commit-queue to reject the patch.
                return
            raise ScriptError('MOCK script error')

        queue.run_webkit_patch = mock_run_webkit_patch
        self.assert_queue_outputs(queue, expected_logs=expected_logs)

    def test_commit_queue_failure_with_failing_tests(self):
        expected_logs = {
            "begin_work_queue": self._default_begin_work_queue_logs("commit-queue"),
            "process_work_item": """MOCK: update_status: commit-queue Cleaned working directory
MOCK: update_status: commit-queue Updated working directory
MOCK: update_status: commit-queue Patch does not apply
MOCK setting flag 'commit-queue' to '-' on attachment '10000' with comment 'Rejecting attachment 10000 from commit-queue.\n\nNew failing tests:
mock_test_name.html
another_test_name.html
Full output: http://dummy_url'
MOCK: update_status: commit-queue Fail
MOCK: release_work_item: commit-queue 10000
""",
            "handle_script_error": "ScriptError error message\n\nMOCK output\n",
            "handle_unexpected_error": "MOCK setting flag 'commit-queue' to '-' on attachment '10000' with comment 'Rejecting attachment 10000 from commit-queue.\n\nMock error message'\n",
        }
        queue = CommitQueue()

        def mock_run_webkit_patch(command):
            if command[0] == 'clean' or command[0] == 'update':
                # We want cleaning to succeed so we can error out on a step
                # that causes the commit-queue to reject the patch.
                return
            queue._expected_failures.unexpected_failures_observed = lambda results: ["mock_test_name.html", "another_test_name.html"]
            raise ScriptError('MOCK script error')

        queue.run_webkit_patch = mock_run_webkit_patch
        self.assert_queue_outputs(queue, expected_logs=expected_logs)

    def test_rollout(self):
        tool = MockTool()
        tool.filesystem.write_text_file('/tmp/layout-test-results/full_results.json', '')  # Otherwise the commit-queue will hit a KeyError trying to read the results from the MockFileSystem.
        tool.filesystem.write_text_file('/tmp/layout-test-results/webkit_unit_tests_output.xml', '')
        tool.buildbot.light_tree_on_fire()
        expected_logs = {
            "begin_work_queue": self._default_begin_work_queue_logs("commit-queue"),
            "process_work_item": """Running: webkit-patch --status-host=example.com clean --port=%(port)s
MOCK: update_status: commit-queue Cleaned working directory
Running: webkit-patch --status-host=example.com update --port=%(port)s
MOCK: update_status: commit-queue Updated working directory
Running: webkit-patch --status-host=example.com apply-attachment --no-update --non-interactive 10000 --port=%(port)s
MOCK: update_status: commit-queue Applied patch
Running: webkit-patch --status-host=example.com validate-changelog --check-oops --non-interactive 10000 --port=%(port)s
MOCK: update_status: commit-queue ChangeLog validated
Running: webkit-patch --status-host=example.com build --no-clean --no-update --build-style=release --port=%(port)s
MOCK: update_status: commit-queue Built patch
Running: webkit-patch --status-host=example.com build-and-test --no-clean --no-update --test --non-interactive --port=%(port)s
MOCK: update_status: commit-queue Passed tests
Running: webkit-patch --status-host=example.com land-attachment --force-clean --non-interactive --parent-command=commit-queue 10000 --port=%(port)s
MOCK: update_status: commit-queue Landed patch
MOCK: update_status: commit-queue Pass
MOCK: release_work_item: commit-queue 10000
""" % {"port": "mac"},
            "handle_script_error": "ScriptError error message\n\nMOCK output\n",
            "handle_unexpected_error": "MOCK setting flag 'commit-queue' to '-' on attachment '10000' with comment 'Rejecting attachment 10000 from commit-queue.\n\nMock error message'\n",
        }
        self.assert_queue_outputs(CommitQueue(), tool=tool, expected_logs=expected_logs)

    def test_rollout_lands(self):
        tool = MockTool()
        tool.buildbot.light_tree_on_fire()
        rollout_patch = tool.bugs.fetch_attachment(10005)  # _patch6, a rollout patch.
        assert(rollout_patch.is_rollout())
        expected_logs = {
            "begin_work_queue": self._default_begin_work_queue_logs("commit-queue"),
            "process_work_item": """Running: webkit-patch --status-host=example.com clean --port=%(port)s
MOCK: update_status: commit-queue Cleaned working directory
Running: webkit-patch --status-host=example.com update --port=%(port)s
MOCK: update_status: commit-queue Updated working directory
Running: webkit-patch --status-host=example.com apply-attachment --no-update --non-interactive 10005 --port=%(port)s
MOCK: update_status: commit-queue Applied patch
Running: webkit-patch --status-host=example.com validate-changelog --check-oops --non-interactive 10005 --port=%(port)s
MOCK: update_status: commit-queue ChangeLog validated
Running: webkit-patch --status-host=example.com land-attachment --force-clean --non-interactive --parent-command=commit-queue 10005 --port=%(port)s
MOCK: update_status: commit-queue Landed patch
MOCK: update_status: commit-queue Pass
MOCK: release_work_item: commit-queue 10005
""" % {"port": "mac"},
            "handle_script_error": "ScriptError error message\n\nMOCK output\n",
            "handle_unexpected_error": "MOCK setting flag 'commit-queue' to '-' on attachment '10005' with comment 'Rejecting attachment 10005 from commit-queue.\n\nMock error message'\n",
        }
        self.assert_queue_outputs(CommitQueue(), tool=tool, work_item=rollout_patch, expected_logs=expected_logs)

    def test_auto_retry(self):
        queue = CommitQueue()
        options = Mock()
        options.parent_command = "commit-queue"
        tool = AlwaysCommitQueueTool()
        sequence = NeedsUpdateSequence(None)

        expected_logs = """Commit failed because the checkout is out of date. Please update and try again.
MOCK: update_status: commit-queue Tests passed, but commit failed (checkout out of date).  Updating, then landing without building or re-running tests.
"""
        state = {'patch': None}
        OutputCapture().assert_outputs(self, sequence.run_and_handle_errors, [tool, options, state], expected_exception=TryAgain, expected_logs=expected_logs)

        self.assertTrue(options.update)
        self.assertFalse(options.build)
        self.assertFalse(options.test)

    def test_manual_reject_during_processing(self):
        queue = SecondThoughtsCommitQueue(MockTool())
        queue.begin_work_queue()
        queue._tool.filesystem.write_text_file('/tmp/layout-test-results/full_results.json', '')  # Otherwise the commit-queue will hit a KeyError trying to read the results from the MockFileSystem.
        queue._tool.filesystem.write_text_file('/tmp/layout-test-results/webkit_unit_tests_output.xml', '')
        queue._options = Mock()
        queue._options.port = None
        expected_logs = """Running: webkit-patch --status-host=example.com clean --port=mac
MOCK: update_status: commit-queue Cleaned working directory
Running: webkit-patch --status-host=example.com update --port=mac
MOCK: update_status: commit-queue Updated working directory
Running: webkit-patch --status-host=example.com apply-attachment --no-update --non-interactive 10000 --port=mac
MOCK: update_status: commit-queue Applied patch
Running: webkit-patch --status-host=example.com validate-changelog --check-oops --non-interactive 10000 --port=mac
MOCK: update_status: commit-queue ChangeLog validated
Running: webkit-patch --status-host=example.com build --no-clean --no-update --build-style=release --port=mac
MOCK: update_status: commit-queue Built patch
Running: webkit-patch --status-host=example.com build-and-test --no-clean --no-update --test --non-interactive --port=mac
MOCK: update_status: commit-queue Passed tests
MOCK: update_status: commit-queue Retry
MOCK: release_work_item: commit-queue 10000
"""
        self.maxDiff = None
        OutputCapture().assert_outputs(self, queue.process_work_item, [QueuesTest.mock_work_item], expected_logs=expected_logs)

    def test_report_flaky_tests(self):
        queue = TestCommitQueue(MockTool())
        expected_logs = """MOCK bug comment: bug_id=50002, cc=None
--- Begin comment ---
The commit-queue just saw foo/bar.html flake (text diff) while processing attachment 10000 on bug 50000.
Port: MockPort  Platform: MockPlatform 1.0
--- End comment ---

MOCK add_attachment_to_bug: bug_id=50002, description=Failure diff from bot filename=failure.diff mimetype=None
MOCK bug comment: bug_id=50002, cc=None
--- Begin comment ---
The commit-queue just saw bar/baz.html flake (text diff) while processing attachment 10000 on bug 50000.
Port: MockPort  Platform: MockPlatform 1.0
--- End comment ---

bar/baz-diffs.txt does not exist in results archive, uploading entire archive.
MOCK add_attachment_to_bug: bug_id=50002, description=Archive of layout-test-results from bot filename=layout-test-results.zip mimetype=None
MOCK bug comment: bug_id=50000, cc=None
--- Begin comment ---
The commit-queue encountered the following flaky tests while processing attachment 10000:

foo/bar.html bug 50002 (author: abarth@webkit.org)
bar/baz.html bug 50002 (author: abarth@webkit.org)
The commit-queue is continuing to process your patch.
--- End comment ---

"""
        test_names = ["foo/bar.html", "bar/baz.html"]
        test_results = [self._mock_test_result(name) for name in test_names]

        class MockZipFile(object):
            def __init__(self):
                self.fp = StringIO()

            def read(self, path):
                return ""

            def namelist(self):
                # This is intentionally missing one diffs.txt to exercise the "upload the whole zip" codepath.
                return ['foo/bar-diffs.txt']

        OutputCapture().assert_outputs(self, queue.report_flaky_tests, [QueuesTest.mock_work_item, test_results, MockZipFile()], expected_logs=expected_logs)

    def test_did_pass_testing_ews(self):
        tool = MockTool()
        patch = tool.bugs.fetch_attachment(10000)
        queue = TestCommitQueue(tool)
        self.assertFalse(queue.did_pass_testing_ews(patch))


class StyleQueueTest(QueuesTest):
    def test_style_queue_with_style_exception(self):
        expected_logs = {
            "begin_work_queue": self._default_begin_work_queue_logs("style-queue"),
            "process_work_item": """Running: webkit-patch --status-host=example.com clean
MOCK: update_status: style-queue Cleaned working directory
Running: webkit-patch --status-host=example.com update
MOCK: update_status: style-queue Updated working directory
Running: webkit-patch --status-host=example.com apply-attachment --no-update --non-interactive 10000
MOCK: update_status: style-queue Applied patch
Running: webkit-patch --status-host=example.com apply-watchlist-local 50000
MOCK: update_status: style-queue Watchlist applied
Running: webkit-patch --status-host=example.com check-style-local --non-interactive --quiet
MOCK: update_status: style-queue Style checked
MOCK: update_status: style-queue Pass
MOCK: release_work_item: style-queue 10000
""",
            "handle_unexpected_error": "Mock error message\n",
            "handle_script_error": "MOCK output\n",
        }
        tool = MockTool(executive_throws_when_run=set(['check-style']))
        self.assert_queue_outputs(StyleQueue(), expected_logs=expected_logs, tool=tool)

    def test_style_queue_with_watch_list_exception(self):
        expected_logs = {
            "begin_work_queue": self._default_begin_work_queue_logs("style-queue"),
            "process_work_item": """Running: webkit-patch --status-host=example.com clean
MOCK: update_status: style-queue Cleaned working directory
Running: webkit-patch --status-host=example.com update
MOCK: update_status: style-queue Updated working directory
Running: webkit-patch --status-host=example.com apply-attachment --no-update --non-interactive 10000
MOCK: update_status: style-queue Applied patch
Running: webkit-patch --status-host=example.com apply-watchlist-local 50000
Exception for ['echo', '--status-host=example.com', 'apply-watchlist-local', 50000]

MOCK command output
MOCK: update_status: style-queue Unabled to apply watchlist
Running: webkit-patch --status-host=example.com check-style-local --non-interactive --quiet
MOCK: update_status: style-queue Style checked
MOCK: update_status: style-queue Pass
MOCK: release_work_item: style-queue 10000
""",
            "handle_unexpected_error": "Mock error message\n",
            "handle_script_error": "MOCK output\n",
        }
        tool = MockTool(executive_throws_when_run=set(['apply-watchlist-local']))
        self.assert_queue_outputs(StyleQueue(), expected_logs=expected_logs, tool=tool)
