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

# Unit tests of parseDiff().

use strict;
use warnings;

use Test::More;
use VCSUtils;

# The array of test cases.
my @testCaseHashRefs = (
{
    # New test
    diffName => "SVN: simple",
    inputText => <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 53052)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
 
 all:
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: Makefile
===================================================================
--- Makefile	(revision 53052)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
 
 all:
END
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "53052",
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: binary file (isBinary true)",
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
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream



Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    indexPath => "test_file.swf",
    isBinary => 1,
    isSvn => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: binary file (isBinary true) using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
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
),
    expectedReturn => [
[{
    svnConvertedText =>  toWindowsLineEndings(<<'END', # Same as input text
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream



Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
),
    indexPath => "test_file.swf",
    isBinary => 1,
    isSvn => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: leading junk",
    inputText => <<'END',

LEADING JUNK

Index: Makefile
===================================================================
--- Makefile	(revision 53052)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
 
 all:
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text

LEADING JUNK

Index: Makefile
===================================================================
--- Makefile	(revision 53052)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
 
 all:
END
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "53052",
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: copied file",
    inputText => <<'END',
Index: Makefile_new
===================================================================
--- Makefile_new	(revision 53131)	(from Makefile:53131)
+++ Makefile_new	(working copy)
@@ -0,0 +1,1 @@
+MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
END
    expectedReturn => [
[{
    copiedFromPath => "Makefile",
    indexPath => "Makefile_new",
    sourceRevision => "53131",
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: two diffs",
    inputText => <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 53131)
+++ Makefile	(working copy)
@@ -1,1 +0,0 @@
-MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
Index: Makefile_new
===================================================================
--- Makefile_new	(revision 53131)	(from Makefile:53131)
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 53131)
+++ Makefile	(working copy)
@@ -1,1 +0,0 @@
-MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
END
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "53131",
}],
"Index: Makefile_new\n"],
    expectedNextLine => "===================================================================\n",
},
{
    # New test
    diffName => "SVN: SVN diff followed by Git diff", # Should not recognize Git start
    inputText => <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 53131)
