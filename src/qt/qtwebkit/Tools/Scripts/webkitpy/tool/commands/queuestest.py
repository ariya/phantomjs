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

from webkitpy.common.net.bugzilla import Attachment
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.system.executive import ScriptError
from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.commands.stepsequence import StepSequenceErrorHandler
from webkitpy.tool.mocktool import MockTool


class MockQueueEngine(object):
    def __init__(self, name, queue, wakeup_event, seconds_to_sleep):
        pass

    def run(self):
        pass


class QueuesTest(unittest.TestCase):
    # This is _patch1 in mocktool.py
    mock_work_item = MockTool().bugs.fetch_attachment(10000)

    def assert_outputs(self, func, func_name, args, expected_stdout, expected_stderr, expected_exceptions, expected_logs):
        exception = None
        if expected_exceptions and func_name in expected_exceptions:
            exception = expected_exceptions[func_name]

        logs = None
        if expected_logs and func_name in expected_logs:
            logs = expected_logs[func_name]

        OutputCapture().assert_outputs(self,
                func,
                args=args,
                expected_stdout=expected_stdout.get(func_name, ""),
                expected_stderr=expected_stderr.get(func_name, ""),
                expected_exception=exception,
                expected_logs=logs)

    def _default_begin_work_queue_stderr(self, name):
        string_replacements = {"name": name}
        return "MOCK: update_status: %(name)s Starting Queue\n" % string_replacements

    def _default_begin_work_queue_logs(self, name):
        checkout_dir = '/mock-checkout'
        string_replacements = {"name": name, 'checkout_dir': checkout_dir}
        return "CAUTION: %(name)s will discard all local changes in \"%(checkout_dir)s\"\nRunning WebKit %(name)s.\nMOCK: update_status: %(name)s Starting Queue\n" % string_replacements

    def assert_queue_outputs(self, queue, args=None, work_item=None, expected_stdout=None, expected_stderr=None, expected_exceptions=None, expected_logs=None, options=None, tool=None):
        if not tool:
            tool = MockTool()
            # This is a hack to make it easy for callers to not have to setup a custom MockFileSystem just to test the commit-queue
            # the cq tries to read the layout test results, and will hit a KeyError in MockFileSystem if we don't do this.
            tool.filesystem.write_text_file('/mock-results/full_results.json', "")
        if not expected_stdout:
            expected_stdout = {}
        if not expected_stderr:
            expected_stderr = {}
        if not args:
            args = []
        if not options:
            options = Mock()
            options.port = None
        if not work_item:
            work_item = self.mock_work_item
        tool.user.prompt = lambda message: "yes"

        queue.execute(options, args, tool, engine=MockQueueEngine)

        self.assert_outputs(queue.queue_log_path, "queue_log_path", [], expected_stdout, expected_stderr, expected_exceptions, expected_logs)
        self.assert_outputs(queue.work_item_log_path, "work_item_log_path", [work_item], expected_stdout, expected_stderr, expected_exceptions, expected_logs)
        self.assert_outputs(queue.begin_work_queue, "begin_work_queue", [], expected_stdout, expected_stderr, expected_exceptions, expected_logs)
        self.assert_outputs(queue.should_continue_work_queue, "should_continue_work_queue", [], expected_stdout, expected_stderr, expected_exceptions, expected_logs)
        self.assert_outputs(queue.next_work_item, "next_work_item", [], expected_stdout, expected_stderr, expected_exceptions, expected_logs)
        self.assert_outputs(queue.process_work_item, "process_work_item", [work_item], expected_stdout, expected_stderr, expected_exceptions, expected_logs)
        self.assert_outputs(queue.handle_unexpected_error, "handle_unexpected_error", [work_item, "Mock error message"], expected_stdout, expected_stderr, expected_exceptions, expected_logs)
        # Should we have a different function for testing StepSequenceErrorHandlers?
        if isinstance(queue, StepSequenceErrorHandler):
            self.assert_outputs(queue.handle_script_error, "handle_script_error", [tool, {"patch": self.mock_work_item}, ScriptError(message="ScriptError error message", script_args="MockErrorCommand", output="MOCK output")], expected_stdout, expected_stderr, expected_exceptions, expected_logs)
