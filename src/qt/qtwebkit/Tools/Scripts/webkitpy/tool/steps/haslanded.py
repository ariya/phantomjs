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

import cStringIO as StringIO
import logging
import sys
import re
import tempfile

from webkitpy.tool.steps.abstractstep import AbstractStep
from webkitpy.common.system.executive import Executive, ScriptError
from webkitpy.common.checkout import diff_parser

from webkitpy.tool.steps import confirmdiff

_log = logging.getLogger(__name__)


class HasLanded(confirmdiff.ConfirmDiff):

    @classmethod
    def convert_to_svn(cls, diff):
        lines = StringIO.StringIO(diff).readlines()
        convert = diff_parser.get_diff_converter(lines)
        return "".join(convert(x) for x in lines)

    @classmethod
    def strip_change_log(cls, diff):
        output = []
        skipping = False
        for line in StringIO.StringIO(diff).readlines():
            indexline = re.match("^Index: ([^\\n]*/)?([^/\\n]*)$", line)
            if skipping and indexline:
                skipping = False
            if indexline and indexline.group(2) == "ChangeLog":
                skipping = True
            if not skipping:
                output.append(line)
        return "".join(output)

    @classmethod
    def diff_diff(cls, diff1, diff2, diff1_suffix, diff2_suffix, executive=None):
        # Now this is where it gets complicated, we need to compare our diff to the diff at landed_revision.
        diff1_patch = tempfile.NamedTemporaryFile(suffix=diff1_suffix + '.patch')
        diff1_patch.write(diff1)
        diff1_patch.flush()

        # Check if there are any differences in the patch that don't happen
        diff2_patch = tempfile.NamedTemporaryFile(suffix=diff2_suffix + '.patch')
        diff2_patch.write(diff2)
        diff2_patch.flush()

        # Diff the two diff's together...
        if not executive:
            executive = Executive()

        try:
            return executive.run_command(
                ["interdiff", diff1_patch.name, diff2_patch.name], decode_output=False)
        except ScriptError, e:
            _log.warning("Unable to find interdiff util (part of GNU difftools package) which is required.")
            raise

    def run(self, state):
        # Check if there are changes first
        if not self._tool.scm().local_changes_exist():
            _log.warn("No local changes found, exiting.")
            return True

        # Check if there is a SVN revision in the bug from the commit queue
        landed_revision = self.cached_lookup(state, "bug").commit_revision()
        if not landed_revision:
            raise ScriptError("Unable to find landed message in associated bug.")

        # Now this is there it gets complicated, we need to compare our diff to the diff at landed_revision.
        landed_diff_bin = self._tool.scm().diff_for_revision(landed_revision)
        landed_diff_trimmed = self.strip_change_log(self.convert_to_svn(landed_diff_bin))

        # Check if there are any differences in the patch that don't happen
        local_diff_bin = self._tool.scm().create_patch()
        local_diff_trimmed = self.strip_change_log(self.convert_to_svn(local_diff_bin))

        # Diff the two diff's together...
        diff_diff = self.diff_diff(landed_diff_trimmed, local_diff_trimmed,
                                   '-landed', '-local',
                                   executive=self._tool.executive)

        with self._show_pretty_diff(diff_diff) as pretty_diff_file:
            if not pretty_diff_file:
                self._tool.user.page(diff_diff)

            if self._tool.user.confirm("May I discard local changes?"):
                # Discard changes if the user confirmed we should
                _log.warn("Discarding changes as requested.")
                self._tool.scm().discard_local_changes()
