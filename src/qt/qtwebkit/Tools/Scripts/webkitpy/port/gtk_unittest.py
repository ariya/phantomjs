# Copyright (C) 2011 Google Inc. All rights reserved.
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
import sys
import os

from webkitpy.common.system.executive_mock import MockExecutive
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.port.gtk import GtkPort
from webkitpy.port.pulseaudio_sanitizer_mock import PulseAudioSanitizerMock
from webkitpy.port import port_testcase
from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.mocktool import MockOptions


class GtkPortTest(port_testcase.PortTestCase):
    port_name = 'gtk'
    port_maker = GtkPort

    # Additionally mocks out the PulseAudioSanitizer methods.
    def make_port(self, host=None, port_name=None, options=None, os_name=None, os_version=None, **kwargs):
        port = super(GtkPortTest, self).make_port(host, port_name, options, os_name, os_version, **kwargs)
        port._pulseaudio_sanitizer = PulseAudioSanitizerMock()
        return port

    def test_default_baseline_search_path(self):
        port = self.make_port()
        self.assertEqual(port.default_baseline_search_path(), ['/mock-checkout/LayoutTests/platform/gtk-wk1',
            '/mock-checkout/LayoutTests/platform/gtk'])

        port = self.make_port(options=MockOptions(webkit_test_runner=True))
        self.assertEqual(port.default_baseline_search_path(), ['/mock-checkout/LayoutTests/platform/gtk-wk2',
            '/mock-checkout/LayoutTests/platform/wk2', '/mock-checkout/LayoutTests/platform/gtk'])

    def test_port_specific_expectations_files(self):
        port = self.make_port()
        self.assertEqual(port.expectations_files(), ['/mock-checkout/LayoutTests/TestExpectations',
            '/mock-checkout/LayoutTests/platform/gtk/TestExpectations',
            '/mock-checkout/LayoutTests/platform/gtk-wk1/TestExpectations'])

        port = self.make_port(options=MockOptions(webkit_test_runner=True))
        self.assertEqual(port.expectations_files(), ['/mock-checkout/LayoutTests/TestExpectations',
            '/mock-checkout/LayoutTests/platform/gtk/TestExpectations',
            '/mock-checkout/LayoutTests/platform/wk2/TestExpectations',
            '/mock-checkout/LayoutTests/platform/gtk-wk2/TestExpectations'])

    def test_show_results_html_file(self):
        port = self.make_port()
        port._executive = MockExecutive(should_log=True)
        expected_logs = "MOCK run_command: ['Tools/Scripts/run-launcher', '--release', '--gtk', 'file://test.html'], cwd=/mock-checkout\n"
        OutputCapture().assert_outputs(self, port.show_results_html_file, ["test.html"], expected_logs=expected_logs)

    def test_default_timeout_ms(self):
        self.assertEqual(self.make_port(options=MockOptions(configuration='Release')).default_timeout_ms(), 6000)
        self.assertEqual(self.make_port(options=MockOptions(configuration='Debug')).default_timeout_ms(), 12000)

    def test_get_crash_log(self):
        core_directory = os.environ.get('WEBKIT_CORE_DUMPS_DIRECTORY', '/path/to/coredumps')
        core_pattern = os.path.join(core_directory, "core-pid_%p-_-process_%e")
        mock_empty_crash_log = """\
Crash log for DumpRenderTree (pid 28529):

Coredump core-pid_28529-_-process_DumpRenderTree not found. To enable crash logs:

- run this command as super-user: echo "%(core_pattern)s" > /proc/sys/kernel/core_pattern
- enable core dumps: ulimit -c unlimited
- set the WEBKIT_CORE_DUMPS_DIRECTORY environment variable: export WEBKIT_CORE_DUMPS_DIRECTORY=%(core_directory)s


STDERR: <empty>""" % locals()

        def _mock_gdb_output(coredump_path):
            return (mock_empty_crash_log, [])

        port = self.make_port()
        port._get_gdb_output = mock_empty_crash_log
        stderr, log = port._get_crash_log("DumpRenderTree", 28529, "", "", newer_than=None)
        self.assertEqual(stderr, "")
        self.assertMultiLineEqual(log, mock_empty_crash_log)

        stderr, log = port._get_crash_log("DumpRenderTree", 28529, "", "", newer_than=0.0)
        self.assertEqual(stderr, "")
        self.assertMultiLineEqual(log, mock_empty_crash_log)
