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
import os
import unittest2 as unittest

from htdigestparser import HTDigestParser


class HTDigestParserTest(unittest.TestCase):
    def assertEntriesEqual(self, entries, additional_content=None):
        digest_file = self.fake_htdigest_file()
        if additional_content is not None:
            digest_file.seek(pos=0, mode=os.SEEK_END)
            digest_file.write(additional_content)
            digest_file.seek(pos=0, mode=os.SEEK_SET)
        self.assertEqual(entries, HTDigestParser(digest_file).entries())

    def test_authenticate(self):
        htdigest = HTDigestParser(self.fake_htdigest_file())
        self.assertTrue(htdigest.authenticate('user1', 'realm 1', 'password1'))
        self.assertTrue(htdigest.authenticate('user2', 'realm 2', 'password2'))
        self.assertTrue(htdigest.authenticate('user3', 'realm 1', 'password3'))
        self.assertTrue(htdigest.authenticate('user3', 'realm 3', 'password3'))

        self.assertFalse(htdigest.authenticate('user1', 'realm', 'password1'))
        self.assertFalse(htdigest.authenticate('user1', 'realm 2', 'password1'))
        self.assertFalse(htdigest.authenticate('user2', 'realm 2', 'password1'))
        self.assertFalse(htdigest.authenticate('user2', 'realm 1', 'password1'))
        self.assertFalse(htdigest.authenticate('', '', ''))

    def test_entries(self):
        entries = [
            ['user1', 'realm 1', '36b8aa27fa5e9051095d37b619f92762'],
            ['user2', 'realm 2', '14f827686fa97778f02fe1314a3337c8'],
            ['user3', 'realm 1', '1817fc8a24119cc57fbafc8a630ea5a5'],
            ['user3', 'realm 3', 'a05f5a2335e9d87bbe75bbe5e53248f0'],
        ]
        self.assertEntriesEqual(entries)
        self.assertEntriesEqual(entries, additional_content='')

    def test_empty_file(self):
        self.assertEqual([], HTDigestParser(StringIO.StringIO()).entries())

    def test_too_few_colons(self):
        self.assertEntriesEqual([], additional_content='user1:realm 1\n')

    def test_too_many_colons(self):
        self.assertEntriesEqual([], additional_content='user1:realm 1:36b8aa27fa5e9051095d37b619f92762:garbage\n')

    def test_invalid_hash(self):
        self.assertEntriesEqual([], additional_content='user1:realm 1:36b8aa27fa5e9051095d37b619f92762000000\n')
        self.assertEntriesEqual([], additional_content='user1:realm 1:36b8aa27fa5e9051095d37b619f9276\n')
        self.assertEntriesEqual([], additional_content='user1:realm 1:36b8aa27fa5e9051095d37b619f9276z\n')
        self.assertEntriesEqual([], additional_content='user1:realm 1: 36b8aa27fa5e9051095d37b619f92762\n')

    def fake_htdigest_file(self):
        return StringIO.StringIO("""user1:realm 1:36b8aa27fa5e9051095d37b619f92762
user2:realm 2:14f827686fa97778f02fe1314a3337c8
user3:realm 1:1817fc8a24119cc57fbafc8a630ea5a5
user3:realm 3:a05f5a2335e9d87bbe75bbe5e53248f0
""")


# FIXME: We should run this file as part of test-rm .
# Unfortunately test-rm  currently requires that unittests
# be located in a directory with a valid module name.
# 'build.webkit.org-config' is not a valid module name (due to '.' and '-')
# so for now this is a stand-alone test harness.
if __name__ == '__main__':
    unittest.main()
