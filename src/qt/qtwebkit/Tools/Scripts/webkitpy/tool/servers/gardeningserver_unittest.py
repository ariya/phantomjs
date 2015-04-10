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

import json
import sys
import unittest2 as unittest

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.layout_tests.models.test_configuration import *
from webkitpy.port import builders
from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.mocktool import MockTool
from webkitpy.common.system.executive_mock import MockExecutive
from webkitpy.common.host_mock import MockHost
from webkitpy.tool.servers.gardeningserver import *


class TestPortFactory(object):
    # FIXME: Why is this a class method?
    @classmethod
    def create(cls):
        host = MockHost()
        return host.port_factory.get("test-win-xp")

    @classmethod
    def path_to_test_expectations_file(cls):
        return cls.create().path_to_test_expectations_file()


class MockServer(object):
    def __init__(self):
        self.tool = MockTool()
        self.tool.executive = MockExecutive(should_log=True)
        self.tool.filesystem.files[TestPortFactory.path_to_test_expectations_file()] = ""


# The real GardeningHTTPRequestHandler has a constructor that's too hard to
# call in a unit test, so we create a subclass that's easier to constrcut.
class TestGardeningHTTPRequestHandler(GardeningHTTPRequestHandler):
    def __init__(self, server):
        self.server = server
        self.body = None

    def _expectations_updater(self):
        return GardeningExpectationsUpdater(self.server.tool, TestPortFactory.create())

    def read_entity_body(self):
        return self.body if self.body else ''

    def _serve_text(self, text):
        print "== Begin Response =="
        print text
        print "== End Response =="

    def _serve_json(self, json_object):
        print "== Begin JSON Response =="
        print json.dumps(json_object)
        print "== End JSON Response =="


class GardeningServerTest(unittest.TestCase):
    def _post_to_path(self, path, body=None, expected_stderr=None, expected_stdout=None, server=None):
        handler = TestGardeningHTTPRequestHandler(server or MockServer())
        handler.path = path
        handler.body = body
        OutputCapture().assert_outputs(self, handler.do_POST, expected_stderr=expected_stderr, expected_stdout=expected_stdout)

    def disabled_test_rollout(self):
        expected_stderr = "MOCK run_command: ['echo', 'rollout', '--force-clean', '--non-interactive', '2314', 'MOCK rollout reason'], cwd=/mock-checkout\n"
        expected_stdout = "== Begin Response ==\nsuccess\n== End Response ==\n"
        self._post_to_path("/rollout?revision=2314&reason=MOCK+rollout+reason", expected_stderr=expected_stderr, expected_stdout=expected_stdout)

    def disabled_test_rebaselineall(self):
        expected_stderr = "MOCK run_command: ['echo', 'rebaseline-json'], cwd=/mock-checkout, input={\"user-scripts/another-test.html\":{\"%s\": [%s]}}\n"
        expected_stdout = "== Begin Response ==\nsuccess\n== End Response ==\n"
        server = MockServer()

        self.output = ['{"add": [], "delete": []}', '']

        def run_command(args, cwd=None, input=None, **kwargs):
            print >> sys.stderr, "MOCK run_command: %s, cwd=%s, input=%s" % (args, cwd, input)
            return self.output.pop(0)

        server.tool.executive.run_command = run_command
        self._post_to_path("/rebaselineall", body='{"user-scripts/another-test.html":{"MOCK builder": ["txt","png"]}}', expected_stderr=expected_stderr % ('MOCK builder', '"txt","png"'), expected_stdout=expected_stdout, server=server)

        self._post_to_path("/rebaselineall", body='{"user-scripts/another-test.html":{"MOCK builder (Debug)": ["txt","png"]}}', expected_stderr=expected_stderr % ('MOCK builder (Debug)', '"txt","png"'), expected_stdout=expected_stdout)
