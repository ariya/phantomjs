#!/usr/bin/env python
#
# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#         * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#         * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#         * Neither the name of Google Inc. nor the names of its
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
#

# This script replaces calls to importScripts with script sources
# in input script file and dumps result into output script file.

from cStringIO import StringIO

import os.path
import re
import sys


def main(argv):

    if len(argv) < 3:
        print('usage: %s inputFile importsDir outputFile' % argv[0])
        return 1

    inputFileName = argv[1]
    importsDir = argv[2]
    outputFileName = argv[3]

    inputFile = open(inputFileName, 'r')
    inputScript = inputFile.read()
    inputFile.close()

    def replace(match):
        importFileName = match.group(1)
        fullPath = os.path.join(importsDir, importFileName)
        if not os.access(fullPath, os.F_OK):
            raise Exception('File %s referenced in %s not found on any source paths, '
                            'check source tree for consistency' %
                            (importFileName, inputFileName))
        importFile = open(fullPath, 'r')
        importScript = importFile.read()
        importFile.close()
        return importScript

    outputScript = re.sub(r'importScripts?\([\'"]([^\'"]+)[\'"]\)', replace, inputScript)

    outputFile = open(outputFileName, 'w')
    outputFile.write(outputScript)
    outputFile.close()

    # Touch output file directory to make sure that Xcode will copy
    # modified resource files.
    if sys.platform == 'darwin':
        outputDirName = os.path.dirname(outputFileName)
        os.utime(outputDirName, None)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
