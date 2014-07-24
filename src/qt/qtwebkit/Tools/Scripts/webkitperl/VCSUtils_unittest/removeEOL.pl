#!/usr/bin/perl
#
# Copyright (C) Research In Motion Limited 2010. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Research In Motion Limited nor the names of
# its contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
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

# Unit tests of VCSUtils::removeEOL().

use Test::Simple tests => 5;
use VCSUtils;

my $title;

# New test
$title = "removeEOL: Undefined argument.";
ok(removeEOL(undef) eq "", $title);

# New test
$title = "removeEOL: Line with Windows line ending.";
ok(removeEOL("This line ends with a Windows line ending.\r\n") eq "This line ends with a Windows line ending.", $title);

# New test
$title = "removeEOL: Line with Unix line ending.";
ok(removeEOL("This line ends with a Unix line ending.\n") eq "This line ends with a Unix line ending.", $title);

# New test
$title = "removeEOL: Line with Mac line ending.";
ok(removeEOL("This line ends with a Mac line ending.\r") eq "This line ends with a Mac line ending.", $title);

# New test
$title = "removeEOL: Line with a mix of line endings.";
ok(removeEOL("This line contains a mix of line endings.\r\n\r\n\r\r\n\n\n\n") eq "This line contains a mix of line endings.", $title);
