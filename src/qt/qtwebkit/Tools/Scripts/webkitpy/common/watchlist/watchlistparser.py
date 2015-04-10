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


import difflib
import logging
import re

from webkitpy.common.watchlist.amountchangedpattern import AmountChangedPattern
from webkitpy.common.watchlist.changedlinepattern import ChangedLinePattern
from webkitpy.common.watchlist.filenamepattern import FilenamePattern
from webkitpy.common.watchlist.watchlist import WatchList
from webkitpy.common.watchlist.watchlistrule import WatchListRule
from webkitpy.common.config.committers import CommitterList


_log = logging.getLogger(__name__)


class WatchListParser(object):
    _DEFINITIONS = 'DEFINITIONS'
    _CC_RULES = 'CC_RULES'
    _MESSAGE_RULES = 'MESSAGE_RULES'
    _INVALID_DEFINITION_NAME_REGEX = r'\|'

    def __init__(self, log_error=None):
        self._log_error = log_error or _log.error
        self._section_parsers = {
            self._DEFINITIONS: self._parse_definition_section,
            self._CC_RULES: self._parse_cc_rules,
            self._MESSAGE_RULES: self._parse_message_rules,
        }
        self._definition_pattern_parsers = {
            'filename': FilenamePattern,
            'in_added_lines': (lambda compiled_regex: ChangedLinePattern(compiled_regex, 0)),
            'in_deleted_lines': (lambda compiled_regex: ChangedLinePattern(compiled_regex, 1)),
            'less': (lambda compiled_regex: AmountChangedPattern(compiled_regex, 1)),
            'more': (lambda compiled_regex: AmountChangedPattern(compiled_regex, 0)),
        }

    def parse(self, watch_list_contents):
        watch_list = WatchList()

        # Change the watch list text into a dictionary.
        dictionary = self._eval_watch_list(watch_list_contents)

        # Parse the top level sections in the watch list.
        for section in dictionary:
            parser = self._section_parsers.get(section)
            if not parser:
                self._log_error(('Unknown section "%s" in watch list.'
                                + self._suggest_words(section, self._section_parsers.keys()))
                               % section)
                continue
            parser(dictionary[section], watch_list)

        self._validate(watch_list)
        return watch_list

    def _eval_watch_list(self, watch_list_contents):
        return eval(watch_list_contents, {'__builtins__': None}, None)

    def _suggest_words(self, invalid_word, valid_words):
        close_matches = difflib.get_close_matches(invalid_word, valid_words)
        if not close_matches:
            return ''
        return '\n\nPerhaps it should be %s.' % (' or '.join(close_matches))

    def _parse_definition_section(self, definition_section, watch_list):
        definitions = {}
        for name in definition_section:
            invalid_character = re.search(self._INVALID_DEFINITION_NAME_REGEX, name)
            if invalid_character:
                self._log_error('Invalid character "%s" in definition "%s".' % (invalid_character.group(0), name))
                continue

            definition = definition_section[name]
            definitions[name] = []
            for pattern_type in definition:
                pattern_parser = self._definition_pattern_parsers.get(pattern_type)
                if not pattern_parser:
                    self._log_error(('Unknown pattern type "%s" in definition "%s".'
                                     + self._suggest_words(pattern_type, self._definition_pattern_parsers.keys()))
                                    % (pattern_type, name))
                    continue

                try:
                    compiled_regex = re.compile(definition[pattern_type])
                except Exception, e:
                    self._log_error('The regex "%s" is invalid due to "%s".' % (definition[pattern_type], str(e)))
                    continue

                pattern = pattern_parser(compiled_regex)
                definitions[name].append(pattern)
            if not definitions[name]:
                self._log_error('The definition "%s" has no patterns, so it should be deleted.' % name)
                continue
        watch_list.definitions = definitions

    def _parse_rules(self, rules_section):
        rules = []
        for complex_definition in rules_section:
            instructions = rules_section[complex_definition]
            if not instructions:
                self._log_error('A rule for definition "%s" is empty, so it should be deleted.' % complex_definition)
                continue
            rules.append(WatchListRule(complex_definition, instructions))
        return rules

    def _parse_cc_rules(self, cc_section, watch_list):
        watch_list.cc_rules = self._parse_rules(cc_section)

    def _parse_message_rules(self, message_section, watch_list):
        watch_list.message_rules = self._parse_rules(message_section)

    def _validate(self, watch_list):
        cc_definitions_set = self._rule_definitions_as_set(watch_list.cc_rules)
        messages_definitions_set = self._rule_definitions_as_set(watch_list.message_rules)
        self._verify_all_definitions_are_used(watch_list, cc_definitions_set.union(messages_definitions_set))

        self._validate_definitions(cc_definitions_set, self._CC_RULES, watch_list)
        self._validate_definitions(messages_definitions_set, self._MESSAGE_RULES, watch_list)

        accounts = CommitterList()
        for cc_rule in watch_list.cc_rules:
            # Copy the instructions since we'll be remove items from the original list and
            # modifying a list while iterating through it leads to undefined behavior.
            intructions_copy = cc_rule.instructions()[:]
            for email in intructions_copy:
                if not accounts.contributor_by_email(email):
                    cc_rule.remove_instruction(email)
                    self._log_error("The email alias %s which is in the watchlist is not listed as a contributor in committers.py" % email)
                    continue

    def _verify_all_definitions_are_used(self, watch_list, used_definitions):
        definitions_not_used = set(watch_list.definitions.keys())
        definitions_not_used.difference_update(used_definitions)
        if definitions_not_used:
            self._log_error('The following definitions are not used and should be removed: %s' % (', '.join(definitions_not_used)))

    def _validate_definitions(self, definitions, rules_section_name, watch_list):
        declared_definitions = watch_list.definitions.keys()
        definition_set = set(definitions)
        definition_set.difference_update(declared_definitions)

        if definition_set:
            suggestions = ''
            if len(definition_set) == 1:
                suggestions = self._suggest_words(set().union(definition_set).pop(), declared_definitions)
            self._log_error('In section "%s", the following definitions are not used and should be removed: %s%s' % (rules_section_name, ', '.join(definition_set), suggestions))

    def _rule_definitions_as_set(self, rules):
        definition_set = set()
        for rule in rules:
            definition_set = definition_set.union(rule.definitions_to_match)
        return definition_set
