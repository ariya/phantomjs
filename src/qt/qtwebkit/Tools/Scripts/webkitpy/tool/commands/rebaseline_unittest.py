# Copyright (C) 2010 Google Inc. All rights reserved.
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
from webkitpy.common.checkout.baselineoptimizer import BaselineOptimizer
from webkitpy.common.net.buildbot.buildbot_mock import MockBuilder
from webkitpy.common.system.executive_mock import MockExecutive2
from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.commands.rebaseline import *
from webkitpy.tool.mocktool import MockTool, MockOptions


class _BaseTestCase(unittest.TestCase):
    MOCK_WEB_RESULT = 'MOCK Web result, convert 404 to None=True'
    WEB_PREFIX = 'http://example.com/f/builders/Apple Lion Release WK1 (Tests)/results/layout-test-results'

    command_constructor = None

    def setUp(self):
        self.tool = MockTool()
        self.command = self.command_constructor()  # lint warns that command_constructor might not be set, but this is intentional; pylint: disable=E1102
        self.command.bind_to_tool(self.tool)
        self.lion_port = self.tool.port_factory.get_from_builder_name("Apple Lion Release WK1 (Tests)")
        self.lion_expectations_path = self.lion_port.path_to_test_expectations_file()

        # FIXME: we should override builders._exact_matches here to point to a set
        # of test ports and restore the value in tearDown(), and that way the
        # individual tests wouldn't have to worry about it.

    def _expand(self, path):
        if self.tool.filesystem.isabs(path):
            return path
        return self.tool.filesystem.join(self.lion_port.layout_tests_dir(), path)

    def _read(self, path):
        return self.tool.filesystem.read_text_file(self._expand(path))

    def _write(self, path, contents):
        self.tool.filesystem.write_text_file(self._expand(path), contents)

    def _zero_out_test_expectations(self):
        for port_name in self.tool.port_factory.all_port_names():
            port = self.tool.port_factory.get(port_name)
            for path in port.expectations_files():
                self._write(path, '')
        self.tool.filesystem.written_files = {}


