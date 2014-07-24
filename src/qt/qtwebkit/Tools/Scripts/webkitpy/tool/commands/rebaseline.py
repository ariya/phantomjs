# Copyright (c) 2010 Google Inc. All rights reserved.
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

import json
import logging
import optparse
import sys

from webkitpy.common.checkout.baselineoptimizer import BaselineOptimizer
from webkitpy.common.system.executive import ScriptError
from webkitpy.layout_tests.controllers.test_result_writer import TestResultWriter
from webkitpy.layout_tests.models import test_failures
from webkitpy.layout_tests.models.test_expectations import TestExpectations, BASELINE_SUFFIX_LIST
from webkitpy.port import builders
from webkitpy.port import factory
from webkitpy.tool.multicommandtool import Command


_log = logging.getLogger(__name__)


# FIXME: Should TestResultWriter know how to compute this string?
def _baseline_name(fs, test_name, suffix):
    return fs.splitext(test_name)[0] + TestResultWriter.FILENAME_SUFFIX_EXPECTED + "." + suffix


class AbstractRebaseliningCommand(Command):
    # not overriding execute() - pylint: disable=W0223

    move_overwritten_baselines_option = optparse.make_option("--move-overwritten-baselines", action="store_true", default=False,
        help="Move overwritten baselines elsewhere in the baseline path. This is for bringing up new ports.")

    no_optimize_option = optparse.make_option('--no-optimize', dest='optimize', action='store_false', default=True,
        help=('Do not optimize/de-dup the expectations after rebaselining (default is to de-dup automatically). '
              'You can use "webkit-patch optimize-baselines" to optimize separately.'))

    platform_options = factory.platform_options(use_globs=True)

    results_directory_option = optparse.make_option("--results-directory", help="Local results directory to use")

    suffixes_option = optparse.make_option("--suffixes", default=','.join(BASELINE_SUFFIX_LIST), action="store",
        help="Comma-separated-list of file types to rebaseline")

    def __init__(self, options=None):
        super(AbstractRebaseliningCommand, self).__init__(options=options)
        self._baseline_suffix_list = BASELINE_SUFFIX_LIST


