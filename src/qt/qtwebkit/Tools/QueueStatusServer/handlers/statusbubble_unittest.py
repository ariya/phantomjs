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


from handlers.statusbubble import StatusBubble
from model.queues import Queue


class MockAttachment(object):
    def __init__(self):
        self.id = 1

    def status_for_queue(self, queue):
        return None

    def position_in_queue(self, queue):
        return 1


class StatusBubbleTest(unittest.TestCase):
    def test_build_bubble(self):
        bubble = StatusBubble()
        queue = Queue("mac-ews")
        attachment = MockAttachment()
        bubble_dict = bubble._build_bubble(queue, attachment, 1)
        # FIXME: assertDictEqual (in Python 2.7) would be better to use here.
        self.assertEqual(bubble_dict["name"], "mac")
        self.assertEqual(bubble_dict["attachment_id"], 1)
        self.assertEqual(bubble_dict["queue_position"], 1)
        self.assertEqual(bubble_dict["state"], "none")
        self.assertEqual(bubble_dict["status"], None)


if __name__ == '__main__':
    unittest.main()
