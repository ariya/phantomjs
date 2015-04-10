# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
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

from webkitpy.port import builders
from webkitpy.tool.commands.rebaseline import AbstractRebaseliningCommand
from webkitpy.tool.servers.gardeningserver import GardeningHTTPServer


class GardenOMatic(AbstractRebaseliningCommand):
    name = "garden-o-matic"
    help_text = "Command for gardening the WebKit tree."

    def __init__(self):
        super(GardenOMatic, self).__init__(options=(self.platform_options + [
            self.move_overwritten_baselines_option,
            self.results_directory_option,
            self.no_optimize_option,
            ]))

    def execute(self, options, args, tool):
        print "This command runs a local HTTP server that changes your working copy"
        print "based on the actions you take in the web-based UI."

        args = {}
        if options.platform:
            # FIXME: This assumes that the port implementation (chromium-, gtk-, etc.) is the first part of options.platform.
            args['platform'] = options.platform.split('-')[0]
            builder = builders.builder_name_for_port_name(options.platform)
            if builder:
                args['builder'] = builder
        if options.results_directory:
            args['useLocalResults'] = "true"

        httpd = GardeningHTTPServer(httpd_port=8127, config={'tool': tool, 'options': options})
        self._tool.user.open_url(httpd.url(args))

        print "Local HTTP server started."
        httpd.serve_forever()
