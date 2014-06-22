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

import datetime
import itertools
import operator

from google.appengine.ext import webapp
from google.appengine.ext.webapp import template

from model.queues import Queue
from model import queuestatus


class ActiveBots(webapp.RequestHandler):
    def get(self):
        # 2000 is the GAE record fetch limit.
        recent_statuses = queuestatus.QueueStatus.all().order("-date").fetch(500)
        queue_name_to_last_status = {}
        for status in recent_statuses:
            last_status = queue_name_to_last_status.get(status.bot_id)
            if not last_status or status.date > last_status.date:
                queue_name_to_last_status[status.bot_id] = status

        sorted_by_bot_id = sorted(queue_name_to_last_status.values(), key=operator.attrgetter('bot_id'))
        # Sorted is stable, so this will be sorted by bot_id, groupped by sorted queue_name.
        sorted_by_queue_name = sorted(sorted_by_bot_id, key=operator.attrgetter('queue_name'))
        template_values = {
            "last_statuses": sorted_by_queue_name,
        }
        self.response.out.write(template.render("templates/activebots.html", template_values))
