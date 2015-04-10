#!/bin/sh
# Copyright (c) 2013 Google Inc. All rights reserved.
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

if [[ $# -ne 0 ]];then
echo "Usage: build-vm.sh"
exit 1
fi

CWD=$(pwd)

# Format the disk
cat <<EOF | sudo fdisk /dev/sdb
n
p
1
8

w
EOF

sudo mkfs.ext4 /dev/sdb1
sudo mount /dev/sdb1 /mnt

echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | sudo debconf-set-selections

curl http://src.chromium.org/svn/trunk/src/build/install-build-deps.sh > install-build-deps.sh
bash install-build-deps.sh --no-prompt
sudo apt-get install xvfb screen git-svn zip -y

# install-build-deps.sh will install flashplugin-installer, which causes some plug-in tests to crash.
sudo apt-get remove flashplugin-installer -y

cd /mnt
sudo mkdir -p git
sudo chown $USER git
sudo chgrp $USER git

sudo chmod 644 /etc/hosts

cd $CWD
