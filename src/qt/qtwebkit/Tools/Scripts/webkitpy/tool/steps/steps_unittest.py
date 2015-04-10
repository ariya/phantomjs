# Copyright (C) 2010 Google Inc. All rights reserved.
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

import unittest2 as unittest

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.config.ports import DeprecatedPort
from webkitpy.tool.mocktool import MockOptions, MockTool

from webkitpy.tool import steps

class StepsTest(unittest.TestCase):
    def _step_options(self):
        options = MockOptions()
        options.non_interactive = True
        options.port = 'MOCK port'
        options.quiet = True
        options.test = True
        return options

    def _run_step(self, step, tool=None, options=None, state=None):
        if not tool:
            tool = MockTool()
        if not options:
            options = self._step_options()
        if not state:
            state = {}
        step(tool, options).run(state)

    def test_update_step(self):
        tool = MockTool()
        options = self._step_options()
        options.update = True
        expected_logs = "Updating working directory\n"
        OutputCapture().assert_outputs(self, self._run_step, [steps.Update, tool, options], expected_logs=expected_logs)

    def test_prompt_for_bug_or_title_step(self):
        tool = MockTool()
        tool.user.prompt = lambda message: 50000
        self._run_step(steps.PromptForBugOrTitle, tool=tool)

    def _post_diff_options(self):
        options = self._step_options()
        options.git_commit = None
        options.description = None
        options.comment = None
        options.review = True
        options.request_commit = False
        options.open_bug = True
        return options

    def _assert_step_output_with_bug(self, step, bug_id, expected_logs, options=None):
        state = {'bug_id': bug_id}
        OutputCapture().assert_outputs(self, self._run_step, [step, MockTool(), options, state], expected_logs=expected_logs)

    def _assert_post_diff_output_for_bug(self, step, bug_id, expected_logs):
        self._assert_step_output_with_bug(step, bug_id, expected_logs, self._post_diff_options())

    def test_post_diff(self):
        expected_logs = "MOCK add_patch_to_bug: bug_id=78, description=Patch, mark_for_review=True, mark_for_commit_queue=False, mark_for_landing=False\nMOCK: user.open_url: http://example.com/78\n"
        self._assert_post_diff_output_for_bug(steps.PostDiff, 78, expected_logs)

    def test_post_diff_for_commit(self):
        expected_logs = "MOCK add_patch_to_bug: bug_id=78, description=Patch for landing, mark_for_review=False, mark_for_commit_queue=False, mark_for_landing=True\n"
        self._assert_post_diff_output_for_bug(steps.PostDiffForCommit, 78, expected_logs)

    def test_ensure_bug_is_open_and_assigned(self):
        expected_logs = "MOCK reopen_bug 50004 with comment 'Reopening to attach new patch.'\n"
        self._assert_step_output_with_bug(steps.EnsureBugIsOpenAndAssigned, 50004, expected_logs)
        expected_logs = "MOCK reassign_bug: bug_id=50002, assignee=None\n"
        self._assert_step_output_with_bug(steps.EnsureBugIsOpenAndAssigned, 50002, expected_logs)

    def test_runtests_args(self):
        mock_options = self._step_options()
        mock_options.non_interactive = False
        step = steps.RunTests(MockTool(log_executive=True), mock_options)
        tool = MockTool(log_executive=True)
        # FIXME: We shouldn't use a real port-object here, but there is too much to mock at the moment.
        tool._deprecated_port = DeprecatedPort()
        step = steps.RunTests(tool, mock_options)
        expected_logs = """Running Python unit tests
MOCK run_and_throw_if_fail: ['Tools/Scripts/test-webkitpy'], cwd=/mock-checkout
Running Perl unit tests
MOCK run_and_throw_if_fail: ['Tools/Scripts/test-webkitperl'], cwd=/mock-checkout
Running JavaScriptCore tests
MOCK run_and_throw_if_fail: ['Tools/Scripts/run-javascriptcore-tests'], cwd=/mock-checkout
Running bindings generation tests
MOCK run_and_throw_if_fail: ['Tools/Scripts/run-bindings-tests'], cwd=/mock-checkout
Running run-webkit-tests
MOCK run_and_throw_if_fail: ['Tools/Scripts/run-webkit-tests', '--quiet'], cwd=/mock-checkout
"""
        OutputCapture().assert_outputs(self, step.run, [{}], expected_logs=expected_logs)
