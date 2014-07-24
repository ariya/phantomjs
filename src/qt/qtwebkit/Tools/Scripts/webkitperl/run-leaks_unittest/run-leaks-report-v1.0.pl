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

# tests run-leaks using original leaks report version 1.0

use strict;
use warnings;

use File::Spec;
use FindBin;
use lib File::Spec->catdir($FindBin::Bin, "..");
use Test::More;
use LoadAsModule qw(RunLeaks run-leaks);

my @input = split(/\n/, <<EOF);
Process 1602: 86671 nodes malloced for 13261 KB
Process 1602: 8 leaks for 160 total leaked bytes.
Leak: 0x114d54708  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x18571798 0x00000001 0x00000000 0x00000000 	..W.............
Leak: 0x1184b92b8  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x184b9048 0x00000001 0x00000000 0x00000000 	H.K.............
Leak: 0x1184c84c8  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x1854e3d8 0x00000001 0x00000000 0x00000000 	..T.............
Leak: 0x11854e3d8  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x1854e360 0x00000001 0x00000000 0x00000000 	`.T.............
Leak: 0x118571798  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x184c84c8 0x00000001 0x00000000 0x00000000 	..L.............
Leak: 0x11858b498  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x1858b4e0 0x00000001 0x00000000 0x00000000 	..X.............
Leak: 0x118572530  size=8  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
Leak: 0x118572538  size=8  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
EOF

my $expectedOutput =
[
  {
    'leaksOutput' => join('', split(/\n/, <<EOF)),
Leak: 0x114d54708  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x18571798 0x00000001 0x00000000 0x00000000 	..W.............
EOF
    'callStack' => '',
    'address' => '0x114d54708',
    'size' => '24',
    'type' => '',
  },
  {
    'leaksOutput' => join('', split(/\n/, <<EOF)),
Leak: 0x1184b92b8  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x184b9048 0x00000001 0x00000000 0x00000000 	H.K.............
EOF
    'callStack' => '',
    'address' => '0x1184b92b8',
    'size' => '24',
    'type' => '',
  },

  {
    'leaksOutput' => join('', split(/\n/, <<EOF)),
Leak: 0x1184c84c8  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x1854e3d8 0x00000001 0x00000000 0x00000000 	..T.............
EOF
    'callStack' => '',
    'address' => '0x1184c84c8',
    'size' => '24',
    'type' => '',
  },

  {
    'leaksOutput' => join('', split(/\n/, <<EOF)),
Leak: 0x11854e3d8  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x1854e360 0x00000001 0x00000000 0x00000000 	`.T.............
EOF
    'callStack' => '',
    'address' => '0x11854e3d8',
    'size' => '24',
    'type' => '',
  },

  {
    'leaksOutput' => join('', split(/\n/, <<EOF)),
Leak: 0x118571798  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x184c84c8 0x00000001 0x00000000 0x00000000 	..L.............
EOF
    'callStack' => '',
    'address' => '0x118571798',
    'size' => '24',
    'type' => '',
  },

  {
    'leaksOutput' => join('', split(/\n/, <<EOF)),
Leak: 0x11858b498  size=24  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
	0x1858b4e0 0x00000001 0x00000000 0x00000000 	..X.............
EOF
    'callStack' => '',
    'address' => '0x11858b498',
    'size' => '24',
    'type' => '',
  },

  {
    'leaksOutput' => join('', split(/\n/, <<EOF)),
Leak: 0x118572530  size=8  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
EOF
    'callStack' => '',
    'address' => '0x118572530',
    'size' => '8',
    'type' => '',
  },

  {
    'leaksOutput' => join('', split(/\n/, <<EOF)),
Leak: 0x118572538  size=8  zone: JavaScriptCore FastMalloc_0x7fff70a09d20
	
EOF
    'callStack' => '',
    'address' => '0x118572538',
    'size' => '8',
    'type' => '',
  },
];

my $actualOutput = RunLeaks::parseLeaksOutput(@input);

plan(tests => 1);
is_deeply($actualOutput, $expectedOutput, "leaks Report Version 1.0 - no call stack");
