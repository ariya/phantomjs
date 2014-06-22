#!/usr/bin/perl
#
# Copyright (C) 2009, 2010 Chris Jerdonek (chris.jerdonek@gmail.com)
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
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
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

# Unit tests of VCSUtils::runPatchCommand().

use Test::Simple tests => 4;
use VCSUtils;

# New test
$title = "runPatchCommand: Unsuccessful patch, forcing.";

# Since $patch has no "Index:" path, passing this to runPatchCommand
# should not affect any files.
my $patch = <<'END';
Garbage patch contents
END

# We call via callSilently() to avoid output like the following to STDERR:
# patch: **** Only garbage was found in the patch input.
$argsHashRef = {ensureForce => 1};
$exitStatus = callSilently(\&runPatchCommand, $patch, ".", "file_to_patch.txt", $argsHashRef);

ok($exitStatus != 0, $title);

# New test
$title = "runPatchCommand: New file, --dry-run.";

# This file should not exist after the tests, but we take care with the
# file name and contents just in case.
my $fileToPatch = "temp_OK_TO_ERASE__README_FOR_MORE.txt";
$patch = <<END;
Index: $fileToPatch
===================================================================
--- $fileToPatch	(revision 0)
+++ $fileToPatch	(revision 0)
@@ -0,0 +1,5 @@
+This is a test file for WebKitTools/Scripts/VCSUtils_unittest.pl.
+This file should not have gotten created on your system.
+If it did, some unit tests don't seem to be working quite right:
+It would be great if you could file a bug report. Thanks!
+---------------------------------------------------------------------
END

# --dry-run prevents creating any files.
# --silent suppresses the success message to STDOUT.
$argsHashRef = {options => ["--dry-run", "--silent"]};
$exitStatus = runPatchCommand($patch, ".", $fileToPatch, $argsHashRef);

ok($exitStatus == 0, $title);

# New test
$title = "runPatchCommand: New file: \"$fileToPatch\".";

$argsHashRef = {options => ["--silent"]};
$exitStatus = runPatchCommand($patch, ".", $fileToPatch, $argsHashRef);

ok($exitStatus == 0, $title);

# New test
$title = "runPatchCommand: Reverse new file (clean up previous).";

$argsHashRef = {shouldReverse => 1,
                options => ["--silent", "--remove-empty-files"]}; # To clean up.
$exitStatus = runPatchCommand($patch, ".", $fileToPatch, $argsHashRef);
ok($exitStatus == 0, $title);
