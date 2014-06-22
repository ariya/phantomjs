# Copyright (C) 2012 Google Inc. All rights reserved.
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

"""Integration tests for run_perf_tests."""

import StringIO
import datetime
import json
import re
import unittest2 as unittest

from webkitpy.common.host_mock import MockHost
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.port.driver import DriverOutput
from webkitpy.port.test import TestPort
from webkitpy.performance_tests.perftest import PerfTest
from webkitpy.performance_tests.perftestsrunner import PerfTestsRunner


class EventTargetWrapperTestData:
    text = """Running 20 times
Ignoring warm-up run (1502)
1504
1505
1510
1504
1507
1509
1510
1487
1488
1472
1472
1488
1473
1472
1475
1487
1486
1486
1475
1471

Time:
values 1486, 1471, 1510, 1505, 1478, 1490 ms
avg 1490 ms
median 1488 ms
stdev 15.13935 ms
min 1471 ms
max 1510 ms
"""

    output = """Running Bindings/event-target-wrapper.html (1 of 2)
RESULT Bindings: event-target-wrapper: Time= 1490.0 ms
median= 1488.0 ms, stdev= 14.11751 ms, min= 1471.0 ms, max= 1510.0 ms
Finished: 0.1 s

"""

    results = {'url': 'http://trac.webkit.org/browser/trunk/PerformanceTests/Bindings/event-target-wrapper.html',
        'metrics': {'Time': {'current': [[1486.0, 1471.0, 1510.0, 1505.0, 1478.0, 1490.0]] * 4}}}


class SomeParserTestData:
    text = """Running 20 times
Ignoring warm-up run (1115)

Time:
values 1080, 1120, 1095, 1101, 1104 ms
avg 1100 ms
median 1101 ms
stdev 14.50861 ms
min 1080 ms
max 1120 ms
"""

    output = """Running Parser/some-parser.html (2 of 2)
RESULT Parser: some-parser: Time= 1100.0 ms
median= 1101.0 ms, stdev= 13.31402 ms, min= 1080.0 ms, max= 1120.0 ms
Finished: 0.1 s

"""

    results = {'url': 'http://trac.webkit.org/browser/trunk/PerformanceTests/Parser/some-parser.html',
        'metrics': {'Time': {'current': [[1080.0, 1120.0, 1095.0, 1101.0, 1104.0]] * 4}}}


class MemoryTestData:
    text = """Running 20 times
Ignoring warm-up run (1115)

Time:
values 1080, 1120, 1095, 1101, 1104 ms
avg 1100 ms
median 1101 ms
stdev 14.50861 ms
min 1080 ms
max 1120 ms

JS Heap:
values 825000, 811000, 848000, 837000, 829000 bytes
avg 830000 bytes
median 829000 bytes
stdev 13784.04875 bytes
min 811000 bytes
max 848000 bytes

Malloc:
values 529000, 511000, 548000, 536000, 521000 bytes
avg 529000 bytes
median 529000 bytes
stdev 14124.44689 bytes
min 511000 bytes
max 548000 bytes
"""

    output = """Running 1 tests
Running Parser/memory-test.html (1 of 1)
RESULT Parser: memory-test: Time= 1100.0 ms
median= 1101.0 ms, stdev= 13.31402 ms, min= 1080.0 ms, max= 1120.0 ms
RESULT Parser: memory-test: JSHeap= 830000.0 bytes
median= 829000.0 bytes, stdev= 12649.11064 bytes, min= 811000.0 bytes, max= 848000.0 bytes
RESULT Parser: memory-test: Malloc= 529000.0 bytes
median= 529000.0 bytes, stdev= 12961.48139 bytes, min= 511000.0 bytes, max= 548000.0 bytes
Finished: 0.1 s
"""

    results = {'current': [[1080, 1120, 1095, 1101, 1104]] * 4}
    js_heap_results = {'current': [[825000, 811000, 848000, 837000, 829000]] * 4}
    malloc_results = {'current': [[529000, 511000, 548000, 536000, 521000]] * 4}


