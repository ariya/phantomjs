#!/bin/sh
# Copyright (c) 2013 Google Inc. All rights reserved.
# Copyright (C) 2013 Apple Inc. All rights reserved.
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

CWD="$(pwd)"
cd "$(dirname "$0")"

QUEUE_TYPE=commit-queue
BUGZILLA_USERNAME=commit-queue@webkit.org
read -s -p "Bugzilla Password: " BUGZILLA_PASSWORD && echo
SVN_USERNAME=commit-queue@webkit.org
read -s -p "Subversion Password: " SVN_PASSWORD && echo

svn checkout http://svn.webkit.org/repository/webkit/trunk/Tools/EWSTools tools
bash tools/configure-svn-config.sh
bash tools/configure-svn-auth.sh $SVN_USERNAME $SVN_PASSWORD

echo "Cloning WebKit git repository, process takes ~30m."
echo "Note: No status output will be shown via remote pipe."
git clone git://git.webkit.org/WebKit.git WebKit
cd WebKit

cat >> .git/config <<EOF
[bugzilla]
	username = $BUGZILLA_USERNAME
	password = $BUGZILLA_PASSWORD
[svn-remote "svn"]
	url = http://svn.webkit.org/repository/webkit
	fetch = trunk:refs/remotes/origin/master
[user]
	email = commit-queue@webkit.org
	name = Commit Queue
EOF
