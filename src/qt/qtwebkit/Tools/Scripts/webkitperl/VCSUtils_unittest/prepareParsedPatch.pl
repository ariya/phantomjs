#!/usr/bin/perl -w
#
# Copyright (C) 2010 Chris Jerdonek (cjerdonek@webkit.org)
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

# Unit tests of prepareParsedPatch().

use strict;
use warnings;

use Test::More;
use VCSUtils;

my $diffHashRef1 = { # not a copy, no source revision
    copiedFromPath => undef,
    indexPath => "indexPath1",
    sourceRevision => undef,
    svnConvertedText => "diff1",
};
my $diffHashRef2 = { # not a copy, has source revision
    copiedFromPath => undef,
    indexPath => "indexPath2",
    sourceRevision => 20,
    svnConvertedText => "diff2",
};
my $diffHashRef3 = { # a copy (copies always have source revision)
    copiedFromPath => "sourcePath3",
    indexPath => "indexPath2", # Deliberately choosing same as $diffHashRef2
    sourceRevision => 3,
    svnConvertedText => "diff3",
};

my @testCases = (
{
    # New test
    testName => "zero diffs: empty array",
    diffHashRefsInput => [],
    expected => {
        copyDiffHashRefs => [],
        nonCopyDiffHashRefs => [],
        sourceRevisionHash => {},
    },
},
{
    # New test
    testName => "one diff: non-copy, no revision",
    diffHashRefsInput => [$diffHashRef1],
    expected => {
        copyDiffHashRefs => [],
        nonCopyDiffHashRefs => [$diffHashRef1],
        sourceRevisionHash => {},
    },
},
{
    # New test
    testName => "one diff: non-copy, has revision",
    diffHashRefsInput => [$diffHashRef2],
    expected => {
        copyDiffHashRefs => [],
        nonCopyDiffHashRefs => [$diffHashRef2],
        sourceRevisionHash => {
            "indexPath2" => 20,
        }
    },
},
{
    # New test
    testName => "one diff: copy (has revision)",
    diffHashRefsInput => [$diffHashRef3],
    expected => {
        copyDiffHashRefs => [$diffHashRef3],
        nonCopyDiffHashRefs => [],
        sourceRevisionHash => {
            "sourcePath3" => 3,
        }
    },
},
{
    # New test
    testName => "two diffs: two non-copies",
    diffHashRefsInput => [$diffHashRef1, $diffHashRef2],
    expected => {
        copyDiffHashRefs => [],
        nonCopyDiffHashRefs => [$diffHashRef1, $diffHashRef2],
        sourceRevisionHash => {
            "indexPath2" => 20,
        }
    },
},
{
    # New test
    testName => "two diffs: non-copy and copy",
    diffHashRefsInput => [$diffHashRef2, $diffHashRef3],
    expected => {
        copyDiffHashRefs => [$diffHashRef3],
        nonCopyDiffHashRefs => [$diffHashRef2],
        sourceRevisionHash => {
            "sourcePath3" => 3,
            "indexPath2" => 20,
        }
    },
},
);

my $testCasesCount = @testCases;
plan(tests => $testCasesCount);

foreach my $testCase (@testCases) {
    my $testName = $testCase->{testName};
    my @diffHashRefs = @{$testCase->{diffHashRefsInput}};
    my $expected = $testCase->{expected};

    my $got = prepareParsedPatch(0, @diffHashRefs);

    is_deeply($got, $expected, $testName);
}

