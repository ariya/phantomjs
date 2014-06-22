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

import unittest2 as unittest
import os
from copy import deepcopy

from webkitpy.common.system.executive_mock import MockExecutive, MockExecutive2
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.system.systemhost_mock import MockSystemHost
from webkitpy.port import port_testcase
from webkitpy.port.qt import QtPort
from webkitpy.tool.mocktool import MockOptions


class QtPortTest(port_testcase.PortTestCase):
    port_name = 'qt-mac'
    port_maker = QtPort
    search_paths_cases = [
        {'search_paths':['qt-mac-wk2', 'qt-wk2', 'qt-5.0', 'qt-mac', 'qt'], 'os_name':'mac', 'use_webkit2':True, 'qt_version':'5.0'},
        {'search_paths':['qt-wk2', 'qt-5.0', 'qt-win', 'qt'], 'os_name':'win', 'use_webkit2':True, 'qt_version':'5.0'},
        {'search_paths':['qt-wk2', 'qt-5.0', 'qt-linux', 'qt'], 'os_name':'linux', 'use_webkit2':True, 'qt_version':'5.0'},

        {'search_paths':['qt-wk1', 'qt-5.0', 'qt-mac', 'qt'], 'os_name':'mac', 'use_webkit2':False, 'qt_version':'5.0'},
        {'search_paths':['qt-wk1', 'qt-5.0', 'qt-win', 'qt'], 'os_name':'win', 'use_webkit2':False, 'qt_version':'5.0'},
        {'search_paths':['qt-wk1', 'qt-5.0', 'qt-linux', 'qt'], 'os_name':'linux', 'use_webkit2':False, 'qt_version':'5.0'},
        {'search_paths':['qt-wk1', 'qt-5.1', 'qt-linux', 'qt'], 'os_name':'linux', 'use_webkit2':False, 'qt_version':'5.1'},
    ]

    def _assert_search_path(self, search_paths, os_name, use_webkit2=False, qt_version='5.0'):
        # FIXME: Port constructors should not "parse" the port name, but
        # rather be passed components (directly or via setters).  Once
        # we fix that, this method will need a re-write.
        host = MockSystemHost(os_name=os_name)
        host.executive = MockExecutive2(self._qt_version(qt_version))
        port_name = 'qt-' + os_name
        port = self.make_port(host=host, qt_version=qt_version, port_name=port_name,
                              options=MockOptions(webkit_test_runner=use_webkit2, platform='qt'))
        absolute_search_paths = map(port._webkit_baseline_path, search_paths)
        self.assertEqual(port.baseline_search_path(), absolute_search_paths)

    def _assert_expectations_files(self, search_paths, os_name, use_webkit2=False, qt_version='5.0'):
        # FIXME: Port constructors should not "parse" the port name, but
        # rather be passed components (directly or via setters).  Once
        # we fix that, this method will need a re-write.
        host = MockSystemHost(os_name=os_name)
        host.executive = MockExecutive2(self._qt_version(qt_version))
        port_name = 'qt-' + os_name
        port = self.make_port(host=host, qt_version=qt_version, port_name=port_name,
                              options=MockOptions(webkit_test_runner=use_webkit2, platform='qt'))
        self.assertEqual(port.expectations_files(), search_paths)

    def _qt_version(self, qt_version):
        if qt_version in '5.0':
            return 'QMake version 2.01a\nUsing Qt version 5.0.0 in /usr/local/Trolltech/Qt-5.0.0/lib'
        if qt_version in '5.1':
            return 'QMake version 3.0\nUsing Qt version 5.1.1 in /usr/local/Qt-5.1/lib'

    def test_baseline_search_path(self):
        for case in self.search_paths_cases:
            self._assert_search_path(**case)

    def test_expectations_files(self):
        for case in self.search_paths_cases:
            expectations_case = deepcopy(case)
            if expectations_case['use_webkit2']:
                expectations_case['search_paths'].append("wk2")
            expectations_case['search_paths'].append('')
            expectations_case['search_paths'].reverse()
            expectations_case['search_paths'] = map(lambda path: '/mock-checkout/LayoutTests/TestExpectations' if not path else '/mock-checkout/LayoutTests/platform/%s/TestExpectations' % (path), expectations_case['search_paths'])
            self._assert_expectations_files(**expectations_case)

    def test_show_results_html_file(self):
        port = self.make_port()
        port._executive = MockExecutive(should_log=True)
        expected_logs = "MOCK run_command: ['Tools/Scripts/run-launcher', '--release', '--qt', 'file://test.html'], cwd=/mock-checkout\n"
        OutputCapture().assert_outputs(self, port.show_results_html_file, ["test.html"], expected_logs=expected_logs)

    def test_setup_environ_for_server(self):
        port = self.make_port()
        env = port.setup_environ_for_server(port.driver_name())
        self.assertEqual(env['QTWEBKIT_PLUGIN_PATH'], '/mock-build/lib/plugins')

    def test_operating_system(self):
        self.assertEqual('linux', self.make_port(port_name='qt-linux', os_name='linux').operating_system())
        self.assertEqual('mac', self.make_port(os_name='mac').operating_system())
        self.assertEqual('win', self.make_port(port_name='qt-win', os_name='win').operating_system())

    def test_check_sys_deps(self):
        port = self.make_port()

        # Success
        os.environ['WEBKIT_TESTFONTS'] = '/tmp/foo'
        port._executive = MockExecutive2(exit_code=0)
        self.assertTrue(port.check_sys_deps(needs_http=False))

        # Failure
        del os.environ['WEBKIT_TESTFONTS']
        port._executive = MockExecutive2(exit_code=1,
            output='testing output failure')
        self.assertFalse(port.check_sys_deps(needs_http=False))
