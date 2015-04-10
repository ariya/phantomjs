# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

'''Unit tests for watchlist.py.'''

import unittest2 as unittest

from webkitpy.common.checkout.diff_test_data import DIFF_TEST_DATA
from webkitpy.common.watchlist.watchlistparser import WatchListParser


class WatchListTest(unittest.TestCase):
    def setUp(self):
        self._watch_list_parser = WatchListParser()

    def test_filename_definition_no_matches(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            '            "filename": r".*\\MyFileName\\.cpp",'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList1": ['
            '            "levin@chromium.org",'
            '        ],'
           '    },'
            '}')
        self.assertEqual(set([]), watch_list.find_matching_definitions(DIFF_TEST_DATA))

    def test_filename_definition(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            '            "filename": r"WebCore/rendering/style/StyleFlexibleBoxData\.h",'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList1": ['
            '            "levin@chromium.org",'
            '        ],'
           '    },'
            '}')
        self.assertEqual(set(['WatchList1']), watch_list.find_matching_definitions(DIFF_TEST_DATA))

    def test_cc_rules_simple(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            '            "filename": r"WebCore/rendering/style/StyleFlexibleBoxData\.h",'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList1": ['
            '            "levin@chromium.org",'
            '        ],'
           '    },'
            '}')
        cc_and_messages = watch_list.determine_cc_and_messages(DIFF_TEST_DATA)
        self.assertEqual({
                'cc_list': ['levin@chromium.org'],
                'messages': [],
                }, cc_and_messages)

    def test_cc_rules_complex(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            '            "filename": r"WebCore/rendering/style/StyleFlexibleBoxData\.h",'
            '        },'
            '        "WatchList2": {'
            '            "filename": r"WillNotMatch",'
            '        },'
            '        "WatchList3": {'
            '            "filename": r"WillNotMatch",'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList2|WatchList1|WatchList3": [ "levin@chromium.org", ],'
            '    },'
            '}')
        cc_and_messages = watch_list.determine_cc_and_messages(DIFF_TEST_DATA)
        self.assertEqual({
                'cc_list': ['levin@chromium.org'],
                'messages': [],
                }, cc_and_messages)

    def test_cc_and_message_rules_complex(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            '            "filename": r"WebCore/rendering/style/StyleFlexibleBoxData\.h",'
            '        },'
            '        "WatchList2": {'
            '            "filename": r"WillNotMatch",'
            '        },'
            '        "WatchList3": {'
            '            "filename": r"WillNotMatch",'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList2|WatchList1|WatchList3": [ "levin@chromium.org", ],'
            '    },'
            '    "MESSAGE_RULES": {'
            '        "WatchList2|WatchList1|WatchList3": [ "msg1", "msg2", ],'
            '    },'
            '}')
        cc_and_messages = watch_list.determine_cc_and_messages(DIFF_TEST_DATA)
        self.assertEqual({
                'cc_list': ['levin@chromium.org'],
                'messages': ['msg1', 'msg2'],
                }, cc_and_messages)

    def test_cc_and_message_rules_no_matches(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            '            "filename": r"WebCore/rendering/style/ThisFileDoesNotExist\.h",'
            '        },'
            '        "WatchList2": {'
            '            "filename": r"WillNotMatch",'
            '        },'
            '        "WatchList3": {'
            '            "filename": r"WillNotMatch",'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList2|WatchList1|WatchList3": [ "levin@chromium.org", ],'
            '    },'
            '    "MESSAGE_RULES": {'
            '        "WatchList2|WatchList1|WatchList3": [ "msg1", "msg2", ],'
            '    },'
            '}')
        cc_and_messages = watch_list.determine_cc_and_messages(DIFF_TEST_DATA)
        self.assertEqual({
                'cc_list': [],
                'messages': [],
                }, cc_and_messages)

    def test_added_match(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            '            "in_added_lines": r"RenderStyle::initialBoxOrient",'
            '        },'
            '        "WatchList2": {'
            '            "in_deleted_lines": r"RenderStyle::initialBoxOrient",'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList1": [ "eric@webkit.org", ],'
            '        "WatchList2": [ "abarth@webkit.org", ],'
            '    },'
            '}')
        cc_and_messages = watch_list.determine_cc_and_messages(DIFF_TEST_DATA)
        self.assertEqual({
                'cc_list': ['eric@webkit.org'],
                'messages': [],
                }, cc_and_messages)

    def test_deleted_match(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            '            "in_added_lines": r"unsigned orient: 1;",'
            '        },'
            '        "WatchList2": {'
            '            "in_deleted_lines": r"unsigned orient: 1;",'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList1": [ "eric@webkit.org", ],'
            '        "WatchList2": [ "abarth@webkit.org", ],'
            '    },'
            '}')
        cc_and_messages = watch_list.determine_cc_and_messages(DIFF_TEST_DATA)
        self.assertEqual({
                'cc_list': ['abarth@webkit.org'],
                'messages': [],
                }, cc_and_messages)

    def test_more_and_less_match(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            # This pattern is in both added and deleted lines, so no match.
            '            "more": r"userSelect == o\.userSelect",'
            '        },'
            '        "WatchList2": {'
            '            "more": r"boxOrient\(o\.boxOrient\)",'
            '        },'
            '        "WatchList3": {'
            '            "less": r"unsigned orient"'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList1": [ "eric@webkit.org", ],'
            '        "WatchList2": [ "levin@chromium.org", ],'
            '    },'
            '    "MESSAGE_RULES": {'
            '        "WatchList3": ["Test message."],'
            '    },'
            '}')
        cc_and_messages = watch_list.determine_cc_and_messages(DIFF_TEST_DATA)
        self.assertEqual({
                'cc_list': ['levin@chromium.org'],
                'messages': ["Test message."],
                }, cc_and_messages)

    def test_complex_match(self):
        watch_list = self._watch_list_parser.parse(
            '{'
            '    "DEFINITIONS": {'
            '        "WatchList1": {'
            '            "filename": r"WebCore/rendering/style/StyleRareInheritedData\.cpp",'
            '            "in_added_lines": r"\&\& boxOrient == o\.boxOrient;",'
            '            "in_deleted_lines": r"\&\& userSelect == o\.userSelect;",'
            '            "more": r"boxOrient\(o\.boxOrient\)",'
            '        },'
            '        "WatchList2": {'
            '            "filename": r"WebCore/rendering/style/StyleRareInheritedData\.cpp",'
            '            "in_added_lines": r"RenderStyle::initialBoxOrient",'
            '            "less": r"userSelect;"'
            '        },'
            # WatchList3 won't match because these two patterns aren't in the same file.
            '        "WatchList3": {'
            '            "in_added_lines": r"RenderStyle::initialBoxOrient",'
            '            "in_deleted_lines": r"unsigned orient: 1;",'
            '        },'
            '     },'
            '    "CC_RULES": {'
            '        "WatchList1": [ "eric@webkit.org", ],'
            '        "WatchList3": [ "abarth@webkit.org", ],'
            '    },'
            '    "MESSAGE_RULES": {'
            '        "WatchList2": ["This is a test message."],'
            '    },'
            '}')
        cc_and_messages = watch_list.determine_cc_and_messages(DIFF_TEST_DATA)
        self.assertEqual({
                'cc_list': ['eric@webkit.org'],
                'messages': ["This is a test message."],
                }, cc_and_messages)