+++ Makefile	(working copy)
@@ -1,1 +0,0 @@
-MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
diff --git a/Makefile b/Makefile
index f5d5e74..3b6aa92 100644
--- a/Makefile
+++ b/Makefile
@@ -1,1 1,1 @@ public:
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: Makefile
===================================================================
--- Makefile	(revision 53131)
+++ Makefile	(working copy)
@@ -1,1 +0,0 @@
-MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKitTools
diff --git a/Makefile b/Makefile
index f5d5e74..3b6aa92 100644
--- a/Makefile
+++ b/Makefile
@@ -1,1 1,1 @@ public:
END
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "53131",
}],
undef],
    expectedNextLine => undef,
},
####
# Property Changes: Simple
##
{
    # New test
    diffName => "SVN: file change diff with property change diff",
    inputText => <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:

Property changes on: Makefile
___________________________________________________________________
Name: svn:executable
   + *
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:

END
    executableBitDelta => 1,
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "60021",
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: file change diff, followed by property change diff on different file",
    inputText => <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:

Property changes on: Makefile.shared
___________________________________________________________________
Name: svn:executable
   + *
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:

END
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "60021",
}],
"Property changes on: Makefile.shared\n"],
    expectedNextLine => "___________________________________________________________________\n",
},
{
    # New test
    diffName => "SVN: property diff, followed by file change diff",
    inputText => <<'END',
Property changes on: Makefile
___________________________________________________________________
Deleted: svn:executable
   - *

Index: Makefile.shared
===================================================================
--- Makefile.shared	(revision 60021)
+++ Makefile.shared	(working copy)
@@ -1,3 +1,4 @@
+
SCRIPTS_PATH ?= ../WebKitTools/Scripts
XCODE_OPTIONS = `perl -I$(SCRIPTS_PATH) -Mwebkitdirs -e 'print XcodeOptionString()'` $(ARGS)
END
    expectedReturn => [
[{
    executableBitDelta => -1,
    indexPath => "Makefile",
    isSvn => 1,
}],
"Index: Makefile.shared\n"],
    expectedNextLine => "===================================================================\n",
},
{
    # New test
    diffName => "SVN: property diff, followed by file change diff using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Property changes on: Makefile
___________________________________________________________________
Deleted: svn:executable
   - *

Index: Makefile.shared
===================================================================
--- Makefile.shared	(revision 60021)
+++ Makefile.shared	(working copy)
@@ -1,3 +1,4 @@
+
SCRIPTS_PATH ?= ../WebKitTools/Scripts
XCODE_OPTIONS = `perl -I$(SCRIPTS_PATH) -Mwebkitdirs -e 'print XcodeOptionString()'` $(ARGS)
END
),
    expectedReturn => [
[{
    executableBitDelta => -1,
    indexPath => "Makefile",
    isSvn => 1,
}],
"Index: Makefile.shared\r\n"],
    expectedNextLine => "===================================================================\r\n",
},
{
    # New test
    diffName => "SVN: copied file with property change",
    inputText => <<'END',
Index: NMakefile
===================================================================
--- NMakefile	(revision 60021)	(from Makefile:60021)
+++ NMakefile	(working copy)
@@ -0,0 +1,1 @@
+MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

Property changes on: NMakefile
___________________________________________________________________
Added: svn:executable
   + *
END
    expectedReturn => [
[{
    copiedFromPath => "Makefile",
    executableBitDelta => 1,
    indexPath => "NMakefile",
    sourceRevision => "60021",
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: two consecutive property diffs",
    inputText => <<'END',
Property changes on: Makefile
___________________________________________________________________
Added: svn:executable
   + *


Property changes on: Makefile.shared
___________________________________________________________________
Added: svn:executable
   + *
END
    expectedReturn => [
[{
    executableBitDelta => 1,
    indexPath => "Makefile",
    isSvn => 1,
}],
"Property changes on: Makefile.shared\n"],
    expectedNextLine => "___________________________________________________________________\n",
},
{
    # New test
    diffName => "SVN: two consecutive property diffs using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Property changes on: Makefile
___________________________________________________________________
Added: svn:executable
   + *


Property changes on: Makefile.shared
___________________________________________________________________
Added: svn:executable
   + *
END
),
    expectedReturn => [
[{
    executableBitDelta => 1,
    indexPath => "Makefile",
    isSvn => 1,
}],
"Property changes on: Makefile.shared\r\n"],
    expectedNextLine => "___________________________________________________________________\r\n",
},
####
# Property Changes: Binary files
##
{
    # New test
    diffName => "SVN: binary file with executable bit change",
    inputText => <<'END',
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream

Property changes on: test_file.swf
___________________________________________________________________
Name: svn:mime-type
   + application/octet-stream
Name: svn:executable
   + *


Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream



Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
    executableBitDelta => 1,
    indexPath => "test_file.swf",
    isBinary => 1,
    isSvn => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: binary file with executable bit change usng Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream

Property changes on: test_file.swf
___________________________________________________________________
Name: svn:mime-type
   + application/octet-stream
Name: svn:executable
   + *


Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
),
    expectedReturn => [
[{
    svnConvertedText =>  toWindowsLineEndings(<<'END', # Same as input text
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream



Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
END
),
    executableBitDelta => 1,
    indexPath => "test_file.swf",
    isBinary => 1,
    isSvn => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "SVN: binary file followed by property change on different file",
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

Property changes on: Makefile
___________________________________________________________________
Added: svn:executable
   + *
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream



Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==

END
    indexPath => "test_file.swf",
    isBinary => 1,
    isSvn => 1,
}],
"Property changes on: Makefile\n"],
    expectedNextLine => "___________________________________________________________________\n",
},
{
    # New test
    diffName => "SVN: binary file followed by property change on different file using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream

Property changes on: test_file.swf
___________________________________________________________________
Name: svn:mime-type
   + application/octet-stream


Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==

Property changes on: Makefile
___________________________________________________________________
Added: svn:executable
   + *
END
),
    expectedReturn => [
[{
    svnConvertedText =>  toWindowsLineEndings(<<'END', # Same as input text
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream



Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==

END
),
    indexPath => "test_file.swf",
    isBinary => 1,
    isSvn => 1,
}],
"Property changes on: Makefile\r\n"],
    expectedNextLine => "___________________________________________________________________\r\n",
},
{
    # New test
    diffName => "SVN: binary file followed by file change on different file",
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

Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream



Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==

END
    indexPath => "test_file.swf",
    isBinary => 1,
    isSvn => 1,
}],
"Index: Makefile\n"],
    expectedNextLine => "===================================================================\n",
},
{
    # New test
    diffName => "SVN: binary file followed by file change on different file using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream

Property changes on: test_file.swf
___________________________________________________________________
Name: svn:mime-type
   + application/octet-stream


Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==

Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:
END
),
    expectedReturn => [
[{
    svnConvertedText =>  toWindowsLineEndings(<<'END', # Same as input text
Index: test_file.swf
===================================================================
Cannot display: file marked as a binary type.
svn:mime-type = application/octet-stream



Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==

END
),
    indexPath => "test_file.swf",
    isBinary => 1,
    isSvn => 1,
}],
"Index: Makefile\r\n"],
    expectedNextLine => "===================================================================\r\n",
},
####
# Property Changes: File change with property change
##
{
    # New test
    diffName => "SVN: file change diff with property change, followed by property change diff",
    inputText => <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:

Property changes on: Makefile
___________________________________________________________________
Added: svn:executable
   + *


Property changes on: Makefile.shared
___________________________________________________________________
Deleted: svn:executable
   - *
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:



END
    executableBitDelta => 1,
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "60021",
}],
"Property changes on: Makefile.shared\n"],
    expectedNextLine => "___________________________________________________________________\n",
},
{
    # New test
    diffName => "SVN: file change diff with property change, followed by property change diff using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:

Property changes on: Makefile
___________________________________________________________________
Added: svn:executable
   + *


Property changes on: Makefile.shared
___________________________________________________________________
Deleted: svn:executable
   - *
END
),
    expectedReturn => [
[{
    svnConvertedText =>  toWindowsLineEndings(<<'END', # Same as input text
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:



END
),
    executableBitDelta => 1,
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "60021",
}],
"Property changes on: Makefile.shared\r\n"],
    expectedNextLine => "___________________________________________________________________\r\n",
},
{
    # New test
    diffName => "SVN: file change diff with property change, followed by file change diff",
    inputText => <<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:

Property changes on: Makefile
___________________________________________________________________
Name: svn:executable
   - *

Index: Makefile.shared
===================================================================
--- Makefile.shared	(revision 60021)
+++ Makefile.shared	(working copy)
@@ -1,3 +1,4 @@
+
SCRIPTS_PATH ?= ../WebKitTools/Scripts
XCODE_OPTIONS = `perl -I$(SCRIPTS_PATH) -Mwebkitdirs -e 'print XcodeOptionString()'` $(ARGS)
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END', # Same as input text
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:


END
    executableBitDelta => -1,
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "60021",
}],
"Index: Makefile.shared\n"],
    expectedNextLine => "===================================================================\n",
},
{
    # New test
    diffName => "SVN: file change diff with property change, followed by file change diff using Windows line endings",
    inputText => toWindowsLineEndings(<<'END',
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:

Property changes on: Makefile
___________________________________________________________________
Name: svn:executable
   - *

Index: Makefile.shared
===================================================================
--- Makefile.shared	(revision 60021)
+++ Makefile.shared	(working copy)
@@ -1,3 +1,4 @@
+
SCRIPTS_PATH ?= ../WebKitTools/Scripts
XCODE_OPTIONS = `perl -I$(SCRIPTS_PATH) -Mwebkitdirs -e 'print XcodeOptionString()'` $(ARGS)
END
),
    expectedReturn => [
[{
    svnConvertedText =>  toWindowsLineEndings(<<'END', # Same as input text
Index: Makefile
===================================================================
--- Makefile	(revision 60021)
+++ Makefile	(working copy)
@@ -1,3 +1,4 @@
+
 MODULES = JavaScriptCore JavaScriptGlue WebCore WebKit WebKit2 WebKitTools 

 all:


END
),
    executableBitDelta => -1,
    indexPath => "Makefile",
    isSvn => 1,
    numTextChunks => 1,
    sourceRevision => "60021",
}],
"Index: Makefile.shared\r\n"],
    expectedNextLine => "===================================================================\r\n",
},
####
#    Git test cases
##
{
    # New test
    diffName => "Git: simple",
    inputText => <<'END',
diff --git a/Makefile b/Makefile
index f5d5e74..3b6aa92 100644
--- a/Makefile
+++ b/Makefile
@@ -1,1 +1,1 @@ public:
END
    expectedReturn => [
[{
    svnConvertedText =>  <<"END",
Index: Makefile
index f5d5e74..3b6aa92 100644
--- Makefile\t(revision 0)
+++ Makefile\t(working copy)
@@ -1,1 +1,1 @@ public:
END
    indexPath => "Makefile",
    isGit => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "Git: Append new line to the end of an existing file",
    inputText => <<'END',
diff --git a/foo b/foo
index 863339f..db418b2 100644
--- a/foo
+++ b/foo
@@ -1 +1,2 @@
 Passed
+
END
    expectedReturn => [
[{
    svnConvertedText =>  <<"END",
Index: foo
index 863339f..db418b2 100644
--- foo\t(revision 0)
+++ foo\t(working copy)
@@ -1 +1,2 @@
 Passed
+
END
    indexPath => "foo",
    isGit => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
{   # New test
    diffName => "Git: new file",
    inputText => <<'END',
diff --git a/foo.h b/foo.h
new file mode 100644
index 0000000..3c9f114
--- /dev/null
+++ b/foo.h
@@ -0,0 +1,34 @@
+<html>
diff --git a/bar b/bar
index d45dd40..3494526 100644
END
    expectedReturn => [
[{
    svnConvertedText => <<"END",
Index: foo.h
new file mode 100644
index 0000000..3c9f114
--- foo.h\t(revision 0)
+++ foo.h\t(working copy)
@@ -0,0 +1,34 @@
+<html>
END
    indexPath => "foo.h",
    isGit => 1,
    isNew => 1,
    numTextChunks => 1,
}],
"diff --git a/bar b/bar\n"],
    expectedNextLine => "index d45dd40..3494526 100644\n",
},
{   # New test
    diffName => "Git: file deletion",
    inputText => <<'END',
diff --git a/foo b/foo
deleted file mode 100644
index 1e50d1d..0000000
--- a/foo
+++ /dev/null
@@ -1,1 +0,0 @@
-line1
diff --git a/bar b/bar
index d45dd40..3494526 100644
END
    expectedReturn => [
[{
    svnConvertedText => <<"END",
Index: foo
deleted file mode 100644
index 1e50d1d..0000000
--- foo\t(revision 0)
+++ foo\t(working copy)
@@ -1,1 +0,0 @@
-line1
END
    indexPath => "foo",
    isDeletion => 1,
    isGit => 1,
    numTextChunks => 1,
}],
"diff --git a/bar b/bar\n"],
    expectedNextLine => "index d45dd40..3494526 100644\n",
},
{
    # New test
    diffName => "Git: Git diff followed by SVN diff", # Should not recognize SVN start
    inputText => <<'END',
diff --git a/Makefile b/Makefile
index f5d5e74..3b6aa92 100644
--- a/Makefile
+++ b/Makefile
@@ -1,1 +1,1 @@ public:
Index: Makefile_new
===================================================================
--- Makefile_new	(revision 53131)	(from Makefile:53131)
END
    expectedReturn => [
[{
    svnConvertedText =>  <<"END",
Index: Makefile
index f5d5e74..3b6aa92 100644
--- Makefile\t(revision 0)
+++ Makefile\t(working copy)
@@ -1,1 +1,1 @@ public:
Index: Makefile_new
===================================================================
--- Makefile_new	(revision 53131)	(from Makefile:53131)
END
    indexPath => "Makefile",
    isGit => 1,
    numTextChunks => 1,
}],
undef],
    expectedNextLine => undef,
},
{
    # New test
    diffName => "Git: file that only has an executable bit change",
    inputText => <<'END',
diff --git a/foo b/foo
old mode 100644
new mode 100755
END
    expectedReturn => [
[{
    svnConvertedText =>  <<'END',
Index: foo
old mode 100644
new mode 100755
END
    executableBitDelta => 1,
    indexPath => "foo",
    isGit => 1,
    numTextChunks => 0,
}],
undef],
    expectedNextLine => undef,
},
####
#    Git test cases: file moves (multiple return values)
##
{
    diffName => "Git: rename (with similarity index 100%)",
    inputText => <<'END',
diff --git a/foo b/foo_new
similarity index 100%
rename from foo
rename to foo_new
diff --git a/bar b/bar
index d45dd40..3494526 100644
END
    expectedReturn => [
[{
    indexPath => "foo",
    isDeletion => 1,
},
{
    copiedFromPath => "foo",
    indexPath => "foo_new",
}],
"diff --git a/bar b/bar\n"],
    expectedNextLine => "index d45dd40..3494526 100644\n",
},
{
    diffName => "rename (with similarity index < 100%)",
    inputText => <<'END',
diff --git a/foo b/foo_new
similarity index 99%
rename from foo
rename to foo_new
index 1e50d1d..1459d21 100644
--- a/foo
+++ b/foo_new
@@ -15,3 +15,4 @@ release r deployment dep deploy:
 line1
 line2
 line3
+line4
diff --git a/bar b/bar
index d45dd40..3494526 100644
END
    expectedReturn => [
[{
    indexPath => "foo",
    isDeletion => 1,
},
{
    copiedFromPath => "foo",
    indexPath => "foo_new",
},
{
    indexPath => "foo_new",
    isGit => 1,
    numTextChunks => 1,
    svnConvertedText => <<"END",
Index: foo_new
similarity index 99%
rename from foo
rename to foo_new
index 1e50d1d..1459d21 100644
--- foo_new\t(revision 0)
+++ foo_new\t(working copy)
@@ -15,3 +15,4 @@ release r deployment dep deploy:
 line1
 line2
 line3
+line4
END
}],
"diff --git a/bar b/bar\n"],
    expectedNextLine => "index d45dd40..3494526 100644\n",
},
{
    diffName => "rename (with executable bit change)",
    inputText => <<'END',
diff --git a/foo b/foo_new
old mode 100644
new mode 100755
similarity index 100%
rename from foo
rename to foo_new
diff --git a/bar b/bar
index d45dd40..3494526 100644
END
    expectedReturn => [
[{
    indexPath => "foo",
    isDeletion => 1,
},
{
    copiedFromPath => "foo",
    indexPath => "foo_new",
},
{
    executableBitDelta => 1,
    indexPath => "foo_new",
    isGit => 1,
    numTextChunks => 0,
    svnConvertedText => <<'END',
Index: foo_new
old mode 100644
new mode 100755
similarity index 100%
rename from foo
rename to foo_new
END
}],
"diff --git a/bar b/bar\n"],
    expectedNextLine => "index d45dd40..3494526 100644\n",
},
);

my $testCasesCount = @testCaseHashRefs;
plan(tests => 2 * $testCasesCount); # Total number of assertions.

foreach my $testCase (@testCaseHashRefs) {
    my $testNameStart = "parseDiff(): $testCase->{diffName}: comparing";

    my $fileHandle;
    open($fileHandle, "<", \$testCase->{inputText});
    my $line = <$fileHandle>;

    my @got = VCSUtils::parseDiff($fileHandle, $line, {"shouldNotUseIndexPathEOL" => 1});
    my $expectedReturn = $testCase->{expectedReturn};

    is_deeply(\@got, $expectedReturn, "$testNameStart return value.");

    my $gotNextLine = <$fileHandle>;
    is($gotNextLine, $testCase->{expectedNextLine},  "$testNameStart next read line.");
}
