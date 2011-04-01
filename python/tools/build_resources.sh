#!/bin/bash

#
# This script will convert each qrc into a 
# Python resource file(s) which can be used
#
# NOTE: This script MUST be called from the same
#       directory containing the qrc's!
#

# convert each filename.qrc to respective filename.py
for f in *.qrc; do
    pyrcc4 -compress 2 -o ${f%.*}.py $f
done
