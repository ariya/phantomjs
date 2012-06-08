#!/usr/bin/env python

# Copyright (c) 2009 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies building a subsidiary dependent target from a .gyp file in a
subdirectory, without specifying an explicit output build directory,
and using the subdirectory's solution or project file as the entry point.
"""

import TestGyp
import errno

test = TestGyp.TestGyp()

test.run_gyp('prog1.gyp', chdir='src')

test.relocate('src', 'relocate/src')

chdir = 'relocate/src/subdir'
target = test.ALL

# Make can build sub-projects, but it's still through the top-level Makefile,
# and there is no 'default' or 'all' sub-project, so the target must be
# explicit.
# TODO(mmoss) Should make create self-contained, sub-project Makefiles,
# equilvalent to the sub-project .sln/SConstruct/etc. files of other generators?
if test.format == 'make':
  chdir = 'relocate/src'
  target = 'prog2'

test.build('prog2.gyp', target, chdir=chdir)

test.built_file_must_not_exist('prog1', type=test.EXECUTABLE, chdir=chdir)

test.run_built_executable('prog2',
                          chdir=chdir,
                          stdout="Hello from prog2.c\n")

test.pass_test()
