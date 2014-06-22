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
from model.attachment import Attachment
from model.queues import Queue


class ReleasePatch(UpdateBase):
    def get(self):
        self.response.out.write(template.render("templates/releasepatch.html", None))

    def post(self):
        queue_name = self.request.get("queue_name")
        # FIXME: This queue lookup should be shared between handlers.
        queue = Queue.queue_with_name(queue_name)
        if not queue:
            self.error(404)
            return

        attachment_id = self._int_from_request("attachment_id")
        attachment = Attachment(attachment_id)
        last_status = attachment.status_for_queue(queue)

        # Ideally we should use a transaction for the calls to
        # WorkItems and ActiveWorkItems.

        # Only remove it from the queue if the last message is not a retry request.
        # Allow removing it from the queue even if there is no last_status for easier testing.
        if not last_status or not last_status.is_retry_request():
            queue.work_items().remove_work_item(attachment_id)
            RecordPatchEvent.stopped(attachment_id, queue_name)
        else:
            RecordPatchEvent.retrying(attachment_id, queue_name)

        # Always release the lock on the item.
        queue.active_work_items().expire_item(attachment_id)
