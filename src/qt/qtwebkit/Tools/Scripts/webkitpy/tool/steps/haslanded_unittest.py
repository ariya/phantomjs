# Copyright (C) 2009 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

import unittest2 as unittest
import subprocess

from webkitpy.tool.steps.haslanded import HasLanded


class HasLandedTest(unittest.TestCase):
    maxDiff = None

    @unittest.skipUnless(subprocess.call('which interdiff', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) == 0, "requires interdiff")
    def test_run(self):
        # These patches require trailing whitespace to remain valid patches.
        diff1 = """\
Index: a.py
===================================================================
--- a.py
+++ a.py
@@ -1,3 +1,5 @@
 A
 B
 C
+D
+E
Index: b.py
===================================================================
--- b.py      2013-01-21 15:20:59.693887185 +1100
+++ b.py      2013-01-21 15:22:24.382555711 +1100
@@ -1,3 +1,5 @@
 1
 2
 3
+4
+5
"""

        diff1_add_line = """\
Index: a.py
===================================================================
--- a.py
+++ a.py
@@ -1,3 +1,6 @@
 A
 B
 C
+D
+E
+F
Index: b.py
===================================================================
--- b.py
+++ b.py
@@ -1,3 +1,5 @@
 1
 2
 3
+4
+5
"""

        diff1_remove_line = """\
Index: a.py
===================================================================
--- a.py
+++ a.py
@@ -1,3 +1,4 @@
 A
 B
 C
+D
Index: b.py
===================================================================
--- b.py
+++ b.py
@@ -1,3 +1,5 @@
 1
 2
 3
+4
+5
"""

        diff1_add_file = diff1 + """\
Index: c.py
===================================================================
--- c.py
+++ c.py
@@ -1,3 +1,5 @@
 1
 2
 3
+4
+5
"""

        diff1_remove_file = """\
Index: a.py
===================================================================
--- a.py
+++ a.py
@@ -1,3 +1,5 @@
 A
 B
 C
+D
+E
"""
        self.assertMultiLineEqual(
            HasLanded.diff_diff(diff1, diff1_add_line, '', 'add-line'),
            """\
diff -u a.py a.py
--- a.py
+++ a.py
@@ -5,0 +6 @@
+F
""")

        self.assertMultiLineEqual(
            HasLanded.diff_diff(diff1, diff1_remove_line, '', 'remove-line'),
            """\
diff -u a.py a.py
--- a.py
+++ a.py
@@ -5 +4,0 @@
-E
""")
        self.assertMultiLineEqual(
            HasLanded.diff_diff(diff1, diff1_add_file, '', 'add-file'),
            """\
only in patch2:
unchanged:
--- c.py
+++ c.py
@@ -1,3 +1,5 @@
 1
 2
 3
+4
+5
""")
        self.assertMultiLineEqual(
            HasLanded.diff_diff(diff1, diff1_remove_file, '', 'remove-file'),
            """\
reverted:
--- b.py      2013-01-21 15:22:24.382555711 +1100
+++ b.py      2013-01-21 15:20:59.693887185 +1100
@@ -1,5 +1,3 @@
 1
 2
 3
-4
-5
""")

    def test_convert_to_svn_and_strip_change_log(self):
        # These patches require trailing whitespace to remain valid patches.
        testbefore1 = HasLanded.convert_to_svn("""\
diff --git a/Tools/ChangeLog b/Tools/ChangeLog
index 219ba72..0390b73 100644
--- a/Tools/ChangeLog
+++ b/Tools/ChangeLog
@@ -1,3 +1,32 @@
+2013-01-17  Tim 'mithro' Ansell  <mithro@mithis.com>
+
+        Adding "has-landed" command to webkit-patch which allows a person to
+        Reviewed by NOBODY (OOPS!).
+
 2013-01-20  Tim 'mithro' Ansell  <mithro@mithis.com>
 
         Extend diff_parser to support the --full-index output.
diff --git a/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py b/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
index 4bf8ec6..3a128cb 100644
--- a/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
+++ b/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
@@ -28,6 +28,8 @@
+import re
+
 from .attachment import Attachment
 
""")
        testafter1 = HasLanded.convert_to_svn("""\
diff --git a/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py b/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
index 4bf8ec6..3a128cb 100644
--- a/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
+++ b/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
@@ -28,6 +28,8 @@
+import re
+
 from .attachment import Attachment
 
diff --git a/Tools/ChangeLog b/Tools/ChangeLog
index 219ba72..0390b73 100644
--- a/Tools/ChangeLog
+++ b/Tools/ChangeLog
@@ -1,3 +1,32 @@
+2013-01-17  Tim 'mithro' Ansell  <mithro@mithis.com>
+
+        Adding "has-landed" command to webkit-patch which allows a person to
+        Reviewed by NOBODY (OOPS!).
+
 2013-01-20  Tim 'mithro' Ansell  <mithro@mithis.com>
 
         Extend diff_parser to support the --full-index output.
""")
        testexpected1 = """\
Index: Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
===================================================================
--- Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
+++ Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
@@ -28,6 +28,8 @@
+import re
+
 from .attachment import Attachment
 
"""
        testmiddle1 = HasLanded.convert_to_svn("""\
diff --git a/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py b/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
index 4bf8ec6..3a128cb 100644
--- a/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
+++ b/Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
@@ -28,6 +28,8 @@
+import re
+
 from .attachment import Attachment
 
diff --git a/ChangeLog b/ChangeLog
index 219ba72..0390b73 100644
--- a/ChangeLog
+++ b/ChangeLog
@@ -1,3 +1,32 @@
+2013-01-17  Tim 'mithro' Ansell  <mithro@mithis.com>
+
+        Adding "has-landed" command to webkit-patch which allows a person to
+        Reviewed by NOBODY (OOPS!).
+
 2013-01-20  Tim 'mithro' Ansell  <mithro@mithis.com>
 
         Extend diff_parser to support the --full-index output.
diff --git a/Tools/Scripts/webkitpy/common/other.py b/Tools/Scripts/webkitpy/common/other.py
index 4bf8ec6..3a128cb 100644
--- a/Tools/Scripts/webkitpy/common/other.py
+++ b/Tools/Scripts/webkitpy/common/other.py
@@ -28,6 +28,8 @@
+import re
+
 from .attachment import Attachment
 
""")
        testexpected2 = """\
Index: Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
===================================================================
--- Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
+++ Tools/Scripts/webkitpy/common/net/bugzilla/bug.py
@@ -28,6 +28,8 @@
+import re
+
 from .attachment import Attachment
 
Index: Tools/Scripts/webkitpy/common/other.py
===================================================================
--- Tools/Scripts/webkitpy/common/other.py
+++ Tools/Scripts/webkitpy/common/other.py
@@ -28,6 +28,8 @@
+import re
+
 from .attachment import Attachment
 
"""

        self.assertMultiLineEqual(testexpected1, HasLanded.strip_change_log(testbefore1))
        self.assertMultiLineEqual(testexpected1, HasLanded.strip_change_log(testafter1))
        self.assertMultiLineEqual(testexpected2, HasLanded.strip_change_log(testmiddle1))
