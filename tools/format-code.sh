#!/usr/bin/env bash

cwd=$(pwd)
clang-format -i --style=WebKit $cwd/src/*.h $cwd/src/*.cpp