class TestDriver:
    def run_test(self, driver_input, stop_when_done):
        text = ''
        timeout = False
        crash = False
        if driver_input.test_name.endswith('pass.html'):
            text = SomeParserTestData.text
        elif driver_input.test_name.endswith('timeout.html'):
            timeout = True
        elif driver_input.test_name.endswith('failed.html'):
            text = None
        elif driver_input.test_name.endswith('tonguey.html'):
            text = 'we are not expecting an output from perf tests but RESULT blablabla'
        elif driver_input.test_name.endswith('crash.html'):
            crash = True
        elif driver_input.test_name.endswith('event-target-wrapper.html'):
            text = EventTargetWrapperTestData.text
        elif driver_input.test_name.endswith('some-parser.html'):
            text = SomeParserTestData.text
        elif driver_input.test_name.endswith('memory-test.html'):
            text = MemoryTestData.text
        return DriverOutput(text, '', '', '', crash=crash, timeout=timeout)

    def start(self):
        """do nothing"""

    def stop(self):
        """do nothing"""


class MainTest(unittest.TestCase):
    def _normalize_output(self, log):
        return re.sub(r'(stdev=\s+\d+\.\d{5})\d+', r'\1', re.sub(r'Finished: [0-9\.]+ s', 'Finished: 0.1 s', log))

    def _load_output_json(self, runner):
        json_content = runner._host.filesystem.read_text_file(runner._output_json_path())
        return json.loads(re.sub(r'("stdev":\s*\d+\.\d{5})\d+', r'\1', json_content))

    def create_runner(self, args=[], driver_class=TestDriver):
        options, parsed_args = PerfTestsRunner._parse_args(args)
        test_port = TestPort(host=MockHost(), options=options)
        test_port.create_driver = lambda worker_number=None, no_timeout=False: driver_class()

        runner = PerfTestsRunner(args=args, port=test_port)
        runner._host.filesystem.maybe_make_directory(runner._base_path, 'inspector')
        runner._host.filesystem.maybe_make_directory(runner._base_path, 'Bindings')
        runner._host.filesystem.maybe_make_directory(runner._base_path, 'Parser')

        return runner, test_port

    def run_test(self, test_name):
        runner, port = self.create_runner()
        tests = [PerfTest(port, test_name, runner._host.filesystem.join('some-dir', test_name))]
        return runner._run_tests_set(tests) == 0

    def test_run_passing_test(self):
        self.assertTrue(self.run_test('pass.html'))

    def test_run_silent_test(self):
        self.assertFalse(self.run_test('silent.html'))

    def test_run_failed_test(self):
        self.assertFalse(self.run_test('failed.html'))

    def test_run_tonguey_test(self):
        self.assertFalse(self.run_test('tonguey.html'))

    def test_run_timeout_test(self):
        self.assertFalse(self.run_test('timeout.html'))

    def test_run_crash_test(self):
        self.assertFalse(self.run_test('crash.html'))

    def _tests_for_runner(self, runner, test_names):
        filesystem = runner._host.filesystem
        tests = []
        for test in test_names:
            path = filesystem.join(runner._base_path, test)
            dirname = filesystem.dirname(path)
            tests.append(PerfTest(runner._port, test, path))
        return tests

    def test_run_test_set_kills_drt_per_run(self):

        class TestDriverWithStopCount(TestDriver):
            stop_count = 0
            def stop(self):
                TestDriverWithStopCount.stop_count += 1

        runner, port = self.create_runner(driver_class=TestDriverWithStopCount)

        tests = self._tests_for_runner(runner, ['inspector/pass.html', 'inspector/silent.html', 'inspector/failed.html',
            'inspector/tonguey.html', 'inspector/timeout.html', 'inspector/crash.html'])
        unexpected_result_count = runner._run_tests_set(tests)

        self.assertEqual(TestDriverWithStopCount.stop_count, 9)

    def test_run_test_set_for_parser_tests(self):
        runner, port = self.create_runner()
        tests = self._tests_for_runner(runner, ['Bindings/event-target-wrapper.html', 'Parser/some-parser.html'])
        output = OutputCapture()
        output.capture_output()
        try:
            unexpected_result_count = runner._run_tests_set(tests)
        finally:
            stdout, stderr, log = output.restore_output()
        self.assertEqual(unexpected_result_count, 0)
        self.assertEqual(self._normalize_output(log), EventTargetWrapperTestData.output + SomeParserTestData.output)

    def test_run_memory_test(self):
        runner, port = self.create_runner_and_setup_results_template()
        runner._timestamp = 123456789
        port.host.filesystem.write_text_file(runner._base_path + '/Parser/memory-test.html', 'some content')

        output = OutputCapture()
        output.capture_output()
        try:
            unexpected_result_count = runner.run()
        finally:
            stdout, stderr, log = output.restore_output()
        self.assertEqual(unexpected_result_count, 0)
        self.assertEqual(self._normalize_output(log), MemoryTestData.output + '\nMOCK: user.open_url: file://...\n')
        parser_tests = self._load_output_json(runner)[0]['tests']['Parser']['tests']
        self.assertEqual(parser_tests['memory-test']['metrics']['Time'], MemoryTestData.results)
        self.assertEqual(parser_tests['memory-test']['metrics']['JSHeap'], MemoryTestData.js_heap_results)
        self.assertEqual(parser_tests['memory-test']['metrics']['Malloc'], MemoryTestData.malloc_results)

    def _test_run_with_json_output(self, runner, filesystem, upload_succeeds=False, results_shown=True, expected_exit_code=0, repeat=1, compare_logs=True):
        filesystem.write_text_file(runner._base_path + '/Parser/some-parser.html', 'some content')
        filesystem.write_text_file(runner._base_path + '/Bindings/event-target-wrapper.html', 'some content')

        uploaded = [False]

        def mock_upload_json(hostname, json_path, host_path=None):
            # FIXME: Get rid of the hard-coded perf.webkit.org once we've completed the transition.
            self.assertIn(hostname, ['some.host'])
            self.assertIn(json_path, ['/mock-checkout/output.json'])
            self.assertIn(host_path, [None, '/api/report'])
            uploaded[0] = upload_succeeds
            return upload_succeeds

        runner._upload_json = mock_upload_json
        runner._timestamp = 123456789
        runner._utc_timestamp = datetime.datetime(2013, 2, 8, 15, 19, 37, 460000)
        output_capture = OutputCapture()
        output_capture.capture_output()
        try:
            self.assertEqual(runner.run(), expected_exit_code)
        finally:
            stdout, stderr, logs = output_capture.restore_output()

        if not expected_exit_code and compare_logs:
            expected_logs = ''
            for i in xrange(repeat):
                runs = ' (Run %d of %d)' % (i + 1, repeat) if repeat > 1 else ''
                expected_logs += 'Running 2 tests%s\n' % runs + EventTargetWrapperTestData.output + SomeParserTestData.output
            if results_shown:
                expected_logs += 'MOCK: user.open_url: file://...\n'
            self.assertEqual(self._normalize_output(logs), expected_logs)

        self.assertEqual(uploaded[0], upload_succeeds)

        return logs

    _event_target_wrapper_and_inspector_results = {
        "Bindings":
            {"url": "http://trac.webkit.org/browser/trunk/PerformanceTests/Bindings",
            "tests": {"event-target-wrapper": EventTargetWrapperTestData.results}},
        "Parser":
            {"url": "http://trac.webkit.org/browser/trunk/PerformanceTests/Parser",
            "tests": {"some-parser": SomeParserTestData.results}}}

    def test_run_with_json_output(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server=some.host'])
        self._test_run_with_json_output(runner, port.host.filesystem, upload_succeeds=True)
        self.assertEqual(self._load_output_json(runner), [{
            "buildTime": "2013-02-08T15:19:37.460000", "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}}])

        filesystem = port.host.filesystem
        self.assertTrue(filesystem.isfile(runner._output_json_path()))
        self.assertTrue(filesystem.isfile(filesystem.splitext(runner._output_json_path())[0] + '.html'))

    def test_run_with_description(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server=some.host', '--description', 'some description'])
        self._test_run_with_json_output(runner, port.host.filesystem, upload_succeeds=True)
        self.assertEqual(self._load_output_json(runner), [{
            "buildTime": "2013-02-08T15:19:37.460000", "description": "some description",
            "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}}])

    def create_runner_and_setup_results_template(self, args=[]):
        runner, port = self.create_runner(args)
        filesystem = port.host.filesystem
        filesystem.write_text_file(runner._base_path + '/resources/results-template.html',
            'BEGIN<script src="%AbsolutePathToWebKitTrunk%/some.js"></script>'
            '<script src="%AbsolutePathToWebKitTrunk%/other.js"></script><script>%PeformanceTestsResultsJSON%</script>END')
        filesystem.write_text_file(runner._base_path + '/Dromaeo/resources/dromaeo/web/lib/jquery-1.6.4.js', 'jquery content')
        return runner, port

    def test_run_respects_no_results(self):
        runner, port = self.create_runner(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server=some.host', '--no-results'])
        self._test_run_with_json_output(runner, port.host.filesystem, upload_succeeds=False, results_shown=False)
        self.assertFalse(port.host.filesystem.isfile('/mock-checkout/output.json'))

    def test_run_generates_json_by_default(self):
        runner, port = self.create_runner_and_setup_results_template()
        filesystem = port.host.filesystem
        output_json_path = runner._output_json_path()
        results_page_path = filesystem.splitext(output_json_path)[0] + '.html'

        self.assertFalse(filesystem.isfile(output_json_path))
        self.assertFalse(filesystem.isfile(results_page_path))

        self._test_run_with_json_output(runner, port.host.filesystem)

        self.assertEqual(self._load_output_json(runner), [{
            "buildTime": "2013-02-08T15:19:37.460000", "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}}])

        self.assertTrue(filesystem.isfile(output_json_path))
        self.assertTrue(filesystem.isfile(results_page_path))

    def test_run_merges_output_by_default(self):
        runner, port = self.create_runner_and_setup_results_template()
        filesystem = port.host.filesystem
        output_json_path = runner._output_json_path()

        filesystem.write_text_file(output_json_path, '[{"previous": "results"}]')

        self._test_run_with_json_output(runner, port.host.filesystem)

        self.assertEqual(self._load_output_json(runner), [{"previous": "results"}, {
            "buildTime": "2013-02-08T15:19:37.460000", "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}}])
        self.assertTrue(filesystem.isfile(filesystem.splitext(output_json_path)[0] + '.html'))

    def test_run_respects_reset_results(self):
        runner, port = self.create_runner_and_setup_results_template(args=["--reset-results"])
        filesystem = port.host.filesystem
        output_json_path = runner._output_json_path()

        filesystem.write_text_file(output_json_path, '[{"previous": "results"}]')

        self._test_run_with_json_output(runner, port.host.filesystem)

        self.assertEqual(self._load_output_json(runner), [{
            "buildTime": "2013-02-08T15:19:37.460000", "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}}])
        self.assertTrue(filesystem.isfile(filesystem.splitext(output_json_path)[0] + '.html'))
        pass

    def test_run_generates_and_show_results_page(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json'])
        page_shown = []
        port.show_results_html_file = lambda path: page_shown.append(path)
        filesystem = port.host.filesystem
        self._test_run_with_json_output(runner, filesystem, results_shown=False)

        expected_entry = {"buildTime": "2013-02-08T15:19:37.460000", "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}}

        self.maxDiff = None
        self.assertEqual(runner._output_json_path(), '/mock-checkout/output.json')
        self.assertEqual(self._load_output_json(runner), [expected_entry])
        self.assertEqual(filesystem.read_text_file('/mock-checkout/output.html'),
            'BEGIN<script src="/test.checkout/some.js"></script><script src="/test.checkout/other.js"></script>'
            '<script>%s</script>END' % port.host.filesystem.read_text_file(runner._output_json_path()))
        self.assertEqual(page_shown[0], '/mock-checkout/output.html')

        self._test_run_with_json_output(runner, filesystem, results_shown=False)
        self.assertEqual(runner._output_json_path(), '/mock-checkout/output.json')
        self.assertEqual(self._load_output_json(runner), [expected_entry, expected_entry])
        self.assertEqual(filesystem.read_text_file('/mock-checkout/output.html'),
            'BEGIN<script src="/test.checkout/some.js"></script><script src="/test.checkout/other.js"></script>'
            '<script>%s</script>END' % port.host.filesystem.read_text_file(runner._output_json_path()))

    def test_run_respects_no_show_results(self):
        show_results_html_file = lambda path: page_shown.append(path)

        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json'])
        page_shown = []
        port.show_results_html_file = show_results_html_file
        self._test_run_with_json_output(runner, port.host.filesystem, results_shown=False)
        self.assertEqual(page_shown[0], '/mock-checkout/output.html')

        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--no-show-results'])
        page_shown = []
        port.show_results_html_file = show_results_html_file
        self._test_run_with_json_output(runner, port.host.filesystem, results_shown=False)
        self.assertEqual(page_shown, [])

    def test_run_with_bad_output_json(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json'])
        port.host.filesystem.write_text_file('/mock-checkout/output.json', 'bad json')
        self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_MERGE)
        port.host.filesystem.write_text_file('/mock-checkout/output.json', '{"another bad json": "1"}')
        self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_MERGE)

    def test_run_with_slave_config_json(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--slave-config-json-path=/mock-checkout/slave-config.json', '--test-results-server=some.host'])
        port.host.filesystem.write_text_file('/mock-checkout/slave-config.json', '{"key": "value"}')
        self._test_run_with_json_output(runner, port.host.filesystem, upload_succeeds=True)
        self.assertEqual(self._load_output_json(runner), [{
            "buildTime": "2013-02-08T15:19:37.460000", "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}, "builderKey": "value"}])

    def test_run_with_bad_slave_config_json(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--slave-config-json-path=/mock-checkout/slave-config.json', '--test-results-server=some.host'])
        logs = self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_SOURCE_JSON)
        self.assertTrue('Missing slave configuration JSON file: /mock-checkout/slave-config.json' in logs)
        port.host.filesystem.write_text_file('/mock-checkout/slave-config.json', 'bad json')
        self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_SOURCE_JSON)
        port.host.filesystem.write_text_file('/mock-checkout/slave-config.json', '["another bad json"]')
        self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_SOURCE_JSON)

    def test_run_with_multiple_repositories(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server=some.host'])
        port.repository_paths = lambda: [('webkit', '/mock-checkout'), ('some', '/mock-checkout/some')]
        self._test_run_with_json_output(runner, port.host.filesystem, upload_succeeds=True)
        self.assertEqual(self._load_output_json(runner), [{
            "buildTime": "2013-02-08T15:19:37.460000", "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"webkit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"},
            "some": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}}])

    def test_run_with_upload_json(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server', 'some.host', '--platform', 'platform1', '--builder-name', 'builder1', '--build-number', '123'])

        self._test_run_with_json_output(runner, port.host.filesystem, upload_succeeds=True)
        generated_json = json.loads(port.host.filesystem.files['/mock-checkout/output.json'])
        self.assertEqual(generated_json[0]['platform'], 'platform1')
        self.assertEqual(generated_json[0]['builderName'], 'builder1')
        self.assertEqual(generated_json[0]['buildNumber'], 123)

        self._test_run_with_json_output(runner, port.host.filesystem, upload_succeeds=False, expected_exit_code=PerfTestsRunner.EXIT_CODE_FAILED_UPLOADING)

    def test_run_with_upload_json_should_generate_perf_webkit_json(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server', 'some.host', '--platform', 'platform1', '--builder-name', 'builder1', '--build-number', '123',
            '--slave-config-json-path=/mock-checkout/slave-config.json'])
        port.host.filesystem.write_text_file('/mock-checkout/slave-config.json', '{"key": "value1"}')

        self._test_run_with_json_output(runner, port.host.filesystem, upload_succeeds=True)
        generated_json = json.loads(port.host.filesystem.files['/mock-checkout/output.json'])
        self.assertTrue(isinstance(generated_json, list))
        self.assertEqual(len(generated_json), 1)

        output = generated_json[0]
        self.maxDiff = None
        self.assertEqual(output['platform'], 'platform1')
        self.assertEqual(output['buildNumber'], 123)
        self.assertEqual(output['buildTime'], '2013-02-08T15:19:37.460000')
        self.assertEqual(output['builderName'], 'builder1')
        self.assertEqual(output['builderKey'], 'value1')
        self.assertEqual(output['revisions'], {'WebKit': {'revision': '5678', 'timestamp': '2013-02-01 08:48:05 +0000'}})
        self.assertEqual(output['tests'].keys(), ['Bindings', 'Parser'])
        self.assertEqual(sorted(output['tests']['Bindings'].keys()), ['tests', 'url'])
        self.assertEqual(output['tests']['Bindings']['url'], 'http://trac.webkit.org/browser/trunk/PerformanceTests/Bindings')
        self.assertEqual(output['tests']['Bindings']['tests'].keys(), ['event-target-wrapper'])
        self.assertEqual(output['tests']['Bindings']['tests']['event-target-wrapper'], {
            'url': 'http://trac.webkit.org/browser/trunk/PerformanceTests/Bindings/event-target-wrapper.html',
            'metrics': {'Time': {'current': [[1486.0, 1471.0, 1510.0, 1505.0, 1478.0, 1490.0]] * 4}}})

    def test_run_with_repeat(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server=some.host', '--repeat', '5'])
        self._test_run_with_json_output(runner, port.host.filesystem, upload_succeeds=True, repeat=5)
        self.assertEqual(self._load_output_json(runner), [
            {"buildTime": "2013-02-08T15:19:37.460000",
            "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}},
            {"buildTime": "2013-02-08T15:19:37.460000",
            "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}},
            {"buildTime": "2013-02-08T15:19:37.460000",
            "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}},
            {"buildTime": "2013-02-08T15:19:37.460000",
            "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}},
            {"buildTime": "2013-02-08T15:19:37.460000",
            "tests": self._event_target_wrapper_and_inspector_results,
            "revisions": {"WebKit": {"timestamp": "2013-02-01 08:48:05 +0000", "revision": "5678"}}}])

    def test_run_with_test_runner_count(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--test-runner-count=3'])
        self._test_run_with_json_output(runner, port.host.filesystem, compare_logs=False)
        generated_json = json.loads(port.host.filesystem.files['/mock-checkout/output.json'])
        self.assertTrue(isinstance(generated_json, list))
        self.assertEqual(len(generated_json), 1)

        output = generated_json[0]['tests']['Bindings']['tests']['event-target-wrapper']['metrics']['Time']['current']
        self.assertEqual(len(output), 3)
        expectedMetrics = EventTargetWrapperTestData.results['metrics']['Time']['current'][0]
        for metrics in output:
            self.assertEqual(metrics, expectedMetrics)
