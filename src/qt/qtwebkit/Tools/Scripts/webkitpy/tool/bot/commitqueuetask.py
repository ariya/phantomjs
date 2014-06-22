# Copyright (c) 2010 Google Inc. All rights reserved.
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

from webkitpy.tool.bot.patchanalysistask import PatchAnalysisTask, PatchAnalysisTaskDelegate


class CommitQueueTaskDelegate(PatchAnalysisTaskDelegate):
    def parent_command(self):
        return "commit-queue"

    def did_pass_testing_ews(self, patch):
        raise NotImplementedError("subclasses must implement")


class CommitQueueTask(PatchAnalysisTask):
    def validate(self):
        # Bugs might get closed, or patches might be obsoleted or r-'d while the
        # commit-queue is processing.
        self._patch = self._delegate.refetch_patch(self._patch)
        if self._patch.is_obsolete():
            return False
        if self._patch.bug().is_closed():
            return False
        if not self._patch.committer():
            return False
        if self._patch.review() == "-":
            return False
        return True

    def _validate_changelog(self):
        return self._run_command([
            "validate-changelog",
            "--check-oops",
            "--non-interactive",
            self._patch.id(),
        ],
        "ChangeLog validated",
        "ChangeLog did not pass validation")

    def _did_pass_tests_recently(self):
        if self._delegate.did_pass_testing_ews(self._patch):
            return True
        return self._test_patch()

    def run(self):
        if not self.validate():
            return False
        if not self._clean():
            return False
        if not self._update():
            return False
        if not self._apply():
            return self.report_failure()
        if not self._validate_changelog():
            return self.report_failure()
        if not self._patch.is_rollout():
            if not self._build():
                if not self._build_without_patch():
                    return False
                return self.report_failure()
            if not self._did_pass_tests_recently():
                return False
        # Make sure the patch is still valid before landing (e.g., make sure
        # no one has set commit-queue- since we started working on the patch.)
        if not self.validate():
            return False
        # FIXME: We should understand why the land failure occurred and retry if possible.
        if not self._land():
            return self.report_failure()
        return True
