# Copyright (c) 2012 Google Inc. All rights reserved.
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

from webkitpy.tool.bot.patchanalysistask import PatchAnalysisTask, PatchAnalysisTaskDelegate, UnableToApplyPatch


class StyleQueueTaskDelegate(PatchAnalysisTaskDelegate):
    def parent_command(self):
        return "style-queue"


class StyleQueueTask(PatchAnalysisTask):
    def validate(self):
        self._patch = self._delegate.refetch_patch(self._patch)
        if self._patch.is_obsolete():
            return False
        if self._patch.bug().is_closed():
            return False
        if self._patch.review() == "-":
            return False
        return True

    def _check_style(self):
        return self._run_command([
            "check-style-local",
            "--non-interactive",
            "--quiet",
        ],
        "Style checked",
        "Patch did not pass style check")

    def _apply_watch_list(self):
        return self._run_command([
            "apply-watchlist-local",
            self._patch.bug_id(),
        ],
        "Watchlist applied",
        "Unabled to apply watchlist")

    def run(self):
        if not self._clean():
            return False
        if not self._update():
            return False
        if not self._apply():
            raise UnableToApplyPatch(self._patch)
        self._apply_watch_list()
        if not self._check_style():
            return self.report_failure()
        return True
