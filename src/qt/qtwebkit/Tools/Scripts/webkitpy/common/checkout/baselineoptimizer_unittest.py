# Copyright (C) 2011 Google Inc. All rights reserved.
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

import sys
import unittest2 as unittest

from webkitpy.common.checkout.baselineoptimizer import BaselineOptimizer
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.host_mock import MockHost


class TestBaselineOptimizer(BaselineOptimizer):
    def __init__(self, mock_results_by_directory):
        host = MockHost()
        BaselineOptimizer.__init__(self, host, host.port_factory.all_port_names())
        self._mock_results_by_directory = mock_results_by_directory

    # We override this method for testing so we don't have to construct an
    # elaborate mock file system.
    def read_results_by_directory(self, baseline_name):
        return self._mock_results_by_directory

    def _move_baselines(self, baseline_name, results_by_directory, new_results_by_directory):
        self.new_results_by_directory = new_results_by_directory


class BaselineOptimizerTest(unittest.TestCase):
    def _assertOptimization(self, results_by_directory, expected_new_results_by_directory):
        baseline_optimizer = TestBaselineOptimizer(results_by_directory)
        self.assertTrue(baseline_optimizer.optimize('mock-baseline.png'))
        self.assertEqual(baseline_optimizer.new_results_by_directory, expected_new_results_by_directory)

    def _assertOptimizationFailed(self, results_by_directory):
        baseline_optimizer = TestBaselineOptimizer(results_by_directory)
        self.assertFalse(baseline_optimizer.optimize('mock-baseline.png'))

    def test_move_baselines(self):
        host = MockHost()
        host.filesystem.write_binary_file('/mock-checkout/LayoutTests/platform/mac-lion/another/test-expected.txt', 'result A')
        host.filesystem.write_binary_file('/mock-checkout/LayoutTests/platform/mac-lion-wk2/another/test-expected.txt', 'result A')
        host.filesystem.write_binary_file('/mock-checkout/LayoutTests/platform/mac/another/test-expected.txt', 'result B')
        baseline_optimizer = BaselineOptimizer(host, host.port_factory.all_port_names())
        baseline_optimizer._move_baselines('another/test-expected.txt', {
            'LayoutTests/platform/mac-lion': 'aaa',
            'LayoutTests/platform/mac-lion-wk2': 'aaa',
            'LayoutTests/platform/mac': 'bbb',
        }, {
            'LayoutTests/platform/mac': 'aaa',
        })
        self.assertEqual(host.filesystem.read_binary_file('/mock-checkout/LayoutTests/platform/mac/another/test-expected.txt'), 'result A')

    def test_efl(self):
        self._assertOptimization({
            'LayoutTests/platform/efl': '462d03b9c025db1b0392d7453310dbee5f9a9e74',
        }, {
            'LayoutTests/platform/efl': '462d03b9c025db1b0392d7453310dbee5f9a9e74',
        })

    def test_no_add_mac_future(self):
        self._assertOptimization({
            'LayoutTests/platform/mac': '29a1715a6470d5dd9486a142f609708de84cdac8',
            'LayoutTests/platform/win-xp': '453e67177a75b2e79905154ece0efba6e5bfb65d',
            'LayoutTests/platform/mac-lion': 'c43eaeb358f49d5e835236ae23b7e49d7f2b089f',
        }, {
            'LayoutTests/platform/mac': '29a1715a6470d5dd9486a142f609708de84cdac8',
            'LayoutTests/platform/win-xp': '453e67177a75b2e79905154ece0efba6e5bfb65d',
            'LayoutTests/platform/mac-lion': 'c43eaeb358f49d5e835236ae23b7e49d7f2b089f',
        })

    def test_mac_future(self):
        self._assertOptimization({
            'LayoutTests/platform/mac-lion': '462d03b9c025db1b0392d7453310dbee5f9a9e74',
        }, {
            'LayoutTests/platform/mac-lion': '462d03b9c025db1b0392d7453310dbee5f9a9e74',
        })

    def test_qt_unknown(self):
        self._assertOptimization({
            'LayoutTests/platform/qt': '462d03b9c025db1b0392d7453310dbee5f9a9e74',
        }, {
            'LayoutTests/platform/qt': '462d03b9c025db1b0392d7453310dbee5f9a9e74',
        })

    def test_win_does_not_drop_to_win_7sp0(self):
        self._assertOptimization({
            'LayoutTests/platform/win': '1',
            'LayoutTests/platform/mac': '2',
            'LayoutTests/platform/gtk': '3',
            'LayoutTests/platform/qt': '4',
        }, {
            'LayoutTests/platform/win': '1',
            'LayoutTests/platform/mac': '2',
            'LayoutTests/platform/gtk': '3',
            'LayoutTests/platform/qt': '4',
        })

    def test_common_directory_includes_root(self):
        # This test case checks that we don't throw an exception when we fail
        # to optimize.
        self._assertOptimizationFailed({
            'LayoutTests/platform/gtk': 'e8608763f6241ddacdd5c1ef1973ba27177d0846',
            'LayoutTests/platform/qt': 'bcbd457d545986b7abf1221655d722363079ac87',
            'LayoutTests/platform/mac': 'e8608763f6241ddacdd5c1ef1973ba27177d0846',
        })

        self._assertOptimization({
            'LayoutTests': '9c876f8c3e4cc2aef9519a6c1174eb3432591127',
        }, {
            'LayoutTests': '9c876f8c3e4cc2aef9519a6c1174eb3432591127',
        })

    def test_complex_shadowing(self):
        # This test relies on OS specific functionality, so it doesn't work on Windows.
        # FIXME: What functionality does this rely on?  When can we remove this if?
        if sys.platform == 'win32':
            return
        self._assertOptimization({
            'LayoutTests/platform/mac': '5daa78e55f05d9f0d1bb1f32b0cd1bc3a01e9364',
            'LayoutTests/platform/mac-lion': '7ad045ece7c030e2283c5d21d9587be22bcba56e',
            'LayoutTests/platform/win-xp': '5b1253ef4d5094530d5f1bc6cdb95c90b446bec7',
        }, {
            'LayoutTests/platform/mac': '5daa78e55f05d9f0d1bb1f32b0cd1bc3a01e9364',
            'LayoutTests/platform/mac-lion': '7ad045ece7c030e2283c5d21d9587be22bcba56e',
            'LayoutTests/platform/win-xp': '5b1253ef4d5094530d5f1bc6cdb95c90b446bec7',
        })

    def test_virtual_ports_filtered(self):
        self._assertOptimization({
            'LayoutTests/platform/gtk': '3',
            'LayoutTests/platform/efl': '3',
            'LayoutTests/platform/qt': '4',
            'LayoutTests/platform/mac': '5',
        }, {
            'LayoutTests': '3',
            'LayoutTests/platform/qt': '4',
            'LayoutTests/platform/mac': '5',
        })
