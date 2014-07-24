# Copyright (C) 2010 Google Inc. All rights reserved.
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

"""FIXME: This script is used by
config_unittest.test_default_configuration__standalone() to read the
default configuration to work around any possible caching / reset bugs. See
https://bugs.webkit.org/show_bug?id=49360 for the motivation. We can remove
this test when we remove the global configuration cache in config.py."""

import os
import sys


# Ensure that webkitpy is in PYTHONPATH.
this_dir = os.path.abspath(sys.path[0])
up = os.path.dirname
script_dir = up(up(up(this_dir)))
if script_dir not in sys.path:
    sys.path.append(script_dir)

from webkitpy.common.system import executive
from webkitpy.common.system import executive_mock
from webkitpy.common.system import filesystem
from webkitpy.common.system import filesystem_mock

import config


def main(argv=None):
    if not argv:
        argv = sys.argv

    if len(argv) == 3 and argv[1] == '--mock':
        e = executive_mock.MockExecutive2(output='foo\nfoo/%s' % argv[2])
        fs = filesystem_mock.MockFileSystem({'foo/Configuration': argv[2]})
    else:
        e = executive.Executive()
        fs = filesystem.FileSystem()

    c = config.Config(e, fs)
    print c.default_configuration()

if __name__ == '__main__':
    main()
