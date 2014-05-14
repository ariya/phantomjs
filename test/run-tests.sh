#!/bin/sh
exec "${0%/*}/../bin/phantomjs" "${0%/*}/run-tests.js" "$@"
