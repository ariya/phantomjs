#!/bin/sh

export SRCROOT=$PWD
export WebCore=$PWD
export CREATE_HASH_TABLE="$SRCROOT/create_hash_table"
export CREATE_REGEXP_TABLES="$SRCROOT/create_regex_tables"
export CREATE_KEYWORD_LOOKUP="$SRCROOT/KeywordLookupGenerator.py"

mkdir -p DerivedSources/JavaScriptCore
cd DerivedSources/JavaScriptCore

make -f ../../DerivedSources.make JavaScriptCore=../.. BUILT_PRODUCTS_DIR=../..
cd ../..
