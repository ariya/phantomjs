#!/bin/sh

mkdir -p include/JavaScriptCore
find ../Source/JavaScriptCore/ -name \*.h -exec cp -f -p {} include/JavaScriptCore/. \;
rm include/JavaScriptCore/config.h
rm include/JavaScriptCore/JavaScriptCorePrefix.h
