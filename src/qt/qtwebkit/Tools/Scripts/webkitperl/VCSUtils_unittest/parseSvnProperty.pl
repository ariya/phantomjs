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

# Unit tests of parseSvnProperty().

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
Added: svn:executable
   + *
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: delete svn:executable",
    inputText => <<'END',
Deleted: svn:executable
   - *
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => -1,
    value => "*",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: add svn:mergeinfo",
    inputText => <<'END',
Added: svn:mergeinfo
   Merged /trunk/Makefile:r33020
END
    expectedReturn => [
{
    name => "svn:mergeinfo",
    propertyChangeDelta => 1,
    value => "/trunk/Makefile:r33020",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: delete svn:mergeinfo",
    inputText => <<'END',
Deleted: svn:mergeinfo
   Reverse-merged /trunk/Makefile:r33020
END
    expectedReturn => [
{
    name => "svn:mergeinfo",
    propertyChangeDelta => -1,
    value => "/trunk/Makefile:r33020",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: modified svn:mergeinfo",
    inputText => <<'END',
Modified: svn:mergeinfo
   Reverse-merged /trunk/Makefile:r33020
   Merged /trunk/Makefile:r41697
END
    expectedReturn => [
{
    name => "svn:mergeinfo",
    propertyChangeDelta => 1,
    value => "/trunk/Makefile:r41697",
},
undef],
    expectedNextLine => undef,
},
####
# Using SVN 1.4 syntax
##
{
    # New test
    diffName => "simple: modified svn:mergeinfo using SVN 1.4 syntax",
    inputText => <<'END',
Name: svn:mergeinfo
   Reverse-merged /trunk/Makefile:r33020
   Merged /trunk/Makefile:r41697
END
    expectedReturn => [
{
    name => "svn:mergeinfo",
    propertyChangeDelta => 1,
    value => "/trunk/Makefile:r41697",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: delete svn:executable using SVN 1.4 syntax",
    inputText => <<'END',
Name: svn:executable
   - *
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => -1,
    value => "*",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: add svn:executable using SVN 1.4 syntax",
    inputText => <<'END',
Name: svn:executable
   + *
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
undef],
    expectedNextLine => undef,
},
####
# Using SVN 1.7 syntax
##
{
    # New test
    diffName => "simple: add svn:executable using SVN 1.7 syntax",
    inputText => <<'END',
Added: svn:executable
## -0,0 +1 ##
+*
\ No newline at end of property
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "simple: delete svn:executable using SVN 1.7 syntax",
    inputText => <<'END',
Deleted: svn:executable
## -1 +0,0 ##
-*
\ No newline at end of property
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => -1,
    value => "*",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "add svn:mime-type and add svn:executable using SVN 1.7 syntax",
    inputText => <<'END',
Added: svn:mime-type
## -0,0 +1 ##
+image/png
\ No newline at end of property
Added: svn:executable
## -0,0 +1 ##
+*
\ No newline at end of property
END
    expectedReturn => [
{
    name => "svn:mime-type",
    propertyChangeDelta => 1,
    value => "image/png",
},
"Added: svn:executable\n"],
    expectedNextLine => "## -0,0 +1 ##\n",
},
####
# Property value followed by empty line and start of next diff
##
{
    # New test
    diffName => "add svn:executable, followed by empty line and start of next diff",
    inputText => <<'END',
Added: svn:executable
   + *

Index: Makefile.shared
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
"\n"],
    expectedNextLine => "Index: Makefile.shared\n",
},
{
    # New test
    diffName => "add svn:executable, followed by empty line and start of next diff using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Added: svn:executable
   + *

Index: Makefile.shared
END
),
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
"\r\n"],
    expectedNextLine => "Index: Makefile.shared\r\n",
},
{
    # New test
    diffName => "add svn:executable, followed by empty line and start of next property diff",
    inputText => <<'END',
Added: svn:executable
   + *

Property changes on: Makefile.shared
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
"\n"],
    expectedNextLine => "Property changes on: Makefile.shared\n",
},
{
    # New test
    diffName => "add svn:executable, followed by empty line and start of next property diff using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Added: svn:executable
   + *

Property changes on: Makefile.shared
END
),
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
"\r\n"],
    expectedNextLine => "Property changes on: Makefile.shared\r\n",
},
{
    # New test
    diffName => "multi-line '+' change, followed by empty line and start of next diff",
    inputText => <<'END',
Name: documentation
   + A
long sentence that spans
multiple lines.

Index: Makefile.shared
END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A\nlong sentence that spans\nmultiple lines.",
},
"\n"],
    expectedNextLine => "Index: Makefile.shared\n",
},
{
    # New test
    diffName => "multi-line '+' change, followed by empty line and start of next diff using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Name: documentation
   + A
long sentence that spans
multiple lines.

Index: Makefile.shared
END
),
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A\r\nlong sentence that spans\r\nmultiple lines.",
},
"\r\n"],
    expectedNextLine => "Index: Makefile.shared\r\n",
},
{
    # New test
    diffName => "multi-line '+' change, followed by empty line and start of next property diff",
    inputText => <<'END',
Name: documentation
   + A
long sentence that spans
multiple lines.

Property changes on: Makefile.shared
END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A\nlong sentence that spans\nmultiple lines.",
},
"\n"],
    expectedNextLine => "Property changes on: Makefile.shared\n",
},
{
    # New test
    diffName => "multi-line '+' change, followed by empty line and start of next property diff using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Name: documentation
   + A
long sentence that spans
multiple lines.

Property changes on: Makefile.shared
END
),
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A\r\nlong sentence that spans\r\nmultiple lines.",
},
"\r\n"],
    expectedNextLine => "Property changes on: Makefile.shared\r\n",
},
####
# Property value followed by empty line and start of binary patch
##
{
    # New test
    diffName => "add svn:executable, followed by empty line and start of binary patch",
    inputText => <<'END',
Added: svn:executable
   + *

Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
"\n"],
    expectedNextLine => "Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==\n",
},
{
    # New test
    diffName => "add svn:executable, followed by empty line and start of binary patch using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Added: svn:executable
   + *

Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
),
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
"\r\n"],
    expectedNextLine => "Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==\r\n",
},
{
    # New test
    diffName => "multi-line '+' change, followed by empty line and start of binary patch",
    inputText => <<'END',
Name: documentation
   + A
long sentence that spans
multiple lines.

Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A\nlong sentence that spans\nmultiple lines.",
},
"\n"],
    expectedNextLine => "Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==\n",
},
{
    # New test
    diffName => "multi-line '+' change, followed by empty line and start of binary patch using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Name: documentation
   + A
long sentence that spans
multiple lines.

Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
),
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A\r\nlong sentence that spans\r\nmultiple lines.",
},
"\r\n"],
    expectedNextLine => "Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==\r\n",
},
{
    # New test
    diffName => "multi-line '-' change, followed by multi-line '+' change, empty line, and start of binary patch",
    inputText => <<'END',
Modified: documentation
   - A
long sentence that spans
multiple lines.
   + Another
long sentence that spans
multiple lines.

Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "Another\nlong sentence that spans\nmultiple lines.",
},
"\n"],
    expectedNextLine => "Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==\n",
},
{
    # New test
    diffName => "multi-line '-' change, followed by multi-line '+' change, empty line, and start of binary patch using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Modified: documentation
   - A
long sentence that spans
multiple lines.
   + Another
long sentence that spans
multiple lines.

Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
),
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "Another\r\nlong sentence that spans\r\nmultiple lines.",
},
"\r\n"],
    expectedNextLine => "Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==\r\n",
},
####
# Successive properties
##
{
    # New test
    diffName => "single-line '+' change followed by custom property with single-line '+' change",
    inputText => <<'END',
Added: svn:executable
   + *
Added: documentation
   + A sentence.
END
    expectedReturn => [
{
    name => "svn:executable",
    propertyChangeDelta => 1,
    value => "*",
},
"Added: documentation\n"],
    expectedNextLine => "   + A sentence.\n",
},
{
    # New test
    diffName => "multi-line '+' change, followed by svn:executable",
    inputText => <<'END',
Name: documentation
   + A
long sentence that spans
multiple lines.
Name: svn:executable
   + *
END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A\nlong sentence that spans\nmultiple lines.",
},
"Name: svn:executable\n"],
    expectedNextLine => "   + *\n",
},
{
    # New test
    diffName => "multi-line '-' change, followed by multi-line '+' change and add svn:executable",
    inputText => <<'END',
Modified: documentation
   - A
long sentence that spans
multiple lines.
   + Another
long sentence that spans
multiple lines.
Added: svn:executable
   + *
END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "Another\nlong sentence that spans\nmultiple lines.",
},
"Added: svn:executable\n"],
    expectedNextLine => "   + *\n",
},
{
    # New test
    diffName => "'Merged' change followed by 'Merged' change",
    inputText => <<'END',
Added: svn:mergeinfo
   Merged /trunk/Makefile:r33020
   Merged /trunk/Makefile.shared:r58350
END
    expectedReturn => [
{
    name => "svn:mergeinfo",
    propertyChangeDelta => 1,
    value => "/trunk/Makefile.shared:r58350",
},
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "'Reverse-merged' change followed by 'Reverse-merged' change",
    inputText => <<'END',
Deleted: svn:mergeinfo
   Reverse-merged /trunk/Makefile:r33020
   Reverse-merged /trunk/Makefile.shared:r58350
END
    expectedReturn => [
{
    name => "svn:mergeinfo",
    propertyChangeDelta => -1,
    value => "/trunk/Makefile.shared:r58350",
},
undef],
    expectedNextLine => undef,
},
####
# Property values with trailing new lines.
##
# FIXME: We do not support property values with trailing new lines, since it is difficult to
#        disambiguate them from the empty line that preceeds the contents of a binary patch as
#        in the test case (above): "multi-line '+' change, followed by empty line and start of binary patch".
{
    # New test
    diffName => "single-line '+' with trailing new line",
    inputText => <<'END',
Added: documentation
   + A sentence.

END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A sentence.",
},
"\n"],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "single-line '+' with trailing new line using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Added: documentation
   + A sentence.

