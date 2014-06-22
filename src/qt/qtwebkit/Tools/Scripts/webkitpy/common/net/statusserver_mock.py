# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

import logging

_log = logging.getLogger(__name__)


class MockStatusServer(object):

    def __init__(self, bot_id=None, work_items=None):
        self.host = "example.com"
        self.bot_id = bot_id
        self._work_items = work_items or []

    def patch_status(self, queue_name, patch_id):
        return None

    def svn_revision(self, svn_revision):
        return None

    def next_work_item(self, queue_name):
        if not self._work_items:
            return None
        return self._work_items.pop(0)

    def release_work_item(self, queue_name, patch):
        _log.info("MOCK: release_work_item: %s %s" % (queue_name, patch.id()))

    def update_work_items(self, queue_name, work_items):
        self._work_items = work_items
        _log.info("MOCK: update_work_items: %s %s" % (queue_name, work_items))

    def submit_to_ews(self, patch_id):
        _log.info("MOCK: submit_to_ews: %s" % (patch_id))

    def update_status(self, queue_name, status, patch=None, results_file=None):
        _log.info("MOCK: update_status: %s %s" % (queue_name, status))
        return 187

    def update_svn_revision(self, svn_revision, broken_bot):
        return 191

    def results_url_for_status(self, status_id):
        return "http://dummy_url"
