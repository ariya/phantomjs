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

from webkitpy.common.system.executive import ScriptError
from webkitpy.common.net.layouttestresults import LayoutTestResults


class UnableToApplyPatch(Exception):
    def __init__(self, patch):
        Exception.__init__(self)
        self.patch = patch


class PatchAnalysisTaskDelegate(object):
    def parent_command(self):
        raise NotImplementedError("subclasses must implement")

    def run_command(self, command):
        raise NotImplementedError("subclasses must implement")

    def command_passed(self, message, patch):
        raise NotImplementedError("subclasses must implement")

    def command_failed(self, message, script_error, patch):
        raise NotImplementedError("subclasses must implement")

    def refetch_patch(self, patch):
        raise NotImplementedError("subclasses must implement")

    def expected_failures(self):
        raise NotImplementedError("subclasses must implement")

    def test_results(self):
        raise NotImplementedError("subclasses must implement")

    def archive_last_test_results(self, patch):
        raise NotImplementedError("subclasses must implement")

    def build_style(self):
        raise NotImplementedError("subclasses must implement")

    # We could make results_archive optional, but for now it's required.
    def report_flaky_tests(self, patch, flaky_tests, results_archive):
        raise NotImplementedError("subclasses must implement")


class PatchAnalysisTask(object):
    def __init__(self, delegate, patch):
        self._delegate = delegate
        self._patch = patch
        self._script_error = None
        self._results_archive_from_patch_test_run = None
        self._results_from_patch_test_run = None
        self._expected_failures = delegate.expected_failures()

    def _run_command(self, command, success_message, failure_message):
        try:
            self._delegate.run_command(command)
            self._delegate.command_passed(success_message, patch=self._patch)
            return True
        except ScriptError, e:
            self._script_error = e
            self.failure_status_id = self._delegate.command_failed(failure_message, script_error=self._script_error, patch=self._patch)
            return False

    def _clean(self):
        return self._run_command([
            "clean",
        ],
        "Cleaned working directory",
        "Unable to clean working directory")

    def _update(self):
        # FIXME: Ideally the status server log message should include which revision we updated to.
        return self._run_command([
            "update",
        ],
        "Updated working directory",
        "Unable to update working directory")

    def _apply(self):
        return self._run_command([
            "apply-attachment",
            "--no-update",
            "--non-interactive",
            self._patch.id(),
        ],
        "Applied patch",
        "Patch does not apply")

    def _build(self):
        return self._run_command([
            "build",
            "--no-clean",
            "--no-update",
            "--build-style=%s" % self._delegate.build_style(),
        ],
        "Built patch",
        "Patch does not build")

    def _build_without_patch(self):
        return self._run_command([
            "build",
            "--force-clean",
            "--no-update",
            "--build-style=%s" % self._delegate.build_style(),
        ],
        "Able to build without patch",
        "Unable to build without patch")

    def _test(self):
        return self._run_command([
            "build-and-test",
            "--no-clean",
            "--no-update",
            # Notice that we don't pass --build, which means we won't build!
            "--test",
            "--non-interactive",
        ],
        "Passed tests",
        "Patch does not pass tests")

    def _build_and_test_without_patch(self):
        return self._run_command([
            "build-and-test",
            "--force-clean",
            "--no-update",
            "--build",
            "--test",
            "--non-interactive",
        ],
        "Able to pass tests without patch",
        "Unable to pass tests without patch (tree is red?)")

    def _land(self):
        # Unclear if this should pass --quiet or not.  If --parent-command always does the reporting, then it should.
        return self._run_command([
            "land-attachment",
            "--force-clean",
            "--non-interactive",
            "--parent-command=" + self._delegate.parent_command(),
            self._patch.id(),
        ],
        "Landed patch",
        "Unable to land patch")

    def _report_flaky_tests(self, flaky_test_results, results_archive):
        self._delegate.report_flaky_tests(self._patch, flaky_test_results, results_archive)

    def _results_failed_different_tests(self, first, second):
        first_failing_tests = [] if not first else first.failing_tests()
        second_failing_tests = [] if not second else second.failing_tests()
        return first_failing_tests != second_failing_tests

    def _test_patch(self):
        if self._test():
            return True

        # Note: archive_last_test_results deletes the results directory, making these calls order-sensitve.
        # We could remove this dependency by building the test_results from the archive.
        first_results = self._delegate.test_results()
        first_results_archive = self._delegate.archive_last_test_results(self._patch)
        first_script_error = self._script_error
        first_failure_status_id = self.failure_status_id

        if self._expected_failures.failures_were_expected(first_results):
            return True

        if self._test():
            # Only report flaky tests if we were successful at parsing results.json and archiving results.
            if first_results and first_results_archive:
                self._report_flaky_tests(first_results.failing_test_results(), first_results_archive)
            return True

        second_results = self._delegate.test_results()
        if self._results_failed_different_tests(first_results, second_results):
            # We could report flaky tests here, but we would need to be careful
            # to use similar checks to ExpectedFailures._can_trust_results
            # to make sure we don't report constant failures as flakes when
            # we happen to hit the --exit-after-N-failures limit.
            # See https://bugs.webkit.org/show_bug.cgi?id=51272
            return False

        # Archive (and remove) second results so test_results() after
        # build_and_test_without_patch won't use second results instead of the clean-tree results.
        second_results_archive = self._delegate.archive_last_test_results(self._patch)

        if self._build_and_test_without_patch():
            # The error from the previous ._test() run is real, report it.
            return self.report_failure(first_results_archive, first_results, first_script_error)

        clean_tree_results = self._delegate.test_results()
        self._expected_failures.update(clean_tree_results)

        # Re-check if the original results are now to be expected to avoid a full re-try.
        if self._expected_failures.failures_were_expected(first_results):
            return True

        # Now that we have updated information about failing tests with a clean checkout, we can
        # tell if our original failures were unexpected and fail the patch if necessary.
        if self._expected_failures.unexpected_failures_observed(first_results):
            self.failure_status_id = first_failure_status_id
            return self.report_failure(first_results_archive, first_results, first_script_error)

        # We don't know what's going on.  The tree is likely very red (beyond our layout-test-results
        # failure limit), just keep retrying the patch. until someone fixes the tree.
        return False

    def results_archive_from_patch_test_run(self, patch):
        assert(self._patch.id() == patch.id())  # PatchAnalysisTask is not currently re-useable.
        return self._results_archive_from_patch_test_run

    def results_from_patch_test_run(self, patch):
        assert(self._patch.id() == patch.id())  # PatchAnalysisTask is not currently re-useable.
        return self._results_from_patch_test_run

    def report_failure(self, results_archive=None, results=None, script_error=None):
        if not self.validate():
            return False
        self._results_archive_from_patch_test_run = results_archive
        self._results_from_patch_test_run = results
        raise script_error or self._script_error

    def validate(self):
        raise NotImplementedError("subclasses must implement")

    def run(self):
        raise NotImplementedError("subclasses must implement")
