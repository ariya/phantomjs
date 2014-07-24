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

import logging

from webkitpy.common.memoized import memoized

_log = logging.getLogger(__name__)


class Attachment(object):

    rollout_preamble = "ROLLOUT of r"

    def __init__(self, attachment_dictionary, bug):
        self._attachment_dictionary = attachment_dictionary
        self._bug = bug
        # FIXME: These should be replaced with @memoized after updating mocks.
        self._reviewer = None
        self._committer = None

    def _bugzilla(self):
        return self._bug._bugzilla

    def id(self):
        return int(self._attachment_dictionary.get("id"))

    @memoized
    def attacher(self):
        return self._bugzilla().committers.contributor_by_email(self.attacher_email())

    def attacher_email(self):
        return self._attachment_dictionary.get("attacher_email")

    def bug(self):
        return self._bug

    def bug_id(self):
        return int(self._attachment_dictionary.get("bug_id"))

    def is_patch(self):
        return not not self._attachment_dictionary.get("is_patch")

    def is_obsolete(self):
        return not not self._attachment_dictionary.get("is_obsolete")

    def is_rollout(self):
        return self.name().startswith(self.rollout_preamble)

    def name(self):
        return self._attachment_dictionary.get("name")

    def attach_date(self):
        return self._attachment_dictionary.get("attach_date")

    def review(self):
        return self._attachment_dictionary.get("review")

    def commit_queue(self):
        return self._attachment_dictionary.get("commit-queue")

    def url(self):
        # FIXME: This should just return
        # self._bugzilla().attachment_url_for_id(self.id()). scm_unittest.py
        # depends on the current behavior.
        return self._attachment_dictionary.get("url")

    def contents(self):
        # FIXME: We shouldn't be grabbing at _bugzilla.
        return self._bug._bugzilla.fetch_attachment_contents(self.id())

    def _validate_flag_value(self, flag):
        email = self._attachment_dictionary.get("%s_email" % flag)
        if not email:
            return None
        # FIXME: This is not a robust way to call committer_by_email
        committer = getattr(self._bugzilla().committers,
                            "%s_by_email" % flag)(email)
        if committer:
            return committer
        _log.warning("Warning, attachment %s on bug %s has invalid %s (%s)" % (
                 self._attachment_dictionary['id'],
                 self._attachment_dictionary['bug_id'], flag, email))

    # FIXME: These could use @memoized like attacher(), but unit tests would need updates.
    def reviewer(self):
        if not self._reviewer:
            self._reviewer = self._validate_flag_value("reviewer")
        return self._reviewer

    def committer(self):
        if not self._committer:
            self._committer = self._validate_flag_value("committer")
        return self._committer
