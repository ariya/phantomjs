# Copyright (c) 2010 Google Inc. All rights reserved.
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

import StringIO

from webkitpy.common.config import urls
from webkitpy.common.checkout.changelog import ChangeLog, parse_bug_id_from_changelog
from webkitpy.common.checkout.commitinfo import CommitInfo
from webkitpy.common.checkout.scm import CommitMessage
from webkitpy.common.memoized import memoized
from webkitpy.common.system.executive import ScriptError


# This class represents the WebKit-specific parts of the checkout (like ChangeLogs).
# FIXME: Move a bunch of ChangeLog-specific processing from SCM to this object.
# NOTE: All paths returned from this class should be absolute.
class Checkout(object):
    def __init__(self, scm, executive=None, filesystem=None):
        self._scm = scm
        # FIXME: We shouldn't be grabbing at private members on scm.
        self._executive = executive or self._scm._executive
        self._filesystem = filesystem or self._scm._filesystem

    def is_path_to_changelog(self, path):
        return self._filesystem.basename(path) == "ChangeLog"

    def _latest_entry_for_changelog_at_revision(self, changelog_path, revision):
        changelog_contents = self._scm.contents_at_revision(changelog_path, revision)
        # contents_at_revision returns a byte array (str()), but we know
        # that ChangeLog files are utf-8.  parse_latest_entry_from_file
        # expects a file-like object which vends unicode(), so we decode here.
        # Old revisions of Sources/WebKit/wx/ChangeLog have some invalid utf8 characters.
        changelog_file = StringIO.StringIO(changelog_contents.decode("utf-8", "ignore"))
        return ChangeLog.parse_latest_entry_from_file(changelog_file)

    def changelog_entries_for_revision(self, revision, changed_files=None):
        if not changed_files:
            changed_files = self._scm.changed_files_for_revision(revision)
        # FIXME: This gets confused if ChangeLog files are moved, as
        # deletes are still "changed files" per changed_files_for_revision.
        # FIXME: For now we hack around this by caching any exceptions
        # which result from having deleted files included the changed_files list.
        changelog_entries = []
        for path in changed_files:
            if not self.is_path_to_changelog(path):
                continue
            try:
                changelog_entries.append(self._latest_entry_for_changelog_at_revision(path, revision))
            except ScriptError:
                pass
        return changelog_entries

    def _changelog_data_for_revision(self, revision):
        changed_files = self._scm.changed_files_for_revision(revision)
        changelog_entries = self.changelog_entries_for_revision(revision, changed_files=changed_files)
        # Assume for now that the first entry has everything we need:
        # FIXME: This will throw an exception if there were no ChangeLogs.
        if not len(changelog_entries):
            return None
        changelog_entry = changelog_entries[0]
        return {
            "bug_id": parse_bug_id_from_changelog(changelog_entry.contents()),
            "author_name": changelog_entry.author_name(),
            "author_email": changelog_entry.author_email(),
            "author": changelog_entry.author(),
            "reviewer_text": changelog_entry.reviewer_text(),
            "reviewer": changelog_entry.reviewer(),
            "contents": changelog_entry.contents(),
            "changed_files": changed_files,
        }

    @memoized
    def commit_info_for_revision(self, revision):
        committer_email = self._scm.committer_email_for_revision(revision)
        changelog_data = self._changelog_data_for_revision(revision)
        if not changelog_data:
            return None
        return CommitInfo(revision, committer_email, changelog_data)

    def bug_id_for_revision(self, revision):
        return self.commit_info_for_revision(revision).bug_id()

    def _modified_files_matching_predicate(self, git_commit, predicate, changed_files=None):
        # SCM returns paths relative to scm.checkout_root
        # Callers (especially those using the ChangeLog class) may
        # expect absolute paths, so this method returns absolute paths.
        if not changed_files:
            changed_files = self._scm.changed_files(git_commit)
        return filter(predicate, map(self._scm.absolute_path, changed_files))

    def modified_changelogs(self, git_commit, changed_files=None):
        return self._modified_files_matching_predicate(git_commit, self.is_path_to_changelog, changed_files=changed_files)

    def modified_non_changelogs(self, git_commit, changed_files=None):
        return self._modified_files_matching_predicate(git_commit, lambda path: not self.is_path_to_changelog(path), changed_files=changed_files)

    def commit_message_for_this_commit(self, git_commit, changed_files=None, return_stderr=False):
        changelog_paths = self.modified_changelogs(git_commit, changed_files)
        if not len(changelog_paths):
            raise ScriptError(message="Found no modified ChangeLogs, cannot create a commit message.\n"
                              "All changes require a ChangeLog.  See:\n %s" % urls.contribution_guidelines)

        message_text = self._scm.run([self._scm.script_path('commit-log-editor'), '--print-log'] + changelog_paths, return_stderr=return_stderr)
        return CommitMessage(message_text.splitlines())

    def recent_commit_infos_for_files(self, paths):
        revisions = set(sum(map(self._scm.revisions_changing_file, paths), []))
        return set(map(self.commit_info_for_revision, revisions))

    def suggested_reviewers(self, git_commit, changed_files=None):
        changed_files = self.modified_non_changelogs(git_commit, changed_files)
        commit_infos = sorted(self.recent_commit_infos_for_files(changed_files), key=lambda info: info.revision(), reverse=True)
        reviewers = filter(lambda person: person and person.can_review, sum(map(lambda info: [info.reviewer(), info.author()], commit_infos), []))
        unique_reviewers = reduce(lambda suggestions, reviewer: suggestions + [reviewer if reviewer not in suggestions else None], reviewers, [])
        return filter(lambda reviewer: reviewer, unique_reviewers)

    def bug_id_for_this_commit(self, git_commit, changed_files=None):
        try:
            return parse_bug_id_from_changelog(self.commit_message_for_this_commit(git_commit, changed_files).message())
        except ScriptError, e:
            pass # We might not have ChangeLogs.

    def apply_patch(self, patch):
        # It's possible that the patch was not made from the root directory.
        # We should detect and handle that case.
        # FIXME: Move _scm.script_path here once we get rid of all the dependencies.
        # --force (continue after errors) is the common case, so we always use it.
        args = [self._scm.script_path('svn-apply'), "--force"]
        if patch.reviewer():
            args += ['--reviewer', patch.reviewer().full_name]
        self._executive.run_command(args, input=patch.contents(), cwd=self._scm.checkout_root)

    def apply_reverse_diff(self, revision):
        self._scm.apply_reverse_diff(revision)

        # We revert the ChangeLogs because removing lines from a ChangeLog
        # doesn't make sense.  ChangeLogs are append only.
        changelog_paths = self.modified_changelogs(git_commit=None)
        if len(changelog_paths):
            self._scm.revert_files(changelog_paths)

        conflicts = self._scm.conflicted_files()
        if len(conflicts):
            raise ScriptError(message="Failed to apply reverse diff for revision %s because of the following conflicts:\n%s" % (revision, "\n".join(conflicts)))

    def apply_reverse_diffs(self, revision_list):
        for revision in sorted(revision_list, reverse=True):
            self.apply_reverse_diff(revision)
