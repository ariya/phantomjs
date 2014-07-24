#!/usr/bin/env python
#
# Copyright (C) 2011 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import StringIO
import __builtin__
import buildbot.status.web.auth
import contextlib
import os
import unittest

from committer_auth import CommitterAuth


# This subclass of StringIO supports the context manager protocol so it works
# with "with" statements, just like real files.
class CMStringIO(StringIO.StringIO):
    def __enter__(self):
        return self

    def __exit__(self, exception, value, traceback):
        self.close()


@contextlib.contextmanager
def open_override(func):
    original_open = __builtin__.open
    __builtin__.open = func
    yield
    __builtin__.open = original_open


class CommitterAuthTest(unittest.TestCase):
    def setUp(self):
        self.auth = CommitterAuth('path/to/auth.json')
        self.auth.open_auth_json_file = self.fake_auth_json_file
        self.auth.open_webkit_committers_file = self.fake_committers_file
        self.auth.open_trac_credentials_file = self.fake_htdigest_file

    def fake_open_function(self, expected_filename):
        def fake_open(name, mode='r'):
            self.fake_open_was_called = True
            self.assertEqual(expected_filename, name)
        return fake_open

    def test_authentication_success(self):
        self.assertTrue(self.auth.authenticate('committer@webkit.org', 'committerpassword'))
        self.assertEqual('', self.auth.errmsg())
        self.assertTrue(self.auth.authenticate('committer2@example.com', 'committer2password'))
        self.assertEqual('', self.auth.errmsg())

    def test_committer_without_trac_credentials_fails(self):
        self.assertFalse(self.auth.authenticate('committer3@webkit.org', 'committer3password'))
        self.assertEqual('Invalid username/password', self.auth.errmsg())

    def test_fail_to_open_auth_json_file(self):
        def raise_IOError():
            raise IOError(2, 'No such file or directory', 'path/to/auth.json')
        auth = CommitterAuth('path/to/auth.json')
        auth.open_auth_json_file = raise_IOError
        self.assertFalse(auth.authenticate('committer@webkit.org', 'committerpassword'))
        self.assertEqual('Error opening auth.json file: No such file or directory', auth.errmsg())

    def test_fail_to_open_trac_credentials_file(self):
        def raise_IOError():
            raise IOError(2, 'No such file or directory', 'path/to/trac/credentials')
        self.auth.open_trac_credentials_file = raise_IOError
        self.assertFalse(self.auth.authenticate('committer@webkit.org', 'committerpassword'))
        self.assertEqual('Error opening Trac credentials file: No such file or directory', self.auth.errmsg())

    def test_fail_to_open_webkit_committers_file(self):
        def raise_IOError():
            raise IOError(2, 'No such file or directory', 'path/to/webkit/committers')
        self.auth.open_webkit_committers_file = raise_IOError
        self.assertFalse(self.auth.authenticate('committer@webkit.org', 'committerpassword'))
        self.assertEqual('Error opening WebKit committers file: No such file or directory', self.auth.errmsg())

    def test_implements_IAuth(self):
        self.assertTrue(buildbot.status.web.auth.IAuth.implementedBy(CommitterAuth))

    def test_invalid_auth_json_file(self):
        auth = CommitterAuth('path/to/auth.json')
        auth.open_auth_json_file = self.invalid_auth_json_file
        self.assertFalse(auth.authenticate('committer@webkit.org', 'committerpassword'))
        self.assertEqual('Error parsing auth.json file: No JSON object could be decoded', auth.errmsg())

    def test_invalid_committers_file(self):
        self.auth.open_webkit_committers_file = self.invalid_committers_file
        self.assertFalse(self.auth.authenticate('committer@webkit.org', 'committerpassword'))
        self.assertEqual('Error parsing WebKit committers file', self.auth.errmsg())

    def test_invalid_trac_credentials_file(self):
        self.auth.open_trac_credentials_file = self.invalid_htdigest_file
        self.assertFalse(self.auth.authenticate('committer@webkit.org', 'committerpassword'))
        self.assertEqual('Error parsing Trac credentials file', self.auth.errmsg())

    def test_missing_auth_json_keys(self):
        auth = CommitterAuth('path/to/auth.json')
        auth.open_auth_json_file = lambda: CMStringIO('{ "trac_credentials": "path/to/trac/credentials" }')
        self.assertFalse(auth.authenticate('committer@webkit.org', 'committerpassword'))
        self.assertEqual('auth.json file is missing "webkit_committers" key', auth.errmsg())

        auth.open_auth_json_file = lambda: CMStringIO('{ "webkit_committers": "path/to/webkit/committers" }')
        auth.open_webkit_committers_file = self.fake_committers_file
        self.assertFalse(auth.authenticate('committer@webkit.org', 'committerpassword'))
        self.assertEqual('auth.json file is missing "trac_credentials" key', auth.errmsg())

    def test_open_auth_json_file(self):
        auth = CommitterAuth('path/to/auth.json')
        self.fake_open_was_called = False
        with open_override(self.fake_open_function(auth.auth_json_filename())):
            auth.open_auth_json_file()
        self.assertTrue(self.fake_open_was_called)

    def test_open_trac_credentials_file(self):
        auth = CommitterAuth('path/to/auth.json')
        auth.trac_credentials_filename = lambda: 'trac credentials filename'
        self.fake_open_was_called = False
        with open_override(self.fake_open_function(auth.trac_credentials_filename())):
            auth.open_trac_credentials_file()
        self.assertTrue(self.fake_open_was_called)

    def test_open_webkit_committers_file(self):
        auth = CommitterAuth('path/to/auth.json')
        auth.webkit_committers_filename = lambda: 'webkit committers filename'
        self.fake_open_was_called = False
        with open_override(self.fake_open_function(auth.webkit_committers_filename())):
            auth.open_webkit_committers_file()
        self.assertTrue(self.fake_open_was_called)

    def test_non_committer_fails(self):
        self.assertFalse(self.auth.authenticate('noncommitter@example.com', 'noncommitterpassword'))
        self.assertEqual('Invalid username/password', self.auth.errmsg())

    def test_trac_credentials_filename(self):
        self.assertEqual('path/to/trac/credentials', self.auth.trac_credentials_filename())

    def test_unknown_user_fails(self):
        self.assertFalse(self.auth.authenticate('nobody@example.com', 'nobodypassword'))
        self.assertEqual('Invalid username/password', self.auth.errmsg())

    def test_username_is_prefix_of_valid_user(self):
        self.assertFalse(self.auth.authenticate('committer@webkit.orgg', 'committerpassword'))
        self.assertEqual('Invalid username/password', self.auth.errmsg())

    def test_webkit_committers(self):
        self.assertEqual(['committer@webkit.org', 'committer2@example.com', 'committer3@webkit.org'], self.auth.webkit_committers())

    def test_webkit_committers_filename(self):
        self.assertEqual('path/to/webkit/committers', self.auth.webkit_committers_filename())

    def test_wrong_password_fails(self):
        self.assertFalse(self.auth.authenticate('committer@webkit.org', 'wrongpassword'))
        self.assertEqual('Invalid username/password', self.auth.errmsg())

    def fake_auth_json_file(self):
        return CMStringIO("""{
    "trac_credentials": "path/to/trac/credentials",
    "webkit_committers": "path/to/webkit/committers"
}""")

    def invalid_auth_json_file(self):
        return CMStringIO('~!@#$%^&*()_+')

    def fake_committers_file(self):
        return CMStringIO("""[groups]
group1 = user@example.com,user2@example.com
group2 = user3@example.com

group3 =
group4 =

webkit = committer@webkit.org,committer2@example.com,committer3@webkit.org

[service:/]
*    = r
""")

    def invalid_committers_file(self):
        return CMStringIO("""[groups]

[[groups2]
""")

    def fake_htdigest_file(self):
        return CMStringIO("""committer@webkit.org:Mac OS Forge:761c8dcb7d9b5908007ed142f62fe73a
committer2@example.com:Mac OS Forge:faeee69acc2e49af3a0dbb15bd593ef4
noncommitter@example.com:Mac OS Forge:b99aa7ad32306a654ca4d57839fde9c1
""")

    def invalid_htdigest_file(self):
        return CMStringIO("""committer@webkit.org:Mac OS Forge:761c8dcb7d9b5908007ed142f62fe73a
committer2@example.com:Mac OS Forge:faeee69acc2e49af3a0dbb15bd593ef4
noncommitter@example.com:Mac OS Forge:b99aa7ad32306a654ca4d57839fde9c1
committer4@example.com:Mac OS Forge:::
""")


if __name__ == '__main__':
    unittest.main()
