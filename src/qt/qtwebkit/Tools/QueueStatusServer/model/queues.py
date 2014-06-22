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


import re

from config.queues import all_queue_names
from model.activeworkitems import ActiveWorkItems
from model.workitems import WorkItems


class Queue(object):
    def __init__(self, name):
        assert(name in all_queue_names)
        self._name = name

    @classmethod
    def queue_with_name(cls, queue_name):
        if queue_name not in all_queue_names:
            return None
        return Queue(queue_name)

    @classmethod
    def all(cls):
        return [Queue(name) for name in all_queue_names]

    @classmethod
    def all_ews(cls):
        return [queue for queue in cls.all() if queue.is_ews()]

    def name(self):
        return self._name

    def work_items(self):
        return WorkItems.lookup_by_queue(self._name)

    # FIXME: active_work_items is a bad name for this lock-table.
    def active_work_items(self):
        return ActiveWorkItems.lookup_by_queue(self._name)

    def _caplitalize_after_dash(self, string):
        return "-".join([word[0].upper() + word[1:] for word in string.split("-")])

    # For use in status bubbles or table headers
    def short_name(self):
        short_name = self._name.replace("-ews", "")
        short_name = short_name.replace("-queue", "")
        return self._caplitalize_after_dash(short_name.capitalize())

    def display_name(self):
        display_name = self._name.replace("-", " ")
        display_name = display_name.title()
        display_name = display_name.replace("Wk2", "WK2")
        display_name = display_name.replace("Ews", "EWS")
        return display_name

    _dash_regexp = re.compile("-")

    def name_with_underscores(self):
        return self._dash_regexp.sub("_", self._name)

    def is_ews(self):
        # Note: The style-queue is just like an EWS in that it has an EWS
        # bubble, and it works off of the r? patches.  If at some later
        # point code wants to not treat the style-queue as an EWS
        # (e.g. expecting is_ews() queues to have build results?)
        # then we should fix all callers and change this check.
        return self._name.endswith("-ews") or self._name == "style-queue"
