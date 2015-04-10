#!/usr/bin/perl -w
#
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

# Unit tests of parseSvnDiffHeader().

use strict;
use warnings;

use Test::More;
use VCSUtils;

# The array of test cases.
my @testCaseHashRefs = (
{
    # New test
    diffName => "simple diff",
    inputText => <<'END',
Index: WebKitTools/Scripts/VCSUtils.pm
===================================================================
--- WebKitTools/Scripts/VCSUtils.pm	(revision 53004)
+++ WebKitTools/Scripts/VCSUtils.pm	(working copy)
@@ -32,6 +32,7 @@ use strict;
 use warnings;
END
    expectedReturn => [
{
    svnConvertedText => <<'END',
Index: WebKitTools/Scripts/VCSUtils.pm
===================================================================
--- WebKitTools/Scripts/VCSUtils.pm	(revision 53004)
+++ WebKitTools/Scripts/VCSUtils.pm	(working copy)
END
    indexPath => "WebKitTools/Scripts/VCSUtils.pm",
    sourceRevision => "53004",
},
"@@ -32,6 +32,7 @@ use strict;\n"],
    expectedNextLine => " use warnings;\n",
},
{
    # New test
    diffName => "new file",
    inputText => <<'END',
Index: WebKitTools/Scripts/webkitperl/VCSUtils_unittest/parseDiffHeader.pl
===================================================================
--- WebKitTools/Scripts/webkitperl/VCSUtils_unittest/parseDiffHeader.pl	(revision 0)
+++ WebKitTools/Scripts/webkitperl/VCSUtils_unittest/parseDiffHeader.pl	(revision 0)
@@ -0,0 +1,262 @@
+#!/usr/bin/perl -w
END
    expectedReturn => [
{
    svnConvertedText => <<'END',
Index: WebKitTools/Scripts/webkitperl/VCSUtils_unittest/parseDiffHeader.pl
===================================================================
--- WebKitTools/Scripts/webkitperl/VCSUtils_unittest/parseDiffHeader.pl	(revision 0)
+++ WebKitTools/Scripts/webkitperl/VCSUtils_unittest/parseDiffHeader.pl	(revision 0)
END
    indexPath => "WebKitTools/Scripts/webkitperl/VCSUtils_unittest/parseDiffHeader.pl",
    isNew => 1,
},
"@@ -0,0 +1,262 @@\n"],
    expectedNextLine => "+#!/usr/bin/perl -w\n",
},
{
    # New test
    diffName => "new file with spaces in its name",
    inputText => <<'END',
Index: WebKit.xcworkspace/xcshareddata/xcschemes/All Source (target WebProcess).xcscheme
===================================================================
--- WebKit.xcworkspace/xcshareddata/xcschemes/All Source (target WebProcess).xcscheme	(revision 0)
+++ WebKit.xcworkspace/xcshareddata/xcschemes/All Source (target WebProcess).xcscheme	(revision 0)
@@ -0,0 +1,8 @@
+<?xml version="1.0" encoding="UTF-8"?>
END
    expectedReturn => [
{
    svnConvertedText => <<'END',
Index: WebKit.xcworkspace/xcshareddata/xcschemes/All Source (target WebProcess).xcscheme
===================================================================
--- WebKit.xcworkspace/xcshareddata/xcschemes/All Source (target WebProcess).xcscheme	(revision 0)
+++ WebKit.xcworkspace/xcshareddata/xcschemes/All Source (target WebProcess).xcscheme	(revision 0)
END
    indexPath => "WebKit.xcworkspace/xcshareddata/xcschemes/All Source (target WebProcess).xcscheme",
    isNew => 1,
},
"@@ -0,0 +1,8 @@\n"],
    expectedNextLine => "+<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n",
},
{
    # New test
    diffName => "copied file",
    inputText => <<'END',
Index: index_path.py
===================================================================
--- index_path.py	(revision 53048)	(from copied_from_path.py:53048)
+++ index_path.py	(working copy)
@@ -0,0 +1,7 @@
+# Python file...
END
    expectedReturn => [
{
    svnConvertedText => <<'END',
Index: index_path.py
===================================================================
--- index_path.py	(revision 53048)	(from copied_from_path.py:53048)
+++ index_path.py	(working copy)
END
    copiedFromPath => "copied_from_path.py",
    indexPath => "index_path.py",
    sourceRevision => 53048,
},
"@@ -0,0 +1,7 @@\n"],
    expectedNextLine => "+# Python file...\n",
},
{
    # New test
    diffName => "contains \\r\\n lines",
    inputText => <<END, # No single quotes to allow interpolation of "\r"
Index: index_path.py\r
===================================================================\r
--- index_path.py	(revision 53048)\r
+++ index_path.py	(working copy)\r
@@ -0,0 +1,7 @@\r
+# Python file...\r
END
    expectedReturn => [
{
    svnConvertedText => <<END, # No single quotes to allow interpolation of "\r"
Index: index_path.py\r
===================================================================\r
--- index_path.py	(revision 53048)\r
+++ index_path.py	(working copy)\r
END
    indexPath => "index_path.py",
    sourceRevision => 53048,
},
"@@ -0,0 +1,7 @@\r\n"],
    expectedNextLine => "+# Python file...\r\n",
},
{
    # New test
    diffName => "contains path corrections",
    inputText => <<'END',
Index: index_path.py
===================================================================
--- bad_path	(revision 53048)	(from copied_from_path.py:53048)
+++ bad_path	(working copy)
@@ -0,0 +1,7 @@
+# Python file...
END
    expectedReturn => [
{
    svnConvertedText => <<'END',
Index: index_path.py
===================================================================
--- index_path.py	(revision 53048)	(from copied_from_path.py:53048)
+++ index_path.py	(working copy)
END
    copiedFromPath => "copied_from_path.py",
    indexPath => "index_path.py",
    sourceRevision => 53048,
},
"@@ -0,0 +1,7 @@\n"],
    expectedNextLine => "+# Python file...\n",
},
####
#    Binary test cases
##
{
    # New test
    diffName => "binary file",
    inputText => <<'END',
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream

Property changes on: test_file.swf
___________________________________________________________________
Name: svn:mime-type
   + application/octet-stream


Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    expectedReturn => [
{
    svnConvertedText => <<'END',
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream

END
    indexPath => "test_file.swf",
    isBinary => 1,
},
"Property changes on: test_file.swf\n"],
    expectedNextLine => "___________________________________________________________________\n",
},
{
    # New test
    diffName => "binary file using SVN 1.7 syntax",
    inputText => <<'END',
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream
Index: test_file.swf
===================================================================
--- test_file.swf
+++ test_file.swf

Property changes on: test_file.swf
___________________________________________________________________
Added: svn:mime-type
## -0,0 +1 ##
+application/octet-stream
\ No newline at end of property

Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    expectedReturn => [
{
    svnConvertedText => <<'END',
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream
Index: test_file.swf
===================================================================
--- test_file.swf
+++ test_file.swf
END
    indexPath => "test_file.swf",
    isBinary => 1,
},
"\n"],
    expectedNextLine => "Property changes on: test_file.swf\n",
},
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => 2 * $testCasesCount); # Total number of assertions.

foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "parseSvnDiffHeader(): $testCase->{diffName}: comparing";

    my $fileHandle;
    open($fileHandle, "<", \$testCase->{inputText});
    my $line = <$fileHandle>;

    my @got = VCSUtils::parseSvnDiffHeader($fileHandle, $line);
    my $expectedReturn = $testCase->{expectedReturn};

    is_deeply(\@got, $expectedReturn, "$testNameStart return value.");

    my $gotNextLine = <$fileHandle>;
    is($gotNextLine, $testCase->{expectedNextLine},  "$testNameStart next read line.");
}
