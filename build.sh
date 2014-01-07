#!/usr/bin/env bash

set -e

QT_CFG=''

BUILD_CONFIRM=0
COMPILE_JOBS=1
MAKEFLAGS_JOBS=''

if [[ "$MAKEFLAGS" != "" ]]; then
  MAKEFLAGS_JOBS=$(echo $MAKEFLAGS | egrep -o '\-j[0-9]+' | egrep -o '[0-9]+')
fi

if [[ "$MAKEFLAGS_JOBS" != "" ]]; then
  # user defined number of jobs in MAKEFLAGS, re-use that number
  COMPILE_JOBS=$MAKEFLAGS_JOBS
elif [[ $OSTYPE = darwin* ]]; then
   # We only support modern Mac machines, they are at least using
   # hyperthreaded dual-core CPU.
   COMPILE_JOBS=4
elif [[ $OSTYPE == freebsd* ]] || [[ $OSTYPE == openbsd* ]]; then
   COMPILE_JOBS=`sysctl -n hw.ncpu`
else
   CPU_CORES=`grep -c ^processor /proc/cpuinfo`
   if [[ "$CPU_CORES" -gt 1 ]]; then
       COMPILE_JOBS=$CPU_CORES
   fi
fi

if [[ "$COMPILE_JOBS" -gt 8 ]]; then
   # Safety net.
   COMPILE_JOBS=8
fi

SILENT=''

until [ -z "$1" ]; do
    case $1 in
        "--qt-config")
            shift
            QT_CFG=" $1"
            shift;;
        "--qmake-args")
            shift
            QMAKE_ARGS=$1
            shift;;
        "--jobs")
            shift
            COMPILE_JOBS=$1
            shift;;
        "--confirm")
            BUILD_CONFIRM=1
            shift;;
        "--silent")
            SILENT='--silent'
            QT_CFG+=" -silent"
            shift;;
        "--help")
            echo "Usage: $0 [--qt-config CONFIG] [--jobs NUM]"
            echo
            echo "  --confirm                   Silently confirm the build."
            echo "  --qt-config CONFIG          Specify extra config options to be used when configuring Qt"
            echo "  --jobs NUM                  How many parallel compile jobs to use. Defaults to 4."
            echo "  --silent                    Produce less verbose output."
            echo
            exit 0
            ;;
        *)
            echo "Unrecognised option: $1"
            exit 1;;
    esac
done


if [[ "$BUILD_CONFIRM" -eq 0 ]]; then
cat << EOF
----------------------------------------
               WARNING
----------------------------------------

Building PhantomJS from source takes a very long time, anywhere from 30 minutes
to several hours (depending on the machine configuration). It is recommended to
use the premade binary packages on supported operating systems.

For details, please go the the web site: http://phantomjs.org/download.html.

EOF

    echo "Do you want to continue (y/n)?"
    read continue
    if [[ "$continue" != "y" ]]; then
        exit 1
    fi
    echo
    echo
fi

echo
echo "Building PhantomJS. Please wait..."
echo

UNAME_SYSTEM=`(uname -s) 2>/dev/null`  || UNAME_SYSTEM=unknown
UNAME_RELEASE=`(uname -r) 2>/dev/null` || UNAME_RELEASE=unknown
UNAME_MACHINE=`(uname -m) 2>/dev/null` || UNAME_MACHINE=unknown

echo "System architecture... ($UNAME_SYSTEM $UNAME_RELEASE $UNAME_MACHINE)"
echo

cd src/qt && ./preconfig.sh --jobs $COMPILE_JOBS --qt-config "$QT_CFG" $SILENT && cd ../..

echo "Building main PhantomJS application. Please wait..."
echo
src/qt/bin/qmake $QMAKE_ARGS
make -j$COMPILE_JOBS
