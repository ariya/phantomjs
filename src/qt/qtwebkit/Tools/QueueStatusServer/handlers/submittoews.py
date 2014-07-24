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

from google.appengine.ext import webapp, db
from google.appengine.ext.webapp import template

from handlers.updatebase import UpdateBase
from loggers.recordpatchevent import RecordPatchEvent
from model.attachment import Attachment
from model.queues import Queue


class SubmitToEWS(UpdateBase):
    def get(self):
        self.response.out.write(template.render("templates/submittoews.html", None))

    def _should_add_to_ews_queue(self, queue, attachment):
        # This assert() is here to make sure we're not submitting to the commit-queue.
        # The commit-queue clients check each patch anyway, but there is not sense
        # in adding things to the commit-queue when they won't be processed by it.
        assert(queue.is_ews())
        latest_status = attachment.status_for_queue(queue)
        if not latest_status:
            return True
        # Only ever re-submit to the EWS if the EWS specifically requested a retry.
        # This allows us to restart the EWS feeder queue, without all r? patches
        # being retried as a result of that restart!
        # In some future version we might add a "force" button to allow the user
        # to override this restriction.
        return latest_status.is_retry_request()

    def _add_attachment_to_ews_queues(self, attachment):
        for queue in Queue.all_ews():  # all_ews() currently includes the style-queue
            if self._should_add_to_ews_queue(queue, attachment):
                queue.work_items().add_work_item(attachment.id)
                RecordPatchEvent.added(attachment.id, queue.name())

    def post(self):
        attachment_id = self._int_from_request("attachment_id")
        attachment = Attachment(attachment_id)
        self._add_attachment_to_ews_queues(attachment)
        if self.request.get("next_action") == "return_to_bubbles":
            self.redirect("/status-bubble/%s" % attachment_id)
