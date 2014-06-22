# Copyright (c) 2009, 2010, 2011 Google Inc. All rights reserved.
# Copyright (c) 2009 Apple Inc. All rights reserved.
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

import datetime
import logging
import os
import re

from webkitpy.common.memoized import memoized
from webkitpy.common.system.executive import Executive, ScriptError

from .commitmessage import CommitMessage
from .scm import AuthenticationError, SCM, commit_error_handler
from .svn import SVN, SVNRepository

_log = logging.getLogger(__name__)


class AmbiguousCommitError(Exception):
    def __init__(self, num_local_commits, has_working_directory_changes):
        Exception.__init__(self, "Found %s local commits and the working directory is %s" % (
            num_local_commits, ["clean", "not clean"][has_working_directory_changes]))
        self.num_local_commits = num_local_commits
        self.has_working_directory_changes = has_working_directory_changes


class Git(SCM, SVNRepository):

    # Git doesn't appear to document error codes, but seems to return
    # 1 or 128, mostly.
    ERROR_FILE_IS_MISSING = 128

    executable_name = 'git'

    def __init__(self, cwd, **kwargs):
        SCM.__init__(self, cwd, **kwargs)
        self._check_git_architecture()

    def _machine_is_64bit(self):
        import platform
        # This only is tested on Mac.
        if not platform.mac_ver()[0]:
            return False

        # platform.architecture()[0] can be '64bit' even if the machine is 32bit:
        # http://mail.python.org/pipermail/pythonmac-sig/2009-September/021648.html
        # Use the sysctl command to find out what the processor actually supports.
        return self.run(['sysctl', '-n', 'hw.cpu64bit_capable']).rstrip() == '1'

    def _executable_is_64bit(self, path):
        # Again, platform.architecture() fails us.  On my machine
        # git_bits = platform.architecture(executable=git_path, bits='default')[0]
        # git_bits is just 'default', meaning the call failed.
        file_output = self.run(['file', path])
        return re.search('x86_64', file_output)

    def _check_git_architecture(self):
        if not self._machine_is_64bit():
            return

        # We could path-search entirely in python or with
        # which.py (http://code.google.com/p/which), but this is easier:
        git_path = self.run(['which', self.executable_name]).rstrip()
        if self._executable_is_64bit(git_path):
            return

        webkit_dev_thread_url = "https://lists.webkit.org/pipermail/webkit-dev/2010-December/015287.html"
        _log.warning("This machine is 64-bit, but the git binary (%s) does not support 64-bit.\nInstall a 64-bit git for better performance, see:\n%s\n" % (git_path, webkit_dev_thread_url))

    def _run_git(self, command_args, **kwargs):
        full_command_args = [self.executable_name] + command_args
        full_kwargs = kwargs
        if not 'cwd' in full_kwargs:
            full_kwargs['cwd'] = self.checkout_root
        return self.run(full_command_args, **full_kwargs)

    @classmethod
    def in_working_directory(cls, path, executive=None):
        try:
            executive = executive or Executive()
            return executive.run_command([cls.executable_name, 'rev-parse', '--is-inside-work-tree'], cwd=path, error_handler=Executive.ignore_error).rstrip() == "true"
        except OSError, e:
            # The Windows bots seem to through a WindowsError when git isn't installed.
            return False

    def find_checkout_root(self, path):
        # "git rev-parse --show-cdup" would be another way to get to the root
        checkout_root = self._run_git(['rev-parse', '--show-toplevel'], cwd=(path or "./")).strip()
        if not self._filesystem.isabs(checkout_root):  # Sometimes git returns relative paths
            checkout_root = self._filesystem.join(path, checkout_root)
        return checkout_root

    def to_object_name(self, filepath):
        # FIXME: This can't be the right way to append a slash.
        root_end_with_slash = self._filesystem.join(self.find_checkout_root(self._filesystem.dirname(filepath)), '')
        # FIXME: This seems to want some sort of rel_path instead?
        return filepath.replace(root_end_with_slash, '')

    @classmethod
    def read_git_config(cls, key, cwd=None, executive=None):
        # FIXME: This should probably use cwd=self.checkout_root.
        # Pass --get-all for cases where the config has multiple values
        # Pass the cwd if provided so that we can handle the case of running webkit-patch outside of the working directory.
        # FIXME: This should use an Executive.
        executive = executive or Executive()
        return executive.run_command([cls.executable_name, "config", "--get-all", key], error_handler=Executive.ignore_error, cwd=cwd).rstrip('\n')

    @staticmethod
    def commit_success_regexp():
        return "^Committed r(?P<svn_revision>\d+)$"

    def discard_local_commits(self):
        self._run_git(['reset', '--hard', self.remote_branch_ref()])

    def local_commits(self):
        return self._run_git(['log', '--pretty=oneline', 'HEAD...' + self.remote_branch_ref()]).splitlines()

    def rebase_in_progress(self):
        return self._filesystem.exists(self.absolute_path(self._filesystem.join('.git', 'rebase-apply')))

    def has_working_directory_changes(self):
        return self._run_git(['diff', 'HEAD', '--no-renames', '--name-only']) != ""

    def discard_working_directory_changes(self):
        # Could run git clean here too, but that wouldn't match subversion
        self._run_git(['reset', 'HEAD', '--hard'])
        # Aborting rebase even though this does not match subversion
        if self.rebase_in_progress():
            self._run_git(['rebase', '--abort'])

    def status_command(self):
        # git status returns non-zero when there are changes, so we use git diff name --name-status HEAD instead.
        # No file contents printed, thus utf-8 autodecoding in self.run is fine.
        return [self.executable_name, "diff", "--name-status", "--no-renames", "HEAD"]

    def _status_regexp(self, expected_types):
        return '^(?P<status>[%s])\t(?P<filename>.+)$' % expected_types

    def add_list(self, paths):
        self._run_git(["add"] + paths)

    def delete_list(self, paths):
        return self._run_git(["rm", "-f"] + paths)

    def exists(self, path):
        return_code = self._run_git(["show", "HEAD:%s" % path], return_exit_code=True, decode_output=False)
        return return_code != self.ERROR_FILE_IS_MISSING

    def _branch_from_ref(self, ref):
        return ref.replace('refs/heads/', '')

    def _current_branch(self):
        return self._branch_from_ref(self._run_git(['symbolic-ref', '-q', 'HEAD']).strip())

    def _upstream_branch(self):
        current_branch = self._current_branch()
        return self._branch_from_ref(self.read_git_config('branch.%s.merge' % current_branch, cwd=self.checkout_root, executive=self._executive).strip())

    def merge_base(self, git_commit):
        if git_commit:
            # Rewrite UPSTREAM to the upstream branch
            if 'UPSTREAM' in git_commit:
                upstream = self._upstream_branch()
                if not upstream:
                    raise ScriptError(message='No upstream/tracking branch set.')
                git_commit = git_commit.replace('UPSTREAM', upstream)

            # Special-case <refname>.. to include working copy changes, e.g., 'HEAD....' shows only the diffs from HEAD.
            if git_commit.endswith('....'):
                return git_commit[:-4]

            if '..' not in git_commit:
                git_commit = git_commit + "^.." + git_commit
            return git_commit

        return self.remote_merge_base()

    def changed_files(self, git_commit=None):
        # FIXME: --diff-filter could be used to avoid the "extract_filenames" step.
        status_command = [self.executable_name, 'diff', '-r', '--name-status', "--no-renames", "--no-ext-diff", "--full-index", self.merge_base(git_commit)]
        # FIXME: I'm not sure we're returning the same set of files that SVN.changed_files is.
        # Added (A), Copied (C), Deleted (D), Modified (M), Renamed (R)
        return self.run_status_and_extract_filenames(status_command, self._status_regexp("ADM"))

    def _changes_files_for_commit(self, git_commit):
        # --pretty="format:" makes git show not print the commit log header,
        changed_files = self._run_git(["show", "--pretty=format:", "--name-only", git_commit]).splitlines()
        # instead it just prints a blank line at the top, so we skip the blank line:
        return changed_files[1:]

    def changed_files_for_revision(self, revision):
        commit_id = self.git_commit_from_svn_revision(revision)
        return self._changes_files_for_commit(commit_id)

    def revisions_changing_file(self, path, limit=5):
        # raise a script error if path does not exists to match the behavior of  the svn implementation.
        if not self._filesystem.exists(path):
            raise ScriptError(message="Path %s does not exist." % path)

        # git rev-list head --remove-empty --limit=5 -- path would be equivalent.
        commit_ids = self._run_git(["log", "--remove-empty", "--pretty=format:%H", "-%s" % limit, "--", path]).splitlines()
        return filter(lambda revision: revision, map(self.svn_revision_from_git_commit, commit_ids))

    def conflicted_files(self):
        # We do not need to pass decode_output for this diff command
        # as we're passing --name-status which does not output any data.
        status_command = [self.executable_name, 'diff', '--name-status', '--no-renames', '--diff-filter=U']
        return self.run_status_and_extract_filenames(status_command, self._status_regexp("U"))

    def added_files(self):
        return self.run_status_and_extract_filenames(self.status_command(), self._status_regexp("A"))

    def deleted_files(self):
        return self.run_status_and_extract_filenames(self.status_command(), self._status_regexp("D"))

    @staticmethod
    def supports_local_commits():
        return True

    def display_name(self):
        return "git"

    def _most_recent_log_matching(self, grep_str, path):
        # We use '--grep=' + foo rather than '--grep', foo because
        # git 1.7.0.4 (and earlier) didn't support the separate arg.
        return self._run_git(['log', '-1', '--grep=' + grep_str, '--date=iso', self.find_checkout_root(path)])

    def svn_revision(self, path):
        git_log = self._most_recent_log_matching('git-svn-id:', path)
        match = re.search("^\s*git-svn-id:.*@(?P<svn_revision>\d+)\ ", git_log, re.MULTILINE)
        if not match:
            return ""
        return str(match.group('svn_revision'))

    def timestamp_of_revision(self, path, revision):
        git_log = self._most_recent_log_matching('git-svn-id:.*@%s' % revision, path)
        match = re.search("^Date:\s*(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2}) ([+-])(\d{2})(\d{2})$", git_log, re.MULTILINE)
        if not match:
            return ""

        # Manually modify the timezone since Git doesn't have an option to show it in UTC.
        # Git also truncates milliseconds but we're going to ignore that for now.
        time_with_timezone = datetime.datetime(int(match.group(1)), int(match.group(2)), int(match.group(3)),
            int(match.group(4)), int(match.group(5)), int(match.group(6)), 0)

        sign = 1 if match.group(7) == '+' else -1
        time_without_timezone = time_with_timezone - datetime.timedelta(hours=sign * int(match.group(8)), minutes=int(match.group(9)))
        return time_without_timezone.strftime('%Y-%m-%dT%H:%M:%SZ')

    def prepend_svn_revision(self, diff):
        revision = self.head_svn_revision()
        if not revision:
            return diff

        return "Subversion Revision: " + revision + '\n' + diff

    def create_patch(self, git_commit=None, changed_files=None):
        """Returns a byte array (str()) representing the patch file.
        Patch files are effectively binary since they may contain
        files of multiple different encodings."""

        # Put code changes at the top of the patch and layout tests
        # at the bottom, this makes for easier reviewing.
        config_path = self._filesystem.dirname(self._filesystem.path_to_module('webkitpy.common.config'))
        order_file = self._filesystem.join(config_path, 'orderfile')
        order = ""
        if self._filesystem.exists(order_file):
            order = "-O%s" % order_file

        command = [self.executable_name, 'diff', '--binary', '--no-color', "--no-ext-diff", "--full-index", "--no-renames", order, self.merge_base(git_commit), "--"]
        if changed_files:
            command += changed_files
        return self.prepend_svn_revision(self.run(command, decode_output=False, cwd=self.checkout_root))

    def _run_git_svn_find_rev(self, arg):
        # git svn find-rev always exits 0, even when the revision or commit is not found.
        return self._run_git(['svn', 'find-rev', arg]).rstrip()

    def _string_to_int_or_none(self, string):
        try:
            return int(string)
        except ValueError, e:
            return None

    @memoized
    def git_commit_from_svn_revision(self, svn_revision):
        # FIXME: https://bugs.webkit.org/show_bug.cgi?id=111668
        # We should change this to run git log --grep 'git-svn-id' instead
        # so that we don't require git+svn to be set up.
        git_commit = self._run_git_svn_find_rev('r%s' % svn_revision)
        if not git_commit:
            # FIXME: Alternatively we could offer to update the checkout? Or return None?
            raise ScriptError(message='Failed to find git commit for revision %s, your checkout likely needs an update.' % svn_revision)
        return git_commit

    @memoized
    def svn_revision_from_git_commit(self, git_commit):
        svn_revision = self._run_git_svn_find_rev(git_commit)
        return self._string_to_int_or_none(svn_revision)

    def contents_at_revision(self, path, revision):
        """Returns a byte array (str()) containing the contents
        of path @ revision in the repository."""
        return self._run_git(["show", "%s:%s" % (self.git_commit_from_svn_revision(revision), path)], decode_output=False)

    def diff_for_revision(self, revision):
        git_commit = self.git_commit_from_svn_revision(revision)
        return self.create_patch(git_commit)

    def diff_for_file(self, path, log=None):
        return self._run_git(['diff', 'HEAD', '--no-renames', '--', path])

    def show_head(self, path):
        return self._run_git(['show', 'HEAD:' + self.to_object_name(path)], decode_output=False)

    def committer_email_for_revision(self, revision):
        git_commit = self.git_commit_from_svn_revision(revision)
        committer_email = self._run_git(["log", "-1", "--pretty=format:%ce", git_commit])
        # Git adds an extra @repository_hash to the end of every committer email, remove it:
        return committer_email.rsplit("@", 1)[0]

    def apply_reverse_diff(self, revision):
        # Assume the revision is an svn revision.
        git_commit = self.git_commit_from_svn_revision(revision)
        # I think this will always fail due to ChangeLogs.
        self._run_git(['revert', '--no-commit', git_commit], error_handler=Executive.ignore_error)

    def revert_files(self, file_paths):
        self._run_git(['checkout', 'HEAD'] + file_paths)

    def _assert_can_squash(self, has_working_directory_changes):
        squash = self.read_git_config('webkit-patch.commit-should-always-squash', cwd=self.checkout_root, executive=self._executive)
        should_squash = squash and squash.lower() == "true"

        if not should_squash:
            # Only warn if there are actually multiple commits to squash.
            num_local_commits = len(self.local_commits())
            if num_local_commits > 1 or (num_local_commits > 0 and has_working_directory_changes):
                raise AmbiguousCommitError(num_local_commits, has_working_directory_changes)

    def commit_with_message(self, message, username=None, password=None, git_commit=None, force_squash=False, changed_files=None):
        # Username is ignored during Git commits.
        has_working_directory_changes = self.has_working_directory_changes()

        if git_commit:
            # Special-case HEAD.. to mean working-copy changes only.
            if git_commit.upper() == 'HEAD..':
                if not has_working_directory_changes:
                    raise ScriptError(message="The working copy is not modified. --git-commit=HEAD.. only commits working copy changes.")
                self.commit_locally_with_message(message)
                return self._commit_on_branch(message, 'HEAD', username=username, password=password)

            # Need working directory changes to be committed so we can checkout the merge branch.
            if has_working_directory_changes:
                # FIXME: webkit-patch land will modify the ChangeLogs to correct the reviewer.
                # That will modify the working-copy and cause us to hit this error.
                # The ChangeLog modification could be made to modify the existing local commit.
                raise ScriptError(message="Working copy is modified. Cannot commit individual git_commits.")
            return self._commit_on_branch(message, git_commit, username=username, password=password)

        if not force_squash:
            self._assert_can_squash(has_working_directory_changes)
        self._run_git(['reset', '--soft', self.remote_merge_base()])
        self.commit_locally_with_message(message)
        return self.push_local_commits_to_server(username=username, password=password)

    def _commit_on_branch(self, message, git_commit, username=None, password=None):
        branch_name = self._current_branch()
        commit_ids = self.commit_ids_from_commitish_arguments([git_commit])

        # We want to squash all this branch's commits into one commit with the proper description.
        # We do this by doing a "merge --squash" into a new commit branch, then dcommitting that.
        MERGE_BRANCH_NAME = 'webkit-patch-land'
        self.delete_branch(MERGE_BRANCH_NAME)

        # We might be in a directory that's present in this branch but not in the
        # trunk.  Move up to the top of the tree so that git commands that expect a
        # valid CWD won't fail after we check out the merge branch.
        # FIXME: We should never be using chdir! We can instead pass cwd= to run_command/self.run!
        self._filesystem.chdir(self.checkout_root)

        # Stuff our change into the merge branch.
        # We wrap in a try...finally block so if anything goes wrong, we clean up the branches.
        commit_succeeded = True
        try:
            self._run_git(['checkout', '-q', '-b', MERGE_BRANCH_NAME, self.remote_branch_ref()])

            for commit in commit_ids:
                # We're on a different branch now, so convert "head" to the branch name.
                commit = re.sub(r'(?i)head', branch_name, commit)
                # FIXME: Once changed_files and create_patch are modified to separately handle each
                # commit in a commit range, commit each cherry pick so they'll get dcommitted separately.
                self._run_git(['cherry-pick', '--no-commit', commit])

            self._run_git(['commit', '-m', message])
            output = self.push_local_commits_to_server(username=username, password=password)
        except Exception, e:
            _log.warning("COMMIT FAILED: " + str(e))
            output = "Commit failed."
            commit_succeeded = False
        finally:
            # And then swap back to the original branch and clean up.
            self.discard_working_directory_changes()
            self._run_git(['checkout', '-q', branch_name])
            self.delete_branch(MERGE_BRANCH_NAME)

        return output

    def svn_commit_log(self, svn_revision):
        svn_revision = self.strip_r_from_svn_revision(svn_revision)
        return self._run_git(['svn', 'log', '-r', svn_revision])

    def last_svn_commit_log(self):
        return self._run_git(['svn', 'log', '--limit=1'])

    def svn_blame(self, path):
        return self._run_git(['svn', 'blame', path])

    # Git-specific methods:
    def _branch_ref_exists(self, branch_ref):
        return self._run_git(['show-ref', '--quiet', '--verify', branch_ref], return_exit_code=True) == 0

    def delete_branch(self, branch_name):
        if self._branch_ref_exists('refs/heads/' + branch_name):
            self._run_git(['branch', '-D', branch_name])

    def remote_merge_base(self):
        return self._run_git(['merge-base', self.remote_branch_ref(), 'HEAD']).strip()

    def remote_branch_ref(self):
        # Use references so that we can avoid collisions, e.g. we don't want to operate on refs/heads/trunk if it exists.
        remote_branch_refs = self.read_git_config('svn-remote.svn.fetch', cwd=self.checkout_root, executive=self._executive)
        if not remote_branch_refs:
            remote_master_ref = 'refs/remotes/origin/master'
            if not self._branch_ref_exists(remote_master_ref):
                raise ScriptError(message="Can't find a branch to diff against. svn-remote.svn.fetch is not in the git config and %s does not exist" % remote_master_ref)
            return remote_master_ref

        # FIXME: What's the right behavior when there are multiple svn-remotes listed?
        # For now, just use the first one.
        first_remote_branch_ref = remote_branch_refs.split('\n')[0]
        return first_remote_branch_ref.split(':')[1]

    def commit_locally_with_message(self, message):
        self._run_git(['commit', '--all', '-F', '-'], input=message)

    def push_local_commits_to_server(self, username=None, password=None):
        dcommit_command = ['svn', 'dcommit']
        if (not username or not password) and not self.has_authorization_for_realm(self.svn_server_realm):
            raise AuthenticationError(self.svn_server_host, prompt_for_password=True)
        if username:
            dcommit_command.extend(["--username", username])
        output = self._run_git(dcommit_command, error_handler=commit_error_handler, input=password)
        return output

    # This function supports the following argument formats:
    # no args : rev-list trunk..HEAD
    # A..B    : rev-list A..B
    # A...B   : error!
    # A B     : [A, B]  (different from git diff, which would use "rev-list A..B")
    def commit_ids_from_commitish_arguments(self, args):
        if not len(args):
            args.append('%s..HEAD' % self.remote_branch_ref())

        commit_ids = []
        for commitish in args:
            if '...' in commitish:
                raise ScriptError(message="'...' is not supported (found in '%s'). Did you mean '..'?" % commitish)
            elif '..' in commitish:
                commit_ids += reversed(self._run_git(['rev-list', commitish]).splitlines())
            else:
                # Turn single commits or branch or tag names into commit ids.
                commit_ids += self._run_git(['rev-parse', '--revs-only', commitish]).splitlines()
        return commit_ids

    def commit_message_for_local_commit(self, commit_id):
        commit_lines = self._run_git(['cat-file', 'commit', commit_id]).splitlines()

        # Skip the git headers.
        first_line_after_headers = 0
        for line in commit_lines:
            first_line_after_headers += 1
            if line == "":
                break
        return CommitMessage(commit_lines[first_line_after_headers:])

    def files_changed_summary_for_commit(self, commit_id):
        return self._run_git(['diff-tree', '--shortstat', '--no-renames', '--no-commit-id', commit_id])