class RebaselineTest(AbstractRebaseliningCommand):
    name = "rebaseline-test-internal"
    help_text = "Rebaseline a single test from a buildbot. Only intended for use by other webkit-patch commands."

    def __init__(self):
        super(RebaselineTest, self).__init__(options=[
            self.no_optimize_option,
            self.results_directory_option,
            self.suffixes_option,
            optparse.make_option("--builder", help="Builder to pull new baselines from"),
            optparse.make_option("--move-overwritten-baselines-to", action="append", default=[],
                help="Platform to move existing baselines to before rebaselining. This is for bringing up new ports."),
            optparse.make_option("--test", help="Test to rebaseline"),
            ])
        self._scm_changes = {'add': []}

    def _results_url(self, builder_name):
        return self._tool.buildbot.builder_with_name(builder_name).latest_layout_test_results_url()

    def _baseline_directory(self, builder_name):
        port = self._tool.port_factory.get_from_builder_name(builder_name)
        override_dir = builders.rebaseline_override_dir(builder_name)
        if override_dir:
            return self._tool.filesystem.join(port.layout_tests_dir(), 'platform', override_dir)
        return port.baseline_version_dir()

    def _copy_existing_baseline(self, move_overwritten_baselines_to, test_name, suffix):
        old_baselines = []
        new_baselines = []

        # Need to gather all the baseline paths before modifying the filesystem since
        # the modifications can affect the results of port.expected_filename.
        for platform in move_overwritten_baselines_to:
            port = self._tool.port_factory.get(platform)
            old_baseline = port.expected_filename(test_name, "." + suffix)
            if not self._tool.filesystem.exists(old_baseline):
                _log.debug("No existing baseline for %s." % test_name)
                continue

            new_baseline = self._tool.filesystem.join(port.baseline_path(), self._file_name_for_expected_result(test_name, suffix))
            if self._tool.filesystem.exists(new_baseline):
                _log.debug("Existing baseline at %s, not copying over it." % new_baseline)
                continue

            old_baselines.append(old_baseline)
            new_baselines.append(new_baseline)

        for i in range(len(old_baselines)):
            old_baseline = old_baselines[i]
            new_baseline = new_baselines[i]

            _log.debug("Copying baseline from %s to %s." % (old_baseline, new_baseline))
            self._tool.filesystem.maybe_make_directory(self._tool.filesystem.dirname(new_baseline))
            self._tool.filesystem.copyfile(old_baseline, new_baseline)
            if not self._tool.scm().exists(new_baseline):
                self._add_to_scm(new_baseline)

    def _save_baseline(self, data, target_baseline):
        if not data:
            return
        filesystem = self._tool.filesystem
        filesystem.maybe_make_directory(filesystem.dirname(target_baseline))
        filesystem.write_binary_file(target_baseline, data)
        if not self._tool.scm().exists(target_baseline):
            self._add_to_scm(target_baseline)

    def _add_to_scm(self, path):
        self._scm_changes['add'].append(path)

    def _update_expectations_file(self, builder_name, test_name):
        port = self._tool.port_factory.get_from_builder_name(builder_name)

        # Since rebaseline-test-internal can be called multiple times in parallel,
        # we need to ensure that we're not trying to update the expectations file
        # concurrently as well.
        # FIXME: We should rework the code to not need this; maybe just download
        # the files in parallel and rebaseline local files serially?
        try:
            path = port.path_to_test_expectations_file()
            lock = self._tool.make_file_lock(path + '.lock')
            lock.acquire_lock()
            expectations = TestExpectations(port, include_generic=False, include_overrides=False)
            for test_configuration in port.all_test_configurations():
                if test_configuration.version == port.test_configuration().version:
                    expectationsString = expectations.remove_configuration_from_test(test_name, test_configuration)

            self._tool.filesystem.write_text_file(path, expectationsString)
        finally:
            lock.release_lock()

    def _test_root(self, test_name):
        return self._tool.filesystem.splitext(test_name)[0]

    def _file_name_for_actual_result(self, test_name, suffix):
        return "%s-actual.%s" % (self._test_root(test_name), suffix)

    def _file_name_for_expected_result(self, test_name, suffix):
        return "%s-expected.%s" % (self._test_root(test_name), suffix)

    def _rebaseline_test(self, builder_name, test_name, move_overwritten_baselines_to, suffix, results_url):
        baseline_directory = self._baseline_directory(builder_name)

        source_baseline = "%s/%s" % (results_url, self._file_name_for_actual_result(test_name, suffix))
        target_baseline = self._tool.filesystem.join(baseline_directory, self._file_name_for_expected_result(test_name, suffix))

        if move_overwritten_baselines_to:
            self._copy_existing_baseline(move_overwritten_baselines_to, test_name, suffix)

        _log.debug("Retrieving %s." % source_baseline)
        self._save_baseline(self._tool.web.get_binary(source_baseline, convert_404_to_None=True), target_baseline)

    def _rebaseline_test_and_update_expectations(self, options):
        if options.results_directory:
            results_url = 'file://' + options.results_directory
        else:
            results_url = self._results_url(options.builder)
        self._baseline_suffix_list = options.suffixes.split(',')
        for suffix in self._baseline_suffix_list:
            self._rebaseline_test(options.builder, options.test, options.move_overwritten_baselines_to, suffix, results_url)
        self._update_expectations_file(options.builder, options.test)

    def execute(self, options, args, tool):
        self._rebaseline_test_and_update_expectations(options)
        print json.dumps(self._scm_changes)