class TestRebaselineTest(_BaseTestCase):
    command_constructor = RebaselineTest  # AKA webkit-patch rebaseline-test-internal

    def setUp(self):
        super(TestRebaselineTest, self).setUp()
        self.options = MockOptions(builder="Apple Lion Release WK1 (Tests)", test="userscripts/another-test.html", suffixes="txt",
                                   move_overwritten_baselines_to=None, results_directory=None)

    def test_baseline_directory(self):
        command = self.command
        self.assertMultiLineEqual(command._baseline_directory("Apple Win XP Debug (Tests)"), "/mock-checkout/LayoutTests/platform/win-xp")
        self.assertMultiLineEqual(command._baseline_directory("Apple Win 7 Release (Tests)"), "/mock-checkout/LayoutTests/platform/win")
        self.assertMultiLineEqual(command._baseline_directory("Apple Lion Release WK1 (Tests)"), "/mock-checkout/LayoutTests/platform/mac-lion")
        self.assertMultiLineEqual(command._baseline_directory("Apple Lion Release WK2 (Tests)"), "/mock-checkout/LayoutTests/platform/mac-wk2")
        self.assertMultiLineEqual(command._baseline_directory("Apple MountainLion Release WK1 (Tests)"), "/mock-checkout/LayoutTests/platform/mac")
        self.assertMultiLineEqual(command._baseline_directory("Apple MountainLion Release WK2 (Tests)"), "/mock-checkout/LayoutTests/platform/mac")
        self.assertMultiLineEqual(command._baseline_directory("GTK Linux 64-bit Debug"), "/mock-checkout/LayoutTests/platform/gtk-wk1")
        self.assertMultiLineEqual(command._baseline_directory("GTK Linux 64-bit Release WK2 (Tests)"), "/mock-checkout/LayoutTests/platform/gtk-wk2")
        self.assertMultiLineEqual(command._baseline_directory("EFL Linux 64-bit Release WK2"), "/mock-checkout/LayoutTests/platform/efl-wk2")
        self.assertMultiLineEqual(command._baseline_directory("Qt Linux Release"), "/mock-checkout/LayoutTests/platform/qt")

    def test_rebaseline_updates_expectations_file_noop(self):
        self._zero_out_test_expectations()
        self._write(self.lion_expectations_path, """Bug(B) [ Mac Linux XP Debug ] fast/dom/Window/window-postmessage-clone-really-deep-array.html [ Pass ]
Bug(A) [ Debug ] : fast/css/large-list-of-rules-crash.html [ Failure ]
""")
        self._write("fast/dom/Window/window-postmessage-clone-really-deep-array.html", "Dummy test contents")
        self._write("fast/css/large-list-of-rules-crash.html", "Dummy test contents")
        self._write("userscripts/another-test.html", "Dummy test contents")

        self.options.suffixes = "png,wav,txt"
        self.command._rebaseline_test_and_update_expectations(self.options)

        self.assertItemsEqual(self.tool.web.urls_fetched,
            [self.WEB_PREFIX + '/userscripts/another-test-actual.png',
             self.WEB_PREFIX + '/userscripts/another-test-actual.wav',
             self.WEB_PREFIX + '/userscripts/another-test-actual.txt'])
        new_expectations = self._read(self.lion_expectations_path)
        self.assertMultiLineEqual(new_expectations, """Bug(B) [ Mac Linux XP Debug ] fast/dom/Window/window-postmessage-clone-really-deep-array.html [ Pass ]
Bug(A) [ Debug ] : fast/css/large-list-of-rules-crash.html [ Failure ]
""")

    def test_rebaseline_updates_expectations_file(self):
        self._write(self.lion_expectations_path, "Bug(x) [ Mac ] userscripts/another-test.html [ ImageOnlyFailure ]\nbug(z) [ Linux ] userscripts/another-test.html [ ImageOnlyFailure ]\n")
        self._write("userscripts/another-test.html", "Dummy test contents")

        self.options.suffixes = 'png,wav,txt'
        self.command._rebaseline_test_and_update_expectations(self.options)

        self.assertItemsEqual(self.tool.web.urls_fetched,
            [self.WEB_PREFIX + '/userscripts/another-test-actual.png',
             self.WEB_PREFIX + '/userscripts/another-test-actual.wav',
             self.WEB_PREFIX + '/userscripts/another-test-actual.txt'])
        new_expectations = self._read(self.lion_expectations_path)
        self.assertMultiLineEqual(new_expectations, "Bug(x) [ Mac ] userscripts/another-test.html [ ImageOnlyFailure ]\nbug(z) [ Linux ] userscripts/another-test.html [ ImageOnlyFailure ]\n")

    def test_rebaseline_does_not_include_overrides(self):
        self._write(self.lion_expectations_path, "Bug(x) [ Mac ] userscripts/another-test.html [ ImageOnlyFailure ]\nBug(z) [ Linux ] userscripts/another-test.html [ ImageOnlyFailure ]\n")
        self._write("userscripts/another-test.html", "Dummy test contents")

        self.options.suffixes = 'png,wav,txt'
        self.command._rebaseline_test_and_update_expectations(self.options)

        self.assertItemsEqual(self.tool.web.urls_fetched,
            [self.WEB_PREFIX + '/userscripts/another-test-actual.png',
             self.WEB_PREFIX + '/userscripts/another-test-actual.wav',
             self.WEB_PREFIX + '/userscripts/another-test-actual.txt'])

        new_expectations = self._read(self.lion_expectations_path)
        self.assertMultiLineEqual(new_expectations, "Bug(x) [ Mac ] userscripts/another-test.html [ ImageOnlyFailure ]\nBug(z) [ Linux ] userscripts/another-test.html [ ImageOnlyFailure ]\n")

    def test_rebaseline_test(self):
        self.command._rebaseline_test("Apple Lion Release WK1 (Tests)", "userscripts/another-test.html", None, "txt", self.WEB_PREFIX)
        self.assertItemsEqual(self.tool.web.urls_fetched, [self.WEB_PREFIX + '/userscripts/another-test-actual.txt'])

    def test_rebaseline_test_with_results_directory(self):
        self._write(self.lion_expectations_path, "Bug(x) [ Mac ] userscripts/another-test.html [ ImageOnlyFailure ]\nbug(z) [ Linux ] userscripts/another-test.html [ ImageOnlyFailure ]\n")
        self.options.results_directory = '/tmp'
        self.command._rebaseline_test_and_update_expectations(self.options)
        self.assertItemsEqual(self.tool.web.urls_fetched, ['file:///tmp/userscripts/another-test-actual.txt'])

    def test_rebaseline_test_and_print_scm_changes(self):
        self.command._print_scm_changes = True
        self.command._scm_changes = {'add': [], 'delete': []}
        self.tool._scm.exists = lambda x: False

        self.command._rebaseline_test("Apple Lion Release WK1 (Tests)", "userscripts/another-test.html", None, "txt", None)

        self.assertDictEqual(self.command._scm_changes, {'add': ['/mock-checkout/LayoutTests/platform/mac-lion/userscripts/another-test-expected.txt'], 'delete': []})

    def test_rebaseline_and_copy_test(self):
        self._write("userscripts/another-test-expected.txt", "generic result")

        self.command._rebaseline_test("Apple Lion Release WK1 (Tests)", "userscripts/another-test.html", ["mac-lion-wk2"], "txt", None)

        self.assertMultiLineEqual(self._read('platform/mac-lion/userscripts/another-test-expected.txt'), self.MOCK_WEB_RESULT)
        self.assertMultiLineEqual(self._read('platform/mac-wk2/userscripts/another-test-expected.txt'), 'generic result')

    def test_rebaseline_and_copy_test_no_existing_result(self):
        self.command._rebaseline_test("Apple Lion Release WK1 (Tests)", "userscripts/another-test.html", ["mac-lion-wk2"], "txt", None)

        self.assertMultiLineEqual(self._read('platform/mac-lion/userscripts/another-test-expected.txt'), self.MOCK_WEB_RESULT)
        self.assertFalse(self.tool.filesystem.exists(self._expand('platform/mac-lion-wk2/userscripts/another-test-expected.txt')))

    def test_rebaseline_and_copy_test_with_lion_result(self):
        self._write("platform/mac-lion/userscripts/another-test-expected.txt", "original lion result")

        self.command._rebaseline_test("Apple Lion Release WK1 (Tests)", "userscripts/another-test.html", ["mac-lion-wk2"], "txt", self.WEB_PREFIX)

        self.assertItemsEqual(self.tool.web.urls_fetched, [self.WEB_PREFIX + '/userscripts/another-test-actual.txt'])
        self.assertMultiLineEqual(self._read("platform/mac-wk2/userscripts/another-test-expected.txt"), "original lion result")
        self.assertMultiLineEqual(self._read("platform/mac-lion/userscripts/another-test-expected.txt"), self.MOCK_WEB_RESULT)

    def test_rebaseline_and_copy_no_overwrite_test(self):
        self._write("platform/mac-lion/userscripts/another-test-expected.txt", "original lion result")
        self._write("platform/mac-lion-wk2/userscripts/another-test-expected.txt", "original lion wk2 result")

        self.command._rebaseline_test("Apple Lion Release WK1 (Tests)", "userscripts/another-test.html", ["mac-lion-wk2"], "txt", None)

        self.assertMultiLineEqual(self._read("platform/mac-lion-wk2/userscripts/another-test-expected.txt"), "original lion wk2 result")
        self.assertMultiLineEqual(self._read("platform/mac-lion/userscripts/another-test-expected.txt"), self.MOCK_WEB_RESULT)

    def test_rebaseline_test_internal_with_move_overwritten_baselines_to(self):
        self.tool.executive = MockExecutive2()

        # FIXME: it's confusing that this is the test- port, and not the regular lion port. Really all of the tests should be using the test ports.
        port = self.tool.port_factory.get('test-mac-snowleopard')
        self._write(port._filesystem.join(port.layout_tests_dir(), 'platform/test-mac-snowleopard/failures/expected/image-expected.txt'), 'original snowleopard result')

        old_exact_matches = builders._exact_matches
        oc = OutputCapture()
        try:
            builders._exact_matches = {
                "MOCK Leopard": {"port_name": "test-mac-leopard", "specifiers": set(["mock-specifier"])},
                "MOCK SnowLeopard": {"port_name": "test-mac-snowleopard", "specifiers": set(["mock-specifier"])},
            }

            options = MockOptions(optimize=True, builder="MOCK SnowLeopard", suffixes="txt",
                move_overwritten_baselines_to=["test-mac-leopard"], verbose=True, test="failures/expected/image.html",
                results_directory=None)

            oc.capture_output()
            self.command.execute(options, [], self.tool)
        finally:
            out, _, _ = oc.restore_output()
            builders._exact_matches = old_exact_matches

        self.assertMultiLineEqual(self._read(self.tool.filesystem.join(port.layout_tests_dir(), 'platform/test-mac-leopard/failures/expected/image-expected.txt')), 'original snowleopard result')
        self.assertMultiLineEqual(out, '{"add": []}\n')


