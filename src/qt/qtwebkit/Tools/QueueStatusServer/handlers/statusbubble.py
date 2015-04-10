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

import operator

from google.appengine.ext import webapp
from google.appengine.ext.webapp import template

from model.attachment import Attachment
from model.workitems import WorkItems
from model.queues import Queue


class StatusBubble(webapp.RequestHandler):
    def _build_bubble(self, queue, attachment, queue_position):
        queue_status = attachment.status_for_queue(queue)
        bubble = {
            "name": queue.short_name().lower(),
            "attachment_id": attachment.id,
            "queue_position": queue_position,
            "state": attachment.state_from_queue_status(queue_status) if queue_status else "none",
            "status": queue_status,
        }
        return bubble

    def _have_status_for(self, attachment, queue):
        # Any pending queue is shown.
        if attachment.position_in_queue(queue):
            return True
        # Complete ewses are also shown.
        return bool(queue.is_ews() and attachment.status_for_queue(queue))

    def _build_bubbles_for_attachment(self, attachment):
        show_submit_to_ews = True
        bubbles = []
        for queue in Queue.all():
            if not self._have_status_for(attachment, queue):
                continue
            queue_position = attachment.position_in_queue(queue)
            if queue_position and queue_position >= 100:
                # This queue is so far behind it's not even worth showing.
                continue
            bubbles.append(self._build_bubble(queue, attachment, queue_position))
            # If even one ews has status, we don't show the submit-to-ews button.
            if queue.is_ews():
                show_submit_to_ews = False

        return (bubbles, show_submit_to_ews)

    def get(self, attachment_id_string):
        attachment_id = int(attachment_id_string)
        attachment = Attachment(attachment_id)
        bubbles, show_submit_to_ews = self._build_bubbles_for_attachment(attachment)

        template_values = {
            "bubbles": bubbles,
            "attachment_id": attachment_id,
            "show_submit_to_ews": show_submit_to_ews,
        }
        self.response.out.write(template.render("templates/statusbubble.html", template_values))
