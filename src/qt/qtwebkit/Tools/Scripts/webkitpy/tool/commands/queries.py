# Copyright (c) 2009 Google Inc. All rights reserved.
# Copyright (c) 2009 Apple Inc. All rights reserved.
# Copyright (c) 2012 Intel Corporation. All rights reserved.
# Copyright (c) 2013 University of Szeged. All rights reserved.
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

import fnmatch
import logging
import re

from datetime import datetime
from optparse import make_option

from webkitpy.tool import steps

from webkitpy.common.checkout.commitinfo import CommitInfo
from webkitpy.common.config.committers import CommitterList
import webkitpy.common.config.urls as config_urls
from webkitpy.common.net.buildbot import BuildBot
from webkitpy.common.net.bugzilla import Bugzilla
from webkitpy.common.net.regressionwindow import RegressionWindow
from webkitpy.common.system.crashlogs import CrashLogs
from webkitpy.common.system.user import User
from webkitpy.tool.commands.abstractsequencedcommand import AbstractSequencedCommand
from webkitpy.tool.grammar import pluralize
from webkitpy.tool.multicommandtool import Command
from webkitpy.layout_tests.models.test_expectations import TestExpectations
from webkitpy.port import platform_options, configuration_options

_log = logging.getLogger(__name__)


class SuggestReviewers(AbstractSequencedCommand):
    name = "suggest-reviewers"
    help_text = "Suggest reviewers for a patch based on recent changes to the modified files."
    steps = [
        steps.SuggestReviewers,
    ]

    def _prepare_state(self, options, args, tool):
        options.suggest_reviewers = True


class BugsToCommit(Command):
    name = "bugs-to-commit"
    help_text = "List bugs in the commit-queue"

    def execute(self, options, args, tool):
        # FIXME: This command is poorly named.  It's fetching the commit-queue list here.  The name implies it's fetching pending-commit (all r+'d patches).
        bug_ids = tool.bugs.queries.fetch_bug_ids_from_commit_queue()
        for bug_id in bug_ids:
            print "%s" % bug_id


class PatchesInCommitQueue(Command):
    name = "patches-in-commit-queue"
    help_text = "List patches in the commit-queue"

    def execute(self, options, args, tool):
        patches = tool.bugs.queries.fetch_patches_from_commit_queue()
        _log.info("Patches in commit queue:")
        for patch in patches:
            print patch.url()


class PatchesToCommitQueue(Command):
    name = "patches-to-commit-queue"
    help_text = "Patches which should be added to the commit queue"
    def __init__(self):
        options = [
            make_option("--bugs", action="store_true", dest="bugs", help="Output bug links instead of patch links"),
        ]
        Command.__init__(self, options=options)

    @staticmethod
    def _needs_commit_queue(patch):
        if patch.commit_queue() == "+": # If it's already cq+, ignore the patch.
            _log.info("%s already has cq=%s" % (patch.id(), patch.commit_queue()))
            return False

        # We only need to worry about patches from contributers who are not yet committers.
        committer_record = CommitterList().committer_by_email(patch.attacher_email())
        if committer_record:
            _log.info("%s committer = %s" % (patch.id(), committer_record))
        return not committer_record

    def execute(self, options, args, tool):
        patches = tool.bugs.queries.fetch_patches_from_pending_commit_list()
        patches_needing_cq = filter(self._needs_commit_queue, patches)
        if options.bugs:
            bugs_needing_cq = map(lambda patch: patch.bug_id(), patches_needing_cq)
            bugs_needing_cq = sorted(set(bugs_needing_cq))
            for bug_id in bugs_needing_cq:
                print "%s" % tool.bugs.bug_url_for_bug_id(bug_id)
        else:
            for patch in patches_needing_cq:
                print "%s" % tool.bugs.attachment_url_for_id(patch.id(), action="edit")


