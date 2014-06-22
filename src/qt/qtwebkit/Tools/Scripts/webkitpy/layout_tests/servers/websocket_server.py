# Copyright (C) 2011 Google Inc. All rights reserved.
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

"""A class to help start/stop the PyWebSocket server used by layout tests."""

import logging
import os
import sys
import time

from webkitpy.layout_tests.servers import http_server
from webkitpy.layout_tests.servers import http_server_base

_log = logging.getLogger(__name__)


_WS_LOG_PREFIX = 'pywebsocket.ws.log-'
_WSS_LOG_PREFIX = 'pywebsocket.wss.log-'


_DEFAULT_WS_PORT = 8880
_DEFAULT_WSS_PORT = 9323


class PyWebSocket(http_server.Lighttpd):
    def __init__(self, port_obj, output_dir, port=_DEFAULT_WS_PORT,
                 root=None, use_tls=False,
                 private_key=None, certificate=None, ca_certificate=None,
                 pidfile=None):
        """Args:
          output_dir: the absolute path to the layout test result directory
        """
        http_server.Lighttpd.__init__(self, port_obj, output_dir,
                                      port=_DEFAULT_WS_PORT,
                                      root=root)
        self._output_dir = output_dir
        self._pid_file = pidfile
        self._process = None

        self._port = port
        self._root = root
        self._use_tls = use_tls

        self._name = 'pywebsocket'
        if self._use_tls:
            self._name = 'pywebsocket_secure'

        if private_key:
            self._private_key = private_key
        else:
            self._private_key = self._pem_file
        if certificate:
            self._certificate = certificate
        else:
            self._certificate = self._pem_file
        self._ca_certificate = ca_certificate
        if self._port:
            self._port = int(self._port)
        self._wsin = None
        self._wsout = None
        self._mappings = [{'port': self._port}]

        if not self._pid_file:
            self._pid_file = self._filesystem.join(self._runtime_path, '%s.pid' % self._name)

        # Webkit tests
        # FIXME: This is the wrong way to detect if we're in Chrome vs. WebKit!
        # The port objects are supposed to abstract this.
        if self._root:
            self._layout_tests = self._filesystem.abspath(self._root)
            self._web_socket_tests = self._filesystem.abspath(self._filesystem.join(self._root, 'http', 'tests', 'websocket', 'tests'))
        else:
            try:
                self._layout_tests = self._port_obj.layout_tests_dir()
                self._web_socket_tests = self._filesystem.join(self._layout_tests, 'http', 'tests', 'websocket', 'tests')
            except:
                self._web_socket_tests = None

        if self._use_tls:
            self._log_prefix = _WSS_LOG_PREFIX
        else:
            self._log_prefix = _WS_LOG_PREFIX

    def _prepare_config(self):
        time_str = time.strftime('%d%b%Y-%H%M%S')
        log_file_name = self._log_prefix + time_str
        # FIXME: Doesn't Executive have a devnull, so that we don't have to use os.devnull directly?
        self._wsin = open(os.devnull, 'r')

        error_log = self._filesystem.join(self._output_dir, log_file_name + "-err.txt")
        output_log = self._filesystem.join(self._output_dir, log_file_name + "-out.txt")
        self._wsout = self._filesystem.open_text_file_for_writing(output_log)

        from webkitpy.thirdparty import mod_pywebsocket
        python_interp = sys.executable
        # FIXME: Use self._filesystem.path_to_module(self.__module__) instead of __file__
        # I think this is trying to get the chrome directory?  Doesn't the port object know that?
        pywebsocket_base = self._filesystem.join(self._filesystem.dirname(self._filesystem.dirname(self._filesystem.dirname(self._filesystem.abspath(__file__)))), 'thirdparty')
        pywebsocket_script = self._filesystem.join(pywebsocket_base, 'mod_pywebsocket', 'standalone.py')
        start_cmd = [
            python_interp, '-u', pywebsocket_script,
            '--server-host', 'localhost',
            '--port', str(self._port),
            # FIXME: Don't we have a self._port_obj.layout_test_path?
            '--document-root', self._filesystem.join(self._layout_tests, 'http', 'tests'),
            '--scan-dir', self._web_socket_tests,
            '--cgi-paths', '/websocket/tests',
            '--log-file', error_log,
        ]

        handler_map_file = self._filesystem.join(self._web_socket_tests, 'handler_map.txt')
        if self._filesystem.exists(handler_map_file):
            _log.debug('Using handler_map_file: %s' % handler_map_file)
            start_cmd.append('--websock-handlers-map-file')
            start_cmd.append(handler_map_file)
        else:
            _log.warning('No handler_map_file found')

        if self._use_tls:
            start_cmd.extend(['-t', '-k', self._private_key,
                              '-c', self._certificate])
            if self._ca_certificate:
                start_cmd.append('--ca-certificate')
                start_cmd.append(self._ca_certificate)

        self._start_cmd = start_cmd
        server_name = self._filesystem.basename(pywebsocket_script)
        self._env = self._port_obj.setup_environ_for_server(server_name)
        self._env['PYTHONPATH'] = (pywebsocket_base + os.path.pathsep + self._env.get('PYTHONPATH', ''))

    def _remove_stale_logs(self):
        try:
            self._remove_log_files(self._output_dir, self._log_prefix)
        except OSError, e:
            _log.warning('Failed to remove stale %s log files: %s' % (self._name, str(e)))

    def _spawn_process(self):
        _log.debug('Starting %s server, cmd="%s"' % (self._name, self._start_cmd))
        self._process = self._executive.popen(self._start_cmd, env=self._env, shell=False, stdin=self._wsin, stdout=self._wsout, stderr=self._executive.STDOUT)
        self._filesystem.write_text_file(self._pid_file, str(self._process.pid))
        return self._process.pid

    def _stop_running_server(self):
        super(PyWebSocket, self)._stop_running_server()

        if self._wsin:
            self._wsin.close()
            self._wsin = None
        if self._wsout:
            self._wsout.close()
            self._wsout = None
