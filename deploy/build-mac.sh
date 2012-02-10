#!/bin/bash
# vim:expandtab:shiftwidth=4:softtabstop=4

source `dirname $0`/build-helper.sh

QT_CFG+=' -arch x86'
QT_CFG+=' -cocoa'               # Cocoa only, ignore Carbon
QT_CFG+=' -no-dwarf2'

if [ $QT_HEADLESS ] ; then
    echo "Building in headless mode on OS X not currently supported. Disabling requested headless build."
    DISABLE_HEADLESS=1
fi

BUILD_ALL_THE_THINGS
