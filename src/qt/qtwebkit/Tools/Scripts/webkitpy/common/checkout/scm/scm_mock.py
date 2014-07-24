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

from webkitpy.common.checkout.scm import CommitMessage
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.executive_mock import MockExecutive


class MockSCM(object):
    def __init__(self, filesystem=None, executive=None):
        self.checkout_root = "/mock-checkout"
        self.added_paths = set()
        self._filesystem = filesystem or MockFileSystem()
        self._executive = executive or MockExecutive()

    def add(self, destination_path):
        self.add_list([destination_path])

    def add_list(self, destination_paths):
        self.added_paths.update(set(destination_paths))

    def has_working_directory_changes(self):
        return False

    def discard_working_directory_changes(self):
        pass

    def supports_local_commits(self):
        return True

    def has_local_commits(self):
        return False

    def discard_local_commits(self):
        pass

    def discard_local_changes(self):
        pass

    def exists(self, path):
        # TestRealMain.test_real_main (and several other rebaseline tests) are sensitive to this return value.
        # We should make those tests more robust, but for now we just return True always (since no test needs otherwise).
        return True

    def absolute_path(self, *comps):
        return self._filesystem.join(self.checkout_root, *comps)

    def changed_files(self, git_commit=None):
        return ["MockFile1"]

    def changed_files_for_revision(self, revision):
        return ["MockFile1"]

    def head_svn_revision(self):
        return '1234'

    def svn_revision(self, path):
        return '5678'

    def timestamp_of_revision(self, path, revision):
        return '2013-02-01 08:48:05 +0000'

    def create_patch(self, git_commit, changed_files=None):
        return "Patch1"

    def commit_ids_from_commitish_arguments(self, args):
        return ["Commitish1", "Commitish2"]

    def committer_email_for_revision(self, revision):
        return "mock@webkit.org"

    def commit_locally_with_message(self, message):
        pass

    def commit_with_message(self, message, username=None, password=None, git_commit=None, force_squash=False, changed_files=None):
        pass

    def merge_base(self, git_commit):
        return None

    def commit_message_for_local_commit(self, commit_id):
        if commit_id == "Commitish1":
            return CommitMessage("CommitMessage1\n" \
                "https://bugs.example.org/show_bug.cgi?id=50000\n")
        if commit_id == "Commitish2":
            return CommitMessage("CommitMessage2\n" \
                "https://bugs.example.org/show_bug.cgi?id=50001\n")
        raise Exception("Bogus commit_id in commit_message_for_local_commit.")

    def diff_for_file(self, path, log=None):
        return path + '-diff'

    def diff_for_revision(self, revision):
        return "DiffForRevision%s\nhttp://bugs.webkit.org/show_bug.cgi?id=12345" % revision

    def show_head(self, path):
        return path

    def svn_revision_from_commit_text(self, commit_text):
        return "49824"

    def delete(self, path):
        return self.delete_list([path])

    def delete_list(self, paths):
        if not self._filesystem:
            return
        for path in paths:
            if self._filesystem.exists(path):
                self._filesystem.remove(path)
