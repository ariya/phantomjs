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

from google.appengine.ext import webapp, db
from google.appengine.ext.webapp import template

from handlers.updatebase import UpdateBase
from loggers.recordpatchevent import RecordPatchEvent
from model.queues import Queue
from model.workitems import WorkItems

from datetime import datetime


class UpdateWorkItems(UpdateBase):
    def get(self):
        self.response.out.write(template.render("templates/updateworkitems.html", None))

    def _parse_work_items_string(self, items_string):
        try:
            item_strings = items_string.split(" ") if items_string else []
            return map(int, item_strings)
        except ValueError:
            return None

    def _update_work_items_from_request(self, work_items):
        items_string = self.request.get("work_items")
        new_work_items = self._parse_work_items_string(items_string)
        if new_work_items == None:
            self.response.out.write("Failed to parse work items: %s" % items_string)
            return False
        work_items.item_ids = new_work_items
        work_items.date = datetime.utcnow()
        return True

    def _queue_from_request(self):
        queue_name = self.request.get("queue_name")
        queue = Queue.queue_with_name(queue_name)
        if not queue:
            self.response.out.write("\"%s\" is not in queues %s" % (queue_name, Queue.all()))
            return None
        return queue

    def post(self):
        queue = self._queue_from_request()
        if not queue:
            self.response.set_status(500)
            return
        work_items = queue.work_items()
        old_items = set(work_items.item_ids)

        success = self._update_work_items_from_request(work_items)
        if not success:
            self.response.set_status(500)
            return
        new_items = set(work_items.item_ids)
        work_items.put()

        for work_item in new_items - old_items:
            RecordPatchEvent.added(work_item, queue.name())
        for work_item in old_items - new_items:
            RecordPatchEvent.stopped(work_item, queue.name())
