# Copyright (c) 2010, Google Inc. All rights reserved.
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

from webkitpy.common.net.resultsjsonparser import ResultsJSONParser
from webkitpy.thirdparty.BeautifulSoup import BeautifulSoup, SoupStrainer
from webkitpy.layout_tests.models import test_results
from webkitpy.layout_tests.models import test_failures

_log = logging.getLogger(__name__)


# FIXME: This should be unified with all the layout test results code in the layout_tests package
# This doesn't belong in common.net, but we don't have a better place for it yet.
def path_for_layout_test(test_name):
    return "LayoutTests/%s" % test_name


# FIXME: This should be unified with ResultsSummary or other NRWT layout tests code
# in the layout_tests package.
# This doesn't belong in common.net, but we don't have a better place for it yet.
class LayoutTestResults(object):
    @classmethod
    def results_from_string(cls, string):
        if not string:
            return None
        test_results = ResultsJSONParser.parse_results_json(string)
        if not test_results:
            return None
        return cls(test_results)

    def __init__(self, test_results):
        self._test_results = test_results
        self._failure_limit_count = None
        self._unit_test_failures = []

    # FIXME: run-webkit-tests should store the --exit-after-N-failures value
    # (or some indication of early exit) somewhere in the results.json
    # file.  Until it does, callers should set the limit to
    # --exit-after-N-failures value used in that run.  Consumers of LayoutTestResults
    # may use that value to know if absence from the failure list means PASS.
    # https://bugs.webkit.org/show_bug.cgi?id=58481
    def set_failure_limit_count(self, limit):
        self._failure_limit_count = limit

    def failure_limit_count(self):
        return self._failure_limit_count

    def test_results(self):
        return self._test_results

    def results_matching_failure_types(self, failure_types):
        return [result for result in self._test_results if result.has_failure_matching_types(*failure_types)]

    def tests_matching_failure_types(self, failure_types):
        return [result.test_name for result in self.results_matching_failure_types(failure_types)]

    def failing_test_results(self):
        return self.results_matching_failure_types(test_failures.ALL_FAILURE_CLASSES)

    def failing_tests(self):
        return [result.test_name for result in self.failing_test_results()] + self._unit_test_failures

    def add_unit_test_failures(self, unit_test_results):
        self._unit_test_failures = unit_test_results
