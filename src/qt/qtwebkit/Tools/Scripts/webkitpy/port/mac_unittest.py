# Copyright (C) 2010 Google Inc. All rights reserved.
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

from webkitpy.port.mac import MacPort
from webkitpy.port import port_testcase
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.mocktool import MockOptions
from webkitpy.common.system.executive_mock import MockExecutive, MockExecutive2, MockProcess, ScriptError
from webkitpy.common.system.systemhost_mock import MockSystemHost


class MacTest(port_testcase.PortTestCase):
    os_name = 'mac'
    os_version = 'lion'
    port_name = 'mac-lion'
    port_maker = MacPort

    def assert_skipped_file_search_paths(self, port_name, expected_paths, use_webkit2=False):
        port = self.make_port(port_name=port_name, options=MockOptions(webkit_test_runner=use_webkit2))
        self.assertEqual(port._skipped_file_search_paths(), expected_paths)

    def test_default_timeout_ms(self):
        super(MacTest, self).test_default_timeout_ms()
        self.assertEqual(self.make_port(options=MockOptions(guard_malloc=True)).default_timeout_ms(), 350000)


    example_skipped_file = u"""
# <rdar://problem/5647952> fast/events/mouseout-on-window.html needs mac DRT to issue mouse out events
fast/events/mouseout-on-window.html

# <rdar://problem/5643675> window.scrollTo scrolls a window with no scrollbars
fast/events/attempt-scroll-with-no-scrollbars.html

# see bug <rdar://problem/5646437> REGRESSION (r28015): svg/batik/text/smallFonts fails
svg/batik/text/smallFonts.svg

# Java tests don't work on WK2
java/
"""
    example_skipped_tests = [
        "fast/events/mouseout-on-window.html",
        "fast/events/attempt-scroll-with-no-scrollbars.html",
        "svg/batik/text/smallFonts.svg",
        "java",
    ]

    def test_tests_from_skipped_file_contents(self):
        port = self.make_port()
        self.assertEqual(port._tests_from_skipped_file_contents(self.example_skipped_file), self.example_skipped_tests)

    def assert_name(self, port_name, os_version_string, expected):
        host = MockSystemHost(os_name='mac', os_version=os_version_string)
        port = self.make_port(host=host, port_name=port_name)
        self.assertEqual(expected, port.name())

    def test_tests_for_other_platforms(self):
        platforms = ['mac', 'chromium-linux', 'mac-snowleopard']
        port = self.make_port(port_name='mac-snowleopard')
        platform_dir_paths = map(port._webkit_baseline_path, platforms)
        # Replace our empty mock file system with one which has our expected platform directories.
        port._filesystem = MockFileSystem(dirs=platform_dir_paths)

        dirs_to_skip = port._tests_for_other_platforms()
        self.assertIn('platform/chromium-linux', dirs_to_skip)
        self.assertNotIn('platform/mac', dirs_to_skip)
        self.assertNotIn('platform/mac-snowleopard', dirs_to_skip)

    def test_version(self):
        port = self.make_port()
        self.assertTrue(port.version())

    def test_versions(self):
        # Note: these tests don't need to be exhaustive as long as we get path coverage.
        self.assert_name('mac', 'snowleopard', 'mac-snowleopard')
        self.assert_name('mac-snowleopard', 'leopard', 'mac-snowleopard')
        self.assert_name('mac-snowleopard', 'lion', 'mac-snowleopard')

        self.assert_name('mac', 'lion', 'mac-lion')
        self.assert_name('mac-lion', 'lion', 'mac-lion')

        self.assert_name('mac', 'mountainlion', 'mac-mountainlion')
        self.assert_name('mac-mountainlion', 'lion', 'mac-mountainlion')

        self.assert_name('mac', 'future', 'mac-future')
        self.assert_name('mac-future', 'future', 'mac-future')

        self.assertRaises(AssertionError, self.assert_name, 'mac-tiger', 'leopard', 'mac-leopard')

    def test_setup_environ_for_server(self):
        port = self.make_port(options=MockOptions(leaks=True, guard_malloc=True))
        env = port.setup_environ_for_server(port.driver_name())
        self.assertEqual(env['MallocStackLogging'], '1')
        self.assertEqual(env['DYLD_INSERT_LIBRARIES'], '/usr/lib/libgmalloc.dylib:/mock-build/libWebCoreTestShim.dylib')

    def _assert_search_path(self, port_name, baseline_path, search_paths, use_webkit2=False):
        port = self.make_port(port_name=port_name, options=MockOptions(webkit_test_runner=use_webkit2))
        absolute_search_paths = map(port._webkit_baseline_path, search_paths)
        self.assertEqual(port.baseline_path(), port._webkit_baseline_path(baseline_path))
        self.assertEqual(port.baseline_search_path(), absolute_search_paths)

    def test_baseline_search_path(self):
        # Note that we don't need total coverage here, just path coverage, since this is all data driven.
        self._assert_search_path('mac-snowleopard', 'mac-snowleopard', ['mac-snowleopard', 'mac-lion', 'mac'])
        self._assert_search_path('mac-lion', 'mac-lion', ['mac-lion', 'mac'])
        self._assert_search_path('mac-mountainlion', 'mac', ['mac'])
        self._assert_search_path('mac-future', 'mac', ['mac'])
        self._assert_search_path('mac-snowleopard', 'mac-wk2', ['mac-wk2', 'wk2', 'mac-snowleopard', 'mac-lion', 'mac'], use_webkit2=True)
        self._assert_search_path('mac-lion', 'mac-wk2', ['mac-wk2', 'wk2', 'mac-lion', 'mac'], use_webkit2=True)
        self._assert_search_path('mac-mountainlion', 'mac-wk2', ['mac-wk2', 'wk2', 'mac'], use_webkit2=True)
        self._assert_search_path('mac-future', 'mac-wk2', ['mac-wk2', 'wk2', 'mac'], use_webkit2=True)

    def test_show_results_html_file(self):
        port = self.make_port()
        # Delay setting a should_log executive to avoid logging from MacPort.__init__.
        port._executive = MockExecutive(should_log=True)
        expected_logs = "MOCK popen: ['Tools/Scripts/run-safari', '--release', '--no-saved-state', '-NSOpen', 'test.html'], cwd=/mock-checkout\n"
        OutputCapture().assert_outputs(self, port.show_results_html_file, ["test.html"], expected_logs=expected_logs)

    def test_operating_system(self):
        self.assertEqual('mac', self.make_port().operating_system())

    def test_default_child_processes(self):
        port = self.make_port(port_name='mac-lion')
        # MockPlatformInfo only has 2 mock cores.  The important part is that 2 > 1.
        self.assertEqual(port.default_child_processes(), 2)

        bytes_for_drt = 200 * 1024 * 1024
        port.host.platform.total_bytes_memory = lambda: bytes_for_drt
        expected_logs = "This machine could support 2 child processes, but only has enough memory for 1.\n"
        child_processes = OutputCapture().assert_outputs(self, port.default_child_processes, (), expected_logs=expected_logs)
        self.assertEqual(child_processes, 1)

        # Make sure that we always use one process, even if we don't have the memory for it.
        port.host.platform.total_bytes_memory = lambda: bytes_for_drt - 1
        expected_logs = "This machine could support 2 child processes, but only has enough memory for 1.\n"
        child_processes = OutputCapture().assert_outputs(self, port.default_child_processes, (), expected_logs=expected_logs)
        self.assertEqual(child_processes, 1)

        # SnowLeopard has a CFNetwork bug which causes crashes if we execute more than one copy of DRT at once.
        port = self.make_port(port_name='mac-snowleopard')
        expected_logs = "Cannot run tests in parallel on Snow Leopard due to rdar://problem/10621525.\n"
        child_processes = OutputCapture().assert_outputs(self, port.default_child_processes, (), expected_logs=expected_logs)
        self.assertEqual(child_processes, 1)

    def test_get_crash_log(self):
        # Mac crash logs are tested elsewhere, so here we just make sure we don't crash.
        def fake_time_cb():
            times = [0, 20, 40]
            return lambda: times.pop(0)
        port = self.make_port(port_name='mac-snowleopard')
        port._get_crash_log('DumpRenderTree', 1234, '', '', 0,
            time_fn=fake_time_cb(), sleep_fn=lambda delay: None)

    def test_helper_starts(self):
        host = MockSystemHost(MockExecutive())
        port = self.make_port(host)
        oc = OutputCapture()
        oc.capture_output()
        host.executive._proc = MockProcess('ready\n')
        port.start_helper()
        port.stop_helper()
        oc.restore_output()

        # make sure trying to stop the helper twice is safe.
        port.stop_helper()

    def test_helper_fails_to_start(self):
        host = MockSystemHost(MockExecutive())
        port = self.make_port(host)
        oc = OutputCapture()
        oc.capture_output()
        port.start_helper()
        port.stop_helper()
        oc.restore_output()

    def test_helper_fails_to_stop(self):
        host = MockSystemHost(MockExecutive())
        host.executive._proc = MockProcess()

        def bad_waiter():
            raise IOError('failed to wait')
        host.executive._proc.wait = bad_waiter

        port = self.make_port(host)
        oc = OutputCapture()
        oc.capture_output()
        port.start_helper()
        port.stop_helper()
        oc.restore_output()

    def test_sample_process(self):

        def logging_run_command(args):
            print args

        port = self.make_port()
        port._executive = MockExecutive2(run_command_fn=logging_run_command)
        expected_stdout = "['/usr/bin/sample', 42, 10, 10, '-file', '/mock-build/layout-test-results/test-42-sample.txt']\n"
        OutputCapture().assert_outputs(self, port.sample_process, args=['test', 42], expected_stdout=expected_stdout)

    def test_sample_process_throws_exception(self):

        def throwing_run_command(args):
            raise ScriptError("MOCK script error")

        port = self.make_port()
        port._executive = MockExecutive2(run_command_fn=throwing_run_command)
        OutputCapture().assert_outputs(self, port.sample_process, args=['test', 42])

    def test_32bit(self):
        port = self.make_port(options=MockOptions(architecture='x86'))

        def run_script(script, args=None, env=None):
            self.args = args

        port._run_script = run_script
        self.assertEqual(port.architecture(), 'x86')
        port._build_driver()
        self.assertEqual(self.args, ['ARCHS=i386'])

    def test_64bit(self):
        # Apple Mac port is 64-bit by default
        port = self.make_port()
        self.assertEqual(port.architecture(), 'x86_64')

        def run_script(script, args=None, env=None):
            self.args = args

        port._run_script = run_script
        port._build_driver()
        self.assertEqual(self.args, [])
