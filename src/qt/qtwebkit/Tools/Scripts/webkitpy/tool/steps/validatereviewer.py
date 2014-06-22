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
import sys

from webkitpy.common.checkout.changelog import ChangeLog
from webkitpy.tool.steps.abstractstep import AbstractStep
from webkitpy.tool.steps.options import Options

_log = logging.getLogger(__name__)


# FIXME: Some of this logic should probably be unified with CommitterValidator?
class ValidateReviewer(AbstractStep):
    @classmethod
    def options(cls):
        return AbstractStep.options() + [
            Options.non_interactive,
        ]

    def run(self, state):
        # FIXME: For now we disable this check when a user is driving the script
        # this check is too draconian (and too poorly tested) to foist upon users.
        if not self._options.non_interactive:
            return
        for changelog_path in self.cached_lookup(state, "changelogs"):
            changelog_entry = ChangeLog(changelog_path).latest_entry()
            if changelog_entry.has_valid_reviewer():
                continue
            reviewer_text = changelog_entry.reviewer_text()
            if reviewer_text:
                _log.info("%s found in %s does not appear to be a valid reviewer according to committers.py." % (reviewer_text, changelog_path))
            _log.error('%s neither lists a valid reviewer nor contains the string "Unreviewed" or "Rubber stamp" (case insensitive).' % changelog_path)
            sys.exit(1)
