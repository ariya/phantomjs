#!/usr/bin/perl -w

# Copyright (C) 2011 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# tests run-leaks using "old" leaks report version 2.0
# - The "old" 2.0 format has "leaks Report Version:  2.0" at the top of the report.

use strict;
use warnings;

use File::Spec;
use FindBin;
use lib File::Spec->catdir($FindBin::Bin, "..");
use Test::More;
use LoadAsModule qw(RunLeaks run-leaks);

my @input = split(/\n/, <<EOF);
leaks Report Version:  2.0
Process:         Safari [53606]
Path:            /Applications/Safari.app/Contents/MacOS/Safari
Load Address:    0x100000000
Identifier:      com.apple.Safari
Version:         5.0 (6533.9)
Build Info:      WebBrowser-75330900~1
Code Type:       X86-64 (Native)
Parent Process:  perl5.10.0 [53599]

Date/Time:       2010-05-27 11:42:27.356 -0700
OS Version:      Mac OS X 10.6.3 (10D571)
Report Version:  6

Process 53606: 112295 nodes malloced for 22367 KB
Process 53606: 1 leak for 32 total leaked bytes.
Leak: 0x1118c0e60  size=32  zone: DefaultMallocZone_0x105a92000	string 'com.apple.quarantine'
	Call stack: [thread 0x7fff70126be0]: | 0x100001e84 | NSApplicationMain | +[NSBundle(NSNibLoading) loadNibNamed:owner:] | +[NSBundle(NSNibLoading) _loadNibFile:nameTable:withZone:ownerBundle:] | loadNib | -[NSIBObjectData nibInstantiateWithOwner:topLevelObjects:] | -[NSSet makeObjectsPerformSelector:] | 0x100003494 | 0x1001013ff | 0x10014dbb9 | 0x10014d923 | 0x10014d7d7 | 0x10014ccd9 | 0x100149c8e | 0x100149bd8 | xar_open | xar_file_unserialize | xar_prop_unserialize | xar_prop_unserialize | strdup | malloc | malloc_zone_malloc 
EOF

my $expectedOutput =
[
  {
    'leaksOutput' => join('', split(/\n/, <<EOF)),
Leak: 0x1118c0e60  size=32  zone: DefaultMallocZone_0x105a92000	string 'com.apple.quarantine'
	Call stack: [thread 0x7fff70126be0]: | 0x100001e84 | NSApplicationMain | +[NSBundle(NSNibLoading) loadNibNamed:owner:] | +[NSBundle(NSNibLoading) _loadNibFile:nameTable:withZone:ownerBundle:] | loadNib | -[NSIBObjectData nibInstantiateWithOwner:topLevelObjects:] | -[NSSet makeObjectsPerformSelector:] | 0x100003494 | 0x1001013ff | 0x10014dbb9 | 0x10014d923 | 0x10014d7d7 | 0x10014ccd9 | 0x100149c8e | 0x100149bd8 | xar_open | xar_file_unserialize | xar_prop_unserialize | xar_prop_unserialize | strdup | malloc | malloc_zone_malloc 
EOF
    'callStack' => 
'	Call stack: [thread 0x7fff70126be0]: | 0x100001e84 | NSApplicationMain | +[NSBundle(NSNibLoading) loadNibNamed:owner:] | +[NSBundle(NSNibLoading) _loadNibFile:nameTable:withZone:ownerBundle:] | loadNib | -[NSIBObjectData nibInstantiateWithOwner:topLevelObjects:] | -[NSSet makeObjectsPerformSelector:] | 0x100003494 | 0x1001013ff | 0x10014dbb9 | 0x10014d923 | 0x10014d7d7 | 0x10014ccd9 | 0x100149c8e | 0x100149bd8 | xar_open | xar_file_unserialize | xar_prop_unserialize | xar_prop_unserialize | strdup | malloc | malloc_zone_malloc ',
    'address' => '0x1118c0e60',
    'size' => '32',
    'type' => 'com.apple.quarantine',
  },
];

my $actualOutput = RunLeaks::parseLeaksOutput(@input);

plan(tests => 1);
is_deeply($actualOutput, $expectedOutput, "leaks Report Version 2.0 (old)");
