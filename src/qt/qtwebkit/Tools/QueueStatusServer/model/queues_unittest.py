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


from model.queues import Queue


class QueueTest(unittest.TestCase):
    def test_is_ews(self):
        mac_ews = Queue("mac-ews")
        self.assertTrue(mac_ews.is_ews())

    def test_queue_with_name(self):
        self.assertEqual(Queue.queue_with_name("bogus"), None)
        self.assertEqual(Queue.queue_with_name("mac-ews").name(), "mac-ews")
        self.assertRaises(AssertionError, Queue, ("bogus"))

    def _assert_short_name(self, queue_name, short_name):
        self.assertEqual(Queue(queue_name).short_name(), short_name)

    def test_short_name(self):
        self._assert_short_name("mac-ews", "Mac")
        self._assert_short_name("commit-queue", "Commit")
        self._assert_short_name("style-queue", "Style")

    def _assert_display_name(self, queue_name, short_name):
        self.assertEqual(Queue(queue_name).display_name(), short_name)

    def test_display_name(self):
        self._assert_display_name("mac-ews", "Mac EWS")
        self._assert_display_name("commit-queue", "Commit Queue")
        self._assert_display_name("style-queue", "Style Queue")

    def _assert_name_with_underscores(self, queue_name, short_name):
        self.assertEqual(Queue(queue_name).name_with_underscores(), short_name)

    def test_name_with_underscores(self):
        self._assert_name_with_underscores("mac-ews", "mac_ews")
        self._assert_name_with_underscores("commit-queue", "commit_queue")

    def test_style_queue_is_ews(self):
        # For now we treat the style-queue as an EWS since most users would
        # describe it as such.  If is_ews() ever needs to mean "builds the patch"
        # or similar, then we will need to adjust all callers.
        self.assertTrue(Queue("style-queue").is_ews())
        self.assertTrue("style-queue" in map(Queue.name, Queue.all_ews()))


if __name__ == '__main__':
    unittest.main()
