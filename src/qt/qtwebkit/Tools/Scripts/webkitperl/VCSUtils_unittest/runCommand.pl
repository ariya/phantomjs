#!/usr/bin/perl -w
#
# Copyright (C) 2012 Daniel Bates (dbates@intudata.com). All rights reserved.
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
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Unit tests of VCSUtils::runCommand().

use strict;
use warnings;

use Test::More;
use VCSUtils;

use constant ENOENT => 2; # See <errno.h>

# The array of test cases.
my @testCaseHashRefs = (
{
    # New test
    testName => "Simple",
    inputArgs => ["echo", "hello"],
    expectedReturn => {
        exitStatus => 0,
        stdout => "hello\n"
    }
},
{
    # New test
    testName => "Multiple commands",
    inputArgs => ["echo", "first-command;echo second-command"],
    expectedReturn => {
        exitStatus => 0,
        stdout => "first-command;echo second-command\n"
    }
},
{
    # New test
    testName => "Non-existent command",
    inputArgs => ["/usr/bin/non-existent-command"],
    expectedReturn => {
        exitStatus => ENOENT
    }
}
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => $testCasesCount); # Total number of assertions.

foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "runCommand(): $testCase->{testName}: comparing";

    my $got = VCSUtils::runCommand(@{$testCase->{inputArgs}});
    my $expectedReturn = $testCase->{expectedReturn};

    is_deeply($got, $expectedReturn, "$testNameStart return value.");
}
