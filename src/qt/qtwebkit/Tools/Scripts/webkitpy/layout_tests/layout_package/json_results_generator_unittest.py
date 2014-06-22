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
import json
import optparse
import random

from webkitpy.common.host_mock import MockHost
from webkitpy.layout_tests.layout_package import json_results_generator
from webkitpy.layout_tests.models import test_expectations
from webkitpy.port import test
from webkitpy.thirdparty.mock import Mock


class JSONGeneratorTest(unittest.TestCase):
    def setUp(self):
        self.builder_name = 'DUMMY_BUILDER_NAME'
        self.build_name = 'DUMMY_BUILD_NAME'
        self.build_number = 'DUMMY_BUILDER_NUMBER'

        # For archived results.
        self._json = None
        self._num_runs = 0
        self._tests_set = set([])
        self._test_timings = {}
        self._failed_count_map = {}

        self._PASS_count = 0
        self._DISABLED_count = 0
        self._FLAKY_count = 0
        self._FAILS_count = 0
        self._fixable_count = 0

    def test_strip_json_wrapper(self):
        json = "['contents']"
        self.assertEqual(json_results_generator.strip_json_wrapper(json_results_generator._JSON_PREFIX + json + json_results_generator._JSON_SUFFIX), json)
        self.assertEqual(json_results_generator.strip_json_wrapper(json), json)

    def _test_json_generation(self, passed_tests_list, failed_tests_list):
        tests_set = set(passed_tests_list) | set(failed_tests_list)

        DISABLED_tests = set([t for t in tests_set
                             if t.startswith('DISABLED_')])
        FLAKY_tests = set([t for t in tests_set
                           if t.startswith('FLAKY_')])
        FAILS_tests = set([t for t in tests_set
                           if t.startswith('FAILS_')])
        PASS_tests = tests_set - (DISABLED_tests | FLAKY_tests | FAILS_tests)

        failed_tests = set(failed_tests_list) - DISABLED_tests
        failed_count_map = dict([(t, 1) for t in failed_tests])

        test_timings = {}
        i = 0
        for test in tests_set:
            test_timings[test] = float(self._num_runs * 100 + i)
            i += 1

        test_results_map = dict()
        for test in tests_set:
            test_results_map[test] = json_results_generator.TestResult(test,
                failed=(test in failed_tests),
                elapsed_time=test_timings[test])

        host = MockHost()
        port = Mock()
        port._filesystem = host.filesystem
        generator = json_results_generator.JSONResultsGenerator(port,
            self.builder_name, self.build_name, self.build_number,
            '',
            None,   # don't fetch past json results archive
            test_results_map)

        failed_count_map = dict([(t, 1) for t in failed_tests])

        # Test incremental json results
        incremental_json = generator.get_json()
        self._verify_json_results(
            tests_set,
            test_timings,
            failed_count_map,
            len(PASS_tests),
            len(DISABLED_tests),
            len(FLAKY_tests),
            len(DISABLED_tests | failed_tests),
            incremental_json,
            1)

        # We don't verify the results here, but at least we make sure the code runs without errors.
        generator.generate_json_output()
        generator.generate_times_ms_file()

    def _verify_json_results(self, tests_set, test_timings, failed_count_map,
                             PASS_count, DISABLED_count, FLAKY_count,
                             fixable_count,
                             json, num_runs):
        # Aliasing to a short name for better access to its constants.
        JRG = json_results_generator.JSONResultsGenerator

        self.assertIn(JRG.VERSION_KEY, json)
        self.assertIn(self.builder_name, json)

        buildinfo = json[self.builder_name]
        self.assertIn(JRG.FIXABLE, buildinfo)
        self.assertIn(JRG.TESTS, buildinfo)
        self.assertEqual(len(buildinfo[JRG.BUILD_NUMBERS]), num_runs)
        self.assertEqual(buildinfo[JRG.BUILD_NUMBERS][0], self.build_number)

        if tests_set or DISABLED_count:
            fixable = {}
            for fixable_items in buildinfo[JRG.FIXABLE]:
                for (type, count) in fixable_items.iteritems():
                    if type in fixable:
                        fixable[type] = fixable[type] + count
                    else:
                        fixable[type] = count

            if PASS_count:
                self.assertEqual(fixable[JRG.PASS_RESULT], PASS_count)
            else:
                self.assertTrue(JRG.PASS_RESULT not in fixable or
                                fixable[JRG.PASS_RESULT] == 0)
            if DISABLED_count:
                self.assertEqual(fixable[JRG.SKIP_RESULT], DISABLED_count)
            else:
                self.assertTrue(JRG.SKIP_RESULT not in fixable or
                                fixable[JRG.SKIP_RESULT] == 0)
            if FLAKY_count:
                self.assertEqual(fixable[JRG.FLAKY_RESULT], FLAKY_count)
            else:
                self.assertTrue(JRG.FLAKY_RESULT not in fixable or
                                fixable[JRG.FLAKY_RESULT] == 0)

        if failed_count_map:
            tests = buildinfo[JRG.TESTS]
            for test_name in failed_count_map.iterkeys():
                test = self._find_test_in_trie(test_name, tests)

                failed = 0
                for result in test[JRG.RESULTS]:
                    if result[1] == JRG.FAIL_RESULT:
                        failed += result[0]
                self.assertEqual(failed_count_map[test_name], failed)

                timing_count = 0
                for timings in test[JRG.TIMES]:
                    if timings[1] == test_timings[test_name]:
                        timing_count = timings[0]
                self.assertEqual(1, timing_count)

        if fixable_count:
            self.assertEqual(sum(buildinfo[JRG.FIXABLE_COUNT]), fixable_count)

    def _find_test_in_trie(self, path, trie):
        nodes = path.split("/")
        sub_trie = trie
        for node in nodes:
            self.assertIn(node, sub_trie)
            sub_trie = sub_trie[node]
        return sub_trie

    def test_json_generation(self):
        self._test_json_generation([], [])
        self._test_json_generation(['A1', 'B1'], [])
        self._test_json_generation([], ['FAILS_A2', 'FAILS_B2'])
        self._test_json_generation(['DISABLED_A3', 'DISABLED_B3'], [])
        self._test_json_generation(['A4'], ['B4', 'FAILS_C4'])
        self._test_json_generation(['DISABLED_C5', 'DISABLED_D5'], ['A5', 'B5'])
        self._test_json_generation(
            ['A6', 'B6', 'FAILS_C6', 'DISABLED_E6', 'DISABLED_F6'],
            ['FAILS_D6'])

        # Generate JSON with the same test sets. (Both incremental results and
        # archived results must be updated appropriately.)
        self._test_json_generation(
            ['A', 'FLAKY_B', 'DISABLED_C'],
            ['FAILS_D', 'FLAKY_E'])
        self._test_json_generation(
            ['A', 'DISABLED_C', 'FLAKY_E'],
            ['FLAKY_B', 'FAILS_D'])
        self._test_json_generation(
            ['FLAKY_B', 'DISABLED_C', 'FAILS_D'],
            ['A', 'FLAKY_E'])

    def test_hierarchical_json_generation(self):
        # FIXME: Re-work tests to be more comprehensible and comprehensive.
        self._test_json_generation(['foo/A'], ['foo/B', 'bar/C'])

    def test_test_timings_trie(self):
        test_port = test.TestPort(MockHost())
        individual_test_timings = []
        individual_test_timings.append(json_results_generator.TestResult('foo/bar/baz.html', elapsed_time=1.2))
        individual_test_timings.append(json_results_generator.TestResult('bar.html', elapsed_time=0.0001))
        trie = json_results_generator.test_timings_trie(test_port, individual_test_timings)

        expected_trie = {
          'bar.html': 0,
          'foo': {
              'bar': {
                  'baz.html': 1200,
              }
          }
        }

        self.assertEqual(json.dumps(trie), json.dumps(expected_trie))
