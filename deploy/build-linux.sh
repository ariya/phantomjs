#!/bin/bash
# vim:expandtab:shiftwidth=4:softtabstop=4

source `dirname $0`/build-helper.sh

QT_CFG+=' -no-xinput'

# Static build with Qt 4.8 isn't working yet
if [ $QT_VERSION = 4.8.0 ] ; then
    STATIC=false
fi

if [ ! $QT_HEADLESS ]; then
    QT_CFG+=' -xrender'
fi

BUILD_ALL_THE_THINGS
