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

import logging
import os
import random
import re
import shutil
import string
import sys
import tempfile

from webkitpy.common.memoized import memoized
from webkitpy.common.system.executive import Executive, ScriptError

from .scm import AuthenticationError, SCM, commit_error_handler

_log = logging.getLogger(__name__)


# A mixin class that represents common functionality for SVN and Git-SVN.
class SVNRepository(object):
    # FIXME: These belong in common.config.urls
    svn_server_host = "svn.webkit.org"
    svn_server_realm = "<http://svn.webkit.org:80> Mac OS Forge"

    def has_authorization_for_realm(self, realm, home_directory=os.getenv("HOME")):
        # If we are working on a file:// repository realm will be None
        if realm is None:
            return True
        # ignore false positives for methods implemented in the mixee class. pylint: disable=E1101
        # Assumes find and grep are installed.
        if not os.path.isdir(os.path.join(home_directory, ".subversion")):
            return False
        find_args = ["find", ".subversion", "-type", "f", "-exec", "grep", "-q", realm, "{}", ";", "-print"]
        find_output = self.run(find_args, cwd=home_directory, error_handler=Executive.ignore_error).rstrip()
        if not find_output or not os.path.isfile(os.path.join(home_directory, find_output)):
            return False
        # Subversion either stores the password in the credential file, indicated by the presence of the key "password",
        # or uses the system password store (e.g. Keychain on Mac OS X) as indicated by the presence of the key "passtype".
        # We assume that these keys will not coincide with the actual credential data (e.g. that a person's username
        # isn't "password") so that we can use grep.
        if self.run(["grep", "password", find_output], cwd=home_directory, return_exit_code=True) == 0:
            return True
        return self.run(["grep", "passtype", find_output], cwd=home_directory, return_exit_code=True) == 0


