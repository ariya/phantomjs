#!/bin/sh

if [ -f $1 ]; then
    $1 || exit $?;
fi
