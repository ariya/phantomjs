# Copyright (c) 2010 Google Inc. All rights reserved.
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

from webkitpy.common.config.committervalidator import CommitterValidator
from webkitpy.tool.grammar import pluralize

_log = logging.getLogger(__name__)


class AbstractFeeder(object):
    def __init__(self, tool):
        self._tool = tool

    def feed(self):
        raise NotImplementedError("subclasses must implement")


class CommitQueueFeeder(AbstractFeeder):
    queue_name = "commit-queue"

    def __init__(self, tool):
        AbstractFeeder.__init__(self, tool)
        self.committer_validator = CommitterValidator(self._tool)

    def _update_work_items(self, item_ids):
        # FIXME: This is the last use of update_work_items, the commit-queue
        # should move to feeding patches one at a time like the EWS does.
        self._tool.status_server.update_work_items(self.queue_name, item_ids)
        _log.info("Feeding %s items %s" % (self.queue_name, item_ids))

    def feed(self):
        patches = self._validate_patches()
        patches = self._patches_with_acceptable_review_flag(patches)
        patches = sorted(patches, self._patch_cmp)
        patch_ids = [patch.id() for patch in patches]
        self._update_work_items(patch_ids)

    def _patches_for_bug(self, bug_id):
        return self._tool.bugs.fetch_bug(bug_id).commit_queued_patches(include_invalid=True)

    # Filters out patches with r? or r-, only r+ or no review are OK to land.
    def _patches_with_acceptable_review_flag(self, patches):
        return [patch for patch in patches if patch.review() in [None, '+']]

    def _validate_patches(self):
        # Not using BugzillaQueries.fetch_patches_from_commit_queue() so we can reject patches with invalid committers/reviewers.
        bug_ids = self._tool.bugs.queries.fetch_bug_ids_from_commit_queue()
        all_patches = sum([self._patches_for_bug(bug_id) for bug_id in bug_ids], [])
        return self.committer_validator.patches_after_rejecting_invalid_commiters_and_reviewers(all_patches)

    def _patch_cmp(self, a, b):
        # Sort first by is_rollout, then by attach_date.
        # Reversing the order so that is_rollout is first.
        rollout_cmp = cmp(b.is_rollout(), a.is_rollout())
        if rollout_cmp != 0:
            return rollout_cmp
        return cmp(a.attach_date(), b.attach_date())


class EWSFeeder(AbstractFeeder):
    def __init__(self, tool):
        self._ids_sent_to_server = set()
        AbstractFeeder.__init__(self, tool)

    def feed(self):
        ids_needing_review = set(self._tool.bugs.queries.fetch_attachment_ids_from_review_queue())
        new_ids = ids_needing_review.difference(self._ids_sent_to_server)
        _log.info("Feeding EWS (%s, %s new)" % (pluralize("r? patch", len(ids_needing_review)), len(new_ids)))
        for attachment_id in new_ids:  # Order doesn't really matter for the EWS.
            self._tool.status_server.submit_to_ews(attachment_id)
            self._ids_sent_to_server.add(attachment_id)
