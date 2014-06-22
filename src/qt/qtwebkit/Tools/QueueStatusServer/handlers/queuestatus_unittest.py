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

from handlers.queuestatus import QueueStatus
from model.queues import Queue


class MockStatus(object):
    def __init__(self, patch_id, bot_id):
        self.active_patch_id = patch_id
        self.bot_id = bot_id


class QueueStatusTest(unittest.TestCase):
    def test_build_status_groups(self):
        queue_status = QueueStatus()
        statuses = [
            MockStatus(1, "foo"),
            MockStatus(1, "foo"),
            MockStatus(2, "foo"),
            MockStatus(1, "foo"),
            MockStatus(1, "bar"),
            MockStatus(1, "foo"),
        ]
        groups = queue_status._build_status_groups(statuses)
        self.assertEqual(len(groups), 5)
        self.assertEqual(groups[0], statuses[0:2])
        self.assertEqual(groups[1], statuses[2:3])
        self.assertEqual(groups[2], statuses[3:4])
        self.assertEqual(groups[3], statuses[4:5])
        self.assertEqual(groups[4], statuses[5:6])


if __name__ == '__main__':
    unittest.main()
