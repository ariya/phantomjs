# Copyright (C) 2012 Balazs Ankes (bank@inf.u-szeged.hu) University of Szeged
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
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

import unittest2 as unittest

from webkitpy.tool.steps.addsvnmimetypeforpng import AddSvnMimetypeForPng
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.tool.mocktool import MockOptions, MockTool
from webkitpy.common.system.systemhost_mock import MockSystemHost
from webkitpy.common.system.outputcapture import OutputCapture


class MockSCMDetector(object):

    def __init__(self, scm):
        self._scm = scm

    def display_name(self):
        return self._scm


class AddSvnMimetypeForPngTest(unittest.TestCase):
    def test_run(self):
        capture = OutputCapture()
        options = MockOptions(git_commit='MOCK git commit')

        files = {'/Users/mock/.subversion/config': 'enable-auto-props = yes\n*.png = svn:mime-type=image/png'}
        fs = MockFileSystem(files)
        scm = MockSCMDetector('git')

        step = AddSvnMimetypeForPng(MockTool(), options, MockSystemHost(os_name='linux', filesystem=fs), scm)
        state = {
            "changed_files": ["test.png", "test.txt"],
        }
        try:
            capture.assert_outputs(self, step.run, [state])
        except SystemExit, e:
            self.assertEqual(e.code, 1)
