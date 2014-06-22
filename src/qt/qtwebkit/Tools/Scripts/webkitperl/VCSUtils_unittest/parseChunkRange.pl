#!/usr/bin/perl -w
#
# Copyright (C) 2011 Research In Motion Limited. All rights reserved.
# Copyright (C) 2010 Chris Jerdonek (chris.jerdonek@gmail.com)
# Copyright (C) 2012 Daniel Bates (dbates@intudata.com)
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
#     * Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

# Unit tests of VCSUtils::parseChunkRange().

use strict;
use warnings;

use Test::More;
use VCSUtils;

my @testCaseHashRefs = (
###
# Invalid and malformed chunk range
##
# FIXME: We should make this set of tests more comprehensive.
{   # New test
    testName => "[invalid] Empty string",
    inputText => "",
    expectedReturn => []
},
{   # New test
    testName => "[invalid] Bogus chunk range",
    inputText => "@@ this is not valid @@",
    expectedReturn => []
},
{   # New test
    testName => "[invalid] Chunk range missing -/+ prefix",
    inputText => "@@ 0,0 1,4 @@",
    expectedReturn => []
},
{   # New test
    testName => "[invalid] Chunk range missing commas",
    inputText => "@@ -0 0 +1 4 @@",
    expectedReturn => []
},
{   # New test
    testName => "[invalid] Chunk range with swapped old and rew ranges",
    inputText => "@@ +0,0 -1,4 @@",
    expectedReturn => []
},
{   # New test
    testName => "[invalid] Chunk range with leading junk",
    inputText => "leading junk @@ -0,0 +1,4 @@",
    expectedReturn => []
},
###
#  Simple test cases
##
{   # New test
    testName => "Line count is 0",
    inputText => "@@ -0,0 +1,4 @@",
    expectedReturn => [
{
    startingLine => 0,
    lineCount => 0,
    newStartingLine => 1,
    newLineCount => 4,
}
]
},
{   # New test
    testName => "Line count is 1",
    inputText => "@@ -1 +1,4 @@",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 1,
    newStartingLine => 1,
    newLineCount => 4,
}
]
},
{   # New test
    testName => "Both original and new line count is 1",
    inputText => "@@ -1 +1 @@",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 1,
    newStartingLine => 1,
    newLineCount => 1,
}
]
},
{   # New test
    testName => "Line count and new line count > 1",
    inputText => "@@ -1,2 +1,4 @@",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 2,
    newStartingLine => 1,
    newLineCount => 4,
}
]
},
{   # New test
    testName => "New line count is 0",
    inputText => "@@ -1,4 +0,0 @@",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 4,
    newStartingLine => 0,
    newLineCount => 0,
}
]
},
{   # New test
    testName => "New line count is 1",
    inputText => "@@ -1,4 +1 @@",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 4,
    newStartingLine => 1,
    newLineCount => 1,
}
]
},
###
#  Simple SVN 1.7 property diff chunk range tests
##
{   # New test
    testName => "Line count is 0",
    inputText => "## -0,0 +1,4 ##",
    chunkSentinel => "##",
    expectedReturn => [
{
    startingLine => 0,
    lineCount => 0,
    newStartingLine => 1,
    newLineCount => 4,
}
]
},
{   # New test
    testName => "New line count is 1",
    inputText => "## -0,0 +1 ##",
    chunkSentinel => "##",
    expectedReturn => [
{
    startingLine => 0,
    lineCount => 0,
    newStartingLine => 1,
    newLineCount => 1,
}
]
},
###
#  Chunk range followed by ending junk
##
{   # New test
    testName => "Line count is 0 and chunk range has ending junk",
    inputText => "@@ -0,0 +1,4 @@ foo()",
    expectedReturn => [
{
    startingLine => 0,
    lineCount => 0,
    newStartingLine => 1,
    newLineCount => 4,
}
]
},
{   # New test
    testName => "Line count is 1 and chunk range has ending junk",
    inputText => "@@ -1 +1,4 @@ foo()",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 1,
    newStartingLine => 1,
    newLineCount => 4,
}
]
},
{   # New test
    testName => "Both original and new line count is 1 and chunk range has ending junk",
    inputText => "@@ -1 +1 @@ foo()",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 1,
    newStartingLine => 1,
    newLineCount => 1,
}
]
},
{   # New test
    testName => "Line count and new line count > 1 and chunk range has ending junk",
    inputText => "@@ -1,2 +1,4 @@ foo()",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 2,
    newStartingLine => 1,
    newLineCount => 4,
}
]
},
{   # New test
    testName => "New line count is 0 and chunk range has ending junk",
    inputText => "@@ -1,4 +0,0 @@ foo()",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 4,
    newStartingLine => 0,
    newLineCount => 0,
}
]
},
{   # New test
    testName => "New line count is 1 and chunk range has ending junk",
    inputText => "@@ -1,4 +1 @@ foo()",
    expectedReturn => [
{
    startingLine => 1,
    lineCount => 4,
    newStartingLine => 1,
    newLineCount => 1,
}
]
},
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => $testCasesCount);

foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "parseChunkRange(): $testCase->{testName}: comparing";

    my @got = VCSUtils::parseChunkRange($testCase->{inputText}, $testCase->{chunkSentinel});
    my $expectedReturn = $testCase->{expectedReturn};

    is_deeply(\@got, $expectedReturn, "$testNameStart return value.");
}
