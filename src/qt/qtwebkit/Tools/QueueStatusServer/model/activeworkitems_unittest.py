# Copyright (C) 2010 Google, Inc. All rights reserved.
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
#    * Neither the name of Research in Motion Ltd. nor the names of its
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

import unittest
from datetime import datetime
from google.appengine.ext import testbed

from model.activeworkitems import ActiveWorkItems


class ActiveWorkItemsTest(unittest.TestCase):

    def setUp(self):
        self.testbed = testbed.Testbed()
        self.testbed.activate()
        self.testbed.init_datastore_v3_stub()
        self.testbed.init_memcache_stub()

    def tearDown(self):
        self.testbed.deactivate()

    def test_basic(self):
        items = ActiveWorkItems.lookup_by_queue("test-queue")
        queued_items = [1, 2]
        # db.Model only stores dates to second resolution, so we use an explicit datetime without milliseconds.
        time = datetime(2011, 4, 18, 18, 50, 44)
        self.assertEqual(items.next_item(queued_items, time), 1)
        self.assertEqual(items.next_item([1], time), None)
        self.assertEqual(items.next_item([], time), None)

        self.assertEqual(items.time_for_item(1), time)
        self.assertEqual(items.time_for_item(2), None)

        items.expire_item(1)
        # expire_item uses a transaction so it doesn't take effect on the current object.
        self.assertEqual(items.time_for_item(1), time)
        # If we look up the saved object, we see it's been updated.
        items = ActiveWorkItems.lookup_by_queue("test-queue")
        self.assertEqual(items.time_for_item(1), None)
