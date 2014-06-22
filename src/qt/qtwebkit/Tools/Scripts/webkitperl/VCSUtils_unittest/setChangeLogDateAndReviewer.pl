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

# Unit tests of setChangeLogDateAndReviewer().

use strict;
use warnings;

use Test::More;
use VCSUtils;

my @testCaseHashRefs = (
{
    testName => "reviewer defined and \"NOBODY (OOPS!)\" in leading junk",
    reviewer => "John Doe",
    epochTime => 1273414321,
    patch => <<'END',
Subject: [PATCH]

Reviewed by NOBODY (OOPS!).

diff --git a/WebCore/ChangeLog b/WebCore/ChangeLog
--- a/WebCore/ChangeLog
+++ b/WebCore/ChangeLog
@@ -1,3 +1,15 @@
+2010-05-08  Chris Jerdonek  <cjerdonek@webkit.org>
+
+        Reviewed by NOBODY (OOPS!).
+
 2010-05-08  Chris Jerdonek  <cjerdonek@webkit.org>
 
         Reviewed by Jane Doe.
END
    expectedReturn => <<'END',
Subject: [PATCH]

Reviewed by NOBODY (OOPS!).

diff --git a/WebCore/ChangeLog b/WebCore/ChangeLog
--- a/WebCore/ChangeLog
+++ b/WebCore/ChangeLog
@@ -1,3 +1,15 @@
+2010-05-09  Chris Jerdonek  <cjerdonek@webkit.org>
+
+        Reviewed by John Doe.
+
 2010-05-08  Chris Jerdonek  <cjerdonek@webkit.org>
 
         Reviewed by Jane Doe.
END
},
{
    testName => "reviewer not defined and \"NOBODY (OOPS!)\" in leading junk",
    reviewer => undef,
    epochTime => 1273414321,
    patch => <<'END',
Subject: [PATCH]

Reviewed by NOBODY (OOPS!).

diff --git a/WebCore/ChangeLog b/WebCore/ChangeLog
--- a/WebCore/ChangeLog
+++ b/WebCore/ChangeLog
@@ -1,3 +1,15 @@
+2010-05-08  Chris Jerdonek  <cjerdonek@webkit.org>
+
+        Reviewed by NOBODY (OOPS!).
+
 2010-05-08  Chris Jerdonek  <cjerdonek@webkit.org>
 
         Reviewed by Jane Doe.
END
    expectedReturn => <<'END',
Subject: [PATCH]

Reviewed by NOBODY (OOPS!).

diff --git a/WebCore/ChangeLog b/WebCore/ChangeLog
--- a/WebCore/ChangeLog
+++ b/WebCore/ChangeLog
@@ -1,3 +1,15 @@
+2010-05-09  Chris Jerdonek  <cjerdonek@webkit.org>
+
+        Reviewed by NOBODY (OOPS!).
+
 2010-05-08  Chris Jerdonek  <cjerdonek@webkit.org>
 
         Reviewed by Jane Doe.
END
},
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => 1 * $testCasesCount); # Total number of assertions.

foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "setChangeLogDateAndReviewer(): $testCase->{testName}: comparing";

    my $patch = $testCase->{patch};
    my $reviewer = $testCase->{reviewer};
    my $epochTime = $testCase->{epochTime};

    my $got = VCSUtils::setChangeLogDateAndReviewer($patch, $reviewer, $epochTime);
    my $expectedReturn = $testCase->{expectedReturn};

    is($got, $expectedReturn, "$testNameStart return value.");
}
