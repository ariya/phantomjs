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

from optparse import make_option
from webkitpy.tool.steps.abstractstep import AbstractStep
from webkitpy.tool.steps.options import Options
from webkitpy.common.checkout.diff_parser import DiffParser

_log = logging.getLogger(__name__)


# This is closely related to the ValidateReviewer step and the CommitterValidator class.
# We may want to unify more of this code in one place.
class ValidateChangeLogs(AbstractStep):
    @classmethod
    def options(cls):
        return AbstractStep.options() + [
            make_option("--check-oops", action="store_true", default=False, help="Check there are no OOPS left in change log"),
            Options.non_interactive,
        ]

    def _check_changelog_diff(self, diff_file):
        # Each line is a tuple, the first value is the deleted line number
        # Date, reviewer, bug title, bug url, and empty lines could all be
        # identical in the most recent entries.  If the diff starts any
        # later than that, assume that the entry is wrong.
        if diff_file.lines[0][0] < 8:
            return True
        if self._options.non_interactive:
            return False

        _log.info("The diff to %s looks wrong. Are you sure your ChangeLog entry is at the top of the file?" % (diff_file.filename))
        # FIXME: Do we need to make the file path absolute?
        self._tool.scm().diff_for_file(diff_file.filename)
        if self._tool.user.confirm("OK to continue?", default='n'):
            return True
        return False

    def _changelog_contains_oops(self, diff_file):
        for diff_line in diff_file.lines:
            if 'OOPS!' in diff_line[2]:
                return True
        return False

    def run(self, state):
        changed_files = self.cached_lookup(state, "changed_files")
        for filename in changed_files:
            if not self._tool.checkout().is_path_to_changelog(filename):
                continue
            # Diff ChangeLogs directly because svn-create-patch will move
            # ChangeLog entries to the # top automatically, defeating our
            # validation here.
            # FIXME: Should we diff all the ChangeLogs at once?
            diff = self._tool.scm().diff_for_file(filename)
            parsed_diff = DiffParser(diff.splitlines())
            for filename, diff_file in parsed_diff.files.items():
                if not self._tool.checkout().is_path_to_changelog(diff_file.filename):
                    continue
                if not self._check_changelog_diff(diff_file):
                    _log.error("ChangeLog entry in %s is not at the top of the file." % diff_file.filename)
                    sys.exit(1)
                if self._options.check_oops and self._changelog_contains_oops(diff_file):
                    _log.error("ChangeLog entry in %s contains OOPS!." % diff_file.filename)
                    sys.exit(1)
