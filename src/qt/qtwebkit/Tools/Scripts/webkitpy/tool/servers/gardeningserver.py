# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
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

import BaseHTTPServer
import SocketServer
import logging
import json
import os
import sys
import urllib

from webkitpy.common.memoized import memoized
from webkitpy.tool.servers.reflectionhandler import ReflectionHandler
from webkitpy.port import builders


_log = logging.getLogger(__name__)


class GardeningHTTPServer(SocketServer.ThreadingMixIn, BaseHTTPServer.HTTPServer):
    def __init__(self, httpd_port, config):
        server_name = ''
        self.tool = config['tool']
        self.options = config['options']
        BaseHTTPServer.HTTPServer.__init__(self, (server_name, httpd_port), GardeningHTTPRequestHandler)

    def url(self, args=None):
        # We can't use urllib.encode() here because that encodes spaces as plus signs and the buildbots don't decode those properly.
        arg_string = ('?' + '&'.join("%s=%s" % (key, urllib.quote(value)) for (key, value) in args.items())) if args else ''
        return 'http://localhost:8127/garden-o-matic.html' + arg_string


class GardeningHTTPRequestHandler(ReflectionHandler):
    STATIC_FILE_NAMES = frozenset()

    STATIC_FILE_EXTENSIONS = ('.js', '.css', '.html', '.gif', '.png', '.ico')

    STATIC_FILE_DIRECTORY = os.path.join(
        os.path.dirname(__file__),
        '..',
        '..',
        '..',
        '..',
        'BuildSlaveSupport',
        'build.webkit.org-config',
        'public_html',
        'TestFailures')

    allow_cross_origin_requests = True
    debug_output = ''

    def ping(self):
        self._serve_text('pong')

    def _run_webkit_patch(self, command, input_string):
        PIPE = self.server.tool.executive.PIPE
        process = self.server.tool.executive.popen([self.server.tool.path()] + command, cwd=self.server.tool.scm().checkout_root, stdin=PIPE, stdout=PIPE, stderr=PIPE)
        process.stdin.write(input_string)
        output, error = process.communicate()
        return (process.returncode, output, error)

    def rebaselineall(self):
        command = ['rebaseline-json']
        if self.server.options.move_overwritten_baselines:
            command.append('--move-overwritten-baselines')
        if self.server.options.results_directory:
            command.extend(['--results-directory', self.server.options.results_directory])
        if not self.server.options.optimize:
            command.append('--no-optimize')
        if self.server.options.verbose:
            command.append('--verbose')
        json_input = self.read_entity_body()

        _log.debug("calling %s, input='%s'", command, json_input)
        return_code, output, error = self._run_webkit_patch(command, json_input)
        print >> sys.stderr, error
        if return_code:
            _log.error("rebaseline-json failed: %d, output='%s'" % (return_code, output))
        else:
            _log.debug("rebaseline-json succeeded")

        # FIXME: propagate error and/or log messages back to the UI.
        self._serve_text('success')

    def localresult(self):
        path = self.query['path'][0]
        filesystem = self.server.tool.filesystem

        # Ensure that we're only serving files from inside the results directory.
        if not filesystem.isabs(path) and self.server.options.results_directory:
            fullpath = filesystem.abspath(filesystem.join(self.server.options.results_directory, path))
            if fullpath.startswith(filesystem.abspath(self.server.options.results_directory)):
                self._serve_file(fullpath, headers_only=(self.command == 'HEAD'))
                return

        self.send_response(403)
