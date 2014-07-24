# Copyright (c) 2009 Google Inc. All rights reserved.
# Copyright (c) 2009 Apple Inc. All rights reserved.
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

import logging
import sys
import traceback

from datetime import datetime, timedelta

from webkitpy.common.system.executive import ScriptError
from webkitpy.common.system.outputtee import OutputTee

_log = logging.getLogger(__name__)


# FIXME: This will be caught by "except Exception:" blocks, we should consider
# making this inherit from SystemExit instead (or BaseException, except that's not recommended).
class TerminateQueue(Exception):
    pass


class QueueEngineDelegate:
    def queue_log_path(self):
        raise NotImplementedError, "subclasses must implement"

    def work_item_log_path(self, work_item):
        raise NotImplementedError, "subclasses must implement"

    def begin_work_queue(self):
        raise NotImplementedError, "subclasses must implement"

    def should_continue_work_queue(self):
        raise NotImplementedError, "subclasses must implement"

    def next_work_item(self):
        raise NotImplementedError, "subclasses must implement"

    def process_work_item(self, work_item):
        raise NotImplementedError, "subclasses must implement"

    def handle_unexpected_error(self, work_item, message):
        raise NotImplementedError, "subclasses must implement"


class QueueEngine:
    def __init__(self, name, delegate, wakeup_event, seconds_to_sleep=120):
        self._name = name
        self._delegate = delegate
        self._wakeup_event = wakeup_event
        self._output_tee = OutputTee()
        self._seconds_to_sleep = seconds_to_sleep

    log_date_format = "%Y-%m-%d %H:%M:%S"
    handled_error_code = 2

    # Child processes exit with a special code to the parent queue process can detect the error was handled.
    @classmethod
    def exit_after_handled_error(cls, error):
        _log.error(error)
        sys.exit(cls.handled_error_code)

    def run(self):
        self._begin_logging()

        self._delegate.begin_work_queue()
        while (self._delegate.should_continue_work_queue()):
            try:
                self._ensure_work_log_closed()
                work_item = self._delegate.next_work_item()
                if not work_item:
                    self._sleep("No work item.")
                    continue

                # FIXME: Work logs should not depend on bug_id specificaly.
                #        This looks fixed, no?
                self._open_work_log(work_item)
                try:
                    if not self._delegate.process_work_item(work_item):
                        _log.warning("Unable to process work item.")
                        continue
                except ScriptError, e:
                    # Use a special exit code to indicate that the error was already
                    # handled in the child process and we should just keep looping.
                    if e.exit_code == self.handled_error_code:
                        continue
                    message = "Unexpected failure when processing patch!  Please file a bug against webkit-patch.\n%s" % e.message_with_output()
                    self._delegate.handle_unexpected_error(work_item, message)
            except TerminateQueue, e:
                self._stopping("TerminateQueue exception received.")
                return 0
            except KeyboardInterrupt, e:
                self._stopping("User terminated queue.")
                return 1
            except Exception, e:
                traceback.print_exc()
                # Don't try tell the status bot, in case telling it causes an exception.
                self._sleep("Exception while preparing queue")
        self._stopping("Delegate terminated queue.")
        return 0

    def _stopping(self, message):
        _log.info("\n%s" % message)
        self._delegate.stop_work_queue(message)
        # Be careful to shut down our OutputTee or the unit tests will be unhappy.
        self._ensure_work_log_closed()
        self._output_tee.remove_log(self._queue_log)

    def _begin_logging(self):
        self._queue_log = self._output_tee.add_log(self._delegate.queue_log_path())
        self._work_log = None

    def _open_work_log(self, work_item):
        work_item_log_path = self._delegate.work_item_log_path(work_item)
        if not work_item_log_path:
            return
        self._work_log = self._output_tee.add_log(work_item_log_path)

    def _ensure_work_log_closed(self):
        # If we still have a bug log open, close it.
        if self._work_log:
            self._output_tee.remove_log(self._work_log)
            self._work_log = None

    def _now(self):
        """Overriden by the unit tests to allow testing _sleep_message"""
        return datetime.now()

    def _sleep_message(self, message):
        wake_time = self._now() + timedelta(seconds=self._seconds_to_sleep)
        if self._seconds_to_sleep < 3 * 60:
            sleep_duration_text = str(self._seconds_to_sleep) + ' seconds'
        else:
            sleep_duration_text = str(round(self._seconds_to_sleep / 60)) + ' minutes'
        return "%s Sleeping until %s (%s)." % (message, wake_time.strftime(self.log_date_format), sleep_duration_text)

    def _sleep(self, message):
        _log.info(self._sleep_message(message))
        self._wakeup_event.wait(self._seconds_to_sleep)
        self._wakeup_event.clear()
