# Copyright (c) 2011 Google Inc. All rights reserved.
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


class ExpectedFailures(object):
    def __init__(self):
        self._failures = set()
        self._is_trustworthy = True

    @classmethod
    def _has_failures(cls, results):
        return bool(results and results.failing_tests())

    @classmethod
    def _should_trust(cls, results):
        return bool(cls._has_failures(results) and results.failure_limit_count() and len(results.failing_tests()) < results.failure_limit_count())

    def failures_were_expected(self, results):
        if not self._is_trustworthy:
            return False
        if not self._should_trust(results):
            return False
        return set(results.failing_tests()) <= self._failures

    def unexpected_failures_observed(self, results):
        if not self._is_trustworthy:
            return None
        if not self._has_failures(results):
            return None
        return set(results.failing_tests()) - self._failures

    def update(self, results):
        if results:
            self._failures = set(results.failing_tests())
            self._is_trustworthy = self._should_trust(results)
