# Copyright (c) 2011 Google Inc. All rights reserved.
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

from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.net.layouttestresults import LayoutTestResults
from webkitpy.common.host_mock import MockHost

from .layouttestresultsreader import LayoutTestResultsReader


class LayoutTestResultsReaderTest(unittest.TestCase):
    def test_missing_layout_test_results(self):
        host = MockHost()
        reader = LayoutTestResultsReader(host, "/mock-results", "/var/logs")
        layout_tests_results_path = '/mock-results/full_results.json'
        unit_tests_results_path = '/mock-results/webkit_unit_tests_output.xml'
        host.filesystem = MockFileSystem({layout_tests_results_path: None,
                                          unit_tests_results_path: None})
        # Make sure that our filesystem mock functions as we expect.
        self.assertRaises(IOError, host.filesystem.read_text_file, layout_tests_results_path)
        self.assertRaises(IOError, host.filesystem.read_text_file, unit_tests_results_path)
        # layout_test_results shouldn't raise even if the results.json file is missing.
        self.assertIsNone(reader.results())

    def test_create_unit_test_results(self):
        host = MockHost()
        reader = LayoutTestResultsReader(host, "/mock-results", "/var/logs")
        unit_tests_results_path = '/mock-results/webkit_unit_tests_output.xml'
        no_failures_xml = """<?xml version="1.0" encoding="UTF-8"?>
<testsuites tests="3" failures="0" disabled="0" errors="0" time="11.35" name="AllTests">
  <testsuite name="RenderTableCellDeathTest" tests="3" failures="0" disabled="0" errors="0" time="0.677">
    <testcase name="CanSetColumn" status="run" time="0.168" classname="RenderTableCellDeathTest" />
    <testcase name="CrashIfSettingUnsetColumnIndex" status="run" time="0.129" classname="RenderTableCellDeathTest" />
    <testcase name="CrashIfSettingUnsetRowIndex" status="run" time="0.123" classname="RenderTableCellDeathTest" />
  </testsuite>
</testsuites>"""
        host.filesystem = MockFileSystem({unit_tests_results_path: no_failures_xml})
        self.assertEqual(reader._create_unit_test_results(), [])

    def test_missing_unit_test_results_path(self):
        host = MockHost()
        reader = LayoutTestResultsReader(host, "/mock-results", "/var/logs")
        reader._create_layout_test_results = lambda: LayoutTestResults([])
        reader._create_unit_test_results = lambda: None
        # layout_test_results shouldn't raise even if the unit tests xml file is missing.
        self.assertIsNotNone(reader.results(), None)
        self.assertEqual(reader.results().failing_tests(), [])


    def test_layout_test_results(self):
        reader = LayoutTestResultsReader(MockHost(), "/mock-results", "/var/logs")
        reader._read_file_contents = lambda path: None
        self.assertIsNone(reader.results())
        reader._read_file_contents = lambda path: ""
        self.assertIsNone(reader.results())
        reader._create_layout_test_results = lambda: LayoutTestResults([])
        results = reader.results()
        self.assertIsNotNone(results)
        self.assertEqual(results.failure_limit_count(), 30)  # This value matches RunTests.NON_INTERACTIVE_FAILURE_LIMIT_COUNT

    def test_archive_last_layout_test_results(self):
        host = MockHost()
        results_directory = "/mock-results"
        reader = LayoutTestResultsReader(host, results_directory, "/var/logs")
        patch = host.bugs.fetch_attachment(10001)
        host.filesystem = MockFileSystem()
        # Should fail because the results_directory does not exist.
        expected_logs = "/mock-results does not exist, not archiving.\n"
        archive = OutputCapture().assert_outputs(self, reader.archive, [patch], expected_logs=expected_logs)
        self.assertIsNone(archive)

        host.filesystem.maybe_make_directory(results_directory)
        self.assertTrue(host.filesystem.exists(results_directory))

        self.assertIsNotNone(reader.archive(patch))
        self.assertFalse(host.filesystem.exists(results_directory))

    def test_archive_last_layout_test_results_with_relative_path(self):
        host = MockHost()
        results_directory = "/mock-checkout/layout-test-results"

        host.filesystem.maybe_make_directory(results_directory)
        host.filesystem.maybe_make_directory('/var/logs')
        self.assertTrue(host.filesystem.exists(results_directory))

        host.filesystem.chdir('/var')
        reader = LayoutTestResultsReader(host, results_directory, 'logs')
        patch = host.bugs.fetch_attachment(10001)
        # Should fail because the results_directory does not exist.
        self.assertIsNotNone(reader.archive(patch))
        self.assertEqual(host.workspace.source_path, results_directory)
        self.assertEqual(host.workspace.zip_path, '/var/logs/50000-layout-test-results.zip')
