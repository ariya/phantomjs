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

import re
import sys
import unittest2 as unittest

from webkitpy.common.host_mock import MockHost
from webkitpy.port import test
from webkitpy.layout_tests.servers.http_server import Lighttpd
from webkitpy.layout_tests.servers.http_server_base import ServerError


class TestHttpServer(unittest.TestCase):
    def test_start_cmd(self):
        # Fails on win - see https://bugs.webkit.org/show_bug.cgi?id=84726
        if sys.platform in ('cygwin', 'win32'):
            return

        host = MockHost()
        test_port = test.TestPort(host)
        host.filesystem.write_text_file(
            "/mock-checkout/Tools/Scripts/webkitpy/layout_tests/servers/lighttpd.conf", "Mock Config\n")
        host.filesystem.write_text_file(
            "/usr/lib/lighttpd/liblightcomp.dylib", "Mock dylib")

        server = Lighttpd(test_port, "/mock/output_dir",
                          additional_dirs={
                              "/mock/one-additional-dir": "/mock-checkout/one-additional-dir",
                              "/mock/another-additional-dir": "/mock-checkout/one-additional-dir"})
        self.assertRaises(ServerError, server.start)

        config_file = host.filesystem.read_text_file("/mock/output_dir/lighttpd.conf")
        self.assertEqual(re.findall(r"alias.url.+", config_file), [
            'alias.url = ( "/js-test-resources" => "/test.checkout/LayoutTests/fast/js/resources" )',
            'alias.url += ( "/mock/one-additional-dir" => "/mock-checkout/one-additional-dir" )',
            'alias.url += ( "/mock/another-additional-dir" => "/mock-checkout/one-additional-dir" )',
            'alias.url += ( "/media-resources" => "/test.checkout/LayoutTests/media" )',
        ])

    def test_win32_start_and_stop(self):
        host = MockHost()
        test_port = test.TestPort(host)
        host.filesystem.write_text_file(
            "/mock-checkout/Tools/Scripts/webkitpy/layout_tests/servers/lighttpd.conf", "Mock Config\n")
        host.filesystem.write_text_file(
            "/usr/lib/lighttpd/liblightcomp.dylib", "Mock dylib")

        host.platform.is_win = lambda: True
        host.platform.is_cygwin = lambda: False

        server = Lighttpd(test_port, "/mock/output_dir",
                          additional_dirs={
                              "/mock/one-additional-dir": "/mock-checkout/one-additional-dir",
                              "/mock/another-additional-dir": "/mock-checkout/one-additional-dir"})
        server._check_that_all_ports_are_available = lambda: True
        server._is_server_running_on_all_ports = lambda: True

        server.start()
        self.assertNotEquals(host.executive.calls, [])

        def wait_for_action(action):
            if action():
                return True
            return action()

        def mock_returns(return_values):
            def return_value_thunk(*args, **kwargs):
                return return_values.pop(0)
            return return_value_thunk

        host.executive.check_running_pid = mock_returns([True, False])
        server._wait_for_action = wait_for_action

        server.stop()
        self.assertEqual(['taskkill.exe', '/f', '/t', '/pid', 42], host.executive.calls[1])
