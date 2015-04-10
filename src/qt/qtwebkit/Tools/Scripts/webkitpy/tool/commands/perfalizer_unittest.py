# Copyright (C) 2012 Google Inc. All rights reserved.
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

from webkitpy.common.net.buildbot import Builder
from webkitpy.common.system.executive import ScriptError
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.port.test import TestPort
from webkitpy.tool.commands.perfalizer import PerfalizerTask
from webkitpy.tool.mocktool import MockTool


class PerfalizerTaskTest(unittest.TestCase):
    def _create_and_run_perfalizer(self, commands_to_fail=[]):
        tool = MockTool()
        patch = tool.bugs.fetch_attachment(10000)

        logs = []

        def logger(message):
            logs.append(message)

        def run_webkit_patch(args):
            if args[0] in commands_to_fail:
                raise ScriptError

        def run_perf_test(build_path, description):
            self.assertTrue(description == 'without 10000' or description == 'with 10000')
            if 'run-perf-tests' in commands_to_fail:
                return -1
            if 'results-page' not in commands_to_fail:
                tool.filesystem.write_text_file(tool.filesystem.join(build_path, 'PerformanceTestResults.html'), 'results page')
            return 0

        perfalizer = PerfalizerTask(tool, patch, logger)
        perfalizer._port = TestPort(tool)
        perfalizer.run_webkit_patch = run_webkit_patch
        perfalizer._run_perf_test = run_perf_test

        capture = OutputCapture()
        capture.capture_output()

        if commands_to_fail:
            self.assertFalse(perfalizer.run())
        else:
            self.assertTrue(perfalizer.run())

        capture.restore_output()

        return logs

    def test_run(self):
        self.assertEqual(self._create_and_run_perfalizer(), [
            'Preparing to run performance tests for the attachment 10000...',
            'Building WebKit at r1234 without the patch',
            'Building WebKit at r1234 with the patch',
            'Running performance tests...',
            'Uploaded the results on the bug 50000'])

    def test_run_with_clean_fails(self):
        self.assertEqual(self._create_and_run_perfalizer(['clean']), [
            'Preparing to run performance tests for the attachment 10000...',
            'Unable to clean working directory'])

    def test_run_with_update_fails(self):
        logs = self._create_and_run_perfalizer(['update'])
        self.assertEqual(len(logs), 2)
        self.assertEqual(logs[-1], 'Unable to update working directory')

    def test_run_with_build_fails(self):
        logs = self._create_and_run_perfalizer(['build'])
        self.assertEqual(len(logs), 3)

    def test_run_with_build_fails(self):
        logs = self._create_and_run_perfalizer(['apply-attachment'])
        self.assertEqual(len(logs), 4)

    def test_run_with_perf_test_fails(self):
        logs = self._create_and_run_perfalizer(['run-perf-tests'])
        self.assertEqual(len(logs), 5)
        self.assertEqual(logs[-1], 'Failed to run performance tests without the patch.')

    def test_run_without_results_page(self):
        logs = self._create_and_run_perfalizer(['results-page'])
        self.assertEqual(len(logs), 5)
        self.assertEqual(logs[-1], 'Failed to generate the results page.')
