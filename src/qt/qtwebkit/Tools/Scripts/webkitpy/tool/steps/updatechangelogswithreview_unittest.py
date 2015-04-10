# Copyright (C) 2009 Google Inc. All rights reserved.
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

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.mocktool import MockOptions, MockTool
from webkitpy.tool.steps.updatechangelogswithreviewer import UpdateChangeLogsWithReviewer

class UpdateChangeLogsWithReviewerTest(unittest.TestCase):
    def test_guess_reviewer_from_bug(self):
        capture = OutputCapture()
        step = UpdateChangeLogsWithReviewer(MockTool(), MockOptions())
        expected_logs = "No reviewed patches on bug 50001, cannot infer reviewer.\n"
        capture.assert_outputs(self, step._guess_reviewer_from_bug, [50001], expected_logs=expected_logs)

    def test_guess_reviewer_from_multipatch_bug(self):
        capture = OutputCapture()
        step = UpdateChangeLogsWithReviewer(MockTool(), MockOptions())
        expected_logs = "Guessing \"Reviewer2\" as reviewer from attachment 10001 on bug 50000.\n"
        capture.assert_outputs(self, step._guess_reviewer_from_bug, [50000], expected_logs=expected_logs)

    def test_empty_state(self):
        capture = OutputCapture()
        options = MockOptions()
        options.reviewer = 'MOCK reviewer'
        options.git_commit = 'MOCK git commit'
        step = UpdateChangeLogsWithReviewer(MockTool(), options)
        capture.assert_outputs(self, step.run, [{}])
