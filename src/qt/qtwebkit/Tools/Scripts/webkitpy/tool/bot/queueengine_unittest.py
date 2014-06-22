# Copyright (c) 2009 Google Inc. All rights reserved.
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
import os
import shutil
import tempfile
import threading
import unittest2 as unittest

from webkitpy.common.system.executive import ScriptError
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.bot.queueengine import QueueEngine, QueueEngineDelegate, TerminateQueue


class LoggingDelegate(QueueEngineDelegate):
    def __init__(self, test):
        self._test = test
        self._callbacks = []
        self._run_before = False
        self.stop_message = None

    expected_callbacks = [
        'queue_log_path',
        'begin_work_queue',
        'should_continue_work_queue',
        'next_work_item',
        'work_item_log_path',
        'process_work_item',
        'should_continue_work_queue',
        'stop_work_queue',
    ]

    def record(self, method_name):
        self._callbacks.append(method_name)

    def queue_log_path(self):
        self.record("queue_log_path")
        return os.path.join(self._test.temp_dir, "queue_log_path")

    def work_item_log_path(self, work_item):
        self.record("work_item_log_path")
        return os.path.join(self._test.temp_dir, "work_log_path", "%s.log" % work_item)

    def begin_work_queue(self):
        self.record("begin_work_queue")

    def should_continue_work_queue(self):
        self.record("should_continue_work_queue")
        if not self._run_before:
            self._run_before = True
            return True
        return False

    def next_work_item(self):
        self.record("next_work_item")
        return "work_item"

    def process_work_item(self, work_item):
        self.record("process_work_item")
        self._test.assertEqual(work_item, "work_item")
        return True

    def handle_unexpected_error(self, work_item, message):
        self.record("handle_unexpected_error")
        self._test.assertEqual(work_item, "work_item")

    def stop_work_queue(self, message):
        self.record("stop_work_queue")
        self.stop_message = message


class RaisingDelegate(LoggingDelegate):
    def __init__(self, test, exception):
        LoggingDelegate.__init__(self, test)
        self._exception = exception

    def process_work_item(self, work_item):
        self.record("process_work_item")
        raise self._exception


class FastQueueEngine(QueueEngine):
    def __init__(self, delegate):
        QueueEngine.__init__(self, "fast-queue", delegate, threading.Event())

    # No sleep for the wicked.
    seconds_to_sleep = 0

    def _sleep(self, message):
        pass


class QueueEngineTest(unittest.TestCase):
    def test_trivial(self):
        delegate = LoggingDelegate(self)
        self._run_engine(delegate)
        self.assertEqual(delegate.stop_message, "Delegate terminated queue.")
        self.assertEqual(delegate._callbacks, LoggingDelegate.expected_callbacks)
        self.assertTrue(os.path.exists(os.path.join(self.temp_dir, "queue_log_path")))
        self.assertTrue(os.path.exists(os.path.join(self.temp_dir, "work_log_path", "work_item.log")))

    def test_unexpected_error(self):
        delegate = RaisingDelegate(self, ScriptError(exit_code=3))
        self._run_engine(delegate)
        expected_callbacks = LoggingDelegate.expected_callbacks[:]
        work_item_index = expected_callbacks.index('process_work_item')
        # The unexpected error should be handled right after process_work_item starts
        # but before any other callback.  Otherwise callbacks should be normal.
        expected_callbacks.insert(work_item_index + 1, 'handle_unexpected_error')
        self.assertEqual(delegate._callbacks, expected_callbacks)

    def test_handled_error(self):
        delegate = RaisingDelegate(self, ScriptError(exit_code=QueueEngine.handled_error_code))
        self._run_engine(delegate)
        self.assertEqual(delegate._callbacks, LoggingDelegate.expected_callbacks)

    def _run_engine(self, delegate, engine=None, termination_message=None):
        if not engine:
            engine = QueueEngine("test-queue", delegate, threading.Event())
        if not termination_message:
            termination_message = "Delegate terminated queue."
        expected_logs = "\n%s\n" % termination_message
        OutputCapture().assert_outputs(self, engine.run, expected_logs=expected_logs)

    def _test_terminating_queue(self, exception, termination_message):
        work_item_index = LoggingDelegate.expected_callbacks.index('process_work_item')
        # The terminating error should be handled right after process_work_item.
        # There should be no other callbacks after stop_work_queue.
        expected_callbacks = LoggingDelegate.expected_callbacks[:work_item_index + 1]
        expected_callbacks.append("stop_work_queue")

        delegate = RaisingDelegate(self, exception)
        self._run_engine(delegate, termination_message=termination_message)

        self.assertEqual(delegate._callbacks, expected_callbacks)
        self.assertEqual(delegate.stop_message, termination_message)

    def test_terminating_error(self):
        self._test_terminating_queue(KeyboardInterrupt(), "User terminated queue.")
        self._test_terminating_queue(TerminateQueue(), "TerminateQueue exception received.")

    def test_now(self):
        """Make sure there are no typos in the QueueEngine.now() method."""
        engine = QueueEngine("test", None, None)
        self.assertIsInstance(engine._now(), datetime.datetime)

    def test_sleep_message(self):
        engine = QueueEngine("test", None, None)
        engine._now = lambda: datetime.datetime(2010, 1, 1)
        expected_sleep_message = "MESSAGE Sleeping until 2010-01-01 00:02:00 (120 seconds)."
        self.assertEqual(engine._sleep_message("MESSAGE"), expected_sleep_message)

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp(suffix="work_queue_test_logs")

    def tearDown(self):
        shutil.rmtree(self.temp_dir)
