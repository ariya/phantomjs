# Copyright (C) 2011 Google Inc. All rights reserved.
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

from webkitpy.common.system import logutils
from webkitpy.tool.steps.abstractstep import AbstractStep
from webkitpy.tool.steps.options import Options


_log = logutils.get_logger(__file__)


class ApplyWatchList(AbstractStep):
    @classmethod
    def options(cls):
        return AbstractStep.options() + [
            Options.git_commit,
        ]

    def run(self, state):
        diff = self.cached_lookup(state, 'diff')
        bug_id = state.get('bug_id')

        cc_and_messages = self._tool.watch_list().determine_cc_and_messages(diff)
        cc_emails = cc_and_messages['cc_list']
        messages = cc_and_messages['messages']
        if bug_id:
            # Remove emails and cc's which are already in the bug or the reporter.
            bug = self._tool.bugs.fetch_bug(bug_id)

            messages = filter(lambda message: not bug.is_in_comments(message), messages)
            cc_emails = set(cc_emails).difference(bug.cc_emails())
            cc_emails.discard(bug.reporter_email())

        comment_text = '\n\n'.join(messages)
        if bug_id:
            if cc_emails or comment_text:
                self._tool.bugs.post_comment_to_bug(bug_id, comment_text, cc_emails)
            log_result = _log.debug
        else:
            _log.info('No bug was updated because no id was given.')
            log_result = _log.info
        log_result('Result of watchlist: cc "%s" messages "%s"' % (', '.join(cc_emails), comment_text))
