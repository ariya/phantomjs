#!/usr/bin/env python

# Copyright (c) 2009 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verified things don't explode when there are targets without outputs.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('nooutput.gyp', chdir='src')
test.relocate('src', 'relocate/src')
test.build('nooutput.gyp', chdir='relocate/src')

test.pass_test()