class PatchesToReview(Command):
    name = "patches-to-review"
    help_text = "List bugs which have attachments pending review"

    def __init__(self):
        options = [
            make_option("--all", action="store_true",
                        help="Show all bugs regardless of who is on CC (it might take a while)"),
            make_option("--include-cq-denied", action="store_true",
                        help="By default, r? patches with cq- are omitted unless this option is set"),
            make_option("--cc-email",
                        help="Specifies the email on the CC field (defaults to your bugzilla login email)"),
        ]
        Command.__init__(self, options=options)

    def _print_report(self, report, cc_email, print_all):
        if print_all:
            print "Bugs with attachments pending review:"
        else:
            print "Bugs with attachments pending review that has %s in the CC list:" % cc_email

        print "http://webkit.org/b/bugid   Description (age in days)"
        for row in report:
            print "%s (%d)" % (row[1], row[0])

        print "Total: %d" % len(report)

    def _generate_report(self, bugs, include_cq_denied):
        report = []

        for bug in bugs:
            patch = bug.unreviewed_patches()[-1]

            if not include_cq_denied and patch.commit_queue() == "-":
                continue

            age_in_days = (datetime.today() - patch.attach_date()).days
            report.append((age_in_days, "http://webkit.org/b/%-7s %s" % (bug.id(), bug.title())))

        report.sort()
        return report

    def execute(self, options, args, tool):
        tool.bugs.authenticate()

        cc_email = options.cc_email
        if not cc_email and not options.all:
            cc_email = tool.bugs.username

        bugs = tool.bugs.queries.fetch_bugs_from_review_queue(cc_email=cc_email)
        report = self._generate_report(bugs, options.include_cq_denied)
        self._print_report(report, cc_email, options.all)


class WhatBroke(Command):
    name = "what-broke"
    help_text = "Print failing buildbots (%s) and what revisions broke them" % config_urls.buildbot_url

    def _print_builder_line(self, builder_name, max_name_width, status_message):
        print "%s : %s" % (builder_name.ljust(max_name_width), status_message)

    def _print_blame_information_for_builder(self, builder_status, name_width, avoid_flakey_tests=True):
        builder = self._tool.buildbot.builder_with_name(builder_status["name"])
        red_build = builder.build(builder_status["build_number"])
        regression_window = builder.find_regression_window(red_build)
        if not regression_window.failing_build():
            self._print_builder_line(builder.name(), name_width, "FAIL (error loading build information)")
            return
        if not regression_window.build_before_failure():
            self._print_builder_line(builder.name(), name_width, "FAIL (blame-list: sometime before %s?)" % regression_window.failing_build().revision())
            return

        revisions = regression_window.revisions()
        first_failure_message = ""
        if (regression_window.failing_build() == builder.build(builder_status["build_number"])):
            first_failure_message = " FIRST FAILURE, possibly a flaky test"
        self._print_builder_line(builder.name(), name_width, "FAIL (blame-list: %s%s)" % (revisions, first_failure_message))
        for revision in revisions:
            commit_info = self._tool.checkout().commit_info_for_revision(revision)
            if commit_info:
                print commit_info.blame_string(self._tool.bugs)
            else:
                print "FAILED to fetch CommitInfo for r%s, likely missing ChangeLog" % revision

    def execute(self, options, args, tool):
        builder_statuses = tool.buildbot.builder_statuses()
        longest_builder_name = max(map(len, map(lambda builder: builder["name"], builder_statuses)))
        failing_builders = 0
        for builder_status in builder_statuses:
            # If the builder is green, print OK, exit.
            if builder_status["is_green"]:
                continue
            self._print_blame_information_for_builder(builder_status, name_width=longest_builder_name)
            failing_builders += 1
        if failing_builders:
            print "%s of %s are failing" % (failing_builders, pluralize("builder", len(builder_statuses)))
        else:
            print "All builders are passing!"


class ResultsFor(Command):
    name = "results-for"
    help_text = "Print a list of failures for the passed revision from bots on %s" % config_urls.buildbot_url
    argument_names = "REVISION"

    def _print_layout_test_results(self, results):
        if not results:
            print " No results."
            return
        for title, files in results.parsed_results().items():
            print " %s" % title
            for filename in files:
                print "  %s" % filename

    def execute(self, options, args, tool):
        builders = self._tool.buildbot.builders()
        for builder in builders:
            print "%s:" % builder.name()
            build = builder.build_for_revision(args[0], allow_failed_lookups=True)
            self._print_layout_test_results(build.layout_test_results())


