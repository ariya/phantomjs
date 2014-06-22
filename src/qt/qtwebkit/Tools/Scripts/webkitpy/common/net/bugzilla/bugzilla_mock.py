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

import datetime
import logging

from .bug import Bug
from .attachment import Attachment
from webkitpy.common.config.committers import CommitterList, Reviewer

_log = logging.getLogger(__name__)


def _id_to_object_dictionary(*objects):
    dictionary = {}
    for thing in objects:
        dictionary[thing["id"]] = thing
    return dictionary

# Testing


_patch1 = {
    "id": 10000,
    "bug_id": 50000,
    "url": "http://example.com/10000",
    "name": "Patch1",
    "is_obsolete": False,
    "is_patch": True,
    "review": "+",
    "reviewer_email": "foo@bar.com",
    "commit-queue": "+",
    "committer_email": "foo@bar.com",
    "attacher_email": "Contributer1",
}


_patch2 = {
    "id": 10001,
    "bug_id": 50000,
    "url": "http://example.com/10001",
    "name": "Patch2",
    "is_obsolete": False,
    "is_patch": True,
    "review": "+",
    "reviewer_email": "reviewer2@webkit.org",
    "commit-queue": "+",
    "committer_email": "non-committer@example.com",
    "attacher_email": "eric@webkit.org",
}


_patch3 = {
    "id": 10002,
    "bug_id": 50001,
    "url": "http://example.com/10002",
    "name": "Patch3",
    "is_obsolete": False,
    "is_patch": True,
    "review": "?",
    "commit-queue": "-",
    "attacher_email": "eric@webkit.org",
    "attach_date": datetime.datetime.today(),
}


_patch4 = {
    "id": 10003,
    "bug_id": 50003,
    "url": "http://example.com/10002",
    "name": "Patch3",
    "is_obsolete": False,
    "is_patch": True,
    "review": "+",
    "commit-queue": "?",
    "reviewer_email": "foo@bar.com",
    "attacher_email": "Contributer2",
}


_patch5 = {
    "id": 10004,
    "bug_id": 50003,
    "url": "http://example.com/10002",
    "name": "Patch5",
    "is_obsolete": False,
    "is_patch": True,
    "review": "+",
    "reviewer_email": "foo@bar.com",
    "attacher_email": "eric@webkit.org",
}


_patch6 = {  # Valid committer, but no reviewer.
    "id": 10005,
    "bug_id": 50003,
    "url": "http://example.com/10002",
    "name": "ROLLOUT of r3489",
    "is_obsolete": False,
    "is_patch": True,
    "commit-queue": "+",
    "committer_email": "foo@bar.com",
    "attacher_email": "eric@webkit.org",
}


_patch7 = {  # Valid review, patch is marked obsolete.
    "id": 10006,
    "bug_id": 50002,
    "url": "http://example.com/10002",
    "name": "Patch7",
    "is_obsolete": True,
    "is_patch": True,
    "review": "+",
    "reviewer_email": "foo@bar.com",
    "attacher_email": "eric@webkit.org",
}


# This matches one of Bug.unassigned_emails
_unassigned_email = "webkit-unassigned@lists.webkit.org"
# This is needed for the FlakyTestReporter to believe the bug
# was filed by one of the webkitpy bots.
_commit_queue_email = "commit-queue@webkit.org"


_bug1 = {
    "id": 50000,
    "title": "Bug with two r+'d and cq+'d patches, one of which has an "
             "invalid commit-queue setter.",
    "reporter_email": "foo@foo.com",
    "assigned_to_email": _unassigned_email,
    "cc_emails": [],
    "attachments": [_patch1, _patch2],
    "bug_status": "UNCONFIRMED",
    "comments": [],
}


_bug2 = {
    "id": 50001,
    "title": "Bug with a patch needing review.",
    "reporter_email": "eric@webkit.org",
    "assigned_to_email": "foo@foo.com",
    "cc_emails": ["abarth@webkit.org", ],
    "attachments": [_patch3],
    "bug_status": "ASSIGNED",
    "comments": [{"comment_date":  datetime.datetime(2011, 6, 11, 9, 4, 3),
                  "comment_email": "bar@foo.com",
                  "text": "Message1.\nCommitted r35: <http://trac.webkit.org/changeset/35>",
                  },
                 ],
}


