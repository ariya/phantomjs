# Copyright (C) 2012 Google, Inc.
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
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import logging
import sys
import unittest2 as unittest
import StringIO

from webkitpy.common.system.filesystem import FileSystem
from webkitpy.common.system.executive import Executive
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.test.main import Tester, _Loader


STUBS_CLASS = __name__ + ".TestStubs"


class TestStubs(unittest.TestCase):
    def test_empty(self):
        pass

    def integration_test_empty(self):
        pass

    def serial_test_empty(self):
        pass

    def serial_integration_test_empty(self):
        pass


class TesterTest(unittest.TestCase):

    def test_no_tests_found(self):
        tester = Tester()
        errors = StringIO.StringIO()

        # Here we need to remove any existing log handlers so that they
        # don't log the messages webkitpy.test while we're testing it.
        root_logger = logging.getLogger()
        root_handlers = root_logger.handlers
        root_logger.handlers = []

        tester.printer.stream = errors
        tester.finder.find_names = lambda args, run_all: []
        oc = OutputCapture()
        try:
            oc.capture_output()
            self.assertFalse(tester.run())
        finally:
            _, _, logs = oc.restore_output()
            root_logger.handlers = root_handlers

        self.assertIn('No tests to run', errors.getvalue())
        self.assertIn('No tests to run', logs)

    def _find_test_names(self, args):
        tester = Tester()
        tester._options, args = tester._parse_args(args)
        return tester._test_names(_Loader(), args)

    def test_individual_names_are_not_run_twice(self):
        args = [STUBS_CLASS + '.test_empty']
        parallel_tests, serial_tests = self._find_test_names(args)
        self.assertEqual(parallel_tests, args)
        self.assertEqual(serial_tests, [])

    def test_integration_tests_are_not_found_by_default(self):
        parallel_tests, serial_tests = self._find_test_names([STUBS_CLASS])
        self.assertEqual(parallel_tests, [
            STUBS_CLASS + '.test_empty',
            ])
        self.assertEqual(serial_tests, [
            STUBS_CLASS + '.serial_test_empty',
            ])

    def test_integration_tests_are_found(self):
        parallel_tests, serial_tests = self._find_test_names(['--integration-tests', STUBS_CLASS])
        self.assertEqual(parallel_tests, [
            STUBS_CLASS + '.integration_test_empty',
            STUBS_CLASS + '.test_empty',
            ])
        self.assertEqual(serial_tests, [
            STUBS_CLASS + '.serial_integration_test_empty',
            STUBS_CLASS + '.serial_test_empty',
            ])

    def integration_test_coverage_works(self):
        filesystem = FileSystem()
        executive = Executive()
        module_path = filesystem.path_to_module(self.__module__)
        script_dir = module_path[0:module_path.find('webkitpy') - 1]
        proc = executive.popen([sys.executable, filesystem.join(script_dir, 'test-webkitpy'), '-c', STUBS_CLASS + '.test_empty'],
                               stdout=executive.PIPE, stderr=executive.PIPE)
        out, _ = proc.communicate()
        retcode = proc.returncode
        self.assertEqual(retcode, 0)
        self.assertIn('Cover', out)
