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

from datetime import timedelta, datetime
import time

from model.queuepropertymixin import QueuePropertyMixin


class ActiveWorkItems(db.Model, QueuePropertyMixin):
    queue_name = db.StringProperty()
    item_ids = db.ListProperty(int)
    item_dates = db.ListProperty(float)
    date = db.DateTimeProperty(auto_now_add=True)

    # The id/date pairs should probably just be their own class.
    def _item_time_pairs(self):
        return zip(self.item_ids, self.item_dates)

    def _set_item_time_pairs(self, pairs):
        if pairs:
            # The * operator raises on an empty list.
            # db.Model does not tuples, we have to make lists.
            self.item_ids, self.item_dates = map(list, zip(*pairs))
        else:
            self.item_ids = []
            self.item_dates = []

    def _append_item_time_pair(self, pair):
        self.item_ids.append(pair[0])
        self.item_dates.append(pair[1])

    def _remove_item(self, item_id):
        nonexpired_pairs = [pair for pair in self._item_time_pairs() if pair[0] != item_id]
        self._set_item_time_pairs(nonexpired_pairs)

    @classmethod
    def key_for_queue(cls, queue_name):
        return "active-work-items-%s" % (queue_name)

    @classmethod
    def lookup_by_queue(cls, queue_name):
        return cls.get_or_insert(key_name=cls.key_for_queue(queue_name), queue_name=queue_name)

    @staticmethod
    def _expire_item(key, item_id):
        active_work_items = db.get(key)
        active_work_items._remove_item(item_id)
        active_work_items.put()

    def expire_item(self, item_id):
        return db.run_in_transaction(self._expire_item, self.key(), item_id)

    def deactivate_expired(self, now):
        one_hour_ago = time.mktime((now - timedelta(minutes=60)).timetuple())
        nonexpired_pairs = [pair for pair in self._item_time_pairs() if pair[1] > one_hour_ago]
        self._set_item_time_pairs(nonexpired_pairs)

    def next_item(self, work_item_ids, now):
        for item_id in work_item_ids:
            if item_id not in self.item_ids:
                self._append_item_time_pair([item_id, time.mktime(now.timetuple())])
                return item_id
        return None

    def time_for_item(self, item_id):
        for active_item_id, time in self._item_time_pairs():
            if active_item_id == item_id:
                return datetime.fromtimestamp(time)
        return None
