# Copyright (C) 2010 Google Inc. All rights reserved.
# Copyright (C) 2010 Gabor Rapcsanyi <rgabor@inf.u-szeged.hu>, University of Szeged
# Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
#     * Neither the Google name nor the names of its
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

"""WebKit implementations of the Port interface."""

import logging
import re
import time

from webkitpy.port import server_process


_log = logging.getLogger(__name__)


class ImageDiffer(object):
    def __init__(self, port):
        self._port = port
        self._tolerance = None
        self._process = None

    def diff_image(self, expected_contents, actual_contents, tolerance):
        if tolerance != self._tolerance:
            self.stop()
        try:
            assert(expected_contents)
            assert(actual_contents)
            assert(tolerance is not None)

            if not self._process:
                self._start(tolerance)
            # Note that although we are handed 'old', 'new', ImageDiff wants 'new', 'old'.
            self._process.write('Content-Length: %d\n%sContent-Length: %d\n%s' % (
                len(actual_contents), actual_contents,
                len(expected_contents), expected_contents))
            return self._read()
        except IOError as exception:
            return (None, 0, "Failed to compute an image diff: %s" % str(exception))

    def _start(self, tolerance):
        command = [self._port._path_to_image_diff(), '--tolerance', str(tolerance)]
        environment = self._port.setup_environ_for_server('ImageDiff')
        self._process = self._port._server_process_constructor(self._port, 'ImageDiff', command, environment)
        self._process.start()
        self._tolerance = tolerance

    def _read(self):
        deadline = time.time() + 2.0
        output = None
        output_image = ""

        while not self._process.timed_out and not self._process.has_crashed():
            output = self._process.read_stdout_line(deadline)
            if self._process.timed_out or self._process.has_crashed() or not output:
                break

            if output.startswith('diff'):  # This is the last line ImageDiff prints.
                break

            if output.startswith('Content-Length'):
                m = re.match('Content-Length: (\d+)', output)
                content_length = int(m.group(1))
                output_image = self._process.read_stdout(deadline, content_length)
                output = self._process.read_stdout_line(deadline)
                break

        stderr = self._process.pop_all_buffered_stderr()
        err_str = ''
        if stderr:
            err_str += "ImageDiff produced stderr output:\n" + stderr
        if self._process.timed_out:
            err_str += "ImageDiff timed out\n"
        if self._process.has_crashed():
            err_str += "ImageDiff crashed\n"

        # FIXME: There is no need to shut down the ImageDiff server after every diff.
        self._process.stop()

        diff_percent = 0
        if output and output.startswith('diff'):
            m = re.match('diff: (.+)% (passed|failed)', output)
            if m.group(2) == 'passed':
                return (None, 0, None)
            diff_percent = float(m.group(1))

        return (output_image, diff_percent, err_str or None)

    def stop(self):
        if self._process:
            self._process.stop()
            self._process = None
