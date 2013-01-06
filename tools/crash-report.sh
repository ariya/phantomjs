#!/usr/bin/env bash

`dirname $0`/../src/breakpad/src/processor/minidump_stackwalk $1 `dirname $0`/../symbols
