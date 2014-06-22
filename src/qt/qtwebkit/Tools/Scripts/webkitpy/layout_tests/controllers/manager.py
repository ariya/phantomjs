# Copyright (C) 2010 Google Inc. All rights reserved.
# Copyright (C) 2010 Gabor Rapcsanyi (rgabor@inf.u-szeged.hu), University of Szeged
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

"""
The Manager runs a series of tests (TestType interface) against a set
of test files.  If a test file fails a TestType, it returns a list of TestFailure
objects to the Manager. The Manager then aggregates the TestFailures to
create a final report.
"""

import json
import logging
import random
import sys
import time

from webkitpy.layout_tests.controllers.layout_test_finder import LayoutTestFinder
from webkitpy.layout_tests.controllers.layout_test_runner import LayoutTestRunner
from webkitpy.layout_tests.controllers.test_result_writer import TestResultWriter
from webkitpy.layout_tests.layout_package import json_layout_results_generator
from webkitpy.layout_tests.layout_package import json_results_generator
from webkitpy.layout_tests.models import test_expectations
from webkitpy.layout_tests.models import test_failures
from webkitpy.layout_tests.models import test_run_results
from webkitpy.layout_tests.models.test_input import TestInput

_log = logging.getLogger(__name__)

# Builder base URL where we have the archived test results.
BUILDER_BASE_URL = "http://build.chromium.org/buildbot/layout_test_results/"

TestExpectations = test_expectations.TestExpectations



