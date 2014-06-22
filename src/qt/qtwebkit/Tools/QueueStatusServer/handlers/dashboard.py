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
from model.queues import Queue


class Dashboard(webapp.RequestHandler):
    # We may want to sort these?
    _ordered_queues = Queue.all()
    _header_names = [queue.short_name() for queue in _ordered_queues]

    def _build_bubble(self, attachment, queue):
        queue_status = attachment.status_for_queue(queue)
        bubble = {
            "status_class": attachment.state_from_queue_status(queue_status) if queue_status else "none",
            "status_date": queue_status.date if queue_status else None,
        }
        return bubble

    def _build_row(self, attachment):
        row = {
            "bug_id": attachment.bug_id(),
            "attachment_id": attachment.id,
            "bubbles": [self._build_bubble(attachment, queue) for queue in self._ordered_queues],
        }
        return row

    def get(self):
        template_values = {
            "headers": self._header_names,
            "rows": [self._build_row(attachment) for attachment in Attachment.recent(limit=25)],
        }
        self.response.out.write(template.render("templates/dashboard.html", template_values))
