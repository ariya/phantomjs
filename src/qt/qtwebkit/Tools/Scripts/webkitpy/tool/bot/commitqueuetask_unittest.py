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

from datetime import datetime
import logging
import unittest2 as unittest

from webkitpy.common.net import bugzilla
from webkitpy.common.net.layouttestresults import LayoutTestResults
from webkitpy.common.system.executive import ScriptError
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.layout_tests.models import test_results
from webkitpy.layout_tests.models import test_failures
from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.bot.commitqueuetask import *
from webkitpy.tool.bot.expectedfailures import ExpectedFailures
from webkitpy.tool.mocktool import MockTool

_log = logging.getLogger(__name__)


class MockCommitQueue(CommitQueueTaskDelegate):
    def __init__(self, error_plan):
        self._error_plan = error_plan
        self._failure_status_id = 0

    def run_command(self, command):
        _log.info("run_webkit_patch: %s" % command)
        if self._error_plan:
            error = self._error_plan.pop(0)
            if error:
                raise error

    def command_passed(self, success_message, patch):
        _log.info("command_passed: success_message='%s' patch='%s'" % (
            success_message, patch.id()))

    def command_failed(self, failure_message, script_error, patch):
        _log.info("command_failed: failure_message='%s' script_error='%s' patch='%s'" % (
            failure_message, script_error, patch.id()))
        self._failure_status_id += 1
        return self._failure_status_id

    def refetch_patch(self, patch):
        return patch

    def expected_failures(self):
        return ExpectedFailures()

    def test_results(self):
        return None

    def report_flaky_tests(self, patch, flaky_results, results_archive):
        flaky_tests = [result.filename for result in flaky_results]
        _log.info("report_flaky_tests: patch='%s' flaky_tests='%s' archive='%s'" % (patch.id(), flaky_tests, results_archive.filename))

    def archive_last_test_results(self, patch):
        _log.info("archive_last_test_results: patch='%s'" % patch.id())
        archive = Mock()
        archive.filename = "mock-archive-%s.zip" % patch.id()
        return archive

    def build_style(self):
        return "both"

    def did_pass_testing_ews(self, patch):
        return False


class FailingTestCommitQueue(MockCommitQueue):
    def __init__(self, error_plan, test_failure_plan):
        MockCommitQueue.__init__(self, error_plan)
        self._test_run_counter = -1  # Special value to indicate tests have never been run.
        self._test_failure_plan = test_failure_plan

    def run_command(self, command):
        if command[0] == "build-and-test":
            self._test_run_counter += 1
        MockCommitQueue.run_command(self, command)

    def _mock_test_result(self, testname):
        return test_results.TestResult(testname, [test_failures.FailureTextMismatch()])

    def test_results(self):
        # Doesn't make sense to ask for the test_results until the tests have run at least once.
        assert(self._test_run_counter >= 0)
        failures_for_run = self._test_failure_plan[self._test_run_counter]
        results = LayoutTestResults(map(self._mock_test_result, failures_for_run))
        # This makes the results trustable by ExpectedFailures.
        results.set_failure_limit_count(10)
        return results


# We use GoldenScriptError to make sure that the code under test throws the
# correct (i.e., golden) exception.
class GoldenScriptError(ScriptError):
    pass