class FailureReason(Command):
    name = "failure-reason"
    help_text = "Lists revisions where individual test failures started at %s" % config_urls.buildbot_url

    def _blame_line_for_revision(self, revision):
        try:
            commit_info = self._tool.checkout().commit_info_for_revision(revision)
        except Exception, e:
            return "FAILED to fetch CommitInfo for r%s, exception: %s" % (revision, e)
        if not commit_info:
            return "FAILED to fetch CommitInfo for r%s, likely missing ChangeLog" % revision
        return commit_info.blame_string(self._tool.bugs)

    def _print_blame_information_for_transition(self, regression_window, failing_tests):
        red_build = regression_window.failing_build()
        print "SUCCESS: Build %s (r%s) was the first to show failures: %s" % (red_build._number, red_build.revision(), failing_tests)
        print "Suspect revisions:"
        for revision in regression_window.revisions():
            print self._blame_line_for_revision(revision)

    def _explain_failures_for_builder(self, builder, start_revision):
        print "Examining failures for \"%s\", starting at r%s" % (builder.name(), start_revision)
        revision_to_test = start_revision
        build = builder.build_for_revision(revision_to_test, allow_failed_lookups=True)
        layout_test_results = build.layout_test_results()
        if not layout_test_results:
            # FIXME: This could be made more user friendly.
            print "Failed to load layout test results from %s; can't continue. (start revision = r%s)" % (build.results_url(), start_revision)
            return 1

        results_to_explain = set(layout_test_results.failing_tests())
        last_build_with_results = build
        print "Starting at %s" % revision_to_test
        while results_to_explain:
            revision_to_test -= 1
            new_build = builder.build_for_revision(revision_to_test, allow_failed_lookups=True)
            if not new_build:
                print "No build for %s" % revision_to_test
                continue
            build = new_build
            latest_results = build.layout_test_results()
            if not latest_results:
                print "No results build %s (r%s)" % (build._number, build.revision())
                continue
            failures = set(latest_results.failing_tests())
            if len(failures) >= 20:
                # FIXME: We may need to move this logic into the LayoutTestResults class.
                # The buildbot stops runs after 20 failures so we don't have full results to work with here.
                print "Too many failures in build %s (r%s), ignoring." % (build._number, build.revision())
                continue
            fixed_results = results_to_explain - failures
            if not fixed_results:
                print "No change in build %s (r%s), %s unexplained failures (%s in this build)" % (build._number, build.revision(), len(results_to_explain), len(failures))
                last_build_with_results = build
                continue
            regression_window = RegressionWindow(build, last_build_with_results)
            self._print_blame_information_for_transition(regression_window, fixed_results)
            last_build_with_results = build
            results_to_explain -= fixed_results
        if results_to_explain:
            print "Failed to explain failures: %s" % results_to_explain
            return 1
        print "Explained all results for %s" % builder.name()
        return 0

    def _builder_to_explain(self):
        builder_statuses = self._tool.buildbot.builder_statuses()
        red_statuses = [status for status in builder_statuses if not status["is_green"]]
        print "%s failing" % (pluralize("builder", len(red_statuses)))
        builder_choices = [status["name"] for status in red_statuses]
        # We could offer an "All" choice here.
        chosen_name = self._tool.user.prompt_with_list("Which builder to diagnose:", builder_choices)
        # FIXME: prompt_with_list should really take a set of objects and a set of names and then return the object.
        for status in red_statuses:
            if status["name"] == chosen_name:
                return (self._tool.buildbot.builder_with_name(chosen_name), status["built_revision"])

    def execute(self, options, args, tool):
        (builder, latest_revision) = self._builder_to_explain()
        start_revision = self._tool.user.prompt("Revision to walk backwards from? [%s] " % latest_revision) or latest_revision
        if not start_revision:
            print "Revision required."
            return 1
        return self._explain_failures_for_builder(builder, start_revision=int(start_revision))


