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

if [[ $# -ne 1 ]];then
echo "Usage: build-feeder-style-sheriffbot.sh BOT_NUMBER"
exit 1
fi

CWD="$(pwd)"
cd "$(dirname "$0")"

BOT_ID=gce-feeder-$1
BUGZILLA_USERNAME=webkit.review.bot@gmail.com
read -s -p "Bugzilla Password: " BUGZILLA_PASSWORD && echo
read -s -p "sheriffbot IRC Password: " IRC_PASSWORD && echo

PROJECT=google.com:webkit
ZONE=$(bash findzone.sh $PROJECT)
IMAGE=projects/google/global/images/gcel-10-04-v20130104
MACHINE_TYPE=n1-standard-4-d

gcutil --project=$PROJECT addinstance $BOT_ID --machine_type=$MACHINE_TYPE --image=$IMAGE --zone=$ZONE --wait_until_running

echo "Sleeping for 30s to let the server spin up ssh..."
sleep 30

gcutil --project=$PROJECT ssh $BOT_ID "
    sudo apt-get install subversion -y &&
    svn checkout http://svn.webkit.org/repository/webkit/trunk/Tools/EWSTools tools &&
    cd tools &&
    bash build-vm.sh &&
    bash build-repo.sh feeder-queue $BUGZILLA_USERNAME $BUGZILLA_PASSWORD &&
    cp -r /mnt/git/webkit-feeder-queue /mnt/git/webkit-style-queue &&
    cp -r /mnt/git/webkit-feeder-queue /mnt/git/webkit-sheriff-bot &&
    bash configure-svn-config.sh &&
    bash configure-git-svn.sh sheriff-bot &&
    bash configure-git-user.sh sheriff-bot \"Sheriff Bot\" $BUGZILLA_USERNAME &&
    bash build-boot-cmd.sh \"\\
screen -t fq ./start-queue.sh feeder-queue $BOT_ID 10
screen -t sq ./start-queue.sh style-queue $BOT_ID 10
screen -t sb ./start-queue.sh -p \\\"--irc-password=$IRC_PASSWORD\\\" sheriff-bot $BOT_ID 180\" &&
    bash boot.sh
"

cd "$CWD"
