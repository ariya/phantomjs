# Copyright (c) 2010 Google Inc. All rights reserved.
# Copyright (c) 2009 Apple Inc. All rights reserved.
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

import logging
import os
import sys

from webkitpy.common.checkout import Checkout
from webkitpy.common.checkout.scm.detection import SCMDetector
from webkitpy.common.memoized import memoized
from webkitpy.common.net import bugzilla, buildbot, web
from webkitpy.common.system.systemhost import SystemHost
from webkitpy.common.watchlist.watchlistparser import WatchListParser
from webkitpy.port.factory import PortFactory


_log = logging.getLogger(__name__)


class Host(SystemHost):
    def __init__(self):
        SystemHost.__init__(self)
        self.web = web.Web()

        # FIXME: Checkout should own the scm object.
        self._scm = None
        self._checkout = None

        # Everything below this line is WebKit-specific and belongs on a higher-level object.
        self.bugs = bugzilla.Bugzilla()
        self.buildbot = buildbot.BuildBot()

        # FIXME: Unfortunately Port objects are currently the central-dispatch objects of the NRWT world.
        # In order to instantiate a port correctly, we have to pass it at least an executive, user, scm, and filesystem
        # so for now we just pass along the whole Host object.
        # FIXME: PortFactory doesn't belong on this Host object if Port is going to have a Host (circular dependency).
        self.port_factory = PortFactory(self)

        self._engage_awesome_locale_hacks()

    # We call this from the Host constructor, as it's one of the
    # earliest calls made for all webkitpy-based programs.
    def _engage_awesome_locale_hacks(self):
        # To make life easier on our non-english users, we override
        # the locale environment variables inside webkitpy.
        # If we don't do this, programs like SVN will output localized
        # messages and svn.py will fail to parse them.
        # FIXME: We should do these overrides *only* for the subprocesses we know need them!
        # This hack only works in unix environments.
        os.environ['LANGUAGE'] = 'en'
        os.environ['LANG'] = 'en_US.UTF-8'
        os.environ['LC_MESSAGES'] = 'en_US.UTF-8'
        os.environ['LC_ALL'] = ''

    def initialize_scm(self, patch_directories=None):
        detector = SCMDetector(self.filesystem, self.executive)
        self._scm = detector.default_scm(patch_directories)
        self._checkout = Checkout(self.scm())

    def scm(self):
        return self._scm

    def checkout(self):
        return self._checkout

    @memoized
    def watch_list(self):
        config_path = self.filesystem.dirname(self.filesystem.path_to_module('webkitpy.common.config'))
        watch_list_full_path = self.filesystem.join(config_path, 'watchlist')
        if not self.filesystem.exists(watch_list_full_path):
            raise Exception('Watch list file (%s) not found.' % watch_list_full_path)

        watch_list_contents = self.filesystem.read_text_file(watch_list_full_path)
        return WatchListParser().parse(watch_list_contents)
