# Copyright (C) 2010 Google Inc. All rights reserved.
# Copyrigth (C) 2012 Intel Corporation
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
#     * Neither the Google name nor the names of its
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
import subprocess


_log = logging.getLogger(__name__)


# Shared by GTK and EFL for pulseaudio sanitizing before running tests.
class PulseAudioSanitizer:
    def unload_pulseaudio_module(self):
        # Unload pulseaudio's module-stream-restore, since it remembers
        # volume settings from different runs, and could affect
        # multimedia tests results
        self._pa_module_index = -1
        with open(os.devnull, 'w') as devnull:
            try:
                pactl_process = subprocess.Popen(["pactl", "list", "short", "modules"], stdout=subprocess.PIPE, stderr=devnull)
                pactl_process.wait()
            except OSError:
                # pactl might not be available.
                _log.debug('pactl not found. Please install pulseaudio-utils to avoid some potential media test failures.')
                return
        modules_list = pactl_process.communicate()[0]
        for module in modules_list.splitlines():
            if module.find("module-stream-restore") >= 0:
                # Some pulseaudio-utils versions don't provide
                # the index, just an empty string
                self._pa_module_index = module.split('\t')[0] or -1
                try:
                    # Since they could provide other stuff (not an index
                    # nor an empty string, let's make sure this is an int.
                    if int(self._pa_module_index) != -1:
                        pactl_process = subprocess.Popen(["pactl", "unload-module", self._pa_module_index])
                        pactl_process.wait()
                        if pactl_process.returncode == 0:
                            _log.debug('Unloaded module-stream-restore successfully')
                        else:
                            _log.debug('Unloading module-stream-restore failed')
                except ValueError:
                        # pactl should have returned an index if the module is found
                        _log.debug('Unable to parse module index. Please check if your pulseaudio-utils version is too old.')
                return

    def restore_pulseaudio_module(self):
        # If pulseaudio's module-stream-restore was previously unloaded,
        # restore it back. We shouldn't need extra checks here, since an
        # index != -1 here means we successfully unloaded it previously.
        if self._pa_module_index != -1:
            with open(os.devnull, 'w') as devnull:
                pactl_process = subprocess.Popen(["pactl", "load-module", "module-stream-restore"], stdout=devnull, stderr=devnull)
                pactl_process.wait()
                if pactl_process.returncode == 0:
                    _log.debug('Restored module-stream-restore successfully')
                else:
                    _log.debug('Restoring module-stream-restore failed')
