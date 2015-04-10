# Copyright (C) 2009 Google Inc. All rights reserved.
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

import unittest2 as unittest

from .bug import Bug


class BugTest(unittest.TestCase):
    def test_is_unassigned(self):
        for email in Bug.unassigned_emails:
            bug = Bug({"assigned_to_email": email}, bugzilla=None)
            self.assertTrue(bug.is_unassigned())
        bug = Bug({"assigned_to_email": "test@test.com"}, bugzilla=None)
        self.assertFalse(bug.is_unassigned())

    def test_is_in_comments(self):
        bug = Bug({"comments": [{"text": "Message1."},
                                {"text": "Message2. Message3. Message4."}, ], },
                  bugzilla=None)
        self.assertTrue(bug.is_in_comments("Message3."))
        self.assertFalse(bug.is_in_comments("Message."))

    def test_commit_revision(self):
        bug = Bug({"comments": []}, bugzilla=None)
        self.assertEqual(bug.commit_revision(), None)

        bug = Bug({"comments": [
            {"text": "Comment 1"},
            {"text": "Comment 2"},
            ]}, bugzilla=None)
        self.assertEqual(bug.commit_revision(), None)

        bug = Bug({"comments": [
            {"text": "Committed r138776: <http://trac.webkit.org/changeset/138776>"},
            ]}, bugzilla=None)
        self.assertEqual(bug.commit_revision(), 138776)

        bug = Bug({"comments": [
            {"text": "(From update of attachment 181269) Clearing flags on attachment: 181269 Committed r138776: <http://trac.webkit.org/changeset/138776>"},
            ]}, bugzilla=None)
        self.assertEqual(bug.commit_revision(), 138776)

        bug = Bug({"comments": [
            {"text": "Comment before"},
            {"text": "(From update of attachment 181269) Clearing flags on attachment: 181269 Committed r138776: <http://trac.webkit.org/changeset/138776>"},
            {"text": "Comment after"},
            ]}, bugzilla=None)
        self.assertEqual(bug.commit_revision(), 138776)

        bug = Bug({"comments": [
            {"text": "Comment before"},
            {"text": "(From update of attachment 181269) Clearing flags on attachment: 181269 Committed r138776: <http://trac.webkit.org/changeset/138776>"},
            {"text": "Comment Middle"},
            {"text": "(From update of attachment 181280) Clearing flags on attachment: 181280 Committed r138976: <http://trac.webkit.org/changeset/138976>"},
            {"text": "Comment After"},
            ]}, bugzilla=None)
        self.assertEqual(bug.commit_revision(), 138976)
