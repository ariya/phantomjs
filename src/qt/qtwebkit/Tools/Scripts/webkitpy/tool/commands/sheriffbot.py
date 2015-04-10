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

import logging

from webkitpy.tool.bot.sheriff import Sheriff
from webkitpy.tool.bot.irc_command import commands as irc_commands
from webkitpy.tool.bot.ircbot import IRCBot
from webkitpy.tool.commands.queues import AbstractQueue
from webkitpy.tool.commands.stepsequence import StepSequenceErrorHandler

_log = logging.getLogger(__name__)


class SheriffBot(AbstractQueue, StepSequenceErrorHandler):
    name = "webkitbot"
    watchers = AbstractQueue.watchers + [
        "abarth@webkit.org",
        "eric@webkit.org",
    ]

    # AbstractQueue methods

    def begin_work_queue(self):
        AbstractQueue.begin_work_queue(self)
        self._sheriff = Sheriff(self._tool, self)
        self._irc_bot = IRCBot(self.name, self._tool, self._sheriff, irc_commands)
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