END
),
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A sentence.",
},
"\r\n"],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "single-line '+' with trailing new line, followed by empty line and start of binary patch",
    inputText => <<'END',
Added: documentation
   + A sentence.


Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A sentence.",
},
"\n"],
    expectedNextLine => "\n",
},
{
    # New test
    diffName => "single-line '+' with trailing new line, followed by empty line and start of binary patch using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Added: documentation
   + A sentence.


Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
),
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => 1,
    value => "A sentence.",
},
"\r\n"],
    expectedNextLine => "\r\n",
},
{
    # New test
    diffName => "single-line '-' change with trailing new line, and single-line '+' change",
    inputText => <<'END',
Modified: documentation
   - A long sentence.

   + A sentence.
END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => -1, # Since we only interpret the '-' property.
    value => "A long sentence.",
},
"\n"],
    expectedNextLine => "   + A sentence.\n",
},
{
    # New test
    diffName => "single-line '-' change with trailing new line, and single-line '+' change using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Modified: documentation
   - A long sentence.

   + A sentence.
END
),
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => -1, # Since we only interpret the '-' property.
    value => "A long sentence.",
},
"\r\n"],
    expectedNextLine => "   + A sentence.\r\n",
},
{
    # New test
    diffName => "multi-line '-' change with trailing new line, and multi-line '+' change",
    inputText => <<'END',
Modified: documentation
   - A
long sentence that spans
multiple lines.

   + Another
long sentence that spans
multiple lines.
END
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => -1, # Since we only interpret the '-' property.
    value => "A\nlong sentence that spans\nmultiple lines.",
},
"\n"],
    expectedNextLine => "   + Another\n",
},
{
    # New test
    diffName => "multi-line '-' change with trailing new line, and multi-line '+' change using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Modified: documentation
   - A
long sentence that spans
multiple lines.

   + Another
long sentence that spans
multiple lines.
END
),
    expectedReturn => [
{
    name => "documentation",
    propertyChangeDelta => -1, # Since we only interpret the '-' property.
    value => "A\r\nlong sentence that spans\r\nmultiple lines.",
},
"\r\n"],
    expectedNextLine => "   + Another\r\n",
},
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => 2 * $testCasesCount); # Total number of assertions.

foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "parseSvnProperty(): $testCase->{diffName}: comparing";

    my $fileHandle;
    open($fileHandle, "<", \$testCase->{inputText});
    my $line = <$fileHandle>;

    my @got = VCSUtils::parseSvnProperty($fileHandle, $line);
    my $expectedReturn = $testCase->{expectedReturn};

    is_deeply(\@got, $expectedReturn, "$testNameStart return value.");

    my $gotNextLine = <$fileHandle>;
    is($gotNextLine, $testCase->{expectedNextLine},  "$testNameStart next read line.");
}
