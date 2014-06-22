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

import StringIO
import json
import math
import unittest2 as unittest

from webkitpy.common.host_mock import MockHost
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.port.driver import DriverOutput
from webkitpy.port.test import TestDriver
from webkitpy.port.test import TestPort
from webkitpy.performance_tests.perftest import PerfTest
from webkitpy.performance_tests.perftest import PerfTestMetric
from webkitpy.performance_tests.perftest import PerfTestFactory
from webkitpy.performance_tests.perftest import ReplayPerfTest
from webkitpy.performance_tests.perftest import SingleProcessPerfTest


class MockPort(TestPort):
    def __init__(self, custom_run_test=None):
        super(MockPort, self).__init__(host=MockHost(), custom_run_test=custom_run_test)


class TestPerfTestMetric(unittest.TestCase):
    def test_init_set_missing_unit(self):
        self.assertEqual(PerfTestMetric('Time', iterations=[1, 2, 3, 4, 5]).unit(), 'ms')
        self.assertEqual(PerfTestMetric('Malloc', iterations=[1, 2, 3, 4, 5]).unit(), 'bytes')
        self.assertEqual(PerfTestMetric('JSHeap', iterations=[1, 2, 3, 4, 5]).unit(), 'bytes')

    def test_init_set_time_metric(self):
        self.assertEqual(PerfTestMetric('Time', 'ms').name(), 'Time')
        self.assertEqual(PerfTestMetric('Time', 'fps').name(), 'FrameRate')
        self.assertEqual(PerfTestMetric('Time', 'runs/s').name(), 'Runs')

    def test_has_values(self):
        self.assertFalse(PerfTestMetric('Time').has_values())
        self.assertTrue(PerfTestMetric('Time', iterations=[1]).has_values())

    def test_append(self):
        metric = PerfTestMetric('Time')
        metric2 = PerfTestMetric('Time')
        self.assertFalse(metric.has_values())
        self.assertFalse(metric2.has_values())

        metric.append_group([1])
        self.assertTrue(metric.has_values())
        self.assertFalse(metric2.has_values())
        self.assertEqual(metric.grouped_iteration_values(), [[1]])
        self.assertEqual(metric.flattened_iteration_values(), [1])

        metric.append_group([2])
        self.assertEqual(metric.grouped_iteration_values(), [[1], [2]])
        self.assertEqual(metric.flattened_iteration_values(), [1, 2])

        metric2.append_group([3])
        self.assertTrue(metric2.has_values())
        self.assertEqual(metric.flattened_iteration_values(), [1, 2])
        self.assertEqual(metric2.flattened_iteration_values(), [3])

        metric.append_group([4, 5])
        self.assertEqual(metric.grouped_iteration_values(), [[1], [2], [4, 5]])
        self.assertEqual(metric.flattened_iteration_values(), [1, 2, 4, 5])


