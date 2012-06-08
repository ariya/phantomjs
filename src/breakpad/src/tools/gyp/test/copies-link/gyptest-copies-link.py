#!/usr/bin/env python

# Copyright (c) 2009 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies file copies using the build tool default.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('copies-link.gyp', chdir='src')

test.relocate('src', 'relocate/src')

test.build('copies-link.gyp', chdir='relocate/src')

test.pass_test()
