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


class WebKitFinder(object):
    def __init__(self, filesystem):
        self._filesystem = filesystem
        self._webkit_base = None

    def webkit_base(self):
        """Returns the absolute path to the top of the WebKit tree.

        Raises an AssertionError if the top dir can't be determined."""
        # Note: This code somewhat duplicates the code in
        # scm.find_checkout_root(). However, that code only works if the top
        # of the SCM repository also matches the top of the WebKit tree. Some SVN users
        # (the chromium test bots, for example), might only check out subdirectories like
        # Tools/Scripts. This code will also work if there is no SCM system at all.
        if not self._webkit_base:
            self._webkit_base = self._webkit_base
            module_path = self._filesystem.path_to_module(self.__module__)
            tools_index = module_path.rfind('Tools')
            assert tools_index != -1, "could not find location of this checkout from %s" % module_path
            self._webkit_base = self._filesystem.normpath(module_path[0:tools_index - 1])
        return self._webkit_base

    def path_from_webkit_base(self, *comps):
        return self._filesystem.join(self.webkit_base(), *comps)

    def path_to_script(self, script_name):
        """Returns the relative path to the script from the top of the WebKit tree."""
        # This is intentionally relative in order to force callers to consider what
        # their current working directory is (and change to the top of the tree if necessary).
        return self._filesystem.join("Tools", "Scripts", script_name)

    def layout_tests_dir(self):
        return self.path_from_webkit_base('LayoutTests')

    def perf_tests_dir(self):
        return self.path_from_webkit_base('PerformanceTests')
