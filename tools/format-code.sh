#!/usr/bin/env bash

cwd=$(pwd)
clang-format-6.0 -i --style=WebKit $cwd/src/*.h $cwd/src/*.cpp
