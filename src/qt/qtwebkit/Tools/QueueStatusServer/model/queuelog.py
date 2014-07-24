# Copyright (C) 2013 Google Inc. All rights reserved.
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

from time import time
from datetime import datetime

from google.appengine.ext import db

from model.workitems import WorkItems
from model.activeworkitems import ActiveWorkItems


class QueueLog(db.Model):
    date = db.DateTimeProperty()
    # duration specifies in seconds the time period these log values apply to.
    duration = db.IntegerProperty()
    queue_name = db.StringProperty()
    bot_ids_seen = db.StringListProperty()
    max_patches_waiting = db.IntegerProperty(default=0)
    patch_wait_durations = db.ListProperty(int)
    patch_process_durations = db.ListProperty(int)
    patch_retry_count = db.IntegerProperty(default=0)
    status_update_count = db.IntegerProperty(default=0)

    @staticmethod
    def create_key(queue_name, duration, timestamp):
        return "%s-%s-%s" % (queue_name, duration, timestamp)

    @classmethod
    def get_at(cls, queue_name, duration, timestamp):
        timestamp = int(timestamp / duration) * duration
        date = datetime.utcfromtimestamp(timestamp)
        key = cls.create_key(queue_name, duration, timestamp)
        return cls.get_or_create(key, date=date, duration=duration, queue_name=queue_name)

    @classmethod
    def get_current(cls, queue_name, duration):
        return cls.get_at(queue_name, duration, time())

    # This is to prevent page requests from generating lots of rows in the database.
    @classmethod
    def get_or_create(cls, key_name, **kwargs):
        return db.run_in_transaction(cls._get_or_create_txn, key_name, **kwargs)

    def update_max_patches_waiting(self):
        patches_waiting = self._get_patches_waiting(self.queue_name)
        if patches_waiting > self.max_patches_waiting:
            self.max_patches_waiting = patches_waiting
            return True
        return False

    @classmethod
    def _get_or_create_txn(cls, key_name, **kwargs):
        entity = cls.get_by_key_name(key_name, parent=kwargs.get('parent'))
        if entity is None:
            entity = cls(key_name=key_name, **kwargs)
        return entity

    @classmethod
    def _get_patches_waiting(cls, queue_name):
        work_items = WorkItems.lookup_by_queue(queue_name)
        active_work_items = ActiveWorkItems.lookup_by_queue(queue_name)
        return len(set(work_items.item_ids) - set(active_work_items.item_ids))
