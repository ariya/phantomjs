# Copyright (c) 2012 Google Inc. All rights reserved.
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

import logging

from webkitpy.tool.bot.expectedfailures import ExpectedFailures
from webkitpy.tool.bot.irc_command import IRCCommand
from webkitpy.tool.bot.irc_command import Help
from webkitpy.tool.bot.irc_command import Hi
from webkitpy.tool.bot.irc_command import Restart
from webkitpy.tool.bot.ircbot import IRCBot
from webkitpy.tool.bot.patchanalysistask import PatchAnalysisTask, PatchAnalysisTaskDelegate, UnableToApplyPatch
from webkitpy.tool.bot.sheriff import Sheriff
from webkitpy.tool.commands.queues import AbstractQueue
from webkitpy.tool.commands.stepsequence import StepSequenceErrorHandler

_log = logging.getLogger(__name__)


class PerfalizerTask(PatchAnalysisTask):
    def __init__(self, tool, patch, logger):
        PatchAnalysisTask.__init__(self, self, patch)
        self._port = tool.port_factory.get()
        self._tool = tool
        self._logger = logger

    def _copy_build_product_without_patch(self):
        filesystem = self._tool.filesystem
        configuration = filesystem.basename(self._port._build_path())
        self._build_directory = filesystem.dirname(self._port._build_path())
        self._build_directory_without_patch = self._build_directory + 'WithoutPatch'

        try:
            filesystem.rmtree(self._build_directory_without_patch)
            filesystem.copytree(filesystem.join(self._build_directory, configuration),
                filesystem.join(self._build_directory_without_patch, configuration))
            return True
        except:
            return False

    def run(self):
        if not self._patch.committer() and not self._patch.attacher().can_commit:
            self._logger('The patch %d is not authorized by a commmitter' % self._patch.id())
            return False

        self._logger('Preparing to run performance tests for the attachment %d...' % self._patch.id())
        if not self._clean() or not self._update():
            return False

        head_revision = self._tool.scm().head_svn_revision()

        self._logger('Building WebKit at r%s without the patch' % head_revision)
        if not self._build_without_patch():
            return False

        if not self._port.check_build(needs_http=False):
            self._logger('Failed to build DumpRenderTree.')
            return False

        if not self._copy_build_product_without_patch():
            self._logger('Failed to copy the build product from %s to %s' % (self._build_directory, self._build_directory_without_patch))
            return False

        self._logger('Building WebKit at r%s with the patch' % head_revision)
        if not self._apply() or not self._build():
            return False

        if not self._port.check_build(needs_http=False):
            self._logger('Failed to build DumpRenderTree.')
            return False

        filesystem = self._tool.filesystem
        if filesystem.exists(self._json_path()):
            filesystem.remove(self._json_path())

        self._logger("Running performance tests...")
        if self._run_perf_test(self._build_directory_without_patch, 'without %d' % self._patch.id()) < 0:
            self._logger('Failed to run performance tests without the patch.')
            return False

        if self._run_perf_test(self._build_directory, 'with %d' % self._patch.id()) < 0:
            self._logger('Failed to run performance tests with the patch.')
            return False

        if not filesystem.exists(self._results_page_path()):
            self._logger('Failed to generate the results page.')
            return False

        results_page = filesystem.read_text_file(self._results_page_path())
        self._tool.bugs.add_attachment_to_bug(self._patch.bug_id(), results_page,
            description="Performance tests results for %d" % self._patch.id(), mimetype='text/html')

        self._logger("Uploaded the results on the bug %d" % self._patch.bug_id())
        return True

    def parent_command(self):
        return "perfalizer"

    def run_webkit_patch(self, args):
        webkit_patch_args = [self._tool.path()]
        webkit_patch_args.extend(args)
        return self._tool.executive.run_and_throw_if_fail(webkit_patch_args, cwd=self._tool.scm().checkout_root)

    def _json_path(self):
        return self._tool.filesystem.join(self._build_directory, 'PerformanceTestResults.json')

    def _results_page_path(self):
        return self._tool.filesystem.join(self._build_directory, 'PerformanceTestResults.html')

    def _run_perf_test(self, build_path, description):
        filesystem = self._tool.filesystem
        script_path = filesystem.join(filesystem.dirname(self._tool.path()), 'run-perf-tests')
        perf_test_runner_args = [script_path, '--no-build', '--no-show-results', '--build-directory', build_path,
            '--output-json-path', self._json_path(), '--description', description]
        return self._tool.executive.run_and_throw_if_fail(perf_test_runner_args, cwd=self._tool.scm().checkout_root)

    def run_command(self, command):
        self.run_webkit_patch(command)

    def command_passed(self, message, patch):
        pass

    def command_failed(self, message, script_error, patch):
        self._logger(message)

    def refetch_patch(self, patch):
        return self._tool.bugs.fetch_attachment(patch.id())

    def expected_failures(self):
        return ExpectedFailures()

    def build_style(self):
        return "release"


class PerfTest(IRCCommand):
    def execute(self, nick, args, tool, sheriff):
        if not args:
            tool.irc().post(nick + ": Please specify an attachment/patch id")
            return

        patch_id = args[0]
        patch = tool.bugs.fetch_attachment(patch_id)
        if not patch:
            tool.irc().post(nick + ": Could not fetch the patch")
            return

        task = PerfalizerTask(tool, patch, lambda message: tool.irc().post('%s: %s' % (nick, message)))
        task.run()


class Perfalizer(AbstractQueue, StepSequenceErrorHandler):
    name = "perfalizer"
    watchers = AbstractQueue.watchers + ["rniwa@webkit.org"]

    _commands = {
        "help": Help,
        "hi": Hi,
        "restart": Restart,
        "test": PerfTest,
    }

    # AbstractQueue methods

    def begin_work_queue(self):
        AbstractQueue.begin_work_queue(self)
        self._sheriff = Sheriff(self._tool, self)
        self._irc_bot = IRCBot("perfalizer", self._tool, self._sheriff, self._commands)
        self._tool.ensure_irc_connected(self._irc_bot.irc_delegate())

    def work_item_log_path(self, failure_map):
        return None

    def _is_old_failure(self, revision):
        return self._tool.status_server.svn_revision(revision)

    def next_work_item(self):
        self._irc_bot.process_pending_messages()
        return

    def process_work_item(self, failure_map):
        return True

    def handle_unexpected_error(self, failure_map, message):
        _log.error(message)

    # StepSequenceErrorHandler methods

    @classmethod
    def handle_script_error(cls, tool, state, script_error):
        # Ideally we would post some information to IRC about what went wrong
        # here, but we don't have the IRC password in the child process.
        pass
