#!/usr/bin/perl
#
# Copyright (C) 2010 Apple Inc. All rights reserved.
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

# Unit tests of VCSUtils::mergeChangeLogs().

use strict;

use Test::Simple tests => 16;
use File::Temp qw(tempfile);
use VCSUtils;

# Read contents of a file and return it.
sub readFile($)
{
    my ($fileName) = @_;

    local $/;
    open(FH, "<", $fileName);
    my $content = <FH>;
    close(FH);

    return $content;
}

# Write a temporary file and return the filename.
sub writeTempFile($$$)
{
    my ($name, $extension, $content) = @_;

    my ($FH, $fileName) = tempfile(
        $name . "-XXXXXXXX",
        DIR => ($ENV{'TMPDIR'} || $ENV{'TEMP'} || "/tmp"),
        UNLINK => 0,
    );
    print $FH $content;
    close $FH;

    if ($extension) {
        my $newFileName = $fileName . $extension;
        rename($fileName, $newFileName);
        $fileName = $newFileName;
    }

    return $fileName;
}

# --------------------------------------------------------------------------------

{
    # New test
    my $title = "mergeChangeLogs: traditional rejected patch success";

    my $fileNewerContent = <<'EOF';
2010-01-29  Mark Rowe  <mrowe@apple.com>

        Fix the Mac build.

        Disable ENABLE_INDEXED_DATABASE since it is "completely non-functional".

2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>

        Rubber-stamped by Maciej Stachowiak.

        Fix the ARM build.
EOF
    my $fileNewer = writeTempFile("file", "", $fileNewerContent);

    my $fileMineContent = <<'EOF';
***************
*** 1,3 ****
  2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>
  
          Rubber-stamped by Maciej Stachowiak.
--- 1,9 ----
+ 2010-01-29  Oliver Hunt  <oliver@apple.com>
+ 
+         Reviewed by Darin Adler.
+ 
+         JSC is failing to propagate anonymous slot count on some transitions
+ 
  2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>
  
          Rubber-stamped by Maciej Stachowiak.
EOF
    my $fileMine = writeTempFile("file", ".rej", $fileMineContent);
    rename($fileMine, $fileNewer . ".rej");
    $fileMine = $fileNewer . ".rej";

    my $fileOlderContent = $fileNewerContent;
    my $fileOlder = writeTempFile("file", ".orig", $fileOlderContent);
    rename($fileOlder, $fileNewer . ".orig");
    $fileOlder = $fileNewer . ".orig";

    my $exitStatus = mergeChangeLogs($fileMine, $fileOlder, $fileNewer);

    # mergeChangeLogs() should return 1 since the patch succeeded.
    ok($exitStatus == 1, "$title: should return 1 for success");

    ok(readFile($fileMine) eq $fileMineContent, "$title: \$fileMine should be unchanged");
    ok(readFile($fileOlder) eq $fileOlderContent, "$title: \$fileOlder should be unchanged");

    my $expectedContent = <<'EOF';
2010-01-29  Oliver Hunt  <oliver@apple.com>

        Reviewed by Darin Adler.

        JSC is failing to propagate anonymous slot count on some transitions

EOF
    $expectedContent .= $fileNewerContent;
    ok(readFile($fileNewer) eq $expectedContent, "$title: \$fileNewer should be updated to include patch");

    unlink($fileMine, $fileOlder, $fileNewer);
}

# --------------------------------------------------------------------------------

{
    # New test
    my $title = "mergeChangeLogs: traditional rejected patch failure";

    my $fileNewerContent = <<'EOF';
2010-01-29  Mark Rowe  <mrowe@apple.com>

        Fix the Mac build.

        Disable ENABLE_INDEXED_DATABASE since it is "completely non-functional".

2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>

        Rubber-stamped by Maciej Stachowiak.

        Fix the ARM build.
EOF
    my $fileNewer = writeTempFile("file", "", $fileNewerContent);

    my $fileMineContent = <<'EOF';
***************
*** 1,9 ****
- 2010-01-29  Oliver Hunt  <oliver@apple.com>
- 
-         Reviewed by Darin Adler.
- 
-         JSC is failing to propagate anonymous slot count on some transitions
- 
  2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>
  
          Rubber-stamped by Maciej Stachowiak.
--- 1,3 ----
  2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>
  
          Rubber-stamped by Maciej Stachowiak.
EOF
    my $fileMine = writeTempFile("file", ".rej", $fileMineContent);
    rename($fileMine, $fileNewer . ".rej");
    $fileMine = $fileNewer . ".rej";

    my $fileOlderContent = $fileNewerContent;
    my $fileOlder = writeTempFile("file", ".orig", $fileOlderContent);
    rename($fileOlder, $fileNewer . ".orig");
    $fileOlder = $fileNewer . ".orig";

    my $exitStatus = mergeChangeLogs($fileMine, $fileOlder, $fileNewer);

    # mergeChangeLogs() should return 0 since the patch failed.
    ok($exitStatus == 0, "$title: should return 0 for failure");

    ok(readFile($fileMine) eq $fileMineContent, "$title: \$fileMine should be unchanged");
    ok(readFile($fileOlder) eq $fileOlderContent, "$title: \$fileOlder should be unchanged");
    ok(readFile($fileNewer) eq $fileNewerContent, "$title: \$fileNewer should be unchanged");

    unlink($fileMine, $fileOlder, $fileNewer);
}

