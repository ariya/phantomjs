# Copyright (C) 2012 Google, Inc.
# Copyright (C) 2010 Chris Jerdonek (cjerdonek@webkit.org)
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

"""unit testing code for webkitpy."""

import logging
import multiprocessing
import optparse
import os
import StringIO
import sys
import time
import traceback
import unittest

from webkitpy.common.system.filesystem import FileSystem
from webkitpy.test.finder import Finder
from webkitpy.test.printer import Printer
from webkitpy.test.runner import Runner, unit_test_name

_log = logging.getLogger(__name__)


def main():
    up = os.path.dirname
    webkit_root = up(up(up(up(up(os.path.abspath(__file__))))))

    tester = Tester()
    tester.add_tree(os.path.join(webkit_root, 'Tools', 'Scripts'), 'webkitpy')
    tester.add_tree(os.path.join(webkit_root, 'Source', 'WebKit2', 'Scripts'), 'webkit2')

    tester.skip(('webkitpy.common.checkout.scm.scm_unittest',), 'are really, really, slow', 31818)
    if sys.platform == 'win32':
        tester.skip(('webkitpy.common.checkout', 'webkitpy.common.config', 'webkitpy.tool'), 'fail horribly on win32', 54526)

    # This only needs to run on Unix, so don't worry about win32 for now.
    appengine_sdk_path = '/usr/local/google_appengine'
    if os.path.exists(appengine_sdk_path):
        if not appengine_sdk_path in sys.path:
            sys.path.append(appengine_sdk_path)
        import dev_appserver
        from google.appengine.dist import use_library
        use_library('django', '1.2')
        dev_appserver.fix_sys_path()
        tester.add_tree(os.path.join(webkit_root, 'Tools', 'QueueStatusServer'))
    else:
        _log.info('Skipping QueueStatusServer tests; the Google AppEngine Python SDK is not installed.')

    return not tester.run()