class TestPerfTest(unittest.TestCase):
    def _assert_results_are_correct(self, test, output):
        test.run_single = lambda driver, path, time_out_ms: output
        self.assertTrue(test._run_with_driver(None, None))
        self.assertEqual(test._metrics.keys(), ['Time'])
        self.assertEqual(test._metrics['Time'].flattened_iteration_values(), [1080, 1120, 1095, 1101, 1104])

    def test_parse_output(self):
        output = DriverOutput("""
Running 20 times
Ignoring warm-up run (1115)

Time:
values 1080, 1120, 1095, 1101, 1104 ms
avg 1100 ms
median 1101 ms
stdev 14.50862 ms
min 1080 ms
max 1120 ms
""", image=None, image_hash=None, audio=None)
        output_capture = OutputCapture()
        output_capture.capture_output()
        try:
            test = PerfTest(MockPort(), 'some-test', '/path/some-dir/some-test')
            self._assert_results_are_correct(test, output)
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()
        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, '')

    def test_parse_output_with_failing_line(self):
        output = DriverOutput("""
Running 20 times
Ignoring warm-up run (1115)

some-unrecognizable-line

Time:
values 1080, 1120, 1095, 1101, 1104 ms
avg 1100 ms
median 1101 ms
stdev 14.50862 ms
min 1080 ms
max 1120 ms
""", image=None, image_hash=None, audio=None)
        output_capture = OutputCapture()
        output_capture.capture_output()
        try:
            test = PerfTest(MockPort(), 'some-test', '/path/some-dir/some-test')
            test.run_single = lambda driver, path, time_out_ms: output
            self.assertFalse(test._run_with_driver(None, None))
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()
        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, 'ERROR: some-unrecognizable-line\n')

    def test_parse_output_with_description(self):
        output = DriverOutput("""
Description: this is a test description.

Running 20 times
Ignoring warm-up run (1115)

Time:
values 1080, 1120, 1095, 1101, 1104 ms
avg 1100 ms
median 1101 ms
stdev 14.50862 ms
min 1080 ms
max 1120 ms""", image=None, image_hash=None, audio=None)
        test = PerfTest(MockPort(), 'some-test', '/path/some-dir/some-test')
        self._assert_results_are_correct(test, output)
        self.assertEqual(test.description(), 'this is a test description.')

    def test_ignored_stderr_lines(self):
        test = PerfTest(MockPort(), 'some-test', '/path/some-dir/some-test')
        output_with_lines_to_ignore = DriverOutput('', image=None, image_hash=None, audio=None, error="""
Unknown option: --foo-bar
Should not be ignored
[WARNING:proxy_service.cc] bad moon a-rising
[WARNING:chrome.cc] Something went wrong
[INFO:SkFontHost_android.cpp(1158)] Use Test Config File Main /data/local/tmp/drt/android_main_fonts.xml, Fallback /data/local/tmp/drt/android_fallback_fonts.xml, Font Dir /data/local/tmp/drt/fonts/
[ERROR:main.cc] The sky has fallen""")
        test._filter_output(output_with_lines_to_ignore)
        self.assertEqual(output_with_lines_to_ignore.error,
            "Should not be ignored\n"
            "[WARNING:chrome.cc] Something went wrong\n"
            "[ERROR:main.cc] The sky has fallen")

    def test_parse_output_with_subtests(self):
        output = DriverOutput("""
Running 20 times
some test: [1, 2, 3, 4, 5]
other test = else: [6, 7, 8, 9, 10]
Ignoring warm-up run (1115)

Time:
values 1080, 1120, 1095, 1101, 1104 ms
avg 1100 ms
median 1101 ms
stdev 14.50862 ms
min 1080 ms
max 1120 ms
""", image=None, image_hash=None, audio=None)
        output_capture = OutputCapture()
        output_capture.capture_output()
        try:
            test = PerfTest(MockPort(), 'some-test', '/path/some-dir/some-test')
            self._assert_results_are_correct(test, output)
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()
        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, '')


class TestSingleProcessPerfTest(unittest.TestCase):
    def test_use_only_one_process(self):
        called = [0]

        def run_single(driver, path, time_out_ms):
            called[0] += 1
            return DriverOutput("""
Running 20 times
Ignoring warm-up run (1115)

Time:
values 1080, 1120, 1095, 1101, 1104 ms
avg 1100 ms
median 1101 ms
stdev 14.50862 ms
min 1080 ms
max 1120 ms""", image=None, image_hash=None, audio=None)

        test = SingleProcessPerfTest(MockPort(), 'some-test', '/path/some-dir/some-test')
        test.run_single = run_single
        self.assertTrue(test.run(0))
        self.assertEqual(called[0], 1)