class FindFlakyTests(Command):
    name = "find-flaky-tests"
    help_text = "Lists tests that often fail for a single build at %s" % config_urls.buildbot_url

    def _find_failures(self, builder, revision):
        build = builder.build_for_revision(revision, allow_failed_lookups=True)
        if not build:
            print "No build for %s" % revision
            return (None, None)
        results = build.layout_test_results()
        if not results:
            print "No results build %s (r%s)" % (build._number, build.revision())
            return (None, None)
        failures = set(results.failing_tests())
        if len(failures) >= 20:
            # FIXME: We may need to move this logic into the LayoutTestResults class.
            # The buildbot stops runs after 20 failures so we don't have full results to work with here.
            print "Too many failures in build %s (r%s), ignoring." % (build._number, build.revision())
            return (None, None)
        return (build, failures)

    def _increment_statistics(self, flaky_tests, flaky_test_statistics):
        for test in flaky_tests:
            count = flaky_test_statistics.get(test, 0)
            flaky_test_statistics[test] = count + 1

    def _print_statistics(self, statistics):
        print "=== Results ==="
        print "Occurances Test name"
        for value, key in sorted([(value, key) for key, value in statistics.items()]):
            print "%10d %s" % (value, key)

    def _walk_backwards_from(self, builder, start_revision, limit):
        flaky_test_statistics = {}
        all_previous_failures = set([])
        one_time_previous_failures = set([])
        previous_build = None
        for i in range(limit):
            revision = start_revision - i
            print "Analyzing %s ... " % revision,
            (build, failures) = self._find_failures(builder, revision)
            if failures == None:
                # Notice that we don't loop on the empty set!
                continue
            print "has %s failures" % len(failures)
            flaky_tests = one_time_previous_failures - failures
            if flaky_tests:
                print "Flaky tests: %s %s" % (sorted(flaky_tests),
                                              previous_build.results_url())
            self._increment_statistics(flaky_tests, flaky_test_statistics)
            one_time_previous_failures = failures - all_previous_failures
            all_previous_failures = failures
            previous_build = build
        self._print_statistics(flaky_test_statistics)

    def _builder_to_analyze(self):
        statuses = self._tool.buildbot.builder_statuses()
        choices = [status["name"] for status in statuses]
        chosen_name = self._tool.user.prompt_with_list("Which builder to analyze:", choices)
        for status in statuses:
            if status["name"] == chosen_name:
                return (self._tool.buildbot.builder_with_name(chosen_name), status["built_revision"])

    def execute(self, options, args, tool):
        (builder, latest_revision) = self._builder_to_analyze()
        limit = self._tool.user.prompt("How many revisions to look through? [10000] ") or 10000
        return self._walk_backwards_from(builder, latest_revision, limit=int(limit))


class TreeStatus(Command):
    name = "tree-status"
    help_text = "Print the status of the %s buildbots" % config_urls.buildbot_url
    long_help = """Fetches build status from http://build.webkit.org/one_box_per_builder
and displayes the status of each builder."""

    def execute(self, options, args, tool):
        for builder in tool.buildbot.builder_statuses():
            status_string = "ok" if builder["is_green"] else "FAIL"
            print "%s : %s" % (status_string.ljust(4), builder["name"])


class CrashLog(Command):
    name = "crash-log"
    help_text = "Print the newest crash log for the given process"
    long_help = """Finds the newest crash log matching the given process name
and PID and prints it to stdout."""
    argument_names = "PROCESS_NAME [PID]"

    def execute(self, options, args, tool):
        crash_logs = CrashLogs(tool)
        pid = None
        if len(args) > 1:
            pid = int(args[1])
        print crash_logs.find_newest_log(args[0], pid)


class PrintExpectations(Command):
    name = 'print-expectations'
    help_text = 'Print the expected result for the given test(s) on the given port(s)'

    def __init__(self):
        options = [
            make_option('--all', action='store_true', default=False,
                        help='display the expectations for *all* tests'),
            make_option('-x', '--exclude-keyword', action='append', default=[],
                        help='limit to tests not matching the given keyword (for example, "skip", "slow", or "crash". May specify multiple times'),
            make_option('-i', '--include-keyword', action='append', default=[],
                        help='limit to tests with the given keyword (for example, "skip", "slow", or "crash". May specify multiple times'),
            make_option('--csv', action='store_true', default=False,
                        help='Print a CSV-style report that includes the port name, modifiers, tests, and expectations'),
            make_option('-f', '--full', action='store_true', default=False,
                        help='Print a full TestExpectations-style line for every match'),
            make_option('--paths', action='store_true', default=False,
                        help='display the paths for all applicable expectation files'),
        ] + platform_options(use_globs=True)

        Command.__init__(self, options=options)
        self._expectation_models = {}

    def execute(self, options, args, tool):
        if not options.paths and not args and not options.all:
            print "You must either specify one or more test paths or --all."
            return

        if options.platform:
            port_names = fnmatch.filter(tool.port_factory.all_port_names(), options.platform)
            if not port_names:
                default_port = tool.port_factory.get(options.platform)
                if default_port:
                    port_names = [default_port.name()]
                else:
                    print "No port names match '%s'" % options.platform
                    return
            else:
                default_port = tool.port_factory.get(port_names[0])
        else:
            default_port = tool.port_factory.get(options=options)
            port_names = [default_port.name()]

        if options.paths:
            files = default_port.expectations_files()
            layout_tests_dir = default_port.layout_tests_dir()
            for file in files:
                if file.startswith(layout_tests_dir):
                    file = file.replace(layout_tests_dir, 'LayoutTests')
                print file
            return

        tests = set(default_port.tests(args))
        for port_name in port_names:
            model = self._model(options, port_name, tests)
            tests_to_print = self._filter_tests(options, model, tests)
            lines = [model.get_expectation_line(test) for test in sorted(tests_to_print)]
            if port_name != port_names[0]:
                print
            print '\n'.join(self._format_lines(options, port_name, lines))

    def _filter_tests(self, options, model, tests):
        filtered_tests = set()
        if options.include_keyword:
            for keyword in options.include_keyword:
                filtered_tests.update(model.get_test_set_for_keyword(keyword))
        else:
            filtered_tests = tests

        for keyword in options.exclude_keyword:
            filtered_tests.difference_update(model.get_test_set_for_keyword(keyword))
        return filtered_tests

    def _format_lines(self, options, port_name, lines):
        output = []
        if options.csv:
            for line in lines:
                output.append("%s,%s" % (port_name, line.to_csv()))
        elif lines:
            include_modifiers = options.full
            include_expectations = options.full or len(options.include_keyword) != 1 or len(options.exclude_keyword)
            output.append("// For %s" % port_name)
            for line in lines:
                output.append("%s" % line.to_string(None, include_modifiers, include_expectations, include_comment=False))
        return output

    def _model(self, options, port_name, tests):
        port = self._tool.port_factory.get(port_name, options)
        return TestExpectations(port, tests).model()


