#!/usr/bin/env bash

LINENOISE_PATH="$PWD/../src/linenoise"
LINENOISE_SRC_PATH="$LINENOISE_PATH/src"
GITHUB_CLONE_URL="http://github.com/tadmarshall/linenoise.git"
TO_REMOVE=".gitignore .git *.vcproj *.sln Makefile"


# Make a new Linenoise source directory
rm -rf $LINENOISE_SRC_PATH
mkdir -p $LINENOISE_SRC_PATH

# Cloning latest 'master' of Linenoise
git clone $GITHUB_CLONE_URL $LINENOISE_SRC_PATH

# From within the source directory...
pushd $LINENOISE_SRC_PATH

# Extract latest commit log info and prepare "README.md" content
LATEST_COMMIT=$(git log -1)
README_CONTENT=$(cat << EOF
This project contains the **Linenoise project**, initially released
by [Salvatore Sanfilippo](https://github.com/antirez). Here we import a fork
by [Tad Marshall](https://github.com/tadmarshall) that lives at
[github.com/tadmarshall/linenoise](https://github.com/tadmarshall/linenoise).

The version of Linenoise included in PhantomJS refers to the commit:
-----
$LATEST_COMMIT
-----

Some files not needed for PhantomJS are removed.

Linenoise is licensed under the BSD-license.
Kudos to all the developers that contribute to this nice little pearl.

EOF)

# Remove unnecessary files
rm -rf $TO_REMOVE

popd # ... and out!

# Save "README.md"
echo "$README_CONTENT" > "$LINENOISE_PATH/README.md"
