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

import logging

from webkitpy.common.checkout.changelog import ChangeLog
from webkitpy.tool.grammar import pluralize
from webkitpy.tool.steps.abstractstep import AbstractStep
from webkitpy.tool.steps.options import Options

_log = logging.getLogger(__name__)


class UpdateChangeLogsWithReviewer(AbstractStep):
    @classmethod
    def options(cls):
        return AbstractStep.options() + [
            Options.git_commit,
            Options.reviewer,
        ]

    def _guess_reviewer_from_bug(self, bug_id):
        # FIXME: It's unclear if it would be safe to use self.cached_lookup(state, 'bug')
        # here as we don't currently have a way to invalidate a bug after making changes (like ObsoletePatches does).
        patches = self._tool.bugs.fetch_bug(bug_id).reviewed_patches()
        if not patches:
            _log.info("%s on bug %s, cannot infer reviewer." % ("No reviewed patches", bug_id))
            return None
        patch = patches[-1]
        _log.info("Guessing \"%s\" as reviewer from attachment %s on bug %s." % (patch.reviewer().full_name, patch.id(), bug_id))
        return patch.reviewer().full_name

    def run(self, state):
        bug_id = state.get("bug_id")
        if not bug_id and state.get("patch"):
            bug_id = state.get("patch").bug_id()

        reviewer = self._options.reviewer
        if not reviewer:
            if not bug_id:
                _log.info("No bug id provided and --reviewer= not provided.  Not updating ChangeLogs with reviewer.")
                return
            reviewer = self._guess_reviewer_from_bug(bug_id)

        if not reviewer:
            _log.info("Failed to guess reviewer from bug %s and --reviewer= not provided.  Not updating ChangeLogs with reviewer." % bug_id)
            return

        # cached_lookup("changelogs") is always absolute paths.
        for changelog_path in self.cached_lookup(state, "changelogs"):
            ChangeLog(changelog_path).set_reviewer(reviewer)

        # Tell the world that we just changed something on disk so that the cached diff is invalidated.
        self.did_modify_checkout(state)