class PrintBaselines(Command):
    name = 'print-baselines'
    help_text = 'Prints the baseline locations for given test(s) on the given port(s)'

    def __init__(self):
        options = [
            make_option('--all', action='store_true', default=False,
                        help='display the baselines for *all* tests'),
            make_option('--csv', action='store_true', default=False,
                        help='Print a CSV-style report that includes the port name, test_name, test platform, baseline type, baseline location, and baseline platform'),
            make_option('--include-virtual-tests', action='store_true',
                        help='Include virtual tests'),
        ] + platform_options(use_globs=True)
        Command.__init__(self, options=options)
        self._platform_regexp = re.compile('platform/([^\/]+)/(.+)')

    def execute(self, options, args, tool):
        if not args and not options.all:
            print "You must either specify one or more test paths or --all."
            return

        default_port = tool.port_factory.get()
        if options.platform:
            port_names = fnmatch.filter(tool.port_factory.all_port_names(), options.platform)
            if not port_names:
                print "No port names match '%s'" % options.platform
        else:
            port_names = [default_port.name()]

        if options.include_virtual_tests:
            tests = sorted(default_port.tests(args))
        else:
            # FIXME: make real_tests() a public method.
            tests = sorted(default_port._real_tests(args))

        for port_name in port_names:
            if port_name != port_names[0]:
                print
            if not options.csv:
                print "// For %s" % port_name
            port = tool.port_factory.get(port_name)
            for test_name in tests:
                self._print_baselines(options, port_name, test_name, port.expected_baselines_by_extension(test_name))

    def _print_baselines(self, options, port_name, test_name, baselines):
        for extension in sorted(baselines.keys()):
            baseline_location = baselines[extension]
            if baseline_location:
                if options.csv:
                    print "%s,%s,%s,%s,%s,%s" % (port_name, test_name, self._platform_for_path(test_name),
                                                 extension[1:], baseline_location, self._platform_for_path(baseline_location))
                else:
                    print baseline_location

    def _platform_for_path(self, relpath):
        platform_matchobj = self._platform_regexp.match(relpath)
        if platform_matchobj:
            return platform_matchobj.group(1)
        return None


class FindResolvedBugs(Command):
    name = "find-resolved-bugs"
    help_text = "Collect the RESOLVED bugs in the given TestExpectations file"
    argument_names = "TEST_EXPECTATIONS_FILE"

    def execute(self, options, args, tool):
        filename = args[0]
        if not tool.filesystem.isfile(filename):
            print "The given path is not a file, please pass a valid path."
            return

        ids = set()
        inputfile = tool.filesystem.open_text_file_for_reading(filename)
        for line in inputfile:
            result = re.search("(https://bugs\.webkit\.org/show_bug\.cgi\?id=|webkit\.org/b/)([0-9]+)", line)
            if result:
                ids.add(result.group(2))
        inputfile.close()

        resolved_ids = set()
        num_of_bugs = len(ids)
        bugzilla = Bugzilla()
        for i, bugid in enumerate(ids, start=1):
            bug = bugzilla.fetch_bug(bugid)
            print "Checking bug %s \t [%d/%d]" % (bugid, i, num_of_bugs)
            if not bug.is_open():
                resolved_ids.add(bugid)

        print "Resolved bugs in %s :" % (filename)
        for bugid in resolved_ids:
            print "https://bugs.webkit.org/show_bug.cgi?id=%s" % (bugid)
