#!/usr/bin/env python

# Copyright (c) 2009 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verify that a link time only dependency will get pulled into the set of built
targets, even if no executable uses it.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('lib_only.gyp')

test.build('lib_only.gyp', test.ALL)

# Make doesn't put static libs in a common 'lib' directory, like it does with
# shared libs, so check in the obj path corresponding to the source path.
test.built_file_must_exist('a', type=test.STATIC_LIB, libdir='obj.target')

# TODO(bradnelson/mark):
# On linux and windows a library target will at least pull its link dependencies
# into the generated sln/_main.scons, since not doing so confuses users.
# This is not currently implemented on mac, which has the opposite behavior.
if test.format == 'xcode':
  test.built_file_must_not_exist('b', type=test.STATIC_LIB)
else:
  test.built_file_must_exist('b', type=test.STATIC_LIB, libdir='obj.target/b')

test.pass_test()
