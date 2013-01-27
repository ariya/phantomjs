#!/usr/bin/env bash

svn export http://google-breakpad.googlecode.com/svn/trunk/ src/breakpad

cd src/breakpad/src

rm -r testing
rm -r processor/testdata
rm -r third_party/protobuf/protobuf/java
rm -r third_party/protobuf/protobuf/gtest

cd ..

ln -s ../.gitignore-breakpad .gitignore
