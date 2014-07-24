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

import calendar
from datetime import datetime
import itertools
from time import time

from google.appengine.ext import webapp
from google.appengine.ext.webapp import template

from config import logging, charts
from model.patchlog import PatchLog
from model.queues import Queue
from model.queuelog import QueueLog


class QueueCharts(webapp.RequestHandler):
    def get(self, queue_name):
        queue_name = queue_name.lower()
        if not Queue.queue_with_name(queue_name):
            self.error(404)
            return

        timestamp = self._get_timestamp()
        view_range = self._get_view_range()
        time_unit, time_unit_name = charts.get_time_unit(view_range)

        all_queue_names = map(Queue.name, Queue.all())

        template_values = {
            "all_queue_names": all_queue_names,
            "patch_data": self._get_patch_data(queue_name, timestamp, view_range),
            "queue_data": self._get_queue_data(queue_name, timestamp, view_range),
            "queue_name": queue_name,
            "seconds_ago_min": 0,
            "seconds_ago_max": view_range,
            "time_unit_name": time_unit_name,
            "time_unit": time_unit,
            "timestamp": timestamp,
            "view_range": view_range,
            "view_range_choices": charts.view_range_choices,
        }
        self.response.out.write(template.render("templates/queuecharts.html", template_values))

    @classmethod
    def _get_min_med_max(cls, values, defaults=(0, 0, 0)):
        if not values:
            return defaults
        length = len(values)
        sorted_values = sorted(values)
        return sorted_values[0], sorted_values[length / 2], sorted_values[length - 1]

    def _get_patch_data(self, queue_name, timestamp, view_range):
        patch_logs = self._get_patch_logs(queue_name, timestamp, view_range)
        patch_data = []
        for patch_log in patch_logs:
            if patch_log.process_duration and patch_log.wait_duration:
                patch_log_timestamp = calendar.timegm(patch_log.date.utctimetuple())
                patch_data.append({
                    "attachment_id": patch_log.attachment_id,
                    "seconds_ago": timestamp - patch_log_timestamp,
                    "process_duration": patch_log.process_duration / charts.one_minute,
                    "retry_count": patch_log.retry_count,
                    "status_update_count": patch_log.status_update_count,
                    "wait_duration": patch_log.wait_duration / charts.one_minute,
               })
        return patch_data

    def _get_patch_logs(self, queue_name, timestamp, view_range):
        patch_log_query = PatchLog.all()
        patch_log_query = patch_log_query.filter("queue_name =", queue_name)
        patch_log_query = patch_log_query.filter("date >=", datetime.utcfromtimestamp(timestamp - view_range))
        patch_log_query = patch_log_query.filter("date <=", datetime.utcfromtimestamp(timestamp))
        patch_log_query = patch_log_query.order("date")
        return patch_log_query.run(limit=charts.patch_log_limit)

    def _get_queue_data(self, queue_name, timestamp, view_range):
        queue_logs = self._get_queue_logs(queue_name, timestamp, view_range)
        queue_data = []
        for queue_log in queue_logs:
            queue_log_timestamp = calendar.timegm(queue_log.date.utctimetuple())
            p_min, p_med, p_max = self._get_min_med_max(queue_log.patch_process_durations)
            w_min, w_med, w_max = self._get_min_med_max(queue_log.patch_wait_durations)
            queue_data.append({
                "bots_seen": len(queue_log.bot_ids_seen),
                "seconds_ago": timestamp - queue_log_timestamp,
                "patch_processing_min": p_min,
                "patch_processing_med": p_med,
                "patch_processing_max": p_max,
                "patch_retry_count": queue_log.patch_retry_count,
                "patch_waiting_min": w_min,
                "patch_waiting_med": w_med,
                "patch_waiting_max": w_max,
                "patches_completed": len(queue_log.patch_process_durations),
                "patches_waiting": queue_log.max_patches_waiting,
                "status_update_count": queue_log.status_update_count,
            })
        return queue_data

    def _get_queue_logs(self, queue_name, timestamp, view_range):
        queue_logs = []
        current_timestamp = timestamp - view_range
        while current_timestamp <= timestamp:
            queue_logs.append(QueueLog.get_at(queue_name, logging.queue_log_duration, current_timestamp))
            current_timestamp += logging.queue_log_duration
        return queue_logs

    @classmethod
    def _get_time_unit(cls, view_range):
        if view_range > charts.one_day * 2:
            return 

    def _get_timestamp(self):
        timestamp = self.request.get("timestamp")
        try:
            return int(timestamp)
        except ValueError:
            return int(time())

    def _get_view_range(self):
        view_range = self.request.get("view_range")
        try:
            return int(view_range)
        except ValueError:
            return charts.default_view_range
