#!/usr/bin/perl -w
#
# Copyright (C) 2010 Chris Jerdonek (cjerdonek@webkit.org)
# Copyright (C) 2010 Research In Motion Limited. All rights reserved.
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

# Unit tests for setChangeLogDateAndReviewer(fixChangeLogPatch()).

use strict;
use warnings;

use Test::More;
use VCSUtils;

my @testCaseHashRefs = (
{
    testName => "New entry inserted earlier in the file, but after an entry with the same author and date, patch applied a day later.",
    reviewer => "Sue",
    epochTime => 1273414321,
    patch => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -70,6 +70,14 @@
 
 2010-05-08  Alice  <alice@email.address>
 
+        Reviewed by NOBODY (OOPS!).
+
+        Changed some more code on 2010-05-08.
+
+        * File:
+
+2010-05-08  Alice  <alice@email.address>
+
         Reviewed by Ray.
 
         Changed some code on 2010-05-08.
END
    expectedReturn => <<'END',
--- ChangeLog
+++ ChangeLog
@@ -1,3 +1,11 @@
+2010-05-09  Alice  <alice@email.address>
+
+        Reviewed by Sue.
+
+        Changed some more code on 2010-05-08.
+
+        * File:
+
 2010-05-08  Alice  <alice@email.address>
 
         Reviewed by Ray.
END
},
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => 1 * $testCasesCount); # Total number of assertions.

foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "setChangeLogDateAndReviewer(fixChangeLogPatch()): $testCase->{testName}: comparing";

    my $patch = $testCase->{patch};
    my $reviewer = $testCase->{reviewer};
    my $epochTime = $testCase->{epochTime};

    my $fixedChangeLog = VCSUtils::fixChangeLogPatch($patch);
    my $got = VCSUtils::setChangeLogDateAndReviewer($fixedChangeLog->{patch}, $reviewer, $epochTime);
    my $expectedReturn = $testCase->{expectedReturn};

    is($got, $expectedReturn, "$testNameStart return value.");
}
