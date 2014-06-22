# Copyright (C) 2010 Google Inc. All rights reserved.
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

import unittest2 as unittest

from webkitpy.common.thread.messagepump import MessagePump, MessagePumpDelegate
from webkitpy.common.thread.threadedmessagequeue import ThreadedMessageQueue


class TestDelegate(MessagePumpDelegate):
    def __init__(self):
        self.log = []

    def schedule(self, interval, callback):
        self.callback = callback
        self.log.append("schedule")

    def message_available(self, message):
        self.log.append("message_available: %s" % message)

    def final_message_delivered(self):
        self.log.append("final_message_delivered")


class MessagePumpTest(unittest.TestCase):

    def test_basic(self):
        queue = ThreadedMessageQueue()
        delegate = TestDelegate()
        pump = MessagePump(delegate, queue)
        self.assertEqual(delegate.log, [
            'schedule'
        ])
        delegate.callback()
        queue.post("Hello")
        queue.post("There")
        delegate.callback()
        self.assertEqual(delegate.log, [
            'schedule',
            'schedule',
            'message_available: Hello',
            'message_available: There',
            'schedule'
        ])
        queue.post("More")
        queue.post("Messages")
        queue.stop()
        delegate.callback()
        self.assertEqual(delegate.log, [
            'schedule',
            'schedule',
            'message_available: Hello',
            'message_available: There',
            'schedule',
            'message_available: More',
            'message_available: Messages',
            'final_message_delivered'
        ])
