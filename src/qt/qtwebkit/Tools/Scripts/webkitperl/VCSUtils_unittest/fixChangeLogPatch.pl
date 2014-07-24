#!/usr/bin/perl
#
# Copyright (C) 2009, 2010 Chris Jerdonek (chris.jerdonek@gmail.com)
# Copyright (C) Research In Motion 2010. All rights reserved.
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

# Unit tests of VCSUtils::fixChangeLogPatch().

use strict;
use warnings;

use Test::More;
use VCSUtils;

# The source ChangeLog for these tests is the following:
# 
# 2009-12-22  Alice  <alice@email.address>
# 
#         Reviewed by Ray.
# 
#         Changed some code on 2009-12-22.
# 
#         * File:
#         * File2:
# 
# 2009-12-21  Alice  <alice@email.address>
# 
#         Reviewed by Ray.
# 
#         Changed some code on 2009-12-21.
# 
#         * File:
#         * File2:

my @testCaseHashRefs = (
{ # New test
    diffName => "fixChangeLogPatch: [no change] In-place change.",
    inputText => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,5 +1,5 @@
 2010-12-22  Bob  <bob@email.address>
 
-        Reviewed by Sue.
+        Reviewed by Ray.
 
         Changed some code on 2010-12-22.
END
    expectedReturn => {
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,5 +1,5 @@
 2010-12-22  Bob  <bob@email.address>
 
-        Reviewed by Sue.
+        Reviewed by Ray.
 
         Changed some code on 2010-12-22.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: [no change] Remove first entry.",
    inputText => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,11 +1,3 @@
-2010-12-22  Bob  <bob@email.address>
-
-        Reviewed by Ray.
-
-        Changed some code on 2010-12-22.
-
-        * File:
-
 2010-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    expectedReturn => {
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,11 +1,3 @@
-2010-12-22  Bob  <bob@email.address>
-
-        Reviewed by Ray.
-
-        Changed some code on 2010-12-22.
-
-        * File:
-
 2010-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: [no change] Remove entry in the middle.",
    inputText => <<'END',
--- ChangeLog
+++ ChangeLog
@@@ -7,10 +7,6 @@
 
         * File:
 
-2010-12-22  Bob  <bob@email.address>
-
-        Changed some code on 2010-12-22.
-
 2010-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    expectedReturn => {
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@@ -7,10 +7,6 @@
 
         * File:
 
-2010-12-22  Bob  <bob@email.address>
-
-        Changed some code on 2010-12-22.
-
 2010-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: [no change] Far apart changes (i.e. more than one chunk).",
    inputText => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -7,7 +7,7 @@
 
         * File:
 
-2010-12-22  Bob  <bob@email.address>
+2010-12-22  Bobby <bob@email.address>
 
         Changed some code on 2010-12-22.
 
@@ -21,7 +21,7 @@
 
         * File2:
 
-2010-12-21  Bob  <bob@email.address>
+2010-12-21  Bobby <bob@email.address>
 
         Changed some code on 2010-12-21.
END
    expectedReturn => {
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -7,7 +7,7 @@
 
         * File:
 
-2010-12-22  Bob  <bob@email.address>
+2010-12-22  Bobby <bob@email.address>
 
         Changed some code on 2010-12-22.
 
@@ -21,7 +21,7 @@
 
         * File2:
 
-2010-12-21  Bob  <bob@email.address>
+2010-12-21  Bobby <bob@email.address>
 
         Changed some code on 2010-12-21.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: [no change] First line is new line.",
    inputText => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,3 +1,11 @@
+2009-12-22  Bob  <bob@email.address>
+
+        Reviewed by Ray.
+
+        Changed some more code on 2009-12-22.
+
+        * File:
+
 2009-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    expectedReturn => {
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,3 +1,11 @@
+2009-12-22  Bob  <bob@email.address>
+
+        Reviewed by Ray.
+
+        Changed some more code on 2009-12-22.
+
+        * File:
+
 2009-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: [no change] No date string.",
    inputText => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -6,6 +6,7 @@
 
         * File:
         * File2:
+        * File3:
 
 2009-12-21  Alice  <alice@email.address>
 
END
    expectedReturn => {
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -6,6 +6,7 @@
 
         * File:
         * File2:
+        * File3:
 
 2009-12-21  Alice  <alice@email.address>
 
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: New entry inserted in middle.",
    inputText => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -11,6 +11,14 @@
 
         Reviewed by Ray.
 
+        Changed some more code on 2009-12-21.
+
+        * File:
+
+2009-12-21  Alice  <alice@email.address>
+
+        Reviewed by Ray.
+
         Changed some code on 2009-12-21.
 
         * File:
END
    expectedReturn => {
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,3 +1,11 @@
+2009-12-21  Alice  <alice@email.address>
+
+        Reviewed by Ray.
+
+        Changed some more code on 2009-12-21.
+
+        * File:
+
 2009-12-21  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: New entry inserted earlier in the file, but after an entry with the same author and date.",
    inputText => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -70,6 +70,14 @@
 
 2009-12-22  Alice  <alice@email.address>
 
+        Reviewed by Sue.
+
+        Changed some more code on 2009-12-22.
+
+        * File:
+
+2009-12-22  Alice  <alice@email.address>
+
         Reviewed by Ray.
 
         Changed some code on 2009-12-22.
END
    expectedReturn => {
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,3 +1,11 @@
+2009-12-22  Alice  <alice@email.address>
+
+        Reviewed by Sue.
+
+        Changed some more code on 2009-12-22.
+
+        * File:
+
 2009-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: Leading context includes first line.",
    inputText => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,5 +1,13 @@
 2009-12-22  Alice  <alice@email.address>
 
+        Reviewed by Sue.
+
+        Changed some more code on 2009-12-22.
+
+        * File:
+
+2009-12-22  Alice  <alice@email.address>
+
         Reviewed by Ray.
 
         Changed some code on 2009-12-22.
END
    expectedReturn => {
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,3 +1,11 @@
+2009-12-22  Alice  <alice@email.address>
+
+        Reviewed by Sue.
+
+        Changed some more code on 2009-12-22.
+
+        * File:
+
 2009-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: Leading context does not include first line.",
    inputText => <<'END',
@@ -2,6 +2,14 @@
 
         Reviewed by Ray.
 
+        Changed some more code on 2009-12-22.
+
+        * File:
+
+2009-12-22  Alice  <alice@email.address>
+
+        Reviewed by Ray.
+
         Changed some code on 2009-12-22.
 
         * File:
END
    expectedReturn => {
    patch => <<'END',
@@ -1,3 +1,11 @@
+2009-12-22  Alice  <alice@email.address>
+
+        Reviewed by Ray.
+
+        Changed some more code on 2009-12-22.
+
+        * File:
+
 2009-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: Non-consecutive line additions.",

# This can occur, for example, if the new ChangeLog entry includes
# trailing white space in the first blank line but not the second.
# A diff command can then match the second blank line of the new
# ChangeLog entry with the first blank line of the old.
# The svn diff command with the default --diff-cmd has done this.
    inputText => <<'END',
@@ -1,5 +1,11 @@
 2009-12-22  Alice  <alice@email.address>
+ <pretend-whitespace>
+        Reviewed by Ray.
 
+        Changed some more code on 2009-12-22.
+
+2009-12-22  Alice  <alice@email.address>
+
         Reviewed by Ray.
 
         Changed some code on 2009-12-22.
END
    expectedReturn => {
    patch => <<'END',
@@ -1,3 +1,9 @@
+2009-12-22  Alice  <alice@email.address>
+ <pretend-whitespace>
+        Reviewed by Ray.
+
+        Changed some more code on 2009-12-22.
+
 2009-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
    }
},
{ # New test
    diffName => "fixChangeLogPatch: Additional edits after new entry.",
    inputText => <<'END',
@@ -2,10 +2,17 @@
 
         Reviewed by Ray.
 
+        Changed some more code on 2009-12-22.
+
+        * File:
+
+2009-12-22  Alice  <alice@email.address>
+
+        Reviewed by Ray.
+
         Changed some code on 2009-12-22.
 
         * File:
-        * File2:
 
 2009-12-21  Alice  <alice@email.address>
 
END
    expectedReturn => {
    patch => <<'END',
@@ -1,11 +1,18 @@
+2009-12-22  Alice  <alice@email.address>
+
+        Reviewed by Ray.
+
+        Changed some more code on 2009-12-22.
+
+        * File:
+
 2009-12-22  Alice  <alice@email.address>
 
         Reviewed by Ray.
 
         Changed some code on 2009-12-22.
 
         * File:
-        * File2:
 
 2009-12-21  Alice  <alice@email.address>
 
END
    }
},
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => $testCasesCount); # Total number of assertions.

foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "fixChangeLogPatch(): $testCase->{diffName}: comparing";

    my $got = VCSUtils::fixChangeLogPatch($testCase->{inputText});
    my $expectedReturn = $testCase->{expectedReturn};
 
    is_deeply($got, $expectedReturn, "$testNameStart return value.");
}
