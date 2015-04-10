# Copyright (c) 2009 Google Inc. All rights reserved.
# Copyright (c) 2009 Apple Inc. All rights reserved.
# Copyright (c) 2010 Research In Motion Limited. All rights reserved.
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

import re

from .attachment import Attachment


class Bug(object):
    # FIXME: This class is kinda a hack for now.  It exists so we have one
    # place to hold bug logic, even if much of the code deals with
    # dictionaries still.

    def __init__(self, bug_dictionary, bugzilla):
        self.bug_dictionary = bug_dictionary
        self._bugzilla = bugzilla

    def id(self):
        return self.bug_dictionary["id"]

    def title(self):
        # FIXME: Do we need to HTML unescape the title?
        return self.bug_dictionary["title"]

    def reporter_email(self):
        return self.bug_dictionary["reporter_email"]

    def assigned_to_email(self):
        return self.bug_dictionary["assigned_to_email"]

    def cc_emails(self):
        return self.bug_dictionary["cc_emails"]

    # FIXME: This information should be stored in some sort of webkit_config.py instead of here.
    unassigned_emails = frozenset([
        "webkit-unassigned@lists.webkit.org",
        "webkit-qt-unassigned@trolltech.com",
    ])

    def is_unassigned(self):
        return self.assigned_to_email() in self.unassigned_emails

    def status(self):
        return self.bug_dictionary["bug_status"]

    # Bugzilla has many status states we don't really use in WebKit:
    # https://bugs.webkit.org/page.cgi?id=fields.html#status
    _open_states = ["UNCONFIRMED", "NEW", "ASSIGNED", "REOPENED"]
    _closed_states = ["RESOLVED", "VERIFIED", "CLOSED"]

    def is_open(self):
        return self.status() in self._open_states

    def is_closed(self):
        return not self.is_open()

    def duplicate_of(self):
        return self.bug_dictionary.get('dup_id', None)

    # Rarely do we actually want obsolete attachments
    def attachments(self, include_obsolete=False):
        attachments = self.bug_dictionary["attachments"]
        if not include_obsolete:
            attachments = filter(lambda attachment:
                                 not attachment["is_obsolete"], attachments)
        return [Attachment(attachment, self) for attachment in attachments]

    def patches(self, include_obsolete=False):
        return [patch for patch in self.attachments(include_obsolete)
                                   if patch.is_patch()]

    def unreviewed_patches(self):
        return [patch for patch in self.patches() if patch.review() == "?"]

    def reviewed_patches(self, include_invalid=False):
        patches = [patch for patch in self.patches() if patch.review() == "+"]
        if include_invalid:
            return patches
        # Checking reviewer() ensures that it was both reviewed and has a valid
        # reviewer.
        return filter(lambda patch: patch.reviewer(), patches)

    def commit_queued_patches(self, include_invalid=False):
        patches = [patch for patch in self.patches()
                                      if patch.commit_queue() == "+"]
        if include_invalid:
            return patches
        # Checking committer() ensures that it was both commit-queue+'d and has
        # a valid committer.
        return filter(lambda patch: patch.committer(), patches)

    def comments(self):
        return self.bug_dictionary["comments"]

    def is_in_comments(self, message):
        for comment in self.comments():
            if message in comment["text"]:
                return True
        return False

    def commit_revision(self):
        # Sort the comments in reverse order as we want the latest committed revision.
        r = re.compile("Committed r(?P<svn_revision>\d+)")
        for comment in sorted(self.comments(), reverse=True):
            rev = r.search(comment['text'])
            if rev:
                return int(rev.group('svn_revision'))

        return None
