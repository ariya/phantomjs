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

"""An implementation of buildbot.status.web.auth.IAuth for authenticating WebKit committers"""

import ConfigParser
import buildbot.status.web.auth
import json
import zope.interface

from htdigestparser import HTDigestParser


class Error(Exception):
    pass


class CommitterAuth(buildbot.status.web.auth.AuthBase):
    zope.interface.implements(buildbot.status.web.auth.IAuth)

    def __init__(self, auth_json_filename):
        self._auth_json_filename = auth_json_filename

    def auth_json(self):
        try:
            with self.open_auth_json_file() as f:
                return json.load(f)
        except IOError, e:
            raise Error('Error opening auth.json file: {0}'.format(e.strerror))
        except ValueError, e:
            raise Error('Error parsing auth.json file: {0}'.format(e.message))

    def auth_json_filename(self):
        return self._auth_json_filename

    def authenticate(self, username, password):
        try:
            return self.is_webkit_committer(username) and self.is_webkit_trac_user(username, password)
        except Error, e:
            self.err = e.message
            return False

    def is_webkit_committer(self, username):
        try:
            if username not in self.webkit_committers():
                self.err = 'Invalid username/password'
                return False
            return True
        except ConfigParser.Error:
            raise Error('Error parsing WebKit committers file')
        except IOError, e:
            raise Error('Error opening WebKit committers file: {0}'.format(e.strerror))

    def is_webkit_trac_user(self, username, password):
        try:
            with self.open_trac_credentials_file() as f:
                htdigest = HTDigestParser(f)
        except IOError, e:
            raise Error('Error opening Trac credentials file: {0}'.format(e.strerror))

        if not htdigest.entries():
            raise Error('Error parsing Trac credentials file')

        if not htdigest.authenticate(username, 'Mac OS Forge', password):
            self.err = 'Invalid username/password'
            return False

        return True

    # These three methods exist for ease of testing.
    def open_auth_json_file(self):
        return open(self.auth_json_filename())

    def open_trac_credentials_file(self):
        return open(self.trac_credentials_filename())

    def open_webkit_committers_file(self):
        return open(self.webkit_committers_filename())

    def trac_credentials_filename(self):
        try:
            return self.auth_json()['trac_credentials']
        except KeyError:
            raise Error('auth.json file is missing "trac_credentials" key')

    def webkit_committers(self):
        config = ConfigParser.RawConfigParser()
        with self.open_webkit_committers_file() as f:
            config.readfp(f)
        return config.get('groups', 'webkit').split(',')

    def webkit_committers_filename(self):
        try:
            return self.auth_json()['webkit_committers']
        except KeyError:
            raise Error('auth.json file is missing "webkit_committers" key')