class OptimizeBaselines(AbstractRebaseliningCommand):
    name = "optimize-baselines"
    help_text = "Reshuffles the baselines for the given tests to use as litte space on disk as possible."
    argument_names = "TEST_NAMES"

    def __init__(self):
        super(OptimizeBaselines, self).__init__(options=[self.suffixes_option] + self.platform_options)

    def _optimize_baseline(self, optimizer, test_name):
        for suffix in self._baseline_suffix_list:
            baseline_name = _baseline_name(self._tool.filesystem, test_name, suffix)
            if not optimizer.optimize(baseline_name):
                print "Heuristics failed to optimize %s" % baseline_name

    def execute(self, options, args, tool):
        self._baseline_suffix_list = options.suffixes.split(',')
        port_names = tool.port_factory.all_port_names(options.platform)
        if not port_names:
            print "No port names match '%s'" % options.platform
            return

        optimizer = BaselineOptimizer(tool, port_names)
        port = tool.port_factory.get(port_names[0])
        for test_name in port.tests(args):
            _log.info("Optimizing %s" % test_name)
            self._optimize_baseline(optimizer, test_name)


class AnalyzeBaselines(AbstractRebaseliningCommand):
    name = "analyze-baselines"
    help_text = "Analyzes the baselines for the given tests and prints results that are identical."
    argument_names = "TEST_NAMES"

    def __init__(self):
        super(AnalyzeBaselines, self).__init__(options=[
            self.suffixes_option,
            optparse.make_option('--missing', action='store_true', default=False, help='show missing baselines as well'),
            ] + self.platform_options)
        self._optimizer_class = BaselineOptimizer  # overridable for testing
        self._baseline_optimizer = None
        self._port = None

    def _write(self, msg):
        print msg

    def _analyze_baseline(self, options, test_name):
        for suffix in self._baseline_suffix_list:
            baseline_name = _baseline_name(self._tool.filesystem, test_name, suffix)
            results_by_directory = self._baseline_optimizer.read_results_by_directory(baseline_name)
            if results_by_directory:
                self._write("%s:" % baseline_name)
                self._baseline_optimizer.write_by_directory(results_by_directory, self._write, "  ")
            elif options.missing:
                self._write("%s: (no baselines found)" % baseline_name)

    def execute(self, options, args, tool):
        self._baseline_suffix_list = options.suffixes.split(',')
        port_names = tool.port_factory.all_port_names(options.platform)
        if not port_names:
            print "No port names match '%s'" % options.platform
            return

        self._baseline_optimizer = self._optimizer_class(tool, port_names)
        self._port = tool.port_factory.get(port_names[0])
        for test_name in self._port.tests(args):
            self._analyze_baseline(options, test_name)


