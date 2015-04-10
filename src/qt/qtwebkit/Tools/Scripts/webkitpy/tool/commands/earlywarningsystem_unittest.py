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

from webkitpy.thirdparty.mock import Mock
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.bot.queueengine import QueueEngine
from webkitpy.tool.commands.earlywarningsystem import *
from webkitpy.tool.commands.queuestest import QueuesTest
from webkitpy.tool.mocktool import MockTool, MockOptions


class AbstractEarlyWarningSystemTest(QueuesTest):
    def test_failing_tests_message(self):
        # Needed to define port_name, used in AbstractEarlyWarningSystem.__init__
        class TestEWS(AbstractEarlyWarningSystem):
            port_name = "win"  # Needs to be a port which port/factory understands.

        ews = TestEWS()
        ews.bind_to_tool(MockTool())
        ews._options = MockOptions(port=None, confirm=False)
        OutputCapture().assert_outputs(self, ews.begin_work_queue, expected_logs=self._default_begin_work_queue_logs(ews.name))
        ews._expected_failures.unexpected_failures_observed = lambda results: set(["foo.html", "bar.html"])
        task = Mock()
        patch = ews._tool.bugs.fetch_attachment(10000)
        self.assertMultiLineEqual(ews._failing_tests_message(task, patch), "New failing tests:\nbar.html\nfoo.html")


class EarlyWarningSystemTest(QueuesTest):
    def _default_expected_logs(self, ews):
        string_replacements = {
            "name": ews.name,
            "port": ews.port_name,
        }
        if ews.run_tests:
            run_tests_line = "Running: webkit-patch --status-host=example.com build-and-test --no-clean --no-update --test --non-interactive --port=%(port)s\n" % string_replacements
        else:
            run_tests_line = ""
        string_replacements['run_tests_line'] = run_tests_line

        expected_logs = {
            "begin_work_queue": self._default_begin_work_queue_logs(ews.name),
            "process_work_item": """Running: webkit-patch --status-host=example.com clean --port=%(port)s
Running: webkit-patch --status-host=example.com update --port=%(port)s
Running: webkit-patch --status-host=example.com apply-attachment --no-update --non-interactive 10000 --port=%(port)s
Running: webkit-patch --status-host=example.com build --no-clean --no-update --build-style=release --port=%(port)s
%(run_tests_line)sMOCK: update_status: %(name)s Pass
MOCK: release_work_item: %(name)s 10000
""" % string_replacements,
            "handle_unexpected_error": "Mock error message\n",
            "handle_script_error": "ScriptError error message\n\nMOCK output\n",
        }
        return expected_logs

    def _test_ews(self, ews):
        ews.bind_to_tool(MockTool())
        options = Mock()
        options.port = None
        options.run_tests = ews.run_tests
        self.assert_queue_outputs(ews, expected_logs=self._default_expected_logs(ews), options=options)

    def test_ewses(self):
        classes = AbstractEarlyWarningSystem.load_ews_classes()
        self.assertTrue(classes)
        self.maxDiff = None
        for ews_class in classes:
            self._test_ews(ews_class())
