#!/bin/sh
export WEBKIT_BUGZILLA_USERNAME=$1
export WEBKIT_BUGZILLA_PASSWORD=$2
export BOT_ID=$3

function error_handler()
{
    start_ews
}

function start_ews()
{
    trap 'error_handler ${LINENO} $?' ERR

    while :
    do
        date
        echo "TASK: cleaning up old log files."
        cd ~/win-ews-logs
        rm -rf win-ews.old
        mv win-ews.log win-ews.old
        find ~/win-ews-logs -mtime +7 -exec rm -f {} \;
        echo "TASK: Starting up"
        cd ~/WebKit
        echo "TASK: Cleaning WebKitBuild"
        rm -rf WebKitBuild
        date
        echo "TASK: Cleaning up"
        svn cleanup
        date
        echo "TASK: svn revert -R *"
        svn revert -R *
        date
        echo "TASK: svn status"
        svn status | grep "?" | awk '{print $2}' | xargs rm -rf
        date
        echo "TASK: webkit-patch clean"
        ~/WebKit/Tools/Scripts/webkit-patch clean
        date
        echo "TASK: svn up --non-interactive"
        svn up --non-interactive
        date
        echo "TASK: kill orphaned tasks"
        taskkill.exe /f /im cl.exe
        echo "TASK: webkit-patch win-ews"
        ~/WebKit/Tools/Scripts/webkit-patch win-ews --bot-id=$BOT_ID --no-confirm --exit-after-iteration 10
        echo "TASK: kill old processes"
        ~/WebKit/Tools/BuildSlaveSupport/kill-old-processes
    done
}

start_ews