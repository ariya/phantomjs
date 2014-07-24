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

"""This class helps to lock files exclusively across processes."""

import logging
import os
import sys
import time


_log = logging.getLogger(__name__)


class FileLock(object):

    def __init__(self, lock_file_path, max_wait_time_sec=20):
        self._lock_file_path = lock_file_path
        self._lock_file_descriptor = None
        self._max_wait_time_sec = max_wait_time_sec

    def _create_lock(self):
        if sys.platform == 'win32':
            import msvcrt
            msvcrt.locking(self._lock_file_descriptor, msvcrt.LK_NBLCK, 32)
        else:
            import fcntl
            fcntl.flock(self._lock_file_descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)

    def _remove_lock(self):
        if sys.platform == 'win32':
            import msvcrt
            msvcrt.locking(self._lock_file_descriptor, msvcrt.LK_UNLCK, 32)
        else:
            import fcntl
            fcntl.flock(self._lock_file_descriptor, fcntl.LOCK_UN)

    def acquire_lock(self):
        self._lock_file_descriptor = os.open(self._lock_file_path, os.O_TRUNC | os.O_CREAT)
        start_time = time.time()
        while True:
            try:
                self._create_lock()
                return True
            except IOError:
                if time.time() - start_time > self._max_wait_time_sec:
                    _log.debug("File locking failed: %s" % str(sys.exc_info()))
                    os.close(self._lock_file_descriptor)
                    self._lock_file_descriptor = None
                    return False
                # There's no compelling reason to spin hard here, so sleep for a bit.
                time.sleep(0.01)

    def release_lock(self):
        try:
            if self._lock_file_descriptor:
                self._remove_lock()
                os.close(self._lock_file_descriptor)
                self._lock_file_descriptor = None
            os.unlink(self._lock_file_path)
        except (IOError, OSError):
            _log.debug("Warning in release lock: %s" % str(sys.exc_info()))