class CommitQueueTaskTest(unittest.TestCase):
    def _run_through_task(self, commit_queue, expected_logs, expected_exception=None, expect_retry=False):
        self.maxDiff = None
        tool = MockTool(log_executive=True)
        patch = tool.bugs.fetch_attachment(10000)
        task = CommitQueueTask(commit_queue, patch)
        success = OutputCapture().assert_outputs(self, task.run, expected_logs=expected_logs, expected_exception=expected_exception)
        if not expected_exception:
            self.assertEqual(success, not expect_retry)
        return task

    def test_success_case(self):
        commit_queue = MockCommitQueue([])
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_passed: success_message='Passed tests' patch='10000'
run_webkit_patch: ['land-attachment', '--force-clean', '--non-interactive', '--parent-command=commit-queue', 10000]
command_passed: success_message='Landed patch' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs)

    def test_fast_success_case(self):
        commit_queue = MockCommitQueue([])
        commit_queue.did_pass_testing_ews = lambda patch: True
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['land-attachment', '--force-clean', '--non-interactive', '--parent-command=commit-queue', 10000]
command_passed: success_message='Landed patch' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs)

    def test_clean_failure(self):
        commit_queue = MockCommitQueue([
            ScriptError("MOCK clean failure"),
        ])
        expected_logs = """run_webkit_patch: ['clean']
command_failed: failure_message='Unable to clean working directory' script_error='MOCK clean failure' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs, expect_retry=True)

    def test_update_failure(self):
        commit_queue = MockCommitQueue([
            None,
            ScriptError("MOCK update failure"),
        ])
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_failed: failure_message='Unable to update working directory' script_error='MOCK update failure' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs, expect_retry=True)

    def test_apply_failure(self):
        commit_queue = MockCommitQueue([
            None,
            None,
            GoldenScriptError("MOCK apply failure"),
        ])
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_failed: failure_message='Patch does not apply' script_error='MOCK apply failure' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs, GoldenScriptError)

    def test_validate_changelog_failure(self):
        commit_queue = MockCommitQueue([
            None,
            None,
            None,
            GoldenScriptError("MOCK validate failure"),
        ])
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_failed: failure_message='ChangeLog did not pass validation' script_error='MOCK validate failure' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs, GoldenScriptError)

    def test_build_failure(self):
        commit_queue = MockCommitQueue([
            None,
            None,
            None,
            None,
            GoldenScriptError("MOCK build failure"),
        ])
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_failed: failure_message='Patch does not build' script_error='MOCK build failure' patch='10000'
run_webkit_patch: ['build', '--force-clean', '--no-update', '--build-style=both']
command_passed: success_message='Able to build without patch' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs, GoldenScriptError)

    def test_red_build_failure(self):
        commit_queue = MockCommitQueue([
            None,
            None,
            None,
            None,
            ScriptError("MOCK build failure"),
            ScriptError("MOCK clean build failure"),
        ])
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_failed: failure_message='Patch does not build' script_error='MOCK build failure' patch='10000'
run_webkit_patch: ['build', '--force-clean', '--no-update', '--build-style=both']
command_failed: failure_message='Unable to build without patch' script_error='MOCK clean build failure' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs, expect_retry=True)

    def test_flaky_test_failure(self):
        commit_queue = MockCommitQueue([
            None,
            None,
            None,
            None,
            None,
            ScriptError("MOCK tests failure"),
        ])
        # CommitQueueTask will only report flaky tests if we successfully parsed
        # results.json and returned a LayoutTestResults object, so we fake one.
        commit_queue.test_results = lambda: LayoutTestResults([])
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK tests failure' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_passed: success_message='Passed tests' patch='10000'
report_flaky_tests: patch='10000' flaky_tests='[]' archive='mock-archive-10000.zip'
run_webkit_patch: ['land-attachment', '--force-clean', '--non-interactive', '--parent-command=commit-queue', 10000]
command_passed: success_message='Landed patch' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs)

    def test_failed_archive(self):
        commit_queue = MockCommitQueue([
            None,
            None,
            None,
            None,
            None,
            ScriptError("MOCK tests failure"),
        ])
        commit_queue.test_results = lambda: LayoutTestResults([])
        # It's possible delegate to fail to archive layout tests, don't try to report
        # flaky tests when that happens.
        commit_queue.archive_last_test_results = lambda patch: None
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK tests failure' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_passed: success_message='Passed tests' patch='10000'
run_webkit_patch: ['land-attachment', '--force-clean', '--non-interactive', '--parent-command=commit-queue', 10000]
command_passed: success_message='Landed patch' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs)

    def test_double_flaky_test_failure(self):
        commit_queue = FailingTestCommitQueue([
            None,
            None,
            None,
            None,
            None,
            ScriptError("MOCK test failure"),
            ScriptError("MOCK test failure again"),
        ], [
            "foo.html",
            "bar.html",
            "foo.html",
        ])
        # The (subtle) point of this test is that report_flaky_tests does not appear
        # in the expected_logs for this run.
        # Note also that there is no attempt to run the tests w/o the patch.
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure again' patch='10000'
"""
        tool = MockTool(log_executive=True)
        patch = tool.bugs.fetch_attachment(10000)
        task = CommitQueueTask(commit_queue, patch)
        success = OutputCapture().assert_outputs(self, task.run, expected_logs=expected_logs)
        self.assertFalse(success)

    def test_test_failure(self):
        commit_queue = MockCommitQueue([
            None,
            None,
            None,
            None,
            None,
            GoldenScriptError("MOCK test failure"),
            ScriptError("MOCK test failure again"),
        ])
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure again' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--force-clean', '--no-update', '--build', '--test', '--non-interactive']
command_passed: success_message='Able to pass tests without patch' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs, GoldenScriptError)

    def test_red_test_failure(self):
        commit_queue = FailingTestCommitQueue([
            None,
            None,
            None,
            None,
            None,
            ScriptError("MOCK test failure"),
            ScriptError("MOCK test failure again"),
            ScriptError("MOCK clean test failure"),
        ], [
            "foo.html",
            "foo.html",
            "foo.html",
        ])

        # Tests always fail, and always return the same results, but we
        # should still be able to land in this case!
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure again' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--force-clean', '--no-update', '--build', '--test', '--non-interactive']
command_failed: failure_message='Unable to pass tests without patch (tree is red?)' script_error='MOCK clean test failure' patch='10000'
run_webkit_patch: ['land-attachment', '--force-clean', '--non-interactive', '--parent-command=commit-queue', 10000]
command_passed: success_message='Landed patch' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs)

    def test_very_red_tree_retry(self):
        lots_of_failing_tests = map(lambda num: "test-%s.html" % num, range(0, 100))
        commit_queue = FailingTestCommitQueue([
            None,
            None,
            None,
            None,
            None,
            ScriptError("MOCK test failure"),
            ScriptError("MOCK test failure again"),
            ScriptError("MOCK clean test failure"),
        ], [
            lots_of_failing_tests,
            lots_of_failing_tests,
            lots_of_failing_tests,
        ])

        # Tests always fail, and return so many failures that we do not
        # trust the results (see ExpectedFailures._can_trust_results) so we
        # just give up and retry the patch.
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure again' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--force-clean', '--no-update', '--build', '--test', '--non-interactive']
command_failed: failure_message='Unable to pass tests without patch (tree is red?)' script_error='MOCK clean test failure' patch='10000'
"""
        self._run_through_task(commit_queue, expected_logs, expect_retry=True)

    def test_red_tree_patch_rejection(self):
        commit_queue = FailingTestCommitQueue([
            None,
            None,
            None,
            None,
            None,
            GoldenScriptError("MOCK test failure"),
            ScriptError("MOCK test failure again"),
            ScriptError("MOCK clean test failure"),
        ], [
            ["foo.html", "bar.html"],
            ["foo.html", "bar.html"],
            ["foo.html"],
        ])

        # Tests always fail, but the clean tree only fails one test
        # while the patch fails two.  So we should reject the patch!
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_failed: failure_message='Patch does not pass tests' script_error='MOCK test failure again' patch='10000'
archive_last_test_results: patch='10000'
run_webkit_patch: ['build-and-test', '--force-clean', '--no-update', '--build', '--test', '--non-interactive']
command_failed: failure_message='Unable to pass tests without patch (tree is red?)' script_error='MOCK clean test failure' patch='10000'
"""
        task = self._run_through_task(commit_queue, expected_logs, GoldenScriptError)
        self.assertEqual(task.results_from_patch_test_run(task._patch).failing_tests(), ["foo.html", "bar.html"])
        # failure_status_id should be of the test with patch (1), not the test without patch (2).
        self.assertEqual(task.failure_status_id, 1)

    def test_land_failure(self):
        commit_queue = MockCommitQueue([
            None,
            None,
            None,
            None,
            None,
            None,
            GoldenScriptError("MOCK land failure"),
        ])
        expected_logs = """run_webkit_patch: ['clean']
command_passed: success_message='Cleaned working directory' patch='10000'
run_webkit_patch: ['update']
command_passed: success_message='Updated working directory' patch='10000'
run_webkit_patch: ['apply-attachment', '--no-update', '--non-interactive', 10000]
command_passed: success_message='Applied patch' patch='10000'
run_webkit_patch: ['validate-changelog', '--check-oops', '--non-interactive', 10000]
command_passed: success_message='ChangeLog validated' patch='10000'
run_webkit_patch: ['build', '--no-clean', '--no-update', '--build-style=both']
command_passed: success_message='Built patch' patch='10000'
run_webkit_patch: ['build-and-test', '--no-clean', '--no-update', '--test', '--non-interactive']
command_passed: success_message='Passed tests' patch='10000'
run_webkit_patch: ['land-attachment', '--force-clean', '--non-interactive', '--parent-command=commit-queue', 10000]
command_failed: failure_message='Unable to land patch' script_error='MOCK land failure' patch='10000'
"""
        # FIXME: This should really be expect_retry=True for a better user experiance.
        self._run_through_task(commit_queue, expected_logs, GoldenScriptError)

    def _expect_validate(self, patch, is_valid):
        class MockDelegate(object):
            def refetch_patch(self, patch):
                return patch

            def expected_failures(self):
                return ExpectedFailures()

        task = CommitQueueTask(MockDelegate(), patch)
        self.assertEqual(task.validate(), is_valid)

    def _mock_patch(self, attachment_dict={}, bug_dict={'bug_status': 'NEW'}, committer="fake"):
        bug = bugzilla.Bug(bug_dict, None)
        patch = bugzilla.Attachment(attachment_dict, bug)
        patch._committer = committer
        return patch

    def test_validate(self):
        self._expect_validate(self._mock_patch(), True)
        self._expect_validate(self._mock_patch({'is_obsolete': True}), False)
        self._expect_validate(self._mock_patch(bug_dict={'bug_status': 'CLOSED'}), False)
        self._expect_validate(self._mock_patch(committer=None), False)
        self._expect_validate(self._mock_patch({'review': '-'}), False)
