#!/usr/bin/bash

# Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 


# Trim any trailing \r or \n from the given variable.
chomp()
{
    local old_value=$(eval echo "\$$1");
    local value=$(echo "$old_value" | sed 's/[\r\n]*$//')
    eval $1=\$value;
}

if [[ -n "$WEBKITLIBRARIESDIR" ]]; then
    FALLBACK_VERSION_PATH=`cygpath -u "$WEBKITLIBRARIESDIR\\tools\\scripts\\VERSION"`
    FALLBACK_VERSION=$(cat "$FALLBACK_VERSION_PATH");

    COPYRIGHT_END_YEAR_PATH=`cygpath -u "$WEBKITLIBRARIESDIR\\tools\\scripts\\COPYRIGHT-END-YEAR"`
    COPYRIGHT_END_YEAR=$(cat "$COPYRIGHT_END_YEAR_PATH");
    chomp COPYRIGHT_END_YEAR
fi

OUTPUT_FILE=$(cygpath -u "$1")/include/autoversion.h
mkdir -p "$(dirname "$OUTPUT_FILE")"

# Take the initial version number from RC_PROJECTSOURCEVERSION if it
# exists, otherwise fall back to the version number stored in the source.
ENVIRONMENT_VERSION="$RC_PROJECTSOURCEVERSION";
PROPOSED_VERSION=${ENVIRONMENT_VERSION:-$FALLBACK_VERSION}
chomp PROPOSED_VERSION

# Split out the three components of the dotted version number.  We pad
# the input with trailing dots to handle the case where the input version
# has fewer components than we expect.
BUILD_MAJOR_VERSION=$(echo "$PROPOSED_VERSION.." | cut -d '.' -f 1)
BUILD_MINOR_VERSION=$(echo "$PROPOSED_VERSION.." | cut -d '.' -f 2)
BUILD_TINY_VERSION=$(echo "$PROPOSED_VERSION.." | cut -d '.' -f 3)

# Cut the major component down to three characters by dropping any
# extra leading digits, then adjust the major version portion of the
# version string to match.
CHARACTERS_TO_DROP=$(( ${#BUILD_MAJOR_VERSION} > 3 ? ${#BUILD_MAJOR_VERSION} - 3 : 0 ))
BUILD_MAJOR_VERSION=${BUILD_MAJOR_VERSION:$CHARACTERS_TO_DROP}
PROPOSED_VERSION=${PROPOSED_VERSION:$CHARACTERS_TO_DROP}

# Have the minor and tiny components default to zero if not present.
BUILD_MINOR_VERSION=${BUILD_MINOR_VERSION:-0}
BUILD_TINY_VERSION=${BUILD_TINY_VERSION:-0}

# Split the first component further by using the first digit for the
# major version and the remaining two characters as the minor version.
# The minor version is shifted down to the tiny version, with the tiny
# version becoming the variant version.
MAJOR_VERSION=${BUILD_MAJOR_VERSION:0:1}
MINOR_VERSION=${BUILD_MAJOR_VERSION:1}
TINY_VERSION=${BUILD_MINOR_VERSION}
VARIANT_VERSION=${BUILD_TINY_VERSION}

VERSION_TEXT=${PROPOSED_VERSION}
VERSION_TEXT_SHORT=${VERSION_TEXT}

if [ -z ${ENVIRONMENT_VERSION} ]; then
    # If we didn't pull the version number from the environment then we're doing
    # an engineering build and we'll stamp the build with some more information.

    BUILD_DATE=$(date)
    SVN_REVISION=$(svn info | grep '^Revision' | sed 's/^Revision: //')

    chomp BUILD_DATE
    chomp SVN_REVISION

    VERSION_TEXT_SHORT="${VERSION_TEXT_SHORT}+"
    VERSION_TEXT="${VERSION_TEXT_SHORT} ${USER} - ${BUILD_DATE} - r${SVN_REVISION}"
fi

cat > "$OUTPUT_FILE" <<EOF
#define __VERSION_TEXT__ "${VERSION_TEXT}"
#define __BUILD_NUMBER__ "${VERSION_TEXT}"
#define __BUILD_NUMBER_SHORT__ "${VERSION_TEXT_SHORT}"
#define __VERSION_MAJOR__ ${MAJOR_VERSION}
#define __VERSION_MINOR__ ${MINOR_VERSION}
#define __VERSION_TINY__ ${TINY_VERSION}
#define __VERSION_BUILD__ ${VARIANT_VERSION}
#define __BUILD_NUMBER_MAJOR__ ${BUILD_MAJOR_VERSION}
#define __BUILD_NUMBER_MINOR__ ${BUILD_MINOR_VERSION}
#define __BUILD_NUMBER_VARIANT__ ${BUILD_TINY_VERSION}
#define __SVN_REVISION__ ${SVN_REVISION}
EOF

if [[ -n "${COPYRIGHT_END_YEAR}" ]]; then
cat >> "$OUTPUT_FILE" <<EOF
#define __COPYRIGHT_YEAR_END_TEXT__ "${COPYRIGHT_END_YEAR}"
EOF
fi