class TestReplayPerfTest(unittest.TestCase):
    class ReplayTestPort(MockPort):
        def __init__(self, custom_run_test=None):

            class ReplayTestDriver(TestDriver):
                def run_test(self, text_input, stop_when_done):
                    return custom_run_test(text_input, stop_when_done) if custom_run_test else None

            self._custom_driver_class = ReplayTestDriver
            super(self.__class__, self).__init__()

        def _driver_class(self):
            return self._custom_driver_class

    class MockReplayServer(object):
        def __init__(self, wait_until_ready=True):
            self.wait_until_ready = lambda: wait_until_ready

        def stop(self):
            pass

    def _add_file(self, port, dirname, filename, content=True):
        port.host.filesystem.maybe_make_directory(dirname)
        port.host.filesystem.write_binary_file(port.host.filesystem.join(dirname, filename), content)

    def _setup_test(self, run_test=None):
        test_port = self.ReplayTestPort(run_test)
        self._add_file(test_port, '/path/some-dir', 'some-test.replay', 'http://some-test/')
        test = ReplayPerfTest(test_port, 'some-test.replay', '/path/some-dir/some-test.replay')
        test._start_replay_server = lambda archive, record: self.__class__.MockReplayServer()
        return test, test_port

    def test_run_single(self):
        output_capture = OutputCapture()
        output_capture.capture_output()

        loaded_pages = []

        def run_test(test_input, stop_when_done):
            if test_input.test_name == test.force_gc_test:
                loaded_pages.append(test_input)
                return
            if test_input.test_name != "about:blank":
                self.assertEqual(test_input.test_name, 'http://some-test/')
            loaded_pages.append(test_input)
            self._add_file(port, '/path/some-dir', 'some-test.wpr', 'wpr content')
            return DriverOutput('actual text', 'actual image', 'actual checksum',
                audio=None, crash=False, timeout=False, error=False, test_time=12345)

        test, port = self._setup_test(run_test)
        test._archive_path = '/path/some-dir/some-test.wpr'
        test._url = 'http://some-test/'

        try:
            driver = port.create_driver(worker_number=1, no_timeout=True)
            output = test.run_single(driver, '/path/some-dir/some-test.replay', time_out_ms=100)
            self.assertTrue(output)
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()

        self.assertEqual(len(loaded_pages), 2)
        self.assertEqual(loaded_pages[0].test_name, test.force_gc_test)
        self.assertEqual(loaded_pages[1].test_name, 'http://some-test/')
        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, '')
        self.assertEqual(port.host.filesystem.read_binary_file('/path/some-dir/some-test-actual.png'), 'actual image')
        self.assertEqual(output.test_time, 12345)

    def test_run_single_fails_without_webpagereplay(self):
        output_capture = OutputCapture()
        output_capture.capture_output()

        test, port = self._setup_test()
        test._start_replay_server = lambda archive, record: None
        test._archive_path = '/path/some-dir.wpr'
        test._url = 'http://some-test/'

        try:
            driver = port.create_driver(worker_number=1, no_timeout=True)
            self.assertEqual(test.run_single(driver, '/path/some-dir/some-test.replay', time_out_ms=100), None)
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()
        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, "Web page replay didn't start.\n")

    def test_run_with_driver_accumulates_results(self):
        port = MockPort()
        test, port = self._setup_test()
        counter = [0]

        def mock_run_signle(drive, path, timeout):
            counter[0] += 1
            return DriverOutput('some output', image=None, image_hash=None, audio=None, test_time=counter[0], measurements={})

        test.run_single = mock_run_signle
        output_capture = OutputCapture()
        output_capture.capture_output()
        try:
            driver = port.create_driver(worker_number=1, no_timeout=True)
            self.assertTrue(test._run_with_driver(driver, None))
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()

        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, '')

        self.assertEqual(test._metrics.keys(), ['Time'])
        self.assertEqual(test._metrics['Time'].flattened_iteration_values(), [float(i * 1000) for i in range(2, 7)])

    def test_run_with_driver_accumulates_memory_results(self):
        port = MockPort()
        test, port = self._setup_test()
        counter = [0]

        def mock_run_signle(drive, path, timeout):
            counter[0] += 1
            return DriverOutput('some output', image=None, image_hash=None, audio=None, test_time=counter[0], measurements={'Malloc': 10, 'JSHeap': 5})

        test.run_single = mock_run_signle
        output_capture = OutputCapture()
        output_capture.capture_output()
        try:
            driver = port.create_driver(worker_number=1, no_timeout=True)
            self.assertTrue(test._run_with_driver(driver, None))
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()

        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, '')

        metrics = test._metrics
        self.assertEqual(sorted(metrics.keys()), ['JSHeap', 'Malloc', 'Time'])
        self.assertEqual(metrics['Time'].flattened_iteration_values(), [float(i * 1000) for i in range(2, 7)])
        self.assertEqual(metrics['Malloc'].flattened_iteration_values(), [float(10)] * 5)
        self.assertEqual(metrics['JSHeap'].flattened_iteration_values(), [float(5)] * 5)

    def test_prepare_fails_when_wait_until_ready_fails(self):
        output_capture = OutputCapture()
        output_capture.capture_output()

        test, port = self._setup_test()
        test._start_replay_server = lambda archive, record: self.__class__.MockReplayServer(wait_until_ready=False)
        test._archive_path = '/path/some-dir.wpr'
        test._url = 'http://some-test/'

        try:
            driver = port.create_driver(worker_number=1, no_timeout=True)
            self.assertEqual(test.run_single(driver, '/path/some-dir/some-test.replay', time_out_ms=100), None)
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()

        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, "Web page replay didn't start.\n")

    def test_run_single_fails_when_output_has_error(self):
        output_capture = OutputCapture()
        output_capture.capture_output()

        loaded_pages = []

        def run_test(test_input, stop_when_done):
            loaded_pages.append(test_input)
            self._add_file(port, '/path/some-dir', 'some-test.wpr', 'wpr content')
            return DriverOutput('actual text', 'actual image', 'actual checksum',
                audio=None, crash=False, timeout=False, error='some error')

        test, port = self._setup_test(run_test)
        test._archive_path = '/path/some-dir.wpr'
        test._url = 'http://some-test/'

        try:
            driver = port.create_driver(worker_number=1, no_timeout=True)
            self.assertEqual(test.run_single(driver, '/path/some-dir/some-test.replay', time_out_ms=100), None)
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()

        self.assertEqual(len(loaded_pages), 2)
        self.assertEqual(loaded_pages[0].test_name, test.force_gc_test)
        self.assertEqual(loaded_pages[1].test_name, 'http://some-test/')
        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, 'error: some-test.replay\nsome error\n')

    def test_prepare(self):
        output_capture = OutputCapture()
        output_capture.capture_output()

        def run_test(test_input, stop_when_done):
            self._add_file(port, '/path/some-dir', 'some-test.wpr', 'wpr content')
            return DriverOutput('actual text', 'actual image', 'actual checksum',
                audio=None, crash=False, timeout=False, error=False)

        test, port = self._setup_test(run_test)

        try:
            self.assertTrue(test.prepare(time_out_ms=100))
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()

        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, 'Preparing replay for some-test.replay\nPrepared replay for some-test.replay\n')
        self.assertEqual(port.host.filesystem.read_binary_file('/path/some-dir/some-test-expected.png'), 'actual image')

    def test_prepare_calls_run_single(self):
        output_capture = OutputCapture()
        output_capture.capture_output()
        called = [False]

        def run_single(driver, url, time_out_ms, record):
            self.assertTrue(record)
            self.assertEqual(url, '/path/some-dir/some-test.wpr')
            called[0] = True
            return False

        test, port = self._setup_test()
        test.run_single = run_single

        try:
            self.assertFalse(test.prepare(time_out_ms=100))
        finally:
            actual_stdout, actual_stderr, actual_logs = output_capture.restore_output()
        self.assertTrue(called[0])
        self.assertEqual(test._archive_path, '/path/some-dir/some-test.wpr')
        self.assertEqual(test._url, 'http://some-test/')
        self.assertEqual(actual_stdout, '')
        self.assertEqual(actual_stderr, '')
        self.assertEqual(actual_logs, "Preparing replay for some-test.replay\nFailed to prepare a replay for some-test.replay\n")


class TestPerfTestFactory(unittest.TestCase):
    def test_regular_test(self):
        test = PerfTestFactory.create_perf_test(MockPort(), 'some-dir/some-test', '/path/some-dir/some-test')
        self.assertEqual(test.__class__, PerfTest)
