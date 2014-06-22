#!/usr/bin/perl -w
#
# Copyright (C) 2011 Research In Motion Limited. All rights reserved.
# Copyright (C) 2013 Apple Inc. All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# Unit tests of parseDiff() with mock files; test override of patch EOL with EOL of target file.

use strict;
use warnings;

use File::Temp;
use POSIX qw/getcwd/;
use Test::More;
use VCSUtils;

# We should consider moving escapeNewLineCharacters() and toMacLineEndings()
# to VCSUtils.pm if they're useful in other places.
sub escapeNewLineCharacters($)
{
    my ($text) = @_;
    my @characters = split(//, $text);
    my $result = "";
    foreach (@characters) {
        if (/^\r$/) {
            $result .= '\r';
            next;
        }
        if (/^\n$/) {
            $result .= '\n';
        }
        $result .= $_;
    }
    return $result;
}

sub toMacLineEndings($)
{
    my ($text) = @_;
    $text =~ s/\n/\r/g;
    return $text;
}

my $gitDiffHeaderForNewFile = <<EOF;
diff --git a/Makefile b/Makefile
new file mode 100644
index 0000000..756e864
--- /dev/null
+++ b/Makefile
@@ -0,0 +1,17 @@
EOF

my $gitDiffHeader = <<EOF;
diff --git a/Makefile b/Makefile
index 756e864..04d2ae1 100644
--- a/Makefile
+++ b/Makefile
@@ -1,3 +1,4 @@
EOF

my $svnConvertedGitDiffHeader = <<"EOF";
Index: Makefile
index 756e864..04d2ae1 100644
--- Makefile\t(revision 0)
+++ Makefile\t(working copy)
@@ -1,3 +1,4 @@
EOF

my $svnConvertedGitDiffHeaderForNewFile = <<"EOF";
Index: Makefile
new file mode 100644
index 0000000..756e864
--- Makefile\t(revision 0)
+++ Makefile\t(working copy)
@@ -0,0 +1,17 @@
EOF

my $svnDiffHeaderForNewFile = <<EOF;
Index: Makefile
===================================================================
--- Makefile	(revision 0)
+++ Makefile	(revision 0)
@@ -0,0 +1,17 @@
EOF

my $svnDiffHeader = <<EOF;
Index: Makefile
===================================================================
--- Makefile	(revision 53052)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
EOF

my $diffBody = <<EOF;
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
 
 all:
EOF

my $MakefileContents = <<EOF;
MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools 

all:
EOF

my $mockDir = File::Temp->tempdir("parseDiffXXXX", CLEANUP => 1);
writeToFile(File::Spec->catfile($mockDir, "MakefileWithUnixEOL"), $MakefileContents);
writeToFile(File::Spec->catfile($mockDir, "MakefileWithWindowsEOL"), toWindowsLineEndings($MakefileContents));
writeToFile(File::Spec->catfile($mockDir, "MakefileWithMacEOL"), toMacLineEndings($MakefileContents));

# The array of test cases.
my @testCaseHashRefs = (
###
# SVN test cases
##
{
    # New test
    diffName => "SVN: Patch with Unix line endings and IndexPath has Unix line endings",
    inputText => substituteString($svnDiffHeader, "Makefile", "MakefileWithUnixEOL") . $diffBody,
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnDiffHeader, "Makefile", "MakefileWithUnixEOL") . $diffBody, # Same as input text
    indexPath => "MakefileWithUnixEOL",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "53052",
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: Patch with Windows line endings and IndexPath has Unix line endings",
    inputText => substituteString($svnDiffHeader, "Makefile", "MakefileWithUnixEOL") . toWindowsLineEndings($diffBody),
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnDiffHeader, "Makefile", "MakefileWithUnixEOL") . $diffBody,
    indexPath => "MakefileWithUnixEOL",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "53052",
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: Patch with Windows line endings and IndexPath has Windows line endings",
    inputText => substituteString($svnDiffHeader, "Makefile", "MakefileWithWindowsEOL") . toWindowsLineEndings($diffBody),
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnDiffHeader, "Makefile", "MakefileWithWindowsEOL") . toWindowsLineEndings($diffBody), # Same as input text
    indexPath => "MakefileWithWindowsEOL",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "53052",
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: Patch adds Windows newline to EOF and IndexPath has Windows line endings",
    inputText => <<"EOF",
