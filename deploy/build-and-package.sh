#!/usr/bin/env bash

cd `dirname $0`/..

./build.py --confirm --release --git-clean-qtbase --git-clean-qtwebkit "$@" || exit 1

