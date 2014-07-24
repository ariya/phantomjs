# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

from .commitinfo import CommitInfo

# FIXME: These imports are wrong, we should use a shared MockCommittersList.
from webkitpy.common.config.committers import CommitterList
from webkitpy.common.net.bugzilla.bugzilla_mock import _mock_reviewers
from webkitpy.common.system.filesystem_mock import MockFileSystem


class MockCommitMessage(object):
    def message(self):
        return "This is a fake commit message that is at least 50 characters."


committer_list = CommitterList()

mock_revisions = {
    1: CommitInfo(852, "eric@webkit.org", {
        "bug_id": 50000,
        "author_name": "Adam Barth",
        "author_email": "abarth@webkit.org",
        "author": committer_list.contributor_by_email("abarth@webkit.org"),
        "reviewer_text": "Darin Adler",
        "reviewer": committer_list.committer_by_name("Darin Adler"),
        "changed_files": [
            "path/to/file",
            "another/file",
        ],
    }),
    3001: CommitInfo(3001, "tomz@codeaurora.org", {
        "bug_id": 50004,
        "author_name": "Tom Zakrajsek",
        "author_email": "tomz@codeaurora.org",
        "author": committer_list.contributor_by_email("tomz@codeaurora.org"),
        "reviewer_text": "Darin Adler",
        "reviewer": committer_list.committer_by_name("Darin Adler"),
        "changed_files": [
            "path/to/file",
            "another/file",
        ],
    })
}

class MockCheckout(object):
    def __init__(self):
        # FIXME: It's unclear if a MockCheckout is very useful.  A normal Checkout
        # with a MockSCM/MockFileSystem/MockExecutive is probably better.
        self._filesystem = MockFileSystem()

    def commit_info_for_revision(self, svn_revision):
        # There are legacy tests that all expected these revision numbers to map
        # to the same commit description (now mock_revisions[1])
        if svn_revision in [32, 123, 852, 853, 854, 1234, 21654, 21655, 21656]:
            return mock_revisions[1]

        if svn_revision in mock_revisions:
            return mock_revisions[svn_revision]

        # any "unrecognized" svn_revision will return None.

    def is_path_to_changelog(self, path):
        return self._filesystem.basename(path) == "ChangeLog"

    def bug_id_for_revision(self, svn_revision):
        return 12345

    def recent_commit_infos_for_files(self, paths):
        return [self.commit_info_for_revision(32)]

    def modified_changelogs(self, git_commit, changed_files=None):
        # Ideally we'd return something more interesting here.  The problem is
        # that LandDiff will try to actually read the patch from disk!
        return []

    def commit_message_for_this_commit(self, git_commit, changed_files=None):
        return MockCommitMessage()

    def apply_patch(self, patch):
        pass

    def apply_reverse_diffs(self, revision):
        pass

    def suggested_reviewers(self, git_commit, changed_files=None):
        # FIXME: We should use a shared mock commiter list.
        return [_mock_reviewers[0]]
