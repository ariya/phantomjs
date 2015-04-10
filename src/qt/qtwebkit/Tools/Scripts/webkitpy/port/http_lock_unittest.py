# Copyright (C) 2010 Gabor Rapcsanyi (rgabor@inf.u-szeged.hu), University of Szeged
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from http_lock import HttpLock
import os  # Used for os.getpid()
import unittest2 as unittest

from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.executive_mock import MockExecutive


# FIXME: These tests all touch the real disk, but could be written to a MockFileSystem instead.
class HttpLockTestWithRealFileSystem(unittest.TestCase):
    # FIXME: Unit tests do not use an __init__ method, but rather setUp and tearDown methods.
    def __init__(self, testFunc):
        self.http_lock = HttpLock(None, "WebKitTestHttpd.lock.", "WebKitTest.lock")
        self.filesystem = self.http_lock._filesystem  # FIXME: We should be passing in a MockFileSystem instead.
        self.lock_file_path_prefix = self.filesystem.join(self.http_lock._lock_path, self.http_lock._lock_file_prefix)
        self.lock_file_name = self.lock_file_path_prefix + "0"
        self.guard_lock_file = self.http_lock._guard_lock_file
        self.clean_all_lockfile()
        unittest.TestCase.__init__(self, testFunc)

    def clean_all_lockfile(self):
        if self.filesystem.exists(self.guard_lock_file):
            self.filesystem.remove(self.guard_lock_file)
        lock_list = self.filesystem.glob(self.lock_file_path_prefix + '*')
        for file_name in lock_list:
            self.filesystem.remove(file_name)

    def assertEqual(self, first, second):
        if first != second:
            self.clean_all_lockfile()
        unittest.TestCase.assertEqual(self, first, second)

    def _check_lock_file(self):
        if self.filesystem.exists(self.lock_file_name):
            pid = os.getpid()
            lock_file_pid = self.filesystem.read_text_file(self.lock_file_name)
            self.assertEqual(pid, int(lock_file_pid))
            return True
        return False

    def test_lock_lifecycle(self):
        self.http_lock._create_lock_file()

        self.assertEqual(True, self._check_lock_file())
        self.assertEqual(1, self.http_lock._next_lock_number())

        self.http_lock.cleanup_http_lock()

        self.assertEqual(False, self._check_lock_file())
        self.assertEqual(0, self.http_lock._next_lock_number())


class HttpLockTest(unittest.TestCase):
    def setUp(self):
        self.filesystem = MockFileSystem()
        self.http_lock = HttpLock(None, "WebKitTestHttpd.lock.", "WebKitTest.lock", filesystem=self.filesystem, executive=MockExecutive())
        # FIXME: Shouldn't we be able to get these values from the http_lock object directly?
        self.lock_file_path_prefix = self.filesystem.join(self.http_lock._lock_path, self.http_lock._lock_file_prefix)
        self.lock_file_name = self.lock_file_path_prefix + "0"

    def test_current_lock_pid(self):
        # FIXME: Once Executive wraps getpid, we can mock this and not use a real pid.
        current_pid = os.getpid()
        self.http_lock._filesystem.write_text_file(self.lock_file_name, str(current_pid))
        self.assertEqual(self.http_lock._current_lock_pid(), current_pid)

    def test_extract_lock_number(self):
        lock_file_list = (
            self.lock_file_path_prefix + "00",
            self.lock_file_path_prefix + "9",
            self.lock_file_path_prefix + "001",
            self.lock_file_path_prefix + "021",
        )

        expected_number_list = (0, 9, 1, 21)

        for lock_file, expected in zip(lock_file_list, expected_number_list):
            self.assertEqual(self.http_lock._extract_lock_number(lock_file), expected)

    def test_lock_file_list(self):
        self.http_lock._filesystem = MockFileSystem({
            self.lock_file_path_prefix + "6": "",
            self.lock_file_path_prefix + "1": "",
            self.lock_file_path_prefix + "4": "",
            self.lock_file_path_prefix + "3": "",
        })

        expected_file_list = [
            self.lock_file_path_prefix + "1",
            self.lock_file_path_prefix + "3",
            self.lock_file_path_prefix + "4",
            self.lock_file_path_prefix + "6",
        ]

        self.assertEqual(self.http_lock._lock_file_list(), expected_file_list)