class Tester(object):
    def __init__(self, filesystem=None):
        self.finder = Finder(filesystem or FileSystem())
        self.printer = Printer(sys.stderr)
        self._options = None

    def add_tree(self, top_directory, starting_subdirectory=None):
        self.finder.add_tree(top_directory, starting_subdirectory)

    def skip(self, names, reason, bugid):
        self.finder.skip(names, reason, bugid)

    def _parse_args(self, argv=None):
        parser = optparse.OptionParser(usage='usage: %prog [options] [args...]')
        parser.add_option('-a', '--all', action='store_true', default=False,
                          help='run all the tests')
        parser.add_option('-c', '--coverage', action='store_true', default=False,
                          help='generate code coverage info (requires http://pypi.python.org/pypi/coverage)')
        parser.add_option('-i', '--integration-tests', action='store_true', default=False,
                          help='run integration tests as well as unit tests'),
        parser.add_option('-j', '--child-processes', action='store', type='int', default=(1 if sys.platform == 'win32' else multiprocessing.cpu_count()),
                          help='number of tests to run in parallel (default=%default)')
        parser.add_option('-p', '--pass-through', action='store_true', default=False,
                          help='be debugger friendly by passing captured output through to the system')
        parser.add_option('-q', '--quiet', action='store_true', default=False,
                          help='run quietly (errors, warnings, and progress only)')
        parser.add_option('-t', '--timing', action='store_true', default=False,
                          help='display per-test execution time (implies --verbose)')
        parser.add_option('-v', '--verbose', action='count', default=0,
                          help='verbose output (specify once for individual test results, twice for debug messages)')

        parser.epilog = ('[args...] is an optional list of modules, test_classes, or individual tests. '
                         'If no args are given, all the tests will be run.')

        return parser.parse_args(argv)

    def run(self):
        self._options, args = self._parse_args()
        self.printer.configure(self._options)

        self.finder.clean_trees()

        names = self.finder.find_names(args, self._options.all)
        if not names:
            _log.error('No tests to run')
            return False

        return self._run_tests(names)

    def _run_tests(self, names):
        # Make sure PYTHONPATH is set up properly.
        sys.path = self.finder.additional_paths(sys.path) + sys.path

        # We autoinstall everything up so that we can run tests concurrently
        # and not have to worry about autoinstalling packages concurrently.
        self.printer.write_update("Checking autoinstalled packages ...")
        from webkitpy.thirdparty import autoinstall_everything
        installed_something = autoinstall_everything()

        # FIXME: There appears to be a bug in Python 2.6.1 that is causing multiprocessing
        # to hang after we install the packages in a clean checkout.
        if installed_something:
            _log.warning("We installed new packages, so running things serially at first")
            self._options.child_processes = 1

        if self._options.coverage:
            _log.warning("Checking code coverage, so running things serially")
            self._options.child_processes = 1

            import webkitpy.thirdparty.autoinstalled.coverage as coverage
            cov = coverage.coverage(omit=["/usr/*", "*/webkitpy/thirdparty/autoinstalled/*", "*/webkitpy/thirdparty/BeautifulSoup.py"])
            cov.start()

        self.printer.write_update("Checking imports ...")
        if not self._check_imports(names):
            return False

        self.printer.write_update("Finding the individual test methods ...")
        loader = _Loader()
        parallel_tests, serial_tests = self._test_names(loader, names)

        self.printer.write_update("Running the tests ...")
        self.printer.num_tests = len(parallel_tests) + len(serial_tests)
        start = time.time()
        test_runner = Runner(self.printer, loader)
        test_runner.run(parallel_tests, self._options.child_processes)
        test_runner.run(serial_tests, 1)

        self.printer.print_result(time.time() - start)

        if self._options.coverage:
            cov.stop()
            cov.save()
            cov.report(show_missing=False)

        return not self.printer.num_errors and not self.printer.num_failures

    def _check_imports(self, names):
        for name in names:
            if self.finder.is_module(name):
                # if we failed to load a name and it looks like a module,
                # try importing it directly, because loadTestsFromName()
                # produces lousy error messages for bad modules.
                try:
                    __import__(name)
                except ImportError:
                    _log.fatal('Failed to import %s:' % name)
                    self._log_exception()
                    return False
        return True

    def _test_names(self, loader, names):
        parallel_test_method_prefixes = ['test_']
        serial_test_method_prefixes = ['serial_test_']
        if self._options.integration_tests:
            parallel_test_method_prefixes.append('integration_test_')
            serial_test_method_prefixes.append('serial_integration_test_')

        parallel_tests = []
        loader.test_method_prefixes = parallel_test_method_prefixes
        for name in names:
            parallel_tests.extend(self._all_test_names(loader.loadTestsFromName(name, None)))

        serial_tests = []
        loader.test_method_prefixes = serial_test_method_prefixes
        for name in names:
            serial_tests.extend(self._all_test_names(loader.loadTestsFromName(name, None)))

        # loader.loadTestsFromName() will not verify that names begin with one of the test_method_prefixes
        # if the names were explicitly provided (e.g., MainTest.test_basic), so this means that any individual
        # tests will be included in both parallel_tests and serial_tests, and we need to de-dup them.
        serial_tests = list(set(serial_tests).difference(set(parallel_tests)))

        return (parallel_tests, serial_tests)

    def _all_test_names(self, suite):
        names = []
        if hasattr(suite, '_tests'):
            for t in suite._tests:
                names.extend(self._all_test_names(t))
        else:
            names.append(unit_test_name(suite))
        return names

    def _log_exception(self):
        s = StringIO.StringIO()
        traceback.print_exc(file=s)
        for l in s.buflist:
            _log.error('  ' + l.rstrip())


class _Loader(unittest.TestLoader):
    test_method_prefixes = []

    def getTestCaseNames(self, testCaseClass):
        def isTestMethod(attrname, testCaseClass=testCaseClass):
            if not hasattr(getattr(testCaseClass, attrname), '__call__'):
                return False
            return (any(attrname.startswith(prefix) for prefix in self.test_method_prefixes))
        testFnNames = filter(isTestMethod, dir(testCaseClass))
        testFnNames.sort()
        return testFnNames


if __name__ == '__main__':
    sys.exit(main())
