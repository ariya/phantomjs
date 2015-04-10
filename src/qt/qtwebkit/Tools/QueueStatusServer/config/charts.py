# Copyright (C) 2013 Google Inc. All rights reserved.
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

patch_log_limit = 500

# All units are represented numerically as seconds.
one_minute = 60.0
one_hour = one_minute * 60.0
one_day = one_hour * 24.0
one_month = one_day * 30.0

# How far back to view the history, specified in seconds.
view_range_choices = [
    {"name": "1 day", "view_range": one_day},
    {"name": "1 week", "view_range": one_day * 7},
    {"name": "1 month", "view_range": one_month},
]

default_view_range = one_day

_time_units = [
    #(threshold, time unit, name)
    (0, one_hour, "hours"),
    (4 * one_day, one_day, "days"),
    (3 * one_month, one_month, "months"),
]


def get_time_unit(view_range):
    current_threshold, current_time_unit, current_name = _time_units[0]
    for threshold, time_unit, name in _time_units[1:]:
        if view_range >= threshold:
            current_time_unit, current_name = time_unit, name
        else:
            break
    return current_time_unit, current_name
