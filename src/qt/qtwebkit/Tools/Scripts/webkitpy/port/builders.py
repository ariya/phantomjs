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

import re

from webkitpy.common.memoized import memoized


# In this dictionary, each item stores:
# * port_name -- a fully qualified port name
# * is_debug -- whether we are using a debug build
# * move_overwritten_baselines_to -- (optional) list of platform directories that we will copy an existing
#      baseline to before pulling down a new baseline during rebaselining. This is useful
#      for bringing up a new port, for example when adding a Lion was the most recent Mac version and
#      we wanted to bring up Mountain Lion, we would want to copy an existing baseline in platform/mac
#      to platform/mac-mountainlion before updating the platform/mac entry.
# * rebaseline_override_dir -- (optional) directory to put baselines in instead of where you would normally put them.
#      This is useful when we don't have bots that cover particular configurations; so, e.g., you might
#      support mac-mountainlion but not have a mac-mountainlion bot yet, so you'd want to put the mac-lion
#      results into platform/mac temporarily.

_exact_matches = {
    # These builders are on build.webkit.org.
    "Apple MountainLion Release WK1 (Tests)": {"port_name": "mac-mountainlion", "is_debug": False, "rebaseline_override_dir": "mac"},
    "Apple MountainLion Debug WK1 (Tests)": {"port_name": "mac-mountainlion", "is_debug": True, "rebaseline_override_dir": "mac"},
    "Apple MountainLion Release WK2 (Tests)": {"port_name": "mac-mountainlion-wk2", "is_debug": False, "rebaseline_override_dir": "mac"},
    "Apple MountainLion Debug WK2 (Tests)": {"port_name": "mac-mountainlion-wk2", "is_debug": True, "rebaseline_override_dir": "mac"},
    "Apple Lion Release WK1 (Tests)": {"port_name": "mac-lion", "is_debug": False},
    "Apple Lion Debug WK1 (Tests)": {"port_name": "mac-lion", "is_debug": True},
    "Apple Lion Release WK2 (Tests)": {"port_name": "mac-lion-wk2", "is_debug": False},
    "Apple Lion Debug WK2 (Tests)": {"port_name": "mac-lion-wk2", "is_debug": True},

    "Apple Win XP Debug (Tests)": {"port_name": "win-xp", "is_debug": True},
    # FIXME: Remove rebaseline_override_dir once there is an Apple buildbot that corresponds to platform/win.
    "Apple Win 7 Release (Tests)": {"port_name": "win-7sp0", "is_debug": False, "rebaseline_override_dir": "win"},

    "GTK Linux 32-bit Release": {"port_name": "gtk", "is_debug": False},
    "GTK Linux 64-bit Debug": {"port_name": "gtk", "is_debug": True},
    "GTK Linux 64-bit Release": {"port_name": "gtk", "is_debug": False},
    "GTK Linux 64-bit Release WK2 (Tests)": {"port_name": "gtk-wk2", "is_debug": False},

    # FIXME: Remove rebaseline_override_dir once there are Qt bots for all the platform/qt-* directories.
    "Qt Linux Release": {"port_name": "qt-linux", "is_debug": False, "rebaseline_override_dir": "qt"},

    "EFL Linux 64-bit Release": {"port_name": "efl", "is_debug": False},
    "EFL Linux 64-bit Release WK2": {"port_name": "efl-wk2", "is_debug": False},
    "EFL Linux 64-bit Debug WK2": {"port_name": "efl-wk2", "is_debug": True},
}


_fuzzy_matches = {
    # These builders are on build.webkit.org.
    r"SnowLeopard": "mac-snowleopard",
    r"Apple Lion": "mac-lion",
    r"Windows": "win",
    r"GTK": "gtk",
    r"Qt": "qt",
}


_ports_without_builders = [
    "qt-mac",
    "qt-win",
    "qt-wk2",
]


def builder_path_from_name(builder_name):
    return re.sub(r'[\s().]', '_', builder_name)


def all_builder_names():
    return sorted(set(_exact_matches.keys()))


def all_port_names():
    return sorted(set(map(lambda x: x["port_name"], _exact_matches.values()) + _ports_without_builders))


def rebaseline_override_dir(builder_name):
    return _exact_matches[builder_name].get("rebaseline_override_dir", None)


def move_overwritten_baselines_to(builder_name):
    return _exact_matches[builder_name].get("move_overwritten_baselines_to", [])


def port_name_for_builder_name(builder_name):
    if builder_name in _exact_matches:
        return _exact_matches[builder_name]["port_name"]

    for regexp, port_name in _fuzzy_matches.items():
        if re.match(regexp, builder_name):
            return port_name


def builder_name_for_port_name(target_port_name):
    debug_builder_name = None
    for builder_name, builder_info in _exact_matches.items():
        if builder_info['port_name'] == target_port_name:
            if builder_info['is_debug']:
                debug_builder_name = builder_name
            else:
                return builder_name
    return debug_builder_name


def builder_path_for_port_name(port_name):
    builder_path_from_name(builder_name_for_port_name(port_name))