class AbstractParallelRebaselineCommand(AbstractRebaseliningCommand):
    # not overriding execute() - pylint: disable=W0223

    def _run_webkit_patch(self, args, verbose):
        try:
            verbose_args = ['--verbose'] if verbose else []
            stderr = self._tool.executive.run_command([self._tool.path()] + verbose_args + args, cwd=self._tool.scm().checkout_root, return_stderr=True)
            for line in stderr.splitlines():
                print >> sys.stderr, line
        except ScriptError, e:
            _log.error(e)

    def _builders_to_fetch_from(self, builders_to_check):
        # This routine returns the subset of builders that will cover all of the baseline search paths
        # used in the input list. In particular, if the input list contains both Release and Debug
        # versions of a configuration, we *only* return the Release version (since we don't save
        # debug versions of baselines).
        release_builders = set()
        debug_builders = set()
        builders_to_fallback_paths = {}
        for builder in builders_to_check:
            port = self._tool.port_factory.get_from_builder_name(builder)
            if port.test_configuration().build_type == 'Release':
                release_builders.add(builder)
            else:
                debug_builders.add(builder)
        for builder in list(release_builders) + list(debug_builders):
            port = self._tool.port_factory.get_from_builder_name(builder)
            fallback_path = port.baseline_search_path()
            if fallback_path not in builders_to_fallback_paths.values():
                builders_to_fallback_paths[builder] = fallback_path
        return builders_to_fallback_paths.keys()

    def _rebaseline_commands(self, test_list, options):

        path_to_webkit_patch = self._tool.path()
        cwd = self._tool.scm().checkout_root
        commands = []
        for test in test_list:
            for builder in self._builders_to_fetch_from(test_list[test]):
                suffixes = ','.join(test_list[test][builder])
                cmd_line = [path_to_webkit_patch, 'rebaseline-test-internal', '--suffixes', suffixes, '--builder', builder, '--test', test]
                if options.move_overwritten_baselines:
                    move_overwritten_baselines_to = builders.move_overwritten_baselines_to(builder)
                    for platform in move_overwritten_baselines_to:
                        cmd_line.extend(['--move-overwritten-baselines-to', platform])
                if options.results_directory:
                    cmd_line.extend(['--results-directory', options.results_directory])
                if options.verbose:
                    cmd_line.append('--verbose')
                commands.append(tuple([cmd_line, cwd]))
        return commands

    def _files_to_add(self, command_results):
        files_to_add = set()
        for output in [result[1].split('\n') for result in command_results]:
            file_added = False
            for line in output:
                try:
                    if line:
                        files_to_add.update(json.loads(line)['add'])
                        file_added = True
                except ValueError:
                    _log.debug('"%s" is not a JSON object, ignoring' % line)

            if not file_added:
                _log.debug('Could not add file based off output "%s"' % output)


        return list(files_to_add)

    def _optimize_baselines(self, test_list, verbose=False):
        # We don't run this in parallel because modifying the SCM in parallel is unreliable.
        for test in test_list:
            all_suffixes = set()
            for builder in self._builders_to_fetch_from(test_list[test]):
                all_suffixes.update(test_list[test][builder])
            # FIXME: We should propagate the platform options as well.
            self._run_webkit_patch(['optimize-baselines', '--suffixes', ','.join(all_suffixes), test], verbose)

    def _rebaseline(self, options, test_list):
        for test, builders_to_check in sorted(test_list.items()):
            _log.info("Rebaselining %s" % test)
            for builder, suffixes in sorted(builders_to_check.items()):
                _log.debug("  %s: %s" % (builder, ",".join(suffixes)))

        commands = self._rebaseline_commands(test_list, options)
        command_results = self._tool.executive.run_in_parallel(commands)

        log_output = '\n'.join(result[2] for result in command_results).replace('\n\n', '\n')
        for line in log_output.split('\n'):
            if line:
                print >> sys.stderr, line  # FIXME: Figure out how to log properly.

        files_to_add = self._files_to_add(command_results)
        if files_to_add:
            self._tool.scm().add_list(list(files_to_add))

        if options.optimize:
            self._optimize_baselines(test_list, options.verbose)


class RebaselineJson(AbstractParallelRebaselineCommand):
    name = "rebaseline-json"
    help_text = "Rebaseline based off JSON passed to stdin. Intended to only be called from other scripts."

    def __init__(self,):
        super(RebaselineJson, self).__init__(options=[
            self.move_overwritten_baselines_option,
            self.no_optimize_option,
            self.results_directory_option,
            ])

    def execute(self, options, args, tool):
        self._rebaseline(options, json.loads(sys.stdin.read()))


