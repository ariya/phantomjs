#!/bin/sh

PHANTOMJS_EXEC="phantomjs"

if which $PHANTOMJS_EXEC >/dev/null; then
   # Launch PhantomJS passing all the parameters from the CLI
   $PHANTOMJS_EXEC `dirname $0`/browsermode.js $@
else
   # Usage
   echo "'$PHANTOMJS_EXEC' must be in the \$PATH"
fi
