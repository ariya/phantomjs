# Copyright (C) 2012 Balazs Ankes (bank@inf.u-szeged.hu) University of Szeged
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

from webkitpy.tool.steps.abstractstep import AbstractStep
from webkitpy.common import checksvnconfigfile
from webkitpy.common.checkout.scm.detection import SCMDetector
from webkitpy.common.system.systemhost import SystemHost

_log = logging.getLogger(__name__)


class AddSvnMimetypeForPng(AbstractStep):
    def __init__(self, tool, options, host=None, scm=None):
        self._tool = tool
        self._options = options
        self._host = host or SystemHost()
        self._fs = self._host.filesystem
        self._detector = scm or SCMDetector(self._fs, self._host.executive).detect_scm_system(self._fs.getcwd())

    def run(self, state):
        png_files = self._check_pngs(self._changed_files(state))

        if png_files:
            detection = self._detector.display_name()

            if detection == "git":
                (file_missing, autoprop_missing, png_missing) = checksvnconfigfile.check(self._host, self._fs)
                config_file_path = checksvnconfigfile.config_file_path(self._host, self._fs)

                if file_missing:
                    _log.info("There is no SVN config file. The svn:mime-type of pngs won't set.")
                    if not self._tool.user.confirm("Are you sure you want to continue?", default="n"):
                        self._exit(1)
                elif autoprop_missing and png_missing:
                    _log.info(checksvnconfigfile.errorstr_autoprop(config_file_path) + checksvnconfigfile.errorstr_png(config_file_path))
                    if not self._tool.user.confirm("Do you want to continue?", default="n"):
                        self._exit(1)
                elif autoprop_missing:
                    _log.info(checksvnconfigfile.errorstr_autoprop(config_file_path))
                    if not self._tool.user.confirm("Do you want to continue?", default="n"):
                        self._exit(1)
                elif png_missing:
                    _log.info(checksvnconfigfile.errorstr_png(config_file_path))
                    if not self._tool.user.confirm("Do you want to continue?", default="n"):
                        self._exit(1)

            elif detection == "svn":
                for filename in png_files:
                    if self._detector.exists(filename) and self._detector.propget('svn:mime-type', filename) != 'image/png':
                        print "Adding image/png mime-type to %s" % filename
                        self._detector.propset('svn:mime-type', 'image/png', filename)

    def _check_pngs(self, changed_files):
        png_files = []
        for filename in changed_files:
            if filename.endswith('.png'):
                png_files.append(filename)
        return png_files
