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

import os
import tempfile
import unittest2 as unittest

from webkitpy.common.system.file_lock import FileLock


class FileLockTest(unittest.TestCase):

    def setUp(self):
        self._lock_name = "TestWebKit" + str(os.getpid()) + ".lock"
        self._lock_path = os.path.join(tempfile.gettempdir(), self._lock_name)
        self._file_lock1 = FileLock(self._lock_path, 0.1)
        self._file_lock2 = FileLock(self._lock_path, 0.1)

    def tearDown(self):
        self._file_lock1.release_lock()
        self._file_lock2.release_lock()

    def test_lock_lifecycle(self):
        # Create the lock.
        self._file_lock1.acquire_lock()
        self.assertTrue(os.path.exists(self._lock_path))

        # Try to lock again.
        self.assertFalse(self._file_lock2.acquire_lock())

        # Release the lock.
        self._file_lock1.release_lock()
        self.assertFalse(os.path.exists(self._lock_path))

    def test_stuck_lock(self):
        open(self._lock_path, 'w').close()
        self._file_lock1.acquire_lock()
        self._file_lock1.release_lock()
