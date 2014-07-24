# Copyright (C) 2011 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""htdigestparser - a parser for htdigest files"""

import hashlib
import string


class HTDigestParser(object):
    def __init__(self, digest_file):
        self._entries = self.parse_file(digest_file)

    def authenticate(self, username, realm, password):
        hashed_password = hashlib.md5(':'.join((username, realm, password))).hexdigest()
        return [username, realm, hashed_password] in self.entries()

    def entries(self):
        return self._entries

    def parse_file(self, digest_file):
        entries = [line.rstrip().split(':') for line in digest_file]

        # Perform some sanity-checking to ensure the file is valid.
        valid_characters = set(string.hexdigits)
        for entry in entries:
            if len(entry) != 3:
                return []
            hashed_password = entry[-1]
            if len(hashed_password) != 32:
                return []
            if not set(hashed_password).issubset(valid_characters):
                return []

        return entries