class TestRebaselineJson(_BaseTestCase):
    command_constructor = RebaselineJson

    def setUp(self):
        super(TestRebaselineJson, self).setUp()
        self.tool.executive = MockExecutive2()
        self.old_exact_matches = builders._exact_matches
        builders._exact_matches = {
            "MOCK builder": {"port_name": "test-mac-snowleopard", "specifiers": set(["mock-specifier"]),
                             "move_overwritten_baselines_to": ["test-mac-leopard"]},
            "MOCK builder (Debug)": {"port_name": "test-mac-snowleopard", "specifiers": set(["mock-specifier", "debug"])},
        }

    def tearDown(self):
        builders._exact_matches = self.old_exact_matches
        super(TestRebaselineJson, self).tearDown()

    def test_rebaseline_all(self):
        options = MockOptions(optimize=True, verbose=True, move_overwritten_baselines=False, results_directory=None)
        self.command._rebaseline(options,  {"user-scripts/another-test.html": {"MOCK builder": ["txt", "png"]}})

        # Note that we have one run_in_parallel() call followed by a run_command()
        self.assertEqual(self.tool.executive.calls,
            [[['echo', 'rebaseline-test-internal', '--suffixes', 'txt,png', '--builder', 'MOCK builder', '--test', 'user-scripts/another-test.html', '--verbose']],
             ['echo', '--verbose', 'optimize-baselines', '--suffixes', 'txt,png', 'user-scripts/another-test.html']])

    def test_rebaseline_debug(self):
        options = MockOptions(optimize=True, verbose=True, move_overwritten_baselines=False, results_directory=None)
        self.command._rebaseline(options,  {"user-scripts/another-test.html": {"MOCK builder (Debug)": ["txt", "png"]}})

        # Note that we have one run_in_parallel() call followed by a run_command()
        self.assertEqual(self.tool.executive.calls,
            [[['echo', 'rebaseline-test-internal', '--suffixes', 'txt,png', '--builder', 'MOCK builder (Debug)', '--test', 'user-scripts/another-test.html', '--verbose']],
             ['echo', '--verbose', 'optimize-baselines', '--suffixes', 'txt,png', 'user-scripts/another-test.html']])

    def test_move_overwritten(self):
        options = MockOptions(optimize=True, verbose=True, move_overwritten_baselines=True, results_directory=None)
        self.command._rebaseline(options,  {"user-scripts/another-test.html": {"MOCK builder": ["txt", "png"]}})

        # Note that we have one run_in_parallel() call followed by a run_command()
        self.assertEqual(self.tool.executive.calls,
            [[['echo', 'rebaseline-test-internal', '--suffixes', 'txt,png', '--builder', 'MOCK builder', '--test', 'user-scripts/another-test.html', '--move-overwritten-baselines-to', 'test-mac-leopard', '--verbose']],
             ['echo', '--verbose', 'optimize-baselines', '--suffixes', 'txt,png', 'user-scripts/another-test.html']])

    def test_no_optimize(self):
        options = MockOptions(optimize=False, verbose=True, move_overwritten_baselines=False, results_directory=None)
        self.command._rebaseline(options,  {"user-scripts/another-test.html": {"MOCK builder (Debug)": ["txt", "png"]}})

        # Note that we have only one run_in_parallel() call
        self.assertEqual(self.tool.executive.calls,
            [[['echo', 'rebaseline-test-internal', '--suffixes', 'txt,png', '--builder', 'MOCK builder (Debug)', '--test', 'user-scripts/another-test.html', '--verbose']]])

    def test_results_directory(self):
        options = MockOptions(optimize=False, verbose=True, move_overwritten_baselines=False, results_directory='/tmp')
        self.command._rebaseline(options,  {"user-scripts/another-test.html": {"MOCK builder": ["txt", "png"]}})

        # Note that we have only one run_in_parallel() call
        self.assertEqual(self.tool.executive.calls,
            [[['echo', 'rebaseline-test-internal', '--suffixes', 'txt,png', '--builder', 'MOCK builder', '--test', 'user-scripts/another-test.html', '--results-directory', '/tmp', '--verbose']]])


