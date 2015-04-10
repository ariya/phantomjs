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

import unittest2 as unittest

from webkitpy.common.net.resultsjsonparser import ResultsJSONParser
from webkitpy.layout_tests.models import test_results
from webkitpy.layout_tests.models import test_failures


class ResultsJSONParserTest(unittest.TestCase):
    # The real files have no whitespace, but newlines make this much more readable.

    _example_full_results_json = """ADD_RESULTS({
    "tests": {
        "fast": {
            "dom": {
                "prototype-inheritance.html": {
                    "expected": "PASS",
                    "actual": "FAIL"
                },
                "prototype-banana.html": {
                    "expected": "FAIL",
                    "actual": "PASS"
                },
                "prototype-taco.html": {
                    "expected": "PASS",
                    "actual": "PASS FAIL"
                },
                "prototype-chocolate.html": {
                    "expected": "FAIL",
                    "actual": "FAIL"
                },
                "prototype-strawberry.html": {
                    "expected": "PASS",
                    "actual": "FAIL PASS"
                }
            }
        },
        "svg": {
            "dynamic-updates": {
                "SVGFEDropShadowElement-dom-stdDeviation-attr.html": {
                    "expected": "PASS",
                    "actual": "IMAGE",
                    "has_stderr": true
                }
            }
        }
    },
    "skipped": 450,
    "num_regressions": 15,
    "layout_tests_dir": "\/b\/build\/slave\/Webkit_Mac10_5\/build\/src\/third_party\/WebKit\/LayoutTests",
    "version": 3,
    "num_passes": 77,
    "has_pretty_patch": false,
    "fixable": 1220,
    "num_flaky": 0,
    "uses_expectations_file": true,
    "has_wdiff": false
});"""

    def test_basic(self):
        expected_results = [
            test_results.TestResult("svg/dynamic-updates/SVGFEDropShadowElement-dom-stdDeviation-attr.html", [test_failures.FailureImageHashMismatch()], 0),
            test_results.TestResult("fast/dom/prototype-inheritance.html", [test_failures.FailureTextMismatch(), test_failures.FailureImageHashMismatch(), test_failures.FailureAudioMismatch()], 0),
        ]
        results = ResultsJSONParser.parse_results_json(self._example_full_results_json)
        self.assertEqual(expected_results, results)
