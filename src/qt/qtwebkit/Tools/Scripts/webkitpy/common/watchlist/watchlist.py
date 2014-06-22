# Copyright (C) 2011 Google Inc. All rights reserved.
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

from webkitpy.common.checkout.diff_parser import DiffParser


class WatchList(object):
    def __init__(self):
        self.definitions = {}
        self.cc_rules = set()
        self.message_rules = set()

    def find_matching_definitions(self, diff):
        matching_definitions = set()
        patch_files = DiffParser(diff.splitlines()).files

        for path, diff_file in patch_files.iteritems():
            for definition in self.definitions:
                # If a definition has already matched, there is no need to process it.
                if definition in matching_definitions:
                    continue

                # See if the definition matches within one file.
                for pattern in self.definitions[definition]:
                    if not pattern.match(path, diff_file.lines):
                        break
                else:
                    matching_definitions.add(definition)
        return matching_definitions

    def _determine_instructions(self, matching_definitions, rules):
        instructions = set()
        for rule in rules:
            if rule.match(matching_definitions):
                instructions.update(rule.instructions())
        # Sort the results to make the order deterministic (for consistency and easier testing).
        return sorted(instructions)

    def determine_cc_list(self, matching_definitions):
        return self._determine_instructions(matching_definitions, self.cc_rules)

    def determine_messages(self, matching_definitions):
        return self._determine_instructions(matching_definitions, self.message_rules)

    def determine_cc_and_messages(self, diff):
        definitions = self.find_matching_definitions(diff)
        return {
            'cc_list': self.determine_cc_list(definitions),
            'messages':  self.determine_messages(definitions),
        }
