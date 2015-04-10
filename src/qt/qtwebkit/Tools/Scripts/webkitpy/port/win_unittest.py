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

import StringIO
import unittest2 as unittest

from webkitpy.common.system.executive import ScriptError
from webkitpy.common.system.executive_mock import MockExecutive, MockExecutive2
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.system.systemhost_mock import MockSystemHost
from webkitpy.port import port_testcase
from webkitpy.port.win import WinPort
from webkitpy.tool.mocktool import MockOptions


class WinPortTest(port_testcase.PortTestCase):
    os_name = 'win'
    os_version = 'xp'
    port_name = 'win-xp'
    port_maker = WinPort

    def test_show_results_html_file(self):
        port = self.make_port()
        port._executive = MockExecutive(should_log=True)
        capture = OutputCapture()
        capture.capture_output()
        port.show_results_html_file('test.html')
        _, _, logs = capture.restore_output()
        # We can't know for sure what path will be produced by cygpath, but we can assert about
        # everything else.
        self.assertTrue(logs.startswith("MOCK run_command: ['Tools/Scripts/run-safari', '--release', '"))
        self.assertTrue(logs.endswith("test.html'], cwd=/mock-checkout\n"))

    def _assert_search_path(self, expected_search_paths, version, use_webkit2=False):
        port = self.make_port(port_name='win', os_version=version, options=MockOptions(webkit_test_runner=use_webkit2))
        absolute_search_paths = map(port._webkit_baseline_path, expected_search_paths)
        self.assertEqual(port.baseline_search_path(), absolute_search_paths)

    def test_baseline_search_path(self):
        self._assert_search_path(['win-xp', 'win-vista', 'win-7sp0', 'win', 'mac-lion', 'mac'], 'xp')
        self._assert_search_path(['win-vista', 'win-7sp0', 'win', 'mac-lion', 'mac'], 'vista')
        self._assert_search_path(['win-7sp0', 'win', 'mac-lion', 'mac'], '7sp0')

        self._assert_search_path(['win-wk2', 'win-xp', 'win-vista', 'win-7sp0', 'win', 'mac-wk2', 'mac-lion', 'mac'], 'xp', use_webkit2=True)
        self._assert_search_path(['win-wk2', 'win-vista', 'win-7sp0', 'win', 'mac-wk2', 'mac-lion', 'mac'], 'vista', use_webkit2=True)
        self._assert_search_path(['win-wk2', 'win-7sp0', 'win', 'mac-wk2', 'mac-lion', 'mac'], '7sp0', use_webkit2=True)

    def _assert_version(self, port_name, expected_version):
        host = MockSystemHost(os_name='win', os_version=expected_version)
        port = WinPort(host, port_name=port_name)
        self.assertEqual(port.version(), expected_version)

    def test_versions(self):
        self._assert_version('win-xp', 'xp')
        self._assert_version('win-vista', 'vista')
        self._assert_version('win-7sp0', '7sp0')
        self.assertRaises(AssertionError, self._assert_version, 'win-me', 'xp')

    def test_compare_text(self):
        expected = "EDITING DELEGATE: webViewDidChangeSelection:WebViewDidChangeSelectionNotification\nfoo\nEDITING DELEGATE: webViewDidChangeSelection:WebViewDidChangeSelectionNotification\n"
        port = self.make_port()
        self.assertFalse(port.do_text_results_differ(expected, "foo\n"))
        self.assertTrue(port.do_text_results_differ(expected, "foo"))
        self.assertTrue(port.do_text_results_differ(expected, "bar"))

        # This hack doesn't exist in WK2.
        port._options = MockOptions(webkit_test_runner=True)
        self.assertTrue(port.do_text_results_differ(expected, "foo\n"))

    def test_operating_system(self):
        self.assertEqual('win', self.make_port().operating_system())

    def test_runtime_feature_list(self):
        port = self.make_port()
        port._executive.run_command = lambda command, cwd=None, error_handler=None: "Nonsense"
        # runtime_features_list returns None when its results are meaningless (it couldn't run DRT or parse the output, etc.)
        self.assertEqual(port._runtime_feature_list(), None)
        port._executive.run_command = lambda command, cwd=None, error_handler=None: "SupportedFeatures:foo bar"
        self.assertEqual(port._runtime_feature_list(), ['foo', 'bar'])

    def test_expectations_files(self):
        self.assertEqual(len(self.make_port().expectations_files()), 3)
        self.assertEqual(len(self.make_port(options=MockOptions(webkit_test_runner=True, configuration='Release')).expectations_files()), 5)

    def test_get_crash_log(self):
        # Win crash logs are tested elsewhere, so here we just make sure we don't crash.
        def fake_time_cb():
            times = [0, 20, 40]
            return lambda: times.pop(0)
        port = self.make_port(port_name='win')
        port._get_crash_log('DumpRenderTree', 1234, '', '', 0,
            time_fn=fake_time_cb(), sleep_fn=lambda delay: None)
