# Copyright (C) 2009 Google Inc. All rights reserved.
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

from model.queues import Queue
from model.queuestatus import QueueStatus
from model.workitems import WorkItems


class Attachment(object):
    @classmethod
    def recent(cls, limit=1):
        statuses = QueueStatus.all().order("-date")
        # Notice that we use both a set and a list here to keep the -date ordering.
        ids = []
        visited_ids = set()
        for status in statuses:
            attachment_id = status.active_patch_id
            if not attachment_id:
                continue
            if attachment_id in visited_ids:
                continue
            visited_ids.add(attachment_id)
            ids.append(attachment_id)
            if len(visited_ids) >= limit:
                break
        return map(cls, ids)

    def __init__(self, attachment_id):
        self.id = attachment_id
        self._summary = None
        self._cached_queue_positions = None

    def summary(self):
        if self._summary:
            return self._summary
        self._summary = self._fetch_summary()
        return self._summary

    def state_from_queue_status(self, status):
        table = {
            "Pass" : "pass",
            "Fail" : "fail",
        }
        state = table.get(status.message)
        if state:
            return state
        if status.message.startswith("Error:"):
            return "error"
        if status:
            return "pending"
        return None

    def position_in_queue(self, queue):
        return self._queue_positions().get(queue.name())

    def status_for_queue(self, queue):
        # summary() is a horrible API and should be killed.
        queue_summary = self.summary().get(queue.name_with_underscores())
        if not queue_summary:
            return None
        return queue_summary.get("status")

    def bug_id(self):
        return self.summary().get("bug_id")

    def _queue_positions(self):
        if self._cached_queue_positions:
            return self._cached_queue_positions
        # FIXME: Should we be mem-caching this?
        self._cached_queue_positions = self._calculate_queue_positions()
        return self._cached_queue_positions

    def _calculate_queue_positions(self):
        all_work_items = WorkItems.all().fetch(limit=len(Queue.all()))
        return dict([(items.queue.name(), items.display_position_for_attachment(self.id)) for items in all_work_items])

    # FIXME: This is controller/view code and does not belong in a model.
    def _fetch_summary(self):
        summary = { "attachment_id" : self.id }

        first_status = QueueStatus.all().filter('active_patch_id =', self.id).get()
        if not first_status:
            # We don't have any record of this attachment.
            return summary
        summary["bug_id"] = first_status.active_bug_id

        for queue in Queue.all():
            summary[queue.name_with_underscores()] = None
            status = QueueStatus.all().filter('queue_name =', queue.name()).filter('active_patch_id =', self.id).order('-date').get()
            if status:
                # summary() is a horrible API and should be killed.
                summary[queue.name_with_underscores()] = {
                    "state": self.state_from_queue_status(status),
                    "status": status,
                }
        return summary
