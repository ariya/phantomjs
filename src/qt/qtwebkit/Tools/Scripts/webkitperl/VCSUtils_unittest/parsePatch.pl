#!/usr/bin/perl -w
#
# Copyright (C) 2010 Chris Jerdonek (chris.jerdonek@gmail.com)
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

# Unit tests of parseDiffHeader().

use strict;
use warnings;

use Test::More;
use VCSUtils;

my @diffHashRefKeys = ( # The hash reference keys to check per diff.
    "copiedFromPath",
    "indexPath",
    "sourceRevision",
    "svnConvertedText",
);

# New test
my $testNameStart = "parsePatch(): [SVN: Rename] ";
my $patch = <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 53131)
+++ Makefile	(working copy)
@@ -1,1 +0,0 @@
-MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
Index: Makefile_new
===================================================================
--- Makefile_new	(revision 53131)	(from Makefile:53131)
+++ Makefile_new	(working copy)
@@ -0,0 +1,1 @@
+MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
END

my @expectedDiffHashRefs = (
{
    svnConvertedText => <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 53131)
+++ Makefile	(working copy)
@@ -1,1 +0,0 @@
-MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
END
    copiedFromPath => undef,
    indexPath => "Makefile",
    sourceRevision => "53131",
},
{
    copiedFromPath => "Makefile",
    indexPath => "Makefile_new",
    sourceRevision => "53131",
},
);

plan(tests => @expectedDiffHashRefs * @diffHashRefKeys);

my $fileHandle;
open($fileHandle, "<", \$patch);

my @gotDiffHashRefs = parsePatch($fileHandle);

my $i = 0;
foreach my $expectedDiffHashRef (@expectedDiffHashRefs) {

    my $gotDiffHashRef = $gotDiffHashRefs[$i++];

    foreach my $diffHashRefKey (@diffHashRefKeys) {
        my $testName = "${testNameStart}[diff $i] key=\"$diffHashRefKey\"";
        is($gotDiffHashRef->{$diffHashRefKey}, $expectedDiffHashRef->{$diffHashRefKey}, $testName);
    }
}