class TestRebaseline(_BaseTestCase):
    # This command shares most of its logic with RebaselineJson, so these tests just test what is different.

    command_constructor = Rebaseline  # AKA webkit-patch rebaseline

    def test_tests_to_update(self):
        build = Mock()
        OutputCapture().assert_outputs(self, self.command._tests_to_update, [build])

    def test_rebaseline(self):
        self.command._builders_to_pull_from = lambda: [MockBuilder('MOCK builder')]
        self.command._tests_to_update = lambda builder: ['mock/path/to/test.html']

        self._zero_out_test_expectations()

        old_exact_matches = builders._exact_matches
        oc = OutputCapture()
        try:
            builders._exact_matches = {
                "MOCK builder": {"port_name": "test-mac-leopard", "specifiers": set(["mock-specifier"])},
            }
            oc.capture_output()
            self.command.execute(MockOptions(optimize=False, builders=None, suffixes="txt,png", verbose=True, move_overwritten_baselines=False), [], self.tool)
        finally:
            oc.restore_output()
            builders._exact_matches = old_exact_matches

        calls = filter(lambda x: x != ['qmake', '-v'] and x[0] != 'perl', self.tool.executive.calls)
        self.assertEqual(calls,
            [[['echo', 'rebaseline-test-internal', '--suffixes', 'txt,png', '--builder', 'MOCK builder', '--test', 'mock/path/to/test.html', '--verbose']]])


