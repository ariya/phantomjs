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
import re
import sys

from webkitpy.common.checkout.changelog import ChangeLog
from webkitpy.common.system.executive import ScriptError
from webkitpy.tool.steps.abstractstep import AbstractStep
from webkitpy.tool.steps.options import Options

_log = logging.getLogger(__name__)


class PrepareChangeLog(AbstractStep):
    @classmethod
    def options(cls):
        return AbstractStep.options() + [
            Options.quiet,
            Options.email,
            Options.git_commit,
            Options.update_changelogs,
        ]

    def _ensure_bug_url(self, state):
        if not state.get("bug_id"):
            return
        bug_id = state.get("bug_id")
        changelogs = self.cached_lookup(state, "changelogs")
        for changelog_path in changelogs:
            changelog = ChangeLog(changelog_path, self._tool.filesystem)
            if not changelog.latest_entry().bug_id():
                changelog.set_short_description_and_bug_url(
                    self.cached_lookup(state, "bug_title"),
                    self._tool.bugs.bug_url_for_bug_id(bug_id))

    def _resolve_existing_entry(self, changelog_path):
        # When this is called, the top entry in the ChangeLog was just created
        # by prepare-ChangeLog, as an clean updated version of the one below it.
        with self._tool.filesystem.open_text_file_for_reading(changelog_path) as changelog_file:
            entries_gen = ChangeLog.parse_entries_from_file(changelog_file)
            entries = zip(entries_gen, range(2))

        if not len(entries):
            raise Exception("Expected to find at least two ChangeLog entries in %s but found none." % changelog_path)
        if len(entries) == 1:
            # If we get here, it probably means we've just rolled over to a
            # new CL file, so we don't have anything to resolve.
            return

        (new_entry, _), (old_entry, _) = entries
        final_entry = self._merge_entries(old_entry, new_entry)

        changelog = ChangeLog(changelog_path, self._tool.filesystem)
        changelog.delete_entries(2)
        changelog.prepend_text(final_entry)

    def _merge_entries(self, old_entry, new_entry):
        final_entry = old_entry.contents()

        final_entry = final_entry.replace(old_entry.date(), new_entry.date(), 1)

        new_bug_desc = new_entry.bug_description()
        old_bug_desc = old_entry.bug_description()
        if new_bug_desc and old_bug_desc and new_bug_desc != old_bug_desc:
            final_entry = final_entry.replace(old_bug_desc, new_bug_desc)

        new_touched = new_entry.touched_functions()
        old_touched = old_entry.touched_functions()
        if new_touched != old_touched:
            if old_entry.is_touched_files_text_clean():
                final_entry = final_entry.replace(old_entry.touched_files_text(), new_entry.touched_files_text())
            else:
                final_entry += "\n" + new_entry.touched_files_text()

        return final_entry + "\n"

    def run(self, state):
        if self.cached_lookup(state, "changelogs"):
            self._ensure_bug_url(state)
            if not self._options.update_changelogs:
                return

        args = self._tool.deprecated_port().prepare_changelog_command()
        if state.get("bug_id"):
            args.append("--bug=%s" % state["bug_id"])
            args.append("--description=%s" % self.cached_lookup(state, 'bug_title'))
        if self._options.email:
            args.append("--email=%s" % self._options.email)

        if self._tool.scm().supports_local_commits():
            args.append("--merge-base=%s" % self._tool.scm().merge_base(self._options.git_commit))

        args.extend(self._changed_files(state))

        try:
            output = self._tool.executive.run_and_throw_if_fail(args, self._options.quiet, cwd=self._tool.scm().checkout_root)
        except ScriptError, e:
            _log.error("Unable to prepare ChangeLogs.")
            sys.exit(1)

        # These are the ChangeLog entries added by prepare-Changelog
        changelogs = re.findall(r'Editing the (\S*/ChangeLog) file.', output)
        changelogs = set(self._tool.filesystem.join(self._tool.scm().checkout_root, f) for f in changelogs)
        for changelog in changelogs & set(self.cached_lookup(state, "changelogs")):
            self._resolve_existing_entry(changelog)

        self.did_modify_checkout(state)