# --------------------------------------------------------------------------------

{
    # New test
    my $title = "mergeChangeLogs: patch succeeds";

    my $fileMineContent = <<'EOF';
2010-01-29  Oliver Hunt  <oliver@apple.com>

        Reviewed by Darin Adler.

        JSC is failing to propagate anonymous slot count on some transitions

2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>

        Rubber-stamped by Maciej Stachowiak.

        Fix the ARM build.
EOF
    my $fileMine = writeTempFile("fileMine", "", $fileMineContent);

    my $fileOlderContent = <<'EOF';
2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>

        Rubber-stamped by Maciej Stachowiak.

        Fix the ARM build.
EOF
    my $fileOlder = writeTempFile("fileOlder", "", $fileOlderContent);

    my $fileNewerContent = <<'EOF';
2010-01-29  Mark Rowe  <mrowe@apple.com>

        Fix the Mac build.

        Disable ENABLE_INDEXED_DATABASE since it is "completely non-functional".

2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>

        Rubber-stamped by Maciej Stachowiak.

        Fix the ARM build.
EOF
    my $fileNewer = writeTempFile("fileNewer", "", $fileNewerContent);

    my $exitStatus = mergeChangeLogs($fileMine, $fileOlder, $fileNewer);

    # mergeChangeLogs() should return 1 since the patch succeeded.
    ok($exitStatus == 1, "$title: should return 1 for success");

    ok(readFile($fileMine) eq $fileMineContent, "$title: \$fileMine should be unchanged");
    ok(readFile($fileOlder) eq $fileOlderContent, "$title: \$fileOlder should be unchanged");

    my $expectedContent = <<'EOF';
2010-01-29  Oliver Hunt  <oliver@apple.com>

        Reviewed by Darin Adler.

        JSC is failing to propagate anonymous slot count on some transitions

EOF
    $expectedContent .= $fileNewerContent;

    ok(readFile($fileNewer) eq $expectedContent, "$title: \$fileNewer should be patched");

    unlink($fileMine, $fileOlder, $fileNewer);
}

# --------------------------------------------------------------------------------

{
    # New test
    my $title = "mergeChangeLogs: patch fails";

    my $fileMineContent = <<'EOF';
2010-01-29  Mark Rowe  <mrowe@apple.com>

        Fix the Mac build.

        Disable ENABLE_INDEXED_DATABASE since it is "completely non-functional".

2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>

        Rubber-stamped by Maciej Stachowiak.

        Fix the ARM build.
EOF
    my $fileMine = writeTempFile("fileMine", "", $fileMineContent);

    my $fileOlderContent = <<'EOF';
2010-01-29  Mark Rowe  <mrowe@apple.com>

        Fix the Mac build.

        Disable ENABLE_INDEXED_DATABASE since it is "completely non-functional".

2010-01-29  Oliver Hunt  <oliver@apple.com>

        Reviewed by Darin Adler.

        JSC is failing to propagate anonymous slot count on some transitions

2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>

        Rubber-stamped by Maciej Stachowiak.

        Fix the ARM build.
EOF
    my $fileOlder = writeTempFile("fileOlder", "", $fileOlderContent);

    my $fileNewerContent = <<'EOF';
2010-01-29  Oliver Hunt  <oliver@apple.com>

        Reviewed by Darin Adler.

        JSC is failing to propagate anonymous slot count on some transitions

2010-01-29  Simon Hausmann  <simon.hausmann@nokia.com>

        Rubber-stamped by Maciej Stachowiak.

        Fix the ARM build.
EOF
    my $fileNewer = writeTempFile("fileNewer", "", $fileNewerContent);

    my $exitStatus = mergeChangeLogs($fileMine, $fileOlder, $fileNewer);

    # mergeChangeLogs() should return a non-zero exit status since the patch failed.
    ok($exitStatus == 0, "$title: return non-zero exit status for failure");

    ok(readFile($fileMine) eq $fileMineContent, "$title: \$fileMine should be unchanged");
    ok(readFile($fileOlder) eq $fileOlderContent, "$title: \$fileOlder should be unchanged");

    # $fileNewer should still exist unchanged because the patch failed
    ok(readFile($fileNewer) eq $fileNewerContent, "$title: \$fileNewer should be unchanged");

    unlink($fileMine, $fileOlder, $fileNewer);
}

# --------------------------------------------------------------------------------