Index: MakefileWithWindowsEOL
===================================================================
--- MakefileWithWindowsEOL	(revision 53052)
+++ MakefileWithWindowsEOL	(working copy)
@@ -1,3 +1,4 @@\r
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools\r
 \r
-all:
\\ No newline at end of file
+all:\r
+\r
EOF
    expectedReturn => [
[{
    # Same as input text
    svnConvertedText => <<"EOF",
Index: MakefileWithWindowsEOL
===================================================================
--- MakefileWithWindowsEOL	(revision 53052)
+++ MakefileWithWindowsEOL	(working copy)
@@ -1,3 +1,4 @@\r
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools\r
 \r
-all:
\\ No newline at end of file
+all:\r
+\r
EOF
    indexPath => "MakefileWithWindowsEOL",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => 53052
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: Patch adds Mac newline to EOF and IndexPath has Mac line endings",
    inputText => <<"EOF",
Index: MakefileWithMacEOL
===================================================================
--- MakefileWithMacEOL	(revision 53052)
+++ MakefileWithMacEOL	(working copy)
@@ -1,3 +1,4 @@\r MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools\r \r-all:
\\ No newline at end of file
+all:\r+\r
EOF
    expectedReturn => [
[{
    # Same as input text
    svnConvertedText => q(Index: MakefileWithMacEOL
===================================================================
--- MakefileWithMacEOL	(revision 53052)
+++ MakefileWithMacEOL	(working copy)
@@ -1,3 +1,4 @@\r MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools\r \r-all:
\\ No newline at end of file
+all:\r+\r),
    indexPath => "MakefileWithMacEOL",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => 53052
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: Patch with Unix line endings and IndexPath has Windows line endings",
    inputText => substituteString($svnDiffHeader, "Makefile", "MakefileWithWindowsEOL") . $diffBody,
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnDiffHeader, "Makefile", "MakefileWithWindowsEOL") . toWindowsLineEndings($diffBody),
    indexPath => "MakefileWithWindowsEOL",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "53052",
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: Patch with Unix line endings and nonexistent IndexPath",
    inputText => substituteString($svnDiffHeaderForNewFile, "Makefile", "NonexistentFile") . $diffBody,
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnDiffHeaderForNewFile, "Makefile", "NonexistentFile") . $diffBody, # Same as input text
    indexPath => "NonexistentFile",
    isSvn => 1,
    isNew => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: Patch with Windows line endings and nonexistent IndexPath",
    inputText => substituteString($svnDiffHeaderForNewFile, "Makefile", "NonexistentFile") . toWindowsLineEndings($diffBody),
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnDiffHeaderForNewFile, "Makefile", "NonexistentFile") . toWindowsLineEndings($diffBody), # Same as input text
    indexPath => "NonexistentFile",
    isSvn => 1,
    isNew => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
###
# Git test cases
##
{
    # New test
    diffName => "Git: Patch with Unix line endings and IndexPath has Unix line endings",
    inputText => substituteString($gitDiffHeader, "Makefile", "MakefileWithUnixEOL") . $diffBody,
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnConvertedGitDiffHeader, "Makefile", "MakefileWithUnixEOL") . $diffBody, # Same as input text
    indexPath => "MakefileWithUnixEOL",
    isGit => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "Git: Patch with Windows line endings and IndexPath has Unix line endings",
    inputText => substituteString($gitDiffHeader, "Makefile", "MakefileWithUnixEOL") . toWindowsLineEndings($diffBody),
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnConvertedGitDiffHeader, "Makefile", "MakefileWithUnixEOL") . $diffBody,
    indexPath => "MakefileWithUnixEOL",
    isGit => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "Git: Patch with Windows line endings and IndexPath has Windows line endings",
    inputText => substituteString($gitDiffHeader, "Makefile", "MakefileWithWindowsEOL") . toWindowsLineEndings($diffBody),
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnConvertedGitDiffHeader, "Makefile", "MakefileWithWindowsEOL") . toWindowsLineEndings($diffBody), # Same as input text
    indexPath => "MakefileWithWindowsEOL",
    isGit => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "Git: Patch adds newline to EOF with Windows line endings and IndexPath has Windows line endings",
    inputText => <<"EOF",
diff --git a/MakefileWithWindowsEOL b/MakefileWithWindowsEOL
index e7e8475..ae16fc3 100644
--- a/MakefileWithWindowsEOL
+++ b/MakefileWithWindowsEOL
@@ -1,3 +1,4 @@\r
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools\r
 \r
-all:
\\ No newline at end of file
+all:\r
+\r
EOF
    expectedReturn => [
[{
    # Same as input text
    svnConvertedText => <<"EOF",
Index: MakefileWithWindowsEOL
index e7e8475..ae16fc3 100644
--- MakefileWithWindowsEOL\t(revision 0)
+++ MakefileWithWindowsEOL\t(working copy)
@@ -1,3 +1,4 @@\r
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools\r
 \r
-all:
\\ No newline at end of file
+all:\r
+\r
EOF
    indexPath => "MakefileWithWindowsEOL",
    isGit => 1,
    numTextChunks => 1
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "Git: Patch adds Mac newline to EOF and IndexPath has Mac line endings",
    inputText => <<"EOF",
diff --git a/MakefileWithMacEOL b/MakefileWithMacEOL
index e7e8475..ae16fc3 100644
--- a/MakefileWithMacEOL
+++ b/MakefileWithMacEOL
@@ -1,3 +1,4 @@\r MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools\r \r-all:
\\ No newline at end of file
+all:\r+\r
EOF
    expectedReturn => [
[{
    # Same as input text
    svnConvertedText => qq(Index: MakefileWithMacEOL
index e7e8475..ae16fc3 100644
--- MakefileWithMacEOL\t(revision 0)
+++ MakefileWithMacEOL\t(working copy)
@@ -1,3 +1,4 @@\r MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools\r \r-all:
\\ No newline at end of file
+all:\r+\r),
    indexPath => "MakefileWithMacEOL",
    isGit => 1,
    numTextChunks => 1
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "Git: Patch with Unix line endings and IndexPath has Windows line endings",
    inputText => substituteString($gitDiffHeader, "Makefile", "MakefileWithWindowsEOL") . $diffBody,
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnConvertedGitDiffHeader, "Makefile", "MakefileWithWindowsEOL") . toWindowsLineEndings($diffBody),
    indexPath => "MakefileWithWindowsEOL",
    isGit => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "Git: Patch with Unix line endings and nonexistent IndexPath",
    inputText => substituteString($gitDiffHeaderForNewFile, "Makefile", "NonexistentFile") . $diffBody,
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnConvertedGitDiffHeaderForNewFile, "Makefile", "NonexistentFile") . $diffBody, # Same as input text
    indexPath => "NonexistentFile",
    isGit => 1,
    isNew => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "Git: Patch with Windows line endings and nonexistent IndexPath",
    inputText => substituteString($gitDiffHeaderForNewFile, "Makefile", "NonexistentFile") . toWindowsLineEndings($diffBody),
    expectedReturn => [
[{
    svnConvertedText => substituteString($svnConvertedGitDiffHeaderForNewFile, "Makefile", "NonexistentFile") . toWindowsLineEndings($diffBody), # Same as input text
    indexPath => "NonexistentFile",
    isGit => 1,
    isNew => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => 2 * $testCasesCount); # Total number of assertions.

my $savedCWD = getcwd();
chdir($mockDir) or die;
foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "parseDiff(): $testCase->{diffName}: comparing";

    my $fileHandle;
    open($fileHandle, "<", \$testCase->{inputText});
    my $line = <$fileHandle>;

    my @got = VCSUtils::parseDiff($fileHandle, $line);
    my $expectedReturn = $testCase->{expectedReturn};

    $got[0][0]->{svnConvertedText} = escapeNewLineCharacters($got[0][0]->{svnConvertedText});
    $expectedReturn->[0][0]->{svnConvertedText} = escapeNewLineCharacters($expectedReturn->[0][0]->{svnConvertedText});
    is_deeply(\@got, $expectedReturn, "$testNameStart return value.");

    my $gotNextLine = <$fileHandle>;
    is($gotNextLine, $testCase->{expectedNextLine},  "$testNameStart next read line.");
}
chdir($savedCWD);

sub substituteString
{
    my ($string, $searchString, $replacementString) = @_;
    $string =~ s/$searchString/$replacementString/g;
    return $string;
}

sub writeToFile
{
    my ($file, $text) = @_;
    open(FILE, ">$file") or die;
    print FILE $text;
    close(FILE);
}
