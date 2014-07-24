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

from google.appengine.ext import webapp
from google.appengine.ext.webapp import template

from model.queues import Queue
from model import queuestatus


class QueueStatus(webapp.RequestHandler):
    def _rows_for_work_items(self, queue):
        queued_items = queue.work_items()
        active_items = queue.active_work_items()
        if not queued_items:
            return []
        rows = []
        for item_id in queued_items.item_ids:
            rows.append({
                "attachment_id": item_id,
                "bug_id": 1,
                "lock_time": active_items and active_items.time_for_item(item_id),
            })
        return rows

    def _grouping_key_for_status(self, status):
        return "%s-%s" % (status.active_patch_id, status.bot_id)

    def _build_status_groups(self, statuses):
        return [list(group) for key, group in itertools.groupby(statuses, self._grouping_key_for_status)]

    def _fetch_statuses(self, queue, bot_id):
        statuses = queuestatus.QueueStatus.all()
        statuses.filter("queue_name =", queue.name())
        if bot_id:
            statuses.filter("bot_id =", bot_id)
        return statuses.order("-date").fetch(15)

    def _fetch_last_message_matching(self, queue, bot_id, message):
        statuses = queuestatus.QueueStatus.all()
        statuses.filter("queue_name =", queue.name())
        if bot_id:
            statuses.filter("bot_id =", bot_id)
        statuses.filter("message =", message)
        return statuses.order("-date").get()

    def _fetch_trailing_days_pass_count(self, queue, bot_id, days):
        statuses = queuestatus.QueueStatus.all()
        statuses.filter("queue_name =", queue.name())
        days_ago = datetime.datetime.now() - datetime.timedelta(days=days)
        statuses.filter("date >", days_ago)
        if bot_id:
            statuses.filter("bot_id =", bot_id)
        statuses.filter("message =", "Pass")
        return statuses.count()

    def _fetch_trailing_days_pass_count_string(self, queue, bot_id, days):
        status_count = self._fetch_trailing_days_pass_count(queue, bot_id, days)
        # GQL has a result limit of 1000, so we return a special string to indicate we hit that limit.
        if status_count == 1000:
            status_count = "1000+"
        return str(status_count)

    def _page_title(self, queue, bot_id):
        title = "%s Messages" % queue.display_name()
        if bot_id:
            title += " from \"%s\"" % (bot_id)
        return title

    def get(self, queue_name, bot_id=None):
        queue_name = queue_name.lower()
        queue = Queue.queue_with_name(queue_name)
        if not queue:
            self.error(404)
            return

        statuses = self._fetch_statuses(queue, bot_id)
        template_values = {
            "queue_name": queue_name,
            "page_title": self._page_title(queue, bot_id),
            "work_item_rows": self._rows_for_work_items(queue),
            "status_groups": self._build_status_groups(statuses),
            "bot_id": bot_id,
            "last_pass": self._fetch_last_message_matching(queue, bot_id, "Pass"),
            "last_boot": self._fetch_last_message_matching(queue, bot_id, "Starting Queue"),
            "trailing_month_pass_count": self._fetch_trailing_days_pass_count_string(queue, bot_id, 30),
            "trailing_week_pass_count": self._fetch_trailing_days_pass_count_string(queue, bot_id, 7),
        }
        self.response.out.write(template.render("templates/queuestatus.html", template_values))
