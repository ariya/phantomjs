# Copyright (C) 2013 Apple Inc. All rights reserved.
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

from webkitpy.tool.commands.newcommitbot import NewCommitBot


class NewCommitBotTest(unittest.TestCase):
    def test_summarize_commit_log_basic(self):
        self.assertEqual(NewCommitBot._summarize_commit_log("""------------------------------------------------------------------------
r143106 | jochen@chromium.org | 2013-02-16 10:27:07 -0800 (Sat, 16 Feb 2013) | 10 lines

[chromium] initialize all variables of TestRunner classes
https://bugs.webkit.org/show_bug.cgi?id=110013

Reviewed by Adam Barth.

* DumpRenderTree/chromium/TestRunner/src/TestInterfaces.cpp:
(WebTestRunner::TestInterfaces::TestInterfaces):
* DumpRenderTree/chromium/TestRunner/src/TestRunner.cpp:
(WebTestRunner::TestRunner::TestRunner):

------------------------------------------------------------------------"""),
            "https://trac.webkit.org/r143106 by Jochen Eisinger (jochen__) [chromium] initialize all variables of TestRunner classes"
            " https://webkit.org/b/110013 Reviewed by Adam Barth (abarth).")

        self.assertEqual(NewCommitBot._summarize_commit_log("""------------------------------------------------------------------------
r140066 | simon.fraser@apple.com | 2013-01-17 16:10:31 -0800 (Thu, 17 Jan 2013) | 10 lines

Allow PaintInfo to carry all PaintBehavior flags
https://bugs.webkit.org/show_bug.cgi?id=106980

Reviewed by Beth Dakin.

In r139908 I missed one instance of the PaintInfo constructor that should take PaintBehaviorNormal
instead of "false".

* rendering/RenderScrollbarPart.cpp:
(WebCore::RenderScrollbarPart::paintIntoRect):
------------------------------------------------------------------------"""),
            "https://trac.webkit.org/r140066 by Simon Fraser (smfr)"
            " Allow PaintInfo to carry all PaintBehavior flags https://webkit.org/b/106980 Reviewed by Beth Dakin (dethbakin).")

    def test_summarize_commit_log_rollout(self):
        self.assertEqual(NewCommitBot._summarize_commit_log("""------------------------------------------------------------------------
r143104 | commit-queue@webkit.org | 2013-02-16 09:09:01 -0800 (Sat, 16 Feb 2013) | 27 lines

Unreviewed, rolling out r142734.
http://trac.webkit.org/changeset/142734
https://bugs.webkit.org/show_bug.cgi?id=110018

"Triggered crashes on lots of websites" (Requested by ggaren
on #webkit).

Patch by Sheriff Bot <webkit.review.bot@gmail.com> on 2013-02-16

Source/WebCore:

------------------------------------------------------------------------"""),
            "Geoffrey Garen (ggaren) rolled out r142734 in https://trac.webkit.org/r143104 : Triggered crashes on lots of websites")

        self.assertEqual(NewCommitBot._summarize_commit_log("""------------------------------------------------------------------------
r139884 | kov@webkit.org | 2013-01-16 08:26:10 -0800 (Wed, 16 Jan 2013) | 23 lines

[GStreamer][Soup] Let GStreamer provide the buffer data is downloaded to, to avoid copying
https://bugs.webkit.org/show_bug.cgi?id=105552

Reverting 139877. It made a couple of API tests fail.

* platform/graphics/gstreamer/GStreamerVersioning.cpp:
* platform/graphics/gstreamer/GStreamerVersioning.h:
* platform/graphics/gstreamer/WebKitWebSourceGStreamer.cpp:
(StreamingClient):
(_WebKitWebSrcPrivate):

------------------------------------------------------------------------"""),
            "Gustavo Noronha Silva (kov) rolled out 139877 in https://trac.webkit.org/r139884"
            " [GStreamer][Soup] Let GStreamer provide the buffer data is downloaded to, to avoid copying"
            " https://webkit.org/b/105552 It made a couple of API tests fail.")

        self.assertEqual(NewCommitBot._summarize_commit_log("""------------------------------------------------------------------------
r135487 | commit-queue@webkit.org | 2012-11-22 00:09:25 -0800 (Thu, 22 Nov 2012) | 52 lines

Unreviewed, rolling out r134927 and r134944.
http://trac.webkit.org/changeset/134927
http://trac.webkit.org/changeset/134944
https://bugs.webkit.org/show_bug.cgi?id=103028

Reverting the reverts after merging. (Requested by vsevik on
#webkit).

Patch by Sheriff Bot <webkit.review.bot@gmail.com> on 2012-11-22

* English.lproj/localizedStrings.js:
* WebCore.gypi:
* WebCore.vcproj/WebCore.vcproj:
* inspector/compile-front-end.py:
* inspector/front-end/AdvancedSearchController.js:
* inspector/front-end/CallStackSidebarPane.js:

------------------------------------------------------------------------"""),
            "Vsevolod Vlasov (vsevik) rolled out r134927 and r134944 in https://trac.webkit.org/r135487 :"
            " Reverting the reverts after merging.")
