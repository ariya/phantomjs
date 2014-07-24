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


class PatchLog(db.Model):
    attachment_id = db.IntegerProperty()
    queue_name = db.StringProperty()
    date = db.DateTimeProperty(auto_now_add=True)
    bot_id = db.StringProperty()
    retry_count = db.IntegerProperty(default=0)
    status_update_count = db.IntegerProperty(default=0)
    finished = db.BooleanProperty(default=False)
    wait_duration = db.IntegerProperty()
    process_duration = db.IntegerProperty()

    @classmethod
    def lookup(cls, attachment_id, queue_name):
        key = cls._generate_key(attachment_id, queue_name)
        return cls.get_or_insert(key, attachment_id=attachment_id, queue_name=queue_name)

    @classmethod
    def lookup_if_exists(cls, attachment_id, queue_name):
        key = cls._generate_key(attachment_id, queue_name)
        return cls.get_by_key_name(key)

    def calculate_wait_duration(self):
        time_delta = datetime.utcnow() - self.date
        self.wait_duration = int(self._time_delta_to_seconds(time_delta))

    def calculate_process_duration(self):
        time_delta = datetime.utcnow() - self.date
        self.process_duration = int(self._time_delta_to_seconds(time_delta)) - (self.wait_duration or 0)

    @classmethod
    def _generate_key(cls, attachment_id, queue_name):
        return "%s-%s" % (attachment_id, queue_name)

    # Needed to support Python 2.5's lack of timedelta.total_seconds().
    @classmethod
    def _time_delta_to_seconds(cls, time_delta):
        return time_delta.seconds + time_delta.days * 24 * 3600
