# Copyright (C) 2010 Google Inc. All rights reserved.
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

from webkitpy.common.checkout.changelog import ChangeLog
from webkitpy.common.config import urls
from webkitpy.tool.grammar import join_with_separators
from webkitpy.tool.steps.abstractstep import AbstractStep


class PrepareChangeLogForRevert(AbstractStep):
    @classmethod
    def _message_for_revert(cls, revision_list, reason, bug_url=None):
        message = "Unreviewed, rolling out %s.\n" % join_with_separators(['r' + str(revision) for revision in revision_list])
        for revision in revision_list:
            message += "%s\n" % urls.view_revision_url(revision)
        if bug_url:
            message += "%s\n" % bug_url
        # Add an extra new line after the rollout links, before any reason.
        message += "\n"
        if reason:
            message += "%s\n\n" % reason
        return message

    def run(self, state):
        # This could move to prepare-ChangeLog by adding a --revert= option.
        self._tool.executive.run_and_throw_if_fail(self._tool.deprecated_port().prepare_changelog_command(), cwd=self._tool.scm().checkout_root)
        changelog_paths = self._tool.checkout().modified_changelogs(git_commit=None)
        bug_url = self._tool.bugs.bug_url_for_bug_id(state["bug_id"]) if state["bug_id"] else None
        message = self._message_for_revert(state["revision_list"], state["reason"], bug_url)
        for changelog_path in changelog_paths:
            # FIXME: Seems we should prepare the message outside of changelogs.py and then just pass in
            # text that we want to use to replace the reviewed by line.
            ChangeLog(changelog_path).update_with_unreviewed_message(message)