class SVN(SCM, SVNRepository):

    executable_name = "svn"

    _svn_metadata_files = frozenset(['.svn', '_svn'])

    def __init__(self, cwd, patch_directories, **kwargs):
        SCM.__init__(self, cwd, **kwargs)
        self._bogus_dir = None
        if patch_directories == []:
            raise Exception(message='Empty list of patch directories passed to SCM.__init__')
        elif patch_directories == None:
            self._patch_directories = [self._filesystem.relpath(cwd, self.checkout_root)]
        else:
            self._patch_directories = patch_directories

    @classmethod
    def in_working_directory(cls, path, executive=None):
        if os.path.isdir(os.path.join(path, '.svn')):
            # This is a fast shortcut for svn info that is usually correct for SVN < 1.7,
            # but doesn't work for SVN >= 1.7.
            return True

        executive = executive or Executive()
        svn_info_args = [cls.executable_name, 'info']
        exit_code = executive.run_command(svn_info_args, cwd=path, return_exit_code=True)
        return (exit_code == 0)

    def find_uuid(self, path):
        if not self.in_working_directory(path):
            return None
        return self.value_from_svn_info(path, 'Repository UUID')

    @classmethod
    def value_from_svn_info(cls, path, field_name):
        svn_info_args = [cls.executable_name, 'info']
        # FIXME: This method should use a passed in executive or be made an instance method and use self._executive.
        info_output = Executive().run_command(svn_info_args, cwd=path).rstrip()
        match = re.search("^%s: (?P<value>.+)$" % field_name, info_output, re.MULTILINE)
        if not match:
            raise ScriptError(script_args=svn_info_args, message='svn info did not contain a %s.' % field_name)
        return match.group('value').rstrip('\r')

    def find_checkout_root(self, path):
        uuid = self.find_uuid(path)
        # If |path| is not in a working directory, we're supposed to return |path|.
        if not uuid:
            return path
        # Search up the directory hierarchy until we find a different UUID.
        last_path = None
        while True:
            if uuid != self.find_uuid(path):
                return last_path
            last_path = path
            (path, last_component) = self._filesystem.split(path)
            if last_path == path:
                return None

    @staticmethod
    def commit_success_regexp():
        return "^Committed revision (?P<svn_revision>\d+)\.$"

    def _run_svn(self, args, **kwargs):
        return self.run([self.executable_name] + args, **kwargs)

    @memoized
    def svn_version(self):
        return self._run_svn(['--version', '--quiet'])

    def has_working_directory_changes(self):
        # FIXME: What about files which are not committed yet?
        return self._run_svn(["diff"], cwd=self.checkout_root, decode_output=False) != ""

    def discard_working_directory_changes(self):
        # Make sure there are no locks lying around from a previously aborted svn invocation.
        # This is slightly dangerous, as it's possible the user is running another svn process
        # on this checkout at the same time.  However, it's much more likely that we're running
        # under windows and svn just sucks (or the user interrupted svn and it failed to clean up).
        self._run_svn(["cleanup"], cwd=self.checkout_root)

        # svn revert -R is not as awesome as git reset --hard.
        # It will leave added files around, causing later svn update
        # calls to fail on the bots.  We make this mirror git reset --hard
        # by deleting any added files as well.
        added_files = reversed(sorted(self.added_files()))
        # added_files() returns directories for SVN, we walk the files in reverse path
        # length order so that we remove files before we try to remove the directories.
        self._run_svn(["revert", "-R", "."], cwd=self.checkout_root)
        for path in added_files:
            # This is robust against cwd != self.checkout_root
            absolute_path = self.absolute_path(path)
            # Completely lame that there is no easy way to remove both types with one call.
            if os.path.isdir(path):
                os.rmdir(absolute_path)
            else:
                os.remove(absolute_path)

    def status_command(self):
        return [self.executable_name, 'status']

    def _status_regexp(self, expected_types):
        field_count = 6 if self.svn_version() > "1.6" else 5
        return "^(?P<status>[%s]).{%s} (?P<filename>.+)$" % (expected_types, field_count)

    def _add_parent_directories(self, path):
        """Does 'svn add' to the path and its parents."""
        if self.in_working_directory(path):
            return
        self.add(path)

    def add_list(self, paths):
        for path in paths:
            self._add_parent_directories(os.path.dirname(os.path.abspath(path)))
        if self.svn_version() >= "1.7":
            # For subversion client 1.7 and later, need to add '--parents' option to ensure intermediate directories
            # are added; in addition, 1.7 returns an exit code of 1 from svn add if one or more of the requested
            # adds are already under version control, including intermediate directories subject to addition
            # due to --parents
            svn_add_args = ['svn', 'add', '--parents'] + paths
            exit_code = self.run(svn_add_args, return_exit_code=True)
            if exit_code and exit_code != 1:
                raise ScriptError(script_args=svn_add_args, exit_code=exit_code)
        else:
            self._run_svn(["add"] + paths)

    def _delete_parent_directories(self, path):
        if not self.in_working_directory(path):
            return
        if set(os.listdir(path)) - self._svn_metadata_files:
            return  # Directory has non-trivial files in it.
        self.delete(path)

    def delete_list(self, paths):
        for path in paths:
            abs_path = os.path.abspath(path)
            parent, base = os.path.split(abs_path)
            result = self._run_svn(["delete", "--force", base], cwd=parent)
            self._delete_parent_directories(os.path.dirname(abs_path))
        return result

    def exists(self, path):
        return not self._run_svn(["info", path], return_exit_code=True, decode_output=False)

    def changed_files(self, git_commit=None):
        status_command = [self.executable_name, "status"]
        status_command.extend(self._patch_directories)
        # ACDMR: Addded, Conflicted, Deleted, Modified or Replaced
        return self.run_status_and_extract_filenames(status_command, self._status_regexp("ACDMR"))

    def changed_files_for_revision(self, revision):
        # As far as I can tell svn diff --summarize output looks just like svn status output.
        # No file contents printed, thus utf-8 auto-decoding in self.run is fine.
        status_command = [self.executable_name, "diff", "--summarize", "-c", revision]
        return self.run_status_and_extract_filenames(status_command, self._status_regexp("ACDMR"))

    def revisions_changing_file(self, path, limit=5):
        revisions = []
        # svn log will exit(1) (and thus self.run will raise) if the path does not exist.
        log_command = ['log', '--quiet', '--limit=%s' % limit, path]
        for line in self._run_svn(log_command, cwd=self.checkout_root).splitlines():
            match = re.search('^r(?P<revision>\d+) ', line)
            if not match:
                continue
            revisions.append(int(match.group('revision')))
        return revisions

    def conflicted_files(self):
        return self.run_status_and_extract_filenames(self.status_command(), self._status_regexp("C"))

    def added_files(self):
        return self.run_status_and_extract_filenames(self.status_command(), self._status_regexp("A"))

    def deleted_files(self):
        return self.run_status_and_extract_filenames(self.status_command(), self._status_regexp("D"))

    @staticmethod
    def supports_local_commits():
        return False

    def display_name(self):
        return "svn"

    def svn_revision(self, path):
        return self.value_from_svn_info(path, 'Revision')

    def timestamp_of_revision(self, path, revision):
        # We use --xml to get timestamps like 2013-02-08T08:18:04.964409Z
        repository_root = self.value_from_svn_info(self.checkout_root, 'Repository Root')
        info_output = Executive().run_command([self.executable_name, 'log', '-r', revision, '--xml', repository_root], cwd=path).rstrip()
        match = re.search(r"^<date>(?P<value>.+)</date>\r?$", info_output, re.MULTILINE)
        return match.group('value')

    # FIXME: This method should be on Checkout.
    def create_patch(self, git_commit=None, changed_files=None):
        """Returns a byte array (str()) representing the patch file.
        Patch files are effectively binary since they may contain
        files of multiple different encodings."""
        if changed_files == []:
            return ""
        elif changed_files == None:
            changed_files = []
        return self.run([self.script_path("svn-create-patch")] + changed_files,
            cwd=self.checkout_root, return_stderr=False,
            decode_output=False)

    def committer_email_for_revision(self, revision):
        return self._run_svn(["propget", "svn:author", "--revprop", "-r", revision]).rstrip()

    def contents_at_revision(self, path, revision):
        """Returns a byte array (str()) containing the contents
        of path @ revision in the repository."""
        remote_path = "%s/%s" % (self._repository_url(), path)
        return self._run_svn(["cat", "-r", revision, remote_path], decode_output=False)

    def diff_for_revision(self, revision):
        # FIXME: This should probably use cwd=self.checkout_root
        return self._run_svn(['diff', '-c', revision])

    def _bogus_dir_name(self):
        rnd = ''.join(random.sample(string.ascii_letters, 5))
        if sys.platform.startswith("win"):
            parent_dir = tempfile.gettempdir()
        else:
            parent_dir = sys.path[0]  # tempdir is not secure.
        return os.path.join(parent_dir, "temp_svn_config_" + rnd)

    def _setup_bogus_dir(self, log):
        self._bogus_dir = self._bogus_dir_name()
        if not os.path.exists(self._bogus_dir):
            os.mkdir(self._bogus_dir)
            self._delete_bogus_dir = True
        else:
            self._delete_bogus_dir = False
        if log:
            log.debug('  Html: temp config dir: "%s".', self._bogus_dir)

    def _teardown_bogus_dir(self, log):
        if self._delete_bogus_dir:
            shutil.rmtree(self._bogus_dir, True)
            if log:
                log.debug('  Html: removed temp config dir: "%s".', self._bogus_dir)
        self._bogus_dir = None

    def diff_for_file(self, path, log=None):
        self._setup_bogus_dir(log)
        try:
            args = ['diff']
            if self._bogus_dir:
                args += ['--config-dir', self._bogus_dir]
            args.append(path)
            return self._run_svn(args, cwd=self.checkout_root)
        finally:
            self._teardown_bogus_dir(log)

    def show_head(self, path):
        return self._run_svn(['cat', '-r', 'BASE', path], decode_output=False)

    def _repository_url(self):
        return self.value_from_svn_info(self.checkout_root, 'URL')

    def apply_reverse_diff(self, revision):
        # '-c -revision' applies the inverse diff of 'revision'
        svn_merge_args = ['merge', '--non-interactive', '-c', '-%s' % revision, self._repository_url()]
        _log.warning("svn merge has been known to take more than 10 minutes to complete.  It is recommended you use git for rollouts.")
        _log.debug("Running 'svn %s'" % " ".join(svn_merge_args))
        # FIXME: Should this use cwd=self.checkout_root?
        self._run_svn(svn_merge_args)

    def revert_files(self, file_paths):
        # FIXME: This should probably use cwd=self.checkout_root.
        self._run_svn(['revert'] + file_paths)

    def commit_with_message(self, message, username=None, password=None, git_commit=None, force_squash=False, changed_files=None):
        # git-commit and force are not used by SVN.
        svn_commit_args = ["commit"]

        if not username and not self.has_authorization_for_realm(self.svn_server_realm):
            raise AuthenticationError(self.svn_server_host)
        if username:
            svn_commit_args.extend(["--username", username])

        svn_commit_args.extend(["-m", message])

        if changed_files:
            svn_commit_args.extend(changed_files)

        return self._run_svn(svn_commit_args, cwd=self.checkout_root, error_handler=commit_error_handler)

    def svn_commit_log(self, svn_revision):
        svn_revision = self.strip_r_from_svn_revision(svn_revision)
        return self._run_svn(['log', '--non-interactive', '--revision', svn_revision])

    def last_svn_commit_log(self):
        # BASE is the checkout revision, HEAD is the remote repository revision
        # http://svnbook.red-bean.com/en/1.0/ch03s03.html
        return self.svn_commit_log('BASE')

    def svn_blame(self, path):
        return self._run_svn(['blame', path])

    def propset(self, pname, pvalue, path):
        dir, base = os.path.split(path)
        return self._run_svn(['pset', pname, pvalue, base], cwd=dir)

    def propget(self, pname, path):
        dir, base = os.path.split(path)
        return self._run_svn(['pget', pname, base], cwd=dir).encode('utf-8').rstrip("\n")
