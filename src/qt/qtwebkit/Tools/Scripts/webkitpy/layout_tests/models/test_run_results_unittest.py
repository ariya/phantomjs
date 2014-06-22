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

from webkitpy.common.host_mock import MockHost
from webkitpy.layout_tests.models import test_expectations
from webkitpy.layout_tests.models import test_failures
from webkitpy.layout_tests.models import test_results
from webkitpy.layout_tests.models import test_run_results


def get_result(test_name, result_type=test_expectations.PASS, run_time=0):
    failures = []
    if result_type == test_expectations.TIMEOUT:
        failures = [test_failures.FailureTimeout()]
    elif result_type == test_expectations.AUDIO:
        failures = [test_failures.FailureAudioMismatch()]
    elif result_type == test_expectations.CRASH:
        failures = [test_failures.FailureCrash()]
    return test_results.TestResult(test_name, failures=failures, test_run_time=run_time)


def run_results(port):
    tests = ['passes/text.html', 'failures/expected/timeout.html', 'failures/expected/crash.html', 'failures/expected/hang.html',
             'failures/expected/audio.html']
    expectations = test_expectations.TestExpectations(port, tests)
    return test_run_results.TestRunResults(expectations, len(tests))


def summarized_results(port, expected, passing, flaky):
    test_is_slow = False

    initial_results = run_results(port)
    if expected:
        initial_results.add(get_result('passes/text.html', test_expectations.PASS), expected, test_is_slow)
        initial_results.add(get_result('failures/expected/audio.html', test_expectations.AUDIO), expected, test_is_slow)
        initial_results.add(get_result('failures/expected/timeout.html', test_expectations.TIMEOUT), expected, test_is_slow)
        initial_results.add(get_result('failures/expected/crash.html', test_expectations.CRASH), expected, test_is_slow)
    elif passing:
        initial_results.add(get_result('passes/text.html'), expected, test_is_slow)
        initial_results.add(get_result('failures/expected/audio.html'), expected, test_is_slow)
        initial_results.add(get_result('failures/expected/timeout.html'), expected, test_is_slow)
        initial_results.add(get_result('failures/expected/crash.html'), expected, test_is_slow)
    else:
        initial_results.add(get_result('passes/text.html', test_expectations.TIMEOUT), expected, test_is_slow)
        initial_results.add(get_result('failures/expected/audio.html', test_expectations.AUDIO), expected, test_is_slow)
        initial_results.add(get_result('failures/expected/timeout.html', test_expectations.CRASH), expected, test_is_slow)
        initial_results.add(get_result('failures/expected/crash.html', test_expectations.TIMEOUT), expected, test_is_slow)

        # we only list hang.html here, since normally this is WontFix
        initial_results.add(get_result('failures/expected/hang.html', test_expectations.TIMEOUT), expected, test_is_slow)

    if flaky:
        retry_results = run_results(port)
        retry_results.add(get_result('passes/text.html'), True, test_is_slow)
        retry_results.add(get_result('failures/expected/timeout.html'), True, test_is_slow)
        retry_results.add(get_result('failures/expected/crash.html'), True, test_is_slow)
    else:
        retry_results = None

    return test_run_results.summarize_results(port, initial_results.expectations, initial_results, retry_results, enabled_pixel_tests_in_retry=False)


class InterpretTestFailuresTest(unittest.TestCase):
    def setUp(self):
        host = MockHost()
        self.port = host.port_factory.get(port_name='test')

    def test_interpret_test_failures(self):
        test_dict = test_run_results._interpret_test_failures([test_failures.FailureImageHashMismatch(diff_percent=0.42)])
        self.assertEqual(test_dict['image_diff_percent'], 0.42)

        test_dict = test_run_results._interpret_test_failures([test_failures.FailureReftestMismatch(self.port.abspath_for_test('foo/reftest-expected.html'))])
        self.assertIn('image_diff_percent', test_dict)

        test_dict = test_run_results._interpret_test_failures([test_failures.FailureReftestMismatchDidNotOccur(self.port.abspath_for_test('foo/reftest-expected-mismatch.html'))])
        self.assertEqual(len(test_dict), 0)

        test_dict = test_run_results._interpret_test_failures([test_failures.FailureMissingAudio()])
        self.assertIn('is_missing_audio', test_dict)

        test_dict = test_run_results._interpret_test_failures([test_failures.FailureMissingResult()])
        self.assertIn('is_missing_text', test_dict)

        test_dict = test_run_results._interpret_test_failures([test_failures.FailureMissingImage()])
        self.assertIn('is_missing_image', test_dict)

        test_dict = test_run_results._interpret_test_failures([test_failures.FailureMissingImageHash()])
        self.assertIn('is_missing_image', test_dict)


class SummarizedResultsTest(unittest.TestCase):
    def setUp(self):
        host = MockHost(initialize_scm_by_default=False)
        self.port = host.port_factory.get(port_name='test')

    def test_no_svn_revision(self):
        summary = summarized_results(self.port, expected=False, passing=False, flaky=False)
        self.assertNotIn('revision', summary)

    def test_svn_revision(self):
        self.port._options.builder_name = 'dummy builder'
        summary = summarized_results(self.port, expected=False, passing=False, flaky=False)
        self.assertNotEquals(summary['revision'], '')

    def test_summarized_results_wontfix(self):
        self.port._options.builder_name = 'dummy builder'
        summary = summarized_results(self.port, expected=False, passing=False, flaky=False)
        self.assertTrue(summary['tests']['failures']['expected']['hang.html']['wontfix'])