class Manager(object):
    """A class for managing running a series of tests on a series of layout
    test files."""

    def __init__(self, port, options, printer):
        """Initialize test runner data structures.

        Args:
          port: an object implementing port-specific
          options: a dictionary of command line options
          printer: a Printer object to record updates to.
        """
        self._port = port
        self._filesystem = port.host.filesystem
        self._options = options
        self._printer = printer
        self._expectations = None

        self.HTTP_SUBDIR = 'http' + port.TEST_PATH_SEPARATOR
        self.PERF_SUBDIR = 'perf'
        self.WEBSOCKET_SUBDIR = 'websocket' + port.TEST_PATH_SEPARATOR
        self.LAYOUT_TESTS_DIRECTORY = 'LayoutTests'

        # disable wss server. need to install pyOpenSSL on buildbots.
        # self._websocket_secure_server = websocket_server.PyWebSocket(
        #        options.results_directory, use_tls=True, port=9323)

        self._results_directory = self._port.results_directory()
        self._finder = LayoutTestFinder(self._port, self._options)
        self._runner = LayoutTestRunner(self._options, self._port, self._printer, self._results_directory, self._test_is_slow)

    def _collect_tests(self, args):
        return self._finder.find_tests(self._options, args)

    def _is_http_test(self, test):
        return self.HTTP_SUBDIR in test or self._is_websocket_test(test)

    def _is_websocket_test(self, test):
        return self.WEBSOCKET_SUBDIR in test

    def _http_tests(self, test_names):
        return set(test for test in test_names if self._is_http_test(test))

    def _is_perf_test(self, test):
        return self.PERF_SUBDIR == test or (self.PERF_SUBDIR + self._port.TEST_PATH_SEPARATOR) in test

    def _prepare_lists(self, paths, test_names):
        tests_to_skip = self._finder.skip_tests(paths, test_names, self._expectations, self._http_tests(test_names))
        tests_to_run = [test for test in test_names if test not in tests_to_skip]

        # Create a sorted list of test files so the subset chunk,
        # if used, contains alphabetically consecutive tests.
        if self._options.order == 'natural':
            tests_to_run.sort(key=self._port.test_key)
        elif self._options.order == 'random':
            random.shuffle(tests_to_run)

        tests_to_run, tests_in_other_chunks = self._finder.split_into_chunks(tests_to_run)
        self._expectations.add_skipped_tests(tests_in_other_chunks)
        tests_to_skip.update(tests_in_other_chunks)

        return tests_to_run, tests_to_skip

    def _test_input_for_file(self, test_file):
        return TestInput(test_file,
            self._options.slow_time_out_ms if self._test_is_slow(test_file) else self._options.time_out_ms,
            self._test_requires_lock(test_file))

    def _test_requires_lock(self, test_file):
        """Return True if the test needs to be locked when
        running multiple copies of NRWTs. Perf tests are locked
        because heavy load caused by running other tests in parallel
        might cause some of them to timeout."""
        return self._is_http_test(test_file) or self._is_perf_test(test_file)

    def _test_is_slow(self, test_file):
        return self._expectations.has_modifier(test_file, test_expectations.SLOW)

    def needs_servers(self, test_names):
        return any(self._test_requires_lock(test_name) for test_name in test_names) and self._options.http

    def _set_up_run(self, test_names):
        self._printer.write_update("Checking build ...")
        if not self._port.check_build(self.needs_servers(test_names)):
            _log.error("Build check failed")
            return False

        # This must be started before we check the system dependencies,
        # since the helper may do things to make the setup correct.
        if self._options.pixel_tests:
            self._printer.write_update("Starting pixel test helper ...")
            self._port.start_helper()

        # Check that the system dependencies (themes, fonts, ...) are correct.
        if not self._options.nocheck_sys_deps:
            self._printer.write_update("Checking system dependencies ...")
            if not self._port.check_sys_deps(self.needs_servers(test_names)):
                self._port.stop_helper()
                return False

        if self._options.clobber_old_results:
            self._clobber_old_results()

        # Create the output directory if it doesn't already exist.
        self._port.host.filesystem.maybe_make_directory(self._results_directory)

        self._port.setup_test_run()
        return True

    def run(self, args):
        """Run the tests and return a RunDetails object with the results."""
        self._printer.write_update("Collecting tests ...")
        try:
            paths, test_names = self._collect_tests(args)
        except IOError:
            # This is raised if --test-list doesn't exist
            return test_run_results.RunDetails(exit_code=-1)

        self._printer.write_update("Parsing expectations ...")
        self._expectations = test_expectations.TestExpectations(self._port, test_names)

        tests_to_run, tests_to_skip = self._prepare_lists(paths, test_names)
        self._printer.print_found(len(test_names), len(tests_to_run), self._options.repeat_each, self._options.iterations)

        # Check to make sure we're not skipping every test.
        if not tests_to_run:
            _log.critical('No tests to run.')
            return test_run_results.RunDetails(exit_code=-1)

        if not self._set_up_run(tests_to_run):
            return test_run_results.RunDetails(exit_code=-1)

        start_time = time.time()
        enabled_pixel_tests_in_retry = False
        try:
            initial_results = self._run_tests(tests_to_run, tests_to_skip, self._options.repeat_each, self._options.iterations,
                int(self._options.child_processes), retrying=False)

            tests_to_retry = self._tests_to_retry(initial_results, include_crashes=self._port.should_retry_crashes())
            if self._options.retry_failures and tests_to_retry and not initial_results.interrupted:
                enabled_pixel_tests_in_retry = self._force_pixel_tests_if_needed()

                _log.info('')
                _log.info("Retrying %d unexpected failure(s) ..." % len(tests_to_retry))
                _log.info('')
                retry_results = self._run_tests(tests_to_retry, tests_to_skip=set(), repeat_each=1, iterations=1,
                    num_workers=1, retrying=True)

                if enabled_pixel_tests_in_retry:
                    self._options.pixel_tests = False
            else:
                retry_results = None
        finally:
            self._clean_up_run()

        end_time = time.time()

        # Some crash logs can take a long time to be written out so look
        # for new logs after the test run finishes.
        _log.debug("looking for new crash logs")
        self._look_for_new_crash_logs(initial_results, start_time)
        if retry_results:
            self._look_for_new_crash_logs(retry_results, start_time)

        _log.debug("summarizing results")
        summarized_results = test_run_results.summarize_results(self._port, self._expectations, initial_results, retry_results, enabled_pixel_tests_in_retry)
        self._printer.print_results(end_time - start_time, initial_results, summarized_results)

        if not self._options.dry_run:
            self._port.print_leaks_summary()
            self._upload_json_files(summarized_results, initial_results)

            results_path = self._filesystem.join(self._results_directory, "results.html")
            self._copy_results_html_file(results_path)
            if self._options.show_results and (initial_results.unexpected_results_by_name or
                                               (self._options.full_results_html and initial_results.total_failures)):
                self._port.show_results_html_file(results_path)

        return test_run_results.RunDetails(self._port.exit_code_from_summarized_results(summarized_results),
                                           summarized_results, initial_results, retry_results, enabled_pixel_tests_in_retry)

    def _run_tests(self, tests_to_run, tests_to_skip, repeat_each, iterations, num_workers, retrying):
        needs_http = any(self._is_http_test(test) for test in tests_to_run)
        needs_websockets = any(self._is_websocket_test(test) for test in tests_to_run)

        test_inputs = []
        for _ in xrange(iterations):
            for test in tests_to_run:
                for _ in xrange(repeat_each):
                    test_inputs.append(self._test_input_for_file(test))
        return self._runner.run_tests(self._expectations, test_inputs, tests_to_skip, num_workers, needs_http, needs_websockets, retrying)

    def _clean_up_run(self):
        _log.debug("Flushing stdout")
        sys.stdout.flush()
        _log.debug("Flushing stderr")
        sys.stderr.flush()
        _log.debug("Stopping helper")
        self._port.stop_helper()
        _log.debug("Cleaning up port")
        self._port.clean_up_test_run()

    def _force_pixel_tests_if_needed(self):
        if self._options.pixel_tests:
            return False

        _log.debug("Restarting helper")
        self._port.stop_helper()
        self._options.pixel_tests = True
        self._port.start_helper()

        return True

    def _look_for_new_crash_logs(self, run_results, start_time):
        """Since crash logs can take a long time to be written out if the system is
           under stress do a second pass at the end of the test run.

           run_results: the results of the test run
           start_time: time the tests started at.  We're looking for crash
               logs after that time.
        """
        crashed_processes = []
        for test, result in run_results.unexpected_results_by_name.iteritems():
            if (result.type != test_expectations.CRASH):
                continue
            for failure in result.failures:
                if not isinstance(failure, test_failures.FailureCrash):
                    continue
                crashed_processes.append([test, failure.process_name, failure.pid])

        sample_files = self._port.look_for_new_samples(crashed_processes, start_time)
        if sample_files:
            for test, sample_file in sample_files.iteritems():
                writer = TestResultWriter(self._port._filesystem, self._port, self._port.results_directory(), test)
                writer.copy_sample_file(sample_file)

        crash_logs = self._port.look_for_new_crash_logs(crashed_processes, start_time)
        if crash_logs:
            for test, crash_log in crash_logs.iteritems():
                writer = TestResultWriter(self._port._filesystem, self._port, self._port.results_directory(), test)
                writer.write_crash_log(crash_log)

    def _clobber_old_results(self):
        # Just clobber the actual test results directories since the other
        # files in the results directory are explicitly used for cross-run
        # tracking.
        self._printer.write_update("Clobbering old results in %s" %
                                   self._results_directory)
        layout_tests_dir = self._port.layout_tests_dir()
        possible_dirs = self._port.test_dirs()
        for dirname in possible_dirs:
            if self._filesystem.isdir(self._filesystem.join(layout_tests_dir, dirname)):
                self._filesystem.rmtree(self._filesystem.join(self._results_directory, dirname))

    def _tests_to_retry(self, run_results, include_crashes):
        return [result.test_name for result in run_results.unexpected_results_by_name.values() if
                   ((result.type != test_expectations.PASS) and
                    (result.type != test_expectations.MISSING) and
                    (result.type != test_expectations.CRASH or include_crashes))]

    def _upload_json_files(self, summarized_results, initial_results):
        """Writes the results of the test run as JSON files into the results
        dir and upload the files to the appengine server.

        Args:
          summarized_results: dict of results
          initial_results: full summary object
        """
        _log.debug("Writing JSON files in %s." % self._results_directory)

        # FIXME: Upload stats.json to the server and delete times_ms.
        times_trie = json_results_generator.test_timings_trie(self._port, initial_results.results_by_name.values())
        times_json_path = self._filesystem.join(self._results_directory, "times_ms.json")
        json_results_generator.write_json(self._filesystem, times_trie, times_json_path)

        stats_trie = self._stats_trie(initial_results)
        stats_path = self._filesystem.join(self._results_directory, "stats.json")
        self._filesystem.write_text_file(stats_path, json.dumps(stats_trie))

        full_results_path = self._filesystem.join(self._results_directory, "full_results.json")
        # We write full_results.json out as jsonp because we need to load it from a file url and Chromium doesn't allow that.
        json_results_generator.write_json(self._filesystem, summarized_results, full_results_path, callback="ADD_RESULTS")

        generator = json_layout_results_generator.JSONLayoutResultsGenerator(
            self._port, self._options.builder_name, self._options.build_name,
            self._options.build_number, self._results_directory,
            BUILDER_BASE_URL,
            self._expectations, initial_results,
            self._options.test_results_server,
            "layout-tests",
            self._options.master_name)

        _log.debug("Finished writing JSON files.")


        json_files = ["incremental_results.json", "full_results.json", "times_ms.json"]

        generator.upload_json_files(json_files)

        incremental_results_path = self._filesystem.join(self._results_directory, "incremental_results.json")

        # Remove these files from the results directory so they don't take up too much space on the buildbot.
        # The tools use the version we uploaded to the results server anyway.
        self._filesystem.remove(times_json_path)
        self._filesystem.remove(incremental_results_path)

    def _copy_results_html_file(self, destination_path):
        base_dir = self._port.path_from_webkit_base('LayoutTests', 'fast', 'harness')
        results_file = self._filesystem.join(base_dir, 'results.html')
        # Note that the results.html template file won't exist when we're using a MockFileSystem during unit tests,
        # so make sure it exists before we try to copy it.
        if self._filesystem.exists(results_file):
            self._filesystem.copyfile(results_file, destination_path)

    def _stats_trie(self, initial_results):
        def _worker_number(worker_name):
            return int(worker_name.split('/')[1]) if worker_name else -1

        stats = {}
        for result in initial_results.results_by_name.values():
            if result.type != test_expectations.SKIP:
                stats[result.test_name] = {'results': (_worker_number(result.worker_name), result.test_number, result.pid, int(result.test_run_time * 1000), int(result.total_run_time * 1000))}
        stats_trie = {}
        for name, value in stats.iteritems():
            json_results_generator.add_path_to_trie(name, value, stats_trie)
        return stats_trie
