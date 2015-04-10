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

import unittest2 as unittest

from webkitpy.port.leakdetector import LeakDetector
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.system.executive_mock import MockExecutive


class LeakDetectorTest(unittest.TestCase):
    def _mock_port(self):
        class MockPort(object):
            def __init__(self):
                self._filesystem = MockFileSystem()
                self._executive = MockExecutive()

        return MockPort()

    def _make_detector(self):
        return LeakDetector(self._mock_port())

    def test_leaks_args(self):
        detector = self._make_detector()
        detector._callstacks_to_exclude_from_leaks = lambda: ['foo bar', 'BAZ']
        detector._types_to_exlude_from_leaks = lambda: ['abcdefg', 'hi jklmno']
        expected_args = ['--exclude-callstack=foo bar', '--exclude-callstack=BAZ', '--exclude-type=abcdefg', '--exclude-type=hi jklmno', 1234]
        self.assertEqual(detector._leaks_args(1234), expected_args)

    example_leaks_output = """Process 5122: 663744 nodes malloced for 78683 KB
Process 5122: 337301 leaks for 6525216 total leaked bytes.
Leak: 0x38cb600  size=3072  zone: DefaultMallocZone_0x1d94000   instance of 'NSCFData', type ObjC, implemented in Foundation
        0xa033f0b8 0x01001384 0x00000b3a 0x00000b3a     ..3.....:...:...
        0x00000000 0x038cb620 0x00000000 0x00000000     .... ...........
        0x00000000 0x21000000 0x726c6468 0x00000000     .......!hdlr....
        0x00000000 0x7269646d 0x6c707061 0x00000000     ....mdirappl....
        0x00000000 0x04000000 0x736c69c1 0x00000074     .........ilst...
        0x6f74a923 0x0000006f 0x7461641b 0x00000061     #.too....data...
        0x00000001 0x76614c00 0x2e323566 0x302e3236     .....Lavf52.62.0
        0x37000000 0x6d616ea9 0x2f000000 0x61746164     ...7.nam.../data
        ...
Leak: 0x2a9c960  size=288  zone: DefaultMallocZone_0x1d94000
        0x09a1cc47 0x1bda8560 0x3d472cd1 0xfbe9bccd     G...`....,G=....
        0x8bcda008 0x9e972a91 0xa892cf63 0x2448bdb0     .....*..c.....H$
        0x4736fc34 0xdbe2d94e 0x25f56688 0x839402a4     4.6GN....f.%....
        0xd12496b3 0x59c40c12 0x8cfcab2a 0xd20ef9c4     ..$....Y*.......
        0xe7c56b1b 0x5835af45 0xc69115de 0x6923e4bb     .k..E.5X......#i
        0x86f15553 0x15d40fa9 0x681288a4 0xc33298a9     SU.........h..2.
        0x439bb535 0xc4fc743d 0x7dfaaff8 0x2cc49a4a     5..C=t.....}J..,
        0xdd119df8 0x7e086821 0x3d7d129e 0x2e1b1547     ....!h.~..}=G...
        ...
Leak: 0x25102fe0  size=176  zone: DefaultMallocZone_0x1d94000   string 'NSException Data'
"""

    example_leaks_output_with_exclusions = """
Process 57064: 865808 nodes malloced for 81032 KB
Process 57064: 282 leaks for 21920 total leaked bytes.
Leak: 0x7fc506023960  size=576  zone: DefaultMallocZone_0x107c29000   URLConnectionLoader::LoaderConnectionEventQueue  C++  CFNetwork
        0x73395460 0x00007fff 0x7488af40 0x00007fff     `T9s....@..t....
        0x73395488 0x00007fff 0x46eecd74 0x0001ed83     .T9s....t..F....
        0x0100000a 0x00000000 0x7488bfc0 0x00007fff     ...........t....
        0x00000000 0x00000000 0x46eecd8b 0x0001ed83     ...........F....
        0x00000000 0x00000000 0x00000000 0x00000000     ................
        0x00000000 0x00000000 0x46eecda3 0x0001ed83     ...........F....
        0x00000000 0x00000000 0x00000000 0x00000000     ................
        0x00000000 0x00000000 0x46eecdbc 0x0001ed83     ...........F....
        ...
Leak: 0x7fc506025980  size=432  zone: DefaultMallocZone_0x107c29000   URLConnectionInstanceData  CFType  CFNetwork
        0x74862b28 0x00007fff 0x00012b80 0x00000001     (+.t.....+......
        0x73395310 0x00007fff 0x733953f8 0x00007fff     .S9s.....S9s....
        0x4d555458 0x00000000 0x00000000 0x00002068     XTUM........h ..
        0x00000000 0x00000000 0x00000b00 0x00000b00     ................
        0x00000000 0x00000000 0x060259b8 0x00007fc5     .........Y......
        0x060259bc 0x00007fc5 0x00000000 0x00000000     .Y..............
        0x73395418 0x00007fff 0x06025950 0x00007fc5     .T9s....PY......
        0x73395440 0x00007fff 0x00005013 0x00000001     @T9s.....P......
        ...


Binary Images:
       0x107ac2000 -        0x107b4aff7 +DumpRenderTree (??? - ???) <5694BE03-A60A-30B2-9D40-27CFFCFB88EE> /Volumes/Data/WebKit-BuildSlave/lion-intel-leaks/build/WebKitBuild/Debug/DumpRenderTree
       0x107c2f000 -        0x107c58fff +libWebCoreTestSupport.dylib (535.8.0 - compatibility 1.0.0) <E4F7A13E-5807-30F7-A399-62F8395F9106> /Volumes/Data/WebKit-BuildSlave/lion-intel-leaks/build/WebKitBuild/Debug/libWebCoreTestSupport.dylib
17 leaks excluded (not printed)
"""

    def test_parse_leaks_output(self):
        self.assertEqual(self._make_detector()._parse_leaks_output(self.example_leaks_output), (337301, 0, 6525216))
        self.assertEqual(self._make_detector()._parse_leaks_output(self.example_leaks_output_with_exclusions), (282, 17, 21920))

    def test_leaks_files_in_directory(self):
        detector = self._make_detector()
        self.assertEqual(detector.leaks_files_in_directory('/bogus-directory'), [])
        detector._filesystem = MockFileSystem({
            '/mock-results/DumpRenderTree-1234-leaks.txt': '',
            '/mock-results/DumpRenderTree-23423-leaks.txt': '',
            '/mock-results/DumpRenderTree-823-leaks.txt': '',
        })
        self.assertEqual(len(detector.leaks_files_in_directory('/mock-results')), 3)

    def test_count_total_bytes_and_unique_leaks(self):
        detector = self._make_detector()

        def mock_run_script(name, args, include_configuration_arguments=False):
            print "MOCK _run_script: %s %s" % (name, args)
            return """1 calls for 16 bytes: -[NSURLRequest mutableCopyWithZone:] | +[NSObject(NSObject) allocWithZone:] | _internal_class_createInstanceFromZone | calloc | malloc_zone_calloc

147 calls for 9,408 bytes: _CFRuntimeCreateInstance | _ZN3WTF24StringWrapperCFAllocatorL8allocateElmPv StringImplCF.cpp:67 | WTF::fastMalloc(unsigned long) FastMalloc.cpp:268 | malloc | malloc_zone_malloc 

total: 5,888 bytes (0 bytes excluded)."""
        detector._port._run_script = mock_run_script

        leak_files = ['/mock-results/DumpRenderTree-1234-leaks.txt', '/mock-results/DumpRenderTree-1235-leaks.txt']
        expected_stdout = "MOCK _run_script: parse-malloc-history ['--merge-depth', 5, '/mock-results/DumpRenderTree-1234-leaks.txt', '/mock-results/DumpRenderTree-1235-leaks.txt']\n"
        results_tuple = OutputCapture().assert_outputs(self, detector.count_total_bytes_and_unique_leaks, [leak_files], expected_stdout=expected_stdout)
        self.assertEqual(results_tuple, ("5,888 bytes", 2))

    def test_count_total_leaks(self):
        detector = self._make_detector()
        detector._filesystem = MockFileSystem({
            # The \xff is some non-utf8 characters to make sure we don't blow up trying to parse the file.
            '/mock-results/DumpRenderTree-1234-leaks.txt': '\xff\nProcess 1234: 12 leaks for 40 total leaked bytes.\n\xff\n',
            '/mock-results/DumpRenderTree-23423-leaks.txt': 'Process 1235: 12341 leaks for 27934 total leaked bytes.\n',
            '/mock-results/DumpRenderTree-823-leaks.txt': 'Process 12356: 23412 leaks for 18 total leaked bytes.\n',
        })
        leak_file_paths = ['/mock-results/DumpRenderTree-1234-leaks.txt', '/mock-results/DumpRenderTree-23423-leaks.txt', '/mock-results/DumpRenderTree-823-leaks.txt']
        self.assertEqual(detector.count_total_leaks(leak_file_paths), 35765)
