# Copyright (c) 2009 Google Inc. All rights reserved.
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
#
# Python module for reading stored web credentials from the OS.

import logging
import os
import platform
import re

from webkitpy.common.checkout.scm import Git
from webkitpy.common.system.executive import Executive, ScriptError
from webkitpy.common.system.user import User

try:
    # Use keyring, a cross platform keyring interface, as a fallback:
    # http://pypi.python.org/pypi/keyring
    import keyring
except ImportError:
    keyring = None

_log = logging.getLogger(__name__)


class Credentials(object):
    _environ_prefix = "webkit_bugzilla_"

    def __init__(self, host, git_prefix=None, executive=None, cwd=os.getcwd(),
                 keyring=keyring):
        self.host = host
        self.git_prefix = "%s." % git_prefix if git_prefix else ""
        self.executive = executive or Executive()
        self.cwd = cwd
        self._keyring = keyring

    def _credentials_from_git(self):
        try:
            if not Git.in_working_directory(self.cwd):
                return (None, None)
            return (Git.read_git_config(self.git_prefix + "username"),
                    Git.read_git_config(self.git_prefix + "password"))
        except OSError, e:
            # Catch and ignore OSError exceptions such as "no such file
            # or directory" (OSError errno 2), which imply that the Git
            # command cannot be found/is not installed.
            pass
        return (None, None)

    def _keychain_value_with_label(self, label, source_text):
        match = re.search("%s\"(?P<value>.+)\"" % label,
                                                  source_text,
                                                  re.MULTILINE)
        if match:
            return match.group('value')

    def _is_mac_os_x(self):
        return platform.mac_ver()[0]

    def _parse_security_tool_output(self, security_output):
        username = self._keychain_value_with_label("^\s*\"acct\"<blob>=",
                                                   security_output)
        password = self._keychain_value_with_label("^password: ",
                                                   security_output)
        return [username, password]

    def _run_security_tool(self, username=None):
        security_command = [
            "/usr/bin/security",
            "find-internet-password",
            "-g",
            "-s",
            self.host,
        ]
        if username:
            security_command += ["-a", username]

        _log.info("Reading Keychain for %s account and password.  "
                  "Click \"Allow\" to continue..." % self.host)
        try:
            return self.executive.run_command(security_command)
        except ScriptError:
            # Failed to either find a keychain entry or somekind of OS-related
            # error occured (for instance, couldn't find the /usr/sbin/security
            # command).
            _log.error("Could not find a keychain entry for %s." % self.host)
            return None

    def _credentials_from_keychain(self, username=None):
        if not self._is_mac_os_x():
            return [username, None]

        security_output = self._run_security_tool(username)
        if security_output:
            return self._parse_security_tool_output(security_output)
        else:
            return [None, None]

    def _read_environ(self, key):
        environ_key = self._environ_prefix + key
        return os.environ.get(environ_key.upper())

    def _credentials_from_environment(self):
        return (self._read_environ("username"), self._read_environ("password"))

    def _offer_to_store_credentials_in_keyring(self, username, password):
        if not self._keyring:
            return
        if not User().confirm("Store password in system keyring?", User.DEFAULT_NO):
            return
        try:
            self._keyring.set_password(self.host, username, password)
        except:
            pass

    def read_credentials(self, user=User):
        username, password = self._credentials_from_environment()
        # FIXME: We don't currently support pulling the username from one
        # source and the password from a separate source.
        if not username or not password:
            username, password = self._credentials_from_git()
        if not username or not password:
            username, password = self._credentials_from_keychain(username)

        if not username:
            username = user.prompt("%s login: " % self.host)

        if username and not password and self._keyring:
            try:
                password = self._keyring.get_password(self.host, username)
            except:
                pass

        if not password:
            password = user.prompt_password("%s password for %s: " % (self.host, username))
            self._offer_to_store_credentials_in_keyring(username, password)

        return (username, password)
