# Copyright (c) 2009 Google Inc. All rights reserved.
# Copyright (c) 2009 Apple Inc. All rights reserved.
# Copyright (c) 2010 Research In Motion Limited. All rights reserved.
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

from webkitpy.common.config import committers, urls


class CommitterValidator(object):
    def __init__(self, host):
        self.host = host

    def _committers_py_path(self):
        # extension can sometimes be .pyc, we always want .py
        committers_path = self.host.filesystem.path_to_module(committers.__name__)
        (path, extension) = self.host.filesystem.splitext(committers_path)
        path = self.host.filesystem.relpath(path, self.host.scm().checkout_root)
        return ".".join([path, "py"])

    def _flag_permission_rejection_message(self, setter_email, flag_name):
        # This could be queried from the tool.
        queue_name = "commit-queue"
        committers_list = self._committers_py_path()
        message = "%s does not have %s permissions according to %s." % (
                        setter_email,
                        flag_name,
                        urls.view_source_url(committers_list))
        message += "\n\n- If you do not have %s rights please read %s for instructions on how to use bugzilla flags." % (
                        flag_name, urls.contribution_guidelines)
        message += "\n\n- If you have %s rights please correct the error in %s by adding yourself to the file (no review needed).  " % (
                        flag_name, committers_list)
        message += "The %s restarts itself every 2 hours.  After restart the %s will correctly respect your %s rights." % (
                        queue_name, queue_name, flag_name)
        return message

    def _validate_setter_email(self, patch, result_key, rejection_function):
        committer = getattr(patch, result_key)()
        # If the flag is set, and we don't recognize the setter, reject the flag!
        setter_email = patch._attachment_dictionary.get("%s_email" % result_key)
        if setter_email and not committer:
            rejection_function(patch.id(), self._flag_permission_rejection_message(setter_email, result_key))
            return False
        return True

    def _reject_patch_if_flags_are_invalid(self, patch):
        return (self._validate_setter_email(patch, "reviewer", self.reject_patch_from_review_queue)
            and self._validate_setter_email(patch, "committer", self.reject_patch_from_commit_queue))

    def patches_after_rejecting_invalid_commiters_and_reviewers(self, patches):
        return [patch for patch in patches if self._reject_patch_if_flags_are_invalid(patch)]

    def reject_patch_from_commit_queue(self,
                                       attachment_id,
                                       additional_comment_text=None):
        comment_text = "Rejecting attachment %s from commit-queue." % attachment_id
        if additional_comment_text:
            comment_text += "\n\n%s" % additional_comment_text
        self.host.bugs.set_flag_on_attachment(attachment_id,
                                              "commit-queue",
                                              "-",
                                              comment_text)

    def reject_patch_from_review_queue(self,
                                       attachment_id,
                                       additional_comment_text=None):
        comment_text = "Rejecting attachment %s from review queue." % attachment_id
        if additional_comment_text:
            comment_text += "\n\n%s" % additional_comment_text
        self.host.bugs.set_flag_on_attachment(attachment_id,
                                              'review',
                                              '-',
                                              comment_text)
