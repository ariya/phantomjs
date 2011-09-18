#!/bin/bash

#
# This script will convert each qrc into a
# Python resource file(s) which can be used
#

# change to script directory
cd `dirname "$0"`

# convert each filename.qrc to respective filename.py
for f in ../*.qrc; do
    pyrcc4 -compress 2 -o "${f%.*}.py" "$f"
done