class TestRebaselineExpectations(_BaseTestCase):
    command_constructor = RebaselineExpectations

    def setUp(self):
        super(TestRebaselineExpectations, self).setUp()
        self.options = MockOptions(optimize=False, builders=None, suffixes=['txt'], verbose=False, platform=None,
                                   move_overwritten_baselines=False, results_directory=None)

    def test_rebaseline_expectations(self):
        self._zero_out_test_expectations()

        self.tool.executive = MockExecutive2()

        self.command._tests_to_rebaseline = lambda port: {'userscripts/another-test.html': set(['txt']), 'userscripts/images.svg': set(['png'])}
        self.command.execute(self.options, [], self.tool)

        # FIXME: change this to use the test- ports.
        calls = filter(lambda x: x != ['qmake', '-v'], self.tool.executive.calls)
        self.assertEqual(len(calls), 1)
        self.assertEqual(len(calls[0]), 22)

    def test_rebaseline_expectations_noop(self):
        self._zero_out_test_expectations()

        oc = OutputCapture()
        try:
            oc.capture_output()
            self.command.execute(self.options, [], self.tool)
        finally:
            _, _, logs = oc.restore_output()
            self.assertEqual(self.tool.filesystem.written_files, {})
            self.assertEqual(logs, 'Did not find any tests marked Rebaseline.\n')

    def disabled_test_overrides_are_included_correctly(self):
        # This tests that the any tests marked as REBASELINE in the overrides are found, but
        # that the overrides do not get written into the main file.
        self._zero_out_test_expectations()

        self._write(self.lion_expectations_path, '')
        self.lion_port.expectations_dict = lambda: {
            self.lion_expectations_path: '',
            'overrides': ('Bug(x) userscripts/another-test.html [ Failure Rebaseline ]\n'
                          'Bug(y) userscripts/test.html [ Crash ]\n')}
        self._write('/userscripts/another-test.html', '')

        self.assertDictEqual(self.command._tests_to_rebaseline(self.lion_port), {'userscripts/another-test.html': set(['png', 'txt', 'wav'])})
        self.assertEqual(self._read(self.lion_expectations_path), '')


class _FakeOptimizer(BaselineOptimizer):
    def read_results_by_directory(self, baseline_name):
        if baseline_name.endswith('txt'):
            return {'LayoutTests/passes/text.html': '123456',
                    'LayoutTests/platform/test-mac-leopard/passes/text.html': 'abcdef'}
        return {}


class TestAnalyzeBaselines(_BaseTestCase):
    command_constructor = AnalyzeBaselines

    def setUp(self):
        super(TestAnalyzeBaselines, self).setUp()
        self.port = self.tool.port_factory.get('test')
        self.tool.port_factory.get = (lambda port_name=None, options=None: self.port)
        self.lines = []
        self.command._optimizer_class = _FakeOptimizer
        self.command._write = (lambda msg: self.lines.append(msg))  # pylint bug warning about unnecessary lambda? pylint: disable=W0108

    def test_default(self):
        self.command.execute(MockOptions(suffixes='txt', missing=False, platform=None), ['passes/text.html'], self.tool)
        self.assertEqual(self.lines,
            ['passes/text-expected.txt:',
             '  (generic): 123456',
             '  test-mac-leopard: abcdef'])

    def test_missing_baselines(self):
        self.command.execute(MockOptions(suffixes='png,txt', missing=True, platform=None), ['passes/text.html'], self.tool)
        self.assertEqual(self.lines,
            ['passes/text-expected.png: (no baselines found)',
             'passes/text-expected.txt:',
             '  (generic): 123456',
             '  test-mac-leopard: abcdef'])
