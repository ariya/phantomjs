# Copyright (c) 2009 Google Inc. All rights reserved.
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
from optparse import make_option

from webkitpy.common.config.committers import CommitterList
from webkitpy.common.config.ports import DeprecatedPort
from webkitpy.common.system.filesystem import FileSystem
from webkitpy.common.system.executive import ScriptError
from webkitpy.tool.bot.earlywarningsystemtask import EarlyWarningSystemTask, EarlyWarningSystemTaskDelegate
from webkitpy.tool.bot.expectedfailures import ExpectedFailures
from webkitpy.tool.bot.layouttestresultsreader import LayoutTestResultsReader
from webkitpy.tool.bot.patchanalysistask import UnableToApplyPatch
from webkitpy.tool.bot.queueengine import QueueEngine
from webkitpy.tool.commands.queues import AbstractReviewQueue

_log = logging.getLogger(__name__)


class AbstractEarlyWarningSystem(AbstractReviewQueue, EarlyWarningSystemTaskDelegate):
    _build_style = "release"
    # FIXME: Switch _default_run_tests from opt-in to opt-out once more bots are ready to run tests.
    run_tests = False

    def __init__(self):
        options = [make_option("--run-tests", action="store_true", dest="run_tests", default=self.run_tests, help="Run the Layout tests for each patch")]
        AbstractReviewQueue.__init__(self, options=options)

    def begin_work_queue(self):
        AbstractReviewQueue.begin_work_queue(self)
        self._expected_failures = ExpectedFailures()
        self._layout_test_results_reader = LayoutTestResultsReader(self._tool, self._port.results_directory(), self._log_directory())

    def _failing_tests_message(self, task, patch):
        results = task.results_from_patch_test_run(patch)
        unexpected_failures = self._expected_failures.unexpected_failures_observed(results)
        if not unexpected_failures:
            return None
        return "New failing tests:\n%s" % "\n".join(unexpected_failures)

    def _post_reject_message_on_bug(self, tool, patch, status_id, extra_message_text=None):
        results_link = tool.status_server.results_url_for_status(status_id)
        message = "Attachment %s did not pass %s (%s):\nOutput: %s" % (patch.id(), self.name, self.port_name, results_link)
        if extra_message_text:
            message += "\n\n%s" % extra_message_text
        # FIXME: We might want to add some text about rejecting from the commit-queue in
        # the case where patch.commit_queue() isn't already set to '-'.
        if self.watchers:
            tool.bugs.add_cc_to_bug(patch.bug_id(), self.watchers)
        tool.bugs.set_flag_on_attachment(patch.id(), "commit-queue", "-", message)

    def review_patch(self, patch):
        task = EarlyWarningSystemTask(self, patch, self._options.run_tests)
        if not task.validate():
            self._did_error(patch, "%s did not process patch." % self.name)
            return False
        try:
            return task.run()
        except UnableToApplyPatch, e:
            self._did_error(patch, "%s unable to apply patch." % self.name)
            return False
        except ScriptError, e:
            self._post_reject_message_on_bug(self._tool, patch, task.failure_status_id, self._failing_tests_message(task, patch))
            results_archive = task.results_archive_from_patch_test_run(patch)
            if results_archive:
                self._upload_results_archive_for_patch(patch, results_archive)
            self._did_fail(patch)
            # FIXME: We're supposed to be able to raise e again here and have
            # one of our base classes mark the patch as fail, but there seems
            # to be an issue with the exit_code.
            return False

    # EarlyWarningSystemDelegate methods

    def parent_command(self):
        return self.name

    def run_command(self, command):
        self.run_webkit_patch(command + [self._deprecated_port.flag()])

    def command_passed(self, message, patch):
        pass

    def command_failed(self, message, script_error, patch):
        failure_log = self._log_from_script_error_for_upload(script_error)
        return self._update_status(message, patch=patch, results_file=failure_log)

    def expected_failures(self):
        return self._expected_failures

    def test_results(self):
        return self._layout_test_results_reader.results()

    def archive_last_test_results(self, patch):
        return self._layout_test_results_reader.archive(patch)

    def build_style(self):
        return self._build_style

    def refetch_patch(self, patch):
        return self._tool.bugs.fetch_attachment(patch.id())

    def report_flaky_tests(self, patch, flaky_test_results, results_archive):
        pass

    # StepSequenceErrorHandler methods

    @classmethod
    def handle_script_error(cls, tool, state, script_error):
        # FIXME: Why does this not exit(1) like the superclass does?
        _log.error(script_error.message_with_output())

    @classmethod
    def load_ews_classes(cls):
        filesystem = FileSystem()
        json_path = filesystem.join(filesystem.dirname(filesystem.path_to_module('webkitpy.common.config')), 'ews.json')
        try:
            ewses = json.loads(filesystem.read_text_file(json_path))
        except ValueError:
            return None

        classes = []
        for name, config in ewses.iteritems():
            classes.append(type(str(name.replace(' ', '')), (AbstractEarlyWarningSystem,), {
                'name': config['port'] + '-ews',
                'port_name': config['port'],
                'watchers': config.get('watchers', []),
                'run_tests': config.get('runTests', cls.run_tests),
            }))
        return classes
