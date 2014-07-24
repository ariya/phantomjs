# Copyright (c) 2011 Google Inc. All rights reserved.
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

from array import array


def edit_distance(str1, str2):
    unsignedShort = 'h'
    distances = [array(unsignedShort, (0,) * (len(str2) + 1)) for i in range(0, len(str1) + 1)]
    # distances[0][0] = 0 since distance between str1[:0] and str2[:0] is 0
    for i in range(1, len(str1) + 1):
        distances[i][0] = i  # Distance between str1[:i] and str2[:0] is i

    for j in range(1, len(str2) + 1):
        distances[0][j] = j  # Distance between str1[:0] and str2[:j] is j

    for i in range(0, len(str1)):
        for j in range(0, len(str2)):
            diff = 0 if str1[i] == str2[j] else 1
            # Deletion, Insertion, Identical / Replacement
            distances[i + 1][j + 1] = min(distances[i + 1][j] + 1, distances[i][j + 1] + 1, distances[i][j] + diff)
    return distances[len(str1)][len(str2)]
