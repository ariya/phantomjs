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

"""Integration tests for the new-run-webkit-httpd and new-run-webkit-websocketserver scripts"""

# FIXME: Rename this file to something more descriptive.

import errno
import os
import socket
import subprocess
import sys
import tempfile
import unittest2 as unittest


class BaseTest(unittest.TestCase):
    """Basic framework for script tests."""
    HOST = 'localhost'

    # Override in actual test classes.
    PORTS = None
    SCRIPT_NAME = None

    def assert_servers_are_down(self, ports=None):
        ports = ports or self.PORTS
        for port in ports:
            try:
                test_socket = socket.socket()
                test_socket.connect((self.HOST, port))
                self.fail()
            except IOError, e:
                self.assertTrue(e.errno in (errno.ECONNREFUSED, errno.ECONNRESET))
            finally:
                test_socket.close()

    def assert_servers_are_up(self, ports=None):
        ports = ports or self.PORTS
        for port in ports:
            try:
                test_socket = socket.socket()
                test_socket.connect((self.HOST, port))
            except IOError, e:
                self.fail('failed to connect to %s:%d' % (self.HOST, port))
            finally:
                test_socket.close()

    def run_script(self, args):
        script_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
        script_path = os.path.join(script_dir, self.SCRIPT_NAME)
        return subprocess.call([sys.executable, script_path] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def integration_test_server__normal(self):
        if not self.SCRIPT_NAME:
            return

        self.assert_servers_are_down()
        self.assertEqual(self.run_script(['--server', 'start']), 0)
        self.assert_servers_are_up()
        self.assertEqual(self.run_script(['--server', 'stop']), 0)
        self.assert_servers_are_down()

    def integration_test_server__fails(self):
        if not self.SCRIPT_NAME:
            return

        # Test that if a port isn't available, the call fails.
        for port_number in self.PORTS:
            test_socket = socket.socket()
            try:
                try:
                    test_socket.bind((self.HOST, port_number))
                except socket.error, e:
                    if e.errno in (errno.EADDRINUSE, errno.EALREADY):
                        self.fail('could not bind to port %d: %s' % (port_number, str(e)))
                    raise
                self.assertEqual(self.run_script(['--server', 'start']), 1)
            finally:
                self.run_script(['--server', 'stop'])
                test_socket.close()

        # Test that calling stop() twice is harmless.
        self.assertEqual(self.run_script(['--server', 'stop']), 0)

    def maybe_make_dir(self, *comps):
        try:
            os.makedirs(os.path.join(*comps))
        except OSError, e:
            if e.errno != errno.EEXIST:
                raise

    def integration_test_port_and_root(self):
        if not self.SCRIPT_NAME:
            return

        tmpdir = tempfile.mkdtemp(prefix='webkitpytest')
        self.maybe_make_dir(tmpdir, 'http', 'tests', 'websocket')
        self.maybe_make_dir(tmpdir, 'fast', 'js', 'resources')
        self.maybe_make_dir(tmpdir, 'media')

        self.assert_servers_are_down([18000])
        self.assertEqual(self.run_script(['--server', 'start', '--port=18000', '--root', tmpdir]), 0)
        self.assert_servers_are_up([18000])
        self.assertEqual(self.run_script(['--server', 'stop']), 0)
        self.assert_servers_are_down([18000])


class HTTPServerTest(BaseTest):
    """Tests that new-run-webkit-http must pass."""

    PORTS = (8000, 8080, 8443)
    SCRIPT_NAME = 'new-run-webkit-httpd'


class WebsocketserverTest(BaseTest):
    """Tests that new-run-webkit-websocketserver must pass."""

    # FIXME: test TLS at some point?
    PORTS = (8880, )
    SCRIPT_NAME = 'new-run-webkit-websocketserver'
