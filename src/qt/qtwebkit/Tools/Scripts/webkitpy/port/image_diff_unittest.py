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

"""Unit testing base class for Port implementations."""

import unittest2 as unittest

from webkitpy.port.server_process_mock import MockServerProcess
from webkitpy.port.image_diff import ImageDiffer


class FakePort(object):
    def __init__(self, server_process_output):
        self._server_process_constructor = lambda port, nm, cmd, env: MockServerProcess(lines=server_process_output)

    def _path_to_image_diff(self):
        return ''

    def setup_environ_for_server(self, nm):
        return None


class TestImageDiffer(unittest.TestCase):
    def test_diff_image_failed(self):
        port = FakePort(['diff: 100% failed\n'])
        image_differ = ImageDiffer(port)
        self.assertEqual(image_differ.diff_image('foo', 'bar', 0.1), ('', 100.0, None))

    def test_diff_image_passed(self):
        port = FakePort(['diff: 0% passed\n'])
        image_differ = ImageDiffer(port)
        self.assertEqual(image_differ.diff_image('foo', 'bar', 0.1), (None, 0, None))
