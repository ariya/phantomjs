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

from datetime import datetime

from google.appengine.ext import db
from google.appengine.ext import webapp

from loggers.recordpatchevent import RecordPatchEvent
from model.queues import Queue


class NextPatch(webapp.RequestHandler):
    # FIXME: This should probably be a post, or an explict lock_patch
    # since GET requests shouldn't really modify the datastore.
    def get(self, queue_name):
        queue = Queue.queue_with_name(queue_name)
        if not queue:
            self.error(404)
            return
        # FIXME: Patch assignment should probably move into Queue.
        patch_id = db.run_in_transaction(self._assign_patch, queue.active_work_items().key(), queue.work_items().item_ids)
        if not patch_id:
            self.error(404)
            return
        RecordPatchEvent.started(patch_id, queue_name)
        self.response.out.write(patch_id)

    @staticmethod
    def _assign_patch(key, work_item_ids):
        now = datetime.utcnow()
        active_work_items = db.get(key)
        active_work_items.deactivate_expired(now)
        next_item = active_work_items.next_item(work_item_ids, now)
        active_work_items.put()
        return next_item
