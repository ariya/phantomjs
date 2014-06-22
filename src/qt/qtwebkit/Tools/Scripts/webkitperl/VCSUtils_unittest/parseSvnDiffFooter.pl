#!/usr/bin/perl -w
#
# Copyright (C) Research in Motion Limited 2010. All Rights Reserved.
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

# Unit tests of parseSvnDiffProperties().

use strict;
use warnings;

use Test::More;
use VCSUtils;

my @testCaseHashRefs = (
####
# Simple test cases
##
{
    # New test
    diffName => "simple: add svn:executable",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: svn:executable
   + *
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: add svn:mergeinfo",
    inputText => <<'END',
Property changes on: Makefile
___________________________________________________________________
Added: svn:mergeinfo
   Merged /trunk/Makefile:r33020
END
    expectedReturn => [
{
    propertyPath => "Makefile",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: delete svn:mergeinfo",
    inputText => <<'END',
Property changes on: Makefile
___________________________________________________________________
Deleted: svn:mergeinfo
   Reverse-merged /trunk/Makefile:r33020
END
    expectedReturn => [
{
    propertyPath => "Makefile",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: modified svn:mergeinfo",
    inputText => <<'END',
Property changes on: Makefile
___________________________________________________________________
Modified: svn:mergeinfo
   Reverse-merged /trunk/Makefile:r33020
   Merged /trunk/Makefile:r41697
END
    expectedReturn => [
{
    propertyPath => "Makefile",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: delete svn:executable",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Deleted: svn:executable
   - *
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => -1,
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: delete svn:executable using SVN 1.4 syntax",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Name: svn:executable
   - *
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => -1,
},
undef],
    expectedNextLine => undef,
},
####
# Property value followed by empty line and start of next diff
##
{
    # New test
    diffName => "add svn:executable, followed by empty line and start of next diff",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: svn:executable
   + *

Index: Makefile.shared
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
"\n"],
    expectedNextLine => "Index: Makefile.shared\n",
},
{
    # New test
    diffName => "add svn:executable, followed by empty line and start of next property diff",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: svn:executable
   + *

Property changes on: Makefile.shared
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
"\n"],
    expectedNextLine => "Property changes on: Makefile.shared\n",
},
####
# Property value followed by empty line and start of the binary contents
##
{
    # New test
    diffName => "add svn:executable, followed by empty line and start of binary contents",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: svn:executable
   + *

Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
"\n"],
    expectedNextLine => "Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==\n",
},
{
    # New test
    diffName => "custom property followed by svn:executable, empty line and start of binary contents",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: documentation
   + This is an example sentence.
Added: svn:executable
   + *

Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
"\n"],
    expectedNextLine => "Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==\n",
},
####
# Successive properties
##
{
    # New test
    diffName => "svn:executable followed by custom property",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: svn:executable
   + *
Added: documentation
   + This is an example sentence.
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "svn:executable followed by custom property using SVN 1.7 syntax",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: svn:executable
## -0,0 +1 ##
+*
\ No newline at end of property
Added: documentation
## -0,0 +1 ##
+This is an example sentence.
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "svn:executable followed by custom property without newline using SVN 1.7 syntax",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: svn:executable
## -0,0 +1 ##
+*
\ No newline at end of property
Added: documentation
## -0,0 +1 ##
+This is an example sentence.
\ No newline at end of property
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "custom property followed by svn:executable",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: documentation
   + This is an example sentence.
Added: svn:executable
   + *
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
undef],
    expectedNextLine => undef,
},
####
# Successive properties followed by empty line and start of next diff
##
{
    # New test
    diffName => "custom property followed by svn:executable, empty line and start of next property diff",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: documentation
   + This is an example sentence.
Added: svn:executable
   + *

Property changes on: Makefile.shared
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
"\n"],
    expectedNextLine => "Property changes on: Makefile.shared\n",
},
{
    # New test
    diffName => "custom property followed by svn:executable, empty line and start of next index diff",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: documentation
   + This is an example sentence.
Added: svn:executable
   + *

Index: Makefile.shared
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => 1,
},
"\n"],
    expectedNextLine => "Index: Makefile.shared\n",
},
####
# Custom properties
##
# FIXME: We do not support anything other than the svn:executable property.
#        We should add support for handling other properties.
{
    # New test
    diffName => "simple: custom property",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Name: documentation
   + This is an example sentence.
END
    expectedReturn => [
{
    propertyPath => "FileA",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "custom property followed by custom property",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: copyright
   + Copyright (C) Research in Motion Limited 2010. All Rights Reserved.
Added: documentation
   + This is an example sentence.
END
    expectedReturn => [
{
    propertyPath => "FileA",
},
undef],
    expectedNextLine => undef,
},
####
# Malformed property diffs
##
# We shouldn't encounter such diffs in practice.
{
    # New test
    diffName => "svn:executable followed by custom property and svn:executable",
    inputText => <<'END',
Property changes on: FileA
___________________________________________________________________
Added: svn:executable
   + *
Added: documentation
   + This is an example sentence.
Deleted: svn:executable
   - *
END
    expectedReturn => [
{
    propertyPath => "FileA",
    executableBitDelta => -1,
},
undef],
    expectedNextLine => undef,
},
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => 2 * $testCasesCount); # Total number of assertions.

foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "parseSvnDiffProperties(): $testCase->{diffName}: comparing";

    my $fileHandle;
    open($fileHandle, "<", \$testCase->{inputText});
    my $line = <$fileHandle>;

    my @got = VCSUtils::parseSvnDiffProperties($fileHandle, $line);
    my $expectedReturn = $testCase->{expectedReturn};

    is_deeply(\@got, $expectedReturn, "$testNameStart return value.");

    my $gotNextLine = <$fileHandle>;
    is($gotNextLine, $testCase->{expectedNextLine},  "$testNameStart next read line.");
}