class RebaselineExpectations(AbstractParallelRebaselineCommand):
    name = "rebaseline-expectations"
    help_text = "Rebaselines the tests indicated in TestExpectations."

    def __init__(self):
        super(RebaselineExpectations, self).__init__(options=[
            self.move_overwritten_baselines_option,
            self.no_optimize_option,
            ] + self.platform_options)
        self._test_list = None

    def _update_expectations_files(self, port_name):
        port = self._tool.port_factory.get(port_name)

        expectations = TestExpectations(port)
        for path in port.expectations_dict():
            if self._tool.filesystem.exists(path):
                self._tool.filesystem.write_text_file(path, expectations.remove_rebaselined_tests(expectations.get_rebaselining_failures(), path))

    def _tests_to_rebaseline(self, port):
        tests_to_rebaseline = {}
        expectations = TestExpectations(port, include_overrides=True)
        for test in expectations.get_rebaselining_failures():
            tests_to_rebaseline[test] = TestExpectations.suffixes_for_expectations(expectations.get_expectations(test))
        return tests_to_rebaseline

    def _add_tests_to_rebaseline_for_port(self, port_name):
        builder_name = builders.builder_name_for_port_name(port_name)
        if not builder_name:
            return
        tests = self._tests_to_rebaseline(self._tool.port_factory.get(port_name)).items()

        if tests:
            _log.info("Retrieving results for %s from %s." % (port_name, builder_name))

        for test_name, suffixes in tests:
            _log.info("    %s (%s)" % (test_name, ','.join(suffixes)))
            if test_name not in self._test_list:
                self._test_list[test_name] = {}
            self._test_list[test_name][builder_name] = suffixes

    def execute(self, options, args, tool):
        options.results_directory = None
        self._test_list = {}
        port_names = tool.port_factory.all_port_names(options.platform)
        for port_name in port_names:
            self._add_tests_to_rebaseline_for_port(port_name)
        if not self._test_list:
            _log.warning("Did not find any tests marked Rebaseline.")
            return

        self._rebaseline(options, self._test_list)

        for port_name in port_names:
            self._update_expectations_files(port_name)


class Rebaseline(AbstractParallelRebaselineCommand):
    name = "rebaseline"
    help_text = "Rebaseline tests with results from the build bots. Shows the list of failing tests on the builders if no test names are provided."
    argument_names = "[TEST_NAMES]"

    def __init__(self):
        super(Rebaseline, self).__init__(options=[
            self.move_overwritten_baselines_option,
            self.no_optimize_option,
            # FIXME: should we support the platform options in addition to (or instead of) --builders?
            self.suffixes_option,
            optparse.make_option("--builders", default=None, action="append", help="Comma-separated-list of builders to pull new baselines from (can also be provided multiple times)"),
            ])

    def _builders_to_pull_from(self):
        webkit_buildbot_builder_names = []
        for name in builders.all_builder_names():
            webkit_buildbot_builder_names.append(name)

        titles = ["build.webkit.org bots"]
        lists = [webkit_buildbot_builder_names]

        chosen_names = self._tool.user.prompt_with_multiple_lists("Which builder to pull results from:", titles, lists, can_choose_multiple=True)
        return [self._builder_with_name(name) for name in chosen_names]

    def _builder_with_name(self, name):
        return self._tool.buildbot.builder_with_name(name)

    def _tests_to_update(self, builder):
        failing_tests = builder.latest_layout_test_results().tests_matching_failure_types([test_failures.FailureTextMismatch])
        return self._tool.user.prompt_with_list("Which test(s) to rebaseline for %s:" % builder.name(), failing_tests, can_choose_multiple=True)

    def execute(self, options, args, tool):
        options.results_directory = None
        if options.builders:
            builders_to_check = []
            for builder_names in options.builders:
                builders_to_check += [self._builder_with_name(name) for name in builder_names.split(",")]
        else:
            builders_to_check = self._builders_to_pull_from()

        test_list = {}
        suffixes_to_update = options.suffixes.split(",")

        for builder in builders_to_check:
            tests = args or self._tests_to_update(builder)
            for test in tests:
                if test not in test_list:
                    test_list[test] = {}
                test_list[test][builder.name()] = suffixes_to_update

        if options.verbose:
            _log.debug("rebaseline-json: " + str(test_list))

        self._rebaseline(options, test_list)