_bug3 = {
    "id": 50002,
    "title": "The third bug",
    "reporter_email": "foo@foo.com",
    "assigned_to_email": _unassigned_email,
    "cc_emails": [],
    "attachments": [_patch7],
    "bug_status": "NEW",
    "comments":  [{"comment_date":  datetime.datetime(2011, 6, 11, 9, 4, 3),
                   "comment_email": "bar@foo.com",
                   "text": "Committed r30: <http://trac.webkit.org/changeset/30>",
                   },
                  {"comment_date":  datetime.datetime(2011, 6, 11, 9, 4, 3),
                   "comment_email": "bar@foo.com",
                   "text": "Committed r31: <http://trac.webkit.org/changeset/31>",
                   },
                  ],
}


_bug4 = {
    "id": 50003,
    "title": "The fourth bug",
    "reporter_email": "foo@foo.com",
    "assigned_to_email": "foo@foo.com",
    "cc_emails": [],
    "attachments": [_patch4, _patch5, _patch6],
    "bug_status": "REOPENED",
    "comments": [{"comment_date":  datetime.datetime(2011, 6, 11, 9, 4, 3),
                  "comment_email": "bar@foo.com",
                  "text": "Committed r25: <http://trac.webkit.org/changeset/30>",
                  },
                 {"comment_date":  datetime.datetime(2011, 6, 11, 9, 4, 3),
                  "comment_email": "bar@foo.com",
                  "text": "Rolled out in <http://trac.webkit.org/changeset/26",
                  },
                 ],
}


_bug5 = {
    "id": 50004,
    "title": "The fifth bug",
    "reporter_email": _commit_queue_email,
    "assigned_to_email": "foo@foo.com",
    "cc_emails": [],
    "attachments": [],
    "bug_status": "RESOLVED",
    "dup_id": 50002,
    "comments": [{"comment_date":  datetime.datetime(2011, 6, 11, 9, 4, 3),
                  "comment_email": "bar@foo.com",
                  "text": "Committed r15: <http://trac.webkit.org/changeset/15>",
                  },
                 ],

}


class MockBugzillaQueries(object):

    def __init__(self, bugzilla):
        self._bugzilla = bugzilla

    def _all_bugs(self):
        return map(lambda bug_dictionary: Bug(bug_dictionary, self._bugzilla),
                   self._bugzilla.bug_cache.values())

    def fetch_bug_ids_from_commit_queue(self):
        bugs_with_commit_queued_patches = filter(
                lambda bug: bug.commit_queued_patches(),
                self._all_bugs())
        return map(lambda bug: bug.id(), bugs_with_commit_queued_patches)

    def fetch_attachment_ids_from_review_queue(self):
        unreviewed_patches = sum([bug.unreviewed_patches()
                                  for bug in self._all_bugs()], [])
        return map(lambda patch: patch.id(), unreviewed_patches)

    def fetch_patches_from_commit_queue(self):
        return sum([bug.commit_queued_patches()
                    for bug in self._all_bugs()], [])

    def fetch_bug_ids_from_pending_commit_list(self):
        bugs_with_reviewed_patches = filter(lambda bug: bug.reviewed_patches(),
                                            self._all_bugs())
        bug_ids = map(lambda bug: bug.id(), bugs_with_reviewed_patches)
        # NOTE: This manual hack here is to allow testing logging in
        # test_assign_to_committer the real pending-commit query on bugzilla
        # will return bugs with patches which have r+, but are also obsolete.
        return bug_ids + [50002]

    def fetch_bugs_from_review_queue(self, cc_email=None):
        unreviewed_bugs = [bug for bug in self._all_bugs() if bug.unreviewed_patches()]

        if cc_email:
            return [bug for bug in unreviewed_bugs if cc_email in bug.cc_emails()]

        return unreviewed_bugs

    def fetch_patches_from_pending_commit_list(self):
        return sum([bug.reviewed_patches() for bug in self._all_bugs()], [])

    def fetch_bugs_matching_search(self, search_string):
        return [self._bugzilla.fetch_bug(50004), self._bugzilla.fetch_bug(50003)]

    def fetch_bugs_matching_quicksearch(self, search_string):
        return [self._bugzilla.fetch_bug(50001), self._bugzilla.fetch_bug(50002),
                self._bugzilla.fetch_bug(50003), self._bugzilla.fetch_bug(50004)]


_mock_reviewers = [Reviewer("Foo Bar", "foo@bar.com"),
                   Reviewer("Reviewer2", "reviewer2@webkit.org")]


