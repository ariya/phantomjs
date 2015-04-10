#!/bin/sh
# Copyright (c) 2012 Google Inc. All rights reserved.
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

if [[ $# -lt 3 ]]; then
    echo "Usage: start-queue-loop.sh QUEUE_NAME BOT_ID RESET_AFTER_ITERATION [QUEUE_PARAMS]"
    exit 1
fi

QUEUE_NAME=$1
BOT_ID=$2
RESET_AFTER_ITERATION=$3
shift 3
QUEUE_PARAMS="$@"

while :
do
    # Delete log files older than 14 days, move aside the main mac-ews.log file to prevent it from growing extra large.
    cd /Volumes/Data/EWS/$QUEUE_NAME-logs
    find . -mtime +14 -delete
    rm $QUEUE_NAME.old
    mv $QUEUE_NAME.log $QUEUE_NAME.old
    cd /Volumes/Data/EWS/Webkit
    
    # Delete WebKitBuild to force a clean build
    rm -rf /Volumes/Data/EWS/WebKit/WebKitBuild
    
    # This somewhat quirky sequence of steps seems to clear up all the broken
    # git situations we've gotten ourself into in the past.
    git clean -f # Remove any left-over layout test results, added files, etc.
    git rebase --abort # If we got killed during a git rebase, we need to clean up.
    git fetch origin # Avoid updating the working copy to a stale revision.
    git checkout origin/master -f
    git branch -D master
    git checkout origin/master -b master

    # Most queues auto-update as part of their normal operation, but updating
    # here makes sure that we get the latest version of the master process.
    ./Tools/Scripts/update-webkit

    # test-webkitpy has code to remove orphaned .pyc files, so we
    # run it before running webkit-patch to avoid stale .pyc files
    # preventing webkit-patch from launching.
    ./Tools/Scripts/test-webkitpy

    # We use --exit-after-iteration to pick up any changes to webkit-patch, including
    # changes to the contributors.json file.
    ./Tools/Scripts/webkit-patch $QUEUE_NAME --bot-id=$BOT_ID --no-confirm --exit-after-iteration $RESET_AFTER_ITERATION $QUEUE_PARAMS
done
