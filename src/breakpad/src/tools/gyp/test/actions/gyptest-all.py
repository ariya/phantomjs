#!/usr/bin/env python

# Copyright (c) 2009 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies simple actions when using an explicit build target of 'all'.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('actions.gyp', chdir='src')

test.relocate('src', 'relocate/src')

test.build('actions.gyp', test.ALL, chdir='relocate/src')


expect = """\
Hello from program.c
Hello from make-prog1.py
Hello from make-prog2.py
"""

if test.format == 'xcode':
  chdir = 'relocate/src/subdir1'
else:
  chdir = 'relocate/src'
test.run_built_executable('program', chdir=chdir, stdout=expect)


test.must_match('relocate/src/subdir2/file.out', "Hello from make-file.py\n")


expect = "Hello from generate_main.py\n"

if test.format == 'xcode':
  chdir = 'relocate/src/subdir3'
else:
  chdir = 'relocate/src'
test.run_built_executable('null_input', chdir=chdir, stdout=expect)


test.pass_test()
