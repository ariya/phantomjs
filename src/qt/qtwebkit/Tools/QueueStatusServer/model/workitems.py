# Copyright (C) 2010 Google Inc. All rights reserved.
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

from google.appengine.ext import db

from model.queuepropertymixin import QueuePropertyMixin


class WorkItems(db.Model, QueuePropertyMixin):
    queue_name = db.StringProperty()
    item_ids = db.ListProperty(int)
    date = db.DateTimeProperty(auto_now_add=True)

    @classmethod
    def key_for_queue(cls, queue_name):
        return "work-items-%s" % (queue_name)

    @classmethod
    def lookup_by_queue(cls, queue_name):
        return cls.get_or_insert(key_name=cls.key_for_queue(queue_name), queue_name=queue_name)

    def display_position_for_attachment(self, attachment_id):
        """Returns a 1-based index corresponding to the position
        of the attachment_id in the queue.  If the attachment is
        not in this queue, this returns None"""
        if attachment_id in self.item_ids:
            return self.item_ids.index(attachment_id) + 1
        return None

    @staticmethod
    def _unguarded_add(key, attachment_id):
        work_items = db.get(key)
        if attachment_id in work_items.item_ids:
            return
        work_items.item_ids.append(attachment_id)
        work_items.put()

    # Because this uses .key() self.is_saved() must be True or this will throw NotSavedError.
    def add_work_item(self, attachment_id):
        db.run_in_transaction(self._unguarded_add, self.key(), attachment_id)

    @staticmethod
    def _unguarded_remove(key, attachment_id):
        work_items = db.get(key)
        if attachment_id in work_items.item_ids:
            # We should never have more than one entry for a work item, so we only need remove the first.
            work_items.item_ids.remove(attachment_id)
        work_items.put()

    # Because this uses .key() self.is_saved() must be True or this will throw NotSavedError.
    def remove_work_item(self, attachment_id):
        db.run_in_transaction(self._unguarded_remove, self.key(), attachment_id)
