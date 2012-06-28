#!/usr/bin/env bash

SOURCE="$0"
while [ -h "$SOURCE" ] ; do SOURCE="$(readlink "$SOURCE")"; done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

COFFEESCRIPT_PATH="$DIR/../src/coffee-script"
TAG_OR_BRANCH="1.3.3"
DOWNLOAD_URL="https://github.com/jashkenas/coffee-script/tarball/$TAG_OR_BRANCH"

# Download CoffeeScript and untar it
mkdir $COFFEESCRIPT_PATH
wget -O coffee-script.tar.gz $DOWNLOAD_URL
tar -xzf coffee-script.tar.gz --strip-components=1 -C $COFFEESCRIPT_PATH
rm -r "$COFFEESCRIPT_PATH/bin"
rm -r "$COFFEESCRIPT_PATH/documentation"
rm -r "$COFFEESCRIPT_PATH/examples"
rm "$COFFEESCRIPT_PATH/extras/jsl.conf"
rm -r "$COFFEESCRIPT_PATH/src"
rm -r "$COFFEESCRIPT_PATH/test"
rm "$COFFEESCRIPT_PATH/Cakefile"
rm "$COFFEESCRIPT_PATH/Rakefile"
rm "$COFFEESCRIPT_PATH/CNAME"
rm "$COFFEESCRIPT_PATH/index.html"
rm coffee-script.tar.gz