# FIXME: Bugzilla is the wrong Mock-point.  Once we have a BugzillaNetwork
#        class we should mock that instead.
# Most of this class is just copy/paste from Bugzilla.
class MockBugzilla(object):

    bug_server_url = "http://example.com"

    bug_cache = _id_to_object_dictionary(_bug1, _bug2, _bug3, _bug4, _bug5)

    attachment_cache = _id_to_object_dictionary(_patch1,
                                                _patch2,
                                                _patch3,
                                                _patch4,
                                                _patch5,
                                                _patch6,
                                                _patch7)

    def __init__(self):
        self.queries = MockBugzillaQueries(self)
        # FIXME: This should move onto the Host object, and we should use a MockCommitterList
        self.committers = CommitterList(reviewers=_mock_reviewers)
        self.username = None
        self._override_patch = None

    def authenticate(self):
        self.username = "username@webkit.org"

    def create_bug(self,
                   bug_title,
                   bug_description,
                   component=None,
                   diff=None,
                   patch_description=None,
                   cc=None,
                   blocked=None,
                   mark_for_review=False,
                   mark_for_commit_queue=False):
        _log.info("MOCK create_bug")
        _log.info("bug_title: %s" % bug_title)
        _log.info("bug_description: %s" % bug_description)
        if component:
            _log.info("component: %s" % component)
        if cc:
            _log.info("cc: %s" % cc)
        if blocked:
            _log.info("blocked: %s" % blocked)
        return 60001

    def quips(self):
        return ["Good artists copy. Great artists steal. - Pablo Picasso"]

    def fetch_bug(self, bug_id):
        return Bug(self.bug_cache.get(int(bug_id)), self)

    def set_override_patch(self, patch):
        self._override_patch = patch

    def fetch_attachment(self, attachment_id):
        if self._override_patch:
            return self._override_patch

        attachment_dictionary = self.attachment_cache.get(attachment_id)
        if not attachment_dictionary:
            print "MOCK: fetch_attachment: %s is not a known attachment id" % attachment_id
            return None
        bug = self.fetch_bug(attachment_dictionary["bug_id"])
        for attachment in bug.attachments(include_obsolete=True):
            if attachment.id() == int(attachment_id):
                return attachment

    def bug_url_for_bug_id(self, bug_id):
        return "%s/%s" % (self.bug_server_url, bug_id)

    def fetch_bug_dictionary(self, bug_id):
        return self.bug_cache.get(bug_id)

    def attachment_url_for_id(self, attachment_id, action="view"):
        action_param = ""
        if action and action != "view":
            action_param = "&action=%s" % action
        return "%s/%s%s" % (self.bug_server_url, attachment_id, action_param)

    def reassign_bug(self, bug_id, assignee=None, comment_text=None):
        _log.info("MOCK reassign_bug: bug_id=%s, assignee=%s" % (bug_id, assignee))
        if comment_text:
            _log.info("-- Begin comment --")
            _log.info(comment_text)
            _log.info("-- End comment --")

    def set_flag_on_attachment(self,
                               attachment_id,
                               flag_name,
                               flag_value,
                               comment_text=None):
        _log.info("MOCK setting flag '%s' to '%s' on attachment '%s' with comment '%s'" % (
                  flag_name, flag_value, attachment_id, comment_text))

    def post_comment_to_bug(self, bug_id, comment_text, cc=None):
        _log.info("MOCK bug comment: bug_id=%s, cc=%s\n--- Begin comment ---\n%s\n--- End comment ---\n" % (
                  bug_id, cc, comment_text))

    def add_attachment_to_bug(self, bug_id, file_or_string, description, filename=None, comment_text=None, mimetype=None):
        _log.info("MOCK add_attachment_to_bug: bug_id=%s, description=%s filename=%s mimetype=%s" %
                  (bug_id, description, filename, mimetype))
        if comment_text:
            _log.info("-- Begin comment --")
            _log.info(comment_text)
            _log.info("-- End comment --")

    def add_patch_to_bug(self,
                         bug_id,
                         diff,
                         description,
                         comment_text=None,
                         mark_for_review=False,
                         mark_for_commit_queue=False,
                         mark_for_landing=False):
        _log.info("MOCK add_patch_to_bug: bug_id=%s, description=%s, mark_for_review=%s, mark_for_commit_queue=%s, mark_for_landing=%s" %
                  (bug_id, description, mark_for_review, mark_for_commit_queue, mark_for_landing))
        if comment_text:
            _log.info("-- Begin comment --")
            _log.info(comment_text)
            _log.info("-- End comment --")

    def add_cc_to_bug(self, bug_id, ccs):
        pass

    def obsolete_attachment(self, attachment_id, message=None):
        pass

    def reopen_bug(self, bug_id, message):
        _log.info("MOCK reopen_bug %s with comment '%s'" % (bug_id, message))

    def close_bug_as_fixed(self, bug_id, message):
        pass

    def clear_attachment_flags(self, attachment_id, message):
        pass
