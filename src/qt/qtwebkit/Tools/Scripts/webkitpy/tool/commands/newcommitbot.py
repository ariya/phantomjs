# Copyright (c) 2012 Google Inc. All rights reserved.
# Copyright (c) 2013 Apple Inc. All rights reserved.
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
import re

from webkitpy.common.config.committers import CommitterList
from webkitpy.common.system.executive import ScriptError
from webkitpy.tool.bot.irc_command import IRCCommand
from webkitpy.tool.bot.irc_command import Help
from webkitpy.tool.bot.irc_command import Hi
from webkitpy.tool.bot.irc_command import PingPong
from webkitpy.tool.bot.irc_command import Restart
from webkitpy.tool.bot.irc_command import YouThere
from webkitpy.tool.bot.ircbot import IRCBot
from webkitpy.tool.commands.queues import AbstractQueue
from webkitpy.tool.commands.stepsequence import StepSequenceErrorHandler

_log = logging.getLogger(__name__)


class Agent(object):
    def __init__(self, tool, newcommitbot):
        self._tool = tool
        self._newcommitbot = newcommitbot

    def name(self):
        return 'WKR'


class NewCommitBot(AbstractQueue, StepSequenceErrorHandler):
    name = "WKR"
    watchers = AbstractQueue.watchers + ["rniwa@webkit.org"]

    _commands = {
        "hi": Hi,
        "ping": PingPong,
        "restart": Restart,
        "yt?": YouThere,
    }

    _maximum_number_of_revisions_to_avoid_spamming_irc = 10

    # AbstractQueue methods

    def begin_work_queue(self):
        AbstractQueue.begin_work_queue(self)
        self._last_svn_revision = int(self._tool.scm().head_svn_revision())
        self._irc_bot = IRCBot(self.name, self._tool, Agent(self._tool, self), self._commands)
        self._tool.ensure_irc_connected(self._irc_bot.irc_delegate())

    def work_item_log_path(self, failure_map):
        return None

    def next_work_item(self):
        self._irc_bot.process_pending_messages()

        _log.info('Last SVN revision: %d' % self._last_svn_revision)

        count = 0
        while count < self._maximum_number_of_revisions_to_avoid_spamming_irc:
            new_revision = self._last_svn_revision + 1
            try:
                commit_log = self._tool.executive.run_command(['svn', 'log', 'https://svn.webkit.org/repository/webkit/trunk', '--non-interactive', '--revision',
                    self._tool.scm().strip_r_from_svn_revision(new_revision)])
            except ScriptError:
                break

            self._last_svn_revision = new_revision
            if self._is_empty_log(commit_log):
                continue

            count += 1
            _log.info('Found revision %d' % new_revision)
            self._tool.irc().post(self._summarize_commit_log(commit_log).encode('utf-8'))

    def _is_empty_log(self, commit_log):
        return re.match(r'^\-+$', commit_log)

    def process_work_item(self, failure_map):
        return True

    _patch_by_regex = re.compile(r'^Patch\s+by\s+(?P<author>.+?)\s+on(\s+\d{4}-\d{2}-\d{2})?\n?', re.MULTILINE | re.IGNORECASE)
    _rollout_regex = re.compile(r'(rolling out|reverting) (?P<revisions>r?\d+((,\s*|,?\s*and\s+)?r?\d+)*)\.?\s*', re.MULTILINE | re.IGNORECASE)
    _requested_by_regex = re.compile(r'^\"?(?P<reason>.+?)\"? \(Requested\s+by\s+(?P<author>.+?)\s+on\s+#webkit\)\.', re.MULTILINE | re.IGNORECASE)
    _bugzilla_url_regex = re.compile(r'http(s?)://bugs\.webkit\.org/show_bug\.cgi\?id=(?P<id>\d+)', re.MULTILINE)
    _trac_url_regex = re.compile(r'http(s?)://trac.webkit.org/changeset/(?P<revision>\d+)', re.MULTILINE)

    @classmethod
    def _summarize_commit_log(self, commit_log, committer_list=CommitterList()):
        patch_by = self._patch_by_regex.search(commit_log)
        commit_log = self._patch_by_regex.sub('', commit_log, count=1)

        rollout = self._rollout_regex.search(commit_log)
        commit_log = self._rollout_regex.sub('', commit_log, count=1)

        requested_by = self._requested_by_regex.search(commit_log)

        commit_log = self._bugzilla_url_regex.sub(r'https://webkit.org/b/\g<id>', commit_log)
        commit_log = self._trac_url_regex.sub(r'https://trac.webkit.org/r\g<revision>', commit_log)

        for contributor in committer_list.contributors():
            if not contributor.irc_nicknames:
                continue
            name_with_nick = "%s (%s)" % (contributor.full_name, contributor.irc_nicknames[0])
            if contributor.full_name in commit_log:
                commit_log = commit_log.replace(contributor.full_name, name_with_nick)
                for email in contributor.emails:
                    commit_log = commit_log.replace(' <' + email + '>', '')
            else:
                for email in contributor.emails:
                    commit_log = commit_log.replace(email, name_with_nick)

        lines = commit_log.split('\n')[1:-2]  # Ignore lines with ----------.

        firstline = re.match(r'^(?P<revision>r\d+) \| (?P<email>[^\|]+) \| (?P<timestamp>[^|]+) \| [^\n]+', lines[0])
        assert firstline
        author = firstline.group('email')
        if patch_by:
            author = patch_by.group('author')

        linkified_revision = 'https://trac.webkit.org/%s' % firstline.group('revision')
        lines[0] = '%s by %s' % (linkified_revision, author)

        if rollout:
            if requested_by:
                author = requested_by.group('author')
                contributor = committer_list.contributor_by_irc_nickname(author)
                if contributor:
                    author = "%s (%s)" % (contributor.full_name, contributor.irc_nicknames[0])
                return '%s rolled out %s in %s : %s' % (author, rollout.group('revisions'),
                    linkified_revision, requested_by.group('reason'))
            lines[0] = '%s rolled out %s in %s' % (author, rollout.group('revisions'), linkified_revision)

        return ' '.join(filter(lambda line: len(line), lines)[0:4])

    def handle_unexpected_error(self, failure_map, message):
        _log.error(message)

    # StepSequenceErrorHandler methods

    @classmethod
    def handle_script_error(cls, tool, state, script_error):
        # Ideally we would post some information to IRC about what went wrong
        # here, but we don't have the IRC password in the child process.
        pass
