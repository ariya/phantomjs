# Copyright (C) 2010 Google Inc. All rights reserved.
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


# FIXME: This probably belongs in the buildbot module.
class FailureMap(object):
    def __init__(self):
        self._failures = []

    def add_regression_window(self, builder, regression_window):
        self._failures.append({
            'builder': builder,
            'regression_window': regression_window,
        })

    def is_empty(self):
        return not self._failures

    def failing_revisions(self):
        failing_revisions = [failure_info['regression_window'].revisions()
                             for failure_info in self._failures]
        return sorted(set(sum(failing_revisions, [])))

    def builders_failing_for(self, revision):
        return self._builders_failing_because_of([revision])

    def tests_failing_for(self, revision):
        tests = [failure_info['regression_window'].failing_tests()
                 for failure_info in self._failures
                 if revision in failure_info['regression_window'].revisions()
                    and failure_info['regression_window'].failing_tests()]
        result = set()
        for test in tests:
            result = result.union(test)
        return sorted(result)

    def failing_tests(self):
        return set(sum([self.tests_failing_for(revision) for revision in self.failing_revisions()], []))

    def _old_failures(self, is_old_failure):
        return filter(lambda revision: is_old_failure(revision),
                      self.failing_revisions())

    def _builders_failing_because_of(self, revisions):
        revision_set = set(revisions)
        return [failure_info['builder'] for failure_info in self._failures
                if revision_set.intersection(
                    failure_info['regression_window'].revisions())]

    # FIXME: We should re-process old failures after some time delay.
    # https://bugs.webkit.org/show_bug.cgi?id=36581
    def filter_out_old_failures(self, is_old_failure):
        old_failures = self._old_failures(is_old_failure)
        old_failing_builder_names = set([builder.name()
            for builder in self._builders_failing_because_of(old_failures)])

        # We filter out all the failing builders that could have been caused
        # by old_failures.  We could miss some new failures this way, but
        # emperically, this reduces the amount of spam we generate.
        failures = self._failures
        self._failures = [failure_info for failure_info in failures
            if failure_info['builder'].name() not in old_failing_builder_names]
        self._cache = {}
